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
#define ZERO_BLOCK_NIL -1.0f
#define ZERO_BLOCK_IS_NIL(zb) ((zb).x < 0.0f)
#define SCALE_FACTOR_16_TO_9 (32767.0f / 255.0f)

struct vertex_shader_consts
{
   struct vertex4f norm;
};

struct fragment_shader_consts
{
   struct vertex4f multiplier;
   struct vertex4f div;
};

struct vert_stream_0
{
   struct vertex2f pos;
   struct vertex2f luma_tc;
   struct vertex2f cb_tc;
   struct vertex2f cr_tc;
};

enum MACROBLOCK_TYPE
{
   MACROBLOCK_TYPE_INTRA,
   MACROBLOCK_TYPE_FWD_FRAME_PRED,
   MACROBLOCK_TYPE_FWD_FIELD_PRED,
   MACROBLOCK_TYPE_BKWD_FRAME_PRED,
   MACROBLOCK_TYPE_BKWD_FIELD_PRED,
   MACROBLOCK_TYPE_BI_FRAME_PRED,
   MACROBLOCK_TYPE_BI_FIELD_PRED,

   NUM_MACROBLOCK_TYPES
};

static void *
create_vert_shader(struct vl_mpeg12_mc_renderer *r, unsigned ref_frames, unsigned mv_per_frame)
{
   struct ureg_program *shader;
   struct ureg_src norm, mbs;
   struct ureg_src vpos, vtex[3], vmv[4];
   struct ureg_dst t_vpos, scale;
   struct ureg_dst o_vpos, o_vtex[3], o_vmv[4], o_line;
   unsigned i, j, count;
   bool interlaced = mv_per_frame == 2;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   norm = ureg_DECL_constant(shader, 0);
   mbs = ureg_imm2f(shader, MACROBLOCK_WIDTH, MACROBLOCK_HEIGHT);

   t_vpos = ureg_DECL_temporary(shader);
   scale = ureg_DECL_temporary(shader);

   vpos = ureg_DECL_vs_input(shader, 0);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, 0);

   for (i = 0; i < 3; ++i) {
      vtex[i] = ureg_DECL_vs_input(shader, 1 + i);
      o_vtex[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, 1 + i);
   }
   
   count=0;
   for (i = 0; i < ref_frames; ++i) {
      for (j = 0; j < 2; ++j) {        
        if(j < mv_per_frame) {
           vmv[count] = ureg_DECL_vs_input(shader, 4 + i * 2 + j);
           o_vmv[count] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, 4 + count);
           count++;
        }
        /* workaround for r600g */
        else if(ref_frames == 2)
           ureg_DECL_vs_input(shader, 4 + i * 2 + j);
      }
   }

   o_line = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, 4 + count);

   /*
    * scale = norm * mbs;
    *
    * t_vpos = vpos * scale
    * o_vpos = t_vpos
    *
    * o_vtex[0..2] = vtex[0..2] * scale
    *
    * if(count > 0) { // Apply motion vectors
    *    scale = norm * 0.5;
    *    o_vmv[0..count] = t_vpos + vmv[0..4] * scale
    * }
    *
    * if(interlaced) {
    *    o_line.x = 1
    *    o_line.y = vpos.y * 8
    * }
    */
   ureg_MUL(shader, ureg_writemask(scale, TGSI_WRITEMASK_XY), norm, mbs);

   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, ureg_src(scale));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   for (i = 0; i < 3; ++i) {
      ureg_MUL(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_XY), vpos, ureg_src(scale));
      ureg_MOV(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_Z), ureg_scalar(vtex[i], TGSI_SWIZZLE_X));
   }

   if(count > 0) {
      ureg_MUL(shader, ureg_writemask(scale, TGSI_WRITEMASK_XY), norm, ureg_imm1f(shader, 0.5f));
      for (i = 0; i < count; ++i)
         ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), ureg_src(scale), vmv[i], ureg_src(t_vpos));
   }

   if (interlaced) {
      ureg_MOV(shader, ureg_writemask(o_line, TGSI_WRITEMASK_X), ureg_imm1f(shader, 1.0f));
      ureg_MUL(shader, ureg_writemask(o_line, TGSI_WRITEMASK_Y), vpos, ureg_imm1f(shader, MACROBLOCK_HEIGHT / 2));
   }

   ureg_release_temporary(shader, t_vpos);
   ureg_release_temporary(shader, scale);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static struct ureg_dst
