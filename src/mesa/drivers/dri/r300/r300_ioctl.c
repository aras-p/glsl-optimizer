/*
Copyright (C) The Weather Channel, Inc.  2002.
Copyright (C) 2004 Nicolai Haehnle.
All Rights Reserved.

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
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#include <sched.h>
#include <errno.h>

#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "context.h"
#include "swrast/swrast.h"

#include "r300_context.h"
#include "radeon_ioctl.h"
#include "r300_ioctl.h"
#include "r300_cmdbuf.h"
#include "r300_state.h"
#include "r300_program.h"
#include "radeon_reg.h"

#include "vblank.h"


static void r300ClearBuffer(r300ContextPtr r300, int buffer)
{
	GLcontext* ctx = r300->radeon.glCtx;
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int i;
	GLuint cboffset, cbpitch;
	drm_r300_cmd_header_t* cmd;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s: %s buffer (%i,%i %ix%i)\n",
			__FUNCTION__, buffer ? "back" : "front",
			dPriv->x, dPriv->y, dPriv->w, dPriv->h);

	if (buffer) {
		cboffset = r300->radeon.radeonScreen->backOffset;
		cbpitch = r300->radeon.radeonScreen->backPitch;
	} else {
		cboffset = r300->radeon.radeonScreen->frontOffset;
		cbpitch = r300->radeon.radeonScreen->frontPitch;
	}

	cboffset += r300->radeon.radeonScreen->fbLocation;

	R300_STATECHANGE(r300, vpt);
	r300->hw.vpt.cmd[R300_VPT_XSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(dPriv->x);
	r300->hw.vpt.cmd[R300_VPT_YSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(dPriv->y);
	r300->hw.vpt.cmd[R300_VPT_ZSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(0.0);

	R300_STATECHANGE(r300, cb);
	r300->hw.cb.cmd[R300_CB_OFFSET] = cboffset;
	r300->hw.cb.cmd[R300_CB_PITCH] = cbpitch | R300_COLOR_UNKNOWN_22_23;

	R300_STATECHANGE(r300, unk221C);
	r300->hw.unk221C.cmd[1] = R300_221C_CLEAR;

	R300_STATECHANGE(r300, ps);
	r300->hw.ps.cmd[R300_PS_POINTSIZE] =
		((dPriv->w * 6) << R300_POINTSIZE_X_SHIFT) |
		((dPriv->h * 6) << R300_POINTSIZE_Y_SHIFT);

	R300_STATECHANGE(r300, ri);
	for(i = 1; i <= 8; ++i)
		r300->hw.ri.cmd[i] = R300_RS_INTERP_USED;

	R300_STATECHANGE(r300, rr);
	((drm_r300_cmd_header_t*)r300->hw.rr.cmd)->unchecked_state.count = 1;
	r300->hw.rr.cmd[1] = 0x00004000;

	R300_STATECHANGE(r300, cmk);
	r300->hw.cmk.cmd[R300_CMK_COLORMASK] = 0xF;

	R300_STATECHANGE(r300, fp);
	r300->hw.fp.cmd[R300_FP_CNTL0] = 0; /* 1 pass, no textures */
	r300->hw.fp.cmd[R300_FP_CNTL1] = 0; /* no temporaries */
	r300->hw.fp.cmd[R300_FP_CNTL2] = 0; /* no offset, one ALU instr */
	r300->hw.fp.cmd[R300_FP_NODE0] = 0;
	r300->hw.fp.cmd[R300_FP_NODE1] = 0;
	r300->hw.fp.cmd[R300_FP_NODE2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE3] = R300_PFS_NODE_LAST_NODE;

	R300_STATECHANGE(r300, fpi[0]);
	R300_STATECHANGE(r300, fpi[1]);
	R300_STATECHANGE(r300, fpi[2]);
	R300_STATECHANGE(r300, fpi[3]);
	((drm_r300_cmd_header_t*)r300->hw.fpi[0].cmd)->unchecked_state.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[1].cmd)->unchecked_state.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[2].cmd)->unchecked_state.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[3].cmd)->unchecked_state.count = 1;

	/* MOV o0, t0 */
	r300->hw.fpi[0].cmd[1] = FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO));
	r300->hw.fpi[1].cmd[1] = FP_SELC(0,NO,XYZ,FP_TMP(0),0,0);
	r300->hw.fpi[2].cmd[1] = FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO));
	r300->hw.fpi[3].cmd[1] = FP_SELA(0,NO,W,FP_TMP(0),0,0);

	R300_STATECHANGE(r300, pvs);
	r300->hw.pvs.cmd[R300_PVS_CNTL_1] =
		(0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT) |
		(0 << R300_PVS_CNTL_1_UNKNOWN_SHIFT) |
		(1 << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	r300->hw.pvs.cmd[R300_PVS_CNTL_2] = 0; /* no parameters */
	r300->hw.pvs.cmd[R300_PVS_CNTL_3] =
		(1 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT);

	R300_STATECHANGE(r300, vpi);
	((drm_r300_cmd_header_t*)r300->hw.vpi.cmd)->unchecked_state.count = 8;

	/* MOV o0, i0; */
	r300->hw.vpi.cmd[1] = VP_OUT(ADD,OUT,0,XYZW);
	r300->hw.vpi.cmd[2] = VP_IN(IN,0);
	r300->hw.vpi.cmd[3] = VP_ZERO();
	r300->hw.vpi.cmd[4] = 0;

	/* MOV o1, i1; */
	r300->hw.vpi.cmd[5] = VP_OUT(ADD,OUT,1,XYZW);
	r300->hw.vpi.cmd[6] = VP_IN(IN,1);
	r300->hw.vpi.cmd[7] = VP_ZERO();
	r300->hw.vpi.cmd[8] = 0;

	/* Make sure we have enough space */
	r300EnsureCmdBufSpace(r300, r300->hw.max_state_size + 9, __FUNCTION__);

	r300EmitState(r300);

	cmd = (drm_r300_cmd_header_t*)r300AllocCmdBuf(r300, 9, __FUNCTION__);
	cmd[0].packet3.cmd_type = R300_CMD_PACKET3;
	cmd[0].packet3.packet = R300_CMD_PACKET3_CLEAR;
	cmd[1].u = r300PackFloat32(dPriv->w / 2.0);
	cmd[2].u = r300PackFloat32(dPriv->h / 2.0);
	cmd[3].u = r300PackFloat32(0.0);
	cmd[4].u = r300PackFloat32(1.0);
	cmd[5].u = r300PackFloat32(ctx->Color.ClearColor[0]);
	cmd[6].u = r300PackFloat32(ctx->Color.ClearColor[1]);
	cmd[7].u = r300PackFloat32(ctx->Color.ClearColor[2]);
	cmd[8].u = r300PackFloat32(ctx->Color.ClearColor[3]);
}


