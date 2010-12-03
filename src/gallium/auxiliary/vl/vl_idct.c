/**************************************************************************
 *
 * Copyright 2010 Christian König
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

#include "vl_idct.h"
#include "vl_vertex_buffers.h"
#include "util/u_draw.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <util/u_format.h>
#include <tgsi/tgsi_ureg.h>
#include "vl_types.h"

#define BLOCK_WIDTH 8
#define BLOCK_HEIGHT 8

#define SCALE_FACTOR_16_TO_9 (32768.0f / 256.0f)

#define STAGE1_SCALE 4.0f
#define STAGE2_SCALE (SCALE_FACTOR_16_TO_9 / STAGE1_SCALE)

#define NR_RENDER_TARGETS 1

struct vertex_shader_consts
{
   struct vertex4f norm;
};

enum VS_INPUT
{
   VS_I_RECT,
   VS_I_VPOS,

   NUM_VS_INPUTS
};

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_BLOCK,
   VS_O_TEX,
   VS_O_START
};

static const float const_matrix[8][8] = {
   {  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.353553f,  0.3535530f },
   {  0.4903930f,  0.4157350f,  0.2777850f,  0.0975451f, -0.0975452f, -0.2777850f, -0.415735f, -0.4903930f },
   {  0.4619400f,  0.1913420f, -0.1913420f, -0.4619400f, -0.4619400f, -0.1913420f,  0.191342f,  0.4619400f },
   {  0.4157350f, -0.0975452f, -0.4903930f, -0.2777850f,  0.2777850f,  0.4903930f,  0.097545f, -0.4157350f },
   {  0.3535530f, -0.3535530f, -0.3535530f,  0.3535540f,  0.3535530f, -0.3535540f, -0.353553f,  0.3535530f },
   {  0.2777850f, -0.4903930f,  0.0975452f,  0.4157350f, -0.4157350f, -0.0975451f,  0.490393f, -0.2777850f },
   {  0.1913420f, -0.4619400f,  0.4619400f, -0.1913420f, -0.1913410f,  0.4619400f, -0.461940f,  0.1913420f },
   {  0.0975451f, -0.2777850f,  0.4157350f, -0.4903930f,  0.4903930f, -0.4157350f,  0.277786f, -0.0975458f }
};

static void *
create_vert_shader(struct vl_idct *idct, bool calc_src_cords)
{
   struct ureg_program *shader;
   struct ureg_src scale;
   struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos;
   struct ureg_dst o_vpos, o_block, o_tex, o_start;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   t_vpos = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   /*
    * scale = (BLOCK_WIDTH, BLOCK_HEIGHT) / (dst.width, dst.height)
    *
    * t_vpos = vpos + vrect
    * o_vpos.xy = t_vpos * scale
    * o_vpos.zw = vpos
    *
    * o_block = vrect
    * o_tex = t_pos
    * o_start = vpos * scale
    *
    */
   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->destination->width0, 
      (float)BLOCK_HEIGHT / idct->destination->height0);

   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   if(calc_src_cords) {
      o_block = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK);
      o_tex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX);
      o_start = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_START);

      ureg_MOV(shader, ureg_writemask(o_block, TGSI_WRITEMASK_XY), vrect);
      ureg_MOV(shader, ureg_writemask(o_tex, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
      ureg_MUL(shader, ureg_writemask(o_start, TGSI_WRITEMASK_XY), vpos, scale);
   }

   ureg_release_temporary(shader, t_vpos);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void
fetch_four(struct ureg_program *shader, struct ureg_dst m[2],
           struct ureg_src tc, struct ureg_src sampler,
           struct ureg_src start, struct ureg_src block,
           bool right_side, bool transposed, float size)
{
   struct ureg_dst t_tc;
   unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
   unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;

   t_tc = ureg_DECL_temporary(shader);
   m[0] = ureg_DECL_temporary(shader);
   m[1] = ureg_DECL_temporary(shader);

   /*
    * t_tc.x = right_side ? start.x : tc.x
    * t_tc.y = right_side ? tc.y : start.y
    * m[0..1] = tex(t_tc++, sampler)
    */
   if(!right_side) {
      ureg_MOV(shader, ureg_writemask(t_tc, wm_start), ureg_scalar(start, TGSI_SWIZZLE_X));
      ureg_MOV(shader, ureg_writemask(t_tc, wm_tc), ureg_scalar(tc, TGSI_SWIZZLE_Y));
   } else {
      ureg_MOV(shader, ureg_writemask(t_tc, wm_start), ureg_scalar(start, TGSI_SWIZZLE_Y));
      ureg_MOV(shader, ureg_writemask(t_tc, wm_tc), ureg_scalar(tc, TGSI_SWIZZLE_X));
   }

#if NR_RENDER_TARGETS == 8
   ureg_MOV(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_Z), ureg_scalar(block, TGSI_SWIZZLE_X));
#else
   ureg_MOV(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_Z), ureg_imm1f(shader, 0.0f));
