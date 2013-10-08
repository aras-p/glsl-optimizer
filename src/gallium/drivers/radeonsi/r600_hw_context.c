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
#include "../radeon/r600_cs.h"
#include "radeonsi_pm4.h"
#include "radeonsi_pipe.h"
#include "sid.h"
#include "util/u_memory.h"
#include <errno.h>

#define GROUP_FORCE_NEW_BLOCK	0

/* Get backends mask */
void si_get_backend_mask(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;
	struct r600_resource *buffer;
	uint32_t *results;
	unsigned num_backends = ctx->screen->b.info.r600_num_backends;
	unsigned i, mask = 0;

	/* if backend_map query is supported by the kernel */
	if (ctx->screen->b.info.r600_backend_map_valid) {
		unsigned num_tile_pipes = ctx->screen->b.info.r600_num_tile_pipes;
		unsigned backend_map = ctx->screen->b.info.r600_backend_map;
		unsigned item_width = 4, item_mask = 0x7;

		while(num_tile_pipes--) {
			i = backend_map & item_mask;
			mask |= (1<<i);
			backend_map >>= item_width;
		}
		if (mask != 0) {
			ctx->backend_mask = mask;
			return;
		}
	}

	/* otherwise backup path for older kernels */

	/* create buffer for event data */
	buffer = r600_resource_create_custom(&ctx->screen->b.b,
					   PIPE_USAGE_STAGING,
					   ctx->max_db*16);
	if (!buffer)
		goto err;

	/* initialize buffer with zeroes */
	results = ctx->b.ws->buffer_map(buffer->cs_buf, ctx->b.rings.gfx.cs, PIPE_TRANSFER_WRITE);
	if (results) {
		uint64_t va = 0;

		memset(results, 0, ctx->max_db * 4 * 4);
		ctx->b.ws->buffer_unmap(buffer->cs_buf);

		/* emit EVENT_WRITE for ZPASS_DONE */
		va = r600_resource_va(&ctx->screen->b.b, (void *)buffer);
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = va >> 32;

		cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
		cs->buf[cs->cdw++] = r600_context_bo_reloc(&ctx->b, &ctx->b.rings.gfx, buffer, RADEON_USAGE_WRITE);

		/* analyze results */
		results = ctx->b.ws->buffer_map(buffer->cs_buf, ctx->b.rings.gfx.cs, PIPE_TRANSFER_READ);
		if (results) {
			for(i = 0; i < ctx->max_db; i++) {
				/* at least highest bit will be set if backend is used */
				if (results[i*4 + 1])
					mask |= (1<<i);
			}
			ctx->b.ws->buffer_unmap(buffer->cs_buf);
		}
	}

	r600_resource_reference(&buffer, NULL);

	if (mask != 0) {
		ctx->backend_mask = mask;
		return;
	}

err:
	/* fallback to old method - set num_backends lower bits to 1 */
	ctx->backend_mask = (~((uint32_t)0))>>(32-num_backends);
	return;
}

bool si_is_timer_query(unsigned type)
{
	return type == PIPE_QUERY_TIME_ELAPSED ||
		type == PIPE_QUERY_TIMESTAMP ||
		type == PIPE_QUERY_TIMESTAMP_DISJOINT;
}

bool si_query_needs_begin(unsigned type)
{
	return type != PIPE_QUERY_TIMESTAMP;
}

/* initialize */
void si_need_cs_space(struct r600_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{
	int i;

	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->b.rings.gfx.cs->cdw;

	for (i = 0; i < SI_NUM_ATOMS(ctx); i++) {
		if (ctx->atoms.array[i]->dirty) {
			num_dw += ctx->atoms.array[i]->num_dw;
		}
	}

	if (count_draw_in) {
		/* The number of dwords all the dirty states would take. */
		num_dw += ctx->pm4_dirty_cdwords;

		/* The upper-bound of how much a draw command would take. */
		num_dw += SI_MAX_DRAW_CS_DWORDS;
	}

	/* Count in queries_suspend. */
	num_dw += ctx->num_cs_dw_nontimer_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	if (ctx->b.streamout.begin_emitted) {
		num_dw += ctx->b.streamout.num_dw_for_end;
	}

	/* Count in render_condition(NULL) at the end of CS. */
	if (ctx->predicate_drawing) {
		num_dw += 3;
	}

	/* Count in framebuffer cache flushes at the end of CS. */
	num_dw += ctx->atoms.cache_flush->num_dw;

#if R600_TRACE_CS
	if (ctx->screen->trace_bo) {
		num_dw += R600_TRACE_CS_DWORDS;
	}
#endif

	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		radeonsi_flush(&ctx->b.b, NULL, RADEON_FLUSH_ASYNC);
	}
}

