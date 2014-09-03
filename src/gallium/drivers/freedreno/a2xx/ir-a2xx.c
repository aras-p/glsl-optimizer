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

#include "ir-a2xx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "freedreno_util.h"
#include "instr-a2xx.h"

#define DEBUG_MSG(f, ...)  do { if (0) DBG(f, ##__VA_ARGS__); } while (0)
#define WARN_MSG(f, ...)   DBG("WARN:  "f, ##__VA_ARGS__)
#define ERROR_MSG(f, ...)  DBG("ERROR: "f, ##__VA_ARGS__)

#define REG_MASK 0x3f

static int cf_emit(struct ir2_cf *cf, instr_cf_t *instr);

static int instr_emit(struct ir2_instruction *instr, uint32_t *dwords,
		uint32_t idx, struct ir2_shader_info *info);

static void reg_update_stats(struct ir2_register *reg,
		struct ir2_shader_info *info, bool dest);
static uint32_t reg_fetch_src_swiz(struct ir2_register *reg, uint32_t n);
static uint32_t reg_fetch_dst_swiz(struct ir2_register *reg);
static uint32_t reg_alu_dst_swiz(struct ir2_register *reg);
static uint32_t reg_alu_src_swiz(struct ir2_register *reg);

/* simple allocator to carve allocations out of an up-front allocated heap,
 * so that we can free everything easily in one shot.
 */
static void * ir2_alloc(struct ir2_shader *shader, int sz)
{
	void *ptr = &shader->heap[shader->heap_idx];
	shader->heap_idx += align(sz, 4);
	return ptr;
}

static char * ir2_strdup(struct ir2_shader *shader, const char *str)
{
	char *ptr = NULL;
	if (str) {
		int len = strlen(str);
		ptr = ir2_alloc(shader, len+1);
		memcpy(ptr, str, len);
		ptr[len] = '\0';
	}
	return ptr;
}

struct ir2_shader * ir2_shader_create(void)
{
	DEBUG_MSG("");
	return calloc(1, sizeof(struct ir2_shader));
}

void ir2_shader_destroy(struct ir2_shader *shader)
{
	DEBUG_MSG("");
	free(shader);
}

/* resolve addr/cnt/sequence fields in the individual CF's */
static int shader_resolve(struct ir2_shader *shader, struct ir2_shader_info *info)
{
	uint32_t addr;
	unsigned i;
	int j;

	addr = shader->cfs_count / 2;
	for (i = 0; i < shader->cfs_count; i++) {
		struct ir2_cf *cf = shader->cfs[i];
		if ((cf->cf_type == EXEC) || (cf->cf_type == EXEC_END)) {
			uint32_t sequence = 0;

			if (cf->exec.addr && (cf->exec.addr != addr))
				WARN_MSG("invalid addr '%d' at CF %d", cf->exec.addr, i);
			if (cf->exec.cnt && (cf->exec.cnt != cf->exec.instrs_count))
				WARN_MSG("invalid cnt '%d' at CF %d", cf->exec.cnt, i);

			for (j = cf->exec.instrs_count - 1; j >= 0; j--) {
				struct ir2_instruction *instr = cf->exec.instrs[j];
				sequence <<= 2;
				if (instr->instr_type == IR2_FETCH)
					sequence |= 0x1;
				if (instr->sync)
					sequence |= 0x2;
			}

			cf->exec.addr = addr;
			cf->exec.cnt  = cf->exec.instrs_count;
			cf->exec.sequence = sequence;

			addr += cf->exec.instrs_count;
		}
	}

	info->sizedwords = 3 * addr;

	return 0;
}