fetch_ycbcr(struct ureg_program *shader)
{
   struct ureg_src tc[3];
   struct ureg_src sampler[3];
   struct ureg_dst texel, tmp;
   unsigned i;

   texel = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);

   for (i = 0; i < 3; ++i)  {
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, i + 1, TGSI_INTERPOLATE_LINEAR);
      sampler[i] = ureg_DECL_sampler(shader, i);
   }

   /*
    * texel.y  = tex(tc[0], sampler[0])
    * texel.cb = tex(tc[1], sampler[1])
    * texel.cr = tex(tc[2], sampler[2])
    */
   for (i = 0; i < 3; ++i) {
      /* Nouveau can't writemask tex dst regs (yet?), do in two steps */
      ureg_TEX(shader, tmp, TGSI_TEXTURE_3D, tc[i], sampler[i]);
      ureg_MOV(shader, ureg_writemask(texel, TGSI_WRITEMASK_X << i), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));
   }

   ureg_release_temporary(shader, tmp);

   return texel;
}

static void *
create_intra_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_dst texel;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = fetch_ycbcr()
    * fragment = texel * scale
    */
   texel = fetch_ycbcr(shader);
   ureg_MUL(shader, fragment, ureg_src(texel), ureg_scalar(ureg_imm1f(shader, SCALE_FACTOR_16_TO_9), TGSI_SWIZZLE_X));

   ureg_release_temporary(shader, texel);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_frame_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src tc;
   struct ureg_src sampler;
   struct ureg_dst texel, ref;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 4, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 3);

   ref = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = fetch_ycbcr()
    * ref = tex(tc, sampler)
    * fragment = texel * scale + ref
    */
   texel = fetch_ycbcr(shader);
   ureg_TEX(shader, ref, TGSI_TEXTURE_2D, tc, sampler);
   ureg_MAD(shader, fragment, ureg_src(texel), ureg_scalar(ureg_imm1f(shader, SCALE_FACTOR_16_TO_9), TGSI_SWIZZLE_X), ureg_src(ref));

   ureg_release_temporary(shader, texel);
   ureg_release_temporary(shader, ref);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_field_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src tc[2], line;
   struct ureg_src sampler;
   struct ureg_dst texel, ref, tmp;
   struct ureg_dst fragment;
   unsigned i, label;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   for (i = 0; i < 2; ++i)
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, i + 4, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 3);

   line = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 6, TGSI_INTERPOLATE_LINEAR);

   ref = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = fetch_ycbcr()
    * if(line % 2)
    *    ref = tex(tc[1], sampler)
    * else
    *    ref = tex(tc[0], sampler)
    * fragment = texel * scale + ref
    */
   texel = fetch_ycbcr(shader);

   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), line);
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(tmp), ureg_imm1f(shader, 0.5f));
   ureg_IF(shader, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y), &label);
      ureg_TEX(shader, ref, TGSI_TEXTURE_2D, tc[1], sampler);
   ureg_ELSE(shader, &label);
      ureg_TEX(shader, ref, TGSI_TEXTURE_2D, tc[0], sampler);
   ureg_ENDIF(shader);

   ureg_MAD(shader, fragment, ureg_src(texel), ureg_scalar(ureg_imm1f(shader, SCALE_FACTOR_16_TO_9), TGSI_SWIZZLE_X), ureg_src(ref));

   ureg_release_temporary(shader, tmp);
   ureg_release_temporary(shader, texel);
   ureg_release_temporary(shader, ref);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_frame_bi_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src tc[2];
   struct ureg_src sampler[2];
   struct ureg_dst texel, ref[2];
   struct ureg_dst fragment;
   unsigned i;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   for (i = 0; i < 2; ++i)  {
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, i + 4, TGSI_INTERPOLATE_LINEAR);
      sampler[i] = ureg_DECL_sampler(shader, i + 3);
   }

   ref[0] = ureg_DECL_temporary(shader);
   ref[1] = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = fetch_ycbcr()
    * ref[0..1 = tex(tc[3..4], sampler[3..4])
    * ref[0] = lerp(ref[0], ref[1], 0.5)
    * fragment = texel * scale + ref[0]
    */
   texel = fetch_ycbcr(shader);
   ureg_TEX(shader, ref[0], TGSI_TEXTURE_2D, tc[0], sampler[0]);
   ureg_TEX(shader, ref[1], TGSI_TEXTURE_2D, tc[1], sampler[1]);
   ureg_LRP(shader, ref[0], ureg_scalar(ureg_imm1f(shader, 0.5f), TGSI_SWIZZLE_X), ureg_src(ref[0]), ureg_src(ref[1]));

   ureg_MAD(shader, fragment, ureg_src(texel), ureg_scalar(ureg_imm1f(shader, SCALE_FACTOR_16_TO_9), TGSI_SWIZZLE_X), ureg_src(ref[0]));

   ureg_release_temporary(shader, texel);
   ureg_release_temporary(shader, ref[0]);
   ureg_release_temporary(shader, ref[1]);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_field_bi_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src tc[4], line;
   struct ureg_src sampler[2];
   struct ureg_dst texel, ref[2], tmp;
   struct ureg_dst fragment;
   unsigned i, label;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   for (i = 0; i < 4; ++i)
      tc[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, i + 4, TGSI_INTERPOLATE_LINEAR);
   for (i = 0; i < 2; ++i)
      sampler[i] = ureg_DECL_sampler(shader, i + 3);

   line = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 8, TGSI_INTERPOLATE_LINEAR);

   texel = ureg_DECL_temporary(shader);
   ref[0] = ureg_DECL_temporary(shader);
   ref[1] = ureg_DECL_temporary(shader);
   tmp = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = fetch_ycbcr()
    * if(line % 2)
    *    ref[0..1] = tex(tc[4|6], sampler[3..4])
    * else
    *    ref[0..1] = tex(tc[3|5], sampler[3..4])
    * ref[0] = lerp(ref[0], ref[1], 0.5)
    * fragment = texel * scale + ref[0]
    */
   texel = fetch_ycbcr(shader);

   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), line);
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(tmp), ureg_imm1f(shader, 0.5f));
   ureg_IF(shader, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y), &label);
      ureg_TEX(shader, ref[0], TGSI_TEXTURE_2D, tc[1], sampler[0]);
      ureg_TEX(shader, ref[1], TGSI_TEXTURE_2D, tc[3], sampler[1]);
   ureg_ELSE(shader, &label);
      ureg_TEX(shader, ref[0], TGSI_TEXTURE_2D, tc[0], sampler[0]);
      ureg_TEX(shader, ref[1], TGSI_TEXTURE_2D, tc[2], sampler[1]);
   ureg_ENDIF(shader);

   ureg_LRP(shader, ref[0], ureg_scalar(ureg_imm1f(shader, 0.5f), TGSI_SWIZZLE_X), ureg_src(ref[0]), ureg_src(ref[1]));

   ureg_MAD(shader, fragment, ureg_src(texel), ureg_scalar(ureg_imm1f(shader, SCALE_FACTOR_16_TO_9), TGSI_SWIZZLE_X), ureg_src(ref[0]));

   ureg_release_temporary(shader, tmp);
   ureg_release_temporary(shader, texel);
   ureg_release_temporary(shader, ref[0]);
   ureg_release_temporary(shader, ref[1]);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void
