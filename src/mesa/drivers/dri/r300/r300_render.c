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

#include "r300_render.h"

#include "main/glheader.h"
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
#include "vbo/vbo_split.h"
#include "r300_context.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_emit.h"
#include "r300_swtcl.h"

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

static void r300FireEB(r300ContextPtr rmesa, int vertex_count, int type, int offset)
{
	BATCH_LOCALS(&rmesa->radeon);
	int size;

	/* offset is in indices */
	BEGIN_BATCH(10);
	OUT_BATCH_PACKET3(R300_PACKET3_3D_DRAW_INDX_2, 0);
	if (rmesa->ind_buf.is_32bit) {
		/* convert to bytes */
		offset *= 4;
		size = vertex_count;
		OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_INDICES |
		  (vertex_count << 16) | type |
		  R300_VAP_VF_CNTL__INDEX_SIZE_32bit);
	} else {
		/* convert to bytes */
		offset *= 2;
		size = (vertex_count + 1) >> 1;
		OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_INDICES |
		   (vertex_count << 16) | type);
	}

	if (!rmesa->radeon.radeonScreen->kernel_mm) {
		OUT_BATCH_PACKET3(R300_PACKET3_INDX_BUFFER, 2);
		OUT_BATCH(R300_INDX_BUFFER_ONE_REG_WR | (0 << R300_INDX_BUFFER_SKIP_SHIFT) |
				 (R300_VAP_PORT_IDX0 >> 2));
		OUT_BATCH_RELOC(0, rmesa->ind_buf.bo, rmesa->ind_buf.bo_offset + offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
		OUT_BATCH(size);
	} else {
		OUT_BATCH_PACKET3(R300_PACKET3_INDX_BUFFER, 2);
		OUT_BATCH(R300_INDX_BUFFER_ONE_REG_WR | (0 << R300_INDX_BUFFER_SKIP_SHIFT) |
				 (R300_VAP_PORT_IDX0 >> 2));
		OUT_BATCH(rmesa->ind_buf.bo_offset + offset);
		OUT_BATCH(size);
		radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
				      rmesa->ind_buf.bo, RADEON_GEM_DOMAIN_GTT, 0, 0);
	}
	END_BATCH();
}

static void r300EmitAOS(r300ContextPtr rmesa, GLuint nr, GLuint offset)
{
	BATCH_LOCALS(&rmesa->radeon);
	uint32_t voffset;
	int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
	int i;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s: nr=%d, ofs=0x%08x\n", __FUNCTION__, nr,
			offset);

	if (!rmesa->radeon.radeonScreen->kernel_mm) {
		BEGIN_BATCH(sz+2+(nr * 2));
		OUT_BATCH_PACKET3(R300_PACKET3_3D_LOAD_VBPNTR, sz - 1);
		OUT_BATCH(nr);

		for (i = 0; i + 1 < nr; i += 2) {
			OUT_BATCH((rmesa->radeon.tcl.aos[i].components << 0) |
				  (rmesa->radeon.tcl.aos[i].stride << 8) |
				  (rmesa->radeon.tcl.aos[i + 1].components << 16) |
				  (rmesa->radeon.tcl.aos[i + 1].stride << 24));

			voffset =  rmesa->radeon.tcl.aos[i + 0].offset +
				offset * 4 * rmesa->radeon.tcl.aos[i + 0].stride;
			OUT_BATCH_RELOC(voffset,
					rmesa->radeon.tcl.aos[i].bo,
					voffset,
					RADEON_GEM_DOMAIN_GTT,
					0, 0);
			voffset =  rmesa->radeon.tcl.aos[i + 1].offset +
			  offset * 4 * rmesa->radeon.tcl.aos[i + 1].stride;
			OUT_BATCH_RELOC(voffset,
					rmesa->radeon.tcl.aos[i+1].bo,
					voffset,
					RADEON_GEM_DOMAIN_GTT,
					0, 0);
		}

		if (nr & 1) {
			OUT_BATCH((rmesa->radeon.tcl.aos[nr - 1].components << 0) |
				  (rmesa->radeon.tcl.aos[nr - 1].stride << 8));
			voffset =  rmesa->radeon.tcl.aos[nr - 1].offset +
				offset * 4 * rmesa->radeon.tcl.aos[nr - 1].stride;
			OUT_BATCH_RELOC(voffset,
					rmesa->radeon.tcl.aos[nr - 1].bo,
					voffset,
					RADEON_GEM_DOMAIN_GTT,
					0, 0);
		}
		END_BATCH();
	} else {

		BEGIN_BATCH(sz+2+(nr * 2));
		OUT_BATCH_PACKET3(R300_PACKET3_3D_LOAD_VBPNTR, sz - 1);
		OUT_BATCH(nr);

		for (i = 0; i + 1 < nr; i += 2) {
			OUT_BATCH((rmesa->radeon.tcl.aos[i].components << 0) |
				  (rmesa->radeon.tcl.aos[i].stride << 8) |
				  (rmesa->radeon.tcl.aos[i + 1].components << 16) |
				  (rmesa->radeon.tcl.aos[i + 1].stride << 24));

			voffset =  rmesa->radeon.tcl.aos[i + 0].offset +
				offset * 4 * rmesa->radeon.tcl.aos[i + 0].stride;
			OUT_BATCH(voffset);
			voffset =  rmesa->radeon.tcl.aos[i + 1].offset +
				offset * 4 * rmesa->radeon.tcl.aos[i + 1].stride;
			OUT_BATCH(voffset);
		}

		if (nr & 1) {
			OUT_BATCH((rmesa->radeon.tcl.aos[nr - 1].components << 0) |
			  (rmesa->radeon.tcl.aos[nr - 1].stride << 8));
			voffset =  rmesa->radeon.tcl.aos[nr - 1].offset +
				offset * 4 * rmesa->radeon.tcl.aos[nr - 1].stride;
			OUT_BATCH(voffset);
		}
		for (i = 0; i + 1 < nr; i += 2) {
			voffset =  rmesa->radeon.tcl.aos[i + 0].offset +
				offset * 4 * rmesa->radeon.tcl.aos[i + 0].stride;
			radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
					      rmesa->radeon.tcl.aos[i+0].bo,
					      RADEON_GEM_DOMAIN_GTT,
					      0, 0);
			voffset =  rmesa->radeon.tcl.aos[i + 1].offset +
				offset * 4 * rmesa->radeon.tcl.aos[i + 1].stride;
			radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
					      rmesa->radeon.tcl.aos[i+1].bo,
					      RADEON_GEM_DOMAIN_GTT,
					      0, 0);
		}
		if (nr & 1) {
			voffset =  rmesa->radeon.tcl.aos[nr - 1].offset +
				offset * 4 * rmesa->radeon.tcl.aos[nr - 1].stride;
			radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
					      rmesa->radeon.tcl.aos[nr-1].bo,
					      RADEON_GEM_DOMAIN_GTT,
					      0, 0);
		}
		END_BATCH();
	}

}

