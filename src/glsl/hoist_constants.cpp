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

/**
 * Hoist all constants out of the program and replace with uniforms
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "program/hash_table.h"
#include "replaceInstruction.h"

class ir_constant_hoisting_visitor : public ir_hierarchical_visitor {
public:
   static const int agalConstLimit = 128;

   float cpool[agalConstLimit*4];
   bool cpoolused[agalConstLimit*4];
   ir_variable *constvars[agalConstLimit];
   int cpoollen, tmpcount;
   ir_instruction *varbase;

   ir_constant_hoisting_visitor()
   {
      memset(&cpool, 0, sizeof(cpool));
      memset(&cpoolused, 0, sizeof(cpoolused));
      memset(&constvars, 0, sizeof(constvars));
      cpoollen = agalConstLimit*4;
      varbase = 0;
      tmpcount = 0;
   }

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit(ir_constant *);
};

ir_visitor_status
ir_constant_hoisting_visitor::visit(ir_variable *ir)
{
   if(!varbase)
      varbase = base_ir;
   return visit_continue;
}

ir_visitor_status
ir_constant_hoisting_visitor::visit(ir_constant *c)
{
   void *ctx = ralloc_parent(c);

   //fprintf(stderr, "const: %f\n", c->get_float_component(0));

   if(c->type == glsl_type::float_type || c->type == glsl_type::int_type || c->type == glsl_type::uint_type || c->type == glsl_type::bool_type) {
      bool allocated = false;
      for (int i=0; i<cpoollen; i++) {
         int v = i/4;
         float val = 0;
         if(c->type == glsl_type::int_type) val = c->get_int_component(0);
         else if(c->type == glsl_type::uint_type) val = c->get_uint_component(0);
         else if (c->type == glsl_type::float_type) val = c->get_float_component(0);
         else if (c->type == glsl_type::bool_type) val = c->get_bool_component(0);

         if(!constvars[v]) {
            char nm[512];
            sprintf(&nm[0], "hoisted_const_%d", tmpcount++);
            constvars[v] = new(ctx) ir_variable(glsl_type::vec4_type, ralloc_strdup(ctx, &nm[0]), ir_var_uniform, glsl_precision_undefined);
            constvars[v]->constant_value = ir_constant::zero(ctx, glsl_type::vec4_type);
            varbase->insert_before(constvars[v]);
         }

         if (!cpoolused[i]) {
            cpoolused[i] = true;
            cpool[i] = val;
         }

         if (cpoolused[i] && cpool[i] == val) {
            char swiz[5] = {0, 0, 0, 0, 0};
            swiz[0] = "xyzw"[i%4];
            //fprintf(stderr, "Hoisted const %f --> %s.%s (%p replaces %p)\n", c->get_float_component(0), constvars[v]->name, &swiz[0], constvars[v], c);
            ir_swizzle *newswiz = ir_swizzle::create(new (ctx) ir_dereference_variable(constvars[v]), &swiz[0], 4);
            constvars[v]->constant_value->value.f[i%4] = val;
            
            if(!newswiz) abort();
            if(!replaceInstruction(base_ir, c, newswiz)) {
               fprintf(stderr, "UNABLE TO REPLACE CONST\n");
            }
            
            allocated = true;
            break;
         }
      }
      if(!allocated)
         abort();
   } else if(c->type == glsl_type::vec2_type || c->type == glsl_type::vec3_type || c->type == glsl_type::vec4_type) {
      int nc = 0;
      if(c->type == glsl_type::vec2_type) nc = 2;
      if(c->type == glsl_type::vec3_type) nc = 3;
      if(c->type == glsl_type::vec4_type) nc = 4;

      bool allocated = false;
      for (int i=0; i<cpoollen; i+=4) {
         int v = i/4;
         if(!constvars[v]) {
            char nm[512];
            sprintf(&nm[0], "hoisted_const_%d", tmpcount++);
            constvars[v] = new(ctx) ir_variable(glsl_type::vec4_type, ralloc_strdup(ctx, &nm[0]), ir_var_uniform, glsl_precision_undefined);
            constvars[v]->constant_value = ir_constant::zero(ctx, glsl_type::vec4_type);
            varbase->insert_before(constvars[v]);
         }

         bool fit = true;
         for(int u=i; u<i+4; u++) {
            if(cpoolused[u] && cpool[u] != c->get_float_component(u-i)) { fit = false; }
         }
         if(!fit) {
            continue;
         }
         for(int u=i; u<i+4; u++) {
            cpoolused[u] = true;
            cpool[u] = c->get_float_component(u-i);
         }

         char swiz[5] = "xyzw";
         swiz[nc] = 0;
         ir_swizzle *newswiz = ir_swizzle::create(new (ctx) ir_dereference_variable(constvars[v]), &swiz[0], 4);

         /*switch(nc) {
            case 2: fprintf(stderr, "Hoisted vec2<%f,%f> --> %s.%s (%p replaces %p)\n", c->get_float_component(0),c->get_float_component(1), constvars[v]->name, &swiz[0], constvars[v], c); break;
            case 3: fprintf(stderr, "Hoisted vec3<%f,%f,%f> --> %s.%s (%p replaces %p)\n", c->get_float_component(0),c->get_float_component(1),c->get_float_component(2), constvars[v]->name, &swiz[0], constvars[v], c); break;
            case 4: fprintf(stderr, "Hoisted vec4<%f,%f,%f,%f> --> %s.%s (%p replaces %p)\n", c->get_float_component(0),c->get_float_component(1),c->get_float_component(2),c->get_float_component(3), constvars[v]->name, &swiz[0], constvars[v], c); break;
         }*/
         
         for(int u=0; u<nc; u++) {
            constvars[v]->constant_value->value.f[i%4 + u] = c->get_float_component(u);
         }
         
         if(!newswiz) abort();
         if(!replaceInstruction(base_ir, c, newswiz))
            fprintf(stderr, "UNABLE TO REPLACE CONST\n");
         
         allocated = true;
         break;
      }
      if(!allocated)
         abort();
   } else {
      fprintf(stderr, "CANT HOIST CONSTANT OF TYPE: %d (%s)\n", c->type, c->type->name);
   }
   
   return visit_continue;
}

bool
do_hoist_constants(exec_list *instructions)
{
   ir_constant_hoisting_visitor v;
   v.run(instructions);
   return false;
}