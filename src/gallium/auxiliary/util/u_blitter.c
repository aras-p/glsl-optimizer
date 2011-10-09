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
 * Blitter utility to facilitate acceleration of the clear, clear_render_target,
 * clear_depth_stencil, and resource_copy_region functions.
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
#include "util/u_sampler.h"
#include "util/u_simple_shaders.h"
#include "util/u_surface.h"
#include "util/u_texture.h"

#define INVALID_PTR ((void*)~0)

struct blitter_context_priv
{
   struct blitter_context base;

   struct pipe_resource *vbuf;  /**< quad */

   float vertices[4][2][4];   /**< {pos, color} or {pos, texcoord} */

   /* Templates for various state objects. */
   struct pipe_sampler_state template_sampler_state;

   /* Constant state objects. */
   /* Vertex shaders. */
   void *vs; /**< Vertex shader which passes {pos, generic} to the output.*/

   /* Fragment shaders. */
   /* The shader at index i outputs color to color buffers 0,1,...,i-1. */
   void *fs_col[PIPE_MAX_COLOR_BUFS+1];
   void *fs_col_int[PIPE_MAX_COLOR_BUFS+1];

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
   void *dsa_keep_depth_write_stencil;

   void *velem_state;
   void *velem_uint_state;
   void *velem_sint_state;

   /* Sampler state for clamping to a miplevel. */
   void *sampler_state[PIPE_MAX_TEXTURE_LEVELS * 2];

   /* Rasterizer state. */
   void *rs_state;

   /* Viewport state. */
   struct pipe_viewport_state viewport;

   /* Clip state. */
   struct pipe_clip_state clip;

   /* Destination surface dimensions. */
   unsigned dst_width;
   unsigned dst_height;

   boolean has_geometry_shader;
   boolean vertex_has_integers;
};

static void blitter_draw_rectangle(struct blitter_context *blitter,
                                   unsigned x, unsigned y,
                                   unsigned width, unsigned height,
                                   float depth,
                                   enum blitter_attrib_type type,
                                   const union pipe_color_union *attrib);


struct blitter_context *util_blitter_create(struct pipe_context *pipe)
{
   struct blitter_context_priv *ctx;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state *sampler_state;
   struct pipe_vertex_element velem[2];
   unsigned i;

   ctx = CALLOC_STRUCT(blitter_context_priv);
   if (!ctx)
      return NULL;

   ctx->base.pipe = pipe;
   ctx->base.draw_rectangle = blitter_draw_rectangle;

   /* init state objects for them to be considered invalid */
   ctx->base.saved_blend_state = INVALID_PTR;
   ctx->base.saved_dsa_state = INVALID_PTR;
   ctx->base.saved_rs_state = INVALID_PTR;
   ctx->base.saved_fs = INVALID_PTR;
   ctx->base.saved_vs = INVALID_PTR;
   ctx->base.saved_gs = INVALID_PTR;
   ctx->base.saved_velem_state = INVALID_PTR;
   ctx->base.saved_fb_state.nr_cbufs = ~0;
   ctx->base.saved_num_sampler_views = ~0;
   ctx->base.saved_num_sampler_states = ~0;
   ctx->base.saved_num_vertex_buffers = ~0;

   ctx->has_geometry_shader =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_GEOMETRY,
                                     PIPE_SHADER_CAP_MAX_INSTRUCTIONS) > 0;
   ctx->vertex_has_integers =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_GEOMETRY,
                                     PIPE_SHADER_CAP_INTEGERS);

   /* blend state objects */
   memset(&blend, 0, sizeof(blend));
   ctx->blend_keep_color = pipe->create_blend_state(pipe, &blend);

   blend.rt[0].colormask = PIPE_MASK_RGBA;
   ctx->blend_write_color = pipe->create_blend_state(pipe, &blend);

   /* depth stencil alpha state objects */
   memset(&dsa, 0, sizeof(dsa));
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


   dsa.depth.enabled = 0;
   dsa.depth.writemask = 0;
   ctx->dsa_keep_depth_write_stencil =
      pipe->create_depth_stencil_alpha_state(pipe, &dsa);

   /* sampler state */
   sampler_state = &ctx->template_sampler_state;
   sampler_state->wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state->wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state->wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state->normalized_coords = TRUE;
   /* The sampler state objects which sample from a specified mipmap level
    * are created on-demand. */

   /* rasterizer state */
   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.cull_face = PIPE_FACE_NONE;
   rs_state.gl_rasterization_rules = 1;
   rs_state.flatshade = 1;
   ctx->rs_state = pipe->create_rasterizer_state(pipe, &rs_state);

   /* vertex elements state */
   memset(&velem[0], 0, sizeof(velem[0]) * 2);
   for (i = 0; i < 2; i++) {
      velem[i].src_offset = i * 4 * sizeof(float);
      velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }
   ctx->velem_state = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);

   if (ctx->vertex_has_integers) {
      memset(&velem[0], 0, sizeof(velem[0]) * 2);
      for (i = 0; i < 2; i++) {
         velem[i].src_offset = i * 4 * sizeof(float);
         if (i == 0) {
            velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         } else {
            velem[i].src_format = PIPE_FORMAT_R32G32B32A32_SINT;
         }
      }
      ctx->velem_sint_state = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);

      memset(&velem[0], 0, sizeof(velem[0]) * 2);
      for (i = 0; i < 2; i++) {
         velem[i].src_offset = i * 4 * sizeof(float);
         if (i == 0) {
            velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         } else {
            velem[i].src_format = PIPE_FORMAT_R32G32B32A32_UINT;
         }
      }
      ctx->velem_uint_state = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);
   }

   /* fragment shaders are created on-demand */

   /* vertex shader */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indices[] = { 0, 0 };
      ctx->vs =
         util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                             semantic_indices);
   }

   /* set invariant vertex coordinates */
   for (i = 0; i < 4; i++)
      ctx->vertices[i][0][3] = 1; /*v.w*/

   /* create the vertex buffer */
   ctx->vbuf = pipe_user_buffer_create(ctx->base.pipe->screen,
                                       ctx->vertices,
                                       sizeof(ctx->vertices),
                                       PIPE_BIND_VERTEX_BUFFER);

   return &ctx->base;
}