void * ir2_shader_assemble(struct ir2_shader *shader, struct ir2_shader_info *info)
{
	uint32_t i, j;
	uint32_t *ptr, *dwords = NULL;
	uint32_t idx = 0;
	int ret;

	info->sizedwords    = 0;
	info->max_reg       = -1;
	info->max_input_reg = 0;
	info->regs_written  = 0;

	/* we need an even # of CF's.. insert a NOP if needed */
	if (shader->cfs_count != align(shader->cfs_count, 2))
		ir2_cf_create(shader, NOP);

	/* first pass, resolve sizes and addresses: */
	ret = shader_resolve(shader, info);
	if (ret) {
		ERROR_MSG("resolve failed: %d", ret);
		goto fail;
	}

	ptr = dwords = calloc(4, info->sizedwords);

	/* second pass, emit CF program in pairs: */
	for (i = 0; i < shader->cfs_count; i += 2) {
		instr_cf_t *cfs = (instr_cf_t *)ptr;
		ret = cf_emit(shader->cfs[i], &cfs[0]);
		if (ret) {
			ERROR_MSG("CF emit failed: %d\n", ret);
			goto fail;
		}
		ret = cf_emit(shader->cfs[i+1], &cfs[1]);
		if (ret) {
			ERROR_MSG("CF emit failed: %d\n", ret);
			goto fail;
		}
		ptr += 3;
		assert((ptr - dwords) <= info->sizedwords);
	}

	/* third pass, emit ALU/FETCH: */
	for (i = 0; i < shader->cfs_count; i++) {
		struct ir2_cf *cf = shader->cfs[i];
		if ((cf->cf_type == EXEC) || (cf->cf_type == EXEC_END)) {
			for (j = 0; j < cf->exec.instrs_count; j++) {
				ret = instr_emit(cf->exec.instrs[j], ptr, idx++, info);
				if (ret) {
					ERROR_MSG("instruction emit failed: %d", ret);
					goto fail;
				}
				ptr += 3;
				assert((ptr - dwords) <= info->sizedwords);
			}
		}
	}

	return dwords;

fail:
	free(dwords);
	return NULL;
}


struct ir2_cf * ir2_cf_create(struct ir2_shader *shader, instr_cf_opc_t cf_type)
{
	struct ir2_cf *cf = ir2_alloc(shader, sizeof(struct ir2_cf));
	DEBUG_MSG("%d", cf_type);
	cf->shader = shader;
	cf->cf_type = cf_type;
	assert(shader->cfs_count < ARRAY_SIZE(shader->cfs));
	shader->cfs[shader->cfs_count++] = cf;
	return cf;
}


/*
 * CF instructions:
 */

static int cf_emit(struct ir2_cf *cf, instr_cf_t *instr)
{
	memset(instr, 0, sizeof(*instr));

	instr->opc = cf->cf_type;

	switch (cf->cf_type) {
	case NOP:
		break;
	case EXEC:
	case EXEC_END:
		assert(cf->exec.addr <= 0x1ff);
		assert(cf->exec.cnt <= 0x6);
		assert(cf->exec.sequence <= 0xfff);
		instr->exec.address = cf->exec.addr;
		instr->exec.count = cf->exec.cnt;
		instr->exec.serialize = cf->exec.sequence;
		break;
	case ALLOC:
		assert(cf->alloc.size <= 0xf);
		instr->alloc.size = cf->alloc.size;
		switch (cf->alloc.type) {
		case SQ_POSITION:
		case SQ_PARAMETER_PIXEL:
			instr->alloc.buffer_select = cf->alloc.type;
			break;
		default:
			ERROR_MSG("invalid alloc type: %d", cf->alloc.type);
			return -1;
		}
		break;
	case COND_EXEC:
	case COND_EXEC_END:
	case COND_PRED_EXEC:
	case COND_PRED_EXEC_END:
	case LOOP_START:
	case LOOP_END:
	case COND_CALL:
	case RETURN:
	case COND_JMP:
	case COND_EXEC_PRED_CLEAN:
	case COND_EXEC_PRED_CLEAN_END:
	case MARK_VS_FETCH_DONE:
		ERROR_MSG("TODO");
		return -1;
	}

	return 0;
}


struct ir2_instruction * ir2_instr_create(struct ir2_cf *cf, int instr_type)
{
	struct ir2_instruction *instr =
			ir2_alloc(cf->shader, sizeof(struct ir2_instruction));
	DEBUG_MSG("%d", instr_type);
	instr->shader = cf->shader;
	instr->pred = cf->shader->pred;
	instr->instr_type = instr_type;
	assert(cf->exec.instrs_count < ARRAY_SIZE(cf->exec.instrs));
	cf->exec.instrs[cf->exec.instrs_count++] = instr;
	return instr;
}


/*
 * FETCH instructions:
 */

static int instr_emit_fetch(struct ir2_instruction *instr,
		uint32_t *dwords, uint32_t idx,
		struct ir2_shader_info *info)
{
	instr_fetch_t *fetch = (instr_fetch_t *)dwords;
	int reg = 0;
	struct ir2_register *dst_reg = instr->regs[reg++];
	struct ir2_register *src_reg = instr->regs[reg++];

	memset(fetch, 0, sizeof(*fetch));