xfer_buffers_map(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 3; ++i) {
      struct pipe_box rect =
      {
         0, 0, 0,
         r->textures.all[i]->width0,
         r->textures.all[i]->height0,
         1
      };

      r->tex_transfer[i] = r->pipe->get_transfer
      (
         r->pipe, r->textures.all[i],
         u_subresource(0, 0),
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &rect
      );

      r->texels[i] = r->pipe->transfer_map(r->pipe, r->tex_transfer[i]);
   }
}

static void
xfer_buffers_unmap(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 3; ++i) {
      r->pipe->transfer_unmap(r->pipe, r->tex_transfer[i]);
      r->pipe->transfer_destroy(r->pipe, r->tex_transfer[i]);
   }
}

static bool
init_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_sampler_state sampler;
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
   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_444 ||
       r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
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

   return true;
}

static void
cleanup_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 5; ++i)
      r->pipe->delete_sampler_state(r->pipe, r->samplers.all[i]);
}

static bool
init_shaders(struct vl_mpeg12_mc_renderer *r)
{
   assert(r);

   assert(r->i_vs = create_vert_shader(r, 0, 0));
   assert(r->i_fs = create_intra_frag_shader(r));
   
   assert(r->p_vs[0] = create_vert_shader(r, 1, 1));
   assert(r->p_vs[1] = create_vert_shader(r, 1, 2));
   assert(r->p_fs[0] = create_frame_pred_frag_shader(r));
   assert(r->p_fs[1] = create_field_pred_frag_shader(r));

   assert(r->b_vs[0] = create_vert_shader(r, 2, 1));
   assert(r->b_vs[1] = create_vert_shader(r, 2, 2));
   assert(r->b_fs[0] = create_frame_bi_pred_frag_shader(r));
   assert(r->b_fs[1] = create_field_bi_pred_frag_shader(r));

   return true;
}

static void
cleanup_shaders(struct vl_mpeg12_mc_renderer *r)
{
   assert(r);

   r->pipe->delete_vs_state(r->pipe, r->i_vs);
   r->pipe->delete_fs_state(r->pipe, r->i_fs);
   r->pipe->delete_vs_state(r->pipe, r->p_vs[0]);
   r->pipe->delete_vs_state(r->pipe, r->p_vs[1]);
   r->pipe->delete_fs_state(r->pipe, r->p_fs[0]);
   r->pipe->delete_fs_state(r->pipe, r->p_fs[1]);
   r->pipe->delete_vs_state(r->pipe, r->b_vs[0]);
   r->pipe->delete_vs_state(r->pipe, r->b_vs[1]);
   r->pipe->delete_fs_state(r->pipe, r->b_fs[0]);
   r->pipe->delete_fs_state(r->pipe, r->b_fs[1]);
}

