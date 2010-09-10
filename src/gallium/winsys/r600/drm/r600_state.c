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

#include "util/u_memory.h"

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
static int r600_state_pm4_db_flush(struct radeon_state *state);
static int r600_state_pm4_cb_flush(struct radeon_state *state);

static int eg_state_pm4_vgt(struct radeon_state *state);

#include "r600_states.h"
#include "eg_states.h"


#define SUB_NONE(param) { { 0, R600_names_##param, (sizeof(R600_names_##param)/sizeof(struct radeon_register)) } }
#define SUB_PS(param) { R600_SHADER_PS, R600_names_##param, (sizeof(R600_names_##param)/sizeof(struct radeon_register)) }
#define SUB_VS(param) { R600_SHADER_VS, R600_names_##param, (sizeof(R600_names_##param)/sizeof(struct radeon_register)) }
#define SUB_GS(param) { R600_SHADER_GS, R600_names_##param, (sizeof(R600_names_##param)/sizeof(struct radeon_register)) }
#define SUB_FS(param) { R600_SHADER_FS, R600_names_##param, (sizeof(R600_names_##param)/sizeof(struct radeon_register)) }

#define EG_SUB_NONE(param) { { 0, EG_names_##param, (sizeof(EG_names_##param)/sizeof(struct radeon_register)) } }
#define EG_SUB_PS(param) { R600_SHADER_PS, EG_names_##param, (sizeof(EG_names_##param)/sizeof(struct radeon_register)) }
#define EG_SUB_VS(param) { R600_SHADER_VS, EG_names_##param, (sizeof(EG_names_##param)/sizeof(struct radeon_register)) }
#define EG_SUB_GS(param) { R600_SHADER_GS, EG_names_##param, (sizeof(EG_names_##param)/sizeof(struct radeon_register)) }
#define EG_SUB_FS(param) { R600_SHADER_FS, EG_names_##param, (sizeof(EG_names_##param)/sizeof(struct radeon_register)) }

/* some of these are overriden at runtime for R700 */
struct radeon_stype_info r600_stypes[] = {
	{ R600_STATE_CONFIG, 1, 0, r600_state_pm4_config, SUB_NONE(CONFIG), },
	{ R600_STATE_CB_CNTL, 1, 0, r600_state_pm4_generic, SUB_NONE(CB_CNTL) },
	{ R600_STATE_RASTERIZER, 1, 0, r600_state_pm4_generic, SUB_NONE(RASTERIZER) },
	{ R600_STATE_VIEWPORT, 1, 0, r600_state_pm4_generic, SUB_NONE(VIEWPORT) },
	{ R600_STATE_SCISSOR, 1, 0, r600_state_pm4_generic, SUB_NONE(SCISSOR) },
	{ R600_STATE_BLEND, 1, 0, r600_state_pm4_generic, SUB_NONE(BLEND), },
	{ R600_STATE_DSA, 1, 0, r600_state_pm4_generic, SUB_NONE(DSA), },
	{ R600_STATE_SHADER, 1, 0, r600_state_pm4_shader, { SUB_PS(PS_SHADER), SUB_VS(VS_SHADER) } },
	{ R600_STATE_CBUF, 1, 0, r600_state_pm4_shader,  { SUB_PS(PS_CBUF), SUB_VS(VS_CBUF) } },
	{ R600_STATE_CONSTANT, 256, 0x10, r600_state_pm4_generic,  { SUB_PS(PS_CONSTANT), SUB_VS(VS_CONSTANT) } },
	{ R600_STATE_RESOURCE, 160, 0x1c, r600_state_pm4_resource, { SUB_PS(PS_RESOURCE), SUB_VS(VS_RESOURCE), SUB_GS(GS_RESOURCE), SUB_FS(FS_RESOURCE)} },
	{ R600_STATE_SAMPLER, 18, 0xc, r600_state_pm4_generic, { SUB_PS(PS_SAMPLER), SUB_VS(VS_SAMPLER), SUB_GS(GS_SAMPLER) } },
	{ R600_STATE_SAMPLER_BORDER, 18, 0x10, r600_state_pm4_generic, { SUB_PS(PS_SAMPLER_BORDER), SUB_VS(VS_SAMPLER_BORDER), SUB_GS(GS_SAMPLER_BORDER) } },
	{ R600_STATE_CB0, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB0) },
	{ R600_STATE_CB1, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB1) },
	{ R600_STATE_CB2, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB2) },
	{ R600_STATE_CB3, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB3) },
	{ R600_STATE_CB4, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB4) },
	{ R600_STATE_CB5, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB5) },
	{ R600_STATE_CB6, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB6) },
	{ R600_STATE_CB7, 1, 0, r600_state_pm4_cb0, SUB_NONE(CB7) },
	{ R600_STATE_QUERY_BEGIN, 1, 0, r600_state_pm4_query_begin, SUB_NONE(VGT_EVENT) },
	{ R600_STATE_QUERY_END, 1, 0, r600_state_pm4_query_end, SUB_NONE(VGT_EVENT) },
	{ R600_STATE_DB, 1, 0, r600_state_pm4_db, SUB_NONE(DB) },
	{ R600_STATE_UCP, 1, 0, r600_state_pm4_generic, SUB_NONE(UCP) },
	{ R600_STATE_VGT, 1, 0, r600_state_pm4_vgt, SUB_NONE(VGT) },
	{ R600_STATE_DRAW, 1, 0, r600_state_pm4_draw, SUB_NONE(DRAW) },
	{ R600_STATE_CB_FLUSH, 1, 0, r600_state_pm4_cb_flush, SUB_NONE(CB_FLUSH) },
	{ R600_STATE_DB_FLUSH, 1, 0, r600_state_pm4_db_flush, SUB_NONE(DB_FLUSH) },
};
#define STYPES_SIZE Elements(r600_stypes)

