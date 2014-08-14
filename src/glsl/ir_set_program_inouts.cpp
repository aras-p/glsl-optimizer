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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_set_program_inouts.cpp
 *
 * Sets the InputsRead and OutputsWritten of Mesa programs.
 *
 * Additionally, for fragment shaders, sets the InterpQualifier array, the
 * IsCentroid and IsSample bitfields, and the UsesDFdy flag.
 *
 * Mesa programs (gl_program, not gl_shader_program) have a set of
 * flags indicating which varyings are read and written.  Computing
 * which are actually read from some sort of backend code can be
 * tricky when variable array indexing involved.  So this pass
 * provides support for setting InputsRead and OutputsWritten right
 * from the GLSL IR.
 */

#include "main/core.h" /* for struct gl_program */
#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

namespace {

class ir_set_program_inouts_visitor : public ir_hierarchical_visitor {
public:
   ir_set_program_inouts_visitor(struct gl_program *prog,
                                 gl_shader_stage shader_stage)
   {
      this->prog = prog;
      this->shader_stage = shader_stage;
   }
   ~ir_set_program_inouts_visitor()
   {
   }

   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_enter(ir_function_signature *);
   virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_discard *);
   virtual ir_visitor_status visit_enter(ir_texture *);
   virtual ir_visitor_status visit(ir_dereference_variable *);

private:
   void mark_whole_variable(ir_variable *var);
   bool try_mark_partial_variable(ir_variable *var, ir_rvalue *index);

   struct gl_program *prog;
   gl_shader_stage shader_stage;
};

} /* anonymous namespace */

static inline bool
is_shader_inout(ir_variable *var)
{
   return var->data.mode == ir_var_shader_in ||
          var->data.mode == ir_var_shader_out ||
          var->data.mode == ir_var_system_value;
}

static void
mark(struct gl_program *prog, ir_variable *var, int offset, int len,
     bool is_fragment_shader)
{
   /* As of GLSL 1.20, varyings can only be floats, floating-point
    * vectors or matrices, or arrays of them.  For Mesa programs using
    * InputsRead/OutputsWritten, everything but matrices uses one
    * slot, while matrices use a slot per column.  Presumably
    * something doing a more clever packing would use something other
    * than InputsRead/OutputsWritten.
    */

   for (int i = 0; i < len; i++) {
      GLbitfield64 bitfield =
         BITFIELD64_BIT(var->data.location + var->data.index + offset + i);
      if (var->data.mode == ir_var_shader_in) {
	 prog->InputsRead |= bitfield;
         if (is_fragment_shader) {
            gl_fragment_program *fprog = (gl_fragment_program *) prog;
            fprog->InterpQualifier[var->data.location +
                                   var->data.index + offset + i] =
               (glsl_interp_qualifier) var->data.interpolation;
            if (var->data.centroid)
               fprog->IsCentroid |= bitfield;
            if (var->data.sample)
               fprog->IsSample |= bitfield;
         }
      } else if (var->data.mode == ir_var_system_value) {
         prog->SystemValuesRead |= bitfield;
      } else {
         assert(var->data.mode == ir_var_shader_out);
	 prog->OutputsWritten |= bitfield;
      }
   }
}

/**
 * Mark an entire variable as used.  Caller must ensure that the variable
 * represents a shader input or output.
 */
void
ir_set_program_inouts_visitor::mark_whole_variable(ir_variable *var)
{
   const glsl_type *type = var->type;
   if (this->shader_stage == MESA_SHADER_GEOMETRY &&
       var->data.mode == ir_var_shader_in && type->is_array()) {
      type = type->fields.array;
   }

   mark(this->prog, var, 0, type->count_attribute_slots(),
        this->shader_stage == MESA_SHADER_FRAGMENT);
}

/* Default handler: Mark all the locations in the variable as used. */
ir_visitor_status
ir_set_program_inouts_visitor::visit(ir_dereference_variable *ir)
{
   if (!is_shader_inout(ir->var))
      return visit_continue;

   mark_whole_variable(ir->var);

   return visit_continue;
}

/**
 * Try to mark a portion of the given variable as used.  Caller must ensure
 * that the variable represents a shader input or output which can be indexed
 * into in array fashion (an array or matrix).  For the purpose of geometry
 * shader inputs (which are always arrays*), this means that the array element
 * must be something that can be indexed into in array fashion.
 *
 * *Except gl_PrimitiveIDIn, as noted below.
 *
 * If the index can't be interpreted as a constant, or some other problem
 * occurs, then nothing will be marked and false will be returned.
 */
