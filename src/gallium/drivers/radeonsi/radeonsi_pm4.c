/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 *      Christian König <christian.koenig@amd.com>
 */

#include "util/u_memory.h"
#include "radeonsi_pipe.h"
#include "radeonsi_pm4.h"
#include "sid.h"
#include "r600_hw_context_priv.h"

#define NUMBER_OF_STATES (sizeof(union si_state) / sizeof(struct si_pm4_state *))

void si_pm4_cmd_begin(struct si_pm4_state *state, unsigned opcode)
{
	state->last_opcode = opcode;
	state->last_pm4 = state->ndw++;
}

void si_pm4_cmd_add(struct si_pm4_state *state, uint32_t dw)
{
	state->pm4[state->ndw++] = dw;
}

void si_pm4_cmd_end(struct si_pm4_state *state, bool predicate)
{
	unsigned count;
	count = state->ndw - state->last_pm4 - 2;
	state->pm4[state->last_pm4] = PKT3(state->last_opcode,
					   count, predicate);

	assert(state->ndw <= SI_PM4_MAX_DW);
}

void si_pm4_set_reg(struct si_pm4_state *state, unsigned reg, uint32_t val)
{
	unsigned opcode;

	if (reg >= SI_CONFIG_REG_OFFSET && reg <= SI_CONFIG_REG_END) {
		opcode = PKT3_SET_CONFIG_REG;
		reg -= SI_CONFIG_REG_OFFSET;

	} else if (reg >= SI_SH_REG_OFFSET && reg <= SI_SH_REG_END) {
		opcode = PKT3_SET_SH_REG;
		reg -= SI_SH_REG_OFFSET;

	} else if (reg >= SI_CONTEXT_REG_OFFSET && reg <= SI_CONTEXT_REG_END) {
		opcode = PKT3_SET_CONTEXT_REG;
		reg -= SI_CONTEXT_REG_OFFSET;
	} else {
		R600_ERR("Invalid register offset %08x!\n", reg);
		return;
	}

	reg >>= 2;

	if (opcode != state->last_opcode || reg != (state->last_reg + 1)) {
		si_pm4_cmd_begin(state, opcode);
		si_pm4_cmd_add(state, reg);
	}

	state->last_reg = reg;
	si_pm4_cmd_add(state, val);
	si_pm4_cmd_end(state, false);
}

void si_pm4_add_bo(struct si_pm4_state *state,
                   struct si_resource *bo,
                   enum radeon_bo_usage usage)
{
	unsigned idx = state->nbo++;
	assert(idx < SI_PM4_MAX_BO);

	si_resource_reference(&state->bo[idx], bo);
	state->bo_usage[idx] = usage;
}

void si_pm4_inval_shader_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_SH_ICACHE_ACTION_ENA(1);
	state->cp_coher_cntl |= S_0085F0_SH_KCACHE_ACTION_ENA(1);
}

void si_pm4_inval_texture_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1);
}

void si_pm4_inval_vertex_cache(struct si_pm4_state *state)
{
        /* Some GPUs don't have the vertex cache and must use the texture cache instead. */
	state->cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1);
}

void si_pm4_inval_fb_cache(struct si_pm4_state *state, unsigned nr_cbufs)
{
	state->cp_coher_cntl |= S_0085F0_CB_ACTION_ENA(1);
	state->cp_coher_cntl |= ((1 << nr_cbufs) - 1) << S_0085F0_CB0_DEST_BASE_ENA_SHIFT;
}

void si_pm4_inval_zsbuf_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_DB_ACTION_ENA(1) | S_0085F0_DB_DEST_BASE_ENA(1);
}

void si_pm4_free_state(struct r600_context *rctx,
		       struct si_pm4_state *state,
		       unsigned idx)
{
	if (state == NULL)
		return;

	if (rctx->emitted.array[idx] == state) {
		rctx->emitted.array[idx] = NULL;
	}

	for (int i = 0; i < state->nbo; ++i) {
		si_resource_reference(&state->bo[i], NULL);
	}
	FREE(state);
}

unsigned si_pm4_dirty_dw(struct r600_context *rctx)
{
	unsigned count = 0;
	uint32_t cp_coher_cntl = 0;

	for (int i = 0; i < NUMBER_OF_STATES; ++i) {
		struct si_pm4_state *state = rctx->queued.array[i];

		if (!state || rctx->emitted.array[i] == state)
			continue;

		count += state->ndw;
		cp_coher_cntl |= state->cp_coher_cntl;
	}

	//TODO
	rctx->atom_surface_sync.flush_flags |= cp_coher_cntl;
	r600_atom_dirty(rctx, &rctx->atom_surface_sync.atom);
	return count;
}

void si_pm4_emit_dirty(struct r600_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->cs;

	for (int i = 0; i < NUMBER_OF_STATES; ++i) {
		struct si_pm4_state *state = rctx->queued.array[i];

		if (!state || rctx->emitted.array[i] == state)
			continue;

		for (int j = 0; j < state->nbo; ++j) {
			r600_context_bo_reloc(rctx, state->bo[j],
					      state->bo_usage[j]);
		}

		memcpy(&cs->buf[cs->cdw], state->pm4, state->ndw * 4);
		cs->cdw += state->ndw;

		rctx->emitted.array[i] = state;
	}
}

void si_pm4_reset_emitted(struct r600_context *rctx)
{
	memset(&rctx->emitted, 0, sizeof(rctx->emitted));
}
