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


#ifndef _R700_SHADERINST_H_
#define _R700_SHADERINST_H_

#include "main/glheader.h"

#include "defaultendian.h" 
#include "sq_micro_reg.h"

#define SQ_ALU_CONSTANT_PS_OFFSET      0x00000000
#define SQ_ALU_CONSTANT_PS_COUNT       0x00000100
#define SQ_ALU_CONSTANT_VS_OFFSET      0x00000100
#define SQ_ALU_CONSTANT_VS_COUNT       0x00000100
#define SQ_FETCH_RESOURCE_PS_OFFSET    0x00000000
#define SQ_FETCH_RESOURCE_PS_COUNT     0x000000a0
#define SQ_FETCH_RESOURCE_VS_OFFSET    0x000000a0
#define SQ_FETCH_RESOURCE_VS_COUNT     0x000000b0

//richard dec.10 glsl
#define SQ_TEX_SAMPLER_PS_OFFSET       0x00000000
#define SQ_TEX_SAMPLER_PS_COUNT        0x00000012
#define SQ_TEX_SAMPLER_VS_OFFSET       0x00000012
#define SQ_TEX_SAMPLER_VS_COUNT        0x00000012
//-------------------

#define SHADERINST_TYPEMASK_CF  0x10
#define SHADERINST_TYPEMASK_ALU 0x20
#define SHADERINST_TYPEMASK_TEX 0x40
#define SHADERINST_TYPEMASK_VTX 0x80

typedef enum ShaderInstType 
{
    SIT_CF = 0x10,            /*SIZE = 0x2*/
        SIT_CF_ALL_EXP = 0x14,    /*SIZE = 0x2, MAX_INSTRUCTIONS = 0x10;*/
            SIT_CF_ALL_EXP_SX = 0x15, /*SIZE = 0x2, MAX_INSTRUCTIONS = 0x10;*/
            SIT_CF_ALL_EXP_SMX= 0x16, /*SIZE = 0x2, MAX_INSTRUCTIONS = 0x10;*/
        SIT_CF_GENERIC = 0x18,    /*SIZE = 0x2, MAX_INSTRUCTIONS = 0x8;  //For tex and vtx*/
        SIT_CF_ALU = 0x19,        /*SIZE = 0x2, MAX_INSTRUCTIONS = 0x80;*/
    SIT_ALU = 0x20,           /*SIZE = 0x2,*/
        SIT_ALU_HALF_LIT = 0x21,  /*SIZE = 0x4,*/
        SIT_ALU_FALL_LIT = 0x22,  /*SIZE = 0x6,*/
    SIT_TEX = 0x40,           /*SIZE = 0x4,*/
    SIT_VTX = 0x80,           /*SIZE = 0x4, MEGA_FETCH_BYTES = 0x20*/
        SIT_VTX_GENERIC = 0x81,   /*SIZE = 0x4, MEGA_FETCH_BYTES = 0x20*/
        SIT_VTX_SEM = 0x82       /*SIZE = 0x4, MEGA_FETCH_BYTES = 0x20*/
} ShaderInstType;

typedef struct R700ShaderInstruction 
{
    ShaderInstType m_ShaderInstType;
    struct R700ShaderInstruction *pNextInst;
    GLuint m_uIndex;
    GLuint useCount;
} R700ShaderInstruction;

// ------------------ CF insts ---------------------------

typedef R700ShaderInstruction R700ControlFlowInstruction;

typedef struct R700ControlFlowAllocExportClause  
{
    ShaderInstType          m_ShaderInstType;
    R700ShaderInstruction * pNextInst;    
    GLuint m_uIndex;
    GLuint useCount;
		
    sq_cf_alloc_export_word0_u      m_Word0;
    sq_cf_alloc_export_word1_u      m_Word1;
} R700ControlFlowAllocExportClause;

typedef struct R700ControlFlowSXClause 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ControlFlowAllocExportClause
		//R700ControlFlowInstruction 
			//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
			//---------------------
		//---------------------------
    sq_cf_alloc_export_word0_u      m_Word0;
    sq_cf_alloc_export_word1_u      m_Word1;
	//-------------------------------------

    sq_cf_alloc_export_word1_swiz_u m_Word1_SWIZ;
} R700ControlFlowSXClause;

typedef struct R700ControlFlowSMXClause 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
    //R700ControlFlowAllocExportClause
		//R700ControlFlowInstruction 
			//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
			//---------------------
		//---------------------------
    sq_cf_alloc_export_word0_u      m_Word0;
    sq_cf_alloc_export_word1_u      m_Word1;
	//-------------------------------

    sq_cf_alloc_export_word1_buf_u m_Word1_BUF;
} R700ControlFlowSMXClause;

typedef struct R700ControlFlowGenericClause 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ControlFlowInstruction
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------
	//---------------------

    sq_cf_word0_u m_Word0;
    sq_cf_word1_u m_Word1;

    struct R700VertexInstruction  *m_pLinkedVTXInstruction;
    struct R700TextureInstruction *m_pLinkedTEXInstruction;
} R700ControlFlowGenericClause;

