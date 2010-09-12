/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef TPF_H_
#define TPF_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include "le32.h"

/* This is an independent implementation of definitions and tools for the
 * "tokenized program format" (TPF) bytecode  documented in the
 * d3d11TokenizedProgramFormat.hpp header in the Windows Driver Development kit
 */

enum tpf_opcode {
	// Shader Model 4.0 (Direct3D 10.0)

	TPF_OPCODE_ADD,
	TPF_OPCODE_AND,
	TPF_OPCODE_BREAK,
	TPF_OPCODE_BREAKC,
	TPF_OPCODE_CALL,
	TPF_OPCODE_CALLC,
	TPF_OPCODE_CASE,
	TPF_OPCODE_CONTINUE,
	TPF_OPCODE_CONTINUEC,
	TPF_OPCODE_CUT,
	TPF_OPCODE_DEFAULT,
	TPF_OPCODE_DERIV_RTX,
	TPF_OPCODE_DERIV_RTY,
	TPF_OPCODE_DISCARD,
	TPF_OPCODE_DIV,
	TPF_OPCODE_DP2,
	TPF_OPCODE_DP3,
	TPF_OPCODE_DP4,
	TPF_OPCODE_ELSE,
	TPF_OPCODE_EMIT,
	TPF_OPCODE_EMITTHENCUT,
	TPF_OPCODE_ENDIF,
	TPF_OPCODE_ENDLOOP,
	TPF_OPCODE_ENDSWITCH,
	TPF_OPCODE_EQ,
	TPF_OPCODE_EXP,
	TPF_OPCODE_FRC,
	TPF_OPCODE_FTOI,
	TPF_OPCODE_FTOU,
	TPF_OPCODE_GE,
	TPF_OPCODE_IADD,
	TPF_OPCODE_IF,
	TPF_OPCODE_IEQ,
	TPF_OPCODE_IGE,
	TPF_OPCODE_ILT,
	TPF_OPCODE_IMAD,
	TPF_OPCODE_IMAX,
	TPF_OPCODE_IMIN,
	TPF_OPCODE_IMUL,
	TPF_OPCODE_INE,
	TPF_OPCODE_INEG,
	TPF_OPCODE_ISHL,
	TPF_OPCODE_ISHR,
	TPF_OPCODE_ITOF,
	TPF_OPCODE_LABEL,
	TPF_OPCODE_LD,
	TPF_OPCODE_LD_MS,
	TPF_OPCODE_LOG,
	TPF_OPCODE_LOOP,
	TPF_OPCODE_LT,
	TPF_OPCODE_MAD,
	TPF_OPCODE_MIN,
	TPF_OPCODE_MAX,
	TPF_OPCODE_CUSTOMDATA,
	TPF_OPCODE_MOV,
	TPF_OPCODE_MOVC,
	TPF_OPCODE_MUL,
	TPF_OPCODE_NE,
	TPF_OPCODE_NOP,
	TPF_OPCODE_NOT,
	TPF_OPCODE_OR,
	TPF_OPCODE_RESINFO,
	TPF_OPCODE_RET,
	TPF_OPCODE_RETC,
	TPF_OPCODE_ROUND_NE,
	TPF_OPCODE_ROUND_NI,
	TPF_OPCODE_ROUND_PI,
	TPF_OPCODE_ROUND_Z,
	TPF_OPCODE_RSQ,
	TPF_OPCODE_SAMPLE,
	TPF_OPCODE_SAMPLE_C,
	TPF_OPCODE_SAMPLE_C_LZ,
	TPF_OPCODE_SAMPLE_L,
	TPF_OPCODE_SAMPLE_D,
	TPF_OPCODE_SAMPLE_B,
	TPF_OPCODE_SQRT,
	TPF_OPCODE_SWITCH,
	TPF_OPCODE_SINCOS,
	TPF_OPCODE_UDIV,
	TPF_OPCODE_ULT,
	TPF_OPCODE_UGE,
	TPF_OPCODE_UMUL,
	TPF_OPCODE_UMAD,
	TPF_OPCODE_UMAX,
	TPF_OPCODE_UMIN,
	TPF_OPCODE_USHR,
	TPF_OPCODE_UTOF,
	TPF_OPCODE_XOR,