static bool
init_buffers(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_resource template;
   struct pipe_vertex_element vertex_elems[8];
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
   r->macroblock_buf = MALLOC(r->macroblocks_per_batch * sizeof(struct pipe_mpeg12_macroblock));

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_3D;
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

   r->vertex_bufs.individual.ycbcr.stride = sizeof(struct vertex2f) * 4;
   r->vertex_bufs.individual.ycbcr.max_index = 24 * r->macroblocks_per_batch - 1;
   r->vertex_bufs.individual.ycbcr.buffer_offset = 0;
   /* XXX: Create with usage DYNAMIC or STREAM */
   r->vertex_bufs.individual.ycbcr.buffer = pipe_buffer_create
   (
      r->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex2f) * 4 * 24 * r->macroblocks_per_batch
   );

   for (i = 1; i < 3; ++i) {
      r->vertex_bufs.all[i].stride = sizeof(struct vertex2f) * 2;
      r->vertex_bufs.all[i].max_index = 24 * r->macroblocks_per_batch - 1;
      r->vertex_bufs.all[i].buffer_offset = 0;
      /* XXX: Create with usage DYNAMIC or STREAM */
      r->vertex_bufs.all[i].buffer = pipe_buffer_create
      (
         r->pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         sizeof(struct vertex2f) * 2 * 24 * r->macroblocks_per_batch
      );
   }

   memset(&vertex_elems, 0, sizeof(vertex_elems));

   /* Position element */
   vertex_elems[0].src_offset = 0;
   vertex_elems[0].instance_divisor = 0;
   vertex_elems[0].vertex_buffer_index = 0;
   vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Luma, texcoord element */
   vertex_elems[1].src_offset = sizeof(struct vertex2f);
   vertex_elems[1].instance_divisor = 0;
   vertex_elems[1].vertex_buffer_index = 0;
   vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Chroma Cr texcoord element */
   vertex_elems[2].src_offset = sizeof(struct vertex2f) * 2;
   vertex_elems[2].instance_divisor = 0;
   vertex_elems[2].vertex_buffer_index = 0;
   vertex_elems[2].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Chroma Cb texcoord element */
   vertex_elems[3].src_offset = sizeof(struct vertex2f) * 3;
   vertex_elems[3].instance_divisor = 0;
   vertex_elems[3].vertex_buffer_index = 0;
   vertex_elems[3].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* First ref surface top field texcoord element */
   vertex_elems[4].src_offset = 0;
   vertex_elems[4].instance_divisor = 0;
   vertex_elems[4].vertex_buffer_index = 1;
   vertex_elems[4].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* First ref surface bottom field texcoord element */
   vertex_elems[5].src_offset = sizeof(struct vertex2f);
   vertex_elems[5].instance_divisor = 0;
   vertex_elems[5].vertex_buffer_index = 1;
   vertex_elems[5].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface top field texcoord element */
   vertex_elems[6].src_offset = 0;
   vertex_elems[6].instance_divisor = 0;
   vertex_elems[6].vertex_buffer_index = 2;
   vertex_elems[6].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface bottom field texcoord element */
   vertex_elems[7].src_offset = sizeof(struct vertex2f);
   vertex_elems[7].instance_divisor = 0;
   vertex_elems[7].vertex_buffer_index = 2;
   vertex_elems[7].src_format = PIPE_FORMAT_R32G32_FLOAT;

   r->vertex_elems_state.individual.i = r->pipe->create_vertex_elements_state(r->pipe, 4, vertex_elems);
   r->vertex_elems_state.individual.p = r->pipe->create_vertex_elements_state(r->pipe, 6, vertex_elems);
   r->vertex_elems_state.individual.b = r->pipe->create_vertex_elements_state(r->pipe, 8, vertex_elems);

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
      r->pipe->delete_vertex_elements_state(r->pipe, r->vertex_elems_state.all[i]);
      pipe_resource_reference(&r->vertex_bufs.all[i].buffer, NULL);
      pipe_resource_reference(&r->textures.all[i], NULL);
   }

   FREE(r->macroblock_buf);
}

static enum MACROBLOCK_TYPE
get_macroblock_type(struct pipe_mpeg12_macroblock *mb)
{
   assert(mb);

   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
         return MACROBLOCK_TYPE_INTRA;
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_FWD_FRAME_PRED : MACROBLOCK_TYPE_FWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_BKWD_FRAME_PRED : MACROBLOCK_TYPE_BKWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_BI_FRAME_PRED : MACROBLOCK_TYPE_BI_FIELD_PRED;
      default:
         assert(0);
   }

   /* Unreachable */
   return -1;
}

