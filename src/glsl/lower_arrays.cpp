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

/*
 * Decomposes all arrays and array accesses. Can't handle non-const indexes.
 * Works best if run after loop unrolling so that as many array indexes are
 * turned into constants.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "program/hash_table.h"
#include "replaceInstruction.h"

class ir_array_decomposer_visitor : public ir_hierarchical_visitor {
public:
   hash_table *varhash;
   ir_variable *basevar;

   ir_array_decomposer_visitor()
   {
      basevar = NULL;
      varhash = hash_table_ctor(0, hash_table_string_hash, hash_table_string_compare);
   }

   ir_rvalue* computeNewRValue(ir_rvalue *rval);

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_enter(ir_assignment *);
};

ir_rvalue*
ir_array_decomposer_visitor::computeNewRValue(ir_rvalue *rval)
{
   ir_dereference_array *da = rval->as_dereference_array();
   if(da && (da->array->type->is_array() || da->array->type->is_matrix())) {
      ir_constant *ci = da->array_index->constant_expression_value();
      if(!ci) {
         return NULL;
      }

      ir_variable *src = da->variable_referenced();
      if(!src) {
         return NULL;
      }

      // make sure unsized arrays end up with a reasonable size
      if(src->max_array_access < ci->get_int_component(0)) {
         src->max_array_access = ci->get_int_component(0);
         //fprintf(stderr, "updating max access: %d\n", src->max_array_access);
      }

      void *ctx = ralloc_parent(base_ir);

      char nm[512] = {0};
      sprintf(&nm[0], "%s_%d", src->name, ci->get_int_component(0));

      ir_variable *v = (ir_variable*)hash_table_find(varhash, &nm[0]);
      if(!v) {
         v = new (ctx) ir_variable(src->type->element_type(), ralloc_strdup(ctx, &nm[0]), (ir_variable_mode)src->mode, (glsl_precision)src->precision);
         v->parent = src;
         v->location = ci->get_int_component(0);
         basevar->insert_before(v);
         hash_table_insert(varhash, v, ralloc_strdup(ctx, &nm[0]));
      }
      
      ir_dereference_variable *dv = new(ctx) ir_dereference_variable(v);

      int c = v->type->vector_elements;
      if(c == 4) {
         return dv;
      }

      char si[5] = "xyzw";
      si[c] = 0;
      ir_swizzle *sz = ir_swizzle::create(dv, si, c);
      return sz;
   }

   return NULL;
}

ir_visitor_status
ir_array_decomposer_visitor::visit(ir_variable *ir)
{
   if(!basevar) basevar = ir;

   if(ir->type->is_array()) {   
      unsigned array_count = ir->type->array_size();
      void *ctx = ir;
      while(array_count-- > 0) {
         char nm[512];
         sprintf(&nm[0], "%s_%d", ir->name, array_count);
         ir_variable *newvar = new (ctx) ir_variable(ir->type->element_type(), ralloc_strdup(ctx, &nm[0]), (ir_variable_mode)ir->mode, (glsl_precision)ir->precision);
         newvar->parent = ir;
         newvar->location = array_count;
         base_ir->insert_after(newvar);
         hash_table_insert(varhash, newvar, ralloc_strdup(ctx, &nm[0]));
      }
   } else if(ir->type->is_matrix()) {
      unsigned n = ir->type->matrix_columns;
      void *ctx = ir;
      while(n-- > 0) {
         char nm[512];
         sprintf(&nm[0], "%s_%d", ir->name, n);
         ir_variable *newvar = new (ctx) ir_variable(glsl_type::vec4_type, ralloc_strdup(ctx, &nm[0]), (ir_variable_mode)ir->mode, (glsl_precision)ir->precision);
         newvar->parent = ir;
         newvar->location = n;
         base_ir->insert_after(newvar);
         hash_table_insert(varhash, newvar, ralloc_strdup(ctx, &nm[0]));
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_array_decomposer_visitor::visit_enter(ir_dereference_array *ir)
{
   ir_rvalue *newir = computeNewRValue(ir);

   if(newir) {
      if(!replaceInstruction(base_ir, ir, newir))
         abort();
   }

   return visit_continue;
}

ir_visitor_status
ir_array_decomposer_visitor::visit_enter(ir_assignment *ir)
{
   ir_dereference *oldlhs = ir->lhs;
   ir_rvalue *newlhs = computeNewRValue(oldlhs);

   if(newlhs) {
      ir->set_lhs(newlhs);
      ralloc_free(oldlhs);
   }

   return visit_continue;
}

bool
do_lower_arrays(exec_list *instructions)
{
   ir_array_decomposer_visitor v;

   v.run(instructions);

   return false;
}