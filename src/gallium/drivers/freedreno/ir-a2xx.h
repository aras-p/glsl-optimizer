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

#ifndef IR2_H_
#define IR2_H_

#include <stdint.h>
#include <stdbool.h>

#include "instr-a2xx.h"

/* low level intermediate representation of an adreno a2xx shader program */

struct ir2_shader;

struct ir2_shader_info {
	uint16_t sizedwords;
	int8_t   max_reg;   /* highest GPR # used by shader */
	uint8_t  max_input_reg;
	uint64_t regs_written;
};

struct ir2_register {
	enum {
		IR2_REG_CONST  = 0x1,
		IR2_REG_EXPORT = 0x2,
		IR2_REG_NEGATE = 0x4,
		IR2_REG_ABS    = 0x8,
	} flags;
	int num;
	char *swizzle;
};

enum ir2_pred {
	IR2_PRED_NONE = 0,
	IR2_PRED_EQ = 1,
	IR2_PRED_NE = 2,
};

struct ir2_instruction {
	struct ir2_shader *shader;
	enum {
		IR2_FETCH,
		IR2_ALU,
	} instr_type;
	enum ir2_pred pred;
	int sync;
	unsigned regs_count;
	struct ir2_register *regs[5];
	union {
		/* FETCH specific: */
		struct {
			instr_fetch_opc_t opc;
			unsigned const_idx;
			/* texture fetch specific: */
			bool is_cube : 1;
			/* vertex fetch specific: */
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

struct ir2_cf {
	struct ir2_shader *shader;
	instr_cf_opc_t cf_type;

	union {
		/* EXEC/EXEC_END specific: */
		struct {
			unsigned instrs_count;
			struct ir2_instruction *instrs[6];
			uint32_t addr, cnt, sequence;
		} exec;
		/* ALLOC specific: */
		struct {
			instr_alloc_type_t type;   /* SQ_POSITION or SQ_PARAMETER_PIXEL */
			int size;
		} alloc;
	};
};

struct ir2_shader {
	unsigned cfs_count;
	struct ir2_cf *cfs[0x56];
	uint32_t heap[100 * 4096];
	unsigned heap_idx;

	enum ir2_pred pred;  /* pred inherited by newly created instrs */
};

struct ir2_shader * ir2_shader_create(void);
void ir2_shader_destroy(struct ir2_shader *shader);
void * ir2_shader_assemble(struct ir2_shader *shader,
		struct ir2_shader_info *info);

struct ir2_cf * ir2_cf_create(struct ir2_shader *shader, instr_cf_opc_t cf_type);

struct ir2_instruction * ir2_instr_create(struct ir2_cf *cf, int instr_type);

struct ir2_register * ir2_reg_create(struct ir2_instruction *instr,
		int num, const char *swizzle, int flags);

/* some helper fxns: */

static inline struct ir2_cf *
ir2_cf_create_alloc(struct ir2_shader *shader, instr_alloc_type_t type, int size)
{
	struct ir2_cf *cf = ir2_cf_create(shader, ALLOC);
	if (!cf)
		return cf;
	cf->alloc.type = type;
	cf->alloc.size = size;
	return cf;
}
static inline struct ir2_instruction *
ir2_instr_create_alu(struct ir2_cf *cf, instr_vector_opc_t vop, instr_scalar_opc_t sop)
{
	struct ir2_instruction *instr = ir2_instr_create(cf, IR2_ALU);
	if (!instr)
		return instr;
	instr->alu.vector_opc = vop;
	instr->alu.scalar_opc = sop;
	return instr;
}
static inline struct ir2_instruction *
ir2_instr_create_vtx_fetch(struct ir2_cf *cf, int ci, int cis,
		enum a2xx_sq_surfaceformat fmt, bool is_signed, int stride)
{
	struct ir2_instruction *instr = instr = ir2_instr_create(cf, IR2_FETCH);
	instr->fetch.opc = VTX_FETCH;
	instr->fetch.const_idx = ci;
	instr->fetch.const_idx_sel = cis;
	instr->fetch.fmt = fmt;
	instr->fetch.is_signed = is_signed;
	instr->fetch.stride = stride;
	return instr;
}
static inline struct ir2_instruction *
ir2_instr_create_tex_fetch(struct ir2_cf *cf, int ci)
{
	struct ir2_instruction *instr = instr = ir2_instr_create(cf, IR2_FETCH);
	instr->fetch.opc = TEX_FETCH;
	instr->fetch.const_idx = ci;
	return instr;
}


#endif /* IR2_H_ */
