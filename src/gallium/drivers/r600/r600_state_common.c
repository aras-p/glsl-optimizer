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
#include "r600_formats.h"
#include "r600_pipe.h"
#include "r600d.h"
#include "r600_hw_context_priv.h"

static void r600_emit_command_buffer(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_command_buffer *cb = (struct r600_command_buffer*)atom;

	assert(cs->cdw + cb->atom.num_dw <= RADEON_MAX_CMDBUF_DWORDS);
	memcpy(cs->buf + cs->cdw, cb->buf, 4 * cb->atom.num_dw);
	cs->cdw += cb->atom.num_dw;
}

void r600_init_command_buffer(struct r600_command_buffer *cb, unsigned num_dw, enum r600_atom_flags flags)
{
	cb->atom.emit = r600_emit_command_buffer;
	cb->atom.num_dw = 0;
	cb->atom.flags = flags;
	cb->buf = CALLOC(1, 4 * num_dw);
	cb->max_num_dw = num_dw;
}

void r600_release_command_buffer(struct r600_command_buffer *cb)
{
	FREE(cb->buf);
}

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

void r600_init_atom(struct r600_atom *atom,
		    void (*emit)(struct r600_context *ctx, struct r600_atom *state),
		    unsigned num_dw, enum r600_atom_flags flags)
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

	/* Workaround for broken flushing on some R6xx chipsets. */
	if (rctx->family == CHIP_RV670 ||
	    rctx->family == CHIP_RS780 ||
	    rctx->family == CHIP_RS880) {
		flags |=  S_0085F0_CB1_DEST_BASE_ENA(1) |
			  S_0085F0_DEST_BASE_0_ENA(1);
	}
	return flags;
}

void r600_texture_barrier(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->atom_surface_sync.flush_flags |= S_0085F0_TC_ACTION_ENA(1) | r600_get_cb_flush_flags(rctx);
	r600_atom_dirty(rctx, &rctx->atom_surface_sync.atom);
}

static bool r600_conv_pipe_prim(unsigned pprim, unsigned *prim)
{
	static const int prim_conv[] = {
		V_008958_DI_PT_POINTLIST,
		V_008958_DI_PT_LINELIST,
		V_008958_DI_PT_LINELOOP,
		V_008958_DI_PT_LINESTRIP,
		V_008958_DI_PT_TRILIST,
		V_008958_DI_PT_TRISTRIP,
		V_008958_DI_PT_TRIFAN,
		V_008958_DI_PT_QUADLIST,
		V_008958_DI_PT_QUADSTRIP,
		V_008958_DI_PT_POLYGON,
		-1,
		-1,
		-1,
		-1
	};

	*prim = prim_conv[pprim];
	if (*prim == -1) {
		fprintf(stderr, "%s:%d unsupported %d\n", __func__, __LINE__, pprim);
		return false;
	}
	return true;
}

/* common state between evergreen and r600 */
void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_blend *blend = (struct r600_pipe_blend *)state;
	struct r600_pipe_state *rstate;

	if (state == NULL)
		return;
	rstate = &blend->rstate;
	rctx->states[rstate->id] = rstate;
	rctx->cb_target_mask = blend->cb_target_mask;

	/* Replace every bit except MULTIWRITE_ENABLE. */
	rctx->cb_color_control &= ~C_028808_MULTIWRITE_ENABLE;
	rctx->cb_color_control |= blend->cb_color_control & C_028808_MULTIWRITE_ENABLE;

	r600_context_pipe_state_set(rctx, rstate);
}

