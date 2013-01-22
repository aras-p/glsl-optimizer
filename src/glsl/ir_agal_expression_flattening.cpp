/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_hierarchical_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"
#include "replaceInstruction.h"
#include "program/hash_table.h"

// ===================================================================================
// ===================================================================================

class ir_expression_replacing_visitor : public ir_rvalue_visitor {
public:
   hash_table *promotions;
   ir_expression_replacing_visitor(hash_table *_promotions)
   {
      this->promotions = _promotions;
      base_ir = NULL;
   }

   virtual ~ir_expression_replacing_visitor()
   {
      /* empty */
   }

   void handle_rvalue(ir_rvalue **rvalue);
};

void
ir_expression_replacing_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   ir_variable *var;
   ir_assignment *assign;
   ir_rvalue *ir = *rvalue;
   ir_dereference_variable *replacement = (ir_dereference_variable*)hash_table_find(promotions, ir);

   if (!ir || !replacement || !base_ir || hash_table_find(promotions, rvalue) == (void*)0x1)
      return;

   //fprintf(stderr, "// replacing %p %p %p\n", ir, replacement, rvalue);
   *rvalue = replacement;
}

// ===================================================================================
// ===================================================================================

class ir_agal_expression_flattening_visitor : public ir_hierarchical_visitor {
public:
   int tmpcount;
   ir_instruction *baseExpr, *current_array_index;
   ir_instruction *insertionPoint, *varbase;
   hash_table *promotions;
   int promotioncount;
   bool verifierAppeasement;

   ir_agal_expression_flattening_visitor(bool _verifierAppeasement)
   {
      base_ir = NULL;
      baseExpr = NULL;
      varbase = NULL;
      insertionPoint = NULL;
      current_array_index = NULL;
      promotioncount = tmpcount = 0;
      verifierAppeasement = _verifierAppeasement;
      promotions = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   virtual ~ir_agal_expression_flattening_visitor()
   {
      /* empty */
   }
   
   ir_dereference_variable* appeaseToVar(ir_rvalue *ir);
   ir_dereference_variable* promoteToVar(ir_rvalue *ir, bool evenSwizzles = false);

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_leave(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_enter(ir_swizzle *);
   virtual ir_visitor_status visit_leave(ir_swizzle *);
   virtual ir_visitor_status visit_enter(ir_texture *);
   virtual ir_visitor_status visit_leave(ir_texture *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_leave(ir_dereference_array *);

};

ir_visitor_status ir_agal_expression_flattening_visitor::visit(ir_variable *v)
{
   if(!varbase)
      varbase = base_ir;
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_enter(ir_call *expr)
{
   if(verifierAppeasement) {
      bool isAllConst = true;
      foreach_list_safe(_arg, &expr->actual_parameters) {
         ir_rvalue *arg = (ir_rvalue*)_arg;
         ir_variable *v = digOutVar(arg);
         bool isOperandConst = v && (v->mode == ir_var_uniform || v->constant_value);
         if(!isOperandConst) isOperandConst = digOutConst(arg);
         isAllConst = isOperandConst && isAllConst;
      }

      if(isAllConst)
         promoteToVar((ir_rvalue*)expr->actual_parameters.get_head());
   }

   if(!baseExpr) {
      baseExpr = expr;
      return visit_continue;
   }

   promoteToVar(expr);
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_leave(ir_call *expr)
{
   if(baseExpr == expr) {
      baseExpr = NULL;
      insertionPoint = NULL;
   }
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_enter(ir_dereference_array *da)
{
   current_array_index = da->array_index;
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_leave(ir_dereference_array *da)
{
   current_array_index = NULL;
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_enter(ir_swizzle *expr)
{
   if(!baseExpr) {
      baseExpr = expr;
      return visit_continue;
   }

   promoteToVar(expr);
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_leave(ir_swizzle *expr)
{
   if(baseExpr == expr) {
      baseExpr = NULL;
      insertionPoint = NULL;
   }
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_enter(ir_texture *expr)
{
   if(!baseExpr) {
      baseExpr = expr;
      return visit_continue;
   }

   promoteToVar(expr);
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_leave(ir_texture *expr)
{
   if(baseExpr == expr) {
      baseExpr = NULL;
      insertionPoint = NULL;
   }
   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_enter(ir_expression *expr)
{
   if(!baseExpr) baseExpr = expr;

   //fprintf(stderr, "// replacing 1 %p %s %d\n", expr, expr->operator_string(), expr->as_swizzle());

   if(expr != baseExpr || expr == current_array_index)
      promoteToVar(expr);

   if(verifierAppeasement) {
      bool isAllConst = true;
      for(int i=0; i<expr->get_num_operands(); i++) {
         ir_variable *v = digOutVar(expr->operands[i]);
         bool isOperandConst = v && (v->mode == ir_var_uniform || v->constant_value);
         if(!isOperandConst) isOperandConst = digOutConst(expr->operands[i]);
         isAllConst = isOperandConst && isAllConst;
      }

      // all operands to this expr use inputs that will map to
      // AGAL program constants, one must be promoted to a tmp
      // to appease the AGAL verifier
      if(isAllConst) {
         if(expr->get_num_operands() == 2) {
            if(expr->operands[0]->type->is_matrix())
               promoteToVar(expr->operands[1], true);
            else
               promoteToVar(expr->operands[0], true);
         } else {
            promoteToVar(expr->operands[0], true);
         }
      }
   }

   return visit_continue;
}

ir_visitor_status ir_agal_expression_flattening_visitor::visit_leave(ir_expression *expr)
{
   if(baseExpr == expr) {
      baseExpr = NULL;
      insertionPoint = NULL;
   }
   return visit_continue;
}

ir_dereference_variable*
ir_agal_expression_flattening_visitor::promoteToVar(ir_rvalue *ir, bool evenSwizzles)
{
   ir_variable *var;
   ir_assignment *assign;

   if (!ir || !base_ir || (!evenSwizzles && ir->as_swizzle())) {
      //fprintf(stderr, "// not replacing %p %p %d\n", ir, base_ir, ir->as_swizzle());
      return NULL;
   }

   void *ctx = ralloc_parent(ir);

   char nm[512];
   sprintf(&nm[0], "flattening_tmp_%d", tmpcount++);
   var = new(ctx) ir_variable(ir->type, ralloc_strdup(ctx, &nm[0]), ir_var_temporary, precision_from_ir(ir));
   varbase->insert_before(var);

   assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(var),
				   ir,
				   NULL);

   if(!insertionPoint)
      insertionPoint = base_ir;
   insertionPoint->insert_before(assign);
   insertionPoint = assign;

   ir_dereference_variable *replacement = new(ctx) ir_dereference_variable(var);

   //fprintf(stderr, "// replacement %p %p %p (%d)\n", ir, replacement, &(assign->rhs), ir->ir_type);
   hash_table_insert(promotions, replacement, ir);
   hash_table_insert(promotions, (void*)0x1, &(assign->rhs));
   promotioncount++;
   return replacement;
}

bool
do_agal_expression_flattening(exec_list *instructions, bool verifierAppeasement)
{
   ir_agal_expression_flattening_visitor v(verifierAppeasement);
   v.run(instructions);

   if(v.promotioncount == 0) return false;

   ir_expression_replacing_visitor r(v.promotions);
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir->accept(&r);
   }

   return true;
}