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
#include "vl_defines.h"
#include "util/u_draw.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <util/u_format.h>
#include <tgsi/tgsi_ureg.h>
#include "vl_types.h"

#define SCALE_FACTOR_16_TO_9 (32768.0f / 256.0f)

#define NR_RENDER_TARGETS 4

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_L_ADDR0,
   VS_O_L_ADDR1,
   VS_O_R_ADDR0,
   VS_O_R_ADDR1
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

static void
calc_addr(struct ureg_program *shader, struct ureg_dst addr[2],
          struct ureg_src tc, struct ureg_src start, bool right_side,
          bool transposed, float size)
{
   unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
   unsigned sw_start = right_side ? TGSI_SWIZZLE_Y : TGSI_SWIZZLE_X;

   unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;
   unsigned sw_tc = right_side ? TGSI_SWIZZLE_X : TGSI_SWIZZLE_Y;

   /*
    * addr[0..1].(start) = right_side ? start.x : tc.x
    * addr[0..1].(tc) = right_side ? tc.y : start.y
    * addr[0..1].z = tc.z
    * addr[1].(start) += 1.0f / scale
    */
   ureg_MOV(shader, ureg_writemask(addr[0], wm_start), ureg_scalar(start, sw_start));
   ureg_MOV(shader, ureg_writemask(addr[0], wm_tc), ureg_scalar(tc, sw_tc));
   ureg_MOV(shader, ureg_writemask(addr[0], TGSI_WRITEMASK_Z), tc);

   ureg_ADD(shader, ureg_writemask(addr[1], wm_start), ureg_scalar(start, sw_start), ureg_imm1f(shader, 1.0f / size));
   ureg_MOV(shader, ureg_writemask(addr[1], wm_tc), ureg_scalar(tc, sw_tc));
   ureg_MOV(shader, ureg_writemask(addr[1], TGSI_WRITEMASK_Z), tc);
}

static void *
create_vert_shader(struct vl_idct *idct, bool matrix_stage)
{
   struct ureg_program *shader;
   struct ureg_src vrect, vpos, vblock, eb;
   struct ureg_src scale, blocks_xy;
   struct ureg_dst t_tex, t_start;
   struct ureg_dst o_vpos, o_l_addr[2], o_r_addr[2];
   unsigned label;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   t_tex = ureg_DECL_temporary(shader);
   t_start = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);
   vblock = ureg_swizzle(vrect, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W, TGSI_SWIZZLE_X, TGSI_SWIZZLE_X);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   eb = ureg_DECL_vs_input(shader, VS_I_EB);

   o_l_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0);
   o_l_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1);

   o_r_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0);
   o_r_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR1);

   /*
    * scale = (BLOCK_WIDTH, BLOCK_HEIGHT) / (dst.width, dst.height)
    * blocks_xy = (blocks_x, blocks_y)
    *
    * if eb.(vblock.y, vblock.x)
    *    o_vpos.xy = -1
    * else
    *    t_tex = vpos * blocks_xy + vblock
    *    t_start = t_tex * scale
    *    t_tex = t_tex + vrect
    *    o_vpos.xy = t_tex * scale
    *
    *    o_l_addr = calc_addr(...)
    *    o_r_addr = calc_addr(...)
    * endif
    * o_vpos.zw = vpos
    *
    */

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->buffer_width,
      (float)BLOCK_HEIGHT / idct->buffer_height);

   blocks_xy = ureg_imm2f(shader, idct->blocks_x, idct->blocks_y);

   if (idct->blocks_x > 1 || idct->blocks_y > 1) {
      ureg_CMP(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY),
         ureg_negate(ureg_scalar(vblock, TGSI_SWIZZLE_Y)),
         ureg_swizzle(eb, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W),
         ureg_swizzle(eb, TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y));

      ureg_CMP(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_X),
         ureg_negate(ureg_scalar(vblock, TGSI_SWIZZLE_X)),
         ureg_scalar(ureg_src(t_tex), TGSI_SWIZZLE_Y),
         ureg_scalar(ureg_src(t_tex), TGSI_SWIZZLE_X));

      eb = ureg_src(t_tex);
   }

   ureg_IF(shader, ureg_scalar(eb, TGSI_SWIZZLE_X), &label);

      ureg_MOV(shader, o_vpos, ureg_imm1f(shader, -1.0f));

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ELSE(shader, &label);

      ureg_MAD(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), vpos, blocks_xy, vblock);
      ureg_MUL(shader, ureg_writemask(t_start, TGSI_WRITEMASK_XY), ureg_src(t_tex), scale);

      ureg_ADD(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), ureg_src(t_tex), vrect);

      ureg_MUL(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), ureg_src(t_tex), scale);
      ureg_MUL(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_Z),
         ureg_scalar(vrect, TGSI_SWIZZLE_X),
         ureg_imm1f(shader, BLOCK_WIDTH / NR_RENDER_TARGETS));

      ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_tex));

      if(matrix_stage) {
         calc_addr(shader, o_l_addr, ureg_src(t_tex), ureg_src(t_start), false, false, idct->buffer_width / 4);
         calc_addr(shader, o_r_addr, vrect, ureg_imm1f(shader, 0.0f), true, true, BLOCK_WIDTH / 4);
      } else {
         calc_addr(shader, o_l_addr, vrect, ureg_imm1f(shader, 0.0f), false, false, BLOCK_WIDTH / 4);
         calc_addr(shader, o_r_addr, ureg_src(t_tex), ureg_src(t_start), true, false, idct->buffer_height / 4);
      }

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ENDIF(shader);

   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   ureg_release_temporary(shader, t_tex);
   ureg_release_temporary(shader, t_start);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void
