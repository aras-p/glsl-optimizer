/*
 * Copyright © 2010 Intel Corporation
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

/** @file brw_fs_emit.cpp
 *
 * This file supports emitting code from the FS LIR to the actual
 * native instructions.
 */

extern "C" {
#include "main/macros.h"
#include "brw_context.h"
#include "brw_eu.h"
} /* extern "C" */

#include "brw_fs.h"
#include "brw_cfg.h"
#include "glsl/ir_print_visitor.h"

fs_generator::fs_generator(struct brw_context *brw,
                           struct brw_wm_compile *c,
                           struct gl_shader_program *prog,
                           struct gl_fragment_program *fp,
                           bool dual_source_output)

   : brw(brw), c(c), prog(prog), fp(fp), dual_source_output(dual_source_output)
{
   intel = &brw->intel;
   ctx = &intel->ctx;

   shader = prog ? prog->_LinkedShaders[MESA_SHADER_FRAGMENT] : NULL;

   mem_ctx = c;

   p = rzalloc(mem_ctx, struct brw_compile);
   brw_init_compile(brw, p, mem_ctx);
}

fs_generator::~fs_generator()
{
}

void
fs_generator::patch_discard_jumps_to_fb_writes()
{
   if (intel->gen < 6 || this->discard_halt_patches.is_empty())
      return;

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
   struct brw_instruction *last_halt = gen6_HALT(p);
   last_halt->bits3.break_cont.uip = 2;
   last_halt->bits3.break_cont.jip = 2;

   int ip = p->nr_insn;

   foreach_list(node, &this->discard_halt_patches) {
      ip_record *patch_ip = (ip_record *)node;
      struct brw_instruction *patch = &p->store[patch_ip->ip];

      assert(patch->header.opcode == BRW_OPCODE_HALT);
      /* HALT takes a half-instruction distance from the pre-incremented IP. */
      patch->bits3.break_cont.uip = (ip - patch_ip->ip) * 2;
   }

   this->discard_halt_patches.make_empty();
}

void
fs_generator::generate_fb_write(fs_inst *inst)
{
   bool eot = inst->eot;
   struct brw_reg implied_header;
   uint32_t msg_control;

   /* Note that the jumps emitted to this point mean that the g0 ->
    * base_mrf setup must be inside of this function, so that we jump
    * to a point containing it.
    */
   patch_discard_jumps_to_fb_writes();

   /* Header is 2 regs, g0 and g1 are the contents. g0 will be implied
    * move, here's g1.
    */
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   if (fp->UsesKill) {
      struct brw_reg pixel_mask;

      if (intel->gen >= 6)
         pixel_mask = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
      else
         pixel_mask = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);

      brw_MOV(p, pixel_mask, brw_flag_reg(0, 1));
   }

   if (inst->header_present) {
      if (intel->gen >= 6) {
	 brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
	 brw_MOV(p,
		 retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD),
		 retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);

         if (inst->target > 0 &&
	     c->key.nr_color_regions > 1 &&
	     c->key.sample_alpha_to_coverage) {
            /* Set "Source0 Alpha Present to RenderTarget" bit in message
             * header.
             */
            brw_OR(p,
		   vec1(retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD)),
		   vec1(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD)),
		   brw_imm_ud(0x1 << 11));
         }

	 if (inst->target > 0) {
	    /* Set the render target index for choosing BLEND_STATE. */
	    brw_MOV(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
					   inst->base_mrf, 2),
			      BRW_REGISTER_TYPE_UD),
		    brw_imm_ud(inst->target));
	 }

	 implied_header = brw_null_reg();
      } else {
	 implied_header = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW);

	 brw_MOV(p,
		 brw_message_reg(inst->base_mrf + 1),
		 brw_vec8_grf(1, 0));
      }
   } else {
      implied_header = brw_null_reg();
   }

   if (this->dual_source_output)
      msg_control = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN01;
   else if (dispatch_width == 16)
      msg_control = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE;
   else
      msg_control = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_SINGLE_SOURCE_SUBSPAN01;

   brw_pop_insn_state(p);

   brw_fb_WRITE(p,
		dispatch_width,
		inst->base_mrf,
		implied_header,
		msg_control,
		inst->target,
		inst->mlen,
		0,
		eot,
		inst->header_present);
}

/* Computes the integer pixel x,y values from the origin.
 *
 * This is the basis of gl_FragCoord computation, but is also used
 * pre-gen6 for computing the deltas from v0 for computing
 * interpolation.
 */
