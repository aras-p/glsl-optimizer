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
#include "../glsl/ir_print_visitor.h"

#define MAX_INSTRUCTION (1 << 30)
static struct brw_reg brw_reg_from_fs_reg(class fs_reg *reg);

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

void
fs_visitor::fail(const char *format, ...)
{
   if (!failed) {
      failed = true;

      if (INTEL_DEBUG & DEBUG_WM) {
	 fprintf(stderr, "FS compile failed: ");

	 va_list va;
	 va_start(va, format);
	 vfprintf(stderr, format, va);
	 va_end(va);
      }
   }
}

void
fs_visitor::push_force_uncompressed()
{
   force_uncompressed_stack++;
}

void
fs_visitor::pop_force_uncompressed()
{
   force_uncompressed_stack--;
   assert(force_uncompressed_stack >= 0);
}

void
fs_visitor::push_force_sechalf()
{
   force_sechalf_stack++;
}

void
fs_visitor::pop_force_sechalf()
{
   force_sechalf_stack--;
   assert(force_sechalf_stack >= 0);
}

/**
 * Returns how many MRFs an FS opcode will write over.
 *
 * Note that this is not the 0 or 1 implied writes in an actual gen
 * instruction -- the FS opcodes often generate MOVs in addition.
 */
int
fs_visitor::implied_mrf_writes(fs_inst *inst)
{
   if (inst->mlen == 0)
      return 0;

   switch (inst->opcode) {
   case FS_OPCODE_RCP:
   case FS_OPCODE_RSQ:
   case FS_OPCODE_SQRT:
   case FS_OPCODE_EXP2:
   case FS_OPCODE_LOG2:
   case FS_OPCODE_SIN:
   case FS_OPCODE_COS:
      return 1 * c->dispatch_width / 8;
   case FS_OPCODE_POW:
      return 2 * c->dispatch_width / 8;
   case FS_OPCODE_TEX:
   case FS_OPCODE_TXB:
   case FS_OPCODE_TXD:
   case FS_OPCODE_TXL:
      return 1;
   case FS_OPCODE_FB_WRITE:
      return 2;
   case FS_OPCODE_PULL_CONSTANT_LOAD:
   case FS_OPCODE_UNSPILL:
      return 1;
   case FS_OPCODE_SPILL:
      return 2;
   default:
      assert(!"not reached");
      return inst->mlen;
   }
}