	reg_update_stats(dst_reg, info, true);
	reg_update_stats(src_reg, info, false);

	fetch->opc = instr->fetch.opc;

	if (instr->fetch.opc == VTX_FETCH) {
		instr_fetch_vtx_t *vtx = &fetch->vtx;

		assert(instr->fetch.stride <= 0xff);
		assert(instr->fetch.fmt <= 0x3f);
		assert(instr->fetch.const_idx <= 0x1f);
		assert(instr->fetch.const_idx_sel <= 0x3);

		vtx->src_reg = src_reg->num;
		vtx->src_swiz = reg_fetch_src_swiz(src_reg, 1);
		vtx->dst_reg = dst_reg->num;
		vtx->dst_swiz = reg_fetch_dst_swiz(dst_reg);
		vtx->must_be_one = 1;
		vtx->const_index = instr->fetch.const_idx;
		vtx->const_index_sel = instr->fetch.const_idx_sel;
		vtx->format_comp_all = !!instr->fetch.is_signed;
		vtx->num_format_all = !instr->fetch.is_normalized;
		vtx->format = instr->fetch.fmt;
		vtx->stride = instr->fetch.stride;
		vtx->offset = instr->fetch.offset;

		if (instr->pred != IR2_PRED_NONE) {
			vtx->pred_select = 1;
			vtx->pred_condition = (instr->pred == IR2_PRED_EQ) ? 1 : 0;
		}

		/* XXX seems like every FETCH but the first has
		 * this bit set:
		 */
		vtx->reserved3 = (idx > 0) ? 0x1 : 0x0;
		vtx->reserved0 = (idx > 0) ? 0x2 : 0x3;
	} else if (instr->fetch.opc == TEX_FETCH) {
		instr_fetch_tex_t *tex = &fetch->tex;

		assert(instr->fetch.const_idx <= 0x1f);

		tex->src_reg = src_reg->num;
		tex->src_swiz = reg_fetch_src_swiz(src_reg, 3);
		tex->dst_reg = dst_reg->num;
		tex->dst_swiz = reg_fetch_dst_swiz(dst_reg);
		tex->const_idx = instr->fetch.const_idx;
		tex->mag_filter = TEX_FILTER_USE_FETCH_CONST;
		tex->min_filter = TEX_FILTER_USE_FETCH_CONST;
		tex->mip_filter = TEX_FILTER_USE_FETCH_CONST;
		tex->aniso_filter = ANISO_FILTER_USE_FETCH_CONST;
		tex->arbitrary_filter = ARBITRARY_FILTER_USE_FETCH_CONST;
		tex->vol_mag_filter = TEX_FILTER_USE_FETCH_CONST;
		tex->vol_min_filter = TEX_FILTER_USE_FETCH_CONST;
		tex->use_comp_lod = 1;
		tex->use_reg_lod = !instr->fetch.is_cube;
		tex->sample_location = SAMPLE_CENTER;

		if (instr->pred != IR2_PRED_NONE) {
			tex->pred_select = 1;
			tex->pred_condition = (instr->pred == IR2_PRED_EQ) ? 1 : 0;
		}

	} else {
		ERROR_MSG("invalid fetch opc: %d\n", instr->fetch.opc);
		return -1;
	}

	return 0;
}

/*
 * ALU instructions:
 */

static int instr_emit_alu(struct ir2_instruction *instr, uint32_t *dwords,
		struct ir2_shader_info *info)
{
	int reg = 0;
	instr_alu_t *alu = (instr_alu_t *)dwords;
	struct ir2_register *dst_reg  = instr->regs[reg++];
	struct ir2_register *src1_reg;
	struct ir2_register *src2_reg;
	struct ir2_register *src3_reg;

	memset(alu, 0, sizeof(*alu));

	/* handle instructions w/ 3 src operands: */
	switch (instr->alu.vector_opc) {
	case MULADDv:
	case CNDEv:
	case CNDGTEv:
	case CNDGTv:
	case DOT2ADDv:
		/* note: disassembler lists 3rd src first, ie:
		 *   MULADDv Rdst = Rsrc3 + (Rsrc1 * Rsrc2)
		 * which is the reason for this strange ordering.
		 */
		src3_reg = instr->regs[reg++];
		break;
	default:
		src3_reg = NULL;
		break;
	}

	src1_reg = instr->regs[reg++];
	src2_reg = instr->regs[reg++];

