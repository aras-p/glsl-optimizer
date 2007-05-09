/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/*
 * \file
 *
 * \brief R300 Render (Vertex Buffer Implementation)
 *
 * The immediate implementation has been removed from CVS in favor of the vertex
 * buffer implementation.
 *
 * The render functions are called by the pipeline manager to render a batch of
 * primitives. They return TRUE to pass on to the next stage (i.e. software
 * rasterization) or FALSE to indicate that the pipeline has finished after
 * rendering something.
 *
 * When falling back to software TCL still attempt to use hardware
 * rasterization.
 *
 * I am not sure that the cache related registers are setup correctly, but
 * obviously this does work... Further investigation is needed.
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
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
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"
#include "radeon_reg.h"
#include "radeon_macros.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"
#include "r300_tex.h"
#include "r300_maos.h"
#include "r300_emit.h"
extern int future_hw_tcl_on;

static int r300PrimitiveType(r300ContextPtr rmesa, GLcontext * ctx, int prim)
{
	int type = -1;

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
		type = R300_VAP_VF_CNTL__PRIM_POINTS;
		break;
	case GL_LINES:
		type = R300_VAP_VF_CNTL__PRIM_LINES;
		break;
	case GL_LINE_STRIP:
		type = R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		type = R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
		break;
	case GL_TRIANGLES:
		type = R300_VAP_VF_CNTL__PRIM_TRIANGLES;
		break;
	case GL_TRIANGLE_STRIP:
		type = R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		type = R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
		break;
	case GL_QUADS:
		type = R300_VAP_VF_CNTL__PRIM_QUADS;
		break;
	case GL_QUAD_STRIP:
		type = R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
		break;
	case GL_POLYGON:
		type = R300_VAP_VF_CNTL__PRIM_POLYGON;
		break;
	default:
		fprintf(stderr,
			"%s:%s Do not know how to handle primitive 0x%04x - help me !\n",
			__FILE__, __FUNCTION__, prim & PRIM_MODE_MASK);
		return -1;
		break;
	}
	return type;
}

int r300NumVerts(r300ContextPtr rmesa, int num_verts, int prim)
{
	int verts_off = 0;

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
		verts_off = 0;
		break;
	case GL_LINES:
		verts_off = num_verts % 2;
		break;
	case GL_LINE_STRIP:
		if (num_verts < 2)
			verts_off = num_verts;
		break;
	case GL_LINE_LOOP:
		if (num_verts < 2)
			verts_off = num_verts;
		break;
	case GL_TRIANGLES:
		verts_off = num_verts % 3;
		break;
	case GL_TRIANGLE_STRIP:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	case GL_TRIANGLE_FAN:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	case GL_QUADS:
		verts_off = num_verts % 4;
		break;
	case GL_QUAD_STRIP:
		if (num_verts < 4)
			verts_off = num_verts;
		else
			verts_off = num_verts % 2;
		break;
	case GL_POLYGON:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	default:
		fprintf(stderr,
			"%s:%s Do not know how to handle primitive 0x%04x - help me !\n",
			__FILE__, __FUNCTION__, prim & PRIM_MODE_MASK);
		return -1;
		break;
	}

	if (RADEON_DEBUG & DEBUG_VERTS) {
		if (num_verts - verts_off == 0) {
			WARN_ONCE
			    ("user error: Need more than %d vertices to draw primitive 0x%04x !\n",
			     num_verts, prim & PRIM_MODE_MASK);
			return 0;
		}

		if (verts_off > 0) {
			WARN_ONCE
			    ("user error: %d is not a valid number of vertices for primitive 0x%04x !\n",
			     num_verts, prim & PRIM_MODE_MASK);
		}
	}

	return num_verts - verts_off;
}

static void inline r300FireEB(r300ContextPtr rmesa, unsigned long addr,
			      int vertex_count, int type, int elt_size)
{
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	unsigned long t_addr;
	unsigned long magic_1, magic_2;

	assert(elt_size == 2 || elt_size == 4);

	if (addr & (elt_size - 1)) {
		WARN_ONCE("Badly aligned buffer\n");
		return;
	}

	magic_1 = (addr % 32) / 4;
	t_addr = addr & ~0x1d;
	magic_2 = (vertex_count + 1 + (t_addr & 0x2)) / 2 + magic_1;

	check_space(6);

	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, 0);
	if (elt_size == 4) {
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES |
		    (vertex_count << 16) | type |
		    R300_VAP_VF_CNTL__INDEX_SIZE_32bit);
	} else {
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES |
		    (vertex_count << 16) | type);
	}

	start_packet3(RADEON_CP_PACKET3_INDX_BUFFER, 2);
#ifdef OPTIMIZE_ELTS
	if (elt_size == 4) {
		e32(R300_EB_UNK1 | (0 << 16) | R300_EB_UNK2);
		e32(addr);
	} else {
		e32(R300_EB_UNK1 | (magic_1 << 16) | R300_EB_UNK2);
		e32(t_addr);
	}
#else
	e32(R300_EB_UNK1 | (0 << 16) | R300_EB_UNK2);
	e32(addr);
#endif

	if (elt_size == 4) {
		e32(vertex_count);
	} else {
#ifdef OPTIMIZE_ELTS
		e32(magic_2);
#else
		e32((vertex_count + 1) / 2);
#endif
	}
}

static void r300RunRenderPrimitive(r300ContextPtr rmesa, GLcontext * ctx,
				   int start, int end, int prim)
{
	int type, num_verts;

	type = r300PrimitiveType(rmesa, ctx, prim);
	num_verts = r300NumVerts(rmesa, end - start, prim);

	if (type < 0 || num_verts <= 0)
		return;

	if (rmesa->state.VB.Elts) {
		r300EmitAOS(rmesa, rmesa->state.aos_count, start);
		if (num_verts > 65535) {
			/* not implemented yet */
			WARN_ONCE("Too many elts\n");
			return;
		}
		r300EmitElts(ctx, rmesa->state.VB.Elts, num_verts,
			     rmesa->state.VB.elt_size);
		r300FireEB(rmesa, rmesa->state.elt_dma.aos_offset,
			   num_verts, type, rmesa->state.VB.elt_size);
	} else {
		r300EmitAOS(rmesa, rmesa->state.aos_count, start);
		fire_AOS(rmesa, num_verts, type);
	}
}

