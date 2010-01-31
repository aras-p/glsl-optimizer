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

#include "main/glheader.h"

#include "r600_context.h"

#include "r700_shader.h"

void r700ShaderInit(GLcontext * ctx)
{
}

void AddInstToList(TypedShaderList * plstCFInstructions, R700ShaderInstruction * pInst)
{
	if(NULL == plstCFInstructions->pTail)
	{	//first
		plstCFInstructions->pHead = pInst;
		plstCFInstructions->pTail = pInst;
	}
	else
	{
		plstCFInstructions->pTail->pNextInst = pInst;
		plstCFInstructions->pTail = pInst;
	}
	pInst->pNextInst = NULL;

	plstCFInstructions->uNumOfNode++;
}

void TakeInstOutFromList(TypedShaderList * plstCFInstructions, R700ShaderInstruction * pInst)
{
    GLuint    ulIndex = 0;
    GLboolean bFound  = GL_FALSE;
    R700ShaderInstruction * pPrevInst = NULL;
    R700ShaderInstruction * pCurInst = plstCFInstructions->pHead;

    /* Need go thro list to make sure pInst is there. */
    while(NULL != pCurInst)
    {
        if(pCurInst == pInst)
        {                        
            bFound  = GL_TRUE;
            break;
        }

        pPrevInst = pCurInst;
        pCurInst  = pCurInst->pNextInst;
    }
    if(GL_TRUE == bFound)
    {
        plstCFInstructions->uNumOfNode--;

        pCurInst = pInst->pNextInst;
        ulIndex  = pInst->m_uIndex;
        while(NULL != pCurInst)
        {
            pCurInst->m_uIndex = ulIndex;
            ulIndex++;
            pCurInst = pCurInst->pNextInst;
        }

        if(plstCFInstructions->pHead == pInst)
        {
            plstCFInstructions->pHead = pInst->pNextInst;
        }
        if(plstCFInstructions->pTail == pInst)
        {
            plstCFInstructions->pTail = pPrevInst;
        }
        if(NULL != pPrevInst)
        {
            pPrevInst->pNextInst = pInst->pNextInst;
        }

        FREE(pInst);
    }
}

void Init_R700_Shader(R700_Shader * pShader)
{
	pShader->Type = R700_SHADER_INVALID;
	pShader->pProgram = NULL;
	pShader->bBinaryShader = GL_FALSE;
	pShader->bFetchShaderRequired = GL_FALSE;
	pShader->bNeedsAssembly = GL_FALSE;
	pShader->bLinksDirty = GL_TRUE;
	pShader->uShaderBinaryDWORDSize = 0;
	pShader->nRegs = 0;
	pShader->nParamExports = 0;
	pShader->nMemExports = 0;
	pShader->resource = 0;

	pShader->exportMode = 0;
	pShader->depthIsImported = GL_FALSE;

	pShader->positionVectorIsExported = GL_FALSE;
	pShader->miscVectorIsExported = GL_FALSE;
	pShader->renderTargetArrayIndexIsExported = GL_FALSE;
	pShader->ccDist0VectorIsExported = GL_FALSE;
	pShader->ccDist1VectorIsExported = GL_FALSE; 


	pShader->depthIsExported = GL_FALSE;
	pShader->stencilRefIsExported = GL_FALSE;
	pShader->coverageToMaskIsExported = GL_FALSE;
	pShader->maskIsExported = GL_FALSE;
	pShader->killIsUsed = GL_FALSE;

	pShader->uCFOffset = 0;
	pShader->uStackSize = 0;
	pShader->uMaxCallDepth = 0;

	pShader->bSurfAllocated = GL_FALSE;
	
	pShader->lstCFInstructions.pHead=NULL;  
	pShader->lstCFInstructions.pTail=NULL;  
	pShader->lstCFInstructions.uNumOfNode=0;
	pShader->lstALUInstructions.pHead=NULL; 
	pShader->lstALUInstructions.pTail=NULL; 
	pShader->lstALUInstructions.uNumOfNode=0;
	pShader->lstTEXInstructions.pHead=NULL; 
	pShader->lstTEXInstructions.pTail=NULL; 
	pShader->lstTEXInstructions.uNumOfNode=0;
	pShader->lstVTXInstructions.pHead=NULL; 
	pShader->lstVTXInstructions.pTail=NULL; 
	pShader->lstVTXInstructions.uNumOfNode=0;
}

