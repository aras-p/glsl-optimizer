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

#include "r600_context.h"
#include "r600_cmdbuf.h"

#include "r700_state.h"
#include "r600_tex.h"
#include "r700_oglprog.h"
#include "r700_fragprog.h"
#include "r700_vertprog.h"
#include "r700_ioctl.h"

#include "radeon_mipmap_tree.h"

#define LINK_STATES(reg)                                            \
do                                                                  \
{                                                                   \
    pStateListWork->puiValue = (unsigned int*)&(r700->reg);         \
    pStateListWork->unOffset = mm##reg - ASIC_CONTEXT_BASE_INDEX; \
    pStateListWork->pNext    = pStateListWork + 1;                  \
    pStateListWork++;                                               \
}while(0)

GLboolean r700InitChipObject(context_t *context)
{
    ContextState * pStateListWork;

    R700_CHIP_CONTEXT *r700 = &context->hw;

    /* init state list */
    r700->pStateList = (ContextState*) MALLOC (sizeof(ContextState)*sizeof(R700_CHIP_CONTEXT)/sizeof(unsigned int));
    pStateListWork = r700->pStateList;

    // misc
    LINK_STATES(TA_CNTL_AUX);
    LINK_STATES(VC_ENHANCE);
    LINK_STATES(SQ_DYN_GPR_CNTL_PS_FLUSH_REQ);
    LINK_STATES(DB_DEBUG);
    LINK_STATES(DB_WATERMARKS);

    // SC
    LINK_STATES(PA_SC_SCREEN_SCISSOR_TL);
    LINK_STATES(PA_SC_SCREEN_SCISSOR_BR);
    LINK_STATES(PA_SC_WINDOW_OFFSET);
    LINK_STATES(PA_SC_WINDOW_SCISSOR_TL);
    LINK_STATES(PA_SC_WINDOW_SCISSOR_BR);
    LINK_STATES(PA_SC_CLIPRECT_RULE);
    LINK_STATES(PA_SC_CLIPRECT_0_TL);
    LINK_STATES(PA_SC_CLIPRECT_0_BR);
    LINK_STATES(PA_SC_CLIPRECT_1_TL);
    LINK_STATES(PA_SC_CLIPRECT_1_BR);
    LINK_STATES(PA_SC_CLIPRECT_2_TL);
    LINK_STATES(PA_SC_CLIPRECT_2_BR);
    LINK_STATES(PA_SC_CLIPRECT_3_TL);
    LINK_STATES(PA_SC_CLIPRECT_3_BR);
    LINK_STATES(PA_SC_EDGERULE);
    LINK_STATES(PA_SC_GENERIC_SCISSOR_TL);
    LINK_STATES(PA_SC_GENERIC_SCISSOR_BR);
    LINK_STATES(PA_SC_LINE_STIPPLE);
    LINK_STATES(PA_SC_MPASS_PS_CNTL);
    LINK_STATES(PA_SC_MODE_CNTL);
    LINK_STATES(PA_SC_LINE_CNTL);
    LINK_STATES(PA_SC_AA_CONFIG);
    LINK_STATES(PA_SC_AA_SAMPLE_LOCS_MCTX);
    LINK_STATES(PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX);
    LINK_STATES(PA_SC_AA_MASK);

    // SU
    LINK_STATES(PA_SU_POINT_SIZE);
    LINK_STATES(PA_SU_POINT_MINMAX);
    LINK_STATES(PA_SU_LINE_CNTL);
    LINK_STATES(PA_SU_SC_MODE_CNTL);
    LINK_STATES(PA_SU_VTX_CNTL);
    LINK_STATES(PA_SU_POLY_OFFSET_DB_FMT_CNTL);
    LINK_STATES(PA_SU_POLY_OFFSET_CLAMP);
    LINK_STATES(PA_SU_POLY_OFFSET_FRONT_SCALE);
    LINK_STATES(PA_SU_POLY_OFFSET_FRONT_OFFSET);
    LINK_STATES(PA_SU_POLY_OFFSET_BACK_SCALE);
    LINK_STATES(PA_SU_POLY_OFFSET_BACK_OFFSET);

    // CL
    LINK_STATES(PA_CL_CLIP_CNTL);
    LINK_STATES(PA_CL_VTE_CNTL);
    LINK_STATES(PA_CL_VS_OUT_CNTL);
    LINK_STATES(PA_CL_NANINF_CNTL);
    LINK_STATES(PA_CL_GB_VERT_CLIP_ADJ);
    LINK_STATES(PA_CL_GB_VERT_DISC_ADJ);
    LINK_STATES(PA_CL_GB_HORZ_CLIP_ADJ);
    LINK_STATES(PA_CL_GB_HORZ_DISC_ADJ);

    // CB
    LINK_STATES(CB_CLEAR_RED_R6XX);
    LINK_STATES(CB_CLEAR_GREEN_R6XX);
    LINK_STATES(CB_CLEAR_BLUE_R6XX);
    LINK_STATES(CB_CLEAR_ALPHA_R6XX);
    LINK_STATES(CB_TARGET_MASK);
    LINK_STATES(CB_SHADER_MASK);
    LINK_STATES(CB_BLEND_RED);
    LINK_STATES(CB_BLEND_GREEN);
    LINK_STATES(CB_BLEND_BLUE);
    LINK_STATES(CB_BLEND_ALPHA);
    LINK_STATES(CB_FOG_RED_R6XX);
    LINK_STATES(CB_FOG_GREEN_R6XX);
    LINK_STATES(CB_FOG_BLUE_R6XX);
    LINK_STATES(CB_SHADER_CONTROL);
    LINK_STATES(CB_COLOR_CONTROL);
    LINK_STATES(CB_CLRCMP_CONTROL);
    LINK_STATES(CB_CLRCMP_SRC);
    LINK_STATES(CB_CLRCMP_DST);
    LINK_STATES(CB_CLRCMP_MSK);
    LINK_STATES(CB_BLEND_CONTROL);

    //DB
    LINK_STATES(DB_HTILE_DATA_BASE);
    LINK_STATES(DB_STENCIL_CLEAR);
    LINK_STATES(DB_DEPTH_CLEAR);
    LINK_STATES(DB_STENCILREFMASK);
    LINK_STATES(DB_STENCILREFMASK_BF);
    LINK_STATES(DB_DEPTH_CONTROL);
    LINK_STATES(DB_SHADER_CONTROL);
    LINK_STATES(DB_RENDER_CONTROL);
    LINK_STATES(DB_RENDER_OVERRIDE);
    LINK_STATES(DB_HTILE_SURFACE);
    LINK_STATES(DB_ALPHA_TO_MASK);

    // SX
    LINK_STATES(SX_MISC);
    LINK_STATES(SX_ALPHA_TEST_CONTROL);
    LINK_STATES(SX_ALPHA_REF);

    // VGT
    LINK_STATES(VGT_MAX_VTX_INDX);
    LINK_STATES(VGT_MIN_VTX_INDX);
    LINK_STATES(VGT_INDX_OFFSET);
    LINK_STATES(VGT_MULTI_PRIM_IB_RESET_INDX);
    LINK_STATES(VGT_OUTPUT_PATH_CNTL);
    LINK_STATES(VGT_HOS_CNTL);
    LINK_STATES(VGT_HOS_MAX_TESS_LEVEL);
    LINK_STATES(VGT_HOS_MIN_TESS_LEVEL);
    LINK_STATES(VGT_HOS_REUSE_DEPTH);
    LINK_STATES(VGT_GROUP_PRIM_TYPE);
    LINK_STATES(VGT_GROUP_FIRST_DECR);
    LINK_STATES(VGT_GROUP_DECR);
    LINK_STATES(VGT_GROUP_VECT_0_CNTL);
    LINK_STATES(VGT_GROUP_VECT_1_CNTL);
    LINK_STATES(VGT_GROUP_VECT_0_FMT_CNTL);
    LINK_STATES(VGT_GROUP_VECT_1_FMT_CNTL);
    LINK_STATES(VGT_GS_MODE);
    LINK_STATES(VGT_PRIMITIVEID_EN);
    LINK_STATES(VGT_MULTI_PRIM_IB_RESET_EN);
    LINK_STATES(VGT_INSTANCE_STEP_RATE_0);
    LINK_STATES(VGT_INSTANCE_STEP_RATE_1);
    LINK_STATES(VGT_STRMOUT_EN);
    LINK_STATES(VGT_REUSE_OFF);
    LINK_STATES(VGT_VTX_CNT_EN);
    LINK_STATES(VGT_STRMOUT_BUFFER_EN);

    LINK_STATES(SQ_VTX_SEMANTIC_0);
    LINK_STATES(SQ_VTX_SEMANTIC_1);
    LINK_STATES(SQ_VTX_SEMANTIC_2);
    LINK_STATES(SQ_VTX_SEMANTIC_3);
    LINK_STATES(SQ_VTX_SEMANTIC_4);
    LINK_STATES(SQ_VTX_SEMANTIC_5);
    LINK_STATES(SQ_VTX_SEMANTIC_6);
    LINK_STATES(SQ_VTX_SEMANTIC_7);
    LINK_STATES(SQ_VTX_SEMANTIC_8);
    LINK_STATES(SQ_VTX_SEMANTIC_9);
    LINK_STATES(SQ_VTX_SEMANTIC_10);
    LINK_STATES(SQ_VTX_SEMANTIC_11);
    LINK_STATES(SQ_VTX_SEMANTIC_12);
    LINK_STATES(SQ_VTX_SEMANTIC_13);
    LINK_STATES(SQ_VTX_SEMANTIC_14);
    LINK_STATES(SQ_VTX_SEMANTIC_15);
    LINK_STATES(SQ_VTX_SEMANTIC_16);
    LINK_STATES(SQ_VTX_SEMANTIC_17);
    LINK_STATES(SQ_VTX_SEMANTIC_18);
    LINK_STATES(SQ_VTX_SEMANTIC_19);
    LINK_STATES(SQ_VTX_SEMANTIC_20);
    LINK_STATES(SQ_VTX_SEMANTIC_21);
    LINK_STATES(SQ_VTX_SEMANTIC_22);
    LINK_STATES(SQ_VTX_SEMANTIC_23);
    LINK_STATES(SQ_VTX_SEMANTIC_24);
    LINK_STATES(SQ_VTX_SEMANTIC_25);
    LINK_STATES(SQ_VTX_SEMANTIC_26);
    LINK_STATES(SQ_VTX_SEMANTIC_27);
    LINK_STATES(SQ_VTX_SEMANTIC_28);
    LINK_STATES(SQ_VTX_SEMANTIC_29);
    LINK_STATES(SQ_VTX_SEMANTIC_30);
    LINK_STATES(SQ_VTX_SEMANTIC_31);

    // SPI
    LINK_STATES(SPI_VS_OUT_ID_0);
    LINK_STATES(SPI_VS_OUT_ID_1);
    LINK_STATES(SPI_VS_OUT_ID_2);
    LINK_STATES(SPI_VS_OUT_ID_3);
    LINK_STATES(SPI_VS_OUT_ID_4);
    LINK_STATES(SPI_VS_OUT_ID_5);
    LINK_STATES(SPI_VS_OUT_ID_6);
    LINK_STATES(SPI_VS_OUT_ID_7);
    LINK_STATES(SPI_VS_OUT_ID_8);
    LINK_STATES(SPI_VS_OUT_ID_9);

    LINK_STATES(SPI_VS_OUT_CONFIG);
    LINK_STATES(SPI_THREAD_GROUPING);
    LINK_STATES(SPI_PS_IN_CONTROL_0);
    LINK_STATES(SPI_PS_IN_CONTROL_1);
    LINK_STATES(SPI_INTERP_CONTROL_0);
    LINK_STATES(SPI_INPUT_Z);
    LINK_STATES(SPI_FOG_CNTL);
    LINK_STATES(SPI_FOG_FUNC_SCALE);
    LINK_STATES(SPI_FOG_FUNC_BIAS);

    // SQ
    LINK_STATES(SQ_ESGS_RING_ITEMSIZE);
    LINK_STATES(SQ_GSVS_RING_ITEMSIZE);
    LINK_STATES(SQ_ESTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_GSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_VSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_PSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_FBUF_RING_ITEMSIZE);
    LINK_STATES(SQ_REDUC_RING_ITEMSIZE);
    //LINK_STATES(SQ_GS_VERT_ITEMSIZE);

    pStateListWork->puiValue = (unsigned int*)&(r700->SQ_GS_VERT_ITEMSIZE);
    pStateListWork->unOffset = mmSQ_GS_VERT_ITEMSIZE - ASIC_CONTEXT_BASE_INDEX;
    pStateListWork->pNext    = NULL;  /* END OF STATE LIST */

    return GL_TRUE;
}

