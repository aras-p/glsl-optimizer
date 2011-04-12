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
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"

#include "radeon_debug.h"
#include "r600_context.h"
#include "r600_cmdbuf.h"
#include "r600_emit.h"
#include "program/programopt.h"

#include "evergreen_vertprog.h"

unsigned int evergreen_Map_Vertex_Output(r700_AssemblerBase       *pAsm, 
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
	unBit = 1 << VERT_RESULT_BFC0;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_BFC0] = unTotal++;
	}

	unBit = 1 << VERT_RESULT_BFC1;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_BFC1] = unTotal++;
	}

	//TODO : dealing fog.
	unBit = 1 << VERT_RESULT_FOGC;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_FOGC] = unTotal++;
	}

	//TODO : dealing point size.
	unBit = 1 << VERT_RESULT_PSIZ;
	if(mesa_vp->Base.OutputsWritten & unBit)
	{
		pAsm->ucVP_OutputMap[VERT_RESULT_PSIZ] = unTotal++;
	}

	for(i=0; i<8; i++)
	{
		unBit = 1 << (VERT_RESULT_TEX0 + i);
		if(mesa_vp->Base.OutputsWritten & unBit)
		{
			pAsm->ucVP_OutputMap[VERT_RESULT_TEX0 + i] = unTotal++;
		}
	}

    for(i=VERT_RESULT_VAR0; i<VERT_RESULT_MAX; i++)
	{
		unBit = 1 << i;
		if(mesa_vp->Base.OutputsWritten & unBit)
		{
			pAsm->ucVP_OutputMap[i] = unTotal++;
		}
	}

	return (unTotal - unStart);
}

