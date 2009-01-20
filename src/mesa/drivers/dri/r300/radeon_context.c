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

/**
 * \file radeon_context.c
 * Common context initialization.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include <dlfcn.h>

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/state.h"
#include "main/matrix.h"
#include "main/framebuffer.h"

#include "drivers/common/driverfuncs.h"
#include "swrast/swrast.h"

#include "radeon_buffer.h"
#include "radeon_screen.h"
#include "radeon_ioctl.h"
#include "radeon_macros.h"
#include "radeon_reg.h"

#include "radeon_state.h"
#include "r300_state.h"

#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */
#include "drirenderbuffer.h"

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean radeonUnbindContext(__DRIcontextPrivate * driContextPriv)
{
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;

	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "%s ctx %p\n", __FUNCTION__,
			radeon->glCtx);

	return GL_TRUE;
}

