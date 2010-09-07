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
#include "r600_asm.h"
#include "r600_context.h"
#include "util/u_memory.h"
#include "r600_sq.h"
#include <stdio.h>
#include <errno.h>

int r700_bc_alu_build(struct r600_bc *bc, struct r600_bc_alu *alu, unsigned id);

static struct r600_bc_cf *r600_bc_cf(void)
{
	struct r600_bc_cf *cf = CALLOC_STRUCT(r600_bc_cf);

	if (cf == NULL)
		return NULL;
	LIST_INITHEAD(&cf->list);
	LIST_INITHEAD(&cf->alu);
	LIST_INITHEAD(&cf->vtx);
	LIST_INITHEAD(&cf->tex);
	return cf;
}

static struct r600_bc_alu *r600_bc_alu(void)
{
	struct r600_bc_alu *alu = CALLOC_STRUCT(r600_bc_alu);

	if (alu == NULL)
		return NULL;
	LIST_INITHEAD(&alu->list);
	return alu;
}

static struct r600_bc_vtx *r600_bc_vtx(void)
{
	struct r600_bc_vtx *vtx = CALLOC_STRUCT(r600_bc_vtx);

	if (vtx == NULL)
		return NULL;
	LIST_INITHEAD(&vtx->list);
	return vtx;
}

static struct r600_bc_tex *r600_bc_tex(void)
{
	struct r600_bc_tex *tex = CALLOC_STRUCT(r600_bc_tex);

	if (tex == NULL)
		return NULL;
	LIST_INITHEAD(&tex->list);
	return tex;
}

int r600_bc_init(struct r600_bc *bc, enum radeon_family family)
{
	LIST_INITHEAD(&bc->cf);
	bc->family = family;
	switch (bc->family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		bc->chiprev = 0;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		bc->chiprev = 1;
		break;
	default:
		R600_ERR("unknown family %d\n", bc->family);
		return -EINVAL;
	}
	return 0;
}

static int r600_bc_add_cf(struct r600_bc *bc)
{
	struct r600_bc_cf *cf = r600_bc_cf();

	if (cf == NULL)
		return -ENOMEM;
	LIST_ADDTAIL(&cf->list, &bc->cf);
	if (bc->cf_last)
		cf->id = bc->cf_last->id + 2;
	bc->cf_last = cf;
	bc->ncf++;
	bc->ndw += 2;
	bc->force_add_cf = 0;
	return 0;
}

int r600_bc_add_output(struct r600_bc *bc, const struct r600_bc_output *output)
{
	int r;

	r = r600_bc_add_cf(bc);
	if (r)
		return r;
	bc->cf_last->inst = output->inst;
	memcpy(&bc->cf_last->output, output, sizeof(struct r600_bc_output));
	return 0;
}

int r600_bc_add_alu_type(struct r600_bc *bc, const struct r600_bc_alu *alu, int type)
{
	struct r600_bc_alu *nalu = r600_bc_alu();
	struct r600_bc_alu *lalu;
	int i, r;

	if (nalu == NULL)
		return -ENOMEM;
	memcpy(nalu, alu, sizeof(struct r600_bc_alu));
	nalu->nliteral = 0;

	/* cf can contains only alu or only vtx or only tex */
	if (bc->cf_last == NULL || bc->cf_last->inst != (type << 3) ||
		bc->force_add_cf) {
		/* at most 128 slots, one add alu can add 4 slots + 4 constant worst case */
		r = r600_bc_add_cf(bc);
		if (r) {
			free(nalu);
			return r;
		}
		bc->cf_last->inst = (type << 3);
	}
	if (alu->last && (bc->cf_last->ndw >> 1) >= 124) {
		bc->force_add_cf = 1;
	}
	/* number of gpr == the last gpr used in any alu */
	for (i = 0; i < 3; i++) {
		if (alu->src[i].sel >= bc->ngpr && alu->src[i].sel < 128) {
			bc->ngpr = alu->src[i].sel + 1;
		}
		/* compute how many literal are needed
		 * either 2 or 4 literals
		 */
		if (alu->src[i].sel == 253) {
			if (((alu->src[i].chan + 2) & 0x6) > nalu->nliteral) {
				nalu->nliteral = (alu->src[i].chan + 2) & 0x6;
			}
		}
	}
	if (!LIST_IS_EMPTY(&bc->cf_last->alu)) {
		lalu = LIST_ENTRY(struct r600_bc_alu, bc->cf_last->alu.prev, list);
		if (!lalu->last && lalu->nliteral > nalu->nliteral) {
			nalu->nliteral = lalu->nliteral;
		}
	}
	if (alu->dst.sel >= bc->ngpr) {
		bc->ngpr = alu->dst.sel + 1;
	}
	LIST_ADDTAIL(&nalu->list, &bc->cf_last->alu);
	/* each alu use 2 dwords */
	bc->cf_last->ndw += 2;
	bc->ndw += 2;

	if (bc->use_mem_constant)
		bc->cf_last->kcache0_mode = 2;

	return 0;
}