void si_context_flush(struct r600_context *ctx, unsigned flags)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;

	if (!cs->cdw)
		return;

	/* suspend queries */
	ctx->nontimer_queries_suspended = false;
	if (ctx->num_cs_dw_nontimer_queries_suspend) {
		r600_context_queries_suspend(ctx);
		ctx->nontimer_queries_suspended = true;
	}

	ctx->b.streamout.suspended = false;

	if (ctx->b.streamout.begin_emitted) {
		r600_emit_streamout_end(&ctx->b);
		ctx->b.streamout.suspended = true;
	}

	ctx->b.flags |= R600_CONTEXT_FLUSH_AND_INV_CB |
			R600_CONTEXT_FLUSH_AND_INV_CB_META |
			R600_CONTEXT_FLUSH_AND_INV_DB |
			R600_CONTEXT_INV_TEX_CACHE;
	si_emit_cache_flush(&ctx->b, NULL);

	/* this is probably not needed anymore */
	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);

	/* force to keep tiling flags */
	flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;

#if R600_TRACE_CS
	if (ctx->screen->trace_bo) {
		struct r600_screen *rscreen = ctx->screen;
		unsigned i;

		for (i = 0; i < cs->cdw; i++) {
			fprintf(stderr, "[%4d] [%5d] 0x%08x\n", rscreen->cs_count, i, cs->buf[i]);
		}
		rscreen->cs_count++;
	}
#endif

	/* Flush the CS. */
	ctx->b.ws->cs_flush(ctx->b.rings.gfx.cs, flags, 0);

#if R600_TRACE_CS
	if (ctx->screen->trace_bo) {
		struct r600_screen *rscreen = ctx->screen;
		unsigned i;

		for (i = 0; i < 10; i++) {
			usleep(5);
			if (!ctx->ws->buffer_is_busy(rscreen->trace_bo->buf, RADEON_USAGE_READWRITE)) {
				break;
			}
		}
		if (i == 10) {
			fprintf(stderr, "timeout on cs lockup likely happen at cs %d dw %d\n",
				rscreen->trace_ptr[1], rscreen->trace_ptr[0]);
		} else {
			fprintf(stderr, "cs %d executed in %dms\n", rscreen->trace_ptr[1], i * 5);
		}
	}
#endif

	si_begin_new_cs(ctx);
}

void si_begin_new_cs(struct r600_context *ctx)
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

	if (ctx->b.streamout.suspended) {
		ctx->b.streamout.append_bitmask = ctx->b.streamout.enabled_mask;
		r600_streamout_buffers_dirty(&ctx->b);
	}

	/* resume queries */
	if (ctx->nontimer_queries_suspended) {
		r600_context_queries_resume(ctx);
	}

	si_all_descriptors_begin_new_cs(ctx);
}

static unsigned r600_query_read_result(char *map, unsigned start_index, unsigned end_index,
				       bool test_status_bit)
{
	uint32_t *current_result = (uint32_t*)map;
	uint64_t start, end;

	start = (uint64_t)current_result[start_index] |
		(uint64_t)current_result[start_index+1] << 32;
	end = (uint64_t)current_result[end_index] |
	      (uint64_t)current_result[end_index+1] << 32;

	if (!test_status_bit ||
	    ((start & 0x8000000000000000UL) && (end & 0x8000000000000000UL))) {
		return end - start;
	}
	return 0;
}

static boolean r600_query_result(struct r600_context *ctx, struct r600_query *query, boolean wait)
{
	unsigned results_base = query->results_start;
	char *map;

	map = ctx->b.ws->buffer_map(query->buffer->cs_buf, ctx->b.rings.gfx.cs,
				  PIPE_TRANSFER_READ |
				  (wait ? 0 : PIPE_TRANSFER_DONTBLOCK));
	if (!map)
		return FALSE;

	/* count all results across all data blocks */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 2, true);
			results_base = (results_base + 16) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		while (results_base != query->results_end) {
			query->result.b = query->result.b ||
				r600_query_read_result(map + results_base, 0, 2, true) != 0;
			results_base = (results_base + 16) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_TIMESTAMP:
	{
		uint32_t *current_result = (uint32_t*)map;
		query->result.u64 = (uint64_t)current_result[0] | (uint64_t)current_result[1] << 32;
		break;
	}
	case PIPE_QUERY_TIME_ELAPSED:
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 2, false);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
		/* SAMPLE_STREAMOUTSTATS stores this structure:
		 * {
		 *    u64 NumPrimitivesWritten;
		 *    u64 PrimitiveStorageNeeded;
		 * }
		 * We only need NumPrimitivesWritten here. */
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 2, 6, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		/* Here we read PrimitiveStorageNeeded. */
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_SO_STATISTICS:
		while (results_base != query->results_end) {
			query->result.so.num_primitives_written +=
				r600_query_read_result(map + results_base, 2, 6, true);
			query->result.so.primitives_storage_needed +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		while (results_base != query->results_end) {
			query->result.b = query->result.b ||
				r600_query_read_result(map + results_base, 2, 6, true) !=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	default:
		assert(0);
	}

	query->results_start = query->results_end;
	ctx->b.ws->buffer_unmap(query->buffer->cs_buf);
	return TRUE;
}

