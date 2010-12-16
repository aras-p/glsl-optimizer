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
#include <stdio.h>
#include <errno.h>
#include "util/u_format.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "r600_pipe.h"
#include "r600_sq.h"
#include "r600_opcodes.h"
#include "r600_asm.h"
#include "r600_formats.h"
#include "r600d.h"

static inline unsigned int r600_bc_get_num_operands(struct r600_bc_alu *alu)
{
	if(alu->is_op3)
		return 3;

	switch (alu->inst) {
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP:
		return 0;
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL: 
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE: 
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4_IEEE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE:
		return 2;

	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV: 
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_FLOOR:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN:
	case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS:
		return 1;
	default: R600_ERR(
		"Need instruction operand number for 0x%x.\n", alu->inst); 
	};

	return 3;
}

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
	LIST_INITHEAD(&alu->bs_list);
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
		bc->chiprev = CHIPREV_R600;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		bc->chiprev = CHIPREV_R700;
		break;
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
	case CHIP_PALM:
		bc->chiprev = CHIPREV_EVERGREEN;
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

const unsigned bank_swizzle_vec[8] = {SQ_ALU_VEC_210,  //000
				      SQ_ALU_VEC_120,  //001
				      SQ_ALU_VEC_102,  //010

				      SQ_ALU_VEC_201,  //011
				      SQ_ALU_VEC_012,  //100
				      SQ_ALU_VEC_021,  //101

				      SQ_ALU_VEC_012,  //110
				      SQ_ALU_VEC_012}; //111

const unsigned bank_swizzle_scl[8] = {SQ_ALU_SCL_210,  //000
				      SQ_ALU_SCL_122,  //001
				      SQ_ALU_SCL_122,  //010

				      SQ_ALU_SCL_221,  //011
				      SQ_ALU_SCL_212,  //100
				      SQ_ALU_SCL_122,  //101

				      SQ_ALU_SCL_122,  //110
				      SQ_ALU_SCL_122}; //111

static int init_gpr(struct r600_bc_alu *alu)
{
	int cycle, component;
	/* set up gpr use */
	for (cycle = 0; cycle < NUM_OF_CYCLES; cycle++)
		for (component = 0; component < NUM_OF_COMPONENTS; component++)
			 alu->hw_gpr[cycle][component] = -1;
	return 0;
}

#if 0
static int reserve_gpr(struct r600_bc_alu *alu, unsigned sel, unsigned chan, unsigned cycle)
{
	if (alu->hw_gpr[cycle][chan] < 0)
		alu->hw_gpr[cycle][chan] = sel;
	else if (alu->hw_gpr[cycle][chan] != (int)sel) {
		R600_ERR("Another scalar operation has already used GPR read port for channel\n");
		return -1;
	}
	return 0;
}

static int cycle_for_scalar_bank_swizzle(const int swiz, const int sel, unsigned *p_cycle)
{
	int table[3];
	int ret = 0;
	switch (swiz) {
	case SQ_ALU_SCL_210:
		table[0] = 2; table[1] = 1; table[2] = 0;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_SCL_122:
		table[0] = 1; table[1] = 2; table[2] = 2;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_SCL_212:
		table[0] = 2; table[1] = 1; table[2] = 2;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_SCL_221:
		table[0] = 2; table[1] = 2; table[2] = 1;
		*p_cycle = table[sel];
                break;
		break;
	default:
		R600_ERR("bad scalar bank swizzle value\n");
		ret = -1;
		break;
	}
	return ret;
}

static int cycle_for_vector_bank_swizzle(const int swiz, const int sel, unsigned *p_cycle)
{
	int table[3];
	int ret;

	switch (swiz) {
	case SQ_ALU_VEC_012:
		table[0] = 0; table[1] = 1; table[2] = 2;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_VEC_021:
		table[0] = 0; table[1] = 2; table[2] = 1;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_VEC_120:
		table[0] = 1; table[1] = 2; table[2] = 0;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_VEC_102:
		table[0] = 1; table[1] = 0; table[2] = 2;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_VEC_201:
		table[0] = 2; table[1] = 0; table[2] = 1;
                *p_cycle = table[sel];
                break;
	case SQ_ALU_VEC_210:
		table[0] = 2; table[1] = 1; table[2] = 0;
                *p_cycle = table[sel];
                break;
	default:
		R600_ERR("bad vector bank swizzle value\n");
		ret = -1;
		break;
	}
	return ret;
}



