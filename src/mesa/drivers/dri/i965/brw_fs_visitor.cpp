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

/** @file brw_fs_visitor.cpp
 *
 * This file supports generating the FS LIR from the GLSL IR.  The LIR
 * makes it easier to do backend-specific optimizations than doing so
 * in the GLSL IR or in the native code.
 */
extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "brw_shader.h"
#include "brw_fs.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

void
fs_visitor::visit(ir_variable *ir)
{
   fs_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   if (strcmp(ir->name, "gl_FragColor") == 0) {
      this->frag_color = ir;
   } else if (strcmp(ir->name, "gl_FragData") == 0) {
      this->frag_data = ir;
   } else if (strcmp(ir->name, "gl_FragDepth") == 0) {
      this->frag_depth = ir;
   }

   if (ir->mode == ir_var_in) {
      if (!strcmp(ir->name, "gl_FragCoord")) {
	 reg = emit_fragcoord_interpolation(ir);
      } else if (!strcmp(ir->name, "gl_FrontFacing")) {
	 reg = emit_frontfacing_interpolation(ir);
      } else {
	 reg = emit_general_interpolation(ir);
      }
      assert(reg);
      hash_table_insert(this->variable_ht, reg, ir);
      return;
   }

   if (ir->mode == ir_var_uniform) {
      int param_index = c->prog_data.nr_params;

      if (c->dispatch_width == 16) {
	 if (!variable_storage(ir)) {
	    fail("Failed to find uniform '%s' in 16-wide\n", ir->name);
	 }
	 return;
      }

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }

      reg = new(this->mem_ctx) fs_reg(UNIFORM, param_index);
      reg->type = brw_type_for_base_type(ir->type);
   }

   if (!reg)
      reg = new(this->mem_ctx) fs_reg(this, ir->type);

   hash_table_insert(this->variable_ht, reg, ir);
}

void
fs_visitor::visit(ir_dereference_variable *ir)
{
   fs_reg *reg = variable_storage(ir->var);
   this->result = *reg;
}

void
fs_visitor::visit(ir_dereference_record *ir)
{
   const glsl_type *struct_type = ir->record->type;

   ir->record->accept(this);

   unsigned int offset = 0;
   for (unsigned int i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   this->result.reg_offset += offset;
   this->result.type = brw_type_for_base_type(ir->type);
}

void
fs_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   int element_size;

   ir->array->accept(this);
   index = ir->array_index->as_constant();

   element_size = type_size(ir->type);
   this->result.type = brw_type_for_base_type(ir->type);

   if (index) {
      assert(this->result.file == UNIFORM ||
	     (this->result.file == GRF &&
	      this->result.reg != 0));
      this->result.reg_offset += index->value.i[0] * element_size;
   } else {
      assert(!"FINISHME: non-constant array element");
   }
}

/* Instruction selection: Produce a MOV.sat instead of
 * MIN(MAX(val, 0), 1) when possible.
 */
bool
fs_visitor::try_emit_saturate(ir_expression *ir)
{
   ir_rvalue *sat_val = ir->as_rvalue_to_saturate();

   if (!sat_val)
      return false;

   this->result = reg_undef;
   sat_val->accept(this);
   fs_reg src = this->result;

   this->result = fs_reg(this, ir->type);
   fs_inst *inst = emit(BRW_OPCODE_MOV, this->result, src);
   inst->saturate = true;

   return true;
}