unsigned int evergreen_Map_Vertex_Input(r700_AssemblerBase       *pAsm, 
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

GLboolean evergreen_Process_Vertex_Program_Vfetch_Instructions(
						struct evergreen_vertex_program *vp,
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

GLboolean evergreen_Process_Vertex_Program_Vfetch_Instructions2(
    struct gl_context *ctx,
	struct evergreen_vertex_program *vp,
	struct gl_vertex_program   *mesa_vp)
{
    int i;
    context_t *context = R700_CONTEXT(ctx);

    VTX_FETCH_METHOD vtxFetchMethod;
	vtxFetchMethod.bEnableMini          = GL_FALSE;
	vtxFetchMethod.mega_fetch_remainder = 0;

    for(i=0; i<context->nNumActiveAos; i++)
    {
        EG_assemble_vfetch_instruction(&vp->r700AsmCode,
                                      vp->r700AsmCode.ucVP_AttributeMap[context->stream_desc[i].element],
                                      context->stream_desc[i].type,
                                      context->stream_desc[i].size,
                                      context->stream_desc[i].element,
                                      context->stream_desc[i]._signed,
                                      context->stream_desc[i].normalize,
                                      context->stream_desc[i].format,
                                     &vtxFetchMethod);
    }

    return GL_TRUE;
}

void evergreen_Map_Vertex_Program(struct gl_context *ctx,
                        struct evergreen_vertex_program *vp,
						struct gl_vertex_program   *mesa_vp)
{
    r700_AssemblerBase *pAsm = &(vp->r700AsmCode);
	unsigned int num_inputs;

	// R0 will always be used for index into vertex buffer
	pAsm->number_used_registers = 1;
	pAsm->starting_vfetch_register_number = pAsm->number_used_registers;

    // Map Inputs: Add 1 to mapping since R0 is used for index
	num_inputs = evergreen_Map_Vertex_Input(pAsm, mesa_vp, pAsm->number_used_registers);
	pAsm->number_used_registers += num_inputs;

	// Create VFETCH instructions for inputs
        if (GL_TRUE != evergreen_Process_Vertex_Program_Vfetch_Instructions2(ctx, vp, mesa_vp) )
	{
		radeon_error("Calling evergreen_Process_Vertex_Program_Vfetch_Instructions2 return error. \n");
		return;
	}

	// Map Outputs
	pAsm->number_of_exports = evergreen_Map_Vertex_Output(pAsm, mesa_vp, pAsm->number_used_registers);

	pAsm->starting_export_register_number = pAsm->number_used_registers;

	pAsm->number_used_registers += pAsm->number_of_exports;

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

    pAsm->flag_reg_index = pAsm->number_used_registers++;

    pAsm->uFirstHelpReg = pAsm->number_used_registers;
}

GLboolean evergreen_Find_Instruction_Dependencies_vp(struct evergreen_vertex_program *vp,
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

struct evergreen_vertex_program* evergreenTranslateVertexShader(struct gl_context *ctx,
						      struct gl_vertex_program *mesa_vp)
{
	context_t *context = EVERGREEN_CONTEXT(ctx);

	struct evergreen_vertex_program *vp;
	unsigned int i;

	vp = calloc(1, sizeof(*vp));
	vp->mesa_program = _mesa_clone_vertex_program(ctx, mesa_vp);
    
    vp->constbo0 = NULL;

	if (mesa_vp->IsPositionInvariant)
	{
                _mesa_insert_mvp_code(ctx, vp->mesa_program);
        }

	for(i=0; i<context->nNumActiveAos; i++)
	{
		vp->aos_desc[i].size   = context->stream_desc[i].size;
		vp->aos_desc[i].stride = context->stream_desc[i].stride;
		vp->aos_desc[i].type   = context->stream_desc[i].type;
		vp->aos_desc[i].format = context->stream_desc[i].format;
	}

	if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
	{
		vp->r700AsmCode.bR6xx = 1;
	}

	//Init_Program
	Init_r700_AssemblerBase(SPT_VP, &(vp->r700AsmCode), &(vp->r700Shader) );
        
    vp->r700AsmCode.bUseMemConstant = GL_TRUE;  
    vp->r700AsmCode.unAsic = 8;

	evergreen_Map_Vertex_Program(ctx, vp, vp->mesa_program );

	if(GL_FALSE == evergreen_Find_Instruction_Dependencies_vp(vp, vp->mesa_program))
	{
		return NULL;
	}

    InitShaderProgram(&(vp->r700AsmCode));

    for(i=0; i < MAX_SAMPLERS; i++)
    {
        vp->r700AsmCode.SamplerUnits[i] = vp->mesa_program->Base.SamplerUnits[i];
    }

    vp->r700AsmCode.unCurNumILInsts = vp->mesa_program->Base.NumInstructions;

	if(GL_FALSE == AssembleInstr(0,
                                 0,
                                 vp->mesa_program->Base.NumInstructions,
                                 &(vp->mesa_program->Base.Instructions[0]),
                                 &(vp->r700AsmCode)) )
	{
		return NULL;
	}

    if(GL_FALSE == Process_Vertex_Exports(&(vp->r700AsmCode), vp->mesa_program->Base.OutputsWritten) )
    {
        return NULL;
    }

    if( GL_FALSE == RelocProgram(&(vp->r700AsmCode), &(vp->mesa_program->Base)) )
    {
        return GL_FALSE;
    }

    vp->r700Shader.nRegs = (vp->r700AsmCode.number_used_registers == 0) ? 0 
                         : (vp->r700AsmCode.number_used_registers - 1);

	vp->r700Shader.nParamExports = vp->r700AsmCode.number_of_exports;

    vp->translated = GL_TRUE;

	return vp;
}

void evergreenSelectVertexShader(struct gl_context *ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_vertex_program_cont *vpc;
    struct evergreen_vertex_program *vp;
    unsigned int i;
    GLboolean match;
    GLbitfield InputsRead;

    vpc = (struct evergreen_vertex_program_cont *)ctx->VertexProgram._Current;

    InputsRead = vpc->mesa_program.Base.InputsRead;
    if (vpc->mesa_program.IsPositionInvariant)
    {
	InputsRead |= VERT_BIT_POS;
    }

    for (vp = vpc->progs; vp; vp = vp->next)
    {
	match = GL_TRUE;
	for(i=0; i<context->nNumActiveAos; i++)
	{
		if (vp->aos_desc[i].size != context->stream_desc[i].size ||
		    vp->aos_desc[i].format != context->stream_desc[i].format)
		{
			match = GL_FALSE;
			break;
		}
	}
	if (match)
	{
		context->selected_vp = (struct r700_vertex_program *) vp;
		return;
	}
    }

    vp = evergreenTranslateVertexShader(ctx, &(vpc->mesa_program));
    if(!vp)
    {
	radeon_error("Failed to translate vertex shader. \n");
	return;
    }
    vp->next = vpc->progs;
    vpc->progs = vp;
    context->selected_vp = (struct r700_vertex_program *) vp;
    return;
}

int evergreen_getTypeSize(GLenum type)
{
    switch (type) 
    {
    case GL_DOUBLE:
        return sizeof(GLdouble);
    case GL_FLOAT:
        return sizeof(GLfloat);
    case GL_INT:
        return sizeof(GLint);
    case GL_UNSIGNED_INT:
        return sizeof(GLuint);
    case GL_SHORT:
        return sizeof(GLshort);
    case GL_UNSIGNED_SHORT:
        return sizeof(GLushort);
    case GL_BYTE:
        return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:
        return sizeof(GLubyte);
    default:
        assert(0);
        return 0;
    }
}

static void evergreenTranslateAttrib(struct gl_context *ctx, GLuint unLoc, int count, const struct gl_client_array *input)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    
    StreamDesc * pStreamDesc = &(context->stream_desc[context->nNumActiveAos]);

	GLuint stride;

	stride = (input->StrideB == 0) ? evergreen_getTypeSize(input->Type) * input->Size 
                                   : input->StrideB;

    if (input->Type == GL_DOUBLE || input->Type == GL_UNSIGNED_INT || input->Type == GL_INT
#if MESA_BIG_ENDIAN
        || evergreen_getTypeSize(input->Type) != 4
#endif
        )
    {
        pStreamDesc->type = GL_FLOAT;

        if (input->StrideB == 0) 
        {
	        pStreamDesc->stride = 0;
        } 
        else 
        {
	        pStreamDesc->stride = sizeof(GLfloat) * input->Size;
        }
        pStreamDesc->dwords = input->Size;
        pStreamDesc->is_named_bo = GL_FALSE;
    } 
    else 
    {
        pStreamDesc->type = input->Type;
        pStreamDesc->dwords = (evergreen_getTypeSize(input->Type) * input->Size + 3)/ 4;
        if (!input->BufferObj->Name) 
        {
            if (input->StrideB == 0) 
            {
                pStreamDesc->stride = 0;
            } 
            else 
            {
                pStreamDesc->stride = (evergreen_getTypeSize(pStreamDesc->type) * input->Size + 3) & ~3;
            }

            pStreamDesc->is_named_bo = GL_FALSE;
        }
    }

	pStreamDesc->size = input->Size;
	pStreamDesc->dst_loc = context->nNumActiveAos;
	pStreamDesc->element = unLoc;
	pStreamDesc->format = input->Format;

	switch (pStreamDesc->type) 
	{ //GetSurfaceFormat
	case GL_FLOAT:
		pStreamDesc->_signed = 0;
		pStreamDesc->normalize = GL_FALSE;
		break;
	case GL_SHORT:
		pStreamDesc->_signed = 1;
		pStreamDesc->normalize = input->Normalized;
		break;
	case GL_BYTE:
		pStreamDesc->_signed = 1;
		pStreamDesc->normalize = input->Normalized;
		break;
	case GL_UNSIGNED_SHORT:
		pStreamDesc->_signed = 0;
		pStreamDesc->normalize = input->Normalized;
		break;
	case GL_UNSIGNED_BYTE:
		pStreamDesc->_signed = 0;
		pStreamDesc->normalize = input->Normalized;
		break;
	default:
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_DOUBLE: 
		assert(0);
		break;
	}
	context->nNumActiveAos++;
}

void evergreenSetVertexFormat(struct gl_context *ctx, const struct gl_client_array *arrays[], int count)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_vertex_program *vpc
           = (struct evergreen_vertex_program *)ctx->VertexProgram._Current;

    struct gl_vertex_program * mesa_vp = (struct gl_vertex_program *)&(vpc->mesa_program);
    unsigned int unLoc = 0;
    unsigned int unBit = mesa_vp->Base.InputsRead;
    context->nNumActiveAos = 0;

    if (mesa_vp->IsPositionInvariant)
    {
        unBit |= VERT_BIT_POS;
    }

    while(unBit) 
    {
        if(unBit & 1)
        {
            evergreenTranslateAttrib(ctx, unLoc, count, arrays[unLoc]);
        }

        unBit >>= 1;
        ++unLoc;
    }
    context->radeon.tcl.aos_count = context->nNumActiveAos;
}

void * evergreenGetActiveVpShaderBo(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *) context->selected_vp;

    if (vp)
	return vp->shaderbo;
    else
	return NULL;
}

