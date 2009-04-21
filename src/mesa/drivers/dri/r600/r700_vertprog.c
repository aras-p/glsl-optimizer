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
 */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "main/imports.h"
#include "main/mtypes.h"

#include "tnl/t_context.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"

#include "r600_context.h"

#include "r700_chip.h"
#include "r700_debug.h"
#include "r700_vertprog.h"

#if 0 /* to be enabled */
#include "r700_emit.h"
#endif

unsigned int Map_Vertex_Output(r700_AssemblerBase       *pAsm, 
					           struct gl_vertex_program *mesa_vp,
					           unsigned int unStart)
{
    unsigned int i;
	unsigned int unBit;
	unsigned int unTotal = unStart;

    //!!!!!!! THE ORDER MATCH FS INPUT 

	unBit = 1 << VERT_RESULT_HPOS;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_HPOS] = unTotal++;
	}

	unBit = 1 << VERT_RESULT_COL0;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_COL0] = unTotal++;
	}

	unBit = 1 << VERT_RESULT_COL1;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_COL1] = unTotal++;
	}

	//TODO : dealing back face.
	//unBit = 1 << VERT_RESULT_BFC0;
	//if(mesa_vp->Base.OutputsWritten & unBit)
	//{
	//	pAsm->ucVP_OutputMap[VERT_RESULT_COL0] = unTotal++;
	//}

	//unBit = 1 << VERT_RESULT_BFC1;
	//if(mesa_vp->Base.OutputsWritten & unBit)
	//{
	//	pAsm->ucVP_OutputMap[VERT_RESULT_COL1] = unTotal++;
	//}

	//TODO : dealing fog.
	//unBit = 1 << VERT_RESULT_FOGC;
	//if(mesa_vp->Base.OutputsWritten & unBit)
	//{
	//	pAsm->ucVP_OutputMap[VERT_RESULT_FOGC] = unTotal++;
	//}

	//TODO : dealing point size.
	//unBit = 1 << VERT_RESULT_PSIZ;
	//if(mesa_vp->Base.OutputsWritten & unBit)
	//{
	//	pAsm->ucVP_OutputMap[VERT_RESULT_PSIZ] = unTotal++;
	//}

	for(i=0; i<8; i++)
	{
		unBit = 1 << (VERT_RESULT_TEX0 + i);
		if(mesa_vp->Base.OutputsWritten & unBit)
		{
			pAsm->ucVP_OutputMap[VERT_RESULT_TEX0 + i] = unTotal++;
		}
	}

	return (unTotal - unStart);
}

unsigned int Map_Vertex_Input(r700_AssemblerBase       *pAsm, 
					  struct gl_vertex_program *mesa_vp,
					  unsigned int unStart)
{
    int i;
	unsigned int unBit;
	unsigned int unTotal = unStart;
	for(i=0; i<VERT_ATTRIB_MAX; i++)
	{
		unBit = 1 << i;
		if(mesa_vp->Base.InputsRead & unBit)
		{
			pAsm->ucVP_AttributeMap[i] = unTotal++;
		}
	}
	return (unTotal - unStart);
}

GLboolean Process_Vertex_Program_Vfetch_Instructions(
						struct r700_vertex_program *vp,
						struct gl_vertex_program   *mesa_vp)
{
    int i;
    unsigned int unBit;
	VTX_FETCH_METHOD vtxFetchMethod;
	vtxFetchMethod.bEnableMini          = GL_FALSE;
	vtxFetchMethod.mega_fetch_remainder = 0;

	for(i=0; i<VERT_ATTRIB_MAX; i++)
	{
		unBit = 1 << i;
		if(mesa_vp->Base.InputsRead & unBit)
		{
			assemble_vfetch_instruction(&vp->r700AsmCode,
								i,
                                vp->r700AsmCode.ucVP_AttributeMap[i],
								vp->aos_desc[i].size,
                                vp->aos_desc[i].type,
								&vtxFetchMethod);
		}
	}
	
	return GL_TRUE;
}

