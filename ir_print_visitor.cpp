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
#include <cstdio>
#include "ir_print_visitor.h"
#include "glsl_types.h"

static void
print_type(const glsl_type *t)
{
   if (t->base_type == GLSL_TYPE_ARRAY) {
      printf("array\n");
      printf("    (");
      print_type(t->fields.array);
      printf(")\n");

      printf("    (%u)\n", t->length);
      printf(")");
   } else if (t->base_type == GLSL_TYPE_STRUCT) {
      printf("struct (%s %u\n", t->name ? t->name : "@", t->length);
      printf("    (FINISHME: structure fields go here)\n");
      printf(")");
   } else {
      printf("%s", t->name);
   }
}


void ir_print_visitor::visit(ir_variable *ir)
{
   if (deref_depth) {
      printf("(%s)", ir->name);
   } else {
      printf("(declare ");

      const char *const cent = (ir->centroid) ? "centroid " : "";
      const char *const inv = (ir->invariant) ? "invariant " : "";
      const char *const mode[] = { "", "uniform ", "in ", "out ", "inout " };
      const char *const interp[] = { "", "flat", "noperspective" };

      printf("(%s%s%s%s) ",
	     cent, inv, mode[ir->mode], interp[ir->interpolation]);

      printf("(");
      print_type(ir->type);
      printf(") ");
      printf("(%s))\n", ir->name);
   }
}


void ir_print_visitor::visit(ir_label *ir)
{
   printf("(label %s)\n", ir->label);
}


void ir_print_visitor::visit(ir_function_signature *ir)
{
   printf("%s:%d:\n", __func__, __LINE__);
   (void) ir;
}


void ir_print_visitor::visit(ir_function *ir)
{
   printf("(function %s\n", ir->name);
   printf(")\n");
}


void ir_print_visitor::visit(ir_expression *ir)
{
   printf("(expression ");

   printf("(FINISHME: operator) ");

   printf("(");
   if (ir->operands[0])
      ir->operands[0]->accept(this);
   printf(") ");

   printf("(");
   if (ir->operands[1])
      ir->operands[1]->accept(this);
   printf(")) ");
}


void ir_print_visitor::visit(ir_dereference *ir)
{
   deref_depth++;

   switch (ir->mode) {
   case ir_dereference::ir_reference_variable: {
      const unsigned swiz[4] = {
	 ir->selector.swizzle.x,
	 ir->selector.swizzle.y,
	 ir->selector.swizzle.z,
	 ir->selector.swizzle.w,
      };

      printf("(var_ref ");
      ir->var->accept(this);
      printf("(");
      for (unsigned i = 0; i < ir->selector.swizzle.num_components; i++) {
	 printf("%c", "xyzw"[swiz[i]]);
      }
      printf("))\n");
      break;
   }
   case ir_dereference::ir_reference_array:
      printf("(array_ref ");
      ir->var->accept(this);
      ir->selector.array_index->accept(this);
      printf(")\n");
      break;
   case ir_dereference::ir_reference_record:
      printf("(record_ref ");
      ir->var->accept(this);
      printf("(%s))\n", ir->selector.field);
      break;
   }

   deref_depth--;
}


void ir_print_visitor::visit(ir_assignment *ir)
{
   printf("(assign\n");

   printf("    (");
   if (ir->condition)
      ir->condition->accept(this);
   else
      printf("true");
   printf(")\n");

   printf("    (");
   ir->lhs->accept(this);
   printf(")\n");

   printf("    (");
   ir->rhs->accept(this);
   printf(")\n");
}


void ir_print_visitor::visit(ir_constant *ir)
{
   (void) ir;

   printf("(constant\n");
   printf("    (");
   print_type(ir->type);
   printf(")\n");
   printf("    (FINISHME: value goes here)\n");
   printf(")\n");
}


void
ir_print_visitor::visit(ir_call *ir)
{
   (void) ir;

   printf("(call FINISHME: function name here\n");
   printf("    (FINISHME: function paramaters here))\n");
}


void
ir_print_visitor::visit(ir_return *ir)
{
   printf("(return");

   ir_expression *const value = ir->get_value();
   if (value) {
      printf(" ");
      value->accept(this);
   }

   printf(")\n");
}
