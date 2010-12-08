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

struct vertex_stream_0
{
   struct vertex2f pos;
   struct {
      float y;
      float cr;
      float cb;
   } eb[2][2];
   float interlaced;
   float frame_pred;
   float ref_frames;
   float bkwd_pred;
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
   VS_I_FRAME_PRED,
   VS_I_REF_FRAMES,
   VS_I_BKWD_PRED,
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
   VS_O_EB_0,
   VS_O_EB_1,
   VS_O_FRAME_PRED,
   VS_O_REF_FRAMES,
   VS_O_BKWD_PRED,
   VS_O_MV0,
   VS_O_MV1,
   VS_O_MV2,
   VS_O_MV3
};

static void *
create_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src scale;
   struct ureg_src vrect, vpos, eb[2][2], vmv[4];
   struct ureg_src interlaced, frame_pred, ref_frames, bkwd_pred;
   struct ureg_dst t_vpos, t_vtex, t_vmv;
   struct ureg_dst o_vpos, o_line, o_vtex[3], o_eb[2], o_vmv[4];
   struct ureg_dst o_frame_pred, o_ref_frames, o_bkwd_pred;
   unsigned i, label;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   t_vpos = ureg_DECL_temporary(shader);
   t_vtex = ureg_DECL_temporary(shader);
   t_vmv = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);
   eb[0][0] = ureg_DECL_vs_input(shader, VS_I_EB_0_0);
   eb[1][0] = ureg_DECL_vs_input(shader, VS_I_EB_1_0);
   eb[0][1] = ureg_DECL_vs_input(shader, VS_I_EB_0_1);
   eb[1][1] = ureg_DECL_vs_input(shader, VS_I_EB_1_1);
   interlaced = ureg_DECL_vs_input(shader, VS_I_INTERLACED);
   frame_pred = ureg_DECL_vs_input(shader, VS_I_FRAME_PRED);
   ref_frames = ureg_DECL_vs_input(shader, VS_I_REF_FRAMES);
   bkwd_pred = ureg_DECL_vs_input(shader, VS_I_BKWD_PRED);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_line = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE);   
   o_vtex[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX0);
   o_vtex[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX1);
   o_vtex[2] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX2);   
   o_eb[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0);
   o_eb[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1);
   o_frame_pred = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_FRAME_PRED);
   o_ref_frames = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_REF_FRAMES);
   o_bkwd_pred = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_BKWD_PRED);
   
   for (i = 0; i < 4; ++i) {
     vmv[i] = ureg_DECL_vs_input(shader, VS_I_MV0 + i);
     o_vmv[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV0 + i);
   }

   /*
    * scale = (MACROBLOCK_WIDTH, MACROBLOCK_HEIGHT) / (dst.width, dst.height)
    *
    * t_vpos = (vpos + vrect) * scale
    * o_vpos.xy = t_vpos
    * o_vpos.zw = vpos
    *
    * o_line.xy = vrect * 8
    * o_line.z = interlaced
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
    *
    * o_eb[0..1] = vrect.x ? eb[0..1][1] : eb[0..1][0]
    *
    * o_frame_pred = frame_pred
    * o_ref_frames = ref_frames
    * o_bkwd_pred = bkwd_pred
    *
    * // Apply motion vectors
    * scale = 0.5 / (dst.width, dst.height);
    * o_vmv[0..count] = t_vpos + vmv[0..count] * scale
    *
    */
   scale = ureg_imm2f(shader,
      (float)MACROBLOCK_WIDTH / r->buffer_width,
      (float)MACROBLOCK_HEIGHT / r->buffer_height);

   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   ureg_MUL(shader, ureg_writemask(o_line, TGSI_WRITEMASK_XY), vrect, 
      ureg_imm2f(shader, MACROBLOCK_WIDTH / 2, MACROBLOCK_HEIGHT / 2));
   ureg_MOV(shader, ureg_writemask(o_line, TGSI_WRITEMASK_Z), 
      ureg_scalar(interlaced, TGSI_SWIZZLE_X));

   ureg_IF(shader, interlaced, &label);

      ureg_MOV(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_X), vrect);
      ureg_MUL(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), vrect, ureg_imm1f(shader, 0.5f));
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_XY), vpos, ureg_src(t_vtex));
      ureg_MUL(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vtex), scale);
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), ureg_src(t_vtex), ureg_imm1f(shader, 0.5f));
      ureg_MUL(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vtex), scale);

   ureg_ELSE(shader, &label);

      ureg_MOV(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vpos));
      ureg_MOV(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vpos));

   ureg_ENDIF(shader);
   ureg_MOV(shader, ureg_writemask(o_vtex[2], TGSI_WRITEMASK_XY), ureg_src(t_vpos));

   ureg_CMP(shader, ureg_writemask(o_eb[0], TGSI_WRITEMASK_XYZ),
            ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_X)),
            eb[0][1], eb[0][0]);
   ureg_CMP(shader, ureg_writemask(o_eb[1], TGSI_WRITEMASK_XYZ),
            ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_X)),
            eb[1][1], eb[1][0]);

   ureg_MOV(shader, ureg_writemask(o_frame_pred, TGSI_WRITEMASK_X), frame_pred);
   ureg_MOV(shader, ureg_writemask(o_ref_frames, TGSI_WRITEMASK_X), ref_frames);
   ureg_MOV(shader, ureg_writemask(o_bkwd_pred, TGSI_WRITEMASK_X), bkwd_pred);

   scale = ureg_imm2f(shader,
      0.5f / r->buffer_width,
      0.5f / r->buffer_height);

   for (i = 0; i < 4; i++)
      ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), scale, vmv[i], ureg_src(t_vpos));

   ureg_release_temporary(shader, t_vtex);
   ureg_release_temporary(shader, t_vpos);
   ureg_release_temporary(shader, t_vmv);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static struct ureg_dst
