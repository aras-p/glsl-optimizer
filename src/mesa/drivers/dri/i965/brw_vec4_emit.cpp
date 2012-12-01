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
vec4_instruction::get_src(int i)
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
      brw_reg = stride(brw_vec4_grf(1 + (src[i].reg + src[i].reg_offset) / 2,
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
                               struct brw_vs_compile *c,
                               struct gl_shader_program *prog,
                               void *mem_ctx)
   : brw(brw), c(c), prog(prog), mem_ctx(mem_ctx)
{
   intel = &brw->intel;
   vp = &c->vp->program;

   shader = prog ? prog->_LinkedShaders[MESA_SHADER_VERTEX] : NULL;

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

   if (intel->gen >= 5) {
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
            assert(intel->is_haswell);
            msg_type = HSW_SAMPLER_MESSAGE_SAMPLE_DERIV_COMPARE;
         } else {
            msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_DERIVS;
         }
	 break;
      case SHADER_OPCODE_TXF:
	 msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
	 break;
      case SHADER_OPCODE_TXF_MS:
         if (intel->gen >= 7)
            msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS;
         else
            msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
         break;
      case SHADER_OPCODE_TXS:
	 msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO;
	 break;
      default:
	 assert(!"should not get here: invalid VS texture opcode");
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
	 assert(!"should not get here: invalid VS texture opcode");
	 break;
      }
   }

   assert(msg_type != -1);

   /* Load the message header if present.  If there's a texture offset, we need
    * to set it up explicitly and load the offset bitfield.  Otherwise, we can
    * use an implied move from g0 to the first message register.
    */
   if (inst->texture_offset) {
      /* Explicitly set up the message header by copying g0 to the MRF. */
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_MOV(p, retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD),
	         retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* Then set the offset bits in DWord 2. */
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_MOV(p,
	      retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, inst->base_mrf, 2),
		     BRW_REGISTER_TYPE_UD),
	      brw_imm_uw(inst->texture_offset));
      brw_pop_insn_state(p);
   } else if (inst->header_present) {
      /* Set up an implied move from g0 to the MRF. */
      src = brw_vec8_grf(0, 0);
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

   brw_SAMPLE(p,
	      dst,
	      inst->base_mrf,
	      src,
	      SURF_INDEX_VS_TEXTURE(inst->sampler),
	      inst->sampler,
	      msg_type,
	      1, /* response length */
	      inst->mlen,
	      inst->header_present,
	      BRW_SAMPLER_SIMD_MODE_SIMD4X2,
	      return_format);
}

void
vec4_generator::generate_urb_write(vec4_instruction *inst)
{
   brw_urb_WRITE(p,
		 brw_null_reg(), /* dest */
		 inst->base_mrf, /* starting mrf reg nr */
		 brw_vec8_grf(0, 0), /* src */
		 false,		/* allocate */
		 true,		/* used */
		 inst->mlen,
		 0,		/* response len */
		 inst->eot,	/* eot */
		 inst->eot,	/* writes complete */
		 inst->offset,	/* urb destination offset */
		 BRW_URB_SWIZZLE_INTERLEAVE);
}

void
vec4_generator::generate_oword_dual_block_offsets(struct brw_reg m1,
                                                  struct brw_reg index)
{
   int second_vertex_offset;

   if (intel->gen >= 6)
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
vec4_generator::generate_scratch_read(vec4_instruction *inst,
                                      struct brw_reg dst,
                                      struct brw_reg index)
{
   struct brw_reg header = brw_vec8_grf(0, 0);

   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   generate_oword_dual_block_offsets(brw_message_reg(inst->base_mrf + 1),
				     index);

   uint32_t msg_type;

