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

#include "main/imports.h"
#include "main/glheader.h"
#include "main/simple_list.h"

#include "r600_context.h"
#include "r600_cmdbuf.h"

#include "r600_tex.h"
#include "r700_oglprog.h"
#include "r700_fragprog.h"
#include "r700_vertprog.h"

#include "radeon_mipmap_tree.h"

static void r700SendTexState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t         *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

    struct r700_vertex_program *vp = context->selected_vp;

	struct radeon_bo *bo = NULL;
	unsigned int i;
	BATCH_LOCALS(&context->radeon);

	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {            
			radeonTexObj *t = r700->textures[i];
			if (t) {
				if (!t->image_override) {
					bo = t->mt->bo;
				} else {
					bo = t->bo;
				}
				if (bo) {

					r700SyncSurf(context, bo,
						     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
						     0, TC_ACTION_ENA_bit);

					BEGIN_BATCH_NO_AUTOSTATE(9 + 4);
					R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));

                    if( (1<<i) & vp->r700AsmCode.unVetTexBits )                    
                    {   /* vs texture */                                     
                        R600_OUT_BATCH((i + VERT_ATTRIB_MAX + SQ_FETCH_RESOURCE_VS_OFFSET) * FETCH_RESOURCE_STRIDE);
                    }
                    else
                    {
					    R600_OUT_BATCH(i * 7);
                    }

					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE0);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE1);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE2);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE3);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE4);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE5);
					R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE6);
					R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE2,
							     bo,
							     r700->textures[i]->SQ_TEX_RESOURCE2,
							     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
					R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE3,
							     bo,
							     r700->textures[i]->SQ_TEX_RESOURCE3,
							     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
					END_BATCH();
					COMMIT_BATCH();
				}
			}
		}
	}
}

#define SAMPLER_STRIDE                 3

static void r700SendTexSamplerState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t         *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	unsigned int i;

    struct r700_vertex_program *vp = context->selected_vp;

	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {            
			radeonTexObj *t = r700->textures[i];
			if (t) {
				BEGIN_BATCH_NO_AUTOSTATE(5);
				R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_SAMPLER, 3));

                if( (1<<i) & vp->r700AsmCode.unVetTexBits )                    
                {   /* vs texture */
                    R600_OUT_BATCH((i+SQ_TEX_SAMPLER_VS_OFFSET) * SAMPLER_STRIDE); //work 1
                }
                else
                {
				    R600_OUT_BATCH(i * 3);
                }

				R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER0);
				R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER1);
				R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER2);
				END_BATCH();
				COMMIT_BATCH();
			}
		}
	}
}

static void r700SendTexBorderColorState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t         *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	unsigned int i;
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			radeonTexObj *t = r700->textures[i];
			if (t) {
				BEGIN_BATCH_NO_AUTOSTATE(2 + 4);
				R600_OUT_BATCH_REGSEQ((TD_PS_SAMPLER0_BORDER_RED + (i * 16)), 4);
				R600_OUT_BATCH(r700->textures[i]->TD_PS_SAMPLER0_BORDER_RED);
				R600_OUT_BATCH(r700->textures[i]->TD_PS_SAMPLER0_BORDER_GREEN);
				R600_OUT_BATCH(r700->textures[i]->TD_PS_SAMPLER0_BORDER_BLUE);
				R600_OUT_BATCH(r700->textures[i]->TD_PS_SAMPLER0_BORDER_ALPHA);
				END_BATCH();
				COMMIT_BATCH();
			}
		}
	}
}

extern int getTypeSize(GLenum type);
static void r700SetupVTXConstants(GLcontext  * ctx,
				  void *       pAos,
				  StreamDesc * pStreamDesc)
{
    context_t *context = R700_CONTEXT(ctx);
    struct radeon_aos * paos = (struct radeon_aos *)pAos;
    unsigned int nVBsize;
    BATCH_LOCALS(&context->radeon);

    unsigned int uSQ_VTX_CONSTANT_WORD0_0;
    unsigned int uSQ_VTX_CONSTANT_WORD1_0;
    unsigned int uSQ_VTX_CONSTANT_WORD2_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD3_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD6_0 = 0;

    if (!paos->bo)
	    return;

    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV610) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV620) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS780) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS880) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV710))
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, TC_ACTION_ENA_bit);
    else
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, VC_ACTION_ENA_bit);

    if(0 == pStreamDesc->stride)
    {
        nVBsize = paos->count * pStreamDesc->size * getTypeSize(pStreamDesc->type);
    }
    else
    {
        nVBsize = (paos->count - 1) * pStreamDesc->stride
                  + pStreamDesc->size * getTypeSize(pStreamDesc->type);
    }

    uSQ_VTX_CONSTANT_WORD0_0 = paos->offset;
    uSQ_VTX_CONSTANT_WORD1_0 = nVBsize - 1;

    SETfield(uSQ_VTX_CONSTANT_WORD2_0, 0, BASE_ADDRESS_HI_shift, BASE_ADDRESS_HI_mask); /* TODO */
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, pStreamDesc->stride, SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift,
	     SQ_VTX_CONSTANT_WORD2_0__STRIDE_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, GetSurfaceFormat(pStreamDesc->type, pStreamDesc->size, NULL),
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift,
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_mask); /* TODO : trace back api for initial data type, not only GL_FLOAT */
    
    if(GL_TRUE == pStreamDesc->normalize)
    {
        SETfield(uSQ_VTX_CONSTANT_WORD2_0, SQ_NUM_FORMAT_NORM,
	             SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift, SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_mask);
    }
    else
    {
        SETfield(uSQ_VTX_CONSTANT_WORD2_0, SQ_NUM_FORMAT_SCALED,
	             SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift, SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_mask);
    }

    if(1 == pStreamDesc->_signed)
    {
        SETbit(uSQ_VTX_CONSTANT_WORD2_0, SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit);
    }

    SETfield(uSQ_VTX_CONSTANT_WORD3_0, 1, MEM_REQUEST_SIZE_shift, MEM_REQUEST_SIZE_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD6_0, SQ_TEX_VTX_VALID_BUFFER,
	     SQ_TEX_RESOURCE_WORD6_0__TYPE_shift, SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

    BEGIN_BATCH_NO_AUTOSTATE(9 + 2);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
    R600_OUT_BATCH((pStreamDesc->element + SQ_FETCH_RESOURCE_VS_OFFSET) * FETCH_RESOURCE_STRIDE);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD0_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD1_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD2_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD3_0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD6_0);
    R600_OUT_BATCH_RELOC(uSQ_VTX_CONSTANT_WORD0_0,
                         paos->bo,
                         uSQ_VTX_CONSTANT_WORD0_0,
                         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    COMMIT_BATCH();

}

