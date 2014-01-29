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

#include "ir3.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include "freedreno_util.h"
#include "instr-a3xx.h"

/* simple allocator to carve allocations out of an up-front allocated heap,
 * so that we can free everything easily in one shot.
 */
void * ir3_alloc(struct ir3_shader *shader, int sz)
{
	void *ptr = &shader->heap[shader->heap_idx];
	shader->heap_idx += align(sz, 4);
	return ptr;
}

struct ir3_shader * ir3_shader_create(void)
{
	return calloc(1, sizeof(struct ir3_shader));
}

void ir3_shader_destroy(struct ir3_shader *shader)
{
	free(shader);
}

#define iassert(cond) do { \
	if (!(cond)) { \
		assert(cond); \
		return -1; \
	} } while (0)

static uint32_t reg(struct ir3_register *reg, struct ir3_shader_info *info,
		uint32_t repeat, uint32_t valid_flags)
{
	reg_t val = { .dummy32 = 0 };

	assert(!(reg->flags & ~valid_flags));

	if (!(reg->flags & IR3_REG_R))
		repeat = 0;

	if (reg->flags & IR3_REG_IMMED) {
		val.iim_val = reg->iim_val;
	} else {
		int8_t components = util_last_bit(reg->wrmask);
		int8_t max = (reg->num + repeat + components - 1) >> 2;

		val.comp = reg->num & 0x3;
		val.num  = reg->num >> 2;

		if (reg->flags & IR3_REG_CONST) {
			info->max_const = MAX2(info->max_const, max);
		} else if ((max != REG_A0) && (max != REG_P0)) {
			if (reg->flags & IR3_REG_HALF) {
				info->max_half_reg = MAX2(info->max_half_reg, max);
			} else {
				info->max_reg = MAX2(info->max_reg, max);
			}
		}
	}

	return val.dummy32;
}

static int emit_cat0(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	instr_cat0_t *cat0 = ptr;

	cat0->immed    = instr->cat0.immed;
	cat0->repeat   = instr->repeat;
	cat0->ss       = !!(instr->flags & IR3_INSTR_SS);
	cat0->inv      = instr->cat0.inv;
	cat0->comp     = instr->cat0.comp;
	cat0->opc      = instr->opc;
	cat0->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat0->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat0->opc_cat  = 0;

	return 0;
}

static uint32_t type_flags(type_t type)
{
	return (type_size(type) == 32) ? 0 : IR3_REG_HALF;
}

static int emit_cat1(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src = instr->regs[1];
	instr_cat1_t *cat1 = ptr;

	iassert(instr->regs_count == 2);
	iassert(!((dst->flags ^ type_flags(instr->cat1.dst_type)) & IR3_REG_HALF));
	iassert((src->flags & IR3_REG_IMMED) ||
			!((src->flags ^ type_flags(instr->cat1.src_type)) & IR3_REG_HALF));

	if (src->flags & IR3_REG_IMMED) {
		cat1->iim_val = src->iim_val;
		cat1->src_im  = 1;
	} else if (src->flags & IR3_REG_RELATIV) {
		cat1->off       = src->offset;
		cat1->src_rel   = 1;
		cat1->src_rel_c = !!(src->flags & IR3_REG_CONST);
	} else {
		cat1->src  = reg(src, info, instr->repeat,
				IR3_REG_IMMED | IR3_REG_R |
				IR3_REG_CONST | IR3_REG_HALF);
		cat1->src_c     = !!(src->flags & IR3_REG_CONST);
	}

	cat1->dst      = reg(dst, info, instr->repeat,
			IR3_REG_RELATIV | IR3_REG_EVEN |
			IR3_REG_R | IR3_REG_POS_INF | IR3_REG_HALF);
	cat1->repeat   = instr->repeat;
	cat1->src_r    = !!(src->flags & IR3_REG_R);
	cat1->ss       = !!(instr->flags & IR3_INSTR_SS);
	cat1->ul       = !!(instr->flags & IR3_INSTR_UL);
	cat1->dst_type = instr->cat1.dst_type;
	cat1->dst_rel  = !!(dst->flags & IR3_REG_RELATIV);
	cat1->src_type = instr->cat1.src_type;
	cat1->even     = !!(dst->flags & IR3_REG_EVEN);
	cat1->pos_inf  = !!(dst->flags & IR3_REG_POS_INF);
	cat1->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat1->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat1->opc_cat  = 1;

	return 0;
}