	// these have custom formats
	TPF_OPCODE_DCL_RESOURCE,
	TPF_OPCODE_DCL_CONSTANT_BUFFER,
	TPF_OPCODE_DCL_SAMPLER,
	TPF_OPCODE_DCL_INDEX_RANGE,
	TPF_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY,
	TPF_OPCODE_DCL_GS_INPUT_PRIMITIVE,
	TPF_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT,
	TPF_OPCODE_DCL_INPUT,
	TPF_OPCODE_DCL_INPUT_SGV,
	TPF_OPCODE_DCL_INPUT_SIV,
	TPF_OPCODE_DCL_INPUT_PS,
	TPF_OPCODE_DCL_INPUT_PS_SGV,
	TPF_OPCODE_DCL_INPUT_PS_SIV,
	TPF_OPCODE_DCL_OUTPUT,
	TPF_OPCODE_DCL_OUTPUT_SGV,
	TPF_OPCODE_DCL_OUTPUT_SIV,
	TPF_OPCODE_DCL_TEMPS,
	TPF_OPCODE_DCL_INDEXABLE_TEMP,
	TPF_OPCODE_DCL_GLOBAL_FLAGS,

	TPF_OPCODE_D3D10_COUNT, // this is really a reserved opcode...

	// Shader Model 4.1 (Direct3D 10.1)

	TPF_OPCODE_LOD,
	TPF_OPCODE_GATHER4,
	TPF_OPCODE_SAMPLE_POS,
	TPF_OPCODE_SAMPLE_INFO,

	TPF_OPCODE_D3D10_1_COUNT, // this is really a reserved opcode...

	// Shader Model 5.0 (Direct3D 11)

	// HS subshader beginning markers
	TPF_OPCODE_HS_DECLS,
	TPF_OPCODE_HS_CONTROL_POINT_PHASE,
	TPF_OPCODE_HS_FORK_PHASE,
	TPF_OPCODE_HS_JOIN_PHASE,

	TPF_OPCODE_EMIT_STREAM,
	TPF_OPCODE_CUT_STREAM,
	TPF_OPCODE_EMITTHENCUT_STREAM,
	TPF_OPCODE_INTERFACE_CALL,

	TPF_OPCODE_BUFINFO,
	TPF_OPCODE_DERIV_RTX_COARSE,
	TPF_OPCODE_DERIV_RTX_FINE,
	TPF_OPCODE_DERIV_RTY_COARSE,
	TPF_OPCODE_DERIV_RTY_FINE,
	TPF_OPCODE_GATHER4_C,
	TPF_OPCODE_GATHER4_PO,
	TPF_OPCODE_GATHER4_PO_C,
	TPF_OPCODE_RCP,
	TPF_OPCODE_F32TOF16,
	TPF_OPCODE_F16TOF32,
	TPF_OPCODE_UADDC,
	TPF_OPCODE_USUBB,
	TPF_OPCODE_COUNTBITS,
	TPF_OPCODE_FIRSTBIT_HI,
	TPF_OPCODE_FIRSTBIT_LO,
	TPF_OPCODE_FIRSTBIT_SHI,
	TPF_OPCODE_UBFE,
	TPF_OPCODE_IBFE,
	TPF_OPCODE_BFI,
	TPF_OPCODE_BFREV,
	TPF_OPCODE_SWAPC,

	// these have custom formats
	TPF_OPCODE_DCL_STREAM,
	TPF_OPCODE_DCL_FUNCTION_BODY,
	TPF_OPCODE_DCL_FUNCTION_TABLE,
	TPF_OPCODE_DCL_INTERFACE,

