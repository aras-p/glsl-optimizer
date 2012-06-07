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

class ir_float_liverange_visitor : public ir_hierarchical_visitor {
public:
   hash_table *firstUse, *lastUse;

   ir_float_liverange_visitor()
   {
      firstUse = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
      lastUse = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   ir_visitor_status handleDereference(ir_dereference *);
};

ir_visitor_status ir_float_liverange_visitor::visit(ir_dereference_variable *d)
{
   return handleDereference(d);
}

ir_visitor_status ir_float_liverange_visitor::visit_enter(ir_dereference_array *d)
{
   return handleDereference(d);
}

ir_visitor_status ir_float_liverange_visitor::handleDereference(ir_dereference *d)
{
   ir_variable *var = d->variable_referenced();

   if(!var)
      abort();

   if(!hash_table_find(firstUse, var)) {
      hash_table_insert(firstUse, d, var);
      //fprintf(stderr, "=== ir_dereference_variable firstUse %p %p : %s ===\n", var, d, var->name);
   }

   if(hash_table_find(lastUse, var))
      hash_table_remove(lastUse, var);
   hash_table_insert(lastUse, d, var);
   //fprintf(stderr, "=== ir_dereference_variable lastUse %p %p : %s ===\n", var, d, var->name);

   return visit_continue;
}

// ===================================================================================
// ===================================================================================

class ir_coalesce_floats_replacing_visitor : public ir_rvalue_visitor {
public:
   hash_table *promotions;
   ir_coalesce_floats_replacing_visitor(hash_table *_promotions)
   {
      this->promotions = _promotions;
      base_ir = NULL;
   }

   virtual ~ir_coalesce_floats_replacing_visitor()
   {
      /* empty */
   }

   void handle_rvalue(ir_rvalue **rvalue);
};

void
ir_coalesce_floats_replacing_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   ir_rvalue *ir = *rvalue;
   if(!ir) return;

   ir_dereference *dv;
   ir_swizzle *swiz;

   ir_swizzle *outerswiz = ir->as_swizzle();
   if( outerswiz ) {
      dv = outerswiz->val->as_dereference();
      if(dv) {
         //fprintf(stderr, "var1: %s\n", dv->variable_referenced()->name);
         swiz = (ir_swizzle*)hash_table_find(promotions, dv->variable_referenced());
         if(swiz) {
            //fprintf(stderr, "// replacing[1] %p %p\n", ir, swiz);
            *rvalue = swiz->clone(ralloc_parent(swiz), NULL);
            return;
         }
      }
   }

   dv = ir->as_dereference();
   if(!dv) return;

   swiz = (ir_swizzle*)hash_table_find(promotions, dv->variable_referenced());

   //fprintf(stderr, "var2: %s %p\n", dv->variable_referenced()->name, dv->variable_referenced());
   if (!swiz) return;

   //fprintf(stderr, "// replacing[2] %p %p\n", ir, swiz);
   *rvalue = swiz->clone(ralloc_parent(swiz), NULL);
}

// ===================================================================================
// ===================================================================================

class ir_coalesce_floats_visitor : public ir_hierarchical_visitor {
public:
   int tmpcount;
   ir_variable *newvar, *basevar;
   ir_variable *floatvars[4];
   int swizzle;
   hash_table *promotions;
   hash_table *firstUse, *lastUse;

   ir_coalesce_floats_visitor()
   {
      basevar = NULL;
      newvar = NULL;
      firstUse = lastUse = NULL;
      tmpcount = 0;
      for(int i=0;i<4;i++) floatvars[i] = NULL;
      promotions = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   virtual ~ir_coalesce_floats_visitor()
   {
      /* empty */
   }

   void handleDereference(ir_dereference *d);

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit_enter(ir_assignment *);

   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
};

void ir_coalesce_floats_visitor::handleDereference(ir_dereference *dr)
{
   ir_variable *v = dr->variable_referenced();
   if(!v) abort();

   if(hash_table_find(lastUse, v) == dr) {
      for(int i=0;i<4;i++) {
         if(floatvars[i] == v) {
            floatvars[i] = NULL;
           //fprintf(stderr, "remove use of float %s %d\n", v->name, i);
      }
      }
   }
}

ir_visitor_status ir_coalesce_floats_visitor::visit(ir_dereference_variable *d)
{
   handleDereference(d);
   return visit_continue;
}

ir_visitor_status ir_coalesce_floats_visitor::visit_enter(ir_dereference_array *d)
{
   handleDereference(d);
   return visit_continue;
}

ir_visitor_status ir_coalesce_floats_visitor::visit_enter(ir_assignment *ir)
{
   ir_dereference *dr = ir->lhs->as_dereference();
   handleDereference(dr);

   ir_swizzle *swiz = (ir_swizzle*)hash_table_find(promotions, dr->variable_referenced());
   if (!swiz) return visit_continue;

   //fprintf(stderr, "// replacing assign %p %p\n", ir, swiz);
   ir->set_lhs(swiz->clone(ralloc_parent(swiz), NULL));

   return visit_continue;
}

ir_visitor_status ir_coalesce_floats_visitor::visit(ir_variable *v)
{
   if(!basevar) basevar = v;

   if(!v) abort();

   void *ctx = ralloc_parent(v);
   if(v->type == glsl_type::float_type && (v->mode == ir_var_temporary || v->mode == ir_var_auto)) {

      //fprintf(stderr, "found var: %p %s\n", v, v->name);

      if(!(floatvars[0] || floatvars[1] || floatvars[2] || floatvars[3]) ||
         (floatvars[0] && floatvars[1] && floatvars[2] && floatvars[3])) {
         newvar = NULL;
         //fprintf(stderr, "force new var %p %p %p %p\n", floatvars[0], floatvars[1], floatvars[2], floatvars[3]);
      }

     if(!newvar) {
         for(int i=0;i<4;i++) floatvars[i] = NULL;
         char nm[512];
         sprintf(&nm[0], "coalesced_float_%d", tmpcount++);
         newvar = new (ctx) ir_variable(glsl_type::vec4_type, ralloc_strdup(ctx, &nm[0]), ir_var_temporary, glsl_precision_undefined);
         basevar->insert_after(newvar);
         basevar = newvar;
         //fprintf(stderr, "created coalesced temp var %p\n", newvar);
     }

     int swizzle = -1;
     for(int i=0;i<4;i++) {
      if (floatvars[i] == NULL) {
         floatvars[i] = v;
         swizzle = i;
         break;
      }
      }
      if(swizzle == -1) abort();

     ir_dereference_variable *dv = new(ctx) ir_dereference_variable(newvar);
     const char swizvals[] = "xyzw";
     char nm[2] = { swizvals[swizzle], 0 };
     ir_swizzle *swiz = ir_swizzle::create(dv, &nm[0], 4);
     if(!swiz) abort();

      hash_table_insert(promotions, swiz, v);
      //fprintf(stderr, "coalesced float %s %d\n", v->name, v->mode);
      v->remove();
   }
   return visit_continue;
}

bool
do_coalesce_floats(exec_list *instructions)
{
   ir_float_liverange_visitor lrv;
   lrv.run(instructions);

   //fprintf(stderr, " --- coalesce floats ---- \n");
   ir_coalesce_floats_visitor v;
   v.lastUse = lrv.lastUse;
   v.firstUse = lrv.firstUse;
   v.run(instructions);

   //fprintf(stderr, " --- coalesce floats ---- \n");
   ir_coalesce_floats_replacing_visitor r(v.promotions);
   r.run(instructions);
   return true;
}