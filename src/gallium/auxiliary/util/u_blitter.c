/**************************************************************************
 *
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
 * Blitter utility to facilitate acceleration of the clear, surface_copy,
 * and surface_fill functions.
 *
 * @author Marek Olšák
 */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_blitter.h"
#include "util/u_draw_quad.h"
#include "util/u_pack_color.h"
#include "util/u_rect.h"
#include "util/u_simple_shaders.h"
#include "util/u_texture.h"

#define INVALID_PTR ((void*)~0)

struct blitter_context_priv
{
   struct blitter_context blitter;

   struct pipe_context *pipe; /**< pipe context */
   struct pipe_buffer *vbuf;  /**< quad */

   float vertices[4][2][4];   /**< {pos, color} or {pos, texcoord} */

   /* Templates for various state objects. */
   struct pipe_sampler_state template_sampler_state;

   /* Constant state objects. */
   /* Vertex shaders. */
   void *vs_col; /**< Vertex shader which passes {pos, color} to the output */
   void *vs_tex; /**< Vertex shader which passes {pos, texcoord} to the output.*/

   /* Fragment shaders. */
   /* FS which outputs a color to multiple color buffers. */
   void *fs_col[PIPE_MAX_COLOR_BUFS];

   /* FS which outputs a color from a texture,
      where the index is PIPE_TEXTURE_* to be sampled. */
   void *fs_texfetch_col[PIPE_MAX_TEXTURE_TYPES];

   /* FS which outputs a depth from a texture,
      where the index is PIPE_TEXTURE_* to be sampled. */
   void *fs_texfetch_depth[PIPE_MAX_TEXTURE_TYPES];

   /* Blend state. */
   void *blend_write_color;   /**< blend state with writemask of RGBA */
   void *blend_keep_color;    /**< blend state with writemask of 0 */

   /* Depth stencil alpha state. */
   void *dsa_write_depth_stencil;
   void *dsa_write_depth_keep_stencil;
   void *dsa_keep_depth_stencil;

   /* Sampler state for clamping to a miplevel. */
   void *sampler_state[PIPE_MAX_TEXTURE_LEVELS];

   /* Rasterizer state. */
   void *rs_state;

   /* Viewport state. */
   struct pipe_viewport_state viewport;

   /* Clip state. */
   struct pipe_clip_state clip;
};

struct blitter_context *util_blitter_create(struct pipe_context *pipe)
{
   struct blitter_context_priv *ctx;
   struct pipe_blend_state blend = { 0 };
   struct pipe_depth_stencil_alpha_state dsa = { { 0 } };
   struct pipe_rasterizer_state rs_state = { 0 };
   struct pipe_sampler_state *sampler_state;
   unsigned i;

   ctx = CALLOC_STRUCT(blitter_context_priv);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;

   /* init state objects for them to be considered invalid */
   ctx->blitter.saved_blend_state = INVALID_PTR;
   ctx->blitter.saved_dsa_state = INVALID_PTR;
   ctx->blitter.saved_rs_state = INVALID_PTR;
   ctx->blitter.saved_fs = INVALID_PTR;
   ctx->blitter.saved_vs = INVALID_PTR;
   ctx->blitter.saved_fb_state.nr_cbufs = ~0;
   ctx->blitter.saved_num_textures = ~0;
   ctx->blitter.saved_num_sampler_states = ~0;

   /* blend state objects */
   ctx->blend_keep_color = pipe->create_blend_state(pipe, &blend);

   blend.rt[0].colormask = PIPE_MASK_RGBA;
   ctx->blend_write_color = pipe->create_blend_state(pipe, &blend);

   /* depth stencil alpha state objects */
   ctx->dsa_keep_depth_stencil =
      pipe->create_depth_stencil_alpha_state(pipe, &dsa);

   dsa.depth.enabled = 1;
   dsa.depth.writemask = 1;
   dsa.depth.func = PIPE_FUNC_ALWAYS;
   ctx->dsa_write_depth_keep_stencil =
      pipe->create_depth_stencil_alpha_state(pipe, &dsa);

