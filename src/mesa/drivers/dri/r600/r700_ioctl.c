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
 */

#include <sched.h>
#include <errno.h>

#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "swrast/swrast.h"

#include "radeon_common.h"
#include "radeon_lock.h"
#include "r600_context.h"

#include "r700_chip.h"
#include "r700_ioctl.h"
#include "r700_clear.h"

static void r700Flush(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
    context_t *     context = R700_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, radeon->cmdbuf.cs->cdw);

	/* okay if we have no cmds in the buffer &&
	   we have no DMA flush &&
	   we have no DMA buffer allocated.
	   then no point flushing anything at all.
	*/
	if (!radeon->dma.flush && !radeon->cmdbuf.cs->cdw && !radeon->dma.current)
		return;

	if (radeon->dma.flush)
		radeon->dma.flush( ctx );

	r700SendContextStates(context, GL_FALSE);
   
	if (radeon->cmdbuf.cs->cdw)
		rcommonFlushCmdBuf(radeon, __FUNCTION__);
}

void r700InitIoctlFuncs(struct dd_function_table *functions)
{
	functions->Clear = r700Clear;
	functions->Finish = radeonFinish;
	functions->Flush = r700Flush;
}