void SetActiveCFlist(R700_Shader *pShader, TypedShaderList * plstCF)
{
    pShader->plstCFInstructions_active = plstCF;
}

void AddCFInstruction(R700_Shader *pShader, R700ControlFlowInstruction *pCFInst)
{
    R700ControlFlowSXClause*  pSXClause; 
    R700ControlFlowSMXClause* pSMXClause;

    pCFInst->m_uIndex = pShader->plstCFInstructions_active->uNumOfNode;
    AddInstToList(pShader->plstCFInstructions_active, 
                  (R700ShaderInstruction*)pCFInst);
    pShader->uShaderBinaryDWORDSize += GetInstructionSize(pCFInst->m_ShaderInstType);

    pSXClause = NULL;
    pSMXClause = NULL; 
	switch (pCFInst->m_ShaderInstType)
	{
	case SIT_CF_ALL_EXP_SX:
		pSXClause =  (R700ControlFlowSXClause*)pCFInst;
		break;
	case SIT_CF_ALL_EXP_SMX:
		pSMXClause = (R700ControlFlowSMXClause*)pCFInst;
		break;
	default:
		break;
	};

    if((pSXClause != NULL) && (pSXClause->m_Word0.f.type == SQ_EXPORT_PARAM))
    {
        pShader->nParamExports += pSXClause->m_Word1.f.burst_count + 1;
    }
    else if ((pSMXClause != NULL) && (pSMXClause->m_Word1.f.cf_inst == SQ_CF_INST_MEM_RING) &&
            (pSMXClause->m_Word0.f.type == SQ_EXPORT_WRITE || pSMXClause->m_Word0.f.type == SQ_EXPORT_WRITE_IND))
    {
        pShader->nMemExports += pSMXClause->m_Word1.f.burst_count + 1;
    }

    pShader->bLinksDirty    = GL_TRUE;
    pShader->bNeedsAssembly = GL_TRUE;

    pCFInst->useCount++;
}

void AddVTXInstruction(R700_Shader *pShader, R700VertexInstruction *pVTXInst)
{
    pVTXInst->m_uIndex = pShader->lstVTXInstructions.uNumOfNode;
	AddInstToList(&(pShader->lstVTXInstructions), 
                  (R700ShaderInstruction*)pVTXInst);
	pShader->uShaderBinaryDWORDSize += GetInstructionSize(pVTXInst->m_ShaderInstType);

	if(pVTXInst->m_ShaderInstType == SIT_VTX_GENERIC)
	{
		R700VertexGenericFetch* pVTXGenericClause = (R700VertexGenericFetch*)pVTXInst;	
		pShader->nRegs = (pShader->nRegs < pVTXGenericClause->m_Word1_GPR.f.dst_gpr) ? pVTXGenericClause->m_Word1_GPR.f.dst_gpr : pShader->nRegs;
	}

    pShader->bLinksDirty    = GL_TRUE;
    pShader->bNeedsAssembly = GL_TRUE;

    pVTXInst->useCount++;
}

void AddTEXInstruction(R700_Shader *pShader, R700TextureInstruction *pTEXInst)
{
    pTEXInst->m_uIndex = pShader->lstTEXInstructions.uNumOfNode;
	AddInstToList(&(pShader->lstTEXInstructions), 
                  (R700ShaderInstruction*)pTEXInst);
	pShader->uShaderBinaryDWORDSize += GetInstructionSize(pTEXInst->m_ShaderInstType);

    pShader->nRegs = (pShader->nRegs < pTEXInst->m_Word1.f.dst_gpr) ? pTEXInst->m_Word1.f.dst_gpr : pShader->nRegs;

    pShader->bLinksDirty    = GL_TRUE;
    pShader->bNeedsAssembly = GL_TRUE;

    pTEXInst->useCount++;
}

void AddALUInstruction(R700_Shader *pShader, R700ALUInstruction *pALUInst)
{
    pALUInst->m_uIndex = pShader->lstALUInstructions.uNumOfNode;
    AddInstToList(&(pShader->lstALUInstructions), 
                  (R700ShaderInstruction*)pALUInst);
    pShader->uShaderBinaryDWORDSize += GetInstructionSize(pALUInst->m_ShaderInstType);

    pShader->nRegs = (pShader->nRegs < pALUInst->m_Word1.f.dst_gpr) ? pALUInst->m_Word1.f.dst_gpr : pShader->nRegs;

    pShader->bLinksDirty    = GL_TRUE;
    pShader->bNeedsAssembly = GL_TRUE;

    pALUInst->useCount++;
}