	// these have custom formats
	TPF_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT,
	TPF_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT,
	TPF_OPCODE_DCL_TESS_DOMAIN,
	TPF_OPCODE_DCL_TESS_PARTITIONING,
	TPF_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE,
	TPF_OPCODE_DCL_HS_MAX_TESSFACTOR,
	TPF_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT,
	TPF_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT,

	// these have custom formats
	TPF_OPCODE_DCL_THREAD_GROUP,
	TPF_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED,
	TPF_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW,
	TPF_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED,
	TPF_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW,
	TPF_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED,
	TPF_OPCODE_DCL_RESOURCE_RAW,
	TPF_OPCODE_DCL_RESOURCE_STRUCTURED,

	TPF_OPCODE_LD_UAV_TYPED,
	TPF_OPCODE_STORE_UAV_TYPED,
	TPF_OPCODE_LD_RAW,
	TPF_OPCODE_STORE_RAW,
	TPF_OPCODE_LD_STRUCTURED,
	TPF_OPCODE_STORE_STRUCTURED,

	TPF_OPCODE_ATOMIC_AND,
	TPF_OPCODE_ATOMIC_OR,
	TPF_OPCODE_ATOMIC_XOR,
	TPF_OPCODE_ATOMIC_CMP_STORE,
	TPF_OPCODE_ATOMIC_IADD,
	TPF_OPCODE_ATOMIC_IMAX,
	TPF_OPCODE_ATOMIC_IMIN,
	TPF_OPCODE_ATOMIC_UMAX,
	TPF_OPCODE_ATOMIC_UMIN,

	TPF_OPCODE_IMM_ATOMIC_ALLOC,
	TPF_OPCODE_IMM_ATOMIC_CONSUME,
	TPF_OPCODE_IMM_ATOMIC_IADD,
	TPF_OPCODE_IMM_ATOMIC_AND,
	TPF_OPCODE_IMM_ATOMIC_OR,
	TPF_OPCODE_IMM_ATOMIC_XOR,
	TPF_OPCODE_IMM_ATOMIC_EXCH,
	TPF_OPCODE_IMM_ATOMIC_CMP_EXCH,
	TPF_OPCODE_IMM_ATOMIC_IMAX,
	TPF_OPCODE_IMM_ATOMIC_IMIN,
	TPF_OPCODE_IMM_ATOMIC_UMAX,
	TPF_OPCODE_IMM_ATOMIC_UMIN,

	TPF_OPCODE_SYNC,

	TPF_OPCODE_DADD,
	TPF_OPCODE_DMAX,
	TPF_OPCODE_DMIN,
	TPF_OPCODE_DMUL,
	TPF_OPCODE_DEQ,
	TPF_OPCODE_DGE,
	TPF_OPCODE_DLT,
	TPF_OPCODE_DNE,
	TPF_OPCODE_DMOV,
	TPF_OPCODE_DMOVC,

	TPF_OPCODE_DTOF,
	TPF_OPCODE_FTOD,

	TPF_OPCODE_EVAL_SNAPPED,
	TPF_OPCODE_EVAL_SAMPLE_INDEX,
	TPF_OPCODE_EVAL_CENTROID,

	TPF_OPCODE_DCL_GS_INSTANCE_COUNT,

	TPF_OPCODE_D3D11_COUNT
};

extern const char* tpf_opcode_names[];

enum tpf_file
{
	TPF_FILE_TEMP = 0,
	TPF_FILE_INPUT = 1,
	TPF_FILE_OUTPUT = 2,
	TPF_FILE_INDEXABLE_TEMP = 3,
	TPF_FILE_IMMEDIATE32    = 4, // one 32-bit value for each component follows
	TPF_FILE_IMMEDIATE64    = 5, // one 64-bit value for each component follows
	TPF_FILE_SAMPLER = 6,
	TPF_FILE_RESOURCE = 7,
	TPF_FILE_CONSTANT_BUFFER= 8,
	TPF_FILE_IMMEDIATE_CONSTANT_BUFFER= 9,
	TPF_FILE_LABEL = 10,
	TPF_FILE_INPUT_PRIMITIVEID = 11,
	TPF_FILE_OUTPUT_DEPTH = 12,
	TPF_FILE_NULL = 13,