increment_addr(struct ureg_program *shader, struct ureg_dst daddr[2],
               struct ureg_src saddr[2], bool right_side, bool transposed,
               int pos, float size)
{
   unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
   unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;

   /*
    * daddr[0..1].(start) = saddr[0..1].(start)
    * daddr[0..1].(tc) = saddr[0..1].(tc)
    */

   ureg_MOV(shader, ureg_writemask(daddr[0], wm_start), saddr[0]);
   ureg_ADD(shader, ureg_writemask(daddr[0], wm_tc), saddr[0], ureg_imm1f(shader, pos / size));
   ureg_MOV(shader, ureg_writemask(daddr[1], wm_start), saddr[1]);
   ureg_ADD(shader, ureg_writemask(daddr[1], wm_tc), saddr[1], ureg_imm1f(shader, pos / size));
}

static void
fetch_four(struct ureg_program *shader, struct ureg_dst m[2], struct ureg_src addr[2], struct ureg_src sampler)
{
   ureg_TEX(shader, m[0], TGSI_TEXTURE_3D, addr[0], sampler);
   ureg_TEX(shader, m[1], TGSI_TEXTURE_3D, addr[1], sampler);
}

static void
matrix_mul(struct ureg_program *shader, struct ureg_dst dst, struct ureg_dst l[2], struct ureg_dst r[2])
{
   struct ureg_dst tmp;

   tmp = ureg_DECL_temporary(shader);

   /*
    * tmp.xy = dot4(m[0][0..1], m[1][0..1])
    * dst = tmp.x + tmp.y
    */
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_src(l[0]), ureg_src(r[0]));
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(l[1]), ureg_src(r[1]));
   ureg_ADD(shader, dst,
      ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X),
      ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));

   ureg_release_temporary(shader, tmp);
}

