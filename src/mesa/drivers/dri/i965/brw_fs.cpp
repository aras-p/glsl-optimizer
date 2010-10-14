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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
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
#include "talloc.h"
}
#include "brw_fs.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

static int using_new_fs = -1;
static struct brw_reg brw_reg_from_fs_reg(class fs_reg *reg);

struct gl_shader *
brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = talloc_zero(NULL, struct brw_shader);
   if (shader) {
      shader->base.Type = type;
      shader->base.Name = name;
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct brw_shader_program *prog;
   prog = talloc_zero(NULL, struct brw_shader_program);
   if (prog) {
      prog->base.Name = name;
      _mesa_init_shader_program(ctx, &prog->base);
   }
   return &prog->base;
}

GLboolean
brw_compile_shader(struct gl_context *ctx, struct gl_shader *shader)
{
   if (!_mesa_ir_compile_shader(ctx, shader))
      return GL_FALSE;

   return GL_TRUE;
}

GLboolean
brw_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct intel_context *intel = intel_context(ctx);

   if (using_new_fs == -1) {
      if (intel->gen >= 6)
	 using_new_fs = 1;
      else
	 using_new_fs = getenv("INTEL_NEW_FS") != NULL;
   }

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      struct brw_shader *shader = (struct brw_shader *)prog->_LinkedShaders[i];

      if (using_new_fs && shader->base.Type == GL_FRAGMENT_SHADER) {
	 void *mem_ctx = talloc_new(NULL);
	 bool progress;

	 if (shader->ir)
	    talloc_free(shader->ir);
	 shader->ir = new(shader) exec_list;
	 clone_ir_list(mem_ctx, shader->ir, shader->base.ir);

	 do_mat_op_to_vec(shader->ir);
	 do_mod_to_fract(shader->ir);
	 do_div_to_mul_rcp(shader->ir);
	 do_sub_to_add_neg(shader->ir);
	 do_explog_to_explog2(shader->ir);
	 do_lower_texture_projection(shader->ir);
	 brw_do_cubemap_normalize(shader->ir);

	 do {
	    progress = false;

	    brw_do_channel_expressions(shader->ir);
	    brw_do_vector_splitting(shader->ir);

	    progress = do_lower_jumps(shader->ir, true, true,
				      true, /* main return */
				      false, /* continue */
				      false /* loops */
				      ) || progress;

	    progress = do_common_optimization(shader->ir, true, 32) || progress;

	    progress = lower_noise(shader->ir) || progress;
	    progress =
	       lower_variable_index_to_cond_assign(shader->ir,
						   GL_TRUE, /* input */
						   GL_TRUE, /* output */
						   GL_TRUE, /* temp */
						   GL_TRUE /* uniform */
						   ) || progress;
	    if (intel->gen == 6) {
	       progress = do_if_to_cond_assign(shader->ir) || progress;
	    }
	 } while (progress);

	 validate_ir_tree(shader->ir);

	 reparent_ir(shader->ir, shader->ir);
	 talloc_free(mem_ctx);
      }
   }

   if (!_mesa_ir_link_shader(ctx, prog))
      return GL_FALSE;

   return GL_TRUE;
}

static int
type_size(const struct glsl_type *type)
{
   unsigned int size, i;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return type->components();
   case GLSL_TYPE_ARRAY:
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up no register space, since they're baked in at
       * link time.
       */
      return 0;
   default:
      assert(!"not reached");
      return 0;
   }
}

static const fs_reg reg_undef;
static const fs_reg reg_null(ARF, BRW_ARF_NULL);

int
fs_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_next) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = talloc_realloc(mem_ctx, virtual_grf_sizes,
					 int, virtual_grf_array_size);

      /* This slot is always unused. */
      virtual_grf_sizes[0] = 0;
   }
   virtual_grf_sizes[virtual_grf_next] = size;
   return virtual_grf_next++;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int hw_reg)
{
   init();
   this->file = file;
   this->hw_reg = hw_reg;
   this->type = BRW_REGISTER_TYPE_F;
}

int
brw_type_for_base_type(const struct glsl_type *type)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      return BRW_REGISTER_TYPE_F;
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      return BRW_REGISTER_TYPE_D;
   case GLSL_TYPE_UINT:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT:
      /* These should be overridden with the type of the member when
       * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
       * way to trip up if we don't.
       */
      return BRW_REGISTER_TYPE_UD;
   default:
      assert(!"not reached");
      return BRW_REGISTER_TYPE_F;
   }
}

/** Automatic reg constructor. */
fs_reg::fs_reg(class fs_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));
   this->reg_offset = 0;
   this->type = brw_type_for_base_type(type);
}

fs_reg *
fs_visitor::variable_storage(ir_variable *var)
{
   return (fs_reg *)hash_table_find(this->variable_ht, var);
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
int
fs_visitor::setup_uniform_values(int loc, const glsl_type *type)
{
   unsigned int offset = 0;
   float *vec_values;

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
      vec_values = fp->Base.Parameters->ParameterValues[loc];
      for (unsigned int i = 0; i < type->vector_elements; i++) {
	 c->prog_data.param[c->prog_data.nr_params++] = &vec_values[i];
      }
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
fs_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const struct gl_builtin_uniform_desc *statevar = NULL;

   for (unsigned int i = 0; _mesa_builtin_uniform_desc[i].name; i++) {
      statevar = &_mesa_builtin_uniform_desc[i];
      if (strcmp(ir->name, _mesa_builtin_uniform_desc[i].name) == 0)
	 break;
   }

   if (!statevar->name) {
      this->fail = true;
      printf("Failed to find builtin uniform `%s'\n", ir->name);
      return;
   }

   int array_count;
   if (ir->type->is_array()) {
      array_count = ir->type->length;
   } else {
      array_count = 1;
   }

   for (int a = 0; a < array_count; a++) {
      for (unsigned int i = 0; i < statevar->num_elements; i++) {
	 struct gl_builtin_uniform_element *element = &statevar->elements[i];
	 int tokens[STATE_LENGTH];

	 memcpy(tokens, element->tokens, sizeof(element->tokens));
	 if (ir->type->is_array()) {
	    tokens[1] = a;
	 }

	 /* This state reference has already been setup by ir_to_mesa,
	  * but we'll get the same index back here.
	  */
	 int index = _mesa_add_state_reference(this->fp->Base.Parameters,
					       (gl_state_index *)tokens);
	 float *vec_values = this->fp->Base.Parameters->ParameterValues[index];

	 /* Add each of the unique swizzles of the element as a
	  * parameter.  This'll end up matching the expected layout of
	  * the array/matrix/structure we're trying to fill in.
	  */
	 int last_swiz = -1;
	 for (unsigned int i = 0; i < 4; i++) {
	    int swiz = GET_SWZ(element->swizzle, i);
	    if (swiz == last_swiz)
	       break;
	    last_swiz = swiz;

	    c->prog_data.param[c->prog_data.nr_params++] = &vec_values[swiz];
	 }
      }
   }
}

fs_reg *
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   fs_reg neg_y = this->pixel_y;
   neg_y.negate = true;

   /* gl_FragCoord.x */
   if (ir->pixel_center_integer) {
      emit(fs_inst(BRW_OPCODE_MOV, wpos, this->pixel_x));
   } else {
      emit(fs_inst(BRW_OPCODE_ADD, wpos, this->pixel_x, fs_reg(0.5f)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (ir->origin_upper_left && ir->pixel_center_integer) {
      emit(fs_inst(BRW_OPCODE_MOV, wpos, this->pixel_y));
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->pixel_center_integer ? 0.0 : 0.5);

      if (!ir->origin_upper_left) {
	 pixel_y.negate = true;
	 offset += c->key.drawable_height - 1.0;
      }

      emit(fs_inst(BRW_OPCODE_ADD, wpos, pixel_y, fs_reg(offset)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   emit(fs_inst(FS_OPCODE_LINTERP, wpos, this->delta_x, this->delta_y,
		interp_reg(FRAG_ATTRIB_WPOS, 2)));
   wpos.reg_offset++;

   /* gl_FragCoord.w: Already set up in emit_interpolation */
   emit(fs_inst(BRW_OPCODE_MOV, wpos, this->wpos_w));

   return reg;
}

fs_reg *
fs_visitor::emit_general_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   /* Interpolation is always in floating point regs. */
   reg->type = BRW_REGISTER_TYPE_F;
   fs_reg attr = *reg;

   unsigned int array_elements;
   const glsl_type *type;

   if (ir->type->is_array()) {
      array_elements = ir->type->length;
      if (array_elements == 0) {
	 this->fail = true;
      }
      type = ir->type->fields.array;
   } else {
      array_elements = 1;
      type = ir->type;
   }

   int location = ir->location;
   for (unsigned int i = 0; i < array_elements; i++) {
      for (unsigned int j = 0; j < type->matrix_columns; j++) {
	 if (urb_setup[location] == -1) {
	    /* If there's no incoming setup data for this slot, don't
	     * emit interpolation for it.
	     */
	    attr.reg_offset += type->vector_elements;
	    location++;
	    continue;
	 }

	 for (unsigned int c = 0; c < type->vector_elements; c++) {
	    struct brw_reg interp = interp_reg(location, c);
	    emit(fs_inst(FS_OPCODE_LINTERP,
			 attr,
			 this->delta_x,
			 this->delta_y,
			 fs_reg(interp)));
	    attr.reg_offset++;
	 }

	 if (intel->gen < 6) {
	    attr.reg_offset -= type->vector_elements;
	    for (unsigned int c = 0; c < type->vector_elements; c++) {
	       emit(fs_inst(BRW_OPCODE_MUL,
			    attr,
			    attr,
			    this->pixel_w));
	       attr.reg_offset++;
	    }
	 }
	 location++;
      }
   }

   return reg;
}

fs_reg *
fs_visitor::emit_frontfacing_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);

   /* The frontfacing comes in as a bit in the thread payload. */
   if (intel->gen >= 6) {
      emit(fs_inst(BRW_OPCODE_ASR,
		   *reg,
		   fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_D)),
		   fs_reg(15)));
      emit(fs_inst(BRW_OPCODE_NOT,
		   *reg,
		   *reg));
      emit(fs_inst(BRW_OPCODE_AND,
		   *reg,
		   *reg,
		   fs_reg(1)));
   } else {
      fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
      struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
      /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
       * us front face
       */
      fs_inst *inst = emit(fs_inst(BRW_OPCODE_CMP,
				   *reg,
				   fs_reg(r1_6ud),
				   fs_reg(1u << 31)));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      emit(fs_inst(BRW_OPCODE_AND, *reg, *reg, fs_reg(1u)));
   }

   return reg;
}

