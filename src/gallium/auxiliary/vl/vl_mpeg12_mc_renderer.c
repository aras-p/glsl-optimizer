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

#include "vl_mpeg12_mc_renderer.h"
#include "util/u_draw.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_memory.h>
#include <util/u_keymap.h>
#include <util/u_sampler.h>
#include <tgsi/tgsi_ureg.h>

#define DEFAULT_BUF_ALIGNMENT 1
#define MACROBLOCK_WIDTH 16
#define MACROBLOCK_HEIGHT 16
#define BLOCK_WIDTH 8
#define BLOCK_HEIGHT 8

struct vertex_shader_consts
{
   struct vertex4f norm;
};

struct vertex_stream_0
{
   struct vertex2f pos;
   struct {
      float y;
      float cr;
      float cb;
   } eb[2][2];
   float interlaced;
};

enum VS_INPUT
{
   VS_I_RECT,
   VS_I_VPOS,
   VS_I_EB_0_0,
   VS_I_EB_0_1,
   VS_I_EB_1_0,
   VS_I_EB_1_1,
   VS_I_INTERLACED,
   VS_I_MV0,
   VS_I_MV1,
   VS_I_MV2,
   VS_I_MV3,

   NUM_VS_INPUTS
};

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_LINE,
   VS_O_TEX0,
   VS_O_TEX1,
   VS_O_TEX2,
   VS_O_EB_0_0,
   VS_O_EB_0_1,
   VS_O_EB_1_0,
   VS_O_EB_1_1,
   VS_O_INTERLACED,
   VS_O_MV0,
   VS_O_MV1,
   VS_O_MV2,
   VS_O_MV3
};

static const unsigned const_mbtype_config[VL_NUM_MACROBLOCK_TYPES][2] = {
   [VL_MACROBLOCK_TYPE_INTRA]           = { 0, 0 },
   [VL_MACROBLOCK_TYPE_FWD_FRAME_PRED]  = { 1, 1 },
   [VL_MACROBLOCK_TYPE_FWD_FIELD_PRED]  = { 1, 2 },
   [VL_MACROBLOCK_TYPE_BKWD_FRAME_PRED] = { 1, 1 },
   [VL_MACROBLOCK_TYPE_BKWD_FIELD_PRED] = { 1, 2 },
   [VL_MACROBLOCK_TYPE_BI_FRAME_PRED]   = { 2, 1 },
   [VL_MACROBLOCK_TYPE_BI_FIELD_PRED]   = { 2, 2 }
};

static void *
create_vert_shader(struct vl_mpeg12_mc_renderer *r, unsigned ref_frames, unsigned mv_per_frame)
{
   struct ureg_program *shader;
   struct ureg_src norm, mbs;
   struct ureg_src vrect, vpos, eb[2][2], interlaced, vmv[4];
   struct ureg_dst scale, t_vpos, t_vtex;
   struct ureg_dst o_vpos, o_line, o_vtex[3], o_eb[2][2], o_interlaced, o_vmv[4];
   unsigned i, j, count, label;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   norm = ureg_DECL_constant(shader, 0);
   mbs = ureg_imm2f(shader, MACROBLOCK_WIDTH, MACROBLOCK_HEIGHT);

   scale = ureg_DECL_temporary(shader);
   t_vpos = ureg_DECL_temporary(shader);
   t_vtex = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);
   eb[0][0] = ureg_DECL_vs_input(shader, VS_I_EB_0_0);
   eb[1][0] = ureg_DECL_vs_input(shader, VS_I_EB_1_0);
   eb[0][1] = ureg_DECL_vs_input(shader, VS_I_EB_0_1);
   eb[1][1] = ureg_DECL_vs_input(shader, VS_I_EB_1_1);
   interlaced = ureg_DECL_vs_input(shader, VS_I_INTERLACED);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_line = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE);   
   o_vtex[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX0);
   o_vtex[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX1);
   o_vtex[2] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX2);   
   o_eb[0][0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0_0);
   o_eb[0][1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0_1);
   o_eb[1][0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1_0);
   o_eb[1][1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1_1);
   o_interlaced = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_INTERLACED);
   
   count=0;
   for (i = 0; i < ref_frames; ++i) {
      for (j = 0; j < 2; ++j) {        
        if(j < mv_per_frame) {
           vmv[count] = ureg_DECL_vs_input(shader, VS_I_MV0 + count);
           o_vmv[count] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV0 + count);
           count++;
        }
      }
   }

   /*
    * scale = norm * mbs;
    *
    * t_vpos = (vpos + vrect) * scale
    * o_vpos.xy = t_vpos
    * o_vpos.zw = vpos
    *
    * o_line = vpos * 8
    *
    * if(interlaced) {
    *    t_vtex.x = vrect.x
    *    t_vtex.y = vrect.y * 0.5
    *    t_vtex += vpos
    *
    *    o_vtex[0].xy = t_vtex * scale
    *
    *    t_vtex.y += 0.5
    *    o_vtex[1].xy = t_vtex * scale
    * } else {
    *    o_vtex[0..1].xy = t_vpos
    * }
    * o_vtex[2].xy = t_vpos
    * o_eb[0..1][0..1] = eb[0..1][0..1]
    * o_interlaced = interlaced
    *
    * if(count > 0) { // Apply motion vectors
    *    scale = norm * 0.5;
    *    o_vmv[0..count] = t_vpos + vmv[0..count] * scale
    * }
    *
    */
   ureg_MUL(shader, ureg_writemask(scale, TGSI_WRITEMASK_XY), norm, mbs);

   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), ureg_src(scale));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   ureg_MUL(shader, ureg_writemask(o_line, TGSI_WRITEMASK_XY), vrect, 
      ureg_imm2f(shader, MACROBLOCK_WIDTH / 2, MACROBLOCK_HEIGHT / 2));

   ureg_IF(shader, interlaced, &label);

      ureg_MOV(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_X), vrect);
      ureg_MUL(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), vrect, ureg_imm1f(shader, 0.5f));
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_XY), vpos, ureg_src(t_vtex));
      ureg_MUL(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vtex), ureg_src(scale));
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), ureg_src(t_vtex), ureg_imm1f(shader, 0.5f));
      ureg_MUL(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vtex), ureg_src(scale));

   ureg_ELSE(shader, &label);

      ureg_MOV(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vpos));
      ureg_MOV(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vpos));

   ureg_ENDIF(shader);
   ureg_MOV(shader, ureg_writemask(o_vtex[2], TGSI_WRITEMASK_XY), ureg_src(t_vpos));

   ureg_MOV(shader, o_eb[0][0], eb[0][0]);
   ureg_MOV(shader, o_eb[0][1], eb[0][1]);
   ureg_MOV(shader, o_eb[1][0], eb[1][0]);
   ureg_MOV(shader, o_eb[1][1], eb[1][1]);

   ureg_MOV(shader, o_interlaced, interlaced);

   if(count > 0) {
      ureg_MUL(shader, ureg_writemask(scale, TGSI_WRITEMASK_XY), norm, ureg_imm1f(shader, 0.5f));
      for (i = 0; i < count; ++i)
         ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), ureg_src(scale), vmv[i], ureg_src(t_vpos));
   }

   ureg_release_temporary(shader, t_vtex);
   ureg_release_temporary(shader, t_vpos);
   ureg_release_temporary(shader, scale);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static struct ureg_dst
