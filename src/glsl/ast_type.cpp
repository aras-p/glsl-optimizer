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

#include <cstdio>
#include "ast.h"
extern "C" {
#include "program/symbol_table.h"
}

void
ast_type_specifier::print(void) const
{
   if (type_specifier == ast_struct) {
      structure->print();
   } else {
      printf("%s ", type_name);
   }

   if (is_array) {
      printf("[ ");

      if (array_size) {
	 array_size->print();
      }

      printf("] ");
   }
}

ast_type_specifier::ast_type_specifier(int specifier)
      : type_specifier(ast_types(specifier)), type_name(NULL), structure(NULL),
	is_array(false), array_size(NULL), precision(ast_precision_high)
{
   static const char *const names[] = {
      "void",
      "float",
      "int",
      "uint",
      "bool",
      "vec2",
      "vec3",
      "vec4",
      "bvec2",
      "bvec3",
      "bvec4",
      "ivec2",
      "ivec3",
      "ivec4",
      "uvec2",
      "uvec3",
      "uvec4",
      "mat2",
      "mat2x3",
      "mat2x4",
      "mat3x2",
      "mat3",
      "mat3x4",
      "mat4x2",
      "mat4x3",
      "mat4",
      "sampler1D",
      "sampler2D",
      "sampler2DRect",
      "sampler3D",
      "samplerCube",
      "sampler1DShadow",
      "sampler2DShadow",
      "sampler2DRectShadow",
      "samplerCubeShadow",
      "sampler1DArray",
      "sampler2DArray",
      "sampler1DArrayShadow",
      "sampler2DArrayShadow",
      "isampler1D",
      "isampler2D",
      "isampler3D",
      "isamplerCube",
      "isampler1DArray",
      "isampler2DArray",
      "usampler1D",
      "usampler2D",
      "usampler3D",
      "usamplerCube",
      "usampler1DArray",
      "usampler2DArray",

      NULL, /* ast_struct */
      NULL  /* ast_type_name */
   };

   type_name = names[specifier];
}

bool
ast_fully_specified_type::has_qualifiers() const
{
   return this->qualifier.flags.i != 0;
}
