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

/**
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
 *
 * \todo Add immediate implementation back? Perhaps this is useful if there are
 * no bugs...
 */

#include "main/glheader.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/dd.h"
#include "main/simple_list.h"
#include "main/api_arrayelt.h"
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
#include "r300_tex.h"
#include "r300_emit.h"
#include "r300_fragprog.h"
extern int future_hw_tcl_on;

/**
 * \brief Convert a OpenGL primitive type into a R300 primitive type.
 */
int r300PrimitiveType(r300ContextPtr rmesa, int prim)
{
	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
		return R300_VAP_VF_CNTL__PRIM_POINTS;
		break;
	case GL_LINES:
		return R300_VAP_VF_CNTL__PRIM_LINES;
		break;
	case GL_LINE_STRIP:
		return R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		return R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
		break;
	case GL_TRIANGLES:
		return R300_VAP_VF_CNTL__PRIM_TRIANGLES;
		break;
	case GL_TRIANGLE_STRIP:
		return R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		return R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
		break;
	case GL_QUADS:
		return R300_VAP_VF_CNTL__PRIM_QUADS;
		break;
	case GL_QUAD_STRIP:
		return R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
		break;
	case GL_POLYGON:
		return R300_VAP_VF_CNTL__PRIM_POLYGON;
		break;
	default:
		assert(0);
		return -1;
		break;
	}
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
		assert(0);
		return -1;
		break;
	}

	return num_verts - verts_off;
}

static void r300EmitElts(GLcontext * ctx, void *elts, unsigned long n_elts)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_dma_region *rvb = &rmesa->state.elt_dma;
	void *out;

	if (r300IsGartMemory(rmesa, elts, n_elts * 4)) {
		rvb->address = rmesa->radeon.radeonScreen->gartTextures.map;
		rvb->start = ((char *)elts) - rvb->address;
		rvb->aos_offset =
		    rmesa->radeon.radeonScreen->gart_texture_offset +
		    rvb->start;
		return;
	} else if (r300IsGartMemory(rmesa, elts, 1)) {
		WARN_ONCE("Pointer not within GART memory!\n");
		_mesa_exit(-1);
	}

	r300AllocDmaRegion(rmesa, rvb, n_elts * 4, 4);
	rvb->aos_offset = GET_START(rvb);

	out = rvb->address + rvb->start;
	memcpy(out, elts, n_elts * 4);
}

static void r300FireEB(r300ContextPtr rmesa, unsigned long addr,
		       int vertex_count, int type)
{
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;

	start_packet3(CP_PACKET3(R300_PACKET3_3D_DRAW_INDX_2, 0), 0);
	e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count << 16) | type | R300_VAP_VF_CNTL__INDEX_SIZE_32bit);

	start_packet3(CP_PACKET3(R300_PACKET3_INDX_BUFFER, 2), 2);
	e32(R300_INDX_BUFFER_ONE_REG_WR | (0 << R300_INDX_BUFFER_SKIP_SHIFT) |
	    (R300_VAP_PORT_IDX0 >> 2));
	e32(addr);
	e32(vertex_count);
}

static void r300EmitAOS(r300ContextPtr rmesa, GLuint nr, GLuint offset)
{
	int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
	int i;
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s: nr=%d, ofs=0x%08x\n", __FUNCTION__, nr,
			offset);

	start_packet3(CP_PACKET3(R300_PACKET3_3D_LOAD_VBPNTR, sz - 1), sz - 1);
	e32(nr);

	for (i = 0; i + 1 < nr; i += 2) {
		e32((rmesa->state.aos[i].aos_size << 0) |
		    (rmesa->state.aos[i].aos_stride << 8) |
		    (rmesa->state.aos[i + 1].aos_size << 16) |
		    (rmesa->state.aos[i + 1].aos_stride << 24));

		e32(rmesa->state.aos[i].aos_offset + offset * 4 * rmesa->state.aos[i].aos_stride);
		e32(rmesa->state.aos[i + 1].aos_offset + offset * 4 * rmesa->state.aos[i + 1].aos_stride);
	}

	if (nr & 1) {
		e32((rmesa->state.aos[nr - 1].aos_size << 0) |
		    (rmesa->state.aos[nr - 1].aos_stride << 8));
		e32(rmesa->state.aos[nr - 1].aos_offset + offset * 4 * rmesa->state.aos[nr - 1].aos_stride);
	}
}