calc_field(struct ureg_program *shader)
{
   struct ureg_dst tmp;
   struct ureg_src line, frame_pred;

   tmp = ureg_DECL_temporary(shader);

   line = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE, TGSI_INTERPOLATE_LINEAR);
   frame_pred = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_FRAME_PRED, TGSI_INTERPOLATE_CONSTANT);

   /*
    * line.xy going from 0 to 8 in steps of 0.5
    * line.z flag that controls interlacing
    *
    * tmp.z = fraction(line.y)
    * tmp.z = tmp.z >= 0.5 ? 1 : 0
    * tmp.xy = line >= 4 ? 1 : 0
    * tmp.w = line.z ? tmp.z : tmp.y
    * tmp.z = frame_pred ? 0.0f : tmp.z
    */
   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z), ureg_scalar(line, TGSI_SWIZZLE_Y));
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z), ureg_src(tmp), ureg_imm1f(shader, 0.5f));
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY), line, ureg_imm2f(shader, BLOCK_WIDTH / 2, BLOCK_HEIGHT / 2));

   ureg_CMP(shader, ureg_writemask(tmp, TGSI_WRITEMASK_W),
            ureg_negate(ureg_scalar(line, TGSI_SWIZZLE_Z)),
            ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Z),
            ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));

   ureg_CMP(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z),
            ureg_negate(ureg_scalar(frame_pred, TGSI_SWIZZLE_X)),
            ureg_imm1f(shader, 0.0f),
            ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Z));

   return tmp;
}

