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
 * \file ir_validate.cpp
 *
 * Attempts to verify that various invariants of the IR tree are true.
 *
 * In particular, at the moment it makes sure that no single
 * ir_instruction node except for ir_variable appears multiple times
 * in the ir tree.  ir_variable does appear multiple times: Once as a
 * declaration in an exec_list, and multiple times as the endpoint of
 * a dereference chain.
 */

#include <inttypes.h>
#include "ir.h"
#include "ir_hierarchical_visitor.h"
#include "hash_table.h"
#include "glsl_types.h"

class ir_validate : public ir_hierarchical_visitor {
public:
   ir_validate()
   {
      this->ht = hash_table_ctor(0, hash_table_pointer_hash,
				 hash_table_pointer_compare);

      this->current_function = NULL;

      this->callback = ir_validate::validate_ir;
      this->data = ht;
   }

   ~ir_validate()
   {
      hash_table_dtor(this->ht);
   }

   virtual ir_visitor_status visit(ir_variable *v);
   virtual ir_visitor_status visit(ir_dereference_variable *ir);
   virtual ir_visitor_status visit(ir_if *ir);

   virtual ir_visitor_status visit_enter(ir_function *ir);
   virtual ir_visitor_status visit_leave(ir_function *ir);
   virtual ir_visitor_status visit_enter(ir_function_signature *ir);

   static void validate_ir(ir_instruction *ir, void *data);

   ir_function *current_function;

   struct hash_table *ht;
};


ir_visitor_status
ir_validate::visit(ir_dereference_variable *ir)
{
   if ((ir->var == NULL) || (ir->var->as_variable() == NULL)) {
      printf("ir_dereference_variable @ %p does not specify a variable %p\n",
	     ir, ir->var);
      abort();
   }

   if (hash_table_find(ht, ir->var) == NULL) {
      printf("ir_dereference_variable @ %p specifies undeclared variable "
	     "`%s' @ %p\n",
	     ir, ir->var->name, ir->var);
      abort();
   }

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit(ir_if *ir)
{
   if (ir->condition->type != glsl_type::bool_type) {
      printf("ir_if condition %s type instead of bool.\n",
	     ir->condition->type->name);
      ir->print();
      printf("\n");
      abort();
   }

   return visit_continue;
}


ir_visitor_status
ir_validate::visit_enter(ir_function *ir)
{
   /* Function definitions cannot be nested.
    */
   if (this->current_function != NULL) {
      printf("Function definition nested inside another function "
	     "definition:\n");
      printf("%s %p inside %s %p\n",
	     ir->name, ir,
	     this->current_function->name, this->current_function);
      abort();
   }

   /* Store the current function hierarchy being traversed.  This is used
    * by the function signature visitor to ensure that the signatures are
    * linked with the correct functions.
    */
   this->current_function = ir;

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_leave(ir_function *ir)
{
   (void) ir;

   this->current_function = NULL;
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_function_signature *ir)
{
   if (this->current_function != ir->function()) {
      printf("Function signature nested inside wrong function "
	     "definition:\n");
      printf("%p inside %s %p instead of %s %p\n",
	     ir,
	     this->current_function->name, this->current_function,
	     ir->function_name(), ir->function());
      abort();
   }

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit(ir_variable *ir)
{
   /* An ir_variable is the one thing that can (and will) appear multiple times
    * in an IR tree.  It is added to the hashtable so that it can be used
    * in the ir_dereference_variable handler to ensure that a variable is
    * declared before it is dereferenced.
    */
   hash_table_insert(ht, ir, ir);
   return visit_continue;
}

void
ir_validate::validate_ir(ir_instruction *ir, void *data)
{
   struct hash_table *ht = (struct hash_table *) data;

   if (hash_table_find(ht, ir)) {
      printf("Instruction node present twice in ir tree:\n");
      ir->print();
      printf("\n");
      abort();
   }
   hash_table_insert(ht, ir, ir);
}

void
check_node_type(ir_instruction *ir, void *data)
{
   if (ir->ir_type <= ir_type_unset || ir->ir_type >= ir_type_max) {
      printf("Instruction node with unset type\n");
      ir->print(); printf("\n");
   }
   assert(ir->type != glsl_type::error_type);
}

void
validate_ir_tree(exec_list *instructions)
{
   ir_validate v;

   v.run(instructions);

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      visit_tree(ir, check_node_type, NULL);
   }
}