static void
gen_block_verts(struct vert_stream_0 *vb, struct pipe_mpeg12_macroblock *mb,
                const struct vertex2f *offset,
                unsigned luma_mask, unsigned cb_mask, unsigned cr_mask,
                bool use_zeroblocks, struct vertex2f *zero_blocks)
{
   unsigned cbp = mb->cbp;
   unsigned mbx = mb->mbx;
   unsigned mby = mb->mby;

   const struct vertex2f half =
   {
      0.5f,
      0.5f
   };
   struct vertex2f v;

   assert(vb);
   assert(offset);
   assert(zero_blocks || !use_zeroblocks);

   /* Generate vertices for two triangles covering a block */
   v.x = mbx + offset->x;
   v.y = mby + offset->y;

   vb[0].pos.x = v.x;
   vb[0].pos.y = v.y;
   vb[1].pos.x = v.x;
   vb[1].pos.y = v.y + half.y;
   vb[2].pos.x = v.x + half.x;
   vb[2].pos.y = v.y;
   vb[3].pos.x = v.x + half.x;
   vb[3].pos.y = v.y;
   vb[4].pos.x = v.x;
   vb[4].pos.y = v.y + half.y;
   vb[5].pos.x = v.x + half.x;
   vb[5].pos.y = v.y + half.y;

   /* Generate texcoords for the triangles, either pointing to the correct area on the luma/chroma texture
      or if zero blocks are being used, to the zero block if the appropriate CBP bits aren't set (i.e. no data
      for this channel is defined for this block) */

   if (!use_zeroblocks || cbp & luma_mask || mb->dct_type == PIPE_MPEG12_DCT_TYPE_FIELD) {
      v.x = 0.0f;
      v.y = 0.0f;
   }
   else {
      v.x = -1.0f;
      v.y = 0.0f;
   }

   vb[0].luma_tc.x = v.x;
   vb[0].luma_tc.y = v.y;
   vb[1].luma_tc.x = v.x;
   vb[1].luma_tc.y = v.y;// + half.y;
   vb[2].luma_tc.x = v.x;// + half.x;
   vb[2].luma_tc.y = v.y;
   vb[3].luma_tc.x = v.x;// + half.x;
   vb[3].luma_tc.y = v.y;
   vb[4].luma_tc.x = v.x;
   vb[4].luma_tc.y = v.y;// + half.y;
   vb[5].luma_tc.x = v.x;// + half.x;
   vb[5].luma_tc.y = v.y;// + half.y;

   if (!use_zeroblocks || cbp & cb_mask) {
      v.x = 0.0f;
      v.y = 0.0f;
   }
   else {
      v.x = -1.0f;
      v.y = 0.0f;
   }

   vb[0].cb_tc.x = v.x;
   vb[0].cb_tc.y = v.y;
   vb[1].cb_tc.x = v.x;
   vb[1].cb_tc.y = v.y;// + half.y;
   vb[2].cb_tc.x = v.x;// + half.x;
   vb[2].cb_tc.y = v.y;
   vb[3].cb_tc.x = v.x;// + half.x;
   vb[3].cb_tc.y = v.y;
   vb[4].cb_tc.x = v.x;
   vb[4].cb_tc.y = v.y;// + half.y;
   vb[5].cb_tc.x = v.x;// + half.x;
   vb[5].cb_tc.y = v.y;// + half.y;

   if (!use_zeroblocks || cbp & cr_mask) {
      v.x = 0.0f;
      v.y = 0.0f;
   }
   else {
      v.x = -1.0f;
      v.y = 0.0f;
   }

   vb[0].cr_tc.x = v.x;
   vb[0].cr_tc.y = v.y;
   vb[1].cr_tc.x = v.x;
   vb[1].cr_tc.y = v.y; // + half.y;
   vb[2].cr_tc.x = v.x; // + half.x;
   vb[2].cr_tc.y = v.y;
   vb[3].cr_tc.x = v.x; // + half.x;
   vb[3].cr_tc.y = v.y;
   vb[4].cr_tc.x = v.x;
   vb[4].cr_tc.y = v.y; // + half.y;
   vb[5].cr_tc.x = v.x; // + half.x;
   vb[5].cr_tc.y = v.y; // + half.y;
}

static void
gen_macroblock_verts(struct vl_mpeg12_mc_renderer *r,
                     struct pipe_mpeg12_macroblock *mb, unsigned pos,
                     struct vert_stream_0 *ycbcr_vb, struct vertex2f **ref_vb)
{
   struct vertex2f mo_vec[2];

   unsigned i;

   assert(r);
   assert(mb);
   assert(ycbcr_vb);
   assert(pos < r->macroblocks_per_batch);

