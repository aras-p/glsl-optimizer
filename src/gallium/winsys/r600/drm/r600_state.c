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
 *
 * Authors:
 *      Jerome Glisse
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "radeon_priv.h"
#include "r600d.h"

static int r600_state_pm4_resource(struct radeon_state *state);
static int r600_state_pm4_cb0(struct radeon_state *state);
static int r600_state_pm4_vgt(struct radeon_state *state);
static int r600_state_pm4_db(struct radeon_state *state);
static int r600_state_pm4_shader(struct radeon_state *state);
static int r600_state_pm4_draw(struct radeon_state *state);
static int r600_state_pm4_config(struct radeon_state *state);
static int r600_state_pm4_generic(struct radeon_state *state);
static int r600_state_pm4_query_begin(struct radeon_state *state);
static int r600_state_pm4_query_end(struct radeon_state *state);
static int r700_state_pm4_config(struct radeon_state *state);
static int r700_state_pm4_cb0(struct radeon_state *state);
static int r700_state_pm4_db(struct radeon_state *state);

#include "r600_states.h"

/*
 * r600/r700 state functions
 */
static int r600_state_pm4_bytecode(struct radeon_state *state, unsigned offset, unsigned id, unsigned nreg)
{
	const struct radeon_register *regs = state->radeon->type[state->type].regs;
	unsigned i;
	int r;

	if (!offset) {
		fprintf(stderr, "%s invalid register for state %d %d\n",
			__func__, state->type, id);
		return -EINVAL;
	}
	if (offset >= R600_CONFIG_REG_OFFSET && offset < R600_CONFIG_REG_END) {
		state->pm4[state->cpm4++] = PKT3(PKT3_SET_CONFIG_REG, nreg);
		state->pm4[state->cpm4++] = (offset - R600_CONFIG_REG_OFFSET) >> 2;
		for (i = 0; i < nreg; i++) {
			state->pm4[state->cpm4++] = state->states[id + i];
		}
		for (i = 0; i < nreg; i++) {
			if (regs[id + i].need_reloc) {
				state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
				r = radeon_state_reloc(state, state->cpm4, regs[id + i].bo_id);
				if (r)
					return r;
				state->pm4[state->cpm4++] = state->bo[regs[id + i].bo_id]->handle;
			}
		}
		return 0;
	}
	if (offset >= R600_CONTEXT_REG_OFFSET && offset < R600_CONTEXT_REG_END) {
		state->pm4[state->cpm4++] = PKT3(PKT3_SET_CONTEXT_REG, nreg);
		state->pm4[state->cpm4++] = (offset - R600_CONTEXT_REG_OFFSET) >> 2;
		for (i = 0; i < nreg; i++) {
			state->pm4[state->cpm4++] = state->states[id + i];
		}
		for (i = 0; i < nreg; i++) {
			if (regs[id + i].need_reloc) {
				state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
				r = radeon_state_reloc(state, state->cpm4, regs[id + i].bo_id);
				if (r)
					return r;
				state->pm4[state->cpm4++] = state->bo[regs[id + i].bo_id]->handle;
			}
		}
		return 0;
	}
	if (offset >= R600_ALU_CONST_OFFSET && offset < R600_ALU_CONST_END) {
		state->pm4[state->cpm4++] = PKT3(PKT3_SET_ALU_CONST, nreg);
		state->pm4[state->cpm4++] = (offset - R600_ALU_CONST_OFFSET) >> 2;
		for (i = 0; i < nreg; i++) {
			state->pm4[state->cpm4++] = state->states[id + i];
		}
		return 0;
	}
	if (offset >= R600_SAMPLER_OFFSET && offset < R600_SAMPLER_END) {
		state->pm4[state->cpm4++] = PKT3(PKT3_SET_SAMPLER, nreg);
		state->pm4[state->cpm4++] = (offset - R600_SAMPLER_OFFSET) >> 2;
		for (i = 0; i < nreg; i++) {
			state->pm4[state->cpm4++] = state->states[id + i];
		}
		return 0;
	}
	fprintf(stderr, "%s unsupported offset 0x%08X\n", __func__, offset);
	return -EINVAL;
}

static int r600_state_pm4_generic(struct radeon_state *state)
{
	struct radeon *radeon = state->radeon;
	unsigned i, offset, nreg, type, coffset, loffset, soffset;
	unsigned start;
	int r;

	if (!state->nstates)
		return 0;
	type = state->type;
	soffset = (state->id - radeon->type[type].id) * radeon->type[type].stride;
	offset = loffset = radeon->type[type].regs[0].offset + soffset;
	start = 0;
	for (i = 1, nreg = 1; i < state->nstates; i++) {
		coffset = radeon->type[type].regs[i].offset + soffset;
		if (coffset == (loffset + 4)) {
			nreg++;
			loffset = coffset;
		} else {
			r = r600_state_pm4_bytecode(state, offset, start, nreg);
			if (r) {
				fprintf(stderr, "%s invalid 0x%08X %d\n", __func__, start, nreg);
				return r;
			}
			offset = loffset = coffset;
			nreg = 1;
			start = i;
		}
	}
	return r600_state_pm4_bytecode(state, offset, start, nreg);
}

