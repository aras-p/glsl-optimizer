/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_util.h"
#include "toy_compiler.h"
#include "toy_tgsi.h"
#include "toy_legalize.h"
#include "toy_optimize.h"
#include "toy_helpers.h"
#include "ilo_shader_internal.h"

struct fs_compile_context {
   struct ilo_shader *shader;
   const struct ilo_shader_variant *variant;

   struct toy_compiler tc;
   struct toy_tgsi tgsi;

   int const_cache;
   int dispatch_mode;

   struct {
      int interp_perspective_pixel;
      int interp_perspective_centroid;
      int interp_perspective_sample;
      int interp_nonperspective_pixel;
      int interp_nonperspective_centroid;
      int interp_nonperspective_sample;
      int source_depth;
      int source_w;
      int pos_offset;
   } payloads[2];

   int first_const_grf;
   int first_attr_grf;
   int first_free_grf;
   int last_free_grf;

   int num_grf_per_vrf;

   int first_free_mrf;
   int last_free_mrf;
};

static void
fetch_position(struct fs_compile_context *fcc, struct toy_dst dst)
{
   struct toy_compiler *tc = &fcc->tc;
   const struct toy_src src_z =
      tsrc(TOY_FILE_GRF, fcc->payloads[0].source_depth, 0);
   const struct toy_src src_w =
      tsrc(TOY_FILE_GRF, fcc->payloads[0].source_w, 0);
   const int fb_height =
      (fcc->variant->u.fs.fb_height) ? fcc->variant->u.fs.fb_height : 1;
   const bool origin_upper_left =
      (fcc->tgsi.props.fs_coord_origin == TGSI_FS_COORD_ORIGIN_UPPER_LEFT);
   const bool pixel_center_integer =
      (fcc->tgsi.props.fs_coord_pixel_center ==
       TGSI_FS_COORD_PIXEL_CENTER_INTEGER);
   struct toy_src subspan_x, subspan_y;
   struct toy_dst tmp, tmp_uw;
   struct toy_dst real_dst[4];

   tdst_transpose(dst, real_dst);

   subspan_x = tsrc_uw(tsrc(TOY_FILE_GRF, 1, 2 * 4));
   subspan_x = tsrc_rect(subspan_x, TOY_RECT_240);

   subspan_y = tsrc_offset(subspan_x, 0, 1);

   tmp_uw = tdst_uw(tc_alloc_tmp(tc));
   tmp = tc_alloc_tmp(tc);

   /* X */
   tc_ADD(tc, tmp_uw, subspan_x, tsrc_imm_v(0x10101010));
   tc_MOV(tc, tmp, tsrc_from(tmp_uw));
   if (pixel_center_integer)
      tc_MOV(tc, real_dst[0], tsrc_from(tmp));
   else
      tc_ADD(tc, real_dst[0], tsrc_from(tmp), tsrc_imm_f(0.5f));

   /* Y */
   tc_ADD(tc, tmp_uw, subspan_y, tsrc_imm_v(0x11001100));
   tc_MOV(tc, tmp, tsrc_from(tmp_uw));
   if (origin_upper_left && pixel_center_integer) {
      tc_MOV(tc, real_dst[1], tsrc_from(tmp));
   }
   else {
      struct toy_src y = tsrc_from(tmp);
      float offset = 0.0f;

      if (!pixel_center_integer)
         offset += 0.5f;

      if (!origin_upper_left) {
         offset += (float) (fb_height - 1);
         y = tsrc_negate(y);
      }

      tc_ADD(tc, real_dst[1], y, tsrc_imm_f(offset));
   }

   /* Z and W */
   tc_MOV(tc, real_dst[2], src_z);
   tc_INV(tc, real_dst[3], src_w);
}

static void
fetch_face(struct fs_compile_context *fcc, struct toy_dst dst)
{
   struct toy_compiler *tc = &fcc->tc;
   const struct toy_src r0 = tsrc_d(tsrc(TOY_FILE_GRF, 0, 0));
   struct toy_dst tmp_f, tmp;
   struct toy_dst real_dst[4];

   tdst_transpose(dst, real_dst);

   tmp_f = tc_alloc_tmp(tc);
   tmp = tdst_d(tmp_f);
   tc_SHR(tc, tmp, tsrc_rect(r0, TOY_RECT_010), tsrc_imm_d(15));
   tc_AND(tc, tmp, tsrc_from(tmp), tsrc_imm_d(1));
   tc_MOV(tc, tmp_f, tsrc_from(tmp));

   /* convert to 1.0 and -1.0 */
   tc_MUL(tc, tmp_f, tsrc_from(tmp_f), tsrc_imm_f(-2.0f));
   tc_ADD(tc, real_dst[0], tsrc_from(tmp_f), tsrc_imm_f(1.0f));

   tc_MOV(tc, real_dst[1], tsrc_imm_f(0.0f));
   tc_MOV(tc, real_dst[2], tsrc_imm_f(0.0f));
   tc_MOV(tc, real_dst[3], tsrc_imm_f(1.0f));
}

static void
fetch_attr(struct fs_compile_context *fcc, struct toy_dst dst, int slot)
{
   struct toy_compiler *tc = &fcc->tc;
   struct toy_dst real_dst[4];
   bool is_const = false;
   int grf, interp, ch;

   tdst_transpose(dst, real_dst);

   grf = fcc->first_attr_grf + slot * 2;

   switch (fcc->tgsi.inputs[slot].interp) {
   case TGSI_INTERPOLATE_CONSTANT:
      is_const = true;
      break;
   case TGSI_INTERPOLATE_LINEAR:
      if (fcc->tgsi.inputs[slot].centroid)
         interp = fcc->payloads[0].interp_nonperspective_centroid;
      else
         interp = fcc->payloads[0].interp_nonperspective_pixel;
      break;
   case TGSI_INTERPOLATE_COLOR:
      if (fcc->variant->u.fs.flatshade) {
         is_const = true;
         break;
      }
      /* fall through */
   case TGSI_INTERPOLATE_PERSPECTIVE:
      if (fcc->tgsi.inputs[slot].centroid)
         interp = fcc->payloads[0].interp_perspective_centroid;
      else
         interp = fcc->payloads[0].interp_perspective_pixel;
      break;
   default:
      assert(!"unexpected FS interpolation");
      interp = fcc->payloads[0].interp_perspective_pixel;
      break;
   }

   if (is_const) {
      struct toy_src a0[4];

      a0[0] = tsrc(TOY_FILE_GRF, grf + 0, 3 * 4);
      a0[1] = tsrc(TOY_FILE_GRF, grf + 0, 7 * 4);
      a0[2] = tsrc(TOY_FILE_GRF, grf + 1, 3 * 4);
      a0[3] = tsrc(TOY_FILE_GRF, grf + 1, 7 * 4);

      for (ch = 0; ch < 4; ch++)
         tc_MOV(tc, real_dst[ch], tsrc_rect(a0[ch], TOY_RECT_010));
   }
   else {
      struct toy_src attr[4], uv;

      attr[0] = tsrc(TOY_FILE_GRF, grf + 0, 0);
      attr[1] = tsrc(TOY_FILE_GRF, grf + 0, 4 * 4);
      attr[2] = tsrc(TOY_FILE_GRF, grf + 1, 0);
      attr[3] = tsrc(TOY_FILE_GRF, grf + 1, 4 * 4);

      uv = tsrc(TOY_FILE_GRF, interp, 0);

      for (ch = 0; ch < 4; ch++) {
         tc_add2(tc, GEN6_OPCODE_PLN, real_dst[ch],
               tsrc_rect(attr[ch], TOY_RECT_010), uv);
      }
   }

   if (fcc->tgsi.inputs[slot].semantic_name == TGSI_SEMANTIC_FOG) {
      tc_MOV(tc, real_dst[1], tsrc_imm_f(0.0f));
      tc_MOV(tc, real_dst[2], tsrc_imm_f(0.0f));
      tc_MOV(tc, real_dst[3], tsrc_imm_f(1.0f));
   }
}