void util_blitter_destroy(struct blitter_context *blitter)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = blitter->pipe;
   int i;

   pipe->delete_blend_state(pipe, ctx->blend_write_color);
   pipe->delete_blend_state(pipe, ctx->blend_keep_color);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe,
                                          ctx->dsa_write_depth_keep_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_write_stencil);

   pipe->delete_rasterizer_state(pipe, ctx->rs_state);
   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_vertex_elements_state(pipe, ctx->velem_state);
   if (ctx->vertex_has_integers) {
      pipe->delete_vertex_elements_state(pipe, ctx->velem_sint_state);
      pipe->delete_vertex_elements_state(pipe, ctx->velem_uint_state);
   }

   for (i = 0; i < PIPE_MAX_TEXTURE_TYPES; i++) {
      if (ctx->fs_texfetch_col[i])
         pipe->delete_fs_state(pipe, ctx->fs_texfetch_col[i]);
      if (ctx->fs_texfetch_depth[i])
         pipe->delete_fs_state(pipe, ctx->fs_texfetch_depth[i]);
   }

   for (i = 0; i <= PIPE_MAX_COLOR_BUFS; i++) {
      if (ctx->fs_col[i])
         pipe->delete_fs_state(pipe, ctx->fs_col[i]);
      if (ctx->fs_col_int[i])
         pipe->delete_fs_state(pipe, ctx->fs_col_int[i]);
   }

   for (i = 0; i < PIPE_MAX_TEXTURE_LEVELS * 2; i++)
      if (ctx->sampler_state[i])
         pipe->delete_sampler_state(pipe, ctx->sampler_state[i]);

   pipe_resource_reference(&ctx->vbuf, NULL);
   FREE(ctx);
}

static void blitter_set_running_flag(struct blitter_context_priv *ctx)
{
   if (ctx->base.running) {
      _debug_printf("u_blitter:%i: Caught recursion. This is a driver bug.\n",
                    __LINE__);
   }
   ctx->base.running = TRUE;
}

static void blitter_unset_running_flag(struct blitter_context_priv *ctx)
{
   if (!ctx->base.running) {
      _debug_printf("u_blitter:%i: Caught recursion. This is a driver bug.\n",
                    __LINE__);
   }
   ctx->base.running = FALSE;
}

static void blitter_check_saved_vertex_states(struct blitter_context_priv *ctx)
{
   assert(ctx->base.saved_num_vertex_buffers != ~0 &&
          ctx->base.saved_velem_state != INVALID_PTR &&
          ctx->base.saved_vs != INVALID_PTR &&
          (!ctx->has_geometry_shader || ctx->base.saved_gs != INVALID_PTR) &&
          ctx->base.saved_rs_state != INVALID_PTR);
}

static void blitter_restore_vertex_states(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->base.pipe;
   unsigned i;

   /* Vertex buffers. */
   pipe->set_vertex_buffers(pipe,
                            ctx->base.saved_num_vertex_buffers,
                            ctx->base.saved_vertex_buffers);

   for (i = 0; i < ctx->base.saved_num_vertex_buffers; i++) {
      if (ctx->base.saved_vertex_buffers[i].buffer) {
         pipe_resource_reference(&ctx->base.saved_vertex_buffers[i].buffer,
                                 NULL);
      }
   }
   ctx->base.saved_num_vertex_buffers = ~0;

   /* Vertex elements. */
   pipe->bind_vertex_elements_state(pipe, ctx->base.saved_velem_state);
   ctx->base.saved_velem_state = INVALID_PTR;

   /* Vertex shader. */
   pipe->bind_vs_state(pipe, ctx->base.saved_vs);
   ctx->base.saved_vs = INVALID_PTR;

   /* Geometry shader. */
   if (ctx->has_geometry_shader) {
      pipe->bind_gs_state(pipe, ctx->base.saved_gs);
      ctx->base.saved_gs = INVALID_PTR;
   }

   /* Rasterizer. */
   pipe->bind_rasterizer_state(pipe, ctx->base.saved_rs_state);
   ctx->base.saved_rs_state = INVALID_PTR;
}

