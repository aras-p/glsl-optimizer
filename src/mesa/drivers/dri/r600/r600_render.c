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
 * \brief R600 Render (Vertex Buffer Implementation)
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
#include "r600_context.h"
#include "r600_ioctl.h"
#include "r600_state.h"
#include "r600_reg.h"
#include "r600_tex.h"
#include "r600_emit.h"
#include "r600_fragprog.h"
extern int future_hw_tcl_on;

/**
 * \brief Convert a OpenGL primitive type into a R600 primitive type.
 */
int r600PrimitiveType(r600ContextPtr rmesa, int prim)
{
	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
		return R600_VAP_VF_CNTL__PRIM_POINTS;
		break;
	case GL_LINES:
		return R600_VAP_VF_CNTL__PRIM_LINES;
		break;
	case GL_LINE_STRIP:
		return R600_VAP_VF_CNTL__PRIM_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		return R600_VAP_VF_CNTL__PRIM_LINE_LOOP;
		break;
	case GL_TRIANGLES:
		return R600_VAP_VF_CNTL__PRIM_TRIANGLES;
		break;
	case GL_TRIANGLE_STRIP:
		return R600_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		return R600_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
		break;
	case GL_QUADS:
		return R600_VAP_VF_CNTL__PRIM_QUADS;
		break;
	case GL_QUAD_STRIP:
		return R600_VAP_VF_CNTL__PRIM_QUAD_STRIP;
		break;
	case GL_POLYGON:
		return R600_VAP_VF_CNTL__PRIM_POLYGON;
		break;
	default:
		assert(0);
		return -1;
		break;
	}
}

int r600NumVerts(r600ContextPtr rmesa, int num_verts, int prim)
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

static void r600EmitElts(GLcontext * ctx, void *elts, unsigned long n_elts)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	void *out;

	radeonAllocDmaRegion(&rmesa->radeon, &rmesa->radeon.tcl.elt_dma_bo,
			     &rmesa->radeon.tcl.elt_dma_offset, n_elts * 4, 4);
	radeon_bo_map(rmesa->radeon.tcl.elt_dma_bo, 1);
	out = rmesa->radeon.tcl.elt_dma_bo->ptr + rmesa->radeon.tcl.elt_dma_offset;
	memcpy(out, elts, n_elts * 4);
	radeon_bo_unmap(rmesa->radeon.tcl.elt_dma_bo);
}

static void r600FireEB(r600ContextPtr rmesa, int vertex_count, int type)
{
	BATCH_LOCALS(&rmesa->radeon);

	if (vertex_count > 0) {
		BEGIN_BATCH(10);
		OUT_BATCH_PACKET3(R600_PACKET3_3D_DRAW_INDX_2, 0);
		OUT_BATCH(R600_VAP_VF_CNTL__PRIM_WALK_INDICES |
			  ((vertex_count + 0) << 16) |
			  type |
			  R600_VAP_VF_CNTL__INDEX_SIZE_32bit);
		
		if (!rmesa->radeon.radeonScreen->kernel_mm) {
			OUT_BATCH_PACKET3(R600_PACKET3_INDX_BUFFER, 2);
			OUT_BATCH(R600_INDX_BUFFER_ONE_REG_WR | (0 << R600_INDX_BUFFER_SKIP_SHIFT) |
	    			 (R600_VAP_PORT_IDX0 >> 2));
			OUT_BATCH_RELOC(rmesa->radeon.tcl.elt_dma_offset,
					rmesa->radeon.tcl.elt_dma_bo,
					rmesa->radeon.tcl.elt_dma_offset,
					RADEON_GEM_DOMAIN_GTT, 0, 0);
			OUT_BATCH(vertex_count);
		} else {
			OUT_BATCH_PACKET3(R600_PACKET3_INDX_BUFFER, 2);
			OUT_BATCH(R600_INDX_BUFFER_ONE_REG_WR | (0 << R600_INDX_BUFFER_SKIP_SHIFT) |
	    			 (R600_VAP_PORT_IDX0 >> 2));
			OUT_BATCH(rmesa->radeon.tcl.elt_dma_offset);
			OUT_BATCH(vertex_count);
			radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
					      rmesa->radeon.tcl.elt_dma_bo,
					      RADEON_GEM_DOMAIN_GTT, 0, 0);
		}
		END_BATCH();
	}
}