static void
fs_lower_opcode_tgsi_in(struct fs_compile_context *fcc,
                        struct toy_dst dst, int dim, int idx)
{
   int slot;

   assert(!dim);

   slot = toy_tgsi_find_input(&fcc->tgsi, idx);
   if (slot < 0)
      return;

   switch (fcc->tgsi.inputs[slot].semantic_name) {
   case TGSI_SEMANTIC_POSITION:
      fetch_position(fcc, dst);
      break;
   case TGSI_SEMANTIC_FACE:
      fetch_face(fcc, dst);
      break;
   default:
      fetch_attr(fcc, dst, slot);
      break;
   }
}

static void
fs_lower_opcode_tgsi_indirect_const(struct fs_compile_context *fcc,
                                    struct toy_dst dst, int dim,
                                    struct toy_src idx)
{
   const struct toy_dst offset =
      tdst_ud(tdst(TOY_FILE_MRF, fcc->first_free_mrf, 0));
   struct toy_compiler *tc = &fcc->tc;
   unsigned simd_mode, param_size;
   struct toy_inst *inst;
   struct toy_src desc, real_src[4];
   struct toy_dst tmp, real_dst[4];
   int i;

   tsrc_transpose(idx, real_src);

   /* set offset */
   inst = tc_MOV(tc, offset, real_src[0]);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   switch (inst->exec_size) {
   case GEN6_EXECSIZE_8:
      simd_mode = GEN6_MSG_SAMPLER_SIMD8;
      param_size = 1;
      break;
   case GEN6_EXECSIZE_16:
      simd_mode = GEN6_MSG_SAMPLER_SIMD16;
      param_size = 2;
      break;
   default:
      assert(!"unsupported execution size");
      tc_MOV(tc, dst, tsrc_imm_f(0.0f));
      return;
      break;
   }

   desc = tsrc_imm_mdesc_sampler(tc, param_size, param_size * 4, false,
         simd_mode,
         GEN6_MSG_SAMPLER_LD,
         0,
         fcc->shader->bt.const_base + dim);

   tmp = tdst(TOY_FILE_VRF, tc_alloc_vrf(tc, param_size * 4), 0);
   inst = tc_SEND(tc, tmp, tsrc_from(offset), desc, GEN6_SFID_SAMPLER);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   tdst_transpose(dst, real_dst);
   for (i = 0; i < 4; i++) {
      const struct toy_src src =
         tsrc_offset(tsrc_from(tmp), param_size * i, 0);

      /* cast to type D to make sure these are raw moves */
      tc_MOV(tc, tdst_d(real_dst[i]), tsrc_d(src));
   }
}

static bool
fs_lower_opcode_tgsi_const_pcb(struct fs_compile_context *fcc,
                               struct toy_dst dst, int dim,
                               struct toy_src idx)
{
   const int grf = fcc->first_const_grf + idx.val32 / 2;
   const int grf_subreg = (idx.val32 & 1) * 16;
   struct toy_src src;
   struct toy_dst real_dst[4];
   int i;

   if (!fcc->variant->use_pcb || dim != 0 || idx.file != TOY_FILE_IMM ||
       grf >= fcc->first_attr_grf)
      return false;

   src = tsrc_rect(tsrc(TOY_FILE_GRF, grf, grf_subreg), TOY_RECT_010);

   tdst_transpose(dst, real_dst);
   for (i = 0; i < 4; i++) {
      /* cast to type D to make sure these are raw moves */
      tc_MOV(&fcc->tc, tdst_d(real_dst[i]), tsrc_d(tsrc_offset(src, 0, i)));
   }

   return true;
}

static void
fs_lower_opcode_tgsi_const_gen6(struct fs_compile_context *fcc,
                                struct toy_dst dst, int dim, struct toy_src idx)
{
   const struct toy_dst header =
      tdst_ud(tdst(TOY_FILE_MRF, fcc->first_free_mrf, 0));
   const struct toy_dst global_offset =
      tdst_ud(tdst(TOY_FILE_MRF, fcc->first_free_mrf, 2 * 4));
   const struct toy_src r0 = tsrc_ud(tsrc(TOY_FILE_GRF, 0, 0));
   struct toy_compiler *tc = &fcc->tc;
   unsigned msg_type, msg_ctrl, msg_len;
   struct toy_inst *inst;
   struct toy_src desc;
   struct toy_dst tmp, real_dst[4];
   int i;

   if (fs_lower_opcode_tgsi_const_pcb(fcc, dst, dim, idx))
      return;

   /* set message header */
   inst = tc_MOV(tc, header, r0);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   /* set global offset */
   inst = tc_MOV(tc, global_offset, idx);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
   inst->exec_size = GEN6_EXECSIZE_1;
   inst->src[0].rect = TOY_RECT_010;

   msg_type = GEN6_MSG_DP_OWORD_BLOCK_READ;
   msg_ctrl = GEN6_MSG_DP_OWORD_BLOCK_SIZE_1_LO;
   msg_len = 1;

   desc = tsrc_imm_mdesc_data_port(tc, false, msg_len, 1, true, false,
         msg_type, msg_ctrl, fcc->shader->bt.const_base + dim);

   tmp = tc_alloc_tmp(tc);

   tc_SEND(tc, tmp, tsrc_from(header), desc, fcc->const_cache);

   tdst_transpose(dst, real_dst);
   for (i = 0; i < 4; i++) {
      const struct toy_src src =
         tsrc_offset(tsrc_rect(tsrc_from(tmp), TOY_RECT_010), 0, i);

      /* cast to type D to make sure these are raw moves */
      tc_MOV(tc, tdst_d(real_dst[i]), tsrc_d(src));
   }
}

static void
fs_lower_opcode_tgsi_const_gen7(struct fs_compile_context *fcc,
                                struct toy_dst dst, int dim, struct toy_src idx)
{
   struct toy_compiler *tc = &fcc->tc;
   const struct toy_dst offset =
      tdst_ud(tdst(TOY_FILE_MRF, fcc->first_free_mrf, 0));
   struct toy_src desc;
   struct toy_inst *inst;
   struct toy_dst tmp, real_dst[4];
   int i;

   if (fs_lower_opcode_tgsi_const_pcb(fcc, dst, dim, idx))
      return;

   /*
    * In 4c1fdae0a01b3f92ec03b61aac1d3df500d51fc6, pull constant load was
    * changed from OWord Block Read to ld to increase performance in the
    * classic driver.  Since we use the constant cache instead of the data
    * cache, I wonder if we still want to follow the classic driver.
    */

   /* set offset */
   inst = tc_MOV(tc, offset, tsrc_rect(idx, TOY_RECT_010));
   inst->exec_size = GEN6_EXECSIZE_8;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   desc = tsrc_imm_mdesc_sampler(tc, 1, 1, false,
         GEN6_MSG_SAMPLER_SIMD4X2,
         GEN6_MSG_SAMPLER_LD,
         0,
         fcc->shader->bt.const_base + dim);

   tmp = tc_alloc_tmp(tc);
   inst = tc_SEND(tc, tmp, tsrc_from(offset), desc, GEN6_SFID_SAMPLER);
   inst->exec_size = GEN6_EXECSIZE_8;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   tdst_transpose(dst, real_dst);
   for (i = 0; i < 4; i++) {
      const struct toy_src src =
         tsrc_offset(tsrc_rect(tsrc_from(tmp), TOY_RECT_010), 0, i);

      /* cast to type D to make sure these are raw moves */
      tc_MOV(tc, tdst_d(real_dst[i]), tsrc_d(src));
   }
}

static void
fs_lower_opcode_tgsi_imm(struct fs_compile_context *fcc,
                         struct toy_dst dst, int idx)
{
   const uint32_t *imm;
   struct toy_dst real_dst[4];
   int ch;

   imm = toy_tgsi_get_imm(&fcc->tgsi, idx, NULL);

   tdst_transpose(dst, real_dst);
   /* raw moves */
   for (ch = 0; ch < 4; ch++)
      tc_MOV(&fcc->tc, tdst_ud(real_dst[ch]), tsrc_imm_ud(imm[ch]));
}