void
fs_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   fs_reg op[2], temp;
   fs_inst *inst;

   assert(ir->get_num_operands() <= 2);

   if (try_emit_saturate(ir))
      return;

   /* This is where our caller would like us to put the result, if possible. */
   fs_reg saved_result_storage = this->result;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result = reg_undef;
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 ir_print_visitor v;
	 fail("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
      /* And then those vector operands should have been broken down to scalar.
       */
      assert(!ir->operands[operand]->type->is_vector());
   }

   /* Inherit storage from our parent if possible, and otherwise we
    * alloc a temporary.
    */
   if (saved_result_storage.file == BAD_FILE) {
      this->result = fs_reg(this, ir->type);
   } else {
      this->result = saved_result_storage;
   }

   switch (ir->operation) {
   case ir_unop_logic_not:
      /* Note that BRW_OPCODE_NOT is not appropriate here, since it is
       * ones complement of the whole register, not just bit 0.
       */
      emit(BRW_OPCODE_XOR, this->result, op[0], fs_reg(1));
      break;
   case ir_unop_neg:
      op[0].negate = !op[0].negate;
      this->result = op[0];
      break;
   case ir_unop_abs:
      op[0].abs = true;
      op[0].negate = false;
      this->result = op[0];
      break;
   case ir_unop_sign:
      temp = fs_reg(this, ir->type);

      /* Unalias the destination.  (imagine a = sign(a)) */
      this->result = fs_reg(this, ir->type);

      emit(BRW_OPCODE_MOV, this->result, fs_reg(0.0f));

      inst = emit(BRW_OPCODE_CMP, reg_null_f, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      inst = emit(BRW_OPCODE_MOV, this->result, fs_reg(1.0f));
      inst->predicated = true;

      inst = emit(BRW_OPCODE_CMP, reg_null_f, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      inst = emit(BRW_OPCODE_MOV, this->result, fs_reg(-1.0f));
      inst->predicated = true;

      break;
   case ir_unop_rcp:
      emit_math(FS_OPCODE_RCP, this->result, op[0]);
      break;

   case ir_unop_exp2:
      emit_math(FS_OPCODE_EXP2, this->result, op[0]);
      break;
   case ir_unop_log2:
      emit_math(FS_OPCODE_LOG2, this->result, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
   case ir_unop_sin_reduced:
      emit_math(FS_OPCODE_SIN, this->result, op[0]);
      break;
   case ir_unop_cos:
   case ir_unop_cos_reduced:
      emit_math(FS_OPCODE_COS, this->result, op[0]);
      break;

   case ir_unop_dFdx:
      emit(FS_OPCODE_DDX, this->result, op[0]);
      break;
   case ir_unop_dFdy:
      emit(FS_OPCODE_DDY, this->result, op[0]);
      break;

   case ir_binop_add:
      emit(BRW_OPCODE_ADD, this->result, op[0], op[1]);
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      emit(BRW_OPCODE_MUL, this->result, op[0], op[1]);
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
      break;
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_all_equal:
   case ir_binop_nequal:
   case ir_binop_any_nequal:
      temp = this->result;
      /* original gen4 does implicit conversion before comparison. */
      if (intel->gen < 5)
	 temp.type = op[0].type;

      inst = emit(BRW_OPCODE_CMP, temp, op[0], op[1]);
      inst->conditional_mod = brw_conditional_for_comparison(ir->operation);
      emit(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1));
      break;

   case ir_binop_logic_xor:
      emit(BRW_OPCODE_XOR, this->result, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      emit(BRW_OPCODE_OR, this->result, op[0], op[1]);
      break;

   case ir_binop_logic_and:
      emit(BRW_OPCODE_AND, this->result, op[0], op[1]);
      break;

   case ir_binop_dot:
   case ir_unop_any:
      assert(!"not reached: should be handled by brw_fs_channel_expressions");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_quadop_vector:
      assert(!"not reached: should be handled by lower_quadop_vector");
      break;

   case ir_unop_sqrt:
      emit_math(FS_OPCODE_SQRT, this->result, op[0]);
      break;

   case ir_unop_rsq:
      emit_math(FS_OPCODE_RSQ, this->result, op[0]);
      break;

   case ir_unop_i2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
   case ir_unop_f2i:
      emit(BRW_OPCODE_MOV, this->result, op[0]);
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      temp = this->result;
      /* original gen4 does implicit conversion before comparison. */
      if (intel->gen < 5)
	 temp.type = op[0].type;

      inst = emit(BRW_OPCODE_CMP, temp, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      inst = emit(BRW_OPCODE_AND, this->result, this->result, fs_reg(1));
      break;

   case ir_unop_trunc:
      emit(BRW_OPCODE_RNDZ, this->result, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = !op[0].negate;
      inst = emit(BRW_OPCODE_RNDD, this->result, op[0]);
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(BRW_OPCODE_RNDD, this->result, op[0]);
      break;
   case ir_unop_fract:
      inst = emit(BRW_OPCODE_FRC, this->result, op[0]);
      break;
   case ir_unop_round_even:
      emit(BRW_OPCODE_RNDE, this->result, op[0]);
      break;

   case ir_binop_min:
      /* Unalias the destination */
      this->result = fs_reg(this, ir->type);

      inst = emit(BRW_OPCODE_CMP, this->result, op[0], op[1]);
      inst->conditional_mod = BRW_CONDITIONAL_L;

      inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
      inst->predicated = true;
      break;
   case ir_binop_max:
      /* Unalias the destination */
      this->result = fs_reg(this, ir->type);

      inst = emit(BRW_OPCODE_CMP, this->result, op[0], op[1]);
      inst->conditional_mod = BRW_CONDITIONAL_G;

      inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
      inst->predicated = true;
      break;

   case ir_binop_pow:
      emit_math(FS_OPCODE_POW, this->result, op[0], op[1]);
      break;

   case ir_unop_bit_not:
      inst = emit(BRW_OPCODE_NOT, this->result, op[0]);
      break;
   case ir_binop_bit_and:
      inst = emit(BRW_OPCODE_AND, this->result, op[0], op[1]);
      break;
   case ir_binop_bit_xor:
      inst = emit(BRW_OPCODE_XOR, this->result, op[0], op[1]);
      break;
   case ir_binop_bit_or:
      inst = emit(BRW_OPCODE_OR, this->result, op[0], op[1]);
      break;

   case ir_unop_u2f:
   case ir_binop_lshift:
   case ir_binop_rshift:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
}

void
fs_visitor::emit_assignment_writes(fs_reg &l, fs_reg &r,
				   const glsl_type *type, bool predicated)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      for (unsigned int i = 0; i < type->components(); i++) {
	 l.type = brw_type_for_base_type(type);
	 r.type = brw_type_for_base_type(type);

	 if (predicated || !l.equals(&r)) {
	    fs_inst *inst = emit(BRW_OPCODE_MOV, l, r);
	    inst->predicated = predicated;
	 }

	 l.reg_offset++;
	 r.reg_offset++;
      }
      break;
   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_assignment_writes(l, r, type->fields.array, predicated);
      }
      break;

   case GLSL_TYPE_STRUCT:
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_assignment_writes(l, r, type->fields.structure[i].type,
				predicated);
      }
      break;

   case GLSL_TYPE_SAMPLER:
      break;

   default:
      assert(!"not reached");
      break;
   }
}

void
fs_visitor::visit(ir_assignment *ir)
{
   struct fs_reg l, r;
   fs_inst *inst;

   /* FINISHME: arrays on the lhs */
   this->result = reg_undef;
   ir->lhs->accept(this);
   l = this->result;

   /* If we're doing a direct assignment, an RHS expression could
    * drop its result right into our destination.  Otherwise, tell it
    * not to.
    */
   if (ir->condition ||
       !(ir->lhs->type->is_scalar() ||
	 (ir->lhs->type->is_vector() &&
	  ir->write_mask == (1 << ir->lhs->type->vector_elements) - 1))) {
      this->result = reg_undef;
   }

   ir->rhs->accept(this);
   r = this->result;

   assert(l.file != BAD_FILE);
   assert(r.file != BAD_FILE);

   if (ir->condition) {
      emit_bool_to_cond_code(ir->condition);
   }

   if (ir->lhs->type->is_scalar() ||
       ir->lhs->type->is_vector()) {
      for (int i = 0; i < ir->lhs->type->vector_elements; i++) {
	 if (ir->write_mask & (1 << i)) {
	    if (ir->condition) {
	       inst = emit(BRW_OPCODE_MOV, l, r);
	       inst->predicated = true;
	    } else if (!l.equals(&r)) {
	       inst = emit(BRW_OPCODE_MOV, l, r);
	    }

	    r.reg_offset++;
	 }
	 l.reg_offset++;
      }
   } else {
      emit_assignment_writes(l, r, ir->lhs->type, ir->condition != NULL);
   }
}

fs_inst *
fs_visitor::emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler)
{
   int mlen;
   int base_mrf = 1;
   bool simd16 = false;
   fs_reg orig_dst;

   /* g0 header. */
   mlen = 1;

   if (ir->shadow_comparitor) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 fs_inst *inst = emit(BRW_OPCODE_MOV,
			      fs_reg(MRF, base_mrf + mlen + i), coordinate);
	 if (i < 3 && c->key.gl_clamp_mask[i] & (1 << sampler))
	    inst->saturate = true;

	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;

      if (ir->op == ir_tex) {
	 /* There's no plain shadow compare message, so we use shadow
	  * compare with a bias of 0.0.
	  */
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), fs_reg(0.0f));
	 mlen++;
      } else if (ir->op == ir_txb) {
	 this->result = reg_undef;
	 ir->lod_info.bias->accept(this);
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
	 mlen++;
      } else {
	 assert(ir->op == ir_txl);
	 this->result = reg_undef;
	 ir->lod_info.lod->accept(this);
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
	 mlen++;
      }

      this->result = reg_undef;
      ir->shadow_comparitor->accept(this);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen++;
   } else if (ir->op == ir_tex) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 fs_inst *inst = emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i),
			      coordinate);
	 if (i < 3 && c->key.gl_clamp_mask[i] & (1 << sampler))
	    inst->saturate = true;
	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;
   } else if (ir->op == ir_txd) {
      assert(!"TXD isn't supported on gen4 yet.");
   } else {
      /* Oh joy.  gen4 doesn't have SIMD8 non-shadow-compare bias/lod
       * instructions.  We'll need to do SIMD16 here.
       */
      assert(ir->op == ir_txb || ir->op == ir_txl);

      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 fs_inst *inst = emit(BRW_OPCODE_MOV, fs_reg(MRF,
						     base_mrf + mlen + i * 2),
			      coordinate);
	 if (i < 3 && c->key.gl_clamp_mask[i] & (1 << sampler))
	    inst->saturate = true;
	 coordinate.reg_offset++;
      }

      /* lod/bias appears after u/v/r. */
      mlen += 6;

      if (ir->op == ir_txb) {
	 this->result = reg_undef;
	 ir->lod_info.bias->accept(this);
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
	 mlen++;
      } else {
	 this->result = reg_undef;
	 ir->lod_info.lod->accept(this);
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
	 mlen++;
      }

      /* The unused upper half. */
      mlen++;

      /* Now, since we're doing simd16, the return is 2 interleaved
       * vec4s where the odd-indexed ones are junk. We'll need to move
       * this weirdness around to the expected layout.
       */
      simd16 = true;
      orig_dst = dst;
      dst = fs_reg(this, glsl_type::get_array_instance(glsl_type::vec4_type,
						       2));
      dst.type = BRW_REGISTER_TYPE_F;
   }

   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex:
      inst = emit(FS_OPCODE_TEX, dst);
      break;
   case ir_txb:
      inst = emit(FS_OPCODE_TXB, dst);
      break;
   case ir_txl:
      inst = emit(FS_OPCODE_TXL, dst);
      break;
   case ir_txd:
      inst = emit(FS_OPCODE_TXD, dst);
      break;
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = true;

   if (simd16) {
      for (int i = 0; i < 4; i++) {
	 emit(BRW_OPCODE_MOV, orig_dst, dst);
	 orig_dst.reg_offset++;
	 dst.reg_offset += 2;
      }
   }

   return inst;
}

