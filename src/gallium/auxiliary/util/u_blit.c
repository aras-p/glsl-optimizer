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
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_blit.h"
#include "util/u_draw_quad.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_shaders.h"
#include "util/u_surface.h"
#include "util/u_rect.h"

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
   struct pipe_clip_state clip;

   void *vs;
   void *fs[TGSI_WRITEMASK_XYZW + 1];

   struct pipe_buffer *vbuf;  /**< quad vertices */
   unsigned vbuf_slot;

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
   ctx->blend.rt[0].colormask = PIPE_MASK_RGBA;

   /* no-op depth/stencil/alpha */
   memset(&ctx->depthstencil, 0, sizeof(ctx->depthstencil));

   /* rasterizer */
   memset(&ctx->rasterizer, 0, sizeof(ctx->rasterizer));
   ctx->rasterizer.front_winding = PIPE_WINDING_CW;
   ctx->rasterizer.cull_mode = PIPE_WINDING_NONE;
   ctx->rasterizer.gl_rasterization_rules = 1;

   /* samplers */
   memset(&ctx->sampler, 0, sizeof(ctx->sampler));
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->sampler.min_img_filter = 0; /* set later */
   ctx->sampler.mag_img_filter = 0; /* set later */
   ctx->sampler.normalized_coords = 1;

   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                                    semantic_indexes);
   }

   /* fragment shader */
   ctx->fs[TGSI_WRITEMASK_XYZW] =
      util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_2D);
   ctx->vbuf = NULL;

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
   unsigned i;

   pipe->delete_vs_state(pipe, ctx->vs);

   for (i = 0; i < Elements(ctx->fs); i++)
      if (ctx->fs[i])
         pipe->delete_fs_state(pipe, ctx->fs[i]);

   pipe_buffer_reference(&ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Get offset of next free slot in vertex buffer for quad vertices.
 */
static unsigned
get_next_slot( struct blit_state *ctx )
{
   const unsigned max_slots = 4096 / sizeof ctx->vertices;

   if (ctx->vbuf_slot >= max_slots) 
      util_blit_flush( ctx );

   if (!ctx->vbuf) {
      ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                     32,
                                     PIPE_BUFFER_USAGE_VERTEX,
                                     max_slots * sizeof ctx->vertices);
   }
   
   return ctx->vbuf_slot++ * sizeof ctx->vertices;
}
                               




/**
 * Setup vertex data for the textured quad we'll draw.
 * Note: y=0=top
 */
static unsigned
setup_vertex_data_tex(struct blit_state *ctx,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   unsigned offset;

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

   offset = get_next_slot( ctx );

   pipe_buffer_write_nooverlap(ctx->pipe->screen, ctx->vbuf,
                               offset, sizeof(ctx->vertices), ctx->vertices);

   return offset;
}


/**
 * \return TRUE if two regions overlap, FALSE otherwise
 */
static boolean
regions_overlap(int srcX0, int srcY0,
                int srcX1, int srcY1,
                int dstX0, int dstY0,
                int dstX1, int dstY1)
{
   if (MAX2(srcX0, srcX1) < MIN2(dstX0, dstX1))
      return FALSE; /* src completely left of dst */

   if (MAX2(dstX0, dstX1) < MIN2(srcX0, srcX1))
      return FALSE; /* dst completely left of src */

   if (MAX2(srcY0, srcY1) < MIN2(dstY0, dstY1))
      return FALSE; /* src completely above dst */

   if (MAX2(dstY0, dstY1) < MIN2(srcY0, srcY1))
      return FALSE; /* dst completely above src */

   return TRUE; /* some overlap */
}


/**
 * Copy pixel block from src surface to dst surface.
 * Overlapping regions are acceptable.
 * Flipping and stretching are supported.
 * \param filter  one of PIPE_TEX_MIPFILTER_NEAREST/LINEAR
 * \param writemask  controls which channels in the dest surface are sourced
 *                   from the src surface.  Disabled channels are sourced
 *                   from (0,0,0,1).
 * XXX need some control over blitting Z and/or stencil.
 */