GLboolean r700SendTextureState(context_t *context)
{
    unsigned int i;
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
    struct radeon_bo *bo = NULL;
    BATCH_LOCALS(&context->radeon);

    for (i=0; i<R700_TEXTURE_NUMBERUNITS; i++) {
	    radeonTexObj *t = r700->textures[i];
	    if (t) {
		    if (!t->image_override)
			    bo = t->mt->bo;
		    else
			    bo = t->bo;
		    if (bo) {

			    r700SyncSurf(context, bo,
					 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
					 0, TC_ACTION_ENA_bit);

			    BEGIN_BATCH_NO_AUTOSTATE(9 + 4);
			    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
			    R600_OUT_BATCH(i * 7);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE0);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE1);
			    R600_OUT_BATCH(0); /* r700->textures[i]->SQ_TEX_RESOURCE2 */
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE3);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE4);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE5);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE6);
			    R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE2,
						 bo,
						 0,
						 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
			    R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE3,
						 bo,
						 r700->textures[i]->SQ_TEX_RESOURCE3,
						 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
			    END_BATCH();

			    BEGIN_BATCH_NO_AUTOSTATE(5);
			    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_SAMPLER, 3));
			    R600_OUT_BATCH(i * 3);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER0);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER1);
			    R600_OUT_BATCH(r700->textures[i]->SQ_TEX_SAMPLER2);
			    END_BATCH();

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
    return GL_TRUE;
}