	reg_update_stats(dst_reg, info, true);
	reg_update_stats(src1_reg, info, false);
	reg_update_stats(src2_reg, info, false);

	assert((dst_reg->flags & ~IR2_REG_EXPORT) == 0);
	assert(!dst_reg->swizzle || (strlen(dst_reg->swizzle) == 4));
	assert((src1_reg->flags & IR2_REG_EXPORT) == 0);
	assert(!src1_reg->swizzle || (strlen(src1_reg->swizzle) == 4));
	assert((src2_reg->flags & IR2_REG_EXPORT) == 0);
	assert(!src2_reg->swizzle || (strlen(src2_reg->swizzle) == 4));

	if (instr->alu.vector_opc == ~0) {
		alu->vector_opc          = MAXv;
		alu->vector_write_mask   = 0;
	} else {
		alu->vector_opc          = instr->alu.vector_opc;
		alu->vector_write_mask   = reg_alu_dst_swiz(dst_reg);
	}

	alu->vector_dest         = dst_reg->num;
	alu->export_data         = !!(dst_reg->flags & IR2_REG_EXPORT);

	// TODO predicate case/condition.. need to add to parser

	alu->src2_reg            = src2_reg->num;
	alu->src2_swiz           = reg_alu_src_swiz(src2_reg);
	alu->src2_reg_negate     = !!(src2_reg->flags & IR2_REG_NEGATE);
	alu->src2_reg_abs        = !!(src2_reg->flags & IR2_REG_ABS);
	alu->src2_sel            = !(src2_reg->flags & IR2_REG_CONST);

	alu->src1_reg            = src1_reg->num;
	alu->src1_swiz           = reg_alu_src_swiz(src1_reg);
	alu->src1_reg_negate     = !!(src1_reg->flags & IR2_REG_NEGATE);
	alu->src1_reg_abs        = !!(src1_reg->flags & IR2_REG_ABS);
	alu->src1_sel            = !(src1_reg->flags & IR2_REG_CONST);

	alu->vector_clamp        = instr->alu.vector_clamp;
	alu->scalar_clamp        = instr->alu.scalar_clamp;

	if (instr->alu.scalar_opc != ~0) {
		struct ir2_register *sdst_reg = instr->regs[reg++];

		reg_update_stats(sdst_reg, info, true);

		assert(sdst_reg->flags == dst_reg->flags);

		if (src3_reg) {
			assert(src3_reg == instr->regs[reg++]);
		} else {
			src3_reg = instr->regs[reg++];
		}

		alu->scalar_dest         = sdst_reg->num;
		alu->scalar_write_mask   = reg_alu_dst_swiz(sdst_reg);
		alu->scalar_opc          = instr->alu.scalar_opc;
	} else {
		/* not sure if this is required, but adreno compiler seems
		 * to always set scalar opc to MAXs if it is not used:
		 */
		alu->scalar_opc = MAXs;
	}

	if (src3_reg) {
		reg_update_stats(src3_reg, info, false);

		alu->src3_reg            = src3_reg->num;
		alu->src3_swiz           = reg_alu_src_swiz(src3_reg);
		alu->src3_reg_negate     = !!(src3_reg->flags & IR2_REG_NEGATE);
		alu->src3_reg_abs        = !!(src3_reg->flags & IR2_REG_ABS);
		alu->src3_sel            = !(src3_reg->flags & IR2_REG_CONST);
	} else {
		/* not sure if this is required, but adreno compiler seems
		 * to always set register bank for 3rd src if unused:
		 */
		alu->src3_sel = 1;
	}

	if (instr->pred != IR2_PRED_NONE) {
		alu->pred_select = (instr->pred == IR2_PRED_EQ) ? 3 : 2;
	}

	return 0;
}

static int instr_emit(struct ir2_instruction *instr, uint32_t *dwords,
		uint32_t idx, struct ir2_shader_info *info)
{
	switch (instr->instr_type) {
	case IR2_FETCH: return instr_emit_fetch(instr, dwords, idx, info);
	case IR2_ALU:   return instr_emit_alu(instr, dwords, info);
	}
	return -1;
}


struct ir2_register * ir2_reg_create(struct ir2_instruction *instr,
		int num, const char *swizzle, int flags)
{
	struct ir2_register *reg =
			ir2_alloc(instr->shader, sizeof(struct ir2_register));
	DEBUG_MSG("%x, %d, %s", flags, num, swizzle);
	assert(num <= REG_MASK);
	reg->flags = flags;
	reg->num = num;
	reg->swizzle = ir2_strdup(instr->shader, swizzle);
	assert(instr->regs_count < ARRAY_SIZE(instr->regs));
	instr->regs[instr->regs_count++] = reg;
	return reg;
}

