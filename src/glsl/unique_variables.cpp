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

class ir_unique_agalvars_visitor : public ir_hierarchical_visitor {
public:
   int tmpcount;
   hash_table *varhash;
   
   ir_unique_agalvars_visitor()
   {
      tmpcount = 0;
      varhash = hash_table_ctor(0, hash_table_string_hash, hash_table_string_compare);
   }

   virtual ir_visitor_status visit(ir_variable *);
};

ir_visitor_status
ir_unique_agalvars_visitor::visit(ir_variable *ir)
{
   if(hash_table_find(varhash, ir->name)) {
      char nm[512];
      sprintf(&nm[0], "%s_%d", ir->name, tmpcount++);
      //fprintf(stderr, "uniquing %s --> %s\n", ir->name, &nm[0]);
      ir->name = (char*)ralloc_strdup(ir, &nm[0]);
   }

   hash_table_insert(varhash, (void*)0x1, ralloc_strdup(ir, ir->name));
   return visit_continue;
}

bool do_unique_variables(exec_list *instructions)
{
   //fprintf(stderr, "uniquing AGAL VARS ------------\n");
   ir_unique_agalvars_visitor v;
   v.run(instructions);

   return false;
}