int
fs_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_next) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);

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

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int hw_reg, uint32_t type)
{
   init();
   this->file = file;
   this->hw_reg = hw_reg;
   this->type = type;
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

void
import_uniforms_callback(const void *key,
			 void *data,
			 void *closure)
{
   struct hash_table *dst_ht = (struct hash_table *)closure;
   const fs_reg *reg = (const fs_reg *)data;

   if (reg->file != UNIFORM)
      return;

   hash_table_insert(dst_ht, data, key);
}

/* For 16-wide, we need to follow from the uniform setup of 8-wide dispatch.
 * This brings in those uniform definitions
 */
void
fs_visitor::import_uniforms(struct hash_table *src_variable_ht)
{
   hash_table_call_foreach(src_variable_ht,
			   import_uniforms_callback,
			   variable_ht);
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
	 unsigned int param = c->prog_data.nr_params++;

	 assert(param < ARRAY_SIZE(c->prog_data.param));

	 switch (type->base_type) {
	 case GLSL_TYPE_FLOAT:
	    c->prog_data.param_convert[param] = PARAM_NO_CONVERT;
	    break;
	 case GLSL_TYPE_UINT:
	    c->prog_data.param_convert[param] = PARAM_CONVERT_F2U;
	    break;
	 case GLSL_TYPE_INT:
	    c->prog_data.param_convert[param] = PARAM_CONVERT_F2I;
	    break;
	 case GLSL_TYPE_BOOL:
	    c->prog_data.param_convert[param] = PARAM_CONVERT_F2B;
	    break;
	 default:
	    assert(!"not reached");
	    c->prog_data.param_convert[param] = PARAM_NO_CONVERT;
	    break;
	 }
	 this->param_index[param] = loc;
	 this->param_offset[param] = i;
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
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa, but we'll
       * get the same index back here.
       */
      int index = _mesa_add_state_reference(this->fp->Base.Parameters,
					    (gl_state_index *)slots[i].tokens);

      /* Add each of the unique swizzles of the element as a parameter.
       * This'll end up matching the expected layout of the
       * array/matrix/structure we're trying to fill in.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 if (swiz == last_swiz)
	    break;
	 last_swiz = swiz;

	 c->prog_data.param_convert[c->prog_data.nr_params] =
	    PARAM_NO_CONVERT;
	 this->param_index[c->prog_data.nr_params] = index;
	 this->param_offset[c->prog_data.nr_params] = swiz;
	 c->prog_data.nr_params++;
      }
   }
}

fs_reg *
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   bool flip = !ir->origin_upper_left ^ c->key.render_to_fbo;

   /* gl_FragCoord.x */
   if (ir->pixel_center_integer) {
      emit(BRW_OPCODE_MOV, wpos, this->pixel_x);
   } else {
      emit(BRW_OPCODE_ADD, wpos, this->pixel_x, fs_reg(0.5f));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (!flip && ir->pixel_center_integer) {
      emit(BRW_OPCODE_MOV, wpos, this->pixel_y);
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->pixel_center_integer ? 0.0 : 0.5);

      if (flip) {
	 pixel_y.negate = true;
	 offset += c->key.drawable_height - 1.0;
      }

      emit(BRW_OPCODE_ADD, wpos, pixel_y, fs_reg(offset));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   if (intel->gen >= 6) {
      emit(BRW_OPCODE_MOV, wpos,
	   fs_reg(brw_vec8_grf(c->source_depth_reg, 0)));
   } else {
      emit(FS_OPCODE_LINTERP, wpos, this->delta_x, this->delta_y,
	   interp_reg(FRAG_ATTRIB_WPOS, 2));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.w: Already set up in emit_interpolation */
   emit(BRW_OPCODE_MOV, wpos, this->wpos_w);

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
	 fail("dereferenced array '%s' has length 0\n", ir->name);
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

	 bool is_gl_Color =
	    location == FRAG_ATTRIB_COL0 || location == FRAG_ATTRIB_COL1;

	 if (c->key.flat_shade && is_gl_Color) {
	    /* Constant interpolation (flat shading) case. The SF has
	     * handed us defined values in only the constant offset
	     * field of the setup reg.
	     */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       struct brw_reg interp = interp_reg(location, k);
	       interp = suboffset(interp, 3);
	       emit(FS_OPCODE_CINTERP, attr, fs_reg(interp));
	       attr.reg_offset++;
	    }
	 } else {
	    /* Perspective interpolation case. */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       struct brw_reg interp = interp_reg(location, k);
	       emit(FS_OPCODE_LINTERP, attr,
		    this->delta_x, this->delta_y, fs_reg(interp));
	       attr.reg_offset++;
	    }

	    if (intel->gen < 6) {
	       attr.reg_offset -= type->vector_elements;
	       for (unsigned int k = 0; k < type->vector_elements; k++) {
		  emit(BRW_OPCODE_MUL, attr, attr, this->pixel_w);
		  attr.reg_offset++;
	       }
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
      emit(BRW_OPCODE_ASR, *reg,
	   fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_D)),
	   fs_reg(15));
      emit(BRW_OPCODE_NOT, *reg, *reg);
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1));
   } else {
      struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
      /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
       * us front face
       */
      fs_inst *inst = emit(BRW_OPCODE_CMP, *reg,
			   fs_reg(r1_6ud),
			   fs_reg(1u << 31));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1u));
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
    *
    * The hardware ignores source modifiers (negate and abs) on math
    * instructions, so we also move to a temp to set those up.
    */
   if (intel->gen >= 6 && (src.file == UNIFORM ||
			   src.abs ||
			   src.negate)) {
      fs_reg expanded = fs_reg(this, glsl_type::float_type);
      emit(BRW_OPCODE_MOV, expanded, src);
      src = expanded;
   }

   fs_inst *inst = emit(opcode, dst, src);

   if (intel->gen < 6) {
      inst->base_mrf = 2;
      inst->mlen = c->dispatch_width / 8;
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
      /* Can't do hstride == 0 args to gen6 math, so expand it out.
       *
       * The hardware ignores source modifiers (negate and abs) on math
       * instructions, so we also move to a temp to set those up.
       */
      if (src0.file == UNIFORM || src0.abs || src0.negate) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 emit(BRW_OPCODE_MOV, expanded, src0);
	 src0 = expanded;
      }

      if (src1.file == UNIFORM || src1.abs || src1.negate) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 emit(BRW_OPCODE_MOV, expanded, src1);
	 src1 = expanded;
      }

      inst = emit(opcode, dst, src0, src1);
   } else {
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + 1), src1);
      inst = emit(opcode, dst, src0, reg_null_f);

      inst->base_mrf = base_mrf;
      inst->mlen = 2 * c->dispatch_width / 8;
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
					     ctx->Shader.CurrentFragmentProgram,
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
	 brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
	 brw_MOV(p,
		 retype(brw_message_reg(inst->base_mrf), BRW_REGISTER_TYPE_UD),
		 retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);

	 if (inst->target > 0) {
	    /* Set the render target index for choosing BLEND_STATE. */
	    brw_MOV(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, 0, 2),
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

   brw_pop_insn_state(p);

   brw_fb_WRITE(p,
		c->dispatch_width,
		inst->base_mrf,
		implied_header,
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
fs_visitor::generate_pixel_xy(struct brw_reg dst, bool is_x)
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

   if (c->dispatch_width == 16) {
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
	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	 brw_math2(p, dst, op, src[0], src[1]);

	 if (c->dispatch_width == 16) {
	    brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
	    brw_math2(p, sechalf(dst), op, sechalf(src[0]), sechalf(src[1]));
	    brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
	 }
      } else {
	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	 brw_math(p, dst,
		  op,
		  inst->saturate ? BRW_MATH_SATURATE_SATURATE :
		  BRW_MATH_SATURATE_NONE,
		  0, src[0],
		  BRW_MATH_DATA_VECTOR,
		  BRW_MATH_PRECISION_FULL);

	 if (c->dispatch_width == 16) {
	    brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
	    brw_math(p, sechalf(dst),
		     op,
		     inst->saturate ? BRW_MATH_SATURATE_SATURATE :
		     BRW_MATH_SATURATE_NONE,
		     0, sechalf(src[0]),
		     BRW_MATH_DATA_VECTOR,
		     BRW_MATH_PRECISION_FULL);
	    brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
	 }
      }
   } else /* gen <= 5 */{
      assert(inst->mlen >= 1);

      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_math(p, dst,
	       op,
	       inst->saturate ? BRW_MATH_SATURATE_SATURATE :
	       BRW_MATH_SATURATE_NONE,
	       inst->base_mrf, src[0],
	       BRW_MATH_DATA_VECTOR,
	       BRW_MATH_PRECISION_FULL);

      if (c->dispatch_width == 16) {
	 brw_set_compression_control(p, BRW_COMPRESSION_2NDHALF);
	 brw_math(p, sechalf(dst),
		  op,
		  inst->saturate ? BRW_MATH_SATURATE_SATURATE :
		  BRW_MATH_SATURATE_NONE,
		  inst->base_mrf + 1, sechalf(src[0]),
		  BRW_MATH_DATA_VECTOR,
		  BRW_MATH_PRECISION_FULL);

	 brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
      }
   }
}

