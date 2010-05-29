/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef R600_SHADER_H
#define R600_SHADER_H

#include "r600_compiler.h"
#include "radeon.h"

struct r600_shader_operand {
	struct c_vector			*vector;
	unsigned			sel;
	unsigned			chan;
	unsigned			neg;
	unsigned			abs;
};

struct r600_shader_vfetch {
	struct r600_shader_vfetch	*next;
	struct r600_shader_vfetch	*prev;
	unsigned			cf_addr;
	struct r600_shader_operand	src[2];
	struct r600_shader_operand	dst[4];
};

struct r600_shader_inst {
	unsigned			is_op3;
	unsigned			opcode;
	unsigned			inst;
	struct r600_shader_operand	src[3];
	struct r600_shader_operand	dst;
	unsigned			last;
};

struct r600_shader_alu {
	struct r600_shader_alu		*next;
	struct r600_shader_alu		*prev;
	unsigned			nalu;
	unsigned			nliteral;
	unsigned			nconstant;
	struct r600_shader_inst		alu[5];
	u32				literal[4];
};

struct r600_shader_node {
	struct r600_shader_node		*next;
	struct r600_shader_node		*prev;
	unsigned			cf_id;		/**< cf index (in dw) in byte code */
	unsigned			cf_addr;	/**< instructions index (in dw) in byte code */
	unsigned			nslot;		/**< number of slot (2 dw) needed by this node */
	unsigned			nfetch;
	struct c_node			*node;		/**< compiler node from which this node originate */
	struct r600_shader_vfetch	vfetch;		/**< list of vfetch instructions */
	struct r600_shader_alu		alu;		/**< list of alu instructions */
};

struct r600_shader_io {
	unsigned	name;
	unsigned	gpr;
	int		sid;
};

struct r600_shader {
	unsigned			stack_size;		/**< stack size needed by this shader */
	unsigned			ngpr;			/**< number of GPR needed by this shader */
	unsigned			nconstant;		/**< number of constants used by this shader */
	unsigned			nresource;		/**< number of resources used by this shader */
	unsigned			noutput;
	unsigned			ninput;
	unsigned			nvector;
	unsigned			ncf;			/**< total number of cf clauses */
	unsigned			nslot;			/**< total number of slots (2 dw) */
	unsigned			flat_shade;		/**< are we flat shading */
	struct r600_shader_node		nodes;			/**< list of node */
	struct r600_shader_io		input[32];
	struct r600_shader_io		output[32];
	/* TODO replace GPR by some better register allocator */
	struct c_vector			**gpr;
	unsigned			ndw;			/**< bytes code size in dw */
	u32				*bcode;			/**< bytes code */
	enum pipe_format		resource_format[160];	/**< format of resource */
	struct c_shader			cshader;
};

void r600_shader_cleanup(struct r600_shader *rshader);
int r600_shader_register(struct r600_shader *rshader);
int r600_shader_node(struct r600_shader *shader);
void r600_shader_node_place(struct r600_shader *rshader);
int r600_shader_find_gpr(struct r600_shader *rshader, struct c_vector *v, unsigned swizzle,
			struct r600_shader_operand *operand);
int r600_shader_vfetch_bytecode(struct r600_shader *rshader,
				struct r600_shader_node *rnode,
				struct r600_shader_vfetch *vfetch,
				unsigned *cid);
int r600_shader_update(struct r600_shader *rshader,
			enum pipe_format *resource_format);
int r600_shader_legalize(struct r600_shader *rshader);

int r700_shader_translate(struct r600_shader *rshader);

int c_shader_from_tgsi(struct c_shader *shader, unsigned type,
			const struct tgsi_token *tokens);
int r600_shader_register(struct r600_shader *rshader);
int r600_shader_translate_rec(struct r600_shader *rshader, struct c_node *node);
int r700_shader_translate(struct r600_shader *rshader);
int r600_shader_insert_fetch(struct c_shader *shader);

