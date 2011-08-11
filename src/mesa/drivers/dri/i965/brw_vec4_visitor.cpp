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
extern "C" {
#include "main/macros.h"
#include "program/prog_parameter.h"
}

namespace brw {

src_reg::src_reg(dst_reg reg)
{
   init();

   this->file = reg.file;
   this->reg = reg.reg;
   this->reg_offset = reg.reg_offset;
   this->type = reg.type;
   this->reladdr = reg.reladdr;

   int swizzles[4];
   int next_chan = 0;
   int last = 0;

   for (int i = 0; i < 4; i++) {
      if (!(reg.writemask & (1 << i)))
	 continue;

      swizzles[next_chan++] = last = i;
   }

   for (; next_chan < 4; next_chan++) {
      swizzles[next_chan] = last;
   }

   this->swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
				swizzles[2], swizzles[3]);
}

dst_reg::dst_reg(src_reg reg)
{
   init();

   this->file = reg.file;
   this->reg = reg.reg;
   this->reg_offset = reg.reg_offset;
   this->type = reg.type;
   this->writemask = WRITEMASK_XYZW;
   this->reladdr = reg.reladdr;
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst,
		   src_reg src0, src_reg src1, src_reg src2)
{
   vec4_instruction *inst = new(mem_ctx) vec4_instruction();

   inst->opcode = opcode;
   inst->dst = dst;
   inst->src[0] = src0;
   inst->src[1] = src1;
   inst->src[2] = src2;
   inst->ir = this->base_ir;
   inst->annotation = this->current_annotation;

   this->instructions.push_tail(inst);

   return inst;
}


vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1)
{
   return emit(opcode, dst, src0, src1, src_reg());
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0)
{
   assert(dst.writemask != 0);
   return emit(opcode, dst, src0, src_reg(), src_reg());
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode)
{
   return emit(opcode, dst_reg(), src_reg(), src_reg(), src_reg());
}

void
vec4_visitor::emit_dp(dst_reg dst, src_reg src0, src_reg src1, unsigned elements)
{
   static enum opcode dot_opcodes[] = {
      BRW_OPCODE_DP2, BRW_OPCODE_DP3, BRW_OPCODE_DP4
   };

   emit(dot_opcodes[elements - 2], dst, src0, src1);
}

void
vec4_visitor::emit_math1_gen6(enum opcode opcode, dst_reg dst, src_reg src)
{
   /* The gen6 math instruction ignores the source modifiers --
    * swizzle, abs, negate, and at least some parts of the register
    * region description.
    */
   src_reg temp_src = src_reg(this, glsl_type::vec4_type);
   emit(BRW_OPCODE_MOV, dst_reg(temp_src), src);

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);

      emit(opcode, temp_dst, temp_src);

      emit(BRW_OPCODE_MOV, dst, src_reg(temp_dst));
   } else {
      emit(opcode, dst, temp_src);
   }
}

void
vec4_visitor::emit_math1_gen4(enum opcode opcode, dst_reg dst, src_reg src)
{
   vec4_instruction *inst = emit(opcode, dst, src);
   inst->base_mrf = 1;
   inst->mlen = 1;
}

void
vec4_visitor::emit_math(opcode opcode, dst_reg dst, src_reg src)
{
   switch (opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      break;
   default:
      assert(!"not reached: bad math opcode");
      return;
   }

   if (intel->gen >= 6) {
      return emit_math1_gen6(opcode, dst, src);
   } else {
      return emit_math1_gen4(opcode, dst, src);
   }
}

void
vec4_visitor::emit_math2_gen6(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   src_reg expanded;

   /* The gen6 math instruction ignores the source modifiers --
    * swizzle, abs, negate, and at least some parts of the register
    * region description.  Move the sources to temporaries to make it
    * generally work.
    */

   expanded = src_reg(this, glsl_type::vec4_type);
   emit(BRW_OPCODE_MOV, dst_reg(expanded), src0);
   src0 = expanded;

   expanded = src_reg(this, glsl_type::vec4_type);
   emit(BRW_OPCODE_MOV, dst_reg(expanded), src1);
   src1 = expanded;

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);

      emit(opcode, temp_dst, src0, src1);

      emit(BRW_OPCODE_MOV, dst, src_reg(temp_dst));
   } else {
      emit(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::emit_math2_gen4(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   vec4_instruction *inst = emit(opcode, dst, src0, src1);
   inst->base_mrf = 1;
   inst->mlen = 2;
}

void
vec4_visitor::emit_math(enum opcode opcode,
			dst_reg dst, src_reg src0, src_reg src1)
{
   assert(opcode == SHADER_OPCODE_POW);

   if (intel->gen >= 6) {
      return emit_math2_gen6(opcode, dst, src0, src1);
   } else {
      return emit_math2_gen4(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::visit_instructions(const exec_list *list)
{
   foreach_iter(exec_list_iterator, iter, *list) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      base_ir = ir;
      ir->accept(this);
   }
}


static int
type_size(const struct glsl_type *type)
{
   unsigned int i;
   int size;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      if (type->is_matrix()) {
	 return type->matrix_columns;
      } else {
	 /* Regardless of size of vector, it gets a vec4. This is bad
	  * packing for things like floats, but otherwise arrays become a
	  * mess.  Hopefully a later pass over the code can pack scalars
	  * down if appropriate.
	  */
	 return 1;
      }
   case GLSL_TYPE_ARRAY:
      assert(type->length > 0);
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up one slot in UNIFORMS[], but they're baked in
       * at link time.
       */
      return 1;
   default:
      assert(0);
      return 0;
   }
}

int
vec4_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_count) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);
   }
   virtual_grf_sizes[virtual_grf_count] = size;
   return virtual_grf_count++;
}

src_reg::src_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->swizzle = BRW_SWIZZLE_NOOP;
   } else {
      this->swizzle = swizzle_for_size(type->vector_elements);
   }

   this->type = brw_type_for_base_type(type);
}