void * evergreenGetActiveVpShaderConstBo(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *) context->selected_vp;

    if (vp)
	return vp->constbo0;
    else
	return NULL;
}

GLboolean evergreenSetupVertexProgram(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *) context->selected_vp;

    if(GL_FALSE == vp->loaded)
    {
	    if(vp->r700Shader.bNeedsAssembly == GL_TRUE)
	    {
		    Assemble( &(vp->r700Shader) );
	    }

        /* Load vp to gpu */
        r600EmitShader(ctx,
                       &(vp->shaderbo),
                       (GLvoid *)(vp->r700Shader.pProgram),
                       vp->r700Shader.uShaderBinaryDWORDSize,
                       "VS");
   
        vp->loaded = GL_TRUE;
    }

    EVERGREEN_STATECHANGE(context, vs);      
    
    /* TODO : enable this after MemUse fixed *=
    (context->chipobj.MemUse)(context, vp->shadercode.buf->id);
    */       

    evergreen->SQ_PGM_RESOURCES_VS.u32All = 0;
    SETbit(evergreen->SQ_PGM_RESOURCES_VS.u32All, PGM_RESOURCES__PRIME_CACHE_ON_DRAW_bit);
    
    evergreen->vs.SQ_ALU_CONST_CACHE_VS_0.u32All = 0; /* set from buffer object. */
    
    evergreen->vs.SQ_PGM_START_VS.u32All = 0;

    SETfield(evergreen->SQ_PGM_RESOURCES_VS.u32All, vp->r700Shader.nRegs + 1,
             NUM_GPRS_shift, NUM_GPRS_mask);

    if(vp->r700Shader.uStackSize) /* we don't use branch for now, it should be zero. */
	{
        SETfield(evergreen->SQ_PGM_RESOURCES_VS.u32All, vp->r700Shader.uStackSize,
                 STACK_SIZE_shift, STACK_SIZE_mask);
    }

    EVERGREEN_STATECHANGE(context, spi);

    SETfield(evergreen->SPI_VS_OUT_CONFIG.u32All,
	     vp->r700Shader.nParamExports ? (vp->r700Shader.nParamExports - 1) : 0,
             VS_EXPORT_COUNT_shift, VS_EXPORT_COUNT_mask);
    SETfield(evergreen->SPI_PS_IN_CONTROL_0.u32All, vp->r700Shader.nParamExports,
             NUM_INTERP_shift, NUM_INTERP_mask);

    /*
    SETbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
    CLEARbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);
    */

    return GL_TRUE;
}

