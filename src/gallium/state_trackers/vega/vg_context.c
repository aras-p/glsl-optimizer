/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "vg_context.h"

#include "paint.h"
#include "renderer.h"
#include "shaders_cache.h"
#include "shader.h"
#include "asm_util.h"
#include "st_inlines.h"

#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"

#include "cso_cache/cso_context.h"

#include "util/u_simple_shaders.h"
#include "util/u_memory.h"
#include "util/u_blit.h"

struct vg_context *_vg_context = 0;

struct vg_context * vg_current_context(void)
{
   return _vg_context;
}

static void init_clear(struct vg_context *st)
{
   struct pipe_context *pipe = st->pipe;

   /* rasterizer state: bypass clipping */
   memset(&st->clear.raster, 0, sizeof(st->clear.raster));
   st->clear.raster.gl_rasterization_rules = 1;

   /* fragment shader state: color pass-through program */
   st->clear.fs =
      util_make_fragment_passthrough_shader(pipe);
}
void vg_set_current_context(struct vg_context *ctx)
{
   _vg_context = ctx;
}

struct vg_context * vg_create_context(struct pipe_context *pipe,
                                      const void *visual,
                                      struct vg_context *share)
{
   struct vg_context *ctx;

   ctx = CALLOC_STRUCT(vg_context);

   ctx->pipe = pipe;

   vg_init_state(&ctx->state.vg);
   ctx->state.dirty = ALL_DIRTY;

   ctx->cso_context = cso_create_context(pipe);

   init_clear(ctx);

   ctx->default_paint = paint_create(ctx);
   ctx->state.vg.stroke_paint = ctx->default_paint;
   ctx->state.vg.fill_paint = ctx->default_paint;


   ctx->mask.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->mask.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->mask.sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->mask.sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->mask.sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->mask.sampler.normalized_coords = 0;

   ctx->blend_sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->blend_sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->blend_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->blend_sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->blend_sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   ctx->blend_sampler.normalized_coords = 0;

   vg_set_error(ctx, VG_NO_ERROR);

   ctx->owned_objects[VG_OBJECT_PAINT] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_IMAGE] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_MASK] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_FONT] = cso_hash_create();
   ctx->owned_objects[VG_OBJECT_PATH] = cso_hash_create();

   ctx->renderer = renderer_create(ctx);
   ctx->sc = shaders_cache_create(ctx);
   ctx->shader = shader_create(ctx);

   ctx->blit = util_create_blit(ctx->pipe, ctx->cso_context);

   return ctx;
}

void vg_destroy_context(struct vg_context *ctx)
{
   struct pipe_buffer **cbuf = &ctx->mask.cbuf;
   struct pipe_buffer **vsbuf = &ctx->vs_const_buffer;

   util_destroy_blit(ctx->blit);
   renderer_destroy(ctx->renderer);
   shaders_cache_destroy(ctx->sc);
   shader_destroy(ctx->shader);
   paint_destroy(ctx->default_paint);

   if (*cbuf)
      pipe_buffer_reference(cbuf, NULL);

   if (*vsbuf)
      pipe_buffer_reference(vsbuf, NULL);

   if (ctx->clear.fs) {
      cso_delete_fragment_shader(ctx->cso_context, ctx->clear.fs);
      ctx->clear.fs = NULL;
   }

   if (ctx->plain_vs) {
      vg_shader_destroy(ctx, ctx->plain_vs);
      ctx->plain_vs = NULL;
   }
   if (ctx->clear_vs) {
      vg_shader_destroy(ctx, ctx->clear_vs);
      ctx->clear_vs = NULL;
   }
   if (ctx->texture_vs) {
      vg_shader_destroy(ctx, ctx->texture_vs);
      ctx->texture_vs = NULL;
   }

   if (ctx->pass_through_depth_fs)
      vg_shader_destroy(ctx, ctx->pass_through_depth_fs);
   if (ctx->mask.union_fs)
      vg_shader_destroy(ctx, ctx->mask.union_fs);
   if (ctx->mask.intersect_fs)
      vg_shader_destroy(ctx, ctx->mask.intersect_fs);
   if (ctx->mask.subtract_fs)
      vg_shader_destroy(ctx, ctx->mask.subtract_fs);
   if (ctx->mask.set_fs)
      vg_shader_destroy(ctx, ctx->mask.set_fs);

   cso_release_all(ctx->cso_context);
   cso_destroy_context(ctx->cso_context);

   cso_hash_delete(ctx->owned_objects[VG_OBJECT_PAINT]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_IMAGE]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_MASK]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_FONT]);
   cso_hash_delete(ctx->owned_objects[VG_OBJECT_PATH]);

   free(ctx);
}

