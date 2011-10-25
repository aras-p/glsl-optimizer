/*
 * Copyright © 2011 Intel Corporation
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

#include "main/core.h"
#include "ir.h"
#include "linker.h"
#include "glsl_symbol_table.h"
#include "program/hash_table.h"

/**
 * \file link_uniforms.cpp
 * Assign locations for GLSL uniforms.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

void
uniform_field_visitor::process(ir_variable *var)
{
   const glsl_type *t = var->type;

   /* Only strdup the name if we actually will need to modify it. */
   if (t->is_record() || (t->is_array() && t->fields.array->is_record())) {
      char *name = ralloc_strdup(NULL, var->name);
      recursion(var->type, &name, strlen(name));
      ralloc_free(name);
   } else {
      this->visit_field(t, var->name);
   }
}

void
uniform_field_visitor::recursion(const glsl_type *t, char **name,
				 unsigned name_length)
{
   /* Records need to have each field processed individually.
    *
    * Arrays of records need to have each array element processed
    * individually, then each field of the resulting array elements processed
    * individually.
    */
   if (t->is_record()) {
      for (unsigned i = 0; i < t->length; i++) {
	 const char *field = t->fields.structure[i].name;

	 /* Append '.field' to the current uniform name. */
	 ralloc_asprintf_rewrite_tail(name, name_length, ".%s", field);

	 recursion(t->fields.structure[i].type, name,
		   name_length + 1 + strlen(field));
      }
   } else if (t->is_array() && t->fields.array->is_record()) {
      for (unsigned i = 0; i < t->length; i++) {
	 char subscript[13];

	 /* Append the subscript to the current uniform name */
	 const unsigned subscript_length = snprintf(subscript, 13, "[%u]", i);
	 ralloc_asprintf_rewrite_tail(name, name_length, "%s", subscript);

	 recursion(t->fields.array, name, name_length + subscript_length);
      }
   } else {
      this->visit_field(t, *name);
   }
}