#endif

   ureg_TEX(shader, m[0], TGSI_TEXTURE_3D, ureg_src(t_tc), sampler);
   ureg_ADD(shader, ureg_writemask(t_tc, wm_start), ureg_src(t_tc), ureg_imm1f(shader, 1.0f / size));
   ureg_TEX(shader, m[1], TGSI_TEXTURE_3D, ureg_src(t_tc), sampler);

   ureg_release_temporary(shader, t_tc);
}

static void
matrix_mul(struct ureg_program *shader, struct ureg_dst dst, struct ureg_dst l[2], struct ureg_dst r[2])
{
   struct ureg_dst tmp[2];
   unsigned i;

   for(i = 0; i < 2; ++i) {
      tmp[i] = ureg_DECL_temporary(shader);
   }

   /*
    * tmp[0..1] = dot4(m[0][0..1], m[1][0..1])
    * dst = tmp[0] + tmp[1]
    */
   ureg_DP4(shader, ureg_writemask(tmp[0], TGSI_WRITEMASK_X), ureg_src(l[0]), ureg_src(r[0]));
   ureg_DP4(shader, ureg_writemask(tmp[1], TGSI_WRITEMASK_X), ureg_src(l[1]), ureg_src(r[1]));
   ureg_ADD(shader, dst,
      ureg_scalar(ureg_src(tmp[0]), TGSI_SWIZZLE_X),
      ureg_scalar(ureg_src(tmp[1]), TGSI_SWIZZLE_X));

   for(i = 0; i < 2; ++i) {
      ureg_release_temporary(shader, tmp[i]);
   }
}