void r700SetupVTXConstants(GLcontext  * ctx,
			   unsigned int nStreamID,
			   void *       pAos,
			   unsigned int size,      /* number of elements in vector */
			   unsigned int stride,
			   unsigned int count)     /* number of vectors in stream */
{
    context_t *context = R700_CONTEXT(ctx);
    struct radeon_aos * paos = (struct radeon_aos *)pAos;
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
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV710))
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, TC_ACTION_ENA_bit);
    else
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, VC_ACTION_ENA_bit);

    uSQ_VTX_CONSTANT_WORD0_0 = paos->offset;
    uSQ_VTX_CONSTANT_WORD1_0 = count * (size * 4) - 1;

    SETfield(uSQ_VTX_CONSTANT_WORD2_0, 0, BASE_ADDRESS_HI_shift, BASE_ADDRESS_HI_mask); /* TODO */
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, stride, SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift,
	     SQ_VTX_CONSTANT_WORD2_0__STRIDE_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, GetSurfaceFormat(GL_FLOAT, size, NULL),
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift,
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_mask); /* TODO : trace back api for initial data type, not only GL_FLOAT */
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, SQ_NUM_FORMAT_SCALED,
	     SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift, SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_mask);
    SETbit(uSQ_VTX_CONSTANT_WORD2_0, SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit);

    SETfield(uSQ_VTX_CONSTANT_WORD3_0, 1, MEM_REQUEST_SIZE_shift, MEM_REQUEST_SIZE_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD6_0, SQ_TEX_VTX_VALID_BUFFER,
	     SQ_TEX_RESOURCE_WORD6_0__TYPE_shift, SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

    BEGIN_BATCH_NO_AUTOSTATE(9 + 2);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
    R600_OUT_BATCH((nStreamID + SQ_FETCH_RESOURCE_VS_OFFSET) * FETCH_RESOURCE_STRIDE);
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

int r700SetupStreams(GLcontext * ctx)
{
    context_t         *context = R700_CONTEXT(ctx);
    BATCH_LOCALS(&context->radeon);

    struct r700_vertex_program *vpc
             = (struct r700_vertex_program *)ctx->VertexProgram._Current;

    TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

    unsigned int unBit;
    unsigned int i, j = 0;

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();
    COMMIT_BATCH();

	for(i=0; i<VERT_ATTRIB_MAX; i++)
	{
		unBit = 1 << i;
		if(vpc->mesa_program.Base.InputsRead & unBit)
		{
			rcommon_emit_vector(ctx,
					    &context->radeon.tcl.aos[j],
					    vb->AttribPtr[i]->data,
					    vb->AttribPtr[i]->size,
					    vb->AttribPtr[i]->stride,
					    vb->Count);

			/* currently aos are packed */
			r700SetupVTXConstants(ctx,
					      i,
					      (void*)(&context->radeon.tcl.aos[j]),
					      (unsigned int)context->radeon.tcl.aos[j].components,
					      (unsigned int)context->radeon.tcl.aos[j].stride * 4,
					      (unsigned int)context->radeon.tcl.aos[j].count);
			j++;
		}
	}
	context->radeon.tcl.aos_count = j;

    return R600_FALLBACK_NONE;
}

GLboolean r700SendContextStates(context_t *context)
{
    BATCH_LOCALS(&context->radeon);

    R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);

    ContextState * pState = r700->pStateList;
    ContextState * pInit;
    unsigned int   toSend;
    unsigned int   ui;

    while(NULL != pState)
    {
        toSend = 1;

        pInit = pState;

	while(NULL != pState->pNext)
	{
                if ((pState->pNext->unOffset - pState->unOffset) > 1)
                {
			break;
                }
                else
                {
			pState = pState->pNext;
			toSend++;
                }
	}

        pState = pState->pNext;

        BEGIN_BATCH_NO_AUTOSTATE(toSend + 2);
        R600_OUT_BATCH_REGSEQ(((pInit->unOffset + ASIC_CONTEXT_BASE_INDEX)<<2), toSend);
        for(ui=0; ui<toSend; ui++)
        {
                R600_OUT_BATCH(*(pInit->puiValue));
		pInit = pInit->pNext;
        };
        END_BATCH();
    };

    /* todo:
     * - split this into a separate function?
     * - only emit the ones we use
     */
    BEGIN_BATCH_NO_AUTOSTATE(2 + R700_MAX_SHADER_EXPORTS);
    R600_OUT_BATCH_REGSEQ(SPI_PS_INPUT_CNTL_0, R700_MAX_SHADER_EXPORTS);
    for(ui = 0; ui < R700_MAX_SHADER_EXPORTS; ui++)
	    R600_OUT_BATCH(r700->SPI_PS_INPUT_CNTL[ui].u32All);
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

    return GL_TRUE;
}

