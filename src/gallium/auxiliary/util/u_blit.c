/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
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
#include "util/u_sampler.h"
#include "util/u_texture.h"
#include "util/u_simple_shaders.h"

#include "cso_cache/cso_context.h"


struct blit_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend_write_color;
   struct pipe_depth_stencil_alpha_state dsa_keep_depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_element velem[2];

   void *vs;
   void *fs[PIPE_MAX_TEXTURE_TYPES][TGSI_WRITEMASK_XYZW + 1];

   struct pipe_resource *vbuf;  /**< quad vertices */
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
   ctx->blend_write_color.rt[0].colormask = PIPE_MASK_RGBA;

   /* rasterizer */
   ctx->rasterizer.cull_face = PIPE_FACE_NONE;
   ctx->rasterizer.half_pixel_center = 1;
   ctx->rasterizer.bottom_edge_rule = 1;
   ctx->rasterizer.depth_clip = 1;

   /* samplers */
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->sampler.min_img_filter = 0; /* set later */
   ctx->sampler.mag_img_filter = 0; /* set later */

   /* vertex elements state */
   for (i = 0; i < 2; i++) {
      ctx->velem[i].src_offset = i * 4 * sizeof(float);
      ctx->velem[i].instance_divisor = 0;
      ctx->velem[i].vertex_buffer_index = cso_get_aux_vertex_buffer_slot(cso);
      ctx->velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }

   ctx->vbuf = NULL;

   /* init vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][3] = 1.0f; /* w */
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
   unsigned i, j;

   if (ctx->vs)
      pipe->delete_vs_state(pipe, ctx->vs);

   for (i = 0; i < Elements(ctx->fs); i++) {
      for (j = 0; j < Elements(ctx->fs[i]); j++) {
         if (ctx->fs[i][j])
            pipe->delete_fs_state(pipe, ctx->fs[i][j]);
      }
   }

   pipe_resource_reference(&ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct blit_state *ctx, uint writemask,
                    enum pipe_texture_target pipe_tex)
{
   if (!ctx->fs[pipe_tex][writemask]) {
      unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex, 0);

      ctx->fs[pipe_tex][writemask] =
         util_make_fragment_tex_shader_writemask(ctx->pipe, tgsi_tex,
                                                 TGSI_INTERPOLATE_LINEAR,
                                                 writemask);
   }

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs[pipe_tex][writemask]);
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct blit_state *ctx)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!ctx->vs) {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(ctx->pipe, 2,
                                                    semantic_names,
                                                    semantic_indexes);
   }

   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);
}


/**
 * Get offset of next free slot in vertex buffer for quad vertices.
 */
static unsigned
get_next_slot( struct blit_state *ctx )
{
   const unsigned max_slots = 4096 / sizeof ctx->vertices;

   if (ctx->vbuf_slot >= max_slots) {
      pipe_resource_reference(&ctx->vbuf, NULL);
      ctx->vbuf_slot = 0;
   }

   if (!ctx->vbuf) {
      ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                     PIPE_BIND_VERTEX_BUFFER,
                                     PIPE_USAGE_STREAM,
                                     max_slots * sizeof ctx->vertices);
   }
   
   return ctx->vbuf_slot++ * sizeof ctx->vertices;
}




/**
 * Setup vertex data for the textured quad we'll draw.
 * Note: y=0=top
 *
 * FIXME: We should call util_map_texcoords2d_onto_cubemap
 * for cubemaps.
 */
static unsigned
setup_vertex_data_tex(struct blit_state *ctx,
                      unsigned src_target,
                      unsigned src_face,
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
   ctx->vertices[0][1][2] = 0;  /*r*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = s1; /*s*/
   ctx->vertices[1][1][1] = t0; /*t*/
   ctx->vertices[1][1][2] = 0;  /*r*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = s1;
   ctx->vertices[2][1][1] = t1;
   ctx->vertices[3][1][2] = 0;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = s0;
   ctx->vertices[3][1][1] = t1;
   ctx->vertices[3][1][2] = 0;

   if (src_target == PIPE_TEXTURE_CUBE ||
       src_target == PIPE_TEXTURE_CUBE_ARRAY) {
      /* Map cubemap texture coordinates inplace. */
      const unsigned stride = sizeof ctx->vertices[0] / sizeof ctx->vertices[0][0][0];
      util_map_texcoords2d_onto_cubemap(src_face,
                                        &ctx->vertices[0][1][0], stride,
                                        &ctx->vertices[0][1][0], stride,
                                        TRUE);
   }

   offset = get_next_slot( ctx );

   if (ctx->vbuf) {
      pipe_buffer_write_nooverlap(ctx->pipe, ctx->vbuf,
                                  offset, sizeof(ctx->vertices), ctx->vertices);
   }

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
 * Can we blit from src format to dest format with a simple copy?
 */