static void blitter_check_saved_fragment_states(struct blitter_context_priv *ctx)
{
   assert(ctx->base.saved_fs != INVALID_PTR &&
          ctx->base.saved_dsa_state != INVALID_PTR &&
          ctx->base.saved_blend_state != INVALID_PTR);
}

static void blitter_restore_fragment_states(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->base.pipe;

   /* Fragment shader. */
   pipe->bind_fs_state(pipe, ctx->base.saved_fs);
   ctx->base.saved_fs = INVALID_PTR;

   /* Depth, stencil, alpha. */
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->base.saved_dsa_state);
   ctx->base.saved_dsa_state = INVALID_PTR;

   /* Blend state. */
   pipe->bind_blend_state(pipe, ctx->base.saved_blend_state);
   ctx->base.saved_blend_state = INVALID_PTR;

   /* Miscellaneous states. */
   /* XXX check whether these are saved and whether they need to be restored
    * (depending on the operation) */
   pipe->set_stencil_ref(pipe, &ctx->base.saved_stencil_ref);
   pipe->set_viewport_state(pipe, &ctx->base.saved_viewport);
   pipe->set_clip_state(pipe, &ctx->base.saved_clip);
}

static void blitter_check_saved_fb_state(struct blitter_context_priv *ctx)
{
   assert(ctx->base.saved_fb_state.nr_cbufs != ~0);
}

static void blitter_restore_fb_state(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->base.pipe;

   pipe->set_framebuffer_state(pipe, &ctx->base.saved_fb_state);
   util_unreference_framebuffer_state(&ctx->base.saved_fb_state);
}

static void blitter_check_saved_textures(struct blitter_context_priv *ctx)
{
   assert(ctx->base.saved_num_sampler_states != ~0 &&
          ctx->base.saved_num_sampler_views != ~0);
}

static void blitter_restore_textures(struct blitter_context_priv *ctx)
{
   struct pipe_context *pipe = ctx->base.pipe;
   unsigned i;

   /* Fragment sampler states. */
   pipe->bind_fragment_sampler_states(pipe,
                                      ctx->base.saved_num_sampler_states,
                                      ctx->base.saved_sampler_states);
   ctx->base.saved_num_sampler_states = ~0;

   /* Fragment sampler views. */
   pipe->set_fragment_sampler_views(pipe,
                                    ctx->base.saved_num_sampler_views,
                                    ctx->base.saved_sampler_views);

   for (i = 0; i < ctx->base.saved_num_sampler_views; i++)
      pipe_sampler_view_reference(&ctx->base.saved_sampler_views[i], NULL);

   ctx->base.saved_num_sampler_views = ~0;
}

static void blitter_set_rectangle(struct blitter_context_priv *ctx,
                                  unsigned x1, unsigned y1,
                                  unsigned x2, unsigned y2,
                                  float depth)
{
   int i;

   /* set vertex positions */
   ctx->vertices[0][0][0] = (float)x1 / ctx->dst_width * 2.0f - 1.0f; /*v0.x*/
   ctx->vertices[0][0][1] = (float)y1 / ctx->dst_height * 2.0f - 1.0f; /*v0.y*/

   ctx->vertices[1][0][0] = (float)x2 / ctx->dst_width * 2.0f - 1.0f; /*v1.x*/
   ctx->vertices[1][0][1] = (float)y1 / ctx->dst_height * 2.0f - 1.0f; /*v1.y*/

   ctx->vertices[2][0][0] = (float)x2 / ctx->dst_width * 2.0f - 1.0f; /*v2.x*/
   ctx->vertices[2][0][1] = (float)y2 / ctx->dst_height * 2.0f - 1.0f; /*v2.y*/

   ctx->vertices[3][0][0] = (float)x1 / ctx->dst_width * 2.0f - 1.0f; /*v3.x*/
   ctx->vertices[3][0][1] = (float)y2 / ctx->dst_height * 2.0f - 1.0f; /*v3.y*/

   for (i = 0; i < 4; i++)
      ctx->vertices[i][0][2] = depth; /*z*/

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * ctx->dst_width;
   ctx->viewport.scale[1] = 0.5f * ctx->dst_height;
   ctx->viewport.scale[2] = 1.0f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * ctx->dst_width;
   ctx->viewport.translate[1] = 0.5f * ctx->dst_height;
   ctx->viewport.translate[2] = 0.0f;
   ctx->viewport.translate[3] = 0.0f;
   ctx->base.pipe->set_viewport_state(ctx->base.pipe, &ctx->viewport);

   /* clip */
   ctx->base.pipe->set_clip_state(ctx->base.pipe, &ctx->clip);
}