GLboolean r700SendDepthTargetState(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);

	rrb = radeon_get_depthbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		fprintf(stderr, "no rrb\n");
		return GL_FALSE;
	}

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

	r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
		     DB_ACTION_ENA_bit | DB_DEST_BASE_ENA_bit);

	return GL_TRUE;
}

GLboolean r700SendRenderTargetState(context_t *context, int id)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);

	rrb = radeon_get_colorbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		fprintf(stderr, "no rrb\n");
		return GL_FALSE;
	}

	if (id > R700_MAX_RENDER_TARGETS)
		return GL_FALSE;

	if (!r700->render_target[id].enabled)
		return GL_FALSE;

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

        BEGIN_BATCH_NO_AUTOSTATE(18);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_SIZE + (4 * id), r700->render_target[id].CB_COLOR0_SIZE.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_VIEW + (4 * id), r700->render_target[id].CB_COLOR0_VIEW.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_INFO + (4 * id), r700->render_target[id].CB_COLOR0_INFO.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_TILE + (4 * id), r700->render_target[id].CB_COLOR0_TILE.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_FRAG + (4 * id), r700->render_target[id].CB_COLOR0_FRAG.u32All);
	R600_OUT_BATCH_REGVAL(CB_COLOR0_MASK + (4 * id), r700->render_target[id].CB_COLOR0_MASK.u32All);
        END_BATCH();

	COMMIT_BATCH();

	r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
		     CB_ACTION_ENA_bit | (1 << (id + 6)));

	return GL_TRUE;
}