void r600_set_blend_color(struct pipe_context *ctx,
			  const struct pipe_blend_color *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rstate->id = R600_PIPE_STATE_BLEND_COLOR;
	r600_pipe_state_add_reg(rstate, R_028414_CB_BLEND_RED, fui(state->color[0]), NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028418_CB_BLEND_GREEN, fui(state->color[1]), NULL, 0);
	r600_pipe_state_add_reg(rstate, R_02841C_CB_BLEND_BLUE, fui(state->color[2]), NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028420_CB_BLEND_ALPHA, fui(state->color[3]), NULL, 0);

	free(rctx->states[R600_PIPE_STATE_BLEND_COLOR]);
	rctx->states[R600_PIPE_STATE_BLEND_COLOR] = rstate;
	r600_context_pipe_state_set(rctx, rstate);
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				 const struct r600_stencil_ref *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rstate->id = R600_PIPE_STATE_STENCIL_REF;
	r600_pipe_state_add_reg(rstate,
				R_028430_DB_STENCILREFMASK,
				S_028430_STENCILREF(state->ref_value[0]) |
				S_028430_STENCILMASK(state->valuemask[0]) |
				S_028430_STENCILWRITEMASK(state->writemask[0]),
				NULL, 0);
	r600_pipe_state_add_reg(rstate,
				R_028434_DB_STENCILREFMASK_BF,
				S_028434_STENCILREF_BF(state->ref_value[1]) |
				S_028434_STENCILMASK_BF(state->valuemask[1]) |
				S_028434_STENCILWRITEMASK_BF(state->writemask[1]),
				NULL, 0);

	free(rctx->states[R600_PIPE_STATE_STENCIL_REF]);
	rctx->states[R600_PIPE_STATE_STENCIL_REF] = rstate;
	r600_context_pipe_state_set(rctx, rstate);
}

void r600_set_pipe_stencil_ref(struct pipe_context *ctx,
			       const struct pipe_stencil_ref *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_dsa *dsa = (struct r600_pipe_dsa*)rctx->states[R600_PIPE_STATE_DSA];
	struct r600_stencil_ref ref;

	rctx->stencil_ref = *state;

	if (!dsa)
		return;

	ref.ref_value[0] = state->ref_value[0];
	ref.ref_value[1] = state->ref_value[1];
	ref.valuemask[0] = dsa->valuemask[0];
	ref.valuemask[1] = dsa->valuemask[1];
	ref.writemask[0] = dsa->writemask[0];
	ref.writemask[1] = dsa->writemask[1];

	r600_set_stencil_ref(ctx, &ref);
}

void r600_bind_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_dsa *dsa = state;
	struct r600_pipe_state *rstate;
	struct r600_stencil_ref ref;

	if (state == NULL)
		return;
	rstate = &dsa->rstate;
	rctx->states[rstate->id] = rstate;
	rctx->alpha_ref = dsa->alpha_ref;
	rctx->alpha_ref_dirty = true;
	r600_context_pipe_state_set(rctx, rstate);

	ref.ref_value[0] = rctx->stencil_ref.ref_value[0];
	ref.ref_value[1] = rctx->stencil_ref.ref_value[1];
	ref.valuemask[0] = dsa->valuemask[0];
	ref.valuemask[1] = dsa->valuemask[1];
	ref.writemask[0] = dsa->writemask[0];
	ref.writemask[1] = dsa->writemask[1];

	r600_set_stencil_ref(ctx, &ref);

	if (rctx->atom_db_misc_state.flush_depthstencil_enabled != dsa->is_flush) {
		rctx->atom_db_misc_state.flush_depthstencil_enabled = dsa->is_flush;
		r600_atom_dirty(rctx, &rctx->atom_db_misc_state.atom);
	}
}

void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (state == NULL)
		return;

	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->two_side = rs->two_side;
	rctx->pa_sc_line_stipple = rs->pa_sc_line_stipple;
	rctx->pa_cl_clip_cntl = rs->pa_cl_clip_cntl;

	rctx->rasterizer = rs;

	rctx->states[rs->rstate.id] = &rs->rstate;
	r600_context_pipe_state_set(rctx, &rs->rstate);

	if (rctx->chip_class >= EVERGREEN) {
		evergreen_polygon_offset_update(rctx);
	} else {
		r600_polygon_offset_update(rctx);
	}
}

void r600_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;

	if (rctx->rasterizer == rs) {
		rctx->rasterizer = NULL;
	}
	if (rctx->states[rs->rstate.id] == &rs->rstate) {
		rctx->states[rs->rstate.id] = NULL;
	}
	free(rs);
}

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
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	rctx->vertex_elements = v;
	if (v) {
		r600_inval_shader_cache(rctx);
		u_vbuf_bind_vertex_elements(rctx->vbuf_mgr, state,
						v->vmgr_elements);

		rctx->states[v->rstate.id] = &v->rstate;
		r600_context_pipe_state_set(rctx, &v->rstate);
	}
}

