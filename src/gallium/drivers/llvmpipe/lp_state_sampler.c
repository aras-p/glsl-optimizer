/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:
 *  Brian Paul
 */

#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "draw/draw_context.h"

#include "lp_context.h"
#include "lp_screen.h"
#include "lp_state.h"
#include "lp_debug.h"
#include "state_tracker/sw_winsys.h"


static void *
llvmpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct pipe_sampler_state *state = mem_dup(sampler, sizeof *sampler);

   if (LP_PERF & PERF_NO_MIP_LINEAR) {
      if (state->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
	 state->min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   }

   if (LP_PERF & PERF_NO_MIPMAPS)
      state->min_mip_filter = PIPE_TEX_MIPFILTER_NONE;

   if (LP_PERF & PERF_NO_LINEAR) {
      state->mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      state->min_img_filter = PIPE_TEX_FILTER_NEAREST;
   }

   return state;
}


static void
llvmpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned shader,
                             unsigned start,
                             unsigned num,
                             void **samplers)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i;

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(llvmpipe->samplers[shader]));

   draw_flush(llvmpipe->draw);

   /* set the new samplers */
   for (i = 0; i < num; i++) {
      llvmpipe->samplers[shader][start + i] = samplers[i];
   }

   /* find highest non-null samplers[] entry */
   {
      unsigned j = MAX2(llvmpipe->num_samplers[shader], start + num);
      while (j > 0 && llvmpipe->samplers[shader][j - 1] == NULL)
         j--;
      llvmpipe->num_samplers[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_samplers(llvmpipe->draw,
                        shader,
                        llvmpipe->samplers[shader],
                        llvmpipe->num_samplers[shader]);
   }

   llvmpipe->dirty |= LP_NEW_SAMPLER;
}


static void
llvmpipe_set_sampler_views(struct pipe_context *pipe,
                           unsigned shader,
                           unsigned start,
                           unsigned num,
                           struct pipe_sampler_view **views)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SHADER_SAMPLER_VIEWS);

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(llvmpipe->sampler_views[shader]));

   draw_flush(llvmpipe->draw);

   /* set the new sampler views */
   for (i = 0; i < num; i++) {
      /* Note: we're using pipe_sampler_view_release() here to work around
       * a possible crash when the old view belongs to another context that
       * was already destroyed.
       */
      pipe_sampler_view_release(pipe,
                                &llvmpipe->sampler_views[shader][start + i]);
      pipe_sampler_view_reference(&llvmpipe->sampler_views[shader][start + i],
                                  views[i]);
   }

   /* find highest non-null sampler_views[] entry */
   {
      unsigned j = MAX2(llvmpipe->num_sampler_views[shader], start + num);
      while (j > 0 && llvmpipe->sampler_views[shader][j - 1] == NULL)
         j--;
      llvmpipe->num_sampler_views[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_sampler_views(llvmpipe->draw,
                             shader,
                             llvmpipe->sampler_views[shader],
                             llvmpipe->num_sampler_views[shader]);
   }

   llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
}


static struct pipe_sampler_view *
llvmpipe_create_sampler_view(struct pipe_context *pipe,
                            struct pipe_resource *texture,
                            const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);
   /*
    * XXX we REALLY want to see the correct bind flag here but the OpenGL
    * state tracker can't guarantee that at least for texture buffer objects.
    */
   if (!(texture->bind & PIPE_BIND_SAMPLER_VIEW))
      debug_printf("Illegal sampler view creation without bind flag\n");

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, texture);
      view->context = pipe;
   }

   return view;
}


static void
llvmpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


static void
llvmpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}


static void
prepare_shader_sampling(
   struct llvmpipe_context *lp,
   unsigned num,
   struct pipe_sampler_view **views,
   unsigned shader_type,
   struct pipe_resource *mapped_tex[PIPE_MAX_SHADER_SAMPLER_VIEWS])
{

   unsigned i;
   uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t mip_offsets[PIPE_MAX_TEXTURE_LEVELS];
   const void *addr;

   assert(num <= PIPE_MAX_SHADER_SAMPLER_VIEWS);
   if (!num)
      return;

