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
#include "ir_rvalue_visitor.h"

class ir_remove_casts_visitor : public ir_rvalue_visitor {
public:

   ir_remove_casts_visitor()
   {
   }

   void handle_rvalue(ir_rvalue **rvalue);
};

void
ir_remove_casts_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   ir_rvalue *ir = *rvalue;
   if (!ir) return;

   if(ir->type == glsl_type::int_type ||
      ir->type == glsl_type::uint_type ||
      ir->type == glsl_type::bool_type)
   {
      ir_constant *c = ir->as_constant();
      if(c && ir->type == glsl_type::int_type) {
         for(int i=0;i<16;i++) c->value.f[i] = c->value.i[i];
      } else if(c && ir->type == glsl_type::uint_type) {
         for(int i=0;i<16;i++) c->value.f[i] = c->value.u[i];
      } else if(c && ir->type == glsl_type::bool_type) {
         for(int i=0;i<16;i++) c->value.f[i] = c->value.b[i];
      }
      ir->type = glsl_type::float_type;
   }

   ir_expression *expr = ir->as_expression();
   if (!expr) return;

   switch(expr->operation) {
      case ir_unop_f2b:
      case ir_unop_b2f:
      case ir_unop_b2i:
      case ir_unop_f2i:
      case ir_unop_i2f:
      case ir_unop_i2b:
      case ir_unop_u2f:
      case ir_unop_i2u:
      case ir_unop_u2i:
         *rvalue = expr->operands[0];
         break;
   }
}

bool
do_remove_casts(exec_list *instructions)
{
   ir_remove_casts_visitor v;
   v.run(instructions);
   return false;
}