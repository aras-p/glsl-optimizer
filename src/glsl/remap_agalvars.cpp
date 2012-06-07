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
#include <list>
#include <string>

class ir_remap_agalvars_visitor : public ir_hierarchical_visitor {
public:
   static const bool renameVars = true;

   bool useNewSyntax;

   hash_table *varhash, *renamedvars, *oldnames;
   ir_instruction *varbase;
   int ccount;

   int num_remapped_vs_builtins;
   ir_variable **remapped_vs_builtins;

   int num_remapped_fs_builtins;
   ir_variable **remapped_fs_builtins;

   PrintGlslMode mode;

   int nVA, nC, nT, nV, nFS;

   std::list<std::string> varyingNames;
   bool namesAreSorted;

   ir_remap_agalvars_visitor(PrintGlslMode _mode, hash_table *_oldnames)
   {
      oldnames = _oldnames;
      varhash = hash_table_ctor(0, hash_table_string_hash, hash_table_string_compare);
      renamedvars = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
      varbase = 0;
      mode = _mode;
      nVA = nC = nT = nV = nFS = 0;
      useNewSyntax = false;
      memset(&agalName[0], 0, sizeof(agalName));
      namesAreSorted = false;
   }

   char agalName[512];
   void computeAgalName(ir_variable *var);
   int computeStableAgalVaryingName(ir_variable *ir);

   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit(ir_variable *);
   void handleDeref(ir_variable **);
};

ir_visitor_status
ir_remap_agalvars_visitor::visit(ir_variable *ir)
{
   if(!varbase) {
      varbase = base_ir;
   }

   if(strcmp(ir->name, "gl_FragData") == 0 || strcmp(ir->name, "gl_FragColor") == 0 || strcmp(ir->name, "gl_Position") == 0) {
      // ignore
   } else if(this->mode == kPrintGlslVertex) {
      if(ir->mode == ir_var_out)
         varyingNames.push_back(ir->name);
   } else if(this->mode == kPrintGlslFragment) {
      if(ir->mode == ir_var_in)
         varyingNames.push_back(ir->name);
   }

   return visit_continue;
}

int ir_remap_agalvars_visitor::computeStableAgalVaryingName(ir_variable *ir)
{
   if(!namesAreSorted)
      varyingNames.sort();

   int n=0;
   for (std::list<std::string>::iterator it=varyingNames.begin(); it!=varyingNames.end(); ++it) {
      if((*it).find(ir->name) == 0)
         return n;
      n++;
   }
}

