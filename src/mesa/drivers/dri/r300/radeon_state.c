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

#include "main/glheader.h"
#include "main/imports.h"
#include "main/api_arrayelt.h"
#include "main/enums.h"
#include "main/framebuffer.h"
#include "main/colormac.h"
#include "main/light.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_ioctl.h"


/* =============================================================
 * Scissoring
 */

static void radeonScissor(GLcontext* ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	if (ctx->Scissor.Enabled) {
		/* We don't pipeline cliprect changes */
		r300Flush(ctx);
		radeonUpdateScissor(ctx);
	}
}

/**
 * Handle common enable bits.
 * Called as a fallback by r200Enable/r300Enable.
 */
void radeonEnable(GLcontext* ctx, GLenum cap, GLboolean state)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	switch(cap) {
	case GL_SCISSOR_TEST:
		/* We don't pipeline cliprect & scissor changes */
		r300Flush(ctx);

		radeon->state.scissor.enabled = state;
		radeonUpdateScissor(ctx);
		break;

	default:
		return;
	}
}


/**
 * Initialize default state.
 * This function is called once at context init time from
 * r200InitState/r300InitState
 */
void radeonInitState(radeonContextPtr radeon)
{
	radeon->Fallback = 0;
}


/**
 * Initialize common state functions.
 * Called by r200InitStateFuncs/r300InitStateFuncs
 */
void radeonInitStateFuncs(struct dd_function_table *functions)
{
	functions->Scissor = radeonScissor;
}