static int emit_cat2(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src1 = instr->regs[1];
	struct ir3_register *src2 = instr->regs[2];
	instr_cat2_t *cat2 = ptr;

	iassert((instr->regs_count == 2) || (instr->regs_count == 3));

	if (src1->flags & IR3_REG_RELATIV) {
		iassert(src1->num < (1 << 10));
		cat2->rel1.src1      = reg(src1, info, instr->repeat,
				IR3_REG_RELATIV | IR3_REG_CONST | IR3_REG_NEGATE |
				IR3_REG_ABS | IR3_REG_R | IR3_REG_HALF);
		cat2->rel1.src1_c    = !!(src1->flags & IR3_REG_CONST);
		cat2->rel1.src1_rel  = 1;
	} else if (src1->flags & IR3_REG_CONST) {
		iassert(src1->num < (1 << 12));
		cat2->c1.src1   = reg(src1, info, instr->repeat,
				IR3_REG_CONST | IR3_REG_NEGATE | IR3_REG_ABS |
				IR3_REG_R | IR3_REG_HALF);
		cat2->c1.src1_c = 1;
	} else {
		iassert(src1->num < (1 << 11));
		cat2->src1 = reg(src1, info, instr->repeat,
				IR3_REG_IMMED | IR3_REG_NEGATE | IR3_REG_ABS |
				IR3_REG_R | IR3_REG_HALF);
	}
	cat2->src1_im  = !!(src1->flags & IR3_REG_IMMED);
	cat2->src1_neg = !!(src1->flags & IR3_REG_NEGATE);
	cat2->src1_abs = !!(src1->flags & IR3_REG_ABS);
	cat2->src1_r   = !!(src1->flags & IR3_REG_R);

	if (src2) {
		iassert((src2->flags & IR3_REG_IMMED) ||
				!((src1->flags ^ src2->flags) & IR3_REG_HALF));

		if (src2->flags & IR3_REG_RELATIV) {
			iassert(src2->num < (1 << 10));
			cat2->rel2.src2      = reg(src2, info, instr->repeat,
					IR3_REG_RELATIV | IR3_REG_CONST | IR3_REG_NEGATE |
					IR3_REG_ABS | IR3_REG_R | IR3_REG_HALF);
			cat2->rel2.src2_c    = !!(src2->flags & IR3_REG_CONST);
			cat2->rel2.src2_rel  = 1;
		} else if (src2->flags & IR3_REG_CONST) {
			iassert(src2->num < (1 << 12));
			cat2->c2.src2   = reg(src2, info, instr->repeat,
					IR3_REG_CONST | IR3_REG_NEGATE | IR3_REG_ABS |
					IR3_REG_R | IR3_REG_HALF);
			cat2->c2.src2_c = 1;
		} else {
			iassert(src2->num < (1 << 11));
			cat2->src2 = reg(src2, info, instr->repeat,
					IR3_REG_IMMED | IR3_REG_NEGATE | IR3_REG_ABS |
					IR3_REG_R | IR3_REG_HALF);
		}

		cat2->src2_im  = !!(src2->flags & IR3_REG_IMMED);
		cat2->src2_neg = !!(src2->flags & IR3_REG_NEGATE);
		cat2->src2_abs = !!(src2->flags & IR3_REG_ABS);
		cat2->src2_r   = !!(src2->flags & IR3_REG_R);
	}

	cat2->dst      = reg(dst, info, instr->repeat,
			IR3_REG_R | IR3_REG_EI | IR3_REG_HALF);
	cat2->repeat   = instr->repeat;
	cat2->ss       = !!(instr->flags & IR3_INSTR_SS);
	cat2->ul       = !!(instr->flags & IR3_INSTR_UL);
	cat2->dst_half = !!((src1->flags ^ dst->flags) & IR3_REG_HALF);
	cat2->ei       = !!(dst->flags & IR3_REG_EI);
	cat2->cond     = instr->cat2.condition;
	cat2->full     = ! (src1->flags & IR3_REG_HALF);
	cat2->opc      = instr->opc;
	cat2->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat2->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat2->opc_cat  = 2;

	return 0;
}