static void *
create_matrix_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;

   struct ureg_src l_addr[2], r_addr[2];

   struct ureg_dst l[4][2], r[2];
   struct ureg_dst fragment[NR_RENDER_TARGETS];

   unsigned i, j;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
   l_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1, TGSI_INTERPOLATE_LINEAR);

   r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
   r_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR1, TGSI_INTERPOLATE_LINEAR);

   for (i = 0; i < NR_RENDER_TARGETS; ++i)
       fragment[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, i);

   for (i = 0; i < 4; ++i) {
      l[i][0] = ureg_DECL_temporary(shader);
      l[i][1] = ureg_DECL_temporary(shader);
   }

   r[0] = ureg_DECL_temporary(shader);
   r[1] = ureg_DECL_temporary(shader);

   for (i = 1; i < 4; ++i) {
      increment_addr(shader, l[i], l_addr, false, false, i, idct->buffer_height);
   }

   for (i = 0; i < 4; ++i) {
      struct ureg_src s_addr[2];
      s_addr[0] = i == 0 ? l_addr[0] : ureg_src(l[i][0]);
      s_addr[1] = i == 0 ? l_addr[1] : ureg_src(l[i][1]);
      fetch_four(shader, l[i], s_addr, ureg_DECL_sampler(shader, 1));
   }

   for (i = 0; i < NR_RENDER_TARGETS; ++i) {
      if(i > 0)
         increment_addr(shader, r, r_addr, true, true, i, BLOCK_HEIGHT);

      struct ureg_src s_addr[2] = { ureg_src(r[0]), ureg_src(r[1]) };
      s_addr[0] = i == 0 ? r_addr[0] : ureg_src(r[0]);
      s_addr[1] = i == 0 ? r_addr[1] : ureg_src(r[1]);
      fetch_four(shader, r, s_addr, ureg_DECL_sampler(shader, 0));

      for (j = 0; j < 4; ++j) {
         matrix_mul(shader, ureg_writemask(fragment[i], TGSI_WRITEMASK_X << j), l[j], r);
      }
   }

   for (i = 0; i < 4; ++i) {
      ureg_release_temporary(shader, l[i][0]);
      ureg_release_temporary(shader, l[i][1]);
   }
   ureg_release_temporary(shader, r[0]);
   ureg_release_temporary(shader, r[1]);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_transpose_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;

   struct ureg_src l_addr[2], r_addr[2];

   struct ureg_dst l[2], r[2];
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
   l_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1, TGSI_INTERPOLATE_LINEAR);

   r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
   r_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR1, TGSI_INTERPOLATE_LINEAR);

   l[0] = ureg_DECL_temporary(shader);
   l[1] = ureg_DECL_temporary(shader);
   r[0] = ureg_DECL_temporary(shader);
   r[1] = ureg_DECL_temporary(shader);

   fetch_four(shader, l, l_addr, ureg_DECL_sampler(shader, 0));
   fetch_four(shader, r, r_addr, ureg_DECL_sampler(shader, 1));

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   matrix_mul(shader, ureg_writemask(fragment, TGSI_WRITEMASK_X), l, r);

   ureg_release_temporary(shader, l[0]);
   ureg_release_temporary(shader, l[1]);
   ureg_release_temporary(shader, r[0]);
   ureg_release_temporary(shader, r[1]);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static bool
init_shaders(struct vl_idct *idct)
{
   idct->matrix_vs = create_vert_shader(idct, true);
   if (!idct->matrix_vs)
      goto error_matrix_vs;

   idct->matrix_fs = create_matrix_frag_shader(idct);
   if (!idct->matrix_fs)
      goto error_matrix_fs;

   idct->transpose_vs = create_vert_shader(idct, false);
   if (!idct->transpose_vs)
      goto error_transpose_vs;

   idct->transpose_fs = create_transpose_frag_shader(idct);
   if (!idct->transpose_fs)
      goto error_transpose_fs;

   return true;

error_transpose_fs:
   idct->pipe->delete_vs_state(idct->pipe, idct->transpose_vs);

error_transpose_vs:
   idct->pipe->delete_fs_state(idct->pipe, idct->matrix_fs);

error_matrix_fs:
   idct->pipe->delete_vs_state(idct->pipe, idct->matrix_vs);

error_matrix_vs:
   return false;
}

static void
cleanup_shaders(struct vl_idct *idct)
{
   idct->pipe->delete_vs_state(idct->pipe, idct->matrix_vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->matrix_fs);
   idct->pipe->delete_vs_state(idct->pipe, idct->transpose_vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->transpose_fs);
}

static bool
init_state(struct vl_idct *idct)
{
   struct pipe_sampler_state sampler;
   struct pipe_rasterizer_state rs_state;
   unsigned i;

   assert(idct);

   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.gl_rasterization_rules = false;
   idct->rs_state = idct->pipe->create_rasterizer_state(idct->pipe, &rs_state);
   if (!idct->rs_state)
      goto error_rs_state;

   for (i = 0; i < 2; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      idct->samplers[i] = idct->pipe->create_sampler_state(idct->pipe, &sampler);
      if (!idct->samplers[i])
         goto error_samplers;
   }

   return true;

error_samplers:
   for (i = 0; i < 2; ++i)
      if (idct->samplers[i])
         idct->pipe->delete_sampler_state(idct->pipe, idct->samplers[i]);

   idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);

