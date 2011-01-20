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

#define NUM_OF_CYCLES 3
#define NUM_OF_COMPONENTS 4

#define PREV_ALU(alu) LIST_ENTRY(struct r600_bc_alu, alu->list.prev, list)
#define NEXT_ALU(alu) LIST_ENTRY(struct r600_bc_alu, alu->list.next, list)

static inline unsigned int r600_bc_get_num_operands(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	if(alu->is_op3)
		return 3;

	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
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
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_FLOOR:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN:
		case V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS:
			return 1;
		default: R600_ERR(
			"Need instruction operand number for 0x%x.\n", alu->inst);
		}
		break;
	case CHIPREV_EVERGREEN:
		switch (alu->inst) {
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP:
			return 0;
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4_IEEE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INTERP_XY:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INTERP_ZW:
			return 2;

		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT_FLOOR:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN:
		case EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS:
			return 1;
		default: R600_ERR(
			"Need instruction operand number for 0x%x.\n", alu->inst);
		}
		break;
	}

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
	cf->barrier = 1;
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
	case CHIP_BARTS:
	case CHIP_TURKS:
	case CHIP_CAICOS:
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

static void r600_bc_remove_cf(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	struct r600_bc_cf *other;
	LIST_FOR_EACH_ENTRY(other, &bc->cf, list) {
		if (other->id > cf->id)
			other->id -= 2;
		if (other->cf_addr > cf->id)
			other->cf_addr -= 2;
	}
	LIST_DEL(&cf->list);
	free(cf);
}

static void r600_bc_move_cf(struct r600_bc *bc, struct r600_bc_cf *cf, struct r600_bc_cf *next)
{
	struct r600_bc_cf *prev = LIST_ENTRY(struct r600_bc_cf, next->list.prev, list);
	unsigned old_id = cf->id;
	unsigned new_id = prev->id + 2;
	struct r600_bc_cf *other;

	if (prev == cf)
		return; /* position hasn't changed */

	LIST_DEL(&cf->list);
	LIST_FOR_EACH_ENTRY(other, &bc->cf, list) {
		if (other->id > old_id)
			other->id -= 2;
		if (other->id >= new_id)
			other->id += 2;
		if (other->cf_addr > old_id)
			other->cf_addr -= 2;
		if (other->cf_addr > new_id)
			other->cf_addr += 2;
	}
	cf->id = new_id;
	LIST_ADD(&cf->list, &prev->list);
}

int r600_bc_add_output(struct r600_bc *bc, const struct r600_bc_output *output)
{
	int r;

	r = r600_bc_add_cf(bc);
	if (r)
		return r;
	bc->cf_last->inst = BC_INST(bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
	memcpy(&bc->cf_last->output, output, sizeof(struct r600_bc_output));
	bc->cf_last->output.burst_count = 1;
	return 0;
}

/* alu predicate instructions */
static int is_alu_pred_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
		return !alu->is_op3 && (
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_UINT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_UINT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_INV ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_POP ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_CLR ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_RESTORE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLT_PUSH_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLE_PUSH_INT);
	case CHIPREV_EVERGREEN:
	default:
		return !alu->is_op3 && (
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_UINT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_UINT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_INV ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_POP ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_CLR ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_RESTORE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLT_PUSH_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLE_PUSH_INT);
	}
}

/* alu kill instructions */
static int is_alu_kill_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
		return !alu->is_op3 && (
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_UINT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_UINT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_INT ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE_INT);
	case CHIPREV_EVERGREEN:
	default:
		return !alu->is_op3 && (
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_UINT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_UINT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_INT ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE_INT);
	}
}

/* alu instructions that can ony exits once per group */
static int is_alu_once_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	return is_alu_kill_inst(bc, alu) ||
		is_alu_pred_inst(bc, alu);
}

static int is_alu_reduction_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
		return !alu->is_op3 && (
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4 ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4_IEEE ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX4);
	case CHIPREV_EVERGREEN:
	default:
		return !alu->is_op3 && (
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4 ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4_IEEE ||
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX4);
	}
}

static int is_alu_mova_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
		return !alu->is_op3 && (
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_FLOOR ||
			alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT);
	case CHIPREV_EVERGREEN:
	default:
		return !alu->is_op3 && (
			alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT);
	}
}

/* alu instructions that can only execute on the vector unit */
static int is_alu_vec_unit_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	return is_alu_reduction_inst(bc, alu) ||
		is_alu_mova_inst(bc, alu);
}

/* alu instructions that can only execute on the trans unit */
static int is_alu_trans_unit_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	switch (bc->chiprev) {
	case CHIPREV_R600:
	case CHIPREV_R700:
		if (!alu->is_op3)
			return alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_INT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_UINT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_FF ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_FF ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN ||
				alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SQRT_IEEE;
		else
			return alu->inst == V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT ||
				alu->inst == V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_D2 ||
				alu->inst == V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_M2 ||
				alu->inst == V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_M4;
	case CHIPREV_EVERGREEN:
	default:
		if (!alu->is_op3)
			return alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_INT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_UINT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_FF ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_FF ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN ||
				alu->inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SQRT_IEEE;
		else
			return alu->inst == EG_V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT;
	}
}

/* alu instructions that can execute on any unit */
static int is_alu_any_unit_inst(struct r600_bc *bc, struct r600_bc_alu *alu)
{
	return !is_alu_vec_unit_inst(bc, alu) &&
		!is_alu_trans_unit_inst(bc, alu);
}

static int assign_alu_units(struct r600_bc *bc, struct r600_bc_alu *alu_first,
			    struct r600_bc_alu *assignment[5])
{
	struct r600_bc_alu *alu;
	unsigned i, chan, trans;

	for (i = 0; i < 5; i++)
		assignment[i] = NULL;

	for (alu = alu_first; alu; alu = LIST_ENTRY(struct r600_bc_alu, alu->list.next, list)) {
		chan = alu->dst.chan;
		if (is_alu_trans_unit_inst(bc, alu))
			trans = 1;
		else if (is_alu_vec_unit_inst(bc, alu))
			trans = 0;
		else if (assignment[chan])
			trans = 1; // assume ALU_INST_PREFER_VECTOR
		else
			trans = 0;

		if (trans) {
			if (assignment[4]) {
				assert(0); //ALU.Trans has already been allocated
				return -1;
			}
			assignment[4] = alu;
		} else {
			if (assignment[chan]) {
				assert(0); //ALU.chan has already been allocated
				return -1;
			}
			assignment[chan] = alu;
		}

		if (alu->last)
			break;
	}
	return 0;
}

struct alu_bank_swizzle {
	int	hw_gpr[NUM_OF_CYCLES][NUM_OF_COMPONENTS];
	int	hw_cfile_addr[4];
	int	hw_cfile_elem[4];
};

const unsigned cycle_for_bank_swizzle_vec[][3] = {
	[SQ_ALU_VEC_012] = { 0, 1, 2 },
	[SQ_ALU_VEC_021] = { 0, 2, 1 },
	[SQ_ALU_VEC_120] = { 1, 2, 0 },
	[SQ_ALU_VEC_102] = { 1, 0, 2 },
	[SQ_ALU_VEC_201] = { 2, 0, 1 },
	[SQ_ALU_VEC_210] = { 2, 1, 0 }
};

const unsigned cycle_for_bank_swizzle_scl[][3] = {
	[SQ_ALU_SCL_210] = { 2, 1, 0 },
	[SQ_ALU_SCL_122] = { 1, 2, 2 },
	[SQ_ALU_SCL_212] = { 2, 1, 2 },
	[SQ_ALU_SCL_221] = { 2, 2, 1 }
};

static void init_bank_swizzle(struct alu_bank_swizzle *bs)
{
	int i, cycle, component;
	/* set up gpr use */
	for (cycle = 0; cycle < NUM_OF_CYCLES; cycle++)
		for (component = 0; component < NUM_OF_COMPONENTS; component++)
			 bs->hw_gpr[cycle][component] = -1;
	for (i = 0; i < 4; i++)
		bs->hw_cfile_addr[i] = -1;
	for (i = 0; i < 4; i++)
		bs->hw_cfile_elem[i] = -1;
}

static int reserve_gpr(struct alu_bank_swizzle *bs, unsigned sel, unsigned chan, unsigned cycle)
{
	if (bs->hw_gpr[cycle][chan] == -1)
		bs->hw_gpr[cycle][chan] = sel;
	else if (bs->hw_gpr[cycle][chan] != (int)sel) {
		// Another scalar operation has already used GPR read port for channel
		return -1;
	}
	return 0;
}