	// Added in D3D10.1

	TPF_FILE_RASTERIZER = 14,
	TPF_FILE_OUTPUT_COVERAGE_MASK = 15,

	// Added in D3D11

	TPF_FILE_STREAM = 16,
	TPF_FILE_FUNCTION_BODY = 17,
	TPF_FILE_FUNCTION_TABLE = 18,
	TPF_FILE_INTERFACE = 19,
	TPF_FILE_FUNCTION_INPUT = 20,
	TPF_FILE_FUNCTION_OUTPUT = 21,
	TPF_FILE_OUTPUT_CONTROL_POINT_ID = 22,
	TPF_FILE_INPUT_FORK_INSTANCE_ID = 23,
	TPF_FILE_INPUT_JOIN_INSTANCE_ID = 24,
	TPF_FILE_INPUT_CONTROL_POINT = 25,
	TPF_FILE_OUTPUT_CONTROL_POINT = 26,
	TPF_FILE_INPUT_PATCH_CONSTANT = 27,
	TPF_FILE_INPUT_DOMAIN_POINT = 28,
	TPF_FILE_THIS_POINTER = 29,
	TPF_FILE_UNORDERED_ACCESS_VIEW = 30,
	TPF_FILE_THREAD_GROUP_SHARED_MEMORY = 31,
	TPF_FILE_INPUT_THREAD_ID = 32,
	TPF_FILE_INPUT_THREAD_GROUP_ID = 33,
	TPF_FILE_INPUT_THREAD_ID_IN_GROUP = 34,
	TPF_FILE_INPUT_COVERAGE_MASK = 35,
	TPF_FILE_INPUT_THREAD_ID_IN_GROUP_FLATTENED = 36,
	TPF_FILE_INPUT_GS_INSTANCE_ID = 37,
	TPF_FILE_OUTPUT_DEPTH_GREATER_EQUAL = 38,
	TPF_FILE_OUTPUT_DEPTH_LESS_EQUAL = 39,
	TPF_FILE_CYCLE_COUNTER = 40,

	TPF_FILE_COUNT = 41,
};

extern const char* tpf_file_names[];
extern const char* tpf_file_ms_names[];

enum tpf_target
{
	TPF_TARGET_UNKNOWN = 0,
	TPF_TARGET_BUFFER = 1,
	TPF_TARGET_TEXTURE1D = 2,
	TPF_TARGET_TEXTURE2D = 3,
	TPF_TARGET_TEXTURE2DMS = 4,
	TPF_TARGET_TEXTURE3D = 5,
	TPF_TARGET_TEXTURECUBE = 6,
	TPF_TARGET_TEXTURE1DARRAY = 7,
	TPF_TARGET_TEXTURE2DARRAY = 8,
	TPF_TARGET_TEXTURE2DMSARRAY = 9,
	TPF_TARGET_TEXTURECUBEARRAY = 10,

	// Added in D3D11
	TPF_TARGET_RAW_BUFFER = 11,
	TPF_TARGET_STRUCTURED_BUFFER = 12,
};

extern const char* tpf_target_names[];

enum tpf_interpolation
{
	TPF_INTERPOLATION_UNDEFINED = 0,
	TPF_INTERPOLATION_CONSTANT = 1,
	TPF_INTERPOLATION_LINEAR = 2,
	TPF_INTERPOLATION_LINEAR_CENTROID = 3,
	TPF_INTERPOLATION_LINEAR_NOPERSPECTIVE = 4,
	TPF_INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID = 5,

