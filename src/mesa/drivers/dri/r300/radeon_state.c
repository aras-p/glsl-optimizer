/**************************************************************************

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

#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"
#include "enums.h"
#include "colormac.h"
#include "light.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "r200_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r200_state.h"


/**
 * Update cliprects and scissors.
 */
void radeonSetCliprects(radeonContextPtr radeon, GLenum mode)
{
	__DRIdrawablePrivate *dPriv = radeon->dri.drawable;

	switch (mode) {
	case GL_FRONT_LEFT:
		radeon->numClipRects = dPriv->numClipRects;
		radeon->pClipRects = dPriv->pClipRects;
		break;
	case GL_BACK_LEFT:
		/* Can't ignore 2d windows if we are page flipping.
		 */
		if (dPriv->numBackClipRects == 0 || radeon->doPageFlip) {
			radeon->numClipRects = dPriv->numClipRects;
			radeon->pClipRects = dPriv->pClipRects;
		} else {
			radeon->numClipRects = dPriv->numBackClipRects;
			radeon->pClipRects = dPriv->pBackClipRects;
		}
		break;
	default:
		fprintf(stderr, "bad mode in radeonSetCliprects\n");
		return;
	}

	if (IS_FAMILY_R200(radeon)) {
		if (((r200ContextPtr)radeon)->state.scissor.enabled)
			r200RecalcScissorRects((r200ContextPtr)radeon);
	}
}

