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

#ifndef _R700_ASSEMBLER_H_
#define _R700_ASSEMBLER_H_

#include "main/mtypes.h"
#include "shader/prog_instruction.h"

#include "r700_chip.h"
#include "r700_shaderinst.h"
#include "r700_shader.h"

typedef enum LOADABLE_SCRIPT_SIGNITURE
{
    GLSL_NOISE1 = 0x10000001,
    GLSL_NOISE2 = 0x10000002,
    GLSL_NOISE3 = 0x10000003,
    GLSL_NOISE4 = 0x10000004
}LOADABLE_SCRIPT_SIGNITURE;

typedef struct COMPILED_SUB
{
    struct  prog_instruction *Instructions;
    GLuint  NumInstructions;
    GLuint  NumTemporaries;
    GLuint  NumParameters;
    GLuint  MinRegIndex;
    GLfloat (*ParameterValues)[4];
    GLbyte  outputSwizzleX; 
    GLbyte  outputSwizzleY;
    GLbyte  outputSwizzleZ;
    GLbyte  outputSwizzleW;
    GLshort srcRegIndex[3];
    GLushort dstRegIndex;
}COMPILED_SUB;

typedef struct PRESUB_DESCtag 
{
    LOADABLE_SCRIPT_SIGNITURE sptSigniture;
    GLint  subIL_Shift;
    struct prog_src_register InReg[3];
    struct prog_dst_register OutReg;

    GLushort maxStartReg;
    GLushort number_used_registers;

    GLuint   unConstantsStart;

    COMPILED_SUB * pCompiledSub;
} PRESUB_DESC;

typedef enum SHADER_PIPE_TYPE 
{
    SPT_VP = 0,
    SPT_FP = 1
} SHADER_PIPE_TYPE;

typedef enum ConstantCycles 
{
    NUMBER_OF_CYCLES     = 3,
    NUMBER_OF_COMPONENTS = 4
} ConstantCycles;

typedef enum  HARDWARE_LIMIT_VALUES  
{
   TEMPORARY_REGISTER_OFFSET = SQ_ALU_SRC_GPR_BASE,
   MAX_TEMPORARY_REGISTERS   = SQ_ALU_SRC_GPR_SIZE,
   MAX_CONSTANT_REGISTERS    = SQ_ALU_SRC_CFILE_SIZE,
   CFILE_REGISTER_OFFSET     = SQ_ALU_SRC_CFILE_BASE,
   NUMBER_OF_INPUT_COLORS    = 2,
   NUMBER_OF_OUTPUT_COLORS   = 8,
   NUMBER_OF_TEXTURE_UNITS   = 16,
   MEGA_FETCH_BYTES          = 32
} HARDWARE_LIMIT_VALUES;

typedef enum AddressMode 
{
    ADDR_ABSOLUTE          = 0,
    ADDR_RELATIVE_A0       = 1,
    ADDR_RELATIVE_FLI_0    = 2,
    NUMBER_OF_ADDR_MOD     = 3
} AddressMode;

typedef enum SrcRegisterType 
{
    SRC_REG_TEMPORARY      = 0,
    SRC_REG_INPUT          = 1,
    SRC_REG_CONSTANT       = 2,
    SRC_REG_ALT_TEMPORARY  = 3,
    SRC_REC_LITERAL        = 4, 
    NUMBER_OF_SRC_REG_TYPE = 5
} SrcRegisterType;

typedef enum DstRegisterType 
{
    DST_REG_TEMPORARY      = 0,
    DST_REG_A0             = 1,
    DST_REG_OUT            = 2,
    DST_REG_OUT_X_REPL     = 3,
    DST_REG_ALT_TEMPORARY  = 4,
    DST_REG_INPUT          = 5,
    NUMBER_OF_DST_REG_TYPE = 6
} DstRegisterType;

typedef unsigned int BITS;

