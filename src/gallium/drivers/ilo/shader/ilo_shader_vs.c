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
#include "ilo_context.h"
#include "ilo_shader_internal.h"

struct vs_compile_context {
   struct ilo_shader *shader;
   const struct ilo_shader_variant *variant;

   struct toy_compiler tc;
   struct toy_tgsi tgsi;
   int const_cache;

   int output_map[PIPE_MAX_SHADER_OUTPUTS];

   int num_grf_per_vrf;
   int first_const_grf;
   int first_ucp_grf;
   int first_vue_grf;
   int first_free_grf;
   int last_free_grf;

   int first_free_mrf;
   int last_free_mrf;
};

static void
vs_lower_opcode_tgsi_in(struct vs_compile_context *vcc,
                        struct toy_dst dst, int dim, int idx)
{
   struct toy_compiler *tc = &vcc->tc;
   int slot;

   assert(!dim);

   slot = toy_tgsi_find_input(&vcc->tgsi, idx);
   if (slot >= 0) {
      const int first_in_grf = vcc->first_vue_grf +
         (vcc->shader->in.count - vcc->tgsi.num_inputs);
      const int grf = first_in_grf + vcc->tgsi.inputs[slot].semantic_index;
      const struct toy_src src = tsrc(TOY_FILE_GRF, grf, 0);

      tc_MOV(tc, dst, src);
   }
   else {
      /* undeclared input */
      tc_MOV(tc, dst, tsrc_imm_f(0.0f));
   }
}

static bool
vs_lower_opcode_tgsi_const_pcb(struct vs_compile_context *vcc,
                               struct toy_dst dst, int dim,
                               struct toy_src idx)
{
   const int i = idx.val32;
   const int grf = vcc->first_const_grf + i / 2;
   const int grf_subreg = (i & 1) * 16;
   struct toy_src src;

   if (!vcc->variant->use_pcb || dim != 0 || idx.file != TOY_FILE_IMM ||
       grf >= vcc->first_ucp_grf)
      return false;


   src = tsrc_rect(tsrc(TOY_FILE_GRF, grf, grf_subreg), TOY_RECT_041);
   tc_MOV(&vcc->tc, dst, src);

   return true;
}

static void
vs_lower_opcode_tgsi_const_gen6(struct vs_compile_context *vcc,
                                struct toy_dst dst, int dim,
                                struct toy_src idx)
{
   const struct toy_dst header =
      tdst_ud(tdst(TOY_FILE_MRF, vcc->first_free_mrf, 0));
   const struct toy_dst block_offsets =
      tdst_ud(tdst(TOY_FILE_MRF, vcc->first_free_mrf + 1, 0));
   const struct toy_src r0 = tsrc_ud(tsrc(TOY_FILE_GRF, 0, 0));
   struct toy_compiler *tc = &vcc->tc;
   unsigned msg_type, msg_ctrl, msg_len;
   struct toy_inst *inst;
   struct toy_src desc;

   if (vs_lower_opcode_tgsi_const_pcb(vcc, dst, dim, idx))
      return;

   /* set message header */
   inst = tc_MOV(tc, header, r0);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   /* set block offsets */
   tc_MOV(tc, block_offsets, idx);

   msg_type = GEN6_MSG_DP_OWORD_DUAL_BLOCK_READ;
   msg_ctrl = GEN6_MSG_DP_OWORD_DUAL_BLOCK_SIZE_1;;
   msg_len = 2;

   desc = tsrc_imm_mdesc_data_port(tc, false, msg_len, 1, true, false,
         msg_type, msg_ctrl, ILO_VS_CONST_SURFACE(dim));

   tc_SEND(tc, dst, tsrc_from(header), desc, vcc->const_cache);
}

static void
vs_lower_opcode_tgsi_const_gen7(struct vs_compile_context *vcc,
                                struct toy_dst dst, int dim,
                                struct toy_src idx)
{
   struct toy_compiler *tc = &vcc->tc;
   const struct toy_dst offset =
      tdst_ud(tdst(TOY_FILE_MRF, vcc->first_free_mrf, 0));
   struct toy_src desc;

   if (vs_lower_opcode_tgsi_const_pcb(vcc, dst, dim, idx))
      return;

   /*
    * In 259b65e2e7938de4aab323033cfe2b33369ddb07, pull constant load was
    * changed from OWord Dual Block Read to ld to increase performance in the
    * classic driver.  Since we use the constant cache instead of the data
    * cache, I wonder if we still want to follow the classic driver.
    */

   /* set offset */
   tc_MOV(tc, offset, idx);

   desc = tsrc_imm_mdesc_sampler(tc, 1, 1, false,
         GEN6_MSG_SAMPLER_SIMD4X2,
         GEN6_MSG_SAMPLER_LD,
         0,
         ILO_VS_CONST_SURFACE(dim));

   tc_SEND(tc, dst, tsrc_from(offset), desc, GEN6_SFID_SAMPLER);
}

static void
vs_lower_opcode_tgsi_imm(struct vs_compile_context *vcc,
                         struct toy_dst dst, int idx)
{
   const uint32_t *imm;
   int ch;