fs_inst *
fs_visitor::emit_math(fs_opcodes opcode, fs_reg dst, fs_reg src)
{
   switch (opcode) {
   case FS_OPCODE_RCP:
   case FS_OPCODE_RSQ:
   case FS_OPCODE_SQRT:
   case FS_OPCODE_EXP2:
   case FS_OPCODE_LOG2:
   case FS_OPCODE_SIN:
   case FS_OPCODE_COS:
      break;
   default:
      assert(!"not reached: bad math opcode");
      return NULL;
   }

   /* Can't do hstride == 0 args to gen6 math, so expand it out.  We
    * might be able to do better by doing execsize = 1 math and then
    * expanding that result out, but we would need to be careful with
    * masking.
    */
   if (intel->gen >= 6 && src.file == UNIFORM) {
      fs_reg expanded = fs_reg(this, glsl_type::float_type);
      emit(fs_inst(BRW_OPCODE_MOV, expanded, src));
      src = expanded;
   }

   fs_inst *inst = emit(fs_inst(opcode, dst, src));

   if (intel->gen < 6) {
      inst->base_mrf = 2;
      inst->mlen = 1;
   }

   return inst;
}

fs_inst *
fs_visitor::emit_math(fs_opcodes opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   int base_mrf = 2;
   fs_inst *inst;

   assert(opcode == FS_OPCODE_POW);

   if (intel->gen >= 6) {
      /* Can't do hstride == 0 args to gen6 math, so expand it out. */
      if (src0.file == UNIFORM) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 emit(fs_inst(BRW_OPCODE_MOV, expanded, src0));
	 src0 = expanded;
      }

      if (src1.file == UNIFORM) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 emit(fs_inst(BRW_OPCODE_MOV, expanded, src1));
	 src1 = expanded;
      }

      inst = emit(fs_inst(opcode, dst, src0, src1));
   } else {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + 1), src1));
      inst = emit(fs_inst(opcode, dst, src0, reg_null));

      inst->base_mrf = base_mrf;
      inst->mlen = 2;
   }
   return inst;
}

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

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }

      reg = new(this->mem_ctx) fs_reg(UNIFORM, param_index);
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

void
fs_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   fs_reg op[2], temp;
   fs_reg result;
   fs_inst *inst;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 ir_print_visitor v;
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
	 this->fail = true;
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

   /* Storage for our result.  If our result goes into an assignment, it will
    * just get copy-propagated out, so no worries.
    */
   this->result = fs_reg(this, ir->type);

   switch (ir->operation) {
   case ir_unop_logic_not:
      emit(fs_inst(BRW_OPCODE_ADD, this->result, op[0], fs_reg(-1)));
      break;
   case ir_unop_neg:
      op[0].negate = !op[0].negate;
      this->result = op[0];
      break;
   case ir_unop_abs:
      op[0].abs = true;
      this->result = op[0];
      break;
   case ir_unop_sign:
      temp = fs_reg(this, ir->type);

      emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(0.0f)));

      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      inst = emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(1.0f)));
      inst->predicated = true;

      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      inst = emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(-1.0f)));
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
      emit_math(FS_OPCODE_SIN, this->result, op[0]);
      break;
   case ir_unop_cos:
      emit_math(FS_OPCODE_COS, this->result, op[0]);
      break;

   case ir_unop_dFdx:
      emit(fs_inst(FS_OPCODE_DDX, this->result, op[0]));
      break;
   case ir_unop_dFdy:
      emit(fs_inst(FS_OPCODE_DDY, this->result, op[0]));
      break;

   case ir_binop_add:
      emit(fs_inst(BRW_OPCODE_ADD, this->result, op[0], op[1]));
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      emit(fs_inst(BRW_OPCODE_MUL, this->result, op[0], op[1]));
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
      break;
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_greater:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_lequal:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_LE;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_gequal:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_GE;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_equal:
   case ir_binop_all_equal: /* same as nequal for scalars */
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_Z;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_nequal:
   case ir_binop_any_nequal: /* same as nequal for scalars */
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;

   case ir_binop_logic_xor:
      emit(fs_inst(BRW_OPCODE_XOR, this->result, op[0], op[1]));
      break;

   case ir_binop_logic_or:
      emit(fs_inst(BRW_OPCODE_OR, this->result, op[0], op[1]));
      break;

   case ir_binop_logic_and:
      emit(fs_inst(BRW_OPCODE_AND, this->result, op[0], op[1]));
      break;

   case ir_binop_dot:
   case ir_binop_cross:
   case ir_unop_any:
      assert(!"not reached: should be handled by brw_fs_channel_expressions");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
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
      emit(fs_inst(BRW_OPCODE_MOV, this->result, op[0]));
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      inst = emit(fs_inst(BRW_OPCODE_AND, this->result,
			  this->result, fs_reg(1)));
      break;

   case ir_unop_trunc:
      emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      break;
   case ir_unop_ceil:
      op[0].negate = ~op[0].negate;
      inst = emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      break;
   case ir_unop_fract:
      inst = emit(fs_inst(BRW_OPCODE_FRC, this->result, op[0]));
      break;

   case ir_binop_min:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_L;

      inst = emit(fs_inst(BRW_OPCODE_SEL, this->result, op[0], op[1]));
      inst->predicated = true;
      break;
   case ir_binop_max:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_G;

      inst = emit(fs_inst(BRW_OPCODE_SEL, this->result, op[0], op[1]));
      inst->predicated = true;
      break;

   case ir_binop_pow:
      emit_math(FS_OPCODE_POW, this->result, op[0], op[1]);
      break;

   case ir_unop_bit_not:
   case ir_unop_u2f:
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
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

	 fs_inst *inst = emit(fs_inst(BRW_OPCODE_MOV, l, r));
	 inst->predicated = predicated;

	 l.reg_offset++;
	 r.reg_offset++;
      }
      break;
   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_assignment_writes(l, r, type->fields.array, predicated);
      }

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
   ir->lhs->accept(this);
   l = this->result;

   ir->rhs->accept(this);
   r = this->result;

   assert(l.file != BAD_FILE);
   assert(r.file != BAD_FILE);

   if (ir->condition) {
      /* Get the condition bool into the predicate. */
      ir->condition->accept(this);
      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, this->result, fs_reg(0)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }

   if (ir->lhs->type->is_scalar() ||
       ir->lhs->type->is_vector()) {
      for (int i = 0; i < ir->lhs->type->vector_elements; i++) {
	 if (ir->write_mask & (1 << i)) {
	    inst = emit(fs_inst(BRW_OPCODE_MOV, l, r));
	    if (ir->condition)
	       inst->predicated = true;
	    r.reg_offset++;
	 }
	 l.reg_offset++;
      }
   } else {
      emit_assignment_writes(l, r, ir->lhs->type, ir->condition != NULL);
   }
}