void
util_blit_pixels_writemask(struct blit_state *ctx,
                           struct pipe_surface *src,
                           int srcX0, int srcY0,
                           int srcX1, int srcY1,
                           struct pipe_surface *dst,
                           int dstX0, int dstY0,
                           int dstX1, int dstY1,
                           float z, uint filter,
                           uint writemask)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *tex = NULL;
   struct pipe_framebuffer_state fb;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   unsigned offset;
   boolean overlap;
   float s0, t0, s1, t1;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(screen->is_format_supported(screen, src->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

   /* do the regions overlap? */
   overlap = util_same_surface(src, dst) &&
      regions_overlap(srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1);

   /*
    * Check for simple case:  no format conversion, no flipping, no stretching,
    * no overlapping.
    * Filter mode should not matter since there's no stretching.
    */
   if (pipe->surface_copy &&
       dst->format == src->format &&
       srcX0 < srcX1 &&
       dstX0 < dstX1 &&
       srcY0 < srcY1 &&
       dstY0 < dstY1 &&
       (dstX1 - dstX0) == (srcX1 - srcX0) &&
       (dstY1 - dstY0) == (srcY1 - srcY0) &&
       !overlap) {
      pipe->surface_copy(pipe,
			 dst, dstX0, dstY0, /* dest */
			 src, srcX0, srcY0, /* src */
			 srcW, srcH);       /* size */
      return;
   }
   
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

   /* Create a temporary texture when src and dest alias or when src
    * is anything other than a single-level 2d texture.
    * 
    * This can still be improved upon.
    */
   if (util_same_surface(src, dst) ||
       src->texture->target != PIPE_TEXTURE_2D ||
       src->texture->last_level != 0)
   {
      struct pipe_texture texTemp;
      struct pipe_surface *texSurf;
      const int srcLeft = MIN2(srcX0, srcX1);
      const int srcTop = MIN2(srcY0, srcY1);

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

      /* create temp texture */
      memset(&texTemp, 0, sizeof(texTemp));
      texTemp.target = PIPE_TEXTURE_2D;
      texTemp.format = src->format;
      texTemp.last_level = 0;
      texTemp.width0 = srcW;
      texTemp.height0 = srcH;
      texTemp.depth0 = 1;

      tex = screen->texture_create(screen, &texTemp);
      if (!tex)
         return;

      texSurf = screen->get_tex_surface(screen, tex, 0, 0, 0, 
                                        PIPE_BUFFER_USAGE_GPU_WRITE);

      /* load temp texture */
      if (pipe->surface_copy) {
         pipe->surface_copy(pipe,
                            texSurf, 0, 0,   /* dest */
                            src, srcLeft, srcTop, /* src */
                            srcW, srcH);     /* size */
      } else {
         util_surface_copy(pipe, FALSE,
                           texSurf, 0, 0,   /* dest */
                           src, srcLeft, srcTop, /* src */
                           srcW, srcH);     /* size */
      }

      /* free the surface, update the texture if necessary.
       */
      pipe_surface_reference(&texSurf, NULL);
      s0 = 0.0f; 
      s1 = 1.0f;
      t0 = 0.0f;
      t1 = 1.0f;
   }
   else {
      pipe_texture_reference(&tex, src->texture);
      s0 = srcX0 / (float)tex->width0;
      s1 = srcX1 / (float)tex->width0;
      t0 = srcY0 / (float)tex->height0;
      t1 = srcY1 / (float)tex->height0;
   }


   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_clip(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_clip(ctx->cso, &ctx->clip);

   /* sampler */
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso);

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * dst->width;
   ctx->viewport.scale[1] = 0.5f * dst->height;
   ctx->viewport.scale[2] = 0.5f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * dst->width;
   ctx->viewport.translate[1] = 0.5f * dst->height;
   ctx->viewport.translate[2] = 0.5f;
   ctx->viewport.translate[3] = 0.0f;
   cso_set_viewport(ctx->cso, &ctx->viewport);

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &tex);

   if (ctx->fs[writemask] == NULL)
      ctx->fs[writemask] =
         util_make_fragment_tex_shader_writemask(pipe, TGSI_TEXTURE_2D,
                                                 writemask);

   /* shaders */
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs[writemask]);
   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   offset = setup_vertex_data_tex(ctx,
                                  (float) dstX0 / dst->width * 2.0f - 1.0f,
                                  (float) dstY0 / dst->height * 2.0f - 1.0f,
                                  (float) dstX1 / dst->width * 2.0f - 1.0f,
                                  (float) dstY1 / dst->height * 2.0f - 1.0f,
                                  s0, t0,
                                  s1, t1,
                                  z);

   util_draw_vertex_buffer(ctx->pipe, ctx->vbuf, offset,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_viewport(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_clip(ctx->cso);

   pipe_texture_reference(&tex, NULL);
}


void
util_blit_pixels(struct blit_state *ctx,
                 struct pipe_surface *src,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter )
{
   util_blit_pixels_writemask( ctx, src, 
                               srcX0, srcY0,
                               srcX1, srcY1,
                               dst,
                               dstX0, dstY0,
                               dstX1, dstY1,
                               z, filter,
                               TGSI_WRITEMASK_XYZW );
}


/* Release vertex buffer at end of frame to avoid synchronous
 * rendering.
 */
void util_blit_flush( struct blit_state *ctx )
{
   pipe_buffer_reference(&ctx->vbuf, NULL);
   ctx->vbuf_slot = 0;
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
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;
   unsigned offset;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   s0 = srcX0 / (float)tex->width0;
   s1 = srcX1 / (float)tex->width0;
   t0 = srcY0 / (float)tex->height0;
   t1 = srcY1 / (float)tex->height0;

   assert(ctx->pipe->screen->is_format_supported(ctx->pipe->screen, dst->format,
                                                 PIPE_TEXTURE_2D,
                                                 PIPE_TEXTURE_USAGE_RENDER_TARGET,
                                                 0));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_clip(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_clip(ctx->cso, &ctx->clip);

   /* sampler */
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso);

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * dst->width;
   ctx->viewport.scale[1] = 0.5f * dst->height;
   ctx->viewport.scale[2] = 0.5f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * dst->width;
   ctx->viewport.translate[1] = 0.5f * dst->height;
   ctx->viewport.translate[2] = 0.5f;
   ctx->viewport.translate[3] = 0.0f;
   cso_set_viewport(ctx->cso, &ctx->viewport);

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &tex);

   /* shaders */
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs[TGSI_WRITEMASK_XYZW]);
   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   offset = setup_vertex_data_tex(ctx,
                                  (float) dstX0 / dst->width * 2.0f - 1.0f,
                                  (float) dstY0 / dst->height * 2.0f - 1.0f,
                                  (float) dstX1 / dst->width * 2.0f - 1.0f,
                                  (float) dstY1 / dst->height * 2.0f - 1.0f,
                                  s0, t0, s1, t1,
                                  z);

   util_draw_vertex_buffer(ctx->pipe, 
                           ctx->vbuf, offset,
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
   cso_restore_clip(ctx->cso);
}