static void r600_state_pm4_with_flush(struct radeon_state *state, u32 flags)
{
	unsigned i, j, add, size;

	state->nreloc = 0;
	for (i = 0; i < state->nbo; i++) {
		for (j = 0, add = 1; j < state->nreloc; j++) {
			if (state->bo[state->reloc_bo_id[j]] == state->bo[i]) {
				add = 0;
				break;
			}
		}
		if (add) {
			state->reloc_bo_id[state->nreloc++] = i;
		}
	}
	for (i = 0; i < state->nreloc; i++) {
		size = (state->bo[state->reloc_bo_id[i]]->size + 255) >> 8;
		state->pm4[state->cpm4++] = PKT3(PKT3_SURFACE_SYNC, 3);
		state->pm4[state->cpm4++] = flags;
		state->pm4[state->cpm4++] = size;
		state->pm4[state->cpm4++] = 0x00000000;
		state->pm4[state->cpm4++] = 0x0000000A;
		state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
		state->reloc_pm4_id[i] = state->cpm4;
		state->pm4[state->cpm4++] = state->bo[state->reloc_bo_id[i]]->handle;
	}
}

static int r600_state_pm4_cb0(struct radeon_state *state)
{
	int r;

	r600_state_pm4_with_flush(state, S_0085F0_CB_ACTION_ENA(1) |
				S_0085F0_CB0_DEST_BASE_ENA(1));
	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0);
	state->pm4[state->cpm4++] = 0x00000002;
	return 0;
}

static int r700_state_pm4_cb0(struct radeon_state *state)
{
	int r;

	r600_state_pm4_with_flush(state, S_0085F0_CB_ACTION_ENA(1) |
				S_0085F0_CB0_DEST_BASE_ENA(1));
	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	return 0;
}

static int r600_state_pm4_db(struct radeon_state *state)
{
	int r;

	r600_state_pm4_with_flush(state, S_0085F0_DB_ACTION_ENA(1) |
				S_0085F0_DB_DEST_BASE_ENA(1));
	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0);
	state->pm4[state->cpm4++] = 0x00000001;
	return 0;
}

static int r700_state_pm4_db(struct radeon_state *state)
{
	int r;

	r600_state_pm4_with_flush(state, S_0085F0_DB_ACTION_ENA(1) |
				S_0085F0_DB_DEST_BASE_ENA(1));
	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	return 0;
}

static int r600_state_pm4_config(struct radeon_state *state)
{
	state->pm4[state->cpm4++] = PKT3(PKT3_START_3D_CMDBUF, 0);
	state->pm4[state->cpm4++] = 0x00000000;
	state->pm4[state->cpm4++] = PKT3(PKT3_CONTEXT_CONTROL, 1);
	state->pm4[state->cpm4++] = 0x80000000;
	state->pm4[state->cpm4++] = 0x80000000;
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 0);
	state->pm4[state->cpm4++] = EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT;
	state->pm4[state->cpm4++] = PKT3(PKT3_SET_CONFIG_REG, 1);
	state->pm4[state->cpm4++] = 0x00000010;
	state->pm4[state->cpm4++] = 0x00028000;
	return r600_state_pm4_generic(state);
}

static int r600_state_pm4_query_begin(struct radeon_state *state)
{
	int r;

	state->cpm4 = 0;
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 2);
	state->pm4[state->cpm4++] = EVENT_TYPE_ZPASS_DONE;
	state->pm4[state->cpm4++] = state->states[0];
	state->pm4[state->cpm4++] = 0x0;
	state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
	r = radeon_state_reloc(state, state->cpm4, 0);
	if (r)
		return r;
	state->pm4[state->cpm4++] = state->bo[0]->handle;
	return 0;
}

static int r600_state_pm4_query_end(struct radeon_state *state)
{
	int r;

	state->cpm4 = 0;
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 2);
	state->pm4[state->cpm4++] = EVENT_TYPE_ZPASS_DONE;
	state->pm4[state->cpm4++] = state->states[0];
	state->pm4[state->cpm4++] = 0x0;
	state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
	r = radeon_state_reloc(state, state->cpm4, 0);
	if (r)
		return r;
	state->pm4[state->cpm4++] = state->bo[0]->handle;
	return 0;
}

static int r700_state_pm4_config(struct radeon_state *state)
{
	state->pm4[state->cpm4++] = PKT3(PKT3_CONTEXT_CONTROL, 1);
	state->pm4[state->cpm4++] = 0x80000000;
	state->pm4[state->cpm4++] = 0x80000000;
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 0);
	state->pm4[state->cpm4++] = EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT;
	state->pm4[state->cpm4++] = PKT3(PKT3_SET_CONFIG_REG, 1);
	state->pm4[state->cpm4++] = 0x00000010;
	state->pm4[state->cpm4++] = 0x00028000;
	return r600_state_pm4_generic(state);
}