error_rs_state:
   return false;
}

static void
cleanup_state(struct vl_idct *idct)
{
   unsigned i;

   for (i = 0; i < 2; ++i)
      idct->pipe->delete_sampler_state(idct->pipe, idct->samplers[i]);

   idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);
}

static bool
init_intermediate(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   struct pipe_resource tex_templ, *tex;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;
   unsigned i;

   assert(idct && buffer);

   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_3D;
   tex_templ.format = PIPE_FORMAT_R16G16B16A16_SNORM;
   tex_templ.width0 = idct->buffer_width / NR_RENDER_TARGETS;
   tex_templ.height0 = idct->buffer_height / 4;
   tex_templ.depth0 = NR_RENDER_TARGETS;
   tex_templ.array_size = 1;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   tex_templ.usage = PIPE_USAGE_STATIC;

   tex = idct->pipe->screen->resource_create(idct->pipe->screen, &tex_templ);
   if (!tex)
      goto error_tex;

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, tex, tex->format);
   buffer->sampler_views.individual.intermediate =
      idct->pipe->create_sampler_view(idct->pipe, tex, &sv_templ);
   if (!buffer->sampler_views.individual.intermediate)
         goto error_sampler_view;

   buffer->fb_state[0].width = tex->width0;
   buffer->fb_state[0].height = tex->height0;
   buffer->fb_state[0].nr_cbufs = NR_RENDER_TARGETS;
   for(i = 0; i < NR_RENDER_TARGETS; ++i) {
      memset(&surf_templ, 0, sizeof(surf_templ));
      surf_templ.format = tex->format;
      surf_templ.u.tex.first_layer = i;
      surf_templ.u.tex.last_layer = i;
      surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
      buffer->fb_state[0].cbufs[i] = idct->pipe->create_surface(
         idct->pipe, tex, &surf_templ);

      if (!buffer->fb_state[0].cbufs[i])
         goto error_surfaces;
   }

   buffer->viewport[0].scale[0] = tex->width0;
   buffer->viewport[0].scale[1] = tex->height0;

   pipe_resource_reference(&tex, NULL);
   return true;

error_surfaces:
   for(i = 0; i < NR_RENDER_TARGETS; ++i)
      pipe_surface_reference(&buffer->fb_state[0].cbufs[i], NULL);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.intermediate, NULL);

error_sampler_view:
   pipe_resource_reference(&tex, NULL);

error_tex:
   return false;
}

static void
cleanup_intermediate(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   unsigned i;

   assert(idct && buffer);

   for(i = 0; i < NR_RENDER_TARGETS; ++i)
      pipe_surface_reference(&buffer->fb_state[0].cbufs[i], NULL);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.intermediate, NULL);
}

struct pipe_sampler_view *
vl_idct_upload_matrix(struct pipe_context *pipe)
{
   const float scale = sqrtf(SCALE_FACTOR_16_TO_9);

   struct pipe_resource tex_templ, *matrix;
   struct pipe_sampler_view sv_templ, *sv;
   struct pipe_transfer *buf_transfer;
   unsigned i, j, pitch;
   float *f;

   struct pipe_box rect =
   {
      0, 0, 0,
      BLOCK_WIDTH / 4,
      BLOCK_HEIGHT,
      1
   };

   assert(pipe);

   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   tex_templ.last_level = 0;
   tex_templ.width0 = 2;
   tex_templ.height0 = 8;
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.usage = PIPE_USAGE_IMMUTABLE;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   tex_templ.flags = 0;

   matrix = pipe->screen->resource_create(pipe->screen, &tex_templ);
   if (!matrix)
      goto error_matrix;

   buf_transfer = pipe->get_transfer
   (
      pipe, matrix,
      0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &rect
   );
   if (!buf_transfer)
      goto error_transfer;

   pitch = buf_transfer->stride / sizeof(float);

   f = pipe->transfer_map(pipe, buf_transfer);
   if (!f)
      goto error_map;

   for(i = 0; i < BLOCK_HEIGHT; ++i)
      for(j = 0; j < BLOCK_WIDTH; ++j)
         // transpose and scale
         f[i * pitch + j] = const_matrix[j][i] * scale;

   pipe->transfer_unmap(pipe, buf_transfer);
   pipe->transfer_destroy(pipe, buf_transfer);

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, matrix, matrix->format);
   sv = pipe->create_sampler_view(pipe, matrix, &sv_templ);
   pipe_resource_reference(&matrix, NULL);
   if (!sv)
      goto error_map;

   return sv;