dst_reg::dst_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->writemask = WRITEMASK_XYZW;
   } else {
      this->writemask = (1 << type->vector_elements) - 1;
   }

   this->type = brw_type_for_base_type(type);
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
int
vec4_visitor::setup_uniform_values(int loc, const glsl_type *type)
{
   unsigned int offset = 0;
   float *values = &this->vp->Base.Parameters->ParameterValues[loc][0].f;

   if (type->is_matrix()) {
      const glsl_type *column = glsl_type::get_instance(GLSL_TYPE_FLOAT,
							type->vector_elements,
							1);

      for (unsigned int i = 0; i < type->matrix_columns; i++) {
	 offset += setup_uniform_values(loc + offset, column);
      }

      return offset;
   }

   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      for (unsigned int i = 0; i < type->vector_elements; i++) {
	 int slot = this->uniforms * 4 + i;
	 switch (type->base_type) {
	 case GLSL_TYPE_FLOAT:
	    c->prog_data.param_convert[slot] = PARAM_NO_CONVERT;
	    break;
	 case GLSL_TYPE_UINT:
	    c->prog_data.param_convert[slot] = PARAM_CONVERT_F2U;
	    break;
	 case GLSL_TYPE_INT:
	    c->prog_data.param_convert[slot] = PARAM_CONVERT_F2I;
	    break;
	 case GLSL_TYPE_BOOL:
	    c->prog_data.param_convert[slot] = PARAM_CONVERT_F2B;
	    break;
	 default:
	    assert(!"not reached");
	    c->prog_data.param_convert[slot] = PARAM_NO_CONVERT;
	    break;
	 }
	 c->prog_data.param[slot] = &values[i];
      }

      for (unsigned int i = type->vector_elements; i < 4; i++) {
	 c->prog_data.param_convert[this->uniforms * 4 + i] =
	    PARAM_CONVERT_ZERO;
	 c->prog_data.param[this->uniforms * 4 + i] = NULL;
      }

      this->uniform_size[this->uniforms] = type->vector_elements;
      this->uniforms++;

      return 1;

   case GLSL_TYPE_STRUCT:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset,
					type->fields.structure[i].type);
      }
      return offset;

   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset, type->fields.array);
      }
      return offset;

   case GLSL_TYPE_SAMPLER:
      /* The sampler takes up a slot, but we don't use any values from it. */
      return 1;

   default:
      assert(!"not reached");
      return 0;
   }
}

/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
vec4_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa,
       * but we'll get the same index back here.  We can reference
       * ParameterValues directly, since unlike brw_fs.cpp, we never
       * add new state references during compile.
       */
      int index = _mesa_add_state_reference(this->vp->Base.Parameters,
					    (gl_state_index *)slots[i].tokens);
      float *values = &this->vp->Base.Parameters->ParameterValues[index][0].f;

      this->uniform_size[this->uniforms] = 0;
      /* Add each of the unique swizzled channels of the element.
       * This will end up matching the size of the glsl_type of this field.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 last_swiz = swiz;

	 c->prog_data.param[this->uniforms * 4 + j] = &values[swiz];
	 c->prog_data.param_convert[this->uniforms * 4 + j] = PARAM_NO_CONVERT;
	 if (swiz <= last_swiz)
	    this->uniform_size[this->uniforms]++;
      }
      this->uniforms++;
   }
}

dst_reg *
vec4_visitor::variable_storage(ir_variable *var)
{
   return (dst_reg *)hash_table_find(this->variable_ht, var);
}

void
vec4_visitor::emit_bool_to_cond_code(ir_rvalue *ir)
{
   ir_expression *expr = ir->as_expression();

   if (expr) {
      src_reg op[2];
      vec4_instruction *inst;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 assert(expr->operands[i]->type->is_scalar());

	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(BRW_OPCODE_AND, dst_null_d(), op[0], src_reg(1));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;

      case ir_binop_logic_xor:
	 inst = emit(BRW_OPCODE_XOR, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_or:
	 inst = emit(BRW_OPCODE_OR, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_and:
	 inst = emit(BRW_OPCODE_AND, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_f2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], src_reg(0.0f));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, dst_null_f(), op[0]);
	 }
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_i2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], src_reg(0));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, dst_null_d(), op[0]);
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
	 inst = emit(BRW_OPCODE_CMP, dst_null_cmp(), op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 break;

      default:
	 assert(!"not reached");
	 break;
      }
      return;
   }

   ir->accept(this);

   if (intel->gen >= 6) {
      vec4_instruction *inst = emit(BRW_OPCODE_AND, dst_null_d(),
			       this->result, src_reg(1));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   } else {
      vec4_instruction *inst = emit(BRW_OPCODE_MOV, dst_null_d(), this->result);
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }
}

/**
 * Emit a gen6 IF statement with the comparison folded into the IF
 * instruction.
 */
void
vec4_visitor::emit_if_gen6(ir_if *ir)
{
   ir_expression *expr = ir->condition->as_expression();

   if (expr) {
      src_reg op[2];
      vec4_instruction *inst;
      dst_reg temp;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), op[0], src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 return;

      case ir_binop_logic_xor:
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_logic_or:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(BRW_OPCODE_OR, temp, op[0], op[1]);
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), src_reg(temp), src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_logic_and:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(BRW_OPCODE_AND, temp, op[0], op[1]);
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), src_reg(temp), src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_unop_f2b:
	 inst = emit(BRW_OPCODE_IF, dst_null_f(), op[0], src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_unop_i2b:
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), op[0], src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_nequal:
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 return;

      case ir_binop_all_equal:
	 inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_Z;

	 inst = emit(BRW_OPCODE_IF);
	 inst->predicate = BRW_PREDICATE_ALIGN16_ALL4H;
	 return;

      case ir_binop_any_nequal:
	 inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;

	 inst = emit(BRW_OPCODE_IF);
	 inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 return;

      case ir_unop_any:
	 inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;

	 inst = emit(BRW_OPCODE_IF);
	 inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 return;

      default:
	 assert(!"not reached");
	 inst = emit(BRW_OPCODE_IF, dst_null_d(), op[0], src_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;
      }
      return;
   }

   ir->condition->accept(this);

   vec4_instruction *inst = emit(BRW_OPCODE_IF, dst_null_d(),
			    this->result, src_reg(0));
   inst->conditional_mod = BRW_CONDITIONAL_NZ;
}