int r600_bc_add_alu(struct r600_bc *bc, const struct r600_bc_alu *alu)
{
	return r600_bc_add_alu_type(bc, alu, V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU);
}

int r600_bc_add_literal(struct r600_bc *bc, const u32 *value)
{
	struct r600_bc_alu *alu;

	if (bc->cf_last == NULL) {
		return 0;
	}
	if (bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_TEX) {
		return 0;
	}
	if (bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_JUMP ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_ELSE ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_POP) {
		return 0;
	}
	if (((bc->cf_last->inst != (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3)) &&
	     (bc->cf_last->inst != (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3))) ||
		LIST_IS_EMPTY(&bc->cf_last->alu)) {
		R600_ERR("last CF is not ALU (%p)\n", bc->cf_last);
		return -EINVAL;
	}
	alu = LIST_ENTRY(struct r600_bc_alu, bc->cf_last->alu.prev, list);
	if (!alu->last || !alu->nliteral || alu->literal_added) {
		return 0;
	}
	memcpy(alu->value, value, 4 * 4);
	bc->cf_last->ndw += alu->nliteral;
	bc->ndw += alu->nliteral;
	alu->literal_added = 1;
	return 0;
}

int r600_bc_add_vtx(struct r600_bc *bc, const struct r600_bc_vtx *vtx)
{
	struct r600_bc_vtx *nvtx = r600_bc_vtx();
	int r;

	if (nvtx == NULL)
		return -ENOMEM;
	memcpy(nvtx, vtx, sizeof(struct r600_bc_vtx));

	/* cf can contains only alu or only vtx or only tex */
	if (bc->cf_last == NULL ||
		(bc->cf_last->inst != V_SQ_CF_WORD1_SQ_CF_INST_VTX &&
		 bc->cf_last->inst != V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC)) {
		r = r600_bc_add_cf(bc);
		if (r) {
			free(nvtx);
			return r;
		}
		bc->cf_last->inst = V_SQ_CF_WORD1_SQ_CF_INST_VTX;
	}
	LIST_ADDTAIL(&nvtx->list, &bc->cf_last->vtx);
	/* each fetch use 4 dwords */
	bc->cf_last->ndw += 4;
	bc->ndw += 4;
	return 0;
}

int r600_bc_add_tex(struct r600_bc *bc, const struct r600_bc_tex *tex)
{
	struct r600_bc_tex *ntex = r600_bc_tex();
	int r;

	if (ntex == NULL)
		return -ENOMEM;
	memcpy(ntex, tex, sizeof(struct r600_bc_tex));

	/* cf can contains only alu or only vtx or only tex */
	if (bc->cf_last == NULL ||
		bc->cf_last->inst != V_SQ_CF_WORD1_SQ_CF_INST_TEX) {
		r = r600_bc_add_cf(bc);
		if (r) {
			free(ntex);
			return r;
		}
		bc->cf_last->inst = V_SQ_CF_WORD1_SQ_CF_INST_TEX;
	}
	LIST_ADDTAIL(&ntex->list, &bc->cf_last->tex);
	/* each texture fetch use 4 dwords */
	bc->cf_last->ndw += 4;
	bc->ndw += 4;
	return 0;
}

int r600_bc_add_cfinst(struct r600_bc *bc, int inst)
{
	int r;
	r = r600_bc_add_cf(bc);
	if (r)
		return r;

	bc->cf_last->cond = V_SQ_CF_COND_ACTIVE;
	bc->cf_last->inst = inst;
	return 0;
}

static int r600_bc_vtx_build(struct r600_bc *bc, struct r600_bc_vtx *vtx, unsigned id)
{
	bc->bytecode[id++] = S_SQ_VTX_WORD0_BUFFER_ID(vtx->buffer_id) |
				S_SQ_VTX_WORD0_SRC_GPR(vtx->src_gpr) |
				S_SQ_VTX_WORD0_SRC_SEL_X(vtx->src_sel_x) |
				S_SQ_VTX_WORD0_MEGA_FETCH_COUNT(vtx->mega_fetch_count);
	bc->bytecode[id++] = S_SQ_VTX_WORD1_DST_SEL_X(vtx->dst_sel_x) |
				S_SQ_VTX_WORD1_DST_SEL_Y(vtx->dst_sel_y) |
				S_SQ_VTX_WORD1_DST_SEL_Z(vtx->dst_sel_z) |
				S_SQ_VTX_WORD1_DST_SEL_W(vtx->dst_sel_w) |
				S_SQ_VTX_WORD1_USE_CONST_FIELDS(1) |
				S_SQ_VTX_WORD1_GPR_DST_GPR(vtx->dst_gpr);
	bc->bytecode[id++] = S_SQ_VTX_WORD2_MEGA_FETCH(1);
	bc->bytecode[id++] = 0;
	return 0;
}