void
fs_visitor::generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   int msg_type = -1;
   int rlen = 4;
   uint32_t simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD8;

   if (c->dispatch_width == 16)
      simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;

   if (intel->gen >= 5) {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
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
      case FS_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE;
	 } else {
	    msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LOD;
	 }
	 break;
      case FS_OPCODE_TXD:
	 assert(!"TXD isn't supported on gen5+ yet.");
	 break;
      }
   } else {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
	 /* Note that G45 and older determines shadow compare and dispatch width
	  * from message length for most messages.
	  */
	 assert(c->dispatch_width == 8);
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
      case FS_OPCODE_TXL:
	 if (inst->shadow_compare) {
	    assert(inst->mlen == 6);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_LOD_COMPARE;
	 } else {
	    assert(inst->mlen == 9);
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_LOD;
	    simd_mode = BRW_SAMPLER_SIMD_MODE_SIMD16;
	 }
	 break;
      case FS_OPCODE_TXD:
	 assert(!"TXD isn't supported on gen4 yet.");
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
	      src,
              SURF_INDEX_TEXTURE(inst->sampler),
	      inst->sampler,
	      WRITEMASK_XYZW,
	      msg_type,
	      rlen,
	      inst->mlen,
	      0,
	      inst->header_present,
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
fs_visitor::generate_discard(fs_inst *inst)
{
   struct brw_reg f0 = brw_flag_reg();

   if (intel->gen >= 6) {
      struct brw_reg g1 = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
      struct brw_reg some_register;

      /* As of gen6, we no longer have the mask register to look at,
       * so life gets a bit more complicated.
       */

      /* Load the flag register with all ones. */
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_MOV(p, f0, brw_imm_uw(0xffff));
      brw_pop_insn_state(p);

      /* Do a comparison that should always fail, to produce 0s in the flag
       * reg where we have active channels.
       */
      some_register = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW);
      brw_CMP(p, retype(brw_null_reg(), BRW_REGISTER_TYPE_UD),
	      BRW_CONDITIONAL_NZ, some_register, some_register);

      /* Undo CMP's whacking of predication*/
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);

      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_AND(p, g1, f0, g1);
      brw_pop_insn_state(p);
   } else {
      struct brw_reg g0 = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);

      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);

      /* Unlike the 965, we have the mask reg, so we just need
       * somewhere to invert that (containing channels to be disabled)
       * so it can be ANDed with the mask of pixels still to be
       * written. Use the flag reg for consistency with gen6+.
       */
      brw_NOT(p, f0, brw_mask_reg(1)); /* IMASK */
      brw_AND(p, g0, f0, g0);

      brw_pop_insn_state(p);
   }
}

void
fs_visitor::generate_spill(fs_inst *inst, struct brw_reg src)
{
   assert(inst->mlen != 0);

   brw_MOV(p,
	   retype(brw_message_reg(inst->base_mrf + 1), BRW_REGISTER_TYPE_UD),
	   retype(src, BRW_REGISTER_TYPE_UD));
   brw_oword_block_write_scratch(p, brw_message_reg(inst->base_mrf), 1,
				 inst->offset);
}

