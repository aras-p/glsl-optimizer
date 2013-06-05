/**
 * \file histogram.h
 * Histogram.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "compiler.h"

struct _glapi_table;

void GLAPIENTRY
_mesa_GetnMinmaxARB(GLenum target, GLboolean reset, GLenum format,
                    GLenum type, GLsizei bufSize, GLvoid *values);
void GLAPIENTRY
_mesa_GetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type,
                GLvoid *values);
void GLAPIENTRY
_mesa_GetnHistogramARB(GLenum target, GLboolean reset, GLenum format,
                       GLenum type, GLsizei bufSize, GLvoid *values);
void GLAPIENTRY
_mesa_GetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type,
                   GLvoid *values);
void GLAPIENTRY
_mesa_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params);
void GLAPIENTRY
_mesa_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params);
void GLAPIENTRY
_mesa_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params);
void GLAPIENTRY
_mesa_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params);
void GLAPIENTRY
_mesa_Histogram(GLenum target, GLsizei width, GLenum internalFormat,
                GLboolean sink);
void GLAPIENTRY
_mesa_Minmax(GLenum target, GLenum internalFormat, GLboolean sink);
void GLAPIENTRY
_mesa_ResetHistogram(GLenum target);
void GLAPIENTRY
_mesa_ResetMinmax(GLenum target);


#endif /* HISTOGRAM_H */
