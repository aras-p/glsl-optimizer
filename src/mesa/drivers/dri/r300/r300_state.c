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
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "enums.h"
#include "macros.h"
#include "context.h"
#include "dd.h"
#include "simple_list.h"

#include "api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"


/**
 * Update our tracked culling state based on Mesa's state.
 */
static void r300UpdateCulling(GLcontext* ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t val = 0;

	R300_STATECHANGE(r300, cul);
	if (ctx->Polygon.CullFlag) {
		if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
			val = R300_CULL_FRONT|R300_CULL_BACK;
		else if (ctx->Polygon.CullFaceMode == GL_FRONT)
			val = R300_CULL_FRONT;
		else
			val = R300_CULL_BACK;

		if (ctx->Polygon.FrontFace == GL_CW)
			val |= R300_FRONT_FACE_CW;
		else
			val |= R300_FRONT_FACE_CCW;
	}

	r300->hw.cul.cmd[R300_CUL_CULL] = val;
}


/**
 * Handle glEnable()/glDisable().
 *
 * \note Mesa already filters redundant calls to glEnable/glDisable.
 */
static void r300Enable(GLcontext* ctx, GLenum cap, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t newval;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s( %s = %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(cap),
			state ? "GL_TRUE" : "GL_FALSE");

	switch (cap) {
	case GL_DEPTH_TEST:
		R300_STATECHANGE(r300, zc);

		if (state) {
			if (ctx->Depth.Mask)
				newval = R300_RB3D_Z_TEST_AND_WRITE;
			else
				newval = R300_RB3D_Z_TEST;
		} else
			newval = 0;

		r300->hw.zc.cmd[R300_ZC_CNTL_0] = newval;
		break;

	case GL_CULL_FACE:
		r300UpdateCulling(ctx);
		break;

	default:
		radeonEnable(ctx, cap, state);
		return;
	}
}


/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300CullFace(GLcontext* ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}


/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300FrontFace(GLcontext* ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}


/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthFunc(GLcontext* ctx, GLenum func)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, zc);

	switch(func) {
	case GL_NEVER:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_NEVER;
		break;
	case GL_LESS:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_LESS;
		break;
	case GL_EQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_EQUAL;
		break;
	case GL_LEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_LEQUAL;
		break;
	case GL_GREATER:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_GREATER;
		break;
	case GL_NOTEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_NEQUAL;
		break;
	case GL_GEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_GEQUAL;
		break;
	case GL_ALWAYS:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_ALWAYS;
		break;
	}
}


/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthMask(GLcontext* ctx, GLboolean mask)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (!ctx->Depth.Test)
		return;

	R300_STATECHANGE(r300, zc);
	r300->hw.zc.cmd[R300_ZC_CNTL_0] = mask
		? R300_RB3D_Z_TEST_AND_WRITE : R300_RB3D_Z_TEST;
}


/**
 * Handle glColorMask()
 */
static void r300ColorMask(GLcontext* ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int mask = (b << 0) | (g << 1) | (r << 2) | (a << 3);

	if (mask != r300->hw.cmk.cmd[R300_CMK_COLORMASK]) {
		R300_STATECHANGE(r300, cmk);
		r300->hw.cmk.cmd[R300_CMK_COLORMASK] = mask;
	}
}

/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

void r300UpdateWindow(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
	GLfloat xoffset = dPriv ? (GLfloat) dPriv->x : 0;
	GLfloat yoffset = dPriv ? (GLfloat) dPriv->y + dPriv->h : 0;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat sx = v[MAT_SX];
	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat sy = -v[MAT_SY];
	GLfloat ty = (-v[MAT_TY]) + yoffset + SUBPIXEL_Y;
	GLfloat sz = v[MAT_SZ] * rmesa->state.depth.scale;
	GLfloat tz = v[MAT_TZ] * rmesa->state.depth.scale;

	R300_FIREVERTICES(rmesa);
	R300_STATECHANGE(rmesa, vpt);

	rmesa->hw.vpt.cmd[R300_VPT_XSCALE]  = r300PackFloat32(sx);
	rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
	rmesa->hw.vpt.cmd[R300_VPT_YSCALE]  = r300PackFloat32(sy);
	rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);
	rmesa->hw.vpt.cmd[R300_VPT_ZSCALE]  = r300PackFloat32(sz);
	rmesa->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(tz);
}

static void r300Viewport(GLcontext * ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height)
{
	/* Don't pipeline viewport changes, conflict with window offset
	 * setting below.  Could apply deltas to rescue pipelined viewport
	 * values, or keep the originals hanging around.
	 */
	R200_FIREVERTICES(R200_CONTEXT(ctx));
	r300UpdateWindow(ctx);
}

