/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

extern "C" {
#include <talloc.h>
}

#include "main/mtypes.h"
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "ir.h"
#include "program.h"
#include "hash_table.h"
#include "linker.h"

class call_link_visitor : public ir_hierarchical_visitor {
public:
   call_link_visitor(gl_shader_program *prog, gl_shader **shader_list,
		     unsigned num_shaders)
   {
      this->prog = prog;
      this->shader_list = shader_list;
      this->num_shaders = num_shaders;
      this->success = true;
   }

   virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      /* If the function call references a function signature that does not
       * have a definition, try to find the definition in one of the other
       * shaders.
       */
      ir_function_signature *callee =
	 const_cast<ir_function_signature *>(ir->get_callee());
      assert(callee != NULL);

      if (callee->is_defined)
	 /* FINISHME: Do children need to be processed, or are all parameters
	  * FINISHME: with function calls already flattend?
	  */
	 return visit_continue;

      const char *const name = callee->function_name();

      ir_function_signature *sig = const_cast<ir_function_signature *>
	 (this->find_matching_signature(name, &ir->actual_parameters));
      if (sig == NULL) {
	 /* FINISHME: Log the full signature of unresolved function.
	  */
	 linker_error_printf(this->prog, "unresolved reference to function "
			     "`%s'\n", name);
	 this->success = false;
	 return visit_stop;
      }

      /* Create an in-place clone of the function definition.  This multistep
       * process introduces some complexity here, but it has some advantages.
       * The parameter list and the and function body are cloned separately.
       * The clone of the parameter list is used to prime the hashtable used
       * to replace variable references in the cloned body.
       *
       * The big advantage is that the ir_function_signature does not change.
       * This means that we don't have to process the rest of the IR tree to
       * patch ir_call nodes.  In addition, there is no way to remove or replace
       * signature stored in a function.  One could easily be added, but this
       * avoids the need.
       */
      struct hash_table *ht = hash_table_ctor(0, hash_table_pointer_hash,
					      hash_table_pointer_compare);
      exec_list formal_parameters;
      foreach_list_const(node, &sig->parameters) {
	 const ir_instruction *const original = (ir_instruction *) node;
	 assert(const_cast<ir_instruction *>(original)->as_variable());

	 ir_instruction *copy = original->clone(ht);
	 formal_parameters.push_tail(copy);
      }

      callee->replace_parameters(&formal_parameters);

      assert(callee->body.is_empty());
      foreach_list_const(node, &sig->body) {
	 const ir_instruction *const original = (ir_instruction *) node;

	 ir_instruction *copy = original->clone(ht);
	 callee->body.push_tail(copy);
      }

      callee->is_defined = true;

      /* FINISHME: Patch references inside the function to things outside the
       * FINISHME: function (i.e., function calls and global variables).
       */

      hash_table_dtor(ht);

      return visit_continue;
   }

   /** Was function linking successful? */
   bool success;

private:
   /**
    * Shader program being linked
    *
    * This is only used for logging error messages.
    */
   gl_shader_program *prog;

   /** List of shaders available for linking. */
   gl_shader **shader_list;

   /** Number of shaders available for linking. */
   unsigned num_shaders;

   /**
    * Searches all shaders for a particular function definition
    */
   const ir_function_signature *
   find_matching_signature(const char *name, exec_list *actual_parameters)
   {
      for (unsigned i = 0; i < this->num_shaders; i++) {
	 ir_function *const f =
	    this->shader_list[i]->symbols->get_function(name);

	 if (f == NULL)
	    continue;

	 const ir_function_signature *sig =
	    f->matching_signature(actual_parameters);

	 if ((sig == NULL) || !sig->is_defined)
	    continue;

	 return sig;
      }

      return NULL;
   }
};


bool
link_function_calls(gl_shader_program *prog, gl_shader *main,
		    gl_shader **shader_list, unsigned num_shaders)
{
   call_link_visitor v(prog, shader_list, num_shaders);

   v.run(main->ir);
   return v.success;
}