typedef struct R700ControlFlowALUClause 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
    //R700ControlFlowInstruction
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------
	//---------------------

    sq_cf_alu_word0_u m_Word0;
    sq_cf_alu_word1_u m_Word1;
    
    struct R700ALUInstruction *m_pLinkedALUInstruction;
} R700ControlFlowALUClause;

// ------------------- End of CF Inst ------------------------

// ------------------- ALU Inst ------------------------------
typedef struct R700ALUInstruction 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
	//---------------------

    sq_alu_word0_u     m_Word0;
    sq_alu_word1_u     m_Word1;
    sq_alu_word1_op2_v2_u m_Word1_OP2;
    sq_alu_word1_op3_u m_Word1_OP3;

    struct R700ControlFlowALUClause *m_pLinkedALUClause;
} R700ALUInstruction;

typedef struct R700ALUInstructionHalfLiteral
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ALUInstruction 
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------

    sq_alu_word0_u     m_Word0;
    sq_alu_word1_u     m_Word1;
    sq_alu_word1_op2_v2_u m_Word1_OP2;
    sq_alu_word1_op3_u m_Word1_OP3;

    struct R700ControlFlowALUClause *m_pLinkedALUClause;
	//-------------------

    GLfloat m_fLiteralX,
            m_fLiteralY;
} R700ALUInstructionHalfLiteral;

typedef struct R700ALUInstructionFullLiteral 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ALUInstruction 
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------

    sq_alu_word0_u     m_Word0;
    sq_alu_word1_u     m_Word1;
    sq_alu_word1_op2_v2_u m_Word1_OP2;
    sq_alu_word1_op3_u m_Word1_OP3;

    struct R700ControlFlowALUClause *m_pLinkedALUClause;
	//-------------------

    GLfloat m_fLiteralX,
            m_fLiteralY,
            m_fLiteralZ,
            m_fLiteralW;
} R700ALUInstructionFullLiteral;
// ------------------- End of ALU Inst -----------------------

// ------------------- Textuer/Vertex Instruction --------------------

typedef struct R700TextureInstruction 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
	//---------------------
	
    sq_tex_word0_u m_Word0;
    sq_tex_word1_u m_Word1;
    sq_tex_word2_u m_Word2;

    struct R700ControlFlowGenericClause *m_pLinkedGenericClause;
} R700TextureInstruction;

typedef struct R700VertexInstruction 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
	//---------------------
	
    sq_vtx_word0_u     m_Word0;
    sq_vtx_word1_u     m_Word1;
    sq_vtx_word2_u     m_Word2;

    struct R700ControlFlowGenericClause *m_pLinkedGenericClause;
} R700VertexInstruction;
//
typedef struct R700VertexSemanticFetch 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700VertexInstruction
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------
	
    sq_vtx_word0_u     m_Word0;
    sq_vtx_word1_u     m_Word1;
    sq_vtx_word2_u     m_Word2;

    struct R700ControlFlowGenericClause *m_pLinkedGenericClause;
	//---------------------------

    sq_vtx_word1_sem_u m_Word1_SEM;
} R700VertexSemanticFetch;
//
typedef struct R700VertexGenericFetch 
{
	ShaderInstType          m_ShaderInstType;
	R700ShaderInstruction * pNextInst;
	//R700VertexInstruction
		//R700ShaderInstruction
	GLuint m_uIndex;
    GLuint useCount;
		//---------------------
	
    sq_vtx_word0_u     m_Word0;
    sq_vtx_word1_u     m_Word1;
    sq_vtx_word2_u     m_Word2;

    struct R700ControlFlowGenericClause *m_pLinkedGenericClause;
	//---------------------------

    sq_vtx_word1_gpr_u m_Word1_GPR;
} R700VertexGenericFetch;

// ------------------- End of Texture Vertex Instruction --------------------

void Init_R700ControlFlowGenericClause(R700ControlFlowGenericClause* pInst);
void Init_R700ControlFlowALUClause(R700ControlFlowALUClause* pInst);
void Init_R700ControlFlowSXClause(R700ControlFlowSXClause* pInst);
void Init_R700ControlFlowSMXClause(R700ControlFlowSMXClause* pInst);
void Init_R700ALUInstruction(R700ALUInstruction* pInst);
void Init_R700ALUInstructionHalfLiteral(R700ALUInstructionHalfLiteral* pInst, GLfloat x, GLfloat y);
void Init_R700ALUInstructionFullLiteral(R700ALUInstructionFullLiteral* pInst, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void Init_R700TextureInstruction(R700TextureInstruction* pInst);
void Init_R700VertexSemanticFetch(R700VertexSemanticFetch* pInst);
void Init_R700VertexGenericFetch(R700VertexGenericFetch* pInst);

unsigned int GetInstructionSize(ShaderInstType instType);
unsigned int GetCFMaxInstructions(ShaderInstType instType);

GLboolean LinkVertexInstruction(R700ControlFlowGenericClause *pCFGeneric,
								R700VertexInstruction *pVTXInstruction);

#endif //_R700_SHADERINST_H_
