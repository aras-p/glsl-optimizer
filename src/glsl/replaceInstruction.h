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

static ir_variable* digOutVar(ir_instruction *ir) {
   if(ir->as_swizzle()) {
      return digOutVar(ir->as_swizzle()->val);
   } else if(ir->as_dereference()) {
      return ir->as_dereference()->variable_referenced();
   } else {
      return NULL;
   }
}

static ir_constant* digOutConst(ir_instruction *ir) {
   if(ir->as_swizzle()) {
      return digOutConst(ir->as_swizzle()->val);
   } else if(ir->as_constant()) {
      return ir->as_constant();
   } else {
      return NULL;
   }
}

static bool replaceInstruction(ir_instruction *in, ir_rvalue *oldi, ir_rvalue *newi)
{
   bool flag = false;

   if(!in)
      return false;

   //fprintf(stderr, "replaceInstruction %d %p\n", in->ir_type, in);

   if(in->as_expression())
   {
      ir_expression *expr = in->as_expression();

      //fprintf(stderr, "replaceInstruction  expr %s %p %p\n", expr->operator_string(), oldi, newi);
      for(int i=0;i<4;i++)
      {
         if(expr->operands[i] == oldi)
         {
            expr->operands[i] = newi;
            flag = true;
         } else if(replaceInstruction(expr->operands[i], oldi, newi)) {
            flag = true;
         }
      }
   } else if(in->as_assignment()) {
      ir_assignment *ass = in->as_assignment();

      if(ass->rhs == oldi) {
         ass->rhs = newi;
         flag = true;
      } else if(replaceInstruction(ass->rhs, oldi, newi)) {
         flag = true;
      }
   } else if(in->as_call()) {
      ir_call *c = in->as_call();

      foreach_iter(exec_list_iterator, iter, *c) {
         ir_rvalue *param_rval = (ir_rvalue *)iter.get();
         if(param_rval == oldi) {
            param_rval->replace_with(newi);
            flag = true;
         } else if(replaceInstruction(param_rval, oldi, newi)) {
            flag = true;
         }
      }
   } else if(in->as_texture()) {
      ir_texture *t = in->as_texture();
      flag |= replaceInstruction(t->sampler, oldi, newi);
      flag |= replaceInstruction(t->coordinate, oldi, newi);
   } else if(in->as_dereference_record()) {
      ir_dereference_record *dr = in->as_dereference_record();

      if(replaceInstruction(dr->record, oldi, newi)) {
         flag = true;
      }
   } else if(in->as_dereference_variable()) {
      ir_dereference_variable *dv = in->as_dereference_variable();

      if(replaceInstruction(dv->var, oldi, newi)) {
         flag = true;
      }
   } else if(in->as_dereference_array()) {
      ir_dereference_array *da = in->as_dereference_array();

      if(da->array == oldi) {
         da->array = newi;
         flag = true;
      } else if(replaceInstruction(da->array, oldi, newi)) {
         flag = true;
      } else if(da->array_index == oldi) {
         da->array_index = newi;
         flag = true;
      } else if(replaceInstruction(da->array_index, oldi, newi)) {
         flag = true;
      }
   } else if(in->as_swizzle()) {
      ir_swizzle *sz = in->as_swizzle();

      if(sz->val == oldi) {
         sz->val = newi;
         flag = true;
      } else if(replaceInstruction(sz->val, oldi, newi)) {
         flag = true;
      }
   } else if(in->as_variable()) {
      // do nothing, this is a const init
   } else if(in->as_constant()) {
      // do nothing, this is a const init
   } else {
      fprintf(stderr, "unknown node: %d\n", in->ir_type);
   }

   return flag;
}