void vg_init_object(struct vg_object *obj, struct vg_context *ctx, enum vg_object_type type)
{
   obj->type = type;
   obj->ctx = ctx;
}

VGboolean vg_context_is_object_valid(struct vg_context *ctx,
                                enum vg_object_type type,
                                void *ptr)
{
    if (ctx) {
       struct cso_hash *hash = ctx->owned_objects[type];
       if (!hash)
          return VG_FALSE;
       return cso_hash_contains(hash, (unsigned)(long)ptr);
    }
    return VG_FALSE;
}

void vg_context_add_object(struct vg_context *ctx,
                           enum vg_object_type type,
                           void *ptr)
{
    if (ctx) {
       struct cso_hash *hash = ctx->owned_objects[type];
       if (!hash)
          return;
       cso_hash_insert(hash, (unsigned)(long)ptr, ptr);
    }
}

void vg_context_remove_object(struct vg_context *ctx,
                              enum vg_object_type type,
                              void *ptr)
{
   if (ctx) {
      struct cso_hash *hash = ctx->owned_objects[type];
      if (!hash)
         return;
      cso_hash_take(hash, (unsigned)(long)ptr);
   }
}

static void update_clip_state(struct vg_context *ctx)
{
   struct pipe_depth_stencil_alpha_state *dsa = &ctx->state.g3d.dsa;
   struct vg_state *state =  &ctx->state.vg;

   memset(dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));

   if (state->scissoring) {
      struct pipe_blend_state *blend = &ctx->state.g3d.blend;
      struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;
      int i;

      dsa->depth.writemask = 1;/*glDepthMask(TRUE);*/
      dsa->depth.func = PIPE_FUNC_ALWAYS;
      dsa->depth.enabled = 1;

      cso_save_blend(ctx->cso_context);
      cso_save_fragment_shader(ctx->cso_context);
      /* set a passthrough shader */
      if (!ctx->pass_through_depth_fs)
         ctx->pass_through_depth_fs = shader_create_from_text(ctx->pipe,
                                                              pass_through_depth_asm,
                                                              40,
                                                              PIPE_SHADER_FRAGMENT);
      cso_set_fragment_shader_handle(ctx->cso_context,
                                     ctx->pass_through_depth_fs->driver);
      cso_set_depth_stencil_alpha(ctx->cso_context, dsa);

      ctx->pipe->clear(ctx->pipe, PIPE_CLEAR_DEPTHSTENCIL, NULL, 1.0, 0);

      /* disable color writes */
      blend->rt[0].colormask = 0; /*disable colorwrites*/
      cso_set_blend(ctx->cso_context, blend);

      /* enable scissoring */
      for (i = 0; i < state->scissor_rects_num; ++i) {
         const float x      = state->scissor_rects[i * 4 + 0].f;
         const float y      = state->scissor_rects[i * 4 + 1].f;
         const float width  = state->scissor_rects[i * 4 + 2].f;
         const float height = state->scissor_rects[i * 4 + 3].f;
         VGfloat minx, miny, maxx, maxy;

         minx = 0;
         miny = 0;
         maxx = fb->width;
         maxy = fb->height;

         if (x > minx)
            minx = x;
         if (y > miny)
            miny = y;

         if (x + width < maxx)
            maxx = x + width;
         if (y + height < maxy)
            maxy = y + height;

         /* check for null space */
         if (minx >= maxx || miny >= maxy)
            minx = miny = maxx = maxy = 0;

         /*glClear(GL_DEPTH_BUFFER_BIT);*/
         renderer_draw_quad(ctx->renderer, minx, miny, maxx, maxy, 0.0f);
      }

      cso_restore_blend(ctx->cso_context);
      cso_restore_fragment_shader(ctx->cso_context);

      dsa->depth.enabled = 1; /* glEnable(GL_DEPTH_TEST); */
      dsa->depth.writemask = 0;/*glDepthMask(FALSE);*/
      dsa->depth.func = PIPE_FUNC_GEQUAL;
   }
}