fs_inst *
fs_visitor::emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate)
{
   int mlen;
   int base_mrf = 1;
   bool simd16 = false;
   fs_reg orig_dst;

   /* g0 header. */
   mlen = 1;

   if (ir->shadow_comparitor) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i),
		      coordinate));
	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;

      if (ir->op == ir_tex) {
	 /* There's no plain shadow compare message, so we use shadow
	  * compare with a bias of 0.0.
	  */
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
		      fs_reg(0.0f)));
	 mlen++;
      } else if (ir->op == ir_txb) {
	 ir->lod_info.bias->accept(this);
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
		      this->result));
	 mlen++;
      } else {
	 assert(ir->op == ir_txl);
	 ir->lod_info.lod->accept(this);
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
		      this->result));
	 mlen++;
      }

      ir->shadow_comparitor->accept(this);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;
   } else if (ir->op == ir_tex) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i),
		      coordinate));
	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;
   } else {
      /* Oh joy.  gen4 doesn't have SIMD8 non-shadow-compare bias/lod
       * instructions.  We'll need to do SIMD16 here.
       */
      assert(ir->op == ir_txb || ir->op == ir_txl);

      for (int i = 0; i < ir->coordinate->type->vector_elements * 2;) {
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i * 2),
		      coordinate));
	 coordinate.reg_offset++;
      }

      /* lod/bias appears after u/v/r. */
      mlen += 6;

      if (ir->op == ir_txb) {
	 ir->lod_info.bias->accept(this);
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
		      this->result));
	 mlen++;
      } else {
	 ir->lod_info.lod->accept(this);
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen),
		      this->result));
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
      inst = emit(fs_inst(FS_OPCODE_TEX, dst));
      break;
   case ir_txb:
      inst = emit(fs_inst(FS_OPCODE_TXB, dst));
      break;
   case ir_txl:
      inst = emit(fs_inst(FS_OPCODE_TXL, dst));
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;

   if (simd16) {
      for (int i = 0; i < 4; i++) {
	 emit(fs_inst(BRW_OPCODE_MOV, orig_dst, dst));
	 orig_dst.reg_offset++;
	 dst.reg_offset += 2;
      }
   }

   return inst;
}

fs_inst *
fs_visitor::emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate)
{
   /* gen5's SIMD8 sampler has slots for u, v, r, array index, then
    * optional parameters like shadow comparitor or LOD bias.  If
    * optional parameters aren't present, those base slots are
    * optional and don't need to be included in the message.
    *
    * We don't fill in the unnecessary slots regardless, which may
    * look surprising in the disassembly.
    */
   int mlen = 1; /* g0 header always present. */
   int base_mrf = 1;

   for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i),
		   coordinate));
      coordinate.reg_offset++;
   }
   mlen += ir->coordinate->type->vector_elements;

   if (ir->shadow_comparitor) {
      mlen = MAX2(mlen, 5);

      ir->shadow_comparitor->accept(this);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;
   }

   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex:
      inst = emit(fs_inst(FS_OPCODE_TEX, dst));
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      mlen = MAX2(mlen, 5);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;

      inst = emit(fs_inst(FS_OPCODE_TXB, dst));
      break;
   case ir_txl:
      ir->lod_info.lod->accept(this);
      mlen = MAX2(mlen, 5);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;

      inst = emit(fs_inst(FS_OPCODE_TXL, dst));
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;

   return inst;
}

void
fs_visitor::visit(ir_texture *ir)
{
   int sampler;
   fs_inst *inst = NULL;

   ir->coordinate->accept(this);
   fs_reg coordinate = this->result;

   /* Should be lowered by do_lower_texture_projection */
   assert(!ir->projector);

   sampler = _mesa_get_sampler_uniform_value(ir->sampler,
					     ctx->Shader.CurrentProgram,
					     &brw->fragment_program->Base);
   sampler = c->fp->program.Base.SamplerUnits[sampler];

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

      fs_reg scale_x = fs_reg(UNIFORM, c->prog_data.nr_params);
      fs_reg scale_y = fs_reg(UNIFORM, c->prog_data.nr_params + 1);
      GLuint index = _mesa_add_state_reference(params,
					       (gl_state_index *)tokens);
      float *vec_values = this->fp->Base.Parameters->ParameterValues[index];

      c->prog_data.param[c->prog_data.nr_params++] = &vec_values[0];
      c->prog_data.param[c->prog_data.nr_params++] = &vec_values[1];

      fs_reg dst = fs_reg(this, ir->coordinate->type);
      fs_reg src = coordinate;
      coordinate = dst;

      emit(fs_inst(BRW_OPCODE_MUL, dst, src, scale_x));
      dst.reg_offset++;
      src.reg_offset++;
      emit(fs_inst(BRW_OPCODE_MUL, dst, src, scale_y));
   }

   /* Writemasking doesn't eliminate channels on SIMD8 texture
    * samples, so don't worry about them.
    */
   fs_reg dst = fs_reg(this, glsl_type::vec4_type);

   if (intel->gen < 5) {
      inst = emit_texture_gen4(ir, dst, coordinate);
   } else {
      inst = emit_texture_gen5(ir, dst, coordinate);
   }

   inst->sampler = sampler;

   this->result = dst;

   if (ir->shadow_comparitor)
      inst->shadow_compare = true;

   if (c->key.tex_swizzles[inst->sampler] != SWIZZLE_NOOP) {
      fs_reg swizzle_dst = fs_reg(this, glsl_type::vec4_type);

      for (int i = 0; i < 4; i++) {
	 int swiz = GET_SWZ(c->key.tex_swizzles[inst->sampler], i);
	 fs_reg l = swizzle_dst;
	 l.reg_offset += i;

	 if (swiz == SWIZZLE_ZERO) {
	    emit(fs_inst(BRW_OPCODE_MOV, l, fs_reg(0.0f)));
	 } else if (swiz == SWIZZLE_ONE) {
	    emit(fs_inst(BRW_OPCODE_MOV, l, fs_reg(1.0f)));
	 } else {
	    fs_reg r = dst;
	    r.reg_offset += GET_SWZ(c->key.tex_swizzles[inst->sampler], i);
	    emit(fs_inst(BRW_OPCODE_MOV, l, r));
	 }
      }
      this->result = swizzle_dst;
   }
}

void
fs_visitor::visit(ir_swizzle *ir)
{
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
      emit(fs_inst(BRW_OPCODE_MOV, result, channel));
      result.reg_offset++;
   }
}

void
fs_visitor::visit(ir_discard *ir)
{
   fs_reg temp = fs_reg(this, glsl_type::uint_type);

   assert(ir->condition == NULL); /* FINISHME */

   emit(fs_inst(FS_OPCODE_DISCARD_NOT, temp, reg_null));
   emit(fs_inst(FS_OPCODE_DISCARD_AND, reg_null, temp));
   kill_emitted = true;
}

void
fs_visitor::visit(ir_constant *ir)
{
   fs_reg reg(this, ir->type);
   this->result = reg;

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.f[i])));
	 break;
      case GLSL_TYPE_UINT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.u[i])));
	 break;
      case GLSL_TYPE_INT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.i[i])));
	 break;
      case GLSL_TYPE_BOOL:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg((int)ir->value.b[i])));
	 break;
      default:
	 assert(!"Non-float/uint/int/bool constant");
      }
      reg.reg_offset++;
   }
}

void
fs_visitor::visit(ir_if *ir)
{
   fs_inst *inst;

   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   /* Generate the condition into the condition code. */
   ir->condition->accept(this);
   inst = emit(fs_inst(BRW_OPCODE_MOV, fs_reg(brw_null_reg()), this->result));
   inst->conditional_mod = BRW_CONDITIONAL_NZ;

   inst = emit(fs_inst(BRW_OPCODE_IF));
   inst->predicated = true;

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      this->base_ir = ir;

      ir->accept(this);
   }

   if (!ir->else_instructions.is_empty()) {
      emit(fs_inst(BRW_OPCODE_ELSE));

      foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 this->base_ir = ir;

	 ir->accept(this);
      }
   }

   emit(fs_inst(BRW_OPCODE_ENDIF));
}