void
fs_generator::generate_pixel_xy(struct brw_reg dst, bool is_x)
{
   struct brw_reg g1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);
   struct brw_reg src;
   struct brw_reg deltas;

   if (is_x) {
      src = stride(suboffset(g1_uw, 4), 2, 4, 0);
      deltas = brw_imm_v(0x10101010);
   } else {
      src = stride(suboffset(g1_uw, 5), 2, 4, 0);
      deltas = brw_imm_v(0x11001100);
   }

   if (dispatch_width == 16) {
      dst = vec16(dst);
   }

   /* We do this 8 or 16-wide, but since the destination is UW we
    * don't do compression in the 16-wide case.
    */
   brw_push_insn_state(p);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_ADD(p, dst, src, deltas);
   brw_pop_insn_state(p);
}

void
fs_generator::generate_linterp(fs_inst *inst,
			     struct brw_reg dst, struct brw_reg *src)
{
   struct brw_reg delta_x = src[0];
   struct brw_reg delta_y = src[1];
   struct brw_reg interp = src[2];

   if (brw->has_pln &&
       delta_y.nr == delta_x.nr + 1 &&
       (intel->gen >= 6 || (delta_x.nr & 1) == 0)) {
      brw_PLN(p, dst, interp, delta_x);
   } else {
      brw_LINE(p, brw_null_reg(), interp, delta_x);
      brw_MAC(p, dst, suboffset(interp, 1), delta_y);
   }
}

void
fs_generator::generate_math1_gen7(fs_inst *inst,
			        struct brw_reg dst,
			        struct brw_reg src0)
{
   assert(inst->mlen == 0);
   brw_math(p, dst,
	    brw_math_function(inst->opcode),
	    0, src0,
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

void
fs_generator::generate_math2_gen7(fs_inst *inst,
			        struct brw_reg dst,
			        struct brw_reg src0,
			        struct brw_reg src1)
{
   assert(inst->mlen == 0);
   brw_math2(p, dst, brw_math_function(inst->opcode), src0, src1);
}

void
fs_generator::generate_math1_gen6(fs_inst *inst,
			        struct brw_reg dst,
			        struct brw_reg src0)
{
   int op = brw_math_function(inst->opcode);

   assert(inst->mlen == 0);

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_math(p, dst,
	    op,
	    0, src0,
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);

   if (dispatch_width == 16) {
      brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
      brw_math(p, sechalf(dst),
	       op,
	       0, sechalf(src0),
	       BRW_MATH_DATA_VECTOR,
	       BRW_MATH_PRECISION_FULL);
      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
   }
}

void
fs_generator::generate_math2_gen6(fs_inst *inst,
			        struct brw_reg dst,
			        struct brw_reg src0,
			        struct brw_reg src1)
{
   int op = brw_math_function(inst->opcode);

   assert(inst->mlen == 0);

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_math2(p, dst, op, src0, src1);

   if (dispatch_width == 16) {
      brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
      brw_math2(p, sechalf(dst), op, sechalf(src0), sechalf(src1));
      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
   }
}

void
fs_generator::generate_math_gen4(fs_inst *inst,
			       struct brw_reg dst,
			       struct brw_reg src)
{
   int op = brw_math_function(inst->opcode);

   assert(inst->mlen >= 1);

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_math(p, dst,
	    op,
	    inst->base_mrf, src,
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);

   if (dispatch_width == 16) {
      brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
      brw_math(p, sechalf(dst),
	       op,
	       inst->base_mrf + 1, sechalf(src),
	       BRW_MATH_DATA_VECTOR,
	       BRW_MATH_PRECISION_FULL);

      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
   }
}

void
fs_generator::generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   int msg_type = -1;
   int rlen = 4;
   uint32_t simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD8;
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

   if (dispatch_width == 16)
      simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;

   if (intel->gen >= 5) {
      switch (inst->opcode) {
      case SHADER_OPCODE_TEX:
	 if (inst->shadow_compare) {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_COMPARE;
	 } else {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE;
	 }
	 break;
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE;
	 } else {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS;
	 }
	 break;
      case SHADER_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE;
	 } else {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD;
	 }
	 break;
      case SHADER_OPCODE_TXS:
	 msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO;
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
      default:
	 assert(!"not reached");
	 break;
      }
   } else {
      switch (inst->opcode) {
      case SHADER_OPCODE_TEX:
	 /* Note that G45 and older determines shadow compare and dispatch width
	  * from message length for most messages.
	  */
	 assert(dispatch_width == 8);
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE;
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 6);
	 } else {
	    assert(inst->mlen <= 4);
	 }
	 break;
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 6);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_BIAS_COMPARE;
	 } else {
	    assert(inst->mlen == 9);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS;
	    simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 }
	 break;
      case SHADER_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 6);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_LOD_COMPARE;
	 } else {
	    assert(inst->mlen == 9);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_LOD;
	    simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 }
	 break;
      case SHADER_OPCODE_TXD:
	 /* There is no sample_d_c message; comparisons are done manually */
	 assert(inst->mlen == 7 || inst->mlen == 10);
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_GRADIENTS;
	 break;
      case SHADER_OPCODE_TXF:
	 assert(inst->mlen == 9);
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD16_LD;
	 simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 break;
      case SHADER_OPCODE_TXS:
	 assert(inst->mlen == 3);
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD16_RESINFO;
	 simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 break;
      default:
	 assert(!"not reached");
	 break;
      }
   }
   assert(msg_type != -1);

   if (simd_mode == BRW_SAMPLER_SIMD_MODE_SIMD16) {
      rlen = 8;
      dst = vec16(dst);
   }

   /* Load the message header if present.  If there's a texture offset,
    * we need to set it up explicitly and load the offset bitfield.
    * Otherwise, we can use an implied move from g0 to the first message reg.
    */
   if (inst->texture_offset) {
      brw_push_insn_state(p);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      /* Explicitly set up the message header by copying g0 to the MRF. */
      brw_MOV(p, retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD),
                 retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* Then set the offset bits in DWord 2. */
      brw_MOV(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
                                     inst->base_mrf, 2), BRW_REGISTER_TYPE_UD),
                 brw_imm_ud(inst->texture_offset));
      brw_pop_insn_state(p);
   } else if (inst->header_present) {
      /* Set up an implied move from g0 to the MRF. */
      src = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW);
   }

   brw_SAMPLE(p,
	      retype(dst, BRW_REGISTER_TYPE_UW),
	      inst->base_mrf,
	      src,
              SURF_INDEX_TEXTURE(inst->sampler),
	      inst->sampler,
	      WRITEMASK_XYZW,
	      msg_type,
	      rlen,
	      inst->mlen,
	      inst->header_present,
	      simd_mode,
	      return_format);
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
fs_generator::generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   struct brw_reg src0 = brw_reg(src.file, src.nr, 1,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_2,
				 BRW_WIDTH_2,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, 0,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_2,
				 BRW_WIDTH_2,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   brw_ADD(p, dst, src0, negate(src1));
}