calc_field(struct ureg_program *shader)
{
   struct ureg_dst tmp;
   struct ureg_src line;

   tmp = ureg_DECL_temporary(shader);
   line = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE, TGSI_INTERPOLATE_LINEAR);

   /*
    * line going from 0 to 8 in steps of 0.5
    *
    * tmp.z = fraction(line.y)
    * tmp.z = tmp.z >= 0.5 ? 1 : 0
    * tmp.xy = line >= 4 ? 1 : 0
    */
   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z), ureg_scalar(line, TGSI_SWIZZLE_Y));
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z), ureg_src(tmp), ureg_imm1f(shader, 0.5f));
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY), line, ureg_imm2f(shader, BLOCK_WIDTH / 2, BLOCK_HEIGHT / 2));

   return tmp;
}

static struct ureg_dst
fetch_ycbcr(struct vl_mpeg12_mc_renderer *r, struct ureg_program *shader, struct ureg_dst field)
{
   struct ureg_src tc[3], sampler[3], eb[2][2], interlaced;
   struct ureg_dst texel, t_tc, t_eb_info, tmp;
   unsigned i, label, l_x, l_y;

   texel = ureg_DECL_temporary(shader);
   t_tc = ureg_DECL_temporary(shader);
   t_eb_info = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX0, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX1, TGSI_INTERPOLATE_LINEAR);
   tc[2] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX2, TGSI_INTERPOLATE_LINEAR);

   eb[0][0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0_0, TGSI_INTERPOLATE_CONSTANT);
   eb[0][1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0_1, TGSI_INTERPOLATE_CONSTANT);
   eb[1][0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1_0, TGSI_INTERPOLATE_CONSTANT);
   eb[1][1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1_1, TGSI_INTERPOLATE_CONSTANT);

   interlaced = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_INTERLACED, TGSI_INTERPOLATE_CONSTANT);

   for (i = 0; i < 3; ++i)  {
      sampler[i] = ureg_DECL_sampler(shader, i);
   }

   /*
    * texel.y  = tex(field.y ? tc[1] : tc[0], sampler[0])
    * texel.cb = tex(tc[2], sampler[1])
    * texel.cr = tex(tc[2], sampler[2])
    */

   ureg_IF(shader, interlaced, &label);
      ureg_MOV(shader, ureg_writemask(field, TGSI_WRITEMASK_Y), ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z));
   ureg_ENDIF(shader);

   ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Y)),
            tc[1], tc[0]);

   ureg_IF(shader, ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Y), &l_y);

      ureg_IF(shader, ureg_scalar(ureg_src(field), TGSI_SWIZZLE_X), &l_x);
         ureg_MOV(shader, t_eb_info, eb[1][1]);
      ureg_ELSE(shader, &l_x);
         ureg_MOV(shader, t_eb_info, eb[1][0]);
      ureg_ENDIF(shader);

   ureg_ELSE(shader, &l_y);

      ureg_IF(shader, ureg_scalar(ureg_src(field), TGSI_SWIZZLE_X), &l_x);
         ureg_MOV(shader, t_eb_info, eb[0][1]);
      ureg_ELSE(shader, &l_x);
         ureg_MOV(shader, t_eb_info, eb[0][0]);
      ureg_ENDIF(shader);

   ureg_ENDIF(shader);

   for (i = 0; i < 3; ++i) {
      ureg_IF(shader, ureg_scalar(ureg_src(t_eb_info), TGSI_SWIZZLE_X + i), &label);
         ureg_MOV(shader, ureg_writemask(texel, TGSI_WRITEMASK_X << i), ureg_imm1f(shader, 0.0f));
      ureg_ELSE(shader, &label);

         /* Nouveau and r600g can't writemask tex dst regs (yet?), do in two steps */
         if(i==0 || r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_444) {
            ureg_TEX(shader, tmp, TGSI_TEXTURE_3D, ureg_src(t_tc), sampler[i]);
         } else {
            ureg_TEX(shader, tmp, TGSI_TEXTURE_3D, tc[2], sampler[i]);
         }

         ureg_MOV(shader, ureg_writemask(texel, TGSI_WRITEMASK_X << i), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));

      ureg_ENDIF(shader);
   }

   ureg_release_temporary(shader, t_tc);
   ureg_release_temporary(shader, t_eb_info);
   ureg_release_temporary(shader, tmp);

   return texel;
}

