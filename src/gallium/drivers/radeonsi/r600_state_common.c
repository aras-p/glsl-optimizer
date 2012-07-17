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
				S_028430_STENCILTESTVAL(state->ref_value[0]) |
				S_028430_STENCILMASK(state->valuemask[0]) |
				S_028430_STENCILWRITEMASK(state->writemask[0]),
				NULL, 0);
	r600_pipe_state_add_reg(rstate,
				R_028434_DB_STENCILREFMASK_BF,
				S_028434_STENCILTESTVAL_BF(state->ref_value[1]) |
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
}

void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (state == NULL)
		return;

	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->pa_sc_line_stipple = rs->pa_sc_line_stipple;
	rctx->pa_su_sc_mode_cntl = rs->pa_su_sc_mode_cntl;
	rctx->pa_cl_clip_cntl = rs->pa_cl_clip_cntl;
	rctx->pa_cl_vs_out_cntl = rs->pa_cl_vs_out_cntl;

	rctx->rasterizer = rs;

	rctx->states[rs->rstate.id] = &rs->rstate;
	r600_context_pipe_state_set(rctx, &rs->rstate);

	if (rctx->chip_class >= CAYMAN) {
		cayman_polygon_offset_update(rctx);
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
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	memcpy(v->elements, elements, sizeof(struct pipe_vertex_element) * count);

	return v;
}

void *si_create_shader_state(struct pipe_context *ctx,
                             const struct pipe_shader_state *state)
{
	struct si_pipe_shader *shader = CALLOC_STRUCT(si_pipe_shader);

	shader->tokens = tgsi_dup_tokens(state->tokens);
	shader->so = state->stream_output;

	return shader;
}

void r600_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (rctx->ps_shader != state)
		rctx->shader_dirty = true;

	/* TODO delete old shader */
	rctx->ps_shader = (struct si_pipe_shader *)state;
	if (state) {
		r600_inval_shader_cache(rctx);
		r600_context_pipe_state_set(rctx, &rctx->ps_shader->rstate);
	}
}

void r600_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (rctx->vs_shader != state)
		rctx->shader_dirty = true;

	/* TODO delete old shader */
	rctx->vs_shader = (struct si_pipe_shader *)state;
	if (state) {
		r600_inval_shader_cache(rctx);
		r600_context_pipe_state_set(rctx, &rctx->vs_shader->rstate);
	}
}

void r600_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader *shader = (struct si_pipe_shader *)state;

	if (rctx->ps_shader == shader) {
		rctx->ps_shader = NULL;
	}

	free(shader->tokens);
	si_pipe_shader_destroy(ctx, shader);
	free(shader);
}

void r600_delete_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader *shader = (struct si_pipe_shader *)state;

	if (rctx->vs_shader == shader) {
		rctx->vs_shader = NULL;
	}

	free(shader->tokens);
	si_pipe_shader_destroy(ctx, shader);
	free(shader);
}

static void r600_update_alpha_ref(struct r600_context *rctx)
{
#if 0
	unsigned alpha_ref;
	struct r600_pipe_state rstate;

	alpha_ref = rctx->alpha_ref;
	rstate.nregs = 0;
	if (rctx->export_16bpc)
		alpha_ref &= ~0x1FFF;
	r600_pipe_state_add_reg(&rstate, R_028438_SX_ALPHA_REF, alpha_ref, NULL, 0);

	r600_context_pipe_state_set(rctx, &rstate);
	rctx->alpha_ref_dirty = false;
#endif
}

void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			      struct pipe_constant_buffer *cb)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_resource *rbuffer = cb ? r600_resource(cb->buffer) : NULL;
	struct r600_pipe_state *rstate;
	uint64_t va_offset;
	uint32_t offset;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (cb == NULL) {
		return;
	}

	r600_inval_shader_cache(rctx);

	if (cb->user_buffer)
		r600_upload_const_buffer(rctx, &rbuffer, cb->user_buffer, cb->buffer_size, &offset);
	else
		offset = 0;
	va_offset = r600_resource_va(ctx->screen, (void*)rbuffer);
	va_offset += offset;
	//va_offset >>= 8;

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		rstate = &rctx->vs_const_buffer;
		rstate->nregs = 0;
		r600_pipe_state_add_reg(rstate,
					R_00B130_SPI_SHADER_USER_DATA_VS_0,
					va_offset, rbuffer, RADEON_USAGE_READ);
		r600_pipe_state_add_reg(rstate,
					R_00B134_SPI_SHADER_USER_DATA_VS_1,
					va_offset >> 32, NULL, 0);
		break;
	case PIPE_SHADER_FRAGMENT:
		rstate = &rctx->ps_const_buffer;
		rstate->nregs = 0;
		r600_pipe_state_add_reg(rstate,
					R_00B030_SPI_SHADER_USER_DATA_PS_0,
					va_offset, rbuffer, RADEON_USAGE_READ);
		r600_pipe_state_add_reg(rstate,
					R_00B034_SPI_SHADER_USER_DATA_PS_1,
					va_offset >> 32, NULL, 0);
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	r600_context_pipe_state_set(rctx, rstate);

	if (cb->buffer != &rbuffer->b.b)
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