static void r700SendVTXState(GLcontext *ctx, struct radeon_state_atom *atom)
{
    context_t         *context = R700_CONTEXT(ctx);
    struct r700_vertex_program *vp = context->selected_vp;
    unsigned int i, j = 0;
    BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    if (context->radeon.tcl.aos_count == 0)
	    return;

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();
    COMMIT_BATCH();

    for(i=0; i<VERT_ATTRIB_MAX; i++) {
	    if(vp->mesa_program->Base.InputsRead & (1 << i))
	    {
                r700SetupVTXConstants(ctx,
				      (void*)(&context->radeon.tcl.aos[j]),
				      &(context->stream_desc[j]));
		j++;
	    }
    }
}

static void r700SetRenderTarget(context_t *context, int id)
{
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

    struct radeon_renderbuffer *rrb;
    unsigned int nPitchInPixel;

    rrb = radeon_get_colorbuffer(&context->radeon);
    if (!rrb || !rrb->bo) {
	    return;
    }

    R600_STATECHANGE(context, cb_target);

    /* color buffer */
    r700->render_target[id].CB_COLOR0_BASE.u32All = context->radeon.state.color.draw_offset / 256;

    nPitchInPixel = rrb->pitch/rrb->cpp;
    SETfield(r700->render_target[id].CB_COLOR0_SIZE.u32All, (nPitchInPixel/8)-1,
             PITCH_TILE_MAX_shift, PITCH_TILE_MAX_mask);
    SETfield(r700->render_target[id].CB_COLOR0_SIZE.u32All, ( (nPitchInPixel * context->radeon.radeonScreen->driScreen->fbHeight)/64 )-1,
             SLICE_TILE_MAX_shift, SLICE_TILE_MAX_mask);
    SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, ENDIAN_NONE, ENDIAN_shift, ENDIAN_mask);
    SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, ARRAY_LINEAR_GENERAL,
             CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
    if(4 == rrb->cpp)
    {
        SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, COLOR_8_8_8_8,
                 CB_COLOR0_INFO__FORMAT_shift, CB_COLOR0_INFO__FORMAT_mask);
        SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, SWAP_ALT, COMP_SWAP_shift, COMP_SWAP_mask);
    }
    else
    {
        SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, COLOR_5_6_5,
                 CB_COLOR0_INFO__FORMAT_shift, CB_COLOR0_INFO__FORMAT_mask);
        SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, SWAP_ALT_REV,
                 COMP_SWAP_shift, COMP_SWAP_mask);
    }
    SETbit(r700->render_target[id].CB_COLOR0_INFO.u32All, SOURCE_FORMAT_bit);
    SETbit(r700->render_target[id].CB_COLOR0_INFO.u32All, BLEND_CLAMP_bit);
    SETfield(r700->render_target[id].CB_COLOR0_INFO.u32All, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);

    r700->render_target[id].enabled = GL_TRUE;
}

static void r700SetDepthTarget(context_t *context)
{
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

    struct radeon_renderbuffer *rrb;
    unsigned int nPitchInPixel;

    rrb = radeon_get_depthbuffer(&context->radeon);
    if (!rrb)
	    return;

    R600_STATECHANGE(context, db_target);

    /* depth buf */
    r700->DB_DEPTH_SIZE.u32All = 0;
    r700->DB_DEPTH_BASE.u32All = 0;
    r700->DB_DEPTH_INFO.u32All = 0;
    r700->DB_DEPTH_VIEW.u32All = 0;

    nPitchInPixel = rrb->pitch/rrb->cpp;

    SETfield(r700->DB_DEPTH_SIZE.u32All, (nPitchInPixel/8)-1,
             PITCH_TILE_MAX_shift, PITCH_TILE_MAX_mask);
    SETfield(r700->DB_DEPTH_SIZE.u32All, ( (nPitchInPixel * context->radeon.radeonScreen->driScreen->fbHeight)/64 )-1,
             SLICE_TILE_MAX_shift, SLICE_TILE_MAX_mask); /* size in pixel / 64 - 1 */

    if(4 == rrb->cpp)
    {
        SETfield(r700->DB_DEPTH_INFO.u32All, DEPTH_8_24,
                 DB_DEPTH_INFO__FORMAT_shift, DB_DEPTH_INFO__FORMAT_mask);
    }
    else
    {
        SETfield(r700->DB_DEPTH_INFO.u32All, DEPTH_16,
                     DB_DEPTH_INFO__FORMAT_shift, DB_DEPTH_INFO__FORMAT_mask);
    }
    SETfield(r700->DB_DEPTH_INFO.u32All, ARRAY_1D_TILED_THIN1,
             DB_DEPTH_INFO__ARRAY_MODE_shift, DB_DEPTH_INFO__ARRAY_MODE_mask);
    /* r700->DB_PREFETCH_LIMIT.bits.DEPTH_HEIGHT_TILE_MAX = (context->currentDraw->h >> 3) - 1; */ /* z buffer sie may much bigger than what need, so use actual used h. */
}