static void
fs_lower_opcode_tgsi_sv(struct fs_compile_context *fcc,
                        struct toy_dst dst, int dim, int idx)
{
   struct toy_compiler *tc = &fcc->tc;
   const struct toy_tgsi *tgsi = &fcc->tgsi;
   int slot;

   assert(!dim);

   slot = toy_tgsi_find_system_value(tgsi, idx);
   if (slot < 0)
      return;

   switch (tgsi->system_values[slot].semantic_name) {
   case TGSI_SEMANTIC_PRIMID:
   case TGSI_SEMANTIC_INSTANCEID:
   case TGSI_SEMANTIC_VERTEXID:
   default:
      tc_fail(tc, "unhandled system value");
      tc_MOV(tc, dst, tsrc_imm_d(0));
      break;
   }
}

static void
fs_lower_opcode_tgsi_direct(struct fs_compile_context *fcc,
                            struct toy_inst *inst)
{
   struct toy_compiler *tc = &fcc->tc;
   int dim, idx;

   assert(inst->src[0].file == TOY_FILE_IMM);
   dim = inst->src[0].val32;

   assert(inst->src[1].file == TOY_FILE_IMM);
   idx = inst->src[1].val32;

   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_IN:
      fs_lower_opcode_tgsi_in(fcc, inst->dst, dim, idx);
      break;
   case TOY_OPCODE_TGSI_CONST:
      if (ilo_dev_gen(tc->dev) >= ILO_GEN(7))
         fs_lower_opcode_tgsi_const_gen7(fcc, inst->dst, dim, inst->src[1]);
      else
         fs_lower_opcode_tgsi_const_gen6(fcc, inst->dst, dim, inst->src[1]);
      break;
   case TOY_OPCODE_TGSI_SV:
      fs_lower_opcode_tgsi_sv(fcc, inst->dst, dim, idx);
      break;
   case TOY_OPCODE_TGSI_IMM:
      assert(!dim);
      fs_lower_opcode_tgsi_imm(fcc, inst->dst, idx);
      break;
   default:
      tc_fail(tc, "unhandled TGSI fetch");
      break;
   }

   tc_discard_inst(tc, inst);
}

static void
fs_lower_opcode_tgsi_indirect(struct fs_compile_context *fcc,
                              struct toy_inst *inst)
{
   struct toy_compiler *tc = &fcc->tc;
   enum tgsi_file_type file;
   int dim, idx;
   struct toy_src indirect_dim, indirect_idx;

   assert(inst->src[0].file == TOY_FILE_IMM);
   file = inst->src[0].val32;

   assert(inst->src[1].file == TOY_FILE_IMM);
   dim = inst->src[1].val32;
   indirect_dim = inst->src[2];

   assert(inst->src[3].file == TOY_FILE_IMM);
   idx = inst->src[3].val32;
   indirect_idx = inst->src[4];

   /* no dimension indirection */
   assert(indirect_dim.file == TOY_FILE_IMM);
   dim += indirect_dim.val32;

   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_INDIRECT_FETCH:
      if (file == TGSI_FILE_CONSTANT) {
         if (idx) {
            struct toy_dst tmp = tc_alloc_tmp(tc);

            tc_ADD(tc, tmp, indirect_idx, tsrc_imm_d(idx));
            indirect_idx = tsrc_from(tmp);
         }

         fs_lower_opcode_tgsi_indirect_const(fcc, inst->dst, dim, indirect_idx);
         break;
      }
      /* fall through */
   case TOY_OPCODE_TGSI_INDIRECT_STORE:
   default:
      tc_fail(tc, "unhandled TGSI indirection");
      break;
   }

   tc_discard_inst(tc, inst);
}

/**
 * Emit instructions to move sampling parameters to the message registers.
 */
static int
fs_add_sampler_params_gen6(struct toy_compiler *tc, int msg_type,
                           int base_mrf, int param_size,
                           struct toy_src *coords, int num_coords,
                           struct toy_src bias_or_lod, struct toy_src ref_or_si,
                           struct toy_src *ddx, struct toy_src *ddy,
                           int num_derivs)
{
   int num_params, i;

   assert(num_coords <= 4);
   assert(num_derivs <= 3 && num_derivs <= num_coords);

#define SAMPLER_PARAM(p) (tdst(TOY_FILE_MRF, base_mrf + (p) * param_size, 0))
   switch (msg_type) {
   case GEN6_MSG_SAMPLER_SAMPLE:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      num_params = num_coords;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_B:
   case GEN6_MSG_SAMPLER_SAMPLE_L:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      tc_MOV(tc, SAMPLER_PARAM(4), bias_or_lod);
      num_params = 5;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_C:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      tc_MOV(tc, SAMPLER_PARAM(4), ref_or_si);
      num_params = 5;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_D:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      for (i = 0; i < num_derivs; i++) {
         tc_MOV(tc, SAMPLER_PARAM(4 + i * 2), ddx[i]);
         tc_MOV(tc, SAMPLER_PARAM(5 + i * 2), ddy[i]);
      }
      num_params = 4 + num_derivs * 2;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_B_C:
   case GEN6_MSG_SAMPLER_SAMPLE_L_C:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      tc_MOV(tc, SAMPLER_PARAM(4), ref_or_si);
      tc_MOV(tc, SAMPLER_PARAM(5), bias_or_lod);
      num_params = 6;
      break;
   case GEN6_MSG_SAMPLER_LD:
      assert(num_coords <= 3);

      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, tdst_d(SAMPLER_PARAM(i)), coords[i]);
      tc_MOV(tc, tdst_d(SAMPLER_PARAM(3)), bias_or_lod);
      tc_MOV(tc, tdst_d(SAMPLER_PARAM(4)), ref_or_si);
      num_params = 5;
      break;
   case GEN6_MSG_SAMPLER_RESINFO:
      tc_MOV(tc, tdst_d(SAMPLER_PARAM(0)), bias_or_lod);
      num_params = 1;
      break;
   default:
      tc_fail(tc, "unknown sampler opcode");
      num_params = 0;
      break;
   }
#undef SAMPLER_PARAM

   return num_params * param_size;
}

static int
fs_add_sampler_params_gen7(struct toy_compiler *tc, int msg_type,
                           int base_mrf, int param_size,
                           struct toy_src *coords, int num_coords,
                           struct toy_src bias_or_lod, struct toy_src ref_or_si,
                           struct toy_src *ddx, struct toy_src *ddy,
                           int num_derivs)
{
   int num_params, i;

   assert(num_coords <= 4);
   assert(num_derivs <= 3 && num_derivs <= num_coords);

#define SAMPLER_PARAM(p) (tdst(TOY_FILE_MRF, base_mrf + (p) * param_size, 0))
   switch (msg_type) {
   case GEN6_MSG_SAMPLER_SAMPLE:
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(i), coords[i]);
      num_params = num_coords;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_B:
   case GEN6_MSG_SAMPLER_SAMPLE_L:
      tc_MOV(tc, SAMPLER_PARAM(0), bias_or_lod);
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(1 + i), coords[i]);
      num_params = 1 + num_coords;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_C:
      tc_MOV(tc, SAMPLER_PARAM(0), ref_or_si);
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(1 + i), coords[i]);
      num_params = 1 + num_coords;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_D:
      for (i = 0; i < num_coords; i++) {
         tc_MOV(tc, SAMPLER_PARAM(i * 3), coords[i]);
         if (i < num_derivs) {
            tc_MOV(tc, SAMPLER_PARAM(i * 3 + 1), ddx[i]);
            tc_MOV(tc, SAMPLER_PARAM(i * 3 + 2), ddy[i]);
         }
      }
      num_params = num_coords * 3 - ((num_coords > num_derivs) ? 2 : 0);
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_B_C:
   case GEN6_MSG_SAMPLER_SAMPLE_L_C:
      tc_MOV(tc, SAMPLER_PARAM(0), ref_or_si);
      tc_MOV(tc, SAMPLER_PARAM(1), bias_or_lod);
      for (i = 0; i < num_coords; i++)
         tc_MOV(tc, SAMPLER_PARAM(2 + i), coords[i]);
      num_params = 2 + num_coords;
      break;
   case GEN6_MSG_SAMPLER_LD:
      assert(num_coords >= 1 && num_coords <= 3);

      tc_MOV(tc, tdst_d(SAMPLER_PARAM(0)), coords[0]);
      tc_MOV(tc, tdst_d(SAMPLER_PARAM(1)), bias_or_lod);
      for (i = 1; i < num_coords; i++)
         tc_MOV(tc, tdst_d(SAMPLER_PARAM(1 + i)), coords[i]);
      num_params = 1 + num_coords;
      break;
   case GEN6_MSG_SAMPLER_RESINFO:
      tc_MOV(tc, tdst_d(SAMPLER_PARAM(0)), bias_or_lod);
      num_params = 1;
      break;
   default:
      tc_fail(tc, "unknown sampler opcode");
      num_params = 0;
      break;
   }
