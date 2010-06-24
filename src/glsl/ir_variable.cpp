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

#include "ir.h"
#include "glsl_parser_extras.h"
#include "glsl_symbol_table.h"
#include "builtin_variables.h"

#ifndef Elements
#define Elements(x) (sizeof(x)/sizeof(*(x)))
#endif

static ir_variable *
add_variable(const char *name, enum ir_variable_mode mode, int slot,
	     const glsl_type *type, exec_list *instructions,
		     glsl_symbol_table *symtab)
{
   ir_variable *var = new(symtab) ir_variable(type, name);

   var->mode = mode;
   switch (var->mode) {
   case ir_var_in:
      var->shader_in = true;
      var->read_only = true;
      break;
   case ir_var_inout:
      var->shader_in = true;
      var->shader_out = true;
      break;
   case ir_var_out:
      var->shader_out = true;
      break;
   case ir_var_uniform:
      var->shader_in = true;
      var->read_only = true;
      break;
   default:
      assert(0);
      break;
   }

   var->location = slot;

   /* Once the variable is created an initialized, add it to the symbol table
    * and add the declaration to the IR stream.
    */
   instructions->push_tail(var);

   symtab->add_variable(var->name, var);
   return var;
}


static void
add_builtin_variable(const builtin_variable *proto, exec_list *instructions,
		     glsl_symbol_table *symtab)
{
   /* Create a new variable declaration from the description supplied by
    * the caller.
    */
   const glsl_type *const type = symtab->get_type(proto->type);

   assert(type != NULL);

   add_variable(proto->name, proto->mode, proto->slot, type, instructions,
		symtab);
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

   /* FINISHME: The size of this array is implementation dependent based on the
    * FINISHME: value of GL_MAX_TEXTURE_COORDS.  Every platform that supports
    * FINISHME: GLSL sets GL_MAX_TEXTURE_COORDS to at least 4, so hard-code 4
    * FINISHME: for now.
    */
   const glsl_type *const mat4_array_type =
      glsl_type::get_array_instance(symtab, glsl_type::mat4_type, 4);

   add_variable("gl_TextureMatrix", ir_var_uniform, -1, mat4_array_type,
		instructions, symtab);

   /* FINISHME: Add support for gl_DepthRangeParameters */
   /* FINISHME: Add support for gl_ClipPlane[] */
   /* FINISHME: Add support for gl_PointParameters */

   /* FINISHME: Add support for gl_MaterialParameters
    * FINISHME: (glFrontMaterial, glBackMaterial)
    */

   /* FINISHME: The size of this array is implementation dependent based on the
    * FINISHME: value of GL_MAX_TEXTURE_LIGHTS.  GL_MAX_TEXTURE_LIGHTS must be
    * FINISHME: at least 8, so hard-code 8 for now.
    */
   const glsl_type *const light_source_array_type =
      glsl_type::get_array_instance(symtab,
				    symtab->get_type("gl_LightSourceParameters"), 8);

   add_variable("gl_LightSource", ir_var_uniform, -1, light_source_array_type,
		instructions, symtab);

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

   /* FINISHME: The size of this array is implementation dependent based on the
    * FINISHME: value of GL_MAX_TEXTURE_COORDS.  Every platform that supports
    * FINISHME: GLSL sets GL_MAX_TEXTURE_COORDS to at least 4, so hard-code 4
    * FINISHME: for now.
    */
   const glsl_type *const vec4_array_type =
      glsl_type::get_array_instance(symtab, glsl_type::vec4_type, 4);

   add_variable("gl_TexCoord", ir_var_out, VERT_RESULT_TEX0, vec4_array_type,
		instructions, symtab);
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
   void *ctx = symtab;
   generate_120_vs_variables(instructions, symtab);

   for (unsigned i = 0; i < Elements(builtin_130_vs_variables); i++) {
      add_builtin_variable(& builtin_130_vs_variables[i],
			   instructions, symtab);
   }

   /* FINISHME: The size of this array is implementation dependent based on
    * FINISHME: the value of GL_MAX_CLIP_DISTANCES.
    */
   const glsl_type *const clip_distance_array_type =
      glsl_type::get_array_instance(ctx, glsl_type::float_type, 8);

   /* FINISHME: gl_ClipDistance needs a real location assigned. */
   add_variable("gl_ClipDistance", ir_var_out, -1, clip_distance_array_type,
		instructions, symtab);

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

   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_fs_variables)
	   ; i++) {
      add_builtin_variable(& builtin_110_deprecated_fs_variables[i],
			   instructions, symtab);
   }
   generate_110_uniforms(instructions, symtab);

   /* FINISHME: The size of this array is implementation dependent based on the
    * FINISHME: value of GL_MAX_TEXTURE_COORDS.  Every platform that supports
    * FINISHME: GLSL sets GL_MAX_TEXTURE_COORDS to at least 4, so hard-code 4
    * FINISHME: for now.
    */
   const glsl_type *const vec4_array_type =
      glsl_type::get_array_instance(symtab, glsl_type::vec4_type, 4);

   add_variable("gl_TexCoord", ir_var_in, FRAG_ATTRIB_TEX0, vec4_array_type,
		instructions, symtab);
}


static void
generate_ARB_draw_buffers_fs_variables(exec_list *instructions,
				       glsl_symbol_table *symtab, bool warn)
{
   /* FINISHME: The size of this array is implementation dependent based on the
    * FINISHME: value of GL_MAX_DRAW_BUFFERS.  GL_MAX_DRAW_BUFFERS must be
    * FINISHME: at least 1, so hard-code 1 for now.
    */
   const glsl_type *const vec4_array_type =
      glsl_type::get_array_instance(symtab, glsl_type::vec4_type, 1);

   ir_variable *const fd =
      add_variable("gl_FragData", ir_var_out, FRAG_RESULT_DATA0,
		   vec4_array_type, instructions, symtab);

   if (warn)
      fd->warn_extension = "GL_ARB_draw_buffers";
}


static void
generate_120_fs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   generate_110_fs_variables(instructions, symtab);
   generate_ARB_draw_buffers_fs_variables(instructions, symtab, false);
}

static void
generate_130_fs_variables(exec_list *instructions,
			  glsl_symbol_table *symtab)
{
   void *ctx = symtab;
   generate_120_fs_variables(instructions, symtab);

   /* FINISHME: The size of this array is implementation dependent based on
    * FINISHME: the value of GL_MAX_CLIP_DISTANCES.
    */
   const glsl_type *const clip_distance_array_type =
      glsl_type::get_array_instance(ctx, glsl_type::float_type, 8);

   /* FINISHME: gl_ClipDistance needs a real location assigned. */
   add_variable("gl_ClipDistance", ir_var_in, -1, clip_distance_array_type,
		instructions, symtab);
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


   /* Since GL_ARB_draw_buffers is included in GLSL 1.20 and later, we
    * can basically ignore any extension settings for it.
    */
   if (state->language_version < 120) {
      if (state->ARB_draw_buffers_enable) {
	 generate_ARB_draw_buffers_fs_variables(instructions, state->symbols,
						state->ARB_draw_buffers_warn);
      }
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
   case ir_shader:
      fprintf(stderr, "ir reader has no builtin variables");
      exit(1);
      break;
   }
}