void
fs_visitor::generate_unspill(fs_inst *inst, struct brw_reg dst)
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
fs_visitor::generate_pull_constant_load(fs_inst *inst, struct brw_reg dst)
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

   brw_oword_block_read(p, dst, brw_message_reg(inst->base_mrf),
			inst->offset, SURF_INDEX_FRAG_CONST_BUFFER);

   if (intel->gen == 4 && !intel->is_g4x) {
      /* gen4 errata: destination from a send can't be used as a
       * destination until it's been read.  Just read it so we don't
       * have to worry.
       */
      brw_MOV(p, brw_null_reg(), dst);
   }
}

/**
 * To be called after the last _mesa_add_state_reference() call, to
 * set up prog_data.param[] for assign_curb_setup() and
 * setup_pull_constants().
 */
void
fs_visitor::setup_paramvalues_refs()
{
   if (c->dispatch_width != 8)
      return;

   /* Set up the pointers to ParamValues now that that array is finalized. */
   for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
      c->prog_data.param[i] =
	 fp->Base.Parameters->ParameterValues[this->param_index[i]] +
	 this->param_offset[i];
   }
}

void
fs_visitor::assign_curb_setup()
{
   c->prog_data.curb_read_length = ALIGN(c->prog_data.nr_params, 8) / 8;
   if (c->dispatch_width == 8) {
      c->prog_data.first_curbe_grf = c->nr_payload_regs;
   } else {
      c->prog_data.first_curbe_grf_16 = c->nr_payload_regs;
   }

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == UNIFORM) {
	    int constant_nr = inst->src[i].hw_reg + inst->src[i].reg_offset;
	    struct brw_reg brw_reg = brw_vec1_grf(c->nr_payload_regs +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = FIXED_HW_REG;
	    inst->src[i].fixed_hw_reg = retype(brw_reg, inst->src[i].type);
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
   int urb_start = c->nr_payload_regs + c->prog_data.curb_read_length;

   /* Offset all the urb_setup[] index by the actual position of the
    * setup regs, now that the location of the constants has been chosen.
    */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode == FS_OPCODE_LINTERP) {
	 assert(inst->src[2].file == FIXED_HW_REG);
	 inst->src[2].fixed_hw_reg.nr += urb_start;
      }

      if (inst->opcode == FS_OPCODE_CINTERP) {
	 assert(inst->src[0].file == FIXED_HW_REG);
	 inst->src[0].fixed_hw_reg.nr += urb_start;
      }
   }

   this->first_non_payload_grf = urb_start + c->prog_data.urb_read_length;
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
      if (inst->is_tex()) {
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
	    (void) reg;
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
   this->live_intervals_valid = false;
}

/**
 * Choose accesses from the UNIFORM file to demote to using the pull
 * constant buffer.
 *
 * We allow a fragment shader to have more than the specified minimum
 * maximum number of fragment shader uniform components (64).  If
 * there are too many of these, they'd fill up all of register space.
 * So, this will push some of them out to the pull constant buffer and
 * update the program to load them.
 */
void
fs_visitor::setup_pull_constants()
{
   /* Only allow 16 registers (128 uniform components) as push constants. */
   unsigned int max_uniform_components = 16 * 8;
   if (c->prog_data.nr_params <= max_uniform_components)
      return;

   if (c->dispatch_width == 16) {
      fail("Pull constants not supported in 16-wide\n");
      return;
   }

   /* Just demote the end of the list.  We could probably do better
    * here, demoting things that are rarely used in the program first.
    */
   int pull_uniform_base = max_uniform_components;
   int pull_uniform_count = c->prog_data.nr_params - pull_uniform_base;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

	 int uniform_nr = inst->src[i].hw_reg + inst->src[i].reg_offset;
	 if (uniform_nr < pull_uniform_base)
	    continue;

	 fs_reg dst = fs_reg(this, glsl_type::float_type);
	 fs_inst *pull = new(mem_ctx) fs_inst(FS_OPCODE_PULL_CONSTANT_LOAD,
					      dst);
	 pull->offset = ((uniform_nr - pull_uniform_base) * 4) & ~15;
	 pull->ir = inst->ir;
	 pull->annotation = inst->annotation;
	 pull->base_mrf = 14;
	 pull->mlen = 1;

	 inst->insert_before(pull);

	 inst->src[i].file = GRF;
	 inst->src[i].reg = dst.reg;
	 inst->src[i].reg_offset = 0;
	 inst->src[i].smear = (uniform_nr - pull_uniform_base) & 3;
      }
   }

   for (int i = 0; i < pull_uniform_count; i++) {
      c->prog_data.pull_param[i] = c->prog_data.param[pull_uniform_base + i];
      c->prog_data.pull_param_convert[i] =
	 c->prog_data.param_convert[pull_uniform_base + i];
   }
   c->prog_data.nr_params -= pull_uniform_count;
   c->prog_data.nr_pull_params = pull_uniform_count;
}