void ResolveLinks(R700_Shader *pShader)
{
    GLuint uiSize;
    R700ShaderInstruction  *pInst;
    R700ALUInstruction     *pALUinst;
    R700TextureInstruction *pTEXinst;
    R700VertexInstruction  *pVTXinst; 

    GLuint vtxOffset;

	GLuint cfOffset = 0x0;  

    GLuint aluOffset = cfOffset + pShader->lstCFInstructions.uNumOfNode * GetInstructionSize(SIT_CF);

    GLuint texOffset = aluOffset;  // + m_lstALUInstructions.size() * R700ALUInstruction::SIZE,

    pInst = pShader->lstALUInstructions.pHead;
    while(NULL != pInst)
    {
        texOffset += GetInstructionSize(pInst->m_ShaderInstType);

        pInst = pInst->pNextInst;
    };
  
    vtxOffset = texOffset + pShader->lstTEXInstructions.uNumOfNode * GetInstructionSize(SIT_TEX);

    if ( ((pShader->lstTEXInstructions.uNumOfNode > 0) && (texOffset % 4 != 0)) || 
         ((pShader->lstVTXInstructions.uNumOfNode > 0) && (vtxOffset % 4 != 0))    )
    {
        pALUinst = (R700ALUInstruction*) CALLOC_STRUCT(R700ALUInstruction);
        Init_R700ALUInstruction(pALUinst);
        AddALUInstruction(pShader, pALUinst);
        texOffset += GetInstructionSize(SIT_ALU);
        vtxOffset += GetInstructionSize(SIT_ALU);
    }

    pInst  = pShader->lstALUInstructions.pHead;
    uiSize = 0;
    while(NULL != pInst)
    {
        pALUinst = (R700ALUInstruction*)pInst;

        if(pALUinst->m_pLinkedALUClause != NULL)
        {
            // This address is quad-word aligned
            pALUinst->m_pLinkedALUClause->m_Word0.f.addr = (aluOffset + uiSize) >> 1;
        }

        uiSize += GetInstructionSize(pALUinst->m_ShaderInstType);

        pInst = pInst->pNextInst;
    };

    pInst  = pShader->lstTEXInstructions.pHead;
    uiSize = 0;
    while(NULL != pInst)
    {
        pTEXinst = (R700TextureInstruction*)pInst;

        if (pTEXinst->m_pLinkedGenericClause != NULL)
        {
            pTEXinst->m_pLinkedGenericClause->m_Word0.f.addr = (texOffset + uiSize) >> 1;
        }

        uiSize += GetInstructionSize(pTEXinst->m_ShaderInstType);

        pInst = pInst->pNextInst;
    };

    pInst  = pShader->lstVTXInstructions.pHead;
    uiSize = 0;
    while(NULL != pInst)
    {
        pVTXinst = (R700VertexInstruction*)pInst;

        if (pVTXinst->m_pLinkedGenericClause != NULL)
        {
            pVTXinst->m_pLinkedGenericClause->m_Word0.f.addr = (vtxOffset + uiSize) >> 1;
        }

        uiSize += GetInstructionSize(pVTXinst->m_ShaderInstType);

        pInst = pInst->pNextInst;
    };

    pShader->bLinksDirty = GL_FALSE;
}

