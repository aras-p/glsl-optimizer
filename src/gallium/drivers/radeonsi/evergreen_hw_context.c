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
#include "r600.h"
#include "r600_hw_context_priv.h"
#include "radeonsi_pipe.h"
#include "sid.h"
#include "util/u_memory.h"
#include <errno.h>

int si_context_init(struct r600_context *ctx)
{
	int r;

	LIST_INITHEAD(&ctx->active_query_list);

	ctx->cs = ctx->ws->cs_create(ctx->ws);

	ctx->max_db = 8;
	return 0;
}

void si_context_draw(struct r600_context *ctx, const struct r600_draw *draw)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	unsigned ndwords = 7;
	uint32_t *pm4;
	uint64_t va;

	if (draw->indices) {
		ndwords = 12;
	}
	if (ctx->num_cs_dw_queries_suspend)
		ndwords += 6;

	/* when increasing ndwords, bump the max limit too */
	assert(ndwords <= SI_MAX_DRAW_CS_DWORDS);

	/* queries need some special values
	 * (this is non-zero if any query is active) */
	if (ctx->num_cs_dw_queries_suspend) {
		pm4 = &cs->buf[cs->cdw];
		pm4[0] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		pm4[1] = (R_028004_DB_COUNT_CONTROL - SI_CONTEXT_REG_OFFSET) >> 2;
		pm4[2] = S_028004_PERFECT_ZPASS_COUNTS(1);
		pm4[3] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		pm4[4] = (R_02800C_DB_RENDER_OVERRIDE - SI_CONTEXT_REG_OFFSET) >> 2;
		pm4[5] = draw->db_render_override | S_02800C_NOOP_CULL_DISABLE(1);
		cs->cdw += 6;
		ndwords -= 6;
	}

	/* draw packet */
	pm4 = &cs->buf[cs->cdw];
	pm4[0] = PKT3(PKT3_INDEX_TYPE, 0, ctx->predicate_drawing);
	pm4[1] = draw->vgt_index_type;
	pm4[2] = PKT3(PKT3_NUM_INSTANCES, 0, ctx->predicate_drawing);
	pm4[3] = draw->vgt_num_instances;
	if (draw->indices) {
		va = r600_resource_va(&ctx->screen->screen, (void*)draw->indices);
		va += draw->indices_bo_offset;
		pm4[4] = PKT3(PKT3_DRAW_INDEX_2, 4, ctx->predicate_drawing);
		pm4[5] = (draw->indices->b.b.width0 - draw->indices_bo_offset) /
			ctx->index_buffer.index_size;
		pm4[6] = va;
		pm4[7] = (va >> 32UL) & 0xFF;
		pm4[8] = draw->vgt_num_indices;
		pm4[9] = draw->vgt_draw_initiator;
		pm4[10] = PKT3(PKT3_NOP, 0, ctx->predicate_drawing);
		pm4[11] = r600_context_bo_reloc(ctx, draw->indices, RADEON_USAGE_READ);
	} else {
		pm4[4] = PKT3(PKT3_DRAW_INDEX_AUTO, 1, ctx->predicate_drawing);
		pm4[5] = draw->vgt_num_indices;
		pm4[6] = draw->vgt_draw_initiator;
	}
	cs->cdw += ndwords;
}