static void r700SendDepthTargetState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	rrb = radeon_get_depthbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		return;
	}

	r700SetDepthTarget(context);

        BEGIN_BATCH_NO_AUTOSTATE(8 + 2);
	R600_OUT_BATCH_REGSEQ(DB_DEPTH_SIZE, 2);
	R600_OUT_BATCH(r700->DB_DEPTH_SIZE.u32All);
	R600_OUT_BATCH(r700->DB_DEPTH_VIEW.u32All);
	R600_OUT_BATCH_REGSEQ(DB_DEPTH_BASE, 2);
	R600_OUT_BATCH(r700->DB_DEPTH_BASE.u32All);
	R600_OUT_BATCH(r700->DB_DEPTH_INFO.u32All);
	R600_OUT_BATCH_RELOC(r700->DB_DEPTH_BASE.u32All,
			     rrb->bo,
			     r700->DB_DEPTH_BASE.u32All,
			     0, RADEON_GEM_DOMAIN_VRAM, 0);
        END_BATCH();

	if ((context->radeon.radeonScreen->chip_family > CHIP_FAMILY_R600) &&
	    (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)) {
		BEGIN_BATCH_NO_AUTOSTATE(2);
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SURFACE_BASE_UPDATE, 0));
		R600_OUT_BATCH(1 << 0);
		END_BATCH();
	}

	COMMIT_BATCH();

}

static void r700SendRenderTargetState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);
	int id = 0;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	rrb = radeon_get_colorbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		return;
	}

	r700SetRenderTarget(context, 0);

	if (id > R700_MAX_RENDER_TARGETS)
		return;

	if (!r700->render_target[id].enabled)
		return;

        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
	R600_OUT_BATCH_REGSEQ(CB_COLOR0_BASE + (4 * id), 1);
	R600_OUT_BATCH(r700->render_target[id].CB_COLOR0_BASE.u32All);
	R600_OUT_BATCH_RELOC(r700->render_target[id].CB_COLOR0_BASE.u32All,
			     rrb->bo,
			     r700->render_target[id].CB_COLOR0_BASE.u32All,
			     0, RADEON_GEM_DOMAIN_VRAM, 0);
        END_BATCH();

	if ((context->radeon.radeonScreen->chip_family > CHIP_FAMILY_R600) &&
	    (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)) {
		BEGIN_BATCH_NO_AUTOSTATE(2);
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SURFACE_BASE_UPDATE, 0));
		R600_OUT_BATCH((2 << id));
		END_BATCH();
	}
	/* Set CMASK & TILE buffer to the offset of color buffer as
	 * we don't use those this shouldn't cause any issue and we
	 * then have a valid cmd stream
	 */
	BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
	R600_OUT_BATCH_REGSEQ(CB_COLOR0_TILE + (4 * id), 1);
	R600_OUT_BATCH(r700->render_target[id].CB_COLOR0_TILE.u32All);
	R600_OUT_BATCH_RELOC(r700->render_target[id].CB_COLOR0_BASE.u32All,
			     rrb->bo,
			     r700->render_target[id].CB_COLOR0_BASE.u32All,
			     0, RADEON_GEM_DOMAIN_VRAM, 0);
	END_BATCH();
	BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
	R600_OUT_BATCH_REGSEQ(CB_COLOR0_FRAG + (4 * id), 1);
	R600_OUT_BATCH(r700->render_target[id].CB_COLOR0_FRAG.u32All);
	R600_OUT_BATCH_RELOC(r700->render_target[id].CB_COLOR0_BASE.u32All,
			     rrb->bo,
			     r700->render_target[id].CB_COLOR0_BASE.u32All,
			     0, RADEON_GEM_DOMAIN_VRAM, 0);
        END_BATCH();

        BEGIN_BATCH_NO_AUTOSTATE(12);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_SIZE + (4 * id), r700->render_target[id].CB_COLOR0_SIZE.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_VIEW + (4 * id), r700->render_target[id].CB_COLOR0_VIEW.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_INFO + (4 * id), r700->render_target[id].CB_COLOR0_INFO.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_MASK + (4 * id), r700->render_target[id].CB_COLOR0_MASK.u32All);
        END_BATCH();

	COMMIT_BATCH();

}

static void r700SendPSState(GLcontext *ctx, struct radeon_state_atom *atom)
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
    struct radeon_bo * pbo;
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    pbo = (struct radeon_bo *)r700GetActiveFpShaderBo(GL_CONTEXT(context));

    if (!pbo)
	    return;

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_PS, 1);
    R600_OUT_BATCH(r700->ps.SQ_PGM_START_PS.u32All);
    R600_OUT_BATCH_RELOC(r700->ps.SQ_PGM_START_PS.u32All,
		         pbo,
		         r700->ps.SQ_PGM_START_PS.u32All,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(9);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_PS, r700->ps.SQ_PGM_RESOURCES_PS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_EXPORTS_PS, r700->ps.SQ_PGM_EXPORTS_PS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_PS, r700->ps.SQ_PGM_CF_OFFSET_PS.u32All);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    R600_OUT_BATCH_REGVAL(SQ_LOOP_CONST_0, 0x01000FFF);
    END_BATCH();

    COMMIT_BATCH();

}