static struct ureg_dst
fetch_ycbcr(struct vl_mpeg12_mc_renderer *r, struct ureg_program *shader, struct ureg_dst field)
{
   struct ureg_src tc[3], sampler[3], eb[2];
   struct ureg_dst texel, t_tc, t_eb_info, tmp;
   unsigned i, label;

   texel = ureg_DECL_temporary(shader);
   t_tc = ureg_DECL_temporary(shader);
   t_eb_info = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX0, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX1, TGSI_INTERPOLATE_LINEAR);
   tc[2] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX2, TGSI_INTERPOLATE_LINEAR);

   eb[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_0, TGSI_INTERPOLATE_CONSTANT);
   eb[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_EB_1, TGSI_INTERPOLATE_CONSTANT);

   for (i = 0; i < 3; ++i)  {
      sampler[i] = ureg_DECL_sampler(shader, i);
   }

   /*
    * texel.y  = tex(field.y ? tc[1] : tc[0], sampler[0])
    * texel.cb = tex(tc[2], sampler[1])
    * texel.cr = tex(tc[2], sampler[2])
    */

   ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_W)),
            tc[1], tc[0]);

   ureg_CMP(shader, ureg_writemask(t_eb_info, TGSI_WRITEMASK_XYZ),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_W)),
            eb[1], eb[0]);

   /* r600g is ignoring TGSI_INTERPOLATE_CONSTANT, just workaround this */
   ureg_SGE(shader, ureg_writemask(t_eb_info, TGSI_WRITEMASK_XYZ), ureg_src(t_eb_info), ureg_imm1f(shader, 0.5f));

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
fetch_ref(struct ureg_program *shader, struct ureg_dst field)
{
   struct ureg_src ref_frames, bkwd_pred;
   struct ureg_src tc[4], sampler[2];
   struct ureg_dst ref[2], t_tc, result;
   unsigned i, intra_label, bi_label, label;

   ref_frames = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_REF_FRAMES, TGSI_INTERPOLATE_CONSTANT);
   bkwd_pred = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_BKWD_PRED, TGSI_INTERPOLATE_CONSTANT);

   for (i = 0; i < 4; ++i)
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV0 + i, TGSI_INTERPOLATE_LINEAR);

   for (i = 0; i < 2; ++i) {
      sampler[i] = ureg_DECL_sampler(shader, i + 3);
      ref[i] = ureg_DECL_temporary(shader);
   }

   result = ureg_DECL_temporary(shader);

   ureg_SEQ(shader, ureg_writemask(result, TGSI_WRITEMASK_X), ref_frames, ureg_imm1f(shader, -1.0f));
   ureg_IF(shader, ureg_scalar(ureg_src(result), TGSI_SWIZZLE_X), &intra_label);
      ureg_MOV(shader, result, ureg_imm1f(shader, 0.5f));

   ureg_ELSE(shader, &intra_label);
   ureg_IF(shader, ureg_scalar(ref_frames, TGSI_SWIZZLE_X), &bi_label);

      t_tc = ureg_DECL_temporary(shader);
      /*
       * result = tex(field.z ? tc[1] : tc[0], sampler[0])
       */
      ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XY),
               ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z)),
               tc[1], tc[0]);

      ureg_IF(shader, bkwd_pred, &label);
         ureg_TEX(shader, result, TGSI_TEXTURE_2D, ureg_src(t_tc), sampler[1]);
      ureg_ELSE(shader, &label);
         ureg_TEX(shader, result, TGSI_TEXTURE_2D, ureg_src(t_tc), sampler[0]);
      ureg_ENDIF(shader);

      ureg_release_temporary(shader, t_tc);

   ureg_ELSE(shader, &bi_label);

      t_tc = ureg_DECL_temporary(shader);

      /*
       * if (field.z)
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

      ureg_LRP(shader, result, ureg_scalar(ureg_imm1f(shader, 0.5f), TGSI_SWIZZLE_X), ureg_src(ref[0]), ureg_src(ref[1]));

   ureg_ENDIF(shader);
   ureg_ENDIF(shader);

   for (i = 0; i < 2; ++i)
      ureg_release_temporary(shader, ref[i]);

   return result;
}

static void *
create_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_dst result;
   struct ureg_dst field, texel;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   field = calc_field(shader);
   texel = fetch_ycbcr(r, shader, field);

   result = fetch_ref(shader, field);

   ureg_ADD(shader, fragment, ureg_src(texel), ureg_src(result));

   ureg_release_temporary(shader, field);
   ureg_release_temporary(shader, texel);
   ureg_release_temporary(shader, result);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static bool
init_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_sampler_state sampler;
   struct pipe_rasterizer_state rs_state;
   unsigned filters[5];
   unsigned i;

   assert(r);

   r->viewport.scale[0] = r->buffer_width;
   r->viewport.scale[1] = r->buffer_height;
   r->viewport.scale[2] = 1;
   r->viewport.scale[3] = 1;
   r->viewport.translate[0] = 0;
   r->viewport.translate[1] = 0;
   r->viewport.translate[2] = 0;
   r->viewport.translate[3] = 0;

   r->fb_state.width = r->buffer_width;
   r->fb_state.height = r->buffer_height;
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
   struct pipe_resource *idct_matrix;
   struct pipe_resource template;
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];
   struct pipe_sampler_view sampler_view;

   const unsigned mbw =
      align(r->buffer_width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;
   const unsigned mbh =
      align(r->buffer_height, MACROBLOCK_HEIGHT) / MACROBLOCK_HEIGHT;

   unsigned i, stride;

   assert(r);

   r->macroblocks_per_batch =
      mbw * (r->bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE ? mbh : 1);
   r->num_macroblocks = 0;

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   /* TODO: Accomodate HW that can't do this and also for cases when this isn't precise enough */
   template.format = PIPE_FORMAT_R16_SNORM;
   template.last_level = 0;
   template.width0 = r->buffer_width;
   template.height0 = r->buffer_height;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_DYNAMIC;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   r->textures.individual.y = r->pipe->screen->resource_create(r->pipe->screen, &template);

   if (!(idct_matrix = vl_idct_upload_matrix(r->pipe)))
      return false;

   if (!vl_idct_init(&r->idct_luma, r->pipe, r->buffer_width, r->buffer_height, idct_matrix))
      return false;

   if (!vl_idct_init_buffer(&r->idct_luma, &r->idct_y, r->textures.individual.y))
      return false;

   vl_idct_map_buffers(&r->idct_luma, &r->idct_y);

   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      template.width0 = r->buffer_width / 2;
      template.height0 = r->buffer_height / 2;
   }
   else if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422)
      template.height0 = r->buffer_height / 2;

   r->textures.individual.cb =
      r->pipe->screen->resource_create(r->pipe->screen, &template);
   r->textures.individual.cr =
      r->pipe->screen->resource_create(r->pipe->screen, &template);

   if(!vl_idct_init(&r->idct_chroma, r->pipe, template.width0, template.height0, idct_matrix))
      return false;

   if (!vl_idct_init_buffer(&r->idct_chroma, &r->idct_cb, r->textures.individual.cb))
      return false;

   vl_idct_map_buffers(&r->idct_chroma, &r->idct_cb);

   if (!vl_idct_init_buffer(&r->idct_chroma, &r->idct_cr, r->textures.individual.cr))
      return false;

   vl_idct_map_buffers(&r->idct_chroma, &r->idct_cr);

   for (i = 0; i < 3; ++i) {
      u_sampler_view_default_template(&sampler_view,
                                      r->textures.all[i],
                                      r->textures.all[i]->format);
      r->sampler_views.all[i] = r->pipe->create_sampler_view(r->pipe, r->textures.all[i], &sampler_view);
   }

   memset(&vertex_elems, 0, sizeof(vertex_elems));

   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();
   r->vertex_bufs.individual.quad = vl_vb_upload_quads(r->pipe, r->macroblocks_per_batch);

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* y, cr, cb empty block element top left block */
   vertex_elems[VS_I_EB_0_0].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element top right block */
   vertex_elems[VS_I_EB_0_1].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element bottom left block */
   vertex_elems[VS_I_EB_1_0].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* y, cr, cb empty block element bottom right block */
   vertex_elems[VS_I_EB_1_1].src_format = PIPE_FORMAT_R32G32B32_FLOAT;

   /* progressive=0.0f interlaced=1.0f */
   vertex_elems[VS_I_INTERLACED].src_format = PIPE_FORMAT_R32_FLOAT;

   /* frame=0.0f field=1.0f */
   vertex_elems[VS_I_FRAME_PRED].src_format = PIPE_FORMAT_R32_FLOAT;

   /* intra=-1.0f forward/backward=1.0f bi=0.0f */
   vertex_elems[VS_I_REF_FRAMES].src_format = PIPE_FORMAT_R32_FLOAT;

   /* forward=0.0f backward=1.0f */
   vertex_elems[VS_I_BKWD_PRED].src_format = PIPE_FORMAT_R32_FLOAT;

   stride = vl_vb_element_helper(&vertex_elems[VS_I_VPOS], 9, 1);

   r->vertex_bufs.individual.pos = vl_vb_init(
      &r->pos, r->pipe, r->macroblocks_per_batch, 
      sizeof(struct vertex_stream_0) / sizeof(float),
      stride);

   for (i = 0; i < 4; ++i) {
      /* motion vector 0..4 element */
      vertex_elems[VS_I_MV0 + i].src_format = PIPE_FORMAT_R32G32_FLOAT;
      stride = vl_vb_element_helper(&vertex_elems[VS_I_MV0 + i], 1, i + 2);
      r->vertex_bufs.individual.mv[i] = vl_vb_init(
         &r->mv[i], r->pipe, r->macroblocks_per_batch,
         sizeof(struct vertex2f) / sizeof(float),
         stride);
   }

   r->vertex_elems_state = r->pipe->create_vertex_elements_state(
      r->pipe, NUM_VS_INPUTS, vertex_elems);

   if (r->vertex_elems_state == NULL)
      return false;

   r->vs = create_vert_shader(r);
   r->fs = create_frag_shader(r);

   if (r->vs == NULL || r->fs == NULL)
      return false;

   return true;
}

