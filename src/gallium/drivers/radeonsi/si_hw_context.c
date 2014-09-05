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

#include "si_pipe.h"

/* initialize */
void si_need_cs_space(struct si_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{
	int i;

	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->b.rings.gfx.cs->cdw;

	if (count_draw_in) {
		for (i = 0; i < SI_NUM_ATOMS(ctx); i++) {
			if (ctx->atoms.array[i]->dirty) {
				num_dw += ctx->atoms.array[i]->num_dw;
			}
		}

		/* The number of dwords all the dirty states would take. */
		num_dw += ctx->pm4_dirty_cdwords;

		/* The upper-bound of how much a draw command would take. */
		num_dw += SI_MAX_DRAW_CS_DWORDS;
	}

	/* Count in queries_suspend. */
	num_dw += ctx->b.num_cs_dw_nontimer_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	if (ctx->b.streamout.begin_emitted) {
		num_dw += ctx->b.streamout.num_dw_for_end;
	}

	/* Count in render_condition(NULL) at the end of CS. */
	if (ctx->b.predicate_drawing) {
		num_dw += 3;
	}

	/* Count in framebuffer cache flushes at the end of CS. */
	num_dw += ctx->atoms.s.cache_flush->num_dw;

#if SI_TRACE_CS
	if (ctx->screen->b.trace_bo) {
		num_dw += SI_TRACE_CS_DWORDS;
	}
#endif

	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		ctx->b.rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
	}
}

void si_context_gfx_flush(void *context, unsigned flags,
			  struct pipe_fence_handle **fence)
{
	struct si_context *ctx = context;
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;

	if (cs->cdw == ctx->b.initial_gfx_cs_size && !fence)
		return;

	ctx->b.rings.gfx.flushing = true;

	r600_preflush_suspend_features(&ctx->b);

	ctx->b.flags |= R600_CONTEXT_FLUSH_AND_INV_CB |
			R600_CONTEXT_FLUSH_AND_INV_CB_META |
			R600_CONTEXT_FLUSH_AND_INV_DB |
			R600_CONTEXT_FLUSH_AND_INV_DB_META |
			R600_CONTEXT_INV_TEX_CACHE |
			/* this is probably not needed anymore */
			R600_CONTEXT_PS_PARTIAL_FLUSH;
	si_emit_cache_flush(&ctx->b, NULL);

	/* force to keep tiling flags */
	flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;

	/* Flush the CS. */
	ctx->b.ws->cs_flush(cs, flags, fence, ctx->screen->b.cs_count++);
	ctx->b.rings.gfx.flushing = false;

#if SI_TRACE_CS
	if (ctx->screen->b.trace_bo) {
		struct si_screen *sscreen = ctx->screen;
		unsigned i;

		for (i = 0; i < 10; i++) {
			usleep(5);
			if (!ctx->b.ws->buffer_is_busy(sscreen->b.trace_bo->buf, RADEON_USAGE_READWRITE)) {
				break;
			}
		}
		if (i == 10) {
			fprintf(stderr, "timeout on cs lockup likely happen at cs %d dw %d\n",
				sscreen->b.trace_ptr[1], sscreen->b.trace_ptr[0]);
		} else {
			fprintf(stderr, "cs %d executed in %dms\n", sscreen->b.trace_ptr[1], i * 5);
		}
	}
#endif

	si_begin_new_cs(ctx);
}

void si_begin_new_cs(struct si_context *ctx)
{
	ctx->pm4_dirty_cdwords = 0;

	/* Flush read caches at the beginning of CS. */
	ctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			R600_CONTEXT_INV_CONST_CACHE |
			R600_CONTEXT_INV_SHADER_CACHE;

	/* set all valid group as dirty so they get reemited on
	 * next draw command
	 */
	si_pm4_reset_emitted(ctx);

	/* The CS initialization should be emitted before everything else. */
	si_pm4_emit(ctx, ctx->queued.named.init);
	ctx->emitted.named.init = ctx->queued.named.init;

	ctx->framebuffer.atom.dirty = true;
	ctx->msaa_config.dirty = true;
	ctx->db_render_state.dirty = true;
	ctx->b.streamout.enable_atom.dirty = true;
	si_all_descriptors_begin_new_cs(ctx);

	r600_postflush_resume_features(&ctx->b);

	ctx->b.initial_gfx_cs_size = ctx->b.rings.gfx.cs->cdw;
}