void
fs_visitor::visit(ir_loop *ir)
{
   fs_reg counter = reg_undef;

   if (ir->counter) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from) {
	 this->base_ir = ir->from;
	 ir->from->accept(this);

	 emit(fs_inst(BRW_OPCODE_MOV, counter, this->result));
      }
   }

   emit(fs_inst(BRW_OPCODE_DO));

   if (ir->to) {
      this->base_ir = ir->to;
      ir->to->accept(this);

      fs_inst *inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null,
				   counter, this->result));
      switch (ir->cmp) {
      case ir_binop_equal:
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;
      case ir_binop_nequal:
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;
      case ir_binop_gequal:
	 inst->conditional_mod = BRW_CONDITIONAL_GE;
	 break;
      case ir_binop_lequal:
	 inst->conditional_mod = BRW_CONDITIONAL_LE;
	 break;
      case ir_binop_greater:
	 inst->conditional_mod = BRW_CONDITIONAL_G;
	 break;
      case ir_binop_less:
	 inst->conditional_mod = BRW_CONDITIONAL_L;
	 break;
      default:
	 assert(!"not reached: unknown loop condition");
	 this->fail = true;
	 break;
      }

      inst = emit(fs_inst(BRW_OPCODE_BREAK));
      inst->predicated = true;
   }

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      this->base_ir = ir;
      ir->accept(this);
   }

   if (ir->increment) {
      this->base_ir = ir->increment;
      ir->increment->accept(this);
      emit(fs_inst(BRW_OPCODE_ADD, counter, counter, this->result));
   }

   emit(fs_inst(BRW_OPCODE_WHILE));
}

void
fs_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(fs_inst(BRW_OPCODE_BREAK));
      break;
   case ir_loop_jump::jump_continue:
      emit(fs_inst(BRW_OPCODE_CONTINUE));
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
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 2),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 3),
		fs_reg(0.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 4),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 5),
		fs_reg(0.0f)));

   fs_inst *write;
   write = emit(fs_inst(FS_OPCODE_FB_WRITE,
			fs_reg(0),
			fs_reg(0)));
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
   struct brw_reg g1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);

   this->current_annotation = "compute pixel centers";
   this->pixel_x = fs_reg(this, glsl_type::uint_type);
   this->pixel_y = fs_reg(this, glsl_type::uint_type);
   this->pixel_x.type = BRW_REGISTER_TYPE_UW;
   this->pixel_y.type = BRW_REGISTER_TYPE_UW;
   emit(fs_inst(BRW_OPCODE_ADD,
		this->pixel_x,
		fs_reg(stride(suboffset(g1_uw, 4), 2, 4, 0)),
		fs_reg(brw_imm_v(0x10101010))));
   emit(fs_inst(BRW_OPCODE_ADD,
		this->pixel_y,
		fs_reg(stride(suboffset(g1_uw, 5), 2, 4, 0)),
		fs_reg(brw_imm_v(0x11001100))));

   this->current_annotation = "compute pixel deltas from v0";
   if (brw->has_pln) {
      this->delta_x = fs_reg(this, glsl_type::vec2_type);
      this->delta_y = this->delta_x;
      this->delta_y.reg_offset++;
   } else {
      this->delta_x = fs_reg(this, glsl_type::float_type);
      this->delta_y = fs_reg(this, glsl_type::float_type);
   }
   emit(fs_inst(BRW_OPCODE_ADD,
		this->delta_x,
		this->pixel_x,
		fs_reg(negate(brw_vec1_grf(1, 0)))));
   emit(fs_inst(BRW_OPCODE_ADD,
		this->delta_y,
		this->pixel_y,
		fs_reg(negate(brw_vec1_grf(1, 1)))));

   this->current_annotation = "compute pos.w and 1/pos.w";
   /* Compute wpos.w.  It's always in our setup, since it's needed to
    * interpolate the other attributes.
    */
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit(fs_inst(FS_OPCODE_LINTERP, wpos_w, this->delta_x, this->delta_y,
		interp_reg(FRAG_ATTRIB_WPOS, 3)));
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
   emit(fs_inst(BRW_OPCODE_ADD,
		int_pixel_x,
		fs_reg(stride(suboffset(g1_uw, 4), 2, 4, 0)),
		fs_reg(brw_imm_v(0x10101010))));
   emit(fs_inst(BRW_OPCODE_ADD,
		int_pixel_y,
		fs_reg(stride(suboffset(g1_uw, 5), 2, 4, 0)),
		fs_reg(brw_imm_v(0x11001100))));

   /* As of gen6, we can no longer mix float and int sources.  We have
    * to turn the integer pixel centers into floats for their actual
    * use.
    */
   this->pixel_x = fs_reg(this, glsl_type::float_type);
   this->pixel_y = fs_reg(this, glsl_type::float_type);
   emit(fs_inst(BRW_OPCODE_MOV, this->pixel_x, int_pixel_x));
   emit(fs_inst(BRW_OPCODE_MOV, this->pixel_y, int_pixel_y));

   this->current_annotation = "compute 1/pos.w";
   this->wpos_w = fs_reg(brw_vec8_grf(c->key.source_w_reg, 0));
   this->pixel_w = fs_reg(this, glsl_type::float_type);
   emit_math(FS_OPCODE_RCP, this->pixel_w, wpos_w);

   this->delta_x = fs_reg(brw_vec8_grf(2, 0));
   this->delta_y = fs_reg(brw_vec8_grf(3, 0));

   this->current_annotation = NULL;
}

void
fs_visitor::emit_fb_writes()
{
   this->current_annotation = "FB write header";
   GLboolean header_present = GL_TRUE;
   int nr = 0;

   if (intel->gen >= 6 &&
       !this->kill_emitted &&
       c->key.nr_color_regions == 1) {
      header_present = false;
   }

   if (header_present) {
      /* m0, m1 header */
      nr += 2;
   }

   if (c->key.aa_dest_stencil_reg) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		   fs_reg(brw_vec8_grf(c->key.aa_dest_stencil_reg, 0))));
   }

   /* Reserve space for color. It'll be filled in per MRT below. */
   int color_mrf = nr;
   nr += 4;

   if (c->key.source_depth_to_render_target) {
      if (c->key.computes_depth) {
	 /* Hand over gl_FragDepth. */
	 assert(this->frag_depth);
	 fs_reg depth = *(variable_storage(this->frag_depth));

	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++), depth));
      } else {
	 /* Pass through the payload depth. */
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		      fs_reg(brw_vec8_grf(c->key.source_depth_reg, 0))));
      }
   }

   if (c->key.dest_depth_reg) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		   fs_reg(brw_vec8_grf(c->key.dest_depth_reg, 0))));
   }

   fs_reg color = reg_undef;
   if (this->frag_color)
      color = *(variable_storage(this->frag_color));
   else if (this->frag_data)
      color = *(variable_storage(this->frag_data));

   for (int target = 0; target < c->key.nr_color_regions; target++) {
      this->current_annotation = talloc_asprintf(this->mem_ctx,
						 "FB write target %d",
						 target);
      if (this->frag_color || this->frag_data) {
	 for (int i = 0; i < 4; i++) {
	    emit(fs_inst(BRW_OPCODE_MOV,
			 fs_reg(MRF, color_mrf + i),
			 color));
	    color.reg_offset++;
	 }
      }

      if (this->frag_color)
	 color.reg_offset -= 4;

      fs_inst *inst = emit(fs_inst(FS_OPCODE_FB_WRITE,
				   reg_undef, reg_undef));
      inst->target = target;
      inst->base_mrf = 0;
      inst->mlen = nr;
      if (target == c->key.nr_color_regions - 1)
	 inst->eot = true;
      inst->header_present = header_present;
   }

   if (c->key.nr_color_regions == 0) {
      fs_inst *inst = emit(fs_inst(FS_OPCODE_FB_WRITE,
				   reg_undef, reg_undef));
      inst->base_mrf = 0;
      inst->mlen = nr;
      inst->eot = true;
      inst->header_present = header_present;
   }

   this->current_annotation = NULL;
}

void
fs_visitor::generate_fb_write(fs_inst *inst)
{
   GLboolean eot = inst->eot;
   struct brw_reg implied_header;

   /* Header is 2 regs, g0 and g1 are the contents. g0 will be implied
    * move, here's g1.
    */
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   if (inst->header_present) {
      if (intel->gen >= 6) {
	 brw_MOV(p,
		 brw_message_reg(inst->base_mrf),
		 brw_vec8_grf(0, 0));
	 implied_header = brw_null_reg();
      } else {
	 implied_header = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW);
      }

      brw_MOV(p,
	      brw_message_reg(inst->base_mrf + 1),
	      brw_vec8_grf(1, 0));
   } else {
      implied_header = brw_null_reg();
   }

   brw_pop_insn_state(p);

   brw_fb_WRITE(p,
		8, /* dispatch_width */
		retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW),
		inst->base_mrf,
		implied_header,
		inst->target,
		inst->mlen,
		0,
		eot);
}