void
fs_visitor::calculate_live_intervals()
{
   int num_vars = this->virtual_grf_next;
   int *def = ralloc_array(mem_ctx, int, num_vars);
   int *use = ralloc_array(mem_ctx, int, num_vars);
   int loop_depth = 0;
   int loop_start = 0;

   if (this->live_intervals_valid)
      return;

   for (int i = 0; i < num_vars; i++) {
      def[i] = MAX_INSTRUCTION;
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
	    /* Patches up the use of vars marked for being live across
	     * the whole loop.
	     */
	    for (int i = 0; i < num_vars; i++) {
	       if (use[i] == loop_start) {
		  use[i] = ip;
	       }
	    }
	 }
      } else {
	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF && inst->src[i].reg != 0) {
	       int reg = inst->src[i].reg;

	       if (!loop_depth) {
		  use[reg] = ip;
	       } else {
		  def[reg] = MIN2(loop_start, def[reg]);
		  use[reg] = loop_start;

		  /* Nobody else is going to go smash our start to
		   * later in the loop now, because def[reg] now
		   * points before the bb header.
		   */
	       }
	    }
	 }
	 if (inst->dst.file == GRF && inst->dst.reg != 0) {
	    int reg = inst->dst.reg;

	    if (!loop_depth) {
	       def[reg] = MIN2(def[reg], ip);
	    } else {
	       def[reg] = MIN2(def[reg], loop_start);
	    }
	 }
      }

      ip++;
   }

   ralloc_free(this->virtual_grf_def);
   ralloc_free(this->virtual_grf_use);
   this->virtual_grf_def = def;
   this->virtual_grf_use = use;

   this->live_intervals_valid = true;
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

   calculate_live_intervals();

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != GRF || inst->src[0].file != IMM ||
	  inst->dst.type != inst->src[0].type ||
	  (c->dispatch_width == 16 &&
	   (inst->force_uncompressed || inst->force_sechalf)))
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
		  progress = true;
	       }
	       break;

	    case BRW_OPCODE_CMP:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  uint32_t new_cmod;

		  new_cmod = brw_swap_cmod(scan_inst->conditional_mod);
		  if (new_cmod != ~0u) {
		     /* Fit this constant in by swapping the operands and
		      * flipping the test
		      */
		     scan_inst->src[0] = scan_inst->src[1];
		     scan_inst->src[1] = inst->src[0];
		     scan_inst->conditional_mod = new_cmod;
		     progress = true;
		  }
	       }
	       break;

	    case BRW_OPCODE_SEL:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  /* Fit this constant in by swapping the operands and
		   * flipping the predicate
		   */
		  scan_inst->src[0] = scan_inst->src[1];
		  scan_inst->src[1] = inst->src[0];
		  scan_inst->predicate_inverse = !scan_inst->predicate_inverse;
		  progress = true;
	       }
	       break;
	    }
	 }

	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->dst.reg &&
	     (scan_inst->dst.reg_offset == inst->dst.reg_offset ||
	      scan_inst->is_tex())) {
	    break;
	 }
      }
   }

   if (progress)
       this->live_intervals_valid = false;

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
   int pc = 0;

   calculate_live_intervals();

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->dst.file == GRF && this->virtual_grf_use[inst->dst.reg] <= pc) {
	 inst->remove();
	 progress = true;
      }

      pc++;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

bool
fs_visitor::register_coalesce()
{
   bool progress = false;
   int if_depth = 0;
   int loop_depth = 0;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      /* Make sure that we dominate the instructions we're going to
       * scan for interfering with our coalescing, or we won't have
       * scanned enough to see if anything interferes with our
       * coalescing.  We don't dominate the following instructions if
       * we're in a loop or an if block.
       */
      switch (inst->opcode) {
      case BRW_OPCODE_DO:
	 loop_depth++;
	 break;
      case BRW_OPCODE_WHILE:
	 loop_depth--;
	 break;
      case BRW_OPCODE_IF:
	 if_depth++;
	 break;
      case BRW_OPCODE_ENDIF:
	 if_depth--;
	 break;
      }
      if (loop_depth || if_depth)
	 continue;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->saturate ||
	  inst->dst.file != GRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type)
	 continue;

      bool has_source_modifiers = inst->src[0].abs || inst->src[0].negate;

      /* Found a move of a GRF to a GRF.  Let's see if we can coalesce
       * them: check for no writes to either one until the exit of the
       * program.
       */
      bool interfered = false;
      exec_list_iterator scan_iter = iter;
      scan_iter.next();
      for (; scan_iter.has_next(); scan_iter.next()) {
	 fs_inst *scan_inst = (fs_inst *)scan_iter.get();

	 if (scan_inst->dst.file == GRF) {
	    if (scan_inst->dst.reg == inst->dst.reg &&
		(scan_inst->dst.reg_offset == inst->dst.reg_offset ||
		 scan_inst->is_tex())) {
	       interfered = true;
	       break;
	    }
	    if (scan_inst->dst.reg == inst->src[0].reg &&
		(scan_inst->dst.reg_offset == inst->src[0].reg_offset ||
		 scan_inst->is_tex())) {
	       interfered = true;
	       break;
	    }
	 }

	 /* The gen6 MATH instruction can't handle source modifiers, so avoid
	  * coalescing those for now.  We should do something more specific.
	  */
	 if (intel->gen >= 6 && scan_inst->is_math() && has_source_modifiers) {
	    interfered = true;
	    break;
	 }
      }
      if (interfered) {
	 continue;
      }

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
	       scan_inst->src[i].smear = inst->src[0].smear;
	    }
	 }
      }

      inst->remove();
      progress = true;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}