static void r700SendVSState(GLcontext *ctx, struct radeon_state_atom *atom)
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
    struct radeon_bo * pbo;
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(GL_CONTEXT(context));

    if (!pbo)
	    return;

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_VS, 1);
    R600_OUT_BATCH(r700->vs.SQ_PGM_START_VS.u32All);
    R600_OUT_BATCH_RELOC(r700->vs.SQ_PGM_START_VS.u32All,
		         pbo,
		         r700->vs.SQ_PGM_START_VS.u32All,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_VS, r700->vs.SQ_PGM_RESOURCES_VS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_VS, r700->vs.SQ_PGM_CF_OFFSET_VS.u32All);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    R600_OUT_BATCH_REGVAL((SQ_LOOP_CONST_0 + 32*4), 0x0100000F);
    //R600_OUT_BATCH_REGVAL((SQ_LOOP_CONST_0 + (SQ_LOOP_CONST_vs<2)), 0x0100000F);
    END_BATCH();

    COMMIT_BATCH();
}

static void r700SendFSState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_bo * pbo;
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	/* XXX fixme
	 * R6xx chips require a FS be emitted, even if it's not used.
	 * since we aren't using FS yet, just send the VS address to make
	 * the kernel command checker happy
	 */
	pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(GL_CONTEXT(context));
	r700->fs.SQ_PGM_START_FS.u32All = r700->vs.SQ_PGM_START_VS.u32All;
	r700->fs.SQ_PGM_RESOURCES_FS.u32All = 0;
	r700->fs.SQ_PGM_CF_OFFSET_FS.u32All = 0;
	/* XXX */

	if (!pbo)
		return;

	r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
	R600_OUT_BATCH_REGSEQ(SQ_PGM_START_FS, 1);
	R600_OUT_BATCH(r700->fs.SQ_PGM_START_FS.u32All);
	R600_OUT_BATCH_RELOC(r700->fs.SQ_PGM_START_FS.u32All,
			     pbo,
			     r700->fs.SQ_PGM_START_FS.u32All,
			     RADEON_GEM_DOMAIN_GTT, 0, 0);
	END_BATCH();

        BEGIN_BATCH_NO_AUTOSTATE(6);
	R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_FS, r700->fs.SQ_PGM_RESOURCES_FS.u32All);
	R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_FS, r700->fs.SQ_PGM_CF_OFFSET_FS.u32All);
        END_BATCH();

	COMMIT_BATCH();

}

static void r700SendViewportState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	int id = 0;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	if (id > R700_MAX_VIEWPORTS)
		return;

	if (!r700->viewport[id].enabled)
		return;

        BEGIN_BATCH_NO_AUTOSTATE(16);
	R600_OUT_BATCH_REGSEQ(PA_SC_VPORT_SCISSOR_0_TL + (8 * id), 2);
	R600_OUT_BATCH(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All);
	R600_OUT_BATCH_REGSEQ(PA_SC_VPORT_ZMIN_0 + (8 * id), 2);
	R600_OUT_BATCH(r700->viewport[id].PA_SC_VPORT_ZMIN_0.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_SC_VPORT_ZMAX_0.u32All);
	R600_OUT_BATCH_REGSEQ(PA_CL_VPORT_XSCALE_0 + (24 * id), 6);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_XSCALE.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_XOFFSET.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_YSCALE.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_YOFFSET.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_ZSCALE.u32All);
	R600_OUT_BATCH(r700->viewport[id].PA_CL_VPORT_ZOFFSET.u32All);
        END_BATCH();

	COMMIT_BATCH();

}

static void r700SendSQConfig(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

        BEGIN_BATCH_NO_AUTOSTATE(34);
	R600_OUT_BATCH_REGSEQ(SQ_CONFIG, 6);
	R600_OUT_BATCH(r700->sq_config.SQ_CONFIG.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_GPR_RESOURCE_MGMT_2.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_STACK_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_STACK_RESOURCE_MGMT_2.u32All);

	R600_OUT_BATCH_REGVAL(TA_CNTL_AUX, r700->TA_CNTL_AUX.u32All);
	R600_OUT_BATCH_REGVAL(VC_ENHANCE, r700->VC_ENHANCE.u32All);
	R600_OUT_BATCH_REGVAL(R7xx_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, r700->SQ_DYN_GPR_CNTL_PS_FLUSH_REQ.u32All);
	R600_OUT_BATCH_REGVAL(DB_DEBUG, r700->DB_DEBUG.u32All);
	R600_OUT_BATCH_REGVAL(DB_WATERMARKS, r700->DB_WATERMARKS.u32All);

	R600_OUT_BATCH_REGSEQ(SQ_ESGS_RING_ITEMSIZE, 9);
	R600_OUT_BATCH(r700->SQ_ESGS_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_GSVS_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_ESTMP_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_GSTMP_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_VSTMP_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_PSTMP_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_FBUF_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_REDUC_RING_ITEMSIZE.u32All);
	R600_OUT_BATCH(r700->SQ_GS_VERT_ITEMSIZE.u32All);
        END_BATCH();

	COMMIT_BATCH();
}

static void r700SendUCPState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	int i;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	for (i = 0; i < R700_MAX_UCP; i++) {
		if (r700->ucp[i].enabled) {
			BEGIN_BATCH_NO_AUTOSTATE(6);
			R600_OUT_BATCH_REGSEQ(PA_CL_UCP_0_X + (16 * i), 4);
			R600_OUT_BATCH(r700->ucp[i].PA_CL_UCP_0_X.u32All);
			R600_OUT_BATCH(r700->ucp[i].PA_CL_UCP_0_Y.u32All);
			R600_OUT_BATCH(r700->ucp[i].PA_CL_UCP_0_Z.u32All);
			R600_OUT_BATCH(r700->ucp[i].PA_CL_UCP_0_W.u32All);
			END_BATCH();
			COMMIT_BATCH();
		}
	}
}

