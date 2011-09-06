/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
 * Copyright © 2010, 2011 Intel Corporation
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
#include "main/core.h"
#include "ir.h"
#include "../glsl/program.h"

extern "C" {
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "uniforms.h"
}

extern "C" void GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB program, GLuint index,
                          GLsizei maxLength, GLsizei *length, GLint *size,
                          GLenum *type, GLcharARB *nameOut)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetActiveUniform");
   const struct gl_program_parameter *param;

   if (!shProg)
      return;

   if (!shProg->Uniforms || index >= shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   param = get_uniform_parameter(shProg, index);
   if (!param)
      return;

   if (nameOut) {
      _mesa_copy_string(nameOut, maxLength, length, param->Name);
   }

   if (size) {
      GLint typeSize = _mesa_sizeof_glsl_type(param->DataType);
      if ((GLint) param->Size > typeSize) {
         /* This is an array.
          * Array elements are placed on vector[4] boundaries so they're
          * a multiple of four floats.  We round typeSize up to next multiple
          * of four to get the right size below.
          */
         typeSize = (typeSize + 3) & ~3;
      }
      /* Note that the returned size is in units of the <type>, not bytes */
      *size = param->Size / typeSize;
   }

   if (type) {
      *type = param->DataType;
   }
}
