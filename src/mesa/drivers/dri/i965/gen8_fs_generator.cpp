/*
 * Copyright © 2010, 2011, 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file gen8_fs_generate.cpp
 *
 * Code generation for Gen8+ hardware.
 */

extern "C" {
#include "main/macros.h"
#include "brw_context.h"
} /* extern "C" */

#include "brw_fs.h"
#include "brw_cfg.h"
#include "glsl/ir_print_visitor.h"

gen8_fs_generator::gen8_fs_generator(struct brw_context *brw,
                                     void *mem_ctx,
                                     const struct brw_wm_prog_key *key,
                                     struct brw_wm_prog_data *prog_data,
                                     struct gl_shader_program *shader_prog,
                                     struct gl_fragment_program *fp,
                                     bool dual_source_output)
   : gen8_generator(brw, shader_prog, fp ? &fp->Base : NULL, mem_ctx),
     key(key), prog_data(prog_data),
     fp(fp), dual_source_output(dual_source_output)
{
}

gen8_fs_generator::~gen8_fs_generator()
{
}

void
gen8_fs_generator::generate_fb_write(fs_inst *ir)
{
   /* Disable the discard condition while setting up the header. */
   default_state.predicate = BRW_PREDICATE_NONE;
   default_state.predicate_inverse = false;
   default_state.flag_subreg_nr = 0;

   if (ir->header_present) {
      /* The GPU will use the predicate on SENDC, unless the header is present.
       */
      if (fp && fp->UsesKill) {
         gen8_instruction *mov =
            MOV(retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW),
                brw_flag_reg(0, 1));
         gen8_set_mask_control(mov, BRW_MASK_DISABLE);
      }

      gen8_instruction *mov =
         MOV_RAW(brw_message_reg(ir->base_mrf), brw_vec8_grf(0, 0));
      gen8_set_exec_size(mov, BRW_EXECUTE_16);

      if (ir->target > 0 && key->replicate_alpha) {
         /* Set "Source0 Alpha Present to RenderTarget" bit in the header. */
         gen8_instruction *inst =
            OR(get_element_ud(brw_message_reg(ir->base_mrf), 0),
               vec1(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD)),
               brw_imm_ud(1 << 11));
         gen8_set_mask_control(inst, BRW_MASK_DISABLE);
      }

      if (ir->target > 0) {
         /* Set the render target index for choosing BLEND_STATE. */
         MOV_RAW(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, ir->base_mrf, 2),
                 brw_imm_ud(ir->target));
      }
   }

   /* Set the predicate back to get the conditional write if necessary for
    * discards.
    */
   default_state.predicate = ir->predicate;
   default_state.predicate_inverse = ir->predicate_inverse;
   default_state.flag_subreg_nr = ir->flag_subreg;

   gen8_instruction *inst = next_inst(BRW_OPCODE_SENDC);
   gen8_set_dst(brw, inst, retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW));
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));

   /* Set up the "Message Specific Control" bits for the Data Port Message
    * Descriptor.  These are documented in the "Render Target Write" message's
    * "Message Descriptor" documentation (vol5c.2).
    */
   uint32_t msg_type;
   /* Set the Message Type */
   if (this->dual_source_output)
      msg_type = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN01;
   else if (dispatch_width == 16)
      msg_type = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE;
   else
      msg_type = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_SINGLE_SOURCE_SUBSPAN01;

   uint32_t msg_control = msg_type;

   /* Set "Last Render Target Select" on the final FB write. */
   if (ir->eot)
      msg_control |= (1 << 4); /* Last Render Target Select */

   uint32_t surf_index =
      prog_data->binding_table.render_target_start + ir->target;

   gen8_set_dp_message(brw, inst,
                       GEN6_SFID_DATAPORT_RENDER_CACHE,
                       surf_index,
                       GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE,
                       msg_control,
                       ir->mlen,
                       0,
                       ir->header_present,
                       ir->eot);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

void
gen8_fs_generator::generate_linterp(fs_inst *inst,
                                    struct brw_reg dst,
                                    struct brw_reg *src)
{
   struct brw_reg delta_x = src[0];
   struct brw_reg delta_y = src[1];
   struct brw_reg interp = src[2];

   (void) delta_y;
   assert(delta_y.nr == delta_x.nr + 1);
   PLN(dst, interp, delta_x);
}

void
gen8_fs_generator::generate_tex(fs_inst *ir,
                                struct brw_reg dst,
                                struct brw_reg src,
                                struct brw_reg sampler_index)
{
   int msg_type = -1;
   int rlen = 4;
   uint32_t simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD8;

   assert(src.file == BRW_GENERAL_REGISTER_FILE);

   if (dispatch_width == 16 && !ir->force_uncompressed && !ir->force_sechalf)
      simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;

