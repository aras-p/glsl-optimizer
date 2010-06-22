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
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "hash_table.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */


class ir_validate : public ir_hierarchical_visitor {
public:
   virtual ir_visitor_status visit_enter(class ir_constant *);
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_enter(class ir_expression *);
   virtual ir_visitor_status visit_enter(class ir_texture *);
   virtual ir_visitor_status visit_enter(class ir_swizzle *);
   virtual ir_visitor_status visit_enter(class ir_dereference_array *);
   virtual ir_visitor_status visit_enter(class ir_dereference_record *);
   virtual ir_visitor_status visit_enter(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_return *);
   virtual ir_visitor_status visit_enter(class ir_if *);

   void validate_ir(ir_instruction *ir);

   struct hash_table *ht;
};

unsigned int hash_func(const void *key)
{
   return (unsigned int)(uintptr_t)key;
}

int hash_compare_func(const void *key1, const void *key2)
{
   return key1 == key2 ? 0 : 1;
}

void
ir_validate::validate_ir(ir_instruction *ir)
{
   if (hash_table_find(this->ht, ir)) {
      printf("Instruction node present twice in ir tree:\n");
      ir->print();
      printf("\n");
      abort();
   }
   hash_table_insert(this->ht, ir, ir);
}

ir_visitor_status
ir_validate::visit_enter(ir_constant *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_loop *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_function_signature *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_function *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_expression *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_texture *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_swizzle *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_dereference_array *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_dereference_record *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_assignment *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_call *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_return *ir)
{
   validate_ir(ir);
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_if *ir)
{
   validate_ir(ir);
   return visit_continue;
}

void
validate_ir_tree(exec_list *instructions)
{
   ir_validate v;

   v.ht = hash_table_ctor(0, hash_func, hash_compare_func);

   v.run(instructions);

   hash_table_dtor(v.ht);
}