static struct ureg_dst
fetch_ref(struct ureg_program *shader, struct ureg_dst field, unsigned ref_frames, unsigned mv_per_frame)
{
   struct ureg_src tc[ref_frames * mv_per_frame], sampler[ref_frames];
   struct ureg_dst ref[ref_frames], t_tc, result;
   unsigned i;

   for (i = 0; i < ref_frames * mv_per_frame; ++i)
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV0 + i, TGSI_INTERPOLATE_LINEAR);

   for (i = 0; i < ref_frames; ++i) {
      sampler[i] = ureg_DECL_sampler(shader, i + 3);
      ref[i] = ureg_DECL_temporary(shader);
   }

   result = ureg_DECL_temporary(shader);

   if (ref_frames == 1) {
      if(mv_per_frame == 1)
         /*
          * result = tex(tc[0], sampler[0])
          */
         ureg_TEX(shader, result, TGSI_TEXTURE_2D, tc[0], sampler[0]);
      else {
         t_tc = ureg_DECL_temporary(shader);
         /*
          * result = tex(field.y ? tc[1] : tc[0], sampler[0])
          */
         ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z)),
            tc[1], tc[0]);
         ureg_TEX(shader, result, TGSI_TEXTURE_2D, ureg_src(t_tc), sampler[0]);

         ureg_release_temporary(shader, t_tc);
      }

   } else if (ref_frames == 2) {
      if(mv_per_frame == 1) {
         /*
          * ref[0..1] = tex(tc[0..1], sampler[0..1])
          */
         ureg_TEX(shader, ref[0], TGSI_TEXTURE_2D, tc[0], sampler[0]);
         ureg_TEX(shader, ref[1], TGSI_TEXTURE_2D, tc[1], sampler[1]);
      } else {
         t_tc = ureg_DECL_temporary(shader);

         /*
          * if (field.y)
          *    ref[0..1] = tex(tc[0..1], sampler[0..1])
          * else
          *    ref[0..1] = tex(tc[2..3], sampler[0..1])
          */
         ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z)),
            tc[1], tc[0]);
         ureg_TEX(shader, ref[0], TGSI_TEXTURE_2D, ureg_src(t_tc), sampler[0]);

         ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z)),
            tc[3], tc[2]);
         ureg_TEX(shader, ref[1], TGSI_TEXTURE_2D, ureg_src(t_tc), sampler[1]);

         ureg_release_temporary(shader, t_tc);
      }

      ureg_LRP(shader, result, ureg_scalar(ureg_imm1f(shader, 0.5f), TGSI_SWIZZLE_X), ureg_src(ref[0]), ureg_src(ref[1]));
   }

   for (i = 0; i < ref_frames; ++i)
      ureg_release_temporary(shader, ref[i]);

   return result;
}

static void *
create_frag_shader(struct vl_mpeg12_mc_renderer *r, unsigned ref_frames, unsigned mv_per_frame)
{
   struct ureg_program *shader;
   struct ureg_src result;
   struct ureg_dst field, texel;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   field = calc_field(shader);
   texel = fetch_ycbcr(r, shader, field);

   if (ref_frames == 0)
      result = ureg_imm1f(shader, 0.5f);
   else
      result = ureg_src(fetch_ref(shader, field, ref_frames, mv_per_frame));

   ureg_ADD(shader, fragment, ureg_src(texel), result);

   ureg_release_temporary(shader, field);
   ureg_release_temporary(shader, texel);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static bool
init_mbtype_handler(struct vl_mpeg12_mc_renderer *r, enum VL_MACROBLOCK_TYPE type,
                    struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS])
{
   unsigned ref_frames, mv_per_frame;
   struct vl_mc_mbtype_handler *handler;
   unsigned i;

   assert(r);

   ref_frames = const_mbtype_config[type][0];
   mv_per_frame = const_mbtype_config[type][1];

   handler = &r->mbtype_handlers[type];

   handler->vs = create_vert_shader(r, ref_frames, mv_per_frame);
   handler->fs = create_frag_shader(r, ref_frames, mv_per_frame);

   if (handler->vs == NULL || handler->fs == NULL)
      return false;

   handler->vertex_elems_state = r->pipe->create_vertex_elements_state(
      r->pipe, 7 + ref_frames * mv_per_frame, vertex_elems);

   if (handler->vertex_elems_state == NULL)
      return false;

   if (!vl_vb_init(&handler->pos, r->macroblocks_per_batch, sizeof(struct vertex_stream_0) / sizeof(float)))
      return false;

   for (i = 0; i < ref_frames * mv_per_frame; ++i) {
      if (!vl_vb_init(&handler->mv[i], r->macroblocks_per_batch, sizeof(struct vertex2f) / sizeof(float)))
         return false;
   }

   return true;
}