   imm = toy_tgsi_get_imm(&vcc->tgsi, idx, NULL);

   for (ch = 0; ch < 4; ch++) {
      /* raw moves */
      tc_MOV(&vcc->tc,
            tdst_writemask(tdst_ud(dst), 1 << ch),
            tsrc_imm_ud(imm[ch]));
   }
}


static void
vs_lower_opcode_tgsi_sv(struct vs_compile_context *vcc,
                        struct toy_dst dst, int dim, int idx)
{
   struct toy_compiler *tc = &vcc->tc;
   const struct toy_tgsi *tgsi = &vcc->tgsi;
   int slot;

   assert(!dim);

   slot = toy_tgsi_find_system_value(tgsi, idx);
   if (slot < 0)
      return;

   switch (tgsi->system_values[slot].semantic_name) {
   case TGSI_SEMANTIC_INSTANCEID:
   case TGSI_SEMANTIC_VERTEXID:
      /*
       * In 3DSTATE_VERTEX_ELEMENTS, we prepend an extra vertex element for
       * the generated IDs, with VID in the X channel and IID in the Y
       * channel.
       */
      {
         const int grf = vcc->first_vue_grf;
         const struct toy_src src = tsrc(TOY_FILE_GRF, grf, 0);
         const enum toy_swizzle swizzle =
            (tgsi->system_values[slot].semantic_name ==
             TGSI_SEMANTIC_INSTANCEID) ? TOY_SWIZZLE_Y : TOY_SWIZZLE_X;

         tc_MOV(tc, tdst_d(dst), tsrc_d(tsrc_swizzle1(src, swizzle)));
      }
      break;
   case TGSI_SEMANTIC_PRIMID:
   default:
      tc_fail(tc, "unhandled system value");
      tc_MOV(tc, dst, tsrc_imm_d(0));
      break;
   }
}

static void
vs_lower_opcode_tgsi_direct(struct vs_compile_context *vcc,
                            struct toy_inst *inst)
{
   struct toy_compiler *tc = &vcc->tc;
   int dim, idx;

   assert(inst->src[0].file == TOY_FILE_IMM);
   dim = inst->src[0].val32;

   assert(inst->src[1].file == TOY_FILE_IMM);
   idx = inst->src[1].val32;

   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_IN:
      vs_lower_opcode_tgsi_in(vcc, inst->dst, dim, idx);
      break;
   case TOY_OPCODE_TGSI_CONST:
      if (tc->dev->gen >= ILO_GEN(7))
         vs_lower_opcode_tgsi_const_gen7(vcc, inst->dst, dim, inst->src[1]);
      else
         vs_lower_opcode_tgsi_const_gen6(vcc, inst->dst, dim, inst->src[1]);
      break;
   case TOY_OPCODE_TGSI_SV:
      vs_lower_opcode_tgsi_sv(vcc, inst->dst, dim, idx);
      break;
   case TOY_OPCODE_TGSI_IMM:
      assert(!dim);
      vs_lower_opcode_tgsi_imm(vcc, inst->dst, idx);
      break;
   default:
      tc_fail(tc, "unhandled TGSI fetch");
      break;
   }

   tc_discard_inst(tc, inst);
}

static void
vs_lower_opcode_tgsi_indirect(struct vs_compile_context *vcc,
                              struct toy_inst *inst)
{
   struct toy_compiler *tc = &vcc->tc;
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

         if (tc->dev->gen >= ILO_GEN(7))
            vs_lower_opcode_tgsi_const_gen7(vcc, inst->dst, dim, indirect_idx);
         else
            vs_lower_opcode_tgsi_const_gen6(vcc, inst->dst, dim, indirect_idx);
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
vs_add_sampler_params(struct toy_compiler *tc, int msg_type, int base_mrf,
                      struct toy_src coords, int num_coords,
                      struct toy_src bias_or_lod, struct toy_src ref_or_si,
                      struct toy_src ddx, struct toy_src ddy, int num_derivs)
{
   const unsigned coords_writemask = (1 << num_coords) - 1;
   struct toy_dst m[3];
   int num_params, i;

   assert(num_coords <= 4);
   assert(num_derivs <= 3 && num_derivs <= num_coords);

   for (i = 0; i < Elements(m); i++)
      m[i] = tdst(TOY_FILE_MRF, base_mrf + i, 0);

