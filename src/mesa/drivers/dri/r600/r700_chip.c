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
#include "r700_tex.h"
#include "r700_oglprog.h"
#include "r700_fragprog.h"
#include "r700_vertprog.h"
#include "r700_ioctl.h"

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

    LINK_STATES(DB_DEPTH_SIZE);  
    LINK_STATES(DB_DEPTH_VIEW);  

    LINK_STATES(DB_DEPTH_BASE);  
    LINK_STATES(DB_DEPTH_INFO);  
    LINK_STATES(DB_HTILE_DATA_BASE);

    LINK_STATES(DB_STENCIL_CLEAR);
    LINK_STATES(DB_DEPTH_CLEAR);  

    LINK_STATES(PA_SC_SCREEN_SCISSOR_TL);  
    LINK_STATES(PA_SC_SCREEN_SCISSOR_BR);  

    LINK_STATES(CB_COLOR0_BASE);  

    LINK_STATES(CB_COLOR0_SIZE);  

    LINK_STATES(CB_COLOR0_VIEW);  

    LINK_STATES(CB_COLOR0_INFO); 
    LINK_STATES(CB_COLOR1_INFO);
    LINK_STATES(CB_COLOR2_INFO);
    LINK_STATES(CB_COLOR3_INFO);
    LINK_STATES(CB_COLOR4_INFO);
    LINK_STATES(CB_COLOR5_INFO);
    LINK_STATES(CB_COLOR6_INFO);
    LINK_STATES(CB_COLOR7_INFO);

    LINK_STATES(CB_COLOR0_TILE);  

    LINK_STATES(CB_COLOR0_FRAG);  

    LINK_STATES(CB_COLOR0_MASK);  

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

    LINK_STATES(CB_TARGET_MASK);  
    LINK_STATES(CB_SHADER_MASK);  
    LINK_STATES(PA_SC_GENERIC_SCISSOR_TL);  
    LINK_STATES(PA_SC_GENERIC_SCISSOR_BR);  

    LINK_STATES(PA_SC_VPORT_SCISSOR_0_TL);  
    LINK_STATES(PA_SC_VPORT_SCISSOR_0_BR);  
    LINK_STATES(PA_SC_VPORT_SCISSOR_1_TL);  
    LINK_STATES(PA_SC_VPORT_SCISSOR_1_BR);  

    LINK_STATES(PA_SC_VPORT_ZMIN_0);
    LINK_STATES(PA_SC_VPORT_ZMAX_0);  

    LINK_STATES(SX_MISC);  

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

    LINK_STATES(VGT_MAX_VTX_INDX);  
    LINK_STATES(VGT_MIN_VTX_INDX);  
    LINK_STATES(VGT_INDX_OFFSET);  
    LINK_STATES(VGT_MULTI_PRIM_IB_RESET_INDX);
    LINK_STATES(SX_ALPHA_TEST_CONTROL); 
    
    LINK_STATES(CB_BLEND_RED);  
    LINK_STATES(CB_BLEND_GREEN);
    LINK_STATES(CB_BLEND_BLUE); 
    LINK_STATES(CB_BLEND_ALPHA);

    LINK_STATES(PA_CL_VPORT_XSCALE);  
    LINK_STATES(PA_CL_VPORT_XOFFSET);  
    LINK_STATES(PA_CL_VPORT_YSCALE);  
    LINK_STATES(PA_CL_VPORT_YOFFSET);  
    LINK_STATES(PA_CL_VPORT_ZSCALE);  
    LINK_STATES(PA_CL_VPORT_ZOFFSET);  

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

    LINK_STATES(SPI_PS_INPUT_CNTL_0);  
    LINK_STATES(SPI_PS_INPUT_CNTL_1);  
    LINK_STATES(SPI_PS_INPUT_CNTL_2);  
    LINK_STATES(SPI_PS_INPUT_CNTL_3); 
    LINK_STATES(SPI_PS_INPUT_CNTL_4);
    LINK_STATES(SPI_PS_INPUT_CNTL_5); 
    LINK_STATES(SPI_PS_INPUT_CNTL_6); 
    LINK_STATES(SPI_PS_INPUT_CNTL_7); 
    LINK_STATES(SPI_PS_INPUT_CNTL_8); 
    LINK_STATES(SPI_PS_INPUT_CNTL_9); 
    LINK_STATES(SPI_PS_INPUT_CNTL_10);
    LINK_STATES(SPI_PS_INPUT_CNTL_11);
    LINK_STATES(SPI_PS_INPUT_CNTL_12);
    LINK_STATES(SPI_PS_INPUT_CNTL_13);
    LINK_STATES(SPI_PS_INPUT_CNTL_14);
    LINK_STATES(SPI_PS_INPUT_CNTL_15);
    LINK_STATES(SPI_PS_INPUT_CNTL_16);
    LINK_STATES(SPI_PS_INPUT_CNTL_17);
    LINK_STATES(SPI_PS_INPUT_CNTL_18);
    LINK_STATES(SPI_PS_INPUT_CNTL_19);
    LINK_STATES(SPI_PS_INPUT_CNTL_20);
    LINK_STATES(SPI_PS_INPUT_CNTL_21);
    LINK_STATES(SPI_PS_INPUT_CNTL_22);
    LINK_STATES(SPI_PS_INPUT_CNTL_23);
    LINK_STATES(SPI_PS_INPUT_CNTL_24);
    LINK_STATES(SPI_PS_INPUT_CNTL_25);
    LINK_STATES(SPI_PS_INPUT_CNTL_26);
    LINK_STATES(SPI_PS_INPUT_CNTL_27);
    LINK_STATES(SPI_PS_INPUT_CNTL_28);
    LINK_STATES(SPI_PS_INPUT_CNTL_29);
    LINK_STATES(SPI_PS_INPUT_CNTL_30);
    LINK_STATES(SPI_PS_INPUT_CNTL_31);
    LINK_STATES(SPI_VS_OUT_CONFIG);  
    LINK_STATES(SPI_THREAD_GROUPING);
    LINK_STATES(SPI_PS_IN_CONTROL_0); 
    LINK_STATES(SPI_PS_IN_CONTROL_1);
    LINK_STATES(SPI_INTERP_CONTROL_0);

    LINK_STATES(SPI_INPUT_Z); 
    LINK_STATES(SPI_FOG_CNTL);

    LINK_STATES(CB_BLEND0_CONTROL);  

    LINK_STATES(CB_SHADER_CONTROL);  

    /*LINK_STATES(VGT_DRAW_INITIATOR);  */

    LINK_STATES(DB_DEPTH_CONTROL);  

    LINK_STATES(CB_COLOR_CONTROL);  
    LINK_STATES(DB_SHADER_CONTROL);  
    LINK_STATES(PA_CL_CLIP_CNTL);  
    LINK_STATES(PA_SU_SC_MODE_CNTL);  
    LINK_STATES(PA_CL_VTE_CNTL);
    LINK_STATES(PA_CL_VS_OUT_CNTL);
    LINK_STATES(PA_CL_NANINF_CNTL);

    LINK_STATES(SQ_PGM_START_PS);   
    LINK_STATES(SQ_PGM_RESOURCES_PS);  
    LINK_STATES(SQ_PGM_EXPORTS_PS);  
    LINK_STATES(SQ_PGM_START_VS);    
    LINK_STATES(SQ_PGM_RESOURCES_VS);  
    LINK_STATES(SQ_PGM_START_GS);          
    LINK_STATES(SQ_PGM_RESOURCES_GS);   
    LINK_STATES(SQ_PGM_START_ES);          
    LINK_STATES(SQ_PGM_RESOURCES_ES);   
    LINK_STATES(SQ_PGM_START_FS);          
    LINK_STATES(SQ_PGM_RESOURCES_FS);   
    LINK_STATES(SQ_ESGS_RING_ITEMSIZE); 
    LINK_STATES(SQ_GSVS_RING_ITEMSIZE); 
    LINK_STATES(SQ_ESTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_GSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_VSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_PSTMP_RING_ITEMSIZE);
    LINK_STATES(SQ_FBUF_RING_ITEMSIZE); 
    LINK_STATES(SQ_REDUC_RING_ITEMSIZE);
    LINK_STATES(SQ_GS_VERT_ITEMSIZE);   
    LINK_STATES(SQ_PGM_CF_OFFSET_PS);  
    LINK_STATES(SQ_PGM_CF_OFFSET_VS);
    LINK_STATES(SQ_PGM_CF_OFFSET_GS);
    LINK_STATES(SQ_PGM_CF_OFFSET_ES);
    LINK_STATES(SQ_PGM_CF_OFFSET_FS);

    LINK_STATES(PA_SU_POINT_SIZE);  
    LINK_STATES(PA_SU_POINT_MINMAX);  
    LINK_STATES(PA_SU_LINE_CNTL);  
    LINK_STATES(PA_SC_LINE_STIPPLE); 
    LINK_STATES(VGT_OUTPUT_PATH_CNTL);

    LINK_STATES(VGT_GS_MODE);
        
    LINK_STATES(PA_SC_MPASS_PS_CNTL);
    LINK_STATES(PA_SC_MODE_CNTL);  

    LINK_STATES(VGT_PRIMITIVEID_EN);
    LINK_STATES(VGT_DMA_NUM_INSTANCES);  

    LINK_STATES(VGT_MULTI_PRIM_IB_RESET_EN);  

    LINK_STATES(VGT_INSTANCE_STEP_RATE_0);
    LINK_STATES(VGT_INSTANCE_STEP_RATE_1);
    
    LINK_STATES(VGT_STRMOUT_EN);  
    LINK_STATES(VGT_REUSE_OFF);  

    LINK_STATES(PA_SC_LINE_CNTL);  
    LINK_STATES(PA_SC_AA_CONFIG);  
    LINK_STATES(PA_SU_VTX_CNTL);  
    LINK_STATES(PA_CL_GB_VERT_CLIP_ADJ);  
    LINK_STATES(PA_CL_GB_VERT_DISC_ADJ);  
    LINK_STATES(PA_CL_GB_HORZ_CLIP_ADJ);  
    LINK_STATES(PA_CL_GB_HORZ_DISC_ADJ); 
    LINK_STATES(PA_SC_AA_SAMPLE_LOCS_MCTX);
    LINK_STATES(PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX);

    LINK_STATES(CB_CLRCMP_CONTROL);  
    LINK_STATES(CB_CLRCMP_SRC);  
    LINK_STATES(CB_CLRCMP_DST);  
    LINK_STATES(CB_CLRCMP_MSK);  

    LINK_STATES(PA_SC_AA_MASK);  

    LINK_STATES(VGT_VERTEX_REUSE_BLOCK_CNTL); 
    LINK_STATES(VGT_OUT_DEALLOC_CNTL);  

    LINK_STATES(DB_RENDER_CONTROL); 
    LINK_STATES(DB_RENDER_OVERRIDE);

    LINK_STATES(DB_HTILE_SURFACE);

    LINK_STATES(DB_ALPHA_TO_MASK);

    LINK_STATES(PA_SU_POLY_OFFSET_DB_FMT_CNTL);
    LINK_STATES(PA_SU_POLY_OFFSET_CLAMP);
    LINK_STATES(PA_SU_POLY_OFFSET_FRONT_SCALE);
    LINK_STATES(PA_SU_POLY_OFFSET_FRONT_OFFSET);
    LINK_STATES(PA_SU_POLY_OFFSET_BACK_SCALE);

    pStateListWork->puiValue = (unsigned int*)&(r700->PA_SU_POLY_OFFSET_BACK_OFFSET); 
    pStateListWork->unOffset = mmPA_SU_POLY_OFFSET_BACK_OFFSET - ASIC_CONTEXT_BASE_INDEX;
    pStateListWork->pNext    = NULL;  /* END OF STATE LIST */

    /* TODO : may need order sorting in case someone break the order of states in R700_CHIP_CONTEXT. */

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
    uint32_t *dest;
    struct radeon_aos * paos = (struct radeon_aos *)pAos;
    offset_modifiers offset_mod = {NO_SHIFT, 0, 0xFFFFFFFF};

    BATCH_LOCALS(&context->radeon);

    unsigned int uSQ_VTX_CONSTANT_WORD0_0;
    unsigned int uSQ_VTX_CONSTANT_WORD1_0;
    unsigned int uSQ_VTX_CONSTANT_WORD2_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD3_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD6_0 = 0;

    uSQ_VTX_CONSTANT_WORD0_0 = paos->offset;
	uSQ_VTX_CONSTANT_WORD1_0 = count * stride - 1;

	uSQ_VTX_CONSTANT_WORD2_0 |= 0 << BASE_ADDRESS_HI_shift /* TODO */
                             |stride << SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift	
	                         |GetSurfaceFormat(GL_FLOAT, size, NULL) << SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift /* TODO : trace back api for initial data type, not only GL_FLOAT */
	                         |SQ_NUM_FORMAT_SCALED << SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift
	                         |SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit;
	
	uSQ_VTX_CONSTANT_WORD3_0 |= 1 << MEM_REQUEST_SIZE_shift;
	
	uSQ_VTX_CONSTANT_WORD6_0 |= SQ_TEX_VTX_VALID_BUFFER << SQ_TEX_RESOURCE_WORD6_0__TYPE_shift;

    BEGIN_BATCH_NO_AUTOSTATE(9);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7)); 
    R600_OUT_BATCH((nStreamID + SQ_FETCH_RESOURCE_VS_OFFSET) * FETCH_RESOURCE_STRIDE);

    R600_OUT_BATCH_RELOC(uSQ_VTX_CONSTANT_WORD0_0, 
                         paos->bo, 
                         uSQ_VTX_CONSTANT_WORD0_0,					                 
                         RADEON_GEM_DOMAIN_GTT, 0, 0, &offset_mod);	
	R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD1_0);
	R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD2_0);
	R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD3_0);
	R600_OUT_BATCH(0);
	R600_OUT_BATCH(0);
	R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD6_0);

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
	unsigned int i;

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
	R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();
    COMMIT_BATCH();

    //context->aos_count = 0;
	for(i=0; i<VERT_ATTRIB_MAX; i++)
	{
		unBit = 1 << i;
		if(vpc->mesa_program.Base.InputsRead & unBit) 
		{            
			rcommon_emit_vector(ctx, 
					    &context->radeon.tcl.aos[i],
					    vb->AttribPtr[i]->data,
					    vb->AttribPtr[i]->size,
					    vb->AttribPtr[i]->stride, 
					    vb->Count);

            /* currently aos are packed */
            r700SetupVTXConstants(ctx, 
                                 i,
                                 (void*)(&context->radeon.tcl.aos[i]),
                                 (unsigned int)vb->AttribPtr[i]->size,
                                 (unsigned int)(vb->AttribPtr[i]->size * 4),
                                 (unsigned int)vb->Count);
		}
	}
    
    return R600_FALLBACK_NONE;
}