error_map:
   pipe->transfer_destroy(pipe, buf_transfer);

error_transfer:
   pipe_resource_reference(&matrix, NULL);

error_matrix:
   return NULL;
}

bool vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe,
                  unsigned buffer_width, unsigned buffer_height,
                  unsigned blocks_x, unsigned blocks_y,
                  struct pipe_sampler_view *matrix)
{
   assert(idct && pipe && matrix);

   idct->pipe = pipe;
   idct->buffer_width = buffer_width;
   idct->buffer_height = buffer_height;
   idct->blocks_x = blocks_x;
   idct->blocks_y = blocks_y;
   pipe_sampler_view_reference(&idct->matrix, matrix);

   if(!init_shaders(idct))
      return false;

   if(!init_state(idct)) {
      cleanup_shaders(idct);
      return false;
   }

   return true;
}

void
vl_idct_cleanup(struct vl_idct *idct)
{
   cleanup_shaders(idct);
   cleanup_state(idct);

   pipe_sampler_view_reference(&idct->matrix, NULL);
}

bool
vl_idct_init_buffer(struct vl_idct *idct, struct vl_idct_buffer *buffer,
                    struct pipe_sampler_view *source, struct pipe_surface *destination)
{
   unsigned i;

   assert(buffer);
   assert(idct);
   assert(source);
   assert(destination);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.matrix, idct->matrix);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.source, source);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.transpose, idct->matrix);

   if (!init_intermediate(idct, buffer))
      return false;

   /* init state */
   buffer->fb_state[1].width = destination->texture->width0;
   buffer->fb_state[1].height = destination->texture->height0;
   buffer->fb_state[1].nr_cbufs = 1;
   pipe_surface_reference(&buffer->fb_state[1].cbufs[0], destination);

   buffer->viewport[1].scale[0] = destination->texture->width0;
   buffer->viewport[1].scale[1] = destination->texture->height0;

   for(i = 0; i < 2; ++i) {
      buffer->viewport[i].scale[2] = 1;
      buffer->viewport[i].scale[3] = 1;
      buffer->viewport[i].translate[0] = 0;
      buffer->viewport[i].translate[1] = 0;
      buffer->viewport[i].translate[2] = 0;
      buffer->viewport[i].translate[3] = 0;

      buffer->fb_state[i].zsbuf = NULL;
   }

   return true;
}

void
vl_idct_cleanup_buffer(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   unsigned i;

   assert(idct && buffer);

   for(i = 0; i < NR_RENDER_TARGETS; ++i)
      pipe_surface_reference(&buffer->fb_state[0].cbufs[i], NULL);

   pipe_surface_reference(&buffer->fb_state[1].cbufs[0], NULL);

   cleanup_intermediate(idct, buffer);
}

void
vl_idct_flush(struct vl_idct *idct, struct vl_idct_buffer *buffer, unsigned num_instances)
{
   unsigned num_verts;

   assert(idct);
   assert(buffer);

   if(num_instances > 0) {
      num_verts = idct->blocks_x * idct->blocks_y * 4;

      idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers);

      /* first stage */
      idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state[0]);
      idct->pipe->set_viewport_state(idct->pipe, &buffer->viewport[0]);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, buffer->sampler_views.stage[0]);
      idct->pipe->bind_vs_state(idct->pipe, idct->matrix_vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->matrix_fs);
      util_draw_arrays_instanced(idct->pipe, PIPE_PRIM_QUADS, 0, num_verts, 0, num_instances);

      /* second stage */
      idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state[1]);
      idct->pipe->set_viewport_state(idct->pipe, &buffer->viewport[1]);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, buffer->sampler_views.stage[1]);
      idct->pipe->bind_vs_state(idct->pipe, idct->transpose_vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->transpose_fs);
      util_draw_arrays_instanced(idct->pipe, PIPE_PRIM_QUADS, 0, num_verts, 0, num_instances);
   }
}
