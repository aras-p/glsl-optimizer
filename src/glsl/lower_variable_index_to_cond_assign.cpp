/*
 * Copyright © 2010 Luca Barbieri
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
 * \file lower_variable_index_to_cond_assign.cpp
 *
 * Turns non-constant indexing into array types to a series of
 * conditional moves of each element into a temporary.
 *
 * Pre-DX10 GPUs often don't have a native way to do this operation,
 * and this works around that.
 */

#include "ir.h"
#include "ir_rvalue_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "main/macros.h"

struct assignment_generator
{
   ir_instruction* base_ir;
   ir_rvalue* array;
   bool is_write;
   ir_variable* var;

   assignment_generator()
   {
   }

   void generate(unsigned i, ir_rvalue* condition, exec_list *list) const
   {
      /* Just clone the rest of the deref chain when trying to get at the
       * underlying variable.
       */
      void *mem_ctx = talloc_parent(base_ir);
      ir_rvalue *element =
	 new(mem_ctx) ir_dereference_array(this->array->clone(mem_ctx, NULL),
					   new(mem_ctx) ir_constant(i));
      ir_rvalue *variable = new(mem_ctx) ir_dereference_variable(this->var);

      ir_assignment *assignment = (is_write)
	 ? new(mem_ctx) ir_assignment(element, variable, condition)
	 : new(mem_ctx) ir_assignment(variable, element, condition);

      list->push_tail(assignment);
   }
};

struct switch_generator
{
   /* make TFunction a template parameter if you need to use other generators */
   typedef assignment_generator TFunction;
   const TFunction& generator;

   ir_variable* index;
   unsigned linear_sequence_max_length;
   unsigned condition_components;

   void *mem_ctx;

   switch_generator(const TFunction& generator, ir_variable *index,
		    unsigned linear_sequence_max_length,
		    unsigned condition_components)
      : generator(generator), index(index),
	linear_sequence_max_length(linear_sequence_max_length),
	condition_components(condition_components)
   {
      this->mem_ctx = talloc_parent(index);
   }

   void linear_sequence(unsigned begin, unsigned end, exec_list *list)
   {
      if (begin == end)
         return;

      /* If the array access is a read, read the first element of this subregion
       * unconditionally.  The remaining tests will possibly overwrite this
       * value with one of the other array elements.
       *
       * This optimization cannot be done for writes because it will cause the
       * first element of the subregion to be written possibly *in addition* to
       * one of the other elements.
       */
      unsigned first;
      if (!this->generator.is_write) {
	 this->generator.generate(begin, 0, list);
	 first = begin + 1;
      } else {
	 first = begin;
      }

      for (unsigned i = first; i < end; i += 4) {
         const unsigned comps = MIN2(condition_components, end - i);

         ir_rvalue *broadcast_index =
	    new(this->mem_ctx) ir_dereference_variable(index);

         if (comps) {
	    const ir_swizzle_mask m = { 0, 0, 0, 0, comps, false };
	    broadcast_index = new(this->mem_ctx) ir_swizzle(broadcast_index, m);
	 }

	 /* Compare the desired index value with the next block of four indices.
	  */
         ir_constant_data test_indices_data;
         memset(&test_indices_data, 0, sizeof(test_indices_data));
         test_indices_data.i[0] = i;
         test_indices_data.i[1] = i + 1;
         test_indices_data.i[2] = i + 2;
         test_indices_data.i[3] = i + 3;
         ir_constant *const test_indices =
	    new(this->mem_ctx) ir_constant(broadcast_index->type,
					   &test_indices_data);

         ir_rvalue *const condition_val =
	    new(this->mem_ctx) ir_expression(ir_binop_equal,
					     &glsl_type::bool_type[comps - 1],
					     broadcast_index,
					     test_indices);

         ir_variable *const condition =
	    new(this->mem_ctx) ir_variable(condition_val->type,
					   "dereference_array_condition",
					   ir_var_temporary, precision_from_ir(condition_val));
         list->push_tail(condition);

	 ir_rvalue *const cond_deref =
	    new(this->mem_ctx) ir_dereference_variable(condition);
         list->push_tail(new(this->mem_ctx) ir_assignment(cond_deref,
							  condition_val, 0));

         if (comps == 1) {
	    ir_rvalue *const cond_deref =
	       new(this->mem_ctx) ir_dereference_variable(condition);

            this->generator.generate(i, cond_deref, list);
         } else {
            for (unsigned j = 0; j < comps; j++) {
	       ir_rvalue *const cond_deref =
		  new(this->mem_ctx) ir_dereference_variable(condition);
	       ir_rvalue *const cond_swiz =
		  new(this->mem_ctx) ir_swizzle(cond_deref, j, 0, 0, 0, 1);

               this->generator.generate(i + j, cond_swiz, list);
            }
         }
      }
   }