void Map_Vertex_Program(struct r700_vertex_program *vp,
						struct gl_vertex_program   *mesa_vp)
{
    GLuint ui;
    r700_AssemblerBase *pAsm = &(vp->r700AsmCode);
	unsigned int num_inputs;

	// R0 will always be used for index into vertex buffer
	pAsm->number_used_registers = 1;
	pAsm->starting_vfetch_register_number = pAsm->number_used_registers;

    // Map Inputs: Add 1 to mapping since R0 is used for index
	num_inputs = Map_Vertex_Input(pAsm, mesa_vp, pAsm->number_used_registers);
	pAsm->number_used_registers += num_inputs;

	// Create VFETCH instructions for inputs
	if (GL_TRUE != Process_Vertex_Program_Vfetch_Instructions(vp, mesa_vp) ) 
	{
		r700_error(ERROR_ASM_VTX_CLAUSE, "Calling Process_Vertex_Program_Vfetch_Instructions return error. \n");
		return; //error
	}

	// Map Outputs
	pAsm->number_of_exports = Map_Vertex_Output(pAsm, mesa_vp, pAsm->number_used_registers);

	pAsm->starting_export_register_number = pAsm->number_used_registers;

	pAsm->number_used_registers += pAsm->number_of_exports;

    pAsm->pucOutMask = (unsigned char*) MALLOC(pAsm->number_of_exports);
    
    for(ui=0; ui<pAsm->number_of_exports; ui++)
    {
        pAsm->pucOutMask[ui] = 0x0;
    }

    /* Map temporary registers (GPRs) */
    pAsm->starting_temp_register_number = pAsm->number_used_registers;

    if(mesa_vp->Base.NumNativeTemporaries >= mesa_vp->Base.NumTemporaries)
    {   /* arb uses NumNativeTemporaries */
        pAsm->number_used_registers += mesa_vp->Base.NumNativeTemporaries;
    }
    else
    {   /* fix func t_vp uses NumTemporaries */
        pAsm->number_used_registers += mesa_vp->Base.NumTemporaries;
    }
	
    pAsm->uFirstHelpReg = pAsm->number_used_registers;
}

GLboolean Find_Instruction_Dependencies_vp(struct r700_vertex_program *vp,
					                	struct gl_vertex_program   *mesa_vp)
{
    GLuint i, j;
    GLint * puiTEMPwrites;
    struct prog_instruction *pILInst;
    InstDeps         *pInstDeps;

    puiTEMPwrites = (GLint*) MALLOC(sizeof(GLuint)*mesa_vp->Base.NumTemporaries);
    for(i=0; i<mesa_vp->Base.NumTemporaries; i++)
    {
        puiTEMPwrites[i] = -1;
    }

    pInstDeps = (InstDeps*)MALLOC(sizeof(InstDeps)*mesa_vp->Base.NumInstructions);

    for(i=0; i<mesa_vp->Base.NumInstructions; i++)
    {
        pInstDeps[i].nDstDep = -1;
        pILInst = &(mesa_vp->Base.Instructions[i]);

        //Dst
        if(pILInst->DstReg.File == PROGRAM_TEMPORARY)
        {
            //Set lastwrite for the temp
            puiTEMPwrites[pILInst->DstReg.Index] = i;
        }

        //Src
        for(j=0; j<3; j++)
        {
            if(pILInst->SrcReg[j].File == PROGRAM_TEMPORARY)
            {
                //Set dep.
                pInstDeps[i].nSrcDeps[j] = puiTEMPwrites[pILInst->SrcReg[j].Index];
            }
            else
            {
                pInstDeps[i].nSrcDeps[j] = -1;
            }
        }
    }

    vp->r700AsmCode.pInstDeps = pInstDeps;

    FREE(puiTEMPwrites);

    return GL_TRUE;
}

GLboolean r700TranslateVertexShader(struct r700_vertex_program *vp,
							   struct gl_vertex_program   *mesa_vp)
{
	//Init_Program
	Init_r700_AssemblerBase(SPT_VP, &(vp->r700AsmCode), &(vp->r700Shader) );
	Map_Vertex_Program( vp, mesa_vp );