static void *
create_transpose_frag_shader(struct vl_idct *idct)
{
   struct pipe_resource *transpose = idct->textures.individual.transpose;
   struct pipe_resource *intermediate = idct->textures.individual.intermediate;

   struct ureg_program *shader;

   struct ureg_src block, tex, sampler[2];
   struct ureg_src start[2];

   struct ureg_dst m[2][2];
   struct ureg_dst tmp, fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   block = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK, TGSI_INTERPOLATE_LINEAR);
   tex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX, TGSI_INTERPOLATE_CONSTANT);

   sampler[0] = ureg_DECL_sampler(shader, 0);
   sampler[1] = ureg_DECL_sampler(shader, 1);

   start[0] = ureg_imm1f(shader, 0.0f);
   start[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_START, TGSI_INTERPOLATE_CONSTANT);

   fetch_four(shader, m[0], block, sampler[0], start[0], block, false, false, transpose->width0);
   fetch_four(shader, m[1], tex, sampler[1], start[1], block, true, false, intermediate->height0);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   tmp = ureg_DECL_temporary(shader);
   matrix_mul(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X), m[0], m[1]);
   ureg_MUL(shader, fragment, ureg_src(tmp), ureg_imm1f(shader, STAGE2_SCALE));

   ureg_release_temporary(shader, tmp);
   ureg_release_temporary(shader, m[0][0]);
   ureg_release_temporary(shader, m[0][1]);
   ureg_release_temporary(shader, m[1][0]);
   ureg_release_temporary(shader, m[1][1]);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_matrix_frag_shader(struct vl_idct *idct)
{
   struct pipe_resource *matrix = idct->textures.individual.matrix;
   struct pipe_resource *source = idct->textures.individual.source;

   struct ureg_program *shader;

   struct ureg_src tex, block, sampler[2];
   struct ureg_src start[2];

   struct ureg_dst l[4][2], r[2];
   struct ureg_dst t_tc, tmp, fragment[NR_RENDER_TARGETS];

   unsigned i, j;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   t_tc = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);

   tex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX, TGSI_INTERPOLATE_LINEAR);
   block = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK, TGSI_INTERPOLATE_LINEAR);

   sampler[0] = ureg_DECL_sampler(shader, 1);
   sampler[1] = ureg_DECL_sampler(shader, 0);

   start[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_START, TGSI_INTERPOLATE_CONSTANT);
   start[1] = ureg_imm1f(shader, 0.0f);

   for (i = 0; i < NR_RENDER_TARGETS; ++i)
       fragment[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, i);

   ureg_MOV(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_Y), tex);
   for (i = 0; i < 4; ++i) {
      fetch_four(shader, l[i], ureg_src(t_tc), sampler[0], start[0], block, false, false, source->width0);
      ureg_MUL(shader, l[i][0], ureg_src(l[i][0]), ureg_imm1f(shader, STAGE1_SCALE));
      ureg_MUL(shader, l[i][1], ureg_src(l[i][1]), ureg_imm1f(shader, STAGE1_SCALE));
      if(i != 3)
         ureg_ADD(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_Y), 
            ureg_src(t_tc), ureg_imm1f(shader, 1.0f / source->height0));
   }
   
   for (i = 0; i < NR_RENDER_TARGETS; ++i) {

#if NR_RENDER_TARGETS == 8
      ureg_MOV(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_X), ureg_imm1f(shader, 1.0f / BLOCK_WIDTH * i));
      fetch_four(shader, r, ureg_src(t_tc), sampler[1], start[1], block, true, true, matrix->width0);
#elif NR_RENDER_TARGETS == 1
      fetch_four(shader, r, block, sampler[1], start[1], block, true, true, matrix->width0);
#else
#error invalid number of render targets
#endif

      for (j = 0; j < 4; ++j) {
         matrix_mul(shader, ureg_writemask(fragment[i], TGSI_WRITEMASK_X << j), l[j], r);
      }
      ureg_release_temporary(shader, r[0]);
      ureg_release_temporary(shader, r[1]);
   }

   ureg_release_temporary(shader, t_tc);
   ureg_release_temporary(shader, tmp);

   for (i = 0; i < 4; ++i) {
      ureg_release_temporary(shader, l[i][0]);
      ureg_release_temporary(shader, l[i][1]);
   }

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_empty_block_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   ureg_MOV(shader, fragment, ureg_imm1f(shader, 0.0f));

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static bool
init_shaders(struct vl_idct *idct)
{
   idct->matrix_vs = create_vert_shader(idct, true);
   idct->matrix_fs = create_matrix_frag_shader(idct);

   idct->transpose_vs = create_vert_shader(idct, true);
   idct->transpose_fs = create_transpose_frag_shader(idct);

   idct->eb_vs = create_vert_shader(idct, false);
   idct->eb_fs = create_empty_block_frag_shader(idct);

   return 
      idct->transpose_vs != NULL && idct->transpose_fs != NULL &&
      idct->matrix_vs != NULL && idct->matrix_fs != NULL &&
      idct->eb_vs != NULL && idct->eb_fs != NULL;
}

static void
cleanup_shaders(struct vl_idct *idct)
{
   idct->pipe->delete_vs_state(idct->pipe, idct->transpose_vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->transpose_fs);

   idct->pipe->delete_vs_state(idct->pipe, idct->matrix_vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->matrix_fs);

   idct->pipe->delete_vs_state(idct->pipe, idct->eb_vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->eb_fs);
}