/* gen5's sampler has slots for u, v, r, array index, then optional
 * parameters like shadow comparitor or LOD bias.  If optional
 * parameters aren't present, those base slots are optional and don't
 * need to be included in the message.
 *
 * We don't fill in the unnecessary slots regardless, which may look
 * surprising in the disassembly.
 */
fs_inst *
fs_visitor::emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler)
{
   int mlen = 0;
   int base_mrf = 2;
   int reg_width = c->dispatch_width / 8;
   bool header_present = false;

   if (ir->offset) {
      /* The offsets set up by the ir_texture visitor are in the
       * m1 header, so we can't go headerless.
       */
      header_present = true;
      mlen++;
      base_mrf--;
   }

   for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
      fs_inst *inst = emit(BRW_OPCODE_MOV,
			   fs_reg(MRF, base_mrf + mlen + i * reg_width),
			   coordinate);
      if (i < 3 && c->key.gl_clamp_mask[i] & (1 << sampler))
	 inst->saturate = true;
      coordinate.reg_offset++;
   }
   mlen += ir->coordinate->type->vector_elements * reg_width;

   if (ir->shadow_comparitor) {
      mlen = MAX2(mlen, header_present + 4 * reg_width);

      this->result = reg_undef;
      ir->shadow_comparitor->accept(this);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;
   }

   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex:
      inst = emit(FS_OPCODE_TEX, dst);
      break;
   case ir_txb:
      this->result = reg_undef;
      ir->lod_info.bias->accept(this);
      mlen = MAX2(mlen, header_present + 4 * reg_width);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;

      inst = emit(FS_OPCODE_TXB, dst);

      break;
   case ir_txl:
      this->result = reg_undef;
      ir->lod_info.lod->accept(this);
      mlen = MAX2(mlen, header_present + 4 * reg_width);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;

      inst = emit(FS_OPCODE_TXL, dst);
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = header_present;

   if (mlen > 11) {
      fail("Message length >11 disallowed by hardware\n");
   }

   return inst;
}