bool
fs_visitor::compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   calculate_live_intervals();

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate || inst->src[0].smear != -1)
	 continue;

      /* Work out which hardware MRF registers are written by this
       * instruction.
       */
      int mrf_low = inst->dst.hw_reg & ~BRW_MRF_COMPR4;
      int mrf_high;
      if (inst->dst.hw_reg & BRW_MRF_COMPR4) {
	 mrf_high = mrf_low + 4;
      } else if (c->dispatch_width == 16 &&
		 (!inst->force_uncompressed && !inst->force_sechalf)) {
	 mrf_high = mrf_low + 1;
      } else {
	 mrf_high = mrf_low;
      }

      /* Can't compute-to-MRF this GRF if someone else was going to
       * read it later.
       */
      if (this->virtual_grf_use[inst->src[0].reg] > ip)
	 continue;

      /* Found a move of a GRF to a MRF.  Let's see if we can go
       * rewrite the thing that made this GRF to write into the MRF.
       */
      fs_inst *scan_inst;
      for (scan_inst = (fs_inst *)inst->prev;
	   scan_inst->prev != NULL;
	   scan_inst = (fs_inst *)scan_inst->prev) {
	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg) {
	    /* Found the last thing to write our reg we want to turn
	     * into a compute-to-MRF.
	     */

	    if (scan_inst->is_tex()) {
	       /* texturing writes several continuous regs, so we can't
		* compute-to-mrf that.
		*/
	       break;
	    }

	    /* If it's predicated, it (probably) didn't populate all
	     * the channels.  We might be able to rewrite everything
	     * that writes that reg, but it would require smarter
	     * tracking to delay the rewriting until complete success.
	     */
	    if (scan_inst->predicated)
	       break;

	    /* If it's half of register setup and not the same half as
	     * our MOV we're trying to remove, bail for now.
	     */
	    if (scan_inst->force_uncompressed != inst->force_uncompressed ||
		scan_inst->force_sechalf != inst->force_sechalf) {
	       break;
	    }

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (intel->gen >= 6) {
	       /* gen6 math instructions must have the destination be
		* GRF, so no compute-to-MRF for them.
		*/
	       if (scan_inst->is_math()) {
		  break;
	       }
	    }

	    if (scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	       /* Found the creator of our MRF's source value. */
	       scan_inst->dst.file = MRF;
	       scan_inst->dst.hw_reg = inst->dst.hw_reg;
	       scan_inst->saturate |= inst->saturate;
	       inst->remove();
	       progress = true;
	    }
	    break;
	 }

	 /* We don't handle flow control here.  Most computation of
	  * values that end up in MRFs are shortly before the MRF
	  * write anyway.
	  */
	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ELSE ||
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

	 if (scan_inst->dst.file == MRF) {
	    /* If somebody else writes our MRF here, we can't
	     * compute-to-MRF before that.
	     */
	    int scan_mrf_low = scan_inst->dst.hw_reg & ~BRW_MRF_COMPR4;
	    int scan_mrf_high;

	    if (scan_inst->dst.hw_reg & BRW_MRF_COMPR4) {
	       scan_mrf_high = scan_mrf_low + 4;
	    } else if (c->dispatch_width == 16 &&
		       (!scan_inst->force_uncompressed &&
			!scan_inst->force_sechalf)) {
	       scan_mrf_high = scan_mrf_low + 1;
	    } else {
	       scan_mrf_high = scan_mrf_low;
	    }

	    if (mrf_low == scan_mrf_low ||
		mrf_low == scan_mrf_high ||
		mrf_high == scan_mrf_low ||
		mrf_high == scan_mrf_high) {
	       break;
	    }
	 }

	 if (scan_inst->mlen > 0) {
	    /* Found a SEND instruction, which means that there are
	     * live values in MRFs from base_mrf to base_mrf +
	     * scan_inst->mlen - 1.  Don't go pushing our MRF write up
	     * above it.
	     */
	    if (mrf_low >= scan_inst->base_mrf &&
		mrf_low < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	    if (mrf_high >= scan_inst->base_mrf &&
		mrf_high < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	 }
      }
   }

   return progress;
}

