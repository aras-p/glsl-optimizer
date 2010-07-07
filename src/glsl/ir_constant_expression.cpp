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

#include <math.h>
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
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_if *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
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
   ir_constant *op[2] = { NULL, NULL };
   ir_constant_data data;

   memset(&data, 0, sizeof(data));

   for (unsigned operand = 0; operand < ir->get_num_operands(); operand++) {
      op[operand] = ir->operands[operand]->constant_expression_value();
      if (!op[operand])
	 return;
   }

   if (op[1] != NULL)
      assert(op[0]->type->base_type == op[1]->type->base_type);

   bool op0_scalar = op[0]->type->is_scalar();
   bool op1_scalar = op[1] != NULL && op[1]->type->is_scalar();

   /* When iterating over a vector or matrix's components, we want to increase
    * the loop counter.  However, for scalars, we want to stay at 0.
    */
   unsigned c0_inc = op0_scalar ? 1 : 0;
   unsigned c1_inc = op1_scalar ? 1 : 0;
   unsigned components = op[op1_scalar ? 0 : 1]->type->components();

   switch (ir->operation) {
   case ir_unop_logic_not:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++)
	 data.b[c] = !op[0]->value.b[c];
      break;

   case ir_unop_f2i:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.i[c] = op[0]->value.f[c];
      }
      break;
   case ir_unop_i2f:
      assert(op[0]->type->base_type == GLSL_TYPE_UINT ||
	     op[0]->type->base_type == GLSL_TYPE_INT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 if (op[0]->type->base_type == GLSL_TYPE_INT)
	    data.f[c] = op[0]->value.i[c];
	 else
	    data.f[c] = op[0]->value.u[c];
      }
      break;
   case ir_unop_b2f:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = op[0]->value.b[c] ? 1.0 : 0.0;
      }
      break;
   case ir_unop_f2b:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.b[c] = bool(op[0]->value.f[c]);
      }
      break;
   case ir_unop_b2i:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.u[c] = op[0]->value.b[c] ? 1 : 0;
      }
      break;
   case ir_unop_i2b:
      assert(op[0]->type->is_integer());
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.b[c] = bool(op[0]->value.u[c]);
      }
      break;

   case ir_unop_fract:
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = 0;
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = 0;
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c] - floor(op[0]->value.f[c]);
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_neg:
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = -op[0]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = -op[0]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = -op[0]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_abs:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c];
	    if (data.i[c] < 0)
	       data.i[c] = -data.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = fabs(op[0]->value.f[c]);
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_rcp:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_UINT:
	    if (op[0]->value.u[c] != 0.0)
	       data.u[c] = 1 / op[0]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    if (op[0]->value.i[c] != 0.0)
	       data.i[c] = 1 / op[0]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    if (op[0]->value.f[c] != 0.0)
	       data.f[c] = 1.0 / op[0]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_rsq:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = 1.0 / sqrtf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_sqrt:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = sqrtf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_exp:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = expf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_log:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = logf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 data.f[c] = 0.0;
      }
      break;

   case ir_binop_dot:
      assert(op[0]->type->is_vector() && op[1]->type->is_vector());
      data.f[0] = 0;
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[0] += op[0]->value.u[c] * op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[0] += op[0]->value.i[c] * op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[0] += op[0]->value.f[c] * op[1]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_add:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c0] + op[1]->value.u[c1];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c0] + op[1]->value.i[c1];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] + op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_sub:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c0] - op[1]->value.u[c1];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c0] - op[1]->value.i[c1];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] - op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_mul:
      /* Check for equal types, or unequal types involving scalars */
      if ((op[0]->type == op[1]->type && !op[0]->type->is_matrix())
	  || op0_scalar || op1_scalar) {
	 for (unsigned c = 0, c0 = 0, c1 = 0;
	      c < components;
	      c0 += c0_inc, c1 += c1_inc, c++) {

	    switch (ir->operands[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       data.u[c] = op[0]->value.u[c0] * op[1]->value.u[c1];
	       break;
	    case GLSL_TYPE_INT:
	       data.i[c] = op[0]->value.i[c0] * op[1]->value.i[c1];
	       break;
	    case GLSL_TYPE_FLOAT:
	       data.f[c] = op[0]->value.f[c0] * op[1]->value.f[c1];
	       break;
	    default:
	       assert(0);
	    }
	 }
      } else {
	 assert(op[0]->type->is_matrix() || op[1]->type->is_matrix());

	 /* Multiply an N-by-M matrix with an M-by-P matrix.  Since either
	  * matrix can be a GLSL vector, either N or P can be 1.
	  *
	  * For vec*mat, the vector is treated as a row vector.  This
	  * means the vector is a 1-row x M-column matrix.
	  *
	  * For mat*vec, the vector is treated as a column vector.  Since
	  * matrix_columns is 1 for vectors, this just works.
	  */
	 const unsigned n = op[0]->type->is_vector()
	    ? 1 : op[0]->type->vector_elements;
	 const unsigned m = op[1]->type->vector_elements;
	 const unsigned p = op[1]->type->matrix_columns;
	 for (unsigned j = 0; j < p; j++) {
	    for (unsigned i = 0; i < n; i++) {
	       for (unsigned k = 0; k < m; k++) {
		  data.f[i+n*j] += op[0]->value.f[i+n*k]*op[1]->value.f[k+m*j];
	       }
	    }
	 }
      }

      break;
   case ir_binop_div:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c0] / op[1]->value.u[c1];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c0] / op[1]->value.i[c1];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] / op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_logic_and:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] && op[1]->value.b[c];
      break;
   case ir_binop_logic_xor:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] ^ op[1]->value.b[c];
      break;
   case ir_binop_logic_or:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] || op[1]->value.b[c];
      break;

   case ir_binop_less:
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 data.b[0] = op[0]->value.u[0] < op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 data.b[0] = op[0]->value.i[0] < op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 data.b[0] = op[0]->value.f[0] < op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_greater:
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 data.b[0] = op[0]->value.u[0] > op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 data.b[0] = op[0]->value.i[0] > op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 data.b[0] = op[0]->value.f[0] > op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_lequal:
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 data.b[0] = op[0]->value.u[0] <= op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 data.b[0] = op[0]->value.i[0] <= op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 data.b[0] = op[0]->value.f[0] <= op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;
   case ir_binop_gequal:
      switch (ir->operands[0]->type->base_type) {
      case GLSL_TYPE_UINT:
	 data.b[0] = op[0]->value.u[0] >= op[1]->value.u[0];
	 break;
      case GLSL_TYPE_INT:
	 data.b[0] = op[0]->value.i[0] >= op[1]->value.i[0];
	 break;
      case GLSL_TYPE_FLOAT:
	 data.b[0] = op[0]->value.f[0] >= op[1]->value.f[0];
	 break;
      default:
	 assert(0);
      }
      break;

   case ir_binop_equal:
      data.b[0] = true;
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[0] = data.b[0] && op[0]->value.u[c] == op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[0] = data.b[0] && op[0]->value.i[c] == op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[0] = data.b[0] && op[0]->value.f[c] == op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[0] = data.b[0] && op[0]->value.b[c] == op[1]->value.b[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_nequal:
      data.b[0] = false;
      for (unsigned c = 0; c < ir->operands[0]->type->components(); c++) {
	 switch (ir->operands[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[0] = data.b[0] || op[0]->value.u[c] != op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[0] = data.b[0] || op[0]->value.i[c] != op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[0] = data.b[0] || op[0]->value.f[c] != op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[0] = data.b[0] || op[0]->value.b[c] != op[1]->value.b[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   default:
      /* FINISHME: Should handle all expression types. */
      return;
   }

   void *ctx = talloc_parent(ir);
   this->value = new(ctx) ir_constant(ir->type, &data);
}


void
ir_constant_visitor::visit(ir_texture *ir)
{
   // FINISHME: Do stuff with texture lookups
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_swizzle *ir)
{
   ir_constant *v = ir->val->constant_expression_value();

   this->value = NULL;

   if (v != NULL) {
      ir_constant_data data;

      const unsigned swiz_idx[4] = {
	 ir->mask.x, ir->mask.y, ir->mask.z, ir->mask.w
      };

      for (unsigned i = 0; i < ir->mask.num_components; i++) {
	 switch (v->type->base_type) {
	 case GLSL_TYPE_UINT:
	 case GLSL_TYPE_INT:   data.u[i] = v->value.u[swiz_idx[i]]; break;
	 case GLSL_TYPE_FLOAT: data.f[i] = v->value.f[swiz_idx[i]]; break;
	 case GLSL_TYPE_BOOL:  data.b[i] = v->value.b[swiz_idx[i]]; break;
	 default:              assert(!"Should not get here."); break;
	 }
      }

      void *ctx = talloc_parent(ir);
      this->value = new(ctx) ir_constant(ir->type, &data);
   }
}


void
ir_constant_visitor::visit(ir_dereference_variable *ir)
{
   value = NULL;

   ir_variable *var = ir->variable_referenced();
   if (var && var->constant_value)
      value = var->constant_value->clone(NULL);
}


void
ir_constant_visitor::visit(ir_dereference_array *ir)
{
   void *ctx = talloc_parent(ir);
   ir_constant *array = ir->array->constant_expression_value();
   ir_constant *idx = ir->array_index->constant_expression_value();

   this->value = NULL;

   if ((array != NULL) && (idx != NULL)) {
      if (array->type->is_matrix()) {
	 /* Array access of a matrix results in a vector.
	  */
	 const unsigned column = idx->value.u[0];

	 const glsl_type *const column_type = array->type->column_type();

	 /* Offset in the constant matrix to the first element of the column
	  * to be extracted.
	  */
	 const unsigned mat_idx = column * column_type->vector_elements;

	 ir_constant_data data;

	 switch (column_type->base_type) {
	 case GLSL_TYPE_UINT:
	 case GLSL_TYPE_INT:
	    for (unsigned i = 0; i < column_type->vector_elements; i++)
	       data.u[i] = array->value.u[mat_idx + i];

	    break;

	 case GLSL_TYPE_FLOAT:
	    for (unsigned i = 0; i < column_type->vector_elements; i++)
	       data.f[i] = array->value.f[mat_idx + i];

	    break;

	 default:
	    assert(!"Should not get here.");
	    break;
	 }

	 this->value = new(ctx) ir_constant(column_type, &data);
      } else if (array->type->is_vector()) {
	 const unsigned component = idx->value.u[0];

	 this->value = new(ctx) ir_constant(array, component);
      } else {
	 /* FINISHME: Handle access of constant arrays. */
      }
   }
}


void
ir_constant_visitor::visit(ir_dereference_record *ir)
{
   ir_constant *v = ir->record->constant_expression_value();

   this->value = (v != NULL) ? v->get_record_field(ir->field) : NULL;
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
ir_constant_visitor::visit(ir_discard *ir)
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


void
ir_constant_visitor::visit(ir_loop *ir)
{
   (void) ir;
   value = NULL;
}


void
ir_constant_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
   value = NULL;
}