fs_inst *
fs_visitor::emit_texture_gen7(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler)
{
   int mlen = 0;
   int base_mrf = 2;
   int reg_width = c->dispatch_width / 8;
   bool header_present = false;

   if (ir->offset) {
      /* The offsets set up by the ir_texture visitor are in the
       * m1 header, so we can't go headerless.
       */
      header_present = true;
      mlen++;
      base_mrf--;
   }

   if (ir->shadow_comparitor) {
      ir->shadow_comparitor->accept(this);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;
   }

   /* Set up the LOD info */
   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;
      break;
   case ir_txl:
      ir->lod_info.lod->accept(this);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result);
      mlen += reg_width;
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   /* Set up the coordinate */
   for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
      fs_inst *inst = emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
			   coordinate);
      if (i < 3 && c->key.gl_clamp_mask[i] & (1 << sampler))
	 inst->saturate = true;
      coordinate.reg_offset++;
      mlen += reg_width;
   }

   /* Generate the SEND */
   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex: inst = emit(FS_OPCODE_TEX, dst); break;
   case ir_txb: inst = emit(FS_OPCODE_TXB, dst); break;
   case ir_txl: inst = emit(FS_OPCODE_TXL, dst); break;
   case ir_txd: inst = emit(FS_OPCODE_TXD, dst); break;
   case ir_txf: assert(!"TXF unsupported.");
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = header_present;

   if (mlen > 11) {
      fail("Message length >11 disallowed by hardware\n");
   }

   return inst;
}

void
fs_visitor::visit(ir_texture *ir)
{
   int sampler;
   fs_inst *inst = NULL;

   this->result = reg_undef;
   ir->coordinate->accept(this);
   fs_reg coordinate = this->result;

   if (ir->offset != NULL) {
      ir_constant *offset = ir->offset->as_constant();
      assert(offset != NULL);

      signed char offsets[3];
      for (unsigned i = 0; i < ir->offset->type->vector_elements; i++)
	 offsets[i] = (signed char) offset->value.i[i];

      /* Combine all three offsets into a single unsigned dword:
       *
       *    bits 11:8 - U Offset (X component)
       *    bits  7:4 - V Offset (Y component)
       *    bits  3:0 - R Offset (Z component)
       */
      unsigned offset_bits = 0;
      for (unsigned i = 0; i < ir->offset->type->vector_elements; i++) {
	 const unsigned shift = 4 * (2 - i);
	 offset_bits |= (offsets[i] << shift) & (0xF << shift);
      }

      /* Explicitly set up the message header by copying g0 to msg reg m1. */
      emit(BRW_OPCODE_MOV, fs_reg(MRF, 1, BRW_REGISTER_TYPE_UD),
	   fs_reg(GRF, 0, BRW_REGISTER_TYPE_UD));

      /* Then set the offset bits in DWord 2 of the message header. */
      emit(BRW_OPCODE_MOV,
	   fs_reg(retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, 1, 2),
			 BRW_REGISTER_TYPE_UD)),
	   fs_reg(brw_imm_uw(offset_bits)));
   }

   /* Should be lowered by do_lower_texture_projection */
   assert(!ir->projector);

   sampler = _mesa_get_sampler_uniform_value(ir->sampler,
					     prog,
					     &fp->Base);
   sampler = fp->Base.SamplerUnits[sampler];

   /* The 965 requires the EU to do the normalization of GL rectangle
    * texture coordinates.  We use the program parameter state
    * tracking to get the scaling factor.
    */
   if (ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_RECT) {
      struct gl_program_parameter_list *params = c->fp->program.Base.Parameters;
      int tokens[STATE_LENGTH] = {
	 STATE_INTERNAL,
	 STATE_TEXRECT_SCALE,
	 sampler,
	 0,
	 0
      };

      if (c->dispatch_width == 16) {
	 fail("rectangle scale uniform setup not supported on 16-wide\n");
	 this->result = fs_reg(this, ir->type);
	 return;
      }

      c->prog_data.param_convert[c->prog_data.nr_params] =
	 PARAM_NO_CONVERT;
      c->prog_data.param_convert[c->prog_data.nr_params + 1] =
	 PARAM_NO_CONVERT;

      fs_reg scale_x = fs_reg(UNIFORM, c->prog_data.nr_params);
      fs_reg scale_y = fs_reg(UNIFORM, c->prog_data.nr_params + 1);
      GLuint index = _mesa_add_state_reference(params,
					       (gl_state_index *)tokens);

      this->param_index[c->prog_data.nr_params] = index;
      this->param_offset[c->prog_data.nr_params] = 0;
      c->prog_data.nr_params++;
      this->param_index[c->prog_data.nr_params] = index;
      this->param_offset[c->prog_data.nr_params] = 1;
      c->prog_data.nr_params++;

      fs_reg dst = fs_reg(this, ir->coordinate->type);
      fs_reg src = coordinate;
      coordinate = dst;

      emit(BRW_OPCODE_MUL, dst, src, scale_x);
      dst.reg_offset++;
      src.reg_offset++;
      emit(BRW_OPCODE_MUL, dst, src, scale_y);
   }

   /* Writemasking doesn't eliminate channels on SIMD8 texture
    * samples, so don't worry about them.
    */
   fs_reg dst = fs_reg(this, glsl_type::vec4_type);

   if (intel->gen >= 7) {
      inst = emit_texture_gen7(ir, dst, coordinate, sampler);
   } else if (intel->gen >= 5) {
      inst = emit_texture_gen5(ir, dst, coordinate, sampler);
   } else {
      inst = emit_texture_gen4(ir, dst, coordinate, sampler);
   }

   /* If there's an offset, we already set up m1.  To avoid the implied move,
    * use the null register.  Otherwise, we want an implied move from g0.
    */
   if (ir->offset != NULL || !inst->header_present)
      inst->src[0] = reg_undef;
   else
      inst->src[0] = fs_reg(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW));

   inst->sampler = sampler;

   this->result = dst;

   if (ir->shadow_comparitor)
      inst->shadow_compare = true;

   if (ir->type == glsl_type::float_type) {
      /* Ignore DEPTH_TEXTURE_MODE swizzling. */
      assert(ir->sampler->type->sampler_shadow);
   } else if (c->key.tex_swizzles[inst->sampler] != SWIZZLE_NOOP) {
      fs_reg swizzle_dst = fs_reg(this, glsl_type::vec4_type);

      for (int i = 0; i < 4; i++) {
	 int swiz = GET_SWZ(c->key.tex_swizzles[inst->sampler], i);
	 fs_reg l = swizzle_dst;
	 l.reg_offset += i;

	 if (swiz == SWIZZLE_ZERO) {
	    emit(BRW_OPCODE_MOV, l, fs_reg(0.0f));
	 } else if (swiz == SWIZZLE_ONE) {
	    emit(BRW_OPCODE_MOV, l, fs_reg(1.0f));
	 } else {
	    fs_reg r = dst;
	    r.reg_offset += GET_SWZ(c->key.tex_swizzles[inst->sampler], i);
	    emit(BRW_OPCODE_MOV, l, r);
	 }
      }
      this->result = swizzle_dst;
   }
}