/**
 * Walks through basic blocks, locking for repeated MRF writes and
 * removing the later ones.
 */
bool
fs_visitor::remove_duplicate_mrf_writes()
{
   fs_inst *last_mrf_move[16];
   bool progress = false;

   /* Need to update the MRF tracking for compressed instructions. */
   if (c->dispatch_width == 16)
      return false;

   memset(last_mrf_move, 0, sizeof(last_mrf_move));

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      switch (inst->opcode) {
      case BRW_OPCODE_DO:
      case BRW_OPCODE_WHILE:
      case BRW_OPCODE_IF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_ENDIF:
	 memset(last_mrf_move, 0, sizeof(last_mrf_move));
	 continue;
      default:
	 break;
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF) {
	 fs_inst *prev_inst = last_mrf_move[inst->dst.hw_reg];
	 if (prev_inst && inst->equals(prev_inst)) {
	    inst->remove();
	    progress = true;
	    continue;
	 }
      }

      /* Clear out the last-write records for MRFs that were overwritten. */
      if (inst->dst.file == MRF) {
	 last_mrf_move[inst->dst.hw_reg] = NULL;
      }

      if (inst->mlen > 0) {
	 /* Found a SEND instruction, which will include two or fewer
	  * implied MRF writes.  We could do better here.
	  */
	 for (int i = 0; i < implied_mrf_writes(inst); i++) {
	    last_mrf_move[inst->base_mrf + i] = NULL;
	 }
      }

      /* Clear out any MRF move records whose sources got overwritten. */
      if (inst->dst.file == GRF) {
	 for (unsigned int i = 0; i < Elements(last_mrf_move); i++) {
	    if (last_mrf_move[i] &&
		last_mrf_move[i]->src[0].reg == inst->dst.reg) {
	       last_mrf_move[i] = NULL;
	    }
	 }
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF &&
	  inst->src[0].file == GRF &&
	  !inst->predicated) {
	 last_mrf_move[inst->dst.hw_reg] = inst;
      }
   }

   return progress;
}