static void update_chan_counter(struct r600_bc_alu *alu, int *chan_counter)
{
	int num_src;
	int i;
	int channel_swizzle;

	num_src = r600_bc_get_num_operands(alu);

	for (i = 0; i < num_src; i++) {
		channel_swizzle = alu->src[i].chan;
		if ((alu->src[i].sel > 0 && alu->src[i].sel < 128) && channel_swizzle <= 3)
			chan_counter[channel_swizzle]++;
	}
}

/* we need something like this I think - but this is bogus */
int check_read_slots(struct r600_bc *bc, struct r600_bc_alu *alu_first)
{
	struct r600_bc_alu *alu;
	int chan_counter[4]  = { 0 };

	update_chan_counter(alu_first, chan_counter);

	LIST_FOR_EACH_ENTRY(alu, &alu_first->bs_list, bs_list) {
		update_chan_counter(alu, chan_counter);
	}

	if (chan_counter[0] > 3 ||
	    chan_counter[1] > 3 ||
	    chan_counter[2] > 3 ||
	    chan_counter[3] > 3) {
		R600_ERR("needed to split instruction for input ran out of banks %x %d %d %d %d\n",
			 alu_first->inst, chan_counter[0], chan_counter[1], chan_counter[2], chan_counter[3]);
		return -1;
	}
	return 0;
}
#endif

static int is_const(int sel)
{
	if (sel > 255 && sel < 512)
		return 1;
	if (sel >= V_SQ_ALU_SRC_0 && sel <= V_SQ_ALU_SRC_LITERAL)
		return 1;
	return 0;
}

static int check_scalar(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	unsigned swizzle_key;

	if (alu->bank_swizzle_force) {
		alu->bank_swizzle = alu->bank_swizzle_force;
		return 0;
	}
	swizzle_key = (is_const(alu->src[0].sel) ? 4 : 0 ) + 
		(is_const(alu->src[1].sel) ? 2 : 0 ) + 
		(is_const(alu->src[2].sel) ? 1 : 0 );

	alu->bank_swizzle = bank_swizzle_scl[swizzle_key];
	return 0;
}

static int check_vector(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	unsigned swizzle_key;

	if (alu->bank_swizzle_force) {
		alu->bank_swizzle = alu->bank_swizzle_force;
		return 0;
	}
	swizzle_key = (is_const(alu->src[0].sel) ? 4 : 0 ) + 
		(is_const(alu->src[1].sel) ? 2 : 0 ) + 
		(is_const(alu->src[2].sel) ? 1 : 0 );

	alu->bank_swizzle = bank_swizzle_vec[swizzle_key];
	return 0;
}

static int check_and_set_bank_swizzle(struct r600_bc *bc, struct r600_bc_alu *alu_first)
{
	struct r600_bc_alu *alu = NULL;
	int num_instr = 1;

	init_gpr(alu_first);

	LIST_FOR_EACH_ENTRY(alu, &alu_first->bs_list, bs_list) {
		num_instr++;
	}

	if (num_instr == 1) {
		check_scalar(bc, alu_first);
		
	} else {
/*		check_read_slots(bc, bc->cf_last->curr_bs_head);*/
		check_vector(bc, alu_first);
		LIST_FOR_EACH_ENTRY(alu, &alu_first->bs_list, bs_list) {
			check_vector(bc, alu);
		}
	}
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
		r = r600_bc_add_cf(bc);
		if (r) {
			free(nalu);
			return r;
		}
		bc->cf_last->inst = (type << 3);
	}
	if (!bc->cf_last->curr_bs_head) {
		bc->cf_last->curr_bs_head = nalu;
		LIST_INITHEAD(&nalu->bs_list);
	} else {
		LIST_ADDTAIL(&nalu->bs_list, &bc->cf_last->curr_bs_head->bs_list);
	}
	/* at most 128 slots, one add alu can add 4 slots + 4 constants(2 slots)
	 * worst case */
	if (alu->last && (bc->cf_last->ndw >> 1) >= 120) {
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

	bc->cf_last->kcache0_mode = 2;

	/* process cur ALU instructions for bank swizzle */
	if (alu->last) {
		check_and_set_bank_swizzle(bc, bc->cf_last->curr_bs_head);
		bc->cf_last->curr_bs_head = NULL;
	}
	return 0;
}