static void r600_vertex_buffer_update(struct r600_context *rctx)
{
	struct pipe_context *ctx = &rctx->context;
	struct r600_pipe_state *rstate = &rctx->vs_user_data;
	struct r600_resource *rbuffer, *t_list_buffer;
	struct pipe_vertex_buffer *vertex_buffer;
	unsigned i, count, offset;
	uint32_t *ptr;
	uint64_t va;

	r600_inval_vertex_cache(rctx);

	/* bind vertex buffer once */
	count = rctx->nr_vertex_buffers;
	assert(count <= 256 / 4);

	t_list_buffer = (struct r600_resource*)
		pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_IMMUTABLE, 4 * 4 * count);
	if (t_list_buffer == NULL)
		return;

	ptr = (uint32_t*)rctx->ws->buffer_map(t_list_buffer->cs_buf,
					      rctx->cs,
					      PIPE_TRANSFER_WRITE);

	for (i = 0 ; i < count; i++, ptr += 4) {
		struct pipe_vertex_element *velem = &rctx->vertex_elements->elements[i];
		const struct util_format_description *desc;
		unsigned data_format, num_format;
		int first_non_void;

		/* bind vertex buffer once */
		vertex_buffer = &rctx->vertex_buffer[i];
		rbuffer = (struct r600_resource*)vertex_buffer->buffer;
		offset = 0;
		if (vertex_buffer == NULL || rbuffer == NULL)
			continue;
		offset += vertex_buffer->buffer_offset;

		va = r600_resource_va(ctx->screen, (void*)rbuffer);
		va += offset;

		desc = util_format_description(velem->src_format);
		first_non_void = util_format_get_first_non_void_channel(velem->src_format);
		data_format = si_translate_vertexformat(ctx->screen,
							velem->src_format,
							desc, first_non_void);

		switch (desc->channel[first_non_void].type) {
		case UTIL_FORMAT_TYPE_FIXED:
			num_format = V_008F0C_BUF_NUM_FORMAT_USCALED; /* XXX */
			break;
		case UTIL_FORMAT_TYPE_SIGNED:
			num_format = V_008F0C_BUF_NUM_FORMAT_SNORM;
			break;
		case UTIL_FORMAT_TYPE_UNSIGNED:
			num_format = V_008F0C_BUF_NUM_FORMAT_UNORM;
			break;
		case UTIL_FORMAT_TYPE_FLOAT:
		default:
			num_format = V_008F14_IMG_NUM_FORMAT_FLOAT;
		}

		/* Fill in T# buffer resource description */
		ptr[0] = va & 0xFFFFFFFF;
		ptr[1] = (S_008F04_BASE_ADDRESS_HI(va >> 32) |
			  S_008F04_STRIDE(vertex_buffer->stride));
		if (vertex_buffer->stride > 0)
			ptr[2] = ((vertex_buffer->buffer->width0 - offset) /
				  vertex_buffer->stride);
		else
			ptr[2] = vertex_buffer->buffer->width0 - offset;
		ptr[3] = (S_008F0C_DST_SEL_X(si_map_swizzle(desc->swizzle[0])) |
			  S_008F0C_DST_SEL_Y(si_map_swizzle(desc->swizzle[1])) |
			  S_008F0C_DST_SEL_Z(si_map_swizzle(desc->swizzle[2])) |
			  S_008F0C_DST_SEL_W(si_map_swizzle(desc->swizzle[3])) |
			  S_008F0C_NUM_FORMAT(num_format) |
			  S_008F0C_DATA_FORMAT(data_format));

		r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ);
	}

	rstate->nregs = 0;

	va = r600_resource_va(ctx->screen, (void*)t_list_buffer);
	r600_pipe_state_add_reg(rstate,
				R_00B148_SPI_SHADER_USER_DATA_VS_6,
				va, t_list_buffer, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate,
				R_00B14C_SPI_SHADER_USER_DATA_VS_7,
				va >> 32,
				NULL, 0);

	r600_context_pipe_state_set(rctx, rstate);
}

