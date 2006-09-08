/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include <errno.h>
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"


#include "nouveau_ioctl.h"
#include "nouveau_context.h"
#include "nouveau_msg.h"

// here we call the fifo initialization ioctl and fill in stuff accordingly
void nouveauIoctlInitFifo()
{
	int ret;
	__DRIscreenPrivate *sPriv;
	drm_nouveau_fifo_init_t fifo_init;

	ret = drmCommandWriteRead(sPriv->fd, DRM_NOUVEAU_FIFO_INIT, &fifo_init, sizeof(fifo_init));
	if (ret)
		FATAL("Fifo initialization ioctl failed (returned %d)\n",ret);
	MESSAGE("Fifo init ok. Channel %d\n", fifo_init.channel);
	// XXX needs more stuff
}

void nouveauIoctlInitFunctions(struct dd_function_table *functions)
{
	// nothing for now
}