   mo_vec[1].x = 0;
   mo_vec[1].y = 0;

   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
      {
         struct vertex2f *vb;

         assert(ref_vb && ref_vb[1]);

         vb = ref_vb[1] + pos * 2 * 24;

         mo_vec[0].x = mb->pmv[0][1][0];
         mo_vec[0].y = mb->pmv[0][1][1];

         if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
            }
         }
         else {
            mo_vec[0].y = mb->pmv[0][1][1] - (mb->pmv[0][1][1] % 4);

            mo_vec[1].x = mb->pmv[1][1][0];
            mo_vec[1].y = mb->pmv[1][1][1] - (mb->pmv[1][1][1] % 4);

            if(mb->mvfs[0][1]) mo_vec[0].y += 2;
            if(!mb->mvfs[1][1]) mo_vec[1].y -= 2;

            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
               vb[i + 1].x = mo_vec[1].x;
               vb[i + 1].y = mo_vec[1].y;
            }
         }

         /* fall-through */
      }
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
      {
         struct vertex2f *vb;

         assert(ref_vb && ref_vb[0]);

         vb = ref_vb[0] + pos * 2 * 24;

         if (mb->mb_type == PIPE_MPEG12_MACROBLOCK_TYPE_BKWD) {
            mo_vec[0].x = mb->pmv[0][1][0];
            mo_vec[0].y = mb->pmv[0][1][1];

            if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FIELD) {
               mo_vec[0].y = mb->pmv[0][1][1] - (mb->pmv[0][1][1] % 4);

               mo_vec[1].x = mb->pmv[1][1][0];
               mo_vec[1].y = mb->pmv[1][1][1] - (mb->pmv[1][1][1] % 4);

               if(mb->mvfs[0][1]) mo_vec[0].y += 2;
               if(!mb->mvfs[1][1]) mo_vec[1].y -= 2;
            }
         }
         else {
            mo_vec[0].x = mb->pmv[0][0][0];
            mo_vec[0].y = mb->pmv[0][0][1];

            if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FIELD) {
               mo_vec[0].y = mb->pmv[0][0][1] - (mb->pmv[0][0][1] % 4);

               mo_vec[1].x = mb->pmv[1][0][0];
               mo_vec[1].y = mb->pmv[1][0][1] - (mb->pmv[1][0][1] % 4);

               if(mb->mvfs[0][0]) mo_vec[0].y += 2;
               if(!mb->mvfs[1][0]) mo_vec[1].y -= 2;
            }
         }

         if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
            }
         }
         else {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
               vb[i + 1].x = mo_vec[1].x;
               vb[i + 1].y = mo_vec[1].y;
            }
         }

         /* fall-through */
      }
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
      {
         const struct vertex2f offsets[2][2] =
         {
            {
               {0, 0}, {0, 0.5f}
            },
            {
               {0.5f, 0}, {0.5f, 0.5f}
            }
         };
         const bool use_zb = r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE;

         struct vert_stream_0 *vb = ycbcr_vb + pos * 24;

         gen_block_verts(vb, mb, &offsets[0][0],
                         32, 2, 1, use_zb, r->zero_block);

         gen_block_verts(vb + 6, mb, &offsets[1][0],
                         16, 2, 1, use_zb, r->zero_block);

         gen_block_verts(vb + 12, mb, &offsets[0][1],
                         8, 2, 1, use_zb, r->zero_block);

         gen_block_verts(vb + 18, mb, &offsets[1][1],
                         4, 2, 1, use_zb, r->zero_block);

         break;
      }
      default:
         assert(0);
   }
}