static int r600_bc_tex_build(struct r600_bc *bc, struct r600_bc_tex *tex, unsigned id)
{
	bc->bytecode[id++] = S_SQ_TEX_WORD0_TEX_INST(tex->inst) |
				S_SQ_TEX_WORD0_RESOURCE_ID(tex->resource_id) |
				S_SQ_TEX_WORD0_SRC_GPR(tex->src_gpr) |
				S_SQ_TEX_WORD0_SRC_REL(tex->src_rel);
	bc->bytecode[id++] = S_SQ_TEX_WORD1_DST_GPR(tex->dst_gpr) |
				S_SQ_TEX_WORD1_DST_REL(tex->dst_rel) |
				S_SQ_TEX_WORD1_DST_SEL_X(tex->dst_sel_x) |
				S_SQ_TEX_WORD1_DST_SEL_Y(tex->dst_sel_y) |
				S_SQ_TEX_WORD1_DST_SEL_Z(tex->dst_sel_z) |
				S_SQ_TEX_WORD1_DST_SEL_W(tex->dst_sel_w) |
				S_SQ_TEX_WORD1_LOD_BIAS(tex->lod_bias) |
				S_SQ_TEX_WORD1_COORD_TYPE_X(tex->coord_type_x) |
				S_SQ_TEX_WORD1_COORD_TYPE_Y(tex->coord_type_y) |
				S_SQ_TEX_WORD1_COORD_TYPE_Z(tex->coord_type_z) |
				S_SQ_TEX_WORD1_COORD_TYPE_W(tex->coord_type_w);
	bc->bytecode[id++] = S_SQ_TEX_WORD2_OFFSET_X(tex->offset_x) |
				S_SQ_TEX_WORD2_OFFSET_Y(tex->offset_y) |
				S_SQ_TEX_WORD2_OFFSET_Z(tex->offset_z) |
				S_SQ_TEX_WORD2_SAMPLER_ID(tex->sampler_id) |
				S_SQ_TEX_WORD2_SRC_SEL_X(tex->src_sel_x) |
				S_SQ_TEX_WORD2_SRC_SEL_Y(tex->src_sel_y) |
				S_SQ_TEX_WORD2_SRC_SEL_Z(tex->src_sel_z) |
				S_SQ_TEX_WORD2_SRC_SEL_W(tex->src_sel_w);
	bc->bytecode[id++] = 0;
	return 0;
}

static int r600_bc_alu_build(struct r600_bc *bc, struct r600_bc_alu *alu, unsigned id)
{
	unsigned i;

	/* don't replace gpr by pv or ps for destination register */
	bc->bytecode[id++] = S_SQ_ALU_WORD0_SRC0_SEL(alu->src[0].sel) |
				S_SQ_ALU_WORD0_SRC0_REL(alu->src[0].rel) |
				S_SQ_ALU_WORD0_SRC0_CHAN(alu->src[0].chan) |
				S_SQ_ALU_WORD0_SRC0_NEG(alu->src[0].neg) |
				S_SQ_ALU_WORD0_SRC1_SEL(alu->src[1].sel) |
				S_SQ_ALU_WORD0_SRC1_REL(alu->src[1].rel) |
				S_SQ_ALU_WORD0_SRC1_CHAN(alu->src[1].chan) |
				S_SQ_ALU_WORD0_SRC1_NEG(alu->src[1].neg) |
				S_SQ_ALU_WORD0_LAST(alu->last);

	if (alu->is_op3) {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
					S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
					S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP3_SRC2_SEL(alu->src[2].sel) |
					S_SQ_ALU_WORD1_OP3_SRC2_REL(alu->src[2].rel) |
					S_SQ_ALU_WORD1_OP3_SRC2_CHAN(alu->src[2].chan) |
					S_SQ_ALU_WORD1_OP3_SRC2_NEG(alu->src[2].neg) |
					S_SQ_ALU_WORD1_OP3_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(0);
	} else {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
					S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
					S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP2_SRC0_ABS(alu->src[0].abs) |
					S_SQ_ALU_WORD1_OP2_SRC1_ABS(alu->src[1].abs) |
					S_SQ_ALU_WORD1_OP2_WRITE_MASK(alu->dst.write) |
					S_SQ_ALU_WORD1_OP2_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(0) |
			                S_SQ_ALU_WORD1_OP2_UPDATE_EXECUTE_MASK(alu->predicate) |
		 	                S_SQ_ALU_WORD1_OP2_UPDATE_PRED(alu->predicate);
	}
	if (alu->last) {
		if (alu->nliteral && !alu->literal_added) {
			R600_ERR("Bug in ALU processing for instruction 0x%08x, literal not added correctly\n", alu->inst);
		}
		for (i = 0; i < alu->nliteral; i++) {
			bc->bytecode[id++] = alu->value[i];
		}
	}
	return 0;
}