#undef SAMPLER_PARAM

   return num_params * param_size;
}

/**
 * Set up message registers and return the message descriptor for sampling.
 */
static struct toy_src
fs_prepare_tgsi_sampling(struct fs_compile_context *fcc,
                         const struct toy_inst *inst,
                         int base_mrf, const uint32_t *saturate_coords,
                         unsigned *ret_sampler_index)
{
   struct toy_compiler *tc = &fcc->tc;
   unsigned simd_mode, msg_type, msg_len, sampler_index, binding_table_index;
   struct toy_src coords[4], ddx[4], ddy[4], bias_or_lod, ref_or_si;
   int num_coords, ref_pos, num_derivs;
   int sampler_src, param_size, i;

   switch (inst->exec_size) {
   case GEN6_EXECSIZE_8:
      simd_mode = GEN6_MSG_SAMPLER_SIMD8;
      param_size = 1;
      break;
   case GEN6_EXECSIZE_16:
      simd_mode = GEN6_MSG_SAMPLER_SIMD16;
      param_size = 2;
      break;
   default:
      tc_fail(tc, "unsupported execute size for sampling");
      return tsrc_null();
      break;
   }

   num_coords = tgsi_util_get_texture_coord_dim(inst->tex.target, &ref_pos);
   tsrc_transpose(inst->src[0], coords);
   bias_or_lod = tsrc_null();
   ref_or_si = tsrc_null();
   num_derivs = 0;
   sampler_src = 1;

   /*
    * For TXD,
    *
    *   src0 := (x, y, z, w)
    *   src1 := ddx
    *   src2 := ddy
    *   src3 := sampler
    *
    * For TEX2, TXB2, and TXL2,
    *
    *   src0 := (x, y, z, w)
    *   src1 := (v or bias or lod, ...)
    *   src2 := sampler
    *
    * For TEX, TXB, TXL, and TXP,
    *
    *   src0 := (x, y, z, w or bias or lod or projection)
    *   src1 := sampler
    *
    * For TXQ,
    *
    *   src0 := (lod, ...)
    *   src1 := sampler
    *
    * For TXQ_LZ,
    *
    *   src0 := sampler
    *
    * And for TXF,
    *
    *   src0 := (x, y, z, w or lod)
    *   src1 := sampler
    *
    * State trackers should not generate opcode+texture combinations with
    * which the two definitions conflict (e.g., TXB with SHADOW2DARRAY).
    */
   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_TEX:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE;
      }
      break;
   case TOY_OPCODE_TGSI_TXD:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN7_MSG_SAMPLER_SAMPLE_D_C;
         ref_or_si = coords[ref_pos];

         if (ilo_dev_gen(tc->dev) < ILO_GEN(7.5))
            tc_fail(tc, "TXD with shadow sampler not supported");
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_D;
      }

      tsrc_transpose(inst->src[1], ddx);
      tsrc_transpose(inst->src[2], ddy);
      num_derivs = num_coords;
      sampler_src = 3;
      break;
   case TOY_OPCODE_TGSI_TXP:
      if (ref_pos >= 0) {
         assert(ref_pos < 3);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE;
      }

      /* project the coordinates */
      {
         struct toy_dst tmp[4];

         tc_alloc_tmp4(tc, tmp);

         tc_INV(tc, tmp[3], coords[3]);
         for (i = 0; i < num_coords && i < 3; i++) {
            tc_MUL(tc, tmp[i], coords[i], tsrc_from(tmp[3]));
            coords[i] = tsrc_from(tmp[i]);
         }

         if (ref_pos >= i) {
            tc_MUL(tc, tmp[ref_pos], ref_or_si, tsrc_from(tmp[3]));
            ref_or_si = tsrc_from(tmp[ref_pos]);
         }
      }
      break;
   case TOY_OPCODE_TGSI_TXB:
      if (ref_pos >= 0) {
         assert(ref_pos < 3);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_B_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_B;
      }

      bias_or_lod = coords[3];
      break;
   case TOY_OPCODE_TGSI_TXL:
      if (ref_pos >= 0) {
         assert(ref_pos < 3);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L;
      }

      bias_or_lod = coords[3];
      break;
   case TOY_OPCODE_TGSI_TXF:
      msg_type = GEN6_MSG_SAMPLER_LD;

      switch (inst->tex.target) {
      case TGSI_TEXTURE_2D_MSAA:
      case TGSI_TEXTURE_2D_ARRAY_MSAA:
         assert(ref_pos >= 0 && ref_pos < 4);
         /* lod is always 0 */
         bias_or_lod = tsrc_imm_d(0);
         ref_or_si = coords[ref_pos];
         break;
      default:
         bias_or_lod = coords[3];
         break;
      }

      /* offset the coordinates */
      if (!tsrc_is_null(inst->tex.offsets[0])) {
         struct toy_dst tmp[4];
         struct toy_src offsets[4];

         tc_alloc_tmp4(tc, tmp);
         tsrc_transpose(inst->tex.offsets[0], offsets);

         for (i = 0; i < num_coords; i++) {
            tc_ADD(tc, tmp[i], coords[i], offsets[i]);
            coords[i] = tsrc_from(tmp[i]);
         }
      }

      sampler_src = 1;
      break;
   case TOY_OPCODE_TGSI_TXQ:
      msg_type = GEN6_MSG_SAMPLER_RESINFO;
      num_coords = 0;
      bias_or_lod = coords[0];
      break;
   case TOY_OPCODE_TGSI_TXQ_LZ:
      msg_type = GEN6_MSG_SAMPLER_RESINFO;
      num_coords = 0;
      sampler_src = 0;
      break;
   case TOY_OPCODE_TGSI_TEX2:
      if (ref_pos >= 0) {
         assert(ref_pos < 5);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_C;

         if (ref_pos >= 4) {
            struct toy_src src1[4];
            tsrc_transpose(inst->src[1], src1);
            ref_or_si = src1[ref_pos - 4];
         }
         else {
            ref_or_si = coords[ref_pos];
         }
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE;
      }

      sampler_src = 2;
      break;
   case TOY_OPCODE_TGSI_TXB2:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_B_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_B;
      }

      {
         struct toy_src src1[4];
         tsrc_transpose(inst->src[1], src1);
         bias_or_lod = src1[0];
      }

      sampler_src = 2;
      break;
   case TOY_OPCODE_TGSI_TXL2:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L_C;
         ref_or_si = coords[ref_pos];
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L;
      }

      {
         struct toy_src src1[4];
         tsrc_transpose(inst->src[1], src1);
         bias_or_lod = src1[0];
      }

      sampler_src = 2;
      break;
   default:
      assert(!"unhandled sampling opcode");
      return tsrc_null();
      break;
   }

   assert(inst->src[sampler_src].file == TOY_FILE_IMM);
   sampler_index = inst->src[sampler_src].val32;
   binding_table_index = fcc->shader->bt.tex_base + sampler_index;

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 18:
    *
    *     "Note that the (cube map) coordinates delivered to the sampling
    *      engine must already have been divided by the component with the
    *      largest absolute value."
    */
   switch (inst->tex.target) {
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      /* TXQ does not need coordinates */
      if (num_coords >= 3) {
         struct toy_dst tmp[4];

         tc_alloc_tmp4(tc, tmp);

         tc_SEL(tc, tmp[3], tsrc_absolute(coords[0]),
               tsrc_absolute(coords[1]), GEN6_COND_GE);
         tc_SEL(tc, tmp[3], tsrc_from(tmp[3]),
               tsrc_absolute(coords[2]), GEN6_COND_GE);
         tc_INV(tc, tmp[3], tsrc_from(tmp[3]));

         for (i = 0; i < 3; i++) {
            tc_MUL(tc, tmp[i], coords[i], tsrc_from(tmp[3]));
            coords[i] = tsrc_from(tmp[i]);
         }
      }
      break;
   }

   /*
    * Saturate (s, t, r).  saturate_coords is set for sampler and coordinate
    * that uses linear filtering and PIPE_TEX_WRAP_CLAMP respectively.  It is
    * so that sampling outside the border gets the correct colors.
    */
   for (i = 0; i < MIN2(num_coords, 3); i++) {
      bool is_rect;

      if (!(saturate_coords[i] & (1 << sampler_index)))
         continue;

      switch (inst->tex.target) {
      case TGSI_TEXTURE_RECT:
      case TGSI_TEXTURE_SHADOWRECT:
         is_rect = true;
         break;
      default:
         is_rect = false;
         break;
      }

      if (is_rect) {
         struct toy_src min, max;
         struct toy_dst tmp;

         tc_fail(tc, "GL_CLAMP with rectangle texture unsupported");
         tmp = tc_alloc_tmp(tc);

         /* saturate to [0, width] or [0, height] */
         /* TODO TXQ? */
         min = tsrc_imm_f(0.0f);
         max = tsrc_imm_f(2048.0f);

         tc_SEL(tc, tmp, coords[i], min, GEN6_COND_G);
         tc_SEL(tc, tmp, tsrc_from(tmp), max, GEN6_COND_L);

         coords[i] = tsrc_from(tmp);
      }
      else {
         struct toy_dst tmp;
         struct toy_inst *inst2;

         tmp = tc_alloc_tmp(tc);

         /* saturate to [0.0f, 1.0f] */
         inst2 = tc_MOV(tc, tmp, coords[i]);
         inst2->saturate = true;

         coords[i] = tsrc_from(tmp);
      }
   }

   /* set up sampler parameters */
   if (ilo_dev_gen(tc->dev) >= ILO_GEN(7)) {
      msg_len = fs_add_sampler_params_gen7(tc, msg_type, base_mrf, param_size,
            coords, num_coords, bias_or_lod, ref_or_si, ddx, ddy, num_derivs);
   }
   else {
      msg_len = fs_add_sampler_params_gen6(tc, msg_type, base_mrf, param_size,
            coords, num_coords, bias_or_lod, ref_or_si, ddx, ddy, num_derivs);
   }

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 136:
    *
    *     "The maximum message length allowed to the sampler is 11. This would
    *      disallow sample_d, sample_b_c, and sample_l_c with a SIMD Mode of
    *      SIMD16."
    */
   if (msg_len > 11)
      tc_fail(tc, "maximum length for messages to the sampler is 11");

   if (ret_sampler_index)
      *ret_sampler_index = sampler_index;

   return tsrc_imm_mdesc_sampler(tc, msg_len, 4 * param_size,
         false, simd_mode, msg_type, sampler_index, binding_table_index);
}

