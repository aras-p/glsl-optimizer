/* Copyright © 2011 Intel Corporation
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

extern "C" {
#include "brw_eu.h"
#include "main/macros.h"
#include "program/prog_print.h"
#include "program/prog_parameter.h"
};

namespace brw {

struct brw_reg
vec4_instruction::get_dst(void)
{
   struct brw_reg brw_reg;

   switch (dst.file) {
   case GRF:
      brw_reg = brw_vec8_grf(dst.reg + dst.reg_offset, 0);
      brw_reg = retype(brw_reg, dst.type);
      brw_reg.dw1.bits.writemask = dst.writemask;
      break;

   case MRF:
      brw_reg = brw_message_reg(dst.reg + dst.reg_offset);
      brw_reg = retype(brw_reg, dst.type);
      brw_reg.dw1.bits.writemask = dst.writemask;
      break;

   case HW_REG:
      assert(dst.type == dst.fixed_hw_reg.type);
      brw_reg = dst.fixed_hw_reg;
      break;

   case BAD_FILE:
      brw_reg = brw_null_reg();
      break;

   default:
      assert(!"not reached");
      brw_reg = brw_null_reg();
      break;
   }
   return brw_reg;
}

struct brw_reg
vec4_instruction::get_src(const struct brw_vec4_prog_data *prog_data, int i)
{
   struct brw_reg brw_reg;

   switch (src[i].file) {
   case GRF:
      brw_reg = brw_vec8_grf(src[i].reg + src[i].reg_offset, 0);
      brw_reg = retype(brw_reg, src[i].type);
      brw_reg.dw1.bits.swizzle = src[i].swizzle;
      if (src[i].abs)
	 brw_reg = brw_abs(brw_reg);
      if (src[i].negate)
	 brw_reg = negate(brw_reg);
      break;

   case IMM:
      switch (src[i].type) {
      case BRW_REGISTER_TYPE_F:
	 brw_reg = brw_imm_f(src[i].imm.f);
	 break;
      case BRW_REGISTER_TYPE_D:
	 brw_reg = brw_imm_d(src[i].imm.i);
	 break;
      case BRW_REGISTER_TYPE_UD:
	 brw_reg = brw_imm_ud(src[i].imm.u);
	 break;
      default:
	 assert(!"not reached");
	 brw_reg = brw_null_reg();
	 break;
      }
      break;

   case UNIFORM:
      brw_reg = stride(brw_vec4_grf(prog_data->dispatch_grf_start_reg +
                                    (src[i].reg + src[i].reg_offset) / 2,
				    ((src[i].reg + src[i].reg_offset) % 2) * 4),
		       0, 4, 1);
      brw_reg = retype(brw_reg, src[i].type);
      brw_reg.dw1.bits.swizzle = src[i].swizzle;
      if (src[i].abs)
	 brw_reg = brw_abs(brw_reg);
      if (src[i].negate)
	 brw_reg = negate(brw_reg);

      /* This should have been moved to pull constants. */
      assert(!src[i].reladdr);
      break;

   case HW_REG:
      assert(src[i].type == src[i].fixed_hw_reg.type);
      brw_reg = src[i].fixed_hw_reg;
      break;

   case BAD_FILE:
      /* Probably unused. */
      brw_reg = brw_null_reg();
      break;
   case ATTR:
   default:
      assert(!"not reached");
      brw_reg = brw_null_reg();
      break;
   }

   return brw_reg;
}

vec4_generator::vec4_generator(struct brw_context *brw,
                               struct gl_shader_program *shader_prog,
                               struct gl_program *prog,
                               struct brw_vec4_prog_data *prog_data,
                               void *mem_ctx,
                               bool debug_flag)
   : brw(brw), shader_prog(shader_prog), prog(prog), prog_data(prog_data),
     mem_ctx(mem_ctx), debug_flag(debug_flag)
{
   p = rzalloc(mem_ctx, struct brw_compile);
   brw_init_compile(brw, p, mem_ctx);
}

vec4_generator::~vec4_generator()
{
}