static void
cleanup_mbtype_handler(struct vl_mpeg12_mc_renderer *r, enum VL_MACROBLOCK_TYPE type)
{
   unsigned ref_frames, mv_per_frame;
   struct vl_mc_mbtype_handler *handler;
   unsigned i;

   assert(r);

   ref_frames = const_mbtype_config[type][0];
   mv_per_frame = const_mbtype_config[type][1];

   handler = &r->mbtype_handlers[type];

   r->pipe->delete_vs_state(r->pipe, handler->vs);
   r->pipe->delete_fs_state(r->pipe, handler->fs);
   r->pipe->delete_vertex_elements_state(r->pipe, handler->vertex_elems_state);

   vl_vb_cleanup(&handler->pos);

   for (i = 0; i < ref_frames * mv_per_frame; ++i)
      vl_vb_cleanup(&handler->mv[i]);
}


static bool
init_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_sampler_state sampler;
   struct pipe_rasterizer_state rs_state;
   unsigned filters[5];
   unsigned i;

   assert(r);

   r->viewport.scale[0] = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   r->viewport.scale[1] = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   r->viewport.scale[2] = 1;
   r->viewport.scale[3] = 1;
   r->viewport.translate[0] = 0;
   r->viewport.translate[1] = 0;
   r->viewport.translate[2] = 0;
   r->viewport.translate[3] = 0;

   r->fb_state.width = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   r->fb_state.height = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   r->fb_state.nr_cbufs = 1;
   r->fb_state.zsbuf = NULL;

   /* Luma filter */
   filters[0] = PIPE_TEX_FILTER_NEAREST;
   /* Chroma filters */
   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_444 || true) { //TODO
      filters[1] = PIPE_TEX_FILTER_NEAREST;
      filters[2] = PIPE_TEX_FILTER_NEAREST;
   }
   else {
      filters[1] = PIPE_TEX_FILTER_LINEAR;
      filters[2] = PIPE_TEX_FILTER_LINEAR;
   }
   /* Fwd, bkwd ref filters */
   filters[3] = PIPE_TEX_FILTER_LINEAR;
   filters[4] = PIPE_TEX_FILTER_LINEAR;

   for (i = 0; i < 5; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      sampler.min_img_filter = filters[i];
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = filters[i];
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      /*sampler.shadow_ambient = ; */
      /*sampler.lod_bias = ; */
      sampler.min_lod = 0;
      /*sampler.max_lod = ; */
      sampler.border_color[0] = 0.0f;
      sampler.border_color[1] = 0.0f;
      sampler.border_color[2] = 0.0f;
      sampler.border_color[3] = 0.0f;
      /*sampler.max_anisotropy = ; */
      r->samplers.all[i] = r->pipe->create_sampler_state(r->pipe, &sampler);
   }

   memset(&rs_state, 0, sizeof(rs_state));
   /*rs_state.sprite_coord_enable */
   rs_state.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
   rs_state.point_quad_rasterization = true;
   rs_state.point_size = BLOCK_WIDTH;
   rs_state.gl_rasterization_rules = true;
   r->rs_state = r->pipe->create_rasterizer_state(r->pipe, &rs_state);

   return true;
}

static void
cleanup_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 5; ++i)
      r->pipe->delete_sampler_state(r->pipe, r->samplers.all[i]);

   r->pipe->delete_rasterizer_state(r->pipe, r->rs_state);
}