static void
cleanup_buffers(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 3; ++i) {
      pipe_sampler_view_reference(&r->sampler_views.all[i], NULL);
      pipe_resource_reference(&r->vertex_bufs.all[i].buffer, NULL);
      pipe_resource_reference(&r->textures.all[i], NULL);
   }

   r->pipe->delete_vs_state(r->pipe, r->vs);
   r->pipe->delete_fs_state(r->pipe, r->fs);

   vl_vb_cleanup(&r->pos);

   for (i = 0; i < 4; ++i)
      vl_vb_cleanup(&r->mv[i]);

   vl_idct_unmap_buffers(&r->idct_luma, &r->idct_y);
   vl_idct_unmap_buffers(&r->idct_chroma, &r->idct_cb);
   vl_idct_unmap_buffers(&r->idct_chroma, &r->idct_cr);

   vl_idct_cleanup_buffer(&r->idct_luma, &r->idct_y);
   vl_idct_cleanup_buffer(&r->idct_chroma, &r->idct_cb);
   vl_idct_cleanup_buffer(&r->idct_chroma, &r->idct_cr);

   vl_idct_cleanup(&r->idct_luma);
   vl_idct_cleanup(&r->idct_chroma);

   r->pipe->delete_vertex_elements_state(r->pipe, r->vertex_elems_state);
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