	if(GL_FALSE == Find_Instruction_Dependencies_vp(vp, mesa_vp))
	{
		return GL_FALSE;
    }

	if(GL_FALSE == AssembleInstr(mesa_vp->Base.NumInstructions,
                                 &(mesa_vp->Base.Instructions[0]), 
                                 &(vp->r700AsmCode)) )
	{
		return GL_FALSE;
	} 

    if(GL_FALSE == Process_Vertex_Exports(&(vp->r700AsmCode), mesa_vp->Base.OutputsWritten) )
    {
        return GL_FALSE;
    }

    vp->r700Shader.nRegs = (vp->r700AsmCode.number_used_registers == 0) ? 0 
                         : (vp->r700AsmCode.number_used_registers - 1);

	vp->r700Shader.nParamExports = vp->r700AsmCode.number_of_exports;

    vp->translated = GL_TRUE;

	return GL_TRUE;
}

void r700SelectVertexShader(GLcontext *ctx)
{
#if 0 /* to be enabled */
    context_t *context = R700_CONTEXT(ctx);
    struct r700_vertex_program *vpc
             = (struct r700_vertex_program *)ctx->VertexProgram._Current;
    if (context->screen->chip.type <= CHIP_TYPE_RV670)
    {
        vpc->r700AsmCode.bR6xx = 1;
    }
    
    TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;

    unsigned int unBit;
	unsigned int i;
	for(i=0; i<VERT_ATTRIB_MAX; i++)
	{
		unBit = 1 << i;
		if(vpc->mesa_program.Base.InputsRead & unBit) /* ctx->Array.ArrayObj->xxxxxxx */
		{
            vpc->aos_desc[i].size   = vb->AttribPtr[i]->size;
            vpc->aos_desc[i].stride = vb->AttribPtr[i]->size * sizeof(GL_FLOAT);/* when emit array, data is packed. vb->AttribPtr[i]->stride;*/
			vpc->aos_desc[i].type   = GL_FLOAT;
		}
	}

    if(GL_FALSE == vpc->translated)
    {
        r700TranslateVertexShader(vpc,
		    					  &(vpc->mesa_program) );
    }
#endif /* to be enabled */
}

void r700SetupVTXConstans(GLcontext  * ctx, 
                          unsigned int nStreamID,
                          unsigned int aos_offset,
                          unsigned int size,      /* number of elements in vector */
                          unsigned int stride,
                          unsigned int count)     /* number of vectors in stream */
{
    context_t *context = R700_CONTEXT(ctx);
    uint32_t *dest;

    unsigned int uSQ_VTX_CONSTANT_WORD0_0;
    unsigned int uSQ_VTX_CONSTANT_WORD1_0;
    unsigned int uSQ_VTX_CONSTANT_WORD2_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD3_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD6_0 = 0;

    uSQ_VTX_CONSTANT_WORD0_0 = aos_offset;
	uSQ_VTX_CONSTANT_WORD1_0 = count * stride - 1;

	uSQ_VTX_CONSTANT_WORD2_0 |= 0 << BASE_ADDRESS_HI_shift /* TODO */
                             |stride << SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift	
	                         |GetSurfaceFormat(GL_FLOAT, size, NULL) << SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift /* TODO : trace back api for initial data type, not only GL_FLOAT */
	                         |SQ_NUM_FORMAT_SCALED << SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift
	                         |SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit;
	
	uSQ_VTX_CONSTANT_WORD3_0 |= 1 << MEM_REQUEST_SIZE_shift;
	
	uSQ_VTX_CONSTANT_WORD6_0 |= SQ_TEX_VTX_VALID_BUFFER << SQ_TEX_RESOURCE_WORD6_0__TYPE_shift;
#if 0 /* to be enabled */
    R700_CMDBUF_CHECK_SPACE(9);
    R700EP3 (context, IT_SET_RESOURCE, 7);
    R700E32 (context, (nStreamID + SQ_FETCH_RESOURCE_VS_OFFSET) * FETCH_RESOURCE_STRIDE);
    
	R700E32 (context, uSQ_VTX_CONSTANT_WORD0_0);
	R700E32 (context, uSQ_VTX_CONSTANT_WORD1_0);
	R700E32 (context, uSQ_VTX_CONSTANT_WORD2_0);
	R700E32 (context, uSQ_VTX_CONSTANT_WORD3_0);
	R700E32 (context, 0);
	R700E32 (context, 0);
	R700E32 (context, uSQ_VTX_CONSTANT_WORD6_0);
#endif /* to be enabled */
}