typedef struct PVSDSTtag 
{
	BITS opcode:8;     //(:6)  //@@@ really should be 10 bits for OP2
	BITS math:1;
	BITS predicated:1; //10   //8
	BITS pred_inv  :1; //11   //8

	BITS rtype:3;
	BITS reg:10;       //24   //20

	BITS writex:1;
	BITS writey:1;
	BITS writez:1;
	BITS writew:1;     //28

	BITS op3:1;       // 29  Represents *_OP3_* ALU opcode

	BITS dualop:1;    // 30  //26

	BITS addrmode0:1; //31   //29
	BITS addrmode1:1; //32
} PVSDST;

typedef struct PVSINSTtag
{
    BITS literal_slots      :2; 
    BITS SaturateMode :2; 
    BITS index_mode   :3;
} PVSINST;

typedef struct PVSSRCtag 
{
	BITS rtype:3;            
	BITS addrmode0:1;        
	BITS reg:10;      //14     (8)
	BITS swizzlex:3;
	BITS swizzley:3;
	BITS swizzlez:3;
	BITS swizzlew:3;  //26        

	BITS abs:1;
	BITS negx:1;
	BITS negy:1;
	BITS negz:1;
	BITS negw:1;      //31
	//BITS addrsel:2;
	BITS addrmode1:1; //32
} PVSSRC;

typedef struct PVSMATHtag 
{
	BITS rtype:4;
	BITS spare:1;
	BITS reg:8;
	BITS swizzlex:3;
	BITS swizzley:3;
	BITS dstoff:2; // 2 bits of dest offset into alt ram
	BITS opcode:4;
	BITS negx:1;
	BITS negy:1;
	BITS dstcomp:2; // select dest component
	BITS spare2:3;
} PVSMATH;

typedef union PVSDWORDtag 
{
	BITS    bits;
	PVSDST  dst;
    PVSINST dst2;
	PVSSRC  src;
	PVSMATH math;
	float   f;
} PVSDWORD;

typedef struct VAP_OUT_VTX_FMT_0tag 
{
	BITS pos:1;      // 0
	BITS misc:1;
	BITS clip_dist0:1;
	BITS clip_dist1:1;
	BITS pos_param:1; // 4

	BITS color0:1;    // 5
	BITS color1:1;
	BITS color2:1;
	BITS color3:1;
	BITS color4:1;
	BITS color5:1;
	BITS color6:1;
	BITS color7:1;

	BITS normal:1;    

	BITS depth:1;          // 14

	BITS point_size:1;     // 15   
	BITS edge_flag:1;      
	BITS rta_index:1;      //     shares same channel as kill_flag
	BITS kill_flag:1;
	BITS viewport_index:1; // 19   

	BITS resvd1:12;        // 20
} VAP_OUT_VTX_FMT_0;

typedef struct VAP_OUT_VTX_FMT_1tag 
{
	BITS tex0comp:3;
	BITS tex1comp:3;
	BITS tex2comp:3;
	BITS tex3comp:3;
	BITS tex4comp:3;
	BITS tex5comp:3;
	BITS tex6comp:3;
	BITS tex7comp:3;

	BITS resvd:8;
} VAP_OUT_VTX_FMT_1;

typedef struct VAP_OUT_VTX_FMT_2tag 
{
	BITS tex8comp :3;
	BITS tex9comp :3;
	BITS tex10comp:3;
	BITS tex11comp:3;
	BITS tex12comp:3;
	BITS tex13comp:3;
	BITS tex14comp:3;
	BITS tex15comp:3;

	BITS resvd:8;
} VAP_OUT_VTX_FMT_2;

typedef struct OUT_FRAGMENT_FMT_0tag 
{
	BITS color0:1;
	BITS color1:1;
	BITS color2:1;
	BITS color3:1;
	BITS color4:1;
	BITS color5:1;
	BITS color6:1;
	BITS color7:1;

	BITS depth:1;
	BITS stencil_ref:1;
	BITS coverage_to_mask:1;
	BITS mask:1;

	BITS resvd1:20;
} OUT_FRAGMENT_FMT_0;