static void
get_motion_vectors(struct pipe_mpeg12_macroblock *mb, struct vertex2f mv[4])
{
   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
      {
         if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {

            mv[2].x = mb->pmv[0][1][0];
            mv[2].y = mb->pmv[0][1][1];

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
   struct vertex2f mv[4];
   struct vertex_stream_0 info;

   unsigned i, j;

   assert(r);
   assert(mb);

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
   info.frame_pred = mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ? 1.0f : 0.0f;
   info.bkwd_pred = mb->mb_type == PIPE_MPEG12_MACROBLOCK_TYPE_BKWD ? 1.0f : 0.0f;
   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
         info.ref_frames = -1.0f;
         break;

      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
         info.ref_frames = 1.0f;
         break;
        
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
         info.ref_frames = 0.0f;
         break;

      default:
         assert(0);
   }

   vl_vb_add_block(&r->pos, (float*)&info);

   get_motion_vectors(mb, mv);
   for ( j = 0; j < 4; ++j )
      vl_vb_add_block(&r->mv[j], (float*)&mv[j]);
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
                           unsigned buffer_width,
                           unsigned buffer_height,
                           enum pipe_video_chroma_format chroma_format,
                           enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode)
{
   assert(renderer);
   assert(pipe);

   /* TODO: Implement other policies */
   assert(bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE);

   memset(renderer, 0, sizeof(struct vl_mpeg12_mc_renderer));

   renderer->pipe = pipe;
   renderer->buffer_width = buffer_width;
   renderer->buffer_height = buffer_height;
   renderer->chroma_format = chroma_format;
   renderer->bufmode = bufmode;

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

   return true;

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

   util_delete_keymap(renderer->texview_map, renderer->pipe);
   cleanup_pipe_state(renderer);
   cleanup_buffers(renderer);

   pipe_surface_reference(&renderer->surface, NULL);
   pipe_surface_reference(&renderer->past, NULL);
   pipe_surface_reference(&renderer->future, NULL);
}

void
vl_mpeg12_mc_map_buffer(struct vl_mpeg12_mc_renderer *renderer)
{
   unsigned i;

   assert(renderer);

   vl_idct_map_buffers(&renderer->idct_luma, &renderer->idct_y);
   vl_idct_map_buffers(&renderer->idct_chroma, &renderer->idct_cr);
   vl_idct_map_buffers(&renderer->idct_chroma, &renderer->idct_cb);

   vl_vb_map(&renderer->pos, renderer->pipe);
   for(i = 0; i < 4; ++i)
      vl_vb_map(&renderer->mv[i], renderer->pipe);
}

void
vl_mpeg12_mc_renderer_render_macroblocks(struct vl_mpeg12_mc_renderer *renderer,
                                         struct pipe_surface *surface,
                                         struct pipe_surface *past,
                                         struct pipe_surface *future,
                                         unsigned num_macroblocks,
                                         struct pipe_mpeg12_macroblock
                                         *mpeg12_macroblocks,
                                         struct pipe_fence_handle **fence)
{
   assert(renderer);
   assert(surface);
   assert(num_macroblocks);
   assert(mpeg12_macroblocks);

   if (surface != renderer->surface) {
      pipe_surface_reference(&renderer->surface, surface);
      pipe_surface_reference(&renderer->past, past);
      pipe_surface_reference(&renderer->future, future);
      renderer->fence = fence;
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
         vl_mpeg12_mc_unmap_buffer(renderer);
         vl_mpeg12_mc_renderer_flush(renderer);
         vl_mpeg12_mc_map_buffer(renderer);
      }
   }
}