void Assemble(R700_Shader *pShader)
{
	GLuint i;
    GLuint *pShaderBinary;
    GLuint size_of_program;
    GLuint *pCurrPos;

    GLuint end_of_cf_instructions;
    GLuint number_of_alu_dwords;

    R700ShaderInstruction  *pInst;

    if(GL_TRUE == pShader->bBinaryShader)
    {
        return;
    }

    if(pShader->bLinksDirty == GL_TRUE) 
    {
        ResolveLinks(pShader);
    }

    size_of_program = pShader->uShaderBinaryDWORDSize;
    
    pShaderBinary = (GLuint*) MALLOC(sizeof(GLuint)*size_of_program);
 
    pCurrPos = pShaderBinary;

    for (i = 0; i < size_of_program; i++)
    {
        pShaderBinary[i] = 0;
    }

    pInst = pShader->lstCFInstructions.pHead;
    while(NULL != pInst)
    {
        switch (pInst->m_ShaderInstType)
        {
        case SIT_CF_GENERIC: 
            {
                R700ControlFlowGenericClause* pCFgeneric = (R700ControlFlowGenericClause*)pInst;
                *pCurrPos++ = pCFgeneric->m_Word0.val;
                *pCurrPos++ = pCFgeneric->m_Word1.val;
            }
            break;
        case SIT_CF_ALU: 
            {
                R700ControlFlowALUClause* pCFalu = (R700ControlFlowALUClause*)pInst;
                *pCurrPos++ = pCFalu->m_Word0.val;
                *pCurrPos++ = pCFalu->m_Word1.val;
            }
            break;
        case SIT_CF_ALL_EXP_SX: 
            {
                R700ControlFlowSXClause* pCFsx = (R700ControlFlowSXClause*)pInst;
                *pCurrPos++ = pCFsx->m_Word0.val;
                *pCurrPos++ = (pCFsx->m_Word1.val | pCFsx->m_Word1_SWIZ.val);
            }
            break;
        case SIT_CF_ALL_EXP_SMX: 
            {
                R700ControlFlowSMXClause* pCFsmx = (R700ControlFlowSMXClause*)pInst;
                *pCurrPos++ = pCFsmx->m_Word0.val;
                *pCurrPos++ = (pCFsmx->m_Word1.val | pCFsmx->m_Word1_BUF.val);
            }
            break;
        default:
            break;
        }

        pInst = pInst->pNextInst;
    };
    
    number_of_alu_dwords = 0;
    pInst = pShader->lstALUInstructions.pHead;
    while(NULL != pInst)
    {
        switch (pInst->m_ShaderInstType)
        {
        case SIT_ALU: 
            {
                R700ALUInstruction* pALU = (R700ALUInstruction*)pInst;

                *pCurrPos++ = pALU->m_Word0.val;
                *pCurrPos++ = (pALU->m_Word1.val | pALU->m_Word1_OP2.val | pALU->m_Word1_OP3.val);

                number_of_alu_dwords += 2;
            }
            break;
        case SIT_ALU_HALF_LIT: 
            {
                R700ALUInstructionHalfLiteral* pALUhalf = (R700ALUInstructionHalfLiteral*)pInst;

                *pCurrPos++ = pALUhalf->m_Word0.val;
                *pCurrPos++ = (pALUhalf->m_Word1.val | pALUhalf->m_Word1_OP2.val | pALUhalf->m_Word1_OP3.val);
                *pCurrPos++ = *((GLuint*)&(pALUhalf->m_fLiteralX));
                *pCurrPos++ = *((GLuint*)&(pALUhalf->m_fLiteralY));

                number_of_alu_dwords += 4;
            }
            break;
        case SIT_ALU_FALL_LIT: 
            {
                R700ALUInstructionFullLiteral* pALUfull = (R700ALUInstructionFullLiteral*)pInst;

                *pCurrPos++ = pALUfull->m_Word0.val;
                *pCurrPos++ = (pALUfull->m_Word1.val | pALUfull->m_Word1_OP2.val | pALUfull->m_Word1_OP3.val);

                *pCurrPos++ = *((GLuint*)&(pALUfull->m_fLiteralX));
                *pCurrPos++ = *((GLuint*)&(pALUfull->m_fLiteralY));
                *pCurrPos++ = *((GLuint*)&(pALUfull->m_fLiteralZ));
                *pCurrPos++ = *((GLuint*)&(pALUfull->m_fLiteralW));

                number_of_alu_dwords += 6;
            }
            break;
        default:
            break;
        }

        pInst = pInst->pNextInst;
    };
    
    pInst = pShader->lstTEXInstructions.pHead;
    while(NULL != pInst)
    {
        R700TextureInstruction* pTEX = (R700TextureInstruction*)pInst;

        *pCurrPos++ = pTEX->m_Word0.val;
        *pCurrPos++ = pTEX->m_Word1.val;
        *pCurrPos++ = pTEX->m_Word2.val;
        *pCurrPos++ = 0x0beadeaf;

        pInst = pInst->pNextInst;
    };
    
    pInst = pShader->lstVTXInstructions.pHead;
    while(NULL != pInst)
    {
        switch (pInst->m_ShaderInstType)
        {
        case SIT_VTX_SEM: //
            {
                R700VertexSemanticFetch* pVTXsem = (R700VertexSemanticFetch*)pInst;

                *pCurrPos++ = pVTXsem->m_Word0.val;
                *pCurrPos++ = (pVTXsem->m_Word1.val | pVTXsem->m_Word1_SEM.val);
                *pCurrPos++ = pVTXsem->m_Word2.val;
                *pCurrPos++ = 0x0beadeaf;
            }
            break;
        case SIT_VTX_GENERIC: //
            {
                R700VertexGenericFetch* pVTXgeneric = (R700VertexGenericFetch*)pInst;

                *pCurrPos++ = pVTXgeneric->m_Word0.val;
                *pCurrPos++ = (pVTXgeneric->m_Word1.val | pVTXgeneric->m_Word1_GPR.val);
                *pCurrPos++ = pVTXgeneric->m_Word2.val;
                *pCurrPos++ = 0x0beadeaf;
            }
            break;
        default:
            break;
        }

        pInst = pInst->pNextInst;
    };

    if(NULL != pShader->pProgram)
    {
        FREE(pShader->pProgram);
    }
    pShader->pProgram = (GLubyte*)pShaderBinary;

    end_of_cf_instructions = pShader->uCFOffset + pShader->lstCFInstructions.uNumOfNode * GetInstructionSize(SIT_CF);
    
    pShader->uEndOfCF = end_of_cf_instructions >> 1;

    pShader->uEndOfALU = (end_of_cf_instructions + number_of_alu_dwords) >> 1;

    pShader->uEndOfFetch = (pShader->uCFOffset + pShader->uShaderBinaryDWORDSize) >> 1;

    pShader->bNeedsAssembly = GL_FALSE;
}

