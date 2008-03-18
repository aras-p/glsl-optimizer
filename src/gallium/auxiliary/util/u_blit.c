/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Copy/blit pixel rect between surfaces
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_debug.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_draw_quad.h"
#include "util/u_blit.h"
#include "util/u_simple_shaders.h"


struct blit_state
{
   struct pipe_context *pipe;

   void *blend;
   void *depthstencil;
   void *rasterizer;
   void *samplers[2];  /* one for linear, one for nearest sampling */

   /*struct pipe_viewport_state viewport;*/
   struct pipe_sampler_state *vs;
   struct pipe_sampler_state *fs;
};


/**
 * Create state object for blit.
 * Intended to be created once and re-used for many blit() calls.
 */
struct blit_state *
util_create_blit(struct pipe_context *pipe)
{
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct blit_state *ctx;
   struct pipe_sampler_state sampler;

   ctx = CALLOC_STRUCT(blit_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;

   /* we don't use blending, but need to set valid values */
   memset(&blend, 0, sizeof(blend));
   blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.colormask = PIPE_MASK_RGBA;
   ctx->blend = pipe->create_blend_state(pipe, &blend);

   /* depth/stencil/alpha */
   memset(&depthstencil, 0, sizeof(depthstencil));
   ctx->depthstencil = pipe->create_depth_stencil_alpha_state(pipe, &depthstencil);

   /* rasterizer */
   memset(&rasterizer, 0, sizeof(rasterizer));
   rasterizer.front_winding = PIPE_WINDING_CW;
   rasterizer.cull_mode = PIPE_WINDING_NONE;
   rasterizer.bypass_clipping = 1;  /* bypasses viewport too */
   /*rasterizer.bypass_vs = 1;*/
   ctx->rasterizer = pipe->create_rasterizer_state(pipe, &rasterizer);

   /* samplers */
   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
   sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
   sampler.normalized_coords = 1;
   ctx->samplers[0] = pipe->create_sampler_state(pipe, &sampler);

   sampler.min_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
   ctx->samplers[1] = pipe->create_sampler_state(pipe, &sampler);


#if 0
   /* viewport */
   ctx->viewport.scale[0] = 1.0;
   ctx->viewport.scale[1] = 1.0;
   ctx->viewport.scale[2] = 1.0;
   ctx->viewport.scale[3] = 1.0;
   ctx->viewport.translate[0] = 0.0;
   ctx->viewport.translate[1] = 0.0;
   ctx->viewport.translate[2] = 0.0;
   ctx->viewport.translate[3] = 0.0;
#endif

   /* vertex shader */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                                    semantic_indexes);
   }

   /* fragment shader */
   ctx->fs = util_make_fragment_tex_shader(pipe);

   return ctx;
}


/**
 * Destroy a blit context
 */
void
util_destroy_blit(struct blit_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   pipe->delete_blend_state(pipe, ctx->blend);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->delete_rasterizer_state(pipe, ctx->rasterizer);
   pipe->delete_sampler_state(pipe, ctx->samplers[0]);
   pipe->delete_sampler_state(pipe, ctx->samplers[1]);

   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_fs_state(pipe, ctx->fs);

   FREE(ctx);
}


/**
 * Copy pixel block from src surface to dst surface.
 * Overlapping regions are acceptable.
 * XXX need some control over blitting Z and/or stencil.
 */
void
util_blit_pixels(struct blit_state *ctx,
                 struct pipe_surface *src,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture texTemp, *tex;
   struct pipe_surface *texSurf;
   struct pipe_framebuffer_state fb;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   const int srcLeft = MIN2(srcX0, srcX1);
   const int srcTop = MIN2(srcY0, srcY1);

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   if (srcLeft != srcX0) {
      /* left-right flip */
      int tmp = dstX0;
      dstX0 = dstX1;
      dstX1 = tmp;
   }

   if (srcTop != srcY0) {
      /* up-down flip */
      int tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   /*
    * XXX for now we're always creating a temporary texture.
    * Strictly speaking that's not always needed.
    */

   /* create temp texture */
   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = src->format;
   texTemp.last_level = 0;
   texTemp.width[0] = srcW;
   texTemp.height[0] = srcH;
   texTemp.depth[0] = 1;
   texTemp.compressed = 0;
   texTemp.cpp = pf_get_bits(src->format) / 8;

   tex = screen->texture_create(screen, &texTemp);
   if (!tex)
      return;

   texSurf = screen->get_tex_surface(screen, tex, 0, 0, 0);

   /* load temp texture */
   pipe->surface_copy(pipe, FALSE,
                      texSurf, 0, 0,   /* dest */
                      src, srcLeft, srcTop, /* src */
                      srcW, srcH);     /* size */

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;
   fb.cbufs[0] = dst;
   pipe->set_framebuffer_state(pipe, &fb);

   /* sampler */
   if (filter == PIPE_TEX_MIPFILTER_NEAREST)
      pipe->bind_sampler_states(pipe, 1, &ctx->samplers[0]);
   else
      pipe->bind_sampler_states(pipe, 1, &ctx->samplers[1]);

   /* texture */
   pipe->set_sampler_textures(pipe, 1, &tex);

   /* shaders */
   pipe->bind_fs_state(pipe, ctx->fs);
   pipe->bind_vs_state(pipe, ctx->vs);

   /* misc state */
   pipe->bind_blend_state(pipe, ctx->blend);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->bind_rasterizer_state(pipe, ctx->rasterizer);

   /* draw quad */
   util_draw_texquad(pipe, dstX0, dstY0, dstX1, dstY1, z);

   /* unbind */
   pipe->set_sampler_textures(pipe, 0, NULL);
   pipe->bind_sampler_states(pipe, 0, NULL);

   /* free stuff */
   pipe_surface_reference(&texSurf, NULL);
   screen->texture_release(screen, &tex);

   /* Note: caller must restore pipe/gallium state at this time */
}

