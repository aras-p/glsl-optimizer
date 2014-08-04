/*
 * Copyright © 2011 Intel Corporation
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

#include "brw_vec4.h"
#include "brw_cfg.h"

extern "C" {
#include "brw_eu.h"
#include "main/macros.h"
#include "program/prog_print.h"
#include "program/prog_parameter.h"
};

namespace brw {

gen8_vec4_generator::gen8_vec4_generator(struct brw_context *brw,
                                         struct gl_shader_program *shader_prog,
                                         struct gl_program *prog,
                                         struct brw_vec4_prog_data *prog_data,
                                         void *mem_ctx,
                                         bool debug_flag)
   : gen8_generator(brw, shader_prog, prog, mem_ctx),
     prog_data(prog_data),
     debug_flag(debug_flag)
{
}

gen8_vec4_generator::~gen8_vec4_generator()
{
}

void
gen8_vec4_generator::generate_tex(vec4_instruction *ir, struct brw_reg dst,
                                  struct brw_reg sampler_index)
{
   int msg_type = 0;

   switch (ir->opcode) {
   case SHADER_OPCODE_TEX:
   case SHADER_OPCODE_TXL:
      if (ir->shadow_compare) {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE;
      } else {
         msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD;
      }
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
   case SHADER_OPCODE_TXF_MCS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS;
      break;
   case SHADER_OPCODE_TXS:
      msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO;
      break;
   case SHADER_OPCODE_TG4:
      if (ir->shadow_compare) {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_C;
      } else {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4;
      }
      break;
   case SHADER_OPCODE_TG4_OFFSET:
      if (ir->shadow_compare) {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO_C;
      } else {
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO;
      }
      break;
   default:
      unreachable("should not get here: invalid VS texture opcode");
   }

   assert(sampler_index.file == BRW_IMMEDIATE_VALUE);
   assert(sampler_index.type == BRW_REGISTER_TYPE_UD);

   uint32_t sampler = sampler_index.dw1.ud;

   if (ir->header_present) {
      MOV_RAW(retype(brw_message_reg(ir->base_mrf), BRW_REGISTER_TYPE_UD),
              retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      default_state.access_mode = BRW_ALIGN_1;

      if (ir->texture_offset) {
         /* Set the offset bits in DWord 2. */
         MOV_RAW(retype(brw_vec1_reg(MRF, ir->base_mrf, 2),
                        BRW_REGISTER_TYPE_UD),
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
            ADD(get_element_ud(brw_message_reg(ir->base_mrf), 3),
                get_element_ud(brw_vec8_grf(0, 0), 3),
                brw_imm_ud(16 * (sampler / 16) * sampler_state_size));
         gen8_set_mask_control(add, BRW_MASK_DISABLE);
      }

      default_state.access_mode = BRW_ALIGN_16;
   }

   uint32_t surf_index =
      prog_data->base.binding_table.texture_start + sampler;

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, dst);
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));
   gen8_set_sampler_message(brw, inst,
                            surf_index,
                            sampler % 16,
                            msg_type,
                            1,
                            ir->mlen,
                            ir->header_present,
                            BRW_SAMPLER_SIMD_MODE_SIMD4X2);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

void
gen8_vec4_generator::generate_urb_write(vec4_instruction *ir, bool vs)
{
   struct brw_reg header = brw_vec8_grf(GEN7_MRF_HACK_START + ir->base_mrf, 0);

   /* Copy g0. */
   if (vs)
      MOV_RAW(header, brw_vec8_grf(0, 0));

   gen8_instruction *inst;
   if (!(ir->urb_write_flags & BRW_URB_WRITE_USE_CHANNEL_MASKS)) {
      /* Enable Channel Masks in the URB_WRITE_OWORD message header */
      default_state.access_mode = BRW_ALIGN_1;
      MOV_RAW(brw_vec1_grf(GEN7_MRF_HACK_START + ir->base_mrf, 5),
              brw_imm_ud(0xff00));
      default_state.access_mode = BRW_ALIGN_16;
   }

   inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_urb_message(brw, inst, ir->urb_write_flags, ir->mlen, 0, ir->offset,
                        true);
   gen8_set_dst(brw, inst, brw_null_reg());
   gen8_set_src0(brw, inst, header);
}

