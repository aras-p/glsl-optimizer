/*
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <GL/gl.h>
#include "main/mtypes.h"

/**
 * Based on gl_shader in Mesa's mtypes.h.
 */
struct glsl_shader {
   GLenum Type;
   GLuint Name;
   GLint RefCount;
   GLboolean DeletePending;
   GLboolean CompileStatus;
   const GLchar *Source;  /**< Source code string */
   size_t SourceLen;
   GLchar *InfoLog;

   struct exec_list ir;
   struct glsl_symbol_table *symbols;
};


typedef int gl_state_index;
#define STATE_LENGTH 5

/**
 * Program parameter.
 * Used by shaders/programs for uniforms, constants, varying vars, etc.
 */
struct gl_program_parameter
{
   const char *Name;        /**< Null-terminated string */
   gl_register_file Type;   /**< PROGRAM_NAMED_PARAM, CONSTANT or STATE_VAR */
   GLenum DataType;         /**< GL_FLOAT, GL_FLOAT_VEC2, etc */
   /**
    * Number of components (1..4), or more.
    * If the number of components is greater than 4,
    * this parameter is part of a larger uniform like a GLSL matrix or array.
    * The next program parameter's Size will be Size-4 of this parameter.
    */
   GLuint Size;
   GLboolean Used;          /**< Helper flag for GLSL uniform tracking */
   GLboolean Initialized;   /**< Has the ParameterValue[] been set? */
   GLbitfield Flags;        /**< Bitmask of PROG_PARAM_*_BIT */
   /**
    * A sequence of STATE_* tokens and integers to identify GL state.
    */
   gl_state_index StateIndexes[STATE_LENGTH];
};


/**
 * List of gl_program_parameter instances.
 */
struct gl_program_parameter_list
{
   GLuint Size;           /**< allocated size of Parameters, ParameterValues */
   GLuint NumParameters;  /**< number of parameters in arrays */
   struct gl_program_parameter *Parameters; /**< Array [Size] */
   GLfloat (*ParameterValues)[4];        /**< Array [Size] of GLfloat[4] */
   GLbitfield StateFlags; /**< _NEW_* flags indicating which state changes
                               might invalidate ParameterValues[] */
};


/**
 * Shader program uniform variable.
 * The glGetUniformLocation() and glUniform() commands will use this
 * information.
 * Note that a uniform such as "binormal" might be used in both the
 * vertex shader and the fragment shader.  When glUniform() is called to
 * set the uniform's value, it must be updated in both the vertex and
 * fragment shaders.  The uniform may be in different locations in the
 * two shaders so we keep track of that here.
 */
struct gl_uniform
{
   const char *Name;        /**< Null-terminated string */
   GLint VertPos;
   GLint FragPos;
   GLboolean Initialized;   /**< For debug.  Has this uniform been set? */
#if 0
   GLenum DataType;         /**< GL_FLOAT, GL_FLOAT_VEC2, etc */
   GLuint Size;             /**< Number of components (1..4) */
#endif
};


/**
 * List of gl_uniforms
 */
struct gl_uniform_list
{
   GLuint Size;                 /**< allocated size of Uniforms array */
   GLuint NumUniforms;          /**< number of uniforms in the array */
   struct gl_uniform *Uniforms; /**< Array [Size] */
};


/**
 * Based on gl_shader_program in Mesa's mtypes.h.
 */
struct glsl_program {
   GLenum Type;  /**< Always GL_SHADER_PROGRAM (internal token) */
   GLuint Name;  /**< aka handle or ID */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;

   GLuint NumShaders;          /**< number of attached shaders */
   struct glsl_shader **Shaders; /**< List of attached the shaders */

   /**
    * Per-stage shaders resulting from the first stage of linking.
    */
   /*@{*/
   unsigned _NumLinkedShaders;
   struct glsl_shader **_LinkedShaders;
   /*@}*/

   /** User-defined attribute bindings (glBindAttribLocation) */
   struct gl_program_parameter_list *Attributes;

   /* post-link info: */
   struct gl_uniform_list *Uniforms;
   struct gl_program_parameter_list *Varying;
   GLboolean LinkStatus;   /**< GL_LINK_STATUS */
   GLboolean Validated;
   GLboolean _Used;        /**< Ever used for drawing? */
   GLchar *InfoLog;
};

extern void
link_shaders(struct glsl_program *prog);
