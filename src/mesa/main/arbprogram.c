/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/**
 * \file arbprogram.c
 * \brief ARB_vertex/fragment_program state management functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "arbprogram.h"



void
_mesa_VertexAttrib1sARB(GLuint index, GLshort x)
{
}

void
_mesa_VertexAttrib1fARB(GLuint index, GLfloat x)
{
}

void
_mesa_VertexAttrib1dARB(GLuint index, GLdouble x)
{
}

void
_mesa_VertexAttrib2sARB(GLuint index, GLshort x, GLshort y)
{
}

void
_mesa_VertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y)
{
}

void
_mesa_VertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y)
{
}

void
_mesa_VertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z)
{
}

void
_mesa_VertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
}

void
_mesa_VertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
}

void
_mesa_VertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
}

void
_mesa_VertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
}

void
_mesa_VertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
}

void
_mesa_VertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
}

void
_mesa_VertexAttrib1svARB(GLuint index, const GLshort *v)
{
}

void
_mesa_VertexAttrib1fvARB(GLuint index, const GLfloat *v)
{
}

void
_mesa_VertexAttrib1dvARB(GLuint index, const GLdouble *v)
{
}

void
_mesa_VertexAttrib2svARB(GLuint index, const GLshort *v)
{
}

void
_mesa_VertexAttrib2fvARB(GLuint index, const GLfloat *v)
{
}

void
_mesa_VertexAttrib2dvARB(GLuint index, const GLdouble *v)
{
}

void
_mesa_VertexAttrib3svARB(GLuint index, const GLshort *v)
{
}

void
_mesa_VertexAttrib3fvARB(GLuint index, const GLfloat *v)
{
}

void
_mesa_VertexAttrib3dvARB(GLuint index, const GLdouble *v)
{
}

void
_mesa_VertexAttrib4bvARB(GLuint index, const GLbyte *v)
{
}

void
_mesa_VertexAttrib4svARB(GLuint index, const GLshort *v)
{
}

void
_mesa_VertexAttrib4ivARB(GLuint index, const GLint *v)
{
}

void
_mesa_VertexAttrib4ubvARB(GLuint index, const GLubyte *v)
{
}

void
_mesa_VertexAttrib4usvARB(GLuint index, const GLushort *v)
{
}

void
_mesa_VertexAttrib4uivARB(GLuint index, const GLuint *v)
{
}

void
_mesa_VertexAttrib4fvARB(GLuint index, const GLfloat *v)
{
}

void
_mesa_VertexAttrib4dvARB(GLuint index, const GLdouble *v)
{
}

void
_mesa_VertexAttrib4NbvARB(GLuint index, const GLbyte *v)
{
}

void
_mesa_VertexAttrib4NsvARB(GLuint index, const GLshort *v)
{
}

void
_mesa_VertexAttrib4NivARB(GLuint index, const GLint *v)
{
}

void
_mesa_VertexAttrib4NubvARB(GLuint index, const GLubyte *v)
{
}

void
_mesa_VertexAttrib4NusvARB(GLuint index, const GLushort *v)
{
}

void
_mesa_VertexAttrib4NuivARB(GLuint index, const GLuint *v)
{
}


void
_mesa_VertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                             GLboolean normalized, GLsizei stride,
                             const GLvoid *pointer)
{
}


void
_mesa_EnableVertexAttribArrayARB(GLuint index)
{
}


void
_mesa_DisableVertexAttribArrayARB(GLuint index)
{
}


void
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params)
{
}


void
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params)
{
}


void
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params)
{
}


void
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer)
{
}


void
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string)
{
}


void
_mesa_BindProgramARB(GLenum target, GLuint program)
{
}


void
_mesa_DeleteProgramsARB(GLsizei n, const GLuint *programs)
{
}


void
_mesa_GenProgramsARB(GLsizei n, GLuint *programs)
{
}


void
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
}


void
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                   const GLdouble *params)
{
}


void
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
}


void
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                   const GLfloat *params)
{
}


#if 111

void
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_FRAGMENT_PROGRAM_NV) {
      struct fragment_program *fprog = ctx->FragmentProgram.Current;
      if (!fprog) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
         return;
      }
      if (index >= MAX_NV_FRAGMENT_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      fprog->Base.LocalParams[index][0] = x;
      fprog->Base.LocalParams[index][1] = y;
      fprog->Base.LocalParams[index][2] = z;
      fprog->Base.LocalParams[index][3] = w;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
      return;
   }
}


void
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params)
{
   glProgramLocalParameter4fARB(target, index, params[0], params[1],
                                params[2], params[3]);
}


void
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w)
{
   glProgramLocalParameter4fARB(target, index, (GLfloat)x, (GLfloat)y, 
		   			       (GLfloat)z, (GLfloat)w);
}


void
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params)
{
   glProgramLocalParameter4fARB(target, index, (GLfloat)params[0], 
		   		(GLfloat)params[1], (GLfloat)params[2],
				(GLfloat)params[3]);
}


void
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index,
                                    GLfloat *params)
{
   struct program *prog;
   GLuint maxParams;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
      maxParams = ctx->Const.MaxVertexProgramParams;
   }
   else if ((target == GL_FRAGMENT_PROGRAM_ARB
             && ctx->Extensions.ARB_fragment_program) ||
            (target == GL_FRAGMENT_PROGRAM_NV
             && ctx->Extensions.NV_fragment_program)) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = ctx->Const.MaxFragmentProgramParams;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetProgramLocalParameterARB(target)");
      return;
   }

   if (index >= maxParams) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetProgramLocalParameterARB(index)");
      return;
   }

   ASSERT(prog);
   params[0] = prog->LocalParams[index][0];
   params[1] = prog->LocalParams[index][1];
   params[2] = prog->LocalParams[index][2];
   params[3] = prog->LocalParams[index][3];
}


void
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params)
{
   GLfloat floatParams[4];
   glGetProgramLocalParameterfvARB(target, index, floatParams);
   COPY_4V(params, floatParams);
}

#else


void
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w)
{
}


void
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params)
{
}


void
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
}


void
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params)
{
}

#endif

void
_mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                  GLdouble *params)
{
}


void
_mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index, 
                                  GLfloat *params)
{
}


#if 000
void
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params)
{
}


void
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index, 
                                    GLfloat *params)
{
}
#endif



void
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB) {
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
      return;
   }

   ASSERT(prog);

   switch (pname) {
      case GL_PROGRAM_LENGTH_ARB:
         *params = prog->String ? _mesa_strlen((char *) prog->String) : 0;
         break;
      case GL_PROGRAM_FORMAT_ARB:
         *params = prog->Format;
         break;
      case GL_PROGRAM_BINDING_ARB:
         *params = prog->Id;
         break;
      case GL_PROGRAM_INSTRUCTIONS_ARB:
         *params = prog->NumInstructions;
         break;
      case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
      case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
      case GL_PROGRAM_TEMPORARIES_ARB:
      case GL_MAX_PROGRAM_TEMPORARIES_ARB:
      case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
      case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
      case GL_PROGRAM_PARAMETERS_ARB:
      case GL_MAX_PROGRAM_PARAMETERS_ARB:
      case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
      case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
      case GL_PROGRAM_ATTRIBS_ARB:
      case GL_MAX_PROGRAM_ATTRIBS_ARB:
      case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
      case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
      case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
      case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
      case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
      case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
      case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
      case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
      case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(pname)");
         return;
   }
}


void
_mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB) {
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(target)");
      return;
   }

   ASSERT(prog);

   if (pname != GL_PROGRAM_STRING_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(pname)");
      return;
   }

   MEMCPY(string, prog->String, _mesa_strlen((char *) prog->String));
}


GLboolean
_mesa_IsProgramARB(GLuint program)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (program == 0)
      return GL_FALSE;

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, program);
   if (prog && prog->Target)
      return GL_TRUE;
   else
      return GL_FALSE;
}
