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

/**
 * \file shader_query.cpp
 * C-to-C++ bridge functions to query GLSL shader data
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "main/core.h"
#include "glsl_symbol_table.h"
#include "ir.h"
#include "shaderobj.h"
#include "program/hash_table.h"
#include "../glsl/program.h"

extern "C" {
#include "shaderapi.h"
}

void GLAPIENTRY
_mesa_BindAttribLocation(GLhandleARB program, GLuint index,
                            const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glBindAttribLocation");
   if (!shProg)
      return;

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindAttribLocation(illegal name)");
      return;
   }

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(index)");
      return;
   }

   /* Replace the current value if it's already in the list.  Add
    * VERT_ATTRIB_GENERIC0 because that's how the linker differentiates
    * between built-in attributes and user-defined attributes.
    */
   shProg->AttributeBindings->put(index + VERT_ATTRIB_GENERIC0, name);

   /*
    * Note that this attribute binding won't go into effect until
    * glLinkProgram is called again.
    */
}

static bool
is_active_attrib(const ir_variable *var)
{
   if (!var)
      return false;

   switch (var->data.mode) {
   case ir_var_shader_in:
      return var->data.location != -1;

   case ir_var_system_value:
      /* From GL 4.3 core spec, section 11.1.1 (Vertex Attributes):
       * "For GetActiveAttrib, all active vertex shader input variables
       * are enumerated, including the special built-in inputs gl_VertexID
       * and gl_InstanceID."
       */
      return var->data.location == SYSTEM_VALUE_VERTEX_ID ||
             var->data.location == SYSTEM_VALUE_VERTEX_ID_ZERO_BASE ||
             var->data.location == SYSTEM_VALUE_INSTANCE_ID;

   default:
      return false;
   }
}

void GLAPIENTRY
_mesa_GetActiveAttrib(GLhandleARB program, GLuint desired_index,
                         GLsizei maxLength, GLsizei * length, GLint * size,
                         GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
      return;

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetActiveAttrib(program not linked)");
      return;
   }

   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(no vertex shader)");
      return;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   unsigned current_index = 0;

   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      if (!is_active_attrib(var))
         continue;

      if (current_index == desired_index) {
         const char *var_name = var->name;

         /* Since gl_VertexID may be lowered to gl_VertexIDMESA, we need to
          * consider gl_VertexIDMESA as gl_VertexID for purposes of checking
          * active attributes.
          */
         if (var->data.mode == ir_var_system_value &&
             var->data.location == SYSTEM_VALUE_VERTEX_ID_ZERO_BASE) {
            var_name = "gl_VertexID";
         }

	 _mesa_copy_string(name, maxLength, length, var_name);

	 if (size)
	    *size = (var->type->is_array()) ? var->type->length : 1;

	 if (type)
	    *type = var->type->gl_type;

	 return;
      }

      current_index++;
   }

   /* If the loop did not return early, the caller must have asked for
    * an index that did not exit.  Set an error.
    */
   _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
}

/* Locations associated with shader variables (array or non-array) can be
 * queried using its base name or using the base name appended with the
 * valid array index. For example, in case of below vertex shader, valid
 * queries can be made to know the location of "xyz", "array", "array[0]",
 * "array[1]", "array[2]" and "array[3]". In this example index reurned
 * will be 0, 0, 0, 1, 2, 3 respectively.
 *
 * [Vertex Shader]
 * layout(location=0) in vec4 xyz;
 * layout(location=1) in vec4[4] array;
 * void main()
 * { }
 *
 * This requirement came up with the addition of ARB_program_interface_query
 * to OpenGL 4.3 specification. See page 101 (page 122 of the PDF) for details.
 *
 * This utility function is used by:
 * _mesa_GetAttribLocation
 * _mesa_GetFragDataLocation
 * _mesa_GetFragDataIndex
 *
 * Returns 0:
 *    if the 'name' string matches var->name.
 * Returns 'matched index':
 *    if the 'name' string matches var->name appended with valid array index.
 */
int static inline
get_matching_index(const ir_variable *const var, const char *name) {
   unsigned idx = 0;
   const char *const paren = strchr(name, '[');
   const unsigned len = (paren != NULL) ? paren - name : strlen(name);

   if (paren != NULL) {
      if (!var->type->is_array())
         return -1;

      char *endptr;
      idx = (unsigned) strtol(paren + 1, &endptr, 10);
      const unsigned idx_len = endptr != (paren + 1) ? endptr - paren - 1 : 0;

      /* Validate the sub string representing index in 'name' string */
      if ((idx > 0 && paren[1] == '0') /* leading zeroes */
          || (idx == 0 && idx_len > 1) /* all zeroes */
          || paren[1] == ' ' /* whitespace */
          || endptr[0] != ']' /* closing brace */
          || endptr[1] != '\0' /* null char */
          || idx_len == 0 /* missing index */
          || idx >= var->type->length) /* exceeding array bound */
         return -1;
   }

   if (strncmp(var->name, name, len) == 0 && var->name[len] == '\0')
      return idx;

   return -1;
}