static int reserve_cfile(struct alu_bank_swizzle *bs, unsigned sel, unsigned chan)
{
	int res, resmatch = -1, resempty = -1;
	for (res = 3; res >= 0; --res) {
		if (bs->hw_cfile_addr[res] == -1)
			resempty = res;
		else if (bs->hw_cfile_addr[res] == sel &&
			bs->hw_cfile_elem[res] == chan)
			resmatch = res;
	}
	if (resmatch != -1)
		return 0; // Read for this scalar element already reserved, nothing to do here.
	else if (resempty != -1) {
		bs->hw_cfile_addr[resempty] = sel;
		bs->hw_cfile_elem[resempty] = chan;
	} else {
		// All cfile read ports are used, cannot reference vector element
		return -1;
	}
	return 0;
}

static int is_gpr(unsigned sel)
{
	return (sel >= 0 && sel <= 127);
}

/* CB constants start at 512, and get translated to a kcache index when ALU
 * clauses are constructed. Note that we handle kcache constants the same way
 * as (the now gone) cfile constants, is that really required? */
static int is_cfile(unsigned sel)
{
	return (sel > 255 && sel < 512) ||
		(sel > 511 && sel < 4607) || // Kcache before translate
		(sel > 127 && sel < 192); // Kcache after translate
}

static int is_const(int sel)
{
	return is_cfile(sel) ||
		(sel >= V_SQ_ALU_SRC_0 &&
		sel <= V_SQ_ALU_SRC_LITERAL);
}

static int check_vector(struct r600_bc *bc, struct r600_bc_alu *alu,
			struct alu_bank_swizzle *bs, int bank_swizzle)
{
	int r, src, num_src, sel, elem, cycle;

	num_src = r600_bc_get_num_operands(bc, alu);
	for (src = 0; src < num_src; src++) {
		sel = alu->src[src].sel;
		elem = alu->src[src].chan;
		if (is_gpr(sel)) {
			cycle = cycle_for_bank_swizzle_vec[bank_swizzle][src];
			if (src == 1 && sel == alu->src[0].sel && elem == alu->src[0].chan)
				// Nothing to do; special-case optimization,
				// second source uses first source’s reservation
				continue;
			else {
				r = reserve_gpr(bs, sel, elem, cycle);
				if (r)
					return r;
			}
		} else if (is_cfile(sel)) {
			r = reserve_cfile(bs, sel, elem);
			if (r)
				return r;
		}
		// No restrictions on PV, PS, literal or special constants
	}
	return 0;
}

static int check_scalar(struct r600_bc *bc, struct r600_bc_alu *alu,
			struct alu_bank_swizzle *bs, int bank_swizzle)
{
	int r, src, num_src, const_count, sel, elem, cycle;

	num_src = r600_bc_get_num_operands(bc, alu);
	for (const_count = 0, src = 0; src < num_src; ++src) {
		sel = alu->src[src].sel;
		elem = alu->src[src].chan;
		if (is_const(sel)) { // Any constant, including literal and inline constants
			if (const_count >= 2)
				// More than two references to a constant in
				// transcendental operation.
				return -1;
			else
				const_count++;
		}
		if (is_cfile(sel)) {
			r = reserve_cfile(bs, sel, elem);
			if (r)
				return r;
		}
	}
	for (src = 0; src < num_src; ++src) {
		sel = alu->src[src].sel;
		elem = alu->src[src].chan;
		if (is_gpr(sel)) {
			cycle = cycle_for_bank_swizzle_scl[bank_swizzle][src];
			if (cycle < const_count)
				// Cycle for GPR load conflicts with
				// constant load in transcendental operation.
				return -1;
			r = reserve_gpr(bs, sel, elem, cycle);
			if (r)
				return r;
		}
		// Constants already processed
		// No restrictions on PV, PS
	}
	return 0;
}

static int check_and_set_bank_swizzle(struct r600_bc *bc,
				      struct r600_bc_alu *slots[5])
{
	struct alu_bank_swizzle bs;
	int bank_swizzle[5];
	int i, r = 0, forced = 0;

	for (i = 0; i < 5; i++)
		if (slots[i] && slots[i]->bank_swizzle_force) {
			slots[i]->bank_swizzle = slots[i]->bank_swizzle_force;
			forced = 1;
		}

	if (forced)
		return 0;

	// just check every possible combination of bank swizzle
	// not very efficent, but works on the first try in most of the cases
	for (i = 0; i < 4; i++)
		bank_swizzle[i] = SQ_ALU_VEC_012;
	bank_swizzle[4] = SQ_ALU_SCL_210;
	while(bank_swizzle[4] <= SQ_ALU_SCL_221) {
		init_bank_swizzle(&bs);
		for (i = 0; i < 4; i++) {
			if (slots[i]) {
				r = check_vector(bc, slots[i], &bs, bank_swizzle[i]);
				if (r)
					break;
			}
		}
		if (!r && slots[4]) {
			r = check_scalar(bc, slots[4], &bs, bank_swizzle[4]);
		}
		if (!r) {
			for (i = 0; i < 5; i++) {
				if (slots[i])
					slots[i]->bank_swizzle = bank_swizzle[i];
			}
			return 0;
		}

		for (i = 0; i < 5; i++) {
			bank_swizzle[i]++;
			if (bank_swizzle[i] <= SQ_ALU_VEC_210)
				break;
			else
				bank_swizzle[i] = SQ_ALU_VEC_012;
		}
	}

	// couldn't find a working swizzle
	return -1;
}

static int replace_gpr_with_pv_ps(struct r600_bc *bc,
				  struct r600_bc_alu *slots[5], struct r600_bc_alu *alu_prev)
{
	struct r600_bc_alu *prev[5];
	int gpr[5], chan[5];
	int i, j, r, src, num_src;

	r = assign_alu_units(bc, alu_prev, prev);
	if (r)
		return r;

	for (i = 0; i < 5; ++i) {
		if(prev[i] && prev[i]->dst.write && !prev[i]->dst.rel) {
			gpr[i] = prev[i]->dst.sel;
			if (is_alu_reduction_inst(bc, prev[i]))
				chan[i] = 0;
			else
				chan[i] = prev[i]->dst.chan;
		} else
			gpr[i] = -1;
	}

	for (i = 0; i < 5; ++i) {
		struct r600_bc_alu *alu = slots[i];
		if(!alu)
			continue;

		num_src = r600_bc_get_num_operands(bc, alu);
		for (src = 0; src < num_src; ++src) {
			if (!is_gpr(alu->src[src].sel) || alu->src[src].rel)
				continue;

			if (alu->src[src].sel == gpr[4] &&
				alu->src[src].chan == chan[4]) {
				alu->src[src].sel = V_SQ_ALU_SRC_PS;
				alu->src[src].chan = 0;
				continue;
			}

			for (j = 0; j < 4; ++j) {
				if (alu->src[src].sel == gpr[j] &&
					alu->src[src].chan == j) {
					alu->src[src].sel = V_SQ_ALU_SRC_PV;
					alu->src[src].chan = chan[j];
					break;
				}
			}
		}
	}

	return 0;
}

void r600_bc_special_constants(u32 value, unsigned *sel, unsigned *neg)
{
	switch(value) {
	case 0:
		*sel = V_SQ_ALU_SRC_0;
		break;
	case 1:
		*sel = V_SQ_ALU_SRC_1_INT;
		break;
	case -1:
		*sel = V_SQ_ALU_SRC_M_1_INT;
		break;
	case 0x3F800000: // 1.0f
		*sel = V_SQ_ALU_SRC_1;
		break;
	case 0x3F000000: // 0.5f
		*sel = V_SQ_ALU_SRC_0_5;
		break;
	case 0xBF800000: // -1.0f
		*sel = V_SQ_ALU_SRC_1;
		*neg ^= 1;
		break;
	case 0xBF000000: // -0.5f
		*sel = V_SQ_ALU_SRC_0_5;
		*neg ^= 1;
		break;
	default:
		*sel = V_SQ_ALU_SRC_LITERAL;
		break;
	}
}

/* compute how many literal are needed */
static int r600_bc_alu_nliterals(struct r600_bc *bc, struct r600_bc_alu *alu,
				 uint32_t literal[4], unsigned *nliteral)
{
	unsigned num_src = r600_bc_get_num_operands(bc, alu);
	unsigned i, j;

	for (i = 0; i < num_src; ++i) {
		if (alu->src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			uint32_t value = alu->src[i].value[alu->src[i].chan];
			unsigned found = 0;
			for (j = 0; j < *nliteral; ++j) {
				if (literal[j] == value) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (*nliteral >= 4)
					return -EINVAL;
				literal[(*nliteral)++] = value;
			}
		}
	}
	return 0;
}