   void bisect(unsigned begin, unsigned end, exec_list *list)
   {
      unsigned middle = (begin + end) >> 1;

      assert(index->type->is_integer());

      ir_constant *const middle_c = (index->type->base_type == GLSL_TYPE_UINT)
	 ? new(this->mem_ctx) ir_constant((unsigned)middle)
         : new(this->mem_ctx) ir_constant((int)middle);


      ir_dereference_variable *deref =
	 new(this->mem_ctx) ir_dereference_variable(this->index);

      ir_expression *less =
	 new(this->mem_ctx) ir_expression(ir_binop_less, glsl_type::bool_type,
					  deref, middle_c);

      ir_if *if_less = new(this->mem_ctx) ir_if(less);

      generate(begin, middle, &if_less->then_instructions);
      generate(middle, end, &if_less->else_instructions);

      list->push_tail(if_less);
   }

   void generate(unsigned begin, unsigned end, exec_list *list)
   {
      unsigned length = end - begin;
      if (length <= this->linear_sequence_max_length)
         return linear_sequence(begin, end, list);
      else
         return bisect(begin, end, list);
   }
};

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class variable_index_to_cond_assign_visitor : public ir_rvalue_visitor {
public:
   variable_index_to_cond_assign_visitor(bool lower_input,
					 bool lower_output,
					 bool lower_temp,
					 bool lower_uniform)
   {
      this->progress = false;
      this->lower_inputs = lower_input;
      this->lower_outputs = lower_output;
      this->lower_temps = lower_temp;
      this->lower_uniforms = lower_uniform;
   }

   bool progress;
   bool lower_inputs;
   bool lower_outputs;
   bool lower_temps;
   bool lower_uniforms;

   bool is_array_or_matrix(const ir_instruction *ir) const
   {
      return (ir->type->is_array() || ir->type->is_matrix());
   }

   bool needs_lowering(ir_dereference_array *deref) const
   {
      if (deref == NULL || deref->array_index->as_constant()
	  || !is_array_or_matrix(deref->array))
	 return false;

      if (deref->array->ir_type == ir_type_constant)
	 return this->lower_temps;

      const ir_variable *const var = deref->array->variable_referenced();
      switch (var->mode) {
      case ir_var_auto:
      case ir_var_temporary:
	 return this->lower_temps;
      case ir_var_uniform:
	 return this->lower_uniforms;
      case ir_var_in:
	 return (var->location == -1) ? this->lower_temps : this->lower_inputs;
      case ir_var_out:
	 return (var->location == -1) ? this->lower_temps : this->lower_outputs;
      case ir_var_inout:
	 return this->lower_temps;
      }

      assert(!"Should not get here.");
      return false;
   }

   ir_variable *convert_dereference_array(ir_dereference_array *orig_deref,
					  ir_rvalue* value)
   {
      assert(is_array_or_matrix(orig_deref->array));

      const unsigned length = (orig_deref->array->type->is_array())
         ? orig_deref->array->type->length
         : orig_deref->array->type->matrix_columns;

      void *const mem_ctx = talloc_parent(base_ir);
      ir_variable *var =
	 new(mem_ctx) ir_variable(orig_deref->type, "dereference_array_value",
				  ir_var_temporary, precision_from_ir(orig_deref));
      base_ir->insert_before(var);

      if (value) {
	 ir_dereference *lhs = new(mem_ctx) ir_dereference_variable(var);
	 ir_assignment *assign = new(mem_ctx) ir_assignment(lhs, value, NULL);

         base_ir->insert_before(assign);
      }

      /* Store the index to a temporary to avoid reusing its tree. */
      ir_variable *index =
	 new(mem_ctx) ir_variable(orig_deref->array_index->type,
				  "dereference_array_index", ir_var_temporary, precision_from_ir(orig_deref->array_index));
      base_ir->insert_before(index);

      ir_dereference *lhs = new(mem_ctx) ir_dereference_variable(index);
      ir_assignment *assign =
	 new(mem_ctx) ir_assignment(lhs, orig_deref->array_index, NULL);
      base_ir->insert_before(assign);

      assignment_generator ag;
      ag.array = orig_deref->array;
      ag.base_ir = base_ir;
      ag.var = var;
      ag.is_write = !!value;

      switch_generator sg(ag, index, 4, 4);

      exec_list list;
      sg.generate(0, length, &list);
      base_ir->insert_before(&list);

      return var;
   }

   virtual void handle_rvalue(ir_rvalue **pir)
   {
      if (!*pir)
         return;

      ir_dereference_array* orig_deref = (*pir)->as_dereference_array();
      if (needs_lowering(orig_deref)) {
         ir_variable* var = convert_dereference_array(orig_deref, 0);
         assert(var);
         *pir = new(talloc_parent(base_ir)) ir_dereference_variable(var);
         this->progress = true;
      }
   }

   ir_visitor_status
   visit_leave(ir_assignment *ir)
   {
      ir_rvalue_visitor::visit_leave(ir);

      ir_dereference_array *orig_deref = ir->lhs->as_dereference_array();

      if (needs_lowering(orig_deref)) {
         convert_dereference_array(orig_deref, ir->rhs);
         ir->remove();
         this->progress = true;
      }

      return visit_continue;
   }
};

bool
lower_variable_index_to_cond_assign(exec_list *instructions,
				    bool lower_input,
				    bool lower_output,
				    bool lower_temp,
				    bool lower_uniform)
{
   variable_index_to_cond_assign_visitor v(lower_input,
					   lower_output,
					   lower_temp,
					   lower_uniform);

   visit_list_elements(&v, instructions);

   return v.progress;
}