int r600_bc_add_alu(struct r600_bc *bc, const struct r600_bc_alu *alu)
{
	return r600_bc_add_alu_type(bc, alu, BC_INST(bc, V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU));
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
	/* all same on EG */
	if (bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_JUMP ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_ELSE ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END ||
	    bc->cf_last->inst == V_SQ_CF_WORD1_SQ_CF_INST_POP) {
		return 0;
	}
	/* same on EG */
	if (((bc->cf_last->inst != (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3)) &&
	     (bc->cf_last->inst != (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3)) &&
	     (bc->cf_last->inst != (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3)) &&
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
		 bc->cf_last->inst != V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) ||
	         bc->force_add_cf) {
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
	if ((bc->ndw / 4) > 7)
		bc->force_add_cf = 1;
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
		bc->cf_last->inst != V_SQ_CF_WORD1_SQ_CF_INST_TEX ||
	        bc->force_add_cf) {
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
	if ((bc->ndw / 4) > 7)
		bc->force_add_cf = 1;
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

/* common to all 3 families */
static int r600_bc_vtx_build(struct r600_bc *bc, struct r600_bc_vtx *vtx, unsigned id)
{
	unsigned fetch_resource_start = 0;

	/* check if we are fetch shader */
			/* fetch shader can also access vertex resource,
			 * first fetch shader resource is at 160
			 */
	if (bc->type == -1) {
		switch (bc->chiprev) {
		/* r600 */
		case CHIPREV_R600:
		/* r700 */
		case CHIPREV_R700:
			fetch_resource_start = 160;
			break;
		/* evergreen */
		case CHIPREV_EVERGREEN:
			fetch_resource_start = 0;
			break;
		default:
			fprintf(stderr,  "%s:%s:%d unknown chiprev %d\n",
				__FILE__, __func__, __LINE__, bc->chiprev);
			break;
		}
	}
	bc->bytecode[id++] = S_SQ_VTX_WORD0_BUFFER_ID(vtx->buffer_id + fetch_resource_start) |
			S_SQ_VTX_WORD0_SRC_GPR(vtx->src_gpr) |
			S_SQ_VTX_WORD0_SRC_SEL_X(vtx->src_sel_x) |
			S_SQ_VTX_WORD0_MEGA_FETCH_COUNT(vtx->mega_fetch_count);
	bc->bytecode[id++] = S_SQ_VTX_WORD1_DST_SEL_X(vtx->dst_sel_x) |
				S_SQ_VTX_WORD1_DST_SEL_Y(vtx->dst_sel_y) |
				S_SQ_VTX_WORD1_DST_SEL_Z(vtx->dst_sel_z) |
				S_SQ_VTX_WORD1_DST_SEL_W(vtx->dst_sel_w) |
				S_SQ_VTX_WORD1_USE_CONST_FIELDS(vtx->use_const_fields) |
				S_SQ_VTX_WORD1_DATA_FORMAT(vtx->data_format) |
				S_SQ_VTX_WORD1_NUM_FORMAT_ALL(vtx->num_format_all) |
				S_SQ_VTX_WORD1_FORMAT_COMP_ALL(vtx->format_comp_all) |
				S_SQ_VTX_WORD1_SRF_MODE_ALL(vtx->srf_mode_all) |
				S_SQ_VTX_WORD1_GPR_DST_GPR(vtx->dst_gpr);
	bc->bytecode[id++] = S_SQ_VTX_WORD2_MEGA_FETCH(1);
	bc->bytecode[id++] = 0;
	return 0;
}

/* common to all 3 families */
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

/* r600 only, r700/eg bits in r700_asm.c */
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
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle);
	} else {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
					S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
					S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP2_SRC0_ABS(alu->src[0].abs) |
					S_SQ_ALU_WORD1_OP2_SRC1_ABS(alu->src[1].abs) |
					S_SQ_ALU_WORD1_OP2_WRITE_MASK(alu->dst.write) |
					S_SQ_ALU_WORD1_OP2_OMOD(alu->omod) |
					S_SQ_ALU_WORD1_OP2_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle) |
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