/* The negate_value boolean is used to negate the derivative computation for
 * FBOs, since they place the origin at the upper left instead of the lower
 * left.
 */
void
fs_generator::generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src,
                         bool negate_value)
{
   struct brw_reg src0 = brw_reg(src.file, src.nr, 0,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_4,
				 BRW_WIDTH_4,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, 2,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_4,
				 BRW_WIDTH_4,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   if (negate_value)
      brw_ADD(p, dst, src1, negate(src0));
   else
      brw_ADD(p, dst, src0, negate(src1));
}

void
fs_generator::generate_discard_jump(fs_inst *inst)
{
   assert(intel->gen >= 6);

   /* This HALT will be patched up at FB write time to point UIP at the end of
    * the program, and at brw_uip_jip() JIP will be set to the end of the
    * current block (or the program).
    */
   this->discard_halt_patches.push_tail(new(mem_ctx) ip_record(p->nr_insn));

   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   gen6_HALT(p);
   brw_pop_insn_state(p);
}

void
fs_generator::generate_spill(fs_inst *inst, struct brw_reg src)
{
   assert(inst->mlen != 0);

   brw_MOV(p,
	   retype(brw_message_reg(inst->base_mrf + 1), BRW_REGISTER_TYPE_UD),
	   retype(src, BRW_REGISTER_TYPE_UD));
   brw_oword_block_write_scratch(p, brw_message_reg(inst->base_mrf), 1,
				 inst->offset);
}

void
fs_generator::generate_unspill(fs_inst *inst, struct brw_reg dst)
{
   assert(inst->mlen != 0);

   /* Clear any post destination dependencies that would be ignored by
    * the block read.  See the B-Spec for pre-gen5 send instruction.
    *
    * This could use a better solution, since texture sampling and
    * math reads could potentially run into it as well -- anywhere
    * that we have a SEND with a destination that is a register that
    * was written but not read within the last N instructions (what's
    * N?  unsure).  This is rare because of dead code elimination, but
    * not impossible.
    */
   if (intel->gen == 4 && !intel->is_g4x)
      brw_MOV(p, brw_null_reg(), dst);

   brw_oword_block_read_scratch(p, dst, brw_message_reg(inst->base_mrf), 1,
				inst->offset);

   if (intel->gen == 4 && !intel->is_g4x) {
      /* gen4 errata: destination from a send can't be used as a
       * destination until it's been read.  Just read it so we don't
       * have to worry.
       */
      brw_MOV(p, brw_null_reg(), dst);
   }
}

void
fs_generator::generate_uniform_pull_constant_load(fs_inst *inst,
                                                  struct brw_reg dst,
                                                  struct brw_reg index,
                                                  struct brw_reg offset)
{
   assert(inst->mlen != 0);

   /* Clear any post destination dependencies that would be ignored by
    * the block read.  See the B-Spec for pre-gen5 send instruction.
    *
    * This could use a better solution, since texture sampling and
    * math reads could potentially run into it as well -- anywhere
    * that we have a SEND with a destination that is a register that
    * was written but not read within the last N instructions (what's
    * N?  unsure).  This is rare because of dead code elimination, but
    * not impossible.
    */
   if (intel->gen == 4 && !intel->is_g4x)
      brw_MOV(p, brw_null_reg(), dst);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   assert(offset.file == BRW_IMMEDIATE_VALUE &&
	  offset.type == BRW_REGISTER_TYPE_UD);
   uint32_t read_offset = offset.dw1.ud;

   brw_oword_block_read(p, dst, brw_message_reg(inst->base_mrf),
			read_offset, surf_index);

   if (intel->gen == 4 && !intel->is_g4x) {
      /* gen4 errata: destination from a send can't be used as a
       * destination until it's been read.  Just read it so we don't
       * have to worry.
       */
      brw_MOV(p, brw_null_reg(), dst);
   }
}

void
fs_generator::generate_uniform_pull_constant_load_gen7(fs_inst *inst,
                                                       struct brw_reg dst,
                                                       struct brw_reg index,
                                                       struct brw_reg offset)
{
   assert(inst->mlen == 0);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   assert(offset.file == BRW_GENERAL_REGISTER_FILE);

   brw_push_insn_state(p);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_pop_insn_state(p);

   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, offset);

   uint32_t msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
   uint32_t msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ;
   bool header_present = true;
   brw_set_dp_read_message(p, send,
                           surf_index,
                           msg_control,
                           msg_type,
                           BRW_DATAPORT_READ_TARGET_DATA_CACHE,
                           1,
                           header_present,
                           1);
}

