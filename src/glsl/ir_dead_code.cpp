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
 * \file ir_dead_code.cpp
 *
 * Eliminates dead assignments and variable declarations from the code.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class variable_entry : public exec_node
{
public:
   variable_entry(ir_variable *var)
   {
      this->var = var;
      assign = NULL;
      referenced_count = 0;
      assigned_count = 0;
      declaration = false;
   }

   ir_variable *var; /* The key: the variable's pointer. */
   ir_assignment *assign; /* An assignment to the variable, if any */

   /** Number of times the variable is referenced, including assignments. */
   unsigned referenced_count;

   /** Number of times the variable is assignmened. */
   unsigned assigned_count;

   bool declaration; /* If the variable had a decl in the instruction stream */
};

class ir_dead_code_visitor : public ir_hierarchical_visitor {
public:
   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit(ir_dereference_variable *);

   virtual ir_visitor_status visit_enter(ir_function *);
   virtual ir_visitor_status visit_leave(ir_assignment *);

   variable_entry *get_variable_entry(ir_variable *var);

   bool (*predicate)(ir_instruction *ir);

   /* List of variable_entry */
   exec_list variable_list;

   void *mem_ctx;
};


variable_entry *
ir_dead_code_visitor::get_variable_entry(ir_variable *var)
{
   assert(var);
   foreach_iter(exec_list_iterator, iter, this->variable_list) {
      variable_entry *entry = (variable_entry *)iter.get();
      if (entry->var == var)
	 return entry;
   }

   variable_entry *entry = new(mem_ctx) variable_entry(var);
   this->variable_list.push_tail(entry);
   return entry;
}


ir_visitor_status
ir_dead_code_visitor::visit(ir_variable *ir)
{
   variable_entry *entry = this->get_variable_entry(ir);
   if (entry)
      entry->declaration = true;

   return visit_continue;
}


ir_visitor_status
ir_dead_code_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *const var = ir->variable_referenced();
   variable_entry *entry = this->get_variable_entry(var);

   if (entry)
      entry->referenced_count++;

   return visit_continue;
}


ir_visitor_status
ir_dead_code_visitor::visit_enter(ir_function *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_dead_code_visitor::visit_leave(ir_assignment *ir)
{
   variable_entry *entry;
   entry = this->get_variable_entry(ir->lhs->variable_referenced());
   if (entry) {
      entry->assigned_count++;
      if (entry->assign == NULL)
	 entry->assign = ir;
   }

   return visit_continue;
}


/**
 * Do a dead code pass over instructions and everything that instructions
 * references.
 *
 * Note that this will remove assignments to globals, so it is not suitable
 * for usage on an unlinked instruction stream.
 */
bool
do_dead_code(struct _mesa_glsl_parse_state *state,
	     exec_list *instructions)
{
   ir_dead_code_visitor v;
   bool progress = false;

   v.mem_ctx = state;
   v.run(instructions);

   foreach_iter(exec_list_iterator, iter, v.variable_list) {
      variable_entry *entry = (variable_entry *)iter.get();

      /* Since each assignment is a reference, the refereneced count must be
       * greater than or equal to the assignment count.  If they are equal,
       * then all of the references are assignments, and the variable is
       * dead.
       *
       * Note that if the variable is neither assigned nor referenced, both
       * counts will be zero and will be caught by the equality test.
       */
      assert(entry->referenced_count >= entry->assigned_count);

      if ((entry->referenced_count > entry->assigned_count)
	  || !entry->declaration)
	 continue;

      if (entry->assign) {
	 /* Remove a single dead assignment to the variable we found.
	  * Don't do so if it's a shader output, though.
	  */
	 if (!entry->var->shader_out) {
	    entry->assign->remove();
	    progress = true;
	 }
      } else {
	 /* If there are no assignments or references to the variable left,
	  * then we can remove its declaration.
	  */
	 entry->var->remove();
	 progress = true;
      }
   }
   return progress;
}

/**
 * Does a dead code pass on the functions present in the instruction stream.
 *
 * This is suitable for use while the program is not linked, as it will
 * ignore variable declarations (and the assignments to them) for variables
 * with global scope.
 */
bool
do_dead_code_unlinked(struct _mesa_glsl_parse_state *state,
		      exec_list *instructions)
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_function *f = ir->as_function();
      if (f) {
	 foreach_iter(exec_list_iterator, sigiter, *f) {
	    ir_function_signature *sig =
	       (ir_function_signature *) sigiter.get();
	    if (do_dead_code(state, &sig->body))
	       progress = true;
	 }
      }
   }

   return progress;
}