void
fs_visitor::generate_linterp(fs_inst *inst,
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
fs_visitor::generate_math(fs_inst *inst,
			  struct brw_reg dst, struct brw_reg *src)
{
   int op;

   switch (inst->opcode) {
   case FS_OPCODE_RCP:
      op = BRW_MATH_FUNCTION_INV;
      break;
   case FS_OPCODE_RSQ:
      op = BRW_MATH_FUNCTION_RSQ;
      break;
   case FS_OPCODE_SQRT:
      op = BRW_MATH_FUNCTION_SQRT;
      break;
   case FS_OPCODE_EXP2:
      op = BRW_MATH_FUNCTION_EXP;
      break;
   case FS_OPCODE_LOG2:
      op = BRW_MATH_FUNCTION_LOG;
      break;
   case FS_OPCODE_POW:
      op = BRW_MATH_FUNCTION_POW;
      break;
   case FS_OPCODE_SIN:
      op = BRW_MATH_FUNCTION_SIN;
      break;
   case FS_OPCODE_COS:
      op = BRW_MATH_FUNCTION_COS;
      break;
   default:
      assert(!"not reached: unknown math function");
      op = 0;
      break;
   }

   if (intel->gen >= 6) {
      assert(inst->mlen == 0);

      if (inst->opcode == FS_OPCODE_POW) {
	 brw_math2(p, dst, op, src[0], src[1]);
      } else {
	 brw_math(p, dst,
		  op,
		  inst->saturate ? BRW_MATH_SATURATE_SATURATE :
		  BRW_MATH_SATURATE_NONE,
		  0, src[0],
		  BRW_MATH_DATA_VECTOR,
		  BRW_MATH_PRECISION_FULL);
      }
   } else {
      assert(inst->mlen >= 1);

      brw_math(p, dst,
	       op,
	       inst->saturate ? BRW_MATH_SATURATE_SATURATE :
	       BRW_MATH_SATURATE_NONE,
	       inst->base_mrf, src[0],
	       BRW_MATH_DATA_VECTOR,
	       BRW_MATH_PRECISION_FULL);
   }
}

void
fs_visitor::generate_tex(fs_inst *inst, struct brw_reg dst)
{
   int msg_type = -1;
   int rlen = 4;
   uint32_t simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD8;

   if (intel->gen >= 5) {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_COMPARE_GEN5;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_GEN5;
	 }
	 break;
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE_GEN5;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_BIAS_GEN5;
	 }
	 break;
      }
   } else {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
	 /* Note that G45 and older determines shadow compare and dispatch width
	  * from message length for most messages.
	  */
	 msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE;
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 5);
	 } else {
	    assert(inst->mlen <= 6);
	 }
	 break;
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 5);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE;
	 } else {
	    assert(inst->mlen == 8);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS;
	    simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 }
	 break;
      }
   }
   assert(msg_type != -1);

   if (simd_mode == BRW_SAMPLER_SIMD_MODE_SIMD16) {
      rlen = 8;
      dst = vec16(dst);
   }

   brw_SAMPLE(p,
	      retype(dst, BRW_REGISTER_TYPE_UW),
	      inst->base_mrf,
	      retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
              SURF_INDEX_TEXTURE(inst->sampler),
	      inst->sampler,
	      WRITEMASK_XYZW,
	      msg_type,
	      rlen,
	      inst->mlen,
	      0,
	      1,
	      simd_mode);
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
fs_visitor::generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
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

void
fs_visitor::generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
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
   brw_ADD(p, dst, src0, negate(src1));
}

void
fs_visitor::generate_discard_not(fs_inst *inst, struct brw_reg mask)
{
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_NOT(p, mask, brw_mask_reg(1)); /* IMASK */
   brw_pop_insn_state(p);
}

void
fs_visitor::generate_discard_and(fs_inst *inst, struct brw_reg mask)
{
   struct brw_reg g0 = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);
   mask = brw_uw1_reg(mask.file, mask.nr, 0);

   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_AND(p, g0, mask, g0);
   brw_pop_insn_state(p);
}

void
fs_visitor::assign_curb_setup()
{
   c->prog_data.first_curbe_grf = c->key.nr_payload_regs;
   c->prog_data.curb_read_length = ALIGN(c->prog_data.nr_params, 8) / 8;

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == UNIFORM) {
	    int constant_nr = inst->src[i].hw_reg + inst->src[i].reg_offset;
	    struct brw_reg brw_reg = brw_vec1_grf(c->prog_data.first_curbe_grf +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = FIXED_HW_REG;
	    inst->src[i].fixed_hw_reg = brw_reg;
	 }
      }
   }
}

void
fs_visitor::calculate_urb_setup()
{
   for (unsigned int i = 0; i < FRAG_ATTRIB_MAX; i++) {
      urb_setup[i] = -1;
   }

   int urb_next = 0;
   /* Figure out where each of the incoming setup attributes lands. */
   if (intel->gen >= 6) {
      for (unsigned int i = 0; i < FRAG_ATTRIB_MAX; i++) {
	 if (brw->fragment_program->Base.InputsRead & BITFIELD64_BIT(i)) {
	    urb_setup[i] = urb_next++;
	 }
      }
   } else {
      /* FINISHME: The sf doesn't map VS->FS inputs for us very well. */
      for (unsigned int i = 0; i < VERT_RESULT_MAX; i++) {
	 if (c->key.vp_outputs_written & BITFIELD64_BIT(i)) {
	    int fp_index;

	    if (i >= VERT_RESULT_VAR0)
	       fp_index = i - (VERT_RESULT_VAR0 - FRAG_ATTRIB_VAR0);
	    else if (i <= VERT_RESULT_TEX7)
	       fp_index = i;
	    else
	       fp_index = -1;

	    if (fp_index >= 0)
	       urb_setup[fp_index] = urb_next++;
	 }
      }
   }

   /* Each attribute is 4 setup channels, each of which is half a reg. */
   c->prog_data.urb_read_length = urb_next * 2;
}

void
fs_visitor::assign_urb_setup()
{
   int urb_start = c->prog_data.first_curbe_grf + c->prog_data.curb_read_length;

   /* Offset all the urb_setup[] index by the actual position of the
    * setup regs, now that the location of the constants has been chosen.
    */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode != FS_OPCODE_LINTERP)
	 continue;

      assert(inst->src[2].file == FIXED_HW_REG);

      inst->src[2].fixed_hw_reg.nr += urb_start;
   }

   this->first_non_payload_grf = urb_start + c->prog_data.urb_read_length;
}

static void
assign_reg(int *reg_hw_locations, fs_reg *reg)
{
   if (reg->file == GRF && reg->reg != 0) {
      assert(reg->reg_offset >= 0);
      reg->hw_reg = reg_hw_locations[reg->reg] + reg->reg_offset;
      reg->reg = 0;
   }
}

void
fs_visitor::assign_regs_trivial()
{
   int last_grf = 0;
   int hw_reg_mapping[this->virtual_grf_next];
   int i;

   hw_reg_mapping[0] = 0;
   hw_reg_mapping[1] = this->first_non_payload_grf;
   for (i = 2; i < this->virtual_grf_next; i++) {
      hw_reg_mapping[i] = (hw_reg_mapping[i - 1] +
			   this->virtual_grf_sizes[i - 1]);
   }
   last_grf = hw_reg_mapping[i - 1] + this->virtual_grf_sizes[i - 1];

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst);
      assign_reg(hw_reg_mapping, &inst->src[0]);
      assign_reg(hw_reg_mapping, &inst->src[1]);
   }

   this->grf_used = last_grf + 1;
}