static bool
init_buffers(struct vl_idct *idct)
{
   struct pipe_resource template;
   struct pipe_sampler_view sampler_view;
   struct pipe_vertex_element vertex_elems[2];
   unsigned i;

   memset(&template, 0, sizeof(struct pipe_resource));
   template.last_level = 0;
   template.depth0 = 1;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_R16G16B16A16_SNORM;
   template.width0 = idct->destination->width0 / 4;
   template.height0 = idct->destination->height0;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_STREAM;
   idct->textures.individual.source = idct->pipe->screen->resource_create(idct->pipe->screen, &template);

   template.target = PIPE_TEXTURE_3D;
   template.format = PIPE_FORMAT_R16G16B16A16_SNORM;
   template.width0 = idct->destination->width0 / NR_RENDER_TARGETS;
   template.height0 = idct->destination->height0 / 4;
   template.depth0 = NR_RENDER_TARGETS;
   template.usage = PIPE_USAGE_STATIC;
   idct->textures.individual.intermediate = idct->pipe->screen->resource_create(idct->pipe->screen, &template);

   for (i = 0; i < 4; ++i) {
      if(idct->textures.all[i] == NULL)
         return false; /* a texture failed to allocate */

      u_sampler_view_default_template(&sampler_view, idct->textures.all[i], idct->textures.all[i]->format);
      idct->sampler_views.all[i] = idct->pipe->create_sampler_view(idct->pipe, idct->textures.all[i], &sampler_view);
   }

   idct->vertex_bufs.individual.quad = vl_vb_upload_quads(idct->pipe, idct->max_blocks);

   if(idct->vertex_bufs.individual.quad.buffer == NULL)
      return false;

   idct->vertex_bufs.individual.pos.stride = sizeof(struct vertex2f);
   idct->vertex_bufs.individual.pos.max_index = 4 * idct->max_blocks - 1;
   idct->vertex_bufs.individual.pos.buffer_offset = 0;
   idct->vertex_bufs.individual.pos.buffer = pipe_buffer_create
   (
      idct->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex2f) * 4 * idct->max_blocks
   );

   if(idct->vertex_bufs.individual.pos.buffer == NULL)
      return false;

   /* Rect element */
   vertex_elems[0].src_offset = 0;
   vertex_elems[0].instance_divisor = 0;
   vertex_elems[0].vertex_buffer_index = 0;
   vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Pos element */
   vertex_elems[1].src_offset = 0;
   vertex_elems[1].instance_divisor = 0;
   vertex_elems[1].vertex_buffer_index = 1;
   vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   idct->vertex_elems_state = idct->pipe->create_vertex_elements_state(idct->pipe, 2, vertex_elems);

   return true;
}

static void
cleanup_buffers(struct vl_idct *idct)
{
   unsigned i;

   assert(idct);

   for (i = 0; i < 4; ++i) {
      pipe_sampler_view_reference(&idct->sampler_views.all[i], NULL);
      pipe_resource_reference(&idct->textures.all[i], NULL);
   }

   idct->pipe->delete_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
   pipe_resource_reference(&idct->vertex_bufs.individual.quad.buffer, NULL);
   pipe_resource_reference(&idct->vertex_bufs.individual.pos.buffer, NULL);
}

