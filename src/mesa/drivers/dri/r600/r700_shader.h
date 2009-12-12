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


#ifndef __R700_SHADER_H__
#define __R700_SHADER_H__

#include "main/mtypes.h"

#include "r700_shaderinst.h"


void r700ShaderInit(GLcontext * ctx);

typedef enum R700ShaderType
{
    R700_SHADER_FS      = 0x0,
    R700_SHADER_ES      = 0x1,
    R700_SHADER_GS      = 0x2,
    R700_SHADER_VS      = 0x3,
    R700_SHADER_PS      = 0x4,
    R700_SHADER_INVALID = 0x5,
} R700ShaderType;

typedef struct TypedShaderList 
{
	R700ShaderInstruction * pHead;
	R700ShaderInstruction * pTail;
	GLuint  uNumOfNode;
} TypedShaderList;

typedef struct RealRegister 
{
    GLuint uAddr;
    GLuint uValue;
} RealRegister;

typedef struct InstDeps
{
    GLint nDstDep;
    GLint nSrcDeps[3];
} InstDeps;

typedef struct R700_Shader 
{
	R700ShaderType   Type;

    GLubyte*  pProgram;

    GLboolean bBinaryShader;
    GLboolean bFetchShaderRequired;
    GLboolean bNeedsAssembly;
    GLboolean bLinksDirty;

    GLuint  uShaderBinaryDWORDSize; // in DWORDS
    GLuint  nRegs;      
    GLuint  nParamExports;   // VS_ EXPORT_COUNT (1 based, the actual register is 0 based!)
    GLuint  nMemExports; 
    GLuint  resource;     // VS and PS _RESOURCE
    GLuint  exportMode;   // VS and PS _EXPORT_MODE

    GLboolean  depthIsImported;             

    // Vertex program exports
    GLboolean  positionVectorIsExported;          

    GLboolean  miscVectorIsExported;               
    GLboolean  renderTargetArrayIndexIsExported;  

    GLboolean  ccDist0VectorIsExported;  
    GLboolean  ccDist1VectorIsExported;  

    // Pixel program exports
    GLboolean  depthIsExported;             
    GLboolean  stencilRefIsExported;        
    GLboolean  coverageToMaskIsExported;    
    GLboolean  maskIsExported;              

    GLboolean  killIsUsed;                  

    GLuint  uStartAddr;
    GLuint  uCFOffset;
    GLuint  uEndOfCF;
    GLuint  uEndOfALU;
    GLuint  uEndOfFetch;
    GLuint  uStackSize;
    GLuint  uMaxCallDepth;

    TypedShaderList * plstCFInstructions_active;
	TypedShaderList lstCFInstructions;
	TypedShaderList lstALUInstructions;
	TypedShaderList lstTEXInstructions;
	TypedShaderList lstVTXInstructions;

    RealRegister RegStartAddr;
    RealRegister RegCFOffset;
    RealRegister RegEndCF;
    RealRegister RegEndALU;
    RealRegister egEndFetcg;

	// -------- constants
	GLfloat   ConstantArray[SQ_ALU_CONSTANT_PS_COUNT * 4];
	
	GLboolean bSurfAllocated;
} R700_Shader;

//Internal
void AddInstToList(TypedShaderList * plstCFInstructions, R700ShaderInstruction * pInst);
void TakeInstOutFromList(TypedShaderList * plstCFInstructions, R700ShaderInstruction * pInst);
void ResolveLinks(R700_Shader *pShader);
void Assemble(R700_Shader *pShader);

//Interface
void Init_R700_Shader(R700_Shader * pShader);
void AddCFInstruction(R700_Shader *pShader, R700ControlFlowInstruction *pCFInst);
void AddVTXInstruction(R700_Shader *pShader, R700VertexInstruction *pVTXInst);
void AddTEXInstruction(R700_Shader *pShader, R700TextureInstruction *pTEXInst);
void AddALUInstruction(R700_Shader *pShader, R700ALUInstruction *pALUInst);
void SetActiveCFlist(R700_Shader *pShader, TypedShaderList * plstCF);

void LoadProgram(R700_Shader *pShader);
void UpdateShaderRegisters(R700_Shader *pShader);
void DeleteInstructions(R700_Shader *pShader);
void DebugPrint(void);
void cleanup_vfetch_shaderinst(R700_Shader *pShader);

void Clean_Up_Shader(R700_Shader *pShader);

#endif /*__R700_SHADER_H__*/