static void r700SendSPIState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	unsigned int ui;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(59 + R700_MAX_SHADER_EXPORTS);

	R600_OUT_BATCH_REGSEQ(SQ_VTX_SEMANTIC_0, 32);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_0.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_1.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_2.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_3.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_4.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_5.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_6.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_7.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_8.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_9.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_10.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_11.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_12.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_13.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_14.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_15.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_16.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_17.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_18.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_19.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_20.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_21.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_22.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_23.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_24.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_25.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_26.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_27.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_28.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_29.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_30.u32All);
	R600_OUT_BATCH(r700->SQ_VTX_SEMANTIC_31.u32All);

	R600_OUT_BATCH_REGSEQ(SPI_VS_OUT_ID_0, 10);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_0.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_1.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_2.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_3.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_4.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_5.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_6.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_7.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_8.u32All);
	R600_OUT_BATCH(r700->SPI_VS_OUT_ID_9.u32All);

	R600_OUT_BATCH_REGSEQ(SPI_VS_OUT_CONFIG, 9);
	R600_OUT_BATCH(r700->SPI_VS_OUT_CONFIG.u32All);
	R600_OUT_BATCH(r700->SPI_THREAD_GROUPING.u32All);
	R600_OUT_BATCH(r700->SPI_PS_IN_CONTROL_0.u32All);
	R600_OUT_BATCH(r700->SPI_PS_IN_CONTROL_1.u32All);
	R600_OUT_BATCH(r700->SPI_INTERP_CONTROL_0.u32All);
	R600_OUT_BATCH(r700->SPI_INPUT_Z.u32All);
	R600_OUT_BATCH(r700->SPI_FOG_CNTL.u32All);
	R600_OUT_BATCH(r700->SPI_FOG_FUNC_SCALE.u32All);
	R600_OUT_BATCH(r700->SPI_FOG_FUNC_BIAS.u32All);

	R600_OUT_BATCH_REGSEQ(SPI_PS_INPUT_CNTL_0, R700_MAX_SHADER_EXPORTS);
	for(ui = 0; ui < R700_MAX_SHADER_EXPORTS; ui++)
		R600_OUT_BATCH(r700->SPI_PS_INPUT_CNTL[ui].u32All);

	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendVGTState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

        BEGIN_BATCH_NO_AUTOSTATE(41);

	R600_OUT_BATCH_REGSEQ(VGT_MAX_VTX_INDX, 4);
	R600_OUT_BATCH(r700->VGT_MAX_VTX_INDX.u32All);
	R600_OUT_BATCH(r700->VGT_MIN_VTX_INDX.u32All);
	R600_OUT_BATCH(r700->VGT_INDX_OFFSET.u32All);
	R600_OUT_BATCH(r700->VGT_MULTI_PRIM_IB_RESET_INDX.u32All);

	R600_OUT_BATCH_REGSEQ(VGT_OUTPUT_PATH_CNTL, 13);
	R600_OUT_BATCH(r700->VGT_OUTPUT_PATH_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_HOS_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_HOS_MAX_TESS_LEVEL.u32All);
	R600_OUT_BATCH(r700->VGT_HOS_MIN_TESS_LEVEL.u32All);
	R600_OUT_BATCH(r700->VGT_HOS_REUSE_DEPTH.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_PRIM_TYPE.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_FIRST_DECR.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_DECR.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_VECT_0_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_VECT_1_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_VECT_0_FMT_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_GROUP_VECT_1_FMT_CNTL.u32All);
	R600_OUT_BATCH(r700->VGT_GS_MODE.u32All);

	R600_OUT_BATCH_REGVAL(VGT_PRIMITIVEID_EN, r700->VGT_PRIMITIVEID_EN.u32All);
	R600_OUT_BATCH_REGVAL(VGT_MULTI_PRIM_IB_RESET_EN, r700->VGT_MULTI_PRIM_IB_RESET_EN.u32All);
	R600_OUT_BATCH_REGVAL(VGT_INSTANCE_STEP_RATE_0, r700->VGT_INSTANCE_STEP_RATE_0.u32All);
	R600_OUT_BATCH_REGVAL(VGT_INSTANCE_STEP_RATE_1, r700->VGT_INSTANCE_STEP_RATE_1.u32All);

	R600_OUT_BATCH_REGSEQ(VGT_STRMOUT_EN, 3);
	R600_OUT_BATCH(r700->VGT_STRMOUT_EN.u32All);
	R600_OUT_BATCH(r700->VGT_REUSE_OFF.u32All);
	R600_OUT_BATCH(r700->VGT_VTX_CNT_EN.u32All);

	R600_OUT_BATCH_REGVAL(VGT_STRMOUT_BUFFER_EN, r700->VGT_STRMOUT_BUFFER_EN.u32All);

	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendSXState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

        BEGIN_BATCH_NO_AUTOSTATE(9);
	R600_OUT_BATCH_REGVAL(SX_MISC, r700->SX_MISC.u32All);
	R600_OUT_BATCH_REGVAL(SX_ALPHA_TEST_CONTROL, r700->SX_ALPHA_TEST_CONTROL.u32All);
	R600_OUT_BATCH_REGVAL(SX_ALPHA_REF, r700->SX_ALPHA_REF.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendDBState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(17);

	R600_OUT_BATCH_REGSEQ(DB_STENCIL_CLEAR, 2);
	R600_OUT_BATCH(r700->DB_STENCIL_CLEAR.u32All);
	R600_OUT_BATCH(r700->DB_DEPTH_CLEAR.u32All);

	R600_OUT_BATCH_REGVAL(DB_DEPTH_CONTROL, r700->DB_DEPTH_CONTROL.u32All);
	R600_OUT_BATCH_REGVAL(DB_SHADER_CONTROL, r700->DB_SHADER_CONTROL.u32All);

	R600_OUT_BATCH_REGSEQ(DB_RENDER_CONTROL, 2);
	R600_OUT_BATCH(r700->DB_RENDER_CONTROL.u32All);
	R600_OUT_BATCH(r700->DB_RENDER_OVERRIDE.u32All);

	R600_OUT_BATCH_REGVAL(DB_ALPHA_TO_MASK, r700->DB_ALPHA_TO_MASK.u32All);

	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendStencilState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

        BEGIN_BATCH_NO_AUTOSTATE(4);
	R600_OUT_BATCH_REGSEQ(DB_STENCILREFMASK, 2);
	R600_OUT_BATCH(r700->DB_STENCILREFMASK.u32All);
	R600_OUT_BATCH(r700->DB_STENCILREFMASK_BF.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendCBState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770) {
		BEGIN_BATCH_NO_AUTOSTATE(11);
		R600_OUT_BATCH_REGSEQ(CB_CLEAR_RED, 4);
		R600_OUT_BATCH(r700->CB_CLEAR_RED_R6XX.u32All);
		R600_OUT_BATCH(r700->CB_CLEAR_GREEN_R6XX.u32All);
		R600_OUT_BATCH(r700->CB_CLEAR_BLUE_R6XX.u32All);
		R600_OUT_BATCH(r700->CB_CLEAR_ALPHA_R6XX.u32All);
		R600_OUT_BATCH_REGSEQ(CB_FOG_RED, 3);
		R600_OUT_BATCH(r700->CB_FOG_RED_R6XX.u32All);
		R600_OUT_BATCH(r700->CB_FOG_GREEN_R6XX.u32All);
		R600_OUT_BATCH(r700->CB_FOG_BLUE_R6XX.u32All);
		END_BATCH();
	}

	BEGIN_BATCH_NO_AUTOSTATE(7);
	R600_OUT_BATCH_REGSEQ(CB_TARGET_MASK, 2);
	R600_OUT_BATCH(r700->CB_TARGET_MASK.u32All);
	R600_OUT_BATCH(r700->CB_SHADER_MASK.u32All);
	R600_OUT_BATCH_REGVAL(R7xx_CB_SHADER_CONTROL, r700->CB_SHADER_CONTROL.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendCBCLRCMPState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(6);
	R600_OUT_BATCH_REGSEQ(CB_CLRCMP_CONTROL, 4);
	R600_OUT_BATCH(r700->CB_CLRCMP_CONTROL.u32All);
	R600_OUT_BATCH(r700->CB_CLRCMP_SRC.u32All);
	R600_OUT_BATCH(r700->CB_CLRCMP_DST.u32All);
	R600_OUT_BATCH(r700->CB_CLRCMP_MSK.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendCBBlendState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	unsigned int ui;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770) {
		BEGIN_BATCH_NO_AUTOSTATE(3);
		R600_OUT_BATCH_REGVAL(CB_BLEND_CONTROL, r700->CB_BLEND_CONTROL.u32All);
		END_BATCH();
	}

	BEGIN_BATCH_NO_AUTOSTATE(3);
	R600_OUT_BATCH_REGVAL(CB_COLOR_CONTROL, r700->CB_COLOR_CONTROL.u32All);
	END_BATCH();

	if (context->radeon.radeonScreen->chip_family > CHIP_FAMILY_R600) {
		for (ui = 0; ui < R700_MAX_RENDER_TARGETS; ui++) {
			if (r700->render_target[ui].enabled) {
				BEGIN_BATCH_NO_AUTOSTATE(3);
				R600_OUT_BATCH_REGVAL(CB_BLEND0_CONTROL + (4 * ui),
						      r700->render_target[ui].CB_BLEND0_CONTROL.u32All);
				END_BATCH();
			}
		}
	}

	COMMIT_BATCH();
}

static void r700SendCBBlendColorState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(6);
	R600_OUT_BATCH_REGSEQ(CB_BLEND_RED, 4);
	R600_OUT_BATCH(r700->CB_BLEND_RED.u32All);
	R600_OUT_BATCH(r700->CB_BLEND_GREEN.u32All);
	R600_OUT_BATCH(r700->CB_BLEND_BLUE.u32All);
	R600_OUT_BATCH(r700->CB_BLEND_ALPHA.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendSUState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(9);
	R600_OUT_BATCH_REGVAL(PA_SU_SC_MODE_CNTL, r700->PA_SU_SC_MODE_CNTL.u32All);
	R600_OUT_BATCH_REGSEQ(PA_SU_POINT_SIZE, 4);
	R600_OUT_BATCH(r700->PA_SU_POINT_SIZE.u32All);
	R600_OUT_BATCH(r700->PA_SU_POINT_MINMAX.u32All);
	R600_OUT_BATCH(r700->PA_SU_LINE_CNTL.u32All);
	R600_OUT_BATCH(r700->PA_SU_VTX_CNTL.u32All);
	END_BATCH();
	COMMIT_BATCH();

}

static void r700SendPolyState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(10);
	R600_OUT_BATCH_REGSEQ(PA_SU_POLY_OFFSET_DB_FMT_CNTL, 2);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_DB_FMT_CNTL.u32All);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_CLAMP.u32All);
	R600_OUT_BATCH_REGSEQ(PA_SU_POLY_OFFSET_FRONT_SCALE, 4);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_FRONT_SCALE.u32All);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_FRONT_OFFSET.u32All);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_BACK_SCALE.u32All);
	R600_OUT_BATCH(r700->PA_SU_POLY_OFFSET_BACK_OFFSET.u32All);
	END_BATCH();
	COMMIT_BATCH();

}