void
gen8_vec4_generator::generate_gs_set_vertex_count(struct brw_reg eot_mrf_header,
                                                  struct brw_reg src)
{
   /* Move the vertex count into the second MRF for the EOT write. */
   assert(eot_mrf_header.file == BRW_MESSAGE_REGISTER_FILE);
   int dst_nr = GEN7_MRF_HACK_START + eot_mrf_header.nr + 1;
   MOV(retype(brw_vec8_grf(dst_nr, 0), BRW_REGISTER_TYPE_UD), src);
}

void
gen8_vec4_generator::generate_gs_thread_end(vec4_instruction *ir)
{
   struct brw_reg src = brw_vec8_grf(GEN7_MRF_HACK_START + ir->base_mrf, 0);
   gen8_instruction *inst;

   /* Enable Channel Masks in the URB_WRITE_HWORD message header */
   default_state.access_mode = BRW_ALIGN_1;
   inst = OR(retype(brw_vec1_grf(GEN7_MRF_HACK_START + ir->base_mrf, 5),
                    BRW_REGISTER_TYPE_UD),
             retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
             brw_imm_ud(0xff00)); /* could be 0x1100 but shouldn't matter */
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);
   default_state.access_mode = BRW_ALIGN_16;

   /* mlen = 2: g0 header + vertex count */
   inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_urb_message(brw, inst, BRW_URB_WRITE_EOT, 2, 0, 0, true);
   gen8_set_dst(brw, inst, brw_null_reg());
   gen8_set_src0(brw, inst, src);
}

void
gen8_vec4_generator::generate_gs_set_write_offset(struct brw_reg dst,
                                                  struct brw_reg src0,
                                                  struct brw_reg src1)
{
   /* From p22 of volume 4 part 2 of the Ivy Bridge PRM (2.4.3.1 Message
    * Header: M0.3):
    *
    *     Slot 0 Offset. This field, after adding to the Global Offset field
    *     in the message descriptor, specifies the offset (in 256-bit units)
    *     from the start of the URB entry, as referenced by URB Handle 0, at
    *     which the data will be accessed.
    *
    * Similar text describes DWORD M0.4, which is slot 1 offset.
    *
    * Therefore, we want to multiply DWORDs 0 and 4 of src0 (the x components
    * of the register for geometry shader invocations 0 and 1) by the
    * immediate value in src1, and store the result in DWORDs 3 and 4 of dst.
    *
    * We can do this with the following EU instruction:
    *
    *     mul(2) dst.3<1>UD src0<8;2,4>UD src1   { Align1 WE_all }
    */
   default_state.access_mode = BRW_ALIGN_1;
   gen8_instruction *inst =
      MUL(suboffset(stride(dst, 2, 2, 1), 3), stride(src0, 8, 2, 4), src1);
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);
   default_state.access_mode = BRW_ALIGN_16;
}

void
gen8_vec4_generator::generate_gs_set_dword_2_immed(struct brw_reg dst,
                                                   struct brw_reg src)
{
   assert(src.file == BRW_IMMEDIATE_VALUE);

   default_state.access_mode = BRW_ALIGN_1;

   gen8_instruction *inst = MOV(suboffset(vec1(dst), 2), src);
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);

   default_state.access_mode = BRW_ALIGN_16;
}

void
gen8_vec4_generator::generate_gs_prepare_channel_masks(struct brw_reg dst)
{
   /* We want to left shift just DWORD 4 (the x component belonging to the
    * second geometry shader invocation) by 4 bits.  So generate the
    * instruction:
    *
    *     shl(1) dst.4<1>UD dst.4<0,1,0>UD 4UD { align1 WE_all }
    */
   dst = suboffset(vec1(dst), 4);
   default_state.access_mode = BRW_ALIGN_1;
   gen8_instruction *inst = SHL(dst, dst, brw_imm_ud(4));
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);
   default_state.access_mode = BRW_ALIGN_16;
}