static void r300FireAOS(r300ContextPtr rmesa, int vertex_count, int type)
{
	BATCH_LOCALS(&rmesa->radeon);

        r300_emit_scissor(rmesa->radeon.glCtx);
	BEGIN_BATCH(3);
	OUT_BATCH_PACKET3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
	OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (vertex_count << 16) | type);
	END_BATCH();
}

void r300RunRenderPrimitive(GLcontext * ctx, int start, int end, int prim)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	BATCH_LOCALS(&rmesa->radeon);
	int type, num_verts;

	type = r300PrimitiveType(rmesa, prim);
	num_verts = r300NumVerts(rmesa, end - start, prim);

	if (type < 0 || num_verts <= 0)
		return;

	if (rmesa->ind_buf.bo) {
		GLuint first, incr, offset = 0;

		if (!split_prim_inplace(prim & PRIM_MODE_MASK, &first, &incr) &&
			num_verts > 65500) {
			WARN_ONCE("Fixme: can't handle spliting prim %d\n", prim);
			return;
		}


		r300EmitAOS(rmesa, rmesa->radeon.tcl.aos_count, 0);
		if (rmesa->radeon.radeonScreen->kernel_mm) {
			BEGIN_BATCH_NO_AUTOSTATE(2);
			OUT_BATCH_REGSEQ(R300_VAP_VF_MAX_VTX_INDX, 1);
			OUT_BATCH(rmesa->radeon.tcl.aos[0].count);
			END_BATCH();
		}

		r300_emit_scissor(rmesa->radeon.glCtx);
		while (num_verts > 0) {
			int nr;
			int align;

			nr = MIN2(num_verts, 65535);
			nr -= (nr - first) % incr;

			/* get alignment for IB correct */
			if (nr != num_verts) {
				do {
				    align = nr * (rmesa->ind_buf.is_32bit ? 4 : 2);
				    if (align % 4)
					nr -= incr;
				} while(align % 4);
				if (nr <= 0) {
					WARN_ONCE("did the impossible happen? we never aligned nr to dword\n");
					return;
				}
					
			}
			r300FireEB(rmesa, nr, type, offset);

			num_verts -= nr;
			offset += nr;
		}

	} else {
		GLuint first, incr, offset = 0;

		if (!split_prim_inplace(prim & PRIM_MODE_MASK, &first, &incr) &&
			num_verts > 65535) {
			WARN_ONCE("Fixme: can't handle spliting prim %d\n", prim);
			return;
		}

		if (rmesa->radeon.radeonScreen->kernel_mm) {
			BEGIN_BATCH_NO_AUTOSTATE(2);
			OUT_BATCH_REGSEQ(R300_VAP_VF_MAX_VTX_INDX, 1);
			OUT_BATCH(rmesa->radeon.tcl.aos[0].count);
			END_BATCH();
		}

		r300_emit_scissor(rmesa->radeon.glCtx);
		while (num_verts > 0) {
			int nr;
			nr = MIN2(num_verts, 65535);
			nr -= (nr - first) % incr;
			r300EmitAOS(rmesa, rmesa->radeon.tcl.aos_count, start + offset);
			r300FireAOS(rmesa, nr, type);
			num_verts -= nr;
			offset += nr;
		}
	}
	COMMIT_BATCH();
}

