/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _VIAVB_H
#define _VIAVB_H

#include "mtypes.h"
#include "swrast/swrast.h"

#define VIA_TEX1_BIT	0x0001
#define VIA_TEX0_BIT	0x0002
#define VIA_RGBA_BIT	0x0004
#define VIA_SPEC_BIT	0x0008
#define VIA_FOG_BIT	0x0010
#define VIA_XYZW_BIT	0x0020
#define VIA_PTEX_BIT	0x0040
#define VIA_MAX_SETUP	0x0080

#define _VIA_NEW_VERTEX (_NEW_TEXTURE |                         \
                         _DD_NEW_SEPARATE_SPECULAR |            \
                         _DD_NEW_TRI_UNFILLED |                 \
                         _DD_NEW_TRI_LIGHT_TWOSIDE |            \
                         _NEW_FOG)


extern void viaChooseVertexState(GLcontext *ctx);
extern void viaCheckTexSizes(GLcontext *ctx);
extern void viaBuildVertices(GLcontext *ctx,
                             GLuint start,
                             GLuint count,
                             GLuint newinputs);


extern void *via_emit_contiguous_verts(GLcontext *ctx,
				       GLuint start,
				       GLuint count,
				       void *dest);

extern void via_translate_vertex(GLcontext *ctx,
                                 const viaVertex *src,
                                 SWvertex *dst);

extern void viaInitVB(GLcontext *ctx);
extern void viaFreeVB(GLcontext *ctx);

extern void via_print_vertex(GLcontext *ctx, const viaVertex *v);
extern void viaPrintSetupFlags(char *msg, GLuint flags);

#endif
