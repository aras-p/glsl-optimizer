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
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
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
#include "tnl/t_context.h"
#include "tnl/t_vertex.h"
#include "tnl/t_pipeline.h"

#include "r600_context.h"
#include "r600_cmdbuf.h"

#include "r600_tex.h"

#include "r700_vertprog.h"
#include "r700_fragprog.h"
#include "r700_state.h"

#include "radeon_common_context.h"

void r700WaitForIdle(context_t *context);
void r700WaitForIdleClean(context_t *context);
GLboolean r700SendTextureState(context_t *context);
static unsigned int r700PrimitiveType(int prim);
void r600UpdateTextureState(GLcontext * ctx);
GLboolean r700SyncSurf(context_t *context,
		       struct radeon_bo *pbo,
		       uint32_t read_domain,
		       uint32_t write_domain,
		       uint32_t sync_type);

void r700WaitForIdle(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    BEGIN_BATCH_NO_AUTOSTATE(3);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
    R600_OUT_BATCH(mmWAIT_UNTIL - ASIC_CONFIG_BASE_INDEX);
    R600_OUT_BATCH(WAIT_3D_IDLE_bit);

    END_BATCH();
    COMMIT_BATCH();
}

void r700WaitForIdleClean(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    BEGIN_BATCH_NO_AUTOSTATE(5);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 0));
    R600_OUT_BATCH(CACHE_FLUSH_AND_INV_EVENT);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
    R600_OUT_BATCH(mmWAIT_UNTIL - ASIC_CONFIG_BASE_INDEX);
    R600_OUT_BATCH(WAIT_3D_IDLE_bit | WAIT_3D_IDLECLEAN_bit);

    END_BATCH();
    COMMIT_BATCH();
}

void r700Start3D(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
    {
        BEGIN_BATCH_NO_AUTOSTATE(2);
        R600_OUT_BATCH(CP_PACKET3(R600_IT_START_3D_CMDBUF, 0));
        R600_OUT_BATCH(0);
        END_BATCH();
    }

    BEGIN_BATCH_NO_AUTOSTATE(3);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_CONTEXT_CONTROL, 1));
    R600_OUT_BATCH(0x80000000);
    R600_OUT_BATCH(0x80000000);
    END_BATCH();

    COMMIT_BATCH();

    r700WaitForIdleClean(context);
}

GLboolean r700SyncSurf(context_t *context,
		       struct radeon_bo *pbo,
		       uint32_t read_domain,
		       uint32_t write_domain,
		       uint32_t sync_type)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    uint32_t cp_coher_size;

    if (!pbo)
	    return GL_FALSE;

    if (pbo->size == 0xffffffff)
	    cp_coher_size = 0xffffffff;
    else
	    cp_coher_size = ((pbo->size + 255) >> 8);

    BEGIN_BATCH_NO_AUTOSTATE(5 + 2);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SURFACE_SYNC, 3));
    R600_OUT_BATCH(sync_type);
    R600_OUT_BATCH(cp_coher_size);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(10);
    R600_OUT_BATCH_RELOC(0,
			 pbo,
			 0,
			 read_domain, write_domain, 0);
    END_BATCH();
    COMMIT_BATCH();

    return GL_TRUE;
}

static unsigned int r700PrimitiveType(int prim)
{
    switch (prim & PRIM_MODE_MASK)
    {
    case GL_POINTS:
        return DI_PT_POINTLIST;
        break;
    case GL_LINES:
        return DI_PT_LINELIST;
        break;
    case GL_LINE_STRIP:
        return DI_PT_LINESTRIP;
        break;
    case GL_LINE_LOOP:
        return DI_PT_LINELOOP;
        break;
    case GL_TRIANGLES:
        return DI_PT_TRILIST;
        break;
    case GL_TRIANGLE_STRIP:
        return DI_PT_TRISTRIP;
        break;
    case GL_TRIANGLE_FAN:
        return DI_PT_TRIFAN;
        break;
    case GL_QUADS:
        return DI_PT_QUADLIST;
        break;
    case GL_QUAD_STRIP:
        return DI_PT_QUADSTRIP;
        break;
    case GL_POLYGON:
        return DI_PT_POLYGON;
        break;
    default:
        assert(0);
        return -1;
        break;
    }
}

