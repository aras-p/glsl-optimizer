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
#include "nvprogram.h"
#include "arbprogram.h"


/* XXX temporary */
static void
_mesa_parse_arb_vertex_program(GLcontext *ctx, GLenum target,
                               const GLubyte *string, GLsizei len,
                               struct vertex_program *prog)
{
}


static void
_mesa_parse_arb_fragment_program(GLcontext *ctx, GLenum target,
                                 const GLubyte *string, GLsizei len,
                                 struct fragment_program *prog)
{
}



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
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   ctx->Array.VertexAttrib[index].Enabled = GL_TRUE;
   ctx->Array._Enabled |= _NEW_ARRAY_ATTRIB(index);
   ctx->Array.NewState |= _NEW_ARRAY_ATTRIB(index);
}


void
_mesa_DisableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   ctx->Array.VertexAttrib[index].Enabled = GL_FALSE;
   ctx->Array._Enabled &= ~_NEW_ARRAY_ATTRIB(index);
   ctx->Array.NewState &= ~_NEW_ARRAY_ATTRIB(index);
}


void
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params)
{
   GLfloat fparams[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_GetVertexAttribfvARB(index, pname, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
         COPY_4V(params, fparams);
      }
      else {
         params[0] = fparams[0];
      }
   }
}


void
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index == 0 || index >= VERT_ATTRIB_MAX) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribfvARB(index)");
      return;
   }

   switch (pname) {
      case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Enabled;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Size;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Stride;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Type;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Normalized;
         break;
      case GL_CURRENT_VERTEX_ATTRIB_ARB:
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribfvARB(pname)");
         return;
   }
}


void
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params)
{
   GLfloat fparams[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_GetVertexAttribfvARB(index, pname, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
         COPY_4V(params, fparams);  /* float to int */
      }
      else {
         params[0] = fparams[0];
      }
   }
}


void
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerARB(index)");
      return;
   }

   if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerARB(pname)");
      return;
   }

   *pointer = ctx->Array.VertexAttrib[index].Ptr;;
}


void
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      struct vertex_program *prog = ctx->VertexProgram.Current;
      if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
         return;
      }
      _mesa_parse_arb_vertex_program(ctx, target, string, len, prog);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      struct fragment_program *prog = ctx->FragmentProgram.Current;
      if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
         return;
      }
      _mesa_parse_arb_fragment_program(ctx, target, string, len, prog);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(target)");
   }
}


void
_mesa_BindProgramARB(GLenum target, GLuint program)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (ctx->VertexProgram.Current &&
          ctx->VertexProgram.Current->Base.Id == program)
         return;
      /* decrement refcount on previously bound vertex program */
      if (ctx->VertexProgram.Current) {
         ctx->VertexProgram.Current->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (ctx->VertexProgram.Current->Base.RefCount <= 0) {
            _mesa_delete_program(ctx, &(ctx->VertexProgram.Current->Base));
            _mesa_HashRemove(ctx->Shared->Programs, program);
         }
      }
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      if (ctx->FragmentProgram.Current &&
          ctx->FragmentProgram.Current->Base.Id == program)
         return;
      /* decrement refcount on previously bound fragment program */
      if (ctx->FragmentProgram.Current) {
         ctx->FragmentProgram.Current->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (ctx->FragmentProgram.Current->Base.RefCount <= 0) {
            _mesa_delete_program(ctx, &(ctx->FragmentProgram.Current->Base));
            _mesa_HashRemove(ctx->Shared->Programs, program);
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramARB(target)");
      return;
   }

   /* NOTE: binding to a non-existant program is not an error.
    * That's supposed to be caught in glBegin.
    * NOTE: program number 0 is legal here.
    */
   if (program == 0) {
      /* default program */
      if (target == GL_VERTEX_PROGRAM_ARB)
         prog = ctx->Shared->DefaultVertexProgram;
      else
         prog = ctx->Shared->DefaultFragmentProgram;
   }
   else {
      prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, program);
   }
   if (!prog && program > 0){
      /* allocate new program */
      prog = _mesa_alloc_program(ctx, target, program);
      if (!prog) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindProgramARB");
         return;
      }
      prog->Id = program;
      prog->Target = target;
      prog->Resident = GL_TRUE;
      prog->RefCount = 1;
      _mesa_HashInsert(ctx->Shared->Programs, program, prog);
   }

   /* bind now */
   if (target == GL_VERTEX_PROGRAM_ARB) {
      ctx->VertexProgram.Current = (struct vertex_program *) prog;
   }
   else {
      ASSERT(target == GL_FRAGMENT_PROGRAM_ARB);
      ctx->FragmentProgram.Current = (struct fragment_program *) prog;
   }

   if (prog)
      prog->RefCount++;
}