static void
fs_lower_opcode_tgsi_sampling(struct fs_compile_context *fcc,
                              struct toy_inst *inst)
{
   struct toy_compiler *tc = &fcc->tc;
   struct toy_dst dst[4], tmp[4];
   struct toy_src desc;
   unsigned sampler_index;
   int swizzles[4], i;
   bool need_filter;

   desc = fs_prepare_tgsi_sampling(fcc, inst,
         fcc->first_free_mrf,
         fcc->variant->saturate_tex_coords,
         &sampler_index);

   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_TXF:
   case TOY_OPCODE_TGSI_TXQ:
   case TOY_OPCODE_TGSI_TXQ_LZ:
      need_filter = false;
      break;
   default:
      need_filter = true;
      break;
   }

   toy_compiler_lower_to_send(tc, inst, false, GEN6_SFID_SAMPLER);
   inst->src[0] = tsrc(TOY_FILE_MRF, fcc->first_free_mrf, 0);
   inst->src[1] = desc;
   for (i = 2; i < Elements(inst->src); i++)
      inst->src[i] = tsrc_null();

   /* write to temps first */
   tc_alloc_tmp4(tc, tmp);
   for (i = 0; i < 4; i++)
      tmp[i].type = inst->dst.type;
   tdst_transpose(inst->dst, dst);
   inst->dst = tmp[0];

   tc_move_inst(tc, inst);

   if (need_filter) {
      assert(sampler_index < fcc->variant->num_sampler_views);
      swizzles[0] = fcc->variant->sampler_view_swizzles[sampler_index].r;
      swizzles[1] = fcc->variant->sampler_view_swizzles[sampler_index].g;
      swizzles[2] = fcc->variant->sampler_view_swizzles[sampler_index].b;
      swizzles[3] = fcc->variant->sampler_view_swizzles[sampler_index].a;
   }
   else {
      swizzles[0] = PIPE_SWIZZLE_RED;
      swizzles[1] = PIPE_SWIZZLE_GREEN;
      swizzles[2] = PIPE_SWIZZLE_BLUE;
      swizzles[3] = PIPE_SWIZZLE_ALPHA;
   }

   /* swizzle the results */
   for (i = 0; i < 4; i++) {
      switch (swizzles[i]) {
      case PIPE_SWIZZLE_ZERO:
         tc_MOV(tc, dst[i], tsrc_imm_f(0.0f));
         break;
      case PIPE_SWIZZLE_ONE:
         tc_MOV(tc, dst[i], tsrc_imm_f(1.0f));
         break;
      default:
         tc_MOV(tc, dst[i], tsrc_from(tmp[swizzles[i]]));
         break;
      }
   }
}

static void
fs_lower_opcode_derivative(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_dst dst[4];
   struct toy_src src[4];
   int i;

   tdst_transpose(inst->dst, dst);
   tsrc_transpose(inst->src[0], src);

   /*
    * Every four fragments are from a 2x2 subspan, with
    *
    *   fragment 1 on the top-left,
    *   fragment 2 on the top-right,
    *   fragment 3 on the bottom-left,
    *   fragment 4 on the bottom-right.
    *
    * DDX should thus produce
    *
    *   dst = src.yyww - src.xxzz
    *
    * and DDY should produce
    *
    *   dst = src.zzww - src.xxyy
    *
    * But since we are in GEN6_ALIGN_1, swizzling does not work and we have to
    * play with the region parameters.
    */
   if (inst->opcode == TOY_OPCODE_DDX) {
      for (i = 0; i < 4; i++) {
         struct toy_src left, right;

         left = tsrc_rect(src[i], TOY_RECT_220);
         right = tsrc_offset(left, 0, 1);

         tc_ADD(tc, dst[i], right, tsrc_negate(left));
      }
   }
   else {
      for (i = 0; i < 4; i++) {
         struct toy_src top, bottom;

         /* approximate with dst = src.zzzz - src.xxxx */
         top = tsrc_rect(src[i], TOY_RECT_440);
         bottom = tsrc_offset(top, 0, 2);

         tc_ADD(tc, dst[i], bottom, tsrc_negate(top));
      }
   }

   tc_discard_inst(tc, inst);
}

static void
fs_lower_opcode_fb_write(struct toy_compiler *tc, struct toy_inst *inst)
{
   /* fs_write_fb() has set up the message registers */
   toy_compiler_lower_to_send(tc, inst, true,
         GEN6_SFID_DP_RC);
}