static void r600_bc_alu_adjust_literals(struct r600_bc *bc,
					struct r600_bc_alu *alu,
					uint32_t literal[4], unsigned nliteral)
{
	unsigned num_src = r600_bc_get_num_operands(bc, alu);
	unsigned i, j;

	for (i = 0; i < num_src; ++i) {
		if (alu->src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			uint32_t value = alu->src[i].value[alu->src[i].chan];
			for (j = 0; j < nliteral; ++j) {
				if (literal[j] == value) {
					alu->src[i].chan = j;
					break;
				}
			}
		}
	}
}

static int merge_inst_groups(struct r600_bc *bc, struct r600_bc_alu *slots[5],
			     struct r600_bc_alu *alu_prev)
{
	struct r600_bc_alu *prev[5];
	struct r600_bc_alu *result[5] = { NULL };

	uint32_t literal[4], prev_literal[4];
	unsigned nliteral = 0, prev_nliteral = 0;

	int i, j, r, src, num_src;
	int num_once_inst = 0;

	r = assign_alu_units(bc, alu_prev, prev);
	if (r)
		return r;

	for (i = 0; i < 5; ++i) {
		struct r600_bc_alu *alu;

		/* check number of literals */
		if (prev[i]) {
			if (r600_bc_alu_nliterals(bc, prev[i], literal, &nliteral))
				return 0;
			if (r600_bc_alu_nliterals(bc, prev[i], prev_literal, &prev_nliteral))
				return 0;
		}
		if (slots[i] && r600_bc_alu_nliterals(bc, slots[i], literal, &nliteral))
			return 0;

		// let's check used slots
		if (prev[i] && !slots[i]) {
			result[i] = prev[i];
			num_once_inst += is_alu_once_inst(bc, prev[i]);
			continue;
		} else if (prev[i] && slots[i]) {
			if (result[4] == NULL && prev[4] == NULL && slots[4] == NULL) {
				// trans unit is still free try to use it
				if (is_alu_any_unit_inst(bc, slots[i])) {
					result[i] = prev[i];
					result[4] = slots[i];
				} else if (is_alu_any_unit_inst(bc, prev[i])) {
					result[i] = slots[i];
					result[4] = prev[i];
				} else
					return 0;
			} else
				return 0;
		} else if(!slots[i]) {
			continue;
		} else
			result[i] = slots[i];

		// let's check source gprs
		alu = slots[i];
		num_once_inst += is_alu_once_inst(bc, alu);

		num_src = r600_bc_get_num_operands(bc, alu);
		for (src = 0; src < num_src; ++src) {
			// constants doesn't matter
			if (!is_gpr(alu->src[src].sel))
				continue;

			for (j = 0; j < 5; ++j) {
				if (!prev[j] || !prev[j]->dst.write)
					continue;

				// if it's relative then we can't determin which gpr is really used
				if (prev[j]->dst.chan == alu->src[src].chan &&
					(prev[j]->dst.sel == alu->src[src].sel ||
					prev[j]->dst.rel || alu->src[src].rel))
					return 0;
			}
		}
	}

	/* more than one PRED_ or KILL_ ? */
	if (num_once_inst > 1)
		return 0;

	/* check if the result can still be swizzlet */
	r = check_and_set_bank_swizzle(bc, result);
	if (r)
		return 0;

	/* looks like everything worked out right, apply the changes */

	/* undo adding previus literals */
	bc->cf_last->ndw -= align(prev_nliteral, 2);

	/* sort instructions */
	for (i = 0; i < 5; ++i) {
		slots[i] = result[i];
		if (result[i]) {
			LIST_DEL(&result[i]->list);
			result[i]->last = 0;
			LIST_ADDTAIL(&result[i]->list, &bc->cf_last->alu);
		}
	}

	/* determine new last instruction */
	LIST_ENTRY(struct r600_bc_alu, bc->cf_last->alu.prev, list)->last = 1;

	/* determine new first instruction */
	for (i = 0; i < 5; ++i) {
		if (result[i]) {
			bc->cf_last->curr_bs_head = result[i];
			break;
		}
	}

	bc->cf_last->prev_bs_head = bc->cf_last->prev2_bs_head;
	bc->cf_last->prev2_bs_head = NULL;

	return 0;
}

/* This code handles kcache lines as single blocks of 32 constants. We could
 * probably do slightly better by recognizing that we actually have two
 * consecutive lines of 16 constants, but the resulting code would also be
 * somewhat more complicated. */
