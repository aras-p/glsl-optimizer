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
#include "replaceInstruction.h"
#include <vector>

// ===================================================================================
 // ===================================================================================

class ir_liverange_visitor : public ir_hierarchical_visitor {
public:
   hash_table *firstUse, *lastUse;

   ir_liverange_visitor()
   {
      firstUse = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
      lastUse = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   ir_visitor_status handleDereference(ir_dereference *);
};

ir_visitor_status ir_liverange_visitor::visit(ir_dereference_variable *d)
{
   return handleDereference(d);
}

ir_visitor_status ir_liverange_visitor::visit_enter(ir_dereference_array *d)
{
   return handleDereference(d);
}

ir_visitor_status ir_liverange_visitor::handleDereference(ir_dereference *d)
{
   ir_variable *var = d->variable_referenced();

   if(!var)
      abort();

   if(!hash_table_find(firstUse, var)) {
      hash_table_insert(firstUse, d, var);
     // fprintf(stderr, "=== ir_dereference_variable firstUse %p %p : %s ===\n", var, d, var->name);
   }

   if(hash_table_find(lastUse, var))
      hash_table_remove(lastUse, var);
   hash_table_insert(lastUse, d, var);
   //fprintf(stderr, "=== ir_dereference_variable lastUse %p %p : %s ===\n", var, d, var->name);

   return visit_continue;
}

// ===================================================================================
// ===================================================================================

class ir_coalesce_temps_visitor : public ir_hierarchical_visitor {
public:
   static const int nAgalTemps = 8;
   static const int nAgalTempElements = 4;
   
   hash_table *firstUse, *lastUse, *replaceVars;

   short tempSlotsUsage[nAgalTemps];
   ir_variable* tempSlotVarAssignments[nAgalTemps][4];
   ir_variable* deadVars[nAgalTemps];

   ir_coalesce_temps_visitor()
   {
      replaceVars = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);

      memset(&tempSlotsUsage[0], 0, sizeof(tempSlotsUsage));
      memset(&deadVars[0], 0, sizeof(deadVars));
      memset(&tempSlotVarAssignments[0][0], 0, sizeof(tempSlotVarAssignments));
   }

   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   ir_visitor_status handleDereference(ir_dereference *);
};
ir_visitor_status
ir_coalesce_temps_visitor::visit(ir_dereference_variable *ir)
{
   return handleDereference(ir);
}
ir_visitor_status
ir_coalesce_temps_visitor::visit_enter(ir_dereference_array *ir)
{
   return handleDereference(ir);
}

ir_visitor_status
ir_coalesce_temps_visitor::handleDereference(ir_dereference *ir)
{
   ir_variable *var = ir->variable_referenced();

   if(!(var->mode == ir_var_auto || var->mode == ir_var_temporary))
      return visit_continue;

   //fprintf(stderr, "=== ir_dereference_variable %p %s ===\n", var, var->name);

   if(hash_table_find(firstUse, var) == ir) {
      //fprintf(stderr, "Variable %s defined\n", var->name);
      bool allocated = false;
      for(int i=0; i<nAgalTemps; i++) {
         if(tempSlotsUsage[i] == 0) {
            tempSlotsUsage[i] = var->component_slots();
            tempSlotVarAssignments[i][0] = var;
            allocated = true;
            break;
         }
      }

      for(int i=0; i<nAgalTemps; i++) {
         if(deadVars[i] != NULL) {
            //fprintf(stderr, "Variable %s sharing %s\n", var->name, deadVars[i]->name);
            hash_table_insert(replaceVars, deadVars[i], var);
            deadVars[i] = NULL;
            break;
         }
      }

      //if(!allocated)
      //   fprintf(stderr, "Variable %s NOT ALLOCATED!!!!\n", var->name);
   }
      

   if(hash_table_find(lastUse, var) == ir) {
      //fprintf(stderr, "Variable %s killed\n", var->name);
      bool allocated = false;

      for(int i=0; i<nAgalTemps; i++) {
         if(tempSlotVarAssignments[i][0] == var) {
            tempSlotsUsage[i] = 0;
            tempSlotVarAssignments[i][0] = NULL;
            allocated = true;
            break;
         }
      }

      ir_variable *replacement = (ir_variable*)hash_table_find(replaceVars, var);
      for(int i=0; i<nAgalTemps; i++) {
         if(deadVars[i] == NULL) {
            deadVars[i] = replacement ? replacement : var;
            break;
         }
      }
      //if(!allocated)
      //   fprintf(stderr, "Variable %s NOT ALLOCATED!!!!\n", var->name);
   }
   
   return visit_continue;
}


// ===================================================================================
// ===================================================================================

class ir_variable_replace_visitor : public ir_hierarchical_visitor {
public:
   hash_table *replaceVars;
   ir_variable *firstTmp;
   ir_instruction *varbase;
   std::vector<ir_variable*> tmps;
   bool inFunction;

   ir_variable_replace_visitor()
   {
       firstTmp = NULL;
       inFunction = false;
   }

   ~ir_variable_replace_visitor()
   {
       for(int i=0; i<tmps.size(); i++){
         firstTmp->insert_after(tmps[i]);
       }
   }

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_enter(ir_function *);
   virtual ir_visitor_status visit_leave(ir_function *);
};

ir_visitor_status
ir_variable_replace_visitor::visit_enter(ir_function *v)
{
   inFunction = true;
   return visit_continue;
}

ir_visitor_status
ir_variable_replace_visitor::visit_leave(ir_function *v)
{
   inFunction = false;
   return visit_continue;
}

ir_visitor_status
ir_variable_replace_visitor::visit(ir_variable *v)
{
   if(!firstTmp && inFunction && !hash_table_find(replaceVars, v)) {
      firstTmp = v;
      varbase = base_ir;
      //fprintf(stderr, "=== first Tmp %s ===\n", v->name);
   }

   if(hash_table_find(replaceVars, v)) {
      //fprintf(stderr, "=== remove %s ===\n", v->name);
      base_ir->remove();
   } else if(firstTmp && firstTmp != v) {
      //tmps.push_back(v);
   }
   return visit_continue;
}

ir_visitor_status
ir_variable_replace_visitor::visit_enter(ir_dereference_array *da)
{
   ir_dereference_variable *dv = da->array->as_dereference_variable();
   if(!dv)
      abort();
   ir_variable *replacement = (ir_variable*)hash_table_find(replaceVars, dv->var);
   if(replacement) {
      //fprintf(stderr, "=== replace %s with %s ===\n", dv->var->name, replacement->name);
      dv->var = replacement;
   }
   return visit_continue;
}

ir_visitor_status
ir_variable_replace_visitor::visit(ir_dereference_variable *dv)
{
   ir_variable *replacement = (ir_variable*)hash_table_find(replaceVars, dv->var);
   if(replacement) {
      //fprintf(stderr, "=== replace %s with %s ===\n", dv->var->name, replacement->name);
      dv->var = replacement;
   }
   return visit_continue;
}

bool
do_coalesce_temps(exec_list *instructions)
{
   //fprintf(stderr, "=== do_coalesce_temps 1 ===\n");

   ir_liverange_visitor lrv;
   lrv.run(instructions);

   //fprintf(stderr, "=== do_coalesce_temps 2 ===\n");

   ir_coalesce_temps_visitor v;
   v.lastUse = lrv.lastUse;
   v.firstUse = lrv.firstUse;
   v.run(instructions);

   //fprintf(stderr, "=== do_coalesce_temps 3 ===\n");

   ir_variable_replace_visitor rv;
   rv.replaceVars = v.replaceVars;
   rv.run(instructions);

   return false;
}