   switch (ir->opcode) {
   case SHADER_OPCODE_TEX:
      if (ir->shadow_compare) {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_COMPARE;
      } else {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE;
      }
      break;
   case FS_OPCODE_TXB:
      if (ir->shadow_compare) {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE;
      } else {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS;
      }
      break;
   case SHADER_OPCODE_TXL:
      if (ir->shadow_compare) {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE;
      } else {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD;
      }
      break;
   case SHADER_OPCODE_TXS:
      msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO;
      break;
   case SHADER_OPCODE_TXD:
      if (ir->shadow_compare) {
         msg_type = HSW_SAMPLER_MESSAGE_SAMPLE_DERIV_COMPARE;
      } else {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_DERIVS;
      }
      break;
   case SHADER_OPCODE_TXF:
      msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
      break;
   case SHADER_OPCODE_TXF_CMS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS;
      break;
   case SHADER_OPCODE_TXF_UMS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DSS;
      break;
   case SHADER_OPCODE_TXF_MCS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS;
      break;
   case SHADER_OPCODE_LOD:
      msg_type = GEN5_SAMPLER_MESSAGE_LOD;
      break;
   case SHADER_OPCODE_TG4:
      if (ir->shadow_compare) {
         assert(brw->gen >= 7);
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_C;
      } else {
         assert(brw->gen >= 6);
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4;
      }
      break;
   case SHADER_OPCODE_TG4_OFFSET:
      assert(brw->gen >= 7);
      if (ir->shadow_compare) {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO_C;
      } else {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO;
      }
      break;
   default:
      unreachable("not reached");
   }
   assert(msg_type != -1);

   if (simd_mode == BRW_SAMPLER_SIMD_MODE_SIMD16) {
      rlen = 8;
      dst = vec16(dst);
   }

   assert(sampler_index.file == BRW_IMMEDIATE_VALUE);
   assert(sampler_index.type == BRW_REGISTER_TYPE_UD);

   uint32_t sampler = sampler_index.dw1.ud;

   if (ir->header_present) {
      /* The send-from-GRF for SIMD16 texturing with a header has an extra
       * hardware register allocated to it, which we need to skip over (since
       * our coordinates in the payload are in the even-numbered registers,
       * and the header comes right before the first one.
       */
      if (dispatch_width == 16)
         src.nr++;

      unsigned save_exec_size = default_state.exec_size;
      default_state.exec_size = BRW_EXECUTE_8;

      MOV_RAW(src, brw_vec8_grf(0, 0));

      if (ir->texture_offset) {
         /* Set the texel offset bits. */
         MOV_RAW(retype(brw_vec1_grf(src.nr, 2), BRW_REGISTER_TYPE_UD),
                 brw_imm_ud(ir->texture_offset));
      }

      if (sampler >= 16) {
         /* The "Sampler Index" field can only store values between 0 and 15.
          * However, we can add an offset to the "Sampler State Pointer"
          * field, effectively selecting a different set of 16 samplers.
          *
          * The "Sampler State Pointer" needs to be aligned to a 32-byte
          * offset, and each sampler state is only 16-bytes, so we can't
          * exclusively use the offset - we have to use both.
          */
         const int sampler_state_size = 16; /* 16 bytes */
         gen8_instruction *add =
            ADD(get_element_ud(src, 3),
                get_element_ud(brw_vec8_grf(0, 0), 3),
                brw_imm_ud(16 * (sampler / 16) * sampler_state_size));
         gen8_set_mask_control(add, BRW_MASK_DISABLE);
      }

      default_state.exec_size = save_exec_size;
   }

   uint32_t surf_index =
      prog_data->base.binding_table.texture_start + sampler;

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, dst);
   gen8_set_src0(brw, inst, src);
   gen8_set_sampler_message(brw, inst,
                            surf_index,
                            sampler % 16,
                            msg_type,
                            rlen,
                            ir->mlen,
                            ir->header_present,
                            simd_mode);

   brw_mark_surface_used(&prog_data->base, surf_index);
}


/* For OPCODE_DDX and OPCODE_DDY, per channel of output we've got input
 * looking like:
 *
 * arg0: ss0.tl ss0.tr ss0.bl ss0.br ss1.tl ss1.tr ss1.bl ss1.br
 *
 * and we're trying to produce:
 *
 *           DDX                     DDY
 * dst: (ss0.tr - ss0.tl)     (ss0.tl - ss0.bl)
 *      (ss0.tr - ss0.tl)     (ss0.tr - ss0.br)
 *      (ss0.br - ss0.bl)     (ss0.tl - ss0.bl)
 *      (ss0.br - ss0.bl)     (ss0.tr - ss0.br)
 *      (ss1.tr - ss1.tl)     (ss1.tl - ss1.bl)
 *      (ss1.tr - ss1.tl)     (ss1.tr - ss1.br)
 *      (ss1.br - ss1.bl)     (ss1.tl - ss1.bl)
 *      (ss1.br - ss1.bl)     (ss1.tr - ss1.br)
 *
 * and add another set of two more subspans if in 16-pixel dispatch mode.
 *
 * For DDX, it ends up being easy: width = 2, horiz=0 gets us the same result
 * for each pair, and vertstride = 2 jumps us 2 elements after processing a
 * pair. But for DDY, it's harder, as we want to produce the pairs swizzled
 * between each other.  We could probably do it like ddx and swizzle the right
 * order later, but bail for now and just produce
 * ((ss0.tl - ss0.bl)x4 (ss1.tl - ss1.bl)x4)
 */