   dsa.stencil[0].enabled = 1;
   dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = 0xff;
   dsa.stencil[0].writemask = 0xff;
   ctx->dsa_write_depth_stencil =
      pipe->create_depth_stencil_alpha_state(pipe, &dsa);
   /* The DSA state objects which write depth and stencil are created
    * on-demand. */

   /* sampler state */
   sampler_state = &ctx->template_sampler_state;
   sampler_state->wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state->wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state->wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   /* The sampler state objects which sample from a specified mipmap level
    * are created on-demand. */

   /* rasterizer state */
   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.front_winding = PIPE_WINDING_CW;
   rs_state.cull_mode = PIPE_WINDING_NONE;
   rs_state.gl_rasterization_rules = 1;
   rs_state.flatshade = 1;
   ctx->rs_state = pipe->create_rasterizer_state(pipe, &rs_state);

   /* fragment shaders are created on-demand */

   /* vertex shaders */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR };
      const uint semantic_indices[] = { 0, 0 };
      ctx->vs_col =
         util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                             semantic_indices);
   }
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indices[] = { 0, 0 };
      ctx->vs_tex =
         util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                             semantic_indices);
   }

   /* set invariant vertex coordinates */
   for (i = 0; i < 4; i++)
      ctx->vertices[i][0][3] = 1; /*v.w*/

   /* create the vertex buffer */
   ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                  32,
                                  PIPE_BUFFER_USAGE_VERTEX,
                                  sizeof(ctx->vertices));

   return &ctx->blitter;
}

void util_blitter_destroy(struct blitter_context *blitter)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   int i;

   pipe->delete_blend_state(pipe, ctx->blend_write_color);
   pipe->delete_blend_state(pipe, ctx->blend_keep_color);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe,
                                          ctx->dsa_write_depth_keep_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);

   pipe->delete_rasterizer_state(pipe, ctx->rs_state);
   pipe->delete_vs_state(pipe, ctx->vs_col);
   pipe->delete_vs_state(pipe, ctx->vs_tex);

   for (i = 0; i < PIPE_MAX_TEXTURE_TYPES; i++) {
      if (ctx->fs_texfetch_col[i])
         pipe->delete_fs_state(pipe, ctx->fs_texfetch_col[i]);
      if (ctx->fs_texfetch_depth[i])
         pipe->delete_fs_state(pipe, ctx->fs_texfetch_depth[i]);
   }

   for (i = 0; i < PIPE_MAX_COLOR_BUFS && ctx->fs_col[i]; i++)
      if (ctx->fs_col[i])
         pipe->delete_fs_state(pipe, ctx->fs_col[i]);

   for (i = 0; i < PIPE_MAX_TEXTURE_LEVELS; i++)
      if (ctx->sampler_state[i])
         pipe->delete_sampler_state(pipe, ctx->sampler_state[i]);

   pipe_buffer_reference(&ctx->vbuf, NULL);
   FREE(ctx);
}

static void blitter_check_saved_CSOs(struct blitter_context_priv *ctx)
{
   /* make sure these CSOs have been saved */
   assert(ctx->blitter.saved_blend_state != INVALID_PTR &&
          ctx->blitter.saved_dsa_state != INVALID_PTR &&
          ctx->blitter.saved_rs_state != INVALID_PTR &&
          ctx->blitter.saved_fs != INVALID_PTR &&
          ctx->blitter.saved_vs != INVALID_PTR);
}