static int r600_state_pm4_shader(struct radeon_state *state)
{
	r600_state_pm4_with_flush(state, S_0085F0_SH_ACTION_ENA(1));
	return r600_state_pm4_generic(state);
}

static int r600_state_pm4_vgt(struct radeon_state *state)
{
	int r;

	r = r600_state_pm4_bytecode(state, R_028400_VGT_MAX_VTX_INDX, R600_VGT__VGT_MAX_VTX_INDX, 1);
	if (r)
		return r;
	r = r600_state_pm4_bytecode(state, R_028404_VGT_MIN_VTX_INDX, R600_VGT__VGT_MIN_VTX_INDX, 1);
	if (r)
		return r;
	r = r600_state_pm4_bytecode(state, R_028408_VGT_INDX_OFFSET, R600_VGT__VGT_INDX_OFFSET, 1);
	if (r)
		return r;
	r = r600_state_pm4_bytecode(state, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, R600_VGT__VGT_MULTI_PRIM_IB_RESET_INDX, 1);
	if (r)
		return r;
	r = r600_state_pm4_bytecode(state, R_008958_VGT_PRIMITIVE_TYPE, R600_VGT__VGT_PRIMITIVE_TYPE, 1);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_INDEX_TYPE, 0);
	state->pm4[state->cpm4++] = state->states[R600_VGT__VGT_DMA_INDEX_TYPE];
	state->pm4[state->cpm4++] = PKT3(PKT3_NUM_INSTANCES, 0);
	state->pm4[state->cpm4++] = state->states[R600_VGT__VGT_DMA_NUM_INSTANCES];
	return 0;
}

static int r600_state_pm4_draw(struct radeon_state *state)
{
	unsigned i;
	int r;

	if (state->nbo) {
		state->pm4[state->cpm4++] = PKT3(PKT3_DRAW_INDEX, 3);
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DMA_BASE];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DMA_BASE_HI];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_NUM_INDICES];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DRAW_INITIATOR];
		state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
		r = radeon_state_reloc(state, state->cpm4, 0);
		if (r)
			return r;
		state->pm4[state->cpm4++] = state->bo[0]->handle;
	} else if  (state->nimmd) {
		state->pm4[state->cpm4++] = PKT3(PKT3_DRAW_INDEX_IMMD, state->nimmd + 1);
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_NUM_INDICES];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DRAW_INITIATOR];
		for (i = 0; i < state->nimmd; i++) {
			state->pm4[state->cpm4++] = state->immd[i];
		}
	} else {
		state->pm4[state->cpm4++] = PKT3(PKT3_DRAW_INDEX_AUTO, 1);
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_NUM_INDICES];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DRAW_INITIATOR];
	}
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 0);
	state->pm4[state->cpm4++] = EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT;
	return 0;
}

static int r600_state_pm4_resource(struct radeon_state *state)
{
	u32 flags, type, nbo, offset, soffset;
	int r;

	soffset = (state->id - state->radeon->type[state->type].id) * state->radeon->type[state->type].stride;
	type = G_038018_TYPE(state->states[6]);
	switch (type) {
	case 2:
		flags = S_0085F0_TC_ACTION_ENA(1);
		nbo = 2;
		break;
	case 3:
		flags = S_0085F0_VC_ACTION_ENA(1);
		nbo = 1;
		break;
	default:
		return 0;
	}
	if (state->nbo != nbo) {
		fprintf(stderr, "%s need %d bo got %d\n", __func__, nbo, state->nbo);
		return -EINVAL;
	}
	r600_state_pm4_with_flush(state, flags);
	offset = state->radeon->type[state->type].regs[0].offset + soffset;
	state->pm4[state->cpm4++] = PKT3(PKT3_SET_RESOURCE, 7);
	state->pm4[state->cpm4++] = (offset - R_038000_SQ_TEX_RESOURCE_WORD0_0) >> 2;
	state->pm4[state->cpm4++] = state->states[0];
	state->pm4[state->cpm4++] = state->states[1];
	state->pm4[state->cpm4++] = state->states[2];
	state->pm4[state->cpm4++] = state->states[3];
	state->pm4[state->cpm4++] = state->states[4];
	state->pm4[state->cpm4++] = state->states[5];
	state->pm4[state->cpm4++] = state->states[6];
	state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
	r = radeon_state_reloc(state, state->cpm4, 0);
	if (r)
		return r;
	state->pm4[state->cpm4++] = state->bo[0]->handle;
	if (type == 2) {
		state->pm4[state->cpm4++] = PKT3(PKT3_NOP, 0);
		r = radeon_state_reloc(state, state->cpm4, 1);
		if (r)
			return r;
		state->pm4[state->cpm4++] = state->bo[1]->handle;
	}
	return 0;
}

int r600_init(struct radeon *radeon)
{
	switch (radeon->family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		radeon->ntype = R600_NTYPE;
		radeon->nstate = R600_NSTATE;
		radeon->type = R600_types;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		radeon->ntype = R600_NTYPE;
		radeon->nstate = R600_NSTATE;
		radeon->type = R700_types;
		break;
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->device);
		return -EINVAL;
	}
	return 0;
}