static void
gen_macroblock_stream(struct vl_mpeg12_mc_renderer *r,
                      unsigned *num_macroblocks)
{
   unsigned offset[NUM_MACROBLOCK_TYPES];
   struct vert_stream_0 *ycbcr_vb;
   struct vertex2f *ref_vb[2];
   struct pipe_transfer *buf_transfer[3];
   unsigned i;

   assert(r);
   assert(num_macroblocks);

   for (i = 0; i < r->num_macroblocks; ++i) {
      enum MACROBLOCK_TYPE mb_type = get_macroblock_type(&r->macroblock_buf[i]);
      ++num_macroblocks[mb_type];
   }

   offset[0] = 0;

   for (i = 1; i < NUM_MACROBLOCK_TYPES; ++i)
      offset[i] = offset[i - 1] + num_macroblocks[i - 1];

   ycbcr_vb = (struct vert_stream_0 *)pipe_buffer_map
   (
      r->pipe,
      r->vertex_bufs.individual.ycbcr.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer[0]
   );

   for (i = 0; i < 2; ++i)
      ref_vb[i] = (struct vertex2f *)pipe_buffer_map
      (
         r->pipe,
         r->vertex_bufs.individual.ref[i].buffer,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &buf_transfer[i + 1]
      );

   for (i = 0; i < r->num_macroblocks; ++i) {
      enum MACROBLOCK_TYPE mb_type = get_macroblock_type(&r->macroblock_buf[i]);

      gen_macroblock_verts(r, &r->macroblock_buf[i], offset[mb_type],
                           ycbcr_vb, ref_vb);

      ++offset[mb_type];
   }

   pipe_buffer_unmap(r->pipe, r->vertex_bufs.individual.ycbcr.buffer, buf_transfer[0]);
   for (i = 0; i < 2; ++i)
      pipe_buffer_unmap(r->pipe, r->vertex_bufs.individual.ref[i].buffer, buf_transfer[i + 1]);
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
flush(struct vl_mpeg12_mc_renderer *r)
{
   unsigned num_macroblocks[NUM_MACROBLOCK_TYPES] = { 0 };
   unsigned vb_start = 0;
   struct vertex_shader_consts *vs_consts;
   struct pipe_transfer *buf_transfer;
   unsigned i;

   assert(r);
   assert(r->num_macroblocks == r->macroblocks_per_batch);

   gen_macroblock_stream(r, num_macroblocks);

   r->fb_state.cbufs[0] = r->surface;

   r->pipe->set_framebuffer_state(r->pipe, &r->fb_state);
   r->pipe->set_viewport_state(r->pipe, &r->viewport);

   vs_consts = pipe_buffer_map
   (
      r->pipe, r->vs_const_buf,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer
   );

   vs_consts->norm.x = 1.0f / r->surface->width;
   vs_consts->norm.y = 1.0f / r->surface->height;

   pipe_buffer_unmap(r->pipe, r->vs_const_buf, buf_transfer);

   r->pipe->set_constant_buffer(r->pipe, PIPE_SHADER_VERTEX, 0,
                                r->vs_const_buf);

   if (num_macroblocks[MACROBLOCK_TYPE_INTRA] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 1, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.i);
      r->pipe->set_fragment_sampler_views(r->pipe, 3, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 3, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->i_vs);
      r->pipe->bind_fs_state(r->pipe, r->i_fs);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_INTRA] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_INTRA] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.p);
      r->textures.individual.ref[0] = r->past->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->past);
      r->pipe->set_fragment_sampler_views(r->pipe, 4, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[0]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.p);
      r->textures.individual.ref[0] = r->past->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->past);
      r->pipe->set_fragment_sampler_views(r->pipe, 4, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[1]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.p);
      r->textures.individual.ref[0] = r->future->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->future);
      r->pipe->set_fragment_sampler_views(r->pipe, 4, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[0]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.p);
      r->textures.individual.ref[0] = r->future->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->future);
      r->pipe->set_fragment_sampler_views(r->pipe, 4, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[1]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 3, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.b);
      r->textures.individual.ref[0] = r->past->texture;
      r->textures.individual.ref[1] = r->future->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->past);
      r->sampler_views.individual.ref[1] = find_or_create_sampler_view(r, r->future);
      r->pipe->set_fragment_sampler_views(r->pipe, 5, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 5, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->b_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->b_fs[0]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 3, r->vertex_bufs.all);
      r->pipe->bind_vertex_elements_state(r->pipe, r->vertex_elems_state.individual.b);
      r->textures.individual.ref[0] = r->past->texture;
      r->textures.individual.ref[1] = r->future->texture;
      r->sampler_views.individual.ref[0] = find_or_create_sampler_view(r, r->past);
      r->sampler_views.individual.ref[1] = find_or_create_sampler_view(r, r->future);
      r->pipe->set_fragment_sampler_views(r->pipe, 5, r->sampler_views.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 5, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->b_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->b_fs[1]);

      util_draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                       num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] * 24;
   }

   r->pipe->flush(r->pipe, PIPE_FLUSH_RENDER_CACHE, r->fence);

   if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE)
      for (i = 0; i < 3; ++i)
         r->zero_block[i].x = ZERO_BLOCK_NIL;

   r->num_macroblocks = 0;
}

static void
grab_frame_coded_block(short *src, short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(src);
   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memcpy(dst + y * dst_pitch, src + y * BLOCK_WIDTH, BLOCK_WIDTH * 2);
}

static void
grab_field_coded_block(short *src, short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(src);
   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memcpy(dst + y * dst_pitch * 2, src + y * BLOCK_WIDTH, BLOCK_WIDTH * 2);
}

static void
fill_frame_zero_block(short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memset(dst + y * dst_pitch, 0, BLOCK_WIDTH * 2);
}

static void
fill_field_zero_block(short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memset(dst + y * dst_pitch * 2, 0, BLOCK_WIDTH * 2);
}

static void
grab_blocks(struct vl_mpeg12_mc_renderer *r, unsigned mbx, unsigned mby,
            enum pipe_mpeg12_dct_type dct_type, unsigned cbp, short *blocks)
{
   unsigned tex_pitch;
   short *texels;
   unsigned tb = 0, sb = 0;
   unsigned mbpx = mbx * MACROBLOCK_WIDTH, mbpy = mby * MACROBLOCK_HEIGHT;
   unsigned x, y;

   assert(r);
   assert(blocks);

   tex_pitch = r->tex_transfer[0]->stride / util_format_get_blocksize(r->tex_transfer[0]->resource->format);
   texels = r->texels[0] + mbpy * tex_pitch + mbpx;