void
vl_mpeg12_mc_unmap_buffer(struct vl_mpeg12_mc_renderer *renderer)
{
   unsigned i;

   assert(renderer);

   vl_idct_unmap_buffers(&renderer->idct_luma, &renderer->idct_y);
   vl_idct_unmap_buffers(&renderer->idct_chroma, &renderer->idct_cr);
   vl_idct_unmap_buffers(&renderer->idct_chroma, &renderer->idct_cb);

   vl_vb_unmap(&renderer->pos, renderer->pipe);
   for(i = 0; i < 4; ++i)
      vl_vb_unmap(&renderer->mv[i], renderer->pipe);
}

void
vl_mpeg12_mc_renderer_flush(struct vl_mpeg12_mc_renderer *renderer)
{
   unsigned i;

   assert(renderer);
   assert(renderer->num_macroblocks <= renderer->macroblocks_per_batch);

   if (renderer->num_macroblocks == 0)
      return;

   vl_idct_flush(&renderer->idct_luma, &renderer->idct_y);
   vl_idct_flush(&renderer->idct_chroma, &renderer->idct_cr);
   vl_idct_flush(&renderer->idct_chroma, &renderer->idct_cb);

   vl_vb_restart(&renderer->pos);
   for(i = 0; i < 4; ++i)
      vl_vb_restart(&renderer->mv[i]);

   renderer->fb_state.cbufs[0] = renderer->surface;
   renderer->pipe->bind_rasterizer_state(renderer->pipe, renderer->rs_state);
   renderer->pipe->set_framebuffer_state(renderer->pipe, &renderer->fb_state);
   renderer->pipe->set_viewport_state(renderer->pipe, &renderer->viewport);
   renderer->pipe->set_vertex_buffers(renderer->pipe, 6, renderer->vertex_bufs.all);
   renderer->pipe->bind_vertex_elements_state(renderer->pipe, renderer->vertex_elems_state);

   if (renderer->past) {
      renderer->textures.individual.ref[0] = renderer->past->texture;
      renderer->sampler_views.individual.ref[0] = find_or_create_sampler_view(renderer, renderer->past);
   }

   if (renderer->future) {
      renderer->textures.individual.ref[1] = renderer->future->texture;
      renderer->sampler_views.individual.ref[1] = find_or_create_sampler_view(renderer, renderer->future);
   }
   renderer->pipe->set_fragment_sampler_views(renderer->pipe, 5, renderer->sampler_views.all);
   renderer->pipe->bind_fragment_sampler_states(renderer->pipe, 5, renderer->samplers.all);

   renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs);
   util_draw_arrays(renderer->pipe, PIPE_PRIM_QUADS, 0, renderer->num_macroblocks * 4);

   renderer->pipe->flush(renderer->pipe, PIPE_FLUSH_RENDER_CACHE, renderer->fence);

   renderer->num_macroblocks = 0;
}