static void r600EmitAOS(r600ContextPtr rmesa, GLuint nr, GLuint offset)
{
	BATCH_LOCALS(&rmesa->radeon);
	uint32_t voffset;
	int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
	int i;
	
	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s: nr=%d, ofs=0x%08x\n", __FUNCTION__, nr,
			offset);

    
	if (!rmesa->radeon.radeonScreen->kernel_mm) {
		BEGIN_BATCH(sz+2+(nr * 2));
		OUT_BATCH_PACKET3(R600_PACKET3_3D_LOAD_VBPNTR, sz - 1);
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
		OUT_BATCH_PACKET3(R600_PACKET3_3D_LOAD_VBPNTR, sz - 1);
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

static void r600FireAOS(r600ContextPtr rmesa, int vertex_count, int type)
{
	BATCH_LOCALS(&rmesa->radeon);

	BEGIN_BATCH(3);
	OUT_BATCH_PACKET3(R600_PACKET3_3D_DRAW_VBUF_2, 0);
	OUT_BATCH(R600_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (vertex_count << 16) | type);
	END_BATCH();
}

static void r600RunRenderPrimitive(r600ContextPtr rmesa, GLcontext * ctx,
				   int start, int end, int prim)
{
	int type, num_verts;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

	type = r600PrimitiveType(rmesa, prim);
	num_verts = r600NumVerts(rmesa, end - start, prim);

	if (type < 0 || num_verts <= 0)
		return;

	/* Make space for at least 64 dwords.
	 * This is supposed to ensure that we can get all rendering
	 * commands into a single command buffer.
	 */
	rcommonEnsureCmdBufSpace(&rmesa->radeon, 64, __FUNCTION__);

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
		r600EmitElts(ctx, vb->Elts, num_verts);
		r600EmitAOS(rmesa, rmesa->radeon.tcl.aos_count, start);
		r600FireEB(rmesa, num_verts, type);
	} else {
		r600EmitAOS(rmesa, rmesa->radeon.tcl.aos_count, start);
		r600FireAOS(rmesa, num_verts, type);
	}
	COMMIT_BATCH();
}

static GLboolean r600RunRender(GLcontext * ctx,
			       struct tnl_pipeline_stage *stage)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	int i;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	r600UpdateShaders(rmesa);
	if (r600EmitArrays(ctx))
		return GL_TRUE;

	r600UpdateShaderStates(rmesa);

	r600EmitCacheFlush(rmesa);
	radeonEmitState(&rmesa->radeon);

	for (i = 0; i < vb->PrimitiveCount; i++) {
		GLuint prim = _tnl_translate_prim(&vb->Primitive[i]);
		GLuint start = vb->Primitive[i].start;
		GLuint end = vb->Primitive[i].start + vb->Primitive[i].count;
		r600RunRenderPrimitive(rmesa, ctx, start, end, prim);
	}

	r600EmitCacheFlush(rmesa);

	radeonReleaseArrays(ctx, ~0);

	return GL_FALSE;
}

#define FALLBACK_IF(expr)						\
	do {								\
		if (expr) {						\
			if (1 || RADEON_DEBUG & DEBUG_FALLBACKS)	\
				WARN_ONCE("Software fallback:%s\n",	\
					  #expr);			\
			return R600_FALLBACK_RAST;			\
		}							\
	} while(0)

static int r600Fallback(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	const unsigned back = ctx->Stencil._BackFace;
	
	FALLBACK_IF(r600->radeon.Fallback);
	/* Do we need to use new-style shaders?
	 * Also is there a better way to do this? */
	{
		struct r600_fragment_program *fp = (struct r600_fragment_program *)
			(char *)ctx->FragmentProgram._Current;
		if (fp) {
			if (!fp->translated) {
				r600TranslateFragmentShader(r600, fp);
				FALLBACK_IF(!fp->translated);
			}
		}
	}

	FALLBACK_IF(ctx->RenderMode != GL_RENDER);

	/* If GL_EXT_stencil_two_side is disabled, this fallback check can
	 * be removed.
	 */
	FALLBACK_IF(ctx->Stencil.Ref[0] != ctx->Stencil.Ref[back]
		    || ctx->Stencil.ValueMask[0] !=
		    ctx->Stencil.ValueMask[back]
		    || ctx->Stencil.WriteMask[0] !=
		    ctx->Stencil.WriteMask[back]);

	if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite)
		FALLBACK_IF(ctx->Point.PointSprite);

	if (!r600->disable_lowimpact_fallback) {
		FALLBACK_IF(ctx->Polygon.StippleFlag);
		FALLBACK_IF(ctx->Multisample._Enabled);
		FALLBACK_IF(ctx->Line.StippleFlag);
		FALLBACK_IF(ctx->Line.SmoothFlag);
		FALLBACK_IF(ctx->Point.SmoothFlag);
	}

	return R600_FALLBACK_NONE;
}

static GLboolean r600RunNonTCLRender(GLcontext * ctx,
				     struct tnl_pipeline_stage *stage)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r600Fallback(ctx) >= R600_FALLBACK_RAST)
		return GL_TRUE;

	if (!(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
 	        return GL_TRUE;

	if (!r600ValidateBuffers(ctx))
	    return GL_TRUE;
	
	return r600RunRender(ctx, stage);
}

static GLboolean r600RunTCLRender(GLcontext * ctx,
				  struct tnl_pipeline_stage *stage)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	struct r600_vertex_program *vp;

	hw_tcl_on = future_hw_tcl_on;

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (hw_tcl_on == GL_FALSE)
		return GL_TRUE;

	if (r600Fallback(ctx) >= R600_FALLBACK_TCL) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}

	if (!r600ValidateBuffers(ctx))
	    return GL_TRUE;
	
	r600UpdateShaders(rmesa);

	vp = (struct r600_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
	if (vp->native == GL_FALSE) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}

	return r600RunRender(ctx, stage);
}

const struct tnl_pipeline_stage _r600_render_stage = {
	"r600 Hardware Rasterization",
	NULL,
	NULL,
	NULL,
	NULL,
	r600RunNonTCLRender
};

const struct tnl_pipeline_stage _r600_tcl_stage = {
	"r600 Hardware Transform, Clipping and Lighting",
	NULL,
	NULL,
	NULL,
	NULL,
	r600RunTCLRender
};