   switch (msg_type) {
   case GEN6_MSG_SAMPLER_SAMPLE_L:
      tc_MOV(tc, tdst_writemask(m[0], coords_writemask), coords);
      tc_MOV(tc, tdst_writemask(m[1], TOY_WRITEMASK_X), bias_or_lod);
      num_params = 5;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_D:
      tc_MOV(tc, tdst_writemask(m[0], coords_writemask), coords);
      tc_MOV(tc, tdst_writemask(m[1], TOY_WRITEMASK_XZ),
            tsrc_swizzle(ddx, 0, 0, 1, 1));
      tc_MOV(tc, tdst_writemask(m[1], TOY_WRITEMASK_YW),
            tsrc_swizzle(ddy, 0, 0, 1, 1));
      if (num_derivs > 2) {
         tc_MOV(tc, tdst_writemask(m[2], TOY_WRITEMASK_X),
               tsrc_swizzle1(ddx, 2));
         tc_MOV(tc, tdst_writemask(m[2], TOY_WRITEMASK_Y),
               tsrc_swizzle1(ddy, 2));
      }
      num_params = 4 + num_derivs * 2;
      break;
   case GEN6_MSG_SAMPLER_SAMPLE_L_C:
      tc_MOV(tc, tdst_writemask(m[0], coords_writemask), coords);
      tc_MOV(tc, tdst_writemask(m[1], TOY_WRITEMASK_X), ref_or_si);
      tc_MOV(tc, tdst_writemask(m[1], TOY_WRITEMASK_Y), bias_or_lod);
      num_params = 6;
      break;
   case GEN6_MSG_SAMPLER_LD:
      assert(num_coords <= 3);
      tc_MOV(tc, tdst_writemask(tdst_d(m[0]), coords_writemask), coords);
      tc_MOV(tc, tdst_writemask(tdst_d(m[0]), TOY_WRITEMASK_W), bias_or_lod);
      if (tc->dev->gen >= ILO_GEN(7)) {
         num_params = 4;
      }
      else {
         tc_MOV(tc, tdst_writemask(tdst_d(m[1]), TOY_WRITEMASK_X), ref_or_si);
         num_params = 5;
      }
      break;
   case GEN6_MSG_SAMPLER_RESINFO:
      tc_MOV(tc, tdst_writemask(tdst_d(m[0]), TOY_WRITEMASK_X), bias_or_lod);
      num_params = 1;
      break;
   default:
      tc_fail(tc, "unknown sampler opcode");
      num_params = 0;
      break;
   }

   return (num_params + 3) / 4;
}

/**
 * Set up message registers and return the message descriptor for sampling.
 */
static struct toy_src
vs_prepare_tgsi_sampling(struct toy_compiler *tc, const struct toy_inst *inst,
                         int base_mrf, unsigned *ret_sampler_index)
{
   unsigned simd_mode, msg_type, msg_len, sampler_index, binding_table_index;
   struct toy_src coords, ddx, ddy, bias_or_lod, ref_or_si;
   int num_coords, ref_pos, num_derivs;
   int sampler_src;

   simd_mode = GEN6_MSG_SAMPLER_SIMD4X2;

   coords = inst->src[0];
   ddx = tsrc_null();
   ddy = tsrc_null();
   bias_or_lod = tsrc_null();
   ref_or_si = tsrc_null();
   num_derivs = 0;
   sampler_src = 1;

   num_coords = tgsi_util_get_texture_coord_dim(inst->tex.target, &ref_pos);

   /* extract the parameters */
   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_TXD:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN7_MSG_SAMPLER_SAMPLE_D_C;
         ref_or_si = tsrc_swizzle1(coords, ref_pos);

         if (tc->dev->gen < ILO_GEN(7.5))
            tc_fail(tc, "TXD with shadow sampler not supported");
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_D;
      }

      ddx = inst->src[1];
      ddy = inst->src[2];
      num_derivs = num_coords;
      sampler_src = 3;
      break;
   case TOY_OPCODE_TGSI_TXL:
      if (ref_pos >= 0) {
         assert(ref_pos < 3);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L_C;
         ref_or_si = tsrc_swizzle1(coords, ref_pos);
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L;
      }

      bias_or_lod = tsrc_swizzle1(coords, TOY_SWIZZLE_W);
      break;
   case TOY_OPCODE_TGSI_TXF:
      msg_type = GEN6_MSG_SAMPLER_LD;

      switch (inst->tex.target) {
      case TGSI_TEXTURE_2D_MSAA:
      case TGSI_TEXTURE_2D_ARRAY_MSAA:
         assert(ref_pos >= 0 && ref_pos < 4);
         /* lod is always 0 */
         bias_or_lod = tsrc_imm_d(0);
         ref_or_si = tsrc_swizzle1(coords, ref_pos);
         break;
      default:
         bias_or_lod = tsrc_swizzle1(coords, TOY_SWIZZLE_W);
         break;
      }

      /* offset the coordinates */
      if (!tsrc_is_null(inst->tex.offsets[0])) {
         struct toy_dst tmp;

         tmp = tc_alloc_tmp(tc);
         tc_ADD(tc, tmp, coords, inst->tex.offsets[0]);
         coords = tsrc_from(tmp);
      }

      sampler_src = 1;
      break;
   case TOY_OPCODE_TGSI_TXQ:
      msg_type = GEN6_MSG_SAMPLER_RESINFO;
      num_coords = 0;
      bias_or_lod = tsrc_swizzle1(coords, TOY_SWIZZLE_X);
      break;
   case TOY_OPCODE_TGSI_TXQ_LZ:
      msg_type = GEN6_MSG_SAMPLER_RESINFO;
      num_coords = 0;
      sampler_src = 0;
      break;
   case TOY_OPCODE_TGSI_TXL2:
      if (ref_pos >= 0) {
         assert(ref_pos < 4);

         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L_C;
         ref_or_si = tsrc_swizzle1(coords, ref_pos);
      }
      else {
         msg_type = GEN6_MSG_SAMPLER_SAMPLE_L;
      }

