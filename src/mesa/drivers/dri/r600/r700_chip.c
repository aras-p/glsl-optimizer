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

#include "r700_chip.h"
#include "r700_state.h"
#include "r700_tex.h"
#include "r700_oglprog.h"
#include "r700_ioctl.h"
/* to be enable
#include "r700_emit.h"
*/

extern const struct tnl_pipeline_stage *r700_pipeline[];

static GLboolean r700DestroyChipObj(void* pvChipObj)
{
    R700_CHIP_CONTEXT *r700;

    if(NULL == pvChipObj)
    {
        return GL_TRUE;
    }

    r700 = (R700_CHIP_CONTEXT *)pvChipObj;

    FREE(r700->pStateList);

    FREE(r700);

    return GL_TRUE;
}

static void r700InitFuncs(struct dd_function_table *functions)
{
    r700InitStateFuncs(functions);
    r700InitTextureFuncs(functions);
    r700InitShaderFuncs(functions);
    r700InitIoctlFuncs(functions);
}

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

    R700_CHIP_CONTEXT *r700 = CALLOC( sizeof(R700_CHIP_CONTEXT) );

    context->chipobj.pvChipObj = (void*)r700;

    context->chipobj.DestroyChipObj = r700DestroyChipObj;

    context->chipobj.GetTexObjSize  = r700GetTexObjSize;

    context->chipobj.stages = r700_pipeline;

    context->chipobj.InitFuncs = r700InitFuncs;

    context->chipobj.InitState = r700InitState;

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

GLboolean r700SendContextStates(context_t *context)
{
#if 0 //to be enable
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
            if( (pState->pNext->unOffset - pState->unOffset) > 1 )
            {
                break;
            }
            else
            {
                pState = pState->pNext;
                toSend++;
            }
        };

        pState = pState->pNext;

        R700_CMDBUF_CHECK_SPACE(toSend + 2);
        R700EP3(context, IT_SET_CONTEXT_REG, toSend);
        R700E32(context, pInit->unOffset);

        for(ui=0; ui<toSend; ui++)
        {
            R700E32(context, *(pInit->puiValue));
            pInit = pInit->pNext;
        };
    };
#endif //to be enable
    return GL_TRUE;
}