   for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, ++tb) {
         if ((cbp >> (5 - tb)) & 1) {
            if (dct_type == PIPE_MPEG12_DCT_TYPE_FRAME) {
               grab_frame_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT,
                                      texels + y * tex_pitch * BLOCK_HEIGHT +
                                      x * BLOCK_WIDTH, tex_pitch);
            }
            else {
               grab_field_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT,
                                      texels + y * tex_pitch + x * BLOCK_WIDTH,
                                      tex_pitch);
            }

            ++sb;
         }
         else if (r->eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE) {
            if(dct_type == PIPE_MPEG12_DCT_TYPE_FRAME) {

               if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ALL ||
                   ZERO_BLOCK_IS_NIL(r->zero_block[0])) {

                  fill_frame_zero_block(texels + y * tex_pitch * BLOCK_WIDTH + x * BLOCK_WIDTH, tex_pitch);
                  if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
                     r->zero_block[0].x = (mbx + x * 0.5f);
                     r->zero_block[0].y = (mby + y * 0.5f);
                  }
               }
            }
            else {

               fill_field_zero_block(texels + y * tex_pitch + x * BLOCK_WIDTH, tex_pitch);
            }
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   mbpx /= 2;
   mbpy /= 2;

   for (tb = 0; tb < 2; ++tb) {
      tex_pitch = r->tex_transfer[tb + 1]->stride / util_format_get_blocksize(r->tex_transfer[tb + 1]->resource->format);
      texels = r->texels[tb + 1] + mbpy * tex_pitch + mbpx;

      if ((cbp >> (1 - tb)) & 1) {
         grab_frame_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT, texels, tex_pitch);
         ++sb;
      }
      else if (r->eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE) {
         if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ALL ||
             ZERO_BLOCK_IS_NIL(r->zero_block[tb + 1])) {
            fill_frame_zero_block(texels, tex_pitch);
            if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
               r->zero_block[tb + 1].x = mbx;
               r->zero_block[tb + 1].y = mby;
            }
         }
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

   memcpy(&r->macroblock_buf[r->num_macroblocks], mb,
          sizeof(struct pipe_mpeg12_macroblock));

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
                           enum VL_MPEG12_MC_RENDERER_EMPTY_BLOCK eb_handling,
                           bool pot_buffers)
{
   unsigned i;

   assert(renderer);
   assert(pipe);
   /* TODO: Implement other policies */
   assert(bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE);
   /* TODO: Implement this */
   /* XXX: XFER_ALL sampling issue at block edges when using bilinear filtering */
   assert(eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE);
   /* TODO: Non-pot buffers untested, probably doesn't work without changes to texcoord generation, vert shader, etc */
   assert(pot_buffers);

   memset(renderer, 0, sizeof(struct vl_mpeg12_mc_renderer));

   renderer->pipe = pipe;
   renderer->picture_width = picture_width;
   renderer->picture_height = picture_height;
   renderer->chroma_format = chroma_format;
   renderer->bufmode = bufmode;
   renderer->eb_handling = eb_handling;
   renderer->pot_buffers = pot_buffers;

   renderer->texview_map = util_new_keymap(sizeof(struct pipe_surface*), -1,
                                           texview_map_delete);
   if (!renderer->texview_map)
      return false;

   if (!init_pipe_state(renderer)) {
      util_delete_keymap(renderer->texview_map, renderer->pipe);
      return false;
   }
   if (!init_shaders(renderer)) {
      util_delete_keymap(renderer->texview_map, renderer->pipe);
      cleanup_pipe_state(renderer);
      return false;
   }
   if (!init_buffers(renderer)) {
      util_delete_keymap(renderer->texview_map, renderer->pipe);
      cleanup_shaders(renderer);
      cleanup_pipe_state(renderer);
      return false;
   }

   renderer->surface = NULL;
   renderer->past = NULL;
   renderer->future = NULL;
   for (i = 0; i < 3; ++i)
      renderer->zero_block[i].x = ZERO_BLOCK_NIL;
   renderer->num_macroblocks = 0;

   xfer_buffers_map(renderer);

   return true;
}

void
vl_mpeg12_mc_renderer_cleanup(struct vl_mpeg12_mc_renderer *renderer)
{
   assert(renderer);

   xfer_buffers_unmap(renderer);

   util_delete_keymap(renderer->texview_map, renderer->pipe);
   cleanup_pipe_state(renderer);
   cleanup_shaders(renderer);
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
            xfer_buffers_unmap(renderer);
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
         xfer_buffers_unmap(renderer);
         flush(renderer);
         xfer_buffers_map(renderer);
         /* Next time we get this surface it may have new ref frames */
         pipe_surface_reference(&renderer->surface, NULL);
         pipe_surface_reference(&renderer->past, NULL);
         pipe_surface_reference(&renderer->future, NULL);
      }
   }
}