	// Added in D3D10.1
	TPF_INTERPOLATION_LINEAR_SAMPLE = 6,
	TPF_INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE = 7,
};

extern const char* tpf_interpolation_names[];

enum tpf_sv
{
	TPF_SV_UNDEFINED,
	TPF_SV_POSITION,
	TPF_SV_CLIP_DISTANCE,
	TPF_SV_CULL_DISTANCE,
	TPF_SV_RENDER_TARGET_ARRAY_INDEX,
	TPF_SV_VIEWPORT_ARRAY_INDEX,
	TPF_SV_VERTEX_ID,
	TPF_SV_PRIMITIVE_ID,
	TPF_SV_INSTANCE_ID,
	TPF_SV_IS_FRONT_FACE,
	TPF_SV_SAMPLE_INDEX,

	// Added in D3D11
	TPF_SV_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR,
	TPF_SV_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR,
	TPF_SV_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR,
	TPF_SV_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR,
	TPF_SV_FINAL_QUAD_U_INSIDE_TESSFACTOR,
	TPF_SV_FINAL_QUAD_V_INSIDE_TESSFACTOR,
	TPF_SV_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR,
	TPF_SV_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR,
	TPF_SV_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR,
	TPF_SV_FINAL_TRI_INSIDE_TESSFACTOR,
	TPF_SV_FINAL_LINE_DETAIL_TESSFACTOR,
	TPF_SV_FINAL_LINE_DENSITY_TESSFACTOR,
};

extern const char* tpf_sv_names[];

struct tpf_token_version
{
	unsigned minor : 4;
	unsigned major : 4;
	unsigned format : 8;
	unsigned type : 16;
};

struct tpf_token_instruction
{
	// we don't make it an union directly because unions can't be inherited from
	union
	{
		// length and extended are always present, but they are only here to reduce duplication
		struct
		{
			unsigned opcode : 11;
			unsigned _11_23 : 13;
			unsigned length : 7;
			unsigned extended : 1;
		};
		struct
		{
			unsigned opcode : 11;
			unsigned resinfo_return_type : 2;
			unsigned sat : 1;
			unsigned _14_17 : 4;
			unsigned test_nz : 1; // bit 18
			unsigned precise_mask : 4;
			unsigned _23 : 1;
			unsigned length : 7;
			unsigned extended : 1;
		} insn;
		struct
		{
			unsigned opcode : 11;
			unsigned threads_in_group : 1;
			unsigned shared_memory : 1;
			unsigned uav_group : 1;
			unsigned uav_global : 1;
			unsigned _15_17 : 3;
		} sync;
		struct
		{
			unsigned opcode : 11;
			unsigned allow_refactoring : 1;
			unsigned fp64 : 1;
			unsigned early_depth_stencil : 1;
			unsigned enable_raw_and_structured_in_non_cs : 1;
		} dcl_global_flags;
		struct
		{
			unsigned opcode : 11;
			unsigned target : 5;
			unsigned nr_samples : 7;
		} dcl_resource;
		struct
		{
			unsigned opcode : 11;
			unsigned shadow : 1;
			unsigned mono : 1;
		} dcl_sampler;
		struct
		{
			unsigned opcode : 11;
			unsigned interpolation : 5;
		} dcl_input_ps;
		struct
		{
			unsigned opcode : 11;
			unsigned dynamic : 1;
		} dcl_constant_buffer;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 6;
		} dcl_gs_input_primitive;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive_topology : 7;
		} dcl_gs_output_primitive_topology;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_input_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_output_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned domain : 3; /* D3D_TESSELLATOR_DOMAIN */
		} dcl_tess_domain;
		struct
		{
			unsigned opcode : 11;
			unsigned partitioning : 3; /* D3D_TESSELLATOR_PARTITIONING */
		} dcl_tess_partitioning;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 3; /* D3D_TESSELLATOR_OUTPUT_PRIMITIVE */
		} dcl_tess_output_primitive;
	};
};

