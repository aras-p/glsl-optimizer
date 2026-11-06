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
 * \file opt_function_inlining.cpp
 *
 * Replaces calls to functions with the body of the function.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_expression_flattening.h"
#include "compiler/glsl_types.h"
#include "util/hash_table.h"

static void
do_variable_replacement(ir_exec_list *instructions,
                        ir_variable *orig,
                        ir_rvalue *repl);

namespace {

class ir_save_lvalue_visitor : public ir_hierarchical_visitor {
public:
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
};

} /* unnamed namespace */

static void
replace_return_with_assignment(ir_instruction *ir, void *data)
{
   ir_dereference *orig_deref = (ir_dereference *) data;
   ir_return *ret = ir->as_return();

   if (ret) {
      if (ret->value) {
	 ir_rvalue *lhs = orig_deref->clone(ir->node_linalloc, NULL);
         ret->replace_with(new(ir->node_linalloc) ir_assignment(lhs, ret->value));
      } else {
	 /* un-valued return has to be the last return, or we shouldn't
	  * have reached here. (see can_inline()).
	  */
	 assert(ret->next->is_tail_sentinel());
	 ret->remove();
      }
   }
}

/* Save the given lvalue before the given instruction.
 *
 * This is done by adding temporary variables into which the current value
 * of any array indices are saved, and then modifying the dereference chain
 * in-place to point to those temporary variables.
 *
 * The hierarchical visitor is only used to traverse the left-hand-side chain
 * of derefs.
 */
ir_visitor_status
ir_save_lvalue_visitor::visit_enter(ir_dereference_array *deref)
{
   if (deref->array_index->ir_type != ir_type_constant) {
      ir_variable *index;
      ir_assignment *assignment;

      index = new(deref->node_linalloc) ir_variable(deref->array_index->type, "saved_idx", ir_var_temporary);
      base_ir->insert_before(index);

      assignment = new(deref->node_linalloc) ir_assignment(new(deref->node_linalloc) ir_dereference_variable(index),
                                          deref->array_index);
      base_ir->insert_before(assignment);

      deref->array_index = new(deref->node_linalloc) ir_dereference_variable(index);
   }

   deref->array->accept(this);
   return visit_stop;
}

static bool
should_replace_variable(ir_variable *sig_param, ir_rvalue *param,
                        bool is_builtin) {

   if (sig_param->data.mode != ir_var_function_in &&
       sig_param->data.mode != ir_var_const_in)
      return false;

   /* Some places in glsl_to_nir() expect images to always be copied to a temp
    * first.
    */
   if (glsl_type_is_image(glsl_without_array(sig_param->type)) && !param->is_dereference())
      return false;

   /* SSBO and shared vars might be passed to a built-in such as an atomic
    * memory function, where copying these to a temp before passing to the
    * atomic function is not valid so we must replace these instead. Also,
    * shader inputs for interpolateAt funtions also need to be replaced.
    *
    * Our builtins should always use temps and not the inputs themselves to
    * store temporay values so just checking is_builtin rather than string
    * comparing the function name for e.g atomic* should always be safe.
    */
   if (is_builtin)
      return true;

   /* For opaque types, we want the inlined variable references
    * referencing the passed in variable, since that will have
    * the location information, which an assignment of an opaque
    * variable wouldn't.
    */
   return glsl_contains_opaque(sig_param->type);
}

