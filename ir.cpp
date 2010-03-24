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
#include <string.h>
#include "main/imports.h"
#include "main/simple_list.h"
#include "ir.h"
#include "glsl_types.h"

ir_assignment::ir_assignment(ir_instruction *lhs, ir_instruction *rhs,
			     ir_expression *condition)
   : ir_instruction(ir_op_assign)
{
   this->lhs = (ir_dereference *) lhs;
   this->rhs = rhs;
   this->condition = condition;
}


ir_expression::ir_expression(int op, const struct glsl_type *type,
			     ir_instruction *op0, ir_instruction *op1)
   : ir_instruction(ir_op_expression)
{
   this->type = type;
   this->operation = ir_expression_operation(op);
   this->operands[0] = op0;
   this->operands[1] = op1;
}


ir_label::ir_label(const char *label)
   : ir_instruction(ir_op_label), label(label)
{
   /* empty */
}


ir_constant::ir_constant(const struct glsl_type *type, const void *data)
   : ir_instruction(ir_op_constant)
{
   const unsigned elements = 
      ((type->vector_elements == 0) ? 1 : type->vector_elements)
      * ((type->matrix_rows == 0) ? 1 : type->matrix_rows);
   unsigned size = 0;

   this->type = type;
   switch (type->base_type) {
   case GLSL_TYPE_UINT:  size = sizeof(this->value.u[0]); break;
   case GLSL_TYPE_INT:   size = sizeof(this->value.i[0]); break;
   case GLSL_TYPE_FLOAT: size = sizeof(this->value.f[0]); break;
   case GLSL_TYPE_BOOL:  size = sizeof(this->value.b[0]); break;
   default:
      /* FINISHME: What to do?  Exceptions are not the answer.
       */
      break;
   }

   memcpy(& this->value, data, size * elements);
}


ir_dereference::ir_dereference(ir_instruction *var)
   : ir_instruction(ir_op_dereference)
{
   this->mode = ir_reference_variable;
   this->var = var;
   this->type = (var != NULL) ? var->type : glsl_error_type;
}


ir_variable::ir_variable(const struct glsl_type *type, const char *name)
   : ir_instruction(ir_op_var_decl), read_only(false), centroid(false),
     invariant(false), mode(ir_var_auto), interpolation(ir_var_smooth)
{
   this->type = type;
   this->name = name;
}


ir_function_signature::ir_function_signature(const glsl_type *return_type)
   : ir_instruction(ir_op_func_sig), return_type(return_type), definition(NULL)
{
   /* empty */
}


ir_function::ir_function(const char *name)
   : ir_instruction(ir_op_func), name(name)
{
   /* empty */
}


ir_call *
ir_call::get_error_instruction()
{
   ir_call *call = new ir_call;

   call->type = glsl_error_type;
   return call;
}