static void
init_state(struct vl_idct *idct)
{
   struct pipe_sampler_state sampler;
   struct pipe_rasterizer_state rs_state;
   unsigned i;

   idct->viewport[0].scale[0] = idct->textures.individual.intermediate->width0;
   idct->viewport[0].scale[1] = idct->textures.individual.intermediate->height0;

   idct->viewport[1].scale[0] = idct->destination->width0;
   idct->viewport[1].scale[1] = idct->destination->height0;

   idct->fb_state[0].width = idct->textures.individual.intermediate->width0;
   idct->fb_state[0].height = idct->textures.individual.intermediate->height0;

   idct->fb_state[0].nr_cbufs = NR_RENDER_TARGETS;
   for(i = 0; i < NR_RENDER_TARGETS; ++i) {
      idct->fb_state[0].cbufs[i] = idct->pipe->screen->get_tex_surface(
         idct->pipe->screen, idct->textures.individual.intermediate, 0, 0, i,
         PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET);
   }

   idct->fb_state[1].width = idct->destination->width0;
   idct->fb_state[1].height = idct->destination->height0;

   idct->fb_state[1].nr_cbufs = 1;
   idct->fb_state[1].cbufs[0] = idct->pipe->screen->get_tex_surface(
      idct->pipe->screen, idct->destination, 0, 0, 0,
      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET);

   for(i = 0; i < 2; ++i) {
      idct->viewport[i].scale[2] = 1;
      idct->viewport[i].scale[3] = 1;
      idct->viewport[i].translate[0] = 0;
      idct->viewport[i].translate[1] = 0;
      idct->viewport[i].translate[2] = 0;
      idct->viewport[i].translate[3] = 0;

      idct->fb_state[i].zsbuf = NULL;
   }

   for (i = 0; i < 4; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      /*sampler.shadow_ambient = ; */
      /*sampler.lod_bias = ; */
      sampler.min_lod = 0;
      /*sampler.max_lod = ; */
      /*sampler.border_color[0] = ; */
      /*sampler.max_anisotropy = ; */
      idct->samplers.all[i] = idct->pipe->create_sampler_state(idct->pipe, &sampler);
   }

   memset(&rs_state, 0, sizeof(rs_state));
   /*rs_state.sprite_coord_enable */
   rs_state.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
   rs_state.point_quad_rasterization = true;
   rs_state.point_size = BLOCK_WIDTH;
   rs_state.gl_rasterization_rules = false;
   idct->rs_state = idct->pipe->create_rasterizer_state(idct->pipe, &rs_state);
}

static void
cleanup_state(struct vl_idct *idct)
{
   unsigned i;

   for(i = 0; i < NR_RENDER_TARGETS; ++i) {
      idct->pipe->screen->tex_surface_destroy(idct->fb_state[0].cbufs[i]);
   }

   idct->pipe->screen->tex_surface_destroy(idct->fb_state[1].cbufs[0]);

   for (i = 0; i < 4; ++i)
      idct->pipe->delete_sampler_state(idct->pipe, idct->samplers.all[i]);

   idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);
}

struct pipe_resource *
vl_idct_upload_matrix(struct pipe_context *pipe)
{
   struct pipe_resource template, *matrix;
   struct pipe_transfer *buf_transfer;
   unsigned i, j, pitch;
   float *f;

   struct pipe_box rect =
   {
      0, 0, 0,
      BLOCK_WIDTH,
      BLOCK_HEIGHT,
      1
   };

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   template.last_level = 0;
   template.width0 = 2;
   template.height0 = 8;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_IMMUTABLE;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   matrix = pipe->screen->resource_create(pipe->screen, &template);

   /* matrix */
   buf_transfer = pipe->get_transfer
   (
      pipe, matrix,
      u_subresource(0, 0),
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &rect
   );
   pitch = buf_transfer->stride / sizeof(float);

   f = pipe->transfer_map(pipe, buf_transfer);
   for(i = 0; i < BLOCK_HEIGHT; ++i)
      for(j = 0; j < BLOCK_WIDTH; ++j)
         f[i * pitch + j] = const_matrix[j][i]; // transpose

   pipe->transfer_unmap(pipe, buf_transfer);
   pipe->transfer_destroy(pipe, buf_transfer);

   return matrix;
}

static void
xfer_buffers_map(struct vl_idct *idct)
{
   struct pipe_box rect =
   {
      0, 0, 0,
      idct->textures.individual.source->width0,
      idct->textures.individual.source->height0,
      1
   };

   idct->tex_transfer = idct->pipe->get_transfer
   (
      idct->pipe, idct->textures.individual.source,
      u_subresource(0, 0),
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &rect
   );

   idct->texels = idct->pipe->transfer_map(idct->pipe, idct->tex_transfer);
}

