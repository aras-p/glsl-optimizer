/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /**
  * @file
  * 
  * Wrap the cso cache & hash mechanisms in a simplified
  * pipe-driver-specific interface.
  *
  * @author Zack Rusin <zack@tungstengraphics.com>
  * @author Keith Whitwell <keith@tungstengraphics.com>
  */

#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"

#include "cso_cache/cso_context.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"
#include "cso_context.h"

struct cso_context {
   struct pipe_context *pipe;
   struct cso_cache *cache;

   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
      unsigned nr_samplers;

      void *vertex_samplers[PIPE_MAX_VERTEX_SAMPLERS];
      unsigned nr_vertex_samplers;
   } hw;

   void *samplers[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers;

   void *vertex_samplers[PIPE_MAX_VERTEX_SAMPLERS];
   unsigned nr_vertex_samplers;

   unsigned nr_samplers_saved;
   void *samplers_saved[PIPE_MAX_SAMPLERS];

   unsigned nr_vertex_samplers_saved;
   void *vertex_samplers_saved[PIPE_MAX_VERTEX_SAMPLERS];

   struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
   uint nr_textures;

   struct pipe_texture *vertex_textures[PIPE_MAX_VERTEX_SAMPLERS];
   uint nr_vertex_textures;

   uint nr_textures_saved;
   struct pipe_texture *textures_saved[PIPE_MAX_SAMPLERS];

   uint nr_vertex_textures_saved;
   struct pipe_texture *vertex_textures_saved[PIPE_MAX_SAMPLERS];

   /** Current and saved state.
    * The saved state is used as a 1-deep stack.
    */
   void *blend, *blend_saved;
   void *depth_stencil, *depth_stencil_saved;
   void *rasterizer, *rasterizer_saved;
   void *fragment_shader, *fragment_shader_saved, *geometry_shader;
   void *vertex_shader, *vertex_shader_saved, *geometry_shader_saved;

   struct pipe_clip_state clip;
   struct pipe_clip_state clip_saved;

   struct pipe_framebuffer_state fb, fb_saved;
   struct pipe_viewport_state vp, vp_saved;
   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref, stencil_ref_saved;
};


static void
free_framebuffer_state(struct pipe_framebuffer_state *fb);


static boolean delete_blend_state(struct cso_context *ctx, void *state)
{
   struct cso_blend *cso = (struct cso_blend *)state;

   if (ctx->blend == cso->data)
      return FALSE;

   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_depth_stencil_state(struct cso_context *ctx, void *state)
{
   struct cso_depth_stencil_alpha *cso = (struct cso_depth_stencil_alpha *)state;

   if (ctx->depth_stencil == cso->data)
      return FALSE;

   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);

   return TRUE;
}

