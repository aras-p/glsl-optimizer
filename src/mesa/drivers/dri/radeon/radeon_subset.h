/**
 * \file radeon_subset.h
 * \brief Radeon subset driver declarations.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

/*
 * Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
 *                      Tungsten Grahpics Inc., Austin, Texas.
 * 
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * ATI, TUNGSTEN GRAHPICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* $XFree86$ */

#ifndef __RADEON_SUBSET_H__
#define __RADEON_SUBSET_H__

extern void radeonPointsBitmap( GLsizei width, GLsizei height,
				GLfloat xorig, GLfloat yorig,
				GLfloat xmove, GLfloat ymove,
				const GLubyte *bitmap );

extern void radeonReadPixels( GLint x, GLint y,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      GLvoid *pixels );

extern void radeon_select_Install( GLcontext *ctx );

extern void radeonInitSelect( GLcontext *ctx );

extern void radeonVtxfmtDestroy( GLcontext *ctx );

extern void radeonVtxfmtMakeCurrent( GLcontext *ctx );

extern void radeonVtxfmtUnbindContext( GLcontext *ctx );

extern void radeonVtxfmtInit( GLcontext *ctx );

extern void radeonTclFallback( GLcontext *ctx, GLuint bit, GLboolean mode );

extern void radeonVtxfmtInvalidate( GLcontext *ctx );

extern void radeonSubsetVtxEnableTCL( radeonContextPtr rmesa, GLboolean flag );

extern void radeonUpdateTextureState( GLcontext *ctx );

extern void radeonInitTextureFuncs( GLcontext *ctx );

extern void radeonAgeTextures( radeonContextPtr rmesa, int heap );

extern void radeonDestroyTexObj( radeonContextPtr rmesa, radeonTexObjPtr t );

#endif
