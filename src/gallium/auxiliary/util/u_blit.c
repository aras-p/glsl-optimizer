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
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_blit.h"
#include "util/u_draw_quad.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_shaders.h"

#include "cso_cache/cso_context.h"


struct blit_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_viewport_state viewport;

   struct pipe_shader_state vert_shader;
   struct pipe_shader_state frag_shader;
   void *vs;
   void *fs;

   struct pipe_buffer *vbuf;  /**< quad vertices */
   float vertices[4][2][4];   /**< vertex/texcoords for quad */
};


/**
 * Create state object for blit.
 * Intended to be created once and re-used for many blit() calls.
 */
struct blit_state *
util_create_blit(struct pipe_context *pipe, struct cso_context *cso)
{
   struct blit_state *ctx;
   uint i;

   ctx = CALLOC_STRUCT(blit_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;
   ctx->cso = cso;

   /* disabled blending/masking */
   memset(&ctx->blend, 0, sizeof(ctx->blend));
   ctx->blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   ctx->blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   ctx->blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   ctx->blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   ctx->blend.colormask = PIPE_MASK_RGBA;

   /* no-op depth/stencil/alpha */
   memset(&ctx->depthstencil, 0, sizeof(ctx->depthstencil));

   /* rasterizer */
   memset(&ctx->rasterizer, 0, sizeof(ctx->rasterizer));
   ctx->rasterizer.front_winding = PIPE_WINDING_CW;
   ctx->rasterizer.cull_mode = PIPE_WINDING_NONE;
   ctx->rasterizer.bypass_clipping = 1;
   /*ctx->rasterizer.bypass_vs = 1;*/

   /* samplers */
   memset(&ctx->sampler, 0, sizeof(ctx->sampler));
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->sampler.min_img_filter = 0; /* set later */
   ctx->sampler.mag_img_filter = 0; /* set later */
   ctx->sampler.normalized_coords = 1;

   /* viewport (identity, we setup vertices in wincoords) */
   ctx->viewport.scale[0] = 1.0;
   ctx->viewport.scale[1] = 1.0;
   ctx->viewport.scale[2] = 1.0;
   ctx->viewport.scale[3] = 1.0;
   ctx->viewport.translate[0] = 0.0;
   ctx->viewport.translate[1] = 0.0;
   ctx->viewport.translate[2] = 0.0;
   ctx->viewport.translate[3] = 0.0;

   /* vertex shader */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                                    semantic_indexes,
                                                    &ctx->vert_shader);
   }

   /* fragment shader */
   ctx->fs = util_make_fragment_tex_shader(pipe, &ctx->frag_shader);

   ctx->vbuf = pipe_buffer_create(pipe->screen,
                                  32,
                                  PIPE_BUFFER_USAGE_VERTEX,
                                  sizeof(ctx->vertices));
   if (!ctx->vbuf) {
      FREE(ctx);
      ctx->pipe->delete_fs_state(ctx->pipe, ctx->fs);
      ctx->pipe->delete_vs_state(ctx->pipe, ctx->vs);
      return NULL;
   }

   /* init vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][3] = 1.0f; /* w */
      ctx->vertices[i][1][2] = 0.0f; /* r */
      ctx->vertices[i][1][3] = 1.0f; /* q */
   }

   return ctx;
}


/**
 * Destroy a blit context
 */
