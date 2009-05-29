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
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/enums.h"

#include "r600_context.h"
#include "r700_chip.h"

#include "r700_shaderinst.h"
#include "r600_emit.h"

extern void r700InitState (GLcontext * ctx);

extern GLboolean r700SyncSurf(context_t *context);
extern void r700Start3D(context_t *context);
extern void r700WaitForIdleClean(context_t *context);

static GLboolean r700ClearFast(context_t *context, GLbitfield mask)
{
    /* TODO, fast clear need implementation */
    return GL_FALSE;
}

static GLboolean r700ClearWithDraw(context_t *context, GLbitfield mask)
{
    GLcontext *ctx = GL_CONTEXT(context);

    BATCH_LOCALS(&context->radeon);

    R700_CHIP_CONTEXT  r700Saved;
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(context->chipobj.pvChipObj);

    void * pbo_vs;
    void * pbo_fs;
    struct radeon_aos aos_vb;
    
    unsigned int ui;
    GLfloat  fTemp;
    GLfloat  fVb[] = { 1.0f,  1.0f, 1.0f, 1.0f,
                      -1.0f, -1.0f, 1.0f, 1.0f,
                       1.0f, -1.0f, 1.0f, 1.0f,
                       1.0f,  1.0f, 1.0f, 1.0f,
                      -1.0f,  1.0f, 1.0f, 1.0f,
                      -1.0f, -1.0f, 1.0f, 1.0f}; /* TODO : Z set here */
    unsigned int uVs[] = { 0xC,        0x81000000, 0x4,        0xA01C0000, 
                           0xC001203C, 0x94000688, 0xC001C000, 0x94200688,
                           0x10000001, 0x540C90,   0x10000401, 0x20540C90,
                           0x10000801, 0x40540C90, 0x90000C01, 0x60400C90,
                           0x10000100, 0x600C90,   0x10000500, 0x20600C90,
                           0x10000900, 0x40600C90, 0x90000D00, 0x60680C90,
                           0x7C000000, 0x2D1001,   0x80000,    0xBEADEAF };
    unsigned int uFs[] = { 0x2,        0xA00C0000, 0xC0008000, 0x94200688,
                           0x10000000, 0x340C90,   0x10000400, 0x20340C90,
                           0x10000800, 0x40340C90, 0x90000C00, 0x60200C90};

    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
    {
        uVs[9]  = 0x541910;
        uVs[11] = 0x20541910;
        uVs[13] = 0x40541910;
        uVs[15] = 0x60401910;
        uVs[17] = 0x601910;
        uVs[19] = 0x20601910;
        uVs[21] = 0x40601910;
        uVs[23] = 0x60681910;
        uFs[5]  = 0x341910;
        uFs[7]  = 0x20341910;
        uFs[9]  = 0x40341910;
        uFs[11] = 0x60201910;
    }

    r700Start3D(context);

    r700SyncSurf(context);

    /* Save current chip object. */
    memcpy(&r700Saved, r700, sizeof(R700_CHIP_CONTEXT));

    r700InitState(ctx);

    r700SetRenderTarget(context);

    /* Turn off perspective divid. */
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

    if( (mask & BUFFER_BIT_FRONT_LEFT) || (mask & BUFFER_BIT_BACK_LEFT) )
    {   /* Enable render target output. */
        SETfield(r700->CB_TARGET_MASK.u32All, 0xF, TARGET0_ENABLE_shift, TARGET0_ENABLE_mask);
    }
    else
    {   /* Disable render target output. */
        CLEARfield(r700->CB_TARGET_MASK.u32All, TARGET0_ENABLE_mask); /* TODO : OGL need 4 rt. */
    }
    if (mask & BUFFER_BIT_DEPTH)
    {   
        /* Set correct Z to clear. */
        SETbit(r700->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        SETbit(r700->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
        SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS, ZFUNC_shift, ZFUNC_mask);
        fTemp = ctx->Depth.Clear;
        for(ui=2; ui<24;)
        {
            fVb[ui] = fTemp;
            ui += 4;
        }
    }
    else
    {   
        /* Disable Z write. */
        CLEARbit(r700->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        CLEARbit(r700->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
    }

    /* Setup vb */
    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
	R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();
    COMMIT_BATCH();

    rcommon_emit_vector(ctx, &aos_vb, (GLvoid *)fVb, 4, 16, 6);

    r700SetupVTXConstans(ctx, VERT_ATTRIB_POS, &aos_vb, 4, 16, 6);

    /* Setup shaders, copied from dump */
    r700->SQ_PGM_RESOURCES_PS.u32All = 0;
	r700->SQ_PGM_RESOURCES_VS.u32All = 0;
	SETbit(r700->SQ_PGM_RESOURCES_PS.u32All, PGM_RESOURCES__PRIME_CACHE_ON_DRAW_bit);
    SETbit(r700->SQ_PGM_RESOURCES_VS.u32All, PGM_RESOURCES__PRIME_CACHE_ON_DRAW_bit);
        /* vs */
    if(0 == r700->pbo_vs_clear)
    {
        (context->chipobj.EmitShader)(ctx, &(r700->pbo_vs_clear), (GLvoid *)(&uVs[0]), 28, "Clr VS");
    }

    r700->SQ_PGM_START_VS.u32All     = 0;
    r700->SQ_PGM_RESOURCES_VS.u32All = 0x00800004;

            /* vs const */ /* TODO : Set color here */
    BEGIN_BATCH_NO_AUTOSTATE(4 + 2);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_ALU_CONST, 4));
    R600_OUT_BATCH(SQ_ALU_CONSTANT_VS_OFFSET * 4); 
    R600_OUT_BATCH(*((unsigned int*)&(ctx->Color.ClearColor[0])));
    R600_OUT_BATCH(*((unsigned int*)&(ctx->Color.ClearColor[1])));
    R600_OUT_BATCH(*((unsigned int*)&(ctx->Color.ClearColor[2])));
    R600_OUT_BATCH(*((unsigned int*)&(ctx->Color.ClearColor[3])));
    END_BATCH();
    COMMIT_BATCH();

    r700->SPI_VS_OUT_CONFIG.u32All   = 0x00000000;
	r700->SPI_PS_IN_CONTROL_0.u32All = 0x20000001;
        /* ps */
    if(0 == r700->pbo_fs_clear)
    {
        (context->chipobj.EmitShader)(ctx, &(r700->pbo_fs_clear), (GLvoid *)(&uFs[0]), 12, "Clr PS"); 
    }

    r700->SQ_PGM_START_PS.u32All     = 0;
    r700->SQ_PGM_RESOURCES_PS.u32All = 0x00800002;
    r700->SQ_PGM_EXPORTS_PS.u32All   = 0x00000002;        
    r700->DB_SHADER_CONTROL.u32All   = 0x00000200; 

    r700->CB_SHADER_CONTROL.u32All = 0x00000001;

    /* set a valid base address to make the command checker happy */
    r700->SQ_PGM_START_FS.u32All     = 0;
    r700->SQ_PGM_START_ES.u32All     = 0;
    r700->SQ_PGM_START_GS.u32All     = 0;

    /* Now, send the states */
    r700SendContextStates(context, GL_TRUE);

    /* Draw */
    GLuint numEntires, j;
    GLuint numIndices = 6;
    unsigned int VGT_DRAW_INITIATOR = 0;
    unsigned int VGT_INDEX_TYPE     = 0;
    unsigned int VGT_PRIMITIVE_TYPE = 0;
    unsigned int VGT_NUM_INDICES    = 0;
    
    numEntires = 2                 /* VGT_INDEX_TYPE */
                 + 3               /* VGT_PRIMITIVE_TYPE */
                 + numIndices + 3; /* DRAW_INDEX_IMMD */                 
                 
    BEGIN_BATCH_NO_AUTOSTATE(numEntires);  

    SETfield(VGT_INDEX_TYPE, DI_INDEX_SIZE_32_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
    R600_OUT_BATCH(VGT_INDEX_TYPE);

    VGT_NUM_INDICES = numIndices;

    SETfield(VGT_PRIMITIVE_TYPE, DI_PT_TRILIST, VGT_PRIMITIVE_TYPE__PRIM_TYPE_shift, VGT_PRIMITIVE_TYPE__PRIM_TYPE_mask);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
    R600_OUT_BATCH(mmVGT_PRIMITIVE_TYPE - ASIC_CONFIG_BASE_INDEX);
    R600_OUT_BATCH(VGT_PRIMITIVE_TYPE);

    SETfield(VGT_DRAW_INITIATOR, DI_SRC_SEL_IMMEDIATE, SOURCE_SELECT_shift, SOURCE_SELECT_mask);
    SETfield(VGT_DRAW_INITIATOR, DI_MAJOR_MODE_0, MAJOR_MODE_shift, MAJOR_MODE_mask);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_IMMD, (numIndices + 1)));
    R600_OUT_BATCH(VGT_NUM_INDICES);
    R600_OUT_BATCH(VGT_DRAW_INITIATOR);

    for (j=0; j<numIndices; j++)
    {
        R600_OUT_BATCH(j);
    }
    END_BATCH();
    COMMIT_BATCH();

    r700WaitForIdleClean(context);

    /* Restore chip object. */
    memcpy(r700, &r700Saved, sizeof(R700_CHIP_CONTEXT));

    return GL_TRUE;
}

void r700Clear(GLcontext * ctx, GLbitfield mask)
{
    context_t *context = R700_CONTEXT(ctx);

    if( GL_TRUE == r700ClearFast(context, mask) )
    {
        return;
    }

    //r700ClearWithDraw(context, mask);
}