   if (intel->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (intel->gen == 5 || intel->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (intel->gen < 6)
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

   if (intel->gen >= 7)
      msg_type = GEN7_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;
   else if (intel->gen == 6)
      msg_type = GEN6_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;
   else
      msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE;

   brw_set_predicate_control(p, inst->predicate);

   /* Pre-gen6, we have to specify write commits to ensure ordering
    * between reads and writes within a thread.  Afterwards, that's
    * guaranteed and write commits only matter for inter-thread
    * synchronization.
    */
   if (intel->gen >= 6) {
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
   if (intel->gen < 6)
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
   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   if (intel->gen == 7) {
      gen6_resolve_implied_move(p, &offset, inst->base_mrf);
      brw_instruction *insn = brw_next_insn(p, BRW_OPCODE_SEND);
      brw_set_dest(p, insn, dst);
      brw_set_src0(p, insn, offset);
      brw_set_sampler_message(p, insn,
                              surf_index,
                              0, /* LD message ignores sampler unit */
                              GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                              1, /* rlen */
                              1, /* mlen */
                              false, /* no header */
                              BRW_SAMPLER_SIMD_MODE_SIMD4X2,
                              0);
      return;
   }

   struct brw_reg header = brw_vec8_grf(0, 0);

   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   brw_MOV(p, retype(brw_message_reg(inst->base_mrf + 1), BRW_REGISTER_TYPE_D),
	   offset);

   uint32_t msg_type;

   if (intel->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (intel->gen == 5 || intel->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   /* Each of the 8 channel enables is considered for whether each
    * dword is written.
    */
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (intel->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_read_message(p, send,
			   surf_index,
			   BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			   msg_type,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   2, /* mlen */
                           true, /* header_present */
			   1 /* rlen */);
}

void
vec4_generator::generate_vs_instruction(vec4_instruction *instruction,
                                        struct brw_reg dst,
                                        struct brw_reg *src)
{
   vec4_instruction *inst = (vec4_instruction *)instruction;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      if (intel->gen == 6) {
	 generate_math1_gen6(inst, dst, src[0]);
      } else {
	 /* Also works for Gen7. */
	 generate_math1_gen4(inst, dst, src[0]);
      }
      break;

   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      if (intel->gen >= 7) {
	 generate_math2_gen7(inst, dst, src[0], src[1]);
      } else if (intel->gen == 6) {
	 generate_math2_gen6(inst, dst, src[0], src[1]);
      } else {
	 generate_math2_gen4(inst, dst, src[0], src[1]);
      }
      break;

   case SHADER_OPCODE_TEX:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXF_MS:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXS:
      generate_tex(inst, dst, src[0]);
      break;

   case VS_OPCODE_URB_WRITE:
      generate_urb_write(inst);
      break;

   case VS_OPCODE_SCRATCH_READ:
      generate_scratch_read(inst, dst, src[0]);
      break;

   case VS_OPCODE_SCRATCH_WRITE:
      generate_scratch_write(inst, dst, src[0], src[1]);
      break;

   case VS_OPCODE_PULL_CONSTANT_LOAD:
      generate_pull_constant_load(inst, dst, src[0], src[1]);
      break;

   case SHADER_OPCODE_SHADER_TIME_ADD:
      brw_shader_time_add(p, src[0], SURF_INDEX_VS_SHADER_TIME);
      break;

   default:
      if (inst->opcode < (int) ARRAY_SIZE(opcode_descs)) {
         _mesa_problem(ctx, "Unsupported opcode in `%s' in VS\n",
                       opcode_descs[inst->opcode].name);
      } else {
         _mesa_problem(ctx, "Unsupported opcode %d in VS", inst->opcode);
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

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      if (shader) {
         printf("Native code for vertex shader %d:\n", prog->Name);
      } else {
         printf("Native code for vertex program %d:\n", c->vp->program.Base.Id);
      }
   }

   foreach_list(node, instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;
      struct brw_reg src[3], dst;

      if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
	 if (last_annotation_ir != inst->ir) {
	    last_annotation_ir = inst->ir;
	    if (last_annotation_ir) {
	       printf("   ");
               if (shader) {
                  ((ir_instruction *) last_annotation_ir)->print();
               } else {
                  const prog_instruction *vpi;
                  vpi = (const prog_instruction *) inst->ir;
                  printf("%d: ", (int)(vpi - vp->Base.Instructions));
                  _mesa_fprint_instruction_opt(stdout, vpi, 0,
                                               PROG_PRINT_DEBUG, NULL);
               }
	       printf("\n");
	    }
	 }
	 if (last_annotation_string != inst->annotation) {
	    last_annotation_string = inst->annotation;
	    if (last_annotation_string)
	       printf("   %s\n", last_annotation_string);
	 }
      }

      for (unsigned int i = 0; i < 3; i++) {
	 src[i] = inst->get_src(i);
      }
      dst = inst->get_dst();

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicate);
      brw_set_predicate_inverse(p, inst->predicate_inverse);
      brw_set_saturate(p, inst->saturate);
      brw_set_mask_control(p, inst->force_writemask_all);

      unsigned pre_emit_nr_insn = p->nr_insn;

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
	 brw_set_acc_write_control(p, 1);
	 brw_MACH(p, dst, src[0], src[1]);
	 brw_set_acc_write_control(p, 0);
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
         brw_F32TO16(p, dst, src[0]);
         break;

      case BRW_OPCODE_F16TO32:
         brw_F16TO32(p, dst, src[0]);
         break;

      case BRW_OPCODE_IF:
	 if (inst->src[0].file != BAD_FILE) {
	    /* The instruction has an embedded compare (only allowed on gen6) */
	    assert(intel->gen == 6);
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
	 if (intel->gen >= 6)
	    gen6_CONT(p);
	 else
	    brw_CONT(p);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;

      case BRW_OPCODE_WHILE:
	 brw_WHILE(p);
	 break;

      default:
	 generate_vs_instruction(inst, dst, src);
	 break;
      }

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

      if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
	 brw_dump_compile(p, stdout,
			  last_native_insn_offset, p->next_insn_offset);
      }

      last_native_insn_offset = p->next_insn_offset;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("\n");
   }

   brw_set_uip_jip(p);

   /* OK, while the INTEL_DEBUG=vs above is very nice for debugging VS
    * emit issues, it doesn't get the jump distances into the output,
    * which is often something we want to debug.  So this is here in
    * case you're doing that.
    */
   if (0 && unlikely(INTEL_DEBUG & DEBUG_VS)) {
      brw_dump_compile(p, stdout, 0, p->next_insn_offset);
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