void
vec4_visitor::visit(ir_variable *ir)
{
   dst_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   switch (ir->mode) {
   case ir_var_in:
      reg = new(mem_ctx) dst_reg(ATTR, ir->location);
      break;

   case ir_var_out:
      reg = new(mem_ctx) dst_reg(this, ir->type);

      for (int i = 0; i < type_size(ir->type); i++) {
	 output_reg[ir->location + i] = *reg;
	 output_reg[ir->location + i].reg_offset = i;
	 output_reg[ir->location + i].type = BRW_REGISTER_TYPE_F;
      }
      break;

   case ir_var_auto:
   case ir_var_temporary:
      reg = new(mem_ctx) dst_reg(this, ir->type);
      break;

   case ir_var_uniform:
      reg = new(this->mem_ctx) dst_reg(UNIFORM, this->uniforms);

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }
      break;

   default:
      assert(!"not reached");
   }

   reg->type = brw_type_for_base_type(ir->type);
   hash_table_insert(this->variable_ht, reg, ir);
}

void
vec4_visitor::visit(ir_loop *ir)
{
   dst_reg counter;

   /* We don't want debugging output to print the whole body of the
    * loop as the annotation.
    */
   this->base_ir = NULL;

   if (ir->counter != NULL) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from != NULL) {
	 this->base_ir = ir->from;
	 ir->from->accept(this);

	 emit(BRW_OPCODE_MOV, counter, this->result);
      }
   }

   emit(BRW_OPCODE_DO);

   if (ir->to) {
      this->base_ir = ir->to;
      ir->to->accept(this);

      vec4_instruction *inst = emit(BRW_OPCODE_CMP, dst_null_d(),
				    src_reg(counter), this->result);
      inst->conditional_mod = brw_conditional_for_comparison(ir->cmp);

      inst = emit(BRW_OPCODE_BREAK);
      inst->predicate = BRW_PREDICATE_NORMAL;
   }

   visit_instructions(&ir->body_instructions);


   if (ir->increment) {
      this->base_ir = ir->increment;
      ir->increment->accept(this);
      emit(BRW_OPCODE_ADD, counter, src_reg(counter), this->result);
   }

   emit(BRW_OPCODE_WHILE);
}

void
vec4_visitor::visit(ir_loop_jump *ir)
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
vec4_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
vec4_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      visit_instructions(&sig->body);
   }
}

GLboolean
vec4_visitor::try_emit_sat(ir_expression *ir)
{
   ir_rvalue *sat_src = ir->as_rvalue_to_saturate();
   if (!sat_src)
      return false;

   sat_src->accept(this);
   src_reg src = this->result;

   this->result = src_reg(this, ir->type);
   vec4_instruction *inst;
   inst = emit(BRW_OPCODE_MOV, dst_reg(this->result), src);
   inst->saturate = true;

   return true;
}

void
vec4_visitor::emit_bool_comparison(unsigned int op,
				 dst_reg dst, src_reg src0, src_reg src1)
{
   /* original gen4 does destination conversion before comparison. */
   if (intel->gen < 5)
      dst.type = src0.type;

   vec4_instruction *inst = emit(BRW_OPCODE_CMP, dst, src0, src1);
   inst->conditional_mod = brw_conditional_for_comparison(op);

   dst.type = BRW_REGISTER_TYPE_D;
   emit(BRW_OPCODE_AND, dst, src_reg(dst), src_reg(0x1));
}