static boolean
formats_compatible(enum pipe_format src_format,
                   enum pipe_format dst_format)
{
   if (src_format == dst_format) {
      return TRUE;
   }
   else {
      const struct util_format_description *src_desc =
         util_format_description(src_format);
      const struct util_format_description *dst_desc =
         util_format_description(dst_format);
      return util_is_format_compatible(src_desc, dst_desc);
   }
}


/**
 * Copy pixel block from src surface to dst surface.
 * Overlapping regions are acceptable.
 * Flipping and stretching are supported.
 * \param filter  one of PIPE_TEX_MIPFILTER_NEAREST/LINEAR
 * \param writemask  controls which channels in the dest surface are sourced
 *                   from the src surface.  Disabled channels are sourced
 *                   from (0,0,0,1).
 */
void
util_blit_pixels(struct blit_state *ctx,
                 struct pipe_resource *src_tex,
                 unsigned src_level,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 int srcZ0,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter,
                 uint writemask, uint zs_writemask)
{
   struct pipe_context *pipe = ctx->pipe;
   enum pipe_format src_format, dst_format;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   boolean overlap;
   boolean is_stencil, is_depth, blit_depth, blit_stencil;
   const struct util_format_description *src_desc =
         util_format_description(src_tex->format);
   struct pipe_blit_info info;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(src_level <= src_tex->last_level);

   /* do the regions overlap? */
   overlap = src_tex == dst->texture &&
             dst->u.tex.level == src_level &&
             dst->u.tex.first_layer == srcZ0 &&
      regions_overlap(srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1);

   src_format = util_format_linear(src_tex->format);
   dst_format = util_format_linear(dst->texture->format);

   /* See whether we will blit depth or stencil. */
   is_depth = util_format_has_depth(src_desc);
   is_stencil = util_format_has_stencil(src_desc);

   blit_depth = is_depth && (zs_writemask & BLIT_WRITEMASK_Z);
   blit_stencil = is_stencil && (zs_writemask & BLIT_WRITEMASK_STENCIL);

   assert((writemask && !zs_writemask && !is_depth && !is_stencil) ||
          (!writemask && (blit_depth || blit_stencil)));

   /*
    * XXX: z parameter is deprecated. dst->u.tex.first_layer
    * specificies the destination layer.
    */
   assert(z == 0.0f);

   /*
    * Check for simple case:  no format conversion, no flipping, no stretching,
    * no overlapping, same number of samples.
    * Filter mode should not matter since there's no stretching.
    */
   if (formats_compatible(src_format, dst_format) &&
       src_tex->nr_samples == dst->texture->nr_samples &&
       is_stencil == blit_stencil &&
       is_depth == blit_depth &&
       srcX0 < srcX1 &&
       dstX0 < dstX1 &&
       srcY0 < srcY1 &&
       dstY0 < dstY1 &&
       (dstX1 - dstX0) == (srcX1 - srcX0) &&
       (dstY1 - dstY0) == (srcY1 - srcY0) &&
       !overlap) {
      struct pipe_box src_box;
      src_box.x = srcX0;
      src_box.y = srcY0;
      src_box.z = srcZ0;
      src_box.width = srcW;
      src_box.height = srcH;
      src_box.depth = 1;
      pipe->resource_copy_region(pipe,
                                 dst->texture, dst->u.tex.level,
                                 dstX0, dstY0, dst->u.tex.first_layer,/* dest */
                                 src_tex, src_level,
                                 &src_box);
      return;
   }

   memset(&info, 0, sizeof info);
   info.dst.resource = dst->texture;
   info.dst.level = dst->u.tex.level;
   info.dst.box.x = dstX0;
   info.dst.box.y = dstY0;
   info.dst.box.z = dst->u.tex.first_layer;
   info.dst.box.width = dstX1 - dstX0;
   info.dst.box.height = dstY1 - dstY0;
   assert(info.dst.box.width >= 0);
   assert(info.dst.box.height >= 0);
   info.dst.box.depth = 1;
   info.dst.format = dst->texture->format;
   info.src.resource = src_tex;
   info.src.level = src_level;
   info.src.box.x = srcX0;
   info.src.box.y = srcY0;
   info.src.box.z = srcZ0;
   info.src.box.width = srcX1 - srcX0;
   info.src.box.height = srcY1 - srcY0;
   info.src.box.depth = 1;
   info.src.format = src_tex->format;
   info.mask = writemask | (zs_writemask << 4);
   info.filter = filter;
   info.scissor_enable = 0;

   pipe->blit(pipe, &info);
}