static bool
init_buffers(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_resource template;
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];
   struct pipe_sampler_view sampler_view;

   const unsigned mbw =
      align(r->picture_width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;
   const unsigned mbh =
      align(r->picture_height, MACROBLOCK_HEIGHT) / MACROBLOCK_HEIGHT;

   unsigned i;

   assert(r);

   r->macroblocks_per_batch =
      mbw * (r->bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE ? mbh : 1);
   r->num_macroblocks = 0;

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   /* TODO: Accomodate HW that can't do this and also for cases when this isn't precise enough */
   template.format = PIPE_FORMAT_R16_SNORM;
   template.last_level = 0;
   template.width0 = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   template.height0 = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_DYNAMIC;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   r->textures.individual.y = r->pipe->screen->resource_create(r->pipe->screen, &template);

   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      template.width0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_width / 2) :
         r->picture_width / 2;
      template.height0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_height / 2) :
         r->picture_height / 2;
   }
   else if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422)
      template.height0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_height / 2) :
         r->picture_height / 2;

   r->textures.individual.cb =
      r->pipe->screen->resource_create(r->pipe->screen, &template);
   r->textures.individual.cr =
      r->pipe->screen->resource_create(r->pipe->screen, &template);

   for (i = 0; i < 3; ++i) {
      u_sampler_view_default_template(&sampler_view,
                                      r->textures.all[i],
                                      r->textures.all[i]->format);
      r->sampler_views.all[i] = r->pipe->create_sampler_view(r->pipe, r->textures.all[i], &sampler_view);
   }

   r->vertex_bufs.individual.quad = vl_vb_upload_quads(r->pipe, r->macroblocks_per_batch);

   r->vertex_bufs.individual.pos.stride = sizeof(struct vertex_stream_0);
   r->vertex_bufs.individual.pos.max_index = 4 * r->macroblocks_per_batch - 1;
   r->vertex_bufs.individual.pos.buffer_offset = 0;
   /* XXX: Create with usage DYNAMIC or STREAM */
   r->vertex_bufs.individual.pos.buffer = pipe_buffer_create
   (
      r->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex_stream_0) * 4 * r->macroblocks_per_batch
   );

   for (i = 0; i < 4; ++i) {
      r->vertex_bufs.individual.mv[i].stride = sizeof(struct vertex2f);
      r->vertex_bufs.individual.mv[i].max_index = 4 * r->macroblocks_per_batch - 1;
      r->vertex_bufs.individual.mv[i].buffer_offset = 0;
      /* XXX: Create with usage DYNAMIC or STREAM */
      r->vertex_bufs.individual.mv[i].buffer = pipe_buffer_create
      (
         r->pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         sizeof(struct vertex2f) * 4 * r->macroblocks_per_batch
      );
   }

   memset(&vertex_elems, 0, sizeof(vertex_elems));

   /* Rectangle element */
   vertex_elems[VS_I_RECT].src_offset = 0;
   vertex_elems[VS_I_RECT].instance_divisor = 0;
   vertex_elems[VS_I_RECT].vertex_buffer_index = 0;
   vertex_elems[VS_I_RECT].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Position element */
   vertex_elems[VS_I_VPOS].src_offset = 0;
   vertex_elems[VS_I_VPOS].instance_divisor = 0;
   vertex_elems[VS_I_VPOS].vertex_buffer_index = 1;
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* y, cr, cb empty block element top left block */
   vertex_elems[VS_I_EB_0_0].src_offset = sizeof(float) * 2;
   vertex_elems[VS_I_EB_0_0].instance_divisor = 0;
   vertex_elems[VS_I_EB_0_0].vertex_buffer_index = 1;
   vertex_elems[VS_I_EB_0_0].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element top right block */
   vertex_elems[VS_I_EB_0_1].src_offset = sizeof(float) * 5;
   vertex_elems[VS_I_EB_0_1].instance_divisor = 0;
   vertex_elems[VS_I_EB_0_1].vertex_buffer_index = 1;
   vertex_elems[VS_I_EB_0_1].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element bottom left block */
   vertex_elems[VS_I_EB_1_0].src_offset = sizeof(float) * 8;
   vertex_elems[VS_I_EB_1_0].instance_divisor = 0;
   vertex_elems[VS_I_EB_1_0].vertex_buffer_index = 1;
   vertex_elems[VS_I_EB_1_0].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element bottom right block */
   vertex_elems[VS_I_EB_1_1].src_offset = sizeof(float) * 11;
   vertex_elems[VS_I_EB_1_1].instance_divisor = 0;
   vertex_elems[VS_I_EB_1_1].vertex_buffer_index = 1;
   vertex_elems[VS_I_EB_1_1].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* progressive=0.0f interlaced=1.0f */
   vertex_elems[VS_I_INTERLACED].src_offset = sizeof(float) * 14;
   vertex_elems[VS_I_INTERLACED].instance_divisor = 0;
   vertex_elems[VS_I_INTERLACED].vertex_buffer_index = 1;
   vertex_elems[VS_I_INTERLACED].src_format = PIPE_FORMAT_R32_FLOAT;

   /* First ref surface top field texcoord element */
   vertex_elems[VS_I_MV0].src_offset = 0;
   vertex_elems[VS_I_MV0].instance_divisor = 0;
   vertex_elems[VS_I_MV0].vertex_buffer_index = 2;
   vertex_elems[VS_I_MV0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* First ref surface bottom field texcoord element */
   vertex_elems[VS_I_MV1].src_offset = 0;
   vertex_elems[VS_I_MV1].instance_divisor = 0;
   vertex_elems[VS_I_MV1].vertex_buffer_index = 3;
   vertex_elems[VS_I_MV1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface top field texcoord element */
   vertex_elems[VS_I_MV2].src_offset = 0;
   vertex_elems[VS_I_MV2].instance_divisor = 0;
   vertex_elems[VS_I_MV2].vertex_buffer_index = 4;
   vertex_elems[VS_I_MV2].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface bottom field texcoord element */
   vertex_elems[VS_I_MV3].src_offset = 0;
   vertex_elems[VS_I_MV3].instance_divisor = 0;
   vertex_elems[VS_I_MV3].vertex_buffer_index = 5;
   vertex_elems[VS_I_MV3].src_format = PIPE_FORMAT_R32G32_FLOAT;

   for(i = 0; i < VL_NUM_MACROBLOCK_TYPES; ++i)
      init_mbtype_handler(r, i, vertex_elems);

   r->vs_const_buf = pipe_buffer_create
   (
      r->pipe->screen,
      PIPE_BIND_CONSTANT_BUFFER,
      sizeof(struct vertex_shader_consts)
   );

   return true;
}

static void
cleanup_buffers(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   pipe_resource_reference(&r->vs_const_buf, NULL);

   for (i = 0; i < 3; ++i) {
      pipe_sampler_view_reference(&r->sampler_views.all[i], NULL);
      pipe_resource_reference(&r->vertex_bufs.all[i].buffer, NULL);
      pipe_resource_reference(&r->textures.all[i], NULL);
   }

   for(i = 0; i<VL_NUM_MACROBLOCK_TYPES; ++i)
      cleanup_mbtype_handler(r, i);
}

static enum VL_MACROBLOCK_TYPE
get_macroblock_type(struct pipe_mpeg12_macroblock *mb)
{
   assert(mb);

   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
         return VL_MACROBLOCK_TYPE_INTRA;
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            VL_MACROBLOCK_TYPE_FWD_FRAME_PRED : VL_MACROBLOCK_TYPE_FWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            VL_MACROBLOCK_TYPE_BKWD_FRAME_PRED : VL_MACROBLOCK_TYPE_BKWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            VL_MACROBLOCK_TYPE_BI_FRAME_PRED : VL_MACROBLOCK_TYPE_BI_FIELD_PRED;
      default:
         assert(0);
   }

   /* Unreachable */
   return -1;
}