void
fs_visitor::assign_regs()
{
   int last_grf = 0;
   int hw_reg_mapping[this->virtual_grf_next + 1];
   int base_reg_count = BRW_MAX_GRF - this->first_non_payload_grf;
   int class_sizes[base_reg_count];
   int class_count = 0;
   int aligned_pair_class = -1;

   /* Set up the register classes.
    *
    * The base registers store a scalar value.  For texture samples,
    * we get virtual GRFs composed of 4 contiguous hw register.  For
    * structures and arrays, we store them as contiguous larger things
    * than that, though we should be able to do better most of the
    * time.
    */
   class_sizes[class_count++] = 1;
   if (brw->has_pln && intel->gen < 6) {
      /* Always set up the (unaligned) pairs for gen5, so we can find
       * them for making the aligned pair class.
       */
      class_sizes[class_count++] = 2;
   }
   for (int r = 1; r < this->virtual_grf_next; r++) {
      int i;

      for (i = 0; i < class_count; i++) {
	 if (class_sizes[i] == this->virtual_grf_sizes[r])
	    break;
      }
      if (i == class_count) {
	 if (this->virtual_grf_sizes[r] >= base_reg_count) {
	    fprintf(stderr, "Object too large to register allocate.\n");
	    this->fail = true;
	 }

	 class_sizes[class_count++] = this->virtual_grf_sizes[r];
      }
   }

   int ra_reg_count = 0;
   int class_base_reg[class_count];
   int class_reg_count[class_count];
   int classes[class_count + 1];

   for (int i = 0; i < class_count; i++) {
      class_base_reg[i] = ra_reg_count;
      class_reg_count[i] = base_reg_count - (class_sizes[i] - 1);
      ra_reg_count += class_reg_count[i];
   }

   struct ra_regs *regs = ra_alloc_reg_set(ra_reg_count);
   for (int i = 0; i < class_count; i++) {
      classes[i] = ra_alloc_reg_class(regs);

      for (int i_r = 0; i_r < class_reg_count[i]; i_r++) {
	 ra_class_add_reg(regs, classes[i], class_base_reg[i] + i_r);
      }

      /* Add conflicts between our contiguous registers aliasing
       * base regs and other register classes' contiguous registers
       * that alias base regs, or the base regs themselves for classes[0].
       */
      for (int c = 0; c <= i; c++) {
	 for (int i_r = 0; i_r < class_reg_count[i]; i_r++) {
	    for (int c_r = MAX2(0, i_r - (class_sizes[c] - 1));
		 c_r < MIN2(class_reg_count[c], i_r + class_sizes[i]);
		 c_r++) {

	       if (0) {
		  printf("%d/%d conflicts %d/%d\n",
			 class_sizes[i], this->first_non_payload_grf + i_r,
			 class_sizes[c], this->first_non_payload_grf + c_r);
	       }

	       ra_add_reg_conflict(regs,
				   class_base_reg[i] + i_r,
				   class_base_reg[c] + c_r);
	    }
	 }
      }
   }

   /* Add a special class for aligned pairs, which we'll put delta_x/y
    * in on gen5 so that we can do PLN.
    */
   if (brw->has_pln && intel->gen < 6) {
      int reg_count = (base_reg_count - 1) / 2;
      int unaligned_pair_class = 1;
      assert(class_sizes[unaligned_pair_class] == 2);

      aligned_pair_class = class_count;
      classes[aligned_pair_class] = ra_alloc_reg_class(regs);
      class_sizes[aligned_pair_class] = 2;
      class_base_reg[aligned_pair_class] = 0;
      class_reg_count[aligned_pair_class] = 0;
      int start = (this->first_non_payload_grf & 1) ? 1 : 0;

      for (int i = 0; i < reg_count; i++) {
	 ra_class_add_reg(regs, classes[aligned_pair_class],
			  class_base_reg[unaligned_pair_class] + i * 2 + start);
      }
      class_count++;
   }

   ra_set_finalize(regs);

   struct ra_graph *g = ra_alloc_interference_graph(regs,
						    this->virtual_grf_next);
   /* Node 0 is just a placeholder to keep virtual_grf[] mapping 1:1
    * with nodes.
    */
   ra_set_node_class(g, 0, classes[0]);

   for (int i = 1; i < this->virtual_grf_next; i++) {
      for (int c = 0; c < class_count; c++) {
	 if (class_sizes[c] == this->virtual_grf_sizes[i]) {
	    if (aligned_pair_class >= 0 &&
		this->delta_x.reg == i) {
	       ra_set_node_class(g, i, classes[aligned_pair_class]);
	    } else {
	       ra_set_node_class(g, i, classes[c]);
	    }
	    break;
	 }
      }

      for (int j = 1; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   /* FINISHME: Handle spilling */
   if (!ra_allocate_no_spills(g)) {
      fprintf(stderr, "Failed to allocate registers.\n");
      this->fail = true;
      return;
   }

   /* Get the chosen virtual registers for each node, and map virtual
    * regs in the register classes back down to real hardware reg
    * numbers.
    */
   hw_reg_mapping[0] = 0; /* unused */
   for (int i = 1; i < this->virtual_grf_next; i++) {
      int reg = ra_get_node_reg(g, i);
      int hw_reg = -1;

      for (int c = 0; c < class_count; c++) {
	 if (reg >= class_base_reg[c] &&
	     reg < class_base_reg[c] + class_reg_count[c]) {
	    hw_reg = reg - class_base_reg[c];
	    break;
	 }
      }

      assert(hw_reg >= 0);
      hw_reg_mapping[i] = this->first_non_payload_grf + hw_reg;
      last_grf = MAX2(last_grf,
		      hw_reg_mapping[i] + this->virtual_grf_sizes[i] - 1);
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst);
      assign_reg(hw_reg_mapping, &inst->src[0]);
      assign_reg(hw_reg_mapping, &inst->src[1]);
   }

   this->grf_used = last_grf + 1;

   talloc_free(g);
   talloc_free(regs);
}

/**
 * Split large virtual GRFs into separate components if we can.
 *
 * This is mostly duplicated with what brw_fs_vector_splitting does,
 * but that's really conservative because it's afraid of doing
 * splitting that doesn't result in real progress after the rest of
 * the optimization phases, which would cause infinite looping in
 * optimization.  We can do it once here, safely.  This also has the
 * opportunity to split interpolated values, or maybe even uniforms,
 * which we don't have at the IR level.
 *
 * We want to split, because virtual GRFs are what we register
 * allocate and spill (due to contiguousness requirements for some
 * instructions), and they're what we naturally generate in the
 * codegen process, but most virtual GRFs don't actually need to be
 * contiguous sets of GRFs.  If we split, we'll end up with reduced
 * live intervals and better dead code elimination and coalescing.
 */
void
fs_visitor::split_virtual_grfs()
{
   int num_vars = this->virtual_grf_next;
   bool split_grf[num_vars];
   int new_virtual_grf[num_vars];

   /* Try to split anything > 0 sized. */
   for (int i = 0; i < num_vars; i++) {
      if (this->virtual_grf_sizes[i] != 1)
	 split_grf[i] = true;
      else
	 split_grf[i] = false;
   }

   if (brw->has_pln) {
      /* PLN opcodes rely on the delta_xy being contiguous. */
      split_grf[this->delta_x.reg] = false;
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      /* Texturing produces 4 contiguous registers, so no splitting. */
      if ((inst->opcode == FS_OPCODE_TEX ||
	   inst->opcode == FS_OPCODE_TXB ||
	   inst->opcode == FS_OPCODE_TXL) &&
	  inst->dst.file == GRF) {
	 split_grf[inst->dst.reg] = false;
      }
   }

   /* Allocate new space for split regs.  Note that the virtual
    * numbers will be contiguous.
    */
   for (int i = 0; i < num_vars; i++) {
      if (split_grf[i]) {
	 new_virtual_grf[i] = virtual_grf_alloc(1);
	 for (int j = 2; j < this->virtual_grf_sizes[i]; j++) {
	    int reg = virtual_grf_alloc(1);
	    assert(reg == new_virtual_grf[i] + j - 1);
	 }
	 this->virtual_grf_sizes[i] = 1;
      }
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->dst.file == GRF &&
	  split_grf[inst->dst.reg] &&
	  inst->dst.reg_offset != 0) {
	 inst->dst.reg = (new_virtual_grf[inst->dst.reg] +
			  inst->dst.reg_offset - 1);
	 inst->dst.reg_offset = 0;
      }
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     split_grf[inst->src[i].reg] &&
	     inst->src[i].reg_offset != 0) {
	    inst->src[i].reg = (new_virtual_grf[inst->src[i].reg] +
				inst->src[i].reg_offset - 1);
	    inst->src[i].reg_offset = 0;
	 }
      }
   }
}

void
fs_visitor::calculate_live_intervals()
{
   int num_vars = this->virtual_grf_next;
   int *def = talloc_array(mem_ctx, int, num_vars);
   int *use = talloc_array(mem_ctx, int, num_vars);
   int loop_depth = 0;
   int loop_start = 0;

   for (int i = 0; i < num_vars; i++) {
      def[i] = 1 << 30;
      use[i] = -1;
   }

   int ip = 0;
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode == BRW_OPCODE_DO) {
	 if (loop_depth++ == 0)
	    loop_start = ip;
      } else if (inst->opcode == BRW_OPCODE_WHILE) {
	 loop_depth--;

	 if (loop_depth == 0) {
	    /* FINISHME:
	     *
	     * Patches up any vars marked for use within the loop as
	     * live until the end.  This is conservative, as there
	     * will often be variables defined and used inside the
	     * loop but dead at the end of the loop body.
	     */
	    for (int i = 0; i < num_vars; i++) {
	       if (use[i] == loop_start) {
		  use[i] = ip;
	       }
	    }
	 }
      } else {
	 int eip = ip;

	 if (loop_depth)
	    eip = loop_start;

	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF && inst->src[i].reg != 0) {
	       use[inst->src[i].reg] = MAX2(use[inst->src[i].reg], eip);
	    }
	 }
	 if (inst->dst.file == GRF && inst->dst.reg != 0) {
	    def[inst->dst.reg] = MIN2(def[inst->dst.reg], eip);
	 }
      }

      ip++;
   }

   talloc_free(this->virtual_grf_def);
   talloc_free(this->virtual_grf_use);
   this->virtual_grf_def = def;
   this->virtual_grf_use = use;
}