void
vec4_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   src_reg op[Elements(ir->operands)];
   src_reg result_src;
   dst_reg result_dst;
   vec4_instruction *inst;

   if (try_emit_sat(ir))
      return;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result.file = BAD_FILE;
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->print();
	 exit(1);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
   }

   int vector_elements = ir->operands[0]->type->vector_elements;
   if (ir->operands[1]) {
      vector_elements = MAX2(vector_elements,
			     ir->operands[1]->type->vector_elements);
   }

   this->result.file = BAD_FILE;

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = src_reg(this, ir->type);
   /* convenience for the emit functions below. */
   result_dst = dst_reg(result_src);
   /* If nothing special happens, this is the result. */
   this->result = result_src;
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      /* Note that BRW_OPCODE_NOT is not appropriate here, since it is
       * ones complement of the whole register, not just bit 0.
       */
      emit(BRW_OPCODE_XOR, result_dst, op[0], src_reg(1));
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
      emit(BRW_OPCODE_MOV, result_dst, src_reg(0.0f));

      inst = emit(BRW_OPCODE_CMP, dst_null_f(), op[0], src_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      inst = emit(BRW_OPCODE_MOV, result_dst, src_reg(1.0f));
      inst->predicate = BRW_PREDICATE_NORMAL;

      inst = emit(BRW_OPCODE_CMP, dst_null_f(), op[0], src_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      inst = emit(BRW_OPCODE_MOV, result_dst, src_reg(-1.0f));
      inst->predicate = BRW_PREDICATE_NORMAL;

      break;

   case ir_unop_rcp:
      emit_math(SHADER_OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp2:
      emit_math(SHADER_OPCODE_EXP2, result_dst, op[0]);
      break;
   case ir_unop_log2:
      emit_math(SHADER_OPCODE_LOG2, result_dst, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
   case ir_unop_sin_reduced:
      emit_math(SHADER_OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
   case ir_unop_cos_reduced:
      emit_math(SHADER_OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(!"derivatives not valid in vertex shader");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_binop_add:
      emit(BRW_OPCODE_ADD, result_dst, op[0], op[1]);
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      emit(BRW_OPCODE_MUL, result_dst, op[0], op[1]);
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal: {
      dst_reg temp = result_dst;
      /* original gen4 does implicit conversion before comparison. */
      if (intel->gen < 5)
	 temp.type = op[0].type;

      inst = emit(BRW_OPCODE_CMP, temp, op[0], op[1]);
      inst->conditional_mod = brw_conditional_for_comparison(ir->operation);
      emit(BRW_OPCODE_AND, result_dst, this->result, src_reg(0x1));
      break;
   }

   case ir_binop_all_equal:
      /* "==" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 inst = emit(BRW_OPCODE_CMP, dst_null_cmp(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_Z;

	 emit(BRW_OPCODE_MOV, result_dst, src_reg(0));
	 inst = emit(BRW_OPCODE_MOV, result_dst, src_reg(1));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ALL4H;
      } else {
	 dst_reg temp = result_dst;
	 /* original gen4 does implicit conversion before comparison. */
	 if (intel->gen < 5)
	    temp.type = op[0].type;

	 inst = emit(BRW_OPCODE_CMP, temp, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 emit(BRW_OPCODE_AND, result_dst, result_src, src_reg(0x1));
      }
      break;
   case ir_binop_any_nequal:
      /* "!=" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 inst = emit(BRW_OPCODE_CMP, dst_null_cmp(), op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;

	 emit(BRW_OPCODE_MOV, result_dst, src_reg(0));
	 inst = emit(BRW_OPCODE_MOV, result_dst, src_reg(1));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      } else {
	 dst_reg temp = result_dst;
	 /* original gen4 does implicit conversion before comparison. */
	 if (intel->gen < 5)
	    temp.type = op[0].type;

	 inst = emit(BRW_OPCODE_CMP, temp, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 emit(BRW_OPCODE_AND, result_dst, result_src, src_reg(0x1));
      }
      break;

   case ir_unop_any:
      inst = emit(BRW_OPCODE_CMP, dst_null_d(), op[0], src_reg(0));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;

      emit(BRW_OPCODE_MOV, result_dst, src_reg(0));

      inst = emit(BRW_OPCODE_MOV, result_dst, src_reg(1));
      inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      break;

   case ir_binop_logic_xor:
      emit(BRW_OPCODE_XOR, result_dst, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      emit(BRW_OPCODE_OR, result_dst, op[0], op[1]);
      break;

   case ir_binop_logic_and:
      emit(BRW_OPCODE_AND, result_dst, op[0], op[1]);
      break;

   case ir_binop_dot:
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      emit_dp(result_dst, op[0], op[1], ir->operands[0]->type->vector_elements);
      break;

   case ir_unop_sqrt:
      emit_math(SHADER_OPCODE_SQRT, result_dst, op[0]);
      break;
   case ir_unop_rsq:
      emit_math(SHADER_OPCODE_RSQ, result_dst, op[0]);
      break;
   case ir_unop_i2f:
   case ir_unop_i2u:
   case ir_unop_u2i:
   case ir_unop_u2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
   case ir_unop_f2i:
      emit(BRW_OPCODE_MOV, result_dst, op[0]);
      break;
   case ir_unop_f2b:
   case ir_unop_i2b: {
      dst_reg temp = result_dst;
      /* original gen4 does implicit conversion before comparison. */
      if (intel->gen < 5)
	 temp.type = op[0].type;

      inst = emit(BRW_OPCODE_CMP, temp, op[0], src_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      inst = emit(BRW_OPCODE_AND, result_dst, result_src, src_reg(1));
      break;
   }

   case ir_unop_trunc:
      emit(BRW_OPCODE_RNDZ, result_dst, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = !op[0].negate;
      inst = emit(BRW_OPCODE_RNDD, result_dst, op[0]);
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(BRW_OPCODE_RNDD, result_dst, op[0]);
      break;
   case ir_unop_fract:
      inst = emit(BRW_OPCODE_FRC, result_dst, op[0]);
      break;
   case ir_unop_round_even:
      emit(BRW_OPCODE_RNDE, result_dst, op[0]);
      break;

   case ir_binop_min:
      inst = emit(BRW_OPCODE_CMP, result_dst, op[0], op[1]);
      inst->conditional_mod = BRW_CONDITIONAL_L;

      inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
      inst->predicate = BRW_PREDICATE_NORMAL;
      break;
   case ir_binop_max:
      inst = emit(BRW_OPCODE_CMP, result_dst, op[0], op[1]);
      inst->conditional_mod = BRW_CONDITIONAL_G;

      inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
      inst->predicate = BRW_PREDICATE_NORMAL;
      break;

   case ir_binop_pow:
      emit_math(SHADER_OPCODE_POW, result_dst, op[0], op[1]);
      break;

   case ir_unop_bit_not:
      inst = emit(BRW_OPCODE_NOT, result_dst, op[0]);
      break;
   case ir_binop_bit_and:
      inst = emit(BRW_OPCODE_AND, result_dst, op[0], op[1]);
      break;
   case ir_binop_bit_xor:
      inst = emit(BRW_OPCODE_XOR, result_dst, op[0], op[1]);
      break;
   case ir_binop_bit_or:
      inst = emit(BRW_OPCODE_OR, result_dst, op[0], op[1]);
      break;

   case ir_binop_lshift:
   case ir_binop_rshift:
      assert(!"GLSL 1.30 features unsupported");
      break;

   case ir_quadop_vector:
      assert(!"not reached: should be handled by lower_quadop_vector");
      break;
   }
}


void
vec4_visitor::visit(ir_swizzle *ir)
{
   src_reg src;
   int i = 0;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src = this->result;
   assert(src.file != BAD_FILE);

   for (i = 0; i < ir->type->vector_elements; i++) {
      switch (i) {
      case 0:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.x);
	 break;
      case 1:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.y);
	 break;
      case 2:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.z);
	 break;
      case 3:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.w);
	    break;
      }
   }
   for (; i < 4; i++) {
      /* Replicate the last channel out. */
      swizzle[i] = swizzle[ir->type->vector_elements - 1];
   }

   src.swizzle = BRW_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_variable *ir)
{
   const struct glsl_type *type = ir->type;
   dst_reg *reg = variable_storage(ir->var);

   if (!reg) {
      fail("Failed to find variable storage for %s\n", ir->var->name);
      this->result = src_reg(brw_null_reg());
      return;
   }

   this->result = src_reg(*reg);

   if (type->is_scalar() || type->is_vector() || type->is_matrix())
      this->result.swizzle = swizzle_for_size(type->vector_elements);
}

void
vec4_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *constant_index;
   src_reg src;
   int element_size = type_size(ir->type);

   constant_index = ir->array_index->constant_expression_value();

   ir->array->accept(this);
   src = this->result;

   if (constant_index) {
      src.reg_offset += constant_index->value.i[0] * element_size;
   } else {
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      src_reg index_reg;

      if (element_size == 1) {
	 index_reg = this->result;
      } else {
	 index_reg = src_reg(this, glsl_type::int_type);

	 emit(BRW_OPCODE_MUL, dst_reg(index_reg),
	      this->result, src_reg(element_size));
      }

      if (src.reladdr) {
	 src_reg temp = src_reg(this, glsl_type::int_type);

	 emit(BRW_OPCODE_ADD, dst_reg(temp), *src.reladdr, index_reg);

	 index_reg = temp;
      }

      src.reladdr = ralloc(mem_ctx, src_reg);
      memcpy(src.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      src.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src.swizzle = BRW_SWIZZLE_NOOP;
   src.type = brw_type_for_base_type(ir->type);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_record *ir)
{
   unsigned int i;
   const glsl_type *struct_type = ir->record->type;
   int offset = 0;

   ir->record->accept(this);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      this->result.swizzle = BRW_SWIZZLE_NOOP;
   this->result.type = brw_type_for_base_type(ir->type);

   this->result.reg_offset += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 */
static dst_reg
get_assignment_lhs(ir_dereference *ir, vec4_visitor *v)
{
   /* The LHS must be a dereference.  If the LHS is a variable indexed array
    * access of a vector, it must be separated into a series conditional moves
    * before reaching this point (see ir_vec_index_to_cond_assign).
    */
   assert(ir->as_dereference());
   ir_dereference_array *deref_array = ir->as_dereference_array();
   if (deref_array) {
      assert(!deref_array->array->type->is_vector());
   }

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   return dst_reg(v->result);
}

void
vec4_visitor::emit_block_move(dst_reg *dst, src_reg *src,
			      const struct glsl_type *type, bool predicated)
{
   if (type->base_type == GLSL_TYPE_STRUCT) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.structure[i].type, predicated);
      }
      return;
   }

   if (type->is_array()) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.array, predicated);
      }
      return;
   }

   if (type->is_matrix()) {
      const struct glsl_type *vec_type;

      vec_type = glsl_type::get_instance(GLSL_TYPE_FLOAT,
					 type->vector_elements, 1);

      for (int i = 0; i < type->matrix_columns; i++) {
	 emit_block_move(dst, src, vec_type, predicated);
      }
      return;
   }

   assert(type->is_scalar() || type->is_vector());

   dst->type = brw_type_for_base_type(type);
   src->type = dst->type;

   dst->writemask = (1 << type->vector_elements) - 1;

   /* Do we need to worry about swizzling a swizzle? */
   assert(src->swizzle = BRW_SWIZZLE_NOOP);
   src->swizzle = swizzle_for_size(type->vector_elements);

   vec4_instruction *inst = emit(BRW_OPCODE_MOV, *dst, *src);
   if (predicated)
      inst->predicate = BRW_PREDICATE_NORMAL;

   dst->reg_offset++;
   src->reg_offset++;
}