static void si_update_derived_state(struct r600_context *rctx)
{
	struct pipe_context * ctx = (struct pipe_context*)rctx;

	if (!rctx->blitter->running) {
		if (rctx->have_depth_fb || rctx->have_depth_texture)
			r600_flush_depth_textures(rctx);
	}

	if ((rctx->ps_shader->shader.fs_write_all &&
	     (rctx->ps_shader->shader.nr_cbufs != rctx->nr_cbufs)) ||
	    (rctx->sprite_coord_enable &&
	     (rctx->ps_shader->sprite_coord_enable != rctx->sprite_coord_enable))) {
		si_pipe_shader_destroy(&rctx->context, rctx->ps_shader);
	}

	if (rctx->alpha_ref_dirty) {
		r600_update_alpha_ref(rctx);
	}

	if (!rctx->vs_shader->bo) {
		si_pipe_shader_vs(ctx, rctx->vs_shader);

		r600_context_pipe_state_set(rctx, &rctx->vs_shader->rstate);
	}

	if (!rctx->ps_shader->bo) {
		si_pipe_shader_ps(ctx, rctx->ps_shader);

		r600_context_pipe_state_set(rctx, &rctx->ps_shader->rstate);
	}

	if (rctx->shader_dirty) {
		si_update_spi_map(rctx);
		rctx->shader_dirty = false;
	}
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_dsa *dsa = (struct r600_pipe_dsa*)rctx->states[R600_PIPE_STATE_DSA];
	struct pipe_draw_info info = *dinfo;
	struct r600_draw rdraw = {};
	struct pipe_index_buffer ib = {};
	unsigned prim, mask, ls_mask = 0;
	struct r600_block *dirty_block = NULL, *next_block = NULL;
	struct r600_atom *state = NULL, *next_state = NULL;
	struct si_state_blend *blend;
	int i;

	if ((!info.count && (info.indexed || !info.count_from_stream_output)) ||
	    (info.indexed && !rctx->index_buffer.buffer) ||
	    !r600_conv_pipe_prim(info.mode, &prim)) {
		return;
	}

	if (!rctx->ps_shader || !rctx->vs_shader)
		return;

	/* only temporary */
	if (!rctx->queued.named.blend)
		return;
	blend = rctx->queued.named.blend;

	si_update_derived_state(rctx);

	r600_vertex_buffer_update(rctx);

	rdraw.vgt_num_indices = info.count;
	rdraw.vgt_num_instances = info.instance_count;

	if (info.indexed) {
		/* Initialize the index buffer struct. */
		pipe_resource_reference(&ib.buffer, rctx->index_buffer.buffer);
		ib.index_size = rctx->index_buffer.index_size;
		ib.offset = rctx->index_buffer.offset + info.start * ib.index_size;

		/* Translate or upload, if needed. */
		r600_translate_index_buffer(rctx, &ib, info.count);

		if (ib.user_buffer) {
			r600_upload_index_buffer(rctx, &ib, info.count);
		}

		/* Initialize the r600_draw struct with index buffer info. */
		if (ib.index_size == 4) {
			rdraw.vgt_index_type = V_028A7C_VGT_INDEX_32 |
				(R600_BIG_ENDIAN ? V_028A7C_VGT_DMA_SWAP_32_BIT : 0);
		} else {
			rdraw.vgt_index_type = V_028A7C_VGT_INDEX_16 |
				(R600_BIG_ENDIAN ? V_028A7C_VGT_DMA_SWAP_16_BIT : 0);
		}
		rdraw.indices = (struct r600_resource*)ib.buffer;
		rdraw.indices_bo_offset = ib.offset;
		rdraw.vgt_draw_initiator = V_0287F0_DI_SRC_SEL_DMA;
	} else {
		info.index_bias = info.start;
		rdraw.vgt_draw_initiator = V_0287F0_DI_SRC_SEL_AUTO_INDEX;
		if (info.count_from_stream_output) {
			rdraw.vgt_draw_initiator |= S_0287F0_USE_OPAQUE(1);

			r600_context_draw_opaque_count(rctx, (struct r600_so_target*)info.count_from_stream_output);
		}
	}

	rctx->vs_shader_so_strides = rctx->vs_shader->so_strides;

	mask = (1ULL << ((unsigned)rctx->framebuffer.nr_cbufs * 4)) - 1;

	if (rctx->vgt.id != R600_PIPE_STATE_VGT) {
		rctx->vgt.id = R600_PIPE_STATE_VGT;
		rctx->vgt.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vgt, R_008958_VGT_PRIMITIVE_TYPE, prim, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028238_CB_TARGET_MASK, blend->cb_target_mask & mask, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028400_VGT_MAX_VTX_INDX, ~0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028404_VGT_MIN_VTX_INDX, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028408_VGT_INDX_OFFSET, info.index_bias, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, info.restart_index, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, info.primitive_restart, NULL, 0);
#if 0
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF4_SQ_VTX_START_INST_LOC, info.start_instance, NULL, 0);
#endif
		r600_pipe_state_add_reg(&rctx->vgt, R_028A0C_PA_SC_LINE_STIPPLE, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028814_PA_SU_SC_MODE_CNTL, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_02881C_PA_CL_VS_OUT_CNTL, 0, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028810_PA_CL_CLIP_CNTL, 0x0, NULL, 0);
	}

	rctx->vgt.nregs = 0;
	r600_pipe_state_mod_reg(&rctx->vgt, prim);
	r600_pipe_state_mod_reg(&rctx->vgt, blend->cb_target_mask & mask);
	r600_pipe_state_mod_reg(&rctx->vgt, ~0);
	r600_pipe_state_mod_reg(&rctx->vgt, 0);
	r600_pipe_state_mod_reg(&rctx->vgt, info.index_bias);
	r600_pipe_state_mod_reg(&rctx->vgt, info.restart_index);
	r600_pipe_state_mod_reg(&rctx->vgt, info.primitive_restart);