static void r700SendCLState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(12);
	R600_OUT_BATCH_REGVAL(PA_CL_CLIP_CNTL, r700->PA_CL_CLIP_CNTL.u32All);
	R600_OUT_BATCH_REGVAL(PA_CL_VTE_CNTL, r700->PA_CL_VTE_CNTL.u32All);
	R600_OUT_BATCH_REGVAL(PA_CL_VS_OUT_CNTL, r700->PA_CL_VS_OUT_CNTL.u32All);
	R600_OUT_BATCH_REGVAL(PA_CL_NANINF_CNTL, r700->PA_CL_NANINF_CNTL.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendGBState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(6);
	R600_OUT_BATCH_REGSEQ(PA_CL_GB_VERT_CLIP_ADJ, 4);
	R600_OUT_BATCH(r700->PA_CL_GB_VERT_CLIP_ADJ.u32All);
	R600_OUT_BATCH(r700->PA_CL_GB_VERT_DISC_ADJ.u32All);
	R600_OUT_BATCH(r700->PA_CL_GB_HORZ_CLIP_ADJ.u32All);
	R600_OUT_BATCH(r700->PA_CL_GB_HORZ_DISC_ADJ.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendScissorState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(22);
	R600_OUT_BATCH_REGSEQ(PA_SC_SCREEN_SCISSOR_TL, 2);
	R600_OUT_BATCH(r700->PA_SC_SCREEN_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_SCREEN_SCISSOR_BR.u32All);

	R600_OUT_BATCH_REGSEQ(PA_SC_WINDOW_OFFSET, 12);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_OFFSET.u32All);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_SCISSOR_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_RULE.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_0_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_0_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_1_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_1_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_2_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_2_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_3_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_3_BR.u32All);

	R600_OUT_BATCH_REGSEQ(PA_SC_GENERIC_SCISSOR_TL, 2);
	R600_OUT_BATCH(r700->PA_SC_GENERIC_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_GENERIC_SCISSOR_BR.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendSCState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	BEGIN_BATCH_NO_AUTOSTATE(15);
	R600_OUT_BATCH_REGVAL(R7xx_PA_SC_EDGERULE, r700->PA_SC_EDGERULE.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_LINE_STIPPLE, r700->PA_SC_LINE_STIPPLE.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_MPASS_PS_CNTL, r700->PA_SC_MPASS_PS_CNTL.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_MODE_CNTL, r700->PA_SC_MODE_CNTL.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_LINE_CNTL, r700->PA_SC_LINE_CNTL.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendAAState(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(12);
	R600_OUT_BATCH_REGVAL(PA_SC_AA_CONFIG, r700->PA_SC_AA_CONFIG.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_AA_SAMPLE_LOCS_MCTX, r700->PA_SC_AA_SAMPLE_LOCS_MCTX.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX, r700->PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX.u32All);
	R600_OUT_BATCH_REGVAL(PA_SC_AA_MASK, r700->PA_SC_AA_MASK.u32All);
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendPSConsts(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	int i;
	BATCH_LOCALS(&context->radeon);

	if (r700->ps.num_consts == 0)
		return;

	BEGIN_BATCH_NO_AUTOSTATE(2 + (r700->ps.num_consts * 4));
	R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_ALU_CONST, (r700->ps.num_consts * 4)));
	/* assembler map const from very beginning. */
	R600_OUT_BATCH(SQ_ALU_CONSTANT_PS_OFFSET * 4);
	for (i = 0; i < r700->ps.num_consts; i++) {
		R600_OUT_BATCH(r700->ps.consts[i][0].u32All);
		R600_OUT_BATCH(r700->ps.consts[i][1].u32All);
		R600_OUT_BATCH(r700->ps.consts[i][2].u32All);
		R600_OUT_BATCH(r700->ps.consts[i][3].u32All);
	}
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendVSConsts(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	int i;
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	if (r700->vs.num_consts == 0)
		return;

	BEGIN_BATCH_NO_AUTOSTATE(2 + (r700->vs.num_consts * 4));
	R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_ALU_CONST, (r700->vs.num_consts * 4)));
	/* assembler map const from very beginning. */
	R600_OUT_BATCH(SQ_ALU_CONSTANT_VS_OFFSET * 4);
	for (i = 0; i < r700->vs.num_consts; i++) {
		R600_OUT_BATCH(r700->vs.consts[i][0].u32All);
		R600_OUT_BATCH(r700->vs.consts[i][1].u32All);
		R600_OUT_BATCH(r700->vs.consts[i][2].u32All);
		R600_OUT_BATCH(r700->vs.consts[i][3].u32All);
	}
	END_BATCH();
	COMMIT_BATCH();
}