static void blitter_set_clear_color(struct blitter_context_priv *ctx,
                                    const union pipe_color_union *color)
{
   int i;

   if (color) {
      for (i = 0; i < 4; i++) {
         uint32_t *uiverts = (uint32_t *)ctx->vertices[i][1];
         uiverts[0] = color->ui[0];
         uiverts[1] = color->ui[1];
         uiverts[2] = color->ui[2];
         uiverts[3] = color->ui[3];
      }
   } else {
      for (i = 0; i < 4; i++) {
         ctx->vertices[i][1][0] = 0;
         ctx->vertices[i][1][1] = 0;
         ctx->vertices[i][1][2] = 0;
         ctx->vertices[i][1][3] = 0;
      }
   }
}

static void get_texcoords(struct pipe_resource *src,
                          unsigned level,
                          unsigned x1, unsigned y1,
                          unsigned x2, unsigned y2,
                          boolean normalized, float out[4])
{
   if(normalized)
   {
      out[0] = x1 / (float)u_minify(src->width0,  level);
      out[1] = y1 / (float)u_minify(src->height0, level);
      out[2] = x2 / (float)u_minify(src->width0,  level);
      out[3] = y2 / (float)u_minify(src->height0, level);
   }
   else
   {
      out[0] = x1;
      out[1] = y1;
      out[2] = x2;
      out[3] = y2;
   }
}

static void set_texcoords_in_vertices(const float coord[4],
                                      float *out, unsigned stride)
{
   out[0] = coord[0]; /*t0.s*/
   out[1] = coord[1]; /*t0.t*/
   out += stride;
   out[0] = coord[2]; /*t1.s*/
   out[1] = coord[1]; /*t1.t*/
   out += stride;
   out[0] = coord[2]; /*t2.s*/
   out[1] = coord[3]; /*t2.t*/
   out += stride;
   out[0] = coord[0]; /*t3.s*/
   out[1] = coord[3]; /*t3.t*/
}

static void blitter_set_texcoords_2d(struct blitter_context_priv *ctx,
                                     struct pipe_resource *src,
                                     unsigned level,
                                     unsigned x1, unsigned y1,
                                     unsigned x2, unsigned y2)
{
   unsigned i;
   float coord[4];

   get_texcoords(src, level, x1, y1, x2, y2, TRUE, coord);
   set_texcoords_in_vertices(coord, &ctx->vertices[0][1][0], 8);

   for (i = 0; i < 4; i++) {
      ctx->vertices[i][1][2] = 0; /*r*/
      ctx->vertices[i][1][3] = 1; /*q*/
   }
}

static void blitter_set_texcoords_3d(struct blitter_context_priv *ctx,
                                     struct pipe_resource *src,
                                     unsigned level,
                                     unsigned zslice,
                                     unsigned x1, unsigned y1,
                                     unsigned x2, unsigned y2,
				     boolean normalized)
{
   int i;
   float r = normalized ? zslice / (float)u_minify(src->depth0, level) : zslice;

   blitter_set_texcoords_2d(ctx, src, level, x1, y1, x2, y2);

   for (i = 0; i < 4; i++)
      ctx->vertices[i][1][2] = r; /*r*/
}

static void blitter_set_texcoords_1d_array(struct blitter_context_priv *ctx,
                                           struct pipe_resource *src,
                                           unsigned level,
                                           unsigned zslice,
                                           unsigned x1, unsigned x2)
{
   int i;
   float r = zslice;

   blitter_set_texcoords_2d(ctx, src, level, x1, 0, x2, 0);

   for (i = 0; i < 4; i++)
      ctx->vertices[i][1][1] = r; /*r*/
}

static void blitter_set_texcoords_cube(struct blitter_context_priv *ctx,
                                       struct pipe_resource *src,
                                       unsigned level, unsigned face,
                                       unsigned x1, unsigned y1,
                                       unsigned x2, unsigned y2)
{
   int i;
   float coord[4];
   float st[4][2];

   get_texcoords(src, level, x1, y1, x2, y2, TRUE, coord);
   set_texcoords_in_vertices(coord, &st[0][0], 2);

   util_map_texcoords2d_onto_cubemap(face,
                                     /* pointer, stride in floats */
                                     &st[0][0], 2,
                                     &ctx->vertices[0][1][0], 8);

   for (i = 0; i < 4; i++)
      ctx->vertices[i][1][3] = 1; /*q*/
}

static void blitter_set_dst_dimensions(struct blitter_context_priv *ctx,
                                       unsigned width, unsigned height)
{
   ctx->dst_width = width;
   ctx->dst_height = height;
}

static INLINE
void **blitter_get_sampler_state(struct blitter_context_priv *ctx,
                                 int miplevel, boolean normalized)
{
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_sampler_state *sampler_state = &ctx->template_sampler_state;

   assert(miplevel < PIPE_MAX_TEXTURE_LEVELS);

   /* Create the sampler state on-demand. */
   if (!ctx->sampler_state[miplevel * 2 + normalized]) {
      sampler_state->lod_bias = miplevel;
      sampler_state->min_lod = miplevel;
      sampler_state->max_lod = miplevel;
      sampler_state->normalized_coords = normalized;

      ctx->sampler_state[miplevel * 2 + normalized] = pipe->create_sampler_state(pipe,
                                                                sampler_state);
   }

   /* Return void** so that it can be passed to bind_fragment_sampler_states
    * directly. */
   return &ctx->sampler_state[miplevel * 2 + normalized];
}