#if 0
	r600_pipe_state_mod_reg(&rctx->vgt, 0);
	r600_pipe_state_mod_reg(&rctx->vgt, info.start_instance);
#endif

	if (prim == V_008958_DI_PT_LINELIST)
		ls_mask = 1;
	else if (prim == V_008958_DI_PT_LINESTRIP) 
		ls_mask = 2;
	r600_pipe_state_mod_reg(&rctx->vgt, S_028A0C_AUTO_RESET_CNTL(ls_mask) | rctx->pa_sc_line_stipple);

	if (info.mode == PIPE_PRIM_QUADS || info.mode == PIPE_PRIM_QUAD_STRIP || info.mode == PIPE_PRIM_POLYGON) {
		r600_pipe_state_mod_reg(&rctx->vgt, S_028814_PROVOKING_VTX_LAST(1) | rctx->pa_su_sc_mode_cntl);
	} else {
		r600_pipe_state_mod_reg(&rctx->vgt, rctx->pa_su_sc_mode_cntl);
	}
	r600_pipe_state_mod_reg(&rctx->vgt,
				prim == PIPE_PRIM_POINTS ? rctx->pa_cl_vs_out_cntl : 0
				/*| (rctx->rasterizer->clip_plane_enable &
				  rctx->vs_shader->shader.clip_dist_write)*/);
	r600_pipe_state_mod_reg(&rctx->vgt,
				rctx->pa_cl_clip_cntl /*|
				(rctx->vs_shader->shader.clip_dist_write ||
				 rctx->vs_shader->shader.vs_prohibit_ucps ?
				 0 : rctx->rasterizer->clip_plane_enable & 0x3F)*/);

	r600_context_pipe_state_set(rctx, &rctx->vgt);

	rdraw.db_render_override = dsa->db_render_override;
	rdraw.db_render_control = dsa->db_render_control;

	/* Emit states. */
	rctx->pm4_dirty_cdwords += si_pm4_dirty_dw(rctx);

	r600_need_cs_space(rctx, 0, TRUE);

	LIST_FOR_EACH_ENTRY_SAFE(state, next_state, &rctx->dirty_states, head) {
		r600_emit_atom(rctx, state);
	}
	LIST_FOR_EACH_ENTRY_SAFE(dirty_block, next_block, &rctx->dirty,list) {
		r600_context_block_emit_dirty(rctx, dirty_block);
	}
	si_pm4_emit_dirty(rctx);
	rctx->pm4_dirty_cdwords = 0;

	/* Enable stream out if needed. */
	if (rctx->streamout_start) {
		r600_context_streamout_begin(rctx);
		rctx->streamout_start = FALSE;
	}

	for (i = 0; i < NUM_TEX_UNITS; i++) {
		if (rctx->ps_samplers.views[i])
			r600_context_bo_reloc(rctx,
					      (struct r600_resource*)rctx->ps_samplers.views[i]->base.texture,
					      RADEON_USAGE_READ);
	}

	si_context_draw(rctx, &rdraw);

	rctx->flags |= R600_CONTEXT_DST_CACHES_DIRTY | R600_CONTEXT_DRAW_PENDING;

	if (rctx->framebuffer.zsbuf)
	{
		struct pipe_resource *tex = rctx->framebuffer.zsbuf->texture;
		((struct r600_resource_texture *)tex)->dirty_db = TRUE;
	}

	pipe_resource_reference(&ib.buffer, NULL);
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