void
_mesa_DeleteProgramsARB(GLsizei n, const GLuint *programs)
{
   _mesa_DeleteProgramsNV(n, programs);
}


void
_mesa_GenProgramsARB(GLsizei n, GLuint *programs)
{
   _mesa_GenProgramsNV(n, programs);
}


GLboolean
_mesa_IsProgramARB(GLuint program)
{
   return _mesa_IsProgramNV(program);
}


void
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramEnvParameter4fARB(target, index, x, y, z, w);
}


void
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                const GLdouble *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, params[0], params[1],
                                  params[2], params[3]);
}


void
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if (index >= ctx->Const.MaxFragmentProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameter(index)");
         return;
      }
      index += MAX_NV_FRAGMENT_PROGRAM_TEMPS;  /* XXX fix */
      ASSIGN_4V(ctx->FragmentProgram.Machine.Registers[index], x, y, z, w);
   }
   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.MaxVertexProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameter(index)");
         return;
      }
      index += MAX_NV_VERTEX_PROGRAM_TEMPS;  /* XXX fix */
      ASSIGN_4V(ctx->VertexProgram.Machine.Registers[index], x, y, z, w);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramEnvParameter(target)");
      return;
   }
}


void
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                   const GLfloat *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, params[0], params[1],
                                  params[2], params[3]);
}


void
_mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                  GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat fparams[4];

   _mesa_GetProgramEnvParameterfvARB(target, index, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      params[0] = fparams[0];
      params[1] = fparams[1];
      params[2] = fparams[2];
      params[3] = fparams[3];
   }
}


void
_mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index, 
                                  GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if (index >= ctx->Const.MaxFragmentProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramEnvParameter(index)");
         return;
      }
      index += MAX_NV_FRAGMENT_PROGRAM_TEMPS;  /* XXX fix */
      COPY_4V(params, ctx->FragmentProgram.Machine.Registers[index]);
   }
   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.MaxVertexProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramEnvParameter(index)");
         return;
      }
      index += MAX_NV_VERTEX_PROGRAM_TEMPS;  /* XXX fix */
      COPY_4V(params, ctx->VertexProgram.Machine.Registers[index]);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramEnvParameter(target)");
      return;
   }
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if ((target == GL_FRAGMENT_PROGRAM_NV
        && ctx->Extensions.NV_fragment_program) ||
       (target == GL_FRAGMENT_PROGRAM_ARB
        && ctx->Extensions.ARB_fragment_program)) {
      struct fragment_program *fprog = ctx->FragmentProgram.Current;
      if (!fprog) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
         return;
      }
      if (index >= ctx->Const.MaxFragmentProgramLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      fprog->Base.LocalParams[index][0] = x;
      fprog->Base.LocalParams[index][1] = y;
      fprog->Base.LocalParams[index][2] = z;
      fprog->Base.LocalParams[index][3] = w;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
            && ctx->Extensions.ARB_vertex_program) {
      struct vertex_program *vprog = ctx->VertexProgram.Current;
      if (!vprog) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
         return;
      }
      if (index >= ctx->Const.MaxVertexProgramLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      vprog->Base.LocalParams[index][0] = x;
      vprog->Base.LocalParams[index][1] = y;
      vprog->Base.LocalParams[index][2] = z;
      vprog->Base.LocalParams[index][3] = w;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
      return;
   }
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params)
{
   _mesa_ProgramLocalParameter4fARB(target, index, params[0], params[1],
                                    params[2], params[3]);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w)
{
   _mesa_ProgramLocalParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, 
                                    (GLfloat) z, (GLfloat) w);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params)
{
   _mesa_ProgramLocalParameter4fARB(target, index,
                                    (GLfloat) params[0], (GLfloat) params[1],
                                    (GLfloat) params[2], (GLfloat) params[3]);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index,
                                    GLfloat *params)
{
   const struct program *prog;
   GLuint maxParams;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
      maxParams = ctx->Const.MaxVertexProgramLocalParams;
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = ctx->Const.MaxFragmentProgramLocalParams;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV
            && ctx->Extensions.NV_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
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
   COPY_4V(params, prog->LocalParams[index]);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat floatParams[4];
   _mesa_GetProgramLocalParameterfvARB(target, index, floatParams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      COPY_4V(params, floatParams);
   }
}


