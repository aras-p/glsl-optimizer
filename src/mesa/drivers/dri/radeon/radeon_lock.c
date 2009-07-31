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
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "dri_util.h"
#include "radeon_screen.h"
#include "radeon_common.h"
#include "radeon_lock.h"
#include "drirenderbuffer.h"

/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void radeonGetLock(radeonContextPtr rmesa, GLuint flags)
{
	__DRIdrawablePrivate *const drawable = radeon_get_drawable(rmesa);
	__DRIdrawablePrivate *const readable = radeon_get_readable(rmesa);
	__DRIscreenPrivate *sPriv = rmesa->dri.screen;

	assert(drawable != NULL);

	drmGetLock(rmesa->dri.fd, rmesa->dri.hwContext, flags);

	/* The window might have moved, so we might need to get new clip
	 * rects.
	 *
	 * NOTE: This releases and regrabs the hw lock to allow the X server
	 * to respond to the DRI protocol request for new drawable info.
	 * Since the hardware state depends on having the latest drawable
	 * clip rects, all state checking must be done _after_ this call.
	 */
	DRI_VALIDATE_DRAWABLE_INFO(sPriv, drawable);
	if (drawable != readable) {
		DRI_VALIDATE_DRAWABLE_INFO(sPriv, readable);
	}

	if (rmesa->lastStamp != drawable->lastStamp) {
		radeon_window_moved(rmesa);
		rmesa->lastStamp = drawable->lastStamp;
	}

	rmesa->vtbl.get_lock(rmesa);
}

void radeon_lock_hardware(radeonContextPtr radeon)
{
	char ret = 0;
	struct radeon_framebuffer *rfb = NULL;
	struct radeon_renderbuffer *rrb = NULL;

	if (radeon_get_drawable(radeon)) {
		rfb = radeon_get_drawable(radeon)->driverPrivate;

		if (rfb)
			rrb = radeon_get_renderbuffer(&rfb->base,
						      rfb->base._ColorDrawBufferIndexes[0]);
	}

	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
		DRM_CAS(radeon->dri.hwLock, radeon->dri.hwContext,
			 (DRM_LOCK_HELD | radeon->dri.hwContext), ret );
		if (ret)
			radeonGetLock(radeon, 0);
	}
}

void radeon_unlock_hardware(radeonContextPtr radeon)
{
	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
		DRM_UNLOCK( radeon->dri.fd,
			    radeon->dri.hwLock,
			    radeon->dri.hwContext );
	}
}
