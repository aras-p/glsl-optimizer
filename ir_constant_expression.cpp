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
 * \file ir_constant_expression.cpp
 * Evaluate and process constant valued expressions
 *
 * In GLSL, constant valued expressions are used in several places.  These
 * must be processed and evaluated very early in the compilation process.
 *
 *    * Sizes of arrays
 *    * Initializers for uniforms
 *    * Initializers for \c const variables
 */

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

/**
 * Visitor class for evaluating constant expressions
 */
class ir_constant_visitor : public ir_visitor {
public:
   ir_constant_visitor()
      : value(NULL)
   {
      /* empty */
   }

   virtual ~ir_constant_visitor()
   {
      /* empty */
   }

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_label *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_if *);
   /*@}*/

   /**
    * Value of the constant expression.
    *
    * \note
    * This field will be \c NULL if the expression is not constant valued.
    */
   /* FINIHSME: This cannot hold values for constant arrays or structures. */
   ir_constant *value;
};


ir_constant *
ir_instruction::constant_expression_value()
{
   ir_constant_visitor visitor;

   this->accept(& visitor);
   return visitor.value;
}


void
ir_constant_visitor::visit(ir_variable *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_label *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_function_signature *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_function *ir)
{
   (void) ir;
   value = NULL;
}

void
ir_constant_visitor::visit(ir_expression *ir)
{
   value = NULL;
   ir_constant *op[2];
   unsigned int operand, c;
   unsigned u[16];
   int i[16];
   float f[16];
   bool b[16];
   const glsl_type *type = NULL;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      op[operand] = ir->operands[operand]->constant_expression_value();
      if (!op[operand])
	 return;
   }

   switch (ir->operation) {
   case ir_unop_logic_not:
      type = ir->operands[0]->type;
      assert(type->base_type == GLSL_TYPE_BOOL);
      for (c = 0; c < ir->operands[0]->type->components(); c++)
	 b[c] = !op[0]->value.b[c];
      break;

   case ir_unop_f2i:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      type = ir->type;
      for (c = 0; c < ir->operands[0]->type->components(); c++) {
	 i[c] = op[0]->value.f[c];
      }
      break;
   case ir_unop_i2f:
      assert(op[0]->type->base_type == GLSL_TYPE_UINT ||
	     op[0]->type->base_type == GLSL_TYPE_INT);
      type = ir->type;
      for (c = 0; c < ir->operands[0]->type->components(); c++) {
	 if (op[0]->type->base_type == GLSL_TYPE_INT)
	    f[c] = op[0]->value.i[c];
	 else
	    f[c] = op[0]->value.u[c];
      }
      break;

   case ir_binop_add:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 type = ir->operands[0]->type;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       u[c] = op[0]->value.u[c] + op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       i[c] = op[0]->value.i[c] + op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       f[c] = op[0]->value.f[c] + op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;
   case ir_binop_sub:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 type = ir->operands[0]->type;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       u[c] = op[0]->value.u[c] - op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       i[c] = op[0]->value.i[c] - op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       f[c] = op[0]->value.f[c] - op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;
   case ir_binop_mul:
      if (ir->operands[0]->type == ir->operands[1]->type &&
	  !ir->operands[0]->type->is_matrix()) {
	 type = ir->operands[0]->type;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       u[c] = op[0]->value.u[c] * op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       i[c] = op[0]->value.i[c] * op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       f[c] = op[0]->value.f[c] * op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;
   case ir_binop_div:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 type = ir->operands[0]->type;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       u[c] = op[0]->value.u[c] / op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       i[c] = op[0]->value.i[c] / op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       f[c] = op[0]->value.f[c] / op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;
   case ir_binop_logic_and:
      type = ir->operands[0]->type;
      assert(type->base_type == GLSL_TYPE_BOOL);
      for (c = 0; c < ir->operands[0]->type->components(); c++)
	 b[c] = op[0]->value.b[c] && op[1]->value.b[c];
      break;
   case ir_binop_logic_xor:
      type = ir->operands[0]->type;
      assert(type->base_type == GLSL_TYPE_BOOL);
      for (c = 0; c < ir->operands[0]->type->components(); c++)
	 b[c] = op[0]->value.b[c] ^ op[1]->value.b[c];
      break;
   case ir_binop_logic_or:
      type = ir->operands[0]->type;
      assert(type->base_type == GLSL_TYPE_BOOL);
      for (c = 0; c < ir->operands[0]->type->components(); c++)
	 b[c] = op[0]->value.b[c] || op[1]->value.b[c];
      break;

   case ir_binop_less:
      type = glsl_type::bool_type;
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 b[0] = op[0]->value.u[0] < op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 b[0] = op[0]->value.i[0] < op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 b[0] = op[0]->value.f[0] < op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_greater:
      type = glsl_type::bool_type;
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 b[0] = op[0]->value.u[0] > op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 b[0] = op[0]->value.i[0] > op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 b[0] = op[0]->value.f[0] > op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_lequal:
      type = glsl_type::bool_type;
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 b[0] = op[0]->value.u[0] <= op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 b[0] = op[0]->value.i[0] <= op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 b[0] = op[0]->value.f[0] <= op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_gequal:
      type = glsl_type::bool_type;
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 b[0] = op[0]->value.u[0] >= op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 b[0] = op[0]->value.i[0] >= op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 b[0] = op[0]->value.f[0] >= op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;

   case ir_binop_equal:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 type = glsl_type::bool_type;
	 b[0] = true;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       b[0] = b[0] && op[0]->value.u[c] == op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       b[0] = b[0] && op[0]->value.i[c] == op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       b[0] = b[0] && op[0]->value.f[c] == op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;
   case ir_binop_nequal:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 type = glsl_type::bool_type;
	 b[0] = false;
	 for (c = 0; c < ir->operands[0]->type->components(); c++) {
	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       b[0] = b[0] || op[0]->value.u[c] != op[1]->value.u[c];
	       break;
	    case GLSL_TYPE_INT:
	       b[0] = b[0] || op[0]->value.i[c] != op[1]->value.i[c];
	       break;
	    case GLSL_TYPE_FLOAT:
	       b[0] = b[0] || op[0]->value.f[c] != op[1]->value.f[c];
	       break;
	    default:
	       assert(0);
	    }
	 }
      }
      break;

   default:
      break;
   }

   if (type) {
      switch (type->base_type) {
      case GLSL_TYPE_UINT:
	 value = new ir_constant(type, u);
	 break;
      case GLSL_TYPE_INT:
	 value = new ir_constant(type, i);
	 break;
      case GLSL_TYPE_FLOAT:
	 value = new ir_constant(type, f);
	 break;
      case GLSL_TYPE_BOOL:
	 value = new ir_constant(type, b);
	 break;
      }
   }
}


void
ir_constant_visitor::visit(ir_swizzle *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_dereference *ir)
{
   value = NULL;

   if (ir->mode == ir_dereference::ir_reference_variable) {
      ir_variable *var = ir->var->as_variable();
      if (var && var->constant_value) {
	 value = new ir_constant(ir->type, &var->constant_value->value);
      }
   }
   /* FINISHME: Other dereference modes. */
}


void
ir_constant_visitor::visit(ir_assignment *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_constant *ir)
{
   value = ir;
}


void
ir_constant_visitor::visit(ir_call *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_return *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_if *ir)
{
   (void) ir;
   value = NULL;
}