bool
ir_set_program_inouts_visitor::try_mark_partial_variable(ir_variable *var,
                                                         ir_rvalue *index)
{
   const glsl_type *type = var->type;

   if (this->shader_stage == MESA_SHADER_GEOMETRY &&
       var->data.mode == ir_var_shader_in) {
      /* The only geometry shader input that is not an array is
       * gl_PrimitiveIDIn, and in that case, this code will never be reached,
       * because gl_PrimitiveIDIn can't be indexed into in array fashion.
       */
      assert(type->is_array());
      type = type->fields.array;
   }

   /* The code below only handles:
    *
    * - Indexing into matrices
    * - Indexing into arrays of (matrices, vectors, or scalars)
    *
    * All other possibilities are either prohibited by GLSL (vertex inputs and
    * fragment outputs can't be structs) or should have been eliminated by
    * lowering passes (do_vec_index_to_swizzle() gets rid of indexing into
    * vectors, and lower_packed_varyings() gets rid of structs that occur in
    * varyings).
    */
   if (!(type->is_matrix() ||
        (type->is_array() &&
         (type->fields.array->is_numeric() ||
          type->fields.array->is_boolean())))) {
      assert(!"Unexpected indexing in ir_set_program_inouts");

      /* For safety in release builds, in case we ever encounter unexpected
       * indexing, give up and let the caller mark the whole variable as used.
       */
      return false;
   }

   ir_constant *index_as_constant = index->as_constant();
   if (!index_as_constant)
      return false;

   unsigned elem_width;
   unsigned num_elems;
   if (type->is_array()) {
      num_elems = type->length;
      if (type->fields.array->is_matrix())
         elem_width = type->fields.array->matrix_columns;
      else
         elem_width = 1;
   } else {
      num_elems = type->matrix_columns;
      elem_width = 1;
   }

   if (index_as_constant->value.u[0] >= num_elems) {
      /* Constant index outside the bounds of the matrix/array.  This could
       * arise as a result of constant folding of a legal GLSL program.
       *
       * Even though the spec says that indexing outside the bounds of a
       * matrix/array results in undefined behaviour, we don't want to pass
       * out-of-range values to mark() (since this could result in slots that
       * don't exist being marked as used), so just let the caller mark the
       * whole variable as used.
       */
      return false;
   }

   mark(this->prog, var, index_as_constant->value.u[0] * elem_width,
        elem_width, this->shader_stage == MESA_SHADER_FRAGMENT);
   return true;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_dereference_array *ir)
{
   /* Note: for geometry shader inputs, lower_named_interface_blocks may
    * create 2D arrays, so we need to be able to handle those.  2D arrays
    * shouldn't be able to crop up for any other reason.
    */
   if (ir_dereference_array * const inner_array =
       ir->array->as_dereference_array()) {
      /*          ir => foo[i][j]
       * inner_array => foo[i]
       */
      if (ir_dereference_variable * const deref_var =
          inner_array->array->as_dereference_variable()) {
         if (this->shader_stage == MESA_SHADER_GEOMETRY &&
             deref_var->var->data.mode == ir_var_shader_in) {
            /* foo is a geometry shader input, so i is the vertex, and j the
             * part of the input we're accessing.
             */
            if (try_mark_partial_variable(deref_var->var, ir->array_index))
            {
               /* We've now taken care of foo and j, but i might contain a
                * subexpression that accesses shader inputs.  So manually
                * visit i and then continue with the parent.
                */
               inner_array->array_index->accept(this);
               return visit_continue_with_parent;
            }
         }
      }
   } else if (ir_dereference_variable * const deref_var =
              ir->array->as_dereference_variable()) {
      /* ir => foo[i], where foo is a variable. */
      if (this->shader_stage == MESA_SHADER_GEOMETRY &&
          deref_var->var->data.mode == ir_var_shader_in) {
         /* foo is a geometry shader input, so i is the vertex, and we're
          * accessing the entire input.
          */
         mark_whole_variable(deref_var->var);
         /* We've now taken care of foo, but i might contain a subexpression
          * that accesses shader inputs.  So manually visit i and then
          * continue with the parent.
          */
         ir->array_index->accept(this);
         return visit_continue_with_parent;
      } else if (is_shader_inout(deref_var->var)) {
         /* foo is a shader input/output, but not a geometry shader input,
          * so i is the part of the input we're accessing.
          */
         if (try_mark_partial_variable(deref_var->var, ir->array_index))
            return visit_continue_with_parent;
      }
   }

   /* The expression is something we don't recognize.  Just visit its
    * subexpressions.
    */
   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_function_signature *ir)
{
   /* We don't want to descend into the function parameters and
    * consider them as shader inputs or outputs.
    */
   visit_list_elements(this, &ir->body);
   return visit_continue_with_parent;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_expression *ir)
{
   if (this->shader_stage == MESA_SHADER_FRAGMENT &&
       (ir->operation == ir_unop_dFdy ||
        ir->operation == ir_unop_dFdy_coarse ||
        ir->operation == ir_unop_dFdy_fine)) {
      gl_fragment_program *fprog = (gl_fragment_program *) prog;
      fprog->UsesDFdy = true;
   }
   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_discard *)
{
   /* discards are only allowed in fragment shaders. */
   assert(this->shader_stage == MESA_SHADER_FRAGMENT);

   gl_fragment_program *fprog = (gl_fragment_program *) prog;
   fprog->UsesKill = true;

   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_texture *ir)
{
   if (ir->op == ir_tg4)
      prog->UsesGather = true;
   return visit_continue;
}

void
do_set_program_inouts(exec_list *instructions, struct gl_program *prog,
                      gl_shader_stage shader_stage)
{
   ir_set_program_inouts_visitor v(prog, shader_stage);

   prog->InputsRead = 0;
   prog->OutputsWritten = 0;
   prog->SystemValuesRead = 0;
   if (shader_stage == MESA_SHADER_FRAGMENT) {
      gl_fragment_program *fprog = (gl_fragment_program *) prog;
      memset(fprog->InterpQualifier, 0, sizeof(fprog->InterpQualifier));
      fprog->IsCentroid = 0;
      fprog->IsSample = 0;
      fprog->UsesDFdy = false;
      fprog->UsesKill = false;
   }
   visit_list_elements(&v, instructions);
}