static int r600_bc_cf_build(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	unsigned id = cf->id;

	switch (cf->inst) {
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
		bc->bytecode[id++] = S_SQ_CF_ALU_WORD0_ADDR(cf->addr >> 1) |
			S_SQ_CF_ALU_WORD0_KCACHE_MODE0(cf->kcache0_mode);

		bc->bytecode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(cf->inst >> 3) |
					S_SQ_CF_ALU_WORD1_BARRIER(1) |
					S_SQ_CF_ALU_WORD1_COUNT((cf->ndw / 2) - 1);
		break;
	case V_SQ_CF_WORD1_SQ_CF_INST_TEX:
	case V_SQ_CF_WORD1_SQ_CF_INST_VTX:
	case V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
					S_SQ_CF_WORD1_BARRIER(1) |
					S_SQ_CF_WORD1_COUNT((cf->ndw / 4) - 1);
		break;
	case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
	case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
		bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(cf->output.gpr) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(cf->output.elem_size) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(cf->output.array_base) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(cf->output.type);
		bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(cf->output.swizzle_x) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(cf->output.swizzle_y) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(cf->output.swizzle_z) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(cf->output.swizzle_w) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(cf->output.barrier) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(cf->output.inst) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(cf->output.end_of_program);
		break;
	case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
	case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
	case V_SQ_CF_WORD1_SQ_CF_INST_POP:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->cf_addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
					S_SQ_CF_WORD1_BARRIER(1) |
			                S_SQ_CF_WORD1_COND(cf->cond) |
			                S_SQ_CF_WORD1_POP_COUNT(cf->pop_count);

		break;
	default:
		R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
		return -EINVAL;
	}
	return 0;
}

int r600_bc_build(struct r600_bc *bc)
{
	struct r600_bc_cf *cf;
	struct r600_bc_alu *alu;
	struct r600_bc_vtx *vtx;
	struct r600_bc_tex *tex;
	unsigned addr;
	int r;

	if (bc->callstack[0].max > 0)
	    bc->nstack = ((bc->callstack[0].max + 3) >> 2) + 2;

	/* first path compute addr of each CF block */
	/* addr start after all the CF instructions */
	addr = bc->cf_last->id + 2;
	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		switch (cf->inst) {
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_TEX:
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX:
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC:
			/* fetch node need to be 16 bytes aligned*/
			addr += 3;
			addr &= 0xFFFFFFFCUL;
			break;
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
		case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
		case V_SQ_CF_WORD1_SQ_CF_INST_POP:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
			break;
		default:
			R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
			return -EINVAL;
		}
		cf->addr = addr;
		addr += cf->ndw;
		bc->ndw = cf->addr + cf->ndw;
	}
	free(bc->bytecode);
	bc->bytecode = calloc(1, bc->ndw * 4);
	if (bc->bytecode == NULL)
		return -ENOMEM;
	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		addr = cf->addr;
		r = r600_bc_cf_build(bc, cf);
		if (r)
			return r;
		switch (cf->inst) {
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
			LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
				switch(bc->chiprev) {
				case 0:
					r = r600_bc_alu_build(bc, alu, addr);
					break;
				case 1:
					r = r700_bc_alu_build(bc, alu, addr);
					break;
				default:
					R600_ERR("unknown family %d\n", bc->family);
					return -EINVAL;
				}
				if (r)
					return r;
				addr += 2;
				if (alu->last) {
					addr += alu->nliteral;
				}
			}
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX:
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC:
			LIST_FOR_EACH_ENTRY(vtx, &cf->vtx, list) {
				r = r600_bc_vtx_build(bc, vtx, addr);
				if (r)
					return r;
				addr += 4;
			}
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_TEX:
			LIST_FOR_EACH_ENTRY(tex, &cf->tex, list) {
				r = r600_bc_tex_build(bc, tex, addr);
				if (r)
					return r;
				addr += 4;
			}
			break;
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
		case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
		case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
		case V_SQ_CF_WORD1_SQ_CF_INST_POP:
			break;
		default:
			R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
			return -EINVAL;
		}
	}
	return 0;
}