static void blitter_restore_CSOs(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   /* restore the state objects which are always required to be saved */
   pipe->bind_blend_state(pipe, ctx->blitter.saved_blend_state);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->blitter.saved_dsa_state);
   pipe->bind_rasterizer_state(pipe, ctx->blitter.saved_rs_state);
   pipe->bind_fs_state(pipe, ctx->blitter.saved_fs);
   pipe->bind_vs_state(pipe, ctx->blitter.saved_vs);

   ctx->blitter.saved_blend_state = INVALID_PTR;
   ctx->blitter.saved_dsa_state = INVALID_PTR;
   ctx->blitter.saved_rs_state = INVALID_PTR;
   ctx->blitter.saved_fs = INVALID_PTR;
   ctx->blitter.saved_vs = INVALID_PTR;

   pipe->set_stencil_ref(pipe, &ctx->blitter.saved_stencil_ref);

   pipe->set_viewport_state(pipe, &ctx->blitter.saved_viewport);
   pipe->set_clip_state(pipe, &ctx->blitter.saved_clip);

   /* restore the state objects which are required to be saved before copy/fill
    */
   if (ctx->blitter.saved_fb_state.nr_cbufs != ~0) {
      pipe->set_framebuffer_state(pipe, &ctx->blitter.saved_fb_state);
      ctx->blitter.saved_fb_state.nr_cbufs = ~0;
   }

   if (ctx->blitter.saved_num_sampler_states != ~0) {
      pipe->bind_fragment_sampler_states(pipe,
                                         ctx->blitter.saved_num_sampler_states,
                                         ctx->blitter.saved_sampler_states);
      ctx->blitter.saved_num_sampler_states = ~0;
   }

   if (ctx->blitter.saved_num_textures != ~0) {
      pipe->set_fragment_sampler_textures(pipe,
                                          ctx->blitter.saved_num_textures,
                                          ctx->blitter.saved_textures);
      ctx->blitter.saved_num_textures = ~0;
   }
}

static void blitter_set_rectangle(struct blitter_context_priv *ctx,
                                  unsigned x1, unsigned y1,
                                  unsigned x2, unsigned y2,
                                  unsigned width, unsigned height,
                                  float depth)
{
   int i;

   /* set vertex positions */
   ctx->vertices[0][0][0] = (float)x1 / width * 2.0f - 1.0f; /*v0.x*/
   ctx->vertices[0][0][1] = (float)y1 / height * 2.0f - 1.0f; /*v0.y*/

   ctx->vertices[1][0][0] = (float)x2 / width * 2.0f - 1.0f; /*v1.x*/
   ctx->vertices[1][0][1] = (float)y1 / height * 2.0f - 1.0f; /*v1.y*/

   ctx->vertices[2][0][0] = (float)x2 / width * 2.0f - 1.0f; /*v2.x*/
   ctx->vertices[2][0][1] = (float)y2 / height * 2.0f - 1.0f; /*v2.y*/

   ctx->vertices[3][0][0] = (float)x1 / width * 2.0f - 1.0f; /*v3.x*/
   ctx->vertices[3][0][1] = (float)y2 / height * 2.0f - 1.0f; /*v3.y*/

   for (i = 0; i < 4; i++)
      ctx->vertices[i][0][2] = depth; /*z*/

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * width;
   ctx->viewport.scale[1] = 0.5f * height;
   ctx->viewport.scale[2] = 1.0f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * width;
   ctx->viewport.translate[1] = 0.5f * height;
   ctx->viewport.translate[2] = 0.0f;
   ctx->viewport.translate[3] = 0.0f;
   ctx->pipe->set_viewport_state(ctx->pipe, &ctx->viewport);

   /* clip */
   ctx->pipe->set_clip_state(ctx->pipe, &ctx->clip);
}

static void blitter_set_clear_color(struct blitter_context_priv *ctx,
                                    const float *rgba)
{
   int i;

   for (i = 0; i < 4; i++) {
      ctx->vertices[i][1][0] = rgba[0];
      ctx->vertices[i][1][1] = rgba[1];
      ctx->vertices[i][1][2] = rgba[2];
      ctx->vertices[i][1][3] = rgba[3];
   }
}