      bias_or_lod = tsrc_swizzle1(inst->src[1], TOY_SWIZZLE_X);
      sampler_src = 2;
      break;
   default:
      assert(!"unhandled sampling opcode");
      if (ret_sampler_index)
         *ret_sampler_index = 0;
      return tsrc_null();
      break;
   }

   assert(inst->src[sampler_src].file == TOY_FILE_IMM);
   sampler_index = inst->src[sampler_src].val32;
   binding_table_index = ILO_VS_TEXTURE_SURFACE(sampler_index);

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
         struct toy_dst tmp, max;
         struct toy_src abs_coords[3];
         int i;

         tmp = tc_alloc_tmp(tc);
         max = tdst_writemask(tmp, TOY_WRITEMASK_W);

         for (i = 0; i < 3; i++)
            abs_coords[i] = tsrc_absolute(tsrc_swizzle1(coords, i));

         tc_SEL(tc, max, abs_coords[0], abs_coords[0], GEN6_COND_GE);
         tc_SEL(tc, max, tsrc_from(max), abs_coords[0], GEN6_COND_GE);
         tc_INV(tc, max, tsrc_from(max));

         for (i = 0; i < 3; i++)
            tc_MUL(tc, tdst_writemask(tmp, 1 << i), coords, tsrc_from(max));

         coords = tsrc_from(tmp);
      }
      break;
   }

   /* set up sampler parameters */
   msg_len = vs_add_sampler_params(tc, msg_type, base_mrf,
         coords, num_coords, bias_or_lod, ref_or_si, ddx, ddy, num_derivs);

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

   return tsrc_imm_mdesc_sampler(tc, msg_len, 1,
         false, simd_mode, msg_type, sampler_index, binding_table_index);
}

static void
vs_lower_opcode_tgsi_sampling(struct vs_compile_context *vcc,
                              struct toy_inst *inst)
{
   struct toy_compiler *tc = &vcc->tc;
   struct toy_src desc;
   struct toy_dst dst, tmp;
   unsigned sampler_index;
   int swizzles[4], i;
   unsigned swizzle_zero_mask, swizzle_one_mask, swizzle_normal_mask;
   bool need_filter;

   desc = vs_prepare_tgsi_sampling(tc, inst,
         vcc->first_free_mrf, &sampler_index);

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
   inst->src[0] = tsrc(TOY_FILE_MRF, vcc->first_free_mrf, 0);
   inst->src[1] = desc;

   /* write to a temp first */
   tmp = tc_alloc_tmp(tc);
   tmp.type = inst->dst.type;
   dst = inst->dst;
   inst->dst = tmp;

   tc_move_inst(tc, inst);

   if (need_filter) {
      assert(sampler_index < vcc->variant->num_sampler_views);
      swizzles[0] = vcc->variant->sampler_view_swizzles[sampler_index].r;
      swizzles[1] = vcc->variant->sampler_view_swizzles[sampler_index].g;
      swizzles[2] = vcc->variant->sampler_view_swizzles[sampler_index].b;
      swizzles[3] = vcc->variant->sampler_view_swizzles[sampler_index].a;
   }
   else {
      swizzles[0] = PIPE_SWIZZLE_RED;
      swizzles[1] = PIPE_SWIZZLE_GREEN;
      swizzles[2] = PIPE_SWIZZLE_BLUE;
      swizzles[3] = PIPE_SWIZZLE_ALPHA;
   }

   swizzle_zero_mask = 0;
   swizzle_one_mask = 0;
   swizzle_normal_mask = 0;
   for (i = 0; i < 4; i++) {
      switch (swizzles[i]) {
      case PIPE_SWIZZLE_ZERO:
         swizzle_zero_mask |= 1 << i;
         swizzles[i] = i;
         break;
      case PIPE_SWIZZLE_ONE:
         swizzle_one_mask |= 1 << i;
         swizzles[i] = i;
         break;
      default:
         swizzle_normal_mask |= 1 << i;
         break;
      }
   }

   /* swizzle the results */
   if (swizzle_normal_mask) {
      tc_MOV(tc, tdst_writemask(dst, swizzle_normal_mask),
            tsrc_swizzle(tsrc_from(tmp), swizzles[0],
               swizzles[1], swizzles[2], swizzles[3]));
   }
   if (swizzle_zero_mask)
      tc_MOV(tc, tdst_writemask(dst, swizzle_zero_mask), tsrc_imm_f(0.0f));
   if (swizzle_one_mask)
      tc_MOV(tc, tdst_writemask(dst, swizzle_one_mask), tsrc_imm_f(1.0f));
}

static void
vs_lower_opcode_urb_write(struct toy_compiler *tc, struct toy_inst *inst)
{
   /* vs_write_vue() has set up the message registers */
   toy_compiler_lower_to_send(tc, inst, false, GEN6_SFID_URB);
}

