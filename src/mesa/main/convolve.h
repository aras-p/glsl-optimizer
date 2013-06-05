
/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef CONVOLVE_H
#define CONVOLVE_H


#include "compiler.h"

struct _glapi_table;

void GLAPIENTRY
_mesa_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width,
                          GLenum format, GLenum type, const GLvoid *image);
void GLAPIENTRY
_mesa_ConvolutionFilter2D(GLenum target, GLenum internalFormat, GLsizei width,
                          GLsizei height, GLenum format, GLenum type,
                          const GLvoid *image);
void GLAPIENTRY
_mesa_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param);
void GLAPIENTRY
_mesa_ConvolutionParameterfv(GLenum target, GLenum pname,
                             const GLfloat *params);
void GLAPIENTRY
_mesa_ConvolutionParameteri(GLenum target, GLenum pname, GLint param);
void GLAPIENTRY
_mesa_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params);
void GLAPIENTRY
_mesa_CopyConvolutionFilter1D(GLenum target, GLenum internalFormat, GLint x,
                              GLint y, GLsizei width);
void GLAPIENTRY
_mesa_CopyConvolutionFilter2D(GLenum target, GLenum internalFormat, GLint x,
                              GLint y, GLsizei width, GLsizei height);
void GLAPIENTRY
_mesa_GetnConvolutionFilterARB(GLenum target, GLenum format, GLenum type,
                               GLsizei bufSize, GLvoid *image);
void GLAPIENTRY
_mesa_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
                           GLvoid *image);
void GLAPIENTRY
_mesa_GetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params);
void GLAPIENTRY
_mesa_GetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params);
void GLAPIENTRY
_mesa_GetnSeparableFilterARB(GLenum target, GLenum format, GLenum type,
                             GLsizei rowBufSize, GLvoid *row,
                             GLsizei columnBufSize,  GLvoid *column,
                             GLvoid *span);
void GLAPIENTRY
_mesa_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
                         GLvoid *row, GLvoid *column, GLvoid *span);
void GLAPIENTRY
_mesa_SeparableFilter2D(GLenum target, GLenum internalFormat, GLsizei width,
                        GLsizei height, GLenum format, GLenum type,
                        const GLvoid *row, const GLvoid *column);

#endif /* CONVOLVE_H */