static void reg_update_stats(struct ir2_register *reg,
		struct ir2_shader_info *info, bool dest)
{
	if (!(reg->flags & (IR2_REG_CONST|IR2_REG_EXPORT))) {
		info->max_reg = MAX2(info->max_reg, reg->num);

		if (dest) {
			info->regs_written |= (1 << reg->num);
		} else if (!(info->regs_written & (1 << reg->num))) {
			/* for registers that haven't been written, they must be an
			 * input register that the thread scheduler (presumably?)
			 * needs to know about:
			 */
			info->max_input_reg = MAX2(info->max_input_reg, reg->num);
		}
	}
}

static uint32_t reg_fetch_src_swiz(struct ir2_register *reg, uint32_t n)
{
	uint32_t swiz = 0;
	int i;

	assert(reg->flags == 0);
	assert(reg->swizzle);

	DEBUG_MSG("fetch src R%d.%s", reg->num, reg->swizzle);

	for (i = n-1; i >= 0; i--) {
		swiz <<= 2;
		switch (reg->swizzle[i]) {
		default:
			ERROR_MSG("invalid fetch src swizzle: %s", reg->swizzle);
		case 'x': swiz |= 0x0; break;
		case 'y': swiz |= 0x1; break;
		case 'z': swiz |= 0x2; break;
		case 'w': swiz |= 0x3; break;
		}
	}

	return swiz;
}

static uint32_t reg_fetch_dst_swiz(struct ir2_register *reg)
{
	uint32_t swiz = 0;
	int i;

	assert(reg->flags == 0);
	assert(!reg->swizzle || (strlen(reg->swizzle) == 4));

	DEBUG_MSG("fetch dst R%d.%s", reg->num, reg->swizzle);

	if (reg->swizzle) {
		for (i = 3; i >= 0; i--) {
			swiz <<= 3;
			switch (reg->swizzle[i]) {
			default:
				ERROR_MSG("invalid dst swizzle: %s", reg->swizzle);
			case 'x': swiz |= 0x0; break;
			case 'y': swiz |= 0x1; break;
			case 'z': swiz |= 0x2; break;
			case 'w': swiz |= 0x3; break;
			case '0': swiz |= 0x4; break;
			case '1': swiz |= 0x5; break;
			case '_': swiz |= 0x7; break;
			}
		}
	} else {
		swiz = 0x688;
	}

	return swiz;
}

/* actually, a write-mask */
static uint32_t reg_alu_dst_swiz(struct ir2_register *reg)
{
	uint32_t swiz = 0;
	int i;

	assert((reg->flags & ~IR2_REG_EXPORT) == 0);
	assert(!reg->swizzle || (strlen(reg->swizzle) == 4));

	DEBUG_MSG("alu dst R%d.%s", reg->num, reg->swizzle);

	if (reg->swizzle) {
		for (i = 3; i >= 0; i--) {
			swiz <<= 1;
			if (reg->swizzle[i] == "xyzw"[i]) {
				swiz |= 0x1;
			} else if (reg->swizzle[i] != '_') {
				ERROR_MSG("invalid dst swizzle: %s", reg->swizzle);
				break;
			}
		}
	} else {
		swiz = 0xf;
	}

	return swiz;
}

static uint32_t reg_alu_src_swiz(struct ir2_register *reg)
{
	uint32_t swiz = 0;
	int i;

	assert((reg->flags & IR2_REG_EXPORT) == 0);
	assert(!reg->swizzle || (strlen(reg->swizzle) == 4));

	DEBUG_MSG("vector src R%d.%s", reg->num, reg->swizzle);

	if (reg->swizzle) {
		for (i = 3; i >= 0; i--) {
			swiz <<= 2;
			switch (reg->swizzle[i]) {
			default:
				ERROR_MSG("invalid vector src swizzle: %s", reg->swizzle);
			case 'x': swiz |= (0x0 - i) & 0x3; break;
			case 'y': swiz |= (0x1 - i) & 0x3; break;
			case 'z': swiz |= (0x2 - i) & 0x3; break;
			case 'w': swiz |= (0x3 - i) & 0x3; break;
			}
		}
	} else {
		swiz = 0x0;
	}

	return swiz;
}