GLint GLAPIENTRY
_mesa_GetAttribLocation(GLhandleARB program, const GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetAttribLocation");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetAttribLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   /* Not having a vertex shader is not an error.
    */
   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL)
      return -1;

   exec_list *ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      /* The extra check against VERT_ATTRIB_GENERIC0 is because
       * glGetAttribLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
	  || var->data.mode != ir_var_shader_in
	  || var->data.location == -1
	  || var->data.location < VERT_ATTRIB_GENERIC0)
	 continue;

      int index = get_matching_index(var, (const char *) name);

      if (index >= 0)
         return var->data.location + index - VERT_ATTRIB_GENERIC0;
   }

   return -1;
}


unsigned
_mesa_count_active_attribs(struct gl_shader_program *shProg)
{
   if (!shProg->LinkStatus
       || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      return 0;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   unsigned i = 0;

   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      if (!is_active_attrib(var))
         continue;

      i++;
   }

   return i;
}


size_t
_mesa_longest_attribute_name_length(struct gl_shader_program *shProg)
{
   if (!shProg->LinkStatus
       || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      return 0;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   size_t longest = 0;

   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      if (var == NULL
	  || var->data.mode != ir_var_shader_in
	  || var->data.location == -1)
	 continue;

      const size_t len = strlen(var->name);
      if (len >= longest)
	 longest = len + 1;
   }

   return longest;
}

void GLAPIENTRY
_mesa_BindFragDataLocation(GLuint program, GLuint colorNumber,
			   const GLchar *name)
{
   _mesa_BindFragDataLocationIndexed(program, colorNumber, 0, name);
}

void GLAPIENTRY
_mesa_BindFragDataLocationIndexed(GLuint program, GLuint colorNumber,
                                  GLuint index, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glBindFragDataLocationIndexed");
   if (!shProg)
      return;

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBindFragDataLocationIndexed(illegal name)");
      return;
   }

   if (index > 1) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(index)");
      return;
   }

   if (index == 0 && colorNumber >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(colorNumber)");
      return;
   }

   if (index == 1 && colorNumber >= ctx->Const.MaxDualSourceDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(colorNumber)");
      return;
   }

   /* Replace the current value if it's already in the list.  Add
    * FRAG_RESULT_DATA0 because that's how the linker differentiates
    * between built-in attributes and user-defined attributes.
    */
   shProg->FragDataBindings->put(colorNumber + FRAG_RESULT_DATA0, name);
   shProg->FragDataIndexBindings->put(index, name);
   /*
    * Note that this binding won't go into effect until
    * glLinkProgram is called again.
    */

}

GLint GLAPIENTRY
_mesa_GetFragDataIndex(GLuint program, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetFragDataIndex");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataIndex(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataIndex(illegal name)");
      return -1;
   }

   /* Not having a fragment shader is not an error.
    */
   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL)
      return -1;

   exec_list *ir = shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->ir;
   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      /* The extra check against FRAG_RESULT_DATA0 is because
       * glGetFragDataLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
          || var->data.mode != ir_var_shader_out
          || var->data.location == -1
          || var->data.location < FRAG_RESULT_DATA0)
         continue;

      if (get_matching_index(var, (const char *) name) >= 0)
         return var->data.index;
   }

   return -1;
}

GLint GLAPIENTRY
_mesa_GetFragDataLocation(GLuint program, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetFragDataLocation");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataLocation(illegal name)");
      return -1;
   }

   /* Not having a fragment shader is not an error.
    */
   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL)
      return -1;

   exec_list *ir = shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->ir;
   foreach_in_list(ir_instruction, node, ir) {
      const ir_variable *const var = node->as_variable();

      /* The extra check against FRAG_RESULT_DATA0 is because
       * glGetFragDataLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
	  || var->data.mode != ir_var_shader_out
	  || var->data.location == -1
	  || var->data.location < FRAG_RESULT_DATA0)
	 continue;

      int index = get_matching_index(var, (const char *) name);

      if (index >= 0)
         return var->data.location + index - FRAG_RESULT_DATA0;
   }

   return -1;
}
