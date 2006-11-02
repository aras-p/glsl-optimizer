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
#include <string.h>

#include "radeon_lock.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_state.h"

#include "framebuffer.h"

#include "drirenderbuffer.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif

/* Turn on/off page flipping according to the flags in the sarea:
 */
static void radeonUpdatePageFlipping(radeonContextPtr radeon)
{
	int use_back;

	radeon->doPageFlip = radeon->sarea->pfState;
        if (!radeon->doPageFlip) {
           driFlipRenderbuffers(radeon->glCtx->WinSysDrawBuffer, GL_FALSE);
        }

	use_back = (radeon->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT);
	use_back ^= (radeon->sarea->pfCurrentPage == 1);

	if (use_back) {
		radeon->state.color.drawOffset = radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->backPitch;
	} else {
		radeon->state.color.drawOffset = radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->frontPitch;
	}
}

/**
 * Called by radeonGetLock() after the lock has been obtained.
 */
static void r300RegainedLock(radeonContextPtr radeon)
{	
	int i;
	__DRIdrawablePrivate *const drawable = radeon->dri.drawable;
	r300ContextPtr r300 = (r300ContextPtr)radeon;
	drm_radeon_sarea_t *sarea = radeon->sarea;

	if ( radeon->lastStamp != drawable->lastStamp ) {
		radeonUpdatePageFlipping(radeon);
		radeonSetCliprects(radeon);
#if 1
		r300UpdateViewportOffset( radeon->glCtx );
		driUpdateFramebufferSize(radeon->glCtx, drawable);
#else
		radeonUpdateScissor(radeon->glCtx);
#endif
		radeon->lastStamp = drawable->lastStamp;
	}

	if (sarea->ctx_owner != radeon->dri.hwContext) {
		sarea->ctx_owner = radeon->dri.hwContext;

		for (i = 0; i < r300->nr_heaps; i++) {
			DRI_AGE_TEXTURES(r300->texture_heaps[i]);
		}
	}
}

/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void radeonGetLock(radeonContextPtr radeon, GLuint flags)
{
	__DRIdrawablePrivate *const drawable = radeon->dri.drawable;
	__DRIdrawablePrivate *const readable = radeon->dri.readable;
	__DRIscreenPrivate *sPriv = radeon->dri.screen;
	
	assert (drawable != NULL);

	drmGetLock(radeon->dri.fd, radeon->dri.hwContext, flags);

	/* The window might have moved, so we might need to get new clip
	 * rects.
	 *
	 * NOTE: This releases and regrabs the hw lock to allow the X server
	 * to respond to the DRI protocol request for new drawable info.
	 * Since the hardware state depends on having the latest drawable
	 * clip rects, all state checking must be done _after_ this call.
	 */
	DRI_VALIDATE_DRAWABLE_INFO( sPriv, drawable );
	if (drawable != readable) {
		DRI_VALIDATE_DRAWABLE_INFO( sPriv, readable );
	}

	if (IS_R300_CLASS(radeon->radeonScreen))
		r300RegainedLock(radeon);
	
	radeon->lost_context = GL_TRUE;
}