void vg_validate_state(struct vg_context *ctx)
{
   if ((ctx->state.dirty & BLEND_DIRTY)) {
      struct pipe_blend_state *blend = &ctx->state.g3d.blend;
      memset(blend, 0, sizeof(struct pipe_blend_state));
      blend->rt[0].blend_enable = 1;
      blend->rt[0].colormask = PIPE_MASK_RGBA;

      switch (ctx->state.vg.blend_mode) {
      case VG_BLEND_SRC:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].blend_enable = 0;
         break;
      case VG_BLEND_SRC_OVER:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_SRC_ALPHA;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
         break;
      case VG_BLEND_DST_OVER:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_INV_DST_ALPHA;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_INV_DST_ALPHA;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_DST_ALPHA;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_DST_ALPHA;
         break;
      case VG_BLEND_SRC_IN:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_DST_ALPHA;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_DST_ALPHA;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
         break;
      case VG_BLEND_DST_IN:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_SRC_ALPHA;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
         break;
      case VG_BLEND_MULTIPLY:
      case VG_BLEND_SCREEN:
      case VG_BLEND_DARKEN:
      case VG_BLEND_LIGHTEN:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
         blend->rt[0].blend_enable = 0;
         break;
      case VG_BLEND_ADDITIVE:
         blend->rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ONE;
         blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
         break;
      default:
         assert(!"not implemented blend mode");
      }
      cso_set_blend(ctx->cso_context, &ctx->state.g3d.blend);
   }
   if ((ctx->state.dirty & RASTERIZER_DIRTY)) {
      struct pipe_rasterizer_state *raster = &ctx->state.g3d.rasterizer;
      memset(raster, 0, sizeof(struct pipe_rasterizer_state));
      raster->gl_rasterization_rules = 1;
      cso_set_rasterizer(ctx->cso_context, &ctx->state.g3d.rasterizer);
   }
   if ((ctx->state.dirty & VIEWPORT_DIRTY)) {
      struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;
      const VGint param_bytes = 8 * sizeof(VGfloat);
      VGfloat vs_consts[8] = {
         2.f/fb->width, 2.f/fb->height, 1, 1,
         -1, -1, 0, 0
      };
      struct pipe_buffer **cbuf = &ctx->vs_const_buffer;

      vg_set_viewport(ctx, VEGA_Y0_BOTTOM);

      pipe_buffer_reference(cbuf, NULL);
      *cbuf = pipe_buffer_create(ctx->pipe->screen, 16,
                                        PIPE_BUFFER_USAGE_CONSTANT,
                                        param_bytes);

      if (*cbuf) {
         st_no_flush_pipe_buffer_write(ctx, *cbuf,
                                       0, param_bytes, vs_consts);
      }
      ctx->pipe->set_constant_buffer(ctx->pipe, PIPE_SHADER_VERTEX, 0, *cbuf);
   }
   if ((ctx->state.dirty & VS_DIRTY)) {
      cso_set_vertex_shader_handle(ctx->cso_context,
                                   vg_plain_vs(ctx));
   }

   /* must be last because it renders to the depth buffer*/
   if ((ctx->state.dirty & DEPTH_STENCIL_DIRTY)) {
      update_clip_state(ctx);
      cso_set_depth_stencil_alpha(ctx->cso_context, &ctx->state.g3d.dsa);
   }

   shader_set_masking(ctx->shader, ctx->state.vg.masking);
   shader_set_image_mode(ctx->shader, ctx->state.vg.image_mode);

   ctx->state.dirty = NONE_DIRTY;
}