void
gen8_fs_generator::generate_ddx(fs_inst *inst,
                                struct brw_reg dst,
                                struct brw_reg src)
{
   unsigned vstride, width;

   if (key->high_quality_derivatives) {
      /* Produce accurate derivatives. */
      vstride = BRW_VERTICAL_STRIDE_2;
      width = BRW_WIDTH_2;
   } else {
      /* Replicate the derivative at the top-left pixel to other pixels. */
      vstride = BRW_VERTICAL_STRIDE_4;
      width = BRW_WIDTH_4;
   }

   struct brw_reg src0 = brw_reg(src.file, src.nr, 1,
                                 BRW_REGISTER_TYPE_F,
                                 vstride,
                                 width,
                                 BRW_HORIZONTAL_STRIDE_0,
                                 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, 0,
                                 BRW_REGISTER_TYPE_F,
                                 vstride,
                                 width,
                                 BRW_HORIZONTAL_STRIDE_0,
                                 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   ADD(dst, src0, negate(src1));
}

/* The negate_value boolean is used to negate the derivative computation for
 * FBOs, since they place the origin at the upper left instead of the lower
 * left.
 */
void
gen8_fs_generator::generate_ddy(fs_inst *inst,
                                struct brw_reg dst,
                                struct brw_reg src,
                                bool negate_value)
{
   unsigned hstride;
   unsigned src0_swizzle;
   unsigned src1_swizzle;
   unsigned src1_subnr;

   if (key->high_quality_derivatives) {
      /* Produce accurate derivatives. */
      hstride = BRW_HORIZONTAL_STRIDE_1;
      src0_swizzle = BRW_SWIZZLE_XYXY;
      src1_swizzle = BRW_SWIZZLE_ZWZW;
      src1_subnr = 0;

      default_state.access_mode = BRW_ALIGN_16;
   } else {
      /* Replicate the derivative at the top-left pixel to other pixels. */
      hstride = BRW_HORIZONTAL_STRIDE_0;
      src0_swizzle = BRW_SWIZZLE_XYZW;
      src1_swizzle = BRW_SWIZZLE_XYZW;
      src1_subnr = 2;
   }

   struct brw_reg src0 = brw_reg(src.file, src.nr, 0,
                                 BRW_REGISTER_TYPE_F,
                                 BRW_VERTICAL_STRIDE_4,
                                 BRW_WIDTH_4,
                                 hstride,
                                 src0_swizzle, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, src1_subnr,
                                 BRW_REGISTER_TYPE_F,
                                 BRW_VERTICAL_STRIDE_4,
                                 BRW_WIDTH_4,
                                 hstride,
                                 src1_swizzle, WRITEMASK_XYZW);

   if (negate_value)
      ADD(dst, src1, negate(src0));
   else
      ADD(dst, src0, negate(src1));

   default_state.access_mode = BRW_ALIGN_1;
}

void
gen8_fs_generator::generate_scratch_write(fs_inst *ir, struct brw_reg src)
{
   MOV(retype(brw_message_reg(ir->base_mrf + 1), BRW_REGISTER_TYPE_UD),
       retype(src, BRW_REGISTER_TYPE_UD));

   struct brw_reg mrf =
      retype(brw_message_reg(ir->base_mrf), BRW_REGISTER_TYPE_UD);

   const int num_regs = dispatch_width / 8;

   uint32_t msg_control;
   if (num_regs == 1)
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
   else
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;

   /* Set up the message header.  This is g0, with g0.2 filled with
    * the offset.  We don't want to leave our offset around in g0 or
    * it'll screw up texture samples, so set it up inside the message
    * reg.
    */
   unsigned save_exec_size = default_state.exec_size;
   default_state.exec_size = BRW_EXECUTE_8;

   MOV_RAW(mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   /* set message header global offset field (reg 0, element 2) */
   MOV_RAW(get_element_ud(mrf, 2), brw_imm_ud(ir->offset / 16));

   struct brw_reg dst;
   if (dispatch_width == 16)
      dst = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
   else
      dst = retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW);

   default_state.exec_size = BRW_EXECUTE_16;

   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, mrf);
   gen8_set_dp_message(brw, send, GEN7_SFID_DATAPORT_DATA_CACHE,
                       255, /* binding table index: stateless access */
                       GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE,
                       msg_control,
                       1 + num_regs, /* mlen */
                       0,            /* rlen */
                       true,         /* header present */
                       false);       /* EOT */

   default_state.exec_size = save_exec_size;
}

void
gen8_fs_generator::generate_scratch_read(fs_inst *ir, struct brw_reg dst)
{
   struct brw_reg mrf =
      retype(brw_message_reg(ir->base_mrf), BRW_REGISTER_TYPE_UD);

   const int num_regs = dispatch_width / 8;

   uint32_t msg_control;
   if (num_regs == 1)
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
   else
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;

   unsigned save_exec_size = default_state.exec_size;
   default_state.exec_size = BRW_EXECUTE_8;

   MOV_RAW(mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   /* set message header global offset field (reg 0, element 2) */
   MOV_RAW(get_element_ud(mrf, 2), brw_imm_ud(ir->offset / 16));

   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, retype(dst, BRW_REGISTER_TYPE_UW));
   gen8_set_src0(brw, send, mrf);
   gen8_set_dp_message(brw, send, GEN7_SFID_DATAPORT_DATA_CACHE,
                       255, /* binding table index: stateless access */
                       BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ,
                       msg_control,
                       1,        /* mlen */
                       num_regs, /* rlen */
                       true,     /* header present */
                       false);   /* EOT */

   default_state.exec_size = save_exec_size;
}