static void
fs_lower_opcode_kil(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_dst pixel_mask_dst;
   struct toy_src f0, pixel_mask;
   struct toy_inst *tmp;

   /* lower half of r1.7:ud */
   pixel_mask_dst = tdst_uw(tdst(TOY_FILE_GRF, 1, 7 * 4));
   pixel_mask = tsrc_rect(tsrc_from(pixel_mask_dst), TOY_RECT_010);

   f0 = tsrc_rect(tsrc_uw(tsrc(TOY_FILE_ARF, GEN6_ARF_F0, 0)), TOY_RECT_010);

   /* KILL or KILL_IF */
   if (tsrc_is_null(inst->src[0])) {
      struct toy_src dummy = tsrc_uw(tsrc(TOY_FILE_GRF, 0, 0));
      struct toy_dst f0_dst = tdst_uw(tdst(TOY_FILE_ARF, GEN6_ARF_F0, 0));

      /* create a mask that masks out all pixels */
      tmp = tc_MOV(tc, f0_dst, tsrc_rect(tsrc_imm_uw(0xffff), TOY_RECT_010));
      tmp->exec_size = GEN6_EXECSIZE_1;
      tmp->mask_ctrl = GEN6_MASKCTRL_NOMASK;

      tc_CMP(tc, tdst_null(), dummy, dummy, GEN6_COND_NZ);

      /* swapping the two src operands breaks glBitmap()!? */
      tmp = tc_AND(tc, pixel_mask_dst, f0, pixel_mask);
      tmp->exec_size = GEN6_EXECSIZE_1;
      tmp->mask_ctrl = GEN6_MASKCTRL_NOMASK;
   }
   else {
      struct toy_src src[4];
      int i;

      tsrc_transpose(inst->src[0], src);
      /* mask out killed pixels */
      for (i = 0; i < 4; i++) {
         tc_CMP(tc, tdst_null(), src[i], tsrc_imm_f(0.0f),
               GEN6_COND_GE);

         /* swapping the two src operands breaks glBitmap()!? */
         tmp = tc_AND(tc, pixel_mask_dst, f0, pixel_mask);
         tmp->exec_size = GEN6_EXECSIZE_1;
         tmp->mask_ctrl = GEN6_MASKCTRL_NOMASK;
      }
   }

   tc_discard_inst(tc, inst);
}

static void
fs_lower_virtual_opcodes(struct fs_compile_context *fcc)
{
   struct toy_compiler *tc = &fcc->tc;
   struct toy_inst *inst;

   /* lower TGSI's first, as they might be lowered to other virtual opcodes */
   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case TOY_OPCODE_TGSI_IN:
      case TOY_OPCODE_TGSI_CONST:
      case TOY_OPCODE_TGSI_SV:
      case TOY_OPCODE_TGSI_IMM:
         fs_lower_opcode_tgsi_direct(fcc, inst);
         break;
      case TOY_OPCODE_TGSI_INDIRECT_FETCH:
      case TOY_OPCODE_TGSI_INDIRECT_STORE:
         fs_lower_opcode_tgsi_indirect(fcc, inst);
         break;
      case TOY_OPCODE_TGSI_TEX:
      case TOY_OPCODE_TGSI_TXB:
      case TOY_OPCODE_TGSI_TXD:
      case TOY_OPCODE_TGSI_TXL:
      case TOY_OPCODE_TGSI_TXP:
      case TOY_OPCODE_TGSI_TXF:
      case TOY_OPCODE_TGSI_TXQ:
      case TOY_OPCODE_TGSI_TXQ_LZ:
      case TOY_OPCODE_TGSI_TEX2:
      case TOY_OPCODE_TGSI_TXB2:
      case TOY_OPCODE_TGSI_TXL2:
      case TOY_OPCODE_TGSI_SAMPLE:
      case TOY_OPCODE_TGSI_SAMPLE_I:
      case TOY_OPCODE_TGSI_SAMPLE_I_MS:
      case TOY_OPCODE_TGSI_SAMPLE_B:
      case TOY_OPCODE_TGSI_SAMPLE_C:
      case TOY_OPCODE_TGSI_SAMPLE_C_LZ:
      case TOY_OPCODE_TGSI_SAMPLE_D:
      case TOY_OPCODE_TGSI_SAMPLE_L:
      case TOY_OPCODE_TGSI_GATHER4:
      case TOY_OPCODE_TGSI_SVIEWINFO:
      case TOY_OPCODE_TGSI_SAMPLE_POS:
      case TOY_OPCODE_TGSI_SAMPLE_INFO:
         fs_lower_opcode_tgsi_sampling(fcc, inst);
         break;
      }
   }

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case TOY_OPCODE_INV:
      case TOY_OPCODE_LOG:
      case TOY_OPCODE_EXP:
      case TOY_OPCODE_SQRT:
      case TOY_OPCODE_RSQ:
      case TOY_OPCODE_SIN:
      case TOY_OPCODE_COS:
      case TOY_OPCODE_FDIV:
      case TOY_OPCODE_POW:
      case TOY_OPCODE_INT_DIV_QUOTIENT:
      case TOY_OPCODE_INT_DIV_REMAINDER:
         toy_compiler_lower_math(tc, inst);
         break;
      case TOY_OPCODE_DDX:
      case TOY_OPCODE_DDY:
         fs_lower_opcode_derivative(tc, inst);
         break;
      case TOY_OPCODE_FB_WRITE:
         fs_lower_opcode_fb_write(tc, inst);
         break;
      case TOY_OPCODE_KIL:
         fs_lower_opcode_kil(tc, inst);
         break;
      default:
         if (inst->opcode > 127)
            tc_fail(tc, "unhandled virtual opcode");
         break;
      }
   }
}

/**
 * Compile the shader.
 */