typedef enum  CF_CLAUSE_TYPE 
{
   CF_EXPORT_CLAUSE,
   CF_ALU_CLAUSE,
   CF_TEX_CLAUSE,
   CF_VTX_CLAUSE,
   CF_OTHER_CLAUSE,
   CF_EMPTY_CLAUSE,
   NUMBER_CF_CLAUSE_TYPES
} CF_CLAUSE_TYPE;

enum 
{
    MAX_BOOL_CONSTANTS   = 32,
    MAX_INT_CONSTANTS    = 32,
    MAX_FLOAT_CONSTANTS  = 256,

    FC_NONE = 0,
    FC_IF = 1,
    FC_LOOP = 2,
    FC_REP = 3,
    FC_PUSH_VPM = 4,
    FC_PUSH_WQM = 5,

    COND_NONE = 0,
    COND_BOOL = 1,
    COND_PRED = 2,
    COND_ALU = 3,

    SAFEDIST_TEX = 6, ///< safe distance for using result of texture lookup in alu or another tex lookup
    SAFEDIST_ALU = 6 ///< the same for alu->fc
};

typedef struct FC_LEVEL 
{
    R700ControlFlowGenericClause *  first;
    R700ControlFlowGenericClause ** mid;
    unsigned int unNumMid;
    unsigned int midLen;
    unsigned int type;
    unsigned int cond;
    unsigned int inv;
    int id; ///< id of bool or int variable
} FC_LEVEL;

typedef struct VTX_FETCH_METHOD 
{
    GLboolean bEnableMini;
    GLuint mega_fetch_remainder;
} VTX_FETCH_METHOD;

typedef struct SUB_OFFSET
{
    GLint  subIL_Offset;
    GLuint unCFoffset;
    GLuint unStackDepthMax;
    PRESUB_DESC *   pPresubDesc;
    TypedShaderList lstCFInstructions_local;
} SUB_OFFSET;

typedef struct CALLER_POINTER
{
    GLint  subIL_Offset;
    GLint  subDescIndex;
    R700ControlFlowGenericClause* cf_ptr;

    R700ControlFlowGenericClause* prelude_cf_ptr;
    R700ControlFlowGenericClause* finale_cf_ptr;
} CALLER_POINTER;

#define SQ_MAX_CALL_DEPTH 0x00000020

typedef struct CALL_LEVEL
{
    unsigned int      FCSP_BeforeEntry;
    GLint             subDescIndex;
    GLushort          current;
    GLushort          max;
    TypedShaderList * plstCFInstructions_local;
} CALL_LEVEL;

#define HAS_CURRENT_LOOPRET 0x1L
#define HAS_LOOPRET         0x2L
#define LOOPRET_FLAGS       HAS_LOOPRET | HAS_CURRENT_LOOPRET

