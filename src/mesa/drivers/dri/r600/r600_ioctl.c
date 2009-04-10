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

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
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
#include "r600_ioctl.h"
#include "r600_cmdbuf.h"
#include "r600_state.h"
#include "r600_vertprog.h"
#include "radeon_reg.h"
#include "r600_emit.h"
#include "r600_fragprog.h"
#include "r600_context.h"

#include "vblank.h"

#define R200_3D_DRAW_IMMD_2      0xC0003500

#define CLEARBUFFER_COLOR	0x1
#define CLEARBUFFER_DEPTH	0x2
#define CLEARBUFFER_STENCIL	0x4

static void r600EmitClearState(GLcontext * ctx);

static void r600UserClear(GLcontext *ctx, GLuint mask)
{
	radeon_clear_tris(ctx, mask);
}

static void r600ClearBuffer(r600ContextPtr r600, int flags,
			    struct radeon_renderbuffer *rrb,
			    struct radeon_renderbuffer *rrbd)
{
	BATCH_LOCALS(&r600->radeon);
	GLcontext *ctx = r600->radeon.glCtx;
	__DRIdrawablePrivate *dPriv = r600->radeon.dri.drawable;
	GLuint cbpitch = 0;
	r600ContextPtr rmesa = r600;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s: buffer %p (%i,%i %ix%i)\n",
			__FUNCTION__, rrb, dPriv->x, dPriv->y,
			dPriv->w, dPriv->h);

	if (rrb) {
		cbpitch = (rrb->pitch / rrb->cpp);
		if (rrb->cpp == 4)
			cbpitch |= R600_COLOR_FORMAT_ARGB8888;
		else
			cbpitch |= R600_COLOR_FORMAT_RGB565;

		if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE){
			cbpitch |= R600_COLOR_TILE_ENABLE;
        }
	}

	/* TODO in bufmgr */
	cp_wait(&r600->radeon, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	end_3d(&rmesa->radeon);

	if (flags & CLEARBUFFER_COLOR) {
		assert(rrb != 0);
		BEGIN_BATCH_NO_AUTOSTATE(6);
		OUT_BATCH_REGSEQ(R600_RB3D_COLOROFFSET0, 1);
		OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
		OUT_BATCH_REGVAL(R600_RB3D_COLORPITCH0, cbpitch);
		END_BATCH();
	}
#if 1
	if (flags & (CLEARBUFFER_DEPTH | CLEARBUFFER_STENCIL)) {
		assert(rrbd != 0);
		cbpitch = (rrbd->pitch / rrbd->cpp);
		if (rrbd->bo->flags & RADEON_BO_FLAGS_MACRO_TILE){
			cbpitch |= R600_DEPTHMACROTILE_ENABLE;
        }
		if (rrbd->bo->flags & RADEON_BO_FLAGS_MICRO_TILE){
            cbpitch |= R600_DEPTHMICROTILE_TILED;
        }
		BEGIN_BATCH_NO_AUTOSTATE(6);
		OUT_BATCH_REGSEQ(R600_ZB_DEPTHOFFSET, 1);
		OUT_BATCH_RELOC(0, rrbd->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
		OUT_BATCH_REGVAL(R600_ZB_DEPTHPITCH, cbpitch);
		END_BATCH();
	}
#endif
	BEGIN_BATCH_NO_AUTOSTATE(6);
	OUT_BATCH_REGSEQ(RB3D_COLOR_CHANNEL_MASK, 1);
	if (flags & CLEARBUFFER_COLOR) {
		OUT_BATCH((ctx->Color.ColorMask[BCOMP] ? RB3D_COLOR_CHANNEL_MASK_BLUE_MASK0 : 0) |
			  (ctx->Color.ColorMask[GCOMP] ? RB3D_COLOR_CHANNEL_MASK_GREEN_MASK0 : 0) |
			  (ctx->Color.ColorMask[RCOMP] ? RB3D_COLOR_CHANNEL_MASK_RED_MASK0 : 0) |
			  (ctx->Color.ColorMask[ACOMP] ? RB3D_COLOR_CHANNEL_MASK_ALPHA_MASK0 : 0));
	} else {
		OUT_BATCH(0);
	}


	{
		uint32_t t1, t2;

		t1 = 0x0;
		t2 = 0x0;

		if (flags & CLEARBUFFER_DEPTH) {
			t1 |= R600_Z_ENABLE | R600_Z_WRITE_ENABLE;
			t2 |=
			    (R600_ZS_ALWAYS << R600_Z_FUNC_SHIFT);
		}

		if (flags & CLEARBUFFER_STENCIL) {
			t1 |= R600_STENCIL_ENABLE;
			t2 |=
			    (R600_ZS_ALWAYS <<
			     R600_S_FRONT_FUNC_SHIFT) |
			    (R600_ZS_REPLACE <<
			     R600_S_FRONT_SFAIL_OP_SHIFT) |
			    (R600_ZS_REPLACE <<
			     R600_S_FRONT_ZPASS_OP_SHIFT) |
			    (R600_ZS_REPLACE <<
			     R600_S_FRONT_ZFAIL_OP_SHIFT);
		}

		OUT_BATCH_REGSEQ(R600_ZB_CNTL, 3);
		OUT_BATCH(t1);
		OUT_BATCH(t2);
		OUT_BATCH(((ctx->Stencil.WriteMask[0] & R600_STENCILREF_MASK) <<
                   R600_STENCILWRITEMASK_SHIFT) |
			  (ctx->Stencil.Clear & R600_STENCILREF_MASK));
		END_BATCH();
	}

	if (!rmesa->radeon.radeonScreen->kernel_mm) {
		BEGIN_BATCH_NO_AUTOSTATE(9);
		OUT_BATCH(cmdpacket3(r600->radeon.radeonScreen, R300_CMD_PACKET3_CLEAR));
		OUT_BATCH_FLOAT32(dPriv->w / 2.0);
		OUT_BATCH_FLOAT32(dPriv->h / 2.0);
		OUT_BATCH_FLOAT32(ctx->Depth.Clear);
		OUT_BATCH_FLOAT32(1.0);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[0]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[1]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[2]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[3]);
		END_BATCH();
	} else {
		OUT_BATCH(CP_PACKET3(R200_3D_DRAW_IMMD_2, 8));
		OUT_BATCH(R600_PRIM_TYPE_POINT | R600_PRIM_WALK_RING |
			  (1 << R600_PRIM_NUM_VERTICES_SHIFT));
		OUT_BATCH_FLOAT32(dPriv->w / 2.0);
		OUT_BATCH_FLOAT32(dPriv->h / 2.0);
		OUT_BATCH_FLOAT32(ctx->Depth.Clear);
		OUT_BATCH_FLOAT32(1.0);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[0]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[1]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[2]);
		OUT_BATCH_FLOAT32(ctx->Color.ClearColor[3]);
	}
	
	r600EmitCacheFlush(rmesa);
	cp_wait(&r600->radeon, R300_WAIT_3D | R300_WAIT_3D_CLEAN);

	R600_STATECHANGE(r600, cb);
	R600_STATECHANGE(r600, cmk);
	R600_STATECHANGE(r600, zs);
}

