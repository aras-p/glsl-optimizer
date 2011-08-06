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
#include "../glsl/ir_print_visitor.h"

extern "C" {
#include "brw_eu.h"
};

using namespace brw;

namespace brw {

int
vec4_visitor::setup_attributes(int payload_reg)
{
   int nr_attributes;
   int attribute_map[VERT_ATTRIB_MAX];

   nr_attributes = 0;
   for (int i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (prog_data->inputs_read & BITFIELD64_BIT(i)) {
	 attribute_map[i] = payload_reg + nr_attributes;
	 nr_attributes++;
      }
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)iter.get();

      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file != ATTR)
	    continue;

	 inst->src[i].file = HW_REG;
	 inst->src[i].fixed_hw_reg = brw_vec8_grf(attribute_map[inst->src[i].reg], 0);
	 inst->src[i].fixed_hw_reg.dw1.bits.swizzle = inst->src[i].swizzle;
      }
   }

   /* The BSpec says we always have to read at least one thing from
    * the VF, and it appears that the hardware wedges otherwise.
    */
   if (nr_attributes == 0)
      nr_attributes = 1;

   prog_data->urb_read_length = (nr_attributes + 1) / 2;

   return payload_reg + nr_attributes;
}

int
vec4_visitor::setup_uniforms(int reg)
{
   /* User clip planes from curbe:
    */
   if (c->key.nr_userclip) {
      if (intel->gen >= 6) {
	 for (int i = 0; i < c->key.nr_userclip; i++) {
	    c->userplane[i] = stride(brw_vec4_grf(reg + i / 2,
						  (i % 2) * 4), 0, 4, 1);
	 }
	 reg += ALIGN(c->key.nr_userclip, 2) / 2;
      } else {
	 for (int i = 0; i < c->key.nr_userclip; i++) {
	    c->userplane[i] = stride(brw_vec4_grf(reg + (6 + i) / 2,
						  (i % 2) * 4), 0, 4, 1);
	 }
	 reg += (ALIGN(6 + c->key.nr_userclip, 4) / 4) * 2;
      }
   }

   /* The pre-gen6 VS requires that some push constants get loaded no
    * matter what, or the GPU would hang.
    */
   if (intel->gen < 6 && this->uniforms == 0) {
      this->uniform_size[this->uniforms] = 1;

      for (unsigned int i = 0; i < 4; i++) {
	 unsigned int slot = this->uniforms * 4 + i;

	 c->prog_data.param[slot] = NULL;
	 c->prog_data.param_convert[slot] = PARAM_CONVERT_ZERO;
      }

      this->uniforms++;
      reg++;
   } else {
      reg += ALIGN(uniforms, 2) / 2;
   }

   /* for now, we are not doing any elimination of unused slots, nor
    * are we packing our uniforms.
    */
   c->prog_data.nr_params = this->uniforms * 4;

   c->prog_data.curb_read_length = reg - 1;
   c->prog_data.uses_new_param_layout = true;

   return reg;
}

void
vec4_visitor::setup_payload(void)
{
   int reg = 0;

   /* The payload always contains important data in g0, which contains
    * the URB handles that are passed on to the URB write at the end
    * of the thread.  So, we always start push constants at g1.
    */
   reg++;

   reg = setup_uniforms(reg);

   reg = setup_attributes(reg);

   this->first_non_payload_grf = reg;
}

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

void
vec4_visitor::generate_math1_gen4(vec4_instruction *inst,
				  struct brw_reg dst,
				  struct brw_reg src)
{
   brw_math(p,
	    dst,
	    brw_math_function(inst->opcode),
	    BRW_MATH_SATURATE_NONE,
	    inst->base_mrf,
	    src,
	    BRW_MATH_DATA_SCALAR,
	    BRW_MATH_PRECISION_FULL);
}

void
vec4_visitor::generate_math1_gen6(vec4_instruction *inst,
				  struct brw_reg dst,
				  struct brw_reg src)
{
   brw_math(p,
	    dst,
	    brw_math_function(inst->opcode),
	    BRW_MATH_SATURATE_NONE,
	    inst->base_mrf,
	    src,
	    BRW_MATH_DATA_SCALAR,
	    BRW_MATH_PRECISION_FULL);
}

void
vec4_visitor::generate_urb_write(vec4_instruction *inst)
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
vec4_visitor::generate_vs_instruction(vec4_instruction *instruction,
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
      if (intel->gen >= 6) {
	 generate_math1_gen6(inst, dst, src[0]);
      } else {
	 generate_math1_gen4(inst, dst, src[0]);
      }
      break;

   case SHADER_OPCODE_POW:
      assert(!"finishme");
      break;

   case VS_OPCODE_URB_WRITE:
      generate_urb_write(inst);
      break;

   default:
      if (inst->opcode < (int)ARRAY_SIZE(brw_opcodes)) {
	 fail("unsupported opcode in `%s' in VS\n",
	      brw_opcodes[inst->opcode].name);
      } else {
	 fail("Unsupported opcode %d in VS", inst->opcode);
      }
   }
}

bool
vec4_visitor::run()
{
   /* Generate FS IR for main().  (the visitor only descends into
    * functions called "main").
    */
   foreach_iter(exec_list_iterator, iter, *shader->ir) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      base_ir = ir;
      ir->accept(this);
   }

   emit_urb_writes();

   if (failed)
      return false;

   setup_payload();
   reg_allocate();

   brw_set_access_mode(p, BRW_ALIGN_16);

   generate_code();

   return !failed;
}