static void blitter_set_texcoords_2d(struct blitter_context_priv *ctx,
                                     struct pipe_surface *surf,
                                     unsigned x1, unsigned y1,
                                     unsigned x2, unsigned y2)
{
   int i;
   float s1 = x1 / (float)surf->width;
   float t1 = y1 / (float)surf->height;
   float s2 = x2 / (float)surf->width;
   float t2 = y2 / (float)surf->height;

   ctx->vertices[0][1][0] = s1; /*t0.s*/
   ctx->vertices[0][1][1] = t1; /*t0.t*/

   ctx->vertices[1][1][0] = s2; /*t1.s*/
   ctx->vertices[1][1][1] = t1; /*t1.t*/

   ctx->vertices[2][1][0] = s2; /*t2.s*/
   ctx->vertices[2][1][1] = t2; /*t2.t*/

   ctx->vertices[3][1][0] = s1; /*t3.s*/
   ctx->vertices[3][1][1] = t2; /*t3.t*/

   for (i = 0; i < 4; i++) {
      ctx->vertices[i][1][2] = 0; /*r*/
      ctx->vertices[i][1][3] = 1; /*q*/
   }
}

static void blitter_set_texcoords_3d(struct blitter_context_priv *ctx,
                                     struct pipe_surface *surf,
                                     unsigned x1, unsigned y1,
                                     unsigned x2, unsigned y2)
{
   int i;
   float depth = u_minify(surf->texture->depth0, surf->level);
   float r = surf->zslice / depth;

   blitter_set_texcoords_2d(ctx, surf, x1, y1, x2, y2);

   for (i = 0; i < 4; i++)
      ctx->vertices[i][1][2] = r; /*r*/
}

static void blitter_set_texcoords_cube(struct blitter_context_priv *ctx,
                                       struct pipe_surface *surf,
                                       unsigned x1, unsigned y1,
                                       unsigned x2, unsigned y2)
{
   int i;
   float s1 = x1 / (float)surf->width;
   float t1 = y1 / (float)surf->height;
   float s2 = x2 / (float)surf->width;
   float t2 = y2 / (float)surf->height;
   float st[4][2];

   st[0][0] = s1;
   st[0][1] = t1;
   st[1][0] = s2;
   st[1][1] = t1;
   st[2][0] = s2;
   st[2][1] = t2;
   st[3][0] = s1;
   st[3][1] = t2;

   util_map_texcoords2d_onto_cubemap(surf->face,
                                     /* pointer, stride in floats */
                                     &st[0][0], 2,
                                     &ctx->vertices[0][1][0], 8);

   for (i = 0; i < 4; i++)
      ctx->vertices[i][1][3] = 1; /*q*/
}

static void blitter_draw_quad(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   /* write vertices and draw them */
   pipe_buffer_write(pipe->screen, ctx->vbuf,
                     0, sizeof(ctx->vertices), ctx->vertices);

   util_draw_vertex_buffer(pipe, ctx->vbuf, 0, PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */
}

static INLINE
void **blitter_get_sampler_state(struct blitter_context_priv *ctx,
                                 int miplevel)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_sampler_state *sampler_state = &ctx->template_sampler_state;

   assert(miplevel < PIPE_MAX_TEXTURE_LEVELS);

   /* Create the sampler state on-demand. */
   if (!ctx->sampler_state[miplevel]) {
      sampler_state->lod_bias = miplevel;
      sampler_state->min_lod = miplevel;
      sampler_state->max_lod = miplevel;

      ctx->sampler_state[miplevel] = pipe->create_sampler_state(pipe,
                                                                sampler_state);
   }

   /* Return void** so that it can be passed to bind_fragment_sampler_states
    * directly. */
   return &ctx->sampler_state[miplevel];
}

static INLINE
void *blitter_get_fs_col(struct blitter_context_priv *ctx, unsigned num_cbufs)
{
   struct pipe_context *pipe = ctx->pipe;
   unsigned index = num_cbufs ? num_cbufs - 1 : 0;

   assert(num_cbufs <= PIPE_MAX_COLOR_BUFS);

   if (!ctx->fs_col[index])
      ctx->fs_col[index] =
         util_make_fragment_clonecolor_shader(pipe, num_cbufs);

   return ctx->fs_col[index];
}