void r600_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	if (rctx->states[v->rstate.id] == &v->rstate) {
		rctx->states[v->rstate.id] = NULL;
	}
	if (rctx->vertex_elements == state)
		rctx->vertex_elements = NULL;

	pipe_resource_reference((struct pipe_resource**)&v->fetch_shader, NULL);
	u_vbuf_destroy_vertex_elements(rctx->vbuf_mgr, v->vmgr_elements);
	FREE(state);
}


void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	u_vbuf_set_index_buffer(rctx->vbuf_mgr, ib);
}

void r600_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
			     const struct pipe_vertex_buffer *buffers)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	int i;

	/* Zero states. */
	for (i = 0; i < count; i++) {
		if (!buffers[i].buffer) {
			r600_context_pipe_state_set_fs_resource(rctx, NULL, i);
		}
	}
	for (; i < rctx->vbuf_mgr->nr_real_vertex_buffers; i++) {
		r600_context_pipe_state_set_fs_resource(rctx, NULL, i);
	}

	u_vbuf_set_vertex_buffers(rctx->vbuf_mgr, count, buffers);
}

void *r600_create_vertex_elements(struct pipe_context *ctx,
				  unsigned count,
				  const struct pipe_vertex_element *elements)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	v->vmgr_elements =
		u_vbuf_create_vertex_elements(rctx->vbuf_mgr, count,
						  elements, v->elements);

	if (r600_vertex_elements_build_fetch_shader(rctx, v)) {
		FREE(v);
		return NULL;
	}

	return v;
}

void *r600_create_shader_state(struct pipe_context *ctx,
			       const struct pipe_shader_state *state)
{
	struct r600_pipe_shader *shader = CALLOC_STRUCT(r600_pipe_shader);
	int r;

	shader->tokens = tgsi_dup_tokens(state->tokens);
	shader->so = state->stream_output;

	r =  r600_pipe_shader_create(ctx, shader);
	if (r) {
		return NULL;
	}
	return shader;
}

void r600_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	/* TODO delete old shader */
	rctx->ps_shader = (struct r600_pipe_shader *)state;
	if (state) {
		r600_inval_shader_cache(rctx);
		r600_context_pipe_state_set(rctx, &rctx->ps_shader->rstate);

		rctx->cb_color_control &= C_028808_MULTIWRITE_ENABLE;
		rctx->cb_color_control |= S_028808_MULTIWRITE_ENABLE(!!rctx->ps_shader->shader.fs_write_all);
	}
	if (rctx->ps_shader && rctx->vs_shader) {
		r600_adjust_gprs(rctx);
	}
}

void r600_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	/* TODO delete old shader */
	rctx->vs_shader = (struct r600_pipe_shader *)state;
	if (state) {
		r600_inval_shader_cache(rctx);
		r600_context_pipe_state_set(rctx, &rctx->vs_shader->rstate);
	}
	if (rctx->ps_shader && rctx->vs_shader) {
		r600_adjust_gprs(rctx);
	}
}

void r600_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->ps_shader == shader) {
		rctx->ps_shader = NULL;
	}

	free(shader->tokens);
	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

void r600_delete_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->vs_shader == shader) {
		rctx->vs_shader = NULL;
	}

	free(shader->tokens);
	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

static void r600_update_alpha_ref(struct r600_context *rctx)
{
	unsigned alpha_ref;
	struct r600_pipe_state rstate;

	alpha_ref = rctx->alpha_ref;
	rstate.nregs = 0;
	if (rctx->export_16bpc)
		alpha_ref &= ~0x1FFF;
	r600_pipe_state_add_reg(&rstate, R_028438_SX_ALPHA_REF, alpha_ref, NULL, 0);

	r600_context_pipe_state_set(rctx, &rstate);
	rctx->alpha_ref_dirty = false;
}