GLboolean r300RunRender(GLcontext * ctx,
			       struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct radeon_vertex_buffer *VB = &rmesa->state.VB;
	int i;
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (stage) {
		TNLcontext *tnl = TNL_CONTEXT(ctx);
		radeon_vb_to_rvb(rmesa, VB, &tnl->vb);
	}

	r300UpdateShaders(rmesa);
	if (r300EmitArrays(ctx))
		return GL_TRUE;

	r300UpdateShaderStates(rmesa);

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT, 0);
	e32(R300_RB3D_DSTCACHE_UNKNOWN_0A);

	reg_start(R300_RB3D_ZCACHE_CTLSTAT, 0);
	e32(R300_RB3D_ZCACHE_UNKNOWN_03);

	r300EmitState(rmesa);

	for (i = 0; i < VB->PrimitiveCount; i++) {
		GLuint prim = _tnl_translate_prim(&VB->Primitive[i]);
		GLuint start = VB->Primitive[i].start;
		GLuint length = VB->Primitive[i].count;

		r300RunRenderPrimitive(rmesa, ctx, start, start + length, prim);
	}

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT, 0);
	e32(R300_RB3D_DSTCACHE_UNKNOWN_0A);

	reg_start(R300_RB3D_ZCACHE_CTLSTAT, 0);
	e32(R300_RB3D_ZCACHE_UNKNOWN_03);

#ifdef USER_BUFFERS
	r300UseArrays(ctx);
#endif
	r300ReleaseArrays(ctx);
	return GL_FALSE;
}

#define FALLBACK_IF(expr)						\
	do {								\
		if (expr) {						\
			if (1 || RADEON_DEBUG & DEBUG_FALLBACKS)	\
				WARN_ONCE("Software fallback:%s\n",	\
					  #expr);			\
			return R300_FALLBACK_RAST;			\
		}							\
	} while(0)

int r300Fallback(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program *fp = (struct r300_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;

	if (fp) {
		if (!fp->translated)
			r300TranslateFragmentShader(r300, fp);
		FALLBACK_IF(!fp->translated);
	}

	FALLBACK_IF(ctx->RenderMode != GL_RENDER);

	FALLBACK_IF(ctx->Stencil._TestTwoSide
		    && (ctx->Stencil.Ref[0] != ctx->Stencil.Ref[1]
			|| ctx->Stencil.ValueMask[0] !=
			ctx->Stencil.ValueMask[1]
			|| ctx->Stencil.WriteMask[0] !=
			ctx->Stencil.WriteMask[1]));

	FALLBACK_IF(ctx->Color.ColorLogicOpEnabled);

	if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite)
		FALLBACK_IF(ctx->Point.PointSprite);

	if (!r300->disable_lowimpact_fallback) {
		FALLBACK_IF(ctx->Polygon.OffsetPoint);
		FALLBACK_IF(ctx->Polygon.OffsetLine);
		FALLBACK_IF(ctx->Polygon.StippleFlag);
		FALLBACK_IF(ctx->Multisample.Enabled);
		FALLBACK_IF(ctx->Line.StippleFlag);
		FALLBACK_IF(ctx->Line.SmoothFlag);
		FALLBACK_IF(ctx->Point.SmoothFlag);
	}

	return R300_FALLBACK_NONE;
}

static GLboolean r300RunNonTNLRender(GLcontext * ctx,
				     struct tnl_pipeline_stage *stage)
{
	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300Fallback(ctx) >= R300_FALLBACK_RAST)
		return GL_TRUE;

	return r300RunRender(ctx, stage);
}

static GLboolean r300RunTCLRender(GLcontext * ctx,
				  struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp;

	hw_tcl_on = future_hw_tcl_on;

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (hw_tcl_on == GL_FALSE)
		return GL_TRUE;

	if (r300Fallback(ctx) >= R300_FALLBACK_TCL) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}

	r300UpdateShaders(rmesa);

	vp = (struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
	if (vp->native == GL_FALSE) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}

	return r300RunRender(ctx, stage);
}

const struct tnl_pipeline_stage _r300_render_stage = {
	"r300 Hardware Rasterization",
	NULL,
	NULL,
	NULL,
	NULL,
	r300RunNonTNLRender
};

const struct tnl_pipeline_stage _r300_tcl_stage = {
	"r300 Hardware Transform, Clipping and Lighting",
	NULL,
	NULL,
	NULL,
	NULL,
	r300RunTCLRender
};