static boolean delete_sampler_state(struct cso_context *ctx, void *state)
{
   struct cso_sampler *cso = (struct cso_sampler *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_rasterizer_state(struct cso_context *ctx, void *state)
{
   struct cso_rasterizer *cso = (struct cso_rasterizer *)state;

   if (ctx->rasterizer == cso->data)
      return FALSE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_fs_state(struct cso_context *ctx, void *state)
{
   struct cso_fragment_shader *cso = (struct cso_fragment_shader *)state;
   if (ctx->fragment_shader == cso->data)
      return FALSE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_vs_state(struct cso_context *ctx, void *state)
{
   struct cso_vertex_shader *cso = (struct cso_vertex_shader *)state;
   if (ctx->vertex_shader == cso->data)
      return TRUE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return FALSE;
}


static INLINE boolean delete_cso(struct cso_context *ctx,
                                 void *state, enum cso_cache_type type)
{
   switch (type) {
   case CSO_BLEND:
      return delete_blend_state(ctx, state);
      break;
   case CSO_SAMPLER:
      return delete_sampler_state(ctx, state);
      break;
   case CSO_DEPTH_STENCIL_ALPHA:
      return delete_depth_stencil_state(ctx, state);
      break;
   case CSO_RASTERIZER:
      return delete_rasterizer_state(ctx, state);
      break;
   case CSO_FRAGMENT_SHADER:
      return delete_fs_state(ctx, state);
      break;
   case CSO_VERTEX_SHADER:
      return delete_vs_state(ctx, state);
      break;
   default:
      assert(0);
      FREE(state);
   }
   return FALSE;
}

static INLINE void sanitize_hash(struct cso_hash *hash, enum cso_cache_type type,
                                 int max_size, void *user_data)
{
   struct cso_context *ctx = (struct cso_context *)user_data;
   /* if we're approach the maximum size, remove fourth of the entries
    * otherwise every subsequent call will go through the same */
   int hash_size = cso_hash_size(hash);
   int max_entries = (max_size > hash_size) ? max_size : hash_size;
   int to_remove =  (max_size < max_entries) * max_entries/4;
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   if (hash_size > max_size)
      to_remove += hash_size - max_size;
   while (to_remove) {
      /*remove elements until we're good */
      /*fixme: currently we pick the nodes to remove at random*/
      void *cso = cso_hash_iter_data(iter);
      if (delete_cso(ctx, cso, type)) {
         iter = cso_hash_erase(hash, iter);
         --to_remove;
      } else
         iter = cso_hash_iter_next(iter);
   }
}


struct cso_context *cso_create_context( struct pipe_context *pipe )
{
   struct cso_context *ctx = CALLOC_STRUCT(cso_context);
   if (ctx == NULL)
      goto out;

   ctx->cache = cso_cache_create();
   if (ctx->cache == NULL)
      goto out;
   cso_cache_set_sanitize_callback(ctx->cache,
                                   sanitize_hash,
                                   ctx);

   ctx->pipe = pipe;

   /* Enable for testing: */
   if (0) cso_set_maximum_cache_size( ctx->cache, 4 );

   return ctx;

out:
   cso_destroy_context( ctx );      
   return NULL;
}


/**
 * Prior to context destruction, this function unbinds all state objects.
 */
void cso_release_all( struct cso_context *ctx )
{
   unsigned i;
   
   if (ctx->pipe) {
      ctx->pipe->bind_blend_state( ctx->pipe, NULL );
      ctx->pipe->bind_rasterizer_state( ctx->pipe, NULL );
      ctx->pipe->bind_fragment_sampler_states( ctx->pipe, 0, NULL );
      if (ctx->pipe->bind_vertex_sampler_states)
         ctx->pipe->bind_vertex_sampler_states(ctx->pipe, 0, NULL);
      ctx->pipe->bind_depth_stencil_alpha_state( ctx->pipe, NULL );
      ctx->pipe->bind_fs_state( ctx->pipe, NULL );
      ctx->pipe->bind_vs_state( ctx->pipe, NULL );
   }

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_texture_reference(&ctx->textures[i], NULL);
      pipe_texture_reference(&ctx->textures_saved[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      pipe_texture_reference(&ctx->vertex_textures[i], NULL);
      pipe_texture_reference(&ctx->vertex_textures_saved[i], NULL);
   }

   free_framebuffer_state(&ctx->fb);
   free_framebuffer_state(&ctx->fb_saved);

   if (ctx->cache) {
      cso_cache_delete( ctx->cache );
      ctx->cache = NULL;
   }
}


void cso_destroy_context( struct cso_context *ctx )
{
   if (ctx) {
      /*cso_release_all( ctx );*/
      FREE( ctx );
   }
}


/* Those function will either find the state of the given template
 * in the cache or they will create a new state from the given
 * template, insert it in the cache and return it.
 */

/*
 * If the driver returns 0 from the create method then they will assign
 * the data member of the cso to be the template itself.
 */

enum pipe_error cso_set_blend(struct cso_context *ctx,
                              const struct pipe_blend_state *templ)
{
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
   void *handle;

   key_size = templ->independent_blend_enable ? sizeof(struct pipe_blend_state) :
              (char *)&(templ->rt[1]) - (char *)templ;
   hash_key = cso_construct_key((void*)templ, key_size);
   iter = cso_find_state_template(ctx->cache, hash_key, CSO_BLEND, (void*)templ, key_size);

   if (cso_hash_iter_is_null(iter)) {
      struct cso_blend *cso = MALLOC(sizeof(struct cso_blend));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, templ, key_size);
      cso->data = ctx->pipe->create_blend_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_blend_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_BLEND, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_blend *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->blend != handle) {
      ctx->blend = handle;
      ctx->pipe->bind_blend_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_blend(struct cso_context *ctx)
{
   assert(!ctx->blend_saved);
   ctx->blend_saved = ctx->blend;
}

void cso_restore_blend(struct cso_context *ctx)
{
   if (ctx->blend != ctx->blend_saved) {
      ctx->blend = ctx->blend_saved;
      ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend_saved);
   }
   ctx->blend_saved = NULL;
}



enum pipe_error cso_single_sampler(struct cso_context *ctx,
                                   unsigned idx,
                                   const struct pipe_sampler_state *templ)
{
   void *handle = NULL;

   if (templ != NULL) {
      unsigned key_size = sizeof(struct pipe_sampler_state);
      unsigned hash_key = cso_construct_key((void*)templ, key_size);
      struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                          hash_key, CSO_SAMPLER,
                                                          (void*)templ, key_size);

      if (cso_hash_iter_is_null(iter)) {
         struct cso_sampler *cso = MALLOC(sizeof(struct cso_sampler));
         if (!cso)
            return PIPE_ERROR_OUT_OF_MEMORY;

         memcpy(&cso->state, templ, sizeof(*templ));
         cso->data = ctx->pipe->create_sampler_state(ctx->pipe, &cso->state);
         cso->delete_state = (cso_state_callback)ctx->pipe->delete_sampler_state;
         cso->context = ctx->pipe;

         iter = cso_insert_state(ctx->cache, hash_key, CSO_SAMPLER, cso);
         if (cso_hash_iter_is_null(iter)) {
            FREE(cso);
            return PIPE_ERROR_OUT_OF_MEMORY;
         }

         handle = cso->data;
      }
      else {
         handle = ((struct cso_sampler *)cso_hash_iter_data(iter))->data;
      }
   }

   ctx->samplers[idx] = handle;
   return PIPE_OK;
}

enum pipe_error
cso_single_vertex_sampler(struct cso_context *ctx,
                          unsigned idx,
                          const struct pipe_sampler_state *templ)
{
   void *handle = NULL;

   if (templ != NULL) {
      unsigned key_size = sizeof(struct pipe_sampler_state);
      unsigned hash_key = cso_construct_key((void*)templ, key_size);
      struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                          hash_key, CSO_SAMPLER,
                                                          (void*)templ, key_size);

      if (cso_hash_iter_is_null(iter)) {
         struct cso_sampler *cso = MALLOC(sizeof(struct cso_sampler));
         if (!cso)
            return PIPE_ERROR_OUT_OF_MEMORY;

         memcpy(&cso->state, templ, sizeof(*templ));
         cso->data = ctx->pipe->create_sampler_state(ctx->pipe, &cso->state);
         cso->delete_state = (cso_state_callback)ctx->pipe->delete_sampler_state;
         cso->context = ctx->pipe;

         iter = cso_insert_state(ctx->cache, hash_key, CSO_SAMPLER, cso);
         if (cso_hash_iter_is_null(iter)) {
            FREE(cso);
            return PIPE_ERROR_OUT_OF_MEMORY;
         }

         handle = cso->data;
      }
      else {
         handle = ((struct cso_sampler *)cso_hash_iter_data(iter))->data;
      }
   }

   ctx->vertex_samplers[idx] = handle;
   return PIPE_OK;
}

void cso_single_sampler_done( struct cso_context *ctx )
{
   unsigned i;

   /* find highest non-null sampler */
   for (i = PIPE_MAX_SAMPLERS; i > 0; i--) {
      if (ctx->samplers[i - 1] != NULL)
         break;
   }

   ctx->nr_samplers = i;

   if (ctx->hw.nr_samplers != ctx->nr_samplers ||
       memcmp(ctx->hw.samplers,
              ctx->samplers,
              ctx->nr_samplers * sizeof(void *)) != 0) 
   {
      memcpy(ctx->hw.samplers, ctx->samplers, ctx->nr_samplers * sizeof(void *));
      ctx->hw.nr_samplers = ctx->nr_samplers;

      ctx->pipe->bind_fragment_sampler_states(ctx->pipe, ctx->nr_samplers, ctx->samplers);
   }
}

void
cso_single_vertex_sampler_done(struct cso_context *ctx)
{
   unsigned i;

   /* find highest non-null sampler */
   for (i = PIPE_MAX_VERTEX_SAMPLERS; i > 0; i--) {
      if (ctx->vertex_samplers[i - 1] != NULL)
         break;
   }

   ctx->nr_vertex_samplers = i;

   if (ctx->hw.nr_vertex_samplers != ctx->nr_vertex_samplers ||
       memcmp(ctx->hw.vertex_samplers,
              ctx->vertex_samplers,
              ctx->nr_vertex_samplers * sizeof(void *)) != 0) 
   {
      memcpy(ctx->hw.vertex_samplers,
             ctx->vertex_samplers,
             ctx->nr_vertex_samplers * sizeof(void *));
      ctx->hw.nr_vertex_samplers = ctx->nr_vertex_samplers;

      ctx->pipe->bind_vertex_sampler_states(ctx->pipe,
                                            ctx->nr_vertex_samplers,
                                            ctx->vertex_samplers);
   }
}

/*
 * If the function encouters any errors it will return the
 * last one. Done to always try to set as many samplers
 * as possible.
 */
enum pipe_error cso_set_samplers( struct cso_context *ctx,
                                  unsigned nr,
                                  const struct pipe_sampler_state **templates )
{
   unsigned i;
   enum pipe_error temp, error = PIPE_OK;

   /* TODO: fastpath
    */

   for (i = 0; i < nr; i++) {
      temp = cso_single_sampler( ctx, i, templates[i] );
      if (temp != PIPE_OK)
         error = temp;
   }

   for ( ; i < ctx->nr_samplers; i++) {
      temp = cso_single_sampler( ctx, i, NULL );
      if (temp != PIPE_OK)
         error = temp;
   }

   cso_single_sampler_done( ctx );

   return error;
}

void cso_save_samplers(struct cso_context *ctx)
{
   ctx->nr_samplers_saved = ctx->nr_samplers;
   memcpy(ctx->samplers_saved, ctx->samplers, sizeof(ctx->samplers));
}

void cso_restore_samplers(struct cso_context *ctx)
{
   ctx->nr_samplers = ctx->nr_samplers_saved;
   memcpy(ctx->samplers, ctx->samplers_saved, sizeof(ctx->samplers));
   cso_single_sampler_done( ctx );
}

/*
 * If the function encouters any errors it will return the
 * last one. Done to always try to set as many samplers
 * as possible.
 */
enum pipe_error cso_set_vertex_samplers(struct cso_context *ctx,
                                        unsigned nr,
                                        const struct pipe_sampler_state **templates)
{
   unsigned i;
   enum pipe_error temp, error = PIPE_OK;

   /* TODO: fastpath
    */

   for (i = 0; i < nr; i++) {
      temp = cso_single_vertex_sampler( ctx, i, templates[i] );
      if (temp != PIPE_OK)
         error = temp;
   }

   for ( ; i < ctx->nr_samplers; i++) {
      temp = cso_single_vertex_sampler( ctx, i, NULL );
      if (temp != PIPE_OK)
         error = temp;
   }

   cso_single_vertex_sampler_done( ctx );

   return error;
}

void
cso_save_vertex_samplers(struct cso_context *ctx)
{
   ctx->nr_vertex_samplers_saved = ctx->nr_vertex_samplers;
   memcpy(ctx->vertex_samplers_saved, ctx->vertex_samplers, sizeof(ctx->vertex_samplers));
}

void
cso_restore_vertex_samplers(struct cso_context *ctx)
{
   ctx->nr_vertex_samplers = ctx->nr_vertex_samplers_saved;
   memcpy(ctx->vertex_samplers, ctx->vertex_samplers_saved, sizeof(ctx->vertex_samplers));
   cso_single_vertex_sampler_done(ctx);
}


enum pipe_error cso_set_sampler_textures( struct cso_context *ctx,
                                          uint count,
                                          struct pipe_texture **textures )
{
   uint i;

   ctx->nr_textures = count;

   for (i = 0; i < count; i++)
      pipe_texture_reference(&ctx->textures[i], textures[i]);
   for ( ; i < PIPE_MAX_SAMPLERS; i++)
      pipe_texture_reference(&ctx->textures[i], NULL);

   ctx->pipe->set_fragment_sampler_textures(ctx->pipe, count, textures);

   return PIPE_OK;
}

void cso_save_sampler_textures( struct cso_context *ctx )
{
   uint i;

   ctx->nr_textures_saved = ctx->nr_textures;
   for (i = 0; i < ctx->nr_textures; i++) {
      assert(!ctx->textures_saved[i]);
      pipe_texture_reference(&ctx->textures_saved[i], ctx->textures[i]);
   }
}

void cso_restore_sampler_textures( struct cso_context *ctx )
{
   uint i;

   ctx->nr_textures = ctx->nr_textures_saved;

   for (i = 0; i < ctx->nr_textures; i++) {
      pipe_texture_reference(&ctx->textures[i], NULL);
      ctx->textures[i] = ctx->textures_saved[i];
      ctx->textures_saved[i] = NULL;
   }
   for ( ; i < PIPE_MAX_SAMPLERS; i++)
      pipe_texture_reference(&ctx->textures[i], NULL);

   ctx->pipe->set_fragment_sampler_textures(ctx->pipe, ctx->nr_textures, ctx->textures);

   ctx->nr_textures_saved = 0;
}



enum pipe_error
cso_set_vertex_sampler_textures(struct cso_context *ctx,
                                uint count,
                                struct pipe_texture **textures)
{
   uint i;

   ctx->nr_vertex_textures = count;

   for (i = 0; i < count; i++) {
      pipe_texture_reference(&ctx->vertex_textures[i], textures[i]);
   }
   for ( ; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      pipe_texture_reference(&ctx->vertex_textures[i], NULL);
   }

   ctx->pipe->set_vertex_sampler_textures(ctx->pipe, count, textures);

   return PIPE_OK;
}

void
cso_save_vertex_sampler_textures(struct cso_context *ctx)
{
   uint i;

   ctx->nr_vertex_textures_saved = ctx->nr_vertex_textures;
   for (i = 0; i < ctx->nr_vertex_textures; i++) {
      assert(!ctx->vertex_textures_saved[i]);
      pipe_texture_reference(&ctx->vertex_textures_saved[i], ctx->vertex_textures[i]);
   }
}

void
cso_restore_vertex_sampler_textures(struct cso_context *ctx)
{
   uint i;

   ctx->nr_vertex_textures = ctx->nr_vertex_textures_saved;

   for (i = 0; i < ctx->nr_vertex_textures; i++) {
      pipe_texture_reference(&ctx->vertex_textures[i], NULL);
      ctx->vertex_textures[i] = ctx->vertex_textures_saved[i];
      ctx->vertex_textures_saved[i] = NULL;
   }
   for ( ; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      pipe_texture_reference(&ctx->vertex_textures[i], NULL);
   }

   ctx->pipe->set_vertex_sampler_textures(ctx->pipe,
                                          ctx->nr_vertex_textures,
                                          ctx->vertex_textures);

   ctx->nr_vertex_textures_saved = 0;
}



enum pipe_error cso_set_depth_stencil_alpha(struct cso_context *ctx,
                                            const struct pipe_depth_stencil_alpha_state *templ)
{
   unsigned key_size = sizeof(struct pipe_depth_stencil_alpha_state);
   unsigned hash_key = cso_construct_key((void*)templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, 
                                                       CSO_DEPTH_STENCIL_ALPHA,
                                                       (void*)templ, key_size);
   void *handle;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_depth_stencil_alpha *cso = MALLOC(sizeof(struct cso_depth_stencil_alpha));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_depth_stencil_alpha_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_DEPTH_STENCIL_ALPHA, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_depth_stencil_alpha *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->depth_stencil != handle) {
      ctx->depth_stencil = handle;
      ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_depth_stencil_alpha(struct cso_context *ctx)
{
   assert(!ctx->depth_stencil_saved);
   ctx->depth_stencil_saved = ctx->depth_stencil;
}

void cso_restore_depth_stencil_alpha(struct cso_context *ctx)
{
   if (ctx->depth_stencil != ctx->depth_stencil_saved) {
      ctx->depth_stencil = ctx->depth_stencil_saved;
      ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, ctx->depth_stencil_saved);
   }
   ctx->depth_stencil_saved = NULL;
}



enum pipe_error cso_set_rasterizer(struct cso_context *ctx,
                                   const struct pipe_rasterizer_state *templ)
{
   unsigned key_size = sizeof(struct pipe_rasterizer_state);
   unsigned hash_key = cso_construct_key((void*)templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_RASTERIZER,
                                                       (void*)templ, key_size);
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_rasterizer *cso = MALLOC(sizeof(struct cso_rasterizer));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_rasterizer_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_rasterizer_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_RASTERIZER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_rasterizer *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->rasterizer != handle) {
      ctx->rasterizer = handle;
      ctx->pipe->bind_rasterizer_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_rasterizer(struct cso_context *ctx)
{
   assert(!ctx->rasterizer_saved);
   ctx->rasterizer_saved = ctx->rasterizer;
}

void cso_restore_rasterizer(struct cso_context *ctx)
{
   if (ctx->rasterizer != ctx->rasterizer_saved) {
      ctx->rasterizer = ctx->rasterizer_saved;
      ctx->pipe->bind_rasterizer_state(ctx->pipe, ctx->rasterizer_saved);
   }
   ctx->rasterizer_saved = NULL;
}



enum pipe_error cso_set_fragment_shader_handle(struct cso_context *ctx,
                                               void *handle )
{
   if (ctx->fragment_shader != handle) {
      ctx->fragment_shader = handle;
      ctx->pipe->bind_fs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_fragment_shader(struct cso_context *ctx, void *handle )
{
   if (handle == ctx->fragment_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_fs_state(ctx->pipe, NULL);
      ctx->fragment_shader = NULL;
   }
   ctx->pipe->delete_fs_state(ctx->pipe, handle);
}

/* Not really working:
 */
#if 0
enum pipe_error cso_set_fragment_shader(struct cso_context *ctx,
                                        const struct pipe_shader_state *templ)
{
   const struct tgsi_token *tokens = templ->tokens;
   unsigned num_tokens = tgsi_num_tokens(tokens);
   size_t tokens_size = num_tokens*sizeof(struct tgsi_token);
   unsigned hash_key = cso_construct_key((void*)tokens, tokens_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, 
                                                       CSO_FRAGMENT_SHADER,
                                                       (void*)tokens,
                                                       sizeof(*templ)); /* XXX correct? tokens_size? */
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_fragment_shader *cso = MALLOC(sizeof(struct cso_fragment_shader) + tokens_size);
      struct tgsi_token *cso_tokens = (struct tgsi_token *)((char *)cso + sizeof(*cso));

      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(cso_tokens, tokens, tokens_size);
      cso->state.tokens = cso_tokens;
      cso->data = ctx->pipe->create_fs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_fs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_FRAGMENT_SHADER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_fragment_shader *)cso_hash_iter_data(iter))->data;
   }

   return cso_set_fragment_shader_handle( ctx, handle );
}
#endif

void cso_save_fragment_shader(struct cso_context *ctx)
{
   assert(!ctx->fragment_shader_saved);
   ctx->fragment_shader_saved = ctx->fragment_shader;
}

void cso_restore_fragment_shader(struct cso_context *ctx)
{
   if (ctx->fragment_shader_saved != ctx->fragment_shader) {
      ctx->pipe->bind_fs_state(ctx->pipe, ctx->fragment_shader_saved);
      ctx->fragment_shader = ctx->fragment_shader_saved;
   }
   ctx->fragment_shader_saved = NULL;
}


enum pipe_error cso_set_vertex_shader_handle(struct cso_context *ctx,
                                             void *handle )
{
   if (ctx->vertex_shader != handle) {
      ctx->vertex_shader = handle;
      ctx->pipe->bind_vs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_vertex_shader(struct cso_context *ctx, void *handle )
{
   if (handle == ctx->vertex_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_vs_state(ctx->pipe, NULL);
      ctx->vertex_shader = NULL;
   }
   ctx->pipe->delete_vs_state(ctx->pipe, handle);
}


/* Not really working:
 */
#if 0
enum pipe_error cso_set_vertex_shader(struct cso_context *ctx,
                                      const struct pipe_shader_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_shader_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_VERTEX_SHADER,
                                                       (void*)templ,
                                                       sizeof(*templ));
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_vertex_shader *cso = MALLOC(sizeof(struct cso_vertex_shader));

      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_vs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_vs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_VERTEX_SHADER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_vertex_shader *)cso_hash_iter_data(iter))->data;
   }

   return cso_set_vertex_shader_handle( ctx, handle );
}
#endif



void cso_save_vertex_shader(struct cso_context *ctx)
{
   assert(!ctx->vertex_shader_saved);
   ctx->vertex_shader_saved = ctx->vertex_shader;
}

void cso_restore_vertex_shader(struct cso_context *ctx)
{
   if (ctx->vertex_shader_saved != ctx->vertex_shader) {
      ctx->pipe->bind_vs_state(ctx->pipe, ctx->vertex_shader_saved);
      ctx->vertex_shader = ctx->vertex_shader_saved;
   }
   ctx->vertex_shader_saved = NULL;
}


/**
 * Copy framebuffer state from src to dst with refcounting of surfaces.
 */
static void
copy_framebuffer_state(struct pipe_framebuffer_state *dst,
                       const struct pipe_framebuffer_state *src)
{
   uint i;

   dst->width = src->width;
   dst->height = src->height;
   dst->nr_cbufs = src->nr_cbufs;
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&dst->cbufs[i], src->cbufs[i]);
   }
   pipe_surface_reference(&dst->zsbuf, src->zsbuf);
}