GLboolean evergreenSetupVPconstants(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *) context->selected_vp;

    struct gl_program_parameter_list *paramList;
    unsigned int unNumParamData;
    unsigned int ui;
    int alloc_size;

    /* sent out shader constants. */
    paramList = vp->mesa_program->Base.Parameters;

    if(NULL != paramList) {
        /* vp->mesa_program was cloned, not updated by glsl shader api. */
        /* _mesa_reference_program has already checked glsl shProg is ok and set ctx->VertexProgem._Current */
        /* so, use ctx->VertexProgem._Current */       
        struct gl_program_parameter_list *paramListOrginal = 
                         ctx->VertexProgram._Current->Base.Parameters;
         
	    _mesa_load_state_parameters(ctx, paramList);

	    if (paramList->NumParameters > EVERGREEN_MAX_DX9_CONSTS)
		    return GL_FALSE;

	    EVERGREEN_STATECHANGE(context, vs);

	    evergreen->vs.num_consts = paramList->NumParameters;

	    unNumParamData = paramList->NumParameters;

	    /* alloc multiple of 16 constants */
	    alloc_size = ((unNumParamData * 4 * 4) + 255) & ~255;

	    for(ui=0; ui<unNumParamData; ui++) {
            if(paramList->Parameters[ui].Type == PROGRAM_UNIFORM) 
            {
                evergreen->vs.consts[ui][0].f32All = paramListOrginal->ParameterValues[ui][0];
		        evergreen->vs.consts[ui][1].f32All = paramListOrginal->ParameterValues[ui][1];
		        evergreen->vs.consts[ui][2].f32All = paramListOrginal->ParameterValues[ui][2];
		        evergreen->vs.consts[ui][3].f32All = paramListOrginal->ParameterValues[ui][3];
            }
            else
            {
		        evergreen->vs.consts[ui][0].f32All = paramList->ParameterValues[ui][0];
		        evergreen->vs.consts[ui][1].f32All = paramList->ParameterValues[ui][1];
		        evergreen->vs.consts[ui][2].f32All = paramList->ParameterValues[ui][2];
		        evergreen->vs.consts[ui][3].f32All = paramList->ParameterValues[ui][3];
            }
	    }

        radeonAllocDmaRegion(&context->radeon, 
                             &context->vp_Constbo, 
                             &context->vp_bo_offset, 
			     alloc_size,
                             256);        
        r600EmitShaderConsts(ctx,
                             context->vp_Constbo,
                             context->vp_bo_offset,
                             (GLvoid *)&(evergreen->vs.consts[0][0]),
                             unNumParamData * 4 * 4);
    } else
	    evergreen->vs.num_consts = 0;

    COMPILED_SUB * pCompiledSub;
    GLuint uj;
    GLuint unConstOffset = evergreen->vs.num_consts;
    for(ui=0; ui<vp->r700AsmCode.unNumPresub; ui++)
    {
        pCompiledSub = vp->r700AsmCode.presubs[ui].pCompiledSub;

        evergreen->vs.num_consts += pCompiledSub->NumParameters;

        for(uj=0; uj<pCompiledSub->NumParameters; uj++)
        {
            evergreen->vs.consts[uj + unConstOffset][0].f32All = pCompiledSub->ParameterValues[uj][0];
		    evergreen->vs.consts[uj + unConstOffset][1].f32All = pCompiledSub->ParameterValues[uj][1];
		    evergreen->vs.consts[uj + unConstOffset][2].f32All = pCompiledSub->ParameterValues[uj][2];
		    evergreen->vs.consts[uj + unConstOffset][3].f32All = pCompiledSub->ParameterValues[uj][3];
        }
        unConstOffset += pCompiledSub->NumParameters;
    }

    return GL_TRUE;
}