static void r300DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval)
{
	r300UpdateWindow(ctx);
}


/**
 * Called by Mesa after an internal state update.
 */
static void r300InvalidateState(GLcontext * ctx, GLuint new_state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_ac_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
	_ae_invalidate_state(ctx, new_state);

	/* Go inefficiency! */
	r300ResetHwState(r300);
}


/**
 * Completely recalculates hardware state based on the Mesa state.
 */
void r300ResetHwState(r300ContextPtr r300)
{
	GLcontext* ctx = r300->radeon.glCtx;
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	r300UpdateWindow(ctx);
	
	r300ColorMask(ctx,
		ctx->Color.ColorMask[RCOMP],
		ctx->Color.ColorMask[GCOMP],
		ctx->Color.ColorMask[BCOMP],
		ctx->Color.ColorMask[ACOMP]);

	r300Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
	r300DepthMask(ctx, ctx->Depth.Mask);
	r300DepthFunc(ctx, ctx->Depth.Func);

	r300UpdateCulling(ctx);

//BEGIN: TODO
	r300->hw.unk2080.cmd[1] = 0x0030045A;

	r300->hw.ovf.cmd[R300_OVF_FMT_0] = 0x00000003;
	r300->hw.ovf.cmd[R300_OVF_FMT_1] = 0x00000000;

	r300->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
				| R300_VPORT_X_OFFSET_ENA
				| R300_VPORT_Y_SCALE_ENA
				| R300_VPORT_Y_OFFSET_ENA
				| R300_VPORT_Z_SCALE_ENA
				| R300_VPORT_Z_OFFSET_ENA
				| R300_VTX_W0_FMT;
	r300->hw.vte.cmd[2] = 0x00000008;

	r300->hw.unk2134.cmd[1] = 0x00FFFFFF;
	r300->hw.unk2134.cmd[2] = 0x00000000;

	r300->hw.unk2140.cmd[1] = 0x00000000;

	((drm_r300_cmd_header_t*)r300->hw.vir[0].cmd)->unchecked_state.count = 1;
	r300->hw.vir[0].cmd[1] = 0x21030003;

	((drm_r300_cmd_header_t*)r300->hw.vir[1].cmd)->unchecked_state.count = 1;
	r300->hw.vir[1].cmd[1] = 0xF688F688;

	r300->hw.vic.cmd[R300_VIR_CNTL_0] = 0x00000001;
	r300->hw.vic.cmd[R300_VIR_CNTL_1] = 0x00000405;

	r300->hw.unk21DC.cmd[1] = 0xAAAAAAAA;

	r300->hw.unk221C.cmd[1] = R300_221C_NORMAL;

	r300->hw.unk2220.cmd[1] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[2] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[4] = r300PackFloat32(1.0);

	if (GET_CHIP(r300->radeon.radeonScreen) == RADEON_CHIP_R300)
		r300->hw.unk2288.cmd[1] = R300_2288_R300;
	else
		r300->hw.unk2288.cmd[1] = R300_2288_RV350;

	r300->hw.vof.cmd[R300_VOF_CNTL_0] = R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT
				| R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;
	r300->hw.vof.cmd[R300_VOF_CNTL_1] = 0; /* no textures */
		
	r300->hw.pvs.cmd[R300_PVS_CNTL_1] = 0;
	r300->hw.pvs.cmd[R300_PVS_CNTL_2] = 0;
	r300->hw.pvs.cmd[R300_PVS_CNTL_3] = 0;

	r300->hw.gb_enable.cmd[1] = R300_GB_POINT_STUFF_ENABLE
		| R300_GB_LINE_STUFF_ENABLE
		| R300_GB_TRIANGLE_STUFF_ENABLE;

	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_0] = 0x66666666;
	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_1] = 0x06666666;
	if (GET_CHIP(r300->radeon.radeonScreen) == RADEON_CHIP_R300)
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] = R300_GB_TILE_ENABLE
							| R300_GB_TILE_PIPE_COUNT_R300
							| R300_GB_TILE_SIZE_16;
	else
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] = R300_GB_TILE_ENABLE
							| R300_GB_TILE_PIPE_COUNT_RV300
							| R300_GB_TILE_SIZE_16;
	r300->hw.gb_misc.cmd[R300_GB_MISC_SELECT] = 0x00000000;
	r300->hw.gb_misc.cmd[R300_GB_MISC_AA_CONFIG] = 0x00000000; /* No antialiasing */

	r300->hw.txe.cmd[R300_TXE_ENABLE] = 0;

	r300->hw.unk4200.cmd[1] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk4200.cmd[4] = r300PackFloat32(1.0);

	r300->hw.unk4214.cmd[1] = 0x00050005;

	r300->hw.ps.cmd[R300_PS_POINTSIZE] = (6 << R300_POINTSIZE_X_SHIFT) |
					     (6 << R300_POINTSIZE_Y_SHIFT);

	r300->hw.unk4230.cmd[1] = 0x01800000;
	r300->hw.unk4230.cmd[2] = 0x00020006;
	r300->hw.unk4230.cmd[3] = r300PackFloat32(1.0 / 192.0);

	r300->hw.unk4260.cmd[1] = 0;
	r300->hw.unk4260.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4260.cmd[3] = r300PackFloat32(1.0);

	r300->hw.unk4274.cmd[1] = 0x00000002;
	r300->hw.unk4274.cmd[2] = 0x0003AAAA;
	r300->hw.unk4274.cmd[3] = 0x00000000;
	r300->hw.unk4274.cmd[4] = 0x00000000;

	r300->hw.unk4288.cmd[1] = 0x00000000;
	r300->hw.unk4288.cmd[2] = 0x00000001;
	r300->hw.unk4288.cmd[3] = 0x00000000;
	r300->hw.unk4288.cmd[4] = 0x00000000;
	r300->hw.unk4288.cmd[5] = 0x00000000;

	r300->hw.unk42A0.cmd[1] = 0x00000000;

	r300->hw.unk42B4.cmd[1] = 0x00000000;

	r300->hw.unk42C0.cmd[1] = 0x4B7FFFFF;
	r300->hw.unk42C0.cmd[2] = 0x00000000;

	/* The second constant is needed to get glxgears display anything .. */
	r300->hw.rc.cmd[1] = R300_RS_CNTL_0_UNKNOWN_7 | R300_RS_CNTL_0_UNKNOWN_18;
	r300->hw.rc.cmd[2] = 0;

	for(i = 1; i <= 8; ++i)
		r300->hw.ri.cmd[i] = 0x00d10000;
	r300->hw.ri.cmd[R300_RI_INTERP_1] |= R300_RS_INTERP_1_UNKNOWN;
	r300->hw.ri.cmd[R300_RI_INTERP_2] |= R300_RS_INTERP_2_UNKNOWN;
	r300->hw.ri.cmd[R300_RI_INTERP_3] |= R300_RS_INTERP_3_UNKNOWN;

	((drm_r300_cmd_header_t*)r300->hw.rr.cmd)->unchecked_state.count = 1;
	for(i = 1; i <= 8; ++i)
		r300->hw.rr.cmd[i] = 0;
	r300->hw.rr.cmd[R300_RR_ROUTE_0] = 0x4000;

	r300->hw.unk43A4.cmd[1] = 0x0000001C;
	r300->hw.unk43A4.cmd[2] = 0x2DA49525;

	r300->hw.unk43E8.cmd[1] = 0x00FFFFFF;

	r300->hw.fp.cmd[R300_FP_CNTL0] = 0;
	r300->hw.fp.cmd[R300_FP_CNTL1] = 0;
	r300->hw.fp.cmd[R300_FP_CNTL2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE0] = 0;
	r300->hw.fp.cmd[R300_FP_NODE1] = 0;
	r300->hw.fp.cmd[R300_FP_NODE2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE3] = 0;

	r300->hw.unk46A4.cmd[1] = 0x00001B01;
	r300->hw.unk46A4.cmd[2] = 0x00001B0F;
	r300->hw.unk46A4.cmd[3] = 0x00001B0F;
	r300->hw.unk46A4.cmd[4] = 0x00001B0F;
	r300->hw.unk46A4.cmd[5] = 0x00000001;

	for(i = 1; i <= 64; ++i) {
		/* create NOP instructions */
		r300->hw.fpi[0].cmd[i] = FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO));
		r300->hw.fpi[1].cmd[i] = FP_SELC(0,XYZ,NO,FP_TMP(0),0,0);
		r300->hw.fpi[2].cmd[i] = FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO));
		r300->hw.fpi[3].cmd[i] = FP_SELA(0,W,NO,FP_TMP(0),0,0);
	}

	r300->hw.unk4BC0.cmd[1] = 0;

	r300->hw.unk4BC8.cmd[1] = 0;
	r300->hw.unk4BC8.cmd[2] = 0;
	r300->hw.unk4BC8.cmd[3] = 0;

	r300->hw.at.cmd[R300_AT_ALPHA_TEST] = 0;

	r300->hw.unk4BD8.cmd[1] = 0;

	r300->hw.unk4E00.cmd[1] = 0;

	r300->hw.bld.cmd[R300_BLD_CBLEND] = 0;
	r300->hw.bld.cmd[R300_BLD_ABLEND] = 0;

	r300->hw.unk4E10.cmd[1] = 0;
	r300->hw.unk4E10.cmd[2] = 0;
	r300->hw.unk4E10.cmd[3] = 0;

	r300->hw.cb.cmd[R300_CB_OFFSET] =
		r300->radeon.radeonScreen->backOffset +
		r300->radeon.radeonScreen->fbLocation;
	r300->hw.cb.cmd[R300_CB_PITCH] = r300->radeon.radeonScreen->backPitch
		| R300_COLOR_UNKNOWN_22_23;

	r300->hw.unk4E50.cmd[1] = 0;
	r300->hw.unk4E50.cmd[2] = 0;
	r300->hw.unk4E50.cmd[3] = 0;
	r300->hw.unk4E50.cmd[4] = 0;
	r300->hw.unk4E50.cmd[5] = 0;
	r300->hw.unk4E50.cmd[6] = 0;
	r300->hw.unk4E50.cmd[7] = 0;
	r300->hw.unk4E50.cmd[8] = 0;
	r300->hw.unk4E50.cmd[9] = 0;

	r300->hw.unk4E88.cmd[1] = 0;

	r300->hw.unk4EA0.cmd[1] = 0x00000000;
	r300->hw.unk4EA0.cmd[2] = 0xffffffff;

	r300->hw.unk4F08.cmd[1] = 0x00FFFF00;

	r300->hw.unk4F10.cmd[1] = 0x00000002; // depthbuffer format?
	r300->hw.unk4F10.cmd[2] = 0x00000000;
	r300->hw.unk4F10.cmd[3] = 0x00000003;
	r300->hw.unk4F10.cmd[4] = 0x00000000;

	r300->hw.zb.cmd[R300_ZB_OFFSET] =
		r300->radeon.radeonScreen->depthOffset +
		r300->radeon.radeonScreen->fbLocation;
	r300->hw.zb.cmd[R300_ZB_PITCH] = r300->radeon.radeonScreen->depthPitch;

	r300->hw.unk4F28.cmd[1] = 0;

	r300->hw.unk4F30.cmd[1] = 0;
	r300->hw.unk4F30.cmd[2] = 0;

	r300->hw.unk4F44.cmd[1] = 0;

	r300->hw.unk4F54.cmd[1] = 0;

	((drm_r300_cmd_header_t*)r300->hw.vpi.cmd)->vpu.count = 0;
	for(i = 1; i < R300_VPI_CMDSIZE; i += 4) {
		/* MOV t0, t0 */
		r300->hw.vpi.cmd[i+0] = VP_OUT(ADD,TMP,0,XYZW);
		r300->hw.vpi.cmd[i+1] = VP_IN(TMP,0);
		r300->hw.vpi.cmd[i+2] = VP_ZERO();
		r300->hw.vpi.cmd[i+3] = VP_ZERO();
	}

	((drm_r300_cmd_header_t*)r300->hw.vpp.cmd)->vpu.count = 0;
	for(i = 1; i < R300_VPP_CMDSIZE; ++i)
		r300->hw.vpp.cmd[i] = 0;

	r300->hw.vps.cmd[R300_VPS_ZERO_0] = 0;
	r300->hw.vps.cmd[R300_VPS_ZERO_1] = 0;
	r300->hw.vps.cmd[R300_VPS_POINTSIZE] = r300PackFloat32(1.0);
	r300->hw.vps.cmd[R300_VPS_ZERO_3] = 0;
//END: TODO

	r300->hw.all_dirty = GL_TRUE;
}



/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r300InitState(r300ContextPtr r300)
{
	radeonInitState(&r300->radeon);
	
	r300->state.depth.scale = 1.0 / (GLfloat) 0xffff;

	r300ResetHwState(r300);
}



/**
 * Initialize driver's state callback functions
 */
void r300InitStateFuncs(struct dd_function_table* functions)
{
	radeonInitStateFuncs(functions);

	functions->UpdateState = r300InvalidateState;
	functions->Enable = r300Enable;
	functions->ColorMask = r300ColorMask;
	functions->DepthFunc = r300DepthFunc;
	functions->DepthMask = r300DepthMask;
	functions->CullFace = r300CullFace;
	functions->FrontFace = r300FrontFace;

	/* Viewport related */
	functions->Viewport = r300Viewport;
	functions->DepthRange = r300DepthRange;
}

