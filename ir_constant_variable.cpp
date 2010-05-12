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
 * \file ir_constant_variable.cpp
 *
 * Marks variables assigned a single constant value over the course
 * of the program as constant.
 *
 * The goal here is to trigger further constant folding and then dead
 * code elimination.  This is common with vector/matrix constructors
 * and calls to builtin functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ir.h"
#include "ir_print_visitor.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "linux_list.h"

struct assignment_entry {
   struct list link;
   int assignment_count;
   ir_variable *var;
   ir_constant *constval;
};

class ir_constant_variable_visitor : public ir_hierarchical_visitor {
public:
   ir_constant_variable_visitor()
   {
      list_init(&list);
   }

   virtual ir_visitor_status visit_enter(ir_assignment *);

   struct list list;
};

static struct assignment_entry *
get_assignment_entry(ir_variable *var, struct list *list)
{
   struct assignment_entry *entry;

   list_foreach_entry(entry, struct assignment_entry, list, link) {
      if (entry->var == var)
	 return entry;
   }

   entry = (struct assignment_entry *)calloc(1, sizeof(*entry));
   entry->var = var;
   list_add(&entry->link, list);
   return entry;
}

ir_visitor_status
ir_constant_variable_visitor::visit_enter(ir_assignment *ir)
{
   ir_constant *constval;
   struct assignment_entry *entry;

   entry = get_assignment_entry(ir->lhs->variable_referenced(), &this->list);
   assert(entry);
   entry->assignment_count++;

   /* If it's already constant, don't do the work. */
   if (entry->var->constant_value)
      return visit_continue;

   /* OK, now find if we actually have all the right conditions for
    * this to be a constant value assigned to the var.
    */
   if (ir->condition) {
      constval = ir->condition->constant_expression_value();
      if (!constval || !constval->value.b[0])
	 return visit_continue;
   }

   ir_variable *var = ir->lhs->whole_variable_referenced();
   if (!var)
      return visit_continue;

   constval = ir->rhs->constant_expression_value();
   if (!constval)
      return visit_continue;

   /* Mark this entry as having a constant assignment (if the
    * assignment count doesn't go >1).  do_constant_variable will fix
    * up the variable with the constant value later.
    */
   entry->constval = constval;

   return visit_continue;
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_constant_variable(exec_list *instructions)
{
   bool progress = false;
   ir_constant_variable_visitor v;

   v.run(instructions);

   while (!list_is_empty(&v.list)) {

      struct assignment_entry *entry;
      entry = list_entry(v.list.next, struct assignment_entry, link);

      if (entry->assignment_count == 1 && entry->constval) {
	 entry->var->constant_value = entry->constval;
	 progress = true;
      }
      list_del(&entry->link);
      free(entry);
   }

   return progress;
}

bool
do_constant_variable_unlinked(exec_list *instructions)
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_function *f = ir->as_function();
      if (f) {
	 foreach_iter(exec_list_iterator, sigiter, *f) {
	    ir_function_signature *sig =
	       (ir_function_signature *) sigiter.get();
	    if (do_constant_variable(&sig->body))
	       progress = true;
	 }
      }
   }

   return progress;
}