void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			      struct pipe_resource *buffer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_resource *rbuffer = r600_resource(buffer);
	struct r600_pipe_resource_state *rstate;
	uint64_t va_offset;
	uint32_t offset;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (buffer == NULL) {
		return;
	}

	r600_inval_shader_cache(rctx);

	r600_upload_const_buffer(rctx, &rbuffer, &offset);
	va_offset = r600_resource_va(ctx->screen, (void*)rbuffer);
	va_offset += offset;
	va_offset >>= 8;

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		rctx->vs_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028180_ALU_CONST_BUFFER_SIZE_VS_0 + index * 4,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					NULL, 0);
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028980_ALU_CONST_CACHE_VS_0 + index * 4,
					va_offset, rbuffer, RADEON_USAGE_READ);
		r600_context_pipe_state_set(rctx, &rctx->vs_const_buffer);

		rstate = &rctx->vs_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}

		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(ctx, rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
		}
		r600_context_pipe_state_set_vs_resource(rctx, rstate, index);
		break;
	case PIPE_SHADER_FRAGMENT:
		rctx->ps_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					NULL, 0);
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028940_ALU_CONST_CACHE_PS_0,
					va_offset, rbuffer, RADEON_USAGE_READ);
		r600_context_pipe_state_set(rctx, &rctx->ps_const_buffer);

		rstate = &rctx->ps_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}
		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(ctx, rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
		}
		r600_context_pipe_state_set_ps_resource(rctx, rstate, index);
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	if (buffer != &rbuffer->b.b.b)
		pipe_resource_reference((struct pipe_resource**)&rbuffer, NULL);
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
	ptr = rctx->ws->buffer_map(t->filled_size->buf, rctx->cs, PIPE_TRANSFER_WRITE);
	memset(ptr, 0, t->filled_size->buf->size);
	rctx->ws->buffer_unmap(t->filled_size->buf);

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

static void r600_vertex_buffer_update(struct r600_context *rctx)
{
	struct r600_pipe_resource_state *rstate;
	struct r600_resource *rbuffer;
	struct pipe_vertex_buffer *vertex_buffer;
	unsigned i, count, offset;

	r600_inval_vertex_cache(rctx);

	if (rctx->vertex_elements->vbuffer_need_offset) {
		/* one resource per vertex elements */
		count = rctx->vertex_elements->count;
	} else {
		/* bind vertex buffer once */
		count = rctx->vbuf_mgr->nr_real_vertex_buffers;
	}

	for (i = 0 ; i < count; i++) {
		rstate = &rctx->fs_resource[i];

		if (rctx->vertex_elements->vbuffer_need_offset) {
			/* one resource per vertex elements */
			unsigned vbuffer_index;
			vbuffer_index = rctx->vertex_elements->elements[i].vertex_buffer_index;
			vertex_buffer = &rctx->vbuf_mgr->real_vertex_buffer[vbuffer_index];
			rbuffer = (struct r600_resource*)vertex_buffer->buffer;
			offset = rctx->vertex_elements->vbuffer_offset[i];
		} else {
			/* bind vertex buffer once */
			vertex_buffer = &rctx->vbuf_mgr->real_vertex_buffer[i];
			rbuffer = (struct r600_resource*)vertex_buffer->buffer;
			offset = 0;
		}
		if (vertex_buffer == NULL || rbuffer == NULL)
			continue;
		offset += vertex_buffer->buffer_offset;

		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}

		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(&rctx->context, rstate, rbuffer, offset, vertex_buffer->stride, RADEON_USAGE_READ);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, vertex_buffer->stride, RADEON_USAGE_READ);
		}
		r600_context_pipe_state_set_fs_resource(rctx, rstate, i);
	}
}

static int r600_shader_rebuild(struct pipe_context * ctx, struct r600_pipe_shader * shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	int r;

	r600_pipe_shader_destroy(ctx, shader);
	r = r600_pipe_shader_create(ctx, shader);
	if (r) {
		return r;
	}
	r600_context_pipe_state_set(rctx, &shader->rstate);

	return 0;
}