void
util_destroy_blit(struct blit_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_fs_state(pipe, ctx->fs);

   FREE((void*) ctx->vert_shader.tokens);
   FREE((void*) ctx->frag_shader.tokens);

   pipe_buffer_reference(pipe->screen, &ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Setup vertex data for the textured quad we'll draw.
 * Note: y=0=top
 */
static void
setup_vertex_data(struct blit_state *ctx,
                  float x0, float y0, float x1, float y1, float z)
{
   void *buf;

   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = 0.0f; /*s*/
   ctx->vertices[0][1][1] = 0.0f; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = 1.0f; /*s*/
   ctx->vertices[1][1][1] = 0.0f; /*t*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = 1.0f;
   ctx->vertices[2][1][1] = 1.0f;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = 0.0f;
   ctx->vertices[3][1][1] = 1.0f;

   buf = pipe_buffer_map(ctx->pipe->screen, ctx->vbuf,
                         PIPE_BUFFER_USAGE_CPU_WRITE);

   memcpy(buf, ctx->vertices, sizeof(ctx->vertices));

   pipe_buffer_unmap(ctx->pipe->screen, ctx->vbuf);
}


/**
 * Setup vertex data for the textured quad we'll draw.
 * Note: y=0=top
 */
static void
setup_vertex_data_tex(struct blit_state *ctx,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   void *buf;

   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = s0; /*s*/
   ctx->vertices[0][1][1] = t0; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = s1; /*s*/
   ctx->vertices[1][1][1] = t0; /*t*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = s1;
   ctx->vertices[2][1][1] = t1;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = s0;
   ctx->vertices[3][1][1] = t1;

   buf = pipe_buffer_map(ctx->pipe->screen, ctx->vbuf,
                         PIPE_BUFFER_USAGE_CPU_WRITE);

   memcpy(buf, ctx->vertices, sizeof(ctx->vertices));

   pipe_buffer_unmap(ctx->pipe->screen, ctx->vbuf);
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

   assert(screen->is_format_supported(screen, src->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));

   if(dst->format == src->format && (dstX1 - dstX0) == srcW && (dstY1 - dstY0) == srcH) {
      /* FIXME: this will most surely fail for overlapping rectangles */
      pipe->surface_copy(pipe, FALSE,
			 dst, dstX0, dstY0,   /* dest */
			 src, srcX0, srcY0, /* src */
			 srcW, srcH);     /* size */
      return;
   }
   
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

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
   pf_get_block(src->format, &texTemp.block);

   tex = screen->texture_create(screen, &texTemp);
   if (!tex)
      return;

   texSurf = screen->get_tex_surface(screen, tex, 0, 0, 0, 
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* load temp texture */
   pipe->surface_copy(pipe, FALSE,
                      texSurf, 0, 0,   /* dest */
                      src, srcLeft, srcTop, /* src */
                      srcW, srcH);     /* size */

   /* free the surface, update the texture if necessary.
    */
   screen->tex_surface_release(screen, &texSurf);

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_viewport(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_viewport(ctx->cso, &ctx->viewport);

   /* sampler */
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso);

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &tex);

   /* shaders */
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs);
   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.num_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   setup_vertex_data(ctx,
                     (float) dstX0, (float) dstY0, 
                     (float) dstX1, (float) dstY1, z);

   util_draw_vertex_buffer(ctx->pipe, ctx->vbuf,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);

   screen->texture_release(screen, &tex);
}

/**
 * Copy pixel block from src texture to dst surface.
 * Overlapping regions are acceptable.
 *
 * XXX Should support selection of level.
 * XXX need some control over blitting Z and/or stencil.
 */
void
util_blit_pixels_tex(struct blit_state *ctx,
                 struct pipe_texture *tex,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(tex->width[0] != 0);
   assert(tex->height[0] != 0);

   s0 = srcX0 / (float)tex->width[0];
   s1 = srcX1 / (float)tex->width[0];
   t0 = srcY0 / (float)tex->height[0];
   t1 = srcY1 / (float)tex->height[0];

   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_viewport(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_viewport(ctx->cso, &ctx->viewport);

   /* sampler */
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso);

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &tex);

   /* shaders */
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs);
   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.num_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   setup_vertex_data_tex(ctx,
                     (float) dstX0, (float) dstY0,
                     (float) dstX1, (float) dstY1,
                     s0, t0, s1, t1,
                     z);

   util_draw_vertex_buffer(ctx->pipe, ctx->vbuf,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);
}
