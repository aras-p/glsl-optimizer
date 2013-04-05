/*
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IR_H_
#define IR_H_

#include <stdint.h>
#include <stdbool.h>

#include "instr-a2xx.h"

/* low level intermediate representation of an adreno shader program */

struct ir_shader;

struct ir_shader * fd_asm_parse(const char *src);

struct ir_shader_info {
	uint16_t sizedwords;
	int8_t   max_reg;   /* highest GPR # used by shader */
	uint8_t  max_input_reg;
	uint64_t regs_written;
};

struct ir_register {
	enum {
		IR_REG_CONST  = 0x1,
		IR_REG_EXPORT = 0x2,
		IR_REG_NEGATE = 0x4,
		IR_REG_ABS    = 0x8,
	} flags;
	int num;
	char *swizzle;
};

enum ir_pred {
	IR_PRED_NONE = 0,
	IR_PRED_EQ = 1,
	IR_PRED_NE = 2,
};

struct ir_instruction {
	struct ir_shader *shader;
	enum {
		IR_FETCH,
		IR_ALU,
	} instr_type;
	enum ir_pred pred;
	int sync;
	unsigned regs_count;
	struct ir_register *regs[5];
	union {
		/* FETCH specific: */
		struct {
			instr_fetch_opc_t opc;
			unsigned const_idx;
			/* maybe vertex fetch specific: */
			unsigned const_idx_sel;
			enum a2xx_sq_surfaceformat fmt;
			bool is_signed : 1;
			bool is_normalized : 1;
			uint32_t stride;
			uint32_t offset;
		} fetch;
		/* ALU specific: */
		struct {
			instr_vector_opc_t vector_opc;
			instr_scalar_opc_t scalar_opc;
			bool vector_clamp : 1;
			bool scalar_clamp : 1;
		} alu;
	};
};

struct ir_cf {
	struct ir_shader *shader;
	instr_cf_opc_t cf_type;

	union {
		/* EXEC/EXEC_END specific: */
		struct {
			unsigned instrs_count;
			struct ir_instruction *instrs[6];
			uint32_t addr, cnt, sequence;
		} exec;
		/* ALLOC specific: */
		struct {
			instr_alloc_type_t type;   /* SQ_POSITION or SQ_PARAMETER_PIXEL */
			int size;
		} alloc;
	};
};

/* somewhat arbitrary limits.. */
#define MAX_ATTRIBUTES 32
#define MAX_CONSTS     32
#define MAX_SAMPLERS   32
#define MAX_UNIFORMS   32
#define MAX_VARYINGS   32

struct ir_attribute {
	const char *name;
	int rstart;         /* first register */
	int num;            /* number of registers */
};

struct ir_const {
	float val[4];
	int cstart;         /* first const register */
};

struct ir_sampler {
	const char *name;
	int idx;
};

struct ir_uniform {
	const char *name;
	int cstart;         /* first const register */
	int num;            /* number of const registers */
};

struct ir_varying {
	const char *name;
	int rstart;         /* first register */
	int num;            /* number of registers */
};

struct ir_shader {
	unsigned cfs_count;
	struct ir_cf *cfs[0x56];
	uint32_t heap[100 * 4096];
	unsigned heap_idx;

	enum ir_pred pred;  /* pred inherited by newly created instrs */

	/* @ headers: */
	uint32_t attributes_count;
	struct ir_attribute *attributes[MAX_ATTRIBUTES];

	uint32_t consts_count;
	struct ir_const *consts[MAX_CONSTS];

	uint32_t samplers_count;
	struct ir_sampler *samplers[MAX_SAMPLERS];

	uint32_t uniforms_count;
	struct ir_uniform *uniforms[MAX_UNIFORMS];

	uint32_t varyings_count;
	struct ir_varying *varyings[MAX_VARYINGS];

};

struct ir_shader * ir_shader_create(void);
void ir_shader_destroy(struct ir_shader *shader);
void * ir_shader_assemble(struct ir_shader *shader,
		struct ir_shader_info *info);

struct ir_attribute * ir_attribute_create(struct ir_shader *shader,
		int rstart, int num, const char *name);
struct ir_const * ir_const_create(struct ir_shader *shader,
		int cstart, float v0, float v1, float v2, float v3);
struct ir_sampler * ir_sampler_create(struct ir_shader *shader,
		int idx, const char *name);
struct ir_uniform * ir_uniform_create(struct ir_shader *shader,
		int cstart, int num, const char *name);
struct ir_varying * ir_varying_create(struct ir_shader *shader,
		int rstart, int num, const char *name);

struct ir_cf * ir_cf_create(struct ir_shader *shader, instr_cf_opc_t cf_type);

struct ir_instruction * ir_instr_create(struct ir_cf *cf, int instr_type);

struct ir_register * ir_reg_create(struct ir_instruction *instr,
		int num, const char *swizzle, int flags);

/* some helper fxns: */

static inline struct ir_cf *
ir_cf_create_alloc(struct ir_shader *shader, instr_alloc_type_t type, int size)
{
	struct ir_cf *cf = ir_cf_create(shader, ALLOC);
	if (!cf)
		return cf;
	cf->alloc.type = type;
	cf->alloc.size = size;
	return cf;
}
static inline struct ir_instruction *
ir_instr_create_alu(struct ir_cf *cf, instr_vector_opc_t vop, instr_scalar_opc_t sop)
{
	struct ir_instruction *instr = ir_instr_create(cf, IR_ALU);
	if (!instr)
		return instr;
	instr->alu.vector_opc = vop;
	instr->alu.scalar_opc = sop;
	return instr;
}
static inline struct ir_instruction *
ir_instr_create_vtx_fetch(struct ir_cf *cf, int ci, int cis,
		enum a2xx_sq_surfaceformat fmt, bool is_signed, int stride)
{
	struct ir_instruction *instr = instr = ir_instr_create(cf, IR_FETCH);
	instr->fetch.opc = VTX_FETCH;
	instr->fetch.const_idx = ci;
	instr->fetch.const_idx_sel = cis;
	instr->fetch.fmt = fmt;
	instr->fetch.is_signed = is_signed;
	instr->fetch.stride = stride;
	return instr;
}
static inline struct ir_instruction *
ir_instr_create_tex_fetch(struct ir_cf *cf, int ci)
{
	struct ir_instruction *instr = instr = ir_instr_create(cf, IR_FETCH);
	instr->fetch.opc = TEX_FETCH;
	instr->fetch.const_idx = ci;
	return instr;
}


#endif /* IR_H_ */