void
gen8_vec4_generator::generate_gs_set_channel_masks(struct brw_reg dst,
                                                   struct brw_reg src)
{
   /* From p21 of volume 4 part 2 of the Ivy Bridge PRM (2.4.3.1 Message
    * Header: M0.5):
    *
    *     15 Vertex 1 DATA [3] / Vertex 0 DATA[7] Channel Mask
    *
    *        When Swizzle Control = URB_INTERLEAVED this bit controls Vertex 1
    *        DATA[3], when Swizzle Control = URB_NOSWIZZLE this bit controls
    *        Vertex 0 DATA[7].  This bit is ANDed with the corresponding
    *        channel enable to determine the final channel enable.  For the
    *        URB_READ_OWORD & URB_READ_HWORD messages, when final channel
    *        enable is 1 it indicates that Vertex 1 DATA [3] will be included
    *        in the writeback message.  For the URB_WRITE_OWORD &
    *        URB_WRITE_HWORD messages, when final channel enable is 1 it
    *        indicates that Vertex 1 DATA [3] will be written to the surface.
    *
    *        0: Vertex 1 DATA [3] / Vertex 0 DATA[7] channel not included
    *        1: Vertex DATA [3] / Vertex 0 DATA[7] channel included
    *
    *     14 Vertex 1 DATA [2] Channel Mask
    *     13 Vertex 1 DATA [1] Channel Mask
    *     12 Vertex 1 DATA [0] Channel Mask
    *     11 Vertex 0 DATA [3] Channel Mask
    *     10 Vertex 0 DATA [2] Channel Mask
    *      9 Vertex 0 DATA [1] Channel Mask
    *      8 Vertex 0 DATA [0] Channel Mask
    *
    * (This is from a section of the PRM that is agnostic to the particular
    * type of shader being executed, so "Vertex 0" and "Vertex 1" refer to
    * geometry shader invocations 0 and 1, respectively).  Since we have the
    * enable flags for geometry shader invocation 0 in bits 3:0 of DWORD 0,
    * and the enable flags for geometry shader invocation 1 in bits 7:0 of
    * DWORD 4, we just need to OR them together and store the result in bits
    * 15:8 of DWORD 5.
    *
    * It's easier to get the EU to do this if we think of the src and dst
    * registers as composed of 32 bytes each; then, we want to pick up the
    * contents of bytes 0 and 16 from src, OR them together, and store them in
    * byte 21.
    *
    * We can do that by the following EU instruction:
    *
    *     or(1) dst.21<1>UB src<0,1,0>UB src.16<0,1,0>UB { align1 WE_all }
    *
    * Note: this relies on the source register having zeros in (a) bits 7:4 of
    * DWORD 0 and (b) bits 3:0 of DWORD 4.  We can rely on (b) because the
    * source register was prepared by GS_OPCODE_PREPARE_CHANNEL_MASKS (which
    * shifts DWORD 4 left by 4 bits), and we can rely on (a) because prior to
    * the execution of GS_OPCODE_PREPARE_CHANNEL_MASKS, DWORDs 0 and 4 need to
    * contain valid channel mask values (which are in the range 0x0-0xf).
    */
   dst = retype(dst, BRW_REGISTER_TYPE_UB);
   src = retype(src, BRW_REGISTER_TYPE_UB);

   default_state.access_mode = BRW_ALIGN_1;

   gen8_instruction *inst =
      OR(suboffset(vec1(dst), 21), vec1(src), suboffset(vec1(src), 16));
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);

   default_state.access_mode = BRW_ALIGN_16;
}

void
gen8_vec4_generator::generate_oword_dual_block_offsets(struct brw_reg m1,
                                                       struct brw_reg index)
{
   int second_vertex_offset = 1;

   m1 = retype(m1, BRW_REGISTER_TYPE_D);

   /* Set up M1 (message payload).  Only the block offsets in M1.0 and
    * M1.4 are used, and the rest are ignored.
    */
   struct brw_reg m1_0 = suboffset(vec1(m1), 0);
   struct brw_reg m1_4 = suboffset(vec1(m1), 4);
   struct brw_reg index_0 = suboffset(vec1(index), 0);
   struct brw_reg index_4 = suboffset(vec1(index), 4);