/**
 * Buffer clear
 */
static void r300Clear(GLcontext * ctx, GLbitfield mask, GLboolean all,
		      GLint cx, GLint cy, GLint cw, GLint ch)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int flags = 0;
	int swapped;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s:  all=%d cx=%d cy=%d cw=%d ch=%d\n",
			__FUNCTION__, all, cx, cy, cw, ch);

	{
		LOCK_HARDWARE(&r300->radeon);
		UNLOCK_HARDWARE(&r300->radeon);
		if (dPriv->numClipRects == 0)
			return;
	}

	if (mask & DD_FRONT_LEFT_BIT) {
		flags |= DD_FRONT_LEFT_BIT;
		mask &= ~DD_FRONT_LEFT_BIT;
	}

	if (mask & DD_BACK_LEFT_BIT) {
		flags |= DD_BACK_LEFT_BIT;
		mask &= ~DD_BACK_LEFT_BIT;
	}

	if (mask) {
		if (RADEON_DEBUG & DEBUG_FALLBACKS)
			fprintf(stderr, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, mask);
		_swrast_Clear(ctx, mask, all, cx, cy, cw, ch);
	}

	swapped = r300->radeon.doPageFlip && (r300->radeon.sarea->pfCurrentPage == 1);

	if (flags & DD_FRONT_LEFT_BIT)
		r300ClearBuffer(r300, swapped);

	if (flags & DD_BACK_LEFT_BIT)
		r300ClearBuffer(r300, swapped ^ 1);

	/* Recalculate the hardware state. This could be done more efficiently,
	 * but do keep it like this for now.
	 */
	r300ResetHwState(r300);
}

void r300Flush(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300->cmdbuf.count_used > r300->cmdbuf.count_reemit)
		r300FlushCmdBuf(r300, __FUNCTION__);
}

void r300InitIoctlFuncs(struct dd_function_table *functions)
{
	functions->Clear = r300Clear;
	functions->Finish = radeonFinish;
	functions->Flush = r300Flush;
}