static INLINE
void *blitter_get_fs_texfetch_col(struct blitter_context_priv *ctx,
                                  unsigned tex_target)
{
   struct pipe_context *pipe = ctx->pipe;

   assert(tex_target < PIPE_MAX_TEXTURE_TYPES);

   /* Create the fragment shader on-demand. */
   if (!ctx->fs_texfetch_col[tex_target]) {
      switch (tex_target) {
         case PIPE_TEXTURE_1D:
            ctx->fs_texfetch_col[PIPE_TEXTURE_1D] =
               util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_1D);
            break;
         case PIPE_TEXTURE_2D:
            ctx->fs_texfetch_col[PIPE_TEXTURE_2D] =
               util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_2D);
            break;
         case PIPE_TEXTURE_3D:
            ctx->fs_texfetch_col[PIPE_TEXTURE_3D] =
               util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_3D);
            break;
         case PIPE_TEXTURE_CUBE:
            ctx->fs_texfetch_col[PIPE_TEXTURE_CUBE] =
               util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_CUBE);
            break;
         default:;
      }
   }

   return ctx->fs_texfetch_col[tex_target];
}

static INLINE
void *blitter_get_fs_texfetch_depth(struct blitter_context_priv *ctx,
                                    unsigned tex_target)
{
   struct pipe_context *pipe = ctx->pipe;

   assert(tex_target < PIPE_MAX_TEXTURE_TYPES);

   /* Create the fragment shader on-demand. */
   if (!ctx->fs_texfetch_depth[tex_target]) {
      switch (tex_target) {
         case PIPE_TEXTURE_1D:
            ctx->fs_texfetch_depth[PIPE_TEXTURE_1D] =
               util_make_fragment_tex_shader_writedepth(pipe, TGSI_TEXTURE_1D);
            break;
         case PIPE_TEXTURE_2D:
            ctx->fs_texfetch_depth[PIPE_TEXTURE_2D] =
               util_make_fragment_tex_shader_writedepth(pipe, TGSI_TEXTURE_2D);
            break;
         case PIPE_TEXTURE_3D:
            ctx->fs_texfetch_depth[PIPE_TEXTURE_3D] =
               util_make_fragment_tex_shader_writedepth(pipe, TGSI_TEXTURE_3D);
            break;
         case PIPE_TEXTURE_CUBE:
            ctx->fs_texfetch_depth[PIPE_TEXTURE_CUBE] =
               util_make_fragment_tex_shader_writedepth(pipe,TGSI_TEXTURE_CUBE);
            break;
         default:;
      }
   }

   return ctx->fs_texfetch_depth[tex_target];
}

void util_blitter_clear(struct blitter_context *blitter,
                        unsigned width, unsigned height,
                        unsigned num_cbufs,
                        unsigned clear_buffers,
                        const float *rgba,
                        double depth, unsigned stencil)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_stencil_ref sr = { { 0 } };

   assert(num_cbufs <= PIPE_MAX_COLOR_BUFS);

   blitter_check_saved_CSOs(ctx);

   /* bind CSOs */
   if (clear_buffers & PIPE_CLEAR_COLOR)
      pipe->bind_blend_state(pipe, ctx->blend_write_color);
   else
      pipe->bind_blend_state(pipe, ctx->blend_keep_color);

   if (clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      sr.ref_value[0] = stencil & 0xff;
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
      pipe->set_stencil_ref(pipe, &sr);
   }
   else
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);

   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, num_cbufs));
   pipe->bind_vs_state(pipe, ctx->vs_col);

   blitter_set_clear_color(ctx, rgba);
   blitter_set_rectangle(ctx, 0, 0, width, height, width, height, depth);
   blitter_draw_quad(ctx);
   blitter_restore_CSOs(ctx);
}