static bool
fs_compile(struct fs_compile_context *fcc)
{
   struct toy_compiler *tc = &fcc->tc;
   struct ilo_shader *sh = fcc->shader;

   fs_lower_virtual_opcodes(fcc);
   toy_compiler_legalize_for_ra(tc);
   toy_compiler_optimize(tc);
   toy_compiler_allocate_registers(tc,
         fcc->first_free_grf,
         fcc->last_free_grf,
         fcc->num_grf_per_vrf);
   toy_compiler_legalize_for_asm(tc);

   if (tc->fail) {
      ilo_err("failed to legalize FS instructions: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_FS) {
      ilo_printf("legalized instructions:\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   if (true) {
      sh->kernel = toy_compiler_assemble(tc, &sh->kernel_size);
   }
   else {
      static const uint32_t microcode[] = {
         /* fill in the microcode here */
         0x0, 0x0, 0x0, 0x0,
      };
      const bool swap = true;

      sh->kernel_size = sizeof(microcode);
      sh->kernel = MALLOC(sh->kernel_size);

      if (sh->kernel) {
         const int num_dwords = sizeof(microcode) / 4;
         const uint32_t *src = microcode;
         uint32_t *dst = (uint32_t *) sh->kernel;
         int i;

         for (i = 0; i < num_dwords; i += 4) {
            if (swap) {
               dst[i + 0] = src[i + 3];
               dst[i + 1] = src[i + 2];
               dst[i + 2] = src[i + 1];
               dst[i + 3] = src[i + 0];
            }
            else {
               memcpy(dst, src, 16);
            }
         }
      }
   }

   if (!sh->kernel) {
      ilo_err("failed to compile FS: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_FS) {
      ilo_printf("disassembly:\n");
      toy_compiler_disassemble(tc->dev, sh->kernel, sh->kernel_size, false);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Emit instructions to write the color buffers (and the depth buffer).
 */
static void
fs_write_fb(struct fs_compile_context *fcc)
{
   struct toy_compiler *tc = &fcc->tc;
   int base_mrf = fcc->first_free_mrf;
   const struct toy_dst header = tdst_ud(tdst(TOY_FILE_MRF, base_mrf, 0));
   bool header_present = false;
   struct toy_src desc;
   unsigned msg_type, ctrl;
   int color_slots[ILO_MAX_DRAW_BUFFERS], num_cbufs;
   int pos_slot = -1, cbuf, i;

   for (i = 0; i < Elements(color_slots); i++)
      color_slots[i] = -1;

   for (i = 0; i < fcc->tgsi.num_outputs; i++) {
      if (fcc->tgsi.outputs[i].semantic_name == TGSI_SEMANTIC_COLOR) {
         assert(fcc->tgsi.outputs[i].semantic_index < Elements(color_slots));
         color_slots[fcc->tgsi.outputs[i].semantic_index] = i;
      }
      else if (fcc->tgsi.outputs[i].semantic_name == TGSI_SEMANTIC_POSITION) {
         pos_slot = i;
      }
   }

   num_cbufs = fcc->variant->u.fs.num_cbufs;
   /* still need to send EOT (and probably depth) */
   if (!num_cbufs)
      num_cbufs = 1;

   /* we need the header to specify the pixel mask or render target */
   if (fcc->tgsi.uses_kill || num_cbufs > 1) {
      const struct toy_src r0 = tsrc_ud(tsrc(TOY_FILE_GRF, 0, 0));
      struct toy_inst *inst;

      inst = tc_MOV(tc, header, r0);
      inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
      base_mrf += fcc->num_grf_per_vrf;

      /* this is a two-register header */
      if (fcc->dispatch_mode == GEN6_WM_DW5_8_PIXEL_DISPATCH) {
         inst = tc_MOV(tc, tdst_offset(header, 1, 0), tsrc_offset(r0, 1, 0));
         inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
         base_mrf += fcc->num_grf_per_vrf;
      }

      header_present = true;
   }

   for (cbuf = 0; cbuf < num_cbufs; cbuf++) {
      const int slot =
         color_slots[(fcc->tgsi.props.fs_color0_writes_all_cbufs) ? 0 : cbuf];
      int mrf = base_mrf, vrf;
      struct toy_src src[4];

      if (slot >= 0) {
         const unsigned undefined_mask =
            fcc->tgsi.outputs[slot].undefined_mask;
         const int index = fcc->tgsi.outputs[slot].index;

         vrf = toy_tgsi_get_vrf(&fcc->tgsi, TGSI_FILE_OUTPUT, 0, index);
         if (vrf >= 0) {
            const struct toy_src tmp = tsrc(TOY_FILE_VRF, vrf, 0);
            tsrc_transpose(tmp, src);
         }
         else {
            /* use (0, 0, 0, 0) */
            tsrc_transpose(tsrc_imm_f(0.0f), src);
         }

         for (i = 0; i < 4; i++) {
            const struct toy_dst dst = tdst(TOY_FILE_MRF, mrf, 0);

            if (undefined_mask & (1 << i))
               src[i] = tsrc_imm_f(0.0f);

            tc_MOV(tc, dst, src[i]);

            mrf += fcc->num_grf_per_vrf;
         }
      }
      else {
         /* use (0, 0, 0, 0) */
         for (i = 0; i < 4; i++) {
            const struct toy_dst dst = tdst(TOY_FILE_MRF, mrf, 0);

            tc_MOV(tc, dst, tsrc_imm_f(0.0f));
            mrf += fcc->num_grf_per_vrf;
         }
      }

      /* select BLEND_STATE[rt] */
      if (cbuf > 0) {
         struct toy_inst *inst;

         inst = tc_MOV(tc, tdst_offset(header, 0, 2), tsrc_imm_ud(cbuf));
         inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
         inst->exec_size = GEN6_EXECSIZE_1;
         inst->src[0].rect = TOY_RECT_010;
      }

      if (cbuf == 0 && pos_slot >= 0) {
         const int index = fcc->tgsi.outputs[pos_slot].index;
         const struct toy_dst dst = tdst(TOY_FILE_MRF, mrf, 0);
         struct toy_src src[4];
         int vrf;

         vrf = toy_tgsi_get_vrf(&fcc->tgsi, TGSI_FILE_OUTPUT, 0, index);
         if (vrf >= 0) {
            const struct toy_src tmp = tsrc(TOY_FILE_VRF, vrf, 0);
            tsrc_transpose(tmp, src);
         }
         else {
            /* use (0, 0, 0, 0) */
            tsrc_transpose(tsrc_imm_f(0.0f), src);
         }

         /* only Z */
         tc_MOV(tc, dst, src[2]);

         mrf += fcc->num_grf_per_vrf;
      }

      msg_type = (fcc->dispatch_mode == GEN6_WM_DW5_16_PIXEL_DISPATCH) ?
         GEN6_MSG_DP_RT_MODE_SIMD16 >> 8 :
         GEN6_MSG_DP_RT_MODE_SIMD8_LO >> 8;

      ctrl = (cbuf == num_cbufs - 1) << 12 |
             msg_type << 8;

      desc = tsrc_imm_mdesc_data_port(tc, cbuf == num_cbufs - 1,
            mrf - fcc->first_free_mrf, 0,
            header_present, false,
            GEN6_MSG_DP_RT_WRITE,
            ctrl, fcc->shader->bt.rt_base + cbuf);

      tc_add2(tc, TOY_OPCODE_FB_WRITE, tdst_null(),
            tsrc(TOY_FILE_MRF, fcc->first_free_mrf, 0), desc);
   }
}

/**
 * Set up shader outputs for fixed-function units.
 */
static void
fs_setup_shader_out(struct ilo_shader *sh, const struct toy_tgsi *tgsi)
{
   int i;

   sh->out.count = tgsi->num_outputs;
   for (i = 0; i < tgsi->num_outputs; i++) {
      sh->out.register_indices[i] = tgsi->outputs[i].index;
      sh->out.semantic_names[i] = tgsi->outputs[i].semantic_name;
      sh->out.semantic_indices[i] = tgsi->outputs[i].semantic_index;

      if (tgsi->outputs[i].semantic_name == TGSI_SEMANTIC_POSITION)
         sh->out.has_pos = true;
   }
}

/**
 * Set up shader inputs for fixed-function units.
 */
static void
fs_setup_shader_in(struct ilo_shader *sh, const struct toy_tgsi *tgsi,
                   bool flatshade)
{
   int i;

   sh->in.count = tgsi->num_inputs;
   for (i = 0; i < tgsi->num_inputs; i++) {
      sh->in.semantic_names[i] = tgsi->inputs[i].semantic_name;
      sh->in.semantic_indices[i] = tgsi->inputs[i].semantic_index;
      sh->in.interp[i] = tgsi->inputs[i].interp;
      sh->in.centroid[i] = tgsi->inputs[i].centroid;

      if (tgsi->inputs[i].semantic_name == TGSI_SEMANTIC_POSITION) {
         sh->in.has_pos = true;
         continue;
      }
      else if (tgsi->inputs[i].semantic_name == TGSI_SEMANTIC_FACE) {
         continue;
      }

      switch (tgsi->inputs[i].interp) {
      case TGSI_INTERPOLATE_CONSTANT:
         sh->in.const_interp_enable |= 1 << i;
         break;
      case TGSI_INTERPOLATE_LINEAR:
         sh->in.has_linear_interp = true;

         if (tgsi->inputs[i].centroid) {
            sh->in.barycentric_interpolation_mode |=
               GEN6_INTERP_NONPERSPECTIVE_CENTROID;
         }
         else {
            sh->in.barycentric_interpolation_mode |=
               GEN6_INTERP_NONPERSPECTIVE_PIXEL;
         }
         break;
      case TGSI_INTERPOLATE_COLOR:
         if (flatshade) {
            sh->in.const_interp_enable |= 1 << i;
            break;
         }
         /* fall through */
      case TGSI_INTERPOLATE_PERSPECTIVE:
         if (tgsi->inputs[i].centroid) {
            sh->in.barycentric_interpolation_mode |=
               GEN6_INTERP_PERSPECTIVE_CENTROID;
         }
         else {
            sh->in.barycentric_interpolation_mode |=
               GEN6_INTERP_PERSPECTIVE_PIXEL;
         }
         break;
      default:
         break;
      }
   }
}

static int
fs_setup_payloads(struct fs_compile_context *fcc)
{
   const struct ilo_shader *sh = fcc->shader;
   int grf, i;

   grf = 0;

   /* r0: header */
   grf++;

   /* r1-r2: coordinates and etc. */
   grf += (fcc->dispatch_mode == GEN6_WM_DW5_32_PIXEL_DISPATCH) ? 2 : 1;

   for (i = 0; i < Elements(fcc->payloads); i++) {
      const int reg_scale =
         (fcc->dispatch_mode == GEN6_WM_DW5_8_PIXEL_DISPATCH) ? 1 : 2;

      /* r3-r26 or r32-r55: barycentric interpolation parameters */
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_PERSPECTIVE_PIXEL)) {
         fcc->payloads[i].interp_perspective_pixel = grf;
         grf += 2 * reg_scale;
      }
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_PERSPECTIVE_CENTROID)) {
         fcc->payloads[i].interp_perspective_centroid = grf;
         grf += 2 * reg_scale;
      }
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_PERSPECTIVE_SAMPLE)) {
         fcc->payloads[i].interp_perspective_sample = grf;
         grf += 2 * reg_scale;
      }
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_NONPERSPECTIVE_PIXEL)) {
         fcc->payloads[i].interp_nonperspective_pixel = grf;
         grf += 2 * reg_scale;
      }
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_NONPERSPECTIVE_CENTROID)) {
         fcc->payloads[i].interp_nonperspective_centroid = grf;
         grf += 2 * reg_scale;
      }
      if (sh->in.barycentric_interpolation_mode &
            (GEN6_INTERP_NONPERSPECTIVE_SAMPLE)) {
         fcc->payloads[i].interp_nonperspective_sample = grf;
         grf += 2 * reg_scale;
      }

      /* r27-r28 or r56-r57: interpoloated depth */
      if (sh->in.has_pos) {
         fcc->payloads[i].source_depth = grf;
         grf += 1 * reg_scale;
      }

      /* r29-r30 or r58-r59: interpoloated w */
      if (sh->in.has_pos) {
         fcc->payloads[i].source_w = grf;
         grf += 1 * reg_scale;
      }

      /* r31 or r60: position offset */
      if (false) {
         fcc->payloads[i].pos_offset = grf;
         grf++;
      }

      if (fcc->dispatch_mode != GEN6_WM_DW5_32_PIXEL_DISPATCH)
         break;
   }

   return grf;
}