GLboolean r700SetupVertexProgram(GLcontext * ctx)
{
    context_t *context = R700_CONTEXT(ctx);

    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(context->chipobj.pvChipObj);

    struct r700_vertex_program *vp
             = (struct r700_vertex_program *)ctx->VertexProgram._Current;

    struct gl_program_parameter_list *paramList;
    unsigned int unNumParamData;

    unsigned int ui;

    if(GL_FALSE == vp->loaded)
    {
        if(vp->r700Shader.bNeedsAssembly == GL_TRUE)
	    {
		    Assemble( &(vp->r700Shader) );
	    }

        /* Load vp to gpu */
        (context->chipobj.EmitShader)(ctx, 
                       &(vp->shadercode), 
                       (GLvoid *)(vp->r700Shader.pProgram),
                       vp->r700Shader.uShaderBinaryDWORDSize); 

        vp->loaded = GL_TRUE;
    }

    DumpHwBinary(DUMP_VERTEX_SHADER, (GLvoid *)(vp->r700Shader.pProgram),
                 vp->r700Shader.uShaderBinaryDWORDSize);

    /* TODO : enable this after MemUse fixed *=
    (context->chipobj.MemUse)(context, vp->shadercode.buf->id);
    */

    r700->SQ_PGM_START_VS.u32All            = (vp->shadercode.aos_offset >> 8) & 0x00FFFFFF;
    
    SETfield(r700->SQ_PGM_RESOURCES_VS.u32All, vp->r700Shader.nRegs + 1,
             NUM_GPRS_shift, NUM_GPRS_mask);

    if(vp->r700Shader.uStackSize) /* we don't use branch for now, it should be zero. */
	{
        SETfield(r700->SQ_PGM_RESOURCES_VS.u32All, vp->r700Shader.uStackSize,
                 STACK_SIZE_shift, STACK_SIZE_mask);
    }

    SETfield(r700->SPI_VS_OUT_CONFIG.u32All, vp->r700Shader.nParamExports - 1,
             VS_EXPORT_COUNT_shift, VS_EXPORT_COUNT_mask);
	SETfield(r700->SPI_PS_IN_CONTROL_0.u32All, vp->r700Shader.nParamExports,
             NUM_INTERP_shift, NUM_INTERP_mask);

    /*
    SETbit(r700->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
    CLEARbit(r700->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);
    */

    /* sent out shader constants. */

    paramList = vp->mesa_program.Base.Parameters;

    if(NULL != paramList)
    {
        _mesa_load_state_parameters(ctx, paramList);
#if 0 /* to be enabled */
        unNumParamData = paramList->NumParameters * 4;

        R700_CMDBUF_CHECK_SPACE(unNumParamData + 2);
        R700EP3 (context, IT_SET_ALU_CONST, unNumParamData);
        /* assembler map const from very beginning. */
        R700E32 (context, SQ_ALU_CONSTANT_VS_OFFSET * 4);

        unNumParamData = paramList->NumParameters;

        for(ui=0; ui<unNumParamData; ui++)
        {
            R700E32 (context, *((unsigned int*)&(paramList->ParameterValues[ui][0])));
            R700E32 (context, *((unsigned int*)&(paramList->ParameterValues[ui][1])));
            R700E32 (context, *((unsigned int*)&(paramList->ParameterValues[ui][2])));
            R700E32 (context, *((unsigned int*)&(paramList->ParameterValues[ui][3])));
        }
#endif /* to be enabled */
    }

    return GL_TRUE;
}




