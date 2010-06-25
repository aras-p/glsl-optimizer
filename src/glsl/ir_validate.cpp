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
extern "C" {
#include "hash_table.h"
}

static unsigned int hash_func(const void *key)
{
   return (unsigned int)(uintptr_t)key;
}

static int hash_compare_func(const void *key1, const void *key2)
{
   return key1 == key2 ? 0 : 1;
}


class ir_validate : public ir_hierarchical_visitor {
public:
   ir_validate()
   {
      this->ht = hash_table_ctor(0, hash_func, hash_compare_func);

      this->callback = ir_validate::validate_ir;
      this->data = ht;
   }

   ~ir_validate()
   {
      hash_table_dtor(this->ht);
   }

   virtual ir_visitor_status visit(ir_variable *v);

   static void validate_ir(ir_instruction *ir, void *data);

   struct hash_table *ht;
};

ir_visitor_status
ir_validate::visit(ir_variable *ir)
{
   /* An ir_variable is the one thing that can (and will) appear multiple times
    * in an IR tree.
    */
   (void) ir;
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
validate_ir_tree(exec_list *instructions)
{
   ir_validate v;

   v.run(instructions);
}