   for (i = 0; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      if (view) {
         struct pipe_resource *tex = view->texture;
         struct llvmpipe_resource *lp_tex = llvmpipe_resource(tex);
         unsigned width0 = tex->width0;
         unsigned num_layers = tex->depth0;
         unsigned first_level = 0;
         unsigned last_level = 0;

         /* We're referencing the texture's internal data, so save a
          * reference to it.
          */
         pipe_resource_reference(&mapped_tex[i], tex);

         if (!lp_tex->dt) {
            /* regular texture - setup array of mipmap level offsets */
            struct pipe_resource *res = view->texture;
            int j;
            void *mip_ptr;

            if (llvmpipe_resource_is_texture(res)) {
               first_level = view->u.tex.first_level;
               last_level = view->u.tex.last_level;
               assert(first_level <= last_level);
               assert(last_level <= res->last_level);

               /* must trigger allocation first before we can get base ptr */
               /* XXX this may fail due to OOM ? */
               mip_ptr = llvmpipe_get_texture_image_all(lp_tex, view->u.tex.first_level,
                                                        LP_TEX_USAGE_READ);
               addr = lp_tex->linear_img.data;

               for (j = first_level; j <= last_level; j++) {
                  mip_ptr = llvmpipe_get_texture_image_all(lp_tex, j,
                                                           LP_TEX_USAGE_READ);
                  mip_offsets[j] = (uint8_t *)mip_ptr - (uint8_t *)addr;
                  /*
                   * could get mip offset directly but need call above to
                   * invoke tiled->linear conversion.
                   */
                  assert(lp_tex->linear_mip_offsets[j] == mip_offsets[j]);
                  row_stride[j] = lp_tex->row_stride[j];
                  img_stride[j] = lp_tex->img_stride[j];
               }
               if (res->target == PIPE_TEXTURE_1D_ARRAY ||
                   res->target == PIPE_TEXTURE_2D_ARRAY) {
                  num_layers = view->u.tex.last_layer - view->u.tex.first_layer + 1;
                  for (j = first_level; j <= last_level; j++) {
                     mip_offsets[j] += view->u.tex.first_layer *
                                       lp_tex->img_stride[j];
                  }
                  assert(view->u.tex.first_layer <= view->u.tex.last_layer);
                  assert(view->u.tex.last_layer < res->array_size);
               }
            }
            else {
               unsigned view_blocksize = util_format_get_blocksize(view->format);
               mip_ptr = lp_tex->data;
               addr = mip_ptr;
               /* probably don't really need to fill that out */
               mip_offsets[0] = 0;
               row_stride[0] = 0;
               row_stride[0] = 0;

               /* everything specified in number of elements here. */
               width0 = view->u.buf.last_element - view->u.buf.first_element + 1;
               addr = (uint8_t *)addr + view->u.buf.first_element *
                               view_blocksize;
               assert(view->u.buf.first_element <= view->u.buf.last_element);
               assert(view->u.buf.last_element * view_blocksize < res->width0);
            }
         }
         else {
            /* display target texture/surface */
            /*
             * XXX: Where should this be unmapped?
             */
            struct llvmpipe_screen *screen = llvmpipe_screen(tex->screen);
            struct sw_winsys *winsys = screen->winsys;
            addr = winsys->displaytarget_map(winsys, lp_tex->dt,
                                                PIPE_TRANSFER_READ);
            row_stride[0] = lp_tex->row_stride[0];
            img_stride[0] = lp_tex->img_stride[0];
            mip_offsets[0] = 0;
            assert(addr);
         }
         draw_set_mapped_texture(lp->draw,
                                 shader_type,
                                 i,
                                 width0, tex->height0, num_layers,
                                 first_level, last_level,
                                 addr,
                                 row_stride, img_stride, mip_offsets);
      }
   }
}
                        

/**
 * Called during state validation when LP_NEW_SAMPLER_VIEW is set.
 */
void
llvmpipe_prepare_vertex_sampling(struct llvmpipe_context *lp,
                                 unsigned num,
                                 struct pipe_sampler_view **views)
{
   prepare_shader_sampling(lp, num, views, PIPE_SHADER_VERTEX,
                           lp->mapped_vs_tex);
}

void
llvmpipe_cleanup_vertex_sampling(struct llvmpipe_context *ctx)
{
   unsigned i;
   for (i = 0; i < Elements(ctx->mapped_vs_tex); i++) {
      pipe_resource_reference(&ctx->mapped_vs_tex[i], NULL);
   }
}


/**
 * Called during state validation when LP_NEW_SAMPLER_VIEW is set.
 */
void
llvmpipe_prepare_geometry_sampling(struct llvmpipe_context *lp,
                                   unsigned num,
                                   struct pipe_sampler_view **views)
{
   prepare_shader_sampling(lp, num, views, PIPE_SHADER_GEOMETRY,
                           lp->mapped_gs_tex);
}

void
llvmpipe_cleanup_geometry_sampling(struct llvmpipe_context *ctx)
{
   unsigned i;
   for (i = 0; i < Elements(ctx->mapped_gs_tex); i++) {
      pipe_resource_reference(&ctx->mapped_gs_tex[i], NULL);
   }
}

void
llvmpipe_init_sampler_funcs(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->pipe.create_sampler_state = llvmpipe_create_sampler_state;

   llvmpipe->pipe.bind_sampler_states = llvmpipe_bind_sampler_states;
   llvmpipe->pipe.create_sampler_view = llvmpipe_create_sampler_view;
   llvmpipe->pipe.set_sampler_views = llvmpipe_set_sampler_views;
   llvmpipe->pipe.sampler_view_destroy = llvmpipe_sampler_view_destroy;
   llvmpipe->pipe.delete_sampler_state = llvmpipe_delete_sampler_state;
}
