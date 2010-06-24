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
 * \file ir_function_inlining.cpp
 *
 * Replaces calls to functions with the body of the function.
 */

#include <inttypes.h>
#include "ir.h"
#include "ir_visitor.h"
#include "ir_function_inlining.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"
#include "hash_table.h"

class ir_function_inlining_visitor : public ir_hierarchical_visitor {
public:
   ir_function_inlining_visitor()
   {
      progress = false;
   }

   virtual ~ir_function_inlining_visitor()
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_enter(ir_assignment *);
   virtual ir_visitor_status visit_enter(ir_return *);
   virtual ir_visitor_status visit_enter(ir_texture *);
   virtual ir_visitor_status visit_enter(ir_swizzle *);

   bool progress;
};


unsigned int hash_func(const void *key)
{
   return (unsigned int)(uintptr_t)key;
}

int hash_compare_func(const void *key1, const void *key2)
{
   return key1 == key2 ? 0 : 1;
}

bool
automatic_inlining_predicate(ir_instruction *ir)
{
   ir_call *call = ir->as_call();

   if (call && can_inline(call))
      return true;

   return false;
}

bool
do_function_inlining(exec_list *instructions)
{
   ir_function_inlining_visitor v;

   do_expression_flattening(instructions, automatic_inlining_predicate);

   v.run(instructions);

   return v.progress;
}

static void
replace_return_with_assignment(ir_instruction *ir, void *data)
{
   void *ctx = talloc_parent(ir);
   ir_variable *retval = (ir_variable *)data;
   ir_return *ret = ir->as_return();

   if (ret) {
      if (ret->value) {
	 ir_rvalue *lhs = new(ctx) ir_dereference_variable(retval);
	 ret->insert_before(new(ctx) ir_assignment(lhs, ret->value, NULL));
	 ret->remove();
      } else {
	 /* un-valued return has to be the last return, or we shouldn't
	  * have reached here. (see can_inline()).
	  */
	 assert(!ret->next->is_tail_sentinal());
      }
   }
}

ir_rvalue *
ir_call::generate_inline(ir_instruction *next_ir)
{
   void *ctx = talloc_parent(this);
   ir_variable **parameters;
   int num_parameters;
   int i;
   ir_variable *retval = NULL;
   struct hash_table *ht;

   ht = hash_table_ctor(0, hash_func, hash_compare_func);

   num_parameters = 0;
   foreach_iter(exec_list_iterator, iter_sig, this->callee->parameters)
      num_parameters++;

   parameters = new ir_variable *[num_parameters];

   /* Generate storage for the return value. */
   if (this->callee->return_type) {
      retval = new(ctx) ir_variable(this->callee->return_type, "__retval");
      next_ir->insert_before(retval);
   }

   /* Generate the declarations for the parameters to our inlined code,
    * and set up the mapping of real function body variables to ours.
    */
   i = 0;
   exec_list_iterator sig_param_iter = this->callee->parameters.iterator();
   exec_list_iterator param_iter = this->actual_parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      const ir_variable *const sig_param = (ir_variable *) sig_param_iter.get();
      ir_rvalue *param = (ir_rvalue *) param_iter.get();

      /* Generate a new variable for the parameter. */
      parameters[i] = (ir_variable *)sig_param->clone(ht);
      parameters[i]->mode = ir_var_auto;
      next_ir->insert_before(parameters[i]);

      /* Move the actual param into our param variable if it's an 'in' type. */
      if (sig_param->mode == ir_var_in ||
	  sig_param->mode == ir_var_inout) {
	 ir_assignment *assign;

	 assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(parameters[i]),
					 param, NULL);
	 next_ir->insert_before(assign);
      }

      sig_param_iter.next();
      param_iter.next();
   }

   /* Generate the inlined body of the function. */
   foreach_iter(exec_list_iterator, iter, callee->body) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_instruction *new_ir = ir->clone(ht);

      next_ir->insert_before(new_ir);
      visit_tree(new_ir, replace_return_with_assignment, retval);
   }

   /* Copy back the value of any 'out' parameters from the function body
    * variables to our own.
    */
   i = 0;
   param_iter = this->actual_parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      ir_instruction *const param = (ir_instruction *) param_iter.get();

      /* Move our param variable into the actual param if it's an 'out' type. */
      if (parameters[i]->mode == ir_var_out ||
	  parameters[i]->mode == ir_var_inout) {
	 ir_assignment *assign;

	 assign = new(ctx) ir_assignment(param->as_rvalue(),
					 new(ctx) ir_dereference_variable(parameters[i]),
					 NULL);
	 next_ir->insert_before(assign);
      }

      param_iter.next();
   }

   delete [] parameters;

   hash_table_dtor(ht);

   if (retval)
      return new(ctx) ir_dereference_variable(retval);
   else
      return NULL;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_expression *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_return *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_texture *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_swizzle *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_call *ir)
{
   if (can_inline(ir)) {
      (void) ir->generate_inline(ir);
      ir->remove();
      this->progress = true;
   }

   return visit_continue;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_assignment *ir)
{
   ir_call *call = ir->rhs->as_call();
   if (!call || !can_inline(call))
      return visit_continue;

   /* generates the parameter setup, function body, and returns the return
    * value of the function
    */
   ir_rvalue *rhs = call->generate_inline(ir);
   assert(rhs);

   ir->rhs = rhs;
   this->progress = true;

   return visit_continue;
}