void
gen8_fs_generator::generate_scratch_read_gen7(fs_inst *ir, struct brw_reg dst)
{
   unsigned save_exec_size = default_state.exec_size;
   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);

   int num_regs = dispatch_width / 8;

   /* According to the docs, offset is "A 12-bit HWord offset into the memory
    * Immediate Memory buffer as specified by binding table 0xFF."  An HWORD
    * is 32 bytes, which happens to be the size of a register.
    */
   int offset = ir->offset / REG_SIZE;

   /* The HW requires that the header is present; this is to get the g0.5
    * scratch offset.
    */
   gen8_set_src0(brw, send, brw_vec8_grf(0, 0));
   gen8_set_dst(brw, send, retype(dst, BRW_REGISTER_TYPE_UW));
   gen8_set_dp_scratch_message(brw, send,
                               false,    /* scratch read */
                               false,    /* OWords */
                               false,    /* invalidate after read */
                               num_regs,
                               offset,
                               1,        /* mlen - just g0 */
                               num_regs, /* rlen */
                               true,     /* header present */
                               false);   /* EOT */

   default_state.exec_size = save_exec_size;
}

void
gen8_fs_generator::generate_uniform_pull_constant_load(fs_inst *inst,
                                                       struct brw_reg dst,
                                                       struct brw_reg index,
                                                       struct brw_reg offset)
{
   assert(inst->mlen == 0);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
          index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   assert(offset.file == BRW_GENERAL_REGISTER_FILE);
   /* Reference only the dword we need lest we anger validate_reg() with
    * reg.width > reg.execszie.
    */
   offset = brw_vec1_grf(offset.nr, 0);

   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_mask_control(send, BRW_MASK_DISABLE);

   /* We use the SIMD4x2 mode because we want to end up with 4 constants in
    * the destination loaded consecutively from the same offset (which appears
    * in the first component, and the rest are ignored).
    */
   dst.width = BRW_WIDTH_4;
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, offset);
   gen8_set_sampler_message(brw, send,
                            surf_index,
                            0, /* The LD message ignores the sampler unit. */
                            GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                            1, /* rlen */
                            1, /* mlen */
                            false, /* no header */
                            BRW_SAMPLER_SIMD_MODE_SIMD4X2);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

void
gen8_fs_generator::generate_varying_pull_constant_load(fs_inst *ir,
                                                       struct brw_reg dst,
                                                       struct brw_reg index,
                                                       struct brw_reg offset)
{
   /* Varying-offset pull constant loads are treated as a normal expression on
    * gen7, so the fact that it's a send message is hidden at the IR level.
    */
   assert(!ir->header_present);
   assert(!ir->mlen);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
          index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   uint32_t simd_mode, rlen, mlen;
   if (dispatch_width == 16) {
      mlen = 2;
      rlen = 8;
      simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
   } else {
      mlen = 1;
      rlen = 4;
      simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD8;
   }

   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, offset);
   gen8_set_sampler_message(brw, send,
                            surf_index,
                            0, /* The LD message ignore the sampler unit. */
                            GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                            rlen, /* rlen */
                            mlen, /* mlen */
                            false, /* no header */
                            simd_mode);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

/**
 * Cause the current pixel/sample mask (from R1.7 bits 15:0) to be transferred
 * into the flags register (f0.0).
 */
void
gen8_fs_generator::generate_mov_dispatch_to_flags(fs_inst *ir)
{
   struct brw_reg flags = brw_flag_reg(0, ir->flag_subreg);
   struct brw_reg dispatch_mask =
      retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);

   gen8_instruction *mov = MOV(flags, dispatch_mask);
   gen8_set_mask_control(mov, BRW_MASK_DISABLE);
}

void
gen8_fs_generator::generate_discard_jump(fs_inst *ir)
{
   /* This HALT will be patched up at FB write time to point UIP at the end of
    * the program, and at brw_uip_jip() JIP will be set to the end of the
    * current block (or the program).
    */
   discard_halt_patches.push_tail(new(mem_ctx) ip_record(nr_inst));

   HALT();
}

bool
gen8_fs_generator::patch_discard_jumps_to_fb_writes()
{
   if (discard_halt_patches.is_empty())
      return false;

   /* There is a somewhat strange undocumented requirement of using
    * HALT, according to the simulator.  If some channel has HALTed to
    * a particular UIP, then by the end of the program, every channel
    * must have HALTed to that UIP.  Furthermore, the tracking is a
    * stack, so you can't do the final halt of a UIP after starting
    * halting to a new UIP.
    *
    * Symptoms of not emitting this instruction on actual hardware
    * included GPU hangs and sparkly rendering on the piglit discard
    * tests.
    */
   gen8_instruction *last_halt = HALT();
   gen8_set_uip(last_halt, 16);
   gen8_set_jip(last_halt, 16);

   int ip = nr_inst;

   foreach_in_list(ip_record, patch_ip, &discard_halt_patches) {
      gen8_instruction *patch = &store[patch_ip->ip];
      assert(gen8_opcode(patch) == BRW_OPCODE_HALT);

      /* HALT takes an instruction distance from the pre-incremented IP. */
      gen8_set_uip(patch, (ip - patch_ip->ip) * 16);
   }

   this->discard_halt_patches.make_empty();
   return true;
}