struct radeon_stype_info eg_stypes[] = {
	{ R600_STATE_CONFIG, 1, 0, r700_state_pm4_config, EG_SUB_NONE(CONFIG), },
	{ R600_STATE_CB_CNTL, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(CB_CNTL) },
	{ R600_STATE_RASTERIZER, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(RASTERIZER) },
	{ R600_STATE_VIEWPORT, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(VIEWPORT) },
	{ R600_STATE_SCISSOR, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(SCISSOR) },
	{ R600_STATE_BLEND, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(BLEND), },
	{ R600_STATE_DSA, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(DSA), },
	{ R600_STATE_SHADER, 1, 0, r600_state_pm4_shader, { EG_SUB_PS(PS_SHADER), EG_SUB_VS(VS_SHADER) } },
	{ R600_STATE_CBUF, 1, 0, r600_state_pm4_shader, { EG_SUB_PS(PS_CBUF), EG_SUB_VS(VS_CBUF) } },
	{ R600_STATE_RESOURCE, 176, 0x20, r600_state_pm4_resource, { EG_SUB_PS(PS_RESOURCE), EG_SUB_VS(VS_RESOURCE), EG_SUB_GS(GS_RESOURCE), EG_SUB_FS(FS_RESOURCE)} },
	{ R600_STATE_SAMPLER, 18, 0xc, r600_state_pm4_generic, { EG_SUB_PS(PS_SAMPLER), EG_SUB_VS(VS_SAMPLER), EG_SUB_GS(GS_SAMPLER) } },
	{ R600_STATE_SAMPLER_BORDER, 18, 0x10, r600_state_pm4_generic, { EG_SUB_PS(PS_SAMPLER_BORDER), EG_SUB_VS(VS_SAMPLER_BORDER), EG_SUB_GS(GS_SAMPLER_BORDER) } },
	{ R600_STATE_CB0, 11, 0x3c, r600_state_pm4_generic, EG_SUB_NONE(CB) },
	{ R600_STATE_QUERY_BEGIN, 1, 0, r600_state_pm4_query_begin, EG_SUB_NONE(VGT_EVENT) },
	{ R600_STATE_QUERY_END, 1, 0, r600_state_pm4_query_end, EG_SUB_NONE(VGT_EVENT) },
	{ R600_STATE_DB, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(DB) },
	{ R600_STATE_UCP, 1, 0, r600_state_pm4_generic, EG_SUB_NONE(UCP) },
	{ R600_STATE_VGT, 1, 0, eg_state_pm4_vgt, EG_SUB_NONE(VGT) },
	{ R600_STATE_DRAW, 1, 0, r600_state_pm4_draw, EG_SUB_NONE(DRAW) },
	{ R600_STATE_CB_FLUSH, 1, 0, r600_state_pm4_cb_flush, EG_SUB_NONE(CB_FLUSH) },
	{ R600_STATE_DB_FLUSH, 1, 0, r600_state_pm4_db_flush, EG_SUB_NONE(DB_FLUSH) },

};
#define EG_STYPES_SIZE Elements(eg_stypes)

