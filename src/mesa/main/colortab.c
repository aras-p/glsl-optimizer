/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "bufferobj.h"
#include "colortab.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "state.h"
#include "teximage.h"
#include "texstate.h"
#include "main/dispatch.h"


void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalFormat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
}



void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
}



void GLAPIENTRY
_mesa_CopyColorTable(GLenum target, GLenum internalformat,
                     GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyColorTable(target)");
}



void GLAPIENTRY
_mesa_CopyColorSubTable(GLenum target, GLsizei start,
                        GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyColorSubTable(target)");
}



void GLAPIENTRY
_mesa_GetnColorTableARB( GLenum target, GLenum format, GLenum type,
                         GLsizei bufSize, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetnColorTableARB(target)");
}


void GLAPIENTRY
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
}


void GLAPIENTRY
_mesa_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   /* no extensions use this function */
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(target)");
}



void GLAPIENTRY
_mesa_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   /* no extensions use this function */
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameteriv(target)");
}



void GLAPIENTRY
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(target)");
}



void GLAPIENTRY
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(target)");
}