static void
upload_vertex_stream(struct vl_mpeg12_mc_renderer *r,
                      unsigned num_macroblocks[VL_NUM_MACROBLOCK_TYPES])
{
   struct vertex_stream_0 *pos;
   struct vertex2f *mv[4];

   struct pipe_transfer *buf_transfer[5];

   unsigned i, j;

   assert(r);
   assert(num_macroblocks);

   pos = (struct vertex_stream_0 *)pipe_buffer_map
   (
      r->pipe,
      r->vertex_bufs.individual.pos.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer[0]
   );

   for (i = 0; i < 4; ++i)
      mv[i] = (struct vertex2f *)pipe_buffer_map
      (
         r->pipe,
         r->vertex_bufs.individual.mv[i].buffer,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &buf_transfer[i + 1]
      );

   for (i = 0; i < VL_NUM_MACROBLOCK_TYPES; ++i) {
      struct vl_mc_mbtype_handler *handler = &r->mbtype_handlers[i];
      unsigned count = vl_vb_upload(&handler->pos, pos);
      if (count > 0) {
         pos += count;

         unsigned ref_frames, mv_per_frame;

         ref_frames = const_mbtype_config[i][0];
         mv_per_frame = const_mbtype_config[i][1];

         for (j = 0; j < ref_frames * mv_per_frame; ++j)
            vl_vb_upload(&handler->mv[j], mv[j]);

         for (j = 0; j < 4; ++j)
            mv[j] += count;
      }
      num_macroblocks[i] = count;
   }

   pipe_buffer_unmap(r->pipe, r->vertex_bufs.individual.pos.buffer, buf_transfer[0]);
   for (i = 0; i < 4; ++i)
      pipe_buffer_unmap(r->pipe, r->vertex_bufs.individual.mv[i].buffer, buf_transfer[i + 1]);
}

static struct pipe_sampler_view
*find_or_create_sampler_view(struct vl_mpeg12_mc_renderer *r, struct pipe_surface *surface)
{
   struct pipe_sampler_view *sampler_view;
   assert(r);
   assert(surface);

   sampler_view = (struct pipe_sampler_view*)util_keymap_lookup(r->texview_map, &surface);
   if (!sampler_view) {
      struct pipe_sampler_view templat;
      boolean added_to_map;

      u_sampler_view_default_template(&templat, surface->texture,
                                      surface->texture->format);
      sampler_view = r->pipe->create_sampler_view(r->pipe, surface->texture,
                                                  &templat);
      if (!sampler_view)
         return NULL;

      added_to_map = util_keymap_insert(r->texview_map, &surface,
                                        sampler_view, r->pipe);
      assert(added_to_map);
   }

   return sampler_view;
}

static unsigned
flush_mbtype_handler(struct vl_mpeg12_mc_renderer *r, enum VL_MACROBLOCK_TYPE type,
                     unsigned vb_start, unsigned num_macroblocks)
{
   unsigned ref_frames, mv_per_frame;
   struct vl_mc_mbtype_handler *handler;

   assert(r);

   ref_frames = const_mbtype_config[type][0];
   mv_per_frame = const_mbtype_config[type][1];

   handler = &r->mbtype_handlers[type];

   r->pipe->set_vertex_buffers(r->pipe, 2 + ref_frames * mv_per_frame, r->vertex_bufs.all);
   r->pipe->bind_vertex_elements_state(r->pipe, handler->vertex_elems_state);

   if(ref_frames == 2) {

      r->textures.individual.ref[0] = r->past->texture;
      r->textures.individual.ref[1] = r->future->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->past);
      r->sampler_views.individual.ref[1] = find_or_create_sampler_view(r, r->future);

   } else if(ref_frames == 1) {

      struct pipe_surface *ref;

      if(type == VL_MACROBLOCK_TYPE_BKWD_FRAME_PRED ||
         type == VL_MACROBLOCK_TYPE_BKWD_FIELD_PRED)
         ref = r->future;
      else
         ref = r->past;

      r->textures.individual.ref[0] = ref->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, ref);
   }

   r->pipe->set_fragment_sampler_views(r->pipe, 3 + ref_frames, r->sampler_views.all);
   r->pipe->bind_fragment_sampler_states(r->pipe, 3 + ref_frames, r->samplers.all);
   r->pipe->bind_vs_state(r->pipe, handler->vs);
   r->pipe->bind_fs_state(r->pipe, handler->fs);

   util_draw_arrays(r->pipe, PIPE_PRIM_QUADS, vb_start, num_macroblocks);
   return num_macroblocks;
}

static void
flush(struct vl_mpeg12_mc_renderer *r)
{
   unsigned num_verts[VL_NUM_MACROBLOCK_TYPES] = { 0 };
   unsigned vb_start = 0, i;

   assert(r);
   assert(r->num_macroblocks == r->macroblocks_per_batch);

   vl_idct_flush(&r->idct_y);
   vl_idct_flush(&r->idct_cr);
   vl_idct_flush(&r->idct_cb);

   upload_vertex_stream(r, num_verts);

   r->pipe->bind_rasterizer_state(r->pipe, r->rs_state);
   r->pipe->set_framebuffer_state(r->pipe, &r->fb_state);
   r->pipe->set_viewport_state(r->pipe, &r->viewport);

   for (i = 0; i < VL_NUM_MACROBLOCK_TYPES; ++i) {
      if (num_verts[i] > 0)
         vb_start += flush_mbtype_handler(r, i, vb_start, num_verts[i]);
   }

   r->pipe->flush(r->pipe, PIPE_FLUSH_RENDER_CACHE, r->fence);

   r->num_macroblocks = 0;
}

static void
update_render_target(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_transfer *buf_transfer;
   struct vertex_shader_consts *vs_consts;

   vs_consts = pipe_buffer_map
   (
      r->pipe, r->vs_const_buf,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer
   );

   vs_consts->norm.x = 1.0f / r->surface->width;
   vs_consts->norm.y = 1.0f / r->surface->height;

   pipe_buffer_unmap(r->pipe, r->vs_const_buf, buf_transfer);

   r->fb_state.cbufs[0] = r->surface;

   r->pipe->set_constant_buffer(r->pipe, PIPE_SHADER_VERTEX, 0, r->vs_const_buf);
}