static int r700NumVerts(int num_verts, int prim)
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

static void r700RunRenderPrimitive(GLcontext * ctx, int start, int end, int prim)
{
	context_t *context = R700_CONTEXT(ctx);
	BATCH_LOCALS(&context->radeon);
	int type, i, total_emit;
	int num_indices;
	uint32_t vgt_draw_initiator = 0;
	uint32_t vgt_index_type     = 0;
	uint32_t vgt_primitive_type = 0;
	uint32_t vgt_num_indices    = 0;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

	type = r700PrimitiveType(prim);
	num_indices = r700NumVerts(end - start, prim);

	radeon_print(RADEON_RENDER, RADEON_TRACE,
		"%s type %x num_indices %d\n",
		__func__, type, num_indices);

	if (type < 0 || num_indices <= 0)
		return;

        total_emit =   3 /* VGT_PRIMITIVE_TYPE */
		     + 2 /* VGT_INDEX_TYPE */
		     + 2 /* NUM_INSTANCES */
                     + num_indices + 3; /* DRAW_INDEX_IMMD */

        BEGIN_BATCH_NO_AUTOSTATE(total_emit);
	// prim
        SETfield(vgt_primitive_type, type,
		 VGT_PRIMITIVE_TYPE__PRIM_TYPE_shift, VGT_PRIMITIVE_TYPE__PRIM_TYPE_mask);
        R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
        R600_OUT_BATCH(mmVGT_PRIMITIVE_TYPE - ASIC_CONFIG_BASE_INDEX);
        R600_OUT_BATCH(vgt_primitive_type);

	// index type
        SETfield(vgt_index_type, DI_INDEX_SIZE_32_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);
        R600_OUT_BATCH(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
        R600_OUT_BATCH(vgt_index_type);

	// num instances
	R600_OUT_BATCH(CP_PACKET3(R600_IT_NUM_INSTANCES, 0));
        R600_OUT_BATCH(1);

	// draw packet
        vgt_num_indices = num_indices;
        SETfield(vgt_draw_initiator, DI_SRC_SEL_IMMEDIATE, SOURCE_SELECT_shift, SOURCE_SELECT_mask);
	SETfield(vgt_draw_initiator, DI_MAJOR_MODE_0, MAJOR_MODE_shift, MAJOR_MODE_mask);

        R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_IMMD, (num_indices + 1)));
        R600_OUT_BATCH(vgt_num_indices);
        R600_OUT_BATCH(vgt_draw_initiator);

        for (i = start; i < (start + num_indices); i++) {
		if(vb->Elts)
			R600_OUT_BATCH(vb->Elts[i]);
		else
			R600_OUT_BATCH(i);
        }
        END_BATCH();
        COMMIT_BATCH();

}

/* start 3d, idle, cb/db flush */
#define PRE_EMIT_STATE_BUFSZ 10 + 5 + 14

static GLuint r700PredictRenderSize(GLcontext* ctx)
{
    context_t *context = R700_CONTEXT(ctx);
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct r700_vertex_program *vp = context->selected_vp;
    struct vertex_buffer *vb = &tnl->vb;
    GLboolean flushed;
    GLuint dwords, i;
    GLuint state_size;
    /* pre calculate aos count so state prediction works */
    context->radeon.tcl.aos_count = _mesa_bitcount(vp->mesa_program->Base.InputsRead);

    dwords = PRE_EMIT_STATE_BUFSZ;
    for (i = 0; i < vb->PrimitiveCount; i++)
        dwords += vb->Primitive[i].count + 10;
    state_size = radeonCountStateEmitSize(&context->radeon);
    flushed = rcommonEnsureCmdBufSpace(&context->radeon,
            dwords + state_size, __FUNCTION__);

    if (flushed)
        dwords += radeonCountStateEmitSize(&context->radeon);
    else
        dwords += state_size;

    radeon_print(RADEON_RENDER, RADEON_VERBOSE,
	"%s: total prediction size is %d.\n", __FUNCTION__, dwords);
    return dwords;
}

static GLboolean r700RunRender(GLcontext * ctx,
			       struct tnl_pipeline_stage *stage)
{
    context_t *context = R700_CONTEXT(ctx);
    radeonContextPtr radeon = &context->radeon;
    unsigned int i, id = 0;
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *vb = &tnl->vb;
    struct radeon_renderbuffer *rrb;

