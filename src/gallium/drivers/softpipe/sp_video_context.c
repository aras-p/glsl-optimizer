/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "sp_video_context.h"
#include "sp_texture.h"


static void
sp_mpeg12_destroy(struct pipe_video_context *vpipe)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;

   assert(vpipe);
	
   /* Asserted in softpipe_delete_fs_state() for some reason */
   ctx->pipe->bind_vs_state(ctx->pipe, NULL);
   ctx->pipe->bind_fs_state(ctx->pipe, NULL);

   ctx->pipe->delete_blend_state(ctx->pipe, ctx->blend);
   ctx->pipe->delete_rasterizer_state(ctx->pipe, ctx->rast);
   ctx->pipe->delete_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);

   pipe_video_surface_reference(&ctx->decode_target, NULL);
   vl_compositor_cleanup(&ctx->compositor);
   vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
   ctx->pipe->destroy(ctx->pipe);

   FREE(ctx);
}

static void
sp_mpeg12_decode_macroblocks(struct pipe_video_context *vpipe,
                             struct pipe_video_surface *past,
                             struct pipe_video_surface *future,
                             unsigned num_macroblocks,
                             struct pipe_macroblock *macroblocks,
                             struct pipe_fence_handle **fence)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;
   struct pipe_mpeg12_macroblock *mpeg12_macroblocks = (struct pipe_mpeg12_macroblock*)macroblocks;

   assert(vpipe);
   assert(num_macroblocks);
   assert(macroblocks);
   assert(macroblocks->codec == PIPE_VIDEO_CODEC_MPEG12);
   assert(ctx->decode_target);

   vl_mpeg12_mc_renderer_render_macroblocks(&ctx->mc_renderer,
                                            softpipe_video_surface(ctx->decode_target)->tex,
                                            past ? softpipe_video_surface(past)->tex : NULL,
                                            future ? softpipe_video_surface(future)->tex : NULL,
                                            num_macroblocks, mpeg12_macroblocks, fence);
}

static void
sp_mpeg12_clear_surface(struct pipe_video_context *vpipe,
                        unsigned x, unsigned y,
                        unsigned width, unsigned height,
                        unsigned value,
                        struct pipe_surface *surface)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(surface);

   ctx->pipe->surface_fill(ctx->pipe, surface, x, y, width, height, value);
}

static void
sp_mpeg12_render_picture(struct pipe_video_context     *vpipe,
                         /*struct pipe_surface         *backround,
                         struct pipe_video_rect        *backround_area,*/
                         struct pipe_video_surface     *src_surface,
                         enum pipe_mpeg12_picture_type picture_type,
                         /*unsigned                    num_past_surfaces,
                         struct pipe_video_surface     *past_surfaces,
                         unsigned                      num_future_surfaces,
                         struct pipe_video_surface     *future_surfaces,*/
                         struct pipe_video_rect        *src_area,
                         struct pipe_surface           *dst_surface,
                         struct pipe_video_rect        *dst_area,
                         /*unsigned                      num_layers,
                         struct pipe_surface           *layers,
                         struct pipe_video_rect        *layer_src_areas,
                         struct pipe_video_rect        *layer_dst_areas*/
                         struct pipe_fence_handle      **fence)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;
	
   assert(vpipe);
   assert(src_surface);
   assert(src_area);
   assert(dst_surface);
   assert(dst_area);
	
   vl_compositor_render(&ctx->compositor, softpipe_video_surface(src_surface)->tex,
                        picture_type, src_area, dst_surface->texture, dst_area, fence);
}

static void
sp_mpeg12_set_decode_target(struct pipe_video_context *vpipe,
                            struct pipe_video_surface *dt)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(dt);

   pipe_video_surface_reference(&ctx->decode_target, dt);
}

static void sp_mpeg12_set_csc_matrix(struct pipe_video_context *vpipe, const float *mat)
{
   struct sp_mpeg12_context *ctx = (struct sp_mpeg12_context*)vpipe;

   assert(vpipe);

   vl_compositor_set_csc_matrix(&ctx->compositor, mat);
}