static boolean
is_overlap(unsigned sx1, unsigned sx2, unsigned sy1, unsigned sy2,
           unsigned dx1, unsigned dx2, unsigned dy1, unsigned dy2)
{
    if (sx1 >= dx2 || sx2 <= dx1 || sy1 >= dy2 || sy2 <= dy1) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static void util_blitter_do_copy(struct blitter_context *blitter,
				 struct pipe_surface *dst,
				 unsigned dstx, unsigned dsty,
				 struct pipe_surface *src,
				 unsigned srcx, unsigned srcy,
				 unsigned width, unsigned height,
				 boolean is_depth)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_framebuffer_state fb_state;

   assert(blitter->saved_fb_state.nr_cbufs != ~0);
   assert(blitter->saved_num_textures != ~0);
   assert(blitter->saved_num_sampler_states != ~0);
   assert(src->texture->target < PIPE_MAX_TEXTURE_TYPES);

   /* bind CSOs */
   fb_state.width = dst->width;
   fb_state.height = dst->height;

   if (is_depth) {
      pipe->bind_blend_state(pipe, ctx->blend_keep_color);
      pipe->bind_depth_stencil_alpha_state(pipe,
                                           ctx->dsa_write_depth_keep_stencil);
      pipe->bind_fs_state(pipe,
         blitter_get_fs_texfetch_depth(ctx, src->texture->target));

      fb_state.nr_cbufs = 0;
      fb_state.zsbuf = dst;
   } else {
      pipe->bind_blend_state(pipe, ctx->blend_write_color);
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
      pipe->bind_fs_state(pipe,
         blitter_get_fs_texfetch_col(ctx, src->texture->target));

      fb_state.nr_cbufs = 1;
      fb_state.cbufs[0] = dst;
      fb_state.zsbuf = 0;
   }

   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_vs_state(pipe, ctx->vs_tex);
   pipe->bind_fragment_sampler_states(pipe, 1,
      blitter_get_sampler_state(ctx, src->level));
   pipe->set_fragment_sampler_textures(pipe, 1, &src->texture);
   pipe->set_framebuffer_state(pipe, &fb_state);

   /* set texture coordinates */
   switch (src->texture->target) {
      case PIPE_TEXTURE_1D:
      case PIPE_TEXTURE_2D:
         blitter_set_texcoords_2d(ctx, src, srcx, srcy,
                                  srcx+width, srcy+height);
         break;
      case PIPE_TEXTURE_3D:
         blitter_set_texcoords_3d(ctx, src, srcx, srcy,
                                  srcx+width, srcy+height);
         break;
      case PIPE_TEXTURE_CUBE:
         blitter_set_texcoords_cube(ctx, src, srcx, srcy,
                                    srcx+width, srcy+height);
         break;
      default:
         assert(0);
   }

   blitter_set_rectangle(ctx, dstx, dsty, dstx+width, dsty+height, dst->width, dst->height, 0);
   blitter_draw_quad(ctx);

}

static void util_blitter_overlap_copy(struct blitter_context *blitter,
				      struct pipe_surface *dst,
				      unsigned dstx, unsigned dsty,
				      struct pipe_surface *src,
				      unsigned srcx, unsigned srcy,
				      unsigned width, unsigned height)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;

   struct pipe_texture texTemp;
   struct pipe_texture *texture;
   struct pipe_surface *tex_surf;

   /* check whether the states are properly saved */
   blitter_check_saved_CSOs(ctx);

   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = dst->texture->format; /* XXX verify supported by driver! */
   texTemp.last_level = 0;
   texTemp.width0 = width;
   texTemp.height0 = height;
   texTemp.depth0 = 1;

   texture = screen->texture_create(screen, &texTemp);
   if (!texture)
      return;

   tex_surf = screen->get_tex_surface(screen, texture, 0, 0, 0,
				      PIPE_BUFFER_USAGE_GPU_READ | 
				      PIPE_BUFFER_USAGE_GPU_WRITE);

   /* blit from the src to the temp */
   util_blitter_do_copy(blitter, tex_surf, 0, 0,
			src, srcx, srcy,
			width, height,
			FALSE);
   util_blitter_do_copy(blitter, dst, dstx, dsty,
			tex_surf, 0, 0,
			width, height,
			FALSE);
   pipe_surface_reference(&tex_surf, NULL);
   pipe_texture_reference(&texture, NULL);
   blitter_restore_CSOs(ctx);
}