static void
free_framebuffer_state(struct pipe_framebuffer_state *fb)
{
   uint i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&fb->cbufs[i], NULL);
   }
   pipe_surface_reference(&fb->zsbuf, NULL);
}


enum pipe_error cso_set_framebuffer(struct cso_context *ctx,
                                    const struct pipe_framebuffer_state *fb)
{
   if (memcmp(&ctx->fb, fb, sizeof(*fb)) != 0) {
      copy_framebuffer_state(&ctx->fb, fb);
      ctx->pipe->set_framebuffer_state(ctx->pipe, fb);
   }
   return PIPE_OK;
}

void cso_save_framebuffer(struct cso_context *ctx)
{
   copy_framebuffer_state(&ctx->fb_saved, &ctx->fb);
}

void cso_restore_framebuffer(struct cso_context *ctx)
{
   if (memcmp(&ctx->fb, &ctx->fb_saved, sizeof(ctx->fb))) {
      copy_framebuffer_state(&ctx->fb, &ctx->fb_saved);
      ctx->pipe->set_framebuffer_state(ctx->pipe, &ctx->fb);
      free_framebuffer_state(&ctx->fb_saved);
   }
}


enum pipe_error cso_set_viewport(struct cso_context *ctx,
                                 const struct pipe_viewport_state *vp)
{
   if (memcmp(&ctx->vp, vp, sizeof(*vp))) {
      ctx->vp = *vp;
      ctx->pipe->set_viewport_state(ctx->pipe, vp);
   }
   return PIPE_OK;
}