    radeon_print(RADEON_RENDER, RADEON_NORMAL, "%s: cs begin at %d\n",
                __func__, context->radeon.cmdbuf.cs->cdw);

    /* always emit CB base to prevent
     * lock ups on some chips.
     */
    R600_STATECHANGE(context, cb_target);
    /* mark vtx as dirty since it changes per-draw */
    R600_STATECHANGE(context, vtx);

    r700SetScissor(context);
    r700SetupVertexProgram(ctx);
    r700SetupFragmentProgram(ctx);
    r600UpdateTextureState(ctx);

    GLuint emit_end = r700PredictRenderSize(ctx) 
        + context->radeon.cmdbuf.cs->cdw;
    r700SetupStreams(ctx);

    radeonEmitState(radeon);

    radeon_debug_add_indent();
    /* richard test code */
    for (i = 0; i < vb->PrimitiveCount; i++) {
        GLuint prim = _tnl_translate_prim(&vb->Primitive[i]);
        GLuint start = vb->Primitive[i].start;
        GLuint end = vb->Primitive[i].start + vb->Primitive[i].count;
        r700RunRenderPrimitive(ctx, start, end, prim);
    }
    radeon_debug_remove_indent();

    /* Flush render op cached for last several quads. */
    r700WaitForIdleClean(context);

    rrb = radeon_get_colorbuffer(&context->radeon);
    if (rrb && rrb->bo)
	    r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
			 CB_ACTION_ENA_bit | (1 << (id + 6)));

    rrb = radeon_get_depthbuffer(&context->radeon);
    if (rrb && rrb->bo)
	    r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
			 DB_ACTION_ENA_bit | DB_DEST_BASE_ENA_bit);

    radeonReleaseArrays(ctx, ~0);

    radeon_print(RADEON_RENDER, RADEON_TRACE, "%s: cs end at %d\n",
                __func__, context->radeon.cmdbuf.cs->cdw);

    if ( emit_end < context->radeon.cmdbuf.cs->cdw )
       WARN_ONCE("Rendering was %d commands larger than predicted size."
	       " We might overflow  command buffer.\n", context->radeon.cmdbuf.cs->cdw - emit_end);

    return GL_FALSE;
}

static GLboolean r700RunNonTCLRender(GLcontext * ctx,
				     struct tnl_pipeline_stage *stage) /* -------------------- */
{
	GLboolean bRet = GL_TRUE;
	
	return bRet;
}

static GLboolean r700RunTCLRender(GLcontext * ctx,  /*----------------------*/
				  struct tnl_pipeline_stage *stage)
{
	GLboolean bRet = GL_FALSE;

    /* TODO : sw fallback */

    /* Need shader bo's setup before bo check */
    r700UpdateShaders(ctx);
    /**

    * Ensure all enabled and complete textures are uploaded along with any buffers being used.
    */
    if(!r600ValidateBuffers(ctx))
    {
        return GL_TRUE;
    }

    bRet = r700RunRender(ctx, stage);

    return bRet;
	//GL_FALSE will stop to do other pipe stage in _tnl_run_pipeline
    //The render here DOES finish the whole pipe, so GL_FALSE should be returned for success.
}

const struct tnl_pipeline_stage _r700_render_stage = {
	"r700 Hardware Rasterization",
	NULL,
	NULL,
	NULL,
	NULL,
	r700RunNonTCLRender
};

const struct tnl_pipeline_stage _r700_tcl_stage = {
	"r700 Hardware Transform, Clipping and Lighting",
	NULL,
	NULL,
	NULL,
	NULL,
	r700RunTCLRender
};

const struct tnl_pipeline_stage *r700_pipeline[] = 
{
    &_r700_tcl_stage,
    &_tnl_vertex_transform_stage,
	&_tnl_normal_transform_stage,
	&_tnl_lighting_stage,
	&_tnl_fog_coordinate_stage,
	&_tnl_texgen_stage,
	&_tnl_texture_transform_stage,
	&_tnl_vertex_program_stage,

    &_r700_render_stage,
    &_tnl_render_stage,
    0,
};


