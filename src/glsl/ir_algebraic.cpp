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
 * \file ir_algebraic.cpp
 *
 * Takes advantage of association, commutivity, and other algebraic
 * properties to simplify expressions.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class ir_algebraic_visitor : public ir_hierarchical_visitor {
public:
   ir_algebraic_visitor()
   {
      this->progress = false;
   }

   virtual ~ir_algebraic_visitor()
   {
   }

   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_leave(ir_dereference_array *);
   virtual ir_visitor_status visit_leave(ir_expression *);
   virtual ir_visitor_status visit_leave(ir_if *);
   virtual ir_visitor_status visit_leave(ir_return *);
   virtual ir_visitor_status visit_leave(ir_swizzle *);
   virtual ir_visitor_status visit_leave(ir_texture *);

   ir_rvalue *handle_expression(ir_rvalue *in_ir);

   bool progress;
};

static bool
is_vec_zero(ir_constant *ir)
{
   int c;

   if (!ir)
      return false;
   if (!ir->type->is_scalar() &&
       !ir->type->is_vector())
      return false;

   for (c = 0; c < ir->type->vector_elements; c++) {
      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 if (ir->value.f[c] != 0.0)
	    return false;
	 break;
      case GLSL_TYPE_INT:
	 if (ir->value.i[c] != 0)
	    return false;
	 break;
      case GLSL_TYPE_UINT:
	 if (ir->value.u[c] != 0)
	    return false;
	 break;
      case GLSL_TYPE_BOOL:
	 if (ir->value.b[c] != false)
	    return false;
	 break;
      default:
	 assert(!"bad base type");
	 return false;
      }
   }

   return true;
}

static bool
is_vec_one(ir_constant *ir)
{
   int c;

   if (!ir)
      return false;
   if (!ir->type->is_scalar() &&
       !ir->type->is_vector())
      return false;

   for (c = 0; c < ir->type->vector_elements; c++) {
      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 if (ir->value.f[c] != 1.0)
	    return false;
	 break;
      case GLSL_TYPE_INT:
	 if (ir->value.i[c] != 1)
	    return false;
	 break;
      case GLSL_TYPE_UINT:
	 if (ir->value.u[c] != 1)
	    return false;
	 break;
      case GLSL_TYPE_BOOL:
	 if (ir->value.b[c] != true)
	    return false;
	 break;
      default:
	 assert(!"bad base type");
	 return false;
      }
   }

   return true;
}

ir_rvalue *
ir_algebraic_visitor::handle_expression(ir_rvalue *in_ir)
{
   ir_expression *ir = (ir_expression *)in_ir;
   ir_constant *op_const[2] = {NULL, NULL};
   ir_expression *op_expr[2] = {NULL, NULL};
   unsigned int i;

   if (!in_ir)
      return NULL;

   if (in_ir->ir_type != ir_type_expression)
      return in_ir;

   for (i = 0; i < ir->get_num_operands(); i++) {
      if (ir->operands[i]->type->is_matrix())
	 return in_ir;

      op_const[i] = ir->operands[i]->constant_expression_value();
      op_expr[i] = ir->operands[i]->as_expression();
   }

   switch (ir->operation) {
   case ir_unop_logic_not: {
      enum ir_expression_operation new_op = ir_unop_logic_not;

      if (op_expr[0] == NULL)
	 break;

      switch (op_expr[0]->operation) {
      case ir_binop_less:    new_op = ir_binop_gequal;  break;
      case ir_binop_greater: new_op = ir_binop_lequal;  break;
      case ir_binop_lequal:  new_op = ir_binop_greater; break;
      case ir_binop_gequal:  new_op = ir_binop_less;    break;
      case ir_binop_equal:   new_op = ir_binop_nequal;  break;
      case ir_binop_nequal:  new_op = ir_binop_equal;   break;

      default:
	 /* The default case handler is here to silence a warning from GCC.
	  */
	 break;
      }

      if (new_op != ir_unop_logic_not) {
	 this->progress = true;
	 return new(ir) ir_expression(new_op,
				      ir->type,
				      op_expr[0]->operands[0],
				      op_expr[0]->operands[1]);
      }

      break;
   }

   case ir_binop_add:
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 return ir->operands[1];
      }
      if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      }
      break;

   case ir_binop_sub:
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 return new(ir) ir_expression(ir_unop_neg,
				      ir->type,
				      ir->operands[1],
				      NULL);
      }
      if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      }
      break;

   case ir_binop_mul:
      if (is_vec_one(op_const[0])) {
	 this->progress = true;
	 return ir->operands[1];
      }
      if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      }

      if (is_vec_zero(op_const[0]) || is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir_constant::zero(ir, ir->type);
      }
      break;

   case ir_binop_div:
      if (is_vec_one(op_const[0]) && ir->type->base_type == GLSL_TYPE_FLOAT) {
	 this->progress = true;
	 return new(ir) ir_expression(ir_unop_rcp,
				      ir->type,
				      ir->operands[1],
				      NULL);
      }
      if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      }
      break;

   case ir_unop_rcp:
      if (op_expr[0] && op_expr[0]->operation == ir_unop_rcp) {
	 this->progress = true;
	 return op_expr[0]->operands[0];
      }

      /* FINISHME: We should do rcp(rsq(x)) -> sqrt(x) for some
       * backends, except that some backends will have done sqrt ->
       * rcp(rsq(x)) and we don't want to undo it for them.
       */

      /* As far as we know, all backends are OK with rsq. */
      if (op_expr[0] && op_expr[0]->operation == ir_unop_sqrt) {
	 this->progress = true;
	 return new(ir) ir_expression(ir_unop_rsq,
				      ir->type,
				      op_expr[0]->operands[0],
				      NULL);
      }

      break;

   default:
      break;
   }

   return in_ir;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand] = handle_expression(ir->operands[operand]);
   }

   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_texture *ir)
{
   ir->coordinate = handle_expression(ir->coordinate);
   ir->projector = handle_expression(ir->projector);
   ir->shadow_comparitor = handle_expression(ir->shadow_comparitor);

   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      ir->lod_info.bias = handle_expression(ir->lod_info.bias);
      break;
   case ir_txf:
   case ir_txl:
      ir->lod_info.lod = handle_expression(ir->lod_info.lod);
      break;
   case ir_txd:
      ir->lod_info.grad.dPdx = handle_expression(ir->lod_info.grad.dPdx);
      ir->lod_info.grad.dPdy = handle_expression(ir->lod_info.grad.dPdy);
      break;
   }

   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_swizzle *ir)
{
   ir->val = handle_expression(ir->val);
   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_dereference_array *ir)
{
   ir->array_index = handle_expression(ir->array_index);
   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_assignment *ir)
{
   ir->rhs = handle_expression(ir->rhs);
   ir->condition = handle_expression(ir->condition);
   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();
      ir_rvalue *new_param = handle_expression(param);

      if (new_param != param) {
	 param->replace_with(new_param);
      }
   }
   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_return *ir)
{
   ir->value = handle_expression(ir->value);;
   return visit_continue;
}

ir_visitor_status
ir_algebraic_visitor::visit_leave(ir_if *ir)
{
   ir->condition = handle_expression(ir->condition);
   return visit_continue;
}


bool
do_algebraic(exec_list *instructions)
{
   ir_algebraic_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