void
fs_generator::generate_varying_pull_constant_load(fs_inst *inst,
                                                  struct brw_reg dst,
                                                  struct brw_reg index)
{
   assert(intel->gen < 7); /* Should use the gen7 variant. */
   assert(inst->header_present);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   uint32_t msg_type, msg_control, rlen;
   if (intel->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ;
   else if (intel->gen == 5 || intel->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ;

   if (dispatch_width == 16) {
      msg_control = BRW_DATAPORT_DWORD_SCATTERED_BLOCK_16DWORDS;
      rlen = 2;
   } else {
      msg_control = BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS;
      rlen = 1;
   }

   struct brw_reg header = brw_vec8_grf(0, 0);
   gen6_resolve_implied_move(p, &header, inst->base_mrf);

   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, header);
   if (intel->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_read_message(p, send,
                           surf_index,
                           msg_control,
                           msg_type,
                           BRW_DATAPORT_READ_TARGET_DATA_CACHE,
                           inst->mlen,
                           inst->header_present,
                           rlen);
}

void
fs_generator::generate_varying_pull_constant_load_gen7(fs_inst *inst,
                                                       struct brw_reg dst,
                                                       struct brw_reg index,
                                                       struct brw_reg offset)
{
   assert(intel->gen >= 7);
   /* Varying-offset pull constant loads are treated as a normal expression on
    * gen7, so the fact that it's a send message is hidden at the IR level.
    */
   assert(!inst->header_present);
   assert(!inst->mlen);

   assert(index.file == BRW_IMMEDIATE_VALUE &&
	  index.type == BRW_REGISTER_TYPE_UD);
   uint32_t surf_index = index.dw1.ud;

   uint32_t msg_control, rlen, mlen;
   if (dispatch_width == 16) {
      msg_control = BRW_DATAPORT_DWORD_SCATTERED_BLOCK_16DWORDS;
      mlen = rlen = 2;
   } else {
      msg_control = BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS;
      mlen = rlen = 1;
   }

   struct brw_instruction *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, offset);
   if (intel->gen < 6)
      send->header.destreg__conditionalmod = inst->base_mrf;
   brw_set_dp_read_message(p, send,
                           surf_index,
                           msg_control,
                           GEN7_DATAPORT_DC_DWORD_SCATTERED_READ,
                           BRW_DATAPORT_READ_TARGET_DATA_CACHE,
                           mlen,
                           inst->header_present,
                           rlen);
}

/**
 * Cause the current pixel/sample mask (from R1.7 bits 15:0) to be transferred
 * into the flags register (f0.0).
 *
 * Used only on Gen6 and above.
 */
void
fs_generator::generate_mov_dispatch_to_flags(fs_inst *inst)
{
   struct brw_reg flags = brw_flag_reg(0, inst->flag_subreg);
   struct brw_reg dispatch_mask;

   if (intel->gen >= 6)
      dispatch_mask = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
   else
      dispatch_mask = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);

   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MOV(p, flags, dispatch_mask);
   brw_pop_insn_state(p);
}