void util_blitter_copy(struct blitter_context *blitter,
                       struct pipe_surface *dst,
                       unsigned dstx, unsigned dsty,
                       struct pipe_surface *src,
                       unsigned srcx, unsigned srcy,
                       unsigned width, unsigned height,
                       boolean ignore_stencil)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   boolean is_stencil, is_depth;
   unsigned dst_tex_usage;

   /* give up if textures are not set */
   assert(dst->texture && src->texture);
   if (!dst->texture || !src->texture)
      return;

   if (dst->texture == src->texture) {
      if (is_overlap(srcx, srcx + width, srcy, srcy + height,
		             dstx, dstx + width, dsty, dsty + height)) {
         util_blitter_overlap_copy(blitter, dst, dstx, dsty, src, srcx, srcy,
                                   width, height);
         return;
      }
   }
		   
   is_depth = util_format_get_component_bits(src->format, UTIL_FORMAT_COLORSPACE_ZS, 0) != 0;
   is_stencil = util_format_get_component_bits(src->format, UTIL_FORMAT_COLORSPACE_ZS, 1) != 0;
   dst_tex_usage = is_depth || is_stencil ? PIPE_TEXTURE_USAGE_DEPTH_STENCIL :
                                            PIPE_TEXTURE_USAGE_RENDER_TARGET;

   /* check if we can sample from and render to the surfaces */
   /* (assuming copying a stencil buffer is not possible) */
   if ((!ignore_stencil && is_stencil) ||
       !screen->is_format_supported(screen, dst->format, dst->texture->target,
                                    dst_tex_usage, 0) ||
       !screen->is_format_supported(screen, src->format, src->texture->target,
                                    PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      util_surface_copy(pipe, FALSE, dst, dstx, dsty, src, srcx, srcy,
                        width, height);
      return;
   }

   /* check whether the states are properly saved */
   blitter_check_saved_CSOs(ctx);
   util_blitter_do_copy(blitter,
			dst, dstx, dsty,
			src, srcx, srcy,
			width, height, is_depth);
   blitter_restore_CSOs(ctx);
}

void util_blitter_fill(struct blitter_context *blitter,
                       struct pipe_surface *dst,
                       unsigned dstx, unsigned dsty,
                       unsigned width, unsigned height,
                       unsigned value)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb_state;
   float rgba[4];
   ubyte ub_rgba[4] = {0};
   union util_color color;
   int i;

   assert(dst->texture);
   if (!dst->texture)
      return;

   /* check if we can render to the surface */
   if (util_format_is_depth_or_stencil(dst->format) || /* unlikely, but you never know */
       !screen->is_format_supported(screen, dst->format, dst->texture->target,
                                    PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
      util_surface_fill(pipe, dst, dstx, dsty, width, height, value);
      return;
   }

   /* unpack the color */
   color.ui = value;
   util_unpack_color_ub(dst->format, &color,
                        ub_rgba, ub_rgba+1, ub_rgba+2, ub_rgba+3);
   for (i = 0; i < 4; i++)
      rgba[i] = ubyte_to_float(ub_rgba[i]);

   /* check the saved state */
   blitter_check_saved_CSOs(ctx);
   assert(blitter->saved_fb_state.nr_cbufs != ~0);

   /* bind CSOs */
   pipe->bind_blend_state(pipe, ctx->blend_write_color);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, 1));
   pipe->bind_vs_state(pipe, ctx->vs_col);

   /* set a framebuffer state */
   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dst;
   fb_state.zsbuf = 0;
   pipe->set_framebuffer_state(pipe, &fb_state);

   blitter_set_clear_color(ctx, rgba);
   blitter_set_rectangle(ctx, 0, 0, width, height, dst->width, dst->height, 0);
   blitter_draw_quad(ctx);
   blitter_restore_CSOs(ctx);
}