/**
 * Sets the first dword of a vgrf for simd4x2 uniform pull constant
 * sampler LD messages.
 *
 * We don't want to bake it into the send message's code generation because
 * that means we don't get a chance to schedule the instruction.
 */
void
gen8_fs_generator::generate_set_simd4x2_offset(fs_inst *ir,
                                               struct brw_reg dst,
                                               struct brw_reg value)
{
   assert(value.file == BRW_IMMEDIATE_VALUE);
   MOV_RAW(retype(brw_vec1_reg(dst.file, dst.nr, 0), value.type), value);
}

/**
 * Sets vstride=16, width=8, hstride=2 or vstride=0, width=1, hstride=0
 * (when mask is passed as a uniform) of register mask before moving it
 * to register dst.
 */
void
gen8_fs_generator::generate_set_omask(fs_inst *inst,
                                      struct brw_reg dst,
                                      struct brw_reg mask)
{
   assert(dst.type == BRW_REGISTER_TYPE_UW);

   if (dispatch_width == 16)
      dst = vec16(dst);

   if (mask.vstride == BRW_VERTICAL_STRIDE_8 &&
       mask.width == BRW_WIDTH_8 &&
       mask.hstride == BRW_HORIZONTAL_STRIDE_1) {
      mask = stride(mask, 16, 8, 2);
   } else {
      assert(mask.vstride == BRW_VERTICAL_STRIDE_0 &&
             mask.width == BRW_WIDTH_1 &&
             mask.hstride == BRW_HORIZONTAL_STRIDE_0);
   }

   gen8_instruction *mov = MOV(dst, retype(mask, dst.type));
   gen8_set_mask_control(mov, BRW_MASK_DISABLE);
}

/**
 * Do a special ADD with vstride=1, width=4, hstride=0 for src1.
 */
void
gen8_fs_generator::generate_set_sample_id(fs_inst *ir,
                                          struct brw_reg dst,
                                          struct brw_reg src0,
                                          struct brw_reg src1)
{
   assert(dst.type == BRW_REGISTER_TYPE_D || dst.type == BRW_REGISTER_TYPE_UD);
   assert(src0.type == BRW_REGISTER_TYPE_D || src0.type == BRW_REGISTER_TYPE_UD);

   struct brw_reg reg = retype(stride(src1, 1, 4, 0), BRW_REGISTER_TYPE_UW);

   unsigned save_exec_size = default_state.exec_size;
   default_state.exec_size = BRW_EXECUTE_8;

   gen8_instruction *add = ADD(dst, src0, reg);
   gen8_set_mask_control(add, BRW_MASK_DISABLE);
   if (dispatch_width == 16) {
      add = ADD(offset(dst, 1), offset(src0, 1), suboffset(reg, 2));
      gen8_set_mask_control(add, BRW_MASK_DISABLE);
   }

   default_state.exec_size = save_exec_size;
}

/**
 * Change the register's data type from UD to HF, doubling the strides in order
 * to compensate for halving the data type width.
 */
static struct brw_reg
ud_reg_to_hf(struct brw_reg r)
{
   assert(r.type == BRW_REGISTER_TYPE_UD);
   r.type = BRW_REGISTER_TYPE_HF;

   /* The BRW_*_STRIDE enums are defined so that incrementing the field
    * doubles the real stride.
    */
   if (r.hstride != 0)
      ++r.hstride;
   if (r.vstride != 0)
      ++r.vstride;

   return r;
}

void
gen8_fs_generator::generate_pack_half_2x16_split(fs_inst *inst,
                                                 struct brw_reg dst,
                                                 struct brw_reg x,
                                                 struct brw_reg y)
{
   assert(dst.type == BRW_REGISTER_TYPE_UD);
   assert(x.type == BRW_REGISTER_TYPE_F);
   assert(y.type == BRW_REGISTER_TYPE_F);

   struct brw_reg dst_hf = ud_reg_to_hf(dst);

   /* Give each 32-bit channel of dst the form below , where "." means
    * unchanged.
    *   0x....hhhh
    */
   MOV(dst_hf, y);

   /* Now the form:
    *   0xhhhh0000
    */
   SHL(dst, dst, brw_imm_ud(16u));

   /* And, finally the form of packHalf2x16's output:
    *   0xhhhhllll
    */
   MOV(dst_hf, x);
}

void
gen8_fs_generator::generate_unpack_half_2x16_split(fs_inst *inst,
                                                   struct brw_reg dst,
                                                   struct brw_reg src)
{
   assert(dst.type == BRW_REGISTER_TYPE_F);
   assert(src.type == BRW_REGISTER_TYPE_UD);

