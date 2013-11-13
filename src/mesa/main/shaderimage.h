/*
 * Copyright 2013 Intel Corporation
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
 *
 * Authors:
 *    Francisco Jerez <currojerez@riseup.net>
 */

#ifndef SHADERIMAGE_H
#define SHADERIMAGE_H

#include "glheader.h"

struct gl_context;

/**
 * Recalculate the \c _Valid flag of a context's shader image units.
 *
 * To be called when the state of any texture bound to an image unit
 * changes.
 */
void
_mesa_validate_image_units(struct gl_context *ctx);

void GLAPIENTRY
_mesa_BindImageTexture(GLuint unit, GLuint texture, GLint level,
                       GLboolean layered, GLint layer, GLenum access,
                       GLenum format);

void GLAPIENTRY
_mesa_BindImageTextures(GLuint first, GLsizei count, const GLuint *textures);

void GLAPIENTRY
_mesa_MemoryBarrier(GLbitfield barriers);

#endif