   default_state.mask_control = BRW_MASK_DISABLE;
   default_state.access_mode = BRW_ALIGN_1;

   MOV(m1_0, index_0);

   if (index.file == BRW_IMMEDIATE_VALUE) {
      index_4.dw1.ud += second_vertex_offset;
      MOV(m1_4, index_4);
   } else {
      ADD(m1_4, index_4, brw_imm_d(second_vertex_offset));
   }

   default_state.mask_control = BRW_MASK_ENABLE;
   default_state.access_mode = BRW_ALIGN_16;
}

void
gen8_vec4_generator::generate_scratch_read(vec4_instruction *ir,
                                           struct brw_reg dst,
                                           struct brw_reg index)
{
   struct brw_reg header = brw_vec8_grf(GEN7_MRF_HACK_START + ir->base_mrf, 0);

   MOV_RAW(header, brw_vec8_grf(0, 0));

   generate_oword_dual_block_offsets(brw_message_reg(ir->base_mrf + 1), index);

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, header);
   gen8_set_dp_message(brw, send, GEN7_SFID_DATAPORT_DATA_CACHE,
                       255, /* binding table index: stateless access */
                       GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ,
                       BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
                       2,      /* mlen */
                       1,      /* rlen */
                       true,   /* header present */
                       false); /* EOT */
}

void
gen8_vec4_generator::generate_scratch_write(vec4_instruction *ir,
                                            struct brw_reg dst,
                                            struct brw_reg src,
                                            struct brw_reg index)
{
   struct brw_reg header = brw_vec8_grf(GEN7_MRF_HACK_START + ir->base_mrf, 0);

   MOV_RAW(header, brw_vec8_grf(0, 0));

   generate_oword_dual_block_offsets(brw_message_reg(ir->base_mrf + 1), index);

   MOV(retype(brw_message_reg(ir->base_mrf + 2), BRW_REGISTER_TYPE_D),
       retype(src, BRW_REGISTER_TYPE_D));

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, header);
   gen8_set_pred_control(send, ir->predicate);
   gen8_set_dp_message(brw, send, GEN7_SFID_DATAPORT_DATA_CACHE,
                       255, /* binding table index: stateless access */
                       GEN7_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE,
                       BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
                       3,      /* mlen */
                       0,      /* rlen */
                       true,   /* header present */
                       false); /* EOT */
}

void
gen8_vec4_generator::generate_pull_constant_load(vec4_instruction *inst,
                                                 struct brw_reg dst,
                                                 struct brw_reg index,
                                                 struct brw_reg offset)
{
   assert(index.file == BRW_IMMEDIATE_VALUE &&
          index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   assert(offset.file == BRW_GENERAL_REGISTER_FILE);

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   gen8_instruction *send = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, send, dst);
   gen8_set_src0(brw, send, offset);
   gen8_set_sampler_message(brw, send,
                            surf_index,
                            0, /* The LD message ignores the sampler unit. */
                            GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                            1,      /* rlen */
                            1,      /* mlen */
                            false,  /* no header */
                            BRW_SAMPLER_SIMD_MODE_SIMD4X2);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

void
gen8_vec4_generator::generate_untyped_atomic(vec4_instruction *ir,
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
      (1 << 5); /* Return data expected */

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, retype(dst, BRW_REGISTER_TYPE_UD));
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));
   gen8_set_dp_message(brw, inst, HSW_SFID_DATAPORT_DATA_CACHE_1,
                       surf_index.dw1.ud,
                       HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2,
                       msg_control,
                       ir->mlen,
                       1,
                       ir->header_present,
                       false);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}



void
gen8_vec4_generator::generate_untyped_surface_read(vec4_instruction *ir,
                                                   struct brw_reg dst,
                                                   struct brw_reg surf_index)
{
   assert(surf_index.file == BRW_IMMEDIATE_VALUE &&
          surf_index.type == BRW_REGISTER_TYPE_UD);