void
fs_visitor::visit(ir_swizzle *ir)
{
   this->result = reg_undef;
   ir->val->accept(this);
   fs_reg val = this->result;

   if (ir->type->vector_elements == 1) {
      this->result.reg_offset += ir->mask.x;
      return;
   }

   fs_reg result = fs_reg(this, ir->type);
   this->result = result;

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      fs_reg channel = val;
      int swiz = 0;

      switch (i) {
      case 0:
	 swiz = ir->mask.x;
	 break;
      case 1:
	 swiz = ir->mask.y;
	 break;
      case 2:
	 swiz = ir->mask.z;
	 break;
      case 3:
	 swiz = ir->mask.w;
	 break;
      }

      channel.reg_offset += swiz;
      emit(BRW_OPCODE_MOV, result, channel);
      result.reg_offset++;
   }
}

void
fs_visitor::visit(ir_discard *ir)
{
   assert(ir->condition == NULL); /* FINISHME */

   emit(FS_OPCODE_DISCARD);
   kill_emitted = true;
}

void
fs_visitor::visit(ir_constant *ir)
{
   /* Set this->result to reg at the bottom of the function because some code
    * paths will cause this visitor to be applied to other fields.  This will
    * cause the value stored in this->result to be modified.
    *
    * Make reg constant so that it doesn't get accidentally modified along the
    * way.  Yes, I actually had this problem. :(
    */
   const fs_reg reg(this, ir->type);
   fs_reg dst_reg = reg;

   if (ir->type->is_array()) {
      const unsigned size = type_size(ir->type->fields.array);

      for (unsigned i = 0; i < ir->type->length; i++) {
	 this->result = reg_undef;
	 ir->array_elements[i]->accept(this);
	 fs_reg src_reg = this->result;

	 dst_reg.type = src_reg.type;
	 for (unsigned j = 0; j < size; j++) {
	    emit(BRW_OPCODE_MOV, dst_reg, src_reg);
	    src_reg.reg_offset++;
	    dst_reg.reg_offset++;
	 }
      }
   } else if (ir->type->is_record()) {
      foreach_list(node, &ir->components) {
	 ir_instruction *const field = (ir_instruction *) node;
	 const unsigned size = type_size(field->type);

	 this->result = reg_undef;
	 field->accept(this);
	 fs_reg src_reg = this->result;

	 dst_reg.type = src_reg.type;
	 for (unsigned j = 0; j < size; j++) {
	    emit(BRW_OPCODE_MOV, dst_reg, src_reg);
	    src_reg.reg_offset++;
	    dst_reg.reg_offset++;
	 }
      }
   } else {
      const unsigned size = type_size(ir->type);

      for (unsigned i = 0; i < size; i++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_FLOAT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.f[i]));
	    break;
	 case GLSL_TYPE_UINT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.u[i]));
	    break;
	 case GLSL_TYPE_INT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.i[i]));
	    break;
	 case GLSL_TYPE_BOOL:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg((int)ir->value.b[i]));
	    break;
	 default:
	    assert(!"Non-float/uint/int/bool constant");
	 }
	 dst_reg.reg_offset++;
      }
   }

   this->result = reg;
}