typedef struct r700_AssemblerBase 
{
	R700ControlFlowSXClause*      cf_last_export_ptr;
	R700ControlFlowSXClause*      cf_current_export_clause_ptr;
	R700ControlFlowALUClause*     cf_current_alu_clause_ptr;
	R700ControlFlowGenericClause* cf_current_tex_clause_ptr;
	R700ControlFlowGenericClause* cf_current_vtx_clause_ptr;
	R700ControlFlowGenericClause* cf_current_cf_clause_ptr;

    //Result shader
    R700_Shader * pR700Shader;

	// No clause has been created yet
	CF_CLAUSE_TYPE cf_current_clause_type;

    BITS alu_x_opcode;

	GLuint number_of_exports;
	GLuint number_of_colorandz_exports;
	GLuint number_of_export_opcodes;

	PVSDWORD D;
    PVSDWORD D2;
	PVSDWORD S[3];
        PVSDWORD C[4];

	unsigned int uLastPosUpdate;
	unsigned int last_cond_register;

	OUT_FRAGMENT_FMT_0     fp_stOutFmt0;

	unsigned int uIIns;
	unsigned int uOIns;
	unsigned int number_used_registers;
	unsigned int uUsedConsts; 

    unsigned int flag_reg_index;

	// Fragment programs
	unsigned int uiFP_AttributeMap[FRAG_ATTRIB_MAX];
	unsigned int uiFP_OutputMap[FRAG_RESULT_MAX];
	unsigned int uBoolConsts;
	unsigned int uIntConsts;
	unsigned int uInsts;
	unsigned int uConsts;

	// Vertex programs
	unsigned char ucVP_AttributeMap[VERT_ATTRIB_MAX];
	unsigned char ucVP_OutputMap[VERT_RESULT_MAX];

    unsigned char * pucOutMask;

	//-----------------------------------------------------------------------------------
	// flow control members
	//-----------------------------------------------------------------------------------
	unsigned int FCSP;
	FC_LEVEL fc_stack[32];

	//-----------------------------------------------------------------------------------
	// ArgSubst used in Assemble_Source() function
	//-----------------------------------------------------------------------------------
	int aArgSubst[4];

    GLint hw_gpr[ NUMBER_OF_CYCLES ][ NUMBER_OF_COMPONENTS ];
    GLint hw_cfile_addr[ NUMBER_OF_COMPONENTS ];
    GLint hw_cfile_chan[ NUMBER_OF_COMPONENTS ];

    GLuint uOutputs;
  
    GLint color_export_register_number[NUMBER_OF_OUTPUT_COLORS];
	GLint depth_export_register_number;

	GLint stencil_export_register_number;
	GLint coverage_to_mask_export_register_number;
	GLint mask_export_register_number;

	GLuint starting_export_register_number;
	GLuint starting_vfetch_register_number;
	GLuint starting_temp_register_number;
	GLuint uHelpReg;
	GLuint uFirstHelpReg;

	GLboolean input_position_is_used;
	GLboolean input_normal_is_used;

	GLboolean input_color_is_used[NUMBER_OF_INPUT_COLORS];
  
	GLboolean input_texture_unit_is_used[NUMBER_OF_TEXTURE_UNITS];
  
    R700VertexGenericFetch* vfetch_instruction_ptr_array[VERT_ATTRIB_MAX];
  
	GLuint number_of_inputs;

    InstDeps *pInstDeps;

    SHADER_PIPE_TYPE currentShaderType;
    struct prog_instruction * pILInst;
    GLuint             uiCurInst;
    GLubyte SamplerUnits[MAX_SAMPLERS];
    GLboolean   bR6xx;
    /* helper to decide which type of instruction to assemble */
    GLboolean is_tex;
    /* we inserted helper intructions and need barrier on next TEX ins */ 
    GLboolean need_tex_barrier; 

    SUB_OFFSET     * subs;
    GLuint           unSubArraySize;
    GLuint           unSubArrayPointer;
    CALLER_POINTER * callers;
    GLuint           unCallerArraySize;
    GLuint           unCallerArrayPointer;
    unsigned int     CALLSP;
    CALL_LEVEL       CALLSTACK[SQ_MAX_CALL_DEPTH];

    GLuint unCFflags;

    PRESUB_DESC * presubs;
    GLuint        unPresubArraySize;
    GLuint        unNumPresub;
    GLuint        unCurNumILInsts;

    GLuint    unVetTexBits;

    GLuint    shadow_regs[R700_MAX_TEXTURE_UNITS];

} r700_AssemblerBase;

