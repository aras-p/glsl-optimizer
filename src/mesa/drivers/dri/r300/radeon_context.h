/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

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
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __RADEON_CONTEXT_H__
#define __RADEON_CONTEXT_H__

#include "main/mtypes.h"
#include "main/colormac.h"
#include "radeon_screen.h"
#include "drm.h"
#include "dri_util.h"

#include "radeon_screen.h"
#include "common_context.h"

#if R200_MERGED
extern void radeonFallback(GLcontext * ctx, GLuint bit, GLboolean mode);

#define FALLBACK( radeon, bit, mode ) do {			\
   if ( 0 ) fprintf( stderr, "FALLBACK in %s: #%d=%d\n",	\
		     __FUNCTION__, bit, mode );			\
   radeonFallback( (radeon)->glCtx, bit, mode );		\
} while (0)
#else
#define FALLBACK( radeon, bit, mode ) fprintf(stderr, "%s:%s\n", __LINE__, __FILE__);
#endif

/* TCL fallbacks */
extern void radeonTclFallback(GLcontext * ctx, GLuint bit, GLboolean mode);

#define RADEON_TCL_FALLBACK_RASTER		0x0001	/* rasterization */
#define RADEON_TCL_FALLBACK_UNFILLED		0x0002	/* unfilled tris */
#define RADEON_TCL_FALLBACK_LIGHT_TWOSIDE	0x0004	/* twoside tris */
#define RADEON_TCL_FALLBACK_MATERIAL		0x0008	/* material in vb */
#define RADEON_TCL_FALLBACK_TEXGEN_0		0x0010	/* texgen, unit 0 */
#define RADEON_TCL_FALLBACK_TEXGEN_1		0x0020	/* texgen, unit 1 */
#define RADEON_TCL_FALLBACK_TEXGEN_2		0x0040	/* texgen, unit 2 */
#define RADEON_TCL_FALLBACK_TEXGEN_3		0x0080	/* texgen, unit 3 */
#define RADEON_TCL_FALLBACK_TEXGEN_4		0x0100	/* texgen, unit 4 */
#define RADEON_TCL_FALLBACK_TEXGEN_5		0x0200	/* texgen, unit 5 */
#define RADEON_TCL_FALLBACK_TCL_DISABLE		0x0400	/* user disable */
#define RADEON_TCL_FALLBACK_BITMAP		0x0800	/* draw bitmap with points */
#define RADEON_TCL_FALLBACK_VERTEX_PROGRAM	0x1000	/* vertex program active */

#if R200_MERGED
#define TCL_FALLBACK( ctx, bit, mode )	radeonTclFallback( ctx, bit, mode )
#else
#define TCL_FALLBACK( ctx, bit, mode )	;
#endif


extern void radeonSwapBuffers(__DRIdrawablePrivate * dPriv);
extern void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
				int x, int y, int w, int h);
extern GLboolean radeonInitContext(radeonContextPtr radeon,
				   struct dd_function_table *functions,
				   const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);
extern void radeonCleanupContext(radeonContextPtr radeon);
extern GLboolean radeonMakeCurrent(__DRIcontextPrivate * driContextPriv,
				   __DRIdrawablePrivate * driDrawPriv,
				   __DRIdrawablePrivate * driReadPriv);
extern GLboolean radeonUnbindContext(__DRIcontextPrivate * driContextPriv);
void radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable);

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int RADEON_DEBUG;
#else
#define RADEON_DEBUG		0
#endif

#define DEBUG_SYNC      0x1000
#define DEBUG_PIXEL     0x2000
#define DEBUG_MEMORY    0x4000

#endif				/* __RADEON_CONTEXT_H__ */