void
fs_visitor::emit_bool_to_cond_code(ir_rvalue *ir)
{
   ir_expression *expr = ir->as_expression();

   if (expr) {
      fs_reg op[2];
      fs_inst *inst;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 assert(expr->operands[i]->type->is_scalar());

	 this->result = reg_undef;
	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(BRW_OPCODE_AND, reg_null_d, op[0], fs_reg(1));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;

      case ir_binop_logic_xor:
	 inst = emit(BRW_OPCODE_XOR, reg_null_d, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_or:
	 inst = emit(BRW_OPCODE_OR, reg_null_d, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_and:
	 inst = emit(BRW_OPCODE_AND, reg_null_d, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_f2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, reg_null_d, op[0], fs_reg(0.0f));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, reg_null_f, op[0]);
	 }
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_i2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, reg_null_d, op[0], fs_reg(0));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, reg_null_d, op[0]);
	 }
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_all_equal:
      case ir_binop_nequal:
      case ir_binop_any_nequal:
	 inst = emit(BRW_OPCODE_CMP, reg_null_cmp, op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 break;

      default:
	 assert(!"not reached");
	 fail("bad cond code\n");
	 break;
      }
      return;
   }

   this->result = reg_undef;
   ir->accept(this);

   if (intel->gen >= 6) {
      fs_inst *inst = emit(BRW_OPCODE_AND, reg_null_d, this->result, fs_reg(1));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   } else {
      fs_inst *inst = emit(BRW_OPCODE_MOV, reg_null_d, this->result);
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }
}

/**
 * Emit a gen6 IF statement with the comparison folded into the IF
 * instruction.
 */
void
fs_visitor::emit_if_gen6(ir_if *ir)
{
   ir_expression *expr = ir->condition->as_expression();

   if (expr) {
      fs_reg op[2];
      fs_inst *inst;
      fs_reg temp;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 assert(expr->operands[i]->type->is_scalar());

	 this->result = reg_undef;
	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(BRW_OPCODE_IF, temp, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 return;

      case ir_binop_logic_xor:
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_logic_or:
	 temp = fs_reg(this, glsl_type::bool_type);
	 emit(BRW_OPCODE_OR, temp, op[0], op[1]);
	 inst = emit(BRW_OPCODE_IF, reg_null_d, temp, fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_logic_and:
	 temp = fs_reg(this, glsl_type::bool_type);
	 emit(BRW_OPCODE_AND, temp, op[0], op[1]);
	 inst = emit(BRW_OPCODE_IF, reg_null_d, temp, fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_unop_f2b:
	 inst = emit(BRW_OPCODE_IF, reg_null_f, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_unop_i2b:
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_all_equal:
      case ir_binop_nequal:
      case ir_binop_any_nequal:
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 return;
      default:
	 assert(!"not reached");
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 fail("bad condition\n");
	 return;
      }
      return;
   }

   this->result = reg_undef;
   ir->condition->accept(this);

   fs_inst *inst = emit(BRW_OPCODE_IF, reg_null_d, this->result, fs_reg(0));
   inst->conditional_mod = BRW_CONDITIONAL_NZ;
}

void
fs_visitor::visit(ir_if *ir)
{
   fs_inst *inst;

   if (intel->gen != 6 && c->dispatch_width == 16) {
      fail("Can't support (non-uniform) control flow on 16-wide\n");
   }

   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   if (intel->gen == 6) {
      emit_if_gen6(ir);
   } else {
      emit_bool_to_cond_code(ir->condition);

      inst = emit(BRW_OPCODE_IF);
      inst->predicated = true;
   }

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      this->base_ir = ir;
      this->result = reg_undef;
      ir->accept(this);
   }

   if (!ir->else_instructions.is_empty()) {
      emit(BRW_OPCODE_ELSE);

      foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 this->base_ir = ir;
	 this->result = reg_undef;
	 ir->accept(this);
      }
   }

   emit(BRW_OPCODE_ENDIF);
}

void
fs_visitor::visit(ir_loop *ir)
{
   fs_reg counter = reg_undef;

   if (c->dispatch_width == 16) {
      fail("Can't support (non-uniform) control flow on 16-wide\n");
   }

   if (ir->counter) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from) {
	 this->result = counter;

	 this->base_ir = ir->from;
	 this->result = counter;
	 ir->from->accept(this);

	 if (!this->result.equals(&counter))
	    emit(BRW_OPCODE_MOV, counter, this->result);
      }
   }

   emit(BRW_OPCODE_DO);

   if (ir->to) {
      this->base_ir = ir->to;
      this->result = reg_undef;
      ir->to->accept(this);

      fs_inst *inst = emit(BRW_OPCODE_CMP, reg_null_cmp, counter, this->result);
      inst->conditional_mod = brw_conditional_for_comparison(ir->cmp);

      inst = emit(BRW_OPCODE_BREAK);
      inst->predicated = true;
   }

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      this->base_ir = ir;
      this->result = reg_undef;
      ir->accept(this);
   }

   if (ir->increment) {
      this->base_ir = ir->increment;
      this->result = reg_undef;
      ir->increment->accept(this);
      emit(BRW_OPCODE_ADD, counter, counter, this->result);
   }

   emit(BRW_OPCODE_WHILE);
}