static void r600_update_derived_state(struct r600_context *rctx)
{
	struct pipe_context * ctx = (struct pipe_context*)rctx;
	struct r600_pipe_state rstate;

	rstate.nregs = 0;

	if (rstate.nregs)
		r600_context_pipe_state_set(rctx, &rstate);

	if (!rctx->blitter->running) {
		if (rctx->have_depth_fb || rctx->have_depth_texture)
			r600_flush_depth_textures(rctx);
	}

	if (rctx->chip_class < EVERGREEN) {
		r600_update_sampler_states(rctx);
	}

	if ((rctx->ps_shader->shader.two_side != rctx->two_side) ||
	    ((rctx->chip_class >= EVERGREEN) && rctx->ps_shader->shader.fs_write_all &&
	     (rctx->ps_shader->shader.nr_cbufs != rctx->nr_cbufs))) {
		r600_shader_rebuild(&rctx->context, rctx->ps_shader);
	}

	if (rctx->alpha_ref_dirty) {
		r600_update_alpha_ref(rctx);
	}

	if (rctx->ps_shader && ((rctx->sprite_coord_enable &&
		(rctx->ps_shader->sprite_coord_enable != rctx->sprite_coord_enable)) ||
		(rctx->rasterizer && rctx->rasterizer->flatshade != rctx->ps_shader->flatshade))) {

		if (rctx->chip_class >= EVERGREEN)
			evergreen_pipe_shader_ps(ctx, rctx->ps_shader);
		else
			r600_pipe_shader_ps(ctx, rctx->ps_shader);

		r600_context_pipe_state_set(rctx, &rctx->ps_shader->rstate);
	}

}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_draw_info info = *dinfo;
	struct pipe_index_buffer ib = {};
	unsigned prim, mask, ls_mask = 0;
	struct r600_block *dirty_block = NULL, *next_block = NULL;
	struct r600_atom *state = NULL, *next_state = NULL;
	struct radeon_winsys_cs *cs = rctx->cs;
	uint64_t va;

	if ((!info.count && (info.indexed || !info.count_from_stream_output)) ||
	    (info.indexed && !rctx->vbuf_mgr->index_buffer.buffer) ||
	    !r600_conv_pipe_prim(info.mode, &prim)) {
		return;
	}

	if (!rctx->ps_shader || !rctx->vs_shader)
		return;

	r600_update_derived_state(rctx);

	u_vbuf_draw_begin(rctx->vbuf_mgr, &info);
	r600_vertex_buffer_update(rctx);

	if (info.indexed) {
		/* Initialize the index buffer struct. */
		pipe_resource_reference(&ib.buffer, rctx->vbuf_mgr->index_buffer.buffer);
		ib.index_size = rctx->vbuf_mgr->index_buffer.index_size;
		ib.offset = rctx->vbuf_mgr->index_buffer.offset + info.start * ib.index_size;

		/* Translate or upload, if needed. */
		r600_translate_index_buffer(rctx, &ib, info.count);

		if (u_vbuf_resource(ib.buffer)->user_ptr) {
			r600_upload_index_buffer(rctx, &ib, info.count);
		}
	} else {
		info.index_bias = info.start;
		if (info.count_from_stream_output) {
			r600_context_draw_opaque_count(rctx, (struct r600_so_target*)info.count_from_stream_output);
		}
	}

	mask = (1ULL << ((unsigned)rctx->framebuffer.nr_cbufs * 4)) - 1;

	if (rctx->vgt.id != R600_PIPE_STATE_VGT) {
		rctx->vgt.id = R600_PIPE_STATE_VGT;
		rctx->vgt.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vgt, R_008958_VGT_PRIMITIVE_TYPE, prim, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028238_CB_TARGET_MASK, rctx->cb_target_mask & mask, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028408_VGT_INDX_OFFSET, info.index_bias, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, info.restart_index, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, info.primitive_restart, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF4_SQ_VTX_START_INST_LOC, info.start_instance, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028A0C_PA_SC_LINE_STIPPLE, 0, NULL, 0);
		if (rctx->chip_class <= R700)
			r600_pipe_state_add_reg(&rctx->vgt, R_028808_CB_COLOR_CONTROL, rctx->cb_color_control, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_02881C_PA_CL_VS_OUT_CNTL, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028810_PA_CL_CLIP_CNTL, 0, NULL, 0);
	}

	rctx->vgt.nregs = 0;
	r600_pipe_state_mod_reg(&rctx->vgt, prim);
	r600_pipe_state_mod_reg(&rctx->vgt, rctx->cb_target_mask & mask);
	r600_pipe_state_mod_reg(&rctx->vgt, info.index_bias);
	r600_pipe_state_mod_reg(&rctx->vgt, info.restart_index);
	r600_pipe_state_mod_reg(&rctx->vgt, info.primitive_restart);
	r600_pipe_state_mod_reg(&rctx->vgt, info.start_instance);

	if (prim == V_008958_DI_PT_LINELIST)
		ls_mask = 1;
	else if (prim == V_008958_DI_PT_LINESTRIP) 
		ls_mask = 2;
	r600_pipe_state_mod_reg(&rctx->vgt, S_028A0C_AUTO_RESET_CNTL(ls_mask) | rctx->pa_sc_line_stipple);
	if (rctx->chip_class <= R700)
		r600_pipe_state_mod_reg(&rctx->vgt, rctx->cb_color_control);
	r600_pipe_state_mod_reg(&rctx->vgt,
				rctx->vs_shader->pa_cl_vs_out_cntl |
				(rctx->rasterizer->clip_plane_enable & rctx->vs_shader->shader.clip_dist_write));
	r600_pipe_state_mod_reg(&rctx->vgt,
				rctx->pa_cl_clip_cntl |
				(rctx->vs_shader->shader.clip_dist_write ||
				 rctx->vs_shader->shader.vs_prohibit_ucps ?
				 0 : rctx->rasterizer->clip_plane_enable & 0x3F));

	r600_context_pipe_state_set(rctx, &rctx->vgt);

	/* Emit states (the function expects that we emit at most 17 dwords here). */
	r600_need_cs_space(rctx, 0, TRUE);

	LIST_FOR_EACH_ENTRY_SAFE(state, next_state, &rctx->dirty_states, head) {
		r600_emit_atom(rctx, state);
	}
	LIST_FOR_EACH_ENTRY_SAFE(dirty_block, next_block, &rctx->dirty,list) {
		r600_context_block_emit_dirty(rctx, dirty_block);
	}
	LIST_FOR_EACH_ENTRY_SAFE(dirty_block, next_block, &rctx->resource_dirty,list) {
		r600_context_block_resource_emit_dirty(rctx, dirty_block);
	}
	rctx->pm4_dirty_cdwords = 0;

	/* Enable stream out if needed. */
	if (rctx->streamout_start) {
		r600_context_streamout_begin(rctx);
		rctx->streamout_start = FALSE;
	}

	/* draw packet */
	cs->buf[cs->cdw++] = PKT3(PKT3_INDEX_TYPE, 0, rctx->predicate_drawing);
	cs->buf[cs->cdw++] = ib.index_size == 4 ?
				(VGT_INDEX_32 | (R600_BIG_ENDIAN ? VGT_DMA_SWAP_32_BIT : 0)) :
				(VGT_INDEX_16 | (R600_BIG_ENDIAN ? VGT_DMA_SWAP_16_BIT : 0));
	cs->buf[cs->cdw++] = PKT3(PKT3_NUM_INSTANCES, 0, rctx->predicate_drawing);
	cs->buf[cs->cdw++] = info.instance_count;
	if (info.indexed) {
		va = r600_resource_va(ctx->screen, ib.buffer);
		va += ib.offset;
		cs->buf[cs->cdw++] = PKT3(PKT3_DRAW_INDEX, 3, rctx->predicate_drawing);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		cs->buf[cs->cdw++] = info.count;
		cs->buf[cs->cdw++] = V_0287F0_DI_SRC_SEL_DMA;
		cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, rctx->predicate_drawing);
		cs->buf[cs->cdw++] = r600_context_bo_reloc(rctx, (struct r600_resource*)ib.buffer, RADEON_USAGE_READ);
	} else {
		cs->buf[cs->cdw++] = PKT3(PKT3_DRAW_INDEX_AUTO, 1, rctx->predicate_drawing);
		cs->buf[cs->cdw++] = info.count;
		cs->buf[cs->cdw++] = V_0287F0_DI_SRC_SEL_AUTO_INDEX |
					(info.count_from_stream_output ? S_0287F0_USE_OPAQUE(1) : 0);
	}

	rctx->flags |= R600_CONTEXT_DST_CACHES_DIRTY | R600_CONTEXT_DRAW_PENDING;

	if (rctx->framebuffer.zsbuf)
	{
		struct pipe_resource *tex = rctx->framebuffer.zsbuf->texture;
		((struct r600_resource_texture *)tex)->dirty_db = TRUE;
	}

	pipe_resource_reference(&ib.buffer, NULL);
	u_vbuf_draw_end(rctx->vbuf_mgr);
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

