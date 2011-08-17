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

extern "C" {
#include "shaderapi.h"
}

void GLAPIENTRY
_mesa_BindAttribLocationARB(GLhandleARB program, GLuint index,
                            const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);

   const GLint size = -1; /* unknown size */
   GLint i;
   GLenum datatype = GL_FLOAT_VEC4;

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

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(index)");
      return;
   }

   /* this will replace the current value if it's already in the list */
   /* Add VERT_ATTRIB_GENERIC0 because that's how the linker differentiates
    * between built-in attributes and user-defined attributes.
    */
   shProg->AttributeBindings->put(index + VERT_ATTRIB_GENERIC0, name);
   i = _mesa_add_attribute(shProg->Attributes, name, size, datatype, index);
   if (i < 0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindAttribLocation");
      return;
   }

   /*
    * Note that this attribute binding won't go into effect until
    * glLinkProgram is called again.
    */
}

void GLAPIENTRY
_mesa_GetActiveAttribARB(GLhandleARB program, GLuint index,
                         GLsizei maxLength, GLsizei * length, GLint * size,
                         GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_program_parameter_list *attribs = NULL;
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
      return;

   if (shProg->VertexProgram)
      attribs = shProg->VertexProgram->Base.Attributes;

   if (!attribs || index >= attribs->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
      return;
   }

   _mesa_copy_string(name, maxLength, length,
                     attribs->Parameters[index].Name);

   if (size)
      *size = attribs->Parameters[index].Size
         / _mesa_sizeof_glsl_type(attribs->Parameters[index].DataType);

   if (type)
      *type = attribs->Parameters[index].DataType;
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB(GLhandleARB program, const GLcharARB * name)
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
   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      /* The extra check against VERT_ATTRIB_GENERIC0 is because
       * glGetAttribLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
	  || var->mode != ir_var_in
	  || var->location == -1
	  || var->location < VERT_ATTRIB_GENERIC0)
	 continue;

      if (strcmp(var->name, name) == 0)
	 return var->location - VERT_ATTRIB_GENERIC0;
   }

   return -1;
}