/**
 * Attempts to move immediate constants into the immediate
 * constant slot of following instructions.
 *
 * Immediate constants are a bit tricky -- they have to be in the last
 * operand slot, you can't do abs/negate on them,
 */

bool
fs_visitor::propagate_constants()
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != GRF || inst->src[0].file != IMM ||
	  inst->dst.type != inst->src[0].type)
	 continue;

      /* Don't bother with cases where we should have had the
       * operation on the constant folded in GLSL already.
       */
      if (inst->saturate)
	 continue;

      /* Found a move of a constant to a GRF.  Find anything else using the GRF
       * before it's written, and replace it with the constant if we can.
       */
      exec_list_iterator scan_iter = iter;
      scan_iter.next();
      for (; scan_iter.has_next(); scan_iter.next()) {
	 fs_inst *scan_inst = (fs_inst *)scan_iter.get();

	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ELSE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    break;
	 }

	 for (int i = 2; i >= 0; i--) {
	    if (scan_inst->src[i].file != GRF ||
		scan_inst->src[i].reg != inst->dst.reg ||
		scan_inst->src[i].reg_offset != inst->dst.reg_offset)
	       continue;

	    /* Don't bother with cases where we should have had the
	     * operation on the constant folded in GLSL already.
	     */
	    if (scan_inst->src[i].negate || scan_inst->src[i].abs)
	       continue;

	    switch (scan_inst->opcode) {
	    case BRW_OPCODE_MOV:
	       scan_inst->src[i] = inst->src[0];
	       progress = true;
	       break;

	    case BRW_OPCODE_MUL:
	    case BRW_OPCODE_ADD:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  /* Fit this constant in by commuting the operands */
		  scan_inst->src[0] = scan_inst->src[1];
		  scan_inst->src[1] = inst->src[0];
	       }
	       break;
	    case BRW_OPCODE_CMP:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       }
	    }
	 }

	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->dst.reg &&
	     (scan_inst->dst.reg_offset == inst->dst.reg_offset ||
	      scan_inst->opcode == FS_OPCODE_TEX)) {
	    break;
	 }
      }
   }

   return progress;
}
/**
 * Must be called after calculate_live_intervales() to remove unused
 * writes to registers -- register allocation will fail otherwise
 * because something deffed but not used won't be considered to
 * interfere with other regs.
 */
bool
fs_visitor::dead_code_eliminate()
{
   bool progress = false;
   int num_vars = this->virtual_grf_next;
   bool dead[num_vars];

   for (int i = 0; i < num_vars; i++) {
      dead[i] = this->virtual_grf_def[i] >= this->virtual_grf_use[i];

      if (dead[i]) {
	 /* Mark off its interval so it won't interfere with anything. */
	 this->virtual_grf_def[i] = -1;
	 this->virtual_grf_use[i] = -1;
      }
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->dst.file == GRF && dead[inst->dst.reg]) {
	 inst->remove();
	 progress = true;
      }
   }

   return progress;
}

bool
fs_visitor::register_coalesce()
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->saturate ||
	  inst->dst.file != GRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type)
	 continue;

      /* Found a move of a GRF to a GRF.  Let's see if we can coalesce
       * them: check for no writes to either one until the exit of the
       * program.
       */
      bool interfered = false;
      exec_list_iterator scan_iter = iter;
      scan_iter.next();
      for (; scan_iter.has_next(); scan_iter.next()) {
	 fs_inst *scan_inst = (fs_inst *)scan_iter.get();

	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    interfered = true;
	    iter = scan_iter;
	    break;
	 }

	 if (scan_inst->dst.file == GRF) {
	    if (scan_inst->dst.reg == inst->dst.reg &&
		(scan_inst->dst.reg_offset == inst->dst.reg_offset ||
		 scan_inst->opcode == FS_OPCODE_TEX)) {
	       interfered = true;
	       break;
	    }
	    if (scan_inst->dst.reg == inst->src[0].reg &&
		(scan_inst->dst.reg_offset == inst->src[0].reg_offset ||
		 scan_inst->opcode == FS_OPCODE_TEX)) {
	       interfered = true;
	       break;
	    }
	 }
      }
      if (interfered) {
	 continue;
      }

      /* Update live interval so we don't have to recalculate. */
      this->virtual_grf_use[inst->src[0].reg] = MAX2(virtual_grf_use[inst->src[0].reg],
						     virtual_grf_use[inst->dst.reg]);

      /* Rewrite the later usage to point at the source of the move to
       * be removed.
       */
      for (exec_list_iterator scan_iter = iter; scan_iter.has_next();
	   scan_iter.next()) {
	 fs_inst *scan_inst = (fs_inst *)scan_iter.get();

	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->dst.reg &&
		scan_inst->src[i].reg_offset == inst->dst.reg_offset) {
	       scan_inst->src[i].reg = inst->src[0].reg;
	       scan_inst->src[i].reg_offset = inst->src[0].reg_offset;
	       scan_inst->src[i].abs |= inst->src[0].abs;
	       scan_inst->src[i].negate ^= inst->src[0].negate;
	    }
	 }
      }

      inst->remove();
      progress = true;
   }

   return progress;
}


bool
fs_visitor::compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate)
	 continue;

      /* Can't compute-to-MRF this GRF if someone else was going to
       * read it later.
       */
      if (this->virtual_grf_use[inst->src[0].reg] > ip)
	 continue;

      /* Found a move of a GRF to a MRF.  Let's see if we can go
       * rewrite the thing that made this GRF to write into the MRF.
       */
      bool found = false;
      fs_inst *scan_inst;
      for (scan_inst = (fs_inst *)inst->prev;
	   scan_inst->prev != NULL;
	   scan_inst = (fs_inst *)scan_inst->prev) {
	 /* We don't handle flow control here.  Most computation of
	  * values that end up in MRFs are shortly before the MRF
	  * write anyway.
	  */
	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    break;
	 }

	 /* You can't read from an MRF, so if someone else reads our
	  * MRF's source GRF that we wanted to rewrite, that stops us.
	  */
	 bool interfered = false;
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->src[0].reg &&
		scan_inst->src[i].reg_offset == inst->src[0].reg_offset) {
	       interfered = true;
	    }
	 }
	 if (interfered)
	    break;

	 if (scan_inst->dst.file == MRF &&
	     scan_inst->dst.hw_reg == inst->dst.hw_reg) {
	    /* Somebody else wrote our MRF here, so we can't can't
	     * compute-to-MRF before that.
	     */
	    break;
	 }

	 if (scan_inst->mlen > 0) {
	    /* Found a SEND instruction, which will do some amount of
	     * implied write that may overwrite our MRF that we were
	     * hoping to compute-to-MRF somewhere above it.  Nothing
	     * we have implied-writes more than 2 MRFs from base_mrf,
	     * though.
	     */
	    int implied_write_len = MIN2(scan_inst->mlen, 2);
	    if (inst->dst.hw_reg >= scan_inst->base_mrf &&
		inst->dst.hw_reg < scan_inst->base_mrf + implied_write_len) {
	       break;
	    }
	 }

	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg) {
	    /* Found the last thing to write our reg we want to turn
	     * into a compute-to-MRF.
	     */

	    if (scan_inst->opcode == FS_OPCODE_TEX) {
	       /* texturing writes several continuous regs, so we can't
		* compute-to-mrf that.
		*/
	       break;
	    }

	    /* If it's predicated, it (probably) didn't populate all
	     * the channels.
	     */
	    if (scan_inst->predicated)
	       break;

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (intel->gen >= 6) {
	       /* gen6 math instructions must have the destination be
		* GRF, so no compute-to-MRF for them.
		*/
	       if (scan_inst->opcode == FS_OPCODE_RCP ||
		   scan_inst->opcode == FS_OPCODE_RSQ ||
		   scan_inst->opcode == FS_OPCODE_SQRT ||
		   scan_inst->opcode == FS_OPCODE_EXP2 ||
		   scan_inst->opcode == FS_OPCODE_LOG2 ||
		   scan_inst->opcode == FS_OPCODE_SIN ||
		   scan_inst->opcode == FS_OPCODE_COS ||
		   scan_inst->opcode == FS_OPCODE_POW) {
		  break;
	       }
	    }

	    if (scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	       /* Found the creator of our MRF's source value. */
	       found = true;
	       break;
	    }
	 }
      }
      if (found) {
	 scan_inst->dst.file = MRF;
	 scan_inst->dst.hw_reg = inst->dst.hw_reg;
	 scan_inst->saturate |= inst->saturate;
	 inst->remove();
	 progress = true;
      }
   }

   return progress;
}