/* If the RHS processing resulted in an instruction generating a
 * temporary value, and it would be easy to rewrite the instruction to
 * generate its result right into the LHS instead, do so.  This ends
 * up reliably removing instructions where it can be tricky to do so
 * later without real UD chain information.
 */
bool
vec4_visitor::try_rewrite_rhs_to_dst(ir_assignment *ir,
				     dst_reg dst,
				     src_reg src,
				     vec4_instruction *pre_rhs_inst,
				     vec4_instruction *last_rhs_inst)
{
   /* This could be supported, but it would take more smarts. */
   if (ir->condition)
      return false;

   if (pre_rhs_inst == last_rhs_inst)
      return false; /* No instructions generated to work with. */

   /* Make sure the last instruction generated our source reg. */
   if (src.file != GRF ||
       src.file != last_rhs_inst->dst.file ||
       src.reg != last_rhs_inst->dst.reg ||
       src.reg_offset != last_rhs_inst->dst.reg_offset ||
       src.reladdr ||
       src.abs ||
       src.negate ||
       last_rhs_inst->predicate != BRW_PREDICATE_NONE)
      return false;

   /* Check that that last instruction fully initialized the channels
    * we want to use, in the order we want to use them.  We could
    * potentially reswizzle the operands of many instructions so that
    * we could handle out of order channels, but don't yet.
    */
   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 if (!(last_rhs_inst->dst.writemask & (1 << i)))
	    return false;

	 if (BRW_GET_SWZ(src.swizzle, i) != i)
	    return false;
      }
   }

   /* Success!  Rewrite the instruction. */
   last_rhs_inst->dst.file = dst.file;
   last_rhs_inst->dst.reg = dst.reg;
   last_rhs_inst->dst.reg_offset = dst.reg_offset;
   last_rhs_inst->dst.reladdr = dst.reladdr;
   last_rhs_inst->dst.writemask &= dst.writemask;

   return true;
}

void
vec4_visitor::visit(ir_assignment *ir)
{
   dst_reg dst = get_assignment_lhs(ir->lhs, this);

   if (!ir->lhs->type->is_scalar() &&
       !ir->lhs->type->is_vector()) {
      ir->rhs->accept(this);
      src_reg src = this->result;

      if (ir->condition) {
	 emit_bool_to_cond_code(ir->condition);
      }

      emit_block_move(&dst, &src, ir->rhs->type, ir->condition != NULL);
      return;
   }

   /* Now we're down to just a scalar/vector with writemasks. */
   int i;

   vec4_instruction *pre_rhs_inst, *last_rhs_inst;
   pre_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   ir->rhs->accept(this);

   last_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   src_reg src = this->result;

   int swizzles[4];
   int first_enabled_chan = 0;
   int src_chan = 0;

   assert(ir->lhs->type->is_vector() ||
	  ir->lhs->type->is_scalar());
   dst.writemask = ir->write_mask;

   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 first_enabled_chan = BRW_GET_SWZ(src.swizzle, i);
	 break;
      }
   }

   /* Swizzle a small RHS vector into the channels being written.
    *
    * glsl ir treats write_mask as dictating how many channels are
    * present on the RHS while in our instructions we need to make
    * those channels appear in the slots of the vec4 they're written to.
    */
   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i))
	 swizzles[i] = BRW_GET_SWZ(src.swizzle, src_chan++);
      else
	 swizzles[i] = first_enabled_chan;
   }
   src.swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
			      swizzles[2], swizzles[3]);

   if (try_rewrite_rhs_to_dst(ir, dst, src, pre_rhs_inst, last_rhs_inst)) {
      return;
   }

   if (ir->condition) {
      emit_bool_to_cond_code(ir->condition);
   }

   for (i = 0; i < type_size(ir->lhs->type); i++) {
      vec4_instruction *inst = emit(BRW_OPCODE_MOV, dst, src);

      if (ir->condition)
	 inst->predicate = BRW_PREDICATE_NORMAL;

      dst.reg_offset++;
      src.reg_offset++;
   }
}