enum r600_instruction {
	INST_ADD			= 0,
	INST_MUL			= 1,
	INST_MUL_IEEE			= 2,
	INST_MAX			= 3,
	INST_MIN			= 4,
	INST_MAX_DX10			= 5,
	INST_MIN_DX10			= 6,
	INST_SETE			= 7,
	INST_SETGT			= 8,
	INST_SETGE			= 9,
	INST_SETNE			= 10,
	INST_SETE_DX10			= 11,
	INST_SETGT_DX10			= 12,
	INST_SETGE_DX10			= 13,
	INST_SETNE_DX10			= 14,
	INST_FRACT			= 15,
	INST_TRUNC			= 16,
	INST_CEIL			= 17,
	INST_RNDNE			= 18,
	INST_FLOOR			= 19,
	INST_MOVA			= 20,
	INST_MOVA_FLOOR			= 21,
	INST_MOVA_INT			= 22,
	INST_MOV			= 23,
	INST_NOP			= 24,
	INST_PRED_SETGT_UINT		= 25,
	INST_PRED_SETGE_UINT		= 26,
	INST_PRED_SETE			= 27,
	INST_PRED_SETGT			= 28,
	INST_PRED_SETGE			= 29,
	INST_PRED_SETNE			= 30,
	INST_PRED_SET_INV		= 31,
	INST_PRED_SET_POP		= 32,
	INST_PRED_SET_CLR		= 33,
	INST_PRED_SET_RESTORE		= 34,
	INST_PRED_SETE_PUSH		= 35,
	INST_PRED_SETGT_PUSH		= 36,
	INST_PRED_SETGE_PUSH		= 37,
	INST_PRED_SETNE_PUSH		= 38,
	INST_KILLE			= 39,
	INST_KILLGT			= 40,
	INST_KILLGE			= 41,
	INST_KILLNE			= 42,
	INST_AND_INT			= 43,
	INST_OR_INT			= 44,
	INST_XOR_INT			= 45,
	INST_NOT_INT			= 46,
	INST_ADD_INT			= 47,
	INST_SUB_INT			= 48,
	INST_MAX_INT			= 49,
	INST_MIN_INT			= 50,
	INST_MAX_UINT			= 51,
	INST_MIN_UINT			= 52,
	INST_SETE_INT			= 53,
	INST_SETGT_INT			= 54,
	INST_SETGE_INT			= 55,
	INST_SETNE_INT			= 56,
	INST_SETGT_UINT			= 57,
	INST_SETGE_UINT			= 58,
	INST_KILLGT_UINT		= 59,
	INST_KILLGE_UINT		= 60,
	INST_PRED_SETE_INT		= 61,
	INST_PRED_SETGT_INT		= 62,
	INST_PRED_SETGE_INT		= 63,
	INST_PRED_SETNE_INT		= 64,
	INST_KILLE_INT			= 65,
	INST_KILLGT_INT			= 66,
	INST_KILLGE_INT			= 67,
	INST_KILLNE_INT			= 68,
	INST_PRED_SETE_PUSH_INT		= 69,
	INST_PRED_SETGT_PUSH_INT	= 70,
	INST_PRED_SETGE_PUSH_INT	= 71,
	INST_PRED_SETNE_PUSH_INT	= 72,
	INST_PRED_SETLT_PUSH_INT	= 73,
	INST_PRED_SETLE_PUSH_INT	= 74,
	INST_DOT4			= 75,
	INST_DOT4_IEEE			= 76,
	INST_CUBE			= 77,
	INST_MAX4			= 78,
	INST_MOVA_GPR_INT		= 79,
	INST_EXP_IEEE			= 80,
	INST_LOG_CLAMPED		= 81,
	INST_LOG_IEEE			= 82,
	INST_RECIP_CLAMPED		= 83,
	INST_RECIP_FF			= 84,
	INST_RECIP_IEEE			= 85,
	INST_RECIPSQRT_CLAMPED		= 86,
	INST_RECIPSQRT_FF		= 87,
	INST_RECIPSQRT_IEEE		= 88,
	INST_SQRT_IEEE			= 89,
	INST_FLT_TO_INT			= 90,
	INST_INT_TO_FLT			= 91,
	INST_UINT_TO_FLT		= 92,
	INST_SIN			= 93,
	INST_COS			= 94,
	INST_ASHR_INT			= 95,
	INST_LSHR_INT			= 96,
	INST_LSHL_INT			= 97,
	INST_MULLO_INT			= 98,
	INST_MULHI_INT			= 99,
	INST_MULLO_UINT			= 100,
	INST_MULHI_UINT			= 101,
	INST_RECIP_INT			= 102,
	INST_RECIP_UINT			= 103,
	INST_FLT_TO_UINT		= 104,
	INST_MUL_LIT			= 105,
	INST_MUL_LIT_M2			= 106,
	INST_MUL_LIT_M4			= 107,
	INST_MUL_LIT_D2			= 108,
	INST_MULADD			= 109,
	INST_MULADD_M2			= 110,
	INST_MULADD_M4			= 111,
	INST_MULADD_D2			= 112,
	INST_MULADD_IEEE		= 113,
	INST_MULADD_IEEE_M2		= 114,
	INST_MULADD_IEEE_M4		= 115,
	INST_MULADD_IEEE_D2		= 116,
	INST_CNDE			= 117,
	INST_CNDGT			= 118,
	INST_CNDGE			= 119,
	INST_CNDE_INT			= 120,
	INST_CNDGT_INT			= 121,
	INST_CNDGE_INT			= 122,
	INST_COUNT
};

struct r600_instruction_info {
	enum r600_instruction		instruction;
	unsigned			opcode;
	unsigned			is_trans;
	unsigned			is_op3;
};


#endif
