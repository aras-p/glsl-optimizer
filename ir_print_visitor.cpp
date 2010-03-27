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
      printf("array (");
      print_type(t->fields.array);
      printf(") (%u))", t->length);
   } else if (t->base_type == GLSL_TYPE_STRUCT) {
      printf("struct (%s %u ", t->name ? t->name : "@", t->length);
      printf("(FINISHME: structure fields go here) ");
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
      printf("(%s)) ", ir->name);
   }
}


void ir_print_visitor::visit(ir_label *ir)
{
   printf("\n(label %s)", ir->label);
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
   static const char *const operators[] = {
      "~",
      "!",
      "-",
      "abs",
      "rcp",
      "rsq",
      "exp",
      "log",
      "f2i",
      "i2f",
      "u2f",
      "trunc",
      "ceil",
      "floor",
      "+",
      "-",
      "*",
      "/",
      "%",
      "<",
      ">",
      "<=",
      ">=",
      "==",
      "!=",
      "<<",
      ">>",
      "&",
      "^",
      "|",
      "&&",
      "^^",
      "||",
      "!",
      "dot",
      "min",
      "max",
      "pow",
   };

   printf("(expression ");

   assert((unsigned int)ir->operation <
	  sizeof(operators) / sizeof(operators[0]));

   printf("%s", operators[ir->operation]);
   printf("(");
   if (ir->operands[0])
      ir->operands[0]->accept(this);
   printf(") ");

   printf("(");
   if (ir->operands[1])
      ir->operands[1]->accept(this);
   printf(")) ");
}


void ir_print_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };

   printf("(swiz ");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
      printf("%c", "xyzw"[swiz[i]]);
   }
   printf(" ");
   ir->val->accept(this);
   printf(")");
}


void ir_print_visitor::visit(ir_dereference *ir)
{
   deref_depth++;

   switch (ir->mode) {
   case ir_dereference::ir_reference_variable: {
      printf("(var_ref ");
      ir->var->accept(this);
      printf(") ");
      break;
   }
   case ir_dereference::ir_reference_array:
      printf("(array_ref ");
      ir->var->accept(this);
      ir->selector.array_index->accept(this);
      printf(") ");
      break;
   case ir_dereference::ir_reference_record:
      printf("(record_ref ");
      ir->var->accept(this);
      printf("(%s)) ", ir->selector.field);
      break;
   }

   deref_depth--;
}


void ir_print_visitor::visit(ir_assignment *ir)
{
   printf("(assign (");

   if (ir->condition)
      ir->condition->accept(this);
   else
      printf("true");

   printf(") (");

   ir->lhs->accept(this);

   printf(") (");

   ir->rhs->accept(this);
   printf(") ");
}


void ir_print_visitor::visit(ir_constant *ir)
{
   const glsl_type *const base_type = ir->type->get_base_type();

   printf("(constant (");
   print_type(base_type);
   printf(") ");

   printf("(%d) (", ir->type->components());
   for (unsigned i = 0; i < ir->type->components(); i++) {
      if (i != 0)
	 printf(", ");

      switch (base_type->base_type) {
      case GLSL_TYPE_UINT:  printf("%u", ir->value.u[i]); break;
      case GLSL_TYPE_INT:   printf("%d", ir->value.i[i]); break;
      case GLSL_TYPE_FLOAT: printf("%f", ir->value.f[i]); break;
      case GLSL_TYPE_BOOL:  printf("%d", ir->value.b[i]); break;
      default: assert(0);
      }
   }
   printf(")) ");
}


void
ir_print_visitor::visit(ir_call *ir)
{
   printf("(call (%s) ", ir->callee_name());
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      inst->accept(this);
   }
}


void
ir_print_visitor::visit(ir_return *ir)
{
   printf("(return");

   ir_rvalue *const value = ir->get_value();
   if (value) {
      printf(" ");
      value->accept(this);
   }

   printf(")");
}