/* common for r600/r700 - eg in eg_asm.c */
static int r600_bc_cf_build(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	unsigned id = cf->id;

	switch (cf->inst) {
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
		bc->bytecode[id++] = S_SQ_CF_ALU_WORD0_ADDR(cf->addr >> 1) |
			S_SQ_CF_ALU_WORD0_KCACHE_MODE0(cf->kcache0_mode) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK0(cf->kcache0_bank) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK1(cf->kcache1_bank);

		bc->bytecode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(cf->inst >> 3) |
			S_SQ_CF_ALU_WORD1_KCACHE_MODE1(cf->kcache1_mode) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR0(cf->kcache0_addr) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR1(cf->kcache1_addr) |
					S_SQ_CF_ALU_WORD1_BARRIER(1) |
					S_SQ_CF_ALU_WORD1_USES_WATERFALL(bc->chiprev == CHIPREV_R600 ? cf->r6xx_uses_waterfall : 0) |
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
	case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
	case V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
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
	if (bc->type == TGSI_PROCESSOR_VERTEX && !bc->nstack) {
		bc->nstack = 1;
	}

	/* first path compute addr of each CF block */
	/* addr start after all the CF instructions */
	addr = bc->cf_last->id + 2;
	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		switch (cf->inst) {
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
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
		case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
		case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
		case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
		case V_SQ_CF_WORD1_SQ_CF_INST_POP:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
		case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
		case V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
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
		if (bc->chiprev == CHIPREV_EVERGREEN)
			r = eg_bc_cf_build(bc, cf);
		else
			r = r600_bc_cf_build(bc, cf);
		if (r)
			return r;
		switch (cf->inst) {
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
			LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
				switch(bc->chiprev) {
				case CHIPREV_R600:
					r = r600_bc_alu_build(bc, alu, addr);
					break;
				case CHIPREV_R700:
				case CHIPREV_EVERGREEN: /* eg alu is same encoding as r700 */
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
		case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
		case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
		case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
		case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
		case V_SQ_CF_WORD1_SQ_CF_INST_POP:
		case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
		case V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
			break;
		default:
			R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
			return -EINVAL;
		}
	}
	return 0;
}

void r600_bc_clear(struct r600_bc *bc)
{
	struct r600_bc_cf *cf = NULL, *next_cf;

	free(bc->bytecode);
	bc->bytecode = NULL;

	LIST_FOR_EACH_ENTRY_SAFE(cf, next_cf, &bc->cf, list) {
		struct r600_bc_alu *alu = NULL, *next_alu;
		struct r600_bc_tex *tex = NULL, *next_tex;
		struct r600_bc_tex *vtx = NULL, *next_vtx;

		LIST_FOR_EACH_ENTRY_SAFE(alu, next_alu, &cf->alu, list) {
			free(alu);
		}

		LIST_INITHEAD(&cf->alu);

		LIST_FOR_EACH_ENTRY_SAFE(tex, next_tex, &cf->tex, list) {
			free(tex);
		}

		LIST_INITHEAD(&cf->tex);

		LIST_FOR_EACH_ENTRY_SAFE(vtx, next_vtx, &cf->vtx, list) {
			free(vtx);
		}

		LIST_INITHEAD(&cf->vtx);

		free(cf);
	}

	LIST_INITHEAD(&cf->list);
}

void r600_bc_dump(struct r600_bc *bc)
{
	struct r600_bc_cf *cf;
	struct r600_bc_alu *alu;
	struct r600_bc_vtx *vtx;
	struct r600_bc_tex *tex;

	unsigned i, id;
	char chip = '6';

	switch (bc->chiprev) {
	case 1:
		chip = '7';
		break;
	case 2:
		chip = 'E';
		break;
	case 0:
	default:
		chip = '6';
		break;
	}
	fprintf(stderr, "bytecode %d dw -----------------------\n", bc->ndw);
	fprintf(stderr, "     %c\n", chip);

	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		id = cf->id;

		switch (cf->inst) {
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
		case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
			fprintf(stderr, "%04d %08X ALU ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%d ", cf->addr);
			fprintf(stderr, "KCACHE_MODE0:%X ", cf->kcache0_mode);
			fprintf(stderr, "KCACHE_BANK0:%X ", cf->kcache0_bank);
			fprintf(stderr, "KCACHE_BANK1:%X\n", cf->kcache1_bank);
			id++;
			fprintf(stderr, "%04d %08X ALU ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "KCACHE_MODE1:%X ", cf->kcache1_mode);
			fprintf(stderr, "KCACHE_ADDR0:%X ", cf->kcache0_addr);
			fprintf(stderr, "KCACHE_ADDR1:%X ", cf->kcache1_addr);
			fprintf(stderr, "COUNT:%d\n", cf->ndw / 2);
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_TEX:
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX:
		case V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC:
			fprintf(stderr, "%04d %08X TEX/VTX ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%d\n", cf->addr);
			id++;
			fprintf(stderr, "%04d %08X TEX/VTX ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "COUNT:%d\n", cf->ndw / 4);
			break;
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
		case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
			fprintf(stderr, "%04d %08X EXPORT ", id, bc->bytecode[id]);
			fprintf(stderr, "GPR:%X ", cf->output.gpr);
			fprintf(stderr, "ELEM_SIZE:%X ", cf->output.elem_size);
			fprintf(stderr, "ARRAY_BASE:%X ", cf->output.array_base);
			fprintf(stderr, "TYPE:%X\n", cf->output.type);
			id++;
			fprintf(stderr, "%04d %08X EXPORT ", id, bc->bytecode[id]);
			fprintf(stderr, "SWIZ_X:%X ", cf->output.swizzle_x);
			fprintf(stderr, "SWIZ_Y:%X ", cf->output.swizzle_y);
			fprintf(stderr, "SWIZ_Z:%X ", cf->output.swizzle_z);
			fprintf(stderr, "SWIZ_W:%X ", cf->output.swizzle_w);
			fprintf(stderr, "SWIZ_W:%X ", cf->output.swizzle_w);
			fprintf(stderr, "BARRIER:%X ", cf->output.barrier);
			fprintf(stderr, "INST:%d ", cf->output.inst);
			fprintf(stderr, "EOP:%X\n", cf->output.end_of_program);
			break;
		case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
		case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
		case V_SQ_CF_WORD1_SQ_CF_INST_POP:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
		case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
		case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
		case V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
			fprintf(stderr, "%04d %08X CF ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%d\n", cf->cf_addr);
			id++;
			fprintf(stderr, "%04d %08X CF ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "COND:%X ", cf->cond);
			fprintf(stderr, "POP_COUNT:%X\n", cf->pop_count);
			break;
		}

		LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
			id = cf->addr;
			fprintf(stderr, "%04d %08X\t", id, bc->bytecode[id]);
			fprintf(stderr, "SRC0(SEL:%d ", alu->src[0].sel);
			fprintf(stderr, "REL:%d ", alu->src[0].rel);
			fprintf(stderr, "CHAN:%d ", alu->src[0].chan);
			fprintf(stderr, "NEG:%d) ", alu->src[0].neg);
			fprintf(stderr, "SRC1(SEL:%d ", alu->src[1].sel);
			fprintf(stderr, "REL:%d ", alu->src[1].rel);
			fprintf(stderr, "CHAN:%d ", alu->src[1].chan);
			fprintf(stderr, "NEG:%d) ", alu->src[1].neg);
			fprintf(stderr, "LAST:%d)\n", alu->last);
			id++;
			if (alu->is_op3) {
				fprintf(stderr, "%04d %08X\t", id, bc->bytecode[id]);
				fprintf(stderr, "DST(SEL:%d ", alu->dst.sel);
				fprintf(stderr, "CHAN:%d ", alu->dst.chan);
				fprintf(stderr, "REL:%d ", alu->dst.rel);
				fprintf(stderr, "CLAMP:%d) ", alu->dst.clamp);
				fprintf(stderr, "SRC2(SEL:%d ", alu->src[2].sel);
				fprintf(stderr, "REL:%d ", alu->src[2].rel);
				fprintf(stderr, "CHAN:%d ", alu->src[2].chan);
				fprintf(stderr, "NEG:%d) ", alu->src[2].neg);
				fprintf(stderr, "INST:%d ", alu->inst);
				fprintf(stderr, "BANK_SWIZZLE:%d\n", alu->bank_swizzle);
			} else {
				fprintf(stderr, "%04d %08X\t", id, bc->bytecode[id]);
				fprintf(stderr, "DST(SEL:%d ", alu->dst.sel);
				fprintf(stderr, "CHAN:%d ", alu->dst.chan);
				fprintf(stderr, "REL:%d ", alu->dst.rel);
				fprintf(stderr, "CLAMP:%d) ", alu->dst.clamp);
				fprintf(stderr, "SRC0_ABS:%d ", alu->src[0].abs);
				fprintf(stderr, "SRC1_ABS:%d ", alu->src[1].abs);
				fprintf(stderr, "WRITE_MASK:%d ", alu->dst.write);
				fprintf(stderr, "OMOD:%d ", alu->omod);
				fprintf(stderr, "INST:%d ", alu->inst);
				fprintf(stderr, "BANK_SWIZZLE:%d ", alu->bank_swizzle);
				fprintf(stderr, "EXECUTE_MASK:%d ", alu->predicate);
				fprintf(stderr, "UPDATE_PRED:%d\n", alu->predicate);
			}

			if (alu->last) {
				for (i = 0; i < alu->nliteral; i++) {
					float *f = (float*)(bc->bytecode + id);
					fprintf(stderr, "%04d %08X %f\n", id, bc->bytecode[id], *f);
				}
			}
		}

		LIST_FOR_EACH_ENTRY(tex, &cf->tex, list) {
			//TODO
		}

		LIST_FOR_EACH_ENTRY(vtx, &cf->vtx, list) {
			//TODO
		}
	}

	fprintf(stderr, "--------------------------------------\n");
}

void r600_cf_vtx(struct r600_vertex_element *ve, u32 *bytecode, unsigned count)
{
	struct r600_pipe_state *rstate;
	unsigned i = 0;

	if (count > 8) {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT(8 - 1);
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(40 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT(count - 8 - 1);
	} else {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT(count - 1);
	}
	bytecode[i++] = S_SQ_CF_WORD0_ADDR(0);
	bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_RETURN) |
			S_SQ_CF_WORD1_BARRIER(1);

	rstate = &ve->rstate;
	rstate->id = R600_PIPE_STATE_FETCH_SHADER;
	rstate->nregs = 0;
	r600_pipe_state_add_reg(rstate, R_0288A4_SQ_PGM_RESOURCES_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288DC_SQ_PGM_CF_OFFSET_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028894_SQ_PGM_START_FS,
				r600_bo_offset(ve->fetch_shader) >> 8,
				0xFFFFFFFF, ve->fetch_shader);
}

void r600_cf_vtx_tc(struct r600_vertex_element *ve, u32 *bytecode, unsigned count)
{
	struct r600_pipe_state *rstate;
	unsigned i = 0;

	if (count > 8) {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT(8 - 1);
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(40 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT((count - 8) - 1);
	} else {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) |
						S_SQ_CF_WORD1_BARRIER(1) |
						S_SQ_CF_WORD1_COUNT(count - 1);
	}
	bytecode[i++] = S_SQ_CF_WORD0_ADDR(0);
	bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_RETURN) |
			S_SQ_CF_WORD1_BARRIER(1);

	rstate = &ve->rstate;
	rstate->id = R600_PIPE_STATE_FETCH_SHADER;
	rstate->nregs = 0;
	r600_pipe_state_add_reg(rstate, R_0288A4_SQ_PGM_RESOURCES_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288DC_SQ_PGM_CF_OFFSET_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028894_SQ_PGM_START_FS,
				r600_bo_offset(ve->fetch_shader) >> 8,
				0xFFFFFFFF, ve->fetch_shader);
}

static void r600_vertex_data_type(enum pipe_format pformat, unsigned *format,
				unsigned *num_format, unsigned *format_comp)
{
	const struct util_format_description *desc;
	unsigned i;

	*format = 0;
	*num_format = 0;
	*format_comp = 0;

	desc = util_format_description(pformat);
	if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
		goto out_unknown;
	}

	/* Find the first non-VOID channel. */
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	switch (desc->channel[i].type) {
		/* Half-floats, floats, doubles */
	case UTIL_FORMAT_TYPE_FLOAT:
		switch (desc->channel[i].size) {
		case 16:
			switch (desc->nr_channels) {
			case 1:
				*format = FMT_16_FLOAT;
				break;
			case 2:
				*format = FMT_16_16_FLOAT;
				break;
			case 3:
				*format = FMT_16_16_16_FLOAT;
				break;
			case 4:
				*format = FMT_16_16_16_16_FLOAT;
				break;
			}
			break;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				*format = FMT_32_FLOAT;
				break;
			case 2:
				*format = FMT_32_32_FLOAT;
				break;
			case 3:
				*format = FMT_32_32_32_FLOAT;
				break;
			case 4:
				*format = FMT_32_32_32_32_FLOAT;
				break;
			}
			break;
		default:
			goto out_unknown;
		}
		break;
		/* Unsigned ints */
	case UTIL_FORMAT_TYPE_UNSIGNED:
		/* Signed ints */
	case UTIL_FORMAT_TYPE_SIGNED:
		switch (desc->channel[i].size) {
		case 8:
			switch (desc->nr_channels) {
			case 1:
				*format = FMT_8;
				break;
			case 2:
				*format = FMT_8_8;
				break;
			case 3:
			//	*format = FMT_8_8_8; /* fails piglit draw-vertices test */
			//	break;
			case 4:
				*format = FMT_8_8_8_8;
				break;
			}
			break;
		case 16:
			switch (desc->nr_channels) {
			case 1:
				*format = FMT_16;
				break;
			case 2:
				*format = FMT_16_16;
				break;
			case 3:
			//	*format = FMT_16_16_16; /* fails piglit draw-vertices test */
			//	break;
			case 4:
				*format = FMT_16_16_16_16;
				break;
			}
			break;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				*format = FMT_32;
				break;
			case 2:
				*format = FMT_32_32;
				break;
			case 3:
				*format = FMT_32_32_32;
				break;
			case 4:
				*format = FMT_32_32_32_32;
				break;
			}
			break;
		default:
			goto out_unknown;
		}
		break;
	default:
		goto out_unknown;
	}

	if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
		*format_comp = 1;
	}
	if (desc->channel[i].normalized) {
		*num_format = 0;
	} else {
		*num_format = 2;
	}
	return;
