/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
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
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */
 
#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "swrast/swrast.h"

#include "radeon_lock.h"
#include "r600_context.h"

#include "r700_shaderinst.h"
#include "r700_clear.h"

static GLboolean r700ClearFast(context_t *context, GLbitfield mask)
{
    /* TODO, fast clear need implementation */
    return GL_FALSE;
}

void r700Clear(GLcontext * ctx, GLbitfield mask)
{
    context_t *context = R700_CONTEXT(ctx);
    __DRIdrawable *dPriv = radeon_get_drawable(&context->radeon);
    const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask[0]);
    GLbitfield swrast_mask = 0, tri_mask = 0;
    int i;
    struct gl_framebuffer *fb = ctx->DrawBuffer;

    radeon_print(RADEON_RENDER, RADEON_VERBOSE, "%s %x\n", __func__, mask);

    if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT)) {
        context->radeon.front_buffer_dirty = GL_TRUE;
    }

    if( GL_TRUE == r700ClearFast(context, mask) )
    {
        return;
    }
	if (!context->radeon.radeonScreen->driScreen->dri2.enabled) {
		LOCK_HARDWARE(&context->radeon);
		UNLOCK_HARDWARE(&context->radeon);
		if (dPriv->numClipRects == 0)
			return;
	}

	R600_NEWPRIM(context);

	if (colorMask == ~0)
	  tri_mask |= (mask & BUFFER_BITS_COLOR);
	else
	  tri_mask |= (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT));


	/* HW stencil */
	if (mask & BUFFER_BIT_STENCIL) {
		tri_mask |= BUFFER_BIT_STENCIL;
	}

	/* HW depth */
	if (mask & BUFFER_BIT_DEPTH) {
    	        tri_mask |= BUFFER_BIT_DEPTH;
	}

	/* If we're doing a tri pass for depth/stencil, include a likely color
	 * buffer with it.
	 */

	for (i = 0; i < BUFFER_COUNT; i++) {
	  GLuint bufBit = 1 << i;
	  if ((tri_mask) & bufBit) {
	    if (!fb->Attachment[i].Renderbuffer->ClassID) {
	      tri_mask &= ~bufBit;
	      swrast_mask |= bufBit;
	    }
	  }
	}

	/* SW fallback clearing */
	swrast_mask = mask & ~tri_mask;

	if (tri_mask) {
		radeonUserClear(ctx, tri_mask);
	}

	if (swrast_mask) {
		radeon_print(RADEON_FALLBACKS, RADEON_IMPORTANT, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, swrast_mask);
		_swrast_Clear(ctx, swrast_mask);
	}

}