static INLINE
void *blitter_get_fs_col(struct blitter_context_priv *ctx, unsigned num_cbufs,
                         boolean int_format)
{
   struct pipe_context *pipe = ctx->base.pipe;

   assert(num_cbufs <= PIPE_MAX_COLOR_BUFS);

   if (int_format) {
      if (!ctx->fs_col_int[num_cbufs])
         ctx->fs_col_int[num_cbufs] =
            util_make_fragment_cloneinput_shader(pipe, num_cbufs,
                                                 TGSI_SEMANTIC_GENERIC,
                                                 TGSI_INTERPOLATE_CONSTANT);
      return ctx->fs_col_int[num_cbufs];
   } else {
      if (!ctx->fs_col[num_cbufs])
         ctx->fs_col[num_cbufs] =
            util_make_fragment_cloneinput_shader(pipe, num_cbufs,
                                                 TGSI_SEMANTIC_GENERIC,
                                                 TGSI_INTERPOLATE_LINEAR);
      return ctx->fs_col[num_cbufs];
   }
}

/** Convert PIPE_TEXTURE_x to TGSI_TEXTURE_x */
static unsigned
pipe_tex_to_tgsi_tex(enum pipe_texture_target pipe_tex_target)
{
   switch (pipe_tex_target) {
   case PIPE_TEXTURE_1D:
      return TGSI_TEXTURE_1D;
   case PIPE_TEXTURE_2D:
      return TGSI_TEXTURE_2D;
   case PIPE_TEXTURE_RECT:
      return TGSI_TEXTURE_RECT;
   case PIPE_TEXTURE_3D:
      return TGSI_TEXTURE_3D;
   case PIPE_TEXTURE_CUBE:
      return TGSI_TEXTURE_CUBE;
   case PIPE_TEXTURE_1D_ARRAY:
      return TGSI_TEXTURE_1D_ARRAY;
   case PIPE_TEXTURE_2D_ARRAY:
      return TGSI_TEXTURE_2D_ARRAY;
   default:
      assert(0 && "unexpected texture target");
      return TGSI_TEXTURE_UNKNOWN;
   }
}


static INLINE
void *blitter_get_fs_texfetch_col(struct blitter_context_priv *ctx,
                                  unsigned tex_target)
{
   struct pipe_context *pipe = ctx->base.pipe;

   assert(tex_target < PIPE_MAX_TEXTURE_TYPES);

   /* Create the fragment shader on-demand. */
   if (!ctx->fs_texfetch_col[tex_target]) {
      unsigned tgsi_tex = pipe_tex_to_tgsi_tex(tex_target);

      ctx->fs_texfetch_col[tex_target] =
        util_make_fragment_tex_shader(pipe, tgsi_tex, TGSI_INTERPOLATE_LINEAR);
   }

   return ctx->fs_texfetch_col[tex_target];
}

static INLINE
void *blitter_get_fs_texfetch_depth(struct blitter_context_priv *ctx,
                                    unsigned tex_target)
{
   struct pipe_context *pipe = ctx->base.pipe;

   assert(tex_target < PIPE_MAX_TEXTURE_TYPES);

   /* Create the fragment shader on-demand. */
   if (!ctx->fs_texfetch_depth[tex_target]) {
      unsigned tgsi_tex = pipe_tex_to_tgsi_tex(tex_target);

      ctx->fs_texfetch_depth[tex_target] =
         util_make_fragment_tex_shader_writedepth(pipe, tgsi_tex,
                                                  TGSI_INTERPOLATE_LINEAR);
   }

   return ctx->fs_texfetch_depth[tex_target];
}

static void blitter_draw_rectangle(struct blitter_context *blitter,
                                   unsigned x1, unsigned y1,
                                   unsigned x2, unsigned y2,
                                   float depth,
                                   enum blitter_attrib_type type,
                                   const union pipe_color_union *attrib)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;

   switch (type) {
      case UTIL_BLITTER_ATTRIB_COLOR:
         blitter_set_clear_color(ctx, attrib);
         break;

      case UTIL_BLITTER_ATTRIB_TEXCOORD:
         set_texcoords_in_vertices(attrib->f, &ctx->vertices[0][1][0], 8);
         break;

      default:;
   }

   blitter_set_rectangle(ctx, x1, y1, x2, y2, depth);
   ctx->base.pipe->redefine_user_buffer(ctx->base.pipe, ctx->vbuf,
                                        0, ctx->vbuf->width0);
   util_draw_vertex_buffer(ctx->base.pipe, NULL, ctx->vbuf, 0,
                           PIPE_PRIM_TRIANGLE_FAN, 4, 2);
}