static void
xfer_buffers_unmap(struct vl_idct *idct)
{
   idct->pipe->transfer_unmap(idct->pipe, idct->tex_transfer);
   idct->pipe->transfer_destroy(idct->pipe, idct->tex_transfer);
}

bool
vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe, struct pipe_resource *dst, struct pipe_resource *matrix)
{
   assert(idct && pipe && dst);

   idct->pipe = pipe;
   pipe_resource_reference(&idct->textures.individual.matrix, matrix);
   pipe_resource_reference(&idct->textures.individual.transpose, matrix);
   pipe_resource_reference(&idct->destination, dst);

   idct->max_blocks =
      align(idct->destination->width0, BLOCK_WIDTH) / BLOCK_WIDTH *
      align(idct->destination->height0, BLOCK_HEIGHT) / BLOCK_HEIGHT *
      idct->destination->depth0;

   if(!init_buffers(idct))
      return false;

   if(!init_shaders(idct)) {
      cleanup_buffers(idct);
      return false;
   }

   if(!vl_vb_init(&idct->blocks, idct->max_blocks)) {
      cleanup_shaders(idct);
      cleanup_buffers(idct);
      return false;
   }

   init_state(idct);

   xfer_buffers_map(idct);

   return true;
}

void
vl_idct_cleanup(struct vl_idct *idct)
{
   vl_vb_cleanup(&idct->blocks);
   cleanup_shaders(idct);
   cleanup_buffers(idct);

   cleanup_state(idct);

   pipe_resource_reference(&idct->destination, NULL);
}

void
vl_idct_add_block(struct vl_idct *idct, unsigned x, unsigned y, short *block)
{
   unsigned tex_pitch;
   short *texels;

   unsigned i;

   assert(idct);

   tex_pitch = idct->tex_transfer->stride / sizeof(short);
   texels = idct->texels + y * tex_pitch * BLOCK_HEIGHT + x * BLOCK_WIDTH;

   for (i = 0; i < BLOCK_HEIGHT; ++i)
      memcpy(texels + i * tex_pitch, block + i * BLOCK_WIDTH, BLOCK_WIDTH * sizeof(short));

   vl_vb_add_block(&idct->blocks, x, y);
}

void
vl_idct_flush(struct vl_idct *idct)
{
   struct pipe_transfer *vec_transfer;
   struct quadf *vectors;
   unsigned num_blocks;

   assert(idct);

   vectors = pipe_buffer_map
   (
      idct->pipe,
      idct->vertex_bufs.individual.pos.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &vec_transfer
   );

   num_blocks = vl_vb_upload(&idct->blocks, vectors);

   pipe_buffer_unmap(idct->pipe, idct->vertex_bufs.individual.pos.buffer, vec_transfer);

   xfer_buffers_unmap(idct);

   if(num_blocks > 0) {

      idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);

      /* first stage */
      idct->pipe->set_framebuffer_state(idct->pipe, &idct->fb_state[0]);
      idct->pipe->set_viewport_state(idct->pipe, &idct->viewport[0]);

      idct->pipe->set_vertex_buffers(idct->pipe, 2, idct->vertex_bufs.all);
      idct->pipe->bind_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, idct->sampler_views.stage[0]);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers.stage[0]);
      idct->pipe->bind_vs_state(idct->pipe, idct->matrix_vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->matrix_fs);

      util_draw_arrays(idct->pipe, PIPE_PRIM_QUADS, 0, num_blocks * 4);

      /* second stage */
      idct->pipe->set_framebuffer_state(idct->pipe, &idct->fb_state[1]);
      idct->pipe->set_viewport_state(idct->pipe, &idct->viewport[1]);

      idct->pipe->set_vertex_buffers(idct->pipe, 2, idct->vertex_bufs.all);
      idct->pipe->bind_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, idct->sampler_views.stage[1]);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers.stage[1]);
      idct->pipe->bind_vs_state(idct->pipe, idct->transpose_vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->transpose_fs);

      util_draw_arrays(idct->pipe, PIPE_PRIM_QUADS, 0, num_blocks * 4);
   }

   xfer_buffers_map(idct);
}
