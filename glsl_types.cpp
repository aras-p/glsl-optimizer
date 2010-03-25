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


const glsl_type *glsl_type::get_base_type() const
{
   switch (base_type) {
   case GLSL_TYPE_UINT:
      return glsl_uint_type;
   case GLSL_TYPE_INT:
      return glsl_int_type;
   case GLSL_TYPE_FLOAT:
      return glsl_float_type;
   case GLSL_TYPE_BOOL:
      return glsl_bool_type;
   default:
      return glsl_error_type;
   }
}


const glsl_type *
glsl_type::get_instance(unsigned base_type, unsigned rows, unsigned columns)
{
   if ((rows < 1) || (rows > 4) || (columns < 1) || (columns > 4))
      return glsl_error_type;


   /* Treat GLSL vectors as Nx1 matrices.
    */
   if (columns == 1) {
      switch (base_type) {
      case GLSL_TYPE_UINT:
	 return glsl_uint_type + (rows - 1);
      case GLSL_TYPE_INT:
	 return glsl_int_type + (rows - 1);
      case GLSL_TYPE_FLOAT:
	 return glsl_float_type + (rows - 1);
      case GLSL_TYPE_BOOL:
	 return glsl_bool_type + (rows - 1);
      default:
	 return glsl_error_type;
      }
   } else {
      if ((base_type != GLSL_TYPE_FLOAT) || (rows == 1))
	 return glsl_error_type;

      /* GLSL matrix types are named mat{COLUMNS}x{ROWS}.  Only the following
       * combinations are valid:
       *
       *   1 2 3 4
       * 1
       * 2   x x x
       * 3   x x x
       * 4   x x x
       */
#define IDX(c,r) (((c-1)*3) + (r-1))

      switch (IDX(columns, rows)) {
      case IDX(2,2): return mat2_type;
      case IDX(2,3): return mat2x3_type;
      case IDX(2,4): return mat2x4_type;
      case IDX(3,2): return mat3x2_type;
      case IDX(3,3): return mat3_type;
      case IDX(3,4): return mat3x4_type;
      case IDX(4,2): return mat4x2_type;
      case IDX(4,3): return mat4x3_type;
      case IDX(4,4): return mat4_type;
      default: return glsl_error_type;
      }
   }

   assert(!"Should not get here.");
   return glsl_error_type;
}