   struct brw_reg src_hf = ud_reg_to_hf(src);

   /* Each channel of src has the form of unpackHalf2x16's input: 0xhhhhllll.
    * For the Y case, we wish to access only the upper word; therefore
    * a 16-bit subregister offset is needed.
    */
   assert(inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_X ||
          inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y);
   if (inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y)
      src_hf.subnr += 2;

   MOV(dst, src_hf);
}

void
gen8_fs_generator::generate_untyped_atomic(fs_inst *ir,
                                           struct brw_reg dst,
                                           struct brw_reg atomic_op,
                                           struct brw_reg surf_index)
{
   assert(atomic_op.file == BRW_IMMEDIATE_VALUE &&
          atomic_op.type == BRW_REGISTER_TYPE_UD &&
          surf_index.file == BRW_IMMEDIATE_VALUE &&
          surf_index.type == BRW_REGISTER_TYPE_UD);
   assert((atomic_op.dw1.ud & ~0xf) == 0);

   unsigned msg_control =
      atomic_op.dw1.ud | /* Atomic Operation Type: BRW_AOP_* */
      ((dispatch_width == 16 ? 0 : 1) << 4) | /* SIMD Mode */
      (1 << 5); /* Return data expected */

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, retype(dst, BRW_REGISTER_TYPE_UD));
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));
   gen8_set_dp_message(brw, inst, HSW_SFID_DATAPORT_DATA_CACHE_1,
                       surf_index.dw1.ud,
                       HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP,
                       msg_control,
                       ir->mlen,
                       dispatch_width / 8,
                       ir->header_present,
                       false);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}

void
gen8_fs_generator::generate_untyped_surface_read(fs_inst *ir,
                                                 struct brw_reg dst,
                                                 struct brw_reg surf_index)
{
   assert(surf_index.file == BRW_IMMEDIATE_VALUE &&
          surf_index.type == BRW_REGISTER_TYPE_UD);

   unsigned msg_control = 0xe | /* Enable only the R channel */
     ((dispatch_width == 16 ? 1 : 2) << 4); /* SIMD Mode */

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, retype(dst, BRW_REGISTER_TYPE_UD));
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));
   gen8_set_dp_message(brw, inst, HSW_SFID_DATAPORT_DATA_CACHE_1,
                       surf_index.dw1.ud,
                       HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ,
                       msg_control,
                       ir->mlen,
                       dispatch_width / 8,
                       ir->header_present,
                       false);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}

