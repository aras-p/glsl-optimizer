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

#include "r600_context.h"
#include "r700_chip.h"

#include "r700_shaderinst.h"
#include "r600_emit.h"

static GLboolean r700ClearFast(context_t *context, GLbitfield mask)
{
    /* TODO, fast clear need implementation */
    return GL_FALSE;
}

static void r700UserClear(GLcontext *ctx, GLuint mask)
{
	radeon_clear_tris(ctx, mask);
}

#define R600_NEWPRIM( rmesa )			\
  do {						\
  if ( rmesa->radeon.dma.flush )			\
    rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	\
  } while (0)

void r700Clear(GLcontext * ctx, GLbitfield mask)
{
    context_t *context = R700_CONTEXT(ctx);
    __DRIdrawablePrivate *dPriv = context->radeon.dri.drawable;
    const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask);
    GLbitfield swrast_mask = 0, tri_mask = 0;
    int i;
    struct gl_framebuffer *fb = ctx->DrawBuffer;

    if( GL_TRUE == r700ClearFast(context, mask) )
    {
        return;
    }

#if 0
	if (!context->radeon.radeonScreen->driScreen->dri2.enabled) {
		LOCK_HARDWARE(&context->radeon);
		UNLOCK_HARDWARE(&context->radeon);
		if (dPriv->numClipRects == 0)
			return;
	}
#endif

	R600_NEWPRIM(context);

	if (colorMask == ~0)
	  tri_mask |= (mask & BUFFER_BITS_COLOR);


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

	if (tri_mask)
		r700UserClear(ctx, tri_mask);
	if (swrast_mask) {
		if (RADEON_DEBUG & DEBUG_FALLBACKS)
			fprintf(stderr, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, swrast_mask);
		_swrast_Clear(ctx, swrast_mask);
	}

}


