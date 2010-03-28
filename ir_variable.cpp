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

#include "glsl_parser_extras.h"
#include "glsl_symbol_table.h"
#include "ir.h"
#include "builtin_variables.h"

#ifndef Elements
#define Elements(x) (sizeof(x)/sizeof(*(x)))
#endif

static void
add_builtin_variable(const builtin_variable *proto, exec_list *instructions,
		     glsl_symbol_table *symtab)
{
   /* Create a new variable declaration from the description supplied by
    * the caller.
    */
   const glsl_type *const type = symtab->get_type(proto->type);

   assert(type != NULL);

   ir_variable *var = new ir_variable(type, proto->name);

   var->mode = proto->mode;
   if (var->mode != ir_var_out)
      var->read_only = true;


   /* Once the variable is created an initialized, add it to the symbol table
    * and add the declaration to the IR stream.
    */
   instructions->push_tail(var);

   symtab->add_variable(var->name, var);
}

static void
generate_110_uniforms(exec_list *instructions,
		      glsl_symbol_table *symtab)
{
   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_uniforms)
	   ; i++) {
      add_builtin_variable(& builtin_110_deprecated_uniforms[i],
			   instructions, symtab);
   }

   /* FINISHME: Add support for gl_TextureMatrix[].  The size of this array is
    * FINISHME: implementation dependent based on the value of
    * FINISHME: GL_MAX_TEXTURE_COORDS.
    */

   /* FINISHME: Add support for gl_DepthRangeParameters */
   /* FINISHME: Add support for gl_ClipPlane[] */
   /* FINISHME: Add support for gl_PointParameters */

   /* FINISHME: Add support for gl_MaterialParameters
    * FINISHME: (glFrontMaterial, glBackMaterial)
    */

   /* FINISHME: Add support for gl_LightSource[] */
   /* FINISHME: Add support for gl_LightModel */
   /* FINISHME: Add support for gl_FrontLightProduct[], gl_BackLightProduct[] */
   /* FINISHME: Add support for gl_TextureEnvColor[] */
   /* FINISHME: Add support for gl_ObjectPlane*[], gl_EyePlane*[] */
   /* FINISHME: Add support for gl_Fog */
}

static void
generate_110_vs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   for (unsigned i = 0; i < Elements(builtin_core_vs_variables); i++) {
      add_builtin_variable(& builtin_core_vs_variables[i],
			   instructions, symtab);
   }

   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_vs_variables)
	   ; i++) {
      add_builtin_variable(& builtin_110_deprecated_vs_variables[i],
			   instructions, symtab);
   }
   generate_110_uniforms(instructions, symtab);

   /* FINISHME: Add support fo gl_TexCoord.  The size of this array is
    * FINISHME: implementation dependent based on the value of
    * FINISHME: GL_MAX_TEXTURE_COORDS.
    */
}


static void
generate_120_vs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   /* GLSL version 1.20 did not add any built-in variables in the vertex
    * shader.
    */
   generate_110_vs_variables(instructions, symtab);
}


static void
generate_130_vs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   generate_120_vs_variables(instructions, symtab);

   for (unsigned i = 0; i < Elements(builtin_130_vs_variables); i++) {
      add_builtin_variable(& builtin_130_vs_variables[i],
			   instructions, symtab);
   }

   /* FINISHME: Add support fo gl_ClipDistance.  The size of this array is
    * FINISHME: implementation dependent based on the value of
    * FINISHME: GL_MAX_CLIP_DISTANCES.
    */
}


static void
initialize_vs_variables(exec_list *instructions,
			struct _mesa_glsl_parse_state *state)
{

   switch (state->language_version) {
   case 110:
      generate_110_vs_variables(instructions, state->symbols);
      break;
   case 120:
      generate_120_vs_variables(instructions, state->symbols);
      break;
   case 130:
      generate_130_vs_variables(instructions, state->symbols);
      break;
   }
}

static void
generate_110_fs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   for (unsigned i = 0; i < Elements(builtin_core_fs_variables); i++) {
      add_builtin_variable(& builtin_core_fs_variables[i],
			   instructions, symtab);
   }

   /* FINISHME: Add support for gl_TexCoord[] */
   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_fs_variables)
	   ; i++) {
      add_builtin_variable(& builtin_110_deprecated_fs_variables[i],
			   instructions, symtab);
   }
   generate_110_uniforms(instructions, symtab);

   /* FINISHME: Add support for gl_FragData[GL_MAX_DRAW_BUFFERS]. */
}

static void
generate_120_fs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   /* GLSL version 1.20 did not add any built-in variables in the fragment
    * shader.
    */
   generate_110_fs_variables(instructions, symtab);
}

static void
generate_130_fs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   generate_120_fs_variables(instructions, symtab);

   /* FINISHME: Add support fo gl_ClipDistance.  The size of this array is
    * FINISHME: implementation dependent based on the value of
    * FINISHME: GL_MAX_CLIP_DISTANCES.
    */
}

static void
initialize_fs_variables(exec_list *instructions,
			struct _mesa_glsl_parse_state *state)
{

   switch (state->language_version) {
   case 110:
      generate_110_fs_variables(instructions, state->symbols);
      break;
   case 120:
      generate_120_fs_variables(instructions, state->symbols);
      break;
   case 130:
      generate_130_fs_variables(instructions, state->symbols);
      break;
   }
}

void
_mesa_glsl_initialize_variables(exec_list *instructions,
				struct _mesa_glsl_parse_state *state)
{
   switch (state->target) {
   case vertex_shader:
      initialize_vs_variables(instructions, state);
      break;
   case geometry_shader:
      break;
   case fragment_shader:
      initialize_fs_variables(instructions, state);
      break;
   }
}