static bool
init_pipe_state(struct sp_mpeg12_context *ctx)
{
   struct pipe_rasterizer_state rast;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   unsigned i;

   assert(ctx);
	
   rast.flatshade = 1;
   rast.flatshade_first = 0;
   rast.light_twoside = 0;
   rast.front_winding = PIPE_WINDING_CCW;
   rast.cull_mode = PIPE_WINDING_CW;
   rast.fill_cw = PIPE_POLYGON_MODE_FILL;
   rast.fill_ccw = PIPE_POLYGON_MODE_FILL;
   rast.offset_cw = 0;
   rast.offset_ccw = 0;
   rast.scissor = 0;
   rast.poly_smooth = 0;
   rast.poly_stipple_enable = 0;
   rast.sprite_coord_enable = 0;
   rast.point_size_per_vertex = 0;
   rast.multisample = 0;
   rast.line_smooth = 0;
   rast.line_stipple_enable = 0;
   rast.line_stipple_factor = 0;
   rast.line_stipple_pattern = 0;
   rast.line_last_pixel = 0;
   rast.line_width = 1;
   rast.point_smooth = 0;
   rast.point_quad_rasterization = 0;
   rast.point_size = 1;
   rast.offset_units = 1;
   rast.offset_scale = 1;
   ctx->rast = ctx->pipe->create_rasterizer_state(ctx->pipe, &rast);
   ctx->pipe->bind_rasterizer_state(ctx->pipe, ctx->rast);

   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 0;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   /* Needed to allow color writes to FB, even if blending disabled */
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   ctx->blend = ctx->pipe->create_blend_state(ctx->pipe, &blend);
   ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend);

   dsa.depth.enabled = 0;
   dsa.depth.writemask = 0;
   dsa.depth.func = PIPE_FUNC_ALWAYS;
   for (i = 0; i < 2; ++i) {
      dsa.stencil[i].enabled = 0;
      dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
      dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].valuemask = 0;
      dsa.stencil[i].writemask = 0;
   }
   dsa.alpha.enabled = 0;
   dsa.alpha.func = PIPE_FUNC_ALWAYS;
   dsa.alpha.ref_value = 0;
   ctx->dsa = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &dsa);
   ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);
	
   return true;
}

static struct pipe_video_context *
sp_mpeg12_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                 enum pipe_video_chroma_format chroma_format,
                 unsigned width, unsigned height)
{
   struct sp_mpeg12_context *ctx;

   assert(u_reduce_video_profile(profile) == PIPE_VIDEO_CODEC_MPEG12);

   ctx = CALLOC_STRUCT(sp_mpeg12_context);

   if (!ctx)
      return NULL;

   ctx->base.profile = profile;
   ctx->base.chroma_format = chroma_format;
   ctx->base.width = width;
   ctx->base.height = height;

   ctx->base.screen = screen;
   ctx->base.destroy = sp_mpeg12_destroy;
   ctx->base.decode_macroblocks = sp_mpeg12_decode_macroblocks;
   ctx->base.clear_surface = sp_mpeg12_clear_surface;
   ctx->base.render_picture = sp_mpeg12_render_picture;
   ctx->base.set_decode_target = sp_mpeg12_set_decode_target;
   ctx->base.set_csc_matrix = sp_mpeg12_set_csc_matrix;

   ctx->pipe = screen->context_create(screen, NULL);
   if (!ctx->pipe) {
      FREE(ctx);
      return NULL;
   }

   /* TODO: Use slice buffering for softpipe when implemented, no advantage to buffering an entire picture */
   if (!vl_mpeg12_mc_renderer_init(&ctx->mc_renderer, ctx->pipe,
                                   width, height, chroma_format,
                                   VL_MPEG12_MC_RENDERER_BUFFER_PICTURE,
                                   /* TODO: Use XFER_NONE when implemented */
                                   VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE,
                                   true)) {
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }
	
   if (!vl_compositor_init(&ctx->compositor, ctx->pipe)) {
      vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }
	
   if (!init_pipe_state(ctx)) {
      vl_compositor_cleanup(&ctx->compositor);
      vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   return &ctx->base;
}

struct pipe_video_context *
sp_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                enum pipe_video_chroma_format chroma_format,
                unsigned width, unsigned height)
{
   assert(screen);
   assert(width && height);

   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12:
         return sp_mpeg12_create(screen, profile,
                                 chroma_format,
                                 width, height);
      default:
         return NULL;
   }
}