static void
vs_lower_virtual_opcodes(struct vs_compile_context *vcc)
{
   struct toy_compiler *tc = &vcc->tc;
   struct toy_inst *inst;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case TOY_OPCODE_TGSI_IN:
      case TOY_OPCODE_TGSI_CONST:
      case TOY_OPCODE_TGSI_SV:
      case TOY_OPCODE_TGSI_IMM:
         vs_lower_opcode_tgsi_direct(vcc, inst);
         break;
      case TOY_OPCODE_TGSI_INDIRECT_FETCH:
      case TOY_OPCODE_TGSI_INDIRECT_STORE:
         vs_lower_opcode_tgsi_indirect(vcc, inst);
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
         vs_lower_opcode_tgsi_sampling(vcc, inst);
         break;
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
      case TOY_OPCODE_URB_WRITE:
         vs_lower_opcode_urb_write(tc, inst);
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
vs_compile(struct vs_compile_context *vcc)
{
   struct toy_compiler *tc = &vcc->tc;
   struct ilo_shader *sh = vcc->shader;

   vs_lower_virtual_opcodes(vcc);
   toy_compiler_legalize_for_ra(tc);
   toy_compiler_optimize(tc);
   toy_compiler_allocate_registers(tc,
         vcc->first_free_grf,
         vcc->last_free_grf,
         vcc->num_grf_per_vrf);
   toy_compiler_legalize_for_asm(tc);

   if (tc->fail) {
      ilo_err("failed to legalize VS instructions: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_VS) {
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
      ilo_err("failed to compile VS: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_VS) {
      ilo_printf("disassembly:\n");
      toy_compiler_disassemble(tc, sh->kernel, sh->kernel_size);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Collect the toy registers to be written to the VUE.
 */
static int
vs_collect_outputs(struct vs_compile_context *vcc, struct toy_src *outs)
{
   const struct toy_tgsi *tgsi = &vcc->tgsi;
   int i;

   for (i = 0; i < vcc->shader->out.count; i++) {
      const int slot = vcc->output_map[i];
      const int vrf = (slot >= 0) ? toy_tgsi_get_vrf(tgsi,
            TGSI_FILE_OUTPUT, 0, tgsi->outputs[slot].index) : -1;
      struct toy_src src;

      if (vrf >= 0) {
         struct toy_dst dst;

         dst = tdst(TOY_FILE_VRF, vrf, 0);
         src = tsrc_from(dst);

         if (i == 0) {
            /* PSIZE is at channel W */
            tc_MOV(&vcc->tc, tdst_writemask(dst, TOY_WRITEMASK_W),
                  tsrc_swizzle1(src, TOY_SWIZZLE_X));

            /* the other channels are for the header */
            dst = tdst_d(dst);
            tc_MOV(&vcc->tc, tdst_writemask(dst, TOY_WRITEMASK_XYZ),
                  tsrc_imm_d(0));
         }
         else {
            /* initialize unused channels to 0.0f */
            if (tgsi->outputs[slot].undefined_mask) {
               dst = tdst_writemask(dst, tgsi->outputs[slot].undefined_mask);
               tc_MOV(&vcc->tc, dst, tsrc_imm_f(0.0f));
            }
         }
      }
      else {
         /* XXX this is too ugly */
         if (vcc->shader->out.semantic_names[i] == TGSI_SEMANTIC_CLIPDIST &&
             slot < 0) {
            /* ok, we need to compute clip distance */
            int clipvert_slot = -1, clipvert_vrf, j;

            for (j = 0; j < tgsi->num_outputs; j++) {
               if (tgsi->outputs[j].semantic_name ==
                     TGSI_SEMANTIC_CLIPVERTEX) {
                  clipvert_slot = j;
                  break;
               }
               else if (tgsi->outputs[j].semantic_name ==
                     TGSI_SEMANTIC_POSITION) {
                  /* remember pos, but keep looking */
                  clipvert_slot = j;
               }
            }

            clipvert_vrf = (clipvert_slot >= 0) ? toy_tgsi_get_vrf(tgsi,
                  TGSI_FILE_OUTPUT, 0, tgsi->outputs[clipvert_slot].index) : -1;
            if (clipvert_vrf >= 0) {
               struct toy_dst tmp = tc_alloc_tmp(&vcc->tc);
               struct toy_src clipvert = tsrc(TOY_FILE_VRF, clipvert_vrf, 0);
               int first_ucp, last_ucp;

               if (vcc->shader->out.semantic_indices[i]) {
                  first_ucp = 4;
                  last_ucp = MIN2(7, vcc->variant->u.vs.num_ucps - 1);
               }
               else {
                  first_ucp = 0;
                  last_ucp = MIN2(3, vcc->variant->u.vs.num_ucps - 1);
               }

               for (j = first_ucp; j <= last_ucp; j++) {
                  const int plane_grf = vcc->first_ucp_grf + j / 2;
                  const int plane_subreg = (j & 1) * 16;
                  const struct toy_src plane = tsrc_rect(tsrc(TOY_FILE_GRF,
                           plane_grf, plane_subreg), TOY_RECT_041);
                  const unsigned writemask = 1 << ((j >= 4) ? j - 4 : j);

                  tc_DP4(&vcc->tc, tdst_writemask(tmp, writemask),
                        clipvert, plane);
               }

               src = tsrc_from(tmp);
            }
            else {
               src = tsrc_imm_f(0.0f);
            }
         }
         else {
            src = (i == 0) ? tsrc_imm_d(0) : tsrc_imm_f(0.0f);
         }
      }

      outs[i] = src;
   }

   return i;
}

/**
 * Emit instructions to write the VUE.
 */
static void
vs_write_vue(struct vs_compile_context *vcc)
{
   struct toy_compiler *tc = &vcc->tc;
   struct toy_src outs[PIPE_MAX_SHADER_OUTPUTS];
   struct toy_dst header;
   struct toy_src r0;
   struct toy_inst *inst;
   int sent_attrs, total_attrs;

   header = tdst_ud(tdst(TOY_FILE_MRF, vcc->first_free_mrf, 0));
   r0 = tsrc_ud(tsrc(TOY_FILE_GRF, 0, 0));
   inst = tc_MOV(tc, header, r0);
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   if (tc->dev->gen >= ILO_GEN(7)) {
      inst = tc_OR(tc, tdst_offset(header, 0, 5),
            tsrc_rect(tsrc_offset(r0, 0, 5), TOY_RECT_010),
            tsrc_rect(tsrc_imm_ud(0xff00), TOY_RECT_010));
      inst->exec_size = GEN6_EXECSIZE_1;
      inst->access_mode = GEN6_ALIGN_1;
      inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
   }

   total_attrs = vs_collect_outputs(vcc, outs);
   sent_attrs = 0;
   while (sent_attrs < total_attrs) {
      struct toy_src desc;
      int mrf = vcc->first_free_mrf + 1, avail_mrf_for_attrs;
      int num_attrs, msg_len, i;
      bool eot;

      num_attrs = total_attrs - sent_attrs;
      eot = true;

      /* see if we need another message */
      avail_mrf_for_attrs = vcc->last_free_mrf - mrf + 1;
      if (num_attrs > avail_mrf_for_attrs) {
         /*
          * From the Sandy Bridge PRM, volume 4 part 2, page 22:
          *
          *     "Offset. This field specifies a destination offset (in 256-bit
          *      units) from the start of the URB entry(s), as referenced by
          *      URB Return Handle n, at which the data (if any) will be
          *      written."
          *
          * As we need to offset the following messages, we must make sure
          * this one writes an even number of attributes.
          */
         num_attrs = avail_mrf_for_attrs & ~1;
         eot = false;
      }

      if (tc->dev->gen >= ILO_GEN(7)) {
         /* do not forget about the header */
         msg_len = 1 + num_attrs;
      }
      else {
         /*
          * From the Sandy Bridge PRM, volume 4 part 2, page 26:
          *
          *     "At least 256 bits per vertex (512 bits total, M1 & M2) must
          *      be written.  Writing only 128 bits per vertex (256 bits
          *      total, M1 only) results in UNDEFINED operation."
          *
          *     "[DevSNB] Interleave writes must be in multiples of 256 per
          *      vertex."
          *
          * That is, we must write or appear to write an even number of
          * attributes, starting from two.
          */
         if (num_attrs % 2 && num_attrs == avail_mrf_for_attrs) {
            num_attrs--;
            eot = false;
         }

         msg_len = 1 + align(num_attrs, 2);
      }

      for (i = 0; i < num_attrs; i++)
         tc_MOV(tc, tdst(TOY_FILE_MRF, mrf++, 0), outs[sent_attrs + i]);

      assert(sent_attrs % 2 == 0);
      desc = tsrc_imm_mdesc_urb(tc, eot, msg_len, 0,
            eot, true, false, true, sent_attrs / 2, 0);

      tc_add2(tc, TOY_OPCODE_URB_WRITE, tdst_null(), tsrc_from(header), desc);

      sent_attrs += num_attrs;
   }
}

/**
 * Set up shader inputs for fixed-function units.
 */
static void
vs_setup_shader_in(struct ilo_shader *sh, const struct toy_tgsi *tgsi)
{
   int num_attrs, i;

   /* vertex/instance id is the first VE if exists */
   for (i = 0; i < tgsi->num_system_values; i++) {
      bool found = false;

      switch (tgsi->system_values[i].semantic_name) {
      case TGSI_SEMANTIC_INSTANCEID:
      case TGSI_SEMANTIC_VERTEXID:
         found = true;
         break;
      default:
         break;
      }

      if (found) {
         sh->in.semantic_names[sh->in.count] =
            tgsi->system_values[i].semantic_name;
         sh->in.semantic_indices[sh->in.count] =
            tgsi->system_values[i].semantic_index;
         sh->in.interp[sh->in.count] = TGSI_INTERPOLATE_CONSTANT;
         sh->in.centroid[sh->in.count] = false;

         sh->in.count++;
         break;
      }
   }

   num_attrs = 0;
   for (i = 0; i < tgsi->num_inputs; i++) {
      assert(tgsi->inputs[i].semantic_name == TGSI_SEMANTIC_GENERIC);
      if (tgsi->inputs[i].semantic_index >= num_attrs)
         num_attrs = tgsi->inputs[i].semantic_index + 1;
   }
   assert(num_attrs <= PIPE_MAX_ATTRIBS);

   /* VF cannot remap VEs.  VE[i] must be used as GENERIC[i]. */
   for (i = 0; i < num_attrs; i++) {
      sh->in.semantic_names[sh->in.count + i] = TGSI_SEMANTIC_GENERIC;
      sh->in.semantic_indices[sh->in.count + i] = i;
      sh->in.interp[sh->in.count + i] = TGSI_INTERPOLATE_CONSTANT;
      sh->in.centroid[sh->in.count + i] = false;
   }

   sh->in.count += num_attrs;

   sh->in.has_pos = false;
   sh->in.has_linear_interp = false;
   sh->in.barycentric_interpolation_mode = 0;
}

/**
 * Set up shader outputs for fixed-function units.
 */
static void
vs_setup_shader_out(struct ilo_shader *sh, const struct toy_tgsi *tgsi,
                    bool output_clipdist, int *output_map)
{
   int psize_slot = -1, pos_slot = -1;
   int clipdist_slot[2] = { -1, -1 };
   int color_slot[4] = { -1, -1, -1, -1 };
   int num_outs, i;

   /* find out the slots of outputs that need special care */
   for (i = 0; i < tgsi->num_outputs; i++) {
      switch (tgsi->outputs[i].semantic_name) {
      case TGSI_SEMANTIC_PSIZE:
         psize_slot = i;
         break;
      case TGSI_SEMANTIC_POSITION:
         pos_slot = i;
         break;
      case TGSI_SEMANTIC_CLIPDIST:
         if (tgsi->outputs[i].semantic_index)
            clipdist_slot[1] = i;
         else
            clipdist_slot[0] = i;
         break;
      case TGSI_SEMANTIC_COLOR:
         if (tgsi->outputs[i].semantic_index)
            color_slot[2] = i;
         else
            color_slot[0] = i;
         break;
      case TGSI_SEMANTIC_BCOLOR:
         if (tgsi->outputs[i].semantic_index)
            color_slot[3] = i;
         else
            color_slot[1] = i;
         break;
      default:
         break;
      }
   }

   /* the first two VUEs are always PSIZE and POSITION */
   num_outs = 2;
   output_map[0] = psize_slot;
   output_map[1] = pos_slot;

   sh->out.register_indices[0] =
      (psize_slot >= 0) ? tgsi->outputs[psize_slot].index : -1;
   sh->out.semantic_names[0] = TGSI_SEMANTIC_PSIZE;
   sh->out.semantic_indices[0] = 0;

   sh->out.register_indices[1] =
      (pos_slot >= 0) ? tgsi->outputs[pos_slot].index : -1;
   sh->out.semantic_names[1] = TGSI_SEMANTIC_POSITION;
   sh->out.semantic_indices[1] = 0;

   sh->out.has_pos = true;

   /* followed by optional clip distances */
   if (output_clipdist) {
      sh->out.register_indices[num_outs] =
         (clipdist_slot[0] >= 0) ? tgsi->outputs[clipdist_slot[0]].index : -1;
      sh->out.semantic_names[num_outs] = TGSI_SEMANTIC_CLIPDIST;
      sh->out.semantic_indices[num_outs] = 0;
      output_map[num_outs++] = clipdist_slot[0];

      sh->out.register_indices[num_outs] =
         (clipdist_slot[1] >= 0) ? tgsi->outputs[clipdist_slot[1]].index : -1;
      sh->out.semantic_names[num_outs] = TGSI_SEMANTIC_CLIPDIST;
      sh->out.semantic_indices[num_outs] = 1;
      output_map[num_outs++] = clipdist_slot[1];
   }

   /*
    * make BCOLOR follow COLOR so that we can make use of
    * ATTRIBUTE_SWIZZLE_INPUTATTR_FACING in 3DSTATE_SF
    */
   for (i = 0; i < 4; i++) {
      const int slot = color_slot[i];

      if (slot < 0)
         continue;

      sh->out.register_indices[num_outs] = tgsi->outputs[slot].index;
      sh->out.semantic_names[num_outs] = tgsi->outputs[slot].semantic_name;
      sh->out.semantic_indices[num_outs] = tgsi->outputs[slot].semantic_index;

      output_map[num_outs++] = slot;
   }

   /* add the rest of the outputs */
   for (i = 0; i < tgsi->num_outputs; i++) {
      switch (tgsi->outputs[i].semantic_name) {
      case TGSI_SEMANTIC_PSIZE:
      case TGSI_SEMANTIC_POSITION:
      case TGSI_SEMANTIC_CLIPDIST:
      case TGSI_SEMANTIC_COLOR:
      case TGSI_SEMANTIC_BCOLOR:
         break;
      default:
         sh->out.register_indices[num_outs] = tgsi->outputs[i].index;
         sh->out.semantic_names[num_outs] = tgsi->outputs[i].semantic_name;
         sh->out.semantic_indices[num_outs] = tgsi->outputs[i].semantic_index;
         output_map[num_outs++] = i;
         break;
      }
   }

   sh->out.count = num_outs;
}

/**
 * Translate the TGSI tokens.
 */
static bool
vs_setup_tgsi(struct toy_compiler *tc, const struct tgsi_token *tokens,
              struct toy_tgsi *tgsi)
{
   if (ilo_debug & ILO_DEBUG_VS) {
      ilo_printf("dumping vertex shader\n");
      ilo_printf("\n");

      tgsi_dump(tokens, 0);
      ilo_printf("\n");
   }

   toy_compiler_translate_tgsi(tc, tokens, true, tgsi);
   if (tc->fail) {
      ilo_err("failed to translate VS TGSI tokens: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_VS) {
      ilo_printf("TGSI translator:\n");
      toy_tgsi_dump(tgsi);
      ilo_printf("\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Set up VS compile context.  This includes translating the TGSI tokens.
 */
static bool
vs_setup(struct vs_compile_context *vcc,
         const struct ilo_shader_state *state,
         const struct ilo_shader_variant *variant)
{
   int num_consts;

   memset(vcc, 0, sizeof(*vcc));

   vcc->shader = CALLOC_STRUCT(ilo_shader);
   if (!vcc->shader)
      return false;

   vcc->variant = variant;

   toy_compiler_init(&vcc->tc, state->info.dev);
   vcc->tc.templ.access_mode = GEN6_ALIGN_16;
   vcc->tc.templ.exec_size = GEN6_EXECSIZE_8;
   vcc->tc.rect_linear_width = 4;

   /*
    * The classic driver uses the sampler cache (gen6) or the data cache
    * (gen7).  Why?
    */
   vcc->const_cache = GEN6_SFID_DP_CC;

   if (!vs_setup_tgsi(&vcc->tc, state->info.tokens, &vcc->tgsi)) {
      toy_compiler_cleanup(&vcc->tc);
      FREE(vcc->shader);
      return false;
   }

   vs_setup_shader_in(vcc->shader, &vcc->tgsi);
   vs_setup_shader_out(vcc->shader, &vcc->tgsi,
         (vcc->variant->u.vs.num_ucps > 0), vcc->output_map);

   if (vcc->variant->use_pcb && !vcc->tgsi.const_indirect) {
      num_consts = (vcc->tgsi.const_count + 1) / 2;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 138:
       *
       *     "The sum of all four read length fields (each incremented to
       *      represent the actual read length) must be less than or equal to
       *      32"
       */
      if (num_consts > 32)
         num_consts = 0;
   }
   else {
      num_consts = 0;
   }

   vcc->shader->skip_cbuf0_upload = (!vcc->tgsi.const_count || num_consts);
   vcc->shader->pcb.cbuf0_size = num_consts * (sizeof(float) * 8);

   /* r0 is reserved for payload header */
   vcc->first_const_grf = 1;
   vcc->first_ucp_grf = vcc->first_const_grf + num_consts;

   /* fit each pair of user clip planes into a register */
   vcc->first_vue_grf = vcc->first_ucp_grf +
      (vcc->variant->u.vs.num_ucps + 1) / 2;

   vcc->first_free_grf = vcc->first_vue_grf + vcc->shader->in.count;
   vcc->last_free_grf = 127;

   /* m0 is reserved for system routines */
   vcc->first_free_mrf = 1;
   vcc->last_free_mrf = 15;

   vcc->num_grf_per_vrf = 1;

   if (vcc->tc.dev->gen >= ILO_GEN(7)) {
      vcc->last_free_grf -= 15;
      vcc->first_free_mrf = vcc->last_free_grf + 1;
      vcc->last_free_mrf = vcc->first_free_mrf + 14;
   }

   vcc->shader->in.start_grf = vcc->first_const_grf;
   vcc->shader->pcb.clip_state_size =
      vcc->variant->u.vs.num_ucps * (sizeof(float) * 4);

   return true;
}

/**
 * Compile the vertex shader.
 */
struct ilo_shader *
ilo_shader_compile_vs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant)
{
   struct vs_compile_context vcc;
   bool need_gs;

   if (!vs_setup(&vcc, state, variant))
      return NULL;

   if (vcc.tc.dev->gen >= ILO_GEN(7)) {
      need_gs = false;
   }
   else {
      need_gs = variant->u.vs.rasterizer_discard ||
                state->info.stream_output.num_outputs;
   }

   vs_write_vue(&vcc);

   if (!vs_compile(&vcc)) {
      FREE(vcc.shader);
      vcc.shader = NULL;
   }

   toy_tgsi_cleanup(&vcc.tgsi);
   toy_compiler_cleanup(&vcc.tc);

   if (need_gs) {
      int so_mapping[PIPE_MAX_SHADER_OUTPUTS];
      int i, j;

      for (i = 0; i < vcc.tgsi.num_outputs; i++) {
         int attr = 0;

         for (j = 0; j < vcc.shader->out.count; j++) {
            if (vcc.tgsi.outputs[i].semantic_name ==
                  vcc.shader->out.semantic_names[j] &&
                vcc.tgsi.outputs[i].semantic_index ==
                  vcc.shader->out.semantic_indices[j]) {
               attr = j;
               break;
            }
         }

         so_mapping[i] = attr;
      }

      if (!ilo_shader_compile_gs_passthrough(state, variant,
               so_mapping, vcc.shader)) {
         ilo_shader_destroy_kernel(vcc.shader);
         vcc.shader = NULL;
      }
   }

   return vcc.shader;
}