static const char *getFallbackString(r300ContextPtr rmesa, uint32_t bit)
{
	static char common_fallback_str[32];
	switch (bit) {
		case R300_FALLBACK_VERTEX_PROGRAM :
			return "vertex program";
		case R300_FALLBACK_LINE_SMOOTH:
			return "smooth lines";
		case R300_FALLBACK_POINT_SMOOTH:
			return "smooth points";
		case R300_FALLBACK_POLYGON_SMOOTH:
			return "smooth polygons";
		case R300_FALLBACK_LINE_STIPPLE:
			return "line stipple";
		case R300_FALLBACK_POLYGON_STIPPLE:
			return "polygon stipple";
		case R300_FALLBACK_STENCIL_TWOSIDE:
			return "two-sided stencil";
		case R300_FALLBACK_RENDER_MODE:
			return "render mode != GL_RENDER";
		case R300_FALLBACK_FRAGMENT_PROGRAM:
			return "fragment program";
		case R300_FALLBACK_RADEON_COMMON:
			snprintf(common_fallback_str, 32, "radeon common 0x%08x", rmesa->radeon.Fallback);
			return common_fallback_str;
		case R300_FALLBACK_AOS_LIMIT:
			return "aos limit";
		case R300_FALLBACK_INVALID_BUFFERS:
			return "invalid buffers";
		default:
			return "unknown";
	}
}

void r300SwitchFallback(GLcontext *ctx, uint32_t bit, GLboolean mode)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	uint32_t old_fallback = rmesa->fallback;
	static uint32_t fallback_warn = 0;

	if (mode) {
		if ((fallback_warn & bit) == 0) {
			if (RADEON_DEBUG & RADEON_FALLBACKS)
				fprintf(stderr, "WARNING! Falling back to software for %s\n", getFallbackString(rmesa, bit));
			fallback_warn |= bit;
		}
		rmesa->fallback |= bit;

		/* update only if we change from no tcl fallbacks to some tcl fallbacks */
		if (rmesa->options.hw_tcl_enabled) {
			if (((old_fallback & R300_TCL_FALLBACK_MASK) == 0) &&
				((bit & R300_TCL_FALLBACK_MASK) > 0)) {
				R300_STATECHANGE(rmesa, vap_cntl_status);
				rmesa->hw.vap_cntl_status.cmd[1] |= R300_VAP_TCL_BYPASS;
			}
		}

		/* update only if we change from no raster fallbacks to some raster fallbacks */
		if (((old_fallback & R300_RASTER_FALLBACK_MASK) == 0) &&
			((bit & R300_RASTER_FALLBACK_MASK) > 0)) {

			radeon_firevertices(&rmesa->radeon);
			rmesa->radeon.swtcl.RenderIndex = ~0;
			_swsetup_Wakeup( ctx );
		}
	} else {
		rmesa->fallback &= ~bit;

		/* update only if we have disabled all tcl fallbacks */
		if (rmesa->options.hw_tcl_enabled) {
			if ((old_fallback & R300_TCL_FALLBACK_MASK) == bit) {
				R300_STATECHANGE(rmesa, vap_cntl_status);
				rmesa->hw.vap_cntl_status.cmd[1] &= ~R300_VAP_TCL_BYPASS;
			}
		}

		/* update only if we have disabled all raster fallbacks */
		if ((old_fallback & R300_RASTER_FALLBACK_MASK) == bit) {
			_swrast_flush( ctx );

			tnl->Driver.Render.Start = r300RenderStart;
			tnl->Driver.Render.Finish = r300RenderFinish;
			tnl->Driver.Render.PrimitiveNotify = r300RenderPrimitive;
			tnl->Driver.Render.ResetLineStipple = r300ResetLineStipple;
			tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
			tnl->Driver.Render.CopyPV = _tnl_copy_pv;
			tnl->Driver.Render.Interp = _tnl_interp;

			_tnl_invalidate_vertex_state( ctx, ~0 );
			_tnl_invalidate_vertices( ctx, ~0 );
		}
	}

}