void r600_query_begin(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;
	unsigned new_results_end, i;
	uint32_t *results;
	uint64_t va;

	si_need_cs_space(ctx, query->num_cs_dw * 2, TRUE);

	new_results_end = (query->results_end + query->result_size) % query->buffer->b.b.width0;

	/* collect current results if query buffer is full */
	if (new_results_end == query->results_start) {
		r600_query_result(ctx, query, TRUE);
	}

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		results = ctx->b.ws->buffer_map(query->buffer->cs_buf, ctx->b.rings.gfx.cs, PIPE_TRANSFER_WRITE);
		if (results) {
			results = (uint32_t*)((char*)results + query->results_end);
			memset(results, 0, query->result_size);

			/* Set top bits for unused backends */
			for (i = 0; i < ctx->max_db; i++) {
				if (!(ctx->backend_mask & (1<<i))) {
					results[(i * 4)+1] = 0x80000000;
					results[(i * 4)+3] = 0x80000000;
				}
			}
			ctx->b.ws->buffer_unmap(query->buffer->cs_buf);
		}
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		results = ctx->b.ws->buffer_map(query->buffer->cs_buf, ctx->b.rings.gfx.cs, PIPE_TRANSFER_WRITE);
		results = (uint32_t*)((char*)results + query->results_end);
		memset(results, 0, query->result_size);
		ctx->b.ws->buffer_unmap(query->buffer->cs_buf);
		break;
	default:
		assert(0);
	}

	/* emit begin query */
	va = r600_resource_va(&ctx->screen->b.b, (void*)query->buffer);
	va += query->results_end;

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SAMPLE_STREAMOUTSTATS) | EVENT_INDEX(3);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (3 << 29) | ((va >> 32UL) & 0xFF);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = 0;
		break;
	default:
		assert(0);
	}
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(&ctx->b, &ctx->b.rings.gfx, query->buffer, RADEON_USAGE_WRITE);

	if (!si_is_timer_query(query->type)) {
		ctx->num_cs_dw_nontimer_queries_suspend += query->num_cs_dw;
	}
}

void r600_query_end(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;
	uint64_t va;
	unsigned new_results_end;

	/* The queries which need begin already called this in begin_query. */
	if (!si_query_needs_begin(query->type)) {
		si_need_cs_space(ctx, query->num_cs_dw, TRUE);

		new_results_end = (query->results_end + query->result_size) % query->buffer->b.b.width0;

		/* collect current results if query buffer is full */
		if (new_results_end == query->results_start) {
		r600_query_result(ctx, query, TRUE);
		}
	}

	va = r600_resource_va(&ctx->screen->b.b, (void*)query->buffer);
	/* emit end query */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		va += query->results_end + 8;
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		va += query->results_end + query->result_size/2;
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SAMPLE_STREAMOUTSTATS) | EVENT_INDEX(3);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		va += query->results_end + query->result_size/2;
		/* fall through */
	case PIPE_QUERY_TIMESTAMP:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (3 << 29) | ((va >> 32UL) & 0xFF);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = 0;
		break;
	default:
		assert(0);
	}
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(&ctx->b, &ctx->b.rings.gfx, query->buffer, RADEON_USAGE_WRITE);

	query->results_end = (query->results_end + query->result_size) % query->buffer->b.b.width0;

	if (si_query_needs_begin(query->type) && !si_is_timer_query(query->type)) {
		ctx->num_cs_dw_nontimer_queries_suspend -= query->num_cs_dw;
	}
}

void r600_query_predication(struct r600_context *ctx, struct r600_query *query, int operation,
			    int flag_wait)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;
	uint64_t va;

	if (operation == PREDICATION_OP_CLEAR) {
		si_need_cs_space(ctx, 3, FALSE);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = PRED_OP(PREDICATION_OP_CLEAR);
	} else {
		unsigned results_base = query->results_start;
		unsigned count;
		uint32_t op;

		/* find count of the query data blocks */
		count = (query->buffer->b.b.width0 + query->results_end - query->results_start) % query->buffer->b.b.width0;
		count /= query->result_size;

		si_need_cs_space(ctx, 5 * count, TRUE);

		op = PRED_OP(operation) | PREDICATION_DRAW_VISIBLE |
				(flag_wait ? PREDICATION_HINT_WAIT : PREDICATION_HINT_NOWAIT_DRAW);
		va = r600_resource_va(&ctx->screen->b.b, (void*)query->buffer);

		/* emit predicate packets for all data blocks */
		while (results_base != query->results_end) {
			cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
			cs->buf[cs->cdw++] = (va + results_base) & 0xFFFFFFFFUL;
			cs->buf[cs->cdw++] = op | (((va + results_base) >> 32UL) & 0xFF);
			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] = r600_context_bo_reloc(&ctx->b, &ctx->b.rings.gfx,
								   query->buffer, RADEON_USAGE_READ);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;

			/* set CONTINUE bit for all packets except the first */
			op |= PREDICATION_CONTINUE;
		}
	}
}