bool
fs_visitor::virtual_grf_interferes(int a, int b)
{
   int start = MAX2(this->virtual_grf_def[a], this->virtual_grf_def[b]);
   int end = MIN2(this->virtual_grf_use[a], this->virtual_grf_use[b]);

   /* We can't handle dead register writes here, without iterating
    * over the whole instruction stream to find every single dead
    * write to that register to compare to the live interval of the
    * other register.  Just assert that dead_code_eliminate() has been
    * called.
    */
   assert((this->virtual_grf_use[a] != -1 ||
	   this->virtual_grf_def[a] == MAX_INSTRUCTION) &&
	  (this->virtual_grf_use[b] != -1 ||
	   this->virtual_grf_def[b] == MAX_INSTRUCTION));

   /* If the register is used to store 16 values of less than float
    * size (only the case for pixel_[xy]), then we can't allocate
    * another dword-sized thing to that register that would be used in
    * the same instruction.  This is because when the GPU decodes (for
    * example):
    *
    * (declare (in ) vec4 gl_FragCoord@0x97766a0)
    * add(16)         g6<1>F          g6<8,8,1>UW     0.5F { align1 compr };
    *
    * it's actually processed as:
    * add(8)         g6<1>F          g6<8,8,1>UW     0.5F { align1 };
    * add(8)         g7<1>F          g6.8<8,8,1>UW   0.5F { align1 sechalf };
    *
    * so our second half values in g6 got overwritten in the first
    * half.
    */
   if (c->dispatch_width == 16 && (this->pixel_x.reg == a ||
				   this->pixel_x.reg == b ||
				   this->pixel_y.reg == a ||
				   this->pixel_y.reg == b)) {
      return start <= end;
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
      if (reg->smear == -1) {
	 brw_reg = brw_vec8_reg(reg->file,
				reg->hw_reg, 0);
      } else {
	 brw_reg = brw_vec1_reg(reg->file,
				reg->hw_reg, reg->smear);
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

void
fs_visitor::generate_code()
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


   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("Native code for fragment shader %d (%d-wide dispatch):\n",
	     ctx->Shader.CurrentFragmentProgram->Name, c->dispatch_width);
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();
      struct brw_reg src[3], dst;

      if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
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
	 src[i] = brw_reg_from_fs_reg(&inst->src[i]);
      }
      dst = brw_reg_from_fs_reg(&inst->dst);

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicated);
      brw_set_predicate_inverse(p, inst->predicate_inverse);
      brw_set_saturate(p, inst->saturate);

      if (inst->force_uncompressed || c->dispatch_width == 8) {
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

      case BRW_OPCODE_IF:
	 if (inst->src[0].file != BAD_FILE) {
	    /* The instruction has an embedded compare (only allowed on gen6) */
	    assert(intel->gen == 6);
	    gen6_IF(p, inst->conditional_mod, src[0], src[1]);
	 } else {
	    brw_IF(p, c->dispatch_width == 16 ? BRW_EXECUTE_16 : BRW_EXECUTE_8);
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
      case FS_OPCODE_TEX:
      case FS_OPCODE_TXB:
      case FS_OPCODE_TXD:
      case FS_OPCODE_TXL:
	 generate_tex(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DISCARD:
	 generate_discard(inst);
	 break;
      case FS_OPCODE_DDX:
	 generate_ddx(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DDY:
	 generate_ddy(inst, dst, src[0]);
	 break;

      case FS_OPCODE_SPILL:
	 generate_spill(inst, src[0]);
	 break;

      case FS_OPCODE_UNSPILL:
	 generate_unspill(inst, dst);
	 break;

      case FS_OPCODE_PULL_CONSTANT_LOAD:
	 generate_pull_constant_load(inst, dst);
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
	 fail("unsupported opcode in FS\n");
      }

      if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
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

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("\n");
   }

   ralloc_free(loop_stack);
   ralloc_free(if_depth_in_loop);

   brw_set_uip_jip(p);

   /* OK, while the INTEL_DEBUG=wm above is very nice for debugging FS
    * emit issues, it doesn't get the jump distances into the output,
    * which is often something we want to debug.  So this is here in
    * case you're doing that.
    */
   if (0) {
      if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
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

bool
fs_visitor::run()
{
   uint32_t prog_offset_16 = 0;
   uint32_t orig_nr_params = c->prog_data.nr_params;

   brw_wm_payload_setup(brw, c);

   if (c->dispatch_width == 16) {
      /* align to 64 byte boundary. */
      while ((c->func.nr_insn * sizeof(struct brw_instruction)) % 64) {
	 brw_NOP(p);
      }

      /* Save off the start of this 16-wide program in case we succeed. */
      prog_offset_16 = c->func.nr_insn * sizeof(struct brw_instruction);

      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
   }

   if (0) {
      emit_dummy_fs();
   } else {
      calculate_urb_setup();
      if (intel->gen < 6)
	 emit_interpolation_setup_gen4();
      else
	 emit_interpolation_setup_gen6();

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      foreach_iter(exec_list_iterator, iter, *shader->ir) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 base_ir = ir;
	 this->result = reg_undef;
	 ir->accept(this);
      }

      emit_fb_writes();

      split_virtual_grfs();

      setup_paramvalues_refs();
      setup_pull_constants();

      bool progress;
      do {
	 progress = false;

	 progress = remove_duplicate_mrf_writes() || progress;

	 progress = propagate_constants() || progress;
	 progress = register_coalesce() || progress;
	 progress = compute_to_mrf() || progress;
	 progress = dead_code_eliminate() || progress;
      } while (progress);

      schedule_instructions();

      assign_curb_setup();
      assign_urb_setup();

      if (0) {
	 /* Debug of register spilling: Go spill everything. */
	 int virtual_grf_count = virtual_grf_next;
	 for (int i = 1; i < virtual_grf_count; i++) {
	    spill_reg(i);
	 }
      }

      if (0)
	 assign_regs_trivial();
      else {
	 while (!assign_regs()) {
	    if (failed)
	       break;
	 }
      }
   }
   assert(force_uncompressed_stack == 0);
   assert(force_sechalf_stack == 0);

   if (failed)
      return false;

   generate_code();

   if (c->dispatch_width == 8) {
      c->prog_data.reg_blocks = brw_register_blocks(grf_used);
   } else {
      c->prog_data.reg_blocks_16 = brw_register_blocks(grf_used);
      c->prog_data.prog_offset_16 = prog_offset_16;

      /* Make sure we didn't try to sneak in an extra uniform */
      assert(orig_nr_params == c->prog_data.nr_params);
   }

   return !failed;
}

bool
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_shader_program *prog = ctx->Shader.CurrentFragmentProgram;

   if (!prog)
      return false;

   struct brw_shader *shader =
     (brw_shader *) prog->_LinkedShaders[MESA_SHADER_FRAGMENT];
   if (!shader)
      return false;

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("GLSL IR for native fragment shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n\n");
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   c->dispatch_width = 8;

   fs_visitor v(c, shader);
   if (!v.run()) {
      /* FINISHME: Cleanly fail, test at link time, etc. */
      assert(!"not reached");
      return false;
   }

   if (intel->gen >= 5 && c->prog_data.nr_pull_params == 0) {
      c->dispatch_width = 16;
      fs_visitor v2(c, shader);
      v2.import_uniforms(v.variable_ht);
      v2.run();
   }

   c->prog_data.dispatch_width = 8;

   return true;
}