void
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_vertex_program) {
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
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramInstructions;
         else
            *params = ctx->Const.MaxFragmentProgramInstructions;
         break;
      case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         *params = prog->NumInstructions;
         break;
      case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramInstructions;
         else
            *params = ctx->Const.MaxFragmentProgramInstructions;
         break;
      case GL_PROGRAM_TEMPORARIES_ARB:
         *params = prog->NumTemporaries;
         break;
      case GL_MAX_PROGRAM_TEMPORARIES_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramTemps;
         else
            *params = ctx->Const.MaxFragmentProgramTemps;
         break;
      case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
         /* XXX same as GL_PROGRAM_TEMPORARIES_ARB? */
         *params = prog->NumTemporaries;
         break;
      case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
         /* XXX same as GL_MAX_PROGRAM_TEMPORARIES_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramTemps;
         else
            *params = ctx->Const.MaxFragmentProgramTemps;
         break;
      case GL_PROGRAM_PARAMETERS_ARB:
         *params = prog->NumParameters;
         break;
      case GL_MAX_PROGRAM_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
         *params = prog->NumParameters;
         break;
      case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_PROGRAM_ATTRIBS_ARB:
         *params = prog->NumAttributes;
         break;
      case GL_MAX_PROGRAM_ATTRIBS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAttribs;
         else
            *params = ctx->Const.MaxFragmentProgramAttribs;
         break;
      case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
         /* XXX same as GL_PROGRAM_ATTRIBS_ARB? */
         *params = prog->NumAttributes;
         break;
      case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
         /* XXX same as GL_MAX_PROGRAM_ATTRIBS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAttribs;
         else
            *params = ctx->Const.MaxFragmentProgramAttribs;
         break;
      case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
         *params = prog->NumAddressRegs;
         break;
      case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAddressRegs;
         else
            *params = ctx->Const.MaxFragmentProgramAddressRegs;
         break;
      case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         /* XXX same as GL_PROGRAM_ADDRESS_REGISTERS_ARB? */
         *params = prog->NumAddressRegs;
         break;
      case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAddressRegs;
         else
            *params = ctx->Const.MaxFragmentProgramAddressRegs;
         break;
      case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramEnvParams;
         else
            *params = ctx->Const.MaxFragmentProgramEnvParams;
         break;
      case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
         /* XXX ok? */
         *params = GL_TRUE;
         break;

      /*
       * The following apply to fragment programs only.
       */
      case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
      case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumAluInstructions;
         break;
      case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
      case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumTexInstructions;
         break;
      case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
      case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumTexIndirections;
         break;
      case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramAluInstructions;
         break;
      case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramTexInstructions;
         break;
      case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramTexIndirections;
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
