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

/** @file brw_fs.cpp
 *
 * This file drives the GLSL IR -> LIR translation, contains the
 * optimizations on the LIR, and drives the generation of native code
 * from the LIR.
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

int
fs_visitor::type_size(const struct glsl_type *type)
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
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "FS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_WM) {
      fprintf(stderr, "%s",  msg);
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
   this->reg = v->virtual_grf_alloc(v->type_size(type));
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
	 if (fp->Base.InputsRead & BITFIELD64_BIT(i)) {
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
		  scan_inst->src[0] = scan_inst->src[1];
		  scan_inst->src[1] = inst->src[0];

		  /* If this was predicated, flipping operands means
		   * we also need to flip the predicate.
		   */
		  if (scan_inst->conditional_mod == BRW_CONDITIONAL_NONE) {
		     scan_inst->predicate_inverse =
			!scan_inst->predicate_inverse;
		  }
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
      if (failed)
	 return false;

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
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c,
	       struct gl_shader_program *prog)
{
   struct intel_context *intel = &brw->intel;

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

   fs_visitor v(c, prog, shader);
   if (!v.run()) {
      prog->LinkStatus = GL_FALSE;
      prog->InfoLog = ralloc_strdup(prog, v.fail_msg);

      return false;
   }

   if (intel->gen >= 5 && c->prog_data.nr_pull_params == 0) {
      c->dispatch_width = 16;
      fs_visitor v2(c, prog, shader);
      v2.import_uniforms(v.variable_ht);
      v2.run();
   }

   c->prog_data.dispatch_width = 8;

   return true;
}

bool
brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_wm_prog_key key;
   struct gl_fragment_program *fp = prog->FragmentProgram;
   struct brw_fragment_program *bfp = brw_fragment_program(fp);

   if (!fp)
      return true;

   memset(&key, 0, sizeof(key));

   if (fp->UsesKill)
      key.iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
      key.iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   /* Just assume depth testing. */
   key.iz_lookup |= IZ_DEPTH_TEST_ENABLE_BIT;
   key.iz_lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   key.vp_outputs_written |= BITFIELD64_BIT(FRAG_ATTRIB_WPOS);
   for (int i = 0; i < FRAG_ATTRIB_MAX; i++) {
      int vp_index = -1;

      if (!(fp->Base.InputsRead & BITFIELD64_BIT(i)))
	 continue;

      key.proj_attrib_mask |= 1 << i;

      if (i <= FRAG_ATTRIB_TEX7)
	 vp_index = i;
      else if (i >= FRAG_ATTRIB_VAR0)
	 vp_index = i - FRAG_ATTRIB_VAR0 + VERT_RESULT_VAR0;

      if (vp_index >= 0)
	 key.vp_outputs_written |= BITFIELD64_BIT(vp_index);
   }

   key.clamp_fragment_color = true;

   for (int i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      if (fp->Base.ShadowSamplers & (1 << i))
	 key.compare_funcs[i] = GL_LESS;

      /* FINISHME: depth compares might use (0,0,0,W) for example */
      key.tex_swizzles[i] = SWIZZLE_XYZW;
   }

   if (fp->Base.InputsRead & FRAG_BIT_WPOS) {
      key.drawable_height = ctx->DrawBuffer->Height;
      key.render_to_fbo = ctx->DrawBuffer->Name != 0;
   }

   key.nr_color_regions = 1;

   key.program_string_id = bfp->id;

   uint32_t old_prog_offset = brw->wm.prog_offset;
   struct brw_wm_prog_data *old_prog_data = brw->wm.prog_data;

   bool success = do_wm_prog(brw, prog, bfp, &key);

   brw->wm.prog_offset = old_prog_offset;
   brw->wm.prog_data = old_prog_data;

   return success;
}
