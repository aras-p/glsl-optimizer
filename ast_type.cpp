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
#include "symbol_table.h"

void
ast_type_specifier::print(void) const
{
   switch (type_specifier) {
   case ast_void: printf("void "); break;
   case ast_float: printf("float "); break;
   case ast_int: printf("int "); break;
   case ast_uint: printf("uint "); break;
   case ast_bool: printf("bool "); break;
   case ast_vec2: printf("vec2 "); break;
   case ast_vec3: printf("vec3 "); break;
   case ast_vec4: printf("vec4 "); break;
   case ast_bvec2: printf("bvec2 "); break;
   case ast_bvec3: printf("bvec3 "); break;
   case ast_bvec4: printf("bvec4 "); break;
   case ast_ivec2: printf("ivec2 "); break;
   case ast_ivec3: printf("ivec3 "); break;
   case ast_ivec4: printf("ivec4 "); break;
   case ast_uvec2: printf("uvec2 "); break;
   case ast_uvec3: printf("uvec3 "); break;
   case ast_uvec4: printf("uvec4 "); break;
   case ast_mat2: printf("mat2 "); break;
   case ast_mat2x3: printf("mat2x3 "); break;
   case ast_mat2x4: printf("mat2x4 "); break;
   case ast_mat3x2: printf("mat3x2 "); break;
   case ast_mat3: printf("mat3 "); break;
   case ast_mat3x4: printf("mat3x4 "); break;
   case ast_mat4x2: printf("mat4x2 "); break;
   case ast_mat4x3: printf("mat4x3 "); break;
   case ast_mat4: printf("mat4 "); break;
   case ast_sampler1d: printf("sampler1d "); break;
   case ast_sampler2d: printf("sampler2d "); break;
   case ast_sampler3d: printf("sampler3d "); break;
   case ast_samplercube: printf("samplercube "); break;
   case ast_sampler1dshadow: printf("sampler1dshadow "); break;
   case ast_sampler2dshadow: printf("sampler2dshadow "); break;
   case ast_samplercubeshadow: printf("samplercubeshadow "); break;
   case ast_sampler1darray: printf("sampler1darray "); break;
   case ast_sampler2darray: printf("sampler2darray "); break;
   case ast_sampler1darrayshadow: printf("sampler1darrayshadow "); break;
   case ast_sampler2darrayshadow: printf("sampler2darrayshadow "); break;
   case ast_isampler1d: printf("isampler1d "); break;
   case ast_isampler2d: printf("isampler2d "); break;
   case ast_isampler3d: printf("isampler3d "); break;
   case ast_isamplercube: printf("isamplercube "); break;
   case ast_isampler1darray: printf("isampler1darray "); break;
   case ast_isampler2darray: printf("isampler2darray "); break;
   case ast_usampler1d: printf("usampler1d "); break;
   case ast_usampler2d: printf("usampler2d "); break;
   case ast_usampler3d: printf("usampler3d "); break;
   case ast_usamplercube: printf("usamplercube "); break;
   case ast_usampler1darray: printf("usampler1darray "); break;
   case ast_usampler2darray: printf("usampler2darray "); break;

   case ast_struct:
      structure->print();
      break;

   case ast_type_name: printf("%s ", type_name); break;
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
{
   type_specifier = ast_types(specifier);
}
