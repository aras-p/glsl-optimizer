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

#include "main/mtypes.h"
#include "main/imports.h"
#include "shader/prog_parameter.h"

#include "radeon_debug.h"
#include "r600_context.h"

#include "r700_assembler.h"

#define USE_CF_FOR_CONTINUE_BREAK 1
#define USE_CF_FOR_POP_AFTER      1

struct prog_instruction noise1_insts[12] = { 
    {OPCODE_BGNSUB , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_MOV , {{0, 0, 0, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 0, 2, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_MOV , {{8, 0, 0, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 0, 4, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_MOV , {{8, 0, 585, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 0, 8, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_SGT , {{0, 0, 585, 0, 0, 0}, {8, 0, 1170, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 1, 1, 0, 8, 1672, 0}, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_IF , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 7, 0, 0}, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0}, 
    {OPCODE_MOV , {{0, 0, 1755, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 0, 1, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_RET , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_ENDIF , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_MOV , {{0, 0, 1170, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {0, 0, 1, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_RET , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}, 
    {OPCODE_ENDSUB , {{13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}, {13, 0, 1672, 0, 0, 0}}, {13, 0, 15, 0, 8, 1672, 0}, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0}
};
float noise1_const[2][4] = {
    {0.300000f, 0.900000f, 0.500000f, 0.300000f}
};

COMPILED_SUB noise1_presub = {
    &(noise1_insts[0]),
    12, 
    2, 
    1, 
    0, 
    &(noise1_const[0]), 
    SWIZZLE_X, 
    SWIZZLE_X, 
    SWIZZLE_X, 
    SWIZZLE_X,
    {0,0,0},
    0 
};

BITS addrmode_PVSDST(PVSDST * pPVSDST)
{
	return pPVSDST->addrmode0 | ((BITS)pPVSDST->addrmode1 << 1);
}

void setaddrmode_PVSDST(PVSDST * pPVSDST, BITS addrmode) 
{
	pPVSDST->addrmode0 = addrmode & 1;
	pPVSDST->addrmode1 = (addrmode >> 1) & 1;
}

void nomask_PVSDST(PVSDST * pPVSDST) 
{
	pPVSDST->writex = pPVSDST->writey = pPVSDST->writez = pPVSDST->writew = 1;
}

BITS addrmode_PVSSRC(PVSSRC* pPVSSRC) 
{
	return pPVSSRC->addrmode0 | ((BITS)pPVSSRC->addrmode1 << 1);
}

void setaddrmode_PVSSRC(PVSSRC* pPVSSRC, BITS addrmode) 
{
	pPVSSRC->addrmode0 = addrmode & 1;
	pPVSSRC->addrmode1 = (addrmode >> 1) & 1;
}


void setswizzle_PVSSRC(PVSSRC* pPVSSRC, BITS swz) 
{
	pPVSSRC->swizzlex = 
	pPVSSRC->swizzley = 
	pPVSSRC->swizzlez = 
	pPVSSRC->swizzlew = swz;
}

void noswizzle_PVSSRC(PVSSRC* pPVSSRC) 
{
	pPVSSRC->swizzlex = SQ_SEL_X;
	pPVSSRC->swizzley = SQ_SEL_Y;
	pPVSSRC->swizzlez = SQ_SEL_Z;
	pPVSSRC->swizzlew = SQ_SEL_W;
}

void
swizzleagain_PVSSRC(PVSSRC * pPVSSRC, BITS x, BITS y, BITS z, BITS w)
{
    switch (x) 
    {
        case SQ_SEL_X: x = pPVSSRC->swizzlex; 
            break;
        case SQ_SEL_Y: x = pPVSSRC->swizzley; 
            break;
        case SQ_SEL_Z: x = pPVSSRC->swizzlez; 
            break;
        case SQ_SEL_W: x = pPVSSRC->swizzlew; 
            break;
        default:;
    }

    switch (y) 
    {
        case SQ_SEL_X: y = pPVSSRC->swizzlex; 
            break;
        case SQ_SEL_Y: y = pPVSSRC->swizzley; 
            break;
        case SQ_SEL_Z: y = pPVSSRC->swizzlez; 
            break;
        case SQ_SEL_W: y = pPVSSRC->swizzlew; 
            break;
        default:;
    }

    switch (z) 
    {
        case SQ_SEL_X: z = pPVSSRC->swizzlex; 
            break;
        case SQ_SEL_Y: z = pPVSSRC->swizzley; 
            break;
        case SQ_SEL_Z: z = pPVSSRC->swizzlez; 
            break;
        case SQ_SEL_W: z = pPVSSRC->swizzlew; 
            break;
        default:;
    }

    switch (w) 
    {
        case SQ_SEL_X: w = pPVSSRC->swizzlex; 
            break;
        case SQ_SEL_Y: w = pPVSSRC->swizzley; 
            break;
        case SQ_SEL_Z: w = pPVSSRC->swizzlez; 
            break;
        case SQ_SEL_W: w = pPVSSRC->swizzlew; 
            break;
        default:;
    }

    pPVSSRC->swizzlex = x;
    pPVSSRC->swizzley = y;
    pPVSSRC->swizzlez = z;
    pPVSSRC->swizzlew = w;
}

void neg_PVSSRC(PVSSRC* pPVSSRC) 
{
	pPVSSRC->negx = 1;
	pPVSSRC->negy = 1;
	pPVSSRC->negz = 1;
	pPVSSRC->negw = 1;
}

void noneg_PVSSRC(PVSSRC* pPVSSRC) 
{
	pPVSSRC->negx = 0;
	pPVSSRC->negy = 0;
	pPVSSRC->negz = 0;
	pPVSSRC->negw = 0;
}

// negate argument (for SUB instead of ADD and alike)
void flipneg_PVSSRC(PVSSRC* pPVSSRC) 
{
	pPVSSRC->negx = !pPVSSRC->negx;
	pPVSSRC->negy = !pPVSSRC->negy;
	pPVSSRC->negz = !pPVSSRC->negz;
	pPVSSRC->negw = !pPVSSRC->negw;
}

void zerocomp_PVSSRC(PVSSRC* pPVSSRC, int c) 
{
	switch (c) 
	{
		case 0: pPVSSRC->swizzlex = SQ_SEL_0; pPVSSRC->negx = 0; break;
		case 1: pPVSSRC->swizzley = SQ_SEL_0; pPVSSRC->negy = 0; break;
		case 2: pPVSSRC->swizzlez = SQ_SEL_0; pPVSSRC->negz = 0; break;
		case 3: pPVSSRC->swizzlew = SQ_SEL_0; pPVSSRC->negw = 0; break;
		default:;
	} 
}

void onecomp_PVSSRC(PVSSRC* pPVSSRC, int c) 
{
	switch (c) 
	{
		case 0: pPVSSRC->swizzlex = SQ_SEL_1; pPVSSRC->negx = 0; break;
		case 1: pPVSSRC->swizzley = SQ_SEL_1; pPVSSRC->negy = 0; break;
		case 2: pPVSSRC->swizzlez = SQ_SEL_1; pPVSSRC->negz = 0; break;
		case 3: pPVSSRC->swizzlew = SQ_SEL_1; pPVSSRC->negw = 0; break;
		default:;
	} 
}

BITS is_misc_component_exported(VAP_OUT_VTX_FMT_0* pOutVTXFmt0)  
{
	  return (pOutVTXFmt0->point_size            |
			  pOutVTXFmt0->edge_flag             |
			  pOutVTXFmt0->rta_index             |
			  pOutVTXFmt0->kill_flag             |
			  pOutVTXFmt0->viewport_index);
}

BITS is_depth_component_exported(OUT_FRAGMENT_FMT_0* pFPOutFmt) 
{
	  return (pFPOutFmt->depth            | 
			  pFPOutFmt->stencil_ref      | 
			  pFPOutFmt->mask             | 
			  pFPOutFmt->coverage_to_mask);
}

GLboolean is_reduction_opcode(PVSDWORD* dest)
{
    if (dest->dst.op3 == 0) 
    {
        if ( (dest->dst.opcode == SQ_OP2_INST_DOT4 || dest->dst.opcode == SQ_OP2_INST_DOT4_IEEE || dest->dst.opcode == SQ_OP2_INST_CUBE) ) 
        {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

GLuint GetSurfaceFormat(GLenum eType, GLuint nChannels, GLuint * pClient_size)
{
    GLuint format = FMT_INVALID;
	GLuint uiElemSize = 0;

    switch (eType)
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
			uiElemSize = 1;
            switch(nChannels)
            {
                case 1:
                    format = FMT_8; break;
                case 2:
                    format = FMT_8_8; break;
                case 3:
                    format = FMT_8_8_8; break;
                case 4:
                    format = FMT_8_8_8_8; break;
                default:
                    break;
            }
            break;

        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
			uiElemSize = 2;
            switch(nChannels)
            {
                case 1:
                    format = FMT_16; break;
                case 2:
                    format = FMT_16_16; break;
                case 3:
                    format = FMT_16_16_16; break;
                case 4:
                    format = FMT_16_16_16_16; break;
                default:
                    break;
            }
            break;

        case GL_UNSIGNED_INT:
        case GL_INT:
			uiElemSize = 4;
            switch(nChannels)
            {
                case 1:
                    format = FMT_32; break;
                case 2:
                    format = FMT_32_32; break;
                case 3:
                    format = FMT_32_32_32; break;
                case 4:
                    format = FMT_32_32_32_32; break;
                default:
                    break;
            }
            break;

        case GL_FLOAT:
			uiElemSize = 4;
			switch(nChannels)
            {
                case 1:
                    format = FMT_32_FLOAT; break;
                case 2:
                    format = FMT_32_32_FLOAT; break;
                case 3:
                    format = FMT_32_32_32_FLOAT; break;
                case 4:
                    format = FMT_32_32_32_32_FLOAT; break;
                default:
                    break;
            }
			break;
        case GL_DOUBLE:
			uiElemSize = 8;
            switch(nChannels)
            {
                case 1:
                    format = FMT_32_FLOAT; break;
                case 2:
                    format = FMT_32_32_FLOAT; break;
                case 3:
                    format = FMT_32_32_32_FLOAT; break;
                case 4:
                    format = FMT_32_32_32_32_FLOAT; break;
                default:
                    break;
            }
            break;
        default:
			;
            //GL_ASSERT_NO_CASE();
    }

    if(NULL != pClient_size)
    {
	    *pClient_size = uiElemSize * nChannels;
    }

    return(format);
}

unsigned int r700GetNumOperands(GLuint opcode, GLuint nIsOp3) 
{
    if(nIsOp3 > 0)
    {
        return 3;
    }

    switch (opcode)
    {
    case SQ_OP2_INST_ADD:
    case SQ_OP2_INST_KILLE:
    case SQ_OP2_INST_KILLGT:
    case SQ_OP2_INST_KILLGE:
    case SQ_OP2_INST_KILLNE:
    case SQ_OP2_INST_MUL: 
    case SQ_OP2_INST_MAX:
    case SQ_OP2_INST_MIN:
    //case SQ_OP2_INST_MAX_DX10:
    //case SQ_OP2_INST_MIN_DX10:
    case SQ_OP2_INST_SETE: 
    case SQ_OP2_INST_SETNE:
    case SQ_OP2_INST_SETGT:
    case SQ_OP2_INST_SETGE:
    case SQ_OP2_INST_PRED_SETE:
    case SQ_OP2_INST_PRED_SETGT:
    case SQ_OP2_INST_PRED_SETGE:
    case SQ_OP2_INST_PRED_SETNE:
    case SQ_OP2_INST_DOT4:
    case SQ_OP2_INST_DOT4_IEEE:
    case SQ_OP2_INST_CUBE:
        return 2;  

    case SQ_OP2_INST_MOV: 
    case SQ_OP2_INST_MOVA_FLOOR:
    case SQ_OP2_INST_FRACT:
    case SQ_OP2_INST_FLOOR:
    case SQ_OP2_INST_TRUNC:
    case SQ_OP2_INST_EXP_IEEE:
    case SQ_OP2_INST_LOG_CLAMPED:
    case SQ_OP2_INST_LOG_IEEE:
    case SQ_OP2_INST_RECIP_IEEE:
    case SQ_OP2_INST_RECIPSQRT_IEEE:
    case SQ_OP2_INST_FLT_TO_INT:
    case SQ_OP2_INST_SIN:
    case SQ_OP2_INST_COS:
        return 1;
        
    default: radeon_error(
		    "Need instruction operand number for %x.\n", opcode); 
    };

    return 3;
}

int Init_r700_AssemblerBase(SHADER_PIPE_TYPE spt, r700_AssemblerBase* pAsm, R700_Shader* pShader)
{
    GLuint i;

    Init_R700_Shader(pShader);
    pAsm->pR700Shader = pShader;
    pAsm->currentShaderType = spt;

    pAsm->cf_last_export_ptr   = NULL;

    pAsm->cf_current_export_clause_ptr = NULL;
    pAsm->cf_current_alu_clause_ptr    = NULL;
    pAsm->cf_current_tex_clause_ptr    = NULL;
    pAsm->cf_current_vtx_clause_ptr    = NULL;
    pAsm->cf_current_cf_clause_ptr     = NULL;

    // No clause has been created yet
    pAsm->cf_current_clause_type = CF_EMPTY_CLAUSE;

    pAsm->number_of_colorandz_exports = 0;
    pAsm->number_of_exports           = 0;
    pAsm->number_of_export_opcodes    = 0;

    pAsm->alu_x_opcode = 0;

    pAsm->D2.bits = 0;

    pAsm->D.bits = 0;
    pAsm->S[0].bits = 0;
    pAsm->S[1].bits = 0;
    pAsm->S[2].bits = 0;

    pAsm->uLastPosUpdate = 0; 
	
    *(BITS *) &pAsm->fp_stOutFmt0 = 0;

    pAsm->uIIns = 0;
    pAsm->uOIns = 0;
    pAsm->number_used_registers = 0;
    pAsm->uUsedConsts = 256; 


    // Fragment programs
    pAsm->uBoolConsts = 0;
    pAsm->uIntConsts = 0;
    pAsm->uInsts = 0;
    pAsm->uConsts = 0;

    pAsm->FCSP = 0;
    pAsm->fc_stack[0].type = FC_NONE;

    pAsm->aArgSubst[0] =
    pAsm->aArgSubst[1] =
    pAsm->aArgSubst[2] =
    pAsm->aArgSubst[3] = (-1);

    pAsm->uOutputs = 0;

    for (i=0; i<NUMBER_OF_OUTPUT_COLORS; i++) 
    {
        pAsm->color_export_register_number[i] = (-1);
    }


    pAsm->depth_export_register_number = (-1);
    pAsm->stencil_export_register_number = (-1);
    pAsm->coverage_to_mask_export_register_number = (-1);
    pAsm->mask_export_register_number = (-1);

    pAsm->starting_export_register_number = 0;
    pAsm->starting_vfetch_register_number = 0;
    pAsm->starting_temp_register_number   = 0;
    pAsm->uFirstHelpReg = 0;

    pAsm->input_position_is_used = GL_FALSE;
    pAsm->input_normal_is_used   = GL_FALSE;

    for (i=0; i<NUMBER_OF_INPUT_COLORS; i++) 
    {
        pAsm->input_color_is_used[ i ] = GL_FALSE;
    }

    for (i=0; i<NUMBER_OF_TEXTURE_UNITS; i++) 
    {
        pAsm->input_texture_unit_is_used[ i ] = GL_FALSE;
    }

    for (i=0; i<VERT_ATTRIB_MAX; i++) 
    {
        pAsm->vfetch_instruction_ptr_array[ i ] = NULL;
    }

    pAsm->number_of_inputs = 0;

    pAsm->is_tex = GL_FALSE;
    pAsm->need_tex_barrier = GL_FALSE;

    pAsm->subs              = NULL;
    pAsm->unSubArraySize    = 0;
    pAsm->unSubArrayPointer = 0;
    pAsm->callers              = NULL;
    pAsm->unCallerArraySize    = 0;
    pAsm->unCallerArrayPointer = 0;

    pAsm->CALLSP = 0;
    pAsm->CALLSTACK[0].FCSP_BeforeEntry = 0;
    pAsm->CALLSTACK[0].plstCFInstructions_local
          = &(pAsm->pR700Shader->lstCFInstructions);

    pAsm->CALLSTACK[0].max = 0;
    pAsm->CALLSTACK[0].current = 0;

    SetActiveCFlist(pAsm->pR700Shader, pAsm->CALLSTACK[0].plstCFInstructions_local);

    pAsm->unCFflags = 0;

    pAsm->presubs           = NULL;
    pAsm->unPresubArraySize = 0;
    pAsm->unNumPresub       = 0;
    pAsm->unCurNumILInsts   = 0;

    pAsm->unVetTexBits      = 0;

    return 0;
}

GLboolean IsTex(gl_inst_opcode Opcode)
{
    if( (OPCODE_TEX==Opcode) || (OPCODE_TXP==Opcode) || (OPCODE_TXB==Opcode) ||
        (OPCODE_DDX==Opcode) || (OPCODE_DDY==Opcode) )
    {
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean IsAlu(gl_inst_opcode Opcode)
{
    //TODO : more for fc and ex for higher spec.
    if( IsTex(Opcode) )
    {
        return GL_FALSE;
    }
    return GL_TRUE;
}

int check_current_clause(r700_AssemblerBase* pAsm,
					     CF_CLAUSE_TYPE      new_clause_type)
{
	if (pAsm->cf_current_clause_type != new_clause_type) 
	{	//Close last open clause
		switch (pAsm->cf_current_clause_type) 
		{
		case CF_ALU_CLAUSE:
			if ( pAsm->cf_current_alu_clause_ptr != NULL) 
            {
                pAsm->cf_current_alu_clause_ptr = NULL;
            }
			break;
		case CF_VTX_CLAUSE:
			if ( pAsm->cf_current_vtx_clause_ptr != NULL) 
            {
                pAsm->cf_current_vtx_clause_ptr = NULL;
            }
			break;
		case CF_TEX_CLAUSE:
			if ( pAsm->cf_current_tex_clause_ptr != NULL) 
            {
                pAsm->cf_current_tex_clause_ptr = NULL;
            }
			break;
		case CF_EXPORT_CLAUSE:
			if ( pAsm->cf_current_export_clause_ptr != NULL) 
            {
                pAsm->cf_current_export_clause_ptr = NULL;
            }
			break;
		case CF_OTHER_CLAUSE:
			if ( pAsm->cf_current_cf_clause_ptr != NULL) 
            {
                pAsm->cf_current_cf_clause_ptr = NULL;
            }
			break;
		case CF_EMPTY_CLAUSE:
			break;
		default:
            radeon_error(
                       "Unknown CF_CLAUSE_TYPE (%d) in check_current_clause. \n", (int) new_clause_type);
			return GL_FALSE;
		}

        pAsm->cf_current_clause_type = CF_EMPTY_CLAUSE;

		// Create new clause
        switch (new_clause_type) 
	    {
        case CF_ALU_CLAUSE:
            pAsm->cf_current_clause_type = CF_ALU_CLAUSE;
            break;
        case CF_VTX_CLAUSE:
            pAsm->cf_current_clause_type = CF_VTX_CLAUSE;
            break;
        case CF_TEX_CLAUSE:        
            pAsm->cf_current_clause_type = CF_TEX_CLAUSE;
            break;
        case CF_EXPORT_CLAUSE:
            {
                R700ControlFlowSXClause* pR700ControlFlowSXClause 
                            = (R700ControlFlowSXClause*) CALLOC_STRUCT(R700ControlFlowSXClause); 
            
                // Add new export instruction to control flow program        
                if (pR700ControlFlowSXClause != 0) 
                {
                    pAsm->cf_current_export_clause_ptr = pR700ControlFlowSXClause;
                    Init_R700ControlFlowSXClause(pR700ControlFlowSXClause);
                    AddCFInstruction( pAsm->pR700Shader, 
                                      (R700ControlFlowInstruction *)pR700ControlFlowSXClause );
                }
                else 
                {
                    radeon_error(
                               "Error allocating new EXPORT CF instruction in check_current_clause. \n");
                    return GL_FALSE;
                }
                pAsm->cf_current_clause_type = CF_EXPORT_CLAUSE;
            }
            break;
        case CF_EMPTY_CLAUSE:
            break;
        case CF_OTHER_CLAUSE:
            pAsm->cf_current_clause_type = CF_OTHER_CLAUSE;
            break;
        default:
            radeon_error(
                       "Unknown CF_CLAUSE_TYPE (%d) in check_current_clause. \n", (int) new_clause_type);
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

GLboolean add_cf_instruction(r700_AssemblerBase* pAsm)
{
    if(GL_FALSE == check_current_clause(pAsm, CF_OTHER_CLAUSE))
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr = 
      (R700ControlFlowGenericClause*) CALLOC_STRUCT(R700ControlFlowGenericClause);

    if (pAsm->cf_current_cf_clause_ptr != NULL) 
	{
		Init_R700ControlFlowGenericClause(pAsm->cf_current_cf_clause_ptr);
		AddCFInstruction( pAsm->pR700Shader, 
                          (R700ControlFlowInstruction *)pAsm->cf_current_cf_clause_ptr );
	}
	else 
	{
        radeon_error("Could not allocate a new VFetch CF instruction.\n");
		return GL_FALSE;
	}

    return GL_TRUE;
}

GLboolean add_vfetch_instruction(r700_AssemblerBase*     pAsm,
								 R700VertexInstruction*  vertex_instruction_ptr)
{
	if( GL_FALSE == check_current_clause(pAsm,  CF_VTX_CLAUSE) )
	{
		return GL_FALSE;
	}

    if( pAsm->cf_current_vtx_clause_ptr == NULL ||
        ( (pAsm->cf_current_vtx_clause_ptr != NULL) && 
         (pAsm->cf_current_vtx_clause_ptr->m_Word1.f.count >= GetCFMaxInstructions(pAsm->cf_current_vtx_clause_ptr->m_ShaderInstType)-1) 
        ) ) 
    { 
		// Create new Vfetch control flow instruction for this new clause
		pAsm->cf_current_vtx_clause_ptr = (R700ControlFlowGenericClause*) CALLOC_STRUCT(R700ControlFlowGenericClause);

		if (pAsm->cf_current_vtx_clause_ptr != NULL) 
		{
			Init_R700ControlFlowGenericClause(pAsm->cf_current_vtx_clause_ptr);
			AddCFInstruction( pAsm->pR700Shader, 
                              (R700ControlFlowInstruction *)pAsm->cf_current_vtx_clause_ptr );
		}
		else 
		{
            radeon_error("Could not allocate a new VFetch CF instruction.\n");
			return GL_FALSE;
		}

		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.pop_count        = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.cf_const         = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.count            = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.end_of_program   = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_VTX;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.barrier          = 0x1;

		LinkVertexInstruction(pAsm->cf_current_vtx_clause_ptr, vertex_instruction_ptr );
	}
	else
	{
		pAsm->cf_current_vtx_clause_ptr->m_Word1.f.count++;
	}

	AddVTXInstruction(pAsm->pR700Shader, vertex_instruction_ptr);

	return GL_TRUE;
}

GLboolean add_tex_instruction(r700_AssemblerBase*     pAsm,
                              R700TextureInstruction* tex_instruction_ptr)
{ 
    if ( GL_FALSE == check_current_clause(pAsm, CF_TEX_CLAUSE) )
    {
        return GL_FALSE;
    }

    if ( pAsm->cf_current_tex_clause_ptr == NULL ||
         ( (pAsm->cf_current_tex_clause_ptr != NULL) && 
           (pAsm->cf_current_tex_clause_ptr->m_Word1.f.count >= GetCFMaxInstructions(pAsm->cf_current_tex_clause_ptr->m_ShaderInstType)-1) 
         ) ) 
    {
        // new tex cf instruction for this new clause  
        pAsm->cf_current_tex_clause_ptr = (R700ControlFlowGenericClause*) CALLOC_STRUCT(R700ControlFlowGenericClause);

		if (pAsm->cf_current_tex_clause_ptr != NULL) 
		{
			Init_R700ControlFlowGenericClause(pAsm->cf_current_tex_clause_ptr);
			AddCFInstruction( pAsm->pR700Shader, 
                              (R700ControlFlowInstruction *)pAsm->cf_current_tex_clause_ptr );
		}
		else 
		{
            radeon_error("Could not allocate a new TEX CF instruction.\n");
			return GL_FALSE;
		}
        
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.pop_count        = 0x0;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.cf_const         = 0x0;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

        pAsm->cf_current_tex_clause_ptr->m_Word1.f.end_of_program   = 0x0;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_TEX;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.barrier          = 0x0;   //0x1;
    }
    else 
    {        
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.count++;
    }

    // If this clause constains any TEX instruction that is dependent on a previous instruction, 
    // set the barrier bit
    if( pAsm->pInstDeps[pAsm->uiCurInst].nDstDep > (-1) || pAsm->need_tex_barrier == GL_TRUE )
    {
        pAsm->cf_current_tex_clause_ptr->m_Word1.f.barrier = 0x1;  
    }

    if(NULL == pAsm->cf_current_tex_clause_ptr->m_pLinkedTEXInstruction)
    {
        pAsm->cf_current_tex_clause_ptr->m_pLinkedTEXInstruction = tex_instruction_ptr;
        tex_instruction_ptr->m_pLinkedGenericClause = pAsm->cf_current_tex_clause_ptr;
    }

    AddTEXInstruction(pAsm->pR700Shader, tex_instruction_ptr);

    return GL_TRUE;
}

GLboolean assemble_vfetch_instruction(r700_AssemblerBase* pAsm,
								GLuint gl_client_id,
                                GLuint destination_register,
								GLuint number_of_elements,
                                GLenum dataElementType,
								VTX_FETCH_METHOD* pFetchMethod)
{
    GLuint client_size_inbyte;
	GLuint data_format;
    GLuint mega_fetch_count;
	GLuint is_mega_fetch_flag;

	R700VertexGenericFetch*   vfetch_instruction_ptr;
	R700VertexGenericFetch*   assembled_vfetch_instruction_ptr = pAsm->vfetch_instruction_ptr_array[ gl_client_id ];

	if (assembled_vfetch_instruction_ptr == NULL) 
	{
		vfetch_instruction_ptr = (R700VertexGenericFetch*) CALLOC_STRUCT(R700VertexGenericFetch);
		if (vfetch_instruction_ptr == NULL) 
		{
			return GL_FALSE;
		}
        Init_R700VertexGenericFetch(vfetch_instruction_ptr);
    }
	else 
	{
		vfetch_instruction_ptr = assembled_vfetch_instruction_ptr;
	}

	data_format = GetSurfaceFormat(dataElementType, number_of_elements, &client_size_inbyte);

	if(GL_TRUE == pFetchMethod->bEnableMini) //More conditions here
	{
		//TODO : mini fetch
		mega_fetch_count = 0;
		is_mega_fetch_flag = 0;
	}
	else
	{
		mega_fetch_count = MEGA_FETCH_BYTES - 1;
		is_mega_fetch_flag       = 0x1;
		pFetchMethod->mega_fetch_remainder = MEGA_FETCH_BYTES - client_size_inbyte;
	}

	vfetch_instruction_ptr->m_Word0.f.vtx_inst         = SQ_VTX_INST_FETCH;
	vfetch_instruction_ptr->m_Word0.f.fetch_type       = SQ_VTX_FETCH_VERTEX_DATA;
	vfetch_instruction_ptr->m_Word0.f.fetch_whole_quad = 0x0;

	vfetch_instruction_ptr->m_Word0.f.buffer_id        = gl_client_id;
	vfetch_instruction_ptr->m_Word0.f.src_gpr          = 0x0; 
	vfetch_instruction_ptr->m_Word0.f.src_rel          = SQ_ABSOLUTE;
	vfetch_instruction_ptr->m_Word0.f.src_sel_x        = SQ_SEL_X;
	vfetch_instruction_ptr->m_Word0.f.mega_fetch_count = mega_fetch_count;

	vfetch_instruction_ptr->m_Word1.f.dst_sel_x        = (number_of_elements < 1) ? SQ_SEL_0 : SQ_SEL_X;
	vfetch_instruction_ptr->m_Word1.f.dst_sel_y        = (number_of_elements < 2) ? SQ_SEL_0 : SQ_SEL_Y;
	vfetch_instruction_ptr->m_Word1.f.dst_sel_z        = (number_of_elements < 3) ? SQ_SEL_0 : SQ_SEL_Z;
	vfetch_instruction_ptr->m_Word1.f.dst_sel_w        = (number_of_elements < 4) ? SQ_SEL_1 : SQ_SEL_W;

	vfetch_instruction_ptr->m_Word1.f.use_const_fields = 1;

	// Destination register
	vfetch_instruction_ptr->m_Word1_GPR.f.dst_gpr = destination_register; 
	vfetch_instruction_ptr->m_Word1_GPR.f.dst_rel = SQ_ABSOLUTE;

	vfetch_instruction_ptr->m_Word2.f.offset              = 0;
	vfetch_instruction_ptr->m_Word2.f.const_buf_no_stride = 0x0;

	vfetch_instruction_ptr->m_Word2.f.mega_fetch          = is_mega_fetch_flag;

	if (assembled_vfetch_instruction_ptr == NULL) 
	{
		if ( GL_FALSE == add_vfetch_instruction(pAsm, (R700VertexInstruction *)vfetch_instruction_ptr) ) 
        {   
			return GL_FALSE;
		}

		if (pAsm->vfetch_instruction_ptr_array[ gl_client_id ] != NULL) 
		{
			return GL_FALSE;
		}
		else 
		{
			pAsm->vfetch_instruction_ptr_array[ gl_client_id ] = vfetch_instruction_ptr;
		}
	}

	return GL_TRUE;
}

GLboolean assemble_vfetch_instruction2(r700_AssemblerBase* pAsm,
                                       GLuint              destination_register,								       
                                       GLenum              type,
                                       GLint               size,
                                       GLubyte             element,
                                       GLuint              _signed,
                                       GLboolean           normalize,
                                       GLenum              format,
                                       VTX_FETCH_METHOD  * pFetchMethod)
{
    GLuint client_size_inbyte;
	GLuint data_format;
    GLuint mega_fetch_count;
	GLuint is_mega_fetch_flag;

	R700VertexGenericFetch*   vfetch_instruction_ptr;
	R700VertexGenericFetch*   assembled_vfetch_instruction_ptr 
                                     = pAsm->vfetch_instruction_ptr_array[element];

	if (assembled_vfetch_instruction_ptr == NULL) 
	{
		vfetch_instruction_ptr = (R700VertexGenericFetch*) CALLOC_STRUCT(R700VertexGenericFetch);
		if (vfetch_instruction_ptr == NULL) 
		{
			return GL_FALSE;
		}
        Init_R700VertexGenericFetch(vfetch_instruction_ptr);
    }
	else 
	{
		vfetch_instruction_ptr = assembled_vfetch_instruction_ptr;
	}

    data_format = GetSurfaceFormat(type, size, &client_size_inbyte);	

	if(GL_TRUE == pFetchMethod->bEnableMini) //More conditions here
	{
		//TODO : mini fetch
		mega_fetch_count = 0;
		is_mega_fetch_flag = 0;
	}
	else
	{
		mega_fetch_count = MEGA_FETCH_BYTES - 1;
		is_mega_fetch_flag       = 0x1;
		pFetchMethod->mega_fetch_remainder = MEGA_FETCH_BYTES - client_size_inbyte;
	}

	vfetch_instruction_ptr->m_Word0.f.vtx_inst         = SQ_VTX_INST_FETCH;
	vfetch_instruction_ptr->m_Word0.f.fetch_type       = SQ_VTX_FETCH_VERTEX_DATA;
	vfetch_instruction_ptr->m_Word0.f.fetch_whole_quad = 0x0;

	vfetch_instruction_ptr->m_Word0.f.buffer_id        = element;
	vfetch_instruction_ptr->m_Word0.f.src_gpr          = 0x0; 
	vfetch_instruction_ptr->m_Word0.f.src_rel          = SQ_ABSOLUTE;
	vfetch_instruction_ptr->m_Word0.f.src_sel_x        = SQ_SEL_X;
	vfetch_instruction_ptr->m_Word0.f.mega_fetch_count = mega_fetch_count;

	if(format == GL_BGRA)
	{
		vfetch_instruction_ptr->m_Word1.f.dst_sel_x        = (size < 1) ? SQ_SEL_0 : SQ_SEL_Z;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_y        = (size < 2) ? SQ_SEL_0 : SQ_SEL_Y;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_z        = (size < 3) ? SQ_SEL_0 : SQ_SEL_X;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_w        = (size < 4) ? SQ_SEL_1 : SQ_SEL_W;
	}
	else
	{
		vfetch_instruction_ptr->m_Word1.f.dst_sel_x        = (size < 1) ? SQ_SEL_0 : SQ_SEL_X;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_y        = (size < 2) ? SQ_SEL_0 : SQ_SEL_Y;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_z        = (size < 3) ? SQ_SEL_0 : SQ_SEL_Z;
		vfetch_instruction_ptr->m_Word1.f.dst_sel_w        = (size < 4) ? SQ_SEL_1 : SQ_SEL_W;

	}

	vfetch_instruction_ptr->m_Word1.f.use_const_fields = 1;
    vfetch_instruction_ptr->m_Word1.f.data_format      = data_format;
    vfetch_instruction_ptr->m_Word2.f.endian_swap      = SQ_ENDIAN_NONE;

    if(1 == _signed)
    {
        vfetch_instruction_ptr->m_Word1.f.format_comp_all  = SQ_FORMAT_COMP_SIGNED;
    }
    else
    {
        vfetch_instruction_ptr->m_Word1.f.format_comp_all  = SQ_FORMAT_COMP_UNSIGNED;
    }

    if(GL_TRUE == normalize)
    {
        vfetch_instruction_ptr->m_Word1.f.num_format_all   = SQ_NUM_FORMAT_NORM;
    }
    else
    {
        vfetch_instruction_ptr->m_Word1.f.num_format_all   = SQ_NUM_FORMAT_INT;
    }

	// Destination register
	vfetch_instruction_ptr->m_Word1_GPR.f.dst_gpr = destination_register; 
	vfetch_instruction_ptr->m_Word1_GPR.f.dst_rel = SQ_ABSOLUTE;

	vfetch_instruction_ptr->m_Word2.f.offset              = 0;
	vfetch_instruction_ptr->m_Word2.f.const_buf_no_stride = 0x0;

	vfetch_instruction_ptr->m_Word2.f.mega_fetch          = is_mega_fetch_flag;

	if (assembled_vfetch_instruction_ptr == NULL) 
	{
		if ( GL_FALSE == add_vfetch_instruction(pAsm, (R700VertexInstruction *)vfetch_instruction_ptr) ) 
        {   
			return GL_FALSE;
		}

		if (pAsm->vfetch_instruction_ptr_array[element] != NULL) 
		{
			return GL_FALSE;
		}
		else 
		{
			pAsm->vfetch_instruction_ptr_array[element] = vfetch_instruction_ptr;
		}
	}

	return GL_TRUE;
}

GLboolean cleanup_vfetch_instructions(r700_AssemblerBase* pAsm)
{
    GLint i;
    pAsm->cf_current_clause_type    = CF_EMPTY_CLAUSE;
    pAsm->cf_current_vtx_clause_ptr = NULL;

    for (i=0; i<VERT_ATTRIB_MAX; i++) 
	{
		pAsm->vfetch_instruction_ptr_array[ i ] = NULL;
	}

    cleanup_vfetch_shaderinst(pAsm->pR700Shader);
    
    return GL_TRUE;
}

GLuint gethelpr(r700_AssemblerBase* pAsm) 
{
    GLuint r = pAsm->uHelpReg;
    pAsm->uHelpReg++;
    if (pAsm->uHelpReg > pAsm->number_used_registers)
    {
        pAsm->number_used_registers = pAsm->uHelpReg;
	}
    return r;
}
void resethelpr(r700_AssemblerBase* pAsm) 
{
    pAsm->uHelpReg = pAsm->uFirstHelpReg;
}

void checkop_init(r700_AssemblerBase* pAsm)
{
    resethelpr(pAsm);
    pAsm->aArgSubst[0] =
    pAsm->aArgSubst[1] =
    pAsm->aArgSubst[2] =
    pAsm->aArgSubst[3] = -1;
}

GLboolean mov_temp(r700_AssemblerBase* pAsm, int src)
{
    GLuint tmp = gethelpr(pAsm);

    //mov src to temp helper gpr.
    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
  
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = tmp;

    nomask_PVSDST(&(pAsm->D.dst));

    if( GL_FALSE == assemble_src(pAsm, src, 0) )
    {
        return GL_FALSE;
    }

    noswizzle_PVSSRC(&(pAsm->S[0].src));
    noneg_PVSSRC(&(pAsm->S[0].src));
   
    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    pAsm->aArgSubst[1 + src] = tmp;

    return GL_TRUE;
}

GLboolean checkop1(r700_AssemblerBase* pAsm)
{
    checkop_init(pAsm);
    return GL_TRUE;
}

GLboolean checkop2(r700_AssemblerBase* pAsm)
{
    GLboolean bSrcConst[2];
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    checkop_init(pAsm);

    if( (pILInst->SrcReg[0].File == PROGRAM_UNIFORM)     || 
        (pILInst->SrcReg[0].File == PROGRAM_CONSTANT)    ||
        (pILInst->SrcReg[0].File == PROGRAM_LOCAL_PARAM) ||
        (pILInst->SrcReg[0].File == PROGRAM_ENV_PARAM)   ||
        (pILInst->SrcReg[0].File == PROGRAM_STATE_VAR) )
    {
        bSrcConst[0] = GL_TRUE;
    }
    else
    {
        bSrcConst[0] = GL_FALSE;
    }
    if( (pILInst->SrcReg[1].File == PROGRAM_UNIFORM)     || 
        (pILInst->SrcReg[1].File == PROGRAM_CONSTANT)    ||
        (pILInst->SrcReg[1].File == PROGRAM_LOCAL_PARAM) ||
        (pILInst->SrcReg[1].File == PROGRAM_ENV_PARAM)   ||
        (pILInst->SrcReg[1].File == PROGRAM_STATE_VAR) )
    {
        bSrcConst[1] = GL_TRUE;
    }
    else
    {
        bSrcConst[1] = GL_FALSE;
    }

    if( (bSrcConst[0] == GL_TRUE) && (bSrcConst[1] == GL_TRUE) )
    {
        if(pILInst->SrcReg[0].Index != pILInst->SrcReg[1].Index)
        {
            if( GL_FALSE == mov_temp(pAsm, 1) )
            {
                return GL_FALSE;
            }
        }
    }

    return GL_TRUE;
}

GLboolean checkop3(r700_AssemblerBase* pAsm)
{
    GLboolean bSrcConst[3];
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    checkop_init(pAsm);

    if( (pILInst->SrcReg[0].File == PROGRAM_UNIFORM)     || 
        (pILInst->SrcReg[0].File == PROGRAM_CONSTANT)    ||
        (pILInst->SrcReg[0].File == PROGRAM_LOCAL_PARAM) ||
        (pILInst->SrcReg[0].File == PROGRAM_ENV_PARAM)   ||
        (pILInst->SrcReg[0].File == PROGRAM_STATE_VAR) )
    {
        bSrcConst[0] = GL_TRUE;
    }
    else
    {
        bSrcConst[0] = GL_FALSE;
    }
    if( (pILInst->SrcReg[1].File == PROGRAM_UNIFORM)     || 
        (pILInst->SrcReg[1].File == PROGRAM_CONSTANT)    ||
        (pILInst->SrcReg[1].File == PROGRAM_LOCAL_PARAM) ||
        (pILInst->SrcReg[1].File == PROGRAM_ENV_PARAM)   ||
        (pILInst->SrcReg[1].File == PROGRAM_STATE_VAR) )
    {
        bSrcConst[1] = GL_TRUE;
    }
    else
    {
        bSrcConst[1] = GL_FALSE;
    }
    if( (pILInst->SrcReg[2].File == PROGRAM_UNIFORM)     || 
        (pILInst->SrcReg[2].File == PROGRAM_CONSTANT)    ||
        (pILInst->SrcReg[2].File == PROGRAM_LOCAL_PARAM) ||
        (pILInst->SrcReg[2].File == PROGRAM_ENV_PARAM)   ||
        (pILInst->SrcReg[2].File == PROGRAM_STATE_VAR) )
    {
        bSrcConst[2] = GL_TRUE;
    }
    else
    {
        bSrcConst[2] = GL_FALSE;
    }

    if( (GL_TRUE == bSrcConst[0]) && 
        (GL_TRUE == bSrcConst[1]) && 
        (GL_TRUE == bSrcConst[2]) ) 
    {
        if( GL_FALSE == mov_temp(pAsm, 1) )
        {
            return GL_FALSE;
        }
        if( GL_FALSE == mov_temp(pAsm, 2) )
        {
            return GL_FALSE;
        }

        return GL_TRUE;
    }
    else if( (GL_TRUE == bSrcConst[0]) && 
             (GL_TRUE == bSrcConst[1]) ) 
    {
        if(pILInst->SrcReg[0].Index != pILInst->SrcReg[1].Index)    
	    {
            if( GL_FALSE == mov_temp(pAsm, 1) )
            {
                return 1;
            }
        }

        return GL_TRUE;
    }
    else if ( (GL_TRUE == bSrcConst[0]) && 
              (GL_TRUE == bSrcConst[2]) )  
    {
        if(pILInst->SrcReg[0].Index != pILInst->SrcReg[2].Index)     
	    {
            if( GL_FALSE == mov_temp(pAsm, 2) )
            {
                return GL_FALSE;
            }
        }

        return GL_TRUE;
    }
    else if( (GL_TRUE == bSrcConst[1]) && 
             (GL_TRUE == bSrcConst[2]) ) 
    {
        if(pILInst->SrcReg[1].Index != pILInst->SrcReg[2].Index)
	    {
            if( GL_FALSE == mov_temp(pAsm, 2) )
            {
                return GL_FALSE;
            }
        }

        return GL_TRUE;
    }

    return GL_TRUE;
}

GLboolean assemble_src(r700_AssemblerBase *pAsm,
                       int src, 
                       int fld)
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    if (fld == -1)
    {
        fld = src;
    }

    if(pAsm->aArgSubst[1+src] >= 0) 
    {
        assert(fld >= 0);
        setaddrmode_PVSSRC(&(pAsm->S[fld].src), ADDR_ABSOLUTE);
        pAsm->S[fld].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[fld].src.reg   = pAsm->aArgSubst[1+src];
    }
    else 
    {
        switch (pILInst->SrcReg[src].File)
        {
        case PROGRAM_TEMPORARY:
            setaddrmode_PVSSRC(&(pAsm->S[fld].src), ADDR_ABSOLUTE);
            pAsm->S[fld].src.rtype = SRC_REG_TEMPORARY;
            pAsm->S[fld].src.reg = pILInst->SrcReg[src].Index + pAsm->starting_temp_register_number;
            break;
        case PROGRAM_CONSTANT:
        case PROGRAM_LOCAL_PARAM:
        case PROGRAM_ENV_PARAM:
        case PROGRAM_STATE_VAR:
        case PROGRAM_UNIFORM:
            if (1 == pILInst->SrcReg[src].RelAddr)
            {
                setaddrmode_PVSSRC(&(pAsm->S[fld].src), ADDR_RELATIVE_A0);
            }
            else
            {
                setaddrmode_PVSSRC(&(pAsm->S[fld].src), ADDR_ABSOLUTE);              
            }

            pAsm->S[fld].src.rtype = SRC_REG_CONSTANT;
            if(pILInst->SrcReg[src].Index < 0)
            {
                WARN_ONCE("Negative register offsets not supported yet!\n");
                pAsm->S[fld].src.reg  = 0;
            } 
            else
            {
                pAsm->S[fld].src.reg = pILInst->SrcReg[src].Index;
            }
            break;      
        case PROGRAM_INPUT:
            setaddrmode_PVSSRC(&(pAsm->S[fld].src), ADDR_ABSOLUTE); 
            pAsm->S[fld].src.rtype = SRC_REG_INPUT;
            switch (pAsm->currentShaderType)
            {
            case SPT_FP:
                pAsm->S[fld].src.reg = pAsm->uiFP_AttributeMap[pILInst->SrcReg[src].Index];
                break;
            case SPT_VP:
                pAsm->S[fld].src.reg = pAsm->ucVP_AttributeMap[pILInst->SrcReg[src].Index];
                break;
            }
            break;      
        default:
            radeon_error("Invalid source argument type : %d \n", pILInst->SrcReg[src].File);
            return GL_FALSE;
        }
    } 

    pAsm->S[fld].src.swizzlex = pILInst->SrcReg[src].Swizzle & 0x7;
    pAsm->S[fld].src.swizzley = (pILInst->SrcReg[src].Swizzle >> 3) & 0x7;
    pAsm->S[fld].src.swizzlez = (pILInst->SrcReg[src].Swizzle >> 6) & 0x7;
    pAsm->S[fld].src.swizzlew = (pILInst->SrcReg[src].Swizzle >> 9) & 0x7;

    pAsm->S[fld].src.negx = pILInst->SrcReg[src].Negate & 0x1;
    pAsm->S[fld].src.negy = (pILInst->SrcReg[src].Negate >> 1) & 0x1;
    pAsm->S[fld].src.negz = (pILInst->SrcReg[src].Negate >> 2) & 0x1;
    pAsm->S[fld].src.negw = (pILInst->SrcReg[src].Negate >> 3) & 0x1;
     
    return GL_TRUE;
}

GLboolean assemble_dst(r700_AssemblerBase *pAsm)
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);
    switch (pILInst->DstReg.File) 
    {
    case PROGRAM_TEMPORARY:
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg = pILInst->DstReg.Index + pAsm->starting_temp_register_number;
        break;
    case PROGRAM_ADDRESS:
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_A0;
        pAsm->D.dst.reg = 0;
        break;
    case PROGRAM_OUTPUT:
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_OUT;
        switch (pAsm->currentShaderType)
        {
        case SPT_FP:
            pAsm->D.dst.reg = pAsm->uiFP_OutputMap[pILInst->DstReg.Index];
            break;
        case SPT_VP:
            pAsm->D.dst.reg = pAsm->ucVP_OutputMap[pILInst->DstReg.Index];
            break;
        }
        break;   
    default:
        radeon_error("Invalid destination output argument type\n");
        return GL_FALSE;
    }

    pAsm->D.dst.writex = pILInst->DstReg.WriteMask & 0x1;
    pAsm->D.dst.writey = (pILInst->DstReg.WriteMask >> 1) & 0x1;
    pAsm->D.dst.writez = (pILInst->DstReg.WriteMask >> 2) & 0x1;
    pAsm->D.dst.writew = (pILInst->DstReg.WriteMask >> 3) & 0x1;
  
    if(pILInst->SaturateMode == SATURATE_ZERO_ONE)
    {
        pAsm->D2.dst2.SaturateMode = 1;
    }
    else
    {
        pAsm->D2.dst2.SaturateMode = 0;
    }

    return GL_TRUE;
}

GLboolean tex_dst(r700_AssemblerBase *pAsm)
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    if(PROGRAM_TEMPORARY == pILInst->DstReg.File)
    {
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = pAsm->pILInst[pAsm->uiCurInst].DstReg.Index + pAsm->starting_temp_register_number;

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    }
    else if(PROGRAM_OUTPUT == pILInst->DstReg.File)
    {
        pAsm->D.dst.rtype = DST_REG_OUT;
        switch (pAsm->currentShaderType)
        {
        case SPT_FP:
            pAsm->D.dst.reg = pAsm->uiFP_OutputMap[pILInst->DstReg.Index];
            break;
        case SPT_VP:
            pAsm->D.dst.reg = pAsm->ucVP_OutputMap[pILInst->DstReg.Index];
            break;
        }

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    }
    else 
    {
        radeon_error("Invalid destination output argument type\n");
        return GL_FALSE;
    }

    pAsm->D.dst.writex = pILInst->DstReg.WriteMask & 0x1;
    pAsm->D.dst.writey = (pILInst->DstReg.WriteMask >> 1) & 0x1;
    pAsm->D.dst.writez = (pILInst->DstReg.WriteMask >> 2) & 0x1;
    pAsm->D.dst.writew = (pILInst->DstReg.WriteMask >> 3) & 0x1;
  
    return GL_TRUE;
}

GLboolean tex_src(r700_AssemblerBase *pAsm)
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    GLboolean bValidTexCoord = GL_FALSE;

    if(pAsm->aArgSubst[1] >= 0)
    {
        bValidTexCoord = GL_TRUE;
        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = pAsm->aArgSubst[1];
    }
    else
    {
    switch (pILInst->SrcReg[0].File) {
        case PROGRAM_UNIFORM: 
        case PROGRAM_CONSTANT:
        case PROGRAM_LOCAL_PARAM:
        case PROGRAM_ENV_PARAM:
        case PROGRAM_STATE_VAR:
            break;
        case PROGRAM_TEMPORARY:
            bValidTexCoord = GL_TRUE;
            pAsm->S[0].src.reg   = pILInst->SrcReg[0].Index +
            pAsm->starting_temp_register_number;
            pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
            break;
        case PROGRAM_INPUT:
            if(SPT_VP == pAsm->currentShaderType)
            {
                switch (pILInst->SrcReg[0].Index)
                {
                    case VERT_ATTRIB_TEX0:
                    case VERT_ATTRIB_TEX1:
                    case VERT_ATTRIB_TEX2:
                    case VERT_ATTRIB_TEX3:
                    case VERT_ATTRIB_TEX4:
                    case VERT_ATTRIB_TEX5:
                    case VERT_ATTRIB_TEX6:
                    case VERT_ATTRIB_TEX7:
                        bValidTexCoord = GL_TRUE;
                        pAsm->S[0].src.reg   =
                            pAsm->ucVP_AttributeMap[pILInst->SrcReg[0].Index];
                        pAsm->S[0].src.rtype = SRC_REG_INPUT;
                        break;
                }
            }
            else
            {
                switch (pILInst->SrcReg[0].Index)
                {
                    case FRAG_ATTRIB_WPOS:
                    case FRAG_ATTRIB_COL0:
                    case FRAG_ATTRIB_COL1:
                    case FRAG_ATTRIB_FOGC:
                    case FRAG_ATTRIB_TEX0:
                    case FRAG_ATTRIB_TEX1:
                    case FRAG_ATTRIB_TEX2:
                    case FRAG_ATTRIB_TEX3:
                    case FRAG_ATTRIB_TEX4:
                    case FRAG_ATTRIB_TEX5:
                    case FRAG_ATTRIB_TEX6:
                    case FRAG_ATTRIB_TEX7:
                        bValidTexCoord = GL_TRUE;
                        pAsm->S[0].src.reg   =
                            pAsm->uiFP_AttributeMap[pILInst->SrcReg[0].Index];
                        pAsm->S[0].src.rtype = SRC_REG_INPUT;
                        break;
                    case FRAG_ATTRIB_FACE:
                        fprintf(stderr, "FRAG_ATTRIB_FACE unsupported\n");
                        break;
                    case FRAG_ATTRIB_PNTC:
                        fprintf(stderr, "FRAG_ATTRIB_PNTC unsupported\n");
                        break;
                }

                if( (pILInst->SrcReg[0].Index >= FRAG_ATTRIB_VAR0) ||
                    (pILInst->SrcReg[0].Index < FRAG_ATTRIB_MAX) )
                {
				    bValidTexCoord = GL_TRUE;
                    pAsm->S[0].src.reg   =
                        pAsm->uiFP_AttributeMap[pILInst->SrcReg[0].Index];
                    pAsm->S[0].src.rtype = SRC_REG_INPUT;
                }
            }

            break;
        }
    }

    if(GL_TRUE == bValidTexCoord)
    {
        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    }
    else
    {
        radeon_error("Invalid source texcoord for TEX instruction\n");
        return GL_FALSE;
    }

    pAsm->S[0].src.swizzlex = pILInst->SrcReg[0].Swizzle & 0x7;
    pAsm->S[0].src.swizzley = (pILInst->SrcReg[0].Swizzle >> 3) & 0x7;
    pAsm->S[0].src.swizzlez = (pILInst->SrcReg[0].Swizzle >> 6) & 0x7;
    pAsm->S[0].src.swizzlew = (pILInst->SrcReg[0].Swizzle >> 9) & 0x7;

    pAsm->S[0].src.negx = pILInst->SrcReg[0].Negate & 0x1;
    pAsm->S[0].src.negy = (pILInst->SrcReg[0].Negate >> 1) & 0x1;
    pAsm->S[0].src.negz = (pILInst->SrcReg[0].Negate >> 2) & 0x1;
    pAsm->S[0].src.negw = (pILInst->SrcReg[0].Negate >> 3) & 0x1;

    return GL_TRUE;
}

GLboolean assemble_tex_instruction(r700_AssemblerBase *pAsm, GLboolean normalized)
{
    PVSSRC *   texture_coordinate_source;
    PVSSRC *   texture_unit_source;
    
    R700TextureInstruction* tex_instruction_ptr = (R700TextureInstruction*) CALLOC_STRUCT(R700TextureInstruction);
	if (tex_instruction_ptr == NULL) 
	{
		return GL_FALSE;
	}
    Init_R700TextureInstruction(tex_instruction_ptr);

    texture_coordinate_source = &(pAsm->S[0].src);
    texture_unit_source       = &(pAsm->S[1].src);

    tex_instruction_ptr->m_Word0.f.tex_inst         = pAsm->D.dst.opcode;
    tex_instruction_ptr->m_Word0.f.bc_frac_mode     = 0x0;
    tex_instruction_ptr->m_Word0.f.fetch_whole_quad = 0x0;
    tex_instruction_ptr->m_Word0.f.alt_const        = 0;

    if(SPT_VP == pAsm->currentShaderType)
    {
        tex_instruction_ptr->m_Word0.f.resource_id      = texture_unit_source->reg + VERT_ATTRIB_MAX;
        pAsm->unVetTexBits |= 1 << texture_unit_source->reg;
    }
    else
    {
        tex_instruction_ptr->m_Word0.f.resource_id      = texture_unit_source->reg;
    }

    tex_instruction_ptr->m_Word1.f.lod_bias     = 0x0;
    if (normalized) {
	    tex_instruction_ptr->m_Word1.f.coord_type_x = SQ_TEX_NORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_y = SQ_TEX_NORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_z = SQ_TEX_NORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_w = SQ_TEX_NORMALIZED;
    } else {
	    /* XXX: UNNORMALIZED tex coords have limited wrap modes */
	    tex_instruction_ptr->m_Word1.f.coord_type_x = SQ_TEX_UNNORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_y = SQ_TEX_UNNORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_z = SQ_TEX_UNNORMALIZED;
	    tex_instruction_ptr->m_Word1.f.coord_type_w = SQ_TEX_UNNORMALIZED;
    }

    tex_instruction_ptr->m_Word2.f.offset_x   = 0x0;
    tex_instruction_ptr->m_Word2.f.offset_y   = 0x0;
    tex_instruction_ptr->m_Word2.f.offset_z   = 0x0;
    tex_instruction_ptr->m_Word2.f.sampler_id = texture_unit_source->reg;

    // dst
    if ( (pAsm->D.dst.rtype == DST_REG_TEMPORARY) || 
         (pAsm->D.dst.rtype == DST_REG_OUT) ) 
    {
        tex_instruction_ptr->m_Word0.f.src_gpr    = texture_coordinate_source->reg;
        tex_instruction_ptr->m_Word0.f.src_rel    = SQ_ABSOLUTE;

        tex_instruction_ptr->m_Word1.f.dst_gpr    = pAsm->D.dst.reg;
        tex_instruction_ptr->m_Word1.f.dst_rel    = SQ_ABSOLUTE;

        tex_instruction_ptr->m_Word1.f.dst_sel_x  = (pAsm->D.dst.writex ? texture_unit_source->swizzlex : SQ_SEL_MASK);
        tex_instruction_ptr->m_Word1.f.dst_sel_y  = (pAsm->D.dst.writey ? texture_unit_source->swizzley : SQ_SEL_MASK);
        tex_instruction_ptr->m_Word1.f.dst_sel_z  = (pAsm->D.dst.writez ? texture_unit_source->swizzlez : SQ_SEL_MASK);
        tex_instruction_ptr->m_Word1.f.dst_sel_w  = (pAsm->D.dst.writew ? texture_unit_source->swizzlew : SQ_SEL_MASK);


        tex_instruction_ptr->m_Word2.f.src_sel_x  = texture_coordinate_source->swizzlex;
        tex_instruction_ptr->m_Word2.f.src_sel_y  = texture_coordinate_source->swizzley;
        tex_instruction_ptr->m_Word2.f.src_sel_z  = texture_coordinate_source->swizzlez;
        tex_instruction_ptr->m_Word2.f.src_sel_w  = texture_coordinate_source->swizzlew;
    }
    else 
    {
        radeon_error("Only temp destination registers supported for TEX dest regs.\n");
        return GL_FALSE;
    }

    if( GL_FALSE == add_tex_instruction(pAsm, tex_instruction_ptr) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

void initialize(r700_AssemblerBase *pAsm)
{
    GLuint cycle, component;

    for (cycle=0; cycle<NUMBER_OF_CYCLES; cycle++) 
    {
        for (component=0; component<NUMBER_OF_COMPONENTS; component++) 
        {
            pAsm->hw_gpr[cycle][component] = (-1);
        }
    }
    for (component=0; component<NUMBER_OF_COMPONENTS; component++) 
    {
        pAsm->hw_cfile_addr[component] = (-1);
        pAsm->hw_cfile_chan[component] = (-1);
    }
}

GLboolean assemble_alu_src(R700ALUInstruction*  alu_instruction_ptr,
                           int                  source_index,
                           PVSSRC*              pSource,
                           BITS                 scalar_channel_index)
{
    BITS src_sel;
    BITS src_rel;
    BITS src_chan;
    BITS src_neg;

    //--------------------------------------------------------------------------
    // Source for operands src0, src1. 
    // Values [0,127] correspond to GPR[0..127]. 
    // Values [256,511] correspond to cfile constants c[0..255]. 

    //--------------------------------------------------------------------------
    // Other special values are shown in the list below.

    // 248	SQ_ALU_SRC_0: special constant 0.0.
    // 249	SQ_ALU_SRC_1: special constant 1.0 float.

    // 250	SQ_ALU_SRC_1_INT: special constant 1 integer.
    // 251	SQ_ALU_SRC_M_1_INT: special constant -1 integer.

    // 252	SQ_ALU_SRC_0_5: special constant 0.5 float.
    // 253	SQ_ALU_SRC_LITERAL: literal constant.

    // 254	SQ_ALU_SRC_PV: previous vector result.
    // 255	SQ_ALU_SRC_PS: previous scalar result.
    //--------------------------------------------------------------------------

    BITS channel_swizzle;
    switch (scalar_channel_index) 
    {
        case 0: channel_swizzle = pSource->swizzlex; break;
        case 1: channel_swizzle = pSource->swizzley; break;
        case 2: channel_swizzle = pSource->swizzlez; break;
        case 3: channel_swizzle = pSource->swizzlew; break;
        default: channel_swizzle = SQ_SEL_MASK; break;
    }

    if(channel_swizzle == SQ_SEL_0) 
    {
        src_sel = SQ_ALU_SRC_0; 
    }
    else if (channel_swizzle == SQ_SEL_1) 
    {
        src_sel = SQ_ALU_SRC_1; 
    }
    else 
    {
        if ( (pSource->rtype == SRC_REG_TEMPORARY) || 
             (pSource->rtype == SRC_REG_INPUT)
        ) 
        {
            src_sel = pSource->reg;
        }
        else if (pSource->rtype == SRC_REG_CONSTANT)
        {
            src_sel = pSource->reg + CFILE_REGISTER_OFFSET;            
        }
        else if (pSource->rtype == SRC_REC_LITERAL)
        {
            src_sel = SQ_ALU_SRC_LITERAL;            
        }
        else
        {
            radeon_error("Source (%d) register type (%d) not one of TEMP, INPUT, or CONSTANT.\n",
                     source_index, pSource->rtype);
            return GL_FALSE;
        }
    }

    if( ADDR_ABSOLUTE == addrmode_PVSSRC(pSource) ) 
    {
        src_rel = SQ_ABSOLUTE;
    }
    else 
    {
        src_rel = SQ_RELATIVE;
    }

    switch (channel_swizzle) 
    {
        case SQ_SEL_X: 
            src_chan = SQ_CHAN_X; 
            break;
        case SQ_SEL_Y: 
            src_chan = SQ_CHAN_Y; 
            break;
        case SQ_SEL_Z: 
            src_chan = SQ_CHAN_Z; 
            break;
        case SQ_SEL_W: 
            src_chan = SQ_CHAN_W; 
            break;
        case SQ_SEL_0:
        case SQ_SEL_1:
            // Does not matter since src_sel controls
            src_chan = SQ_CHAN_X; 
            break;
        default:
            radeon_error("Unknown source select value (%d) in assemble_alu_src().\n", channel_swizzle);
            return GL_FALSE;
            break;
    }

    switch (scalar_channel_index) 
    {
        case 0: src_neg = pSource->negx; break;
        case 1: src_neg = pSource->negy; break;
        case 2: src_neg = pSource->negz; break;
        case 3: src_neg = pSource->negw; break;
        default: src_neg = 0; break;
    }

    switch (source_index) 
    {
        case 0:
            assert(alu_instruction_ptr);
            alu_instruction_ptr->m_Word0.f.src0_sel  = src_sel;
            alu_instruction_ptr->m_Word0.f.src0_rel  = src_rel;
            alu_instruction_ptr->m_Word0.f.src0_chan = src_chan;
            alu_instruction_ptr->m_Word0.f.src0_neg  = src_neg;
            break;
        case 1:
            assert(alu_instruction_ptr);
            alu_instruction_ptr->m_Word0.f.src1_sel  = src_sel;
            alu_instruction_ptr->m_Word0.f.src1_rel  = src_rel;
            alu_instruction_ptr->m_Word0.f.src1_chan = src_chan;
            alu_instruction_ptr->m_Word0.f.src1_neg  = src_neg;
            break;
        case 2:
            assert(alu_instruction_ptr);
            alu_instruction_ptr->m_Word1_OP3.f.src2_sel  = src_sel;
            alu_instruction_ptr->m_Word1_OP3.f.src2_rel  = src_rel;
            alu_instruction_ptr->m_Word1_OP3.f.src2_chan = src_chan;
            alu_instruction_ptr->m_Word1_OP3.f.src2_neg  = src_neg;
            break;
        default:
            radeon_error("Only three sources allowed in ALU opcodes.\n");
          return GL_FALSE;
          break;
    }

    return GL_TRUE;
}

GLboolean add_alu_instruction(r700_AssemblerBase* pAsm,
                              R700ALUInstruction* alu_instruction_ptr,
                              GLuint              contiguous_slots_needed)
{
    if( GL_FALSE == check_current_clause(pAsm, CF_ALU_CLAUSE) )
    {
        return GL_FALSE;
    }

    if ( pAsm->alu_x_opcode != 0 ||
         pAsm->cf_current_alu_clause_ptr == NULL ||
         ( (pAsm->cf_current_alu_clause_ptr != NULL) && 
           (pAsm->cf_current_alu_clause_ptr->m_Word1.f.count >= (GetCFMaxInstructions(pAsm->cf_current_alu_clause_ptr->m_ShaderInstType)-contiguous_slots_needed-1) )
         ) ) 
    {

        //new cf inst for this clause
        pAsm->cf_current_alu_clause_ptr = (R700ControlFlowALUClause*) CALLOC_STRUCT(R700ControlFlowALUClause);
            
        // link the new cf to cf segment    
        if(NULL != pAsm->cf_current_alu_clause_ptr) 
        {
            Init_R700ControlFlowALUClause(pAsm->cf_current_alu_clause_ptr);
			AddCFInstruction( pAsm->pR700Shader, 
                              (R700ControlFlowInstruction *)pAsm->cf_current_alu_clause_ptr );            
        }
        else 
        {
            radeon_error("Could not allocate a new ALU CF instruction.\n");
            return GL_FALSE;
        }

        pAsm->cf_current_alu_clause_ptr->m_Word0.f.kcache_bank0 = 0x0;
        pAsm->cf_current_alu_clause_ptr->m_Word0.f.kcache_bank1 = 0x0;
        pAsm->cf_current_alu_clause_ptr->m_Word0.f.kcache_mode0 = SQ_CF_KCACHE_NOP;

        pAsm->cf_current_alu_clause_ptr->m_Word1.f.kcache_mode1 = SQ_CF_KCACHE_NOP;
        pAsm->cf_current_alu_clause_ptr->m_Word1.f.kcache_addr0 = 0x0;
        pAsm->cf_current_alu_clause_ptr->m_Word1.f.kcache_addr1 = 0x0;

        pAsm->cf_current_alu_clause_ptr->m_Word1.f.count           = 0x0;

        if(pAsm->alu_x_opcode != 0)
        {
            pAsm->cf_current_alu_clause_ptr->m_Word1.f.cf_inst = pAsm->alu_x_opcode;
            pAsm->alu_x_opcode = 0;
        }
        else
        {
            pAsm->cf_current_alu_clause_ptr->m_Word1.f.cf_inst = SQ_CF_INST_ALU;
        }

        pAsm->cf_current_alu_clause_ptr->m_Word1.f.whole_quad_mode = 0x0;

        pAsm->cf_current_alu_clause_ptr->m_Word1.f.barrier         = 0x1;
    }
    else 
    {
        pAsm->cf_current_alu_clause_ptr->m_Word1.f.count += (GetInstructionSize(alu_instruction_ptr->m_ShaderInstType) / 2);
    }

    // If this clause constains any instruction that is forward dependent on a TEX instruction, 
    // set the whole_quad_mode for this clause
    if ( pAsm->pInstDeps[pAsm->uiCurInst].nDstDep > (-1) ) 
    {
        pAsm->cf_current_alu_clause_ptr->m_Word1.f.whole_quad_mode = 0x1;   
    }

    if (pAsm->cf_current_alu_clause_ptr->m_Word1.f.count >= (GetCFMaxInstructions(pAsm->cf_current_alu_clause_ptr->m_ShaderInstType)-1) ) 
    {
        alu_instruction_ptr->m_Word0.f.last = 1;
    }

    if(NULL == pAsm->cf_current_alu_clause_ptr->m_pLinkedALUInstruction)
    {
        pAsm->cf_current_alu_clause_ptr->m_pLinkedALUInstruction = alu_instruction_ptr;
        alu_instruction_ptr->m_pLinkedALUClause = pAsm->cf_current_alu_clause_ptr;
    }
    
    AddALUInstruction(pAsm->pR700Shader, alu_instruction_ptr);

    return GL_TRUE;
}

void get_src_properties(R700ALUInstruction*  alu_instruction_ptr,
                        int                  source_index,
                        BITS*                psrc_sel,
                        BITS*                psrc_rel,
                        BITS*                psrc_chan,
                        BITS*                psrc_neg)
{
    switch (source_index) 
    {
        case 0:
            *psrc_sel  = alu_instruction_ptr->m_Word0.f.src0_sel ;
            *psrc_rel  = alu_instruction_ptr->m_Word0.f.src0_rel ;
            *psrc_chan = alu_instruction_ptr->m_Word0.f.src0_chan;
            *psrc_neg  = alu_instruction_ptr->m_Word0.f.src0_neg ;
            break;

        case 1:
            *psrc_sel  = alu_instruction_ptr->m_Word0.f.src1_sel ;
            *psrc_rel  = alu_instruction_ptr->m_Word0.f.src1_rel ;
            *psrc_chan = alu_instruction_ptr->m_Word0.f.src1_chan;
            *psrc_neg  = alu_instruction_ptr->m_Word0.f.src1_neg ;
            break;

        case 2:
            *psrc_sel  = alu_instruction_ptr->m_Word1_OP3.f.src2_sel;
            *psrc_rel  = alu_instruction_ptr->m_Word1_OP3.f.src2_rel;
            *psrc_chan = alu_instruction_ptr->m_Word1_OP3.f.src2_chan;
            *psrc_neg  = alu_instruction_ptr->m_Word1_OP3.f.src2_neg;
            break;
    }
}

int is_cfile(BITS sel) 
{
    if (sel > 255 && sel < 512) 
    {
        return 1;
    }
    return 0;
}

int is_const(BITS sel) 
{
    if (is_cfile(sel)) 
    {
        return 1;
    }
    else if(sel >= SQ_ALU_SRC_0 && sel <= SQ_ALU_SRC_LITERAL) 
    {
        return 1;
    }
    return 0;
}

int is_gpr(BITS sel) 
{
    if (sel >= 0 && sel < 128) 
    {
        return 1;
    }
    return 0;
}

const GLuint BANK_SWIZZLE_VEC[8] = {SQ_ALU_VEC_210,  //000
                                    SQ_ALU_VEC_120,  //001
                                    SQ_ALU_VEC_102,  //010

                                    SQ_ALU_VEC_201,  //011
                                    SQ_ALU_VEC_012,  //100
                                    SQ_ALU_VEC_021,  //101

                                    SQ_ALU_VEC_012,  //110
                                    SQ_ALU_VEC_012}; //111

const GLuint BANK_SWIZZLE_SCL[8] = {SQ_ALU_SCL_210,  //000
                                    SQ_ALU_SCL_122,  //001 
                                    SQ_ALU_SCL_122,  //010

                                    SQ_ALU_SCL_221,  //011
                                    SQ_ALU_SCL_212,  //100
                                    SQ_ALU_SCL_122,  //101

                                    SQ_ALU_SCL_122,  //110
                                    SQ_ALU_SCL_122}; //111

GLboolean reserve_cfile(r700_AssemblerBase* pAsm, 
                        GLuint sel, 
                        GLuint chan)
{
    int res_match = (-1);
    int res_empty = (-1);

    GLint res;

    for (res=3; res>=0; res--) 
    {
        if(pAsm->hw_cfile_addr[ res] < 0)  
        {
            res_empty = res;
        }
        else if( (pAsm->hw_cfile_addr[res] == (int)sel)
                 &&
                 (pAsm->hw_cfile_chan[ res ] == (int) chan) ) 
        {
            res_match = res;
        }
    }

    if(res_match >= 0) 
    {
        // Read for this scalar component already reserved, nothing to do here.
        ;
    }
    else if(res_empty >= 0) 
    {
        pAsm->hw_cfile_addr[ res_empty ] = sel;
        pAsm->hw_cfile_chan[ res_empty ] = chan;
    }
    else 
    {
        radeon_error("All cfile read ports are used, cannot reference C$sel, channel $chan.\n");
        return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean reserve_gpr(r700_AssemblerBase* pAsm, GLuint sel, GLuint chan, GLuint cycle)
{
    if(pAsm->hw_gpr[cycle][chan] < 0) 
    {
        pAsm->hw_gpr[cycle][chan] = sel;
    }
    else if(pAsm->hw_gpr[cycle][chan] != (int)sel) 
    {
        radeon_error("Another scalar operation has already used GPR read port for given channel\n");
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean cycle_for_scalar_bank_swizzle(const int swiz, const int sel, GLuint* pCycle)
{
    switch (swiz) 
    {
        case SQ_ALU_SCL_210:
            {
                int table[3] = {2,	1,	0};
                *pCycle = table[sel];
                return GL_TRUE;
            }
            break;
        case SQ_ALU_SCL_122:
            {
                int table[3] = {1,	2,	2};
                *pCycle = table[sel];
                return GL_TRUE;
            }
            break;
        case SQ_ALU_SCL_212:
            {	
                int table[3] = {2,	1,	2};
                *pCycle = table[sel];
                return GL_TRUE;
            }
            break;
        case SQ_ALU_SCL_221:
            {
                int table[3] = {2, 2, 1};
                *pCycle = table[sel];
                return GL_TRUE;
            }
            break;
        default:
            radeon_error("Bad Scalar bank swizzle value\n");
            break;
    }

    return GL_FALSE;
}

GLboolean cycle_for_vector_bank_swizzle(const int swiz, const int sel, GLuint* pCycle)
{
    switch (swiz) 
    {
        case SQ_ALU_VEC_012:
            {
                int table[3] = {0, 1, 2};
                *pCycle = table[sel];
            }
            break;
        case SQ_ALU_VEC_021:
            {
                int table[3] = {0, 2,	1};
                *pCycle = table[sel];
            }
            break;        
        case SQ_ALU_VEC_120:
            {
                int table[3] = {1, 2,	0};
                *pCycle = table[sel];
            }
            break;
        case SQ_ALU_VEC_102:
            {
                int table[3] = {1, 0,	2};
                *pCycle = table[sel];
            }
            break;
        case SQ_ALU_VEC_201:
            {
                int table[3] = {2, 0,	1};
                *pCycle = table[sel];
            }
            break;
        case SQ_ALU_VEC_210:
            {
                int table[3] = {2, 1,	0};
                *pCycle = table[sel];
            }
            break;
        default:
            radeon_error("Bad Vec bank swizzle value\n");
            return GL_FALSE;
            break;
    }

    return GL_TRUE;
}

GLboolean check_scalar(r700_AssemblerBase* pAsm,
                       R700ALUInstruction* alu_instruction_ptr)
{
    GLuint cycle;
    GLuint bank_swizzle;
    GLuint const_count = 0;

    BITS sel;
    BITS chan;
    BITS rel;
    BITS neg;

    GLuint src;

    BITS src_sel [3] = {0,0,0};
    BITS src_chan[3] = {0,0,0};
    BITS src_rel [3] = {0,0,0};
    BITS src_neg [3] = {0,0,0};

    GLuint swizzle_key;

    GLuint number_of_operands = r700GetNumOperands(pAsm->D.dst.opcode, pAsm->D.dst.op3);

    for (src=0; src<number_of_operands; src++) 
    {
        get_src_properties(alu_instruction_ptr,
                           src,
                           &(src_sel[src]), 
                           &(src_rel[src]), 
                           &(src_chan[src]), 
                           &(src_neg[src]) );
    }


    swizzle_key = ( (is_const( src_sel[0] ) ? 4 : 0) + 
                    (is_const( src_sel[1] ) ? 2 : 0) + 
                    (is_const( src_sel[2] ) ? 1 : 0) );
  
    alu_instruction_ptr->m_Word1.f.bank_swizzle = BANK_SWIZZLE_SCL[ swizzle_key ];

    for (src=0; src<number_of_operands; src++) 
    {
        sel  = src_sel [src];
        chan = src_chan[src];
        rel  = src_rel [src];
        neg  = src_neg [src];

        if (is_const( sel )) 
        {
            // Any constant, including literal and inline constants
            const_count++;

            if (is_cfile( sel )) 
            {
                reserve_cfile(pAsm, sel, chan);
            }

        }
    }

    for (src=0; src<number_of_operands; src++) 
    {
        sel  = src_sel [src];
        chan = src_chan[src];
        rel  = src_rel [src];
        neg  = src_neg [src];

        if( is_gpr(sel) ) 
        {
            bank_swizzle = alu_instruction_ptr->m_Word1.f.bank_swizzle;

            if( GL_FALSE == cycle_for_scalar_bank_swizzle(bank_swizzle, src, &cycle) )
            {
                return GL_FALSE;
            }

            if(cycle < const_count) 
            {
                if( GL_FALSE == reserve_gpr(pAsm, sel, chan, cycle) )
                {
                    return GL_FALSE;
                }
            }
        }
    }

    return GL_TRUE;
}

GLboolean check_vector(r700_AssemblerBase* pAsm,
                       R700ALUInstruction* alu_instruction_ptr)
{
    GLuint cycle;
    GLuint bank_swizzle;
    GLuint const_count = 0;

    GLuint src;

    BITS sel;
    BITS chan;
    BITS rel;
    BITS neg;

    BITS src_sel [3] = {0,0,0};
    BITS src_chan[3] = {0,0,0};
    BITS src_rel [3] = {0,0,0};
    BITS src_neg [3] = {0,0,0};

    GLuint swizzle_key;

    GLuint number_of_operands = r700GetNumOperands(pAsm->D.dst.opcode, pAsm->D.dst.op3);

    for (src=0; src<number_of_operands; src++) 
    {
        get_src_properties(alu_instruction_ptr,
                           src,
                           &(src_sel[src]), 
                           &(src_rel[src]), 
                           &(src_chan[src]), 
                           &(src_neg[src]) );
    }


    swizzle_key = ( (is_const( src_sel[0] ) ? 4 : 0) + 
                           (is_const( src_sel[1] ) ? 2 : 0) + 
                           (is_const( src_sel[2] ) ? 1 : 0) 
                         );

    alu_instruction_ptr->m_Word1.f.bank_swizzle = BANK_SWIZZLE_VEC[swizzle_key];

    for (src=0; src<number_of_operands; src++) 
    {
        sel  = src_sel [src];
        chan = src_chan[src];
        rel  = src_rel [src];
        neg  = src_neg [src];


        bank_swizzle = alu_instruction_ptr->m_Word1.f.bank_swizzle;

        if( is_gpr(sel) ) 
        {
            if( GL_FALSE == cycle_for_vector_bank_swizzle(bank_swizzle, src, &cycle) )
            {             
                return GL_FALSE;
            }

            if ( (src  == 1)          && 
                 (sel  == src_sel[0]) &&
                 (chan == src_chan[0]) ) 
            {        
            }
            else 
            {
                if( GL_FALSE == reserve_gpr(pAsm, sel, chan, cycle) )
                {                    
                    return GL_FALSE;
                }
            }
        }
        else if( is_const(sel) ) 
        {                  
            const_count++;

            if( is_cfile(sel) ) 
            {        
                if( GL_FALSE == reserve_cfile(pAsm, sel, chan) )
                {                    
                    return GL_FALSE;
                }
            }
        }
    }

    return GL_TRUE;
}

GLboolean assemble_alu_instruction(r700_AssemblerBase *pAsm)
{
    R700ALUInstruction            * alu_instruction_ptr = NULL;
    R700ALUInstructionHalfLiteral * alu_instruction_ptr_hl;
    R700ALUInstructionFullLiteral * alu_instruction_ptr_fl;

    GLuint    number_of_scalar_operations;
    GLboolean is_single_scalar_operation;
    GLuint    scalar_channel_index;

    PVSSRC * pcurrent_source;
    int    current_source_index;
    GLuint contiguous_slots_needed;

    GLuint    uNumSrc = r700GetNumOperands(pAsm->D.dst.opcode, pAsm->D.dst.op3);
    //GLuint    channel_swizzle, j;
    //GLuint    chan_counter[4] = {0, 0, 0, 0};
    //PVSSRC *  pSource[3];
    GLboolean bSplitInst = GL_FALSE;

    if (1 == pAsm->D.dst.math) 
    {
        is_single_scalar_operation = GL_TRUE;
        number_of_scalar_operations = 1;
    }
    else 
    {
        is_single_scalar_operation = GL_FALSE;
        number_of_scalar_operations = 4;

/* current assembler doesn't do more than 1 register per source */
#if 0
        /* check read port, only very preliminary algorithm, not count in 
           src0/1 same comp case and prev slot repeat case; also not count relative
           addressing. TODO: improve performance. */
        for(j=0; j<uNumSrc; j++)
        {
            pSource[j] = &(pAsm->S[j].src);
        }
        for(scalar_channel_index=0; scalar_channel_index<4; scalar_channel_index++) 
        {
            for(j=0; j<uNumSrc; j++) 
            {
                switch (scalar_channel_index) 
                {
                    case 0: channel_swizzle = pSource[j]->swizzlex; break;
                    case 1: channel_swizzle = pSource[j]->swizzley; break;
                    case 2: channel_swizzle = pSource[j]->swizzlez; break;
                    case 3: channel_swizzle = pSource[j]->swizzlew; break;
                    default: channel_swizzle = SQ_SEL_MASK; break;
                }
                if ( ((pSource[j]->rtype == SRC_REG_TEMPORARY) || 
                     (pSource[j]->rtype == SRC_REG_INPUT))
                     && (channel_swizzle <= SQ_SEL_W) )
                {                    
                    chan_counter[channel_swizzle]++;                        
                }
            }
        }
        if(   (chan_counter[SQ_SEL_X] > 3)
           || (chan_counter[SQ_SEL_Y] > 3)
           || (chan_counter[SQ_SEL_Z] > 3)
           || (chan_counter[SQ_SEL_W] > 3) ) /* each chan bank has only 3 ports. */
        {
            bSplitInst = GL_TRUE;
        }
#endif
    }

    contiguous_slots_needed = 0;

    if(!is_single_scalar_operation) 
    {
        contiguous_slots_needed = 4;
    }

    contiguous_slots_needed += pAsm->D2.dst2.literal_slots;

    initialize(pAsm);    

    for (scalar_channel_index=0;
            scalar_channel_index < number_of_scalar_operations; 
                scalar_channel_index++) 
    {
        if(scalar_channel_index == (number_of_scalar_operations-1))
        {
            switch(pAsm->D2.dst2.literal_slots)
            {
            case 0:
                alu_instruction_ptr = (R700ALUInstruction*) CALLOC_STRUCT(R700ALUInstruction);
                Init_R700ALUInstruction(alu_instruction_ptr);
                break;
            case 1:
                alu_instruction_ptr_hl = (R700ALUInstructionHalfLiteral*) CALLOC_STRUCT(R700ALUInstructionHalfLiteral);
                Init_R700ALUInstructionHalfLiteral(alu_instruction_ptr_hl, pAsm->C[0].f, pAsm->C[1].f);
                alu_instruction_ptr = (R700ALUInstruction*)alu_instruction_ptr_hl;
                break;
            case 2:
                alu_instruction_ptr_fl = (R700ALUInstructionFullLiteral*) CALLOC_STRUCT(R700ALUInstructionFullLiteral);
                Init_R700ALUInstructionFullLiteral(alu_instruction_ptr_fl,pAsm->C[0].f, pAsm->C[1].f, pAsm->C[2].f, pAsm->C[3].f);
                alu_instruction_ptr = (R700ALUInstruction*)alu_instruction_ptr_fl;
            break;
            };
        }
        else
        {
            alu_instruction_ptr = (R700ALUInstruction*) CALLOC_STRUCT(R700ALUInstruction);
            Init_R700ALUInstruction(alu_instruction_ptr);
        }
        
        //src 0
        current_source_index = 0;
        pcurrent_source = &(pAsm->S[0].src);

        if (GL_FALSE == assemble_alu_src(alu_instruction_ptr,
                                         current_source_index,
                                         pcurrent_source, 
                                         scalar_channel_index) )     
        {            
            return GL_FALSE;
        }
   
        if (uNumSrc > 1) 
        {            
            // Process source 1            
            current_source_index = 1;
            pcurrent_source = &(pAsm->S[current_source_index].src);

            if (GL_FALSE == assemble_alu_src(alu_instruction_ptr,
                                             current_source_index,
                                             pcurrent_source, 
                                             scalar_channel_index) ) 
            {                
                return GL_FALSE;
            }
        }

        //other bits
        alu_instruction_ptr->m_Word0.f.index_mode = pAsm->D2.dst2.index_mode;

        if(   (is_single_scalar_operation == GL_TRUE) 
           || (GL_TRUE == bSplitInst) )
        {
            alu_instruction_ptr->m_Word0.f.last = 1;
        }
        else 
        {
            alu_instruction_ptr->m_Word0.f.last = (scalar_channel_index == 3) ?  1 : 0;
        }

        alu_instruction_ptr->m_Word0.f.pred_sel = (pAsm->D.dst.pred_inv > 0) ? 1 : 0;
        if(1 == pAsm->D.dst.predicated)
        {
            alu_instruction_ptr->m_Word1_OP2.f.update_pred         = 0x1;
            alu_instruction_ptr->m_Word1_OP2.f.update_execute_mask = 0x1;
        }
        else
        {
            alu_instruction_ptr->m_Word1_OP2.f.update_pred         = 0x0;
            alu_instruction_ptr->m_Word1_OP2.f.update_execute_mask = 0x0;
        }

        // dst
        if( (pAsm->D.dst.rtype == DST_REG_TEMPORARY) || 
            (pAsm->D.dst.rtype == DST_REG_OUT) ) 
        {
            alu_instruction_ptr->m_Word1.f.dst_gpr  = pAsm->D.dst.reg;
        }
        else 
        {            
            radeon_error("Only temp destination registers supported for ALU dest regs.\n");
            return GL_FALSE;
        }

        alu_instruction_ptr->m_Word1.f.dst_rel  = SQ_ABSOLUTE;  //D.rtype

        if ( is_single_scalar_operation == GL_TRUE ) 
        {
            // Override scalar_channel_index since only one scalar value will be written
            if(pAsm->D.dst.writex) 
            {
                scalar_channel_index = 0;
            }
            else if(pAsm->D.dst.writey) 
            {
                scalar_channel_index = 1;
            }
            else if(pAsm->D.dst.writez) 
            {
                scalar_channel_index = 2;
            }
            else if(pAsm->D.dst.writew) 
            {
                scalar_channel_index = 3;
            }
        }

        alu_instruction_ptr->m_Word1.f.dst_chan = scalar_channel_index;

        alu_instruction_ptr->m_Word1.f.clamp    = pAsm->D2.dst2.SaturateMode;

        if (pAsm->D.dst.op3) 
        {            
            //op3

            alu_instruction_ptr->m_Word1_OP3.f.alu_inst = pAsm->D.dst.opcode;

            //There's 3rd src for op3
            current_source_index = 2;
            pcurrent_source = &(pAsm->S[current_source_index].src);

            if ( GL_FALSE == assemble_alu_src(alu_instruction_ptr,
                                              current_source_index,
                                              pcurrent_source, 
                                              scalar_channel_index) ) 
            {
                return GL_FALSE;
            }
        }
        else 
        {
            //op2
            if (pAsm->bR6xx)
            {
                alu_instruction_ptr->m_Word1_OP2.f6.alu_inst           = pAsm->D.dst.opcode;

                alu_instruction_ptr->m_Word1_OP2.f6.src0_abs           = pAsm->S[0].src.abs;
                alu_instruction_ptr->m_Word1_OP2.f6.src1_abs           = pAsm->S[1].src.abs;

                //alu_instruction_ptr->m_Word1_OP2.f6.update_execute_mask = 0x0;
                //alu_instruction_ptr->m_Word1_OP2.f6.update_pred         = 0x0;
                switch (scalar_channel_index) 
                {
                    case 0: 
                        alu_instruction_ptr->m_Word1_OP2.f6.write_mask = pAsm->D.dst.writex; 
                        break;
                    case 1: 
                        alu_instruction_ptr->m_Word1_OP2.f6.write_mask = pAsm->D.dst.writey; 
                        break;
                    case 2: 
                        alu_instruction_ptr->m_Word1_OP2.f6.write_mask = pAsm->D.dst.writez; 
                        break;
                    case 3: 
                        alu_instruction_ptr->m_Word1_OP2.f6.write_mask = pAsm->D.dst.writew; 
                        break;
                    default: 
                        alu_instruction_ptr->m_Word1_OP2.f6.write_mask = 1; //SQ_SEL_MASK;
                        break;
                }            
                alu_instruction_ptr->m_Word1_OP2.f6.omod               = SQ_ALU_OMOD_OFF;
            }
            else
            {
                alu_instruction_ptr->m_Word1_OP2.f.alu_inst           = pAsm->D.dst.opcode;

                alu_instruction_ptr->m_Word1_OP2.f.src0_abs           = pAsm->S[0].src.abs;
                alu_instruction_ptr->m_Word1_OP2.f.src1_abs           = pAsm->S[1].src.abs;

                //alu_instruction_ptr->m_Word1_OP2.f.update_execute_mask = 0x0;
                //alu_instruction_ptr->m_Word1_OP2.f.update_pred         = 0x0;
                switch (scalar_channel_index) 
                {
                    case 0: 
                        alu_instruction_ptr->m_Word1_OP2.f.write_mask = pAsm->D.dst.writex; 
                        break;
                    case 1: 
                        alu_instruction_ptr->m_Word1_OP2.f.write_mask = pAsm->D.dst.writey; 
                        break;
                    case 2: 
                        alu_instruction_ptr->m_Word1_OP2.f.write_mask = pAsm->D.dst.writez; 
                        break;
                    case 3: 
                        alu_instruction_ptr->m_Word1_OP2.f.write_mask = pAsm->D.dst.writew; 
                        break;
                    default: 
                        alu_instruction_ptr->m_Word1_OP2.f.write_mask = 1; //SQ_SEL_MASK;
                        break;
                }            
                alu_instruction_ptr->m_Word1_OP2.f.omod               = SQ_ALU_OMOD_OFF;
            }
        }

        if(GL_FALSE == add_alu_instruction(pAsm, alu_instruction_ptr, contiguous_slots_needed) )
        {            
            return GL_FALSE;
        }

        /*
         * Judge the type of current instruction, is it vector or scalar 
         * instruction.
         */        
        if (is_single_scalar_operation) 
        {
            if(GL_FALSE == check_scalar(pAsm, alu_instruction_ptr) )
            {                
                return GL_FALSE;
            }
        }
        else 
        {
            if(GL_FALSE == check_vector(pAsm, alu_instruction_ptr) )
            {                
                return GL_FALSE; 
            }
        }

        contiguous_slots_needed -= 1;
    }

    return GL_TRUE;
}

GLboolean next_ins(r700_AssemblerBase *pAsm)
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    if( GL_TRUE == pAsm->is_tex )
    {
	    if (pILInst->TexSrcTarget == TEXTURE_RECT_INDEX) {
		    if( GL_FALSE == assemble_tex_instruction(pAsm, GL_FALSE) ) 
		    {
			    radeon_error("Error assembling TEX instruction\n");
			    return GL_FALSE;
		    }
	    } else {
		    if( GL_FALSE == assemble_tex_instruction(pAsm, GL_TRUE) ) 
		    {
			    radeon_error("Error assembling TEX instruction\n");
			    return GL_FALSE;
		    }
	    }
    }
    else 
    {   //ALU      
        if( GL_FALSE == assemble_alu_instruction(pAsm) ) 
        {
            radeon_error("Error assembling ALU instruction\n");
            return GL_FALSE;
        }
    } 
      
    if(pAsm->D.dst.rtype == DST_REG_OUT) 
    {
        if(pAsm->D.dst.op3) 
        {        
            // There is no mask for OP3 instructions, so all channels are written        
            pAsm->pucOutMask[pAsm->D.dst.reg - pAsm->starting_export_register_number] = 0xF;
        }
        else 
        {
            pAsm->pucOutMask[pAsm->D.dst.reg - pAsm->starting_export_register_number] 
               |= (unsigned char)pAsm->pILInst[pAsm->uiCurInst].DstReg.WriteMask;
        }
    }
    
    //reset for next inst.
    pAsm->D.bits    = 0;
    pAsm->D2.bits   = 0;
    pAsm->S[0].bits = 0;
    pAsm->S[1].bits = 0;
    pAsm->S[2].bits = 0;
    pAsm->is_tex = GL_FALSE;
    pAsm->need_tex_barrier = GL_FALSE;
    pAsm->D2.bits = 0;
    pAsm->C[0].bits = pAsm->C[1].bits = pAsm->C[2].bits = pAsm->C[3].bits = 0;
    return GL_TRUE;
}

GLboolean assemble_math_function(r700_AssemblerBase* pAsm, BITS opcode)
{
    BITS tmp;

    checkop1(pAsm);

    tmp = gethelpr(pAsm);

    // opcode  tmp.x,    a.x
    // MOV     dst,      tmp.x

    pAsm->D.dst.opcode = opcode;
    pAsm->D.dst.math = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp;
    pAsm->D.dst.writex = 1;

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    // Now replicate result to all necessary channels in destination
    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_ABS(r700_AssemblerBase *pAsm)
{
    checkop1(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_MAX;  

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }
    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }
 
    pAsm->S[1].bits = pAsm->S[0].bits;
    flipneg_PVSSRC(&(pAsm->S[1].src));

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_ADD(r700_AssemblerBase *pAsm)
{
    if( GL_FALSE == checkop2(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_ADD;
 
    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }
 
    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }

    if(pAsm->pILInst[pAsm->uiCurInst].Opcode == OPCODE_SUB)
    {
        flipneg_PVSSRC(&(pAsm->S[1].src));
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_ARL(r700_AssemblerBase *pAsm)
{ /* TODO: ar values dont' persist between clauses */
    if( GL_FALSE == checkop1(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_MOVA_FLOOR;
    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg = 0;
    pAsm->D.dst.writex = 0;
    pAsm->D.dst.writey = 0;
    pAsm->D.dst.writez = 0;
    pAsm->D.dst.writew = 0;

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_BAD(char *opcode_str) 
{
    radeon_error("Not yet implemented instruction (%s)\n", opcode_str);
    return GL_FALSE;
}

GLboolean assemble_CMP(r700_AssemblerBase *pAsm)
{
    int tmp;

    if( GL_FALSE == checkop3(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP3_INST_CNDGE;
    pAsm->D.dst.op3     = 1;  

    tmp = (-1);

    if(0xF != pAsm->pILInst[pAsm->uiCurInst].DstReg.WriteMask)
    {
        //OP3 has no support for write mask
        tmp = gethelpr(pAsm);

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp;

        nomask_PVSDST(&(pAsm->D.dst));
    }
    else 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }
              
    if( GL_FALSE == assemble_src(pAsm, 2, 1) )  
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, 2) ) 
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    if (0xF != pAsm->pILInst[pAsm->uiCurInst].DstReg.WriteMask) 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

        //tmp for source
        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp;

        noneg_PVSSRC(&(pAsm->S[0].src));
        noswizzle_PVSSRC(&(pAsm->S[0].src));

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

GLboolean assemble_TRIG(r700_AssemblerBase *pAsm, BITS opcode)
{
    int tmp;
    checkop1(pAsm);

    tmp = gethelpr(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_MUL;
    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp;
    pAsm->D.dst.writex = 1;

    assemble_src(pAsm, 0, -1);

    pAsm->S[1].src.rtype = SRC_REC_LITERAL;
    setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_X);
    pAsm->D2.dst2.literal_slots = 1;
    pAsm->C[0].f = 1/(3.1415926535 * 2);
    pAsm->C[1].f = 0.0F;
    next_ins(pAsm);

    pAsm->D.dst.opcode = opcode;
    pAsm->D.dst.math = 1;

    assemble_dst(pAsm);

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    next_ins(pAsm);

    //TODO - replicate if more channels set in WriteMask
    return GL_TRUE;

}
 
GLboolean assemble_DOT(r700_AssemblerBase *pAsm)
{
    if( GL_FALSE == checkop2(pAsm) )
    {
        return GL_FALSE;
    }
 
    pAsm->D.dst.opcode = SQ_OP2_INST_DOT4;  

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }

    if(OPCODE_DP3 == pAsm->pILInst[pAsm->uiCurInst].Opcode)
    {
        zerocomp_PVSSRC(&(pAsm->S[0].src), 3);
        zerocomp_PVSSRC(&(pAsm->S[1].src), 3);
    }
    else if(pAsm->pILInst[pAsm->uiCurInst].Opcode == OPCODE_DPH) 
    {
        onecomp_PVSSRC(&(pAsm->S[0].src), 3);
    } 

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_DST(r700_AssemblerBase *pAsm)
{
    if( GL_FALSE == checkop2(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_MUL;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }

    onecomp_PVSSRC(&(pAsm->S[0].src), 0);
    onecomp_PVSSRC(&(pAsm->S[0].src), 3);

    onecomp_PVSSRC(&(pAsm->S[1].src), 0);
    onecomp_PVSSRC(&(pAsm->S[1].src), 2);

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_EX2(r700_AssemblerBase *pAsm)
{
    return assemble_math_function(pAsm, SQ_OP2_INST_EXP_IEEE);
}

GLboolean assemble_EXP(r700_AssemblerBase *pAsm)
{
    BITS tmp;

    checkop1(pAsm);

    tmp = gethelpr(pAsm);

    // FLOOR   tmp.x,    a.x
    // EX2     dst.x     tmp.x

    if (pAsm->pILInst->DstReg.WriteMask & 0x1) {
        pAsm->D.dst.opcode = SQ_OP2_INST_FLOOR;

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
        pAsm->D.dst.reg    = tmp;
        pAsm->D.dst.writex = 1;

        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.opcode = SQ_OP2_INST_EXP_IEEE;
        pAsm->D.dst.math = 1;

        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.writey = pAsm->D.dst.writez = pAsm->D.dst.writew = 0;

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp;

        setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
        noneg_PVSSRC(&(pAsm->S[0].src));

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    // FRACT   dst.y     a.x

    if ((pAsm->pILInst->DstReg.WriteMask >> 1) & 0x1) {
        pAsm->D.dst.opcode = SQ_OP2_INST_FRACT;

        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.writex = pAsm->D.dst.writez = pAsm->D.dst.writew = 0;

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    // EX2     dst.z,    a.x

    if ((pAsm->pILInst->DstReg.WriteMask >> 2) & 0x1) {
        pAsm->D.dst.opcode = SQ_OP2_INST_EXP_IEEE;
        pAsm->D.dst.math = 1;

        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.writex = pAsm->D.dst.writey = pAsm->D.dst.writew = 0;

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    // MOV     dst.w     1.0

    if ((pAsm->pILInst->DstReg.WriteMask >> 3) & 0x1) {
        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.writex = pAsm->D.dst.writey = pAsm->D.dst.writez = 0;

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp;

        setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_1);
        noneg_PVSSRC(&(pAsm->S[0].src));

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}
 
GLboolean assemble_FLR(r700_AssemblerBase *pAsm)
{
    checkop1(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_FLOOR;  

    if ( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_FLR_INT(r700_AssemblerBase *pAsm)
{
    return assemble_math_function(pAsm, SQ_OP2_INST_FLT_TO_INT);
}

GLboolean assemble_FRC(r700_AssemblerBase *pAsm)
{
    checkop1(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_FRACT; 

    if ( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_KIL(r700_AssemblerBase *pAsm, GLuint opcode)
{  
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    if(pILInst->Opcode == OPCODE_KIL)
        checkop1(pAsm);

    pAsm->D.dst.opcode = opcode;  
    //pAsm->D.dst.math = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = 0;
    pAsm->D.dst.writex = 0;
    pAsm->D.dst.writey = 0;
    pAsm->D.dst.writez = 0;
    pAsm->D.dst.writew = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = 0;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_0);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if(pILInst->Opcode == OPCODE_KIL_NV)
    {
        setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
        pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[1].src.reg = 0;
        setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_1);
        neg_PVSSRC(&(pAsm->S[1].src));
    }
    else
    {
        if( GL_FALSE == assemble_src(pAsm, 0, 1) )
        {
            return GL_FALSE;
        }

    }

    if ( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    /* Doc says KILL has to be last(end) ALU clause */
    pAsm->pR700Shader->killIsUsed = GL_TRUE;
    pAsm->alu_x_opcode = SQ_CF_INST_ALU;
    
    return GL_TRUE;
}

GLboolean assemble_LG2(r700_AssemblerBase *pAsm) 
{ 
    return assemble_math_function(pAsm, SQ_OP2_INST_LOG_IEEE);
}

GLboolean assemble_LRP(r700_AssemblerBase *pAsm) 
{
    BITS tmp;

    if( GL_FALSE == checkop3(pAsm) )
    {
        return GL_FALSE;
    }

    tmp = gethelpr(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_ADD;

    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = tmp;
    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    nomask_PVSDST(&(pAsm->D.dst));

          
    if( GL_FALSE == assemble_src(pAsm, 1, 0) ) 
    {
	    return GL_FALSE;
    }

    if ( GL_FALSE == assemble_src(pAsm, 2, 1) )   
    {
	    return GL_FALSE;
    }

    neg_PVSSRC(&(pAsm->S[1].src));

    if( GL_FALSE == next_ins(pAsm) ) 
    {
	    return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP3_INST_MULADD;
    pAsm->D.dst.op3    = 1;

    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg = tmp;
    nomask_PVSDST(&(pAsm->D.dst));
    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = tmp;
    noswizzle_PVSSRC(&(pAsm->S[0].src));


    if( GL_FALSE == assemble_src(pAsm, 0, 1) ) 
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 2, -1) ) 
    {
        return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = tmp;
    noswizzle_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_LOG(r700_AssemblerBase *pAsm)
{
    BITS tmp1, tmp2, tmp3;

    checkop1(pAsm);

    tmp1 = gethelpr(pAsm);
    tmp2 = gethelpr(pAsm);
    tmp3 = gethelpr(pAsm);

    // FIXME: The hardware can do fabs() directly on input
    //        elements, but the compiler doesn't have the
    //        capability to use that.

    // MAX     tmp1.x,   a.x,    -a.x   (fabs(a.x))

    pAsm->D.dst.opcode = SQ_OP2_INST_MAX;  

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp1;
    pAsm->D.dst.writex = 1;

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }
 
    pAsm->S[1].bits = pAsm->S[0].bits;
    flipneg_PVSSRC(&(pAsm->S[1].src));

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    // Entire algo:
    //
    // LG2     tmp2.x,   tmp1.x
    // FLOOR   tmp3.x,   tmp2.x
    // MOV     dst.x,    tmp3.x
    // ADD     tmp3.x,   tmp2.x,    -tmp3.x
    // EX2     dst.y,    tmp3.x
    // MOV     dst.z,    tmp2.x
    // MOV     dst.w,    1.0

    // LG2     tmp2.x,   tmp1.x
    // FLOOR   tmp3.x,   tmp2.x

    pAsm->D.dst.opcode = SQ_OP2_INST_LOG_IEEE;
    pAsm->D.dst.math = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp2;
    pAsm->D.dst.writex = 1;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp1;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_FLOOR;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp3;
    pAsm->D.dst.writex = 1;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp2;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    // MOV     dst.x,    tmp3.x

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.writey = pAsm->D.dst.writez = pAsm->D.dst.writew = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp3;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    // ADD     tmp3.x,   tmp2.x,    -tmp3.x
    // EX2     dst.y,    tmp3.x

    pAsm->D.dst.opcode = SQ_OP2_INST_ADD;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp3;
    pAsm->D.dst.writex = 1;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp2;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    pAsm->S[1].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[1].src.reg   = tmp3;

    setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_X);
    neg_PVSSRC(&(pAsm->S[1].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_EXP_IEEE;
    pAsm->D.dst.math = 1;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.writex = pAsm->D.dst.writez = pAsm->D.dst.writew = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp3;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    // MOV     dst.z,    tmp2.x

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.writex = pAsm->D.dst.writey = pAsm->D.dst.writew = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp2;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    // MOV     dst.w     1.0

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->D.dst.writex = pAsm->D.dst.writey = pAsm->D.dst.writez = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp1;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_1);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_MAD(struct r700_AssemblerBase *pAsm) 
{
    int tmp, ii;
    GLboolean bReplaceDst = GL_FALSE;
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

	if( GL_FALSE == checkop3(pAsm) )
    {
        return GL_FALSE;
    }

	pAsm->D.dst.opcode = SQ_OP3_INST_MULADD;  
	pAsm->D.dst.op3     = 1; 

	tmp = (-1);

    if(PROGRAM_TEMPORARY == pILInst->DstReg.File)
    {   /* TODO : more investigation on MAD src and dst using same register */
        for(ii=0; ii<3; ii++)
        {
            if(   (PROGRAM_TEMPORARY == pILInst->SrcReg[ii].File)
               && (pILInst->DstReg.Index == pILInst->SrcReg[ii].Index) )
            {
                bReplaceDst = GL_TRUE;
                break;
            }
        }
    }
    if(0xF != pILInst->DstReg.WriteMask)
    {   /* OP3 has no support for write mask */
        bReplaceDst = GL_TRUE;
    }

	if(GL_TRUE == bReplaceDst)
    {
        tmp = gethelpr(pAsm);

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp;

        nomask_PVSDST(&(pAsm->D.dst));
    }
    else 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }
    }

	if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }
              
    if( GL_FALSE == assemble_src(pAsm, 1, -1) )  
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 2, -1) ) 
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

	if (GL_TRUE == bReplaceDst) 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

        //tmp for source
        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp;

        noneg_PVSSRC(&(pAsm->S[0].src));
        noswizzle_PVSSRC(&(pAsm->S[0].src));

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

/* LIT dst, src */
GLboolean assemble_LIT(r700_AssemblerBase *pAsm)
{
    unsigned int dstReg;
    unsigned int dstType;
    unsigned int srcReg;
    unsigned int srcType;
    checkop1(pAsm);
    int tmp = gethelpr(pAsm);

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }
    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }
    dstReg  = pAsm->D.dst.reg;
    dstType = pAsm->D.dst.rtype;
    srcReg  = pAsm->S[0].src.reg;
    srcType = pAsm->S[0].src.rtype;

    /* dst.xw, <- 1.0  */
    pAsm->D.dst.opcode   = SQ_OP2_INST_MOV;
    pAsm->D.dst.rtype    = dstType;
    pAsm->D.dst.reg      = dstReg;
    pAsm->D.dst.writex   = 1;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 0;
    pAsm->D.dst.writew   = 1;
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[0].src));
    pAsm->S[0].src.swizzlex = SQ_SEL_1;
    pAsm->S[0].src.swizzley = SQ_SEL_1;
    pAsm->S[0].src.swizzlez = SQ_SEL_1;
    pAsm->S[0].src.swizzlew = SQ_SEL_1;
    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    /* dst.y = max(src.x, 0.0) */
    pAsm->D.dst.opcode   = SQ_OP2_INST_MAX;
    pAsm->D.dst.rtype    = dstType;
    pAsm->D.dst.reg      = dstReg;
    pAsm->D.dst.writex   = 0;
    pAsm->D.dst.writey   = 1;
    pAsm->D.dst.writez   = 0;
    pAsm->D.dst.writew   = 0;
    pAsm->S[0].src.rtype = srcType;
    pAsm->S[0].src.reg   = srcReg;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X, SQ_SEL_X, SQ_SEL_X, SQ_SEL_X);
    pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[1].src.reg   = tmp;
    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[1].src));
    pAsm->S[1].src.swizzlex = SQ_SEL_0;
    pAsm->S[1].src.swizzley = SQ_SEL_0;
    pAsm->S[1].src.swizzlez = SQ_SEL_0;
    pAsm->S[1].src.swizzlew = SQ_SEL_0;
    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_Y, SQ_SEL_Y, SQ_SEL_Y, SQ_SEL_Y);

    /* dst.z = log(src.y) */
    pAsm->D.dst.opcode   = SQ_OP2_INST_LOG_CLAMPED;
    pAsm->D.dst.math     = 1;
    pAsm->D.dst.rtype    = dstType;
    pAsm->D.dst.reg      = dstReg;
    pAsm->D.dst.writex   = 0;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 1;
    pAsm->D.dst.writew   = 0;
    pAsm->S[0].src.rtype = srcType;
    pAsm->S[0].src.reg   = srcReg;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, 2) )
    {
        return GL_FALSE;
    }

    swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_W, SQ_SEL_W, SQ_SEL_W, SQ_SEL_W);

    swizzleagain_PVSSRC(&(pAsm->S[2].src), SQ_SEL_X, SQ_SEL_X, SQ_SEL_X, SQ_SEL_X);

    /* tmp.x = amd MUL_LIT(src.w, dst.z, src.x ) */
    pAsm->D.dst.opcode   = SQ_OP3_INST_MUL_LIT;
    pAsm->D.dst.math     = 1;
    pAsm->D.dst.op3      = 1;
    pAsm->D.dst.rtype    = DST_REG_TEMPORARY;
    pAsm->D.dst.reg      = tmp;
    pAsm->D.dst.writex   = 1;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 0;
    pAsm->D.dst.writew   = 0;

    pAsm->S[0].src.rtype = srcType;
    pAsm->S[0].src.reg   = srcReg;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);

    pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[1].src.reg   = dstReg;
    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[1].src));
    pAsm->S[1].src.swizzlex = SQ_SEL_Z;
    pAsm->S[1].src.swizzley = SQ_SEL_Z;
    pAsm->S[1].src.swizzlez = SQ_SEL_Z;
    pAsm->S[1].src.swizzlew = SQ_SEL_Z;

    pAsm->S[2].src.rtype = srcType;
    pAsm->S[2].src.reg   = srcReg;
    setaddrmode_PVSSRC(&(pAsm->S[2].src), ADDR_ABSOLUTE);

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    /* dst.z = exp(tmp.x) */
    pAsm->D.dst.opcode   = SQ_OP2_INST_EXP_IEEE;
    pAsm->D.dst.math     = 1;
    pAsm->D.dst.rtype    = dstType;
    pAsm->D.dst.reg      = dstReg;
    pAsm->D.dst.writex   = 0;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 1;
    pAsm->D.dst.writew   = 0;

    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[0].src));
    pAsm->S[0].src.swizzlex = SQ_SEL_X;
    pAsm->S[0].src.swizzley = SQ_SEL_X;
    pAsm->S[0].src.swizzlez = SQ_SEL_X;
    pAsm->S[0].src.swizzlew = SQ_SEL_X;

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_MAX(r700_AssemblerBase *pAsm) 
{
	if( GL_FALSE == checkop2(pAsm) )
	{
		return GL_FALSE;
	}

	pAsm->D.dst.opcode = SQ_OP2_INST_MAX; 
	
	if( GL_FALSE == assemble_dst(pAsm) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 0, -1) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 1, -1) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == next_ins(pAsm) )
	{
		return GL_FALSE;
	}

    return GL_TRUE;
}
 
GLboolean assemble_MIN(r700_AssemblerBase *pAsm) 
{
	if( GL_FALSE == checkop2(pAsm) )
	{
		return GL_FALSE;
	}

	pAsm->D.dst.opcode = SQ_OP2_INST_MIN;  

	if( GL_FALSE == assemble_dst(pAsm) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 0, -1) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 1, -1) )
	{
		return GL_FALSE;
	}
 
	if( GL_FALSE == next_ins(pAsm) )
	{
		return GL_FALSE;
	}

    return GL_TRUE;
}
 
GLboolean assemble_MOV(r700_AssemblerBase *pAsm) 
{
    checkop1(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if (GL_FALSE == assemble_dst(pAsm))
    {
        return GL_FALSE;
    }

    if (GL_FALSE == assemble_src(pAsm, 0, -1))
    {
        return GL_FALSE;
    }

    if ( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_MUL(r700_AssemblerBase *pAsm) 
{
	if( GL_FALSE == checkop2(pAsm) )
	{
		return GL_FALSE;
	}

	pAsm->D.dst.opcode = SQ_OP2_INST_MUL;

	if( GL_FALSE == assemble_dst(pAsm) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 0, -1) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == assemble_src(pAsm, 1, -1) )
	{
		return GL_FALSE;
	}

	if( GL_FALSE == next_ins(pAsm) ) 
	{
		return GL_FALSE;
	}

    return GL_TRUE;
}
 
GLboolean assemble_POW(r700_AssemblerBase *pAsm) 
{
    BITS tmp;

    checkop1(pAsm);

    tmp = gethelpr(pAsm);

    // LG2 tmp.x,     a.swizzle
    pAsm->D.dst.opcode = SQ_OP2_INST_LOG_IEEE;  
    pAsm->D.dst.math = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = tmp;
    nomask_PVSDST(&(pAsm->D.dst));

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    // MUL tmp.x,     tmp.x, b.swizzle
    pAsm->D.dst.opcode = SQ_OP2_INST_MUL;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg = tmp;
    nomask_PVSDST(&(pAsm->D.dst));

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = tmp;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    // EX2 dst.mask,          tmp.x
    // EX2 tmp.x,             tmp.x
    pAsm->D.dst.opcode = SQ_OP2_INST_EXP_IEEE;
    pAsm->D.dst.math = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg = tmp;
    nomask_PVSDST(&(pAsm->D.dst));

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = tmp;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    // Now replicate result to all necessary channels in destination
    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;

    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_RCP(r700_AssemblerBase *pAsm) 
{
    return assemble_math_function(pAsm, SQ_OP2_INST_RECIP_IEEE);
}
 
GLboolean assemble_RSQ(r700_AssemblerBase *pAsm) 
{
    return assemble_math_function(pAsm, SQ_OP2_INST_RECIPSQRT_IEEE);
}
 
GLboolean assemble_SCS(r700_AssemblerBase *pAsm) 
{
    BITS tmp;

    checkop1(pAsm);

    tmp = gethelpr(pAsm);
    /* tmp.x = src /2*PI */
    pAsm->D.dst.opcode = SQ_OP2_INST_MUL;
    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype  = DST_REG_TEMPORARY;
    pAsm->D.dst.reg    = tmp;
    pAsm->D.dst.writex = 1;

    assemble_src(pAsm, 0, -1);

    pAsm->S[1].src.rtype = SRC_REC_LITERAL;
    setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_X);
    pAsm->D2.dst2.literal_slots = 1;
    pAsm->C[0].f = 1/(3.1415926535 * 2);
    pAsm->C[1].f = 0.0F;

    next_ins(pAsm);

    // COS dst.x,    a.x
    pAsm->D.dst.opcode = SQ_OP2_INST_COS;
    pAsm->D.dst.math = 1;

    assemble_dst(pAsm);
    /* mask y */
    pAsm->D.dst.writey = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if ( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    // SIN dst.y,    a.x
    pAsm->D.dst.opcode = SQ_OP2_INST_SIN;
    pAsm->D.dst.math = 1;

    assemble_dst(pAsm);
    /* mask x */
    pAsm->D.dst.writex = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg   = tmp;
    setswizzle_PVSSRC(&(pAsm->S[0].src), SQ_SEL_X);
    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_LOGIC(r700_AssemblerBase *pAsm, BITS opcode) 
{
    if( GL_FALSE == checkop2(pAsm) )
    {
	    return GL_FALSE;
    }

    pAsm->D.dst.opcode = opcode;
    //pAsm->D.dst.math   = 1;

    if( GL_FALSE == assemble_dst(pAsm) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
	    return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean assemble_LOGIC_PRED(r700_AssemblerBase *pAsm, BITS opcode) 
{
    struct prog_instruction *pILInst = &(pAsm->pILInst[pAsm->uiCurInst]);

    pAsm->D.dst.opcode = opcode;
    pAsm->D.dst.math   = 1;
    pAsm->D.dst.predicated = 1;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg = pAsm->uHelpReg;
    pAsm->D.dst.writex = 1;
    pAsm->D.dst.writey = pAsm->D.dst.writez = pAsm->D.dst.writew = 0;

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[0].src.reg = pAsm->last_cond_register + pAsm->starting_temp_register_number;
    pAsm->S[0].src.swizzlex = pILInst->DstReg.CondSwizzle & 0x7;
    noneg_PVSSRC(&(pAsm->S[0].src));

    pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[1].src.reg   = pAsm->uHelpReg;
    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[1].src));
    pAsm->S[1].src.swizzlex = SQ_SEL_0;
    pAsm->S[1].src.swizzley = SQ_SEL_0;
    pAsm->S[1].src.swizzlez = SQ_SEL_0;
    pAsm->S[1].src.swizzlew = SQ_SEL_0;

    if( GL_FALSE == next_ins(pAsm) ) 
    {
	    return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_SGE(r700_AssemblerBase *pAsm) 
{
    if( GL_FALSE == checkop2(pAsm) )
    {
	    return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_SETGE;  

    if( GL_FALSE == assemble_dst(pAsm) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
	    return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) ) 
    {
	    return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_SLT(r700_AssemblerBase *pAsm) 
{
    if( GL_FALSE == checkop2(pAsm) )
    {
	    return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP2_INST_SETGT;  

    if( GL_FALSE == assemble_dst(pAsm) )
    {
        return GL_FALSE;
    }
                
    if( GL_FALSE == assemble_src(pAsm, 0, 1) )  
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, 0) )  
    {
        return GL_FALSE;
    }

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    return GL_TRUE;
}
 
GLboolean assemble_STP(r700_AssemblerBase *pAsm) 
{
    return GL_TRUE;
}
 
GLboolean assemble_TEX(r700_AssemblerBase *pAsm) 
{
    GLboolean src_const;
    GLboolean need_barrier = GL_FALSE; 

    checkop1(pAsm);
    
    switch (pAsm->pILInst[pAsm->uiCurInst].SrcReg[0].File)
    {
    case PROGRAM_UNIFORM: 
    case PROGRAM_CONSTANT:
    case PROGRAM_LOCAL_PARAM:
    case PROGRAM_ENV_PARAM:
    case PROGRAM_STATE_VAR:
        src_const = GL_TRUE;
        break;
    case PROGRAM_TEMPORARY:
    case PROGRAM_INPUT:
    default:
        src_const = GL_FALSE;
	break;
    }

    if (GL_TRUE == src_const)
    {
	    if ( GL_FALSE == mov_temp(pAsm, 0) )
		    return GL_FALSE;
	    need_barrier = GL_TRUE;
    }

    if (pAsm->pILInst[pAsm->uiCurInst].Opcode == OPCODE_TXP)
    {
        GLuint tmp = gethelpr(pAsm);
        pAsm->D.dst.opcode = SQ_OP2_INST_RECIP_IEEE;
        pAsm->D.dst.math = 1;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp;
        pAsm->D.dst.writew = 1;

        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }
        swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_W, SQ_SEL_W, SQ_SEL_W, SQ_SEL_W);
        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.opcode = SQ_OP2_INST_MUL;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp;
        pAsm->D.dst.writex = 1;
        pAsm->D.dst.writey = 1;
        pAsm->D.dst.writez = 1;
        pAsm->D.dst.writew = 0;

        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }
        setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
        pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[1].src.reg   = tmp;
        setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_W);

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
        
        pAsm->aArgSubst[1] = tmp;
        need_barrier = GL_TRUE;
    }

    if (pAsm->pILInst[pAsm->uiCurInst].TexSrcTarget == TEXTURE_CUBE_INDEX )
    {
        GLuint tmp1 = gethelpr(pAsm);
        GLuint tmp2 = gethelpr(pAsm);
        
        /* tmp1.xyzw = CUBE(R0.zzxy, R0.yxzz) */
        pAsm->D.dst.opcode = SQ_OP2_INST_CUBE;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp1;
        nomask_PVSDST(&(pAsm->D.dst));
	
        if( GL_FALSE == assemble_src(pAsm, 0, -1) )
        {
            return GL_FALSE;
        }

        if( GL_FALSE == assemble_src(pAsm, 0, 1) )
        {
            return GL_FALSE;
        }

        swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_Z, SQ_SEL_Z, SQ_SEL_X, SQ_SEL_Y);
        swizzleagain_PVSSRC(&(pAsm->S[1].src), SQ_SEL_Y, SQ_SEL_X, SQ_SEL_Z, SQ_SEL_Z); 

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
 
        /* tmp1.z = RCP_e(|tmp1.z|) */
        pAsm->D.dst.opcode = SQ_OP2_INST_RECIP_IEEE;
        pAsm->D.dst.math = 1;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp1;
        pAsm->D.dst.writez = 1;

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg = tmp1;
        pAsm->S[0].src.swizzlex = SQ_SEL_Z;
        pAsm->S[0].src.abs = 1;

        next_ins(pAsm);

        /* MULADD R0.x,  R0.x,  PS1,  (0x3FC00000, 1.5f).x
         * MULADD R0.y,  R0.y,  PS1,  (0x3FC00000, 1.5f).x
         * muladd has no writemask, have to use another temp 
         */
        pAsm->D.dst.opcode = SQ_OP3_INST_MULADD;
        pAsm->D.dst.op3    = 1;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp2;

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp1;
        noswizzle_PVSSRC(&(pAsm->S[0].src));
        setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
        pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[1].src.reg   = tmp1;
        setswizzle_PVSSRC(&(pAsm->S[1].src), SQ_SEL_Z);
        setaddrmode_PVSSRC(&(pAsm->S[2].src), ADDR_ABSOLUTE);
        /* immediate c 1.5 */
        pAsm->D2.dst2.literal_slots = 1;
        pAsm->C[0].f = 1.5F;
        pAsm->S[2].src.rtype = SRC_REC_LITERAL;
        pAsm->S[2].src.reg   = tmp1;
        setswizzle_PVSSRC(&(pAsm->S[2].src), SQ_SEL_X);

        next_ins(pAsm);

        /* tmp1.xy = temp2.xy */
        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp1;
        pAsm->D.dst.writex = 1;
        pAsm->D.dst.writey = 1;
        pAsm->D.dst.writez = 0;
        pAsm->D.dst.writew = 0;

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp2;
        noswizzle_PVSSRC(&(pAsm->S[0].src));

        next_ins(pAsm);
        pAsm->aArgSubst[1] = tmp1;
        need_barrier = GL_TRUE;

    }

    switch(pAsm->pILInst[pAsm->uiCurInst].Opcode)
    {
        case OPCODE_DDX:
            /* will these need WQM(1) on CF inst ? */
            pAsm->D.dst.opcode = SQ_TEX_INST_GET_GRADIENTS_H;
            break;
        case OPCODE_DDY:
            pAsm->D.dst.opcode = SQ_TEX_INST_GET_GRADIENTS_V;
            break;
        case OPCODE_TXB:
            pAsm->D.dst.opcode = SQ_TEX_INST_SAMPLE_L;
            break;
        default:
            if(pAsm->pILInst[pAsm->uiCurInst].TexShadow == 1)
                pAsm->D.dst.opcode = SQ_TEX_INST_SAMPLE_C;
            else
                pAsm->D.dst.opcode = SQ_TEX_INST_SAMPLE;
    }

    pAsm->is_tex = GL_TRUE;
    if ( GL_TRUE == need_barrier )

    pAsm->is_tex = GL_TRUE;
    if ( GL_TRUE == need_barrier )
    {
        pAsm->need_tex_barrier = GL_TRUE;
    }
    // Set src1 to tex unit id
    pAsm->S[1].src.reg   = pAsm->SamplerUnits[pAsm->pILInst[pAsm->uiCurInst].TexSrcUnit];
    pAsm->S[1].src.rtype = SRC_REG_TEMPORARY;

    //No sw info from mesa compiler, so hard code here.
    pAsm->S[1].src.swizzlex = SQ_SEL_X;
    pAsm->S[1].src.swizzley = SQ_SEL_Y;
    pAsm->S[1].src.swizzlez = SQ_SEL_Z;
    pAsm->S[1].src.swizzlew = SQ_SEL_W;

    if( GL_FALSE == tex_dst(pAsm) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == tex_src(pAsm) )
    {
        return GL_FALSE;
    }

    if(pAsm->pILInst[pAsm->uiCurInst].Opcode == OPCODE_TXP)
    {
        /* hopefully did swizzles before */
        noswizzle_PVSSRC(&(pAsm->S[0].src));
    }
   
    if(pAsm->pILInst[pAsm->uiCurInst].TexSrcTarget == TEXTURE_CUBE_INDEX)
    {
        /* SAMPLE dst, tmp.yxwy, CUBE */
        pAsm->S[0].src.swizzlex = SQ_SEL_Y;
        pAsm->S[0].src.swizzley = SQ_SEL_X;
        pAsm->S[0].src.swizzlez = SQ_SEL_W;
        pAsm->S[0].src.swizzlew = SQ_SEL_Y;
    }
 
    if(pAsm->pILInst[pAsm->uiCurInst].TexShadow == 1)
    {
        /* compare value goes to w chan ? */
        pAsm->S[0].src.swizzlew = SQ_SEL_Z;
    }

    if ( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }

    /* add ARB shadow ambient but clamp to 0..1 */
    if(pAsm->pILInst[pAsm->uiCurInst].TexShadow == 1)
    {
	/* ADD_SAT dst,  dst,  ambient[texunit] */
	pAsm->D.dst.opcode = SQ_OP2_INST_ADD;

	if( GL_FALSE == assemble_dst(pAsm) )
	{
	    return GL_FALSE;
	}
	pAsm->D2.dst2.SaturateMode = 1;

	pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
	pAsm->S[0].src.reg = pAsm->D.dst.reg;
	noswizzle_PVSSRC(&(pAsm->S[0].src));
	noneg_PVSSRC(&(pAsm->S[0].src));

	pAsm->S[1].src.rtype = SRC_REG_CONSTANT;
	pAsm->S[1].src.reg = pAsm->shadow_regs[pAsm->pILInst[pAsm->uiCurInst].TexSrcUnit];
	noswizzle_PVSSRC(&(pAsm->S[1].src));
	noneg_PVSSRC(&(pAsm->S[1].src));

	if( GL_FALSE == next_ins(pAsm) )
	{
	    return GL_FALSE;
	}

    }

    return GL_TRUE;
}

GLboolean assemble_XPD(r700_AssemblerBase *pAsm) 
{
    BITS tmp1;
    BITS tmp2 = 0;

    if( GL_FALSE == checkop2(pAsm) )
    {
	    return GL_FALSE;
    }

    tmp1 = gethelpr(pAsm);

    pAsm->D.dst.opcode = SQ_OP2_INST_MUL;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = tmp1;
    nomask_PVSDST(&(pAsm->D.dst));
  
    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }
 
    swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_Z, SQ_SEL_X, SQ_SEL_Y, SQ_SEL_0);
    swizzleagain_PVSSRC(&(pAsm->S[1].src), SQ_SEL_Y, SQ_SEL_Z, SQ_SEL_X, SQ_SEL_0);

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }

    pAsm->D.dst.opcode = SQ_OP3_INST_MULADD;
    pAsm->D.dst.op3    = 1;

    if(0xF != pAsm->pILInst[pAsm->uiCurInst].DstReg.WriteMask)
    {
        tmp2 = gethelpr(pAsm);

        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = tmp2;

        nomask_PVSDST(&(pAsm->D.dst));
    }
    else 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }
    }

    if( GL_FALSE == assemble_src(pAsm, 0, -1) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == assemble_src(pAsm, 1, -1) )
    {
        return GL_FALSE;
    }
 
    swizzleagain_PVSSRC(&(pAsm->S[0].src), SQ_SEL_Y, SQ_SEL_Z, SQ_SEL_X, SQ_SEL_0);
    swizzleagain_PVSSRC(&(pAsm->S[1].src), SQ_SEL_Z, SQ_SEL_X, SQ_SEL_Y, SQ_SEL_0);

    // result1 + (neg) result0
    setaddrmode_PVSSRC(&(pAsm->S[2].src),ADDR_ABSOLUTE);
    pAsm->S[2].src.rtype = SRC_REG_TEMPORARY;
    pAsm->S[2].src.reg   = tmp1;

    neg_PVSSRC(&(pAsm->S[2].src));
    noswizzle_PVSSRC(&(pAsm->S[2].src));

    if( GL_FALSE == next_ins(pAsm) ) 
    {
        return GL_FALSE;
    }


    if(0xF != pAsm->pILInst[pAsm->uiCurInst].DstReg.WriteMask) 
    {
        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

        // Use tmp as source
        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = tmp2;

        noneg_PVSSRC(&(pAsm->S[0].src));
        noswizzle_PVSSRC(&(pAsm->S[0].src));

        if( GL_FALSE == next_ins(pAsm) )
        {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

GLboolean assemble_EXPORT(r700_AssemblerBase *pAsm)
{
    return GL_TRUE;
}

static inline void decreaseCurrent(r700_AssemblerBase *pAsm, GLuint uReason)
{
    switch (uReason)
    {
    case FC_PUSH_VPM:
        pAsm->CALLSTACK[pAsm->CALLSP].current--;
        break;
    case FC_PUSH_WQM:
        pAsm->CALLSTACK[pAsm->CALLSP].current -= 4;
        break;
    case FC_LOOP:
        pAsm->CALLSTACK[pAsm->CALLSP].current -= 4;
        break;
    case FC_REP:
        /* TODO : for 16 vp asic, should -= 2; */
        pAsm->CALLSTACK[pAsm->CALLSP].current -= 1;
        break;
    };
}

static inline void checkStackDepth(r700_AssemblerBase *pAsm, GLuint uReason, GLboolean bCheckMaxOnly)
{
    if(GL_TRUE == bCheckMaxOnly)
    {
        switch (uReason)
        {
        case FC_PUSH_VPM:
            if((pAsm->CALLSTACK[pAsm->CALLSP].current + 1)
                    > pAsm->CALLSTACK[pAsm->CALLSP].max)
            {
                pAsm->CALLSTACK[pAsm->CALLSP].max =
                    pAsm->CALLSTACK[pAsm->CALLSP].current + 1;
            }
            break;
        case FC_PUSH_WQM:
            if((pAsm->CALLSTACK[pAsm->CALLSP].current + 4)
                    > pAsm->CALLSTACK[pAsm->CALLSP].max)
            {
                pAsm->CALLSTACK[pAsm->CALLSP].max =
                    pAsm->CALLSTACK[pAsm->CALLSP].current + 4;
            }
            break;
        }
        return;
    }

    switch (uReason)
    {
    case FC_PUSH_VPM:
        pAsm->CALLSTACK[pAsm->CALLSP].current++;
        break;
    case FC_PUSH_WQM:
        pAsm->CALLSTACK[pAsm->CALLSP].current += 4;
        break;
    case FC_LOOP:
        pAsm->CALLSTACK[pAsm->CALLSP].current += 4;
        break;
    case FC_REP:
        /* TODO : for 16 vp asic, should += 2; */
        pAsm->CALLSTACK[pAsm->CALLSP].current += 1;
        break;
    };

    if(pAsm->CALLSTACK[pAsm->CALLSP].current
         > pAsm->CALLSTACK[pAsm->CALLSP].max)
    {
        pAsm->CALLSTACK[pAsm->CALLSP].max =
            pAsm->CALLSTACK[pAsm->CALLSP].current;
    }
}

GLboolean jumpToOffest(r700_AssemblerBase *pAsm, GLuint pops, GLint offset)
{
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = pops;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_JUMP;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->cf_current_cf_clause_ptr->m_Word0.f.addr = pAsm->cf_current_cf_clause_ptr->m_uIndex + offset;

    return GL_TRUE;
}

GLboolean pops(r700_AssemblerBase *pAsm, GLuint pops)
{
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = pops;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_POP;
 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;
    pAsm->cf_current_cf_clause_ptr->m_Word0.f.addr             = pAsm->cf_current_cf_clause_ptr->m_uIndex + 1;

    return GL_TRUE;
}

GLboolean assemble_IF(r700_AssemblerBase *pAsm, GLboolean bHasElse)
{
    pAsm->alu_x_opcode = SQ_CF_INST_ALU_PUSH_BEFORE;

    assemble_LOGIC_PRED(pAsm, SQ_OP2_INST_PRED_SETNE);


    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    if(GL_TRUE != bHasElse)
    {
        pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count = 1; 
    }
    else
    {
        pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count = 0;
    }
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_JUMP;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->FCSP++;
	pAsm->fc_stack[pAsm->FCSP].type  = FC_IF;
    pAsm->fc_stack[pAsm->FCSP].mid   = NULL;
    pAsm->fc_stack[pAsm->FCSP].midLen= 0;
    pAsm->fc_stack[pAsm->FCSP].first = pAsm->cf_current_cf_clause_ptr;

#ifndef USE_CF_FOR_POP_AFTER
    if(GL_TRUE != bHasElse)
    {
        pAsm->alu_x_opcode = SQ_CF_INST_ALU_POP_AFTER;
    }
#endif /* USE_CF_FOR_POP_AFTER */

    checkStackDepth(pAsm, FC_PUSH_VPM, GL_FALSE); 

    return GL_TRUE;
}

GLboolean assemble_ELSE(r700_AssemblerBase *pAsm)
{
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1; ///
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_ELSE;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->fc_stack[pAsm->FCSP].mid = (R700ControlFlowGenericClause **)_mesa_realloc( (void *)pAsm->fc_stack[pAsm->FCSP].mid,
                                                                                     0,
                                                                                     sizeof(R700ControlFlowGenericClause *) );
    pAsm->fc_stack[pAsm->FCSP].mid[0] = pAsm->cf_current_cf_clause_ptr;
    //pAsm->fc_stack[pAsm->FCSP].unNumMid = 1;

#ifndef USE_CF_FOR_POP_AFTER
    pAsm->alu_x_opcode = SQ_CF_INST_ALU_POP_AFTER;
#endif /* USE_CF_FOR_POP_AFTER */

    pAsm->fc_stack[pAsm->FCSP].first->m_Word0.f.addr = pAsm->pR700Shader->plstCFInstructions_active->uNumOfNode - 1; 

    return GL_TRUE;
}

GLboolean assemble_ENDIF(r700_AssemblerBase *pAsm)
{
#ifdef USE_CF_FOR_POP_AFTER
    pops(pAsm, 1); 
#endif /* USE_CF_FOR_POP_AFTER */

    pAsm->alu_x_opcode = SQ_CF_INST_ALU;

    if(NULL == pAsm->fc_stack[pAsm->FCSP].mid)
    {
        /* no else in between */
        pAsm->fc_stack[pAsm->FCSP].first->m_Word0.f.addr = pAsm->pR700Shader->plstCFInstructions_active->uNumOfNode;
    }
    else
    {
        pAsm->fc_stack[pAsm->FCSP].mid[0]->m_Word0.f.addr = pAsm->pR700Shader->plstCFInstructions_active->uNumOfNode;
    }

    if(NULL != pAsm->fc_stack[pAsm->FCSP].mid)
    {
        FREE(pAsm->fc_stack[pAsm->FCSP].mid);
    }

    if(pAsm->fc_stack[pAsm->FCSP].type != FC_IF)
    {
        radeon_error("if/endif in shader code are not paired. \n");
        return GL_FALSE;
    }
    
    pAsm->FCSP--;

    decreaseCurrent(pAsm, FC_PUSH_VPM);

    return GL_TRUE;
}

GLboolean assemble_BGNLOOP(r700_AssemblerBase *pAsm)
{
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_LOOP_START_NO_AL;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->FCSP++;
	pAsm->fc_stack[pAsm->FCSP].type  = FC_LOOP;
    pAsm->fc_stack[pAsm->FCSP].mid   = NULL;
    pAsm->fc_stack[pAsm->FCSP].unNumMid = 0;
    pAsm->fc_stack[pAsm->FCSP].midLen   = 0;
    pAsm->fc_stack[pAsm->FCSP].first    = pAsm->cf_current_cf_clause_ptr;

    checkStackDepth(pAsm, FC_LOOP, GL_FALSE);

    return GL_TRUE;
}

GLboolean assemble_BRK(r700_AssemblerBase *pAsm)
{
#ifdef USE_CF_FOR_CONTINUE_BREAK

    pAsm->alu_x_opcode = SQ_CF_INST_ALU_PUSH_BEFORE;

    assemble_LOGIC_PRED(pAsm, SQ_OP2_INST_PRED_SETNE);
    
    unsigned int unFCSP;
    for(unFCSP=pAsm->FCSP; unFCSP>0; unFCSP--)
    {
        if(FC_LOOP == pAsm->fc_stack[unFCSP].type)
        {
            break;
        }
    }
    if(0 == FC_LOOP)
    {
        radeon_error("Break is not inside loop/endloop pair.\n");
        return GL_FALSE;
    }

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_LOOP_BREAK;
 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->fc_stack[unFCSP].mid = (R700ControlFlowGenericClause **)_mesa_realloc( 
                                              (void *)pAsm->fc_stack[unFCSP].mid,
                                              sizeof(R700ControlFlowGenericClause *) * pAsm->fc_stack[unFCSP].unNumMid,
                                              sizeof(R700ControlFlowGenericClause *) * (pAsm->fc_stack[unFCSP].unNumMid + 1) );
    pAsm->fc_stack[unFCSP].mid[pAsm->fc_stack[unFCSP].unNumMid] = pAsm->cf_current_cf_clause_ptr;
    pAsm->fc_stack[unFCSP].unNumMid++;

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_POP;
 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;
    pAsm->cf_current_cf_clause_ptr->m_Word0.f.addr             = pAsm->cf_current_cf_clause_ptr->m_uIndex + 1;

    checkStackDepth(pAsm, FC_PUSH_VPM, GL_TRUE);

#endif //USE_CF_FOR_CONTINUE_BREAK
    return GL_TRUE;
}

GLboolean assemble_CONT(r700_AssemblerBase *pAsm)
{
#ifdef USE_CF_FOR_CONTINUE_BREAK
    pAsm->alu_x_opcode = SQ_CF_INST_ALU_PUSH_BEFORE;

    assemble_LOGIC_PRED(pAsm, SQ_OP2_INST_PRED_SETNE);

    unsigned int unFCSP;
    for(unFCSP=pAsm->FCSP; unFCSP>0; unFCSP--)
    {
        if(FC_LOOP == pAsm->fc_stack[unFCSP].type)
        {
            break;
        }
    }
    if(0 == FC_LOOP)
    {
        radeon_error("Continue is not inside loop/endloop pair.\n");
        return GL_FALSE;
    }

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_LOOP_CONTINUE;
 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->fc_stack[unFCSP].mid = (R700ControlFlowGenericClause **)_mesa_realloc( 
                                              (void *)pAsm->fc_stack[unFCSP].mid,
                                              sizeof(R700ControlFlowGenericClause *) * pAsm->fc_stack[unFCSP].unNumMid,
                                              sizeof(R700ControlFlowGenericClause *) * (pAsm->fc_stack[unFCSP].unNumMid + 1) );
    pAsm->fc_stack[unFCSP].mid[pAsm->fc_stack[unFCSP].unNumMid] = pAsm->cf_current_cf_clause_ptr;
    pAsm->fc_stack[unFCSP].unNumMid++;

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_POP;
 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;
    pAsm->cf_current_cf_clause_ptr->m_Word0.f.addr             = pAsm->cf_current_cf_clause_ptr->m_uIndex + 1;

    checkStackDepth(pAsm, FC_PUSH_VPM, GL_TRUE);

#endif /* USE_CF_FOR_CONTINUE_BREAK */

    return GL_TRUE;
}

GLboolean assemble_ENDLOOP(r700_AssemblerBase *pAsm)
{
    GLuint i;

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_LOOP_END;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->cf_current_cf_clause_ptr->m_Word0.f.addr   = pAsm->fc_stack[pAsm->FCSP].first->m_uIndex + 1;
    pAsm->fc_stack[pAsm->FCSP].first->m_Word0.f.addr = pAsm->cf_current_cf_clause_ptr->m_uIndex + 1;

#ifdef USE_CF_FOR_CONTINUE_BREAK
    for(i=0; i<pAsm->fc_stack[pAsm->FCSP].unNumMid; i++)
    {
        pAsm->fc_stack[pAsm->FCSP].mid[i]->m_Word0.f.addr = pAsm->cf_current_cf_clause_ptr->m_uIndex;
    }
    if(NULL != pAsm->fc_stack[pAsm->FCSP].mid)
    {
        FREE(pAsm->fc_stack[pAsm->FCSP].mid);
    }
#endif

    if(pAsm->fc_stack[pAsm->FCSP].type != FC_LOOP)
    {
        radeon_error("loop/endloop in shader code are not paired. \n");
        return GL_FALSE;
    }

    GLuint unFCSP;
    GLuint unIF = 0;
    if((pAsm->unCFflags & HAS_CURRENT_LOOPRET) > 0)
    {        
        for(unFCSP=(pAsm->FCSP-1); unFCSP>pAsm->CALLSTACK[pAsm->CALLSP].FCSP_BeforeEntry; unFCSP--)
        {
            if(FC_LOOP == pAsm->fc_stack[unFCSP].type)
            {
                breakLoopOnFlag(pAsm, unFCSP);
                break;
            }
            else if(FC_IF == pAsm->fc_stack[unFCSP].type)
            {
                unIF++;
            }
        }
        if(unFCSP <= pAsm->CALLSTACK[pAsm->CALLSP].FCSP_BeforeEntry)
        {            
#ifdef USE_CF_FOR_POP_AFTER
            returnOnFlag(pAsm, unIF); 
#else
            returnOnFlag(pAsm, 0);
#endif /* USE_CF_FOR_POP_AFTER */
            pAsm->unCFflags &= ~HAS_CURRENT_LOOPRET;
        }
    }

    pAsm->FCSP--;

    decreaseCurrent(pAsm, FC_LOOP);
    
    return GL_TRUE;
}

void add_return_inst(r700_AssemblerBase *pAsm)
{
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return;
    }
    //pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_RETURN;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;
}

GLboolean assemble_BGNSUB(r700_AssemblerBase *pAsm, GLint nILindex, GLuint uiIL_Shift)
{
    /* Put in sub */
    if( (pAsm->unSubArrayPointer + 1) > pAsm->unSubArraySize )
    {
        pAsm->subs = (SUB_OFFSET*)_mesa_realloc( (void *)pAsm->subs,
                                  sizeof(SUB_OFFSET) * pAsm->unSubArraySize,
                                  sizeof(SUB_OFFSET) * (pAsm->unSubArraySize + 10) );
        if(NULL == pAsm->subs)
        {
            return GL_FALSE;
        }
        pAsm->unSubArraySize += 10;
    }

    pAsm->subs[pAsm->unSubArrayPointer].subIL_Offset = nILindex + uiIL_Shift;
    pAsm->subs[pAsm->unSubArrayPointer].lstCFInstructions_local.pHead=NULL;  
    pAsm->subs[pAsm->unSubArrayPointer].lstCFInstructions_local.pTail=NULL;  
    pAsm->subs[pAsm->unSubArrayPointer].lstCFInstructions_local.uNumOfNode=0;

    pAsm->CALLSP++;
    pAsm->CALLSTACK[pAsm->CALLSP].subDescIndex = pAsm->unSubArrayPointer;
    pAsm->CALLSTACK[pAsm->CALLSP].FCSP_BeforeEntry = pAsm->FCSP;
    pAsm->CALLSTACK[pAsm->CALLSP].plstCFInstructions_local
                   = &(pAsm->subs[pAsm->unSubArrayPointer].lstCFInstructions_local);
    pAsm->CALLSTACK[pAsm->CALLSP].max = 0;
    pAsm->CALLSTACK[pAsm->CALLSP].current = 0;
    SetActiveCFlist(pAsm->pR700Shader, 
                    pAsm->CALLSTACK[pAsm->CALLSP].plstCFInstructions_local);

    pAsm->unSubArrayPointer++;

    /* start sub */
    pAsm->alu_x_opcode = SQ_CF_INST_ALU;

    pAsm->FCSP++;
    pAsm->fc_stack[pAsm->FCSP].type  = FC_REP;

    checkStackDepth(pAsm, FC_REP, GL_FALSE);

    return GL_TRUE;
}

GLboolean assemble_ENDSUB(r700_AssemblerBase *pAsm)
{
    if(pAsm->fc_stack[pAsm->FCSP].type != FC_REP)
    {
        radeon_error("BGNSUB/ENDSUB in shader code are not paired. \n");
        return GL_FALSE;
    }

    /* copy max to sub structure */
    pAsm->subs[pAsm->CALLSTACK[pAsm->CALLSP].subDescIndex].unStackDepthMax
        = pAsm->CALLSTACK[pAsm->CALLSP].max;

    decreaseCurrent(pAsm, FC_REP);

    pAsm->CALLSP--;
    SetActiveCFlist(pAsm->pR700Shader, 
                    pAsm->CALLSTACK[pAsm->CALLSP].plstCFInstructions_local);
    
    pAsm->alu_x_opcode = SQ_CF_INST_ALU;

    pAsm->FCSP--;

    return GL_TRUE;
}

GLboolean assemble_RET(r700_AssemblerBase *pAsm)
{
    GLuint unIF = 0;

    if(pAsm->CALLSP > 0)
    {   /* in sub */
        GLuint unFCSP;        
        for(unFCSP=pAsm->FCSP; unFCSP>pAsm->CALLSTACK[pAsm->CALLSP].FCSP_BeforeEntry; unFCSP--)
        {
            if(FC_LOOP == pAsm->fc_stack[unFCSP].type)
            {
                setRetInLoopFlag(pAsm, SQ_SEL_1);
                breakLoopOnFlag(pAsm, unFCSP);
                pAsm->unCFflags |= LOOPRET_FLAGS;

                return GL_TRUE;
            }
            else if(FC_IF == pAsm->fc_stack[unFCSP].type)
            {
                unIF++;
            }
        }
    }

#ifdef USE_CF_FOR_POP_AFTER    
    if(unIF > 0)
    {
        pops(pAsm, unIF);
    }
#endif /* USE_CF_FOR_POP_AFTER */

    add_return_inst(pAsm);

    return GL_TRUE;
}

GLboolean assemble_CAL(r700_AssemblerBase *pAsm, 
                       GLint nILindex,
                       GLuint uiIL_Shift,
                       GLuint uiNumberInsts,
                       struct prog_instruction *pILInst,
                       PRESUB_DESC * pPresubDesc)
{
    GLint uiIL_Offset;

    pAsm->alu_x_opcode = SQ_CF_INST_ALU;

    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.call_count       = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_CALL;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    /* Put in caller */
    if( (pAsm->unCallerArrayPointer + 1) > pAsm->unCallerArraySize )
    {
        pAsm->callers = (CALLER_POINTER*)_mesa_realloc( (void *)pAsm->callers, 
                       sizeof(CALLER_POINTER) * pAsm->unCallerArraySize, 
                       sizeof(CALLER_POINTER) * (pAsm->unCallerArraySize + 10) );
        if(NULL == pAsm->callers)
        {
            return GL_FALSE;
        }
        pAsm->unCallerArraySize += 10;
    }
    
    uiIL_Offset = nILindex + uiIL_Shift;
    pAsm->callers[pAsm->unCallerArrayPointer].subIL_Offset = uiIL_Offset; 
    pAsm->callers[pAsm->unCallerArrayPointer].cf_ptr       = pAsm->cf_current_cf_clause_ptr;
    
    pAsm->callers[pAsm->unCallerArrayPointer].finale_cf_ptr  = NULL; 
    pAsm->callers[pAsm->unCallerArrayPointer].prelude_cf_ptr = NULL; 

    pAsm->unCallerArrayPointer++;

    int j;
    GLuint max;
    GLuint unSubID;
    GLboolean bRet;
    for(j=0; j<pAsm->unSubArrayPointer; j++)
    {
        if(uiIL_Offset == pAsm->subs[j].subIL_Offset)
        {   /* compiled before */

            max = pAsm->subs[j].unStackDepthMax 
                + pAsm->CALLSTACK[pAsm->CALLSP].current;
            if(max > pAsm->CALLSTACK[pAsm->CALLSP].max)
            {
                pAsm->CALLSTACK[pAsm->CALLSP].max = max;
            }
            
            pAsm->callers[pAsm->unCallerArrayPointer - 1].subDescIndex = j; 
            return GL_TRUE;
        }
    }

    pAsm->callers[pAsm->unCallerArrayPointer - 1].subDescIndex = pAsm->unSubArrayPointer;
    unSubID = pAsm->unSubArrayPointer;

    bRet = AssembleInstr(nILindex, uiIL_Shift, uiNumberInsts, pILInst, pAsm);

    if(GL_TRUE == bRet)
    {
        max = pAsm->subs[unSubID].unStackDepthMax 
            + pAsm->CALLSTACK[pAsm->CALLSP].current;
        if(max > pAsm->CALLSTACK[pAsm->CALLSP].max)
        {
            pAsm->CALLSTACK[pAsm->CALLSP].max = max;
        }

        pAsm->subs[unSubID].pPresubDesc = pPresubDesc;
    }

    return bRet;
}

GLboolean setRetInLoopFlag(r700_AssemblerBase *pAsm, GLuint flagValue)
{
    /*GLfloat fLiteral[2] = {0.1, 0.0};*/

    pAsm->D.dst.opcode   = SQ_OP2_INST_MOV;
    pAsm->D.dst.op3      = 0;
    pAsm->D.dst.rtype    = DST_REG_TEMPORARY;
    pAsm->D.dst.reg      = pAsm->flag_reg_index;
    pAsm->D.dst.writex   = 1;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 0;
    pAsm->D.dst.writew   = 0;
    pAsm->D2.dst2.literal_slots      = 1;
    pAsm->D2.dst2.SaturateMode = SATURATE_OFF;
    pAsm->D.dst.predicated     = 0;
    /* in reloc where dislink flag init inst, only one slot alu inst is handled. */
    pAsm->D.dst.math           = 1; /* TODO : not math really, but one channel op, more generic alu assembler needed */
    pAsm->D2.dst2.index_mode = SQ_INDEX_LOOP; /* Check this ! */
#if 0
    pAsm->S[0].src.rtype = SRC_REC_LITERAL;
    //pAsm->S[0].src.reg   = 0;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[0].src));
    pAsm->S[0].src.swizzlex = SQ_SEL_X;
    pAsm->S[0].src.swizzley = SQ_SEL_Y;
    pAsm->S[0].src.swizzlez = SQ_SEL_Z;
    pAsm->S[0].src.swizzlew = SQ_SEL_W;

    if( GL_FALSE == next_ins_literal(pAsm, &(fLiteral[0])) )
    {
        return GL_FALSE;
    }
#else
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = 0;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[0].src));
    pAsm->S[0].src.swizzlex = flagValue;
    pAsm->S[0].src.swizzley = flagValue;
    pAsm->S[0].src.swizzlez = flagValue;
    pAsm->S[0].src.swizzlew = flagValue;

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }
#endif

    return GL_TRUE;
}

GLboolean testFlag(r700_AssemblerBase *pAsm)
{
    /*GLfloat fLiteral[2] = {0.1, 0.0};*/

    //Test flag
    GLuint tmp = gethelpr(pAsm);
    pAsm->alu_x_opcode = SQ_CF_INST_ALU_PUSH_BEFORE;

    pAsm->D.dst.opcode   = SQ_OP2_INST_PRED_SETE;
    pAsm->D.dst.math     = 1;
    pAsm->D.dst.rtype    = DST_REG_TEMPORARY;
    pAsm->D.dst.reg      = tmp;
    pAsm->D.dst.writex   = 1;
    pAsm->D.dst.writey   = 0;
    pAsm->D.dst.writez   = 0;
    pAsm->D.dst.writew   = 0;
    pAsm->D2.dst2.literal_slots      = 1;
    pAsm->D2.dst2.SaturateMode = SATURATE_OFF;
    pAsm->D.dst.predicated     = 1;
    pAsm->D2.dst2.index_mode = SQ_INDEX_LOOP; /* Check this ! */

    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = pAsm->flag_reg_index;
    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[0].src));
    pAsm->S[0].src.swizzlex = SQ_SEL_X;
    pAsm->S[0].src.swizzley = SQ_SEL_Y;
    pAsm->S[0].src.swizzlez = SQ_SEL_Z;
    pAsm->S[0].src.swizzlew = SQ_SEL_W;
#if 0
    pAsm->S[1].src.rtype = SRC_REC_LITERAL;
    //pAsm->S[1].src.reg   = 0;
    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[1].src));
    pAsm->S[1].src.swizzlex = SQ_SEL_X;
    pAsm->S[1].src.swizzley = SQ_SEL_Y;
    pAsm->S[1].src.swizzlez = SQ_SEL_Z;
    pAsm->S[1].src.swizzlew = SQ_SEL_W;

    if( GL_FALSE == next_ins_literal(pAsm, &(fLiteral[0])) )
    {
        return GL_FALSE;
    }
#else
    pAsm->S[1].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[1].src.reg   = 0;
    setaddrmode_PVSSRC(&(pAsm->S[1].src), ADDR_ABSOLUTE);
    noneg_PVSSRC(&(pAsm->S[1].src));
    pAsm->S[1].src.swizzlex = SQ_SEL_1;
    pAsm->S[1].src.swizzley = SQ_SEL_1;
    pAsm->S[1].src.swizzlez = SQ_SEL_1;
    pAsm->S[1].src.swizzlew = SQ_SEL_1;

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }
#endif

    checkStackDepth(pAsm, FC_PUSH_VPM, GL_TRUE);

    return GL_TRUE;
}

GLboolean returnOnFlag(r700_AssemblerBase *pAsm, GLuint unIF)
{
    testFlag(pAsm);
    jumpToOffest(pAsm, 1, 4);
    setRetInLoopFlag(pAsm, SQ_SEL_0);
    pops(pAsm, unIF + 1);
    add_return_inst(pAsm);

    return GL_TRUE;
}

GLboolean breakLoopOnFlag(r700_AssemblerBase *pAsm, GLuint unFCSP)
{
    testFlag(pAsm);
 
    //break
    if(GL_FALSE == add_cf_instruction(pAsm) )
    {
        return GL_FALSE;
    }
    
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.pop_count        = 1;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_const         = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cond             = SQ_CF_COND_ACTIVE;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0; 
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_LOOP_BREAK;
    pAsm->cf_current_cf_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;

    pAsm->cf_current_cf_clause_ptr->m_Word1.f.barrier          = 0x1;

    pAsm->fc_stack[unFCSP].mid = (R700ControlFlowGenericClause **)_mesa_realloc( 
                                              (void *)pAsm->fc_stack[unFCSP].mid,
                                              sizeof(R700ControlFlowGenericClause *) * pAsm->fc_stack[unFCSP].unNumMid,
                                              sizeof(R700ControlFlowGenericClause *) * (pAsm->fc_stack[unFCSP].unNumMid + 1) );
    pAsm->fc_stack[unFCSP].mid[pAsm->fc_stack[unFCSP].unNumMid] = pAsm->cf_current_cf_clause_ptr;
    pAsm->fc_stack[unFCSP].unNumMid++;

    pops(pAsm, 1);
               
    return GL_TRUE;
}

GLboolean AssembleInstr(GLuint uiFirstInst,
                        GLuint uiIL_Shift,
                        GLuint uiNumberInsts,
                        struct prog_instruction *pILInst, 
						r700_AssemblerBase *pR700AsmCode)
{
    GLuint i;

    pR700AsmCode->pILInst = pILInst;
	for(i=uiFirstInst; i<uiNumberInsts; i++)
    {
        pR700AsmCode->uiCurInst = i;

#ifndef USE_CF_FOR_CONTINUE_BREAK
        if(OPCODE_BRK == pILInst[i+1].Opcode)
        {
            switch(pILInst[i].Opcode)            
            {
            case OPCODE_SLE:
                pILInst[i].Opcode = OPCODE_SGT;
                break;
            case OPCODE_SLT:
                pILInst[i].Opcode = OPCODE_SGE;
                break;
            case OPCODE_SGE:
                pILInst[i].Opcode = OPCODE_SLT;
                break;
            case OPCODE_SGT:
                pILInst[i].Opcode = OPCODE_SLE;
                break;
            case OPCODE_SEQ:
                pILInst[i].Opcode = OPCODE_SNE;
                break;
            case OPCODE_SNE:
                pILInst[i].Opcode = OPCODE_SEQ;
                break;
            default:
                break;
            }
        }
#endif
        if(pILInst[i].CondUpdate == 1)
        {
            /* remember dest register used for cond evaluation */
            /* XXX also handle PROGRAM_OUTPUT registers here? */
            pR700AsmCode->last_cond_register = pILInst[i].DstReg.Index; 
        }

        switch (pILInst[i].Opcode)
        {
        case OPCODE_ABS: 
            if ( GL_FALSE == assemble_ABS(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_ADD: 
        case OPCODE_SUB: 
            if ( GL_FALSE == assemble_ADD(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_ARL: 
            if ( GL_FALSE == assemble_ARL(pR700AsmCode) ) 
                return GL_FALSE;
            break;
        case OPCODE_ARR: 
            radeon_error("Not yet implemented instruction OPCODE_ARR \n");
            //if ( GL_FALSE == assemble_BAD("ARR") ) 
                return GL_FALSE;
            break;

        case OPCODE_CMP: 
            if ( GL_FALSE == assemble_CMP(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_COS: 
            if ( GL_FALSE == assemble_TRIG(pR700AsmCode, SQ_OP2_INST_COS) ) 
                return GL_FALSE;
            break;  

        case OPCODE_DP3: 
        case OPCODE_DP4: 
        case OPCODE_DPH: 
            if ( GL_FALSE == assemble_DOT(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_DST: 
            if ( GL_FALSE == assemble_DST(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_EX2: 
            if ( GL_FALSE == assemble_EX2(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_EXP: 
            if ( GL_FALSE == assemble_EXP(pR700AsmCode) ) 
                return GL_FALSE;
            break;

        case OPCODE_FLR:     
            if ( GL_FALSE == assemble_FLR(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        //case OP_FLR_INT: ;

        //    if ( GL_FALSE == assemble_FLR_INT() ) 
        //        return GL_FALSE;
        //    break;  

        case OPCODE_FRC: 
            if ( GL_FALSE == assemble_FRC(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_KIL: 
        case OPCODE_KIL_NV: 
            if ( GL_FALSE == assemble_KIL(pR700AsmCode, SQ_OP2_INST_KILLGT) ) 
                return GL_FALSE;
            break;
        case OPCODE_LG2: 
            if ( GL_FALSE == assemble_LG2(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_LIT:
            if ( GL_FALSE == assemble_LIT(pR700AsmCode) ) 
                return GL_FALSE;
            break;
        case OPCODE_LRP: 
            if ( GL_FALSE == assemble_LRP(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_LOG: 
            if ( GL_FALSE == assemble_LOG(pR700AsmCode) ) 
                return GL_FALSE;
            break;

        case OPCODE_MAD: 
            if ( GL_FALSE == assemble_MAD(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_MAX: 
            if ( GL_FALSE == assemble_MAX(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_MIN: 
            if ( GL_FALSE == assemble_MIN(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_MOV: 
            if ( GL_FALSE == assemble_MOV(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_MUL: 
            if ( GL_FALSE == assemble_MUL(pR700AsmCode) ) 
                return GL_FALSE;
            break;
            
        case OPCODE_NOISE1:
            {                                               
                callPreSub(pR700AsmCode, 
                           GLSL_NOISE1,                         
                           &noise1_presub,                                                  
                           pILInst->DstReg.Index + pR700AsmCode->starting_temp_register_number, 
                           1); 
                radeon_error("noise1: not yet supported shader instruction\n");
            };
            break; 
        case OPCODE_NOISE2: 
            radeon_error("noise2: not yet supported shader instruction\n");
            break; 
        case OPCODE_NOISE3: 
            radeon_error("noise3: not yet supported shader instruction\n");
            break; 
        case OPCODE_NOISE4: 
            radeon_error("noise4: not yet supported shader instruction\n");
            break; 

        case OPCODE_POW: 
            if ( GL_FALSE == assemble_POW(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_RCP: 
            if ( GL_FALSE == assemble_RCP(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_RSQ: 
            if ( GL_FALSE == assemble_RSQ(pR700AsmCode) ) 
                return GL_FALSE;
            break;  
        case OPCODE_SIN: 
            if ( GL_FALSE == assemble_TRIG(pR700AsmCode, SQ_OP2_INST_SIN) ) 
                return GL_FALSE;
            break;  
        case OPCODE_SCS: 
            if ( GL_FALSE == assemble_SCS(pR700AsmCode) ) 
                return GL_FALSE;
            break; 
            
        case OPCODE_SEQ:
            if ( GL_FALSE == assemble_LOGIC(pR700AsmCode, SQ_OP2_INST_SETE) ) 
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_SGT: 
            if ( GL_FALSE == assemble_LOGIC(pR700AsmCode, SQ_OP2_INST_SETGT) ) 
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_SGE: 
            if ( GL_FALSE == assemble_SGE(pR700AsmCode) ) 
            { 
                return GL_FALSE;
            }
            break;
        
        /* NO LT, LE, TODO : use GE => LE, GT => LT : reverse 2 src order would be simpliest. Or use SQ_CF_COND_FALSE for SQ_CF_COND_ACTIVE.*/
        case OPCODE_SLT: 
            {
                struct prog_src_register SrcRegSave[2];
                SrcRegSave[0] = pILInst[i].SrcReg[0];
                SrcRegSave[1] = pILInst[i].SrcReg[1];
                pILInst[i].SrcReg[0] = SrcRegSave[1];
                pILInst[i].SrcReg[1] = SrcRegSave[0];
                if ( GL_FALSE == assemble_LOGIC(pR700AsmCode, SQ_OP2_INST_SETGT) ) 
                {
                    pILInst[i].SrcReg[0] = SrcRegSave[0];
                    pILInst[i].SrcReg[1] = SrcRegSave[1];
                    return GL_FALSE;
                }
                pILInst[i].SrcReg[0] = SrcRegSave[0];
                pILInst[i].SrcReg[1] = SrcRegSave[1];
            }
            break;

        case OPCODE_SLE: 
            {
                struct prog_src_register SrcRegSave[2];
                SrcRegSave[0] = pILInst[i].SrcReg[0];
                SrcRegSave[1] = pILInst[i].SrcReg[1];
                pILInst[i].SrcReg[0] = SrcRegSave[1];
                pILInst[i].SrcReg[1] = SrcRegSave[0];
                if ( GL_FALSE == assemble_LOGIC(pR700AsmCode, SQ_OP2_INST_SETGE) ) 
                {
                    pILInst[i].SrcReg[0] = SrcRegSave[0];
                    pILInst[i].SrcReg[1] = SrcRegSave[1];
                    return GL_FALSE;
                }
                pILInst[i].SrcReg[0] = SrcRegSave[0];
                pILInst[i].SrcReg[1] = SrcRegSave[1];
            }
            break;

        case OPCODE_SNE: 
            if ( GL_FALSE == assemble_LOGIC(pR700AsmCode, SQ_OP2_INST_SETNE) ) 
            {
                return GL_FALSE;
            }
            break;

        //case OP_STP: 
        //    if ( GL_FALSE == assemble_STP(pR700AsmCode) ) 
        //        return GL_FALSE;
        //    break;

        case OPCODE_SWZ: 
            if ( GL_FALSE == assemble_MOV(pR700AsmCode) ) 
            {
                return GL_FALSE; 
            }
            else
            {
                if( (i+1)<uiNumberInsts )
                {
                    if(OPCODE_END != pILInst[i+1].Opcode)
                    {
                        if( GL_TRUE == IsTex(pILInst[i+1].Opcode) )
                        {
                            pR700AsmCode->pInstDeps[i+1].nDstDep = i+1; //=1?
                        }
                    }
                }
            }
            break;
        case OPCODE_DDX:
        case OPCODE_DDY:
        case OPCODE_TEX: 
        case OPCODE_TXB:  
        case OPCODE_TXP: 
            if ( GL_FALSE == assemble_TEX(pR700AsmCode) ) 
                return GL_FALSE;
            break;

        case OPCODE_TRUNC:
            if ( GL_FALSE == assemble_math_function(pR700AsmCode, SQ_OP2_INST_TRUNC) )
                return GL_FALSE;
            break;

        case OPCODE_XPD: 
            if ( GL_FALSE == assemble_XPD(pR700AsmCode) ) 
                return GL_FALSE;
            break;  

        case OPCODE_IF:
            {                
                GLboolean bHasElse = GL_FALSE;

                if(pILInst[pILInst[i].BranchTarget].Opcode == OPCODE_ELSE)
                {
                    bHasElse = GL_TRUE;
                }

                if ( GL_FALSE == assemble_IF(pR700AsmCode, bHasElse) ) 
                {
                    return GL_FALSE;
                }
            }
            break;

        case OPCODE_ELSE : 
            if ( GL_FALSE == assemble_ELSE(pR700AsmCode) ) 
                return GL_FALSE;
            break;

        case OPCODE_ENDIF: 
            if ( GL_FALSE == assemble_ENDIF(pR700AsmCode) ) 
                return GL_FALSE;
            break;

        case OPCODE_BGNLOOP:
            if( GL_FALSE == assemble_BGNLOOP(pR700AsmCode) )
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_BRK:
            if( GL_FALSE == assemble_BRK(pR700AsmCode) )
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_CONT:
            if( GL_FALSE == assemble_CONT(pR700AsmCode) )
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_ENDLOOP:
            if( GL_FALSE == assemble_ENDLOOP(pR700AsmCode) )
            {
                return GL_FALSE;
            }
            break;

        case OPCODE_BGNSUB:
            if( GL_FALSE == assemble_BGNSUB(pR700AsmCode, i, uiIL_Shift) )
            {
                return GL_FALSE;
            }
            break;
        
        case OPCODE_RET:
            if( GL_FALSE == assemble_RET(pR700AsmCode) )
            {
                return GL_FALSE;
            }
            break;
        
        case OPCODE_CAL:
            if( GL_FALSE == assemble_CAL(pR700AsmCode, 
                                         pILInst[i].BranchTarget,
                                         uiIL_Shift,
                                         uiNumberInsts,
                                         pILInst,
                                         NULL) )
            {
                return GL_FALSE;
            }
            break;

        //case OPCODE_EXPORT: 
        //    if ( GL_FALSE == assemble_EXPORT() ) 
        //        return GL_FALSE;
        //    break;

        case OPCODE_ENDSUB:
            return assemble_ENDSUB(pR700AsmCode);

        case OPCODE_END: 
			//pR700AsmCode->uiCurInst = i;
			//This is to remaind that if in later exoort there is depth/stencil
			//export, we need a mov to re-arrange DST channel, where using a
			//psuedo inst, we will use this end inst to do it.
            return GL_TRUE;

        default:
            radeon_error("internal: unknown instruction\n");
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

GLboolean InitShaderProgram(r700_AssemblerBase * pAsm)
{
    setRetInLoopFlag(pAsm, SQ_SEL_0);
    pAsm->alu_x_opcode = SQ_CF_INST_ALU;
    return GL_TRUE;
}

GLboolean RelocProgram(r700_AssemblerBase * pAsm, struct gl_program * pILProg)
{
    GLuint i;
    GLuint unCFoffset;
    TypedShaderList * plstCFmain;
    TypedShaderList * plstCFsub;

    R700ShaderInstruction *        pInst;
    R700ControlFlowGenericClause * pCFInst;

    R700ControlFlowALUClause * pCF_ALU;
    R700ALUInstruction       * pALU;
    GLuint                     unConstOffset = 0;
    GLuint                     unRegOffset;
    GLuint                     unMinRegIndex;

    plstCFmain = pAsm->CALLSTACK[0].plstCFInstructions_local;

    /* remove flags init if they are not used */
    if((pAsm->unCFflags & HAS_LOOPRET) == 0)
    {
        R700ControlFlowALUClause * pCF_ALU;
        pInst = plstCFmain->pHead;
        while(pInst)
        {
            if(SIT_CF_ALU == pInst->m_ShaderInstType)
            {
                pCF_ALU = (R700ControlFlowALUClause *)pInst;
                if(0 == pCF_ALU->m_Word1.f.count)
                {
                    pCF_ALU->m_Word1.f.cf_inst = SQ_CF_INST_NOP;
                }
                else
                {
                    R700ALUInstruction * pALU = pCF_ALU->m_pLinkedALUInstruction;
                    
                    pALU->m_pLinkedALUClause = NULL;
                    pALU = (R700ALUInstruction *)(pALU->pNextInst);
                    pALU->m_pLinkedALUClause = pCF_ALU;
                    pCF_ALU->m_pLinkedALUInstruction = pALU;

                    pCF_ALU->m_Word1.f.count--;
                }
                break;
            }
            pInst = pInst->pNextInst;
        };
    }

    if(pAsm->CALLSTACK[0].max > 0)
    {
        pAsm->pR700Shader->uStackSize = ((pAsm->CALLSTACK[0].max + 3)>>2) + 2;
    }

    if(0 == pAsm->unSubArrayPointer)
    {
        return GL_TRUE;
    }

    unCFoffset = plstCFmain->uNumOfNode;

    if(NULL != pILProg->Parameters)
    {        
        unConstOffset = pILProg->Parameters->NumParameters;
    }

    /* Reloc subs */
    for(i=0; i<pAsm->unSubArrayPointer; i++)
    {
        pAsm->subs[i].unCFoffset = unCFoffset;
        plstCFsub = &(pAsm->subs[i].lstCFInstructions_local);

        pInst = plstCFsub->pHead;

        /* reloc instructions */
        while(pInst)
        {
            if(SIT_CF_GENERIC == pInst->m_ShaderInstType)
            {
                pCFInst = (R700ControlFlowGenericClause *)pInst;

                switch (pCFInst->m_Word1.f.cf_inst)
                {
                case SQ_CF_INST_POP:
                case SQ_CF_INST_JUMP:
                case SQ_CF_INST_ELSE:
                case SQ_CF_INST_LOOP_END:
                case SQ_CF_INST_LOOP_START:
                case SQ_CF_INST_LOOP_START_NO_AL:
                case SQ_CF_INST_LOOP_CONTINUE:
                case SQ_CF_INST_LOOP_BREAK:
                    pCFInst->m_Word0.f.addr += unCFoffset;
                    break;
                default:
                    break;
                }
            }  
            
            pInst->m_uIndex += unCFoffset;

            pInst = pInst->pNextInst;
        };

        if(NULL != pAsm->subs[i].pPresubDesc)
        {
            GLuint                     uNumSrc;            
            
            unMinRegIndex  = pAsm->subs[i].pPresubDesc->pCompiledSub->MinRegIndex;
            unRegOffset    = pAsm->subs[i].pPresubDesc->maxStartReg;            
            unConstOffset += pAsm->subs[i].pPresubDesc->unConstantsStart;

            pInst = plstCFsub->pHead;
            while(pInst)
            {
                if(SIT_CF_ALU == pInst->m_ShaderInstType)
                {
                    pCF_ALU = (R700ControlFlowALUClause *)pInst;

                    pALU = pCF_ALU->m_pLinkedALUInstruction;
                    for(int j=0; j<=pCF_ALU->m_Word1.f.count; j++)
                    {
                        pALU->m_Word1.f.dst_gpr = pALU->m_Word1.f.dst_gpr + unRegOffset - unMinRegIndex;

                        if(pALU->m_Word0.f.src0_sel < SQ_ALU_SRC_GPR_SIZE)
                        {   
                            pALU->m_Word0.f.src0_sel = pALU->m_Word0.f.src0_sel + unRegOffset - unMinRegIndex;
                        }
                        else if(pALU->m_Word0.f.src0_sel >= SQ_ALU_SRC_CFILE_BASE)
                        {   
                            pALU->m_Word0.f.src0_sel += unConstOffset;
                        }

                        if( ((pALU->m_Word1.val >> SQ_ALU_WORD1_OP3_ALU_INST_SHIFT) & 0x0000001F) 
                            >= SQ_OP3_INST_MUL_LIT )
                        {   /* op3 : 3 srcs */
                            if(pALU->m_Word1_OP3.f.src2_sel < SQ_ALU_SRC_GPR_SIZE)
                            {   
                                pALU->m_Word1_OP3.f.src2_sel = pALU->m_Word1_OP3.f.src2_sel + unRegOffset - unMinRegIndex;
                            }
                            else if(pALU->m_Word1_OP3.f.src2_sel >= SQ_ALU_SRC_CFILE_BASE)
                            {   
                                pALU->m_Word1_OP3.f.src2_sel += unConstOffset;
                            }    
                            if(pALU->m_Word0.f.src1_sel < SQ_ALU_SRC_GPR_SIZE)
                            {   
                                pALU->m_Word0.f.src1_sel = pALU->m_Word0.f.src1_sel + unRegOffset - unMinRegIndex;
                            }
                            else if(pALU->m_Word0.f.src1_sel >= SQ_ALU_SRC_CFILE_BASE)
                            {   
                                pALU->m_Word0.f.src1_sel += unConstOffset;
                            }                                 
                        }
                        else
                        {
                            if(pAsm->bR6xx)
                            {
                                uNumSrc = r700GetNumOperands(pALU->m_Word1_OP2.f6.alu_inst, 0);
                            }
                            else
                            {
                                uNumSrc = r700GetNumOperands(pALU->m_Word1_OP2.f.alu_inst, 0);
                            }
                            if(2 == uNumSrc)
                            {   /* 2 srcs */
                                if(pALU->m_Word0.f.src1_sel < SQ_ALU_SRC_GPR_SIZE)
                                {   
                                    pALU->m_Word0.f.src1_sel = pALU->m_Word0.f.src1_sel + unRegOffset - unMinRegIndex;
                                }
                                else if(pALU->m_Word0.f.src1_sel >= SQ_ALU_SRC_CFILE_BASE)
                                {   
                                    pALU->m_Word0.f.src1_sel += unConstOffset;
                                }                                  
                            }                            
                        }
                        pALU = (R700ALUInstruction*)(pALU->pNextInst);
                    }                    
                }             
                pInst = pInst->pNextInst;
            };
        }

        /* Put sub into main */
        plstCFmain->pTail->pNextInst = plstCFsub->pHead;
        plstCFmain->pTail            = plstCFsub->pTail;
        plstCFmain->uNumOfNode      += plstCFsub->uNumOfNode;

        unCFoffset += plstCFsub->uNumOfNode;
    }

    /* reloc callers */
    for(i=0; i<pAsm->unCallerArrayPointer; i++)
    {
        pAsm->callers[i].cf_ptr->m_Word0.f.addr
            = pAsm->subs[pAsm->callers[i].subDescIndex].unCFoffset; 

        if(NULL != pAsm->subs[pAsm->callers[i].subDescIndex].pPresubDesc)
        {                 
            unMinRegIndex = pAsm->subs[pAsm->callers[i].subDescIndex].pPresubDesc->pCompiledSub->MinRegIndex;
            unRegOffset = pAsm->subs[pAsm->callers[i].subDescIndex].pPresubDesc->maxStartReg;

            if(NULL != pAsm->callers[i].prelude_cf_ptr)
            {                
                pCF_ALU = (R700ControlFlowALUClause * )(pAsm->callers[i].prelude_cf_ptr);
                pALU = pCF_ALU->m_pLinkedALUInstruction;
                for(int j=0; j<=pCF_ALU->m_Word1.f.count; j++)
                {
                    pALU->m_Word1.f.dst_gpr = pALU->m_Word1.f.dst_gpr + unRegOffset - unMinRegIndex;
                    pALU = (R700ALUInstruction*)(pALU->pNextInst);
                }
            }
            if(NULL != pAsm->callers[i].finale_cf_ptr)
            {
                pCF_ALU = (R700ControlFlowALUClause * )(pAsm->callers[i].finale_cf_ptr);
                pALU = pCF_ALU->m_pLinkedALUInstruction;
                for(int j=0; j<=pCF_ALU->m_Word1.f.count; j++)
                {
                    pALU->m_Word0.f.src0_sel = pALU->m_Word0.f.src0_sel + unRegOffset - unMinRegIndex;
                    pALU = (R700ALUInstruction*)(pALU->pNextInst);
                }
            }
        }
    }

    return GL_TRUE;
}

GLboolean callPreSub(r700_AssemblerBase* pAsm, 
                         LOADABLE_SCRIPT_SIGNITURE scriptSigniture,                          
                         COMPILED_SUB * pCompiledSub,                                               
                         GLshort uOutReg,
                         GLshort uNumValidSrc)
{
    /* save assemble context */
    GLuint starting_temp_register_number_save;
    GLuint number_used_registers_save;
    GLuint uFirstHelpReg_save;
    GLuint uHelpReg_save;
    GLuint uiCurInst_save;
    struct prog_instruction *pILInst_save;
    PRESUB_DESC * pPresubDesc;
    GLboolean     bRet;
    int i;

    R700ControlFlowGenericClause* prelude_cf_ptr = NULL;

    /* copy srcs to presub inputs */
    pAsm->alu_x_opcode = SQ_CF_INST_ALU;
    for(i=0; i<uNumValidSrc; i++)
    {
        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;
        setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
        pAsm->D.dst.rtype = DST_REG_TEMPORARY;
        pAsm->D.dst.reg   = pCompiledSub->srcRegIndex[i];
        pAsm->D.dst.writex = 1;
        pAsm->D.dst.writey = 1;
        pAsm->D.dst.writez = 1;
        pAsm->D.dst.writew = 1;

        if( GL_FALSE == assemble_src(pAsm, i, 0) )
        {
            return GL_FALSE;
        }

        next_ins(pAsm);
    }
    if(uNumValidSrc > 0)
    {
        prelude_cf_ptr     = pAsm->cf_current_alu_clause_ptr;
        pAsm->alu_x_opcode = SQ_CF_INST_ALU;
    }

    /* browse thro existing presubs. */
    for(i=0; i<pAsm->unNumPresub; i++)
    {
        if(pAsm->presubs[i].sptSigniture == scriptSigniture)
        {
            break;
        }
    }

    if(i == pAsm->unNumPresub)
    {   /* not loaded yet */
        /* save assemble context */
        number_used_registers_save         = pAsm->number_used_registers;
        uFirstHelpReg_save                 = pAsm->uFirstHelpReg;
        uHelpReg_save                      = pAsm->uHelpReg;
        starting_temp_register_number_save = pAsm->starting_temp_register_number;
        pILInst_save                       = pAsm->pILInst;
        uiCurInst_save                     = pAsm->uiCurInst;

        /* alloc in presub */
        if( (pAsm->unNumPresub + 1) > pAsm->unPresubArraySize )
        {
            pAsm->presubs = (PRESUB_DESC*)_mesa_realloc( (void *)pAsm->presubs,
                                      sizeof(PRESUB_DESC) * pAsm->unPresubArraySize,
                                      sizeof(PRESUB_DESC) * (pAsm->unPresubArraySize + 4) );
            if(NULL == pAsm->presubs)
            {
                radeon_error("No memeory to allocate built in shader function description structures. \n");
                return GL_FALSE;
            }
            pAsm->unPresubArraySize += 4;
        }
        
        pPresubDesc = &(pAsm->presubs[i]);
        pPresubDesc->sptSigniture = scriptSigniture;

        /* constants offsets need to be final resolved at reloc. */
        if(0 == pAsm->unNumPresub)
        {
            pPresubDesc->unConstantsStart = 0; 
        }
        else
        {
            pPresubDesc->unConstantsStart =  pAsm->presubs[i-1].unConstantsStart
                                           + pAsm->presubs[i-1].pCompiledSub->NumParameters;
        }

        pPresubDesc->pCompiledSub = pCompiledSub;

        pPresubDesc->subIL_Shift = pAsm->unCurNumILInsts;
        pPresubDesc->maxStartReg  = uFirstHelpReg_save;
        pAsm->unCurNumILInsts    += pCompiledSub->NumInstructions;

        pAsm->unNumPresub++;

        /* setup new assemble context */
        pAsm->starting_temp_register_number = 0;
        pAsm->number_used_registers = pCompiledSub->NumTemporaries;
        pAsm->uFirstHelpReg         = pAsm->number_used_registers;
        pAsm->uHelpReg              = pAsm->uFirstHelpReg;

        bRet = assemble_CAL(pAsm, 
                            0, 
                            pPresubDesc->subIL_Shift, 
                            pCompiledSub->NumInstructions,
                            pCompiledSub->Instructions,
                            pPresubDesc);

        
        pPresubDesc->number_used_registers = pAsm->number_used_registers;        

        /* restore assemble context */
        pAsm->number_used_registers         = number_used_registers_save; 
        pAsm->uFirstHelpReg                 = uFirstHelpReg_save;
        pAsm->uHelpReg                      = uHelpReg_save;
        pAsm->starting_temp_register_number = starting_temp_register_number_save;
        pAsm->pILInst                       = pILInst_save; 
        pAsm->uiCurInst                     = uiCurInst_save;
    }
    else
    {   /* was loaded */
        pPresubDesc = &(pAsm->presubs[i]);  
        
        bRet = assemble_CAL(pAsm, 
                            0, 
                            pPresubDesc->subIL_Shift, 
                            pCompiledSub->NumInstructions,
                            pCompiledSub->Instructions,
                            pPresubDesc);
    }

    if(GL_FALSE == bRet)
    {
        radeon_error("Shader presub assemble failed. \n");
    }
    else
    {
        /* copy presub output to real dst */ 
        pAsm->alu_x_opcode = SQ_CF_INST_ALU;
        pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

        if( GL_FALSE == assemble_dst(pAsm) )
        {
            return GL_FALSE;
        }

        setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
        pAsm->S[0].src.rtype = SRC_REG_TEMPORARY;
        pAsm->S[0].src.reg   = pCompiledSub->dstRegIndex;
        pAsm->S[0].src.swizzlex = pCompiledSub->outputSwizzleX;
        pAsm->S[0].src.swizzley = pCompiledSub->outputSwizzleY;
        pAsm->S[0].src.swizzlez = pCompiledSub->outputSwizzleZ;
        pAsm->S[0].src.swizzlew = pCompiledSub->outputSwizzleW;

        next_ins(pAsm);        

        pAsm->callers[pAsm->unCallerArrayPointer - 1].finale_cf_ptr  = pAsm->cf_current_alu_clause_ptr;
        pAsm->callers[pAsm->unCallerArrayPointer - 1].prelude_cf_ptr = prelude_cf_ptr;
        pAsm->alu_x_opcode = SQ_CF_INST_ALU;
    }

    if( (pPresubDesc->number_used_registers + pAsm->uFirstHelpReg) > pAsm->number_used_registers )
    {
        pAsm->number_used_registers = pPresubDesc->number_used_registers + pAsm->uFirstHelpReg;
    }
    if(pAsm->uFirstHelpReg > pPresubDesc->maxStartReg)
    {
        pPresubDesc->maxStartReg = pAsm->uFirstHelpReg;
    }

    return bRet;
}

GLboolean Process_Export(r700_AssemblerBase* pAsm,
                         GLuint type,
                         GLuint export_starting_index,
                         GLuint export_count, 
                         GLuint starting_register_number,
                         GLboolean is_depth_export)
{
    unsigned char ucWriteMask;

    check_current_clause(pAsm, CF_EMPTY_CLAUSE);
    check_current_clause(pAsm, CF_EXPORT_CLAUSE); //alloc the cf_current_export_clause_ptr

    pAsm->cf_current_export_clause_ptr->m_Word0.f.type = type;

    switch (type) 
    {
        case SQ_EXPORT_PIXEL:
            if(GL_TRUE == is_depth_export) 
            {
                pAsm->cf_current_export_clause_ptr->m_Word0.f.array_base  = SQ_CF_PIXEL_Z;
            }
            else 
            {
                pAsm->cf_current_export_clause_ptr->m_Word0.f.array_base  = SQ_CF_PIXEL_MRT0 + export_starting_index;
            }
            break;

        case SQ_EXPORT_POS:
            pAsm->cf_current_export_clause_ptr->m_Word0.f.array_base  = SQ_CF_POS_0 + export_starting_index; 
            break;

        case SQ_EXPORT_PARAM:
            pAsm->cf_current_export_clause_ptr->m_Word0.f.array_base  = 0x0 + export_starting_index; 
            break;

        default:
            radeon_error("Unknown export type: %d\n", type);
            return GL_FALSE;
            break;
    }

    pAsm->cf_current_export_clause_ptr->m_Word0.f.rw_gpr      = starting_register_number;

    pAsm->cf_current_export_clause_ptr->m_Word0.f.rw_rel      = SQ_ABSOLUTE;
    pAsm->cf_current_export_clause_ptr->m_Word0.f.index_gpr   = 0x0;
    pAsm->cf_current_export_clause_ptr->m_Word0.f.elem_size   = 0x3; 

    pAsm->cf_current_export_clause_ptr->m_Word1.f.burst_count      = (export_count - 1);
    pAsm->cf_current_export_clause_ptr->m_Word1.f.end_of_program   = 0x0;
    pAsm->cf_current_export_clause_ptr->m_Word1.f.valid_pixel_mode = 0x0;
    pAsm->cf_current_export_clause_ptr->m_Word1.f.cf_inst          = SQ_CF_INST_EXPORT;  // _DONE
    pAsm->cf_current_export_clause_ptr->m_Word1.f.whole_quad_mode  = 0x0;
    pAsm->cf_current_export_clause_ptr->m_Word1.f.barrier          = 0x1;

    if (export_count == 1) 
    {
        ucWriteMask = pAsm->pucOutMask[starting_register_number - pAsm->starting_export_register_number];
	/* exports Z as a float into Red channel */
	if (GL_TRUE == is_depth_export)
	    ucWriteMask = 0x1;

        if( (ucWriteMask & 0x1) != 0)
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_x = SQ_SEL_X;
        }
        else
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_x = SQ_SEL_MASK;
        }
        if( ((ucWriteMask>>1) & 0x1) != 0)
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_y = SQ_SEL_Y;
        }
        else
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_y = SQ_SEL_MASK;
        }
        if( ((ucWriteMask>>2) & 0x1) != 0)
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_z = SQ_SEL_Z;
        }
        else
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_z = SQ_SEL_MASK;
        }
        if( ((ucWriteMask>>3) & 0x1) != 0)
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_w = SQ_SEL_W;
        }
        else
        {
            pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_w = SQ_SEL_MASK;
        }
    }
    else 
    {
        // This should only be used if all components for all registers have been written
        pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_x = SQ_SEL_X;
        pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_y = SQ_SEL_Y;
        pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_z = SQ_SEL_Z;
        pAsm->cf_current_export_clause_ptr->m_Word1_SWIZ.f.sel_w = SQ_SEL_W;
    }

    pAsm->cf_last_export_ptr = pAsm->cf_current_export_clause_ptr;

    return GL_TRUE;
}

GLboolean Move_Depth_Exports_To_Correct_Channels(r700_AssemblerBase *pAsm, BITS depth_channel_select)
{
	gl_inst_opcode Opcode_save = pAsm->pILInst[pAsm->uiCurInst].Opcode; //Should be OPCODE_END
    pAsm->pILInst[pAsm->uiCurInst].Opcode = OPCODE_MOV;

    // MOV depth_export_register.hw_depth_channel, depth_export_register.depth_channel_select

    pAsm->D.dst.opcode = SQ_OP2_INST_MOV;

    setaddrmode_PVSDST(&(pAsm->D.dst), ADDR_ABSOLUTE);
    pAsm->D.dst.rtype = DST_REG_TEMPORARY;
    pAsm->D.dst.reg   = pAsm->depth_export_register_number;

    pAsm->D.dst.writex = 1;   // depth          goes in R channel for HW                       

    setaddrmode_PVSSRC(&(pAsm->S[0].src), ADDR_ABSOLUTE);
    pAsm->S[0].src.rtype = DST_REG_TEMPORARY;
    pAsm->S[0].src.reg   = pAsm->depth_export_register_number;

    setswizzle_PVSSRC(&(pAsm->S[0].src), depth_channel_select);

    noneg_PVSSRC(&(pAsm->S[0].src));

    if( GL_FALSE == next_ins(pAsm) )
    {
        return GL_FALSE;
    }

    pAsm->pILInst[pAsm->uiCurInst].Opcode = Opcode_save;

    return GL_TRUE;
}
 
GLboolean Process_Fragment_Exports(r700_AssemblerBase *pR700AsmCode,
                                   GLbitfield          OutputsWritten)  
{ 
    unsigned int unBit;
    GLuint export_count = 0;

    if(pR700AsmCode->depth_export_register_number >= 0) 
    {
        if( GL_FALSE == Move_Depth_Exports_To_Correct_Channels(pR700AsmCode, SQ_SEL_Z) )  // depth
		{
			return GL_FALSE;
		}
    }

    unBit = 1 << FRAG_RESULT_COLOR;
	if(OutputsWritten & unBit)
	{
		if( GL_FALSE == Process_Export(pR700AsmCode,
                                       SQ_EXPORT_PIXEL, 
                                       0, 
                                       1, 
                                       pR700AsmCode->uiFP_OutputMap[FRAG_RESULT_COLOR], 
                                       GL_FALSE) ) 
        {
            return GL_FALSE;
        }
        export_count++;
	}
	unBit = 1 << FRAG_RESULT_DEPTH;
	if(OutputsWritten & unBit)
	{
        if( GL_FALSE == Process_Export(pR700AsmCode,
                                       SQ_EXPORT_PIXEL, 
                                       0, 
                                       1, 
                                       pR700AsmCode->uiFP_OutputMap[FRAG_RESULT_DEPTH], 
                                       GL_TRUE)) 
        {
            return GL_FALSE;
        }
        export_count++;
	}
    /* Need to export something, otherwise we'll hang
     * results are undefined anyway */
    if(export_count == 0)
    {
        Process_Export(pR700AsmCode, SQ_EXPORT_PIXEL, 0, 1, 0, GL_FALSE);
    }
    
    if(pR700AsmCode->cf_last_export_ptr != NULL) 
    {
        pR700AsmCode->cf_last_export_ptr->m_Word1.f.cf_inst        = SQ_CF_INST_EXPORT_DONE;
        pR700AsmCode->cf_last_export_ptr->m_Word1.f.end_of_program = 0x1;
    }

    return GL_TRUE;
}

GLboolean Process_Vertex_Exports(r700_AssemblerBase *pR700AsmCode,
                                 GLbitfield          OutputsWritten)  
{
    unsigned int unBit;
    unsigned int i;

    GLuint export_starting_index  = 0;
    GLuint export_count           = pR700AsmCode->number_of_exports;

    unBit = 1 << VERT_RESULT_HPOS;
	if(OutputsWritten & unBit)
	{
        if( GL_FALSE == Process_Export(pR700AsmCode, 
                                       SQ_EXPORT_POS, 
                                       export_starting_index, 
                                       1, 
                                       pR700AsmCode->ucVP_OutputMap[VERT_RESULT_HPOS],
                                       GL_FALSE) )
        {
            return GL_FALSE;
        }

        export_count--;

        pR700AsmCode->cf_last_export_ptr->m_Word1.f.cf_inst = SQ_CF_INST_EXPORT_DONE;
	}

    pR700AsmCode->number_of_exports = export_count;

	unBit = 1 << VERT_RESULT_COL0;
	if(OutputsWritten & unBit)
	{
        if( GL_FALSE == Process_Export(pR700AsmCode, 
                                       SQ_EXPORT_PARAM, 
                                       export_starting_index, 
                                       1, 
                                       pR700AsmCode->ucVP_OutputMap[VERT_RESULT_COL0],
                                       GL_FALSE) )
        {
            return GL_FALSE;
        }

        export_starting_index++;
	}

	unBit = 1 << VERT_RESULT_COL1;
	if(OutputsWritten & unBit)
	{
        if( GL_FALSE == Process_Export(pR700AsmCode, 
                                       SQ_EXPORT_PARAM, 
                                       export_starting_index, 
                                       1, 
                                       pR700AsmCode->ucVP_OutputMap[VERT_RESULT_COL1],
                                       GL_FALSE) )
        {
            return GL_FALSE;
        }

        export_starting_index++;
	}

        unBit = 1 << VERT_RESULT_FOGC;
        if(OutputsWritten & unBit)
        {
        if( GL_FALSE == Process_Export(pR700AsmCode,
                                       SQ_EXPORT_PARAM,
                                       export_starting_index,
                                       1,
                                       pR700AsmCode->ucVP_OutputMap[VERT_RESULT_FOGC],
                                       GL_FALSE) )
        {
            return GL_FALSE;
        }

        export_starting_index++;
        }

	for(i=0; i<8; i++)
	{
		unBit = 1 << (VERT_RESULT_TEX0 + i);
		if(OutputsWritten & unBit)
		{
            if( GL_FALSE == Process_Export(pR700AsmCode,
                                          SQ_EXPORT_PARAM, 
                                          export_starting_index, 
                                          1, 
                                          pR700AsmCode->ucVP_OutputMap[VERT_RESULT_TEX0 + i],
                                          GL_FALSE) )
            {
                return GL_FALSE;
            }

            export_starting_index++;
		}
	}
    
    for(i=VERT_RESULT_VAR0; i<VERT_RESULT_MAX; i++)
	{
        unBit = 1 << i;
        if(OutputsWritten & unBit)
		{
            if( GL_FALSE == Process_Export(pR700AsmCode,
                                          SQ_EXPORT_PARAM, 
                                          export_starting_index, 
                                          1, 
                                          pR700AsmCode->ucVP_OutputMap[i],
                                          GL_FALSE) )
            {                
                return GL_FALSE;
            }

            export_starting_index++;
		}
    }

    // At least one param should be exported
    if (export_count) 
    {
        pR700AsmCode->cf_last_export_ptr->m_Word1.f.cf_inst = SQ_CF_INST_EXPORT_DONE;    
    }
    else
    {
        if( GL_FALSE == Process_Export(pR700AsmCode,
                                       SQ_EXPORT_PARAM, 
                                       0, 
                                       1, 
                                       pR700AsmCode->starting_export_register_number,
                                       GL_FALSE) )
        {
            return GL_FALSE;
        }
      
        pR700AsmCode->cf_last_export_ptr->m_Word1_SWIZ.f.sel_x = SQ_SEL_0;
        pR700AsmCode->cf_last_export_ptr->m_Word1_SWIZ.f.sel_y = SQ_SEL_0;
        pR700AsmCode->cf_last_export_ptr->m_Word1_SWIZ.f.sel_z = SQ_SEL_0;
        pR700AsmCode->cf_last_export_ptr->m_Word1_SWIZ.f.sel_w = SQ_SEL_1;
        pR700AsmCode->cf_last_export_ptr->m_Word1.f.cf_inst = SQ_CF_INST_EXPORT_DONE;
    }

    pR700AsmCode->cf_last_export_ptr->m_Word1.f.end_of_program = 0x1;

    return GL_TRUE;
}

GLboolean Clean_Up_Assembler(r700_AssemblerBase *pR700AsmCode)
{
    FREE(pR700AsmCode->pucOutMask);
    FREE(pR700AsmCode->pInstDeps);

    if(NULL != pR700AsmCode->subs)
    {
        FREE(pR700AsmCode->subs);
    }
    if(NULL != pR700AsmCode->callers)
    {
        FREE(pR700AsmCode->callers);
    }

    if(NULL != pR700AsmCode->presubs)
    {
        FREE(pR700AsmCode->presubs);
    }

    return GL_TRUE;
}