static int emit_cat3(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src1 = instr->regs[1];
	struct ir3_register *src2 = instr->regs[2];
	struct ir3_register *src3 = instr->regs[3];
	instr_cat3_t *cat3 = ptr;
	uint32_t src_flags = 0;

	switch (instr->opc) {
	case OPC_MAD_F16:
	case OPC_MAD_U16:
	case OPC_MAD_S16:
	case OPC_SEL_B16:
	case OPC_SEL_S16:
	case OPC_SEL_F16:
	case OPC_SAD_S16:
	case OPC_SAD_S32:  // really??
		src_flags |= IR3_REG_HALF;
		break;
	default:
		break;
	}

	iassert(instr->regs_count == 4);
	iassert(!((src1->flags ^ src_flags) & IR3_REG_HALF));
	iassert(!((src2->flags ^ src_flags) & IR3_REG_HALF));
	iassert(!((src3->flags ^ src_flags) & IR3_REG_HALF));

	if (src1->flags & IR3_REG_RELATIV) {
		iassert(src1->num < (1 << 10));
		cat3->rel1.src1      = reg(src1, info, instr->repeat,
				IR3_REG_RELATIV | IR3_REG_CONST | IR3_REG_NEGATE |
				IR3_REG_R | IR3_REG_HALF);
		cat3->rel1.src1_c    = !!(src1->flags & IR3_REG_CONST);
		cat3->rel1.src1_rel  = 1;
	} else if (src1->flags & IR3_REG_CONST) {
		iassert(src1->num < (1 << 12));
		cat3->c1.src1   = reg(src1, info, instr->repeat,
				IR3_REG_CONST | IR3_REG_NEGATE | IR3_REG_R |
				IR3_REG_HALF);
		cat3->c1.src1_c = 1;
	} else {
		iassert(src1->num < (1 << 11));
		cat3->src1 = reg(src1, info, instr->repeat,
				IR3_REG_NEGATE | IR3_REG_R | IR3_REG_HALF);
	}

	cat3->src1_neg = !!(src1->flags & IR3_REG_NEGATE);
	cat3->src1_r   = !!(src1->flags & IR3_REG_R);

	cat3->src2     = reg(src2, info, instr->repeat,
			IR3_REG_CONST | IR3_REG_NEGATE |
			IR3_REG_R | IR3_REG_HALF);
	cat3->src2_c   = !!(src2->flags & IR3_REG_CONST);
	cat3->src2_neg = !!(src2->flags & IR3_REG_NEGATE);
	cat3->src2_r   = !!(src2->flags & IR3_REG_R);


	if (src3->flags & IR3_REG_RELATIV) {
		iassert(src3->num < (1 << 10));
		cat3->rel2.src3      = reg(src3, info, instr->repeat,
				IR3_REG_RELATIV | IR3_REG_CONST | IR3_REG_NEGATE |
				IR3_REG_R | IR3_REG_HALF);
		cat3->rel2.src3_c    = !!(src3->flags & IR3_REG_CONST);
		cat3->rel2.src3_rel  = 1;
	} else if (src3->flags & IR3_REG_CONST) {
		iassert(src3->num < (1 << 12));
		cat3->c2.src3   = reg(src3, info, instr->repeat,
				IR3_REG_CONST | IR3_REG_NEGATE | IR3_REG_R |
				IR3_REG_HALF);
		cat3->c2.src3_c = 1;
	} else {
		iassert(src3->num < (1 << 11));
		cat3->src3 = reg(src3, info, instr->repeat,
				IR3_REG_NEGATE | IR3_REG_R | IR3_REG_HALF);
	}

	cat3->src3_neg = !!(src3->flags & IR3_REG_NEGATE);
	cat3->src3_r   = !!(src3->flags & IR3_REG_R);

	cat3->dst      = reg(dst, info, instr->repeat, IR3_REG_R | IR3_REG_HALF);
	cat3->repeat   = instr->repeat;
	cat3->ss       = !!(instr->flags & IR3_INSTR_SS);
	cat3->ul       = !!(instr->flags & IR3_INSTR_UL);
	cat3->dst_half = !!((src_flags ^ dst->flags) & IR3_REG_HALF);
	cat3->opc      = instr->opc;
	cat3->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat3->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat3->opc_cat  = 3;

	return 0;
}