//Internal use
BITS addrmode_PVSDST(PVSDST * pPVSDST);
void setaddrmode_PVSDST(PVSDST * pPVSDST, BITS addrmode);
void nomask_PVSDST(PVSDST * pPVSDST);
BITS addrmode_PVSSRC(PVSSRC* pPVSSRC);
void setaddrmode_PVSSRC(PVSSRC* pPVSSRC, BITS addrmode);
void setswizzle_PVSSRC(PVSSRC* pPVSSRC, BITS swz);
void noswizzle_PVSSRC(PVSSRC* pPVSSRC);
void swizzleagain_PVSSRC(PVSSRC * pPVSSRC, BITS x, BITS y, BITS z, BITS w);
void neg_PVSSRC(PVSSRC* pPVSSRC);
void noneg_PVSSRC(PVSSRC* pPVSSRC);
void flipneg_PVSSRC(PVSSRC* pPVSSRC);
void zerocomp_PVSSRC(PVSSRC* pPVSSRC, int c);
void onecomp_PVSSRC(PVSSRC* pPVSSRC, int c);
BITS is_misc_component_exported(VAP_OUT_VTX_FMT_0* pOutVTXFmt0);
BITS is_depth_component_exported(OUT_FRAGMENT_FMT_0* pFPOutFmt) ;
GLboolean is_reduction_opcode(PVSDWORD * dest);
GLuint GetSurfaceFormat(GLenum eType, GLuint nChannels, GLuint * pClient_size);

unsigned int r700GetNumOperands(GLuint opcode, GLuint nIsOp3);

GLboolean IsTex(gl_inst_opcode Opcode);
GLboolean IsAlu(gl_inst_opcode Opcode);
int check_current_clause(r700_AssemblerBase* pAsm,
					     CF_CLAUSE_TYPE      new_clause_type);
GLboolean add_vfetch_instruction(r700_AssemblerBase*     pAsm,
								 R700VertexInstruction*  vertex_instruction_ptr);
GLboolean add_tex_instruction(r700_AssemblerBase*     pAsm,
                              R700TextureInstruction* tex_instruction_ptr);
GLboolean assemble_vfetch_instruction(r700_AssemblerBase* pAsm,
								GLuint gl_client_id,
                                GLuint destination_register,
								GLuint number_of_elements,
                                GLenum dataElementType,
								VTX_FETCH_METHOD* pFetchMethod);
GLboolean assemble_vfetch_instruction2(r700_AssemblerBase* pAsm,
                                       GLuint              destination_register,								       
                                       GLenum              type,
                                       GLint               size,
                                       GLubyte             element,
                                       GLuint              _signed,
                                       GLboolean           normalize,
                                       GLenum              format,
                                       VTX_FETCH_METHOD  * pFetchMethod);
GLboolean cleanup_vfetch_instructions(r700_AssemblerBase* pAsm);
GLuint gethelpr(r700_AssemblerBase* pAsm);
void resethelpr(r700_AssemblerBase* pAsm);
void checkop_init(r700_AssemblerBase* pAsm);
GLboolean mov_temp(r700_AssemblerBase* pAsm, int src);
GLboolean checkop1(r700_AssemblerBase* pAsm);
GLboolean checkop2(r700_AssemblerBase* pAsm);
GLboolean checkop3(r700_AssemblerBase* pAsm);
GLboolean assemble_src(r700_AssemblerBase *pAsm,
                       int src, 
                       int fld);
GLboolean assemble_dst(r700_AssemblerBase *pAsm);
GLboolean tex_dst(r700_AssemblerBase *pAsm);
GLboolean tex_src(r700_AssemblerBase *pAsm);
GLboolean assemble_tex_instruction(r700_AssemblerBase *pAsm, GLboolean normalized);
void initialize(r700_AssemblerBase *pAsm);
GLboolean assemble_alu_src(R700ALUInstruction*  alu_instruction_ptr,
                           int                  source_index,
                           PVSSRC*              pSource,
                           BITS                 scalar_channel_index);
GLboolean add_alu_instruction(r700_AssemblerBase* pAsm,
                              R700ALUInstruction* alu_instruction_ptr,
                              GLuint              contiguous_slots_needed);

