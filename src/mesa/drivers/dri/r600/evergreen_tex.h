/*
 * Copyright (C) 2008-2010  Advanced Micro Devices, Inc.
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
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#ifndef _EVERGREEN_TEX_H_
#define _EVERGREEN_TEX_H_

extern GLboolean evergreenValidateBuffers(struct gl_context * ctx);

extern void evergreenUpdateTextureState(struct gl_context * ctx);
extern void evergreenInitTextureFuncs(radeonContextPtr radeon, struct dd_function_table *functions);
extern void evergreenSetTexOffset(__DRIcontext * pDRICtx, GLint texname,
		                          unsigned long long offset, GLint depth, GLuint pitch);
extern void evergreenSetTexBuffer(__DRIcontext *pDRICtx, GLint target, GLint glx_texture_format, __DRIdrawable *dPriv);

#endif   /* _EVERGREEN_TEX_H_ */