static int emit_cat4(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src = instr->regs[1];
	instr_cat4_t *cat4 = ptr;

	iassert(instr->regs_count == 2);

	if (src->flags & IR3_REG_RELATIV) {
		iassert(src->num < (1 << 10));
		cat4->rel.src      = reg(src, info, instr->repeat,
				IR3_REG_RELATIV | IR3_REG_CONST | IR3_REG_NEGATE |
				IR3_REG_ABS | IR3_REG_R | IR3_REG_HALF);
		cat4->rel.src_c    = !!(src->flags & IR3_REG_CONST);
		cat4->rel.src_rel  = 1;
	} else if (src->flags & IR3_REG_CONST) {
		iassert(src->num < (1 << 12));
		cat4->c.src   = reg(src, info, instr->repeat,
				IR3_REG_CONST | IR3_REG_NEGATE | IR3_REG_ABS |
				IR3_REG_R | IR3_REG_HALF);
		cat4->c.src_c = 1;
	} else {
		iassert(src->num < (1 << 11));
		cat4->src = reg(src, info, instr->repeat,
				IR3_REG_IMMED | IR3_REG_NEGATE | IR3_REG_ABS |
				IR3_REG_R | IR3_REG_HALF);
	}

	cat4->src_im   = !!(src->flags & IR3_REG_IMMED);
	cat4->src_neg  = !!(src->flags & IR3_REG_NEGATE);
	cat4->src_abs  = !!(src->flags & IR3_REG_ABS);
	cat4->src_r    = !!(src->flags & IR3_REG_R);

	cat4->dst      = reg(dst, info, instr->repeat, IR3_REG_R | IR3_REG_HALF);
	cat4->repeat   = instr->repeat;
	cat4->ss       = !!(instr->flags & IR3_INSTR_SS);
	cat4->ul       = !!(instr->flags & IR3_INSTR_UL);
	cat4->dst_half = !!((src->flags ^ dst->flags) & IR3_REG_HALF);
	cat4->full     = ! (src->flags & IR3_REG_HALF);
	cat4->opc      = instr->opc;
	cat4->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat4->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat4->opc_cat  = 4;

	return 0;
}