static void util_blitter_clear_custom(struct blitter_context *blitter,
                                      unsigned width, unsigned height,
                                      unsigned num_cbufs,
                                      unsigned clear_buffers,
                                      enum pipe_format cbuf_format,
                                      const union pipe_color_union *color,
                                      double depth, unsigned stencil,
                                      void *custom_blend, void *custom_dsa)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_stencil_ref sr = { { 0 } };
   boolean int_format = util_format_is_pure_integer(cbuf_format);
   assert(num_cbufs <= PIPE_MAX_COLOR_BUFS);

   blitter_set_running_flag(ctx);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);

   /* bind states */
   if (custom_blend) {
      pipe->bind_blend_state(pipe, custom_blend);
   } else if (clear_buffers & PIPE_CLEAR_COLOR) {
      pipe->bind_blend_state(pipe, ctx->blend_write_color);
   } else {
      pipe->bind_blend_state(pipe, ctx->blend_keep_color);
   }

   if (custom_dsa) {
      pipe->bind_depth_stencil_alpha_state(pipe, custom_dsa);
   } else if ((clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL) {
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
   } else if (clear_buffers & PIPE_CLEAR_DEPTH) {
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_keep_stencil);
   } else if (clear_buffers & PIPE_CLEAR_STENCIL) {
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_write_stencil);
   } else {
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   }

   sr.ref_value[0] = stencil & 0xff;
   pipe->set_stencil_ref(pipe, &sr);

   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   if (util_format_is_pure_sint(cbuf_format)) {
      pipe->bind_vertex_elements_state(pipe, ctx->velem_sint_state);
   } else if (util_format_is_pure_uint(cbuf_format)) {
      pipe->bind_vertex_elements_state(pipe, ctx->velem_uint_state);
   } else {
      pipe->bind_vertex_elements_state(pipe, ctx->velem_state);
   }
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, num_cbufs, int_format));
   pipe->bind_vs_state(pipe, ctx->vs);
   if (ctx->has_geometry_shader)
      pipe->bind_gs_state(pipe, NULL);

   blitter_set_dst_dimensions(ctx, width, height);
   blitter->draw_rectangle(blitter, 0, 0, width, height, depth,
                           UTIL_BLITTER_ATTRIB_COLOR, color);

   blitter_restore_vertex_states(ctx);
   blitter_restore_fragment_states(ctx);
   blitter_unset_running_flag(ctx);
}

void util_blitter_clear(struct blitter_context *blitter,
                        unsigned width, unsigned height,
                        unsigned num_cbufs,
                        unsigned clear_buffers,
                        enum pipe_format cbuf_format,
                        const union pipe_color_union *color,
                        double depth, unsigned stencil)
{
   util_blitter_clear_custom(blitter, width, height, num_cbufs,
                             clear_buffers, cbuf_format, color, depth, stencil,
                             NULL, NULL);
}

void util_blitter_clear_depth_custom(struct blitter_context *blitter,
                                     unsigned width, unsigned height,
                                     double depth, void *custom_dsa)
{
    static const union pipe_color_union color;
    util_blitter_clear_custom(blitter, width, height, 0,
                              0, PIPE_FORMAT_NONE, &color, depth, 0, NULL, custom_dsa);
}

static
boolean is_overlap(unsigned sx1, unsigned sx2, unsigned sy1, unsigned sy2,
                   unsigned dx1, unsigned dx2, unsigned dy1, unsigned dy2)
{
   return sx1 < dx2 && sx2 > dx1 && sy1 < dy2 && sy2 > dy1;
}

