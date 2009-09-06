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


#include "main/mtypes.h"

#include "radeon_debug.h"
#include "r700_shaderinst.h"

void Init_R700ControlFlowGenericClause(R700ControlFlowGenericClause* pInst)
{
    pInst->m_Word0.val = 0x00000000;
    pInst->m_Word1.val = 0x00000000;

    pInst->m_pLinkedVTXInstruction = 0;
    pInst->m_pLinkedTEXInstruction = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_CF_GENERIC;
}

void Init_R700ControlFlowALUClause(R700ControlFlowALUClause* pInst)
{
    pInst->m_Word0.val = 0x00000000;
    pInst->m_Word1.val = 0x00000000;

    pInst->m_pLinkedALUInstruction = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_CF_ALU;
}

void Init_R700ControlFlowSXClause(R700ControlFlowSXClause* pInst)
{
    pInst->m_Word0.val      = 0x00000000;
    pInst->m_Word1.val      = 0x00000000;
    pInst->m_Word1_SWIZ.val = 0x00000000;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_CF_ALL_EXP_SX;
}

void Init_R700ControlFlowSMXClause(R700ControlFlowSMXClause* pInst)
{
    pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_BUF.val = 0x00000000;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_CF_ALL_EXP_SMX;
}

void Init_R700ALUInstruction(R700ALUInstruction* pInst)
{
    pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_OP2.val = 0x00000000;
    pInst->m_Word1_OP3.val = 0x00000000;

    pInst->m_pLinkedALUClause = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_ALU;
}

void Init_R700ALUInstructionHalfLiteral(R700ALUInstructionHalfLiteral* pInst, GLfloat x, GLfloat y)
{
	pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_OP2.val = 0x00000000;
    pInst->m_Word1_OP3.val = 0x00000000;

	pInst->m_pLinkedALUClause = 0;

    pInst->m_fLiteralX = x;
    pInst->m_fLiteralY = y;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_ALU_HALF_LIT;
}

void Init_R700ALUInstructionFullLiteral(R700ALUInstructionFullLiteral* pInst, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_OP2.val = 0x00000000;
    pInst->m_Word1_OP3.val = 0x00000000;

	pInst->m_pLinkedALUClause = 0;

    pInst->m_fLiteralX = x;
    pInst->m_fLiteralY = y;
    pInst->m_fLiteralZ = z;
    pInst->m_fLiteralW = w;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_ALU_FALL_LIT;
}

void Init_R700TextureInstruction(R700TextureInstruction* pInst)
{
    pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word2.val     = 0x00000000;

    pInst->m_pLinkedGenericClause = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_TEX;
}

void Init_R700VertexSemanticFetch(R700VertexSemanticFetch* pInst)
{
    pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_SEM.val = 0x00000000;
    pInst->m_Word2.val     = 0x00000000;

    pInst->m_pLinkedGenericClause = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_VTX_SEM;
}

void Init_R700VertexGenericFetch(R700VertexGenericFetch* pInst)
{
    pInst->m_Word0.val     = 0x00000000;
    pInst->m_Word1.val     = 0x00000000;
    pInst->m_Word1_GPR.val = 0x00000000;
    pInst->m_Word2.val     = 0x00000000;

    pInst->m_pLinkedGenericClause = 0;

    pInst->useCount = 0;

	pInst->m_ShaderInstType = SIT_VTX_GENERIC;
}

unsigned int GetInstructionSize(ShaderInstType instType)
{
    switch(instType)
    {
    case SIT_ALU_HALF_LIT:  
    case SIT_TEX:           
    case SIT_VTX:           
    case SIT_VTX_GENERIC:   
    case SIT_VTX_SEM:       
        return 4;
    case SIT_ALU_FALL_LIT:
        return 6;
    default:
        break;
    }

    return 2;
}

unsigned int GetCFMaxInstructions(ShaderInstType instType)
{
    switch (instType)
    {
    case SIT_CF_ALL_EXP:    
    case SIT_CF_ALL_EXP_SX: 
    case SIT_CF_ALL_EXP_SMX:  
        return 0x10;
    case SIT_CF_GENERIC:
        return 0x8;  //For tex and vtx
    case SIT_CF_ALU:
        return 0x80;
    default:
        break;
    }
    return 0x10;
}

GLboolean LinkVertexInstruction(R700ControlFlowGenericClause *pCFGeneric,
								R700VertexInstruction *pVTXInstruction)
{
    if (pCFGeneric->m_pLinkedTEXInstruction != 0)
    {
	radeon_error("This instruction is already linked to a texture instruction.\n");
	return GL_FALSE;
    }

    pCFGeneric->m_pLinkedVTXInstruction     = pVTXInstruction;
    pVTXInstruction->m_pLinkedGenericClause = pCFGeneric;

    return GL_TRUE;
}