void
vec4_generator::generate_math1_gen4(vec4_instruction *inst,
                                    struct brw_reg dst,
                                    struct brw_reg src)
{
   brw_math(p,
	    dst,
	    brw_math_function(inst->opcode),
	    inst->base_mrf,
	    src,
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

static void
check_gen6_math_src_arg(struct brw_reg src)
{
   /* Source swizzles are ignored. */
   assert(!src.abs);
   assert(!src.negate);
   assert(src.dw1.bits.swizzle == BRW_SWIZZLE_XYZW);
}

void
vec4_generator::generate_math1_gen6(vec4_instruction *inst,
                                    struct brw_reg dst,
                                    struct brw_reg src)
{
   /* Can't do writemask because math can't be align16. */
   assert(dst.dw1.bits.writemask == WRITEMASK_XYZW);
   check_gen6_math_src_arg(src);

   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_math(p,
	    dst,
	    brw_math_function(inst->opcode),
	    inst->base_mrf,
	    src,
	    BRW_MATH_DATA_SCALAR,
	    BRW_MATH_PRECISION_FULL);
   brw_set_access_mode(p, BRW_ALIGN_16);
}

void
vec4_generator::generate_math2_gen7(vec4_instruction *inst,
                                    struct brw_reg dst,
                                    struct brw_reg src0,
                                    struct brw_reg src1)
{
   brw_math2(p,
	     dst,
	     brw_math_function(inst->opcode),
	     src0, src1);
}

void
vec4_generator::generate_math2_gen6(vec4_instruction *inst,
                                    struct brw_reg dst,
                                    struct brw_reg src0,
                                    struct brw_reg src1)
{
   /* Can't do writemask because math can't be align16. */
   assert(dst.dw1.bits.writemask == WRITEMASK_XYZW);
   /* Source swizzles are ignored. */
   check_gen6_math_src_arg(src0);
   check_gen6_math_src_arg(src1);

   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_math2(p,
	     dst,
	     brw_math_function(inst->opcode),
	     src0, src1);
   brw_set_access_mode(p, BRW_ALIGN_16);
}

void
vec4_generator::generate_math2_gen4(vec4_instruction *inst,
                                    struct brw_reg dst,
                                    struct brw_reg src0,
                                    struct brw_reg src1)
{
   /* From the Ironlake PRM, Volume 4, Part 1, Section 6.1.13
    * "Message Payload":
    *
    * "Operand0[7].  For the INT DIV functions, this operand is the
    *  denominator."
    *  ...
    * "Operand1[7].  For the INT DIV functions, this operand is the
    *  numerator."
    */
   bool is_int_div = inst->opcode != SHADER_OPCODE_POW;
   struct brw_reg &op0 = is_int_div ? src1 : src0;
   struct brw_reg &op1 = is_int_div ? src0 : src1;

   brw_push_insn_state(p);
   brw_set_saturate(p, false);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_MOV(p, retype(brw_message_reg(inst->base_mrf + 1), op1.type), op1);
   brw_pop_insn_state(p);

   brw_math(p,
	    dst,
	    brw_math_function(inst->opcode),
	    inst->base_mrf,
	    op0,
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

void
vec4_generator::generate_tex(vec4_instruction *inst,
                             struct brw_reg dst,
                             struct brw_reg src)
{
   int msg_type = -1;

   if (brw->gen >= 5) {
      switch (inst->opcode) {
      case SHADER_OPCODE_TEX:
      case SHADER_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE;
	 } else {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD;
	 }
	 break;
      case SHADER_OPCODE_TXD:
         if (inst->shadow_compare) {
            /* Gen7.5+.  Otherwise, lowered by brw_lower_texture_gradients(). */
            assert(brw->is_haswell);
            msg_type = HSW_SAMPLER_MESSAGE_SAMPLE_DERIV_COMPARE;
         } else {
            msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_DERIVS;
         }
	 break;
      case SHADER_OPCODE_TXF:
	 msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
	 break;
      case SHADER_OPCODE_TXF_CMS:
         if (brw->gen >= 7)
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS;
         else
            msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
         break;
      case SHADER_OPCODE_TXF_MCS:
         assert(brw->gen >= 7);
         msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS;
         break;
      case SHADER_OPCODE_TXS:
	 msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO;
	 break;
      case SHADER_OPCODE_TG4:
         if (inst->shadow_compare) {
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_C;
         } else {
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4;
         }
         break;
      case SHADER_OPCODE_TG4_OFFSET:
         if (inst->shadow_compare) {
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO_C;
         } else {
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO;
         }
         break;
      default:
	 assert(!"should not get here: invalid vec4 texture opcode");
	 break;
      }
   } else {
      switch (inst->opcode) {
      case SHADER_OPCODE_TEX:
      case SHADER_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_LOD_COMPARE;
	    assert(inst->mlen == 3);
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_LOD;
	    assert(inst->mlen == 2);
	 }
	 break;
      case SHADER_OPCODE_TXD:
	 /* There is no sample_d_c message; comparisons are done manually. */
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_GRADIENTS;
	 assert(inst->mlen == 4);
	 break;
      case SHADER_OPCODE_TXF:
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD4X2_LD;
	 assert(inst->mlen == 2);
	 break;
      case SHADER_OPCODE_TXS:
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD4X2_RESINFO;
	 assert(inst->mlen == 2);
	 break;
      default:
	 assert(!"should not get here: invalid vec4 texture opcode");
	 break;
      }
   }

   assert(msg_type != -1);

   /* Load the message header if present.  If there's a texture offset, we need
    * to set it up explicitly and load the offset bitfield.  Otherwise, we can
    * use an implied move from g0 to the first message register.
    */
   if (inst->header_present) {
      if (brw->gen < 6 && !inst->texture_offset) {
         /* Set up an implied move from g0 to the MRF. */
         src = brw_vec8_grf(0, 0);
      } else {
         struct brw_reg header =
            retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD);

         /* Explicitly set up the message header by copying g0 to the MRF. */
         brw_push_insn_state(p);
         brw_set_mask_control(p, BRW_MASK_DISABLE);
         brw_MOV(p, header, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

         brw_set_access_mode(p, BRW_ALIGN_1);

         if (inst->texture_offset) {
            /* Set the texel offset bits in DWord 2. */
            brw_MOV(p, get_element_ud(header, 2),
                    brw_imm_ud(inst->texture_offset));
         }

         if (inst->sampler >= 16) {
            /* The "Sampler Index" field can only store values between 0 and 15.
             * However, we can add an offset to the "Sampler State Pointer"
             * field, effectively selecting a different set of 16 samplers.
             *
             * The "Sampler State Pointer" needs to be aligned to a 32-byte
             * offset, and each sampler state is only 16-bytes, so we can't
             * exclusively use the offset - we have to use both.
             */
            assert(brw->is_haswell); /* field only exists on Haswell */
            brw_ADD(p,
                    get_element_ud(header, 3),
                    get_element_ud(brw_vec8_grf(0, 0), 3),
                    brw_imm_ud(16 * (inst->sampler / 16) *
                               sizeof(gen7_sampler_state)));
         }
         brw_pop_insn_state(p);
      }
   }

   uint32_t return_format;

   switch (dst.type) {
   case BRW_REGISTER_TYPE_D:
      return_format = BRW_SAMPLER_RETURN_FORMAT_SINT32;
      break;
   case BRW_REGISTER_TYPE_UD:
      return_format = BRW_SAMPLER_RETURN_FORMAT_UINT32;
      break;
   default:
      return_format = BRW_SAMPLER_RETURN_FORMAT_FLOAT32;
      break;
   }

   uint32_t surface_index = ((inst->opcode == SHADER_OPCODE_TG4 ||
      inst->opcode == SHADER_OPCODE_TG4_OFFSET)
      ? prog_data->base.binding_table.gather_texture_start
      : prog_data->base.binding_table.texture_start) + inst->sampler;

   brw_SAMPLE(p,
	      dst,
	      inst->base_mrf,
	      src,
              surface_index,
	      inst->sampler % 16,
	      msg_type,
	      1, /* response length */
	      inst->mlen,
	      inst->header_present,
	      BRW_SAMPLER_SIMD_MODE_SIMD4X2,
	      return_format);

   brw_mark_surface_used(&prog_data->base, surface_index);
}

void
vec4_generator::generate_vs_urb_write(vec4_instruction *inst)
{
   brw_urb_WRITE(p,
		 brw_null_reg(), /* dest */
		 inst->base_mrf, /* starting mrf reg nr */
		 brw_vec8_grf(0, 0), /* src */
                 inst->urb_write_flags,
		 inst->mlen,
		 0,		/* response len */
		 inst->offset,	/* urb destination offset */
		 BRW_URB_SWIZZLE_INTERLEAVE);
}

void
vec4_generator::generate_gs_urb_write(vec4_instruction *inst)
{
   struct brw_reg src = brw_message_reg(inst->base_mrf);
   brw_urb_WRITE(p,
                 brw_null_reg(), /* dest */
                 inst->base_mrf, /* starting mrf reg nr */
                 src,
                 inst->urb_write_flags,
                 inst->mlen,
                 0,             /* response len */
                 inst->offset,  /* urb destination offset */
                 BRW_URB_SWIZZLE_INTERLEAVE);
}

void
vec4_generator::generate_gs_thread_end(vec4_instruction *inst)
{
   struct brw_reg src = brw_message_reg(inst->base_mrf);
   brw_urb_WRITE(p,
                 brw_null_reg(), /* dest */
                 inst->base_mrf, /* starting mrf reg nr */
                 src,
                 BRW_URB_WRITE_EOT,
                 1,              /* message len */
                 0,              /* response len */
                 0,              /* urb destination offset */
                 BRW_URB_SWIZZLE_INTERLEAVE);
}

void
vec4_generator::generate_gs_set_write_offset(struct brw_reg dst,
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
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MUL(p, suboffset(stride(dst, 2, 2, 1), 3), stride(src0, 8, 2, 4),
           src1);
   brw_set_access_mode(p, BRW_ALIGN_16);
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_gs_set_vertex_count(struct brw_reg dst,
                                             struct brw_reg src)
{
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_mask_control(p, BRW_MASK_DISABLE);

   /* If we think of the src and dst registers as composed of 8 DWORDs each,
    * we want to pick up the contents of DWORDs 0 and 4 from src, truncate
    * them to WORDs, and then pack them into DWORD 2 of dst.
    *
    * It's easier to get the EU to do this if we think of the src and dst
    * registers as composed of 16 WORDS each; then, we want to pick up the
    * contents of WORDs 0 and 8 from src, and pack them into WORDs 4 and 5 of
    * dst.
    *
    * We can do that by the following EU instruction:
    *
    *     mov (2) dst.4<1>:uw src<8;1,0>:uw   { Align1, Q1, NoMask }
    */
   brw_MOV(p, suboffset(stride(retype(dst, BRW_REGISTER_TYPE_UW), 2, 2, 1), 4),
           stride(retype(src, BRW_REGISTER_TYPE_UW), 8, 1, 0));
   brw_set_access_mode(p, BRW_ALIGN_16);
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_gs_set_dword_2_immed(struct brw_reg dst,
                                              struct brw_reg src)
{
   assert(src.file == BRW_IMMEDIATE_VALUE);

   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MOV(p, suboffset(vec1(dst), 2), src);
   brw_set_access_mode(p, BRW_ALIGN_16);
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_gs_prepare_channel_masks(struct brw_reg dst)
{
   /* We want to left shift just DWORD 4 (the x component belonging to the
    * second geometry shader invocation) by 4 bits.  So generate the
    * instruction:
    *
    *     shl(1) dst.4<1>UD dst.4<0,1,0>UD 4UD { align1 WE_all }
    */
   dst = suboffset(vec1(dst), 4);
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_SHL(p, dst, dst, brw_imm_ud(4));
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_gs_set_channel_masks(struct brw_reg dst,
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
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_OR(p, suboffset(vec1(dst), 21), vec1(src), suboffset(vec1(src), 16));
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_gs_get_instance_id(struct brw_reg dst)
{
   /* We want to right shift R0.0 & R0.1 by GEN7_GS_PAYLOAD_INSTANCE_ID_SHIFT
    * and store into dst.0 & dst.4. So generate the instruction:
    *
    *     shr(8) dst<1> R0<1,4,0> GEN7_GS_PAYLOAD_INSTANCE_ID_SHIFT { align1 WE_normal 1Q }
    */
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   dst = retype(dst, BRW_REGISTER_TYPE_UD);
   struct brw_reg r0(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   brw_SHR(p, dst, stride(r0, 1, 4, 0),
           brw_imm_ud(GEN7_GS_PAYLOAD_INSTANCE_ID_SHIFT));
   brw_pop_insn_state(p);
}

void
vec4_generator::generate_oword_dual_block_offsets(struct brw_reg m1,
                                                  struct brw_reg index)
{
   int second_vertex_offset;

   if (brw->gen >= 6)
      second_vertex_offset = 1;
   else
      second_vertex_offset = 16;

   m1 = retype(m1, BRW_REGISTER_TYPE_D);

   /* Set up M1 (message payload).  Only the block offsets in M1.0 and
    * M1.4 are used, and the rest are ignored.
    */
   struct brw_reg m1_0 = suboffset(vec1(m1), 0);
   struct brw_reg m1_4 = suboffset(vec1(m1), 4);
   struct brw_reg index_0 = suboffset(vec1(index), 0);
   struct brw_reg index_4 = suboffset(vec1(index), 4);

   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_access_mode(p, BRW_ALIGN_1);

   brw_MOV(p, m1_0, index_0);

   if (index.file == BRW_IMMEDIATE_VALUE) {
      index_4.dw1.ud += second_vertex_offset;
      brw_MOV(p, m1_4, index_4);
   } else {
      brw_ADD(p, m1_4, index_4, brw_imm_d(second_vertex_offset));
   }

   brw_pop_insn_state(p);
}

void
vec4_generator::generate_unpack_flags(vec4_instruction *inst,
                                      struct brw_reg dst)
{
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_access_mode(p, BRW_ALIGN_1);

   struct brw_reg flags = brw_flag_reg(0, 0);
   struct brw_reg dst_0 = suboffset(vec1(dst), 0);
   struct brw_reg dst_4 = suboffset(vec1(dst), 4);

   brw_AND(p, dst_0, flags, brw_imm_ud(0x0f));
   brw_AND(p, dst_4, flags, brw_imm_ud(0xf0));
   brw_SHR(p, dst_4, dst_4, brw_imm_ud(4));

   brw_pop_insn_state(p);
}

void
vec4_generator::generate_scratch_read(vec4_instruction *inst,
                                      struct brw_reg dst,
                                      struct brw_reg index)
{
   struct brw_reg header = brw_vec8_grf(0, 0);

   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   generate_oword_dual_block_offsets(brw_message_reg(inst->base_mrf + 1),
				     index);

   uint32_t msg_type;

   if (brw->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (brw->gen == 5 || brw->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (brw->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_read_message(p, send,
			   255, /* binding table index: stateless access */
			   BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			   msg_type,
			   BRW_DATAPORT_READ_TARGET_RENDER_CACHE,
			   2, /* mlen */
                           true, /* header_present */
			   1 /* rlen */);
}

void
vec4_generator::generate_scratch_write(vec4_instruction *inst,
                                       struct brw_reg dst,
                                       struct brw_reg src,
                                       struct brw_reg index)
{
   struct brw_reg header = brw_vec8_grf(0, 0);
   bool write_commit;

   /* If the instruction is predicated, we'll predicate the send, not
    * the header setup.
    */
   brw_set_predicate_control(p, false);

   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   generate_oword_dual_block_offsets(brw_message_reg(inst->base_mrf + 1),
				     index);

   brw_MOV(p,
	   retype(brw_message_reg(inst->base_mrf + 2), BRW_REGISTER_TYPE_D),
	   retype(src, BRW_REGISTER_TYPE_D));

   uint32_t msg_type;

   if (brw->gen >= 7)
      msg_type = GEN7_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;
   else if (brw->gen == 6)
      msg_type = GEN6_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;
   else
      msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;

   brw_set_predicate_control(p, inst->predicate);

   /* Pre-gen6, we have to specify write commits to ensure ordering
    * between reads and writes within a thread.  Afterwards, that's
    * guaranteed and write commits only matter for inter-thread
    * synchronization.
    */
   if (brw->gen >= 6) {
      write_commit = false;
   } else {
      /* The visitor set up our destination register to be g0.  This
       * means that when the next read comes along, we will end up
       * reading from g0 and causing a block on the write commit.  For
       * write-after-read, we are relying on the value of the previous
       * read being used (and thus blocking on completion) before our
       * write is executed.  This means we have to be careful in
       * instruction scheduling to not violate this assumption.
       */
      write_commit = true;
   }

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (brw->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_write_message(p, send,
			    255, /* binding table index: stateless access */
			    BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			    msg_type,
			    3, /* mlen */
			    true, /* header present */
			    false, /* not a render target write */
			    write_commit, /* rlen */
			    false, /* eot */
			    write_commit);
}

void
vec4_generator::generate_pull_constant_load(vec4_instruction *inst,
                                            struct brw_reg dst,
                                            struct brw_reg index,
                                            struct brw_reg offset)
{
   assert(brw->gen <= 7);
   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   struct brw_reg header = brw_vec8_grf(0, 0);

   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   brw_MOV(p, retype(brw_message_reg(inst->base_mrf + 1), BRW_REGISTER_TYPE_D),
	   offset);

   uint32_t msg_type;

   if (brw->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (brw->gen == 5 || brw->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (brw->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_read_message(p, send,
			   surf_index,
			   BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			   msg_type,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   2, /* mlen */
                           true, /* header_present */
			   1 /* rlen */);

   brw_mark_surface_used(&prog_data->base, surf_index);
}

void
vec4_generator::generate_pull_constant_load_gen7(vec4_instruction *inst,
                                                 struct brw_reg dst,
                                                 struct brw_reg surf_index,
                                                 struct brw_reg offset)
{
   assert(surf_index.file == BRW_IMMEDIATE_VALUE &&
	  surf_index.type == BRW_REGISTER_TYPE_UD);

   brw_instruction *insn = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dst);
   brw_set_src0(p, insn, offset);
   brw_set_sampler_message(p, insn,
                           surf_index.dw1.ud,
                           0, /* LD message ignores sampler unit */
                           GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                           1, /* rlen */
                           1, /* mlen */
                           false, /* no header */
                           BRW_SAMPLER_SIMD_MODE_SIMD4X2,
                           0);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}

void
vec4_generator::generate_untyped_atomic(vec4_instruction *inst,
                                        struct brw_reg dst,
                                        struct brw_reg atomic_op,
                                        struct brw_reg surf_index)
{
   assert(atomic_op.file == BRW_IMMEDIATE_VALUE &&
          atomic_op.type == BRW_REGISTER_TYPE_UD &&
          surf_index.file == BRW_IMMEDIATE_VALUE &&
	  surf_index.type == BRW_REGISTER_TYPE_UD);

   brw_untyped_atomic(p, dst, brw_message_reg(inst->base_mrf),
                      atomic_op.dw1.ud, surf_index.dw1.ud,
                      inst->mlen, 1);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}

void
vec4_generator::generate_untyped_surface_read(vec4_instruction *inst,
                                              struct brw_reg dst,
                                              struct brw_reg surf_index)
{
   assert(surf_index.file == BRW_IMMEDIATE_VALUE &&
	  surf_index.type == BRW_REGISTER_TYPE_UD);

   brw_untyped_surface_read(p, dst, brw_message_reg(inst->base_mrf),
                            surf_index.dw1.ud,
                            inst->mlen, 1);

   brw_mark_surface_used(&prog_data->base, surf_index.dw1.ud);
}

/**
 * Generate assembly for a Vec4 IR instruction.
 *
 * \param instruction The Vec4 IR instruction to generate code for.
 * \param dst         The destination register.
 * \param src         An array of up to three source registers.
 */
void
vec4_generator::generate_vec4_instruction(vec4_instruction *instruction,
                                          struct brw_reg dst,
                                          struct brw_reg *src)
{
   vec4_instruction *inst = (vec4_instruction *) instruction;

   if (dst.width == BRW_WIDTH_4) {
      /* This happens in attribute fixups for "dual instanced" geometry
       * shaders, since they use attributes that are vec4's.  Since the exec
       * width is only 4, it's essential that the caller set
       * force_writemask_all in order to make sure the instruction is executed
       * regardless of which channels are enabled.
       */
      assert(inst->force_writemask_all);

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

   switch (inst->opcode) {
   case BRW_OPCODE_MOV:
      brw_MOV(p, dst, src[0]);
      break;
   case BRW_OPCODE_ADD:
      brw_ADD(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_MUL:
      brw_MUL(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_MACH:
      brw_MACH(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_MAD:
      assert(brw->gen >= 6);
      brw_MAD(p, dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_FRC:
      brw_FRC(p, dst, src[0]);
      break;
   case BRW_OPCODE_RNDD:
      brw_RNDD(p, dst, src[0]);
      break;
   case BRW_OPCODE_RNDE:
      brw_RNDE(p, dst, src[0]);
      break;
   case BRW_OPCODE_RNDZ:
      brw_RNDZ(p, dst, src[0]);
      break;

   case BRW_OPCODE_AND:
      brw_AND(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_OR:
      brw_OR(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_XOR:
      brw_XOR(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_NOT:
      brw_NOT(p, dst, src[0]);
      break;
   case BRW_OPCODE_ASR:
      brw_ASR(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_SHR:
      brw_SHR(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_SHL:
      brw_SHL(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_CMP:
      brw_CMP(p, dst, inst->conditional_mod, src[0], src[1]);
      break;
   case BRW_OPCODE_SEL:
      brw_SEL(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DPH:
      brw_DPH(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP4:
      brw_DP4(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP3:
      brw_DP3(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_DP2:
      brw_DP2(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_F32TO16:
      assert(brw->gen >= 7);
      brw_F32TO16(p, dst, src[0]);
      break;

   case BRW_OPCODE_F16TO32:
      assert(brw->gen >= 7);
      brw_F16TO32(p, dst, src[0]);
      break;

   case BRW_OPCODE_LRP:
      assert(brw->gen >= 6);
      brw_LRP(p, dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_BFREV:
      assert(brw->gen >= 7);
      /* BFREV only supports UD type for src and dst. */
      brw_BFREV(p, retype(dst, BRW_REGISTER_TYPE_UD),
                   retype(src[0], BRW_REGISTER_TYPE_UD));
      break;
   case BRW_OPCODE_FBH:
      assert(brw->gen >= 7);
      /* FBH only supports UD type for dst. */
      brw_FBH(p, retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
      break;
   case BRW_OPCODE_FBL:
      assert(brw->gen >= 7);
      /* FBL only supports UD type for dst. */
      brw_FBL(p, retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
      break;
   case BRW_OPCODE_CBIT:
      assert(brw->gen >= 7);
      /* CBIT only supports UD type for dst. */
      brw_CBIT(p, retype(dst, BRW_REGISTER_TYPE_UD), src[0]);
      break;
   case BRW_OPCODE_ADDC:
      assert(brw->gen >= 7);
      brw_ADDC(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_SUBB:
      assert(brw->gen >= 7);
      brw_SUBB(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_MAC:
      brw_MAC(p, dst, src[0], src[1]);
      break;

   case BRW_OPCODE_BFE:
      assert(brw->gen >= 7);
      brw_BFE(p, dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_BFI1:
      assert(brw->gen >= 7);
      brw_BFI1(p, dst, src[0], src[1]);
      break;
   case BRW_OPCODE_BFI2:
      assert(brw->gen >= 7);
      brw_BFI2(p, dst, src[0], src[1], src[2]);
      break;

   case BRW_OPCODE_IF:
      if (inst->src[0].file != BAD_FILE) {
         /* The instruction has an embedded compare (only allowed on gen6) */
         assert(brw->gen == 6);
         gen6_IF(p, inst->conditional_mod, src[0], src[1]);
      } else {
         struct brw_instruction *brw_inst = brw_IF(p, BRW_EXECUTE_8);
         brw_inst->header.predicate_control = inst->predicate;
      }
      break;

   case BRW_OPCODE_ELSE:
      brw_ELSE(p);
      break;
   case BRW_OPCODE_ENDIF:
      brw_ENDIF(p);
      break;

   case BRW_OPCODE_DO:
      brw_DO(p, BRW_EXECUTE_8);
      break;

   case BRW_OPCODE_BREAK:
      brw_BREAK(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case BRW_OPCODE_CONTINUE:
      /* FINISHME: We need to write the loop instruction support still. */
      if (brw->gen >= 6)
         gen6_CONT(p);
      else
         brw_CONT(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;

   case BRW_OPCODE_WHILE:
      brw_WHILE(p);
      break;

   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      if (brw->gen == 6) {
	 generate_math1_gen6(inst, dst, src[0]);
      } else {
	 /* Also works for Gen7. */
	 generate_math1_gen4(inst, dst, src[0]);
      }
      break;

   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      if (brw->gen >= 7) {
	 generate_math2_gen7(inst, dst, src[0], src[1]);
      } else if (brw->gen == 6) {
	 generate_math2_gen6(inst, dst, src[0], src[1]);
      } else {
	 generate_math2_gen4(inst, dst, src[0], src[1]);
      }
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
      generate_tex(inst, dst, src[0]);
      break;

   case VS_OPCODE_URB_WRITE:
      generate_vs_urb_write(inst);
      break;

   case SHADER_OPCODE_GEN4_SCRATCH_READ:
      generate_scratch_read(inst, dst, src[0]);
      break;

   case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
      generate_scratch_write(inst, dst, src[0], src[1]);
      break;

   case VS_OPCODE_PULL_CONSTANT_LOAD:
      generate_pull_constant_load(inst, dst, src[0], src[1]);
      break;

   case VS_OPCODE_PULL_CONSTANT_LOAD_GEN7:
      generate_pull_constant_load_gen7(inst, dst, src[0], src[1]);
      break;

   case GS_OPCODE_URB_WRITE:
      generate_gs_urb_write(inst);
      break;

   case GS_OPCODE_THREAD_END:
      generate_gs_thread_end(inst);
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

   case GS_OPCODE_GET_INSTANCE_ID:
      generate_gs_get_instance_id(dst);
      break;

   case SHADER_OPCODE_SHADER_TIME_ADD:
      brw_shader_time_add(p, src[0],
                          prog_data->base.binding_table.shader_time_start);
      brw_mark_surface_used(&prog_data->base,
                            prog_data->base.binding_table.shader_time_start);
      break;

   case SHADER_OPCODE_UNTYPED_ATOMIC:
      generate_untyped_atomic(inst, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_UNTYPED_SURFACE_READ:
      generate_untyped_surface_read(inst, dst, src[0]);
      break;

   case VS_OPCODE_UNPACK_FLAGS_SIMD4X2:
      generate_unpack_flags(inst, dst);
      break;

   default:
      if (inst->opcode < (int) ARRAY_SIZE(opcode_descs)) {
         _mesa_problem(&brw->ctx, "Unsupported opcode in `%s' in vec4\n",
                       opcode_descs[inst->opcode].name);
      } else {
         _mesa_problem(&brw->ctx, "Unsupported opcode %d in vec4", inst->opcode);
      }
      abort();
   }
}

void
vec4_generator::generate_code(exec_list *instructions)
{
   int last_native_insn_offset = 0;
   const char *last_annotation_string = NULL;
   const void *last_annotation_ir = NULL;

   if (unlikely(debug_flag)) {
      if (shader_prog) {
         fprintf(stderr, "Native code for %s vertex shader %d:\n",
                 shader_prog->Label ? shader_prog->Label : "unnamed",
                 shader_prog->Name);
      } else {
         fprintf(stderr, "Native code for vertex program %d:\n", prog->Id);
      }
   }

   foreach_list(node, instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;
      struct brw_reg src[3], dst;

      if (unlikely(debug_flag)) {
	 if (last_annotation_ir != inst->ir) {
	    last_annotation_ir = inst->ir;
	    if (last_annotation_ir) {
	       fprintf(stderr, "   ");
               if (shader_prog) {
                  ((ir_instruction *) last_annotation_ir)->fprint(stderr);
               } else {
                  const prog_instruction *vpi;
                  vpi = (const prog_instruction *) inst->ir;
                  fprintf(stderr, "%d: ", (int)(vpi - prog->Instructions));
                  _mesa_fprint_instruction_opt(stderr, vpi, 0,
                                               PROG_PRINT_DEBUG, NULL);
               }
	       fprintf(stderr, "\n");
	    }
	 }
	 if (last_annotation_string != inst->annotation) {
	    last_annotation_string = inst->annotation;
	    if (last_annotation_string)
	       fprintf(stderr, "   %s\n", last_annotation_string);
	 }
      }

      for (unsigned int i = 0; i < 3; i++) {
	 src[i] = inst->get_src(this->prog_data, i);
      }
      dst = inst->get_dst();

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicate);
      brw_set_predicate_inverse(p, inst->predicate_inverse);
      brw_set_saturate(p, inst->saturate);
      brw_set_mask_control(p, inst->force_writemask_all);
      brw_set_acc_write_control(p, inst->writes_accumulator);

      unsigned pre_emit_nr_insn = p->nr_insn;

      generate_vec4_instruction(inst, dst, src);

      if (inst->no_dd_clear || inst->no_dd_check) {
         assert(p->nr_insn == pre_emit_nr_insn + 1 ||
                !"no_dd_check or no_dd_clear set for IR emitting more "
                "than 1 instruction");

         struct brw_instruction *last = &p->store[pre_emit_nr_insn];

         if (inst->no_dd_clear)
            last->header.dependency_control |= BRW_DEPENDENCY_NOTCLEARED;
         if (inst->no_dd_check)
            last->header.dependency_control |= BRW_DEPENDENCY_NOTCHECKED;
      }

      if (unlikely(debug_flag)) {
	 brw_dump_compile(p, stderr,
			  last_native_insn_offset, p->next_insn_offset);
      }

      last_native_insn_offset = p->next_insn_offset;
   }

   if (unlikely(debug_flag)) {
      fprintf(stderr, "\n");
   }

   brw_set_uip_jip(p);

   /* OK, while the INTEL_DEBUG=vs above is very nice for debugging VS
    * emit issues, it doesn't get the jump distances into the output,
    * which is often something we want to debug.  So this is here in
    * case you're doing that.
    */
   if (0 && unlikely(debug_flag)) {
      brw_dump_compile(p, stderr, 0, p->next_insn_offset);
   }
}

const unsigned *
vec4_generator::generate_assembly(exec_list *instructions,
                                  unsigned *assembly_size)
{
   brw_set_access_mode(p, BRW_ALIGN_16);
   generate_code(instructions);
   return brw_get_program(p, assembly_size);
}

} /* namespace brw */
