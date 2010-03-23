/*
 * Copyright © 2009 Intel Corporation
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

#include <stdlib.h>
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "builtin_types.h"


static void
add_types_to_symbol_table(glsl_symbol_table *symtab,
			  const struct glsl_type *types,
			  unsigned num_types)
{
   unsigned i;

   for (i = 0; i < num_types; i++) {
      symtab->add_type(types[i].name, & types[i]);
   }
}


static void
generate_110_types(glsl_symbol_table *symtab)
{
   add_types_to_symbol_table(symtab, builtin_core_types,
			     Elements(builtin_core_types));
   add_types_to_symbol_table(symtab, builtin_structure_types,
			     Elements(builtin_structure_types));
   add_types_to_symbol_table(symtab, builtin_110_deprecated_structure_types,
			     Elements(builtin_110_deprecated_structure_types));
   add_types_to_symbol_table(symtab, & void_type, 1);
}


static void
generate_120_types(glsl_symbol_table *symtab)
{
   generate_110_types(symtab);

   add_types_to_symbol_table(symtab, builtin_120_types,
			     Elements(builtin_120_types));
}


static void
generate_130_types(glsl_symbol_table *symtab)
{
   generate_120_types(symtab);

   add_types_to_symbol_table(symtab, builtin_130_types,
			     Elements(builtin_130_types));
}


void
_mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *state)
{
   switch (state->language_version) {
   case 110:
      generate_110_types(state->symbols);
      break;
   case 120:
      generate_120_types(state->symbols);
      break;
   case 130:
      generate_130_types(state->symbols);
      break;
   default:
      /* error */
      break;
   }
}


const struct glsl_type *
_mesa_glsl_get_vector_type(unsigned base_type, unsigned vector_length)
{
   switch (base_type) {
   case GLSL_TYPE_UINT:
      switch (vector_length) {
      case 1:
      case 2:
      case 3:
      case 4:
	 return glsl_uint_type + (vector_length - 1);
      default:
	 return glsl_error_type;
      }
   case GLSL_TYPE_INT:
      switch (vector_length) {
      case 1:
      case 2:
      case 3:
      case 4:
	 return glsl_int_type + (vector_length - 1);
      default:
	 return glsl_error_type;
      }
   case GLSL_TYPE_FLOAT:
      switch (vector_length) {
      case 1:
      case 2:
      case 3:
      case 4:
	 return glsl_float_type + (vector_length - 1);
      default:
	 return glsl_error_type;
      }
   case GLSL_TYPE_BOOL:
      switch (vector_length) {
      case 1:
      case 2:
      case 3:
      case 4:
	 return glsl_bool_type + (vector_length - 1);
      default:
	 return glsl_error_type;
      }
   default:
      return glsl_error_type;
   }
}