GLboolean r700SendPSState(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_bo * pbo;
	BATCH_LOCALS(&context->radeon);

	pbo = (struct radeon_bo *)r700GetActiveFpShaderBo(GL_CONTEXT(context));

	if (!pbo)
		return GL_FALSE;

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

	COMMIT_BATCH();

	return GL_TRUE;
}

GLboolean r700SendVSState(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_bo * pbo;
	BATCH_LOCALS(&context->radeon);

	pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(GL_CONTEXT(context));

	if (!pbo)
		return GL_FALSE;

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

	COMMIT_BATCH();

	return GL_TRUE;
}

GLboolean r700SendFSState(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	struct radeon_bo * pbo;
	BATCH_LOCALS(&context->radeon);

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
		return GL_FALSE;

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

	return GL_TRUE;
}

GLboolean r700SendViewportState(context_t *context, int id)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

	if (id > R700_MAX_VIEWPORTS)
		return GL_FALSE;

	if (!r700->viewport[id].enabled)
		return GL_FALSE;

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

	return GL_TRUE;
}

GLboolean r700SendSQConfig(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);

        BEGIN_BATCH_NO_AUTOSTATE(8);
	R600_OUT_BATCH_REGSEQ(SQ_CONFIG, 6);
	R600_OUT_BATCH(r700->sq_config.SQ_CONFIG.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_GPR_RESOURCE_MGMT_2.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_STACK_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(r700->sq_config.SQ_STACK_RESOURCE_MGMT_2.u32All);
        END_BATCH();
	COMMIT_BATCH();

	return GL_TRUE;
}

GLboolean r700SendUCPState(context_t *context)
{
	R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);
	BATCH_LOCALS(&context->radeon);
	int i;

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

	return GL_TRUE;
}