void
vec4_visitor::emit_constant_values(dst_reg *dst, ir_constant *ir)
{
   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      foreach_list(node, &ir->components) {
	 ir_constant *field_value = (ir_constant *)node;

	 emit_constant_values(dst, field_value);
      }
      return;
   }

   if (ir->type->is_array()) {
      for (unsigned int i = 0; i < ir->type->length; i++) {
	 emit_constant_values(dst, ir->array_elements[i]);
      }
      return;
   }

   if (ir->type->is_matrix()) {
      for (int i = 0; i < ir->type->matrix_columns; i++) {
	 for (int j = 0; j < ir->type->vector_elements; j++) {
	    dst->writemask = 1 << j;
	    dst->type = BRW_REGISTER_TYPE_F;

	    emit(BRW_OPCODE_MOV, *dst,
		 src_reg(ir->value.f[i * ir->type->vector_elements + j]));
	 }
	 dst->reg_offset++;
      }
      return;
   }

   for (int i = 0; i < ir->type->vector_elements; i++) {
      dst->writemask = 1 << i;
      dst->type = brw_type_for_base_type(ir->type);

      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 emit(BRW_OPCODE_MOV, *dst, src_reg(ir->value.f[i]));
	 break;
      case GLSL_TYPE_INT:
	 emit(BRW_OPCODE_MOV, *dst, src_reg(ir->value.i[i]));
	 break;
      case GLSL_TYPE_UINT:
	 emit(BRW_OPCODE_MOV, *dst, src_reg(ir->value.u[i]));
	 break;
      case GLSL_TYPE_BOOL:
	 emit(BRW_OPCODE_MOV, *dst, src_reg(ir->value.b[i]));
	 break;
      default:
	 assert(!"Non-float/uint/int/bool constant");
	 break;
      }
   }
   dst->reg_offset++;
}

void
vec4_visitor::visit(ir_constant *ir)
{
   dst_reg dst = dst_reg(this, ir->type);
   this->result = src_reg(dst);

   emit_constant_values(&dst, ir);
}

void
vec4_visitor::visit(ir_call *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_texture *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_return *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_discard *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_if *ir)
{
   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   if (intel->gen == 6) {
      emit_if_gen6(ir);
   } else {
      emit_bool_to_cond_code(ir->condition);
      vec4_instruction *inst = emit(BRW_OPCODE_IF);
      inst->predicate = BRW_PREDICATE_NORMAL;
   }

   visit_instructions(&ir->then_instructions);

   if (!ir->else_instructions.is_empty()) {
      this->base_ir = ir->condition;
      emit(BRW_OPCODE_ELSE);

      visit_instructions(&ir->else_instructions);
   }

   this->base_ir = ir->condition;
   emit(BRW_OPCODE_ENDIF);
}

int
vec4_visitor::emit_vue_header_gen4(int header_mrf)
{
   /* Get the position */
   src_reg pos = src_reg(output_reg[VERT_RESULT_HPOS]);

   /* Build ndc coords, which are (x/w, y/w, z/w, 1/w) */
   dst_reg ndc = dst_reg(this, glsl_type::vec4_type);

   current_annotation = "NDC";
   dst_reg ndc_w = ndc;
   ndc_w.writemask = WRITEMASK_W;
   src_reg pos_w = pos;
   pos_w.swizzle = BRW_SWIZZLE4(SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W);
   emit_math(SHADER_OPCODE_RCP, ndc_w, pos_w);

   dst_reg ndc_xyz = ndc;
   ndc_xyz.writemask = WRITEMASK_XYZ;

   emit(BRW_OPCODE_MUL, ndc_xyz, pos, src_reg(ndc_w));

   if ((c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) ||
       c->key.nr_userclip || brw->has_negative_rhw_bug) {
      dst_reg header1 = dst_reg(this, glsl_type::uvec4_type);
      GLuint i;

      emit(BRW_OPCODE_MOV, header1, 0u);

      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 assert(!"finishme: psiz");
	 src_reg psiz;

	 header1.writemask = WRITEMASK_W;
	 emit(BRW_OPCODE_MUL, header1, psiz, 1u << 11);
	 emit(BRW_OPCODE_AND, header1, src_reg(header1), 0x7ff << 8);
      }

      for (i = 0; i < c->key.nr_userclip; i++) {
	 vec4_instruction *inst;

	 inst = emit(BRW_OPCODE_DP4, dst_reg(brw_null_reg()),
		     pos, src_reg(c->userplane[i]));
	 inst->conditional_mod = BRW_CONDITIONAL_L;

	 emit(BRW_OPCODE_OR, header1, src_reg(header1), 1u << i);
	 inst->predicate = BRW_PREDICATE_NORMAL;
      }

      /* i965 clipping workaround:
       * 1) Test for -ve rhw
       * 2) If set,
       *      set ndc = (0,0,0,0)
       *      set ucp[6] = 1
       *
       * Later, clipping will detect ucp[6] and ensure the primitive is
       * clipped against all fixed planes.
       */
      if (brw->has_negative_rhw_bug) {
#if 0
	 /* FINISHME */
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(ndc, 3),
		 brw_imm_f(0));

	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<6));
	 brw_MOV(p, ndc, brw_imm_f(0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
#endif
      }

      header1.writemask = WRITEMASK_XYZW;
      emit(BRW_OPCODE_MOV, brw_message_reg(header_mrf++), src_reg(header1));
   } else {
      emit(BRW_OPCODE_MOV, retype(brw_message_reg(header_mrf++),
				  BRW_REGISTER_TYPE_UD), 0u);
   }

   if (intel->gen == 5) {
      /* There are 20 DWs (D0-D19) in VUE header on Ironlake:
       * dword 0-3 (m1) of the header is indices, point width, clip flags.
       * dword 4-7 (m2) is the ndc position (set above)
       * dword 8-11 (m3) of the vertex header is the 4D space position
       * dword 12-19 (m4,m5) of the vertex header is the user clip distance.
       * m6 is a pad so that the vertex element data is aligned
       * m7 is the first vertex data we fill.
       */
      current_annotation = "NDC";
      emit(BRW_OPCODE_MOV, brw_message_reg(header_mrf++), src_reg(ndc));

      current_annotation = "gl_Position";
      emit(BRW_OPCODE_MOV, brw_message_reg(header_mrf++), pos);

      /* user clip distance. */
      header_mrf += 2;

      /* Pad so that vertex element data is aligned. */
      header_mrf++;
   } else {
      /* There are 8 dwords in VUE header pre-Ironlake:
       * dword 0-3 (m1) is indices, point width, clip flags.
       * dword 4-7 (m2) is ndc position (set above)
       *
       * dword 8-11 (m3) is the first vertex data.
       */
      current_annotation = "NDC";
      emit(BRW_OPCODE_MOV, brw_message_reg(header_mrf++), src_reg(ndc));

      current_annotation = "gl_Position";
      emit(BRW_OPCODE_MOV, brw_message_reg(header_mrf++), pos);
   }

   return header_mrf;
}