static uint32_t brw_file_from_reg(fs_reg *reg)
{
   switch (reg->file) {
   case ARF:
      return BRW_ARCHITECTURE_REGISTER_FILE;
   case GRF:
      return BRW_GENERAL_REGISTER_FILE;
   case MRF:
      return BRW_MESSAGE_REGISTER_FILE;
   case IMM:
      return BRW_IMMEDIATE_VALUE;
   default:
      assert(!"not reached");
      return BRW_GENERAL_REGISTER_FILE;
   }
}

static struct brw_reg
brw_reg_from_fs_reg(fs_reg *reg)
{
   struct brw_reg brw_reg;

   switch (reg->file) {
   case GRF:
   case ARF:
   case MRF:
      if (reg->smear == -1) {
	 brw_reg = brw_vec8_reg(brw_file_from_reg(reg), reg->reg, 0);
      } else {
	 brw_reg = brw_vec1_reg(brw_file_from_reg(reg), reg->reg, reg->smear);
      }
      brw_reg = retype(brw_reg, reg->type);
      if (reg->sechalf)
	 brw_reg = sechalf(brw_reg);
      break;
   case IMM:
      switch (reg->type) {
      case BRW_REGISTER_TYPE_F:
	 brw_reg = brw_imm_f(reg->imm.f);
	 break;
      case BRW_REGISTER_TYPE_D:
	 brw_reg = brw_imm_d(reg->imm.i);
	 break;
      case BRW_REGISTER_TYPE_UD:
	 brw_reg = brw_imm_ud(reg->imm.u);
	 break;
      default:
	 assert(!"not reached");
	 brw_reg = brw_null_reg();
	 break;
      }
      break;
   case FIXED_HW_REG:
      brw_reg = reg->fixed_hw_reg;
      break;
   case BAD_FILE:
      /* Probably unused. */
      brw_reg = brw_null_reg();
      break;
   case UNIFORM:
      assert(!"not reached");
      brw_reg = brw_null_reg();
      break;
   default:
      assert(!"not reached");
      brw_reg = brw_null_reg();
      break;
   }
   if (reg->abs)
      brw_reg = brw_abs(brw_reg);
   if (reg->negate)
      brw_reg = negate(brw_reg);

   return brw_reg;
}

/**
 * Sets the second dword of a vgrf for gen7+ message setup.
 *
 * For setting up gen7 messages in VGRFs, we need to be able to set the second
 * dword for some payloads where in the MRF world we'd have just used
 * brw_message_reg().  We don't want to bake it into the send message's code
 * generation because that means we don't get a chance to schedule the
 * instructions.
 */
void
fs_generator::generate_set_global_offset(fs_inst *inst,
                                         struct brw_reg dst,
                                         struct brw_reg src,
                                         struct brw_reg value)
{
   /* We use a matching src and dst to get the information on how this
    * instruction works exposed to various optimization passes that would
    * otherwise treat it as completely overwriting the dst.
    */
   assert(src.file == dst.file && src.nr == dst.nr);
   assert(value.file == BRW_IMMEDIATE_VALUE);