static void r600EmitClearState(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	__DRIdrawablePrivate *dPriv = r600->radeon.dri.drawable;
	int i;
	GLuint vap_cntl;

	/* State atom dirty tracking is a little subtle here.
	 *
	 * On the one hand, we need to make sure base state is emitted
	 * here if we start with an empty batch buffer, otherwise clear
	 * works incorrectly with multiple processes. Therefore, the first
	 * BEGIN_BATCH cannot be a BEGIN_BATCH_NO_AUTOSTATE.
	 *
	 * On the other hand, implicit state emission clears the state atom
	 * dirty bits, so we have to call R600_STATECHANGE later than the
	 * first BEGIN_BATCH.
	 *
	 * The final trickiness is that, because we change state, we need
	 * to ensure that any stored swtcl primitives are flushed properly
	 * before we start changing state. See the R600_NEWPRIM in r600Clear
	 * for this.
	 */
	BEGIN_BATCH(31);
	OUT_BATCH_REGSEQ(R600_VAP_PROG_STREAM_CNTL_0, 1);

	OUT_BATCH(((((0 << R600_DST_VEC_LOC_SHIFT) | R600_DATA_TYPE_FLOAT_4) << R600_DATA_TYPE_0_SHIFT) |
		   ((R600_LAST_VEC | (1 << R600_DST_VEC_LOC_SHIFT) | R600_DATA_TYPE_FLOAT_4) << R600_DATA_TYPE_1_SHIFT)));

	OUT_BATCH_REGVAL(R600_FG_FOG_BLEND, 0);
	OUT_BATCH_REGVAL(R600_VAP_PROG_STREAM_CNTL_EXT_0,
	   ((((R600_SWIZZLE_SELECT_X << R600_SWIZZLE_SELECT_X_SHIFT) |
	       (R600_SWIZZLE_SELECT_Y << R600_SWIZZLE_SELECT_Y_SHIFT) |
	       (R600_SWIZZLE_SELECT_Z << R600_SWIZZLE_SELECT_Z_SHIFT) |
	       (R600_SWIZZLE_SELECT_W << R600_SWIZZLE_SELECT_W_SHIFT) |
	       ((R600_WRITE_ENA_X | R600_WRITE_ENA_Y | R600_WRITE_ENA_Z | R600_WRITE_ENA_W) << R600_WRITE_ENA_SHIFT))
	      << R600_SWIZZLE0_SHIFT) |
	     (((R600_SWIZZLE_SELECT_X << R600_SWIZZLE_SELECT_X_SHIFT) |
	       (R600_SWIZZLE_SELECT_Y << R600_SWIZZLE_SELECT_Y_SHIFT) |
	       (R600_SWIZZLE_SELECT_Z << R600_SWIZZLE_SELECT_Z_SHIFT) |
	       (R600_SWIZZLE_SELECT_W << R600_SWIZZLE_SELECT_W_SHIFT) |
	       ((R600_WRITE_ENA_X | R600_WRITE_ENA_Y | R600_WRITE_ENA_Z | R600_WRITE_ENA_W) << R600_WRITE_ENA_SHIFT))
	      << R600_SWIZZLE1_SHIFT)));

	/* R600_VAP_INPUT_CNTL_0, R600_VAP_INPUT_CNTL_1 */
	OUT_BATCH_REGSEQ(R600_VAP_VTX_STATE_CNTL, 2);
	OUT_BATCH((R600_SEL_USER_COLOR_0 << R600_COLOR_0_ASSEMBLY_SHIFT));
	OUT_BATCH(R600_INPUT_CNTL_POS | R600_INPUT_CNTL_COLOR | R600_INPUT_CNTL_TC0);

	/* comes from fglrx startup of clear */
	OUT_BATCH_REGSEQ(R600_SE_VTE_CNTL, 2);
	OUT_BATCH(R600_VTX_W0_FMT | R600_VPORT_X_SCALE_ENA |
		  R600_VPORT_X_OFFSET_ENA | R600_VPORT_Y_SCALE_ENA |
		  R600_VPORT_Y_OFFSET_ENA | R600_VPORT_Z_SCALE_ENA |
		  R600_VPORT_Z_OFFSET_ENA);
	OUT_BATCH(0x8);

	OUT_BATCH_REGVAL(R600_VAP_PSC_SGN_NORM_CNTL, 0xaaaaaaaa);

	OUT_BATCH_REGSEQ(R600_VAP_OUTPUT_VTX_FMT_0, 2);
	OUT_BATCH(R600_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT |
		  R600_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT);
	OUT_BATCH(0); /* no textures */

	OUT_BATCH_REGVAL(R600_TX_ENABLE, 0);

	OUT_BATCH_REGSEQ(R600_SE_VPORT_XSCALE, 6);
	OUT_BATCH_FLOAT32(1.0);
	OUT_BATCH_FLOAT32(dPriv->x);
	OUT_BATCH_FLOAT32(1.0);
	OUT_BATCH_FLOAT32(dPriv->y);
	OUT_BATCH_FLOAT32(1.0);
	OUT_BATCH_FLOAT32(0.0);

	OUT_BATCH_REGVAL(R600_FG_ALPHA_FUNC, 0);

	OUT_BATCH_REGSEQ(R600_RB3D_CBLEND, 2);
	OUT_BATCH(0x0);
	OUT_BATCH(0x0);
	END_BATCH();

	R600_STATECHANGE(r600, vir[0]);
	R600_STATECHANGE(r600, fogs);
	R600_STATECHANGE(r600, vir[1]);
	R600_STATECHANGE(r600, vic);
	R600_STATECHANGE(r600, vte);
	R600_STATECHANGE(r600, vof);
	R600_STATECHANGE(r600, txe);
	R600_STATECHANGE(r600, vpt);
	R600_STATECHANGE(r600, at);
	R600_STATECHANGE(r600, bld);
	R600_STATECHANGE(r600, ps);

	R600_STATECHANGE(r600, vap_clip_cntl);

	BEGIN_BATCH_NO_AUTOSTATE(2);
	OUT_BATCH_REGVAL(R600_VAP_CLIP_CNTL, R600_PS_UCP_MODE_CLIP_AS_TRIFAN | R600_CLIP_DISABLE);
	END_BATCH();

	BEGIN_BATCH_NO_AUTOSTATE(2);
	OUT_BATCH_REGVAL(R600_GA_POINT_SIZE,
		((dPriv->w * 6) << R600_POINTSIZE_X_SHIFT) |
		((dPriv->h * 6) << R600_POINTSIZE_Y_SHIFT));
	END_BATCH();


	R600_STATECHANGE(r600, ri);
	R600_STATECHANGE(r600, rc);
	R600_STATECHANGE(r600, rr);

	BEGIN_BATCH(14);
	OUT_BATCH_REGSEQ(R600_RS_IP_0, 8);
	for (i = 0; i < 8; ++i)
		OUT_BATCH(R600_RS_SEL_T(1) | R600_RS_SEL_R(2) | R600_RS_SEL_Q(3));

	OUT_BATCH_REGSEQ(R600_RS_COUNT, 2);
	OUT_BATCH((1 << R600_IC_COUNT_SHIFT) | R600_HIRES_EN);
	OUT_BATCH(0x0);

	OUT_BATCH_REGVAL(R600_RS_INST_0, R600_RS_INST_COL_CN_WRITE);
	END_BATCH();

	R600_STATECHANGE(r600, fp);
	R600_STATECHANGE(r600, fpi[0]);
	R600_STATECHANGE(r600, fpi[1]);
	R600_STATECHANGE(r600, fpi[2]);
	R600_STATECHANGE(r600, fpi[3]);

	BEGIN_BATCH(17);
	OUT_BATCH_REGSEQ(R600_US_CONFIG, 3);
	OUT_BATCH(0x0);
	OUT_BATCH(0x0);
	OUT_BATCH(0x0);
	OUT_BATCH_REGSEQ(R600_US_CODE_ADDR_0, 4);
	OUT_BATCH(0x0);
	OUT_BATCH(0x0);
	OUT_BATCH(0x0);
	OUT_BATCH(R600_RGBA_OUT);

	OUT_BATCH_REGVAL(R600_US_ALU_RGB_INST_0,
			 FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO)));
	OUT_BATCH_REGVAL(R600_US_ALU_RGB_ADDR_0,
			 FP_SELC(0, NO, XYZ, FP_TMP(0), 0, 0));
	OUT_BATCH_REGVAL(R600_US_ALU_ALPHA_INST_0,
			 FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO)));
	OUT_BATCH_REGVAL(R600_US_ALU_ALPHA_ADDR_0,
			 FP_SELA(0, NO, W, FP_TMP(0), 0, 0));
	END_BATCH();

	BEGIN_BATCH(2);
	OUT_BATCH_REGVAL(R600_VAP_PVS_STATE_FLUSH_REG, 0);
	END_BATCH();

	vap_cntl = ((10 << R600_PVS_NUM_SLOTS_SHIFT) |
		    (5 << R600_PVS_NUM_CNTLRS_SHIFT) |
		    (12 << R600_VF_MAX_VTX_NUM_SHIFT));

	if (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV515)
		vap_cntl |= (2 << R600_PVS_NUM_FPUS_SHIFT);
	else if ((r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530) ||
		 (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV560) ||
		 (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV570))
		vap_cntl |= (5 << R600_PVS_NUM_FPUS_SHIFT);
	else if ((r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV410) ||
		 (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_R420))
		vap_cntl |= (6 << R600_PVS_NUM_FPUS_SHIFT);
	else if ((r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_R520) ||
		 (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_R580))
		vap_cntl |= (8 << R600_PVS_NUM_FPUS_SHIFT);
	else
		vap_cntl |= (4 << R600_PVS_NUM_FPUS_SHIFT);

	R600_STATECHANGE(r600, vap_cntl);

	BEGIN_BATCH(2);
	OUT_BATCH_REGVAL(R600_VAP_CNTL, vap_cntl);
	END_BATCH();

	{
		struct radeon_state_atom vpu;
		uint32_t _cmd[10];
		R600_STATECHANGE(r600, pvs);
		R600_STATECHANGE(r600, vpi);

		BEGIN_BATCH(4);
		OUT_BATCH_REGSEQ(R600_VAP_PVS_CODE_CNTL_0, 3);
		OUT_BATCH((0 << R600_PVS_FIRST_INST_SHIFT) |
			  (0 << R600_PVS_XYZW_VALID_INST_SHIFT) |
			  (1 << R600_PVS_LAST_INST_SHIFT));
		OUT_BATCH((0 << R600_PVS_CONST_BASE_OFFSET_SHIFT) |
			  (0 << R600_PVS_MAX_CONST_ADDR_SHIFT));
		OUT_BATCH(1 << R600_PVS_LAST_VTX_SRC_INST_SHIFT);
		END_BATCH();

		vpu.check = check_vpu;
		vpu.cmd = _cmd;
		vpu.cmd[0] = cmdvpu(r600->radeon.radeonScreen, 0, 2);

		vpu.cmd[1] = PVS_OP_DST_OPERAND(VE_ADD, GL_FALSE, GL_FALSE,
                                         0, 0xf, PVS_DST_REG_OUT);
		vpu.cmd[2] = PVS_SRC_OPERAND(0, PVS_SRC_SELECT_X, PVS_SRC_SELECT_Y,
                                      PVS_SRC_SELECT_Z, PVS_SRC_SELECT_W,
                                      PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
		vpu.cmd[3] = PVS_SRC_OPERAND(0, PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
		vpu.cmd[4] = 0x0;

		vpu.cmd[5] = PVS_OP_DST_OPERAND(VE_ADD, GL_FALSE, GL_FALSE, 1, 0xf,
                                         PVS_DST_REG_OUT);
		vpu.cmd[6] = PVS_SRC_OPERAND(1, PVS_SRC_SELECT_X,
                                      PVS_SRC_SELECT_Y, PVS_SRC_SELECT_Z,
                                      PVS_SRC_SELECT_W, PVS_SRC_REG_INPUT,

                                      VSF_FLAG_NONE);
		vpu.cmd[7] = PVS_SRC_OPERAND(1, PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_SELECT_FORCE_0,
                                      PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
		vpu.cmd[8] = 0x0;

		r600->vap_flush_needed = GL_TRUE;
		emit_vpu(ctx, &vpu);
	}
}

static void r600KernelClear(GLcontext *ctx, GLuint flags)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = r600->radeon.dri.drawable;
	struct radeon_framebuffer *rfb = dPriv->driverPrivate;
	struct radeon_renderbuffer *rrb;
	struct radeon_renderbuffer *rrbd;
	int bits = 0;

	/* Make sure it fits there. */
	rcommonEnsureCmdBufSpace(&r600->radeon, 421 * 3, __FUNCTION__);
	if (flags || bits)
		r600EmitClearState(ctx);
	rrbd = radeon_get_renderbuffer(&rfb->base, BUFFER_DEPTH);
	if (rrbd && (flags & BUFFER_BIT_DEPTH))
		bits |= CLEARBUFFER_DEPTH;

	if (rrbd && (flags & BUFFER_BIT_STENCIL))
		bits |= CLEARBUFFER_STENCIL;

	if (flags & BUFFER_BIT_COLOR0) {
		rrb = radeon_get_renderbuffer(&rfb->base, BUFFER_COLOR0);
		r600ClearBuffer(r600, CLEARBUFFER_COLOR, rrb, NULL);
		bits = 0;
	}

	if (flags & BUFFER_BIT_FRONT_LEFT) {
		rrb = radeon_get_renderbuffer(&rfb->base, BUFFER_FRONT_LEFT);
		r600ClearBuffer(r600, bits | CLEARBUFFER_COLOR, rrb, rrbd);
		bits = 0;
	}

	if (flags & BUFFER_BIT_BACK_LEFT) {
		rrb = radeon_get_renderbuffer(&rfb->base, BUFFER_BACK_LEFT);
		r600ClearBuffer(r600, bits | CLEARBUFFER_COLOR, rrb, rrbd);
		bits = 0;
	}

	if (bits)
		r600ClearBuffer(r600, bits, NULL, rrbd);

	COMMIT_BATCH();
}

/**
 * Buffer clear
 */
static void r600Clear(GLcontext * ctx, GLbitfield mask)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = r600->radeon.dri.drawable;
	const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask);
	GLbitfield swrast_mask = 0, tri_mask = 0;
	int i;
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "r600Clear\n");

	if (!r600->radeon.radeonScreen->driScreen->dri2.enabled) {
		LOCK_HARDWARE(&r600->radeon);
		UNLOCK_HARDWARE(&r600->radeon);
		if (dPriv->numClipRects == 0)
			return;
	}

	/* Flush swtcl vertices if necessary, because we will change hardware
	 * state during clear. See also the state-related comment in
	 * r600EmitClearState.
	 */
	R600_NEWPRIM(r600);

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

	if (tri_mask) {
		if (r600->radeon.radeonScreen->kernel_mm)
			r600UserClear(ctx, tri_mask);
		else
			r600KernelClear(ctx, tri_mask);
	}
	if (swrast_mask) {
		if (RADEON_DEBUG & DEBUG_FALLBACKS)
			fprintf(stderr, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, swrast_mask);
		_swrast_Clear(ctx, swrast_mask);
	}
}


void r600InitIoctlFuncs(struct dd_function_table *functions)
{
	functions->Clear = r600Clear;
	functions->Finish = radeonFinish;
	functions->Flush = radeonFlush;
}