void
vec4_visitor::generate_code()
{
   int last_native_inst = p->nr_insn;
   const char *last_annotation_string = NULL;
   ir_instruction *last_annotation_ir = NULL;

   int loop_stack_array_size = 16;
   int loop_stack_depth = 0;
   brw_instruction **loop_stack =
      rzalloc_array(this->mem_ctx, brw_instruction *, loop_stack_array_size);
   int *if_depth_in_loop =
      rzalloc_array(this->mem_ctx, int, loop_stack_array_size);


   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("Native code for vertex shader %d:\n", prog->Name);
   }

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;
      struct brw_reg src[3], dst;

      if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
	 if (last_annotation_ir != inst->ir) {
	    last_annotation_ir = inst->ir;
	    if (last_annotation_ir) {
	       printf("   ");
	       last_annotation_ir->print();
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

      case BRW_OPCODE_DP4:
	 brw_DP4(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_DP3:
	 brw_DP3(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_DP2:
	 brw_DP2(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_IF:
	 if (inst->src[0].file != BAD_FILE) {
	    /* The instruction has an embedded compare (only allowed on gen6) */
	    assert(intel->gen == 6);
	    gen6_IF(p, inst->conditional_mod, src[0], src[1]);
	 } else {
	    brw_IF(p, BRW_EXECUTE_8);
	 }
	 if_depth_in_loop[loop_stack_depth]++;
	 break;

      case BRW_OPCODE_ELSE:
	 brw_ELSE(p);
	 break;
      case BRW_OPCODE_ENDIF:
	 brw_ENDIF(p);
	 if_depth_in_loop[loop_stack_depth]--;
	 break;

      case BRW_OPCODE_DO:
	 loop_stack[loop_stack_depth++] = brw_DO(p, BRW_EXECUTE_8);
	 if (loop_stack_array_size <= loop_stack_depth) {
	    loop_stack_array_size *= 2;
	    loop_stack = reralloc(this->mem_ctx, loop_stack, brw_instruction *,
				  loop_stack_array_size);
	    if_depth_in_loop = reralloc(this->mem_ctx, if_depth_in_loop, int,
				        loop_stack_array_size);
	 }
	 if_depth_in_loop[loop_stack_depth] = 0;
	 break;

      case BRW_OPCODE_BREAK:
	 brw_BREAK(p, if_depth_in_loop[loop_stack_depth]);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;
      case BRW_OPCODE_CONTINUE:
	 /* FINISHME: We need to write the loop instruction support still. */
	 if (intel->gen >= 6)
	    gen6_CONT(p, loop_stack[loop_stack_depth - 1]);
	 else
	    brw_CONT(p, if_depth_in_loop[loop_stack_depth]);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;

      case BRW_OPCODE_WHILE: {
	 struct brw_instruction *inst0, *inst1;
	 GLuint br = 1;

	 if (intel->gen >= 5)
	    br = 2;

	 assert(loop_stack_depth > 0);
	 loop_stack_depth--;
	 inst0 = inst1 = brw_WHILE(p, loop_stack[loop_stack_depth]);
	 if (intel->gen < 6) {
	    /* patch all the BREAK/CONT instructions from last BGNLOOP */
	    while (inst0 > loop_stack[loop_stack_depth]) {
	       inst0--;
	       if (inst0->header.opcode == BRW_OPCODE_BREAK &&
		   inst0->bits3.if_else.jump_count == 0) {
		  inst0->bits3.if_else.jump_count = br * (inst1 - inst0 + 1);
	    }
	       else if (inst0->header.opcode == BRW_OPCODE_CONTINUE &&
			inst0->bits3.if_else.jump_count == 0) {
		  inst0->bits3.if_else.jump_count = br * (inst1 - inst0);
	       }
	    }
	 }
      }
	 break;

      default:
	 generate_vs_instruction(inst, dst, src);
	 break;
      }

      if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
	 for (unsigned int i = last_native_inst; i < p->nr_insn; i++) {
	    if (0) {
	       printf("0x%08x 0x%08x 0x%08x 0x%08x ",
		      ((uint32_t *)&p->store[i])[3],
		      ((uint32_t *)&p->store[i])[2],
		      ((uint32_t *)&p->store[i])[1],
		      ((uint32_t *)&p->store[i])[0]);
	    }
	    brw_disasm(stdout, &p->store[i], intel->gen);
	 }
      }

      last_native_inst = p->nr_insn;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("\n");
   }

   ralloc_free(loop_stack);
   ralloc_free(if_depth_in_loop);

   brw_set_uip_jip(p);

   /* OK, while the INTEL_DEBUG=vs above is very nice for debugging VS
    * emit issues, it doesn't get the jump distances into the output,
    * which is often something we want to debug.  So this is here in
    * case you're doing that.
    */
   if (0) {
      if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
	 for (unsigned int i = 0; i < p->nr_insn; i++) {
	    printf("0x%08x 0x%08x 0x%08x 0x%08x ",
		   ((uint32_t *)&p->store[i])[3],
		   ((uint32_t *)&p->store[i])[2],
		   ((uint32_t *)&p->store[i])[1],
		   ((uint32_t *)&p->store[i])[0]);
	    brw_disasm(stdout, &p->store[i], intel->gen);
	 }
      }
   }
}

extern "C" {

bool
brw_vs_emit(struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_shader_program *prog = ctx->Shader.CurrentVertexProgram;

   if (!prog)
      return false;

   struct brw_shader *shader =
     (brw_shader *) prog->_LinkedShaders[MESA_SHADER_VERTEX];
   if (!shader)
      return false;

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("GLSL IR for native vertex shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n\n");
   }

   vec4_visitor v(c, prog, shader);
   if (!v.run()) {
      /* FINISHME: Cleanly fail, test at link time, etc. */
      assert(!"not reached");
      return false;
   }

   return true;
}

} /* extern "C" */

} /* namespace brw */