static int r600_bc_alloc_kcache_lines(struct r600_bc *bc, struct r600_bc_alu *alu, int type)
{
	struct r600_bc_kcache *kcache = bc->cf_last->kcache;
	unsigned int required_lines;
	unsigned int free_lines = 0;
	unsigned int cache_line[3];
	unsigned int count = 0;
	unsigned int i, j;
	int r;

	/* Collect required cache lines. */
	for (i = 0; i < 3; ++i) {
		bool found = false;
		unsigned int line;

		if (alu->src[i].sel < 512)
			continue;

		line = ((alu->src[i].sel - 512) / 32) * 2;

		for (j = 0; j < count; ++j) {
			if (cache_line[j] == line) {
				found = true;
				break;
			}
		}

		if (!found)
			cache_line[count++] = line;
	}

	/* This should never actually happen. */
	if (count >= 3) return -ENOMEM;

	for (i = 0; i < 2; ++i) {
		if (kcache[i].mode == V_SQ_CF_KCACHE_NOP) {
			++free_lines;
		}
	}

	/* Filter lines pulled in by previous intructions. Note that this is
	 * only for the required_lines count, we can't remove these from the
	 * cache_line array since we may have to start a new ALU clause. */
	for (i = 0, required_lines = count; i < count; ++i) {
		for (j = 0; j < 2; ++j) {
			if (kcache[j].mode == V_SQ_CF_KCACHE_LOCK_2 &&
			    kcache[j].addr == cache_line[i]) {
				--required_lines;
				break;
			}
		}
	}

	/* Start a new ALU clause if needed. */
	if (required_lines > free_lines) {
		if ((r = r600_bc_add_cf(bc))) {
			return r;
		}
		bc->cf_last->inst = (type << 3);
		kcache = bc->cf_last->kcache;
	}

	/* Setup the kcache lines. */
	for (i = 0; i < count; ++i) {
		bool found = false;

		for (j = 0; j < 2; ++j) {
			if (kcache[j].mode == V_SQ_CF_KCACHE_LOCK_2 &&
			    kcache[j].addr == cache_line[i]) {
				found = true;
				break;
			}
		}

		if (found) continue;

		for (j = 0; j < 2; ++j) {
			if (kcache[j].mode == V_SQ_CF_KCACHE_NOP) {
				kcache[j].bank = 0;
				kcache[j].addr = cache_line[i];
				kcache[j].mode = V_SQ_CF_KCACHE_LOCK_2;
				break;
			}
		}
	}

	/* Alter the src operands to refer to the kcache. */
	for (i = 0; i < 3; ++i) {
		static const unsigned int base[] = {128, 160, 256, 288};
		unsigned int line;

		if (alu->src[i].sel < 512)
			continue;

		alu->src[i].sel -= 512;
		line = (alu->src[i].sel / 32) * 2;

		for (j = 0; j < 2; ++j) {
			if (kcache[j].mode == V_SQ_CF_KCACHE_LOCK_2 &&
			    kcache[j].addr == line) {
				alu->src[i].sel &= 0x1f;
				alu->src[i].sel += base[j];
				break;
			}
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

	if (bc->cf_last != NULL && bc->cf_last->inst != (type << 3)) {
		/* check if we could add it anyway */
		if (bc->cf_last->inst == (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3) &&
			type == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE) {
			LIST_FOR_EACH_ENTRY(lalu, &bc->cf_last->alu, list) {
				if (lalu->predicate) {
					bc->force_add_cf = 1;
					break;
				}
			}
		} else
			bc->force_add_cf = 1;
	}

	/* cf can contains only alu or only vtx or only tex */
	if (bc->cf_last == NULL || bc->force_add_cf) {
		r = r600_bc_add_cf(bc);
		if (r) {
			free(nalu);
			return r;
		}
	}
	bc->cf_last->inst = (type << 3);

	/* Setup the kcache for this ALU instruction. This will start a new
	 * ALU clause if needed. */
	if ((r = r600_bc_alloc_kcache_lines(bc, nalu, type))) {
		free(nalu);
		return r;
	}

	if (!bc->cf_last->curr_bs_head) {
		bc->cf_last->curr_bs_head = nalu;
	}
	/* replace special constants */
	for (i = 0; i < 3; i++) {
		if (nalu->src[i].sel == V_SQ_ALU_SRC_LITERAL)
			r600_bc_special_constants(
				nalu->src[i].value[nalu->src[i].chan],
				&nalu->src[i].sel, &nalu->src[i].neg);

		if (nalu->src[i].sel >= bc->ngpr && nalu->src[i].sel < 128) {
			bc->ngpr = nalu->src[i].sel + 1;
		}
	}
	if (nalu->dst.sel >= bc->ngpr) {
		bc->ngpr = nalu->dst.sel + 1;
	}

	LIST_ADDTAIL(&nalu->list, &bc->cf_last->alu);
	/* each alu use 2 dwords */
	bc->cf_last->ndw += 2;
	bc->ndw += 2;

	/* process cur ALU instructions for bank swizzle */
	if (nalu->last) {
		uint32_t literal[4];
		unsigned nliteral;
		struct r600_bc_alu *slots[5];
		r = assign_alu_units(bc, bc->cf_last->curr_bs_head, slots);
		if (r)
			return r;

		if (bc->cf_last->prev_bs_head) {
			r = merge_inst_groups(bc, slots, bc->cf_last->prev_bs_head);
			if (r)
				return r;
		}

		if (bc->cf_last->prev_bs_head) {
			r = replace_gpr_with_pv_ps(bc, slots, bc->cf_last->prev_bs_head);
			if (r)
				return r;
		}

		r = check_and_set_bank_swizzle(bc, slots);
		if (r)
			return r;

		for (i = 0, nliteral = 0; i < 5; i++) {
			if (slots[i]) {
				r = r600_bc_alu_nliterals(bc, slots[i], literal, &nliteral);
				if (r)
					return r;
			}
		}
		bc->cf_last->ndw += align(nliteral, 2);

		/* at most 128 slots, one add alu can add 5 slots + 4 constants(2 slots)
		 * worst case */
		if ((bc->cf_last->ndw >> 1) >= 120) {
			bc->force_add_cf = 1;
		}

		bc->cf_last->prev2_bs_head = bc->cf_last->prev_bs_head;
		bc->cf_last->prev_bs_head = bc->cf_last->curr_bs_head;
		bc->cf_last->curr_bs_head = NULL;
	}
	return 0;
}

int r600_bc_add_alu(struct r600_bc *bc, const struct r600_bc_alu *alu)
{
	return r600_bc_add_alu_type(bc, alu, BC_INST(bc, V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU));
}

static void r600_bc_remove_alu(struct r600_bc_cf *cf, struct r600_bc_alu *alu)
{
	if (alu->last && alu->list.prev != &cf->alu) {
		PREV_ALU(alu)->last = 1;
	}
	LIST_DEL(&alu->list);
	free(alu);
	cf->ndw -= 2;
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
	if ((bc->cf_last->ndw / 4) > 7)
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
	if (ntex->src_gpr >= bc->ngpr) {
		bc->ngpr = ntex->src_gpr + 1;
	}
	if (ntex->dst_gpr >= bc->ngpr) {
		bc->ngpr = ntex->dst_gpr + 1;
	}
	LIST_ADDTAIL(&ntex->list, &bc->cf_last->tex);
	/* each texture fetch use 4 dwords */
	bc->cf_last->ndw += 4;
	bc->ndw += 4;
	if ((bc->cf_last->ndw / 4) > 7)
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
	return 0;
}

enum cf_class
{
	CF_CLASS_ALU,
	CF_CLASS_TEXTURE,
	CF_CLASS_VERTEX,
	CF_CLASS_EXPORT,
	CF_CLASS_OTHER
};

static enum cf_class get_cf_class(struct r600_bc_cf *cf)
{
	switch (cf->inst) {
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
	case (V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
		return CF_CLASS_ALU;

	case V_SQ_CF_WORD1_SQ_CF_INST_TEX:
		return CF_CLASS_TEXTURE;

	case V_SQ_CF_WORD1_SQ_CF_INST_VTX:
	case V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC:
		return CF_CLASS_VERTEX;

	case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
	case V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
		return CF_CLASS_EXPORT;

	case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
	case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
	case V_SQ_CF_WORD1_SQ_CF_INST_POP:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
	case V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
	case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
	case V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
		return CF_CLASS_OTHER;

	default:
		R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
		return -EINVAL;
	}
}

/* common for r600/r700 - eg in eg_asm.c */
static int r600_bc_cf_build(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	unsigned id = cf->id;
	unsigned end_of_program = bc->cf.prev == &cf->list;

	switch (get_cf_class(cf)) {
	case CF_CLASS_ALU:
		assert(!end_of_program);
		bc->bytecode[id++] = S_SQ_CF_ALU_WORD0_ADDR(cf->addr >> 1) |
			S_SQ_CF_ALU_WORD0_KCACHE_MODE0(cf->kcache[0].mode) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK0(cf->kcache[0].bank) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK1(cf->kcache[1].bank);

		bc->bytecode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(cf->inst >> 3) |
			S_SQ_CF_ALU_WORD1_KCACHE_MODE1(cf->kcache[1].mode) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR0(cf->kcache[0].addr) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR1(cf->kcache[1].addr) |
			S_SQ_CF_ALU_WORD1_BARRIER(cf->barrier) |
			S_SQ_CF_ALU_WORD1_USES_WATERFALL(bc->chiprev == CHIPREV_R600 ? cf->r6xx_uses_waterfall : 0) |
			S_SQ_CF_ALU_WORD1_COUNT((cf->ndw / 2) - 1);
		break;
	case CF_CLASS_TEXTURE:
	case CF_CLASS_VERTEX:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
			S_SQ_CF_WORD1_BARRIER(cf->barrier) |
			S_SQ_CF_WORD1_COUNT((cf->ndw / 4) - 1) |
			S_SQ_CF_WORD1_END_OF_PROGRAM(end_of_program);
		break;
	case CF_CLASS_EXPORT:
		bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(cf->output.gpr) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(cf->output.elem_size) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(cf->output.array_base) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(cf->output.type);
		bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT(cf->output.burst_count - 1) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(cf->output.swizzle_x) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(cf->output.swizzle_y) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(cf->output.swizzle_z) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(cf->output.swizzle_w) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(cf->barrier) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(cf->inst) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(end_of_program);
		break;
	case CF_CLASS_OTHER:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->cf_addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
			S_SQ_CF_WORD1_BARRIER(cf->barrier) |
			S_SQ_CF_WORD1_COND(cf->cond) |
			S_SQ_CF_WORD1_POP_COUNT(cf->pop_count) |
			S_SQ_CF_WORD1_END_OF_PROGRAM(end_of_program);

		break;
	default:
		R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
		return -EINVAL;
	}
	return 0;
}

struct gpr_usage_range {
	int	replacement;
	int32_t	start;
	int32_t	end;
};

struct gpr_usage {
	unsigned		channels:4;
	int32_t			first_write;
	int32_t			last_write[4];
	unsigned	        nranges;
	struct gpr_usage_range  *ranges;
};

static struct gpr_usage_range* add_gpr_usage_range(struct gpr_usage *usage)
{
	usage->nranges++;
	usage->ranges = realloc(usage->ranges, usage->nranges * sizeof(struct gpr_usage_range));
	if (!usage->ranges)
		return NULL;
	return &usage->ranges[usage->nranges-1];
}

static void notice_gpr_read(struct gpr_usage *usage, int32_t id, unsigned chan)
{
        usage->channels |= 1 << chan;
        usage->first_write = -1;
        if (!usage->nranges) {
        	struct gpr_usage_range* range = add_gpr_usage_range(usage);
        	range->replacement = -1;
                range->start = -1;
                range->end = -1;
        }
        if (usage->ranges[usage->nranges-1].end < id)
		usage->ranges[usage->nranges-1].end = id;
}

static void notice_gpr_rel_read(struct gpr_usage usage[128], int32_t id, unsigned chan)
{
	unsigned i;
	for (i = 0; i < 128; ++i)
		notice_gpr_read(&usage[i], id, chan);
}

static void notice_gpr_last_write(struct gpr_usage *usage, int32_t id, unsigned chan)
{
        usage->last_write[chan] = id;
}

static void notice_gpr_write(struct gpr_usage *usage, int32_t id, unsigned chan,
				int predicate, int prefered_replacement)
{
	int32_t start = usage->first_write != -1 ? usage->first_write : id;
	usage->channels &= ~(1 << chan);
	if (usage->channels) {
		if (usage->first_write == -1)
			usage->first_write = id;
	} else if (!usage->nranges || (usage->ranges[usage->nranges-1].start != start && !predicate)) {
		usage->first_write = start;
		struct gpr_usage_range* range = add_gpr_usage_range(usage);
		range->replacement = prefered_replacement;
                range->start = start;
                range->end = -1;
        } else if (usage->ranges[usage->nranges-1].start == start && prefered_replacement != -1) {
        	usage->ranges[usage->nranges-1].replacement = prefered_replacement;
        }
        notice_gpr_last_write(usage, id, chan);
}

static void notice_gpr_rel_last_write(struct gpr_usage usage[128], int32_t id, unsigned chan)
{
	unsigned i;
	for (i = 0; i < 128; ++i)
		notice_gpr_last_write(&usage[i], id, chan);
}

static void notice_gpr_rel_write(struct gpr_usage usage[128], int32_t id, unsigned chan)
{
	unsigned i;
	for (i = 0; i < 128; ++i)
		notice_gpr_write(&usage[i], id, chan, 1, -1);
}

static void notice_alu_src_gprs(struct r600_bc *bc, struct r600_bc_alu *alu,
                                struct gpr_usage usage[128], int32_t id)
{
	unsigned src, num_src;

	num_src = r600_bc_get_num_operands(bc, alu);
	for (src = 0; src < num_src; ++src) {
		// constants doesn't matter
		if (!is_gpr(alu->src[src].sel))
			continue;

		if (alu->src[src].rel)
			notice_gpr_rel_read(usage, id, alu->src[src].chan);
		else
			notice_gpr_read(&usage[alu->src[src].sel], id, alu->src[src].chan);
	}
}

static void notice_alu_dst_gprs(struct r600_bc_alu *alu_first, struct gpr_usage usage[128],
				int32_t id, int predicate)
{
	struct r600_bc_alu *alu;
	for (alu = alu_first; alu; alu = LIST_ENTRY(struct r600_bc_alu, alu->list.next, list)) {
		if (alu->dst.write) {
			if (alu->dst.rel)
				notice_gpr_rel_write(usage, id, alu->dst.chan);
			else if (alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV && is_gpr(alu->src[0].sel))
				notice_gpr_write(&usage[alu->dst.sel], id, alu->dst.chan,
						predicate, alu->src[0].sel);
			else
				notice_gpr_write(&usage[alu->dst.sel], id, alu->dst.chan, predicate, -1);
		}

		if (alu->last)
			break;
	}
}

static void notice_tex_gprs(struct r600_bc_tex *tex, struct gpr_usage usage[128],
				int32_t id, int predicate)
{
	if (tex->src_rel) {
                if (tex->src_sel_x < 4)
			notice_gpr_rel_read(usage, id, tex->src_sel_x);
		if (tex->src_sel_y < 4)
			notice_gpr_rel_read(usage, id, tex->src_sel_y);
		if (tex->src_sel_z < 4)
			notice_gpr_rel_read(usage, id, tex->src_sel_z);
		if (tex->src_sel_w < 4)
			notice_gpr_rel_read(usage, id, tex->src_sel_w);
        } else {
		if (tex->src_sel_x < 4)
			notice_gpr_read(&usage[tex->src_gpr], id, tex->src_sel_x);
		if (tex->src_sel_y < 4)
			notice_gpr_read(&usage[tex->src_gpr], id, tex->src_sel_y);
		if (tex->src_sel_z < 4)
			notice_gpr_read(&usage[tex->src_gpr], id, tex->src_sel_z);
		if (tex->src_sel_w < 4)
			notice_gpr_read(&usage[tex->src_gpr], id, tex->src_sel_w);
	}
	if (tex->dst_rel) {
		if (tex->dst_sel_x != 7)
			notice_gpr_rel_write(usage, id, 0);
		if (tex->dst_sel_y != 7)
			notice_gpr_rel_write(usage, id, 1);
		if (tex->dst_sel_z != 7)
			notice_gpr_rel_write(usage, id, 2);
		if (tex->dst_sel_w != 7)
			notice_gpr_rel_write(usage, id, 3);
	} else {
		if (tex->dst_sel_x != 7)
			notice_gpr_write(&usage[tex->dst_gpr], id, 0, predicate, -1);
		if (tex->dst_sel_y != 7)
			notice_gpr_write(&usage[tex->dst_gpr], id, 1, predicate, -1);
		if (tex->dst_sel_z != 7)
			notice_gpr_write(&usage[tex->dst_gpr], id, 2, predicate, -1);
		if (tex->dst_sel_w != 7)
			notice_gpr_write(&usage[tex->dst_gpr], id, 3, predicate, -1);
	}
}

static void notice_vtx_gprs(struct r600_bc_vtx *vtx, struct gpr_usage usage[128],
				int32_t id, int predicate)
{
	notice_gpr_read(&usage[vtx->src_gpr], id, vtx->src_sel_x);

	if (vtx->dst_sel_x != 7)
		notice_gpr_write(&usage[vtx->dst_gpr], id, 0, predicate, -1);
	if (vtx->dst_sel_y != 7)
		notice_gpr_write(&usage[vtx->dst_gpr], id, 1, predicate, -1);
	if (vtx->dst_sel_z != 7)
		notice_gpr_write(&usage[vtx->dst_gpr], id, 2, predicate, -1);
	if (vtx->dst_sel_w != 7)
		notice_gpr_write(&usage[vtx->dst_gpr], id, 3, predicate, -1);
}

static void notice_export_gprs(struct r600_bc_cf *cf, struct gpr_usage usage[128],
				struct r600_bc_cf *export_cf[128], int32_t export_remap[128])
{
	//TODO handle other memory operations
	struct gpr_usage *output = &usage[cf->output.gpr];
	int32_t id = (output->last_write[0] + 0x100) & ~0xFF;

	export_cf[cf->output.gpr] = cf;
	export_remap[cf->output.gpr] = id;
	if (cf->output.swizzle_x < 4)
		notice_gpr_read(output, id, cf->output.swizzle_x);
	if (cf->output.swizzle_y < 4)
		notice_gpr_read(output, id, cf->output.swizzle_y);
	if (cf->output.swizzle_z < 4)
		notice_gpr_read(output, id, cf->output.swizzle_z);
	if (cf->output.swizzle_w < 4)
		notice_gpr_read(output, id, cf->output.swizzle_w);
}

static struct gpr_usage_range *find_src_range(struct gpr_usage *usage, int32_t id)
{
	unsigned i;
	for (i = 0; i < usage->nranges; ++i) {
		struct gpr_usage_range* range = &usage->ranges[i];

		if (range->start < id && id <= range->end)
			return range;
	}
	return NULL;
}

static struct gpr_usage_range *find_dst_range(struct gpr_usage *usage, int32_t id)
{
	unsigned i;
	for (i = 0; i < usage->nranges; ++i) {
		struct gpr_usage_range* range = &usage->ranges[i];
		int32_t end = range->end;

		if (range->start <= id && (id < end || end == -1))
			return range;
	}
	assert(0); /* should not happen */
	return NULL;
}

static int is_barrier_needed(struct gpr_usage *usage, int32_t id, unsigned chan, int32_t last_barrier)
{
	if (usage->last_write[chan] != (id & ~0xFF))
		return usage->last_write[chan] >= last_barrier;
	else
		return 0;
}

static int is_intersection(struct gpr_usage_range* a, struct gpr_usage_range* b)
{
	return a->start <= b->end && b->start < a->end;
}

static int rate_replacement(struct gpr_usage *usage, struct gpr_usage_range* range)
{
	unsigned i;
	int32_t best_start = 0x3FFFFFFF, best_end = 0x3FFFFFFF;

	for (i = 0; i < usage->nranges; ++i) {
		if (usage->ranges[i].replacement != -1)
			continue; /* ignore already remapped ranges */

		if (is_intersection(&usage->ranges[i], range))
			return -1; /* forget it if usages overlap */

		if (range->start >= usage->ranges[i].end)
			best_start = MIN2(best_start, range->start - usage->ranges[i].end);

		if (range->end != -1 && range->end <= usage->ranges[i].start)
			best_end = MIN2(best_end, usage->ranges[i].start - range->end);
	}
	return best_start + best_end;
}

static void find_replacement(struct gpr_usage usage[128], unsigned current,
				struct gpr_usage_range *range, int is_export)
{
	unsigned i;
	int best_gpr = -1, best_rate = 0x7FFFFFFF;

	if (range->replacement != -1 && range->replacement <= current) {
		struct gpr_usage_range *other = find_src_range(&usage[range->replacement], range->start);
		if (other && other->replacement != -1)
			range->replacement = other->replacement;
	}

	if (range->replacement != -1 && range->replacement < current) {
		int rate = rate_replacement(&usage[range->replacement], range);

		/* check if prefered replacement can be used */
		if (rate != -1) {
			best_rate = rate;
			best_gpr = range->replacement;
		}
	}

	if (best_gpr == -1 && (range->start & ~0xFF) == (range->end & ~0xFF)) {
		/* register is just used inside one ALU clause */
		/* try to use clause temporaryis for it */
		for (i = 127; i > 123; --i) {
			int rate = rate_replacement(&usage[i], range);

			if (rate == -1) /* can't be used because ranges overlap */
				continue;

			if (rate < best_rate) {
				best_rate = rate;
				best_gpr = i;

				/* can't get better than this */
				if (rate == 0 || is_export)
					break;
			}
		}
	}

	if (best_gpr == -1) {
		for (i = 0; i < current; ++i) {
			int rate = rate_replacement(&usage[i], range);

			if (rate == -1) /* can't be used because ranges overlap */
				continue;

			if (rate < best_rate) {
				best_rate = rate;
				best_gpr = i;

				/* can't get better than this */
				if (rate == 0)
					break;
			}
		}
	}

	range->replacement = best_gpr;
	if (best_gpr != -1) {
		struct gpr_usage_range *reservation = add_gpr_usage_range(&usage[best_gpr]);
		reservation->replacement = -1;
		reservation->start = range->start;
		reservation->end = range->end;
	}
}

static void find_export_replacement(struct gpr_usage usage[128],
				struct gpr_usage_range *range, struct r600_bc_cf *current,
				struct r600_bc_cf *next, int32_t next_id)
{
	if (!next || next_id <= range->start || next_id > range->end)
		return;

	if (current->output.type != next->output.type)
		return;

	if ((current->output.array_base + 1) != next->output.array_base)
		return;

	find_src_range(&usage[next->output.gpr], next_id)->replacement = range->replacement + 1;
}

static void replace_alu_gprs(struct r600_bc *bc, struct r600_bc_alu *alu, struct gpr_usage usage[128],
				int32_t id, int32_t last_barrier, unsigned *barrier)
{
	struct gpr_usage *cur_usage;
	struct gpr_usage_range *range;
	unsigned src, num_src;

	num_src = r600_bc_get_num_operands(bc, alu);
	for (src = 0; src < num_src; ++src) {
		// constants doesn't matter
		if (!is_gpr(alu->src[src].sel))
			continue;

		cur_usage = &usage[alu->src[src].sel];
		range = find_src_range(cur_usage, id);
		if (range->replacement != -1)
			alu->src[src].sel = range->replacement;

		*barrier |= is_barrier_needed(cur_usage, id, alu->src[src].chan, last_barrier);
	}

	if (alu->dst.write) {
		cur_usage = &usage[alu->dst.sel];
		range = find_dst_range(cur_usage, id);
		if (range->replacement == alu->dst.sel) {
			if (!alu->is_op3)
				alu->dst.write = 0;
			else
				/*TODO: really check that register 123 is useable */
				alu->dst.sel = 123;
		} else if (range->replacement != -1) {
			alu->dst.sel = range->replacement;
		}
		if (alu->dst.rel)
			notice_gpr_rel_last_write(usage, id, alu->dst.chan);
		else
			notice_gpr_last_write(cur_usage, id, alu->dst.chan);
	}
}

static void replace_tex_gprs(struct r600_bc_tex *tex, struct gpr_usage usage[128],
				int32_t id, int32_t last_barrier, unsigned *barrier)
{
	struct gpr_usage *cur_usage = &usage[tex->src_gpr];
	struct gpr_usage_range *range = find_src_range(cur_usage, id);

	if (tex->src_rel) {
		*barrier = 1;
        } else {
		if (tex->src_sel_x < 4)
			*barrier |= is_barrier_needed(cur_usage, id, tex->src_sel_x, last_barrier);
		if (tex->src_sel_y < 4)
			*barrier |= is_barrier_needed(cur_usage, id, tex->src_sel_y, last_barrier);
		if (tex->src_sel_z < 4)
			*barrier |= is_barrier_needed(cur_usage, id, tex->src_sel_z, last_barrier);
		if (tex->src_sel_w < 4)
			*barrier |= is_barrier_needed(cur_usage, id, tex->src_sel_w, last_barrier);
	}

	if (range->replacement != -1)
		tex->src_gpr = range->replacement;

	cur_usage = &usage[tex->dst_gpr];
	range = find_dst_range(cur_usage, id);
	if (range->replacement != -1)
		tex->dst_gpr = range->replacement;

	if (tex->dst_rel) {
		if (tex->dst_sel_x != 7)
			notice_gpr_rel_last_write(usage, id, tex->dst_sel_x);
		if (tex->dst_sel_y != 7)
			notice_gpr_rel_last_write(usage, id, tex->dst_sel_y);
		if (tex->dst_sel_z != 7)
			notice_gpr_rel_last_write(usage, id, tex->dst_sel_z);
		if (tex->dst_sel_w != 7)
			notice_gpr_rel_last_write(usage, id, tex->dst_sel_w);
	} else {
		if (tex->dst_sel_x != 7)
			notice_gpr_last_write(cur_usage, id, tex->dst_sel_x);
		if (tex->dst_sel_y != 7)
			notice_gpr_last_write(cur_usage, id, tex->dst_sel_y);
		if (tex->dst_sel_z != 7)
			notice_gpr_last_write(cur_usage, id, tex->dst_sel_z);
		if (tex->dst_sel_w != 7)
			notice_gpr_last_write(cur_usage, id, tex->dst_sel_w);
	}
}

static void replace_vtx_gprs(struct r600_bc_vtx *vtx, struct gpr_usage usage[128],
				int32_t id, int32_t last_barrier, unsigned *barrier)
{
	struct gpr_usage *cur_usage = &usage[vtx->src_gpr];
	struct gpr_usage_range *range = find_src_range(cur_usage, id);

	*barrier |= is_barrier_needed(cur_usage, id, vtx->src_sel_x, last_barrier);

	if (range->replacement != -1)
		vtx->src_gpr = range->replacement;

	cur_usage = &usage[vtx->dst_gpr];
	range = find_dst_range(cur_usage, id);
	if (range->replacement != -1)
		vtx->dst_gpr = range->replacement;

	if (vtx->dst_sel_x != 7)
		notice_gpr_last_write(cur_usage, id, vtx->dst_sel_x);
	if (vtx->dst_sel_y != 7)
		notice_gpr_last_write(cur_usage, id, vtx->dst_sel_y);
	if (vtx->dst_sel_z != 7)
		notice_gpr_last_write(cur_usage, id, vtx->dst_sel_z);
	if (vtx->dst_sel_w != 7)
		notice_gpr_last_write(cur_usage, id, vtx->dst_sel_w);
}

static void replace_export_gprs(struct r600_bc_cf *cf, struct gpr_usage usage[128],
				int32_t id, int32_t last_barrier)
{
	//TODO handle other memory operations
	struct gpr_usage *cur_usage = &usage[cf->output.gpr];
	struct gpr_usage_range *range = find_src_range(cur_usage, id);

	cf->barrier = 0;
	if (cf->output.swizzle_x < 4)
		cf->barrier |= is_barrier_needed(cur_usage, -1, cf->output.swizzle_x, last_barrier);
	if (cf->output.swizzle_y < 4)
		cf->barrier |= is_barrier_needed(cur_usage, -1, cf->output.swizzle_y, last_barrier);
	if (cf->output.swizzle_z < 4)
		cf->barrier |= is_barrier_needed(cur_usage, -1, cf->output.swizzle_z, last_barrier);
	if (cf->output.swizzle_w < 4)
		cf->barrier |= is_barrier_needed(cur_usage, -1, cf->output.swizzle_w, last_barrier);

	if (range->replacement != -1)
		cf->output.gpr = range->replacement;
}

static void optimize_alu_inst(struct r600_bc *bc, struct r600_bc_cf *cf, struct r600_bc_alu *alu)
{
	struct r600_bc_alu *alu_next;
	unsigned chan;
	unsigned src, num_src;

	/* check if a MOV could be optimized away */
	if (alu->inst == V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV) {

		/* destination equals source? */
		if (alu->dst.sel != alu->src[0].sel ||
			alu->dst.chan != alu->src[0].chan)
			return;

		/* any special handling for the source? */
		if (alu->src[0].rel || alu->src[0].neg || alu->src[0].abs)
			return;

		/* any special handling for destination? */
		if (alu->dst.rel || alu->dst.clamp)
			return;

		/* ok find next instruction group and check if ps/pv is used */
		for (alu_next = alu; !alu_next->last; alu_next = NEXT_ALU(alu_next));

		if (alu_next->list.next != &cf->alu) {
			chan = is_alu_reduction_inst(bc, alu) ? 0 : alu->dst.chan;
			for (alu_next = NEXT_ALU(alu_next); alu_next; alu_next = NEXT_ALU(alu_next)) {
				num_src = r600_bc_get_num_operands(bc, alu_next);
				for (src = 0; src < num_src; ++src) {
					if (alu_next->src[src].sel == V_SQ_ALU_SRC_PV &&
						alu_next->src[src].chan == chan)
						return;

					if (alu_next->src[src].sel == V_SQ_ALU_SRC_PS)
						return;
				}

				if (alu_next->last)
					break;
			}
		}

		r600_bc_remove_alu(cf, alu);
	}
}

static void optimize_export_inst(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	struct r600_bc_cf *prev = LIST_ENTRY(struct r600_bc_cf, cf->list.prev, list);
	if (&prev->list == &bc->cf ||
		prev->inst != cf->inst ||
		prev->output.type != cf->output.type ||
		prev->output.elem_size != cf->output.elem_size ||
		prev->output.swizzle_x != cf->output.swizzle_x ||
		prev->output.swizzle_y != cf->output.swizzle_y ||
		prev->output.swizzle_z != cf->output.swizzle_z ||
		prev->output.swizzle_w != cf->output.swizzle_w)
		return;

	if ((prev->output.burst_count + cf->output.burst_count) > 16)
		return;

	if ((prev->output.gpr + prev->output.burst_count) == cf->output.gpr &&
		(prev->output.array_base + prev->output.burst_count) == cf->output.array_base) {

		prev->output.burst_count += cf->output.burst_count;
		r600_bc_remove_cf(bc, cf);

	} else if (prev->output.gpr == (cf->output.gpr + cf->output.burst_count) &&
		prev->output.array_base == (cf->output.array_base + cf->output.burst_count)) {

		cf->output.burst_count += prev->output.burst_count;
		r600_bc_remove_cf(bc, prev);
	}
}

static void r600_bc_optimize(struct r600_bc *bc)
{
	struct r600_bc_cf *cf, *next_cf;
	struct r600_bc_alu *first, *next_alu;
	struct r600_bc_alu *alu;
	struct r600_bc_vtx *vtx;
	struct r600_bc_tex *tex;
	struct gpr_usage usage[128];

	/* assume that each gpr is exported only once */
	struct r600_bc_cf *export_cf[128] = { NULL };
	int32_t export_remap[128];

	int32_t id, barrier[bc->nstack];
	unsigned i, j, stack, predicate, old_stack;

	memset(&usage, 0, sizeof(usage));
	for (i = 0; i < 128; ++i) {
		usage[i].first_write = -1;
		usage[i].last_write[0] = -1;
		usage[i].last_write[1] = -1;
		usage[i].last_write[2] = -1;
		usage[i].last_write[3] = -1;
	}

	/* first gather some informations about the gpr usage */
	id = 0; stack = 0;
	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		switch (get_cf_class(cf)) {
		case CF_CLASS_ALU:
			predicate = 0;
			first = NULL;
			LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
				if (!first)
					first = alu;
				notice_alu_src_gprs(bc, alu, usage, id);
				if (alu->last) {
					notice_alu_dst_gprs(first, usage, id, predicate || stack > 0);
					first = NULL;
					++id;
				}
				if (is_alu_pred_inst(bc, alu))
					predicate++;
			}
			if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3)
				stack += predicate;
			else if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3)
				stack -= 1;
			else if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3)
				stack -= 2;
			break;
		case CF_CLASS_TEXTURE:
			LIST_FOR_EACH_ENTRY(tex, &cf->tex, list) {
				notice_tex_gprs(tex, usage, id++, stack > 0);
			}
			break;
		case CF_CLASS_VERTEX:
			LIST_FOR_EACH_ENTRY(vtx, &cf->vtx, list) {
				notice_vtx_gprs(vtx, usage, id++, stack > 0);
			}
			break;
		case CF_CLASS_EXPORT:
			notice_export_gprs(cf, usage, export_cf, export_remap);
			continue; // don't increment id
		case CF_CLASS_OTHER:
			switch (cf->inst) {
			case V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
			case V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
			case V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
				break;

			case V_SQ_CF_WORD1_SQ_CF_INST_POP:
				stack -= cf->pop_count;
				break;

			default:
				// TODO implement loop handling
				goto out;
			}
		}
		id += 0x100;
	        id &= ~0xFF;
	}
	assert(stack == 0);

	/* try to optimize gpr usage */
	for (i = 0; i < 124; ++i) {
		for (j = 0; j < usage[i].nranges; ++j) {
			struct gpr_usage_range *range = &usage[i].ranges[j];
			int is_export = export_cf[i] && export_cf[i + 1] &&
				range->start < export_remap[i] &&
				export_remap[i] <= range->end;

			if (range->start == -1)
				range->replacement = -1;
			else if (range->end == -1)
				range->replacement = i;
			else
				find_replacement(usage, i, range, is_export);

			if (range->replacement == -1)
				bc->ngpr = i;
			else if (range->replacement < i && range->replacement > bc->ngpr)
				bc->ngpr = range->replacement;

			if (is_export && range->replacement != -1) {
				find_export_replacement(usage, range, export_cf[i],
							export_cf[i + 1], export_remap[i + 1]);
			}
		}
	}
	bc->ngpr++;

	/* apply the changes */
	for (i = 0; i < 128; ++i) {
		usage[i].last_write[0] = -1;
		usage[i].last_write[1] = -1;
		usage[i].last_write[2] = -1;
		usage[i].last_write[3] = -1;
	}
	barrier[0] = 0;
	id = 0; stack = 0;
	LIST_FOR_EACH_ENTRY_SAFE(cf, next_cf, &bc->cf, list) {
		old_stack = stack;
		switch (get_cf_class(cf)) {
		case CF_CLASS_ALU:
			predicate = 0;
			first = NULL;
			cf->barrier = 0;
			LIST_FOR_EACH_ENTRY_SAFE(alu, next_alu, &cf->alu, list) {
				replace_alu_gprs(bc, alu, usage, id, barrier[stack], &cf->barrier);
				if (alu->last)
					++id;

				if (is_alu_pred_inst(bc, alu))
					predicate++;

				if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3)
					optimize_alu_inst(bc, cf, alu);
			}
			if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3)
				stack += predicate;
			else if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3)
				stack -= 1;
			else if (cf->inst == V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3)
				stack -= 2;
			if (LIST_IS_EMPTY(&cf->alu)) {
				r600_bc_remove_cf(bc, cf);
				cf = NULL;
			}
			break;
		case CF_CLASS_TEXTURE:
			cf->barrier = 0;
			LIST_FOR_EACH_ENTRY(tex, &cf->tex, list) {
				replace_tex_gprs(tex, usage, id++, barrier[stack], &cf->barrier);
			}
			break;
		case CF_CLASS_VERTEX:
			cf->barrier = 0;
			LIST_FOR_EACH_ENTRY(vtx, &cf->vtx, list) {
				replace_vtx_gprs(vtx, usage, id++, barrier[stack], &cf->barrier);
			}
			break;
		case CF_CLASS_EXPORT:
			continue; // don't increment id
		case CF_CLASS_OTHER:
			if (cf->inst == V_SQ_CF_WORD1_SQ_CF_INST_POP) {
				cf->barrier = 0;
				stack -= cf->pop_count;
			}
			break;
		}

		id &= ~0xFF;
		if (cf && cf->barrier)
			barrier[old_stack] = id;

		for (i = old_stack + 1; i <= stack; ++i)
			barrier[i] = barrier[old_stack];

		id += 0x100;
		if (stack != 0) /* ensue exports are placed outside of conditional blocks */
			continue;

		for (i = 0; i < 128; ++i) {
			if (!export_cf[i] || id < export_remap[i])
				continue;

			r600_bc_move_cf(bc, export_cf[i], next_cf);
			replace_export_gprs(export_cf[i], usage, export_remap[i], barrier[stack]);
			if (export_cf[i]->barrier)
				barrier[stack] = id - 1;
			next_cf = LIST_ENTRY(struct r600_bc_cf, export_cf[i]->list.next, list);
			optimize_export_inst(bc, export_cf[i]);
			export_cf[i] = NULL;
		}
	}
	assert(stack == 0);