void
ir_call::generate_inline(ir_instruction *next_ir)
{
   linear_ctx *linalloc = this->node_linalloc;
   ir_variable **parameters;
   unsigned num_parameters;
   int i;
   struct hash_table ht;

   _mesa_pointer_hash_table_init(&ht, NULL);

   num_parameters = this->callee->parameters.length();
   parameters = new ir_variable *[num_parameters];

   /* Generate the declarations for the parameters to our inlined code,
    * and set up the mapping of real function body variables to ours.
    */
   i = 0;
   ir_foreach_two_lists(formal_node, &this->callee->parameters,
                     actual_node, &this->actual_parameters) {
      ir_variable *sig_param = (ir_variable *) formal_node;
      ir_rvalue *param = (ir_rvalue *) actual_node;

      /* Generate a new variable for the parameter. */
      if (should_replace_variable(sig_param, param,
                                  this->callee->is_builtin())) {
         /* Actual replacement happens below */
	 parameters[i] = NULL;
      } else {
	 parameters[i] = sig_param->clone(linalloc, &ht);
	 parameters[i]->data.mode = ir_var_temporary;

	 /* Remove the read-only decoration because we're going to write
	  * directly to this variable.  If the cloned variable is left
	  * read-only and the inlined function is inside a loop, the loop
	  * analysis code will get confused.
	  */
	 parameters[i]->data.read_only = false;
	 next_ir->insert_before(parameters[i]);
      }

      /* Section 6.1.1 (Function Calling Conventions) of the OpenGL Shading
       * Language 4.5 spec says:
       *
       *    "All arguments are evaluated at call time, exactly once, in order,
       *     from left to right. [...] Evaluation of an out parameter results
       *     in an l-value that is used to copy out a value when the function
       *     returns."
       *
       * I.e., we have to take temporary copies of any relevant array indices
       * before the function body is executed.
       *
       * This ensures that
       * (a) if an array index expressions refers to a variable that is
       *     modified by the execution of the function body, we use the
       *     original value as intended, and
       * (b) if an array index expression has side effects, those side effects
       *     are only executed once and at the right time.
       */
      if (parameters[i]) {
         if (sig_param->data.mode == ir_var_function_in ||
             sig_param->data.mode == ir_var_const_in) {
            ir_assignment *assign;

            assign = new(linalloc) ir_assignment(new(linalloc) ir_dereference_variable(parameters[i]),
                                            param);
            next_ir->insert_before(assign);
         } else {
            assert(sig_param->data.mode == ir_var_function_out ||
                   sig_param->data.mode == ir_var_function_inout);
            assert(param->is_lvalue());

            ir_save_lvalue_visitor v;
            v.base_ir = next_ir;

            param->accept(&v);

            if (sig_param->data.mode == ir_var_function_inout) {
               ir_assignment *assign;

               assign = new(linalloc) ir_assignment(new(linalloc) ir_dereference_variable(parameters[i]),
                                               param->clone(linalloc, NULL)->as_rvalue());
               next_ir->insert_before(assign);
            }
         }
      }

      ++i;
   }

   ir_exec_list new_instructions;

   /* Generate the inlined body of the function to a new list */
   ir_foreach_in_list(ir_instruction, ir, &callee->body) {
      ir_instruction *new_ir = ir->clone(linalloc, &ht);

      new_instructions.push_tail(new_ir);
      visit_tree(new_ir, replace_return_with_assignment, this->return_deref);
   }

   /* If any opaque types were passed in, replace any deref of the
    * opaque variable with a deref of the argument.
    */
   ir_foreach_two_lists(formal_node, &this->callee->parameters,
                     actual_node, &this->actual_parameters) {
      ir_rvalue *const param = (ir_rvalue *) actual_node;
      ir_variable *sig_param = (ir_variable *) formal_node;

      if (should_replace_variable(sig_param, param,
                                  this->callee->is_builtin())) {
         do_variable_replacement(&new_instructions, sig_param, param);
      }
   }

   /* Now push those new instructions in. */
   next_ir->insert_before(&new_instructions);

   /* Copy back the value of any 'out' parameters from the function body
    * variables to our own.
    */
   i = 0;
   ir_foreach_two_lists(formal_node, &this->callee->parameters,
                     actual_node, &this->actual_parameters) {
      ir_rvalue *const param = (ir_rvalue *) actual_node;
      const ir_variable *const sig_param = (ir_variable *) formal_node;

      /* Move our param variable into the actual param if it's an 'out' type. */
      if (parameters[i] && (sig_param->data.mode == ir_var_function_out ||
			    sig_param->data.mode == ir_var_function_inout)) {
	 ir_assignment *assign;

         assign = new(linalloc) ir_assignment(param,
                                         new(linalloc) ir_dereference_variable(parameters[i]));
	 next_ir->insert_before(assign);
      }

      ++i;
   }

   delete [] parameters;

   _mesa_hash_table_fini(&ht, NULL);
}

/**
 * Replaces references to the "orig" variable with a clone of "repl."
 *
 * From the spec, opaque types can appear in the tree as function
 * (non-out) parameters and as the result of array indexing and
 * structure field selection.  In our builtin implementation, they
 * also appear in the sampler field of an ir_tex instruction.
 */

class ir_variable_replacement_visitor : public ir_rvalue_visitor {
public:
   ir_variable_replacement_visitor(ir_variable *orig, ir_rvalue *repl)
   {
      this->orig = orig;
      this->repl = repl;
   }

   virtual ~ir_variable_replacement_visitor()
   {
   }

   virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_leave(ir_texture *);
   virtual ir_visitor_status visit_leave(ir_assignment *);

   void handle_rvalue(ir_rvalue **rvalue);
   void replace_deref(ir_dereference **deref);
   void replace_rvalue(ir_rvalue **rvalue);

   ir_variable *orig;
   ir_rvalue *repl;
};

void
ir_variable_replacement_visitor::replace_deref(ir_dereference **deref)
{
   ir_dereference_variable *deref_var = (*deref)->as_dereference_variable();
   if (deref_var && deref_var->var == this->orig)
      *deref = this->repl->as_dereference()->clone((*deref)->node_linalloc, NULL);
}

void
ir_variable_replacement_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   replace_rvalue(rvalue);
}

void
ir_variable_replacement_visitor::replace_rvalue(ir_rvalue **rvalue)
{
   if (!*rvalue)
      return;

   ir_dereference *deref = (*rvalue)->as_dereference();

   if (!deref)
      return;

   ir_dereference_variable *deref_var = (deref)->as_dereference_variable();
   if (deref_var && deref_var->var == this->orig)
      *rvalue = this->repl->clone(deref->node_linalloc, NULL);
}

ir_visitor_status
ir_variable_replacement_visitor::visit_leave(ir_texture *ir)
{
   replace_deref(&ir->sampler);

   return rvalue_visit(ir);
}

ir_visitor_status
ir_variable_replacement_visitor::visit_leave(ir_assignment *ir)
{
   replace_deref(&ir->lhs);
   replace_rvalue(&ir->rhs);

   return visit_continue;
}

ir_visitor_status
ir_variable_replacement_visitor::visit_leave(ir_call *ir)
{
   ir_foreach_in_list_safe(ir_rvalue, param, &ir->actual_parameters) {
      ir_rvalue *new_param = param;
      replace_rvalue(&new_param);

      if (new_param != param) {
         param->replace_with(new_param);
      }
   }
   return visit_continue;
}

static void
do_variable_replacement(ir_exec_list *instructions,
                        ir_variable *orig,
                        ir_rvalue *repl)
{
   ir_variable_replacement_visitor v(orig, repl);

   visit_list_elements(&v, instructions);
}
