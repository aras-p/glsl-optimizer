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
#include "shader/prog_parameter.h"
#include "shader/prog_uniform.h"

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