out_unknown:
	R600_ERR("unsupported vertex format %s\n", util_format_name(pformat));
}

int r600_vertex_elements_build_fetch_shader(struct r600_pipe_context *rctx, struct r600_vertex_element *ve)
{
	unsigned ndw, i;
	u32 *bytecode;
	unsigned fetch_resource_start = 0, format, num_format, format_comp;
	struct pipe_vertex_element *elements = ve->elements;
	const struct util_format_description *desc;

	/* 2 dwords for cf aligned to 4 + 4 dwords per input */
	ndw = 8 + ve->count * 4;
	ve->fs_size = ndw * 4;

	/* use PIPE_BIND_VERTEX_BUFFER so we use the cache buffer manager */
	ve->fetch_shader = r600_bo(rctx->radeon, ndw*4, 256, PIPE_BIND_VERTEX_BUFFER, 0);
	if (ve->fetch_shader == NULL) {
		return -ENOMEM;
	}

	bytecode = r600_bo_map(rctx->radeon, ve->fetch_shader, 0, NULL);
	if (bytecode == NULL) {
		r600_bo_reference(rctx->radeon, &ve->fetch_shader, NULL);
		return -ENOMEM;
	}

	if (rctx->family >= CHIP_CEDAR) {
		eg_cf_vtx(ve, &bytecode[0], (ndw - 8) / 4);
	} else {
		r600_cf_vtx(ve, &bytecode[0], (ndw - 8) / 4);
		fetch_resource_start = 160;
	}

	/* vertex elements offset need special handling, if offset is bigger
	 * than what we can put in fetch instruction then we need to alterate
	 * the vertex resource offset. In such case in order to simplify code
	 * we will bound one resource per elements. It's a worst case scenario.
	 */
	for (i = 0; i < ve->count; i++) {
		ve->vbuffer_offset[i] = C_SQ_VTX_WORD2_OFFSET & elements[i].src_offset;
		if (ve->vbuffer_offset[i]) {
			ve->vbuffer_need_offset = 1;
		}
	}

	for (i = 0; i < ve->count; i++) {
		unsigned vbuffer_index;
		r600_vertex_data_type(ve->hw_format[i], &format, &num_format, &format_comp);
		desc = util_format_description(ve->hw_format[i]);
		if (desc == NULL) {
			R600_ERR("unknown format %d\n", ve->hw_format[i]);
			r600_bo_reference(rctx->radeon, &ve->fetch_shader, NULL);
			return -EINVAL;
		}

		/* see above for vbuffer_need_offset explanation */
		vbuffer_index = elements[i].vertex_buffer_index;
		if (ve->vbuffer_need_offset) {
			bytecode[8 + i * 4 + 0] = S_SQ_VTX_WORD0_BUFFER_ID(i + fetch_resource_start);
		} else {
			bytecode[8 + i * 4 + 0] = S_SQ_VTX_WORD0_BUFFER_ID(vbuffer_index + fetch_resource_start);
		}
		bytecode[8 + i * 4 + 0] |= S_SQ_VTX_WORD0_SRC_GPR(0) |
					S_SQ_VTX_WORD0_SRC_SEL_X(0) |
					S_SQ_VTX_WORD0_MEGA_FETCH_COUNT(0x1F);
		bytecode[8 + i * 4 + 1] = S_SQ_VTX_WORD1_DST_SEL_X(desc->swizzle[0]) |
					S_SQ_VTX_WORD1_DST_SEL_Y(desc->swizzle[1]) |
					S_SQ_VTX_WORD1_DST_SEL_Z(desc->swizzle[2]) |
					S_SQ_VTX_WORD1_DST_SEL_W(desc->swizzle[3]) |
					S_SQ_VTX_WORD1_USE_CONST_FIELDS(0) |
					S_SQ_VTX_WORD1_DATA_FORMAT(format) |
					S_SQ_VTX_WORD1_NUM_FORMAT_ALL(num_format) |
					S_SQ_VTX_WORD1_FORMAT_COMP_ALL(format_comp) |
					S_SQ_VTX_WORD1_SRF_MODE_ALL(1) |
					S_SQ_VTX_WORD1_GPR_DST_GPR(i + 1);
		bytecode[8 + i * 4 + 2] = S_SQ_VTX_WORD2_OFFSET(elements[i].src_offset) |
					S_SQ_VTX_WORD2_MEGA_FETCH(1);
		bytecode[8 + i * 4 + 3] = 0;
	}
	r600_bo_unmap(rctx->radeon, ve->fetch_shader);
	return 0;
}