void util_blitter_copy_texture(struct blitter_context *blitter,
                               struct pipe_resource *dst,
                               unsigned dstlevel,
                               unsigned dstx, unsigned dsty, unsigned dstz,
                               struct pipe_resource *src,
                               unsigned srclevel,
                               const struct pipe_box *srcbox,
                               boolean ignore_stencil)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_surface *dstsurf, surf_templ;
   struct pipe_framebuffer_state fb_state;
   struct pipe_sampler_view viewTempl, *view;
   unsigned bind;
   unsigned width = srcbox->width;
   unsigned height = srcbox->height;
   boolean is_stencil, is_depth;
   boolean normalized;

   /* Give up if textures are not set. */
   assert(dst && src);
   if (!dst || !src)
      return;

   /* Sanity checks. */
   if (dst == src) {
      assert(!is_overlap(srcbox->x, srcbox->x + width, srcbox->y, srcbox->y + height,
                         dstx, dstx + width, dsty, dsty + height));
   }
   assert(src->target < PIPE_MAX_TEXTURE_TYPES);
   /* XXX should handle 3d regions */
   assert(srcbox->depth == 1);

   /* Is this a ZS format? */
   is_depth = util_format_get_component_bits(src->format, UTIL_FORMAT_COLORSPACE_ZS, 0) != 0;
   is_stencil = util_format_get_component_bits(src->format, UTIL_FORMAT_COLORSPACE_ZS, 1) != 0;

   if (is_depth || is_stencil)
      bind = PIPE_BIND_DEPTH_STENCIL;
   else
      bind = PIPE_BIND_RENDER_TARGET;

   /* Check if we can sample from and render to the surfaces. */
   /* (assuming copying a stencil buffer is not possible) */
   if ((!ignore_stencil && is_stencil) ||
       !screen->is_format_supported(screen, dst->format, dst->target,
                                    dst->nr_samples, bind) ||
       !screen->is_format_supported(screen, src->format, src->target,
                                    src->nr_samples, PIPE_BIND_SAMPLER_VIEW)) {
      ctx->base.running = TRUE;
      util_resource_copy_region(pipe, dst, dstlevel, dstx, dsty, dstz,
                                src, srclevel, srcbox);
      ctx->base.running = FALSE;
      return;
   }

   /* Get surface. */
   memset(&surf_templ, 0, sizeof(surf_templ));
   u_surface_default_template(&surf_templ, dst, bind);
   surf_templ.format = util_format_linear(dst->format);
   surf_templ.u.tex.level = dstlevel;
   surf_templ.u.tex.first_layer = dstz;
   surf_templ.u.tex.last_layer = dstz;
   dstsurf = pipe->create_surface(pipe, dst, &surf_templ);

   /* Check whether the states are properly saved. */
   blitter_set_running_flag(ctx);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_textures(ctx);
   blitter_check_saved_fb_state(ctx);

   /* Initialize framebuffer state. */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;

   if (is_depth) {
      pipe->bind_blend_state(pipe, ctx->blend_keep_color);
      pipe->bind_depth_stencil_alpha_state(pipe,
                                           ctx->dsa_write_depth_keep_stencil);
      pipe->bind_fs_state(pipe,
                          blitter_get_fs_texfetch_depth(ctx, src->target));

      fb_state.nr_cbufs = 0;
      fb_state.zsbuf = dstsurf;
   } else {
      pipe->bind_blend_state(pipe, ctx->blend_write_color);
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
      pipe->bind_fs_state(pipe,
                          blitter_get_fs_texfetch_col(ctx, src->target));

      fb_state.nr_cbufs = 1;
      fb_state.cbufs[0] = dstsurf;
      fb_state.zsbuf = 0;
   }

   normalized = src->target != PIPE_TEXTURE_RECT;

   /* Initialize sampler view. */
   u_sampler_view_default_template(&viewTempl, src, util_format_linear(src->format));
   view = pipe->create_sampler_view(pipe, src, &viewTempl);

   /* Set rasterizer state, shaders, and textures. */
   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_vs_state(pipe, ctx->vs);
   if (ctx->has_geometry_shader)
      pipe->bind_gs_state(pipe, NULL);
   pipe->bind_fragment_sampler_states(pipe, 1,
                                      blitter_get_sampler_state(ctx, srclevel, normalized));
   pipe->bind_vertex_elements_state(pipe, ctx->velem_state);
   pipe->set_fragment_sampler_views(pipe, 1, &view);
   pipe->set_framebuffer_state(pipe, &fb_state);

   blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);

   switch (src->target) {
      /* Draw the quad with the draw_rectangle callback. */
      case PIPE_TEXTURE_1D:
      case PIPE_TEXTURE_2D:
      case PIPE_TEXTURE_RECT:
         {
            /* Set texture coordinates. - use a pipe color union
             * for interface purposes
             */
            union pipe_color_union coord;
            get_texcoords(src, srclevel, srcbox->x, srcbox->y,
                          srcbox->x+width, srcbox->y+height, normalized, coord.f);

            /* Draw. */
            blitter->draw_rectangle(blitter, dstx, dsty, dstx+width, dsty+height, 0,
                                    UTIL_BLITTER_ATTRIB_TEXCOORD, &coord);
         }
         break;

      /* Draw the quad with the generic codepath. */
      default:
         /* Set texture coordinates. */
         switch (src->target) {
         case PIPE_TEXTURE_1D_ARRAY:
            blitter_set_texcoords_1d_array(ctx, src, srclevel, srcbox->y,
                                           srcbox->x, srcbox->x + width);
            break;

         case PIPE_TEXTURE_2D_ARRAY:
         case PIPE_TEXTURE_3D:
            blitter_set_texcoords_3d(ctx, src, srclevel, srcbox->z,
                                     srcbox->x, srcbox->y,
                                     srcbox->x + width, srcbox->y + height,
                                     src->target == PIPE_TEXTURE_3D);
            break;

         case PIPE_TEXTURE_CUBE:
            blitter_set_texcoords_cube(ctx, src, srclevel, srcbox->z,
                                       srcbox->x, srcbox->y,
                                       srcbox->x + width, srcbox->y + height);
            break;

         default:
            assert(0);
         }

         /* Draw. */
         blitter_set_rectangle(ctx, dstx, dsty, dstx+width, dsty+height, 0);
         ctx->base.pipe->redefine_user_buffer(ctx->base.pipe, ctx->vbuf,
                                              0, ctx->vbuf->width0);
         util_draw_vertex_buffer(ctx->base.pipe, NULL, ctx->vbuf, 0,
                                 PIPE_PRIM_TRIANGLE_FAN, 4, 2);
         break;
   }

   blitter_restore_vertex_states(ctx);
   blitter_restore_fragment_states(ctx);
   blitter_restore_textures(ctx);
   blitter_restore_fb_state(ctx);
   blitter_unset_running_flag(ctx);

   pipe_surface_reference(&dstsurf, NULL);
   pipe_sampler_view_reference(&view, NULL);
}