static void r300FireAOS(r300ContextPtr rmesa, int vertex_count, int type)
{
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;

	start_packet3(CP_PACKET3(R300_PACKET3_3D_DRAW_VBUF_2, 0), 0);
	e32(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (vertex_count << 16) | type);
}

static void r300RunRenderPrimitive(r300ContextPtr rmesa, GLcontext * ctx,
				   int start, int end, int prim)
{
	int type, num_verts;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

	type = r300PrimitiveType(rmesa, prim);
	num_verts = r300NumVerts(rmesa, end - start, prim);

	if (type < 0 || num_verts <= 0)
		return;

	if (vb->Elts) {
		if (num_verts > 65535) {
			/* not implemented yet */
			WARN_ONCE("Too many elts\n");
			return;
		}
		/* Note: The following is incorrect, but it's the best I can do
		 * without a major refactoring of how DMA memory is handled.
		 * The problem: Ensuring that both vertex arrays *and* index
		 * arrays are at the right position, and then ensuring that
		 * the LOAD_VBPNTR, DRAW_INDX and INDX_BUFFER packets are emitted
		 * at once.
		 *
		 * So why is the following incorrect? Well, it seems like
		 * allocating the index array might actually evict the vertex
		 * arrays. *sigh*
		 */
		r300EmitElts(ctx, vb->Elts, num_verts);
		r300EmitAOS(rmesa, rmesa->state.aos_count, start);
		r300FireEB(rmesa, rmesa->state.elt_dma.aos_offset, num_verts, type);
	} else {
		r300EmitAOS(rmesa, rmesa->state.aos_count, start);
		r300FireAOS(rmesa, num_verts, type);
	}
}

static GLboolean r300RunRender(GLcontext * ctx,
			       struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int i;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;


	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	r300UpdateShaders(rmesa);
	if (r300EmitArrays(ctx))
		return GL_TRUE;

	r300UpdateShaderStates(rmesa);

	r300EmitCacheFlush(rmesa);
	r300EmitState(rmesa);

	for (i = 0; i < vb->PrimitiveCount; i++) {
		GLuint prim = _tnl_translate_prim(&vb->Primitive[i]);
		GLuint start = vb->Primitive[i].start;
		GLuint end = vb->Primitive[i].start + vb->Primitive[i].count;
		r300RunRenderPrimitive(rmesa, ctx, start, end, prim);
	}

	r300EmitCacheFlush(rmesa);

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

static int r300Fallback(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	/* Do we need to use new-style shaders?
	 * Also is there a better way to do this? */
	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
		struct r500_fragment_program *fp = (struct r500_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;
		if (fp) {
			if (!fp->translated) {
				r500TranslateFragmentShader(r300, fp);
				FALLBACK_IF(!fp->translated);
			}
		}
	} else {
		struct r300_fragment_program *fp = (struct r300_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;
		if (fp) {
			if (!fp->translated) {
				r300TranslateFragmentShader(r300, fp);
				FALLBACK_IF(!fp->translated);
			}
		}
	}

	FALLBACK_IF(ctx->RenderMode != GL_RENDER);

	FALLBACK_IF(ctx->Stencil._TestTwoSide
		    && (ctx->Stencil.Ref[0] != ctx->Stencil.Ref[1]
			|| ctx->Stencil.ValueMask[0] !=
			ctx->Stencil.ValueMask[1]
			|| ctx->Stencil.WriteMask[0] !=
			ctx->Stencil.WriteMask[1]));

	if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite)
		FALLBACK_IF(ctx->Point.PointSprite);

	if (!r300->disable_lowimpact_fallback) {
		FALLBACK_IF(ctx->Polygon.StippleFlag);
		FALLBACK_IF(ctx->Multisample._Enabled);
		FALLBACK_IF(ctx->Line.StippleFlag);
		FALLBACK_IF(ctx->Line.SmoothFlag);
		FALLBACK_IF(ctx->Point.SmoothFlag);
	}

	return R300_FALLBACK_NONE;
}

static GLboolean r300RunNonTCLRender(GLcontext * ctx,
				     struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300Fallback(ctx) >= R300_FALLBACK_RAST)
		return GL_TRUE;

	if (!(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
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
	r300RunNonTCLRender
};

const struct tnl_pipeline_stage _r300_tcl_stage = {
	"r300 Hardware Transform, Clipping and Lighting",
	NULL,
	NULL,
	NULL,
	NULL,
	r300RunTCLRender
};