   gen8_instruction *inst = next_inst(BRW_OPCODE_SEND);
   gen8_set_dst(brw, inst, retype(dst, BRW_REGISTER_TYPE_UD));
   gen8_set_src0(brw, inst, brw_message_reg(ir->base_mrf));
   gen8_set_dp_message(brw, inst, HSW_SFID_DATAPORT_DATA_CACHE_1,
                       surf_index.dw1.ud,
                       HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ,
                       0xe, /* enable only the R channel */
                       ir->mlen,
                       1,
                       ir->header_present,
                       false);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}


void
gen8_vec4_generator::generate_vec4_instruction(vec4_instruction *instruction,
                                               struct brw_reg dst,
                                               struct brw_reg *src)
{
   vec4_instruction *ir = (vec4_instruction *) instruction;

   if (dst.width == BRW_WIDTH_4) {
      /* This happens in attribute fixups for "dual instanced" geometry
       * shaders, since they use attributes that are vec4's.  Since the exec
       * width is only 4, it's essential that the caller set
       * force_writemask_all in order to make sure the instruction is executed
       * regardless of which channels are enabled.
       */
      assert(ir->force_writemask_all);

      /* Fix up any <8;8,1> or <0;4,1> source registers to <4;4,1> to satisfy
       * the following register region restrictions (from Graphics BSpec:
       * 3D-Media-GPGPU Engine > EU Overview > Registers and Register Regions
       * > Register Region Restrictions)
       *
       *     1. ExecSize must be greater than or equal to Width.
       *
       *     2. If ExecSize = Width and HorzStride != 0, VertStride must be set
       *        to Width * HorzStride."
       */
      for (int i = 0; i < 3; i++) {
         if (src[i].file == BRW_GENERAL_REGISTER_FILE)
            src[i] = stride(src[i], 4, 4, 1);
      }
   }

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
      MAD(dst, src[0], src[1], src[2]);
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

   case BRW_OPCODE_CMP:
      CMP(dst, ir->conditional_mod, src[0], src[1]);
      break;