bool
fs_visitor::virtual_grf_interferes(int a, int b)
{
   int start = MAX2(this->virtual_grf_def[a], this->virtual_grf_def[b]);
   int end = MIN2(this->virtual_grf_use[a], this->virtual_grf_use[b]);

   /* For dead code, just check if the def interferes with the other range. */
   if (this->virtual_grf_use[a] == -1) {
      return (this->virtual_grf_def[a] >= this->virtual_grf_def[b] &&
	      this->virtual_grf_def[a] < this->virtual_grf_use[b]);
   }
   if (this->virtual_grf_use[b] == -1) {
      return (this->virtual_grf_def[b] >= this->virtual_grf_def[a] &&
	      this->virtual_grf_def[b] < this->virtual_grf_use[a]);
   }

   return start < end;
}

static struct brw_reg brw_reg_from_fs_reg(fs_reg *reg)
{
   struct brw_reg brw_reg;

   switch (reg->file) {
   case GRF:
   case ARF:
   case MRF:
      brw_reg = brw_vec8_reg(reg->file,
			    reg->hw_reg, 0);
      brw_reg = retype(brw_reg, reg->type);
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
   }
   if (reg->abs)
      brw_reg = brw_abs(brw_reg);
   if (reg->negate)
      brw_reg = negate(brw_reg);

   return brw_reg;
}

void
fs_visitor::generate_code()
{
   unsigned int annotation_len = 0;
   int last_native_inst = 0;
   struct brw_instruction *if_stack[16], *loop_stack[16];
   int if_stack_depth = 0, loop_stack_depth = 0;
   int if_depth_in_loop[16];

   if_depth_in_loop[loop_stack_depth] = 0;

   memset(&if_stack, 0, sizeof(if_stack));
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();
      struct brw_reg src[3], dst;

      for (unsigned int i = 0; i < 3; i++) {
	 src[i] = brw_reg_from_fs_reg(&inst->src[i]);
      }
      dst = brw_reg_from_fs_reg(&inst->dst);

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicated);

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

      case BRW_OPCODE_IF:
	 assert(if_stack_depth < 16);
	 if_stack[if_stack_depth] = brw_IF(p, BRW_EXECUTE_8);
	 if_depth_in_loop[loop_stack_depth]++;
	 if_stack_depth++;
	 break;
      case BRW_OPCODE_ELSE:
	 if_stack[if_stack_depth - 1] =
	    brw_ELSE(p, if_stack[if_stack_depth - 1]);
	 break;
      case BRW_OPCODE_ENDIF:
	 if_stack_depth--;
	 brw_ENDIF(p , if_stack[if_stack_depth]);
	 if_depth_in_loop[loop_stack_depth]--;
	 break;

      case BRW_OPCODE_DO:
	 loop_stack[loop_stack_depth++] = brw_DO(p, BRW_EXECUTE_8);
	 if_depth_in_loop[loop_stack_depth] = 0;
	 break;

      case BRW_OPCODE_BREAK:
	 brw_BREAK(p, if_depth_in_loop[loop_stack_depth]);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;
      case BRW_OPCODE_CONTINUE:
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
	 break;

      case FS_OPCODE_RCP:
      case FS_OPCODE_RSQ:
      case FS_OPCODE_SQRT:
      case FS_OPCODE_EXP2:
      case FS_OPCODE_LOG2:
      case FS_OPCODE_POW:
      case FS_OPCODE_SIN:
      case FS_OPCODE_COS:
	 generate_math(inst, dst, src);
	 break;
      case FS_OPCODE_LINTERP:
	 generate_linterp(inst, dst, src);
	 break;
      case FS_OPCODE_TEX:
      case FS_OPCODE_TXB:
      case FS_OPCODE_TXL:
	 generate_tex(inst, dst);
	 break;
      case FS_OPCODE_DISCARD_NOT:
	 generate_discard_not(inst, dst);
	 break;
      case FS_OPCODE_DISCARD_AND:
	 generate_discard_and(inst, src[0]);
	 break;
      case FS_OPCODE_DDX:
	 generate_ddx(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DDY:
	 generate_ddy(inst, dst, src[0]);
	 break;
      case FS_OPCODE_FB_WRITE:
	 generate_fb_write(inst);
	 break;
      default:
	 if (inst->opcode < (int)ARRAY_SIZE(brw_opcodes)) {
	    _mesa_problem(ctx, "Unsupported opcode `%s' in FS",
			  brw_opcodes[inst->opcode].name);
	 } else {
	    _mesa_problem(ctx, "Unsupported opcode %d in FS", inst->opcode);
	 }
	 this->fail = true;
      }

      if (annotation_len < p->nr_insn) {
	 annotation_len *= 2;
	 if (annotation_len < 16)
	    annotation_len = 16;

	 this->annotation_string = talloc_realloc(this->mem_ctx,
						  annotation_string,
						  const char *,
						  annotation_len);
	 this->annotation_ir = talloc_realloc(this->mem_ctx,
					      annotation_ir,
					      ir_instruction *,
					      annotation_len);
      }

      for (unsigned int i = last_native_inst; i < p->nr_insn; i++) {
	 this->annotation_string[i] = inst->annotation;
	 this->annotation_ir[i] = inst->ir;
      }
      last_native_inst = p->nr_insn;
   }
}

GLboolean
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct brw_shader *shader = NULL;
   struct gl_shader_program *prog = ctx->Shader.CurrentProgram;

   if (!prog)
      return GL_FALSE;

   if (!using_new_fs)
      return GL_FALSE;

   for (unsigned int i = 0; i < prog->_NumLinkedShaders; i++) {
      if (prog->_LinkedShaders[i]->Type == GL_FRAGMENT_SHADER) {
	 shader = (struct brw_shader *)prog->_LinkedShaders[i];
	 break;
      }
   }
   if (!shader)
      return GL_FALSE;

   /* We always use 8-wide mode, at least for now.  For one, flow
    * control only works in 8-wide.  Also, when we're fragment shader
    * bound, we're almost always under register pressure as well, so
    * 8-wide would save us from the performance cliff of spilling
    * regs.
    */
   c->dispatch_width = 8;

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("GLSL IR for native fragment shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n");
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   fs_visitor v(c, shader);

   if (0) {
      v.emit_dummy_fs();
   } else {
      v.calculate_urb_setup();
      if (intel->gen < 6)
	 v.emit_interpolation_setup_gen4();
      else
	 v.emit_interpolation_setup_gen6();

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      foreach_iter(exec_list_iterator, iter, *shader->ir) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 v.base_ir = ir;
	 ir->accept(&v);
      }

      v.emit_fb_writes();

      v.split_virtual_grfs();

      v.assign_curb_setup();
      v.assign_urb_setup();

      bool progress;
      do {
	 progress = false;
	 v.calculate_live_intervals();
	 progress = v.propagate_constants() || progress;
	 progress = v.register_coalesce() || progress;
	 progress = v.compute_to_mrf() || progress;
	 progress = v.dead_code_eliminate() || progress;
      } while (progress);

      if (0)
	 v.assign_regs_trivial();
      else
	 v.assign_regs();
   }

   if (!v.fail)
      v.generate_code();

   assert(!v.fail); /* FINISHME: Cleanly fail, tested at link time, etc. */

   if (v.fail)
      return GL_FALSE;

   if (INTEL_DEBUG & DEBUG_WM) {
      const char *last_annotation_string = NULL;
      ir_instruction *last_annotation_ir = NULL;

      printf("Native code for fragment shader %d:\n", prog->Name);
      for (unsigned int i = 0; i < p->nr_insn; i++) {
	 if (last_annotation_ir != v.annotation_ir[i]) {
	    last_annotation_ir = v.annotation_ir[i];
	    if (last_annotation_ir) {
	       printf("   ");
	       last_annotation_ir->print();
	       printf("\n");
	    }
	 }
	 if (last_annotation_string != v.annotation_string[i]) {
	    last_annotation_string = v.annotation_string[i];
	    if (last_annotation_string)
	       printf("   %s\n", last_annotation_string);
	 }
	 brw_disasm(stdout, &p->store[i], intel->gen);
      }
      printf("\n");
   }

   c->prog_data.total_grf = v.grf_used;
   c->prog_data.total_scratch = 0;

   return GL_TRUE;
}