void cso_save_viewport(struct cso_context *ctx)
{
   ctx->vp_saved = ctx->vp;
}


void cso_restore_viewport(struct cso_context *ctx)
{
   if (memcmp(&ctx->vp, &ctx->vp_saved, sizeof(ctx->vp))) {
      ctx->vp = ctx->vp_saved;
      ctx->pipe->set_viewport_state(ctx->pipe, &ctx->vp);
   }
}


enum pipe_error cso_set_blend_color(struct cso_context *ctx,
                                    const struct pipe_blend_color *bc)
{
   if (memcmp(&ctx->blend_color, bc, sizeof(ctx->blend_color))) {
      ctx->blend_color = *bc;
      ctx->pipe->set_blend_color(ctx->pipe, bc);
   }
   return PIPE_OK;
}

enum pipe_error cso_set_stencil_ref(struct cso_context *ctx,
                                    const struct pipe_stencil_ref *sr)
{
   if (memcmp(&ctx->stencil_ref, sr, sizeof(ctx->stencil_ref))) {
      ctx->stencil_ref = *sr;
      ctx->pipe->set_stencil_ref(ctx->pipe, sr);
   }
   return PIPE_OK;
}

void cso_save_stencil_ref(struct cso_context *ctx)
{
   ctx->stencil_ref_saved = ctx->stencil_ref;
}