VGboolean vg_object_is_valid(void *ptr, enum vg_object_type type)
{
   struct vg_object *obj = ptr;
   if (ptr && is_aligned(obj) && obj->type == type)
      return VG_TRUE;
   else
      return VG_FALSE;
}

void vg_set_error(struct vg_context *ctx,
                  VGErrorCode code)
{
   /*vgGetError returns the oldest error code provided by
    * an API call on the current context since the previous
    * call to vgGetError on that context (or since the creation
    of the context).*/
   if (ctx->_error == VG_NO_ERROR)
      ctx->_error = code;
}

void vg_prepare_blend_surface(struct vg_context *ctx)
{
   struct pipe_surface *dest_surface = NULL;
   struct pipe_context *pipe = ctx->pipe;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->strb;

   /* first finish all pending rendering */
   vgFinish();

   dest_surface = pipe->screen->get_tex_surface(pipe->screen,
                                                stfb->blend_texture,
                                                0, 0, 0,
                                                PIPE_BUFFER_USAGE_GPU_WRITE);
   /* flip it, because we want to use it as a sampler */
   util_blit_pixels_tex(ctx->blit,
                        strb->texture,
                        0, strb->height,
                        strb->width, 0,
                        dest_surface,
                        0, 0,
                        strb->width, strb->height,
                        0.0, PIPE_TEX_MIPFILTER_NEAREST);

   if (dest_surface)
      pipe_surface_reference(&dest_surface, NULL);

   /* make sure it's complete */
   vgFinish();
}


void vg_prepare_blend_surface_from_mask(struct vg_context *ctx)
{
   struct pipe_surface *dest_surface = NULL;
   struct pipe_context *pipe = ctx->pipe;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->strb;

   vg_validate_state(ctx);

   /* first finish all pending rendering */
   vgFinish();

   dest_surface = pipe->screen->get_tex_surface(pipe->screen,
                                                stfb->blend_texture,
                                                0, 0, 0,
                                                PIPE_BUFFER_USAGE_GPU_WRITE);

   /* flip it, because we want to use it as a sampler */
   util_blit_pixels_tex(ctx->blit,
                        stfb->alpha_mask,
                        0, strb->height,
                        strb->width, 0,
                        dest_surface,
                        0, 0,
                        strb->width, strb->height,
                        0.0, PIPE_TEX_MIPFILTER_NEAREST);

   /* make sure it's complete */
   vgFinish();

   if (dest_surface)
      pipe_surface_reference(&dest_surface, NULL);
}

void * vg_plain_vs(struct vg_context *ctx)
{
   if (!ctx->plain_vs) {
      ctx->plain_vs = shader_create_from_text(ctx->pipe,
                                              vs_plain_asm,
                                              200,
                                              PIPE_SHADER_VERTEX);
   }

   return ctx->plain_vs->driver;
}


void * vg_clear_vs(struct vg_context *ctx)
{
   if (!ctx->clear_vs) {
      ctx->clear_vs = shader_create_from_text(ctx->pipe,
                                              vs_clear_asm,
                                              200,
                                              PIPE_SHADER_VERTEX);
   }

   return ctx->clear_vs->driver;
}

void * vg_texture_vs(struct vg_context *ctx)
{
   if (!ctx->texture_vs) {
      ctx->texture_vs = shader_create_from_text(ctx->pipe,
                                                vs_texture_asm,
                                                200,
                                                PIPE_SHADER_VERTEX);
   }

   return ctx->texture_vs->driver;
}

void vg_set_viewport(struct vg_context *ctx, VegaOrientation orientation)
{
   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;
   VGfloat y_scale = (orientation == VEGA_Y0_BOTTOM) ? -2.f : 2.f;

   viewport.scale[0] =  fb->width / 2.f;
   viewport.scale[1] =  fb->height / y_scale;
   viewport.scale[2] =  1.0;
   viewport.scale[3] =  1.0;
   viewport.translate[0] = fb->width / 2.f;
   viewport.translate[1] = fb->height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.translate[3] = 0.0;

   cso_set_viewport(ctx->cso_context, &viewport);
}