static int emit_cat5(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src1 = instr->regs[1];
	struct ir3_register *src2 = instr->regs[2];
	struct ir3_register *src3 = instr->regs[3];
	instr_cat5_t *cat5 = ptr;

	iassert(!((dst->flags ^ type_flags(instr->cat5.type)) & IR3_REG_HALF));

	if (src1) {
		cat5->full = ! (src1->flags & IR3_REG_HALF);
		cat5->src1 = reg(src1, info, instr->repeat, IR3_REG_HALF);
	}


	if (instr->flags & IR3_INSTR_S2EN) {
		if (src2) {
			iassert(!((src1->flags ^ src2->flags) & IR3_REG_HALF));
			cat5->s2en.src2 = reg(src2, info, instr->repeat, IR3_REG_HALF);
		}
		if (src3) {
			iassert(src3->flags & IR3_REG_HALF);
			cat5->s2en.src3 = reg(src3, info, instr->repeat, IR3_REG_HALF);
		}
		iassert(!(instr->cat5.samp | instr->cat5.tex));
	} else {
		iassert(!src3);
		if (src2) {
			iassert(!((src1->flags ^ src2->flags) & IR3_REG_HALF));
			cat5->norm.src2 = reg(src2, info, instr->repeat, IR3_REG_HALF);
		}
		cat5->norm.samp = instr->cat5.samp;
		cat5->norm.tex  = instr->cat5.tex;
	}

	cat5->dst      = reg(dst, info, instr->repeat, IR3_REG_R | IR3_REG_HALF);
	cat5->wrmask   = dst->wrmask;
	cat5->type     = instr->cat5.type;
	cat5->is_3d    = !!(instr->flags & IR3_INSTR_3D);
	cat5->is_a     = !!(instr->flags & IR3_INSTR_A);
	cat5->is_s     = !!(instr->flags & IR3_INSTR_S);
	cat5->is_s2en  = !!(instr->flags & IR3_INSTR_S2EN);
	cat5->is_o     = !!(instr->flags & IR3_INSTR_O);
	cat5->is_p     = !!(instr->flags & IR3_INSTR_P);
	cat5->opc      = instr->opc;
	cat5->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat5->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat5->opc_cat  = 5;

	return 0;
}

static int emit_cat6(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info)
{
	struct ir3_register *dst = instr->regs[0];
	struct ir3_register *src = instr->regs[1];
	instr_cat6_t *cat6 = ptr;

	iassert(instr->regs_count == 2);

	switch (instr->opc) {
	/* load instructions: */
	case OPC_LDG:
	case OPC_LDP:
	case OPC_LDL:
	case OPC_LDLW:
	case OPC_LDLV:
	case OPC_PREFETCH: {
		instr_cat6a_t *cat6a = ptr;

		iassert(!((dst->flags ^ type_flags(instr->cat6.type)) & IR3_REG_HALF));

		cat6a->must_be_one1  = 1;
		cat6a->must_be_one2  = 1;
		cat6a->off = instr->cat6.offset;
		cat6a->src = reg(src, info, instr->repeat, 0);
		cat6a->dst = reg(dst, info, instr->repeat, IR3_REG_R | IR3_REG_HALF);
		break;
	}
	/* store instructions: */
	case OPC_STG:
	case OPC_STP:
	case OPC_STL:
	case OPC_STLW:
	case OPC_STI: {
		instr_cat6b_t *cat6b = ptr;
		uint32_t src_flags = type_flags(instr->cat6.type);
		uint32_t dst_flags = (instr->opc == OPC_STI) ? IR3_REG_HALF : 0;

		iassert(!((src->flags ^ src_flags) & IR3_REG_HALF));

		cat6b->must_be_one1  = 1;
		cat6b->must_be_one2  = 1;
		cat6b->src    = reg(src, info, instr->repeat, src_flags);
		cat6b->off_hi = instr->cat6.offset >> 8;
		cat6b->off    = instr->cat6.offset;
		cat6b->dst    = reg(dst, info, instr->repeat, IR3_REG_R | dst_flags);

		break;
	}
	default:
		// TODO
		break;
	}

	cat6->iim_val  = instr->cat6.iim_val;
	cat6->type     = instr->cat6.type;
	cat6->opc      = instr->opc;
	cat6->jmp_tgt  = !!(instr->flags & IR3_INSTR_JP);
	cat6->sync     = !!(instr->flags & IR3_INSTR_SY);
	cat6->opc_cat  = 6;

	return 0;
}

static int (*emit[])(struct ir3_instruction *instr, void *ptr,
		struct ir3_shader_info *info) = {
	emit_cat0, emit_cat1, emit_cat2, emit_cat3, emit_cat4, emit_cat5, emit_cat6,
};