   brw_push_insn_state(p);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MOV(p, retype(brw_vec1_reg(dst.file, dst.nr, 2), value.type), value);
   brw_pop_insn_state(p);
}

/**
 * Change the register's data type from UD to W, doubling the strides in order
 * to compensate for halving the data type width.
 */
static struct brw_reg
ud_reg_to_w(struct brw_reg r)
{
   assert(r.type == BRW_REGISTER_TYPE_UD);
   r.type = BRW_REGISTER_TYPE_W;

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
fs_generator::generate_pack_half_2x16_split(fs_inst *inst,
                                            struct brw_reg dst,
                                            struct brw_reg x,
                                            struct brw_reg y)
{
   assert(intel->gen >= 7);
   assert(dst.type == BRW_REGISTER_TYPE_UD);
   assert(x.type = BRW_REGISTER_TYPE_F);
   assert(y.type = BRW_REGISTER_TYPE_F);

   /* From the Ivybridge PRM, Vol4, Part3, Section 6.27 f32to16:
    *
    *   Because this instruction does not have a 16-bit floating-point type,
    *   the destination data type must be Word (W).
    *
    *   The destination must be DWord-aligned and specify a horizontal stride
    *   (HorzStride) of 2. The 16-bit result is stored in the lower word of
    *   each destination channel and the upper word is not modified.
    */
   struct brw_reg dst_w = ud_reg_to_w(dst);

   /* Give each 32-bit channel of dst the form below , where "." means
    * unchanged.
    *   0x....hhhh
    */
   brw_F32TO16(p, dst_w, y);

   /* Now the form:
    *   0xhhhh0000
    */
   brw_SHL(p, dst, dst, brw_imm_ud(16u));

   /* And, finally the form of packHalf2x16's output:
    *   0xhhhhllll
    */
   brw_F32TO16(p, dst_w, x);
}

void
fs_generator::generate_unpack_half_2x16_split(fs_inst *inst,
                                              struct brw_reg dst,
                                              struct brw_reg src)
{
   assert(intel->gen >= 7);
   assert(dst.type == BRW_REGISTER_TYPE_F);
   assert(src.type == BRW_REGISTER_TYPE_UD);

   /* From the Ivybridge PRM, Vol4, Part3, Section 6.26 f16to32:
    *
    *   Because this instruction does not have a 16-bit floating-point type,
    *   the source data type must be Word (W). The destination type must be
    *   F (Float).
    */
   struct brw_reg src_w = ud_reg_to_w(src);

   /* Each channel of src has the form of unpackHalf2x16's input: 0xhhhhllll.
    * For the Y case, we wish to access only the upper word; therefore
    * a 16-bit subregister offset is needed.
    */
   assert(inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_X ||
          inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y);
   if (inst->opcode == FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y)
      src_w.subnr += 2;

   brw_F16TO32(p, dst, src_w);
}

void
fs_generator::generate_code(exec_list *instructions)
{
   int last_native_insn_offset = p->next_insn_offset;
   const char *last_annotation_string = NULL;
   const void *last_annotation_ir = NULL;

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      if (shader) {
         printf("Native code for fragment shader %d (%d-wide dispatch):\n",
                prog->Name, dispatch_width);
      } else {
         printf("Native code for fragment program %d (%d-wide dispatch):\n",
                fp->Base.Id, dispatch_width);
      }
   }

   cfg_t *cfg = NULL;
   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      cfg = new(mem_ctx) cfg_t(mem_ctx, instructions);

   foreach_list(node, instructions) {
      fs_inst *inst = (fs_inst *)node;
      struct brw_reg src[3], dst;

      if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
	 foreach_list(node, &cfg->block_list) {
	    bblock_link *link = (bblock_link *)node;
	    bblock_t *block = link->block;

	    if (block->start == inst) {
	       printf("   START B%d", block->block_num);
	       foreach_list(predecessor_node, &block->parents) {
		  bblock_link *predecessor_link =
		     (bblock_link *)predecessor_node;
		  bblock_t *predecessor_block = predecessor_link->block;
		  printf(" <-B%d", predecessor_block->block_num);
	       }
	       printf("\n");
	    }
	 }

	 if (last_annotation_ir != inst->ir) {
	    last_annotation_ir = inst->ir;
	    if (last_annotation_ir) {
	       printf("   ");
               if (shader)
                  ((ir_instruction *)inst->ir)->print();
               else {
                  const prog_instruction *fpi;
                  fpi = (const prog_instruction *)inst->ir;
                  printf("%d: ", (int)(fpi - fp->Base.Instructions));
                  _mesa_fprint_instruction_opt(stdout,
                                               fpi,
                                               0, PROG_PRINT_DEBUG, NULL);
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
	 src[i] = brw_reg_from_fs_reg(&inst->src[i]);

	 /* The accumulator result appears to get used for the
	  * conditional modifier generation.  When negating a UD
	  * value, there is a 33rd bit generated for the sign in the
	  * accumulator value, so now you can't check, for example,
	  * equality with a 32-bit value.  See piglit fs-op-neg-uvec4.
	  */
	 assert(!inst->conditional_mod ||
		inst->src[i].type != BRW_REGISTER_TYPE_UD ||
		!inst->src[i].negate);
      }
      dst = brw_reg_from_fs_reg(&inst->dst);

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicate);
      brw_set_predicate_inverse(p, inst->predicate_inverse);
      brw_set_flag_reg(p, 0, inst->flag_subreg);
      brw_set_saturate(p, inst->saturate);
      brw_set_mask_control(p, inst->force_writemask_all);

      if (inst->force_uncompressed || dispatch_width == 8) {
	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      } else if (inst->force_sechalf) {
	 brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
      } else {
	 brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
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
	 brw_set_acc_write_control(p, 1);
	 brw_MACH(p, dst, src[0], src[1]);
	 brw_set_acc_write_control(p, 0);
	 break;

      case BRW_OPCODE_MAD:
	 brw_set_access_mode(p, BRW_ALIGN_16);
	 if (dispatch_width == 16) {
	    brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	    brw_MAD(p, dst, src[0], src[1], src[2]);
	    brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
	    brw_MAD(p, sechalf(dst), sechalf(src[0]), sechalf(src[1]), sechalf(src[2]));
	    brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
	 } else {
	    brw_MAD(p, dst, src[0], src[1], src[2]);
	 }
	 brw_set_access_mode(p, BRW_ALIGN_1);
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
      case BRW_OPCODE_F32TO16:
         brw_F32TO16(p, dst, src[0]);
         break;
      case BRW_OPCODE_F16TO32:
         brw_F16TO32(p, dst, src[0]);
         break;
      case BRW_OPCODE_CMP:
	 brw_CMP(p, dst, inst->conditional_mod, src[0], src[1]);
	 break;
      case BRW_OPCODE_SEL:
	 brw_SEL(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_IF:
	 if (inst->src[0].file != BAD_FILE) {
	    /* The instruction has an embedded compare (only allowed on gen6) */
	    assert(intel->gen == 6);
	    gen6_IF(p, inst->conditional_mod, src[0], src[1]);
	 } else {
	    brw_IF(p, dispatch_width == 16 ? BRW_EXECUTE_16 : BRW_EXECUTE_8);
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

      case SHADER_OPCODE_RCP:
      case SHADER_OPCODE_RSQ:
      case SHADER_OPCODE_SQRT:
      case SHADER_OPCODE_EXP2:
      case SHADER_OPCODE_LOG2:
      case SHADER_OPCODE_SIN:
      case SHADER_OPCODE_COS:
	 if (intel->gen >= 7) {
	    generate_math1_gen7(inst, dst, src[0]);
	 } else if (intel->gen == 6) {
	    generate_math1_gen6(inst, dst, src[0]);
	 } else {
	    generate_math_gen4(inst, dst, src[0]);
	 }
	 break;
      case SHADER_OPCODE_INT_QUOTIENT:
      case SHADER_OPCODE_INT_REMAINDER:
      case SHADER_OPCODE_POW:
	 if (intel->gen >= 7) {
	    generate_math2_gen7(inst, dst, src[0], src[1]);
	 } else if (intel->gen == 6) {
	    generate_math2_gen6(inst, dst, src[0], src[1]);
	 } else {
	    generate_math_gen4(inst, dst, src[0]);
	 }
	 break;
      case FS_OPCODE_PIXEL_X:
	 generate_pixel_xy(dst, true);
	 break;
      case FS_OPCODE_PIXEL_Y:
	 generate_pixel_xy(dst, false);
	 break;
      case FS_OPCODE_CINTERP:
	 brw_MOV(p, dst, src[0]);
	 break;
      case FS_OPCODE_LINTERP:
	 generate_linterp(inst, dst, src);
	 break;
      case SHADER_OPCODE_TEX:
      case FS_OPCODE_TXB:
      case SHADER_OPCODE_TXD:
      case SHADER_OPCODE_TXF:
      case SHADER_OPCODE_TXL:
      case SHADER_OPCODE_TXS:
	 generate_tex(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DDX:
	 generate_ddx(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DDY:
         /* Make sure fp->UsesDFdy flag got set (otherwise there's no
          * guarantee that c->key.render_to_fbo is set).
          */
         assert(fp->UsesDFdy);
	 generate_ddy(inst, dst, src[0], c->key.render_to_fbo);
	 break;

      case FS_OPCODE_SPILL:
	 generate_spill(inst, src[0]);
	 break;

      case FS_OPCODE_UNSPILL:
	 generate_unspill(inst, dst);
	 break;

      case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
	 generate_uniform_pull_constant_load(inst, dst, src[0], src[1]);
	 break;

      case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GEN7:
	 generate_uniform_pull_constant_load_gen7(inst, dst, src[0], src[1]);
	 break;

      case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD:
	 generate_varying_pull_constant_load(inst, dst, src[0]);
	 break;

      case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7:
	 generate_varying_pull_constant_load_gen7(inst, dst, src[0], src[1]);
	 break;

      case FS_OPCODE_FB_WRITE:
	 generate_fb_write(inst);
	 break;

      case FS_OPCODE_MOV_DISPATCH_TO_FLAGS:
         generate_mov_dispatch_to_flags(inst);
         break;

      case FS_OPCODE_DISCARD_JUMP:
         generate_discard_jump(inst);
         break;

      case SHADER_OPCODE_SHADER_TIME_ADD:
         brw_shader_time_add(p, inst->base_mrf, SURF_INDEX_WM_SHADER_TIME);
         break;

      case FS_OPCODE_SET_GLOBAL_OFFSET:
         generate_set_global_offset(inst, dst, src[0], src[1]);
         break;

      case FS_OPCODE_PACK_HALF_2x16_SPLIT:
          generate_pack_half_2x16_split(inst, dst, src[0], src[1]);
          break;

      case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_X:
      case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y:
         generate_unpack_half_2x16_split(inst, dst, src[0]);
         break;

      default:
	 if (inst->opcode < (int) ARRAY_SIZE(opcode_descs)) {
	    _mesa_problem(ctx, "Unsupported opcode `%s' in FS",
			  opcode_descs[inst->opcode].name);
	 } else {
	    _mesa_problem(ctx, "Unsupported opcode %d in FS", inst->opcode);
	 }
	 abort();
      }

      if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
	 brw_dump_compile(p, stdout,
			  last_native_insn_offset, p->next_insn_offset);

	 foreach_list(node, &cfg->block_list) {
	    bblock_link *link = (bblock_link *)node;
	    bblock_t *block = link->block;

	    if (block->end == inst) {
	       printf("   END B%d", block->block_num);
	       foreach_list(successor_node, &block->children) {
		  bblock_link *successor_link =
		     (bblock_link *)successor_node;
		  bblock_t *successor_block = successor_link->block;
		  printf(" ->B%d", successor_block->block_num);
	       }
	       printf("\n");
	    }
	 }
      }

      last_native_insn_offset = p->next_insn_offset;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("\n");
   }

   brw_set_uip_jip(p);

   /* OK, while the INTEL_DEBUG=wm above is very nice for debugging FS
    * emit issues, it doesn't get the jump distances into the output,
    * which is often something we want to debug.  So this is here in
    * case you're doing that.
    */
   if (0) {
      brw_dump_compile(p, stdout, 0, p->next_insn_offset);
   }
}

const unsigned *
fs_generator::generate_assembly(exec_list *simd8_instructions,
                                exec_list *simd16_instructions,
                                unsigned *assembly_size)
{
   dispatch_width = 8;
   generate_code(simd8_instructions);

   if (simd16_instructions) {
      /* We have to do a compaction pass now, or the one at the end of
       * execution will squash down where our prog_offset start needs
       * to be.
       */
      brw_compact_instructions(p);

      /* align to 64 byte boundary. */
      while ((p->nr_insn * sizeof(struct brw_instruction)) % 64) {
         brw_NOP(p);
      }

      /* Save off the start of this 16-wide program */
      c->prog_data.prog_offset_16 = p->nr_insn * sizeof(struct brw_instruction);

      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);

      dispatch_width = 16;
      generate_code(simd16_instructions);
   }

   return brw_get_program(p, assembly_size);
}