void
gen8_fs_generator::generate_code(exec_list *instructions)
{
   int start_offset = next_inst_offset;

   struct annotation_info annotation;
   memset(&annotation, 0, sizeof(annotation));

   cfg_t *cfg = NULL;
   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      cfg = new(mem_ctx) cfg_t(instructions);

   foreach_in_list(fs_inst, ir, instructions) {
      struct brw_reg src[3], dst;

      if (unlikely(INTEL_DEBUG & DEBUG_WM))
         annotate(brw, &annotation, cfg, ir, next_inst_offset);

      for (unsigned int i = 0; i < 3; i++) {
         src[i] = brw_reg_from_fs_reg(&ir->src[i]);

         /* The accumulator result appears to get used for the
          * conditional modifier generation.  When negating a UD
          * value, there is a 33rd bit generated for the sign in the
          * accumulator value, so now you can't check, for example,
          * equality with a 32-bit value.  See piglit fs-op-neg-uvec4.
          */
         assert(!ir->conditional_mod ||
                ir->src[i].type != BRW_REGISTER_TYPE_UD ||
                !ir->src[i].negate);
      }
      dst = brw_reg_from_fs_reg(&ir->dst);

      default_state.conditional_mod = ir->conditional_mod;
      default_state.predicate = ir->predicate;
      default_state.predicate_inverse = ir->predicate_inverse;
      default_state.saturate = ir->saturate;
      default_state.mask_control = ir->force_writemask_all;
      default_state.flag_subreg_nr = ir->flag_subreg;

      if (dispatch_width == 16 && !ir->force_uncompressed && !ir->force_sechalf)
         default_state.exec_size = BRW_EXECUTE_16;
      else
         default_state.exec_size = BRW_EXECUTE_8;

      if (ir->force_uncompressed || dispatch_width == 8)
         default_state.qtr_control = GEN6_COMPRESSION_1Q;
      else if (ir->force_sechalf)
         default_state.qtr_control = GEN6_COMPRESSION_2Q;
      else
         default_state.qtr_control = GEN6_COMPRESSION_1H;

      switch (ir->opcode) {
      case BRW_OPCODE_MOV:
         MOV(dst, src[0]);
         break;
      case BRW_OPCODE_ADD:
         ADD(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_MUL:
         MUL(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_MACH:
         MACH(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_MAD:
         default_state.access_mode = BRW_ALIGN_16;
         MAD(dst, src[0], src[1], src[2]);
         default_state.access_mode = BRW_ALIGN_1;
         break;

      case BRW_OPCODE_LRP:
         default_state.access_mode = BRW_ALIGN_16;
         LRP(dst, src[0], src[1], src[2]);
         default_state.access_mode = BRW_ALIGN_1;
         break;


      case BRW_OPCODE_FRC:
         FRC(dst, src[0]);
         break;
      case BRW_OPCODE_RNDD:
         RNDD(dst, src[0]);
         break;
      case BRW_OPCODE_RNDE:
         RNDE(dst, src[0]);
         break;
      case BRW_OPCODE_RNDZ:
         RNDZ(dst, src[0]);
         break;

      case BRW_OPCODE_AND:
         AND(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_OR:
         OR(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_XOR:
         XOR(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_NOT:
         NOT(dst, src[0]);
         break;
      case BRW_OPCODE_ASR:
         ASR(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_SHR:
         SHR(dst, src[0], src[1]);
         break;
      case BRW_OPCODE_SHL:
         SHL(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_F32TO16:
         MOV(retype(dst, BRW_REGISTER_TYPE_HF), src[0]);
         break;
      case BRW_OPCODE_F16TO32:
         MOV(dst, retype(src[0], BRW_REGISTER_TYPE_HF));
         break;

      case BRW_OPCODE_CMP:
         CMP(dst, ir->conditional_mod, src[0], src[1]);
         break;
      case BRW_OPCODE_SEL:
         SEL(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_BFREV:
         /* BFREV only supports UD type for src and dst. */
         BFREV(retype(dst, BRW_REGISTER_TYPE_UD),
               retype(src[0], BRW_REGISTER_TYPE_UD));
         break;

      case BRW_OPCODE_FBH:
         /* FBH only supports UD type for dst. */
         FBH(retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
         break;

      case BRW_OPCODE_FBL:
         /* FBL only supports UD type for dst. */
         FBL(retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
         break;

      case BRW_OPCODE_CBIT:
         /* CBIT only supports UD type for dst. */
         CBIT(retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
         break;

      case BRW_OPCODE_ADDC:
         ADDC(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_SUBB:
         SUBB(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_BFE:
         default_state.access_mode = BRW_ALIGN_16;
         BFE(dst, src[0], src[1], src[2]);
         default_state.access_mode = BRW_ALIGN_1;
         break;

      case BRW_OPCODE_BFI1:
         BFI1(dst, src[0], src[1]);
         break;

      case BRW_OPCODE_BFI2:
         default_state.access_mode = BRW_ALIGN_16;
         BFI2(dst, src[0], src[1], src[2]);
         default_state.access_mode = BRW_ALIGN_1;
         break;

      case BRW_OPCODE_IF:
         IF(BRW_PREDICATE_NORMAL);
         break;

      case BRW_OPCODE_ELSE:
         ELSE();
         break;

      case BRW_OPCODE_ENDIF:
         ENDIF();
         break;

      case BRW_OPCODE_DO:
         DO();
         break;

      case BRW_OPCODE_BREAK:
         BREAK();
         break;

      case BRW_OPCODE_CONTINUE:
         CONTINUE();
         break;

      case BRW_OPCODE_WHILE:
         WHILE();
         break;

      case SHADER_OPCODE_RCP:
         MATH(BRW_MATH_FUNCTION_INV, dst, src[0]);
         break;

      case SHADER_OPCODE_RSQ:
         MATH(BRW_MATH_FUNCTION_RSQ, dst, src[0]);
         break;

      case SHADER_OPCODE_SQRT:
         MATH(BRW_MATH_FUNCTION_SQRT, dst, src[0]);
         break;

      case SHADER_OPCODE_EXP2:
         MATH(BRW_MATH_FUNCTION_EXP, dst, src[0]);
         break;

      case SHADER_OPCODE_LOG2:
         MATH(BRW_MATH_FUNCTION_LOG, dst, src[0]);
         break;

      case SHADER_OPCODE_SIN:
         MATH(BRW_MATH_FUNCTION_SIN, dst, src[0]);
         break;

      case SHADER_OPCODE_COS:
         MATH(BRW_MATH_FUNCTION_COS, dst, src[0]);
         break;

      case SHADER_OPCODE_INT_QUOTIENT:
         MATH(BRW_MATH_FUNCTION_INT_DIV_QUOTIENT, dst, src[0], src[1]);
         break;

      case SHADER_OPCODE_INT_REMAINDER:
         MATH(BRW_MATH_FUNCTION_INT_DIV_REMAINDER, dst, src[0], src[1]);
         break;

      case SHADER_OPCODE_POW:
         MATH(BRW_MATH_FUNCTION_POW, dst, src[0], src[1]);
         break;

      case FS_OPCODE_PIXEL_X:
      case FS_OPCODE_PIXEL_Y:
         unreachable("FS_OPCODE_PIXEL_X and FS_OPCODE_PIXEL_Y are only for Gen4-5.");

      case FS_OPCODE_CINTERP:
         MOV(dst, src[0]);
         break;
      case FS_OPCODE_LINTERP:
         generate_linterp(ir, dst, src);
         break;
      case SHADER_OPCODE_TEX:
      case FS_OPCODE_TXB:
      case SHADER_OPCODE_TXD:
      case SHADER_OPCODE_TXF:
      case SHADER_OPCODE_TXF_CMS:
      case SHADER_OPCODE_TXF_UMS:
      case SHADER_OPCODE_TXF_MCS:
      case SHADER_OPCODE_TXL:
      case SHADER_OPCODE_TXS:
      case SHADER_OPCODE_LOD:
      case SHADER_OPCODE_TG4:
      case SHADER_OPCODE_TG4_OFFSET:
         generate_tex(ir, dst, src[0], src[1]);
         break;

      case FS_OPCODE_DDX:
         generate_ddx(ir, dst, src[0]);
         break;
      case FS_OPCODE_DDY:
         /* Make sure fp->UsesDFdy flag got set (otherwise there's no
          * guarantee that key->render_to_fbo is set).
          */
         assert(fp->UsesDFdy);
         generate_ddy(ir, dst, src[0], key->render_to_fbo);
         break;

      case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
         generate_scratch_write(ir, src[0]);
         break;

      case SHADER_OPCODE_GEN4_SCRATCH_READ:
         generate_scratch_read(ir, dst);
         break;

      case SHADER_OPCODE_GEN7_SCRATCH_READ:
         generate_scratch_read_gen7(ir, dst);
         break;

      case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GEN7:
         generate_uniform_pull_constant_load(ir, dst, src[0], src[1]);
         break;

      case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7:
         generate_varying_pull_constant_load(ir, dst, src[0], src[1]);
         break;

      case FS_OPCODE_FB_WRITE:
         generate_fb_write(ir);
         break;

      case FS_OPCODE_MOV_DISPATCH_TO_FLAGS:
         generate_mov_dispatch_to_flags(ir);
         break;

      case FS_OPCODE_DISCARD_JUMP:
         generate_discard_jump(ir);
         break;

      case SHADER_OPCODE_SHADER_TIME_ADD:
         unreachable("XXX: Missing Gen8 scalar support for INTEL_DEBUG=shader_time");

      case SHADER_OPCODE_UNTYPED_ATOMIC:
         generate_untyped_atomic(ir, dst, src[0], src[1]);
         break;

      case SHADER_OPCODE_UNTYPED_SURFACE_READ:
         generate_untyped_surface_read(ir, dst, src[0]);
         break;

      case FS_OPCODE_SET_SIMD4X2_OFFSET:
         generate_set_simd4x2_offset(ir, dst, src[0]);
         break;

      case FS_OPCODE_SET_OMASK:
         generate_set_omask(ir, dst, src[0]);
         break;

      case FS_OPCODE_SET_SAMPLE_ID:
         generate_set_sample_id(ir, dst, src[0], src[1]);
         break;

      case FS_OPCODE_PACK_HALF_2x16_SPLIT:
         generate_pack_half_2x16_split(ir, dst, src[0], src[1]);
         break;

      case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_X:
      case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y:
         generate_unpack_half_2x16_split(ir, dst, src[0]);
         break;

      case FS_OPCODE_PLACEHOLDER_HALT:
         /* This is the place where the final HALT needs to be inserted if
          * we've emitted any discards.  If not, this will emit no code.
          */
         if (!patch_discard_jumps_to_fb_writes()) {
            if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
               annotation.ann_count--;
            }
         }
         break;

      default:
         if (ir->opcode < int(ARRAY_SIZE(opcode_descs))) {
            _mesa_problem(ctx, "Unsupported opcode `%s' in FS",
                          opcode_descs[ir->opcode].name);
         } else {
            _mesa_problem(ctx, "Unsupported opcode %d in FS", ir->opcode);
         }
         abort();
      }
   }

   patch_jump_targets();
   annotation_finalize(&annotation, next_inst_offset);

   int before_size = next_inst_offset - start_offset;

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      if (shader_prog) {
         fprintf(stderr,
                 "Native code for %s fragment shader %d (SIMD%d dispatch):\n",
                shader_prog->Label ? shader_prog->Label : "unnamed",
                shader_prog->Name, dispatch_width);
      } else if (fp) {
         fprintf(stderr,
                 "Native code for fragment program %d (SIMD%d dispatch):\n",
                 prog->Id, dispatch_width);
      } else {
         fprintf(stderr, "Native code for blorp program (SIMD%d dispatch):\n",
                 dispatch_width);
      }
      fprintf(stderr, "SIMD%d shader: %d instructions.\n",
              dispatch_width, before_size / 16);

      dump_assembly(store, annotation.ann_count, annotation.ann, brw, prog);
      ralloc_free(annotation.ann);
   }
}

const unsigned *
gen8_fs_generator::generate_assembly(exec_list *simd8_instructions,
                                     exec_list *simd16_instructions,
                                     unsigned *assembly_size)
{
   assert(simd8_instructions || simd16_instructions);

   if (simd8_instructions) {
      dispatch_width = 8;
      generate_code(simd8_instructions);
   }

   if (simd16_instructions) {
      /* Align to a 64-byte boundary. */
      while (next_inst_offset % 64)
         NOP();

      /* Save off the start of this SIMD16 program */
      prog_data->prog_offset_16 = next_inst_offset;

      dispatch_width = 16;
      generate_code(simd16_instructions);
   }

   *assembly_size = next_inst_offset;
   return (const unsigned *) store;
}