void * ir3_shader_assemble(struct ir3_shader *shader, struct ir3_shader_info *info)
{
	uint32_t *ptr, *dwords;
	uint32_t i;

	info->max_reg       = -1;
	info->max_half_reg  = -1;
	info->max_const     = -1;
	info->instrs_count  = 0;

	/* need a integer number of instruction "groups" (sets of four
	 * instructions), so pad out w/ NOPs if needed:
	 * (each instruction is 64bits)
	 */
	info->sizedwords = 2 * align(shader->instrs_count, 4);

	ptr = dwords = calloc(1, 4 * info->sizedwords);

	for (i = 0; i < shader->instrs_count; i++) {
		struct ir3_instruction *instr = shader->instrs[i];
		int ret = emit[instr->category](instr, dwords, info);
		if (ret)
			goto fail;
		info->instrs_count += 1 + instr->repeat;
		dwords += 2;
	}

	return ptr;

fail:
	free(ptr);
	return NULL;
}

static struct ir3_register * reg_create(struct ir3_shader *shader,
		int num, int flags)
{
	struct ir3_register *reg =
			ir3_alloc(shader, sizeof(struct ir3_register));
	reg->wrmask = 1;
	reg->flags = flags;
	reg->num = num;
	return reg;
}

static void insert_instr(struct ir3_shader *shader,
		struct ir3_instruction *instr)
{
#ifdef DEBUG
	static uint32_t serialno = 0;
	instr->serialno = ++serialno;
#endif
	assert(shader->instrs_count < ARRAY_SIZE(shader->instrs));
	shader->instrs[shader->instrs_count++] = instr;
}

struct ir3_block * ir3_block_create(struct ir3_shader *shader,
		unsigned ntmp, unsigned nin, unsigned nout)
{
	struct ir3_block *block;
	unsigned size;
	char *ptr;

	size = sizeof(*block);
	size += sizeof(block->temporaries[0]) * ntmp;
	size += sizeof(block->inputs[0]) * nin;
	size += sizeof(block->outputs[0]) * nout;

	ptr = ir3_alloc(shader, size);

	block = (void *)ptr;
	ptr += sizeof(*block);

	block->temporaries = (void *)ptr;
	block->ntemporaries = ntmp;
	ptr += sizeof(block->temporaries[0]) * ntmp;

	block->inputs = (void *)ptr;
	block->ninputs = nin;
	ptr += sizeof(block->inputs[0]) * nin;

	block->outputs = (void *)ptr;
	block->noutputs = nout;
	ptr += sizeof(block->outputs[0]) * nout;

	block->shader = shader;

	return block;
}

struct ir3_instruction * ir3_instr_create(struct ir3_block *block,
		int category, opc_t opc)
{
	struct ir3_instruction *instr =
			ir3_alloc(block->shader, sizeof(struct ir3_instruction));
	instr->block = block;
	instr->category = category;
	instr->opc = opc;
	insert_instr(block->shader, instr);
	return instr;
}

struct ir3_instruction * ir3_instr_clone(struct ir3_instruction *instr)
{
	struct ir3_instruction *new_instr =
			ir3_alloc(instr->block->shader, sizeof(struct ir3_instruction));
	unsigned i;

	*new_instr = *instr;
	insert_instr(instr->block->shader, new_instr);

	/* clone registers: */
	new_instr->regs_count = 0;
	for (i = 0; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		struct ir3_register *new_reg =
				ir3_reg_create(new_instr, reg->num, reg->flags);
		*new_reg = *reg;
	}

	return new_instr;
}

struct ir3_register * ir3_reg_create(struct ir3_instruction *instr,
		int num, int flags)
{
	struct ir3_register *reg = reg_create(instr->block->shader, num, flags);
	assert(instr->regs_count < ARRAY_SIZE(instr->regs));
	instr->regs[instr->regs_count++] = reg;
	return reg;
}
