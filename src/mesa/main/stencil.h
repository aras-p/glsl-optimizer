/**
 * \file stencil.h
 * Stencil operations.
 */

/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef STENCIL_H
#define STENCIL_H


#include "glheader.h"

struct gl_context;

extern void GLAPIENTRY
_mesa_ClearStencil( GLint s );


extern void GLAPIENTRY
_mesa_StencilFunc( GLenum func, GLint ref, GLuint mask );


extern void GLAPIENTRY
_mesa_StencilMask( GLuint mask );


extern void GLAPIENTRY
_mesa_StencilOp( GLenum fail, GLenum zfail, GLenum zpass );


extern void GLAPIENTRY
_mesa_ActiveStencilFaceEXT(GLenum face);


extern void GLAPIENTRY
_mesa_StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);


extern void GLAPIENTRY
_mesa_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);


extern void GLAPIENTRY
_mesa_StencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

extern void GLAPIENTRY
_mesa_StencilMaskSeparate(GLenum face, GLuint mask);


extern void
_mesa_update_stencil(struct gl_context *ctx);


extern void 
_mesa_init_stencil( struct gl_context * ctx );

/* From the GL 4.3 spec, 17.3.5:
 *    "Stencil comparison operations and queries of <ref> clamp its value
 *    to the range [0, 2^s-1], where <s> is the number of bits in the
 *    stencil buffer attached to the draw framebuffer."
 */

static inline GLint
_mesa_get_stencil_ref(struct gl_context const *ctx, int face)
{
   GLint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   GLint ref = ctx->Stencil.Ref[face];
   return CLAMP(ref, 0, stencilMax);
}

#endif