struct r600_query *r600_context_query_create(struct r600_context *ctx, unsigned query_type)
{
	struct r600_query *query;
	unsigned buffer_size = 4096;

	query = CALLOC_STRUCT(r600_query);
	if (query == NULL)
		return NULL;

	query->type = query_type;

	switch (query_type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		query->result_size = 16 * ctx->max_db;
		query->num_cs_dw = 6;
		break;
	case PIPE_QUERY_TIMESTAMP:
		query->result_size = 8;
		query->num_cs_dw = 8;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		query->result_size = 16;
		query->num_cs_dw = 8;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		/* NumPrimitivesWritten, PrimitiveStorageNeeded. */
		query->result_size = 32;
		query->num_cs_dw = 6;
		break;
	default:
		assert(0);
		FREE(query);
		return NULL;
	}

	/* adjust buffer size to simplify offsets wrapping math */
	buffer_size -= buffer_size % query->result_size;

	/* Queries are normally read by the CPU after
	 * being written by the gpu, hence staging is probably a good
	 * usage pattern.
	 */
	query->buffer = r600_resource_create_custom(&ctx->screen->b.b,
						  PIPE_USAGE_STAGING,
						  buffer_size);
	if (!query->buffer) {
		FREE(query);
		return NULL;
	}
	return query;
}

void r600_context_query_destroy(struct r600_context *ctx, struct r600_query *query)
{
	r600_resource_reference(&query->buffer, NULL);
	free(query);
}

boolean r600_context_query_result(struct r600_context *ctx,
				struct r600_query *query,
				boolean wait, void *vresult)
{
	boolean *result_b = (boolean*)vresult;
	uint64_t *result_u64 = (uint64_t*)vresult;
	struct pipe_query_data_so_statistics *result_so =
		(struct pipe_query_data_so_statistics*)vresult;

	if (!r600_query_result(ctx, query, wait))
		return FALSE;

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		*result_u64 = query->result.u64;
		break;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		*result_b = query->result.b;
		break;
	case PIPE_QUERY_TIMESTAMP:
	case PIPE_QUERY_TIME_ELAPSED:
		*result_u64 = (1000000 * query->result.u64) / ctx->screen->b.info.r600_clock_crystal_freq;
		break;
	case PIPE_QUERY_SO_STATISTICS:
		*result_so = query->result.so;
		break;
	default:
		assert(0);
	}
	return TRUE;
}

void r600_context_queries_suspend(struct r600_context *ctx)
{
	struct r600_query *query;

	LIST_FOR_EACH_ENTRY(query, &ctx->active_nontimer_query_list, list) {
		r600_query_end(ctx, query);
	}
	assert(ctx->num_cs_dw_nontimer_queries_suspend == 0);
}

void r600_context_queries_resume(struct r600_context *ctx)
{
	struct r600_query *query;

	assert(ctx->num_cs_dw_nontimer_queries_suspend == 0);

	LIST_FOR_EACH_ENTRY(query, &ctx->active_nontimer_query_list, list) {
		r600_query_begin(ctx, query);
	}
}

#if R600_TRACE_CS
void r600_trace_emit(struct r600_context *rctx)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_winsys_cs *cs = rctx->cs;
	uint64_t va;

	va = r600_resource_va(&rscreen->screen, (void*)rscreen->trace_bo);
	r600_context_bo_reloc(rctx, rscreen->trace_bo, RADEON_USAGE_READWRITE);
	cs->buf[cs->cdw++] = PKT3(PKT3_WRITE_DATA, 4, 0);
	cs->buf[cs->cdw++] = PKT3_WRITE_DATA_DST_SEL(PKT3_WRITE_DATA_DST_SEL_MEM_SYNC) |
				PKT3_WRITE_DATA_WR_CONFIRM |
				PKT3_WRITE_DATA_ENGINE_SEL(PKT3_WRITE_DATA_ENGINE_SEL_ME);
	cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL;
	cs->buf[cs->cdw++] = (va >> 32UL) & 0xFFFFFFFFUL;
	cs->buf[cs->cdw++] = cs->cdw;
	cs->buf[cs->cdw++] = rscreen->cs_count;
}
#endif