int
vec4_visitor::emit_vue_header_gen6(int header_mrf)
{
   struct brw_reg reg;

   /* There are 8 or 16 DWs (D0-D15) in VUE header on Sandybridge:
    * dword 0-3 (m2) of the header is indices, point width, clip flags.
    * dword 4-7 (m3) is the 4D space position
    * dword 8-15 (m4,m5) of the vertex header is the user clip distance if
    * enabled.
    *
    * m4 or 6 is the first vertex element data we fill.
    */

   current_annotation = "indices, point width, clip flags";
   reg = brw_message_reg(header_mrf++);
   emit(BRW_OPCODE_MOV, retype(reg, BRW_REGISTER_TYPE_D), src_reg(0));
   if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
      emit(BRW_OPCODE_MOV, brw_writemask(reg, WRITEMASK_W),
	   src_reg(output_reg[VERT_RESULT_PSIZ]));
   }

   current_annotation = "gl_Position";
   emit(BRW_OPCODE_MOV,
	brw_message_reg(header_mrf++), src_reg(output_reg[VERT_RESULT_HPOS]));

   current_annotation = "user clip distances";
   if (c->key.nr_userclip) {
      for (int i = 0; i < c->key.nr_userclip; i++) {
	 struct brw_reg m;
	 if (i < 4)
	    m = brw_message_reg(header_mrf);
	 else
	    m = brw_message_reg(header_mrf + 1);

	 emit(BRW_OPCODE_DP4,
	      dst_reg(brw_writemask(m, 1 << (i & 3))),
	      src_reg(c->userplane[i]));
      }
      header_mrf += 2;
   }

   current_annotation = NULL;

   return header_mrf;
}

static int
align_interleaved_urb_mlen(struct brw_context *brw, int mlen)
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 6) {
      /* URB data written (does not include the message header reg) must
       * be a multiple of 256 bits, or 2 VS registers.  See vol5c.5,
       * section 5.4.3.2.2: URB_INTERLEAVED.
       *
       * URB entries are allocated on a multiple of 1024 bits, so an
       * extra 128 bits written here to make the end align to 256 is
       * no problem.
       */
      if ((mlen % 2) != 1)
	 mlen++;
   }

   return mlen;
}

/**
 * Generates the VUE payload plus the 1 or 2 URB write instructions to
 * complete the VS thread.
 *
 * The VUE layout is documented in Volume 2a.
 */
void
vec4_visitor::emit_urb_writes()
{
   /* MRF 0 is reserved for the debugger, so start with message header
    * in MRF 1.
    */
   int base_mrf = 1;
   int mrf = base_mrf;
   int urb_entry_size;
   uint64_t outputs_remaining = c->prog_data.outputs_written;
   /* In the process of generating our URB write message contents, we
    * may need to unspill a register or load from an array.  Those
    * reads would use MRFs 14-15.
    */
   int max_usable_mrf = 13;

   /* FINISHME: edgeflag */

   /* First mrf is the g0-based message header containing URB handles and such,
    * which is implied in VS_OPCODE_URB_WRITE.
    */
   mrf++;

   if (intel->gen >= 6) {
      mrf = emit_vue_header_gen6(mrf);
   } else {
      mrf = emit_vue_header_gen4(mrf);
   }

   /* Set up the VUE data for the first URB write */
   int attr;
   for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
      if (!(c->prog_data.outputs_written & BITFIELD64_BIT(attr)))
	 continue;

      outputs_remaining &= ~BITFIELD64_BIT(attr);

      /* This is set up in the VUE header. */
      if (attr == VERT_RESULT_HPOS)
	 continue;

      /* This is loaded into the VUE header, and thus doesn't occupy
       * an attribute slot.
       */
      if (attr == VERT_RESULT_PSIZ)
	 continue;

      vec4_instruction *inst = emit(BRW_OPCODE_MOV, brw_message_reg(mrf++),
				    src_reg(output_reg[attr]));

      if ((attr == VERT_RESULT_COL0 ||
	   attr == VERT_RESULT_COL1 ||
	   attr == VERT_RESULT_BFC0 ||
	   attr == VERT_RESULT_BFC1) &&
	  c->key.clamp_vertex_color) {
	 inst->saturate = true;
      }

      /* If this was MRF 15, we can't fit anything more into this URB
       * WRITE.  Note that base_mrf of 1 means that MRF 15 is an
       * even-numbered amount of URB write data, which will meet
       * gen6's requirements for length alignment.
       */
      if (mrf > max_usable_mrf) {
	 attr++;
	 break;
      }
   }

   vec4_instruction *inst = emit(VS_OPCODE_URB_WRITE);
   inst->base_mrf = base_mrf;
   inst->mlen = align_interleaved_urb_mlen(brw, mrf - base_mrf);
   inst->eot = !outputs_remaining;

   urb_entry_size = mrf - base_mrf;

   /* Optional second URB write */
   if (outputs_remaining) {
      mrf = base_mrf + 1;

      for (; attr < VERT_RESULT_MAX; attr++) {
	 if (!(c->prog_data.outputs_written & BITFIELD64_BIT(attr)))
	    continue;

	 assert(mrf < max_usable_mrf);

	 emit(BRW_OPCODE_MOV, brw_message_reg(mrf++), src_reg(output_reg[attr]));
      }

      inst = emit(VS_OPCODE_URB_WRITE);
      inst->base_mrf = base_mrf;
      inst->mlen = align_interleaved_urb_mlen(brw, mrf - base_mrf);
      inst->eot = true;
      /* URB destination offset.  In the previous write, we got MRFs
       * 2-13 minus the one header MRF, so 12 regs.  URB offset is in
       * URB row increments, and each of our MRFs is half of one of
       * those, since we're doing interleaved writes.
       */
      inst->offset = (max_usable_mrf - base_mrf) / 2;

      urb_entry_size += mrf - base_mrf;
   }

   if (intel->gen == 6)
      c->prog_data.urb_entry_size = ALIGN(urb_entry_size, 8) / 8;
   else
      c->prog_data.urb_entry_size = ALIGN(urb_entry_size, 4) / 4;
}