enum tpf_token_instruction_extended_type
{
	TPF_TOKEN_INSTRUCTION_EXTENDED_TYPE_EMPTY = 0,
	TPF_TOKEN_INSTRUCTION_EXTENDED_TYPE_SAMPLE_CONTROLS = 1,
	TPF_TOKEN_INSTRUCTION_EXTENDED_TYPE_RESOURCE_DIM = 2,
	TPF_TOKEN_INSTRUCTION_EXTENDED_TYPE_RESOURCE_RETURN_TYPE = 3,
};

union  tpf_token_instruction_extended
{
	struct
	{
		unsigned type : 6;
		unsigned _6_30 : 25;
		unsigned extended :1;
	};
	struct
	{
		unsigned type : 6;
		unsigned _6_8 : 3;
		int offset_u : 4;
		int offset_v : 4;
		int offset_w : 4;
	} sample_controls;
	struct
	{
		unsigned type : 6;
		unsigned target : 5;
	} resource_target;
	struct
	{
		unsigned type : 6;
		unsigned x : 4;
		unsigned y : 4;
		unsigned z : 4;
		unsigned w : 4;
	} resource_return_type;
};

struct tpf_token_resource_return_type
{
	unsigned x : 4;
	unsigned y : 4;
	unsigned z : 4;
	unsigned w : 4;
};

enum tpf_operand_comps
{
	TPF_OPERAND_COMPS_0 = 0,
	TPF_OPERAND_COMPS_1 = 1,
	TPF_OPERAND_COMPS_4 = 2,
	TPF_OPERAND_COMPS_N = 3
};

enum tpf_operand_mode
{
	TPF_OPERAND_MODE_MASK = 0,
	TPF_OPERAND_MODE_SWIZZLE = 1,
	TPF_OPERAND_MODE_SCALAR = 2
};

enum tpf_operand_index_repr
{
	TPF_OPERAND_INDEX_REPR_IMM32 = 0,
	TPF_OPERAND_INDEX_REPR_IMM64 = 1,
	TPF_OPERAND_INDEX_REPR_REG = 2,
	TPF_OPERAND_INDEX_REPR_REG_IMM32 = 3,
	TPF_OPERAND_INDEX_REPR_REG_IMM64 = 4,
};

struct tpf_token_operand
{
	unsigned comps_enum : 2; /* tpf_operands_comps */
	unsigned mode : 2; /* tpf_operand_mode */
	unsigned sel : 8;
	unsigned file : 8; /* tpf_file */
	unsigned num_indices : 2;
	unsigned index0_repr : 3; /* tpf_operand_index_repr */
	unsigned index1_repr : 3; /* tpf_operand_index_repr */
	unsigned index2_repr : 3; /* tpf_operand_index_repr */
	unsigned extended : 1;
};

#define TPF_OPERAND_SEL_MASK(sel) ((sel) & 0xf)
#define TPF_OPERAND_SEL_SWZ(sel, i) (((sel) >> ((i) * 2)) & 3)
#define TPF_OPERAND_SEL_SCALAR(sel) ((sel) & 3)

enum tpf_token_operand_extended_type
{
	TPF_TOKEN_OPERAND_EXTENDED_TYPE_EMPTY = 0,
	TPF_TOKEN_OPERAND_EXTENDED_TYPE_MODIFIER = 1,
};

struct tpf_token_operand_extended
{
	unsigned type : 6;
	unsigned neg : 1;
	unsigned abs : 1;
};

union tpf_any
{
	double f64;
	float f32;
	int64_t i64;
	int32_t i32;
	uint64_t u64;
	int64_t u32;
};

struct tpf_op;
struct tpf_insn;
struct tpf_dcl;
struct tpf_program;
std::ostream& operator <<(std::ostream& out, const tpf_op& op);
std::ostream& operator <<(std::ostream& out, const tpf_insn& op);
std::ostream& operator <<(std::ostream& out, const tpf_dcl& op);
std::ostream& operator <<(std::ostream& out, const tpf_program& op);

