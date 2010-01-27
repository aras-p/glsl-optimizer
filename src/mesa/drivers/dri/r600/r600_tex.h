/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __r600_TEX_H__
#define __r600_TEX_H__

/* TODO : review this after texture load code. */
#define R700_BLIT_WIDTH_BYTES 1024
/* The BASE_ADDRESS and MIP_ADDRESS fields are 256-byte-aligned */
#define R700_TEXTURE_ALIGNMENT_MASK     0x255
/* Texel pitch is 8 alignment. */
#define R700_TEXEL_PITCH_ALIGNMENT_MASK 0x7

#define R700_MAX_TEXTURE_UNITS 16

extern void r600SetDepthTexMode(struct gl_texture_object *tObj);

extern void r600SetTexBuffer(__DRIcontext *pDRICtx, GLint target,
			     __DRIdrawable *dPriv);

extern void r600SetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
			      GLint format, __DRIdrawable *dPriv);

extern void r600SetTexOffset(__DRIcontext *pDRICtx, GLint texname,
			     unsigned long long offset, GLint depth,
			     GLuint pitch);

extern GLboolean r600ValidateBuffers(GLcontext * ctx);

extern void r600InitTextureFuncs(radeonContextPtr radeon, struct dd_function_table *functions);

#endif				/* __r600_TEX_H__ */