static void
get_motion_vectors(struct pipe_mpeg12_macroblock *mb, struct vertex2f mv[4])
{
   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
      {
         if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {

            mv[1].x = mb->pmv[0][1][0];
            mv[1].y = mb->pmv[0][1][1];

         } else {
            mv[2].x = mb->pmv[0][1][0];
            mv[2].y = mb->pmv[0][1][1] - (mb->pmv[0][1][1] % 4);

            mv[3].x = mb->pmv[1][1][0];
            mv[3].y = mb->pmv[1][1][1] - (mb->pmv[1][1][1] % 4);

            if(mb->mvfs[0][1]) mv[2].y += 2;
            if(!mb->mvfs[1][1]) mv[3].y -= 2;
         }

         /* fall-through */
      }
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
      {
         if (mb->mb_type == PIPE_MPEG12_MACROBLOCK_TYPE_BKWD) {

            if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
               mv[0].x = mb->pmv[0][1][0];
               mv[0].y = mb->pmv[0][1][1];

            } else {
               mv[0].x = mb->pmv[0][1][0];
               mv[0].y = mb->pmv[0][1][1] - (mb->pmv[0][1][1] % 4);

               mv[1].x = mb->pmv[1][1][0];
               mv[1].y = mb->pmv[1][1][1] - (mb->pmv[1][1][1] % 4);

               if(mb->mvfs[0][1]) mv[0].y += 2;
               if(!mb->mvfs[1][1]) mv[1].y -= 2;
            }

         } else {

            if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
               mv[0].x = mb->pmv[0][0][0];
               mv[0].y = mb->pmv[0][0][1];

            } else {
               mv[0].x = mb->pmv[0][0][0];
               mv[0].y = mb->pmv[0][0][1] - (mb->pmv[0][0][1] % 4);

               mv[1].x = mb->pmv[1][0][0];
               mv[1].y = mb->pmv[1][0][1] - (mb->pmv[1][0][1] % 4);

               if(mb->mvfs[0][0]) mv[0].y += 2;
               if(!mb->mvfs[1][0]) mv[1].y -= 2;
            }
         }
      }
      default:
         break;
   }
}