static const struct radeon_register *get_regs(struct radeon_state *state)
{
	return state->stype->reginfo[state->shader_index].regs;
}

/*
 * r600/r700 state functions
 */
static int r600_state_pm4_bytecode(struct radeon_state *state, unsigned offset, unsigned id, unsigned nreg)
{
	const struct radeon_register *regs = get_regs(state);
	unsigned i;
	int r;

	if (!offset) {
		fprintf(stderr, "%s invalid register for state %d %d\n",
			__func__, state->stype->stype, id);
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

static int eg_state_pm4_bytecode(struct radeon_state *state, unsigned offset, unsigned id, unsigned nreg)
{
	const struct radeon_register *regs = get_regs(state);
	unsigned i;
	int r;

	if (!offset) {
		fprintf(stderr, "%s invalid register for state %d %d\n",
			__func__, state->stype->stype, id);
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
	if (offset >= EG_RESOURCE_OFFSET && offset < EG_RESOURCE_END) {
		state->pm4[state->cpm4++] = PKT3(PKT3_SET_RESOURCE, nreg);
		state->pm4[state->cpm4++] = (offset - EG_RESOURCE_OFFSET) >> 2;
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
	const struct radeon_register *regs = get_regs(state);
	unsigned i, offset, nreg, coffset, loffset, soffset;
	unsigned start;
	int r;

	if (!state->nstates)
		return 0;
	soffset = state->id * state->stype->stride;
	offset = loffset = regs[0].offset + soffset;
	start = 0;
	for (i = 1, nreg = 1; i < state->nstates; i++) {
		coffset = regs[i].offset + soffset;
		if (coffset == (loffset + 4)) {
			nreg++;
			loffset = coffset;
		} else {
			if (state->radeon->family >= CHIP_CEDAR)
				r = eg_state_pm4_bytecode(state, offset, start, nreg);
			else
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
	if (state->radeon->family >= CHIP_CEDAR)
		r = eg_state_pm4_bytecode(state, offset, start, nreg);
	else
		r = r600_state_pm4_bytecode(state, offset, start, nreg);
	return r;
}

static void r600_state_pm4_with_flush(struct radeon_state *state, u32 flags, int bufs_are_cbs)
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
		if (bufs_are_cbs)
			flags |= S_0085F0_CB0_DEST_BASE_ENA(1 << i);
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

	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0);
	state->pm4[state->cpm4++] = 0x00000002;
	return 0;
}

static int r600_state_pm4_db(struct radeon_state *state)
{
	int r;

	r = r600_state_pm4_generic(state);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0);
	state->pm4[state->cpm4++] = 0x00000001;
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
	r600_state_pm4_with_flush(state, S_0085F0_SH_ACTION_ENA(1), 0);
	return r600_state_pm4_generic(state);
}

static int eg_state_pm4_vgt(struct radeon_state *state)
{
	int r;
	r = eg_state_pm4_bytecode(state, R_028400_VGT_MAX_VTX_INDX, EG_VGT__VGT_MAX_VTX_INDX, 1);
	if (r)
		return r;
	r = eg_state_pm4_bytecode(state, R_028404_VGT_MIN_VTX_INDX, EG_VGT__VGT_MIN_VTX_INDX, 1);
	if (r)
		return r;
	r = eg_state_pm4_bytecode(state, R_028408_VGT_INDX_OFFSET, EG_VGT__VGT_INDX_OFFSET, 1);
	if (r)
		return r;
	r = eg_state_pm4_bytecode(state, R_008958_VGT_PRIMITIVE_TYPE, EG_VGT__VGT_PRIMITIVE_TYPE, 1);
	if (r)
		return r;
	state->pm4[state->cpm4++] = PKT3(PKT3_INDEX_TYPE, 0);
	state->pm4[state->cpm4++] = state->states[EG_VGT__VGT_DMA_INDEX_TYPE];
	state->pm4[state->cpm4++] = PKT3(PKT3_NUM_INSTANCES, 0);
	state->pm4[state->cpm4++] = state->states[EG_VGT__VGT_DMA_NUM_INSTANCES];
	return 0;
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
	} else {
		state->pm4[state->cpm4++] = PKT3(PKT3_DRAW_INDEX_AUTO, 1);
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_NUM_INDICES];
		state->pm4[state->cpm4++] = state->states[R600_DRAW__VGT_DRAW_INITIATOR];
	}
	state->pm4[state->cpm4++] = PKT3(PKT3_EVENT_WRITE, 0);
	state->pm4[state->cpm4++] = EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT;

	return 0;
}

static int r600_state_pm4_cb_flush(struct radeon_state *state)
{
	if (!state->nbo)
		return 0;

	r600_state_pm4_with_flush(state, S_0085F0_CB_ACTION_ENA(1), 1);

	return 0;
}

static int r600_state_pm4_db_flush(struct radeon_state *state)
{
	if (!state->nbo)
		return 0;

	r600_state_pm4_with_flush(state, S_0085F0_DB_ACTION_ENA(1) |
				  S_0085F0_DB_DEST_BASE_ENA(1), 0);

	return 0;
}

static int r600_state_pm4_resource(struct radeon_state *state)
{
	u32 flags, type, nbo, offset, soffset;
	int r, nres;
	const struct radeon_register *regs = get_regs(state);

	soffset = state->id * state->stype->stride;
	if (state->radeon->family >= CHIP_CEDAR)
		type = G_038018_TYPE(state->states[7]);
	else
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
	r600_state_pm4_with_flush(state, flags, 0);
	offset = regs[0].offset + soffset;
	if (state->radeon->family >= CHIP_CEDAR)
		nres = 8;
	else
		nres = 7;
	state->pm4[state->cpm4++] = PKT3(PKT3_SET_RESOURCE, nres);
	if (state->radeon->family >= CHIP_CEDAR)
		state->pm4[state->cpm4++] = (offset - EG_RESOURCE_OFFSET) >> 2;
	else
		state->pm4[state->cpm4++] = (offset - R_038000_SQ_TEX_RESOURCE_WORD0_0) >> 2;
	state->pm4[state->cpm4++] = state->states[0];
	state->pm4[state->cpm4++] = state->states[1];
	state->pm4[state->cpm4++] = state->states[2];
	state->pm4[state->cpm4++] = state->states[3];
	state->pm4[state->cpm4++] = state->states[4];
	state->pm4[state->cpm4++] = state->states[5];
	state->pm4[state->cpm4++] = state->states[6];
	if (state->radeon->family >= CHIP_CEDAR)
		state->pm4[state->cpm4++] = state->states[7];

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


static void r600_modify_type_array(struct radeon *radeon)
{
	int i;
	switch (radeon->family) {
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		break;
	default:
		return;
	}

	/* r700 needs some mods */
	for (i = 0; i < radeon->nstype; i++) {
		struct radeon_stype_info *info = &radeon->stype[i];
		
		switch(info->stype) {
		case R600_STATE_CONFIG:
			info->pm4 = r700_state_pm4_config;
			break;
		case R600_STATE_CB0:
			info->pm4 = r600_state_pm4_generic;
			break;
		case R600_STATE_DB:
			info->pm4 = r600_state_pm4_generic;
			break;
		};
	}
}

static void build_types_array(struct radeon *radeon, struct radeon_stype_info *types, int size)
{
	int i, j;
	int id = 0;

	for (i = 0; i < size; i++) {
		types[i].base_id = id;
		types[i].npm4 = 128;
		if (types[i].reginfo[0].shader_type == 0) {
			id += types[i].num;
		} else {
			for (j = 0; j < R600_SHADER_MAX; j++) {
				if (types[i].reginfo[j].shader_type)
					id += types[i].num;
			}
		}
	}
	radeon->max_states = id;
	radeon->stype = types;
	radeon->nstype = size;
}

static void r600_build_types_array(struct radeon *radeon)
{
	build_types_array(radeon, r600_stypes, STYPES_SIZE);
	r600_modify_type_array(radeon);
}

static void eg_build_types_array(struct radeon *radeon)
{
	build_types_array(radeon, eg_stypes, EG_STYPES_SIZE);
}

int r600_init(struct radeon *radeon)
{
	if (radeon->family >= CHIP_CEDAR)
		eg_build_types_array(radeon);
	else
		r600_build_types_array(radeon);
	return 0;
}
