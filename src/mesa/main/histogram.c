/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "histogram.h"
#include "macros.h"
#include "main/dispatch.h"


/**********************************************************************
 * API functions
 */


void GLAPIENTRY
_mesa_GetnMinmaxARB(GLenum target, GLboolean reset, GLenum format,
                    GLenum type, GLsizei bufSize, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmax");
}


void GLAPIENTRY
_mesa_GetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type,
                GLvoid *values)
{
   _mesa_GetnMinmaxARB(target, reset, format, type, INT_MAX, values);
}


void GLAPIENTRY
_mesa_GetnHistogramARB(GLenum target, GLboolean reset, GLenum format,
                       GLenum type, GLsizei bufSize, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogram");
}


void GLAPIENTRY
_mesa_GetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type,
                   GLvoid *values)
{
   _mesa_GetnHistogramARB(target, reset, format, type, INT_MAX, values);
}


void GLAPIENTRY
_mesa_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameterfv");
}


void GLAPIENTRY
_mesa_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameteriv");
}


void GLAPIENTRY
_mesa_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameterfv");
}


void GLAPIENTRY
_mesa_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameteriv");
}


void GLAPIENTRY
_mesa_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glHistogram");
}


void GLAPIENTRY
_mesa_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glMinmax");
}


void GLAPIENTRY
_mesa_ResetHistogram(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glResetHistogram");
}


void GLAPIENTRY
_mesa_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glResetMinmax");
}
