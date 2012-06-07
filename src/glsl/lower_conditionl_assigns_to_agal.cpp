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
#include "ir_optimization.h"
#include "glsl_types.h"
#include "program/hash_table.h"
#include "builtin_variables.h"
#include "replaceInstruction.h"
#include "ir_print_agal_visitor.h"

class ir_lower_conditional_assigns_to_agal_visitor : public ir_hierarchical_visitor {
public:
   ir_instruction *varbase;
   int tmpcount;
   hash_table *assignedvars;
   ir_lower_conditional_assigns_to_agal_visitor()
   {
      varbase = NULL;
      tmpcount = 0;
      assignedvars = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit(ir_variable *);
};

ir_visitor_status ir_lower_conditional_assigns_to_agal_visitor::visit(ir_variable *v)
{
   if(!varbase)
      varbase = base_ir;

   bool previouslywritten = hash_table_find(assignedvars, v) != NULL;
   if(!previouslywritten)
   hash_table_insert(assignedvars, (void*)0x1, v);

   return visit_continue;
}

ir_visitor_status
ir_lower_conditional_assigns_to_agal_visitor::visit_leave(ir_assignment *ir)
{
   ir_rvalue *realresult = ir->lhs;
   ir_variable *var = digOutVar(realresult);
   bool previouslywritten = hash_table_find(assignedvars, var) != NULL;
   if(!previouslywritten)
      hash_table_insert(assignedvars, (void*)0x1, var);

   if(!ir->condition)
      return visit_continue;

   void *ctx = ralloc_parent(ir);

   ir_swizzle *swiz = realresult->as_swizzle();

   // new tmporary to store the conditionalised computation in
   char nm[128];
   sprintf(&nm[0], "cond_assign_tmp_result_%d", tmpcount++);
   ir_variable *tmpval = new(ctx) ir_variable(var->type, ralloc_strdup(ctx, &nm[0]), ir_var_auto, glsl_precision_undefined);
   
   // switch the assignment so it outputs to the temp var
   if(swiz) {
      ir_swizzle *newswiz = swiz->clone(ctx, NULL);
      swiz->val = new (ctx) ir_dereference_variable(tmpval);
      ir->set_lhs(newswiz);
   } else {
      ir->set_lhs(new (ctx) ir_dereference_variable(tmpval));
   }
   varbase->insert_before(tmpval);

   // temporary to hold the result of the conditional (a float with value of 0 or 1)
   sprintf(&nm[0], "cond_assign_tmp_cond_%d", tmpcount++);
   ir_variable *condvar = new (ctx) ir_variable(glsl_type::float_type, ralloc_strdup(ctx, &nm[0]), ir_var_auto, glsl_precision_undefined);
   ir_assignment *condassign = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(condvar), ir->condition);
   varbase->insert_before(condvar);
   base_ir->insert_before(condassign);

   char tmpvalswiz[] = "xyzw";
   tmpvalswiz[tmpval->component_slots()] = 0;

   if(previouslywritten)
   {
      ir_assignment *copysrc = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(tmpval),
         new (ctx) ir_dereference_variable(var));
      base_ir->insert_before(copysrc);
   }

   ir_assignment *mult1 = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(tmpval),
      new (ctx) ir_expression(ir_binop_mul,
         new (ctx) ir_dereference_variable(tmpval),
        new (ctx) ir_dereference_variable(condvar)));
   base_ir->insert_after(mult1);

   ir_assignment *invert = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(condvar),
      new (ctx) ir_expression(ir_binop_sub,
         new (ctx) ir_constant(1.0f),
         new (ctx) ir_dereference_variable(condvar)));
   mult1->insert_after(invert);

   ir_assignment *mult2 = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(var),
      new (ctx) ir_expression(ir_binop_mul,
         new (ctx) ir_dereference_variable(var),
        new (ctx) ir_dereference_variable(condvar)));
   invert->insert_after(mult2);

   ir_assignment *add = new (ctx) ir_assignment(new (ctx) ir_dereference_variable(var),
      new (ctx) ir_expression(ir_binop_add,
         new (ctx) ir_dereference_variable(var),
        new (ctx) ir_dereference_variable(tmpval)));
   mult2->insert_after(add);

   ir->condition = NULL;

   return visit_continue;
}


bool
do_lower_conditionl_assigns_to_agal(exec_list *instructions)
{
   ir_lower_conditional_assigns_to_agal_visitor v;
   v.run(instructions);
   return false;
}