static void r700SendQueryBegin(GLcontext *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;
	BATCH_LOCALS(radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	/* clear the buffer */
	radeon_bo_map(query->bo, GL_FALSE);
	memset(query->bo->ptr, 0, 4 * 2 * sizeof(uint64_t)); /* 4 DBs, 2 qwords each */
	radeon_bo_unmap(query->bo);

	radeon_cs_space_check_with_bo(radeon->cmdbuf.cs,
				      query->bo,
				      0, RADEON_GEM_DOMAIN_GTT);

	BEGIN_BATCH_NO_AUTOSTATE(4 + 2);
	R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 2));
	R600_OUT_BATCH(ZPASS_DONE);
	R600_OUT_BATCH(query->curr_offset); /* hw writes qwords */
	R600_OUT_BATCH(0x00000000);
	R600_OUT_BATCH_RELOC(VGT_EVENT_INITIATOR, query->bo, 0, 0, RADEON_GEM_DOMAIN_GTT, 0);
	END_BATCH();
	query->emitted_begin = GL_TRUE;
}

static int check_always(GLcontext *ctx, struct radeon_state_atom *atom)
{
	return atom->cmd_size;
}

static int check_cb(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	int count = 7;

	if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
		count += 11;
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);

	return count;
}

static int check_blnd(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	unsigned int ui;
	int count = 3;

	if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
		count += 3;

	if (context->radeon.radeonScreen->chip_family > CHIP_FAMILY_R600) {
		/* targets are enabled in r700SetRenderTarget but state
		   size is calculated before that. Until MRT's are done
		   hardcode target0 as enabled. */
		count += 3;
		for (ui = 1; ui < R700_MAX_RENDER_TARGETS; ui++) {
                        if (r700->render_target[ui].enabled)
				count += 3;
		}
	}
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);

	return count;
}

