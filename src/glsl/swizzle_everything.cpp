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
#include "ir_rvalue_visitor.h"
#include "glsl_types.h"
#include "program/hash_table.h"
#include "replaceInstruction.h"
#include <vector>


// ===================================================================================
// ===================================================================================

class ir_swizzle_everything_visitor : public ir_rvalue_visitor {
public:
   ir_rvalue **swizzled;
   ir_swizzle_everything_visitor()
   {
   }

   virtual ir_visitor_status visit_enter(ir_swizzle *);
   virtual void handle_rvalue(ir_rvalue **rvalue);
};

ir_visitor_status
ir_swizzle_everything_visitor::visit_enter(ir_swizzle *ir)
{
   swizzled = &ir->val;
   return visit_continue;
}

void ir_swizzle_everything_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   if(swizzled == rvalue)
      return;

   ir_instruction *ir = *rvalue;

   if(!ir || !ir->as_dereference_variable())
      return;

   ir_variable *var = ir->as_dereference_variable()->variable_referenced();
   int n =  var->component_slots();

   if(n == 0 || n > 4) {
      //fprintf(stderr, "CANT SWIZZLE ARRAY %s %d\n", var->name, n);
      return;
   }

   //fprintf(stderr, "SWIZ %s %d\n", var->name, n);

   char swiz[] = "xyzw";
   swiz[n] = 0;

   ir_swizzle *s = ir_swizzle::create(*rvalue, swiz, n);

   if(!s) {
      //fprintf(stderr, "FAILED TO CREATE SWIZZLE\n", n);
      return;
   }

   *rvalue = s;

   return;
}

bool
do_swizzle_everything(exec_list *instructions)
{
   ir_swizzle_everything_visitor sv;
   sv.run(instructions);

   return false;
}