   case BRW_OPCODE_SEL:
      SEL(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DPH:
      DPH(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP4:
      DP4(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP3:
      DP3(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP2:
      DP2(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_F32TO16:
      /* Emulate the Gen7 zeroing bug. */
      MOV(retype(dst, BRW_REGISTER_TYPE_UD), brw_imm_ud(0u));
      MOV(retype(dst, BRW_REGISTER_TYPE_HF), src[0]);
      break;

   case BRW_OPCODE_F16TO32:
      MOV(dst, retype(src[0], BRW_REGISTER_TYPE_HF));
      break;

   case BRW_OPCODE_LRP:
      LRP(dst, src[0], src[1], src[2]);
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
      BFE(dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_BFI1:
      BFI1(dst, src[0], src[1]);
      break;

   case BRW_OPCODE_BFI2:
      BFI2(dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_IF:
      IF(ir->predicate);
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

   case SHADER_OPCODE_POW:
      MATH(BRW_MATH_FUNCTION_POW, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_INT_QUOTIENT:
      MATH(BRW_MATH_FUNCTION_INT_DIV_QUOTIENT, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_INT_REMAINDER:
      MATH(BRW_MATH_FUNCTION_INT_DIV_REMAINDER, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_TEX:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXF_CMS:
   case SHADER_OPCODE_TXF_MCS:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXS:
   case SHADER_OPCODE_TG4:
   case SHADER_OPCODE_TG4_OFFSET:
      /* note: src[0] is unused. */
      generate_tex(ir, dst, src[1]);
      break;

   case VS_OPCODE_URB_WRITE:
      generate_urb_write(ir, true);
      break;

   case SHADER_OPCODE_GEN4_SCRATCH_READ:
      generate_scratch_read(ir, dst, src[0]);
      break;

   case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
      generate_scratch_write(ir, dst, src[0], src[1]);
      break;

   case VS_OPCODE_PULL_CONSTANT_LOAD:
   case VS_OPCODE_PULL_CONSTANT_LOAD_GEN7:
      generate_pull_constant_load(ir, dst, src[0], src[1]);
      break;

   case GS_OPCODE_URB_WRITE:
      generate_urb_write(ir, false);
      break;

   case GS_OPCODE_THREAD_END:
      generate_gs_thread_end(ir);
      break;

   case GS_OPCODE_SET_WRITE_OFFSET:
      generate_gs_set_write_offset(dst, src[0], src[1]);
      break;

   case GS_OPCODE_SET_VERTEX_COUNT:
      generate_gs_set_vertex_count(dst, src[0]);
      break;

   case GS_OPCODE_SET_DWORD_2_IMMED:
      generate_gs_set_dword_2_immed(dst, src[0]);
      break;

   case GS_OPCODE_PREPARE_CHANNEL_MASKS:
      generate_gs_prepare_channel_masks(dst);
      break;

   case GS_OPCODE_SET_CHANNEL_MASKS:
      generate_gs_set_channel_masks(dst, src[0]);
      break;

   case SHADER_OPCODE_SHADER_TIME_ADD:
      unreachable("XXX: Missing Gen8 vec4 support for INTEL_DEBUG=shader_time");

   case SHADER_OPCODE_UNTYPED_ATOMIC:
      generate_untyped_atomic(ir, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_UNTYPED_SURFACE_READ:
      generate_untyped_surface_read(ir, dst, src[0]);
      break;

   case VS_OPCODE_UNPACK_FLAGS_SIMD4X2:
      unreachable("VS_OPCODE_UNPACK_FLAGS_SIMD4X2 should not be used on Gen8+.");

   default:
      if (ir->opcode < (int) ARRAY_SIZE(opcode_descs)) {
         _mesa_problem(ctx, "Unsupported opcode in `%s' in VS\n",
                       opcode_descs[ir->opcode].name);
      } else {
         _mesa_problem(ctx, "Unsupported opcode %d in VS", ir->opcode);
      }
      abort();
   }
}

void
gen8_vec4_generator::generate_code(exec_list *instructions)
{
   struct annotation_info annotation;
   memset(&annotation, 0, sizeof(annotation));

   cfg_t *cfg = NULL;
   if (unlikely(debug_flag))
      cfg = new(mem_ctx) cfg_t(instructions);

   foreach_in_list(vec4_instruction, ir, instructions) {
      struct brw_reg src[3], dst;

      if (unlikely(debug_flag))
         annotate(brw, &annotation, cfg, ir, next_inst_offset);

      for (unsigned int i = 0; i < 3; i++) {
         src[i] = ir->get_src(prog_data, i);
      }
      dst = ir->get_dst();

      default_state.conditional_mod = ir->conditional_mod;
      default_state.predicate = ir->predicate;
      default_state.predicate_inverse = ir->predicate_inverse;
      default_state.saturate = ir->saturate;

      const unsigned pre_emit_nr_inst = nr_inst;

      generate_vec4_instruction(ir, dst, src);

      if (ir->no_dd_clear || ir->no_dd_check) {
         assert(nr_inst == pre_emit_nr_inst + 1 ||
                !"no_dd_check or no_dd_clear set for IR emitting more "
                 "than 1 instruction");

         gen8_instruction *last = &store[pre_emit_nr_inst];
         gen8_set_no_dd_clear(last, ir->no_dd_clear);
         gen8_set_no_dd_check(last, ir->no_dd_check);
      }
   }

   patch_jump_targets();
   annotation_finalize(&annotation, next_inst_offset);

   int before_size = next_inst_offset;

   if (unlikely(debug_flag)) {
      if (shader_prog) {
         fprintf(stderr, "Native code for %s vertex shader %d:\n",
                 shader_prog->Label ? shader_prog->Label : "unnamed",
                 shader_prog->Name);
      } else {
         fprintf(stderr, "Native code for vertex program %d:\n", prog->Id);
      }
      fprintf(stderr, "vec4 shader: %d instructions.\n", before_size / 16);

      dump_assembly(store, annotation.ann_count, annotation.ann, brw, prog);
      ralloc_free(annotation.ann);
   }
}

const unsigned *
gen8_vec4_generator::generate_assembly(exec_list *instructions,
                                       unsigned *assembly_size)
{
   default_state.access_mode = BRW_ALIGN_16;
   default_state.exec_size = BRW_EXECUTE_8;
   generate_code(instructions);

   *assembly_size = next_inst_offset;
   return (const unsigned *) store;
}

} /* namespace brw */