uint32_t r600_translate_stencil_op(int s_op)
{
	switch (s_op) {
	case PIPE_STENCIL_OP_KEEP:
		return V_028800_STENCIL_KEEP;
	case PIPE_STENCIL_OP_ZERO:
		return V_028800_STENCIL_ZERO;
	case PIPE_STENCIL_OP_REPLACE:
		return V_028800_STENCIL_REPLACE;
	case PIPE_STENCIL_OP_INCR:
		return V_028800_STENCIL_INCR;
	case PIPE_STENCIL_OP_DECR:
		return V_028800_STENCIL_DECR;
	case PIPE_STENCIL_OP_INCR_WRAP:
		return V_028800_STENCIL_INCR_WRAP;
	case PIPE_STENCIL_OP_DECR_WRAP:
		return V_028800_STENCIL_DECR_WRAP;
	case PIPE_STENCIL_OP_INVERT:
		return V_028800_STENCIL_INVERT;
	default:
		R600_ERR("Unknown stencil op %d", s_op);
		assert(0);
		break;
	}
	return 0;
}

uint32_t r600_translate_fill(uint32_t func)
{
	switch(func) {
	case PIPE_POLYGON_MODE_FILL:
		return 2;
	case PIPE_POLYGON_MODE_LINE:
		return 1;
	case PIPE_POLYGON_MODE_POINT:
		return 0;
	default:
		assert(0);
		return 0;
	}
}

