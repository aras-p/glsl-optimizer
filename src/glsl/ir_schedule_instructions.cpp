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
#include "ir_print_glsl_visitor.h"
#include "glsl_types.h"
#include "replaceInstruction.h"
#include "program/hash_table.h"

// ===================================================================================
// ===================================================================================

class ir_compute_uses : public ir_rvalue_visitor {
public:
   hash_table *recentDep;
   ir_assignment *instr;

   _mesa_glsl_parse_state *state;
   int mode;

   ir_compute_uses(ir_assignment *_instr, hash_table *_recentDep, _mesa_glsl_parse_state *_state, int _mode)
   {
      recentDep = _recentDep;
      instr = _instr;
      state = _state;
      mode = _mode;
   }

  virtual void handle_rvalue(ir_rvalue **rv)
   {
      if(!*rv) return;

      ir_variable *var = digOutVar(*rv);
      if(!var) return;

      ir_instruction *def = (ir_instruction*)hash_table_find(recentDep, var);
      if(!def) return;

      if(!instr) abort();

      if(def == instr)
         return;

      def->users->insert(instr);
      instr->uses->insert(def);

      //char *buffer1 = _mesa_print_ir_glsl(instr, state, ralloc_strdup(def, ""), (PrintGlslMode)kPrintGlslNone);
      //char *buffer2 = _mesa_print_ir_glsl(def, state, ralloc_strdup(def, ""), (PrintGlslMode)kPrintGlslNone);

      //fprintf(stderr, "\"%s %p\" -> \"%s %p\"\n", buffer2, def, buffer1, instr);
      //fprintf(stderr, "// %p uses %s(%p) (%d)\n", instr, var->name, def, def->users->size());
   }
};

// ===================================================================================
// ===================================================================================

void reschedule_single_function(exec_list *body, _mesa_glsl_parse_state *state, int mode)
{
   exec_list tmp;
   body->move_nodes_to(&tmp);

   // Float variables to the top of the function
   foreach_list_safe(instr, &tmp) {
      ir_instruction *const stmt = (ir_instruction *)instr;
      if(stmt->as_variable())
      {
         stmt->remove();
         body->push_tail(stmt);
      }
   }

   // maps a variable --> most recent instruction defining it
   hash_table *definedBy = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);

   //fprintf(stderr, "strict digraph defusecahin {\n");

   foreach_list_safe(instr, &tmp) {
      
      ir_assignment *ass = ((ir_instruction *)instr)->as_assignment();
      if(!ass)
         abort();

      ir_variable *def = digOutVar(ass->lhs);



      // For any variable mentioned in ths rhs of this assignment
      // we must insert dependencies
      ass->uses = new std::set<ir_instruction*>();
      ass->users = new std::set<ir_instruction*>();
      ir_compute_uses uv(ass, definedBy, state, mode);
      ass->rhs->accept(&uv);
      //fprintf(stderr, "\n");

      // replace the current definition of this variable
      // with the current instruction
      
      ir_instruction *prevdef = (ir_instruction*)hash_table_find(definedBy, def);
      if(prevdef) {
         hash_table_remove(definedBy, def);

         /*for(std::set<ir_instruction*>::iterator it=prevdef->users->begin(); it != prevdef->users->end(); it++) {
            if(*it == ass) continue;

            ass->uses->insert(*it);
            (*it)->users->insert(ass);
         }*/

         ass->uses->insert(prevdef);
         prevdef->users->insert(ass);

         //char *buffer1 = _mesa_print_ir_glsl(ass, state, ralloc_strdup(ass, ""), (PrintGlslMode)kPrintGlslNone);
         //char *buffer2 = _mesa_print_ir_glsl(prevdef, state, ralloc_strdup(prevdef, ""), (PrintGlslMode)kPrintGlslNone);
         //fprintf(stderr, "\"%s %p\" -> \"%s %p\"\n", buffer2, prevdef, buffer1, ass);
      }

      hash_table_insert(definedBy, ass, def);

      

      //fprintf(stderr, "%p defined %s (%p)\n", ass, def->name, ass->uses->size());
   }
   //fprintf(stderr, "}\n");

   // Find all instructions with no dependencies
   exec_list roots;
   foreach_list_safe(instr, &tmp) {
      ir_assignment *ass = ((ir_instruction *)instr)->as_assignment();
      if(!ass) abort();

      ir_variable *def = digOutVar(ass->lhs);

      if(ass->uses->size() == 0)
      {
            ass->remove();
            roots.push_tail(ass);
            //fprintf(stderr, "root %p defines %s\n", ass, def->name);
      }
   }

   while(!roots.is_empty()) {
      ir_assignment *ass = (ir_assignment*)roots.pop_head();
      body->push_tail(ass);
      //fprintf(stderr, " scheduled %p (%s)\n", ass, digOutVar(ass->lhs)->name);

      //if(ass->uses->size() == 0) continue;

      //fprintf(stderr, " removing users of %p (%s) (%d)\n", ass, digOutVar(ass->lhs)->name, ass->users->size());
      while(!ass->users->empty()) {
         ir_instruction *user = *(ass->users->begin());
         ass->users->erase(user);
         user->uses->erase(ass);
         //fprintf(stderr, " removing user %s of %s (user users %d)\n", digOutVar(user->as_assignment()->lhs)->name, digOutVar(ass->lhs)->name, user->users->size());

         if(user->uses->size() == 0) {
            delete user->uses;
            user->uses = NULL;

            roots.push_head(user);
            //fprintf(stderr, "new root %p (%s)\n", user, digOutVar(user->as_assignment()->lhs)->name);
         }
      }

      delete ass->users;
      ass->users = NULL;
   }
}

exec_list* schedule_instructions(exec_list *instructions, _mesa_glsl_parse_state *state, int mode)
{
   foreach_list_safe(n, instructions) {
      ir_instruction *const stmt = (ir_instruction *) n;
      if(stmt->ir_type == ir_type_function) {
         //fprintf(stderr, "%s\n", stmt->as_function()->name);
         foreach_list_safe(s, &stmt->as_function()->signatures) {
            ir_function_signature *const sig = (ir_function_signature *) s;
            reschedule_single_function(&sig->body, state, mode);
            foreach_list_safe(f, &sig->body) {
               ir_instruction *const fstmt = (ir_instruction *) f;
               //fprintf(stderr, "-- %d\n", fstmt->ir_type);
            }
         }
      }
   }

   return NULL;
}