src_reg
vec4_visitor::get_scratch_offset(vec4_instruction *inst,
				 src_reg *reladdr, int reg_offset)
{
   /* Because we store the values to scratch interleaved like our
    * vertex data, we need to scale the vec4 index by 2.
    */
   int message_header_scale = 2;

   /* Pre-gen6, the message header uses byte offsets instead of vec4
    * (16-byte) offset units.
    */
   if (intel->gen < 6)
      message_header_scale *= 16;

   if (reladdr) {
      src_reg index = src_reg(this, glsl_type::int_type);

      vec4_instruction *add = emit(BRW_OPCODE_ADD,
				   dst_reg(index),
				   *reladdr,
				   src_reg(reg_offset));
      /* Move our new instruction from the tail to its correct place. */
      add->remove();
      inst->insert_before(add);

      vec4_instruction *mul = emit(BRW_OPCODE_MUL, dst_reg(index),
				   index, src_reg(message_header_scale));
      mul->remove();
      inst->insert_before(mul);

      return index;
   } else {
      return src_reg(reg_offset * message_header_scale);
   }
}

/**
 * Emits an instruction before @inst to load the value named by @orig_src
 * from scratch space at @base_offset to @temp.
 */
void
vec4_visitor::emit_scratch_read(vec4_instruction *inst,
				dst_reg temp, src_reg orig_src,
				int base_offset)
{
   int reg_offset = base_offset + orig_src.reg_offset;
   src_reg index = get_scratch_offset(inst, orig_src.reladdr, reg_offset);

   vec4_instruction *scratch_read_inst = emit(VS_OPCODE_SCRATCH_READ,
					      temp, index);

   scratch_read_inst->base_mrf = 14;
   scratch_read_inst->mlen = 1;
   /* Move our instruction from the tail to its correct place. */
   scratch_read_inst->remove();
   inst->insert_before(scratch_read_inst);
}

/**
 * Emits an instruction after @inst to store the value to be written
 * to @orig_dst to scratch space at @base_offset, from @temp.
 */
void
vec4_visitor::emit_scratch_write(vec4_instruction *inst,
				 src_reg temp, dst_reg orig_dst,
				 int base_offset)
{
   int reg_offset = base_offset + orig_dst.reg_offset;
   src_reg index = get_scratch_offset(inst, orig_dst.reladdr, reg_offset);

   dst_reg dst = dst_reg(brw_writemask(brw_vec8_grf(0, 0),
				       orig_dst.writemask));
   vec4_instruction *scratch_write_inst = emit(VS_OPCODE_SCRATCH_WRITE,
					       dst, temp, index);
   scratch_write_inst->base_mrf = 13;
   scratch_write_inst->mlen = 2;
   scratch_write_inst->predicate = inst->predicate;
   /* Move our instruction from the tail to its correct place. */
   scratch_write_inst->remove();
   inst->insert_after(scratch_write_inst);
}

/**
 * We can't generally support array access in GRF space, because a
 * single instruction's destination can only span 2 contiguous
 * registers.  So, we send all GRF arrays that get variable index
 * access to scratch space.
 */
void
vec4_visitor::move_grf_array_access_to_scratch()
{
   int scratch_loc[this->virtual_grf_count];

   for (int i = 0; i < this->virtual_grf_count; i++) {
      scratch_loc[i] = -1;
   }

   /* First, calculate the set of virtual GRFs that need to be punted
    * to scratch due to having any array access on them, and where in
    * scratch.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      if (inst->dst.file == GRF && inst->dst.reladdr &&
	  scratch_loc[inst->dst.reg] == -1) {
	 scratch_loc[inst->dst.reg] = c->last_scratch;
	 c->last_scratch += this->virtual_grf_sizes[inst->dst.reg] * 8 * 4;
      }

      for (int i = 0 ; i < 3; i++) {
	 src_reg *src = &inst->src[i];

	 if (src->file == GRF && src->reladdr &&
	     scratch_loc[src->reg] == -1) {
	    scratch_loc[src->reg] = c->last_scratch;
	    c->last_scratch += this->virtual_grf_sizes[src->reg] * 8 * 4;
	 }
      }
   }

   /* Now, for anything that will be accessed through scratch, rewrite
    * it to load/store.  Note that this is a _safe list walk, because
    * we may generate a new scratch_write instruction after the one
    * we're processing.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      /* Set up the annotation tracking for new generated instructions. */
      base_ir = inst->ir;
      current_annotation = inst->annotation;

      if (inst->dst.file == GRF && scratch_loc[inst->dst.reg] != -1) {
	 src_reg temp = src_reg(this, glsl_type::vec4_type);

	 emit_scratch_write(inst, temp, inst->dst, scratch_loc[inst->dst.reg]);

	 inst->dst.file = temp.file;
	 inst->dst.reg = temp.reg;
	 inst->dst.reg_offset = temp.reg_offset;
	 inst->dst.reladdr = NULL;
      }

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != GRF || scratch_loc[inst->src[i].reg] == -1)
	    continue;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_scratch_read(inst, temp, inst->src[i],
			   scratch_loc[inst->src[i].reg]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }
}


vec4_visitor::vec4_visitor(struct brw_vs_compile *c,
			   struct gl_shader_program *prog,
			   struct brw_shader *shader)
{
   this->c = c;
   this->p = &c->func;
   this->brw = p->brw;
   this->intel = &brw->intel;
   this->ctx = &intel->ctx;
   this->prog = prog;
   this->shader = shader;

   this->mem_ctx = ralloc_context(NULL);
   this->failed = false;

   this->base_ir = NULL;
   this->current_annotation = NULL;

   this->c = c;
   this->vp = prog->VertexProgram;
   this->prog_data = &c->prog_data;

   this->variable_ht = hash_table_ctor(0,
				       hash_table_pointer_hash,
				       hash_table_pointer_compare);

   this->virtual_grf_sizes = NULL;
   this->virtual_grf_count = 0;
   this->virtual_grf_array_size = 0;

   this->uniforms = 0;

   this->variable_ht = hash_table_ctor(0,
				       hash_table_pointer_hash,
				       hash_table_pointer_compare);
}

vec4_visitor::~vec4_visitor()
{
   hash_table_dtor(this->variable_ht);
}


void
vec4_visitor::fail(const char *format, ...)
{
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "VS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_VS) {
      fprintf(stderr, "%s",  msg);
   }
}

} /* namespace brw */
