/*
 * Copyright 2010 Red Hat Inc.
 *           2010 Jerome Glisse
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
 * Authors: Dave Airlie <airlied@redhat.com>
 *          Jerome Glisse <jglisse@redhat.com>
 */
#include "util/u_blitter.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "pipebuffer/pb_buffer.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "r600_hw_context_priv.h"
#include "radeonsi_pipe.h"
#include "sid.h"
#include "si_state.h"

static void r600_emit_surface_sync(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_atom_surface_sync *a = (struct r600_atom_surface_sync*)atom;

	cs->buf[cs->cdw++] = PKT3(PKT3_SURFACE_SYNC, 3, 0);
	cs->buf[cs->cdw++] = a->flush_flags;  /* CP_COHER_CNTL */
	cs->buf[cs->cdw++] = 0xffffffff;      /* CP_COHER_SIZE */
	cs->buf[cs->cdw++] = 0;               /* CP_COHER_BASE */
	cs->buf[cs->cdw++] = 0x0000000A;      /* POLL_INTERVAL */

	a->flush_flags = 0;
}

static void r600_emit_r6xx_flush_and_inv(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT) | EVENT_INDEX(0);
}

static void r600_init_atom(struct r600_atom *atom,
			   void (*emit)(struct r600_context *ctx, struct r600_atom *state),
			   unsigned num_dw,
			   enum r600_atom_flags flags)
{
	atom->emit = emit;
	atom->num_dw = num_dw;
	atom->flags = flags;
}

void r600_init_common_atoms(struct r600_context *rctx)
{
	r600_init_atom(&rctx->atom_surface_sync.atom,	r600_emit_surface_sync,		5, EMIT_EARLY);
	r600_init_atom(&rctx->atom_r6xx_flush_and_inv,	r600_emit_r6xx_flush_and_inv,	2, EMIT_EARLY);
}

unsigned r600_get_cb_flush_flags(struct r600_context *rctx)
{
	unsigned flags = 0;

	if (rctx->framebuffer.nr_cbufs) {
		flags |= S_0085F0_CB_ACTION_ENA(1) |
			 (((1 << rctx->framebuffer.nr_cbufs) - 1) << S_0085F0_CB0_DEST_BASE_ENA_SHIFT);
	}

	return flags;
}

void r600_texture_barrier(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->atom_surface_sync.flush_flags |= S_0085F0_TC_ACTION_ENA(1) | r600_get_cb_flush_flags(rctx);
	r600_atom_dirty(rctx, &rctx->atom_surface_sync.atom);
}

/* common state between evergreen and r600 */
void r600_sampler_view_destroy(struct pipe_context *ctx,
			       struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = (struct r600_pipe_sampler_view *)state;

	pipe_resource_reference(&state->texture, NULL);
	FREE(resource);
}

void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = (struct r600_pipe_state *)state;

	if (rctx->states[rstate->id] == rstate) {
		rctx->states[rstate->id] = NULL;
	}
	for (int i = 0; i < rstate->nregs; i++) {
		pipe_resource_reference((struct pipe_resource**)&rstate->regs[i].bo, NULL);
	}
	free(rstate);
}

void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_vertex_element *v = (struct r600_vertex_element*)state;

	rctx->vertex_elements = v;
	if (v) {
		r600_inval_shader_cache(rctx);
	}
}

void r600_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	if (rctx->vertex_elements == state)
		rctx->vertex_elements = NULL;
	FREE(state);
}


void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
	        memcpy(&rctx->index_buffer, ib, sizeof(*ib));
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
	}
}

void r600_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
			     const struct pipe_vertex_buffer *buffers)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	util_copy_vertex_buffers(rctx->vertex_buffer, &rctx->nr_vertex_buffers, buffers, count);
}

void *si_create_vertex_elements(struct pipe_context *ctx,
				unsigned count,
				const struct pipe_vertex_element *elements)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_vertex_element *v = CALLOC_STRUCT(si_vertex_element);

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	memcpy(v->elements, elements, sizeof(struct pipe_vertex_element) * count);

	return v;
}

struct pipe_stream_output_target *
r600_create_so_target(struct pipe_context *ctx,
		      struct pipe_resource *buffer,
		      unsigned buffer_offset,
		      unsigned buffer_size)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_so_target *t;
	void *ptr;

	t = CALLOC_STRUCT(r600_so_target);
	if (!t) {
		return NULL;
	}

	t->b.reference.count = 1;
	t->b.context = ctx;
	pipe_resource_reference(&t->b.buffer, buffer);
	t->b.buffer_offset = buffer_offset;
	t->b.buffer_size = buffer_size;

	t->filled_size = (struct r600_resource*)
		pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_STATIC, 4);
	ptr = rctx->ws->buffer_map(t->filled_size->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);
	memset(ptr, 0, t->filled_size->buf->size);
	rctx->ws->buffer_unmap(t->filled_size->cs_buf);

	return &t->b;
}

void r600_so_target_destroy(struct pipe_context *ctx,
			    struct pipe_stream_output_target *target)
{
	struct r600_so_target *t = (struct r600_so_target*)target;
	pipe_resource_reference(&t->b.buffer, NULL);
	pipe_resource_reference((struct pipe_resource**)&t->filled_size, NULL);
	FREE(t);
}

void r600_set_so_targets(struct pipe_context *ctx,
			 unsigned num_targets,
			 struct pipe_stream_output_target **targets,
			 unsigned append_bitmask)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned i;

	/* Stop streamout. */
	if (rctx->num_so_targets) {
		r600_context_streamout_end(rctx);
	}

	/* Set the new targets. */
	for (i = 0; i < num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->so_targets[i], targets[i]);
	}
	for (; i < rctx->num_so_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->so_targets[i], NULL);
	}

	rctx->num_so_targets = num_targets;
	rctx->streamout_start = num_targets != 0;
	rctx->streamout_append_bitmask = append_bitmask;
}

void _r600_pipe_state_add_reg(struct r600_context *ctx,
			      struct r600_pipe_state *state,
			      uint32_t offset, uint32_t value,
			      uint32_t range_id, uint32_t block_id,
			      struct r600_resource *bo,
			      enum radeon_bo_usage usage)
{
	struct r600_range *range;
	struct r600_block *block;

	if (bo) assert(usage);

	range = &ctx->range[range_id];
	block = range->blocks[block_id];
	state->regs[state->nregs].block = block;
	state->regs[state->nregs].id = (offset - block->start_offset) >> 2;

	state->regs[state->nregs].value = value;
	state->regs[state->nregs].bo = bo;
	state->regs[state->nregs].bo_usage = usage;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}

void r600_pipe_state_add_reg_noblock(struct r600_pipe_state *state,
				     uint32_t offset, uint32_t value,
				     struct r600_resource *bo,
				     enum radeon_bo_usage usage)
{
	if (bo) assert(usage);

	state->regs[state->nregs].id = offset;
	state->regs[state->nregs].block = NULL;
	state->regs[state->nregs].value = value;
	state->regs[state->nregs].bo = bo;
	state->regs[state->nregs].bo_usage = usage;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}
