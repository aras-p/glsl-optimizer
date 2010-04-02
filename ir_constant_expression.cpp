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
   unsigned int i;

   for (i = 0; i < ir->get_num_operands(); i++) {
      op[i] = ir->operands[i]->constant_expression_value();
      if (!op[i])
	 return;
   }

   switch (ir->operation) {
   case ir_unop_logic_not:
      value = new ir_constant(!op[0]->value.b[0]);
      value->type = glsl_type::bool_type;
      break;
   case ir_binop_mul:
      if (ir->operands[0]->type == ir->operands[1]->type) {
	 if (ir->operands[1]->type == glsl_type::float_type) {
	    value = new ir_constant(op[0]->value.f[0] * op[1]->value.f[0]);
	    value->type = glsl_type::float_type;
	 }
      }
      if (value)
	 value->type = ir->operands[1]->type;
      break;
   case ir_binop_logic_and:
      value = new ir_constant(op[0]->value.b[0] && op[1]->value.b[0]);
      value->type = glsl_type::bool_type;
      break;
   case ir_binop_logic_or:
      value = new ir_constant(op[0]->value.b[0] || op[1]->value.b[0]);
      value->type = glsl_type::bool_type;
      break;
   default:
      break;
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
   (void) ir;
   value = NULL;
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