static int check_ucp(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	int i;
	int count = 0;

	for (i = 0; i < R700_MAX_UCP; i++) {
		if (r700->ucp[i].enabled)
			count += 6;
	}
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count;
}

static int check_vtx(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	int count = context->radeon.tcl.aos_count * 18;

	if (count)
		count += 6;

	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count;
}

static int check_tx(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	unsigned int i, count = 0;
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			radeonTexObj *t = r700->textures[i];
			if (t)
				count++;
		}
	}
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count * 31;
}

static int check_ps_consts(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	int count = r700->ps.num_consts * 4;

	if (count)
		count += 2;
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);

	return count;
}

static int check_vs_consts(GLcontext *ctx, struct radeon_state_atom *atom)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	int count = r700->vs.num_consts * 4;

	if (count)
		count += 2;
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);

	return count;
}

static int check_queryobj(GLcontext *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;
	int count;

	if (!query || query->emitted_begin)
		count = 0;
	else
		count = atom->cmd_size;
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count;
}

#define ALLOC_STATE( ATOM, CHK, SZ, EMIT )				\
do {									\
	context->atoms.ATOM.cmd_size = (SZ);				\
	context->atoms.ATOM.cmd = NULL;					\
	context->atoms.ATOM.name = #ATOM;				\
	context->atoms.ATOM.idx = 0;					\
	context->atoms.ATOM.check = check_##CHK;			\
	context->atoms.ATOM.dirty = GL_FALSE;				\
	context->atoms.ATOM.emit = (EMIT);				\
	context->radeon.hw.max_state_size += (SZ);			\
	insert_at_tail(&context->radeon.hw.atomlist, &context->atoms.ATOM); \
} while (0)

static void r600_init_query_stateobj(radeonContextPtr radeon, int SZ)
{
	radeon->query.queryobj.cmd_size = (SZ);
	radeon->query.queryobj.cmd = NULL;
	radeon->query.queryobj.name = "queryobj";
	radeon->query.queryobj.idx = 0;
	radeon->query.queryobj.check = check_queryobj;
	radeon->query.queryobj.dirty = GL_FALSE;
	radeon->query.queryobj.emit = r700SendQueryBegin;
	radeon->hw.max_state_size += (SZ);
	insert_at_tail(&radeon->hw.atomlist, &radeon->query.queryobj);
}

void r600InitAtoms(context_t *context)
{
	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s %p\n", __func__, context);
	context->radeon.hw.max_state_size = 10 + 5 + 14; /* start 3d, idle, cb/db flush */

	/* Setup the atom linked list */
	make_empty_list(&context->radeon.hw.atomlist);
	context->radeon.hw.atomlist.name = "atom-list";

	ALLOC_STATE(sq, always, 34, r700SendSQConfig);
	ALLOC_STATE(db, always, 17, r700SendDBState);
	ALLOC_STATE(stencil, always, 4, r700SendStencilState);
	ALLOC_STATE(db_target, always, 12, r700SendDepthTargetState);
	ALLOC_STATE(sc, always, 15, r700SendSCState);
	ALLOC_STATE(scissor, always, 22, r700SendScissorState);
	ALLOC_STATE(aa, always, 12, r700SendAAState);
	ALLOC_STATE(cl, always, 12, r700SendCLState);
	ALLOC_STATE(gb, always, 6, r700SendGBState);
	ALLOC_STATE(ucp, ucp, (R700_MAX_UCP * 6), r700SendUCPState);
	ALLOC_STATE(su, always, 9, r700SendSUState);
	ALLOC_STATE(poly, always, 10, r700SendPolyState);
	ALLOC_STATE(cb, cb, 18, r700SendCBState);
	ALLOC_STATE(clrcmp, always, 6, r700SendCBCLRCMPState);
	ALLOC_STATE(cb_target, always, 29, r700SendRenderTargetState);
	ALLOC_STATE(blnd, blnd, (6 + (R700_MAX_RENDER_TARGETS * 3)), r700SendCBBlendState);
	ALLOC_STATE(blnd_clr, always, 6, r700SendCBBlendColorState);
	ALLOC_STATE(sx, always, 9, r700SendSXState);
	ALLOC_STATE(vgt, always, 41, r700SendVGTState);
	ALLOC_STATE(spi, always, (59 + R700_MAX_SHADER_EXPORTS), r700SendSPIState);
	ALLOC_STATE(vpt, always, 16, r700SendViewportState);
	ALLOC_STATE(fs, always, 18, r700SendFSState);
	ALLOC_STATE(vs, always, 21, r700SendVSState);
	ALLOC_STATE(ps, always, 24, r700SendPSState);
	ALLOC_STATE(vs_consts, vs_consts, (2 + (R700_MAX_DX9_CONSTS * 4)), r700SendVSConsts);
	ALLOC_STATE(ps_consts, ps_consts, (2 + (R700_MAX_DX9_CONSTS * 4)), r700SendPSConsts);
	ALLOC_STATE(vtx, vtx, (6 + (VERT_ATTRIB_MAX * 18)), r700SendVTXState);
	ALLOC_STATE(tx, tx, (R700_TEXTURE_NUMBERUNITS * 20), r700SendTexState);
	ALLOC_STATE(tx_smplr, tx, (R700_TEXTURE_NUMBERUNITS * 5), r700SendTexSamplerState);
	ALLOC_STATE(tx_brdr_clr, tx, (R700_TEXTURE_NUMBERUNITS * 6), r700SendTexBorderColorState);
	r600_init_query_stateobj(&context->radeon, 6 * 2);

	context->radeon.hw.is_dirty = GL_TRUE;
	context->radeon.hw.all_dirty = GL_TRUE;
}