unsigned r600_tex_wrap(unsigned wrap)
{
	switch (wrap) {
	default:
	case PIPE_TEX_WRAP_REPEAT:
		return V_03C000_SQ_TEX_WRAP;
	case PIPE_TEX_WRAP_CLAMP:
		return V_03C000_SQ_TEX_CLAMP_HALF_BORDER;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return V_03C000_SQ_TEX_CLAMP_LAST_TEXEL;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return V_03C000_SQ_TEX_CLAMP_BORDER;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return V_03C000_SQ_TEX_MIRROR;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		return V_03C000_SQ_TEX_MIRROR_ONCE_HALF_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return V_03C000_SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		return V_03C000_SQ_TEX_MIRROR_ONCE_BORDER;
	}
}

unsigned r600_tex_filter(unsigned filter)
{
	switch (filter) {
	default:
	case PIPE_TEX_FILTER_NEAREST:
		return V_03C000_SQ_TEX_XY_FILTER_POINT;
	case PIPE_TEX_FILTER_LINEAR:
		return V_03C000_SQ_TEX_XY_FILTER_BILINEAR;
	}
}

unsigned r600_tex_mipfilter(unsigned filter)
{
	switch (filter) {
	case PIPE_TEX_MIPFILTER_NEAREST:
		return V_03C000_SQ_TEX_Z_FILTER_POINT;
	case PIPE_TEX_MIPFILTER_LINEAR:
		return V_03C000_SQ_TEX_Z_FILTER_LINEAR;
	default:
	case PIPE_TEX_MIPFILTER_NONE:
		return V_03C000_SQ_TEX_Z_FILTER_NONE;
	}
}

unsigned r600_tex_compare(unsigned compare)
{
	switch (compare) {
	default:
	case PIPE_FUNC_NEVER:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_NEVER;
	case PIPE_FUNC_LESS:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_LESS;
	case PIPE_FUNC_EQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_EQUAL;
	case PIPE_FUNC_LEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_LESSEQUAL;
	case PIPE_FUNC_GREATER:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_GREATER;
	case PIPE_FUNC_NOTEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_NOTEQUAL;
	case PIPE_FUNC_GEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL;
	case PIPE_FUNC_ALWAYS:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_ALWAYS;
	}
}