GLboolean add_cf_instruction(r700_AssemblerBase* pAsm);
void add_return_inst(r700_AssemblerBase *pAsm);

void get_src_properties(R700ALUInstruction*  alu_instruction_ptr,
                        int                  source_index,
                        BITS*                psrc_sel,
                        BITS*                psrc_rel,
                        BITS*                psrc_chan,
                        BITS*                psrc_neg);
int is_cfile(BITS sel);
int is_const(BITS sel);
int is_gpr(BITS sel);
GLboolean reserve_cfile(r700_AssemblerBase* pAsm, 
                        GLuint sel, 
                        GLuint chan);
GLboolean reserve_gpr(r700_AssemblerBase* pAsm, GLuint sel, GLuint chan, GLuint cycle);
GLboolean cycle_for_scalar_bank_swizzle(const int swiz, const int sel, GLuint* pCycle);
GLboolean cycle_for_vector_bank_swizzle(const int swiz, const int sel, GLuint* pCycle);
GLboolean check_scalar(r700_AssemblerBase* pAsm,
                       R700ALUInstruction* alu_instruction_ptr);
GLboolean check_vector(r700_AssemblerBase* pAsm,
                       R700ALUInstruction* alu_instruction_ptr);
GLboolean assemble_alu_instruction(r700_AssemblerBase *pAsm);
GLboolean next_ins(r700_AssemblerBase *pAsm);

GLboolean pops(r700_AssemblerBase *pAsm, GLuint pops);
GLboolean jumpToOffest(r700_AssemblerBase *pAsm, GLuint pops, GLint offset);
GLboolean setRetInLoopFlag(r700_AssemblerBase *pAsm, GLuint flagValue);
GLboolean testFlag(r700_AssemblerBase *pAsm);
GLboolean breakLoopOnFlag(r700_AssemblerBase *pAsm, GLuint unFCSP);
GLboolean returnOnFlag(r700_AssemblerBase *pAsm, GLuint unIF);

GLboolean assemble_math_function(r700_AssemblerBase* pAsm, BITS opcode);
GLboolean assemble_ABS(r700_AssemblerBase *pAsm);
GLboolean assemble_ADD(r700_AssemblerBase *pAsm);
GLboolean assemble_ARL(r700_AssemblerBase *pAsm);
GLboolean assemble_BAD(char *opcode_str);
GLboolean assemble_CMP(r700_AssemblerBase *pAsm);
GLboolean assemble_DOT(r700_AssemblerBase *pAsm);
GLboolean assemble_DST(r700_AssemblerBase *pAsm);
GLboolean assemble_EX2(r700_AssemblerBase *pAsm);
GLboolean assemble_EXP(r700_AssemblerBase *pAsm);
GLboolean assemble_FLR(r700_AssemblerBase *pAsm);
GLboolean assemble_FLR_INT(r700_AssemblerBase *pAsm);
GLboolean assemble_FRC(r700_AssemblerBase *pAsm);
GLboolean assemble_KIL(r700_AssemblerBase *pAsm, GLuint opcode);
GLboolean assemble_LG2(r700_AssemblerBase *pAsm);
GLboolean assemble_LRP(r700_AssemblerBase *pAsm);
GLboolean assemble_LOG(r700_AssemblerBase *pAsm);
GLboolean assemble_MAD(r700_AssemblerBase *pAsm);
GLboolean assemble_LIT(r700_AssemblerBase *pAsm);
GLboolean assemble_MAX(r700_AssemblerBase *pAsm);
GLboolean assemble_MIN(r700_AssemblerBase *pAsm);
GLboolean assemble_MOV(r700_AssemblerBase *pAsm);
GLboolean assemble_MUL(r700_AssemblerBase *pAsm);
GLboolean assemble_POW(r700_AssemblerBase *pAsm);
GLboolean assemble_RCP(r700_AssemblerBase *pAsm);
GLboolean assemble_RSQ(r700_AssemblerBase *pAsm);
GLboolean assemble_SCS(r700_AssemblerBase *pAsm);
GLboolean assemble_SGE(r700_AssemblerBase *pAsm);
GLboolean assemble_CONT(r700_AssemblerBase *pAsm);