void LoadProgram(R700_Shader *pShader) //context
{
}

void UpdateShaderRegisters(R700_Shader *pShader) //context
{
}

void DeleteInstructions(R700_Shader *pShader)
{
}

void DebugPrint(void)
{
}

void cleanup_vfetch_shaderinst(R700_Shader *pShader)
{
    R700ShaderInstruction      *pInst;
    R700ShaderInstruction      *pInstToFree;
    R700VertexInstruction      *pVTXInst;
    R700ControlFlowInstruction *pCFInst;

    pInst = pShader->lstVTXInstructions.pHead;
    while(NULL != pInst)
    {
        pVTXInst = (R700VertexInstruction  *)pInst;        
        pShader->uShaderBinaryDWORDSize -= GetInstructionSize(pVTXInst->m_ShaderInstType);

        if(NULL != pVTXInst->m_pLinkedGenericClause)
        {
            pCFInst = (R700ControlFlowInstruction*)(pVTXInst->m_pLinkedGenericClause);

            TakeInstOutFromList(&(pShader->lstCFInstructions), 
                                 (R700ShaderInstruction*)pCFInst);

            pShader->uShaderBinaryDWORDSize -= GetInstructionSize(pCFInst->m_ShaderInstType);
        }

        pInst = pInst->pNextInst;
    };

    //destroy each item in pShader->lstVTXInstructions;
    pInst = pShader->lstVTXInstructions.pHead;
    while(NULL != pInst)
    {
        pInstToFree = pInst;
        pInst = pInst->pNextInst;
        FREE(pInstToFree);
    };

    //set NULL pShader->lstVTXInstructions
    pShader->lstVTXInstructions.pHead=NULL; 
	pShader->lstVTXInstructions.pTail=NULL; 
	pShader->lstVTXInstructions.uNumOfNode=0;
}

void Clean_Up_Shader(R700_Shader *pShader)
{
    FREE(pShader->pProgram);

    R700ShaderInstruction  *pInst;
    R700ShaderInstruction  *pInstToFree;

    pInst = pShader->lstCFInstructions.pHead;
    while(NULL != pInst)
    {
        pInstToFree = pInst;
        pInst = pInst->pNextInst;
        FREE(pInstToFree);
    };
    pInst = pShader->lstALUInstructions.pHead;
    while(NULL != pInst)
    {
        pInstToFree = pInst;
        pInst = pInst->pNextInst;
        FREE(pInstToFree);
    };
    pInst = pShader->lstTEXInstructions.pHead;
    while(NULL != pInst)
    {
        pInstToFree = pInst;
        pInst = pInst->pNextInst;
        FREE(pInstToFree);
    };
    pInst = pShader->lstVTXInstructions.pHead;
    while(NULL != pInst)
    {
        pInstToFree = pInst;
        pInst = pInst->pNextInst;
        FREE(pInstToFree);
    };
}