struct tpf_op
{
	uint8_t mode;
	uint8_t comps;
	uint8_t mask;
	uint8_t num_indices;
	uint8_t swizzle[4];
	tpf_file file;
	tpf_any imm_values[4];
	bool neg;
	bool abs;
	struct
	{
		int64_t disp;
		std::auto_ptr<tpf_op> reg;
	} indices[3];

	bool is_index_simple(unsigned i) const
	{
		 return !indices[i].reg.get() && indices[i].disp >= 0 && (int64_t)(int32_t)indices[i].disp == indices[i].disp;
	}

	bool has_simple_index() const
	{
		return num_indices == 1 && is_index_simple(0);
	}

	tpf_op()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	tpf_op(const tpf_op& op)
	{}
};

/* for sample_d */
#define TPF_MAX_OPS 6

struct tpf_insn : public tpf_token_instruction
{
	int8_t sample_offset[3];
	uint8_t resource_target;
	uint8_t resource_return_type[4];

	unsigned num;
	unsigned num_ops;
	std::auto_ptr<tpf_op> ops[TPF_MAX_OPS];

	tpf_insn()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	tpf_insn(const tpf_insn& op)
	{}
};

struct tpf_dcl : public tpf_token_instruction
{
	std::auto_ptr<tpf_op> op;
	union
	{
		unsigned num;
		float f32;
		tpf_sv sv;
		struct
		{
			unsigned id;
			unsigned expected_function_table_length;
			unsigned table_length;
			unsigned array_length;
		} intf;
		unsigned thread_group_size[3];
		tpf_token_resource_return_type rrt;
		struct
		{
			unsigned num;
			unsigned comps;
		} indexable_temp;
		struct
		{
			unsigned stride;
			unsigned count;
		} structured;
	};

	void* data;

	tpf_dcl()
	{
		memset(this, 0, sizeof(*this));
	}

	~tpf_dcl()
	{
		free(data);
	}

	void dump();

private:
	tpf_dcl(const tpf_dcl& op)
	{}
};

struct tpf_program
{
	tpf_token_version version;
	std::vector<tpf_dcl*> dcls;
	std::vector<tpf_insn*> insns;

	/* for ifs, the insn number of the else or endif if there is no else
	 * for elses, the insn number of the endif
	 * for endifs, the insn number of the if
	 * for loops, the insn number of the endloop
	 * for endloops, the insn number of the loop
	 * for all others, -1
	 */
	std::vector<int> cf_insn_linked;

	/* NOTE: sampler 0 is the unnormalized nearest sampler for LD/LD_MS, while
	 * sampler 1 is user-specified sampler 0
	 */
	bool resource_sampler_slots_assigned;
	std::vector<int> slot_to_resource;
	std::vector<int> slot_to_sampler;
	std::map<std::pair<int, int>, int> resource_sampler_to_slot;
	std::map<int, int> resource_to_slot;

	bool labels_found;
	std::vector<int> label_to_insn_num;

	tpf_program()
	{
		memset(&version, 0, sizeof(version));
		labels_found = false;
		resource_sampler_slots_assigned = false;
	}

	~tpf_program()
	{
		for(std::vector<tpf_dcl*>::iterator i = dcls.begin(), e = dcls.end(); i != e; ++i)
			delete *i;
		for(std::vector<tpf_insn*>::iterator i = insns.begin(), e = insns.end(); i != e; ++i)
			delete *i;
	}

	void dump();

private:
	tpf_program(const tpf_dcl& op)
	{}
};

tpf_program* tpf_parse(void* tokens, int size);

bool tpf_link_cf_insns(tpf_program& program);
bool tpf_find_labels(tpf_program& program);
bool tpf_allocate_resource_sampler_pairs(tpf_program& program);

#endif /* TPF_H_ */