void
fs_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(BRW_OPCODE_BREAK);
      break;
   case ir_loop_jump::jump_continue:
      emit(BRW_OPCODE_CONTINUE);
      break;
   }
}

void
fs_visitor::visit(ir_call *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_return *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to ir_to_mesa.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      foreach_iter(exec_list_iterator, iter, sig->body) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 this->base_ir = ir;
	 this->result = reg_undef;
	 ir->accept(this);
      }
   }
}

void
fs_visitor::visit(ir_function_signature *ir)
{
   assert(!"not reached");
   (void)ir;
}

fs_inst *
fs_visitor::emit(fs_inst inst)
{
   fs_inst *list_inst = new(mem_ctx) fs_inst;
   *list_inst = inst;

   if (force_uncompressed_stack > 0)
      list_inst->force_uncompressed = true;
   else if (force_sechalf_stack > 0)
      list_inst->force_sechalf = true;

   list_inst->annotation = this->current_annotation;
   list_inst->ir = this->base_ir;

   this->instructions.push_tail(list_inst);

   return list_inst;
}

/** Emits a dummy fragment shader consisting of magenta for bringup purposes. */
void
fs_visitor::emit_dummy_fs()
{
   /* Everyone's favorite color. */
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 2), fs_reg(1.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 3), fs_reg(0.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 4), fs_reg(1.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 5), fs_reg(0.0f));

   fs_inst *write;
   write = emit(FS_OPCODE_FB_WRITE, fs_reg(0), fs_reg(0));
   write->base_mrf = 0;
}

/* The register location here is relative to the start of the URB
 * data.  It will get adjusted to be a real location before
 * generate_code() time.
 */
struct brw_reg
fs_visitor::interp_reg(int location, int channel)
{
   int regnr = urb_setup[location] * 2 + channel / 2;
   int stride = (channel & 1) * 4;

   assert(urb_setup[location] != -1);

   return brw_vec1_grf(regnr, stride);
}

/** Emits the interpolation for the varying inputs. */
void
fs_visitor::emit_interpolation_setup_gen4()
{
   this->current_annotation = "compute pixel centers";
   this->pixel_x = fs_reg(this, glsl_type::uint_type);
   this->pixel_y = fs_reg(this, glsl_type::uint_type);
   this->pixel_x.type = BRW_REGISTER_TYPE_UW;
   this->pixel_y.type = BRW_REGISTER_TYPE_UW;

   emit(FS_OPCODE_PIXEL_X, this->pixel_x);
   emit(FS_OPCODE_PIXEL_Y, this->pixel_y);

   this->current_annotation = "compute pixel deltas from v0";
   if (brw->has_pln) {
      this->delta_x = fs_reg(this, glsl_type::vec2_type);
      this->delta_y = this->delta_x;
      this->delta_y.reg_offset++;
   } else {
      this->delta_x = fs_reg(this, glsl_type::float_type);
      this->delta_y = fs_reg(this, glsl_type::float_type);
   }
   emit(BRW_OPCODE_ADD, this->delta_x,
	this->pixel_x, fs_reg(negate(brw_vec1_grf(1, 0))));
   emit(BRW_OPCODE_ADD, this->delta_y,
	this->pixel_y, fs_reg(negate(brw_vec1_grf(1, 1))));

   this->current_annotation = "compute pos.w and 1/pos.w";
   /* Compute wpos.w.  It's always in our setup, since it's needed to
    * interpolate the other attributes.
    */
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit(FS_OPCODE_LINTERP, wpos_w, this->delta_x, this->delta_y,
	interp_reg(FRAG_ATTRIB_WPOS, 3));
   /* Compute the pixel 1/W value from wpos.w. */
   this->pixel_w = fs_reg(this, glsl_type::float_type);
   emit_math(FS_OPCODE_RCP, this->pixel_w, wpos_w);
   this->current_annotation = NULL;
}

/** Emits the interpolation for the varying inputs. */
void
fs_visitor::emit_interpolation_setup_gen6()
{
   struct brw_reg g1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);

   /* If the pixel centers end up used, the setup is the same as for gen4. */
   this->current_annotation = "compute pixel centers";
   fs_reg int_pixel_x = fs_reg(this, glsl_type::uint_type);
   fs_reg int_pixel_y = fs_reg(this, glsl_type::uint_type);
   int_pixel_x.type = BRW_REGISTER_TYPE_UW;
   int_pixel_y.type = BRW_REGISTER_TYPE_UW;
   emit(BRW_OPCODE_ADD,
	int_pixel_x,
	fs_reg(stride(suboffset(g1_uw, 4), 2, 4, 0)),
	fs_reg(brw_imm_v(0x10101010)));
   emit(BRW_OPCODE_ADD,
	int_pixel_y,
	fs_reg(stride(suboffset(g1_uw, 5), 2, 4, 0)),
	fs_reg(brw_imm_v(0x11001100)));

   /* As of gen6, we can no longer mix float and int sources.  We have
    * to turn the integer pixel centers into floats for their actual
    * use.
    */
   this->pixel_x = fs_reg(this, glsl_type::float_type);
   this->pixel_y = fs_reg(this, glsl_type::float_type);
   emit(BRW_OPCODE_MOV, this->pixel_x, int_pixel_x);
   emit(BRW_OPCODE_MOV, this->pixel_y, int_pixel_y);

   this->current_annotation = "compute pos.w";
   this->pixel_w = fs_reg(brw_vec8_grf(c->source_w_reg, 0));
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit_math(FS_OPCODE_RCP, this->wpos_w, this->pixel_w);

   this->delta_x = fs_reg(brw_vec8_grf(2, 0));
   this->delta_y = fs_reg(brw_vec8_grf(3, 0));

   this->current_annotation = NULL;
}