GLboolean assemble_LOGIC(r700_AssemblerBase *pAsm, BITS opcode);
GLboolean assemble_LOGIC_PRED(r700_AssemblerBase *pAsm, BITS opcode); 
GLboolean assemble_TRIG(r700_AssemblerBase *pAsm, BITS opcode);

GLboolean assemble_SLT(r700_AssemblerBase *pAsm);
GLboolean assemble_STP(r700_AssemblerBase *pAsm);
GLboolean assemble_TEX(r700_AssemblerBase *pAsm);
GLboolean assemble_XPD(r700_AssemblerBase *pAsm);
GLboolean assemble_EXPORT(r700_AssemblerBase *pAsm);
GLboolean assemble_IF(r700_AssemblerBase *pAsm, GLboolean bHasElse);
GLboolean assemble_ELSE(r700_AssemblerBase *pAsm);
GLboolean assemble_ENDIF(r700_AssemblerBase *pAsm);

GLboolean assemble_BGNLOOP(r700_AssemblerBase *pAsm);
GLboolean assemble_BRK(r700_AssemblerBase *pAsm);
GLboolean assemble_COND(r700_AssemblerBase *pAsm);
GLboolean assemble_ENDLOOP(r700_AssemblerBase *pAsm);

GLboolean assemble_BGNSUB(r700_AssemblerBase *pAsm, GLint nILindex, GLuint uiIL_Shift);
GLboolean assemble_ENDSUB(r700_AssemblerBase *pAsm);
GLboolean assemble_RET(r700_AssemblerBase *pAsm);
GLboolean assemble_CAL(r700_AssemblerBase *pAsm, 
                       GLint nILindex,
                       GLuint uiIL_Offest,
                       GLuint uiNumberInsts,
                       struct prog_instruction *pILInst,
                       PRESUB_DESC * pPresubDesc);

GLboolean Process_Export(r700_AssemblerBase* pAsm,
                         GLuint type, 
                         GLuint export_starting_index,
                         GLuint export_count, 
                         GLuint starting_register_number,
                         GLboolean is_depth_export);
GLboolean Move_Depth_Exports_To_Correct_Channels(r700_AssemblerBase *pAsm, 
                                                 BITS depth_channel_select);

GLboolean callPreSub(r700_AssemblerBase* pAsm, 
                     LOADABLE_SCRIPT_SIGNITURE scriptSigniture,
                     /* struct prog_instruction ** pILInstParent, */
                     COMPILED_SUB * pCompiledSub,                                            
                     GLshort uOutReg,
                     GLshort uNumValidSrc);

//Interface
GLboolean AssembleInstr(GLuint uiFirstInst,
                        GLuint uiIL_Shift,
                        GLuint uiNumberInsts,
                        struct prog_instruction *pILInst, 
						r700_AssemblerBase *pR700AsmCode);
GLboolean Process_Fragment_Exports(r700_AssemblerBase *pR700AsmCode, GLbitfield OutputsWritten);  
GLboolean Process_Vertex_Exports(r700_AssemblerBase *pR700AsmCode, GLbitfield OutputsWritten);

GLboolean RelocProgram(r700_AssemblerBase * pAsm, struct gl_program * pILProg);
GLboolean InitShaderProgram(r700_AssemblerBase * pAsm);

int       Init_r700_AssemblerBase(SHADER_PIPE_TYPE spt, r700_AssemblerBase* pAsm, R700_Shader* pShader);
GLboolean Clean_Up_Assembler(r700_AssemblerBase *pR700AsmCode);

#endif //_R700_ASSEMBLER_H_