static bool
empty_block(enum pipe_video_chroma_format chroma_format,
            unsigned cbp, unsigned component,
            unsigned x, unsigned y)
{
   /* TODO: Implement 422, 444 */
   assert(chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   if(component == 0) /*luma*/
      return !(cbp  & (1 << (5 - (x + y * 2))));
   else /*cr cb*/
      return !(cbp & (1 << (2 - component)));
}

static void
grab_vectors(struct vl_mpeg12_mc_renderer *r,
             struct pipe_mpeg12_macroblock *mb)
{
   enum VL_MACROBLOCK_TYPE type;
   struct vl_mc_mbtype_handler *handler;
   struct vertex2f mv[4];
   struct vertex_stream_0 info;

   unsigned ref_frames, mv_per_frame;
   unsigned i, j, pos;

   assert(r);
   assert(mb);

   type = get_macroblock_type(mb);

   ref_frames = const_mbtype_config[type][0];
   mv_per_frame = const_mbtype_config[type][1];

   handler = &r->mbtype_handlers[type];

   pos = handler->pos.num_verts;

   info.pos.x = mb->mbx;
   info.pos.y = mb->mby;
   for ( i = 0; i < 2; ++i) {
      for ( j = 0; j < 2; ++j) {
         info.eb[i][j].y = empty_block(r->chroma_format, mb->cbp, 0, j, i);
         info.eb[i][j].cr = empty_block(r->chroma_format, mb->cbp, 1, j, i);
         info.eb[i][j].cb = empty_block(r->chroma_format, mb->cbp, 2, j, i);         
      }
   }
   info.interlaced = mb->dct_type == PIPE_MPEG12_DCT_TYPE_FIELD ? 1.0f : 0.0f;
   vl_vb_add_block(&handler->pos, (float*)&info);

   get_motion_vectors(mb, mv);
   for ( j = 0; j < ref_frames * mv_per_frame; ++j )
      vl_vb_add_block(&handler->mv[j], (float*)&mv[j]);
}

static void
grab_blocks(struct vl_mpeg12_mc_renderer *r, unsigned mbx, unsigned mby,
            enum pipe_mpeg12_dct_type dct_type, unsigned cbp, short *blocks)
{
   unsigned tb = 0;
   unsigned x, y;

   assert(r);
   assert(blocks);

   for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, ++tb) {
         if (!empty_block(r->chroma_format, cbp, 0, x, y)) {
            vl_idct_add_block(&r->idct_y, mbx * 2 + x, mby * 2 + y, blocks);
            blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   for (tb = 1; tb < 3; ++tb) {
      if (!empty_block(r->chroma_format, cbp, tb, 0, 0)) {
         if(tb == 1)
            vl_idct_add_block(&r->idct_cb, mbx, mby, blocks);
         else
            vl_idct_add_block(&r->idct_cr, mbx, mby, blocks);
         blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
      }
   }
}

static void
grab_macroblock(struct vl_mpeg12_mc_renderer *r,
                struct pipe_mpeg12_macroblock *mb)
{
   assert(r);
   assert(mb);
   assert(mb->blocks);
   assert(r->num_macroblocks < r->macroblocks_per_batch);

   grab_vectors(r, mb);
   grab_blocks(r, mb->mbx, mb->mby, mb->dct_type, mb->cbp, mb->blocks);

   ++r->num_macroblocks;
}

static void
texview_map_delete(const struct keymap *map,
                   const void *key, void *data,
                   void *user)
{
   struct pipe_sampler_view *sv = (struct pipe_sampler_view*)data;

   assert(map);
   assert(key);
   assert(data);
   assert(user);

   pipe_sampler_view_reference(&sv, NULL);
}

bool
vl_mpeg12_mc_renderer_init(struct vl_mpeg12_mc_renderer *renderer,
                           struct pipe_context *pipe,
                           unsigned picture_width,
                           unsigned picture_height,
                           enum pipe_video_chroma_format chroma_format,
                           enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode,
                           bool pot_buffers)
{
   struct pipe_resource *idct_matrix;

   assert(renderer);
   assert(pipe);

   /* TODO: Implement other policies */
   assert(bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE);
   /* TODO: Non-pot buffers untested, probably doesn't work without changes to texcoord generation, vert shader, etc */
   assert(pot_buffers);

   memset(renderer, 0, sizeof(struct vl_mpeg12_mc_renderer));

   renderer->pipe = pipe;
   renderer->picture_width = picture_width;
   renderer->picture_height = picture_height;
   renderer->chroma_format = chroma_format;
   renderer->bufmode = bufmode;
   renderer->pot_buffers = pot_buffers;

   renderer->texview_map = util_new_keymap(sizeof(struct pipe_surface*), -1,
                                           texview_map_delete);
   if (!renderer->texview_map)
      return false;

   if (!init_pipe_state(renderer))
      goto error_pipe_state;

   if (!init_buffers(renderer))
      goto error_buffers;

   renderer->surface = NULL;
   renderer->past = NULL;
   renderer->future = NULL;
   renderer->num_macroblocks = 0;

   if(!(idct_matrix = vl_idct_upload_matrix(pipe)))
      goto error_idct_matrix;

   if(!vl_idct_init(&renderer->idct_y, pipe, renderer->textures.individual.y, idct_matrix))
      goto error_idct_y;

   if(!vl_idct_init(&renderer->idct_cr, pipe, renderer->textures.individual.cr, idct_matrix))
      goto error_idct_cr;

   if(!vl_idct_init(&renderer->idct_cb, pipe, renderer->textures.individual.cb, idct_matrix))
      goto error_idct_cb;

   return true;

error_idct_cb:
   vl_idct_cleanup(&renderer->idct_cr);

error_idct_cr:
   vl_idct_cleanup(&renderer->idct_y);

error_idct_y:
error_idct_matrix:
   cleanup_buffers(renderer);

error_buffers:
   cleanup_pipe_state(renderer);

error_pipe_state:
   util_delete_keymap(renderer->texview_map, renderer->pipe);
   return false;
}

void
vl_mpeg12_mc_renderer_cleanup(struct vl_mpeg12_mc_renderer *renderer)
{
   assert(renderer);

   vl_idct_cleanup(&renderer->idct_y);
   vl_idct_cleanup(&renderer->idct_cr);
   vl_idct_cleanup(&renderer->idct_cb);

   util_delete_keymap(renderer->texview_map, renderer->pipe);
   cleanup_pipe_state(renderer);
   cleanup_buffers(renderer);

   pipe_surface_reference(&renderer->surface, NULL);
   pipe_surface_reference(&renderer->past, NULL);
   pipe_surface_reference(&renderer->future, NULL);
}

void
vl_mpeg12_mc_renderer_render_macroblocks(struct vl_mpeg12_mc_renderer
                                         *renderer,
                                         struct pipe_surface *surface,
                                         struct pipe_surface *past,
                                         struct pipe_surface *future,
                                         unsigned num_macroblocks,
                                         struct pipe_mpeg12_macroblock
                                         *mpeg12_macroblocks,
                                         struct pipe_fence_handle **fence)
{
   bool new_surface = false;

   assert(renderer);
   assert(surface);
   assert(num_macroblocks);
   assert(mpeg12_macroblocks);

   if (renderer->surface) {
      if (surface != renderer->surface) {
         if (renderer->num_macroblocks > 0) {
            flush(renderer);
         }

         new_surface = true;
      }

      /* If the surface we're rendering hasn't changed the ref frames shouldn't change. */
      assert(surface != renderer->surface || renderer->past == past);
      assert(surface != renderer->surface || renderer->future == future);
   }
   else
      new_surface = true;

   if (new_surface) {
      pipe_surface_reference(&renderer->surface, surface);
      pipe_surface_reference(&renderer->past, past);
      pipe_surface_reference(&renderer->future, future);
      renderer->fence = fence;
      update_render_target(renderer);
   }

   while (num_macroblocks) {
      unsigned left_in_batch = renderer->macroblocks_per_batch - renderer->num_macroblocks;
      unsigned num_to_submit = MIN2(num_macroblocks, left_in_batch);
      unsigned i;

      for (i = 0; i < num_to_submit; ++i) {
         assert(mpeg12_macroblocks[i].base.codec == PIPE_VIDEO_CODEC_MPEG12);
         grab_macroblock(renderer, &mpeg12_macroblocks[i]);
      }

      num_macroblocks -= num_to_submit;

      if (renderer->num_macroblocks == renderer->macroblocks_per_batch) {
         flush(renderer);
         /* Next time we get this surface it may have new ref frames */
         pipe_surface_reference(&renderer->surface, NULL);
         pipe_surface_reference(&renderer->past, NULL);
         pipe_surface_reference(&renderer->future, NULL);
      }
   }
}