void cso_restore_stencil_ref(struct cso_context *ctx)
{
   if (memcmp(&ctx->stencil_ref, &ctx->stencil_ref_saved, sizeof(ctx->stencil_ref))) {
      ctx->stencil_ref = ctx->stencil_ref_saved;
      ctx->pipe->set_stencil_ref(ctx->pipe, &ctx->stencil_ref);
   }
}

enum pipe_error cso_set_geometry_shader_handle(struct cso_context *ctx,
                                               void *handle)
{
   if (ctx->geometry_shader != handle) {
      ctx->geometry_shader = handle;
      ctx->pipe->bind_gs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_geometry_shader(struct cso_context *ctx, void *handle)
{
    if (handle == ctx->geometry_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_gs_state(ctx->pipe, NULL);
      ctx->geometry_shader = NULL;
   }
   ctx->pipe->delete_gs_state(ctx->pipe, handle);
}

void cso_save_geometry_shader(struct cso_context *ctx)
{
   assert(!ctx->geometry_shader_saved);
   ctx->geometry_shader_saved = ctx->geometry_shader;
}

void cso_restore_geometry_shader(struct cso_context *ctx)
{
   if (ctx->geometry_shader_saved != ctx->geometry_shader) {
      ctx->pipe->bind_gs_state(ctx->pipe, ctx->geometry_shader_saved);
      ctx->geometry_shader = ctx->geometry_shader_saved;
   }
   ctx->geometry_shader_saved = NULL;
}


/* clip state */

static INLINE void
clip_state_cpy(struct pipe_clip_state *dst,
               const struct pipe_clip_state *src)
{
   dst->nr = src->nr;
   if (src->nr) {
      memcpy(dst->ucp, src->ucp, src->nr * sizeof(src->ucp[0]));
   }
}

static INLINE int
clip_state_cmp(const struct pipe_clip_state *a,
               const struct pipe_clip_state *b)
{
   if (a->nr != b->nr) {
      return 1;
   }
   if (a->nr) {
      return memcmp(a->ucp, b->ucp, a->nr * sizeof(a->ucp[0]));
   }
   return 0;
}

void
cso_set_clip(struct cso_context *ctx,
             const struct pipe_clip_state *clip)
{
   if (clip_state_cmp(&ctx->clip, clip)) {
      clip_state_cpy(&ctx->clip, clip);
      ctx->pipe->set_clip_state(ctx->pipe, clip);
   }
}

void
cso_save_clip(struct cso_context *ctx)
{
   clip_state_cpy(&ctx->clip_saved, &ctx->clip);
}

void
cso_restore_clip(struct cso_context *ctx)
{
   if (clip_state_cmp(&ctx->clip, &ctx->clip_saved)) {
      clip_state_cpy(&ctx->clip, &ctx->clip_saved);
      ctx->pipe->set_clip_state(ctx->pipe, &ctx->clip_saved);
   }
}