void
fs_visitor::emit_color_write(int index, int first_color_mrf, fs_reg color)
{
   int reg_width = c->dispatch_width / 8;

   if (c->dispatch_width == 8 || intel->gen == 6) {
      /* SIMD8 write looks like:
       * m + 0: r0
       * m + 1: r1
       * m + 2: g0
       * m + 3: g1
       *
       * gen6 SIMD16 DP write looks like:
       * m + 0: r0
       * m + 1: r1
       * m + 2: g0
       * m + 3: g1
       * m + 4: b0
       * m + 5: b1
       * m + 6: a0
       * m + 7: a1
       */
      emit(BRW_OPCODE_MOV, fs_reg(MRF, first_color_mrf + index * reg_width),
	   color);
   } else {
      /* pre-gen6 SIMD16 single source DP write looks like:
       * m + 0: r0
       * m + 1: g0
       * m + 2: b0
       * m + 3: a0
       * m + 4: r1
       * m + 5: g1
       * m + 6: b1
       * m + 7: a1
       */
      if (brw->has_compr4) {
	 /* By setting the high bit of the MRF register number, we
	  * indicate that we want COMPR4 mode - instead of doing the
	  * usual destination + 1 for the second half we get
	  * destination + 4.
	  */
	 emit(BRW_OPCODE_MOV,
	      fs_reg(MRF, BRW_MRF_COMPR4 + first_color_mrf + index), color);
      } else {
	 push_force_uncompressed();
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, first_color_mrf + index), color);
	 pop_force_uncompressed();

	 push_force_sechalf();
	 color.sechalf = true;
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, first_color_mrf + index + 4), color);
	 pop_force_sechalf();
	 color.sechalf = false;
      }
   }
}

void
fs_visitor::emit_fb_writes()
{
   this->current_annotation = "FB write header";
   GLboolean header_present = GL_TRUE;
   int nr = 0;
   int reg_width = c->dispatch_width / 8;

   if (intel->gen >= 6 &&
       !this->kill_emitted &&
       c->key.nr_color_regions == 1) {
      header_present = false;
   }

   if (header_present) {
      /* m0, m1 header */
      nr += 2;
   }

   if (c->aa_dest_stencil_reg) {
      push_force_uncompressed();
      emit(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
	   fs_reg(brw_vec8_grf(c->aa_dest_stencil_reg, 0)));
      pop_force_uncompressed();
   }

   /* Reserve space for color. It'll be filled in per MRT below. */
   int color_mrf = nr;
   nr += 4 * reg_width;

   if (c->source_depth_to_render_target) {
      if (intel->gen == 6 && c->dispatch_width == 16) {
	 /* For outputting oDepth on gen6, SIMD8 writes have to be
	  * used.  This would require 8-wide moves of each half to
	  * message regs, kind of like pre-gen5 SIMD16 FB writes.
	  * Just bail on doing so for now.
	  */
	 fail("Missing support for simd16 depth writes on gen6\n");
      }

      if (c->computes_depth) {
	 /* Hand over gl_FragDepth. */
	 assert(this->frag_depth);
	 fs_reg depth = *(variable_storage(this->frag_depth));

	 emit(BRW_OPCODE_MOV, fs_reg(MRF, nr), depth);
      } else {
	 /* Pass through the payload depth. */
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, nr),
	      fs_reg(brw_vec8_grf(c->source_depth_reg, 0)));
      }
      nr += reg_width;
   }

   if (c->dest_depth_reg) {
      emit(BRW_OPCODE_MOV, fs_reg(MRF, nr),
	   fs_reg(brw_vec8_grf(c->dest_depth_reg, 0)));
      nr += reg_width;
   }

   fs_reg color = reg_undef;
   if (this->frag_color)
      color = *(variable_storage(this->frag_color));
   else if (this->frag_data) {
      color = *(variable_storage(this->frag_data));
      color.type = BRW_REGISTER_TYPE_F;
   }

   for (int target = 0; target < c->key.nr_color_regions; target++) {
      this->current_annotation = ralloc_asprintf(this->mem_ctx,
						 "FB write target %d",
						 target);
      if (this->frag_color || this->frag_data) {
	 for (int i = 0; i < 4; i++) {
	    emit_color_write(i, color_mrf, color);
	    color.reg_offset++;
	 }
      }

      if (this->frag_color)
	 color.reg_offset -= 4;

      fs_inst *inst = emit(FS_OPCODE_FB_WRITE);
      inst->target = target;
      inst->base_mrf = 0;
      inst->mlen = nr;
      if (target == c->key.nr_color_regions - 1)
	 inst->eot = true;
      inst->header_present = header_present;
   }

   if (c->key.nr_color_regions == 0) {
      if (c->key.alpha_test && (this->frag_color || this->frag_data)) {
	 /* If the alpha test is enabled but there's no color buffer,
	  * we still need to send alpha out the pipeline to our null
	  * renderbuffer.
	  */
	 color.reg_offset += 3;
	 emit_color_write(3, color_mrf, color);
      }

      fs_inst *inst = emit(FS_OPCODE_FB_WRITE);
      inst->base_mrf = 0;
      inst->mlen = nr;
      inst->eot = true;
      inst->header_present = header_present;
   }

   this->current_annotation = NULL;
}