/**
 * Translate the TGSI tokens.
 */
static bool
fs_setup_tgsi(struct toy_compiler *tc, const struct tgsi_token *tokens,
              struct toy_tgsi *tgsi)
{
   if (ilo_debug & ILO_DEBUG_FS) {
      ilo_printf("dumping fragment shader\n");
      ilo_printf("\n");

      tgsi_dump(tokens, 0);
      ilo_printf("\n");
   }

   toy_compiler_translate_tgsi(tc, tokens, false, tgsi);
   if (tc->fail) {
      ilo_err("failed to translate FS TGSI tokens: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_FS) {
      ilo_printf("TGSI translator:\n");
      toy_tgsi_dump(tgsi);
      ilo_printf("\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Set up FS compile context.  This includes translating the TGSI tokens.
 */
static bool
fs_setup(struct fs_compile_context *fcc,
         const struct ilo_shader_state *state,
         const struct ilo_shader_variant *variant)
{
   int num_consts;

   memset(fcc, 0, sizeof(*fcc));

   fcc->shader = CALLOC_STRUCT(ilo_shader);
   if (!fcc->shader)
      return false;

   fcc->variant = variant;

   toy_compiler_init(&fcc->tc, state->info.dev);

   fcc->dispatch_mode = GEN6_WM_DW5_8_PIXEL_DISPATCH;

   fcc->tc.templ.access_mode = GEN6_ALIGN_1;
   if (fcc->dispatch_mode == GEN6_WM_DW5_16_PIXEL_DISPATCH) {
      fcc->tc.templ.qtr_ctrl = GEN6_QTRCTRL_1H;
      fcc->tc.templ.exec_size = GEN6_EXECSIZE_16;
   }
   else {
      fcc->tc.templ.qtr_ctrl = GEN6_QTRCTRL_1Q;
      fcc->tc.templ.exec_size = GEN6_EXECSIZE_8;
   }

   fcc->tc.rect_linear_width = 8;

   /*
    * The classic driver uses the sampler cache (gen6) or the data cache
    * (gen7).  Why?
    */
   fcc->const_cache = GEN6_SFID_DP_CC;

   if (!fs_setup_tgsi(&fcc->tc, state->info.tokens, &fcc->tgsi)) {
      toy_compiler_cleanup(&fcc->tc);
      FREE(fcc->shader);
      return false;
   }

   fs_setup_shader_in(fcc->shader, &fcc->tgsi, fcc->variant->u.fs.flatshade);
   fs_setup_shader_out(fcc->shader, &fcc->tgsi);

   if (fcc->variant->use_pcb && !fcc->tgsi.const_indirect) {
      num_consts = (fcc->tgsi.const_count + 1) / 2;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 287:
       *
       *     "The sum of all four read length fields (each incremented to
       *      represent the actual read length) must be less than or equal to
       *      64"
       *
       * Since we are usually under a high register pressure, do not allow
       * for more than 8.
       */
      if (num_consts > 8)
         num_consts = 0;
   }
   else {
      num_consts = 0;
   }

   fcc->shader->skip_cbuf0_upload = (!fcc->tgsi.const_count || num_consts);
   fcc->shader->pcb.cbuf0_size = num_consts * (sizeof(float) * 8);

   fcc->first_const_grf = fs_setup_payloads(fcc);
   fcc->first_attr_grf = fcc->first_const_grf + num_consts;
   fcc->first_free_grf = fcc->first_attr_grf + fcc->shader->in.count * 2;
   fcc->last_free_grf = 127;

   /* m0 is reserved for system routines */
   fcc->first_free_mrf = 1;
   fcc->last_free_mrf = 15;

   /* instructions are compressed with GEN6_EXECSIZE_16 */
   fcc->num_grf_per_vrf =
      (fcc->dispatch_mode == GEN6_WM_DW5_16_PIXEL_DISPATCH) ? 2 : 1;

   if (ilo_dev_gen(fcc->tc.dev) >= ILO_GEN(7)) {
      fcc->last_free_grf -= 15;
      fcc->first_free_mrf = fcc->last_free_grf + 1;
      fcc->last_free_mrf = fcc->first_free_mrf + 14;
   }

   fcc->shader->in.start_grf = fcc->first_const_grf;
   fcc->shader->has_kill = fcc->tgsi.uses_kill;
   fcc->shader->dispatch_16 =
      (fcc->dispatch_mode == GEN6_WM_DW5_16_PIXEL_DISPATCH);

   fcc->shader->bt.rt_base = 0;
   fcc->shader->bt.rt_count = fcc->variant->u.fs.num_cbufs;
   /* to send EOT */
   if (!fcc->shader->bt.rt_count)
      fcc->shader->bt.rt_count = 1;

   fcc->shader->bt.tex_base = fcc->shader->bt.rt_base +
                              fcc->shader->bt.rt_count;
   fcc->shader->bt.tex_count = fcc->variant->num_sampler_views;

   fcc->shader->bt.const_base = fcc->shader->bt.tex_base +
                                fcc->shader->bt.tex_count;
   fcc->shader->bt.const_count = state->info.constant_buffer_count;

   fcc->shader->bt.total_count = fcc->shader->bt.const_base +
                                 fcc->shader->bt.const_count;

   return true;
}

/**
 * Compile the fragment shader.
 */
struct ilo_shader *
ilo_shader_compile_fs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant)
{
   struct fs_compile_context fcc;

   if (!fs_setup(&fcc, state, variant))
      return NULL;

   fs_write_fb(&fcc);

   if (!fs_compile(&fcc)) {
      FREE(fcc.shader);
      fcc.shader = NULL;
   }

   toy_tgsi_cleanup(&fcc.tgsi);
   toy_compiler_cleanup(&fcc.tc);

   return fcc.shader;
}