out:
	for (i = 0; i < 128; ++i) {
		free(usage[i].ranges);
	}
}

int r600_bc_build(struct r600_bc *bc)
{
	struct r600_bc_cf *cf;
	struct r600_bc_alu *alu;
	struct r600_bc_vtx *vtx;
	struct r600_bc_tex *tex;
	struct r600_bc_cf *exports[4] = { NULL };
	uint32_t literal[4];
	unsigned nliteral;
	unsigned addr;
	int i, r;

	if (bc->callstack[0].max > 0)
		bc->nstack = ((bc->callstack[0].max + 3) >> 2) + 2;
	if (bc->type == TGSI_PROCESSOR_VERTEX && !bc->nstack) {
		bc->nstack = 1;
	}

	r600_bc_optimize(bc);

	/* first path compute addr of each CF block */
	/* addr start after all the CF instructions */
	addr = LIST_ENTRY(struct r600_bc_cf, bc->cf.prev, list)->id + 2;
	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		switch (get_cf_class(cf)) {
		case CF_CLASS_ALU:
			break;
		case CF_CLASS_TEXTURE:
		case CF_CLASS_VERTEX:
			/* fetch node need to be 16 bytes aligned*/
			addr += 3;
			addr &= 0xFFFFFFFCUL;
			break;
			break;
		case CF_CLASS_EXPORT:
			if (cf->inst == BC_INST(bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT))
				exports[cf->output.type] = cf;
			break;
		case CF_CLASS_OTHER:
			break;
		default:
			R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
			return -EINVAL;
		}
		cf->addr = addr;
		addr += cf->ndw;
		bc->ndw = cf->addr + cf->ndw;
	}

	/* set export done on last export of each type */
	for (i = 0; i < 4; ++i) {
		if (exports[i]) {
			exports[i]->inst = BC_INST(bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE);
		}
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
		switch (get_cf_class(cf)) {
		case CF_CLASS_ALU:
			nliteral = 0;
			memset(literal, 0, sizeof(literal));
			LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
				r = r600_bc_alu_nliterals(bc, alu, literal, &nliteral);
				if (r)
					return r;
				r600_bc_alu_adjust_literals(bc, alu, literal, nliteral);
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
					for (i = 0; i < align(nliteral, 2); ++i) {
						bc->bytecode[addr++] = literal[i];
					}
					nliteral = 0;
					memset(literal, 0, sizeof(literal));
				}
			}
			break;
		case CF_CLASS_VERTEX:
			LIST_FOR_EACH_ENTRY(vtx, &cf->vtx, list) {
				r = r600_bc_vtx_build(bc, vtx, addr);
				if (r)
					return r;
				addr += 4;
			}
			break;
		case CF_CLASS_TEXTURE:
			LIST_FOR_EACH_ENTRY(tex, &cf->tex, list) {
				r = r600_bc_tex_build(bc, tex, addr);
				if (r)
					return r;
				addr += 4;
			}
			break;
		case CF_CLASS_EXPORT:
		case CF_CLASS_OTHER:
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
	struct r600_bc_cf *cf = NULL;
	struct r600_bc_alu *alu = NULL;
	struct r600_bc_vtx *vtx = NULL;
	struct r600_bc_tex *tex = NULL;

	unsigned i, id;
	uint32_t literal[4];
	unsigned nliteral;
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
	fprintf(stderr, "bytecode %d dw -- %d gprs ---------------------\n", bc->ndw, bc->ngpr);
	fprintf(stderr, "     %c\n", chip);

	LIST_FOR_EACH_ENTRY(cf, &bc->cf, list) {
		id = cf->id;

		switch (get_cf_class(cf)) {
		case CF_CLASS_ALU:
			fprintf(stderr, "%04d %08X ALU ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%04d ", cf->addr);
			fprintf(stderr, "KCACHE_MODE0:%X ", cf->kcache[0].mode);
			fprintf(stderr, "KCACHE_BANK0:%X ", cf->kcache[0].bank);
			fprintf(stderr, "KCACHE_BANK1:%X\n", cf->kcache[1].bank);
			id++;
			fprintf(stderr, "%04d %08X ALU ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "KCACHE_MODE1:%X ", cf->kcache[1].mode);
			fprintf(stderr, "KCACHE_ADDR0:%X ", cf->kcache[0].addr);
			fprintf(stderr, "KCACHE_ADDR1:%X ", cf->kcache[1].addr);
			fprintf(stderr, "BARRIER:%d ", cf->barrier);
			fprintf(stderr, "COUNT:%d\n", cf->ndw / 2);
			break;
		case CF_CLASS_TEXTURE:
		case CF_CLASS_VERTEX:
			fprintf(stderr, "%04d %08X TEX/VTX ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%04d\n", cf->addr);
			id++;
			fprintf(stderr, "%04d %08X TEX/VTX ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "BARRIER:%d ", cf->barrier);
			fprintf(stderr, "COUNT:%d\n", cf->ndw / 4);
			break;
		case CF_CLASS_EXPORT:
			fprintf(stderr, "%04d %08X EXPORT ", id, bc->bytecode[id]);
			fprintf(stderr, "GPR:%d ", cf->output.gpr);
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
			fprintf(stderr, "BARRIER:%d ", cf->barrier);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "BURST_COUNT:%d\n", cf->output.burst_count);
			break;
		case CF_CLASS_OTHER:
			fprintf(stderr, "%04d %08X CF ", id, bc->bytecode[id]);
			fprintf(stderr, "ADDR:%04d\n", cf->cf_addr);
			id++;
			fprintf(stderr, "%04d %08X CF ", id, bc->bytecode[id]);
			fprintf(stderr, "INST:%d ", cf->inst);
			fprintf(stderr, "COND:%X ", cf->cond);
			fprintf(stderr, "BARRIER:%d ", cf->barrier);
			fprintf(stderr, "POP_COUNT:%X\n", cf->pop_count);
			break;
		}

		id = cf->addr;
		nliteral = 0;
		LIST_FOR_EACH_ENTRY(alu, &cf->alu, list) {
			r600_bc_alu_nliterals(bc, alu, literal, &nliteral);

			fprintf(stderr, "%04d %08X   ", id, bc->bytecode[id]);
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
			fprintf(stderr, "%04d %08X %c ", id, bc->bytecode[id], alu->last ? '*' : ' ');
			fprintf(stderr, "INST:%d ", alu->inst);
			fprintf(stderr, "DST(SEL:%d ", alu->dst.sel);
			fprintf(stderr, "CHAN:%d ", alu->dst.chan);
			fprintf(stderr, "REL:%d ", alu->dst.rel);
			fprintf(stderr, "CLAMP:%d) ", alu->dst.clamp);
			fprintf(stderr, "BANK_SWIZZLE:%d ", alu->bank_swizzle);
			if (alu->is_op3) {
				fprintf(stderr, "SRC2(SEL:%d ", alu->src[2].sel);
				fprintf(stderr, "REL:%d ", alu->src[2].rel);
				fprintf(stderr, "CHAN:%d ", alu->src[2].chan);
				fprintf(stderr, "NEG:%d)\n", alu->src[2].neg);
			} else {
				fprintf(stderr, "SRC0_ABS:%d ", alu->src[0].abs);
				fprintf(stderr, "SRC1_ABS:%d ", alu->src[1].abs);
				fprintf(stderr, "WRITE_MASK:%d ", alu->dst.write);
				fprintf(stderr, "OMOD:%d ", alu->omod);
				fprintf(stderr, "EXECUTE_MASK:%d ", alu->predicate);
				fprintf(stderr, "UPDATE_PRED:%d\n", alu->predicate);
			}

			id++;
			if (alu->last) {
				for (i = 0; i < nliteral; i++, id++) {
					float *f = (float*)(bc->bytecode + id);
					fprintf(stderr, "%04d %08X\t%f\n", id, bc->bytecode[id], *f);
				}
				id += nliteral & 1;
				nliteral = 0;
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
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT(8 - 1);
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(40 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT(count - 8 - 1);
	} else {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT(count - 1);
	}
	bytecode[i++] = S_SQ_CF_WORD0_ADDR(0);
	bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_RETURN) |
			S_SQ_CF_WORD1_BARRIER(0);

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
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT(8 - 1);
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(40 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) |
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT((count - 8) - 1);
	} else {
		bytecode[i++] = S_SQ_CF_WORD0_ADDR(8 >> 1);
		bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX_TC) |
						S_SQ_CF_WORD1_BARRIER(0) |
						S_SQ_CF_WORD1_COUNT(count - 1);
	}
	bytecode[i++] = S_SQ_CF_WORD0_ADDR(0);
	bytecode[i++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_RETURN) |
			S_SQ_CF_WORD1_BARRIER(0);

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