/* Clear a region of a color surface to a constant value. */
void util_blitter_clear_render_target(struct blitter_context *blitter,
                                      struct pipe_surface *dstsurf,
                                      const union pipe_color_union *color,
                                      unsigned dstx, unsigned dsty,
                                      unsigned width, unsigned height)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;

   assert(dstsurf->texture);
   if (!dstsurf->texture)
      return;

   /* check the saved state */
   blitter_set_running_flag(ctx);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);

   /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend_write_color);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, 1, FALSE));
   pipe->bind_vs_state(pipe, ctx->vs);
   if (ctx->has_geometry_shader)
      pipe->bind_gs_state(pipe, NULL);
   pipe->bind_vertex_elements_state(pipe, ctx->velem_state);

   /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dstsurf;
   fb_state.zsbuf = 0;
   pipe->set_framebuffer_state(pipe, &fb_state);

   blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);
   blitter->draw_rectangle(blitter, dstx, dsty, dstx+width, dsty+height, 0,
                           UTIL_BLITTER_ATTRIB_COLOR, color);

   blitter_restore_vertex_states(ctx);
   blitter_restore_fragment_states(ctx);
   blitter_restore_fb_state(ctx);
   blitter_unset_running_flag(ctx);
}

/* Clear a region of a depth stencil surface. */
void util_blitter_clear_depth_stencil(struct blitter_context *blitter,
                                      struct pipe_surface *dstsurf,
                                      unsigned clear_flags,
                                      double depth,
                                      unsigned stencil,
                                      unsigned dstx, unsigned dsty,
                                      unsigned width, unsigned height)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;
   struct pipe_stencil_ref sr = { { 0 } };

   assert(dstsurf->texture);
   if (!dstsurf->texture)
      return;

   /* check the saved state */
   blitter_set_running_flag(ctx);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);

   /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend_keep_color);
   if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL) {
      sr.ref_value[0] = stencil & 0xff;
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
      pipe->set_stencil_ref(pipe, &sr);
   }
   else if (clear_flags & PIPE_CLEAR_DEPTH) {
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_keep_stencil);
   }
   else if (clear_flags & PIPE_CLEAR_STENCIL) {
      sr.ref_value[0] = stencil & 0xff;
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_write_stencil);
      pipe->set_stencil_ref(pipe, &sr);
   }
   else
      /* hmm that should be illegal probably, or make it a no-op somewhere */
      pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);

   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, 0, FALSE));
   pipe->bind_vs_state(pipe, ctx->vs);
   if (ctx->has_geometry_shader)
      pipe->bind_gs_state(pipe, NULL);
   pipe->bind_vertex_elements_state(pipe, ctx->velem_state);

   /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 0;
   fb_state.cbufs[0] = 0;
   fb_state.zsbuf = dstsurf;
   pipe->set_framebuffer_state(pipe, &fb_state);

   blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);
   blitter->draw_rectangle(blitter, dstx, dsty, dstx+width, dsty+height, depth,
                           UTIL_BLITTER_ATTRIB_NONE, NULL);

   blitter_restore_vertex_states(ctx);
   blitter_restore_fragment_states(ctx);
   blitter_restore_fb_state(ctx);
   blitter_unset_running_flag(ctx);
}

/* draw a rectangle across a region using a custom dsa stage - for r600g */
void util_blitter_custom_depth_stencil(struct blitter_context *blitter,
				       struct pipe_surface *zsurf,
				       struct pipe_surface *cbsurf,
				       void *dsa_stage, float depth)
{
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;

   assert(zsurf->texture);
   if (!zsurf->texture)
      return;

   /* check the saved state */
   blitter_set_running_flag(ctx);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);

   /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend_write_color);
   pipe->bind_depth_stencil_alpha_state(pipe, dsa_stage);

   pipe->bind_rasterizer_state(pipe, ctx->rs_state);
   pipe->bind_fs_state(pipe, blitter_get_fs_col(ctx, 0, FALSE));
   pipe->bind_vs_state(pipe, ctx->vs);
   if (ctx->has_geometry_shader)
      pipe->bind_gs_state(pipe, NULL);
   pipe->bind_vertex_elements_state(pipe, ctx->velem_state);

   /* set a framebuffer state */
   fb_state.width = zsurf->width;
   fb_state.height = zsurf->height;
   fb_state.nr_cbufs = 1;
   if (cbsurf) {
	   fb_state.cbufs[0] = cbsurf;
	   fb_state.nr_cbufs = 1;
   } else {
	   fb_state.cbufs[0] = NULL;
	   fb_state.nr_cbufs = 0;
   }
   fb_state.zsbuf = zsurf;
   pipe->set_framebuffer_state(pipe, &fb_state);

   blitter_set_dst_dimensions(ctx, zsurf->width, zsurf->height);
   blitter->draw_rectangle(blitter, 0, 0, zsurf->width, zsurf->height, depth,
                           UTIL_BLITTER_ATTRIB_NONE, NULL);

   blitter_restore_vertex_states(ctx);
   blitter_restore_fragment_states(ctx);
   blitter_restore_fb_state(ctx);
   blitter_unset_running_flag(ctx);
}
