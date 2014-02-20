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

#ifndef SI_PM4_H
#define SI_PM4_H

#include "../../winsys/radeon/drm/radeon_winsys.h"

#define SI_PM4_MAX_DW		256
#define SI_PM4_MAX_BO		32
#define SI_PM4_MAX_RELOCS	4

// forward defines
struct si_context;
enum chip_class;

struct si_pm4_state
{
	/* family specific handling */
	enum chip_class chip_class;
	/* PKT3_SET_*_REG handling */
	unsigned	last_opcode;
	unsigned	last_reg;
	unsigned	last_pm4;

	/* flush flags for SURFACE_SYNC */
	uint32_t	cp_coher_cntl;

	/* commands for the DE */
	unsigned	ndw;
	uint32_t	pm4[SI_PM4_MAX_DW];

	/* BO's referenced by this state */
	unsigned		nbo;
	struct r600_resource	*bo[SI_PM4_MAX_BO];
	enum radeon_bo_usage	bo_usage[SI_PM4_MAX_BO];
	enum radeon_bo_priority	bo_priority[SI_PM4_MAX_BO];

	/* relocs for shader data */
	unsigned	nrelocs;
	unsigned	relocs[SI_PM4_MAX_RELOCS];

	bool compute_pkt;
};

void si_pm4_cmd_begin(struct si_pm4_state *state, unsigned opcode);
void si_pm4_cmd_add(struct si_pm4_state *state, uint32_t dw);
void si_pm4_cmd_end(struct si_pm4_state *state, bool predicate);

void si_pm4_set_reg(struct si_pm4_state *state, unsigned reg, uint32_t val);
void si_pm4_add_bo(struct si_pm4_state *state,
		   struct r600_resource *bo,
		   enum radeon_bo_usage usage,
		   enum radeon_bo_priority priority);

void si_pm4_sh_data_begin(struct si_pm4_state *state);
void si_pm4_sh_data_add(struct si_pm4_state *state, uint32_t dw);
void si_pm4_sh_data_end(struct si_pm4_state *state, unsigned base, unsigned idx);

void si_pm4_inval_shader_cache(struct si_pm4_state *state);
void si_pm4_inval_texture_cache(struct si_pm4_state *state);

void si_pm4_free_state(struct si_context *sctx,
		       struct si_pm4_state *state,
		       unsigned idx);
struct si_pm4_state * si_pm4_alloc_state(struct si_context *sctx);

uint32_t si_pm4_sync_flags(struct si_context *sctx);
unsigned si_pm4_dirty_dw(struct si_context *sctx);
void si_pm4_emit(struct si_context *sctx, struct si_pm4_state *state);
void si_pm4_emit_dirty(struct si_context *sctx);
void si_pm4_reset_emitted(struct si_context *sctx);
void si_pm4_cleanup(struct si_context *sctx);

#endif