inline GLboolean needRelocReg(context_t *context, unsigned int reg)
{
    switch (reg + ASIC_CONTEXT_BASE_INDEX) 
    {
        case mmCB_COLOR0_BASE:
        case mmCB_COLOR1_BASE:
        case mmCB_COLOR2_BASE:
        case mmCB_COLOR3_BASE:
        case mmCB_COLOR4_BASE:
        case mmCB_COLOR5_BASE:
        case mmCB_COLOR6_BASE:
        case mmCB_COLOR7_BASE: 
        case mmDB_DEPTH_BASE:
        case mmSQ_PGM_START_VS:   
        case mmSQ_PGM_START_FS:            
        case mmSQ_PGM_START_ES:            
        case mmSQ_PGM_START_GS:            
        case mmSQ_PGM_START_PS:	
            return GL_TRUE;
            break;
    }

    return GL_FALSE;
}

inline GLboolean setRelocReg(context_t *context, unsigned int reg)
{
    BATCH_LOCALS(&context->radeon);
    R700_CHIP_CONTEXT *r700 = R700_CONTEXT_STATES(context);

    struct radeon_bo * pbo;
    uint32_t voffset;
    offset_modifiers offset_mod;

    switch (reg + ASIC_CONTEXT_BASE_INDEX) 
    {
        case mmCB_COLOR0_BASE:
        case mmCB_COLOR1_BASE:
        case mmCB_COLOR2_BASE:
        case mmCB_COLOR3_BASE:
        case mmCB_COLOR4_BASE:
        case mmCB_COLOR5_BASE:
        case mmCB_COLOR6_BASE:
        case mmCB_COLOR7_BASE:
            {
                GLcontext *ctx = GL_CONTEXT(context);
                struct radeon_renderbuffer *rrb;

                rrb = radeon_get_colorbuffer(&context->radeon);
                if (!rrb || !rrb->bo) 
                {
                    fprintf(stderr, "no rrb\n");
                    return GL_FALSE;
                }

                /* refer to radeonCreateScreen : screen->fbLocation = (temp & 0xffff) << 16; */
                offset_mod.shift     = NO_SHIFT;
                offset_mod.shiftbits = 0;
                offset_mod.mask      = 0xFFFFFFFF;

                R600_OUT_BATCH_RELOC(r700->CB_COLOR0_BASE.u32All, 
                                     rrb->bo, 
                                     r700->CB_COLOR0_BASE.u32All, 
                                     0, RADEON_GEM_DOMAIN_VRAM, 0, &offset_mod);
                return GL_TRUE;
            }
            break;  
        case mmDB_DEPTH_BASE:
            {
                GLcontext *ctx = GL_CONTEXT(context);
                struct radeon_renderbuffer *rrb;
                rrb = radeon_get_depthbuffer(&context->radeon);

                offset_mod.shift     = NO_SHIFT;
                offset_mod.shiftbits = 0;
                offset_mod.mask      = 0xFFFFFFFF;

                R600_OUT_BATCH_RELOC(r700->DB_DEPTH_BASE.u32All, 
                                     rrb->bo, 
                                     r700->DB_DEPTH_BASE.u32All, 
                                     0, RADEON_GEM_DOMAIN_VRAM, 0, &offset_mod);

                return GL_TRUE;
            }
            break;
        case mmSQ_PGM_START_VS:
            {
                pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(GL_CONTEXT(context));

                offset_mod.shift     = NO_SHIFT;
                offset_mod.shiftbits = 0;
                offset_mod.mask      = 0xFFFFFFFF;

                R600_OUT_BATCH_RELOC(r700->SQ_PGM_START_VS.u32All,
                                     pbo,
                                     r700->SQ_PGM_START_VS.u32All,
					                 RADEON_GEM_DOMAIN_GTT, 0, 0, &offset_mod);
                return GL_TRUE;
            }
            break;
        case mmSQ_PGM_START_FS:
        case mmSQ_PGM_START_ES:
        case mmSQ_PGM_START_GS:
        case mmSQ_PGM_START_PS:
            {
                pbo = (struct radeon_bo *)r700GetActiveFpShaderBo(GL_CONTEXT(context));

                offset_mod.shift     = NO_SHIFT;
                offset_mod.shiftbits = 0;
                offset_mod.mask      = 0xFFFFFFFF;

                voffset = 0;
                R600_OUT_BATCH_RELOC(r700->SQ_PGM_START_PS.u32All,
                                     pbo,
                                     r700->SQ_PGM_START_PS.u32All,
					                 RADEON_GEM_DOMAIN_GTT, 0, 0, &offset_mod);
                return GL_TRUE;
            }
            break;
    }

    return GL_FALSE;
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

        if(GL_FALSE == needRelocReg(context, pState->unOffset))
        {
            while(NULL != pState->pNext)
            {
                if( ((pState->pNext->unOffset - pState->unOffset) > 1)
                    || (GL_TRUE == needRelocReg(context, pState->pNext->unOffset)) )
                {
                    break;
                }
                else
                {
                    pState = pState->pNext;
                    toSend++;
                }
            };
        }

        pState = pState->pNext;

        BEGIN_BATCH_NO_AUTOSTATE(toSend + 2);
        R600_OUT_BATCH_REGSEQ(((pInit->unOffset + ASIC_CONTEXT_BASE_INDEX)<<2), toSend);
        for(ui=0; ui<toSend; ui++)
        {
            if( GL_FALSE == setRelocReg(context, pInit->unOffset) )
            {
                /* for not reloc reg. */
                R600_OUT_BATCH(*(pInit->puiValue));
            }
            pInit = pInit->pNext;
        };
        END_BATCH();
    };
    COMMIT_BATCH();

    return GL_TRUE;
}