/**
 * Copy pixel block from src sampler view to dst surface.
 *
 * The sampler view's first_level field indicates the source
 * mipmap level to use.
 *
 * The sampler view's first_layer indicate the layer to use, but for
 * cube maps it must point to the first face.  Face is passed in src_face.
 *
 * The main advantage over util_blit_pixels is that it allows to specify swizzles in
 * pipe_sampler_view::swizzle_?.
 *
 * But there is no control over blitting Z and/or stencil.
 */
void
util_blit_pixels_tex(struct blit_state *ctx,
                     struct pipe_sampler_view *src_sampler_view,
                     int srcX0, int srcY0,
                     int srcX1, int srcY1,
                     unsigned src_face,
                     struct pipe_surface *dst,
                     int dstX0, int dstY0,
                     int dstX1, int dstY1,
                     float z, uint filter)
{
   boolean normalized = src_sampler_view->texture->target != PIPE_TEXTURE_RECT;
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;
   unsigned offset;
   struct pipe_resource *tex = src_sampler_view->texture;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(tex);
   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   s0 = (float) srcX0;
   s1 = (float) srcX1;
   t0 = (float) srcY0;
   t1 = (float) srcY1;

   if(normalized)
   {
      /* normalize according to the mipmap level's size */
      int level = src_sampler_view->u.tex.first_level;
      float w = (float) u_minify(tex->width0, level);
      float h = (float) u_minify(tex->height0, level);
      s0 /= w;
      s1 /= w;
      t0 /= h;
      t1 /= h;
   }

   assert(ctx->pipe->screen->is_format_supported(ctx->pipe->screen, dst->format,
                                                 PIPE_TEXTURE_2D,
                                                 dst->texture->nr_samples,
                                                 PIPE_BIND_RENDER_TARGET));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_sample_mask(ctx->cso);
   cso_save_min_samples(ctx->cso);
   cso_save_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_stream_outputs(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_geometry_shader(ctx->cso);
   cso_save_vertex_elements(ctx->cso);
   cso_save_aux_vertex_buffer_slot(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend_write_color);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_keep_depthstencil);
   cso_set_sample_mask(ctx->cso, ~0);
   cso_set_min_samples(ctx->cso, 1);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_vertex_elements(ctx->cso, 2, ctx->velem);
   cso_set_stream_outputs(ctx->cso, 0, NULL, NULL);

   /* sampler */
   ctx->sampler.normalized_coords = normalized;
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso, PIPE_SHADER_FRAGMENT);

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
   cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, 1, &src_sampler_view);

   /* shaders */
   set_fragment_shader(ctx, TGSI_WRITEMASK_XYZW,
                       src_sampler_view->texture->target);
   set_vertex_shader(ctx);
   cso_set_geometry_shader_handle(ctx->cso, NULL);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   offset = setup_vertex_data_tex(ctx,
                                  src_sampler_view->texture->target,
                                  src_face,
                                  (float) dstX0 / dst->width * 2.0f - 1.0f,
                                  (float) dstY0 / dst->height * 2.0f - 1.0f,
                                  (float) dstX1 / dst->width * 2.0f - 1.0f,
                                  (float) dstY1 / dst->height * 2.0f - 1.0f,
                                  s0, t0, s1, t1,
                                  z);

   util_draw_vertex_buffer(ctx->pipe, ctx->cso, ctx->vbuf,
                           cso_get_aux_vertex_buffer_slot(ctx->cso),
                           offset,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_sample_mask(ctx->cso);
   cso_restore_min_samples(ctx->cso);
   cso_restore_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_viewport(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_geometry_shader(ctx->cso);
   cso_restore_vertex_elements(ctx->cso);
   cso_restore_aux_vertex_buffer_slot(ctx->cso);
   cso_restore_stream_outputs(ctx->cso);
}