void ir_remap_agalvars_visitor::computeAgalName(ir_variable *ir)
{
   const char *oldname = (const char*)hash_table_find(oldnames, ir->name);
   if(hash_table_find(oldnames, ir->name)) {
      strcpy(&agalName[0], oldname);
      return;
   }

   // parent must be allocated before any synthetic children
   if(ir->parent)
      computeAgalName(ir->parent);

   agalName[0] = 0;

   if(!renameVars) {
      agalName[0] = '_';
      strcpy(&agalName[1], ir->name);
      return;
   }
   int slots = ((3+ir->component_slots()) & ~0x3) / 4;

   if(ir->type->base_type == GLSL_TYPE_SAMPLER)
      slots = 1;

   if(slots == 0) {
      int minsz = ir->max_array_access;
      //fprintf(stderr, "zero sized var: %s (needs at least %d)\n", ir->name, minsz);
      slots = minsz > 0 ? minsz : 1;
   }

   if(ir->parent) {
      // must be a synthetic child of an array
      ir->location = ir->parent->location + (ir->location * slots);

      //fprintf(stderr, "remapping %s to %d\n", ir->name, ir->location);
   } else {
      switch(ir->mode) {
         case ir_var_auto:
         case ir_var_temporary:
            ir->location = this->nT;
            this->nT += slots;
            break;
         case ir_var_in:
            if(this->mode == kPrintGlslVertex) {
               ir->location = this->nVA;
               this->nVA += slots;
            } else {
               ir->location = computeStableAgalVaryingName(ir);
            }
            break;
         case ir_var_out:
            ir->location = computeStableAgalVaryingName(ir);
            break;
         case ir_var_uniform :
            if(this->mode == kPrintGlslVertex) {
               ir->location = this->nC;
               this->nC += slots;
            } else {
               if(ir->type->base_type == GLSL_TYPE_SAMPLER) {
                  ir->location = this->nFS;
                  this->nFS += slots;
               } else {
                  ir->location = this->nC;
                  this->nC += slots;
               }
            }
            break;
         default: fprintf(stderr, "unknown mode %d\n", ir->mode); abort();
      }
   }

   if(strcmp(ir->name, "gl_FragData") == 0 || strcmp(ir->name, "gl_FragColor") == 0) {
      sprintf(&agalName[0], useNewSyntax ? "fo0" : "oc");
   } else if(strcmp(ir->name, "gl_Position") == 0) {
      sprintf(&agalName[0], useNewSyntax ? "vo0" : "op");
   } else if(this->mode == kPrintGlslVertex) {
      switch(ir->mode) {
         case ir_var_auto:
         case ir_var_temporary: sprintf(&agalName[0], "vt%d", ir->location); break;
         case ir_var_in      :  sprintf(&agalName[0], "va%d", ir->location); break;
         case ir_var_out     :  sprintf(&agalName[0], useNewSyntax ? "vi%d" : "v%d", ir->location); break;
         case ir_var_uniform :  sprintf(&agalName[0], "vc%d", ir->location); break;
         default: fprintf(stderr, "unknown v mode %d\n", ir->mode); abort();
      }
   } else if(this->mode == kPrintGlslFragment) {
      switch(ir->mode) {
         case ir_var_auto:
         case ir_var_temporary: sprintf(&agalName[0], "ft%d", ir->location); break;
         case ir_var_in      :  sprintf(&agalName[0], useNewSyntax ? "vi%d" : "v%d", ir->location); break;
         case ir_var_out     :  sprintf(&agalName[0], useNewSyntax ? "fo%d" : "oc", ir->location); break;
         case ir_var_uniform:
            if(ir->type->base_type == GLSL_TYPE_SAMPLER) {
               sprintf(&agalName[0], "fs%d", ir->location);
            } else {
               sprintf(&agalName[0], "fc%d", ir->location);
            }

            break;
         default: fprintf(stderr, "unknown f mode %d\n", ir->mode);;
      }
   }

   hash_table_insert(oldnames, ralloc_strdup(ir, &agalName[0]), ralloc_strdup(ir, ir->name));
}

void ir_remap_agalvars_visitor::handleDeref(ir_variable **varloc)
{
   ir_variable *glvar = *varloc;
   ir_variable *agalvar = (ir_variable*)hash_table_find(varhash, glvar->name);

   if(agalvar) {
      *varloc = agalvar;
      //fprintf(stderr, "remapping deref -- %s\n", glvar->name);
   } else if(glvar->name[0] == 'g' && glvar->name[1] == 'l' && glvar->name[2] == '_') {
      computeAgalName(glvar);
      agalvar = new (glvar) ir_variable(glvar->type, ralloc_strdup(glvar, &agalName[0]), (ir_variable_mode)glvar->mode, (glsl_precision)glvar->precision);
      hash_table_insert(varhash, agalvar, ralloc_strdup(agalvar, glvar->name));
      hash_table_insert(varhash, agalvar, ralloc_strdup(agalvar, &agalName[0]));
      hash_table_insert(renamedvars, glvar, glvar);
      varbase->insert_before(agalvar);
      *varloc = agalvar;
      //fprintf(stderr, "renaming: %s -> %s\n", glvar->name, &agalName[0]);
   } else if (hash_table_find(renamedvars, glvar)) {
      // already renamed
   } else {
      computeAgalName(glvar);
      glvar->name = ralloc_strdup(glvar, agalName);
      hash_table_insert(renamedvars, glvar, glvar);
   }
}

ir_visitor_status
ir_remap_agalvars_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *glvar = ir->variable_referenced();
   ir_variable *agalvar = (ir_variable*)hash_table_find(varhash, glvar->name);
   handleDeref(&ir->var);

   return visit_continue;
}

ir_visitor_status
ir_remap_agalvars_visitor::visit_enter(ir_dereference_array *ir)
{
   ir_dereference_variable *dv = ir->array->as_dereference_variable();

   if(dv) {
      handleDeref(&dv->var);
   } else {
      fprintf(stderr, "REMAP DEREF ARRAY FAILED\n");
   }

   return visit_continue;
}

hash_table*
do_remap_agalvars(exec_list *instructions, int mode)
{
   hash_table *oldnames = hash_table_ctor(0, hash_table_string_hash, hash_table_string_compare);

   ir_remap_agalvars_visitor v((PrintGlslMode)mode, oldnames);
   v.run(instructions);

   return oldnames;
}
