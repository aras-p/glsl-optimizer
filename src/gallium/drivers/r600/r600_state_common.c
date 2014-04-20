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
#include "r600_formats.h"
#include "r600_shader.h"
#include "r600d.h"

#include "util/u_draw_quad.h"
#include "util/u_format_s3tc.h"
#include "util/u_index_modify.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"

#define R600_PRIM_RECTANGLE_LIST PIPE_PRIM_MAX

void r600_init_command_buffer(struct r600_command_buffer *cb, unsigned num_dw)
{
	assert(!cb->buf);
	cb->buf = CALLOC(1, 4 * num_dw);
	cb->max_num_dw = num_dw;
}

void r600_release_command_buffer(struct r600_command_buffer *cb)
{
	FREE(cb->buf);
}

void r600_init_atom(struct r600_context *rctx,
		    struct r600_atom *atom,
		    unsigned id,
		    void (*emit)(struct r600_context *ctx, struct r600_atom *state),
		    unsigned num_dw)
{
	assert(id < R600_NUM_ATOMS);
	assert(rctx->atoms[id] == NULL);
	rctx->atoms[id] = atom;
	atom->emit = (void*)emit;
	atom->num_dw = num_dw;
	atom->dirty = false;
}

void r600_emit_cso_state(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_command_buffer(rctx->b.rings.gfx.cs, ((struct r600_cso_state*)atom)->cb);
}

void r600_emit_alphatest_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_alphatest_state *a = (struct r600_alphatest_state*)atom;
	unsigned alpha_ref = a->sx_alpha_ref;

	if (rctx->b.chip_class >= EVERGREEN && a->cb0_export_16bpc) {
		alpha_ref &= ~0x1FFF;
	}

	r600_write_context_reg(cs, R_028410_SX_ALPHA_TEST_CONTROL,
			       a->sx_alpha_test_control |
			       S_028410_ALPHA_TEST_BYPASS(a->bypass));
	r600_write_context_reg(cs, R_028438_SX_ALPHA_REF, alpha_ref);
}

static void r600_texture_barrier(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
		       R600_CONTEXT_FLUSH_AND_INV_CB |
		       R600_CONTEXT_FLUSH_AND_INV |
		       R600_CONTEXT_WAIT_3D_IDLE;
}

static unsigned r600_conv_pipe_prim(unsigned prim)
{
	static const unsigned prim_conv[] = {
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
		V_008958_DI_PT_LINELIST_ADJ,
		V_008958_DI_PT_LINESTRIP_ADJ,
		V_008958_DI_PT_TRILIST_ADJ,
		V_008958_DI_PT_TRISTRIP_ADJ,
		V_008958_DI_PT_RECTLIST
	};
	return prim_conv[prim];
}

/* common state between evergreen and r600 */

static void r600_bind_blend_state_internal(struct r600_context *rctx,
		struct r600_blend_state *blend, bool blend_disable)
{
	unsigned color_control;
	bool update_cb = false;

	rctx->alpha_to_one = blend->alpha_to_one;
	rctx->dual_src_blend = blend->dual_src_blend;

	if (!blend_disable) {
		r600_set_cso_state_with_cb(&rctx->blend_state, blend, &blend->buffer);
		color_control = blend->cb_color_control;
	} else {
		/* Blending is disabled. */
		r600_set_cso_state_with_cb(&rctx->blend_state, blend, &blend->buffer_no_blend);
		color_control = blend->cb_color_control_no_blend;
	}

	/* Update derived states. */
	if (rctx->cb_misc_state.blend_colormask != blend->cb_target_mask) {
		rctx->cb_misc_state.blend_colormask = blend->cb_target_mask;
		update_cb = true;
	}
	if (rctx->b.chip_class <= R700 &&
	    rctx->cb_misc_state.cb_color_control != color_control) {
		rctx->cb_misc_state.cb_color_control = color_control;
		update_cb = true;
	}
	if (rctx->cb_misc_state.dual_src_blend != blend->dual_src_blend) {
		rctx->cb_misc_state.dual_src_blend = blend->dual_src_blend;
		update_cb = true;
	}
	if (update_cb) {
		rctx->cb_misc_state.atom.dirty = true;
	}
}

static void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_blend_state *blend = (struct r600_blend_state *)state;

	if (blend == NULL)
		return;

	r600_bind_blend_state_internal(rctx, blend, rctx->force_blend_disable);
}

static void r600_set_blend_color(struct pipe_context *ctx,
				 const struct pipe_blend_color *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->blend_color.state = *state;
	rctx->blend_color.atom.dirty = true;
}

void r600_emit_blend_color(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct pipe_blend_color *state = &rctx->blend_color.state;

	r600_write_context_reg_seq(cs, R_028414_CB_BLEND_RED, 4);
	radeon_emit(cs, fui(state->color[0])); /* R_028414_CB_BLEND_RED */
	radeon_emit(cs, fui(state->color[1])); /* R_028418_CB_BLEND_GREEN */
	radeon_emit(cs, fui(state->color[2])); /* R_02841C_CB_BLEND_BLUE */
	radeon_emit(cs, fui(state->color[3])); /* R_028420_CB_BLEND_ALPHA */
}

void r600_emit_vgt_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_vgt_state *a = (struct r600_vgt_state *)atom;

	r600_write_context_reg(cs, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, a->vgt_multi_prim_ib_reset_en);
	r600_write_context_reg_seq(cs, R_028408_VGT_INDX_OFFSET, 2);
	radeon_emit(cs, a->vgt_indx_offset); /* R_028408_VGT_INDX_OFFSET */
	radeon_emit(cs, a->vgt_multi_prim_ib_reset_indx); /* R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX */
}

static void r600_set_clip_state(struct pipe_context *ctx,
				const struct pipe_clip_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_constant_buffer cb;

	rctx->clip_state.state = *state;
	rctx->clip_state.atom.dirty = true;

	cb.buffer = NULL;
	cb.user_buffer = state->ucp;
	cb.buffer_offset = 0;
	cb.buffer_size = 4*4*8;
	ctx->set_constant_buffer(ctx, PIPE_SHADER_VERTEX, R600_UCP_CONST_BUFFER, &cb);
	pipe_resource_reference(&cb.buffer, NULL);
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				 const struct r600_stencil_ref *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->stencil_ref.state = *state;
	rctx->stencil_ref.atom.dirty = true;
}

void r600_emit_stencil_ref(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_stencil_ref_state *a = (struct r600_stencil_ref_state*)atom;

	r600_write_context_reg_seq(cs, R_028430_DB_STENCILREFMASK, 2);
	radeon_emit(cs, /* R_028430_DB_STENCILREFMASK */
			 S_028430_STENCILREF(a->state.ref_value[0]) |
			 S_028430_STENCILMASK(a->state.valuemask[0]) |
			 S_028430_STENCILWRITEMASK(a->state.writemask[0]));
	radeon_emit(cs, /* R_028434_DB_STENCILREFMASK_BF */
			 S_028434_STENCILREF_BF(a->state.ref_value[1]) |
			 S_028434_STENCILMASK_BF(a->state.valuemask[1]) |
			 S_028434_STENCILWRITEMASK_BF(a->state.writemask[1]));
}

static void r600_set_pipe_stencil_ref(struct pipe_context *ctx,
				      const struct pipe_stencil_ref *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_dsa_state *dsa = (struct r600_dsa_state*)rctx->dsa_state.cso;
	struct r600_stencil_ref ref;

	rctx->stencil_ref.pipe_state = *state;

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

static void r600_bind_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_dsa_state *dsa = state;
	struct r600_stencil_ref ref;

	if (state == NULL) {
		r600_set_cso_state_with_cb(&rctx->dsa_state, NULL, NULL);
		return;
	}

	r600_set_cso_state_with_cb(&rctx->dsa_state, dsa, &dsa->buffer);

	ref.ref_value[0] = rctx->stencil_ref.pipe_state.ref_value[0];
	ref.ref_value[1] = rctx->stencil_ref.pipe_state.ref_value[1];
	ref.valuemask[0] = dsa->valuemask[0];
	ref.valuemask[1] = dsa->valuemask[1];
	ref.writemask[0] = dsa->writemask[0];
	ref.writemask[1] = dsa->writemask[1];
	if (rctx->zwritemask != dsa->zwritemask) {
		rctx->zwritemask = dsa->zwritemask;
		if (rctx->b.chip_class >= EVERGREEN) {
			/* work around some issue when not writting to zbuffer
			 * we are having lockup on evergreen so do not enable
			 * hyperz when not writting zbuffer
			 */
			rctx->db_misc_state.atom.dirty = true;
		}
	}

	r600_set_stencil_ref(ctx, &ref);

	/* Update alphatest state. */
	if (rctx->alphatest_state.sx_alpha_test_control != dsa->sx_alpha_test_control ||
	    rctx->alphatest_state.sx_alpha_ref != dsa->alpha_ref) {
		rctx->alphatest_state.sx_alpha_test_control = dsa->sx_alpha_test_control;
		rctx->alphatest_state.sx_alpha_ref = dsa->alpha_ref;
		rctx->alphatest_state.atom.dirty = true;
	}
}

static void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_rasterizer_state *rs = (struct r600_rasterizer_state *)state;
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (state == NULL)
		return;

	rctx->rasterizer = rs;

	r600_set_cso_state_with_cb(&rctx->rasterizer_state, rs, &rs->buffer);

	if (rs->offset_enable &&
	    (rs->offset_units != rctx->poly_offset_state.offset_units ||
	     rs->offset_scale != rctx->poly_offset_state.offset_scale)) {
		rctx->poly_offset_state.offset_units = rs->offset_units;
		rctx->poly_offset_state.offset_scale = rs->offset_scale;
		rctx->poly_offset_state.atom.dirty = true;
	}

	/* Update clip_misc_state. */
	if (rctx->clip_misc_state.pa_cl_clip_cntl != rs->pa_cl_clip_cntl ||
	    rctx->clip_misc_state.clip_plane_enable != rs->clip_plane_enable) {
		rctx->clip_misc_state.pa_cl_clip_cntl = rs->pa_cl_clip_cntl;
		rctx->clip_misc_state.clip_plane_enable = rs->clip_plane_enable;
		rctx->clip_misc_state.atom.dirty = true;
	}

	/* Workaround for a missing scissor enable on r600. */
	if (rctx->b.chip_class == R600 &&
	    rs->scissor_enable != rctx->scissor[0].enable) {
		rctx->scissor[0].enable = rs->scissor_enable;
		rctx->scissor[0].atom.dirty = true;
	}

	/* Re-emit PA_SC_LINE_STIPPLE. */
	rctx->last_primitive_type = -1;
}

static void r600_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_rasterizer_state *rs = (struct r600_rasterizer_state *)state;

	r600_release_command_buffer(&rs->buffer);
	FREE(rs);
}

static void r600_sampler_view_destroy(struct pipe_context *ctx,
				      struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = (struct r600_pipe_sampler_view *)state;

	pipe_resource_reference(&state->texture, NULL);
	FREE(resource);
}

void r600_sampler_states_dirty(struct r600_context *rctx,
			       struct r600_sampler_states *state)
{
	if (state->dirty_mask) {
		if (state->dirty_mask & state->has_bordercolor_mask) {
			rctx->b.flags |= R600_CONTEXT_WAIT_3D_IDLE;
		}
		state->atom.num_dw =
			util_bitcount(state->dirty_mask & state->has_bordercolor_mask) * 11 +
			util_bitcount(state->dirty_mask & ~state->has_bordercolor_mask) * 5;
		state->atom.dirty = true;
	}
}

static void r600_bind_sampler_states(struct pipe_context *pipe,
                               unsigned shader,
			       unsigned start,
			       unsigned count, void **states)
{
	struct r600_context *rctx = (struct r600_context *)pipe;
	struct r600_textures_info *dst = &rctx->samplers[shader];
	struct r600_pipe_sampler_state **rstates = (struct r600_pipe_sampler_state**)states;
	int seamless_cube_map = -1;
	unsigned i;
	/* This sets 1-bit for states with index >= count. */
	uint32_t disable_mask = ~((1ull << count) - 1);
	/* These are the new states set by this function. */
	uint32_t new_mask = 0;

	assert(start == 0); /* XXX fix below */

	if (shader != PIPE_SHADER_VERTEX &&
	    shader != PIPE_SHADER_FRAGMENT) {
		return;
	}

	for (i = 0; i < count; i++) {
		struct r600_pipe_sampler_state *rstate = rstates[i];

		if (rstate == dst->states.states[i]) {
			continue;
		}

		if (rstate) {
			if (rstate->border_color_use) {
				dst->states.has_bordercolor_mask |= 1 << i;
			} else {
				dst->states.has_bordercolor_mask &= ~(1 << i);
			}
			seamless_cube_map = rstate->seamless_cube_map;

			new_mask |= 1 << i;
		} else {
			disable_mask |= 1 << i;
		}
	}

	memcpy(dst->states.states, rstates, sizeof(void*) * count);
	memset(dst->states.states + count, 0, sizeof(void*) * (NUM_TEX_UNITS - count));

	dst->states.enabled_mask &= ~disable_mask;
	dst->states.dirty_mask &= dst->states.enabled_mask;
	dst->states.enabled_mask |= new_mask;
	dst->states.dirty_mask |= new_mask;
	dst->states.has_bordercolor_mask &= dst->states.enabled_mask;

	r600_sampler_states_dirty(rctx, &dst->states);

	/* Seamless cubemap state. */
	if (rctx->b.chip_class <= R700 &&
	    seamless_cube_map != -1 &&
	    seamless_cube_map != rctx->seamless_cube_map.enabled) {
		/* change in TA_CNTL_AUX need a pipeline flush */
		rctx->b.flags |= R600_CONTEXT_WAIT_3D_IDLE;
		rctx->seamless_cube_map.enabled = seamless_cube_map;
		rctx->seamless_cube_map.atom.dirty = true;
	}
}

static void r600_delete_sampler_state(struct pipe_context *ctx, void *state)
{
	free(state);
}

static void r600_delete_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_blend_state *blend = (struct r600_blend_state*)state;

	r600_release_command_buffer(&blend->buffer);
	r600_release_command_buffer(&blend->buffer_no_blend);
	FREE(blend);
}

static void r600_delete_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_dsa_state *dsa = (struct r600_dsa_state *)state;

	if (rctx->dsa_state.cso == state) {
		ctx->bind_depth_stencil_alpha_state(ctx, NULL);
	}

	r600_release_command_buffer(&dsa->buffer);
	free(dsa);
}

static void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_set_cso_state(&rctx->vertex_fetch_shader, state);
}

static void r600_delete_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_fetch_shader *shader = (struct r600_fetch_shader*)state;
	pipe_resource_reference((struct pipe_resource**)&shader->buffer, NULL);
	FREE(shader);
}

static void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
		memcpy(&rctx->index_buffer, ib, sizeof(*ib));
		r600_context_add_resource_size(ctx, ib->buffer);
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
	}
}

void r600_vertex_buffers_dirty(struct r600_context *rctx)
{
	if (rctx->vertex_buffer_state.dirty_mask) {
		rctx->b.flags |= R600_CONTEXT_INV_VERTEX_CACHE;
		rctx->vertex_buffer_state.atom.num_dw = (rctx->b.chip_class >= EVERGREEN ? 12 : 11) *
					       util_bitcount(rctx->vertex_buffer_state.dirty_mask);
		rctx->vertex_buffer_state.atom.dirty = true;
	}
}

static void r600_set_vertex_buffers(struct pipe_context *ctx,
				    unsigned start_slot, unsigned count,
				    const struct pipe_vertex_buffer *input)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_vertexbuf_state *state = &rctx->vertex_buffer_state;
	struct pipe_vertex_buffer *vb = state->vb + start_slot;
	unsigned i;
	uint32_t disable_mask = 0;
	/* These are the new buffers set by this function. */
	uint32_t new_buffer_mask = 0;

	/* Set vertex buffers. */
	if (input) {
		for (i = 0; i < count; i++) {
			if (memcmp(&input[i], &vb[i], sizeof(struct pipe_vertex_buffer))) {
				if (input[i].buffer) {
					vb[i].stride = input[i].stride;
					vb[i].buffer_offset = input[i].buffer_offset;
					pipe_resource_reference(&vb[i].buffer, input[i].buffer);
					new_buffer_mask |= 1 << i;
					r600_context_add_resource_size(ctx, input[i].buffer);
				} else {
					pipe_resource_reference(&vb[i].buffer, NULL);
					disable_mask |= 1 << i;
				}
			}
		}
	} else {
		for (i = 0; i < count; i++) {
			pipe_resource_reference(&vb[i].buffer, NULL);
		}
		disable_mask = ((1ull << count) - 1);
	}

	disable_mask <<= start_slot;
	new_buffer_mask <<= start_slot;

	rctx->vertex_buffer_state.enabled_mask &= ~disable_mask;
	rctx->vertex_buffer_state.dirty_mask &= rctx->vertex_buffer_state.enabled_mask;
	rctx->vertex_buffer_state.enabled_mask |= new_buffer_mask;
	rctx->vertex_buffer_state.dirty_mask |= new_buffer_mask;

	r600_vertex_buffers_dirty(rctx);
}

void r600_sampler_views_dirty(struct r600_context *rctx,
			      struct r600_samplerview_state *state)
{
	if (state->dirty_mask) {
		rctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE;
		state->atom.num_dw = (rctx->b.chip_class >= EVERGREEN ? 14 : 13) *
				     util_bitcount(state->dirty_mask);
		state->atom.dirty = true;
	}
}

static void r600_set_sampler_views(struct pipe_context *pipe, unsigned shader,
				   unsigned start, unsigned count,
				   struct pipe_sampler_view **views)
{
	struct r600_context *rctx = (struct r600_context *) pipe;
	struct r600_textures_info *dst = &rctx->samplers[shader];
	struct r600_pipe_sampler_view **rviews = (struct r600_pipe_sampler_view **)views;
	uint32_t dirty_sampler_states_mask = 0;
	unsigned i;
	/* This sets 1-bit for textures with index >= count. */
	uint32_t disable_mask = ~((1ull << count) - 1);
	/* These are the new textures set by this function. */
	uint32_t new_mask = 0;

	/* Set textures with index >= count to NULL. */
	uint32_t remaining_mask;

	assert(start == 0); /* XXX fix below */

	if (shader == PIPE_SHADER_COMPUTE) {
		evergreen_set_cs_sampler_view(pipe, start, count, views);
		return;
	}

	remaining_mask = dst->views.enabled_mask & disable_mask;

	while (remaining_mask) {
		i = u_bit_scan(&remaining_mask);
		assert(dst->views.views[i]);

		pipe_sampler_view_reference((struct pipe_sampler_view **)&dst->views.views[i], NULL);
	}

	for (i = 0; i < count; i++) {
		if (rviews[i] == dst->views.views[i]) {
			continue;
		}

		if (rviews[i]) {
			struct r600_texture *rtex =
				(struct r600_texture*)rviews[i]->base.texture;

			if (rviews[i]->base.texture->target != PIPE_BUFFER) {
				if (rtex->is_depth && !rtex->is_flushing_texture) {
					dst->views.compressed_depthtex_mask |= 1 << i;
				} else {
					dst->views.compressed_depthtex_mask &= ~(1 << i);
				}

				/* Track compressed colorbuffers. */
				if (rtex->cmask.size) {
					dst->views.compressed_colortex_mask |= 1 << i;
				} else {
					dst->views.compressed_colortex_mask &= ~(1 << i);
				}
			}
			/* Changing from array to non-arrays textures and vice versa requires
			 * updating TEX_ARRAY_OVERRIDE in sampler states on R6xx-R7xx. */
			if (rctx->b.chip_class <= R700 &&
			    (dst->states.enabled_mask & (1 << i)) &&
			    (rviews[i]->base.texture->target == PIPE_TEXTURE_1D_ARRAY ||
			     rviews[i]->base.texture->target == PIPE_TEXTURE_2D_ARRAY) != dst->is_array_sampler[i]) {
				dirty_sampler_states_mask |= 1 << i;
			}

			pipe_sampler_view_reference((struct pipe_sampler_view **)&dst->views.views[i], views[i]);
			new_mask |= 1 << i;
			r600_context_add_resource_size(pipe, views[i]->texture);
		} else {
			pipe_sampler_view_reference((struct pipe_sampler_view **)&dst->views.views[i], NULL);
			disable_mask |= 1 << i;
		}
	}

	dst->views.enabled_mask &= ~disable_mask;
	dst->views.dirty_mask &= dst->views.enabled_mask;
	dst->views.enabled_mask |= new_mask;
	dst->views.dirty_mask |= new_mask;
	dst->views.compressed_depthtex_mask &= dst->views.enabled_mask;
	dst->views.compressed_colortex_mask &= dst->views.enabled_mask;
	dst->views.dirty_txq_constants = TRUE;
	dst->views.dirty_buffer_constants = TRUE;
	r600_sampler_views_dirty(rctx, &dst->views);

	if (dirty_sampler_states_mask) {
		dst->states.dirty_mask |= dirty_sampler_states_mask;
		r600_sampler_states_dirty(rctx, &dst->states);
	}
}

static void r600_set_viewport_states(struct pipe_context *ctx,
                                     unsigned start_slot,
                                     unsigned num_viewports,
                                     const struct pipe_viewport_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	int i;

	for (i = start_slot; i < start_slot + num_viewports; i++) {
		rctx->viewport[i].state = state[i - start_slot];
		rctx->viewport[i].atom.dirty = true;
	}
}

void r600_emit_viewport_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_viewport_state *rstate = (struct r600_viewport_state *)atom;
	struct pipe_viewport_state *state = &rstate->state;
	int offset = rstate->idx * 6 * 4;

	r600_write_context_reg_seq(cs, R_02843C_PA_CL_VPORT_XSCALE_0 + offset, 6);
	radeon_emit(cs, fui(state->scale[0]));     /* R_02843C_PA_CL_VPORT_XSCALE_0  */
	radeon_emit(cs, fui(state->translate[0])); /* R_028440_PA_CL_VPORT_XOFFSET_0 */
	radeon_emit(cs, fui(state->scale[1]));     /* R_028444_PA_CL_VPORT_YSCALE_0  */
	radeon_emit(cs, fui(state->translate[1])); /* R_028448_PA_CL_VPORT_YOFFSET_0 */
	radeon_emit(cs, fui(state->scale[2]));     /* R_02844C_PA_CL_VPORT_ZSCALE_0  */
	radeon_emit(cs, fui(state->translate[2])); /* R_028450_PA_CL_VPORT_ZOFFSET_0 */
}

/* Compute the key for the hw shader variant */
static INLINE struct r600_shader_key r600_shader_selector_key(struct pipe_context * ctx,
		struct r600_pipe_shader_selector * sel)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_shader_key key;
	memset(&key, 0, sizeof(key));

	if (sel->type == PIPE_SHADER_FRAGMENT) {
		key.color_two_side = rctx->rasterizer && rctx->rasterizer->two_side;
		key.alpha_to_one = rctx->alpha_to_one &&
				   rctx->rasterizer && rctx->rasterizer->multisample_enable &&
				   !rctx->framebuffer.cb0_is_integer;
		key.nr_cbufs = rctx->framebuffer.state.nr_cbufs;
		/* Dual-source blending only makes sense with nr_cbufs == 1. */
		if (key.nr_cbufs == 1 && rctx->dual_src_blend)
			key.nr_cbufs = 2;
	} else if (sel->type == PIPE_SHADER_VERTEX) {
		key.vs_as_es = (rctx->gs_shader != NULL);
	}
	return key;
}

/* Select the hw shader variant depending on the current state.
 * (*dirty) is set to 1 if current variant was changed */
static int r600_shader_select(struct pipe_context *ctx,
        struct r600_pipe_shader_selector* sel,
        bool *dirty)
{
	struct r600_shader_key key;
	struct r600_pipe_shader * shader = NULL;
	int r;

	memset(&key, 0, sizeof(key));
	key = r600_shader_selector_key(ctx, sel);

	/* Check if we don't need to change anything.
	 * This path is also used for most shaders that don't need multiple
	 * variants, it will cost just a computation of the key and this
	 * test. */
	if (likely(sel->current && memcmp(&sel->current->key, &key, sizeof(key)) == 0)) {
		return 0;
	}

	/* lookup if we have other variants in the list */
	if (sel->num_shaders > 1) {
		struct r600_pipe_shader *p = sel->current, *c = p->next_variant;

		while (c && memcmp(&c->key, &key, sizeof(key)) != 0) {
			p = c;
			c = c->next_variant;
		}

		if (c) {
			p->next_variant = c->next_variant;
			shader = c;
		}
	}

	if (unlikely(!shader)) {
		shader = CALLOC(1, sizeof(struct r600_pipe_shader));
		shader->selector = sel;

		r = r600_pipe_shader_create(ctx, shader, key);
		if (unlikely(r)) {
			R600_ERR("Failed to build shader variant (type=%u) %d\n",
				 sel->type, r);
			sel->current = NULL;
			FREE(shader);
			return r;
		}

		/* We don't know the value of nr_ps_max_color_exports until we built
		 * at least one variant, so we may need to recompute the key after
		 * building first variant. */
		if (sel->type == PIPE_SHADER_FRAGMENT &&
				sel->num_shaders == 0) {
			sel->nr_ps_max_color_exports = shader->shader.nr_ps_max_color_exports;
			key = r600_shader_selector_key(ctx, sel);
		}

		memcpy(&shader->key, &key, sizeof(key));
		sel->num_shaders++;
	}

	if (dirty)
		*dirty = true;

	shader->next_variant = sel->current;
	sel->current = shader;

	return 0;
}

static void *r600_create_shader_state(struct pipe_context *ctx,
			       const struct pipe_shader_state *state,
			       unsigned pipe_shader_type)
{
	struct r600_pipe_shader_selector *sel = CALLOC_STRUCT(r600_pipe_shader_selector);

	sel->type = pipe_shader_type;
	sel->tokens = tgsi_dup_tokens(state->tokens);
	sel->so = state->stream_output;
	return sel;
}

static void *r600_create_ps_state(struct pipe_context *ctx,
					 const struct pipe_shader_state *state)
{
	return r600_create_shader_state(ctx, state, PIPE_SHADER_FRAGMENT);
}

static void *r600_create_vs_state(struct pipe_context *ctx,
					 const struct pipe_shader_state *state)
{
	return r600_create_shader_state(ctx, state, PIPE_SHADER_VERTEX);
}

static void *r600_create_gs_state(struct pipe_context *ctx,
					 const struct pipe_shader_state *state)
{
	return r600_create_shader_state(ctx, state, PIPE_SHADER_GEOMETRY);
}

static void r600_bind_ps_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (!state)
		state = rctx->dummy_pixel_shader;

	rctx->ps_shader = (struct r600_pipe_shader_selector *)state;
}

static void r600_bind_vs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (!state)
		return;

	rctx->vs_shader = (struct r600_pipe_shader_selector *)state;
	rctx->b.streamout.stride_in_dw = rctx->vs_shader->so.stride;
}

static void r600_bind_gs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->gs_shader = (struct r600_pipe_shader_selector *)state;

	if (!state)
		return;
	rctx->b.streamout.stride_in_dw = rctx->gs_shader->so.stride;
}

static void r600_delete_shader_selector(struct pipe_context *ctx,
		struct r600_pipe_shader_selector *sel)
{
	struct r600_pipe_shader *p = sel->current, *c;
	while (p) {
		c = p->next_variant;
		r600_pipe_shader_destroy(ctx, p);
		free(p);
		p = c;
	}

	free(sel->tokens);
	free(sel);
}


static void r600_delete_ps_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader_selector *sel = (struct r600_pipe_shader_selector *)state;

	if (rctx->ps_shader == sel) {
		rctx->ps_shader = NULL;
	}

	r600_delete_shader_selector(ctx, sel);
}

static void r600_delete_vs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader_selector *sel = (struct r600_pipe_shader_selector *)state;

	if (rctx->vs_shader == sel) {
		rctx->vs_shader = NULL;
	}

	r600_delete_shader_selector(ctx, sel);
}


static void r600_delete_gs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader_selector *sel = (struct r600_pipe_shader_selector *)state;

	if (rctx->gs_shader == sel) {
		rctx->gs_shader = NULL;
	}

	r600_delete_shader_selector(ctx, sel);
}


void r600_constant_buffers_dirty(struct r600_context *rctx, struct r600_constbuf_state *state)
{
	if (state->dirty_mask) {
		rctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE;
		state->atom.num_dw = rctx->b.chip_class >= EVERGREEN ? util_bitcount(state->dirty_mask)*20
								   : util_bitcount(state->dirty_mask)*19;
		state->atom.dirty = true;
	}
}

static void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
				     struct pipe_constant_buffer *input)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_constbuf_state *state = &rctx->constbuf_state[shader];
	struct pipe_constant_buffer *cb;
	const uint8_t *ptr;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (unlikely(!input || (!input->buffer && !input->user_buffer))) {
		state->enabled_mask &= ~(1 << index);
		state->dirty_mask &= ~(1 << index);
		pipe_resource_reference(&state->cb[index].buffer, NULL);
		return;
	}

	cb = &state->cb[index];
	cb->buffer_size = input->buffer_size;

	ptr = input->user_buffer;

	if (ptr) {
		/* Upload the user buffer. */
		if (R600_BIG_ENDIAN) {
			uint32_t *tmpPtr;
			unsigned i, size = input->buffer_size;

			if (!(tmpPtr = malloc(size))) {
				R600_ERR("Failed to allocate BE swap buffer.\n");
				return;
			}

			for (i = 0; i < size / 4; ++i) {
				tmpPtr[i] = util_cpu_to_le32(((uint32_t *)ptr)[i]);
			}

			u_upload_data(rctx->b.uploader, 0, size, tmpPtr, &cb->buffer_offset, &cb->buffer);
			free(tmpPtr);
		} else {
			u_upload_data(rctx->b.uploader, 0, input->buffer_size, ptr, &cb->buffer_offset, &cb->buffer);
		}
		/* account it in gtt */
		rctx->b.gtt += input->buffer_size;
	} else {
		/* Setup the hw buffer. */
		cb->buffer_offset = input->buffer_offset;
		pipe_resource_reference(&cb->buffer, input->buffer);
		r600_context_add_resource_size(ctx, input->buffer);
	}

	state->enabled_mask |= 1 << index;
	state->dirty_mask |= 1 << index;
	r600_constant_buffers_dirty(rctx, state);
}

static void r600_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
	struct r600_context *rctx = (struct r600_context*)pipe;

	if (rctx->sample_mask.sample_mask == (uint16_t)sample_mask)
		return;

	rctx->sample_mask.sample_mask = sample_mask;
	rctx->sample_mask.atom.dirty = true;
}

/*
 * On r600/700 hw we don't have vertex fetch swizzle, though TBO
 * doesn't require full swizzles it does need masking and setting alpha
 * to one, so we setup a set of 5 constants with the masks + alpha value
 * then in the shader, we AND the 4 components with 0xffffffff or 0,
 * then OR the alpha with the value given here.
 * We use a 6th constant to store the txq buffer size in
 */
static void r600_setup_buffer_constants(struct r600_context *rctx, int shader_type)
{
	struct r600_textures_info *samplers = &rctx->samplers[shader_type];
	int bits;
	uint32_t array_size;
	struct pipe_constant_buffer cb;
	int i, j;

	if (!samplers->views.dirty_buffer_constants)
		return;

	samplers->views.dirty_buffer_constants = FALSE;

	bits = util_last_bit(samplers->views.enabled_mask);
	array_size = bits * 8 * sizeof(uint32_t) * 4;
	samplers->buffer_constants = realloc(samplers->buffer_constants, array_size);
	memset(samplers->buffer_constants, 0, array_size);
	for (i = 0; i < bits; i++) {
		if (samplers->views.enabled_mask & (1 << i)) {
			int offset = i * 8;
			const struct util_format_description *desc;
			desc = util_format_description(samplers->views.views[i]->base.format);

			for (j = 0; j < 4; j++)
				if (j < desc->nr_channels)
					samplers->buffer_constants[offset+j] = 0xffffffff;
				else
					samplers->buffer_constants[offset+j] = 0x0;
			if (desc->nr_channels < 4) {
				if (desc->channel[0].pure_integer)
					samplers->buffer_constants[offset+4] = 1;
				else
					samplers->buffer_constants[offset+4] = 0x3f800000;
			} else
				samplers->buffer_constants[offset + 4] = 0;

			samplers->buffer_constants[offset + 5] = samplers->views.views[i]->base.texture->width0 / util_format_get_blocksize(samplers->views.views[i]->base.format);
		}
	}

	cb.buffer = NULL;
	cb.user_buffer = samplers->buffer_constants;
	cb.buffer_offset = 0;
	cb.buffer_size = array_size;
	rctx->b.b.set_constant_buffer(&rctx->b.b, shader_type, R600_BUFFER_INFO_CONST_BUFFER, &cb);
	pipe_resource_reference(&cb.buffer, NULL);
}

/* On evergreen we only need to store the buffer size for TXQ */
static void eg_setup_buffer_constants(struct r600_context *rctx, int shader_type)
{
	struct r600_textures_info *samplers = &rctx->samplers[shader_type];
	int bits;
	uint32_t array_size;
	struct pipe_constant_buffer cb;
	int i;

	if (!samplers->views.dirty_buffer_constants)
		return;

	samplers->views.dirty_buffer_constants = FALSE;

	bits = util_last_bit(samplers->views.enabled_mask);
	array_size = bits * sizeof(uint32_t) * 4;
	samplers->buffer_constants = realloc(samplers->buffer_constants, array_size);
	memset(samplers->buffer_constants, 0, array_size);
	for (i = 0; i < bits; i++)
		if (samplers->views.enabled_mask & (1 << i))
		   samplers->buffer_constants[i] = samplers->views.views[i]->base.texture->width0 / util_format_get_blocksize(samplers->views.views[i]->base.format);

	cb.buffer = NULL;
	cb.user_buffer = samplers->buffer_constants;
	cb.buffer_offset = 0;
	cb.buffer_size = array_size;
	rctx->b.b.set_constant_buffer(&rctx->b.b, shader_type, R600_BUFFER_INFO_CONST_BUFFER, &cb);
	pipe_resource_reference(&cb.buffer, NULL);
}

static void r600_setup_txq_cube_array_constants(struct r600_context *rctx, int shader_type)
{
	struct r600_textures_info *samplers = &rctx->samplers[shader_type];
	int bits;
	uint32_t array_size;
	struct pipe_constant_buffer cb;
	int i;

	if (!samplers->views.dirty_txq_constants)
		return;

	samplers->views.dirty_txq_constants = FALSE;

	bits = util_last_bit(samplers->views.enabled_mask);
	array_size = bits * sizeof(uint32_t) * 4;
	samplers->txq_constants = realloc(samplers->txq_constants, array_size);
	memset(samplers->txq_constants, 0, array_size);
	for (i = 0; i < bits; i++)
		if (samplers->views.enabled_mask & (1 << i))
			samplers->txq_constants[i] = samplers->views.views[i]->base.texture->array_size / 6;

	cb.buffer = NULL;
	cb.user_buffer = samplers->txq_constants;
	cb.buffer_offset = 0;
	cb.buffer_size = array_size;
	rctx->b.b.set_constant_buffer(&rctx->b.b, shader_type, R600_TXQ_CONST_BUFFER, &cb);
	pipe_resource_reference(&cb.buffer, NULL);
}

static void update_shader_atom(struct pipe_context *ctx,
			       struct r600_shader_state *state,
			       struct r600_pipe_shader *shader)
{
	state->shader = shader;
	if (shader) {
		state->atom.num_dw = shader->command_buffer.num_dw;
		state->atom.dirty = true;
		r600_context_add_resource_size(ctx, (struct pipe_resource *)shader->bo);
	} else {
		state->atom.num_dw = 0;
		state->atom.dirty = false;
	}
}

static void update_gs_block_state(struct r600_context *rctx, unsigned enable)
{
	if (rctx->shader_stages.geom_enable != enable) {
		rctx->shader_stages.geom_enable = enable;
		rctx->shader_stages.atom.dirty = true;
	}

	if (rctx->gs_rings.enable != enable) {
		rctx->gs_rings.enable = enable;
		rctx->gs_rings.atom.dirty = true;

		if (enable && !rctx->gs_rings.esgs_ring.buffer) {
			unsigned size = 0x1C000;
			rctx->gs_rings.esgs_ring.buffer =
					pipe_buffer_create(rctx->b.b.screen, PIPE_BIND_CUSTOM,
							PIPE_USAGE_DEFAULT, size);
			rctx->gs_rings.esgs_ring.buffer_size = size;

			size = 0x4000000;

			rctx->gs_rings.gsvs_ring.buffer =
					pipe_buffer_create(rctx->b.b.screen, PIPE_BIND_CUSTOM,
							PIPE_USAGE_DEFAULT, size);
			rctx->gs_rings.gsvs_ring.buffer_size = size;
		}

		if (enable) {
			r600_set_constant_buffer(&rctx->b.b, PIPE_SHADER_GEOMETRY,
					R600_GS_RING_CONST_BUFFER, &rctx->gs_rings.esgs_ring);
			r600_set_constant_buffer(&rctx->b.b, PIPE_SHADER_VERTEX,
					R600_GS_RING_CONST_BUFFER, &rctx->gs_rings.gsvs_ring);
		} else {
			r600_set_constant_buffer(&rctx->b.b, PIPE_SHADER_GEOMETRY,
					R600_GS_RING_CONST_BUFFER, NULL);
			r600_set_constant_buffer(&rctx->b.b, PIPE_SHADER_VERTEX,
					R600_GS_RING_CONST_BUFFER, NULL);
		}
	}
}

static bool r600_update_derived_state(struct r600_context *rctx)
{
	struct pipe_context * ctx = (struct pipe_context*)rctx;
	bool ps_dirty = false, vs_dirty = false, gs_dirty = false;
	bool blend_disable;

	if (!rctx->blitter->running) {
		unsigned i;

		/* Decompress textures if needed. */
		for (i = 0; i < PIPE_SHADER_TYPES; i++) {
			struct r600_samplerview_state *views = &rctx->samplers[i].views;
			if (views->compressed_depthtex_mask) {
				r600_decompress_depth_textures(rctx, views);
			}
			if (views->compressed_colortex_mask) {
				r600_decompress_color_textures(rctx, views);
			}
		}
	}

	update_gs_block_state(rctx, rctx->gs_shader != NULL);

	if (rctx->gs_shader) {
		r600_shader_select(ctx, rctx->gs_shader, &gs_dirty);
		if (unlikely(!rctx->gs_shader->current))
			return false;

		if (!rctx->shader_stages.geom_enable) {
			rctx->shader_stages.geom_enable = true;
			rctx->shader_stages.atom.dirty = true;
		}

		/* gs_shader provides GS and VS (copy shader) */
		if (unlikely(rctx->geometry_shader.shader != rctx->gs_shader->current)) {
			update_shader_atom(ctx, &rctx->geometry_shader, rctx->gs_shader->current);
			update_shader_atom(ctx, &rctx->vertex_shader, rctx->gs_shader->current->gs_copy_shader);
			/* Update clip misc state. */
			if (rctx->gs_shader->current->gs_copy_shader->pa_cl_vs_out_cntl != rctx->clip_misc_state.pa_cl_vs_out_cntl ||
					rctx->gs_shader->current->gs_copy_shader->shader.clip_dist_write != rctx->clip_misc_state.clip_dist_write) {
				rctx->clip_misc_state.pa_cl_vs_out_cntl = rctx->gs_shader->current->gs_copy_shader->pa_cl_vs_out_cntl;
				rctx->clip_misc_state.clip_dist_write = rctx->gs_shader->current->gs_copy_shader->shader.clip_dist_write;
				rctx->clip_misc_state.atom.dirty = true;
			}
		}

		r600_shader_select(ctx, rctx->vs_shader, &vs_dirty);
		if (unlikely(!rctx->vs_shader->current))
			return false;

		/* vs_shader is used as ES */
		if (unlikely(vs_dirty || rctx->export_shader.shader != rctx->vs_shader->current)) {
			update_shader_atom(ctx, &rctx->export_shader, rctx->vs_shader->current);
		}
	} else {
		if (unlikely(rctx->geometry_shader.shader)) {
			update_shader_atom(ctx, &rctx->geometry_shader, NULL);
			update_shader_atom(ctx, &rctx->export_shader, NULL);
			rctx->shader_stages.geom_enable = false;
			rctx->shader_stages.atom.dirty = true;
		}

		r600_shader_select(ctx, rctx->vs_shader, &vs_dirty);
		if (unlikely(!rctx->vs_shader->current))
			return false;

		if (unlikely(vs_dirty || rctx->vertex_shader.shader != rctx->vs_shader->current)) {
			update_shader_atom(ctx, &rctx->vertex_shader, rctx->vs_shader->current);

			/* Update clip misc state. */
			if (rctx->vs_shader->current->pa_cl_vs_out_cntl != rctx->clip_misc_state.pa_cl_vs_out_cntl ||
					rctx->vs_shader->current->shader.clip_dist_write != rctx->clip_misc_state.clip_dist_write) {
				rctx->clip_misc_state.pa_cl_vs_out_cntl = rctx->vs_shader->current->pa_cl_vs_out_cntl;
				rctx->clip_misc_state.clip_dist_write = rctx->vs_shader->current->shader.clip_dist_write;
				rctx->clip_misc_state.atom.dirty = true;
			}
		}
	}

	r600_shader_select(ctx, rctx->ps_shader, &ps_dirty);
	if (unlikely(!rctx->ps_shader->current))
		return false;

	if (unlikely(ps_dirty || rctx->pixel_shader.shader != rctx->ps_shader->current)) {

		if (rctx->cb_misc_state.nr_ps_color_outputs != rctx->ps_shader->current->nr_ps_color_outputs) {
			rctx->cb_misc_state.nr_ps_color_outputs = rctx->ps_shader->current->nr_ps_color_outputs;
			rctx->cb_misc_state.atom.dirty = true;
		}

		if (rctx->b.chip_class <= R700) {
			bool multiwrite = rctx->ps_shader->current->shader.fs_write_all;

			if (rctx->cb_misc_state.multiwrite != multiwrite) {
				rctx->cb_misc_state.multiwrite = multiwrite;
				rctx->cb_misc_state.atom.dirty = true;
			}
		}

		if (rctx->b.chip_class >= EVERGREEN) {
			evergreen_update_db_shader_control(rctx);
		} else {
			r600_update_db_shader_control(rctx);
		}

		if (unlikely(!ps_dirty && rctx->ps_shader && rctx->rasterizer &&
				((rctx->rasterizer->sprite_coord_enable != rctx->ps_shader->current->sprite_coord_enable) ||
						(rctx->rasterizer->flatshade != rctx->ps_shader->current->flatshade)))) {

			if (rctx->b.chip_class >= EVERGREEN)
				evergreen_update_ps_state(ctx, rctx->ps_shader->current);
			else
				r600_update_ps_state(ctx, rctx->ps_shader->current);
		}

		update_shader_atom(ctx, &rctx->pixel_shader, rctx->ps_shader->current);
	}

	/* on R600 we stuff masks + txq info into one constant buffer */
	/* on evergreen we only need a txq info one */
	if (rctx->b.chip_class < EVERGREEN) {
		if (rctx->ps_shader && rctx->ps_shader->current->shader.uses_tex_buffers)
			r600_setup_buffer_constants(rctx, PIPE_SHADER_FRAGMENT);
		if (rctx->vs_shader && rctx->vs_shader->current->shader.uses_tex_buffers)
			r600_setup_buffer_constants(rctx, PIPE_SHADER_VERTEX);
		if (rctx->gs_shader && rctx->gs_shader->current->shader.uses_tex_buffers)
			r600_setup_buffer_constants(rctx, PIPE_SHADER_GEOMETRY);
	} else {
		if (rctx->ps_shader && rctx->ps_shader->current->shader.uses_tex_buffers)
			eg_setup_buffer_constants(rctx, PIPE_SHADER_FRAGMENT);
		if (rctx->vs_shader && rctx->vs_shader->current->shader.uses_tex_buffers)
			eg_setup_buffer_constants(rctx, PIPE_SHADER_VERTEX);
		if (rctx->gs_shader && rctx->gs_shader->current->shader.uses_tex_buffers)
			eg_setup_buffer_constants(rctx, PIPE_SHADER_GEOMETRY);
	}


	if (rctx->ps_shader && rctx->ps_shader->current->shader.has_txq_cube_array_z_comp)
		r600_setup_txq_cube_array_constants(rctx, PIPE_SHADER_FRAGMENT);
	if (rctx->vs_shader && rctx->vs_shader->current->shader.has_txq_cube_array_z_comp)
		r600_setup_txq_cube_array_constants(rctx, PIPE_SHADER_VERTEX);
	if (rctx->gs_shader && rctx->gs_shader->current->shader.has_txq_cube_array_z_comp)
		r600_setup_txq_cube_array_constants(rctx, PIPE_SHADER_GEOMETRY);

	if (rctx->b.chip_class < EVERGREEN && rctx->ps_shader && rctx->vs_shader) {
		if (!r600_adjust_gprs(rctx)) {
			/* discard rendering */
			return false;
		}
	}

	blend_disable = (rctx->dual_src_blend &&
			rctx->ps_shader->current->nr_ps_color_outputs < 2);

	if (blend_disable != rctx->force_blend_disable) {
		rctx->force_blend_disable = blend_disable;
		r600_bind_blend_state_internal(rctx,
					       rctx->blend_state.cso,
					       blend_disable);
	}

	return true;
}

void r600_emit_clip_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_clip_misc_state *state = &rctx->clip_misc_state;

	r600_write_context_reg(cs, R_028810_PA_CL_CLIP_CNTL,
			       state->pa_cl_clip_cntl |
			       (state->clip_dist_write ? 0 : state->clip_plane_enable & 0x3F));
	r600_write_context_reg(cs, R_02881C_PA_CL_VS_OUT_CNTL,
			       state->pa_cl_vs_out_cntl |
			       (state->clip_plane_enable & state->clip_dist_write));
}

static void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_draw_info info = *dinfo;
	struct pipe_index_buffer ib = {};
	unsigned i;
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;

	if (!info.count && (info.indexed || !info.count_from_stream_output)) {
		assert(0);
		return;
	}

	if (!rctx->vs_shader || !rctx->ps_shader) {
		assert(0);
		return;
	}

	/* make sure that the gfx ring is only one active */
	if (rctx->b.rings.dma.cs && rctx->b.rings.dma.cs->cdw) {
		rctx->b.rings.dma.flush(rctx, RADEON_FLUSH_ASYNC, NULL);
	}

	if (!r600_update_derived_state(rctx)) {
		/* useless to render because current rendering command
		 * can't be achieved
		 */
		return;
	}

	if (info.indexed) {
		/* Initialize the index buffer struct. */
		pipe_resource_reference(&ib.buffer, rctx->index_buffer.buffer);
		ib.user_buffer = rctx->index_buffer.user_buffer;
		ib.index_size = rctx->index_buffer.index_size;
		ib.offset = rctx->index_buffer.offset + info.start * ib.index_size;

		/* Translate 8-bit indices to 16-bit. */
		if (ib.index_size == 1) {
			struct pipe_resource *out_buffer = NULL;
			unsigned out_offset;
			void *ptr;

			u_upload_alloc(rctx->b.uploader, 0, info.count * 2,
				       &out_offset, &out_buffer, &ptr);

			util_shorten_ubyte_elts_to_userptr(
						&rctx->b.b, &ib, 0, ib.offset, info.count, ptr);

			pipe_resource_reference(&ib.buffer, NULL);
			ib.user_buffer = NULL;
			ib.buffer = out_buffer;
			ib.offset = out_offset;
			ib.index_size = 2;
		}

		/* Upload the index buffer.
		 * The upload is skipped for small index counts on little-endian machines
		 * and the indices are emitted via PKT3_DRAW_INDEX_IMMD.
		 * Note: Instanced rendering in combination with immediate indices hangs. */
		if (ib.user_buffer && (R600_BIG_ENDIAN || info.instance_count > 1 ||
				       info.count*ib.index_size > 20)) {
			u_upload_data(rctx->b.uploader, 0, info.count * ib.index_size,
				      ib.user_buffer, &ib.offset, &ib.buffer);
			ib.user_buffer = NULL;
		}
	} else {
		info.index_bias = info.start;
	}

	/* Set the index offset and primitive restart. */
	if (rctx->vgt_state.vgt_multi_prim_ib_reset_en != info.primitive_restart ||
	    rctx->vgt_state.vgt_multi_prim_ib_reset_indx != info.restart_index ||
	    rctx->vgt_state.vgt_indx_offset != info.index_bias) {
		rctx->vgt_state.vgt_multi_prim_ib_reset_en = info.primitive_restart;
		rctx->vgt_state.vgt_multi_prim_ib_reset_indx = info.restart_index;
		rctx->vgt_state.vgt_indx_offset = info.index_bias;
		rctx->vgt_state.atom.dirty = true;
	}

	/* Workaround for hardware deadlock on certain R600 ASICs: write into a CB register. */
	if (rctx->b.chip_class == R600) {
		rctx->b.flags |= R600_CONTEXT_PS_PARTIAL_FLUSH;
		rctx->cb_misc_state.atom.dirty = true;
	}

	/* Emit states. */
	r600_need_cs_space(rctx, ib.user_buffer ? 5 : 0, TRUE);
	r600_flush_emit(rctx);

	for (i = 0; i < R600_NUM_ATOMS; i++) {
		if (rctx->atoms[i] == NULL || !rctx->atoms[i]->dirty) {
			continue;
		}
		r600_emit_atom(rctx, rctx->atoms[i]);
	}

	/* On R6xx, CULL_FRONT=1 culls all points, lines, and rectangles,
	 * even though it should have no effect on those. */
	if (rctx->b.chip_class == R600 && rctx->rasterizer) {
		unsigned su_sc_mode_cntl = rctx->rasterizer->pa_su_sc_mode_cntl;
		unsigned prim = info.mode;

		if (rctx->gs_shader) {
			prim = rctx->gs_shader->current->shader.gs_output_prim;
		}
		prim = r600_conv_prim_to_gs_out(prim); /* decrease the number of types to 3 */

		if (prim == V_028A6C_OUTPRIM_TYPE_POINTLIST ||
		    prim == V_028A6C_OUTPRIM_TYPE_LINESTRIP ||
		    info.mode == R600_PRIM_RECTANGLE_LIST) {
			su_sc_mode_cntl &= C_028814_CULL_FRONT;
		}
		r600_write_context_reg(cs, R_028814_PA_SU_SC_MODE_CNTL, su_sc_mode_cntl);
	}

	/* Update start instance. */
	if (rctx->last_start_instance != info.start_instance) {
		r600_write_ctl_const(cs, R_03CFF4_SQ_VTX_START_INST_LOC, info.start_instance);
		rctx->last_start_instance = info.start_instance;
	}

	/* Update the primitive type. */
	if (rctx->last_primitive_type != info.mode) {
		unsigned ls_mask = 0;

		if (info.mode == PIPE_PRIM_LINES)
			ls_mask = 1;
		else if (info.mode == PIPE_PRIM_LINE_STRIP ||
			 info.mode == PIPE_PRIM_LINE_LOOP)
			ls_mask = 2;

		r600_write_context_reg(cs, R_028A0C_PA_SC_LINE_STIPPLE,
				       S_028A0C_AUTO_RESET_CNTL(ls_mask) |
				       (rctx->rasterizer ? rctx->rasterizer->pa_sc_line_stipple : 0));
		r600_write_config_reg(cs, R_008958_VGT_PRIMITIVE_TYPE,
				      r600_conv_pipe_prim(info.mode));

		rctx->last_primitive_type = info.mode;
	}

	/* Draw packets. */
	cs->buf[cs->cdw++] = PKT3(PKT3_NUM_INSTANCES, 0, rctx->b.predicate_drawing);
	cs->buf[cs->cdw++] = info.instance_count;
	if (info.indexed) {
		cs->buf[cs->cdw++] = PKT3(PKT3_INDEX_TYPE, 0, rctx->b.predicate_drawing);
		cs->buf[cs->cdw++] = ib.index_size == 4 ?
					(VGT_INDEX_32 | (R600_BIG_ENDIAN ? VGT_DMA_SWAP_32_BIT : 0)) :
					(VGT_INDEX_16 | (R600_BIG_ENDIAN ? VGT_DMA_SWAP_16_BIT : 0));

		if (ib.user_buffer) {
			unsigned size_bytes = info.count*ib.index_size;
			unsigned size_dw = align(size_bytes, 4) / 4;
			cs->buf[cs->cdw++] = PKT3(PKT3_DRAW_INDEX_IMMD, 1 + size_dw, rctx->b.predicate_drawing);
			cs->buf[cs->cdw++] = info.count;
			cs->buf[cs->cdw++] = V_0287F0_DI_SRC_SEL_IMMEDIATE;
			memcpy(cs->buf+cs->cdw, ib.user_buffer, size_bytes);
			cs->cdw += size_dw;
		} else {
			uint64_t va = r600_resource_va(ctx->screen, ib.buffer) + ib.offset;
			cs->buf[cs->cdw++] = PKT3(PKT3_DRAW_INDEX, 3, rctx->b.predicate_drawing);
			cs->buf[cs->cdw++] = va;
			cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
			cs->buf[cs->cdw++] = info.count;
			cs->buf[cs->cdw++] = V_0287F0_DI_SRC_SEL_DMA;
			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, rctx->b.predicate_drawing);
			cs->buf[cs->cdw++] = r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
								   (struct r600_resource*)ib.buffer,
								   RADEON_USAGE_READ, RADEON_PRIO_MIN);
		}
	} else {
		if (info.count_from_stream_output) {
			struct r600_so_target *t = (struct r600_so_target*)info.count_from_stream_output;
			uint64_t va = r600_resource_va(&rctx->screen->b.b, (void*)t->buf_filled_size) + t->buf_filled_size_offset;

			r600_write_context_reg(cs, R_028B30_VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE, t->stride_in_dw);

			cs->buf[cs->cdw++] = PKT3(PKT3_COPY_DW, 4, 0);
			cs->buf[cs->cdw++] = COPY_DW_SRC_IS_MEM | COPY_DW_DST_IS_REG;
			cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL;     /* src address lo */
			cs->buf[cs->cdw++] = (va >> 32UL) & 0xFFUL; /* src address hi */
			cs->buf[cs->cdw++] = R_028B2C_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE >> 2; /* dst register */
			cs->buf[cs->cdw++] = 0; /* unused */

			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] = r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
								   t->buf_filled_size, RADEON_USAGE_READ,
								   RADEON_PRIO_MIN);
		}

		cs->buf[cs->cdw++] = PKT3(PKT3_DRAW_INDEX_AUTO, 1, rctx->b.predicate_drawing);
		cs->buf[cs->cdw++] = info.count;
		cs->buf[cs->cdw++] = V_0287F0_DI_SRC_SEL_AUTO_INDEX |
					(info.count_from_stream_output ? S_0287F0_USE_OPAQUE(1) : 0);
	}

	if (rctx->screen->b.trace_bo) {
		r600_trace_emit(rctx);
	}

	/* Set the depth buffer as dirty. */
	if (rctx->framebuffer.state.zsbuf) {
		struct pipe_surface *surf = rctx->framebuffer.state.zsbuf;
		struct r600_texture *rtex = (struct r600_texture *)surf->texture;

		rtex->dirty_level_mask |= 1 << surf->u.tex.level;
	}
	if (rctx->framebuffer.compressed_cb_mask) {
		struct pipe_surface *surf;
		struct r600_texture *rtex;
		unsigned mask = rctx->framebuffer.compressed_cb_mask;

		do {
			unsigned i = u_bit_scan(&mask);
			surf = rctx->framebuffer.state.cbufs[i];
			rtex = (struct r600_texture*)surf->texture;

			rtex->dirty_level_mask |= 1 << surf->u.tex.level;

		} while (mask);
	}

	pipe_resource_reference(&ib.buffer, NULL);
	rctx->b.num_draw_calls++;
}

void r600_draw_rectangle(struct blitter_context *blitter,
			 int x1, int y1, int x2, int y2, float depth,
			 enum blitter_attrib_type type, const union pipe_color_union *attrib)
{
	struct r600_context *rctx = (struct r600_context*)util_blitter_get_pipe(blitter);
	struct pipe_viewport_state viewport;
	struct pipe_resource *buf = NULL;
	unsigned offset = 0;
	float *vb;

	if (type == UTIL_BLITTER_ATTRIB_TEXCOORD) {
		util_blitter_draw_rectangle(blitter, x1, y1, x2, y2, depth, type, attrib);
		return;
	}

	/* Some operations (like color resolve on r6xx) don't work
	 * with the conventional primitive types.
	 * One that works is PT_RECTLIST, which we use here. */

	/* setup viewport */
	viewport.scale[0] = 1.0f;
	viewport.scale[1] = 1.0f;
	viewport.scale[2] = 1.0f;
	viewport.scale[3] = 1.0f;
	viewport.translate[0] = 0.0f;
	viewport.translate[1] = 0.0f;
	viewport.translate[2] = 0.0f;
	viewport.translate[3] = 0.0f;
	rctx->b.b.set_viewport_states(&rctx->b.b, 0, 1, &viewport);

	/* Upload vertices. The hw rectangle has only 3 vertices,
	 * I guess the 4th one is derived from the first 3.
	 * The vertex specification should match u_blitter's vertex element state. */
	u_upload_alloc(rctx->b.uploader, 0, sizeof(float) * 24, &offset, &buf, (void**)&vb);
	vb[0] = x1;
	vb[1] = y1;
	vb[2] = depth;
	vb[3] = 1;

	vb[8] = x1;
	vb[9] = y2;
	vb[10] = depth;
	vb[11] = 1;

	vb[16] = x2;
	vb[17] = y1;
	vb[18] = depth;
	vb[19] = 1;

	if (attrib) {
		memcpy(vb+4, attrib->f, sizeof(float)*4);
		memcpy(vb+12, attrib->f, sizeof(float)*4);
		memcpy(vb+20, attrib->f, sizeof(float)*4);
	}

	/* draw */
	util_draw_vertex_buffer(&rctx->b.b, NULL, buf, rctx->blitter->vb_slot, offset,
				R600_PRIM_RECTANGLE_LIST, 3, 2);
	pipe_resource_reference(&buf, NULL);
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

static bool wrap_mode_uses_border_color(unsigned wrap, bool linear_filter)
{
	return wrap == PIPE_TEX_WRAP_CLAMP_TO_BORDER ||
	       wrap == PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER ||
	       (linear_filter &&
	        (wrap == PIPE_TEX_WRAP_CLAMP ||
		 wrap == PIPE_TEX_WRAP_MIRROR_CLAMP));
}

bool sampler_state_needs_border_color(const struct pipe_sampler_state *state)
{
	bool linear_filter = state->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
			     state->mag_img_filter != PIPE_TEX_FILTER_NEAREST;

	return (state->border_color.ui[0] || state->border_color.ui[1] ||
		state->border_color.ui[2] || state->border_color.ui[3]) &&
	       (wrap_mode_uses_border_color(state->wrap_s, linear_filter) ||
		wrap_mode_uses_border_color(state->wrap_t, linear_filter) ||
		wrap_mode_uses_border_color(state->wrap_r, linear_filter));
}

void r600_emit_shader(struct r600_context *rctx, struct r600_atom *a)
{

	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_pipe_shader *shader = ((struct r600_shader_state*)a)->shader;

	if (!shader)
		return;

	r600_emit_command_buffer(cs, &shader->command_buffer);
	radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
	radeon_emit(cs, r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, shader->bo,
					      RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA));
}

unsigned r600_get_swizzle_combined(const unsigned char *swizzle_format,
				   const unsigned char *swizzle_view,
				   boolean vtx)
{
	unsigned i;
	unsigned char swizzle[4];
	unsigned result = 0;
	const uint32_t tex_swizzle_shift[4] = {
		16, 19, 22, 25,
	};
	const uint32_t vtx_swizzle_shift[4] = {
		3, 6, 9, 12,
	};
	const uint32_t swizzle_bit[4] = {
		0, 1, 2, 3,
	};
	const uint32_t *swizzle_shift = tex_swizzle_shift;

	if (vtx)
		swizzle_shift = vtx_swizzle_shift;

	if (swizzle_view) {
		util_format_compose_swizzles(swizzle_format, swizzle_view, swizzle);
	} else {
		memcpy(swizzle, swizzle_format, 4);
	}

	/* Get swizzle. */
	for (i = 0; i < 4; i++) {
		switch (swizzle[i]) {
		case UTIL_FORMAT_SWIZZLE_Y:
			result |= swizzle_bit[1] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_Z:
			result |= swizzle_bit[2] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_W:
			result |= swizzle_bit[3] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_0:
			result |= V_038010_SQ_SEL_0 << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_1:
			result |= V_038010_SQ_SEL_1 << swizzle_shift[i];
			break;
		default: /* UTIL_FORMAT_SWIZZLE_X */
			result |= swizzle_bit[0] << swizzle_shift[i];
		}
	}
	return result;
}

/* texture format translate */
uint32_t r600_translate_texformat(struct pipe_screen *screen,
				  enum pipe_format format,
				  const unsigned char *swizzle_view,
				  uint32_t *word4_p, uint32_t *yuv_format_p)
{
	struct r600_screen *rscreen = (struct r600_screen *)screen;
	uint32_t result = 0, word4 = 0, yuv_format = 0;
	const struct util_format_description *desc;
	boolean uniform = TRUE;
	bool enable_s3tc = rscreen->b.info.drm_minor >= 9;
	bool is_srgb_valid = FALSE;
	const unsigned char swizzle_xxxx[4] = {0, 0, 0, 0};
	const unsigned char swizzle_yyyy[4] = {1, 1, 1, 1};

	int i;
	const uint32_t sign_bit[4] = {
		S_038010_FORMAT_COMP_X(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Y(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Z(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_W(V_038010_SQ_FORMAT_COMP_SIGNED)
	};
	desc = util_format_description(format);

	/* Depth and stencil swizzling is handled separately. */
	if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS) {
		word4 |= r600_get_swizzle_combined(desc->swizzle, swizzle_view, FALSE);
	}

	/* Colorspace (return non-RGB formats directly). */
	switch (desc->colorspace) {
	/* Depth stencil formats */
	case UTIL_FORMAT_COLORSPACE_ZS:
		switch (format) {
		/* Depth sampler formats. */
		case PIPE_FORMAT_Z16_UNORM:
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_16;
			goto out_word4;
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_8_24;
			goto out_word4;
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			if (rscreen->b.chip_class < EVERGREEN)
				goto out_unknown;
			word4 |= r600_get_swizzle_combined(swizzle_yyyy, swizzle_view, FALSE);
			result = FMT_24_8;
			goto out_word4;
		case PIPE_FORMAT_Z32_FLOAT:
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_32_FLOAT;
			goto out_word4;
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_X24_8_32_FLOAT;
			goto out_word4;
		/* Stencil sampler formats. */
		case PIPE_FORMAT_S8_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_8;
			goto out_word4;
		case PIPE_FORMAT_X24S8_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
			word4 |= r600_get_swizzle_combined(swizzle_yyyy, swizzle_view, FALSE);
			result = FMT_8_24;
			goto out_word4;
		case PIPE_FORMAT_S8X24_UINT:
			if (rscreen->b.chip_class < EVERGREEN)
				goto out_unknown;
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
			word4 |= r600_get_swizzle_combined(swizzle_xxxx, swizzle_view, FALSE);
			result = FMT_24_8;
			goto out_word4;
		case PIPE_FORMAT_X32_S8X24_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
			word4 |= r600_get_swizzle_combined(swizzle_yyyy, swizzle_view, FALSE);
			result = FMT_X24_8_32_FLOAT;
			goto out_word4;
		default:
			goto out_unknown;
		}

	case UTIL_FORMAT_COLORSPACE_YUV:
		yuv_format |= (1 << 30);
		switch (format) {
		case PIPE_FORMAT_UYVY:
		case PIPE_FORMAT_YUYV:
		default:
			break;
		}
		goto out_unknown; /* XXX */

	case UTIL_FORMAT_COLORSPACE_SRGB:
		word4 |= S_038010_FORCE_DEGAMMA(1);
		break;

	default:
		break;
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
		if (!enable_s3tc)
			goto out_unknown;

		switch (format) {
		case PIPE_FORMAT_RGTC1_SNORM:
		case PIPE_FORMAT_LATC1_SNORM:
			word4 |= sign_bit[0];
		case PIPE_FORMAT_RGTC1_UNORM:
		case PIPE_FORMAT_LATC1_UNORM:
			result = FMT_BC4;
			goto out_word4;
		case PIPE_FORMAT_RGTC2_SNORM:
		case PIPE_FORMAT_LATC2_SNORM:
			word4 |= sign_bit[0] | sign_bit[1];
		case PIPE_FORMAT_RGTC2_UNORM:
		case PIPE_FORMAT_LATC2_UNORM:
			result = FMT_BC5;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {

		if (!enable_s3tc)
			goto out_unknown;

		if (!util_format_s3tc_enabled) {
			goto out_unknown;
		}

		switch (format) {
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT1_SRGB:
		case PIPE_FORMAT_DXT1_SRGBA:
			result = FMT_BC1;
			is_srgb_valid = TRUE;
			goto out_word4;
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT3_SRGBA:
			result = FMT_BC2;
			is_srgb_valid = TRUE;
			goto out_word4;
		case PIPE_FORMAT_DXT5_RGBA:
		case PIPE_FORMAT_DXT5_SRGBA:
			result = FMT_BC3;
			is_srgb_valid = TRUE;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
		switch (format) {
		case PIPE_FORMAT_R8G8_B8G8_UNORM:
		case PIPE_FORMAT_G8R8_B8R8_UNORM:
			result = FMT_GB_GR;
			goto out_word4;
		case PIPE_FORMAT_G8R8_G8B8_UNORM:
		case PIPE_FORMAT_R8G8_R8B8_UNORM:
			result = FMT_BG_RG;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
		result = FMT_5_9_9_9_SHAREDEXP;
		goto out_word4;
	} else if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
		result = FMT_10_11_11_FLOAT;
		goto out_word4;
	}


	for (i = 0; i < desc->nr_channels; i++) {
		if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
			word4 |= sign_bit[i];
		}
	}

	/* R8G8Bx_SNORM - XXX CxV8U8 */

	/* See whether the components are of the same size. */
	for (i = 1; i < desc->nr_channels; i++) {
		uniform = uniform && desc->channel[0].size == desc->channel[i].size;
	}

	/* Non-uniform formats. */
	if (!uniform) {
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB &&
		    desc->channel[0].pure_integer)
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		switch(desc->nr_channels) {
		case 3:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 6 &&
			    desc->channel[2].size == 5) {
				result = FMT_5_6_5;
				goto out_word4;
			}
			goto out_unknown;
		case 4:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 5 &&
			    desc->channel[2].size == 5 &&
			    desc->channel[3].size == 1) {
				result = FMT_1_5_5_5;
				goto out_word4;
			}
			if (desc->channel[0].size == 10 &&
			    desc->channel[1].size == 10 &&
			    desc->channel[2].size == 10 &&
			    desc->channel[3].size == 2) {
				result = FMT_2_10_10_10;
				goto out_word4;
			}
			goto out_unknown;
		}
		goto out_unknown;
	}

	/* Find the first non-VOID channel. */
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	if (i == 4)
		goto out_unknown;

	/* uniform formats */
	switch (desc->channel[i].type) {
	case UTIL_FORMAT_TYPE_UNSIGNED:
	case UTIL_FORMAT_TYPE_SIGNED:
#if 0
		if (!desc->channel[i].normalized &&
		    desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
			goto out_unknown;
		}
#endif
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB &&
		    desc->channel[i].pure_integer)
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);

		switch (desc->channel[i].size) {
		case 4:
			switch (desc->nr_channels) {
			case 2:
				result = FMT_4_4;
				goto out_word4;
			case 4:
				result = FMT_4_4_4_4;
				goto out_word4;
			}
			goto out_unknown;
		case 8:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_8;
				goto out_word4;
			case 2:
				result = FMT_8_8;
				goto out_word4;
			case 4:
				result = FMT_8_8_8_8;
				is_srgb_valid = TRUE;
				goto out_word4;
			}
			goto out_unknown;
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_16;
				goto out_word4;
			case 2:
				result = FMT_16_16;
				goto out_word4;
			case 4:
				result = FMT_16_16_16_16;
				goto out_word4;
			}
			goto out_unknown;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_32;
				goto out_word4;
			case 2:
				result = FMT_32_32;
				goto out_word4;
			case 4:
				result = FMT_32_32_32_32;
				goto out_word4;
			}
		}
		goto out_unknown;

	case UTIL_FORMAT_TYPE_FLOAT:
		switch (desc->channel[i].size) {
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_16_FLOAT;
				goto out_word4;
			case 2:
				result = FMT_16_16_FLOAT;
				goto out_word4;
			case 4:
				result = FMT_16_16_16_16_FLOAT;
				goto out_word4;
			}
			goto out_unknown;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_32_FLOAT;
				goto out_word4;
			case 2:
				result = FMT_32_32_FLOAT;
				goto out_word4;
			case 4:
				result = FMT_32_32_32_32_FLOAT;
				goto out_word4;
			}
		}
		goto out_unknown;
	}

out_word4:

	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB && !is_srgb_valid)
		return ~0;
	if (word4_p)
		*word4_p = word4;
	if (yuv_format_p)
		*yuv_format_p = yuv_format;
	return result;
out_unknown:
	/* R600_ERR("Unable to handle texformat %d %s\n", format, util_format_name(format)); */
	return ~0;
}

uint32_t r600_translate_colorformat(enum chip_class chip, enum pipe_format format)
{
	const struct util_format_description *desc = util_format_description(format);
	int channel = util_format_get_first_non_void_channel(format);
	bool is_float;

#define HAS_SIZE(x,y,z,w) \
	(desc->channel[0].size == (x) && desc->channel[1].size == (y) && \
         desc->channel[2].size == (z) && desc->channel[3].size == (w))

	if (format == PIPE_FORMAT_R11G11B10_FLOAT) /* isn't plain */
		return V_0280A0_COLOR_10_11_11_FLOAT;

	if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
	    channel == -1)
		return ~0U;

	is_float = desc->channel[channel].type == UTIL_FORMAT_TYPE_FLOAT;

	switch (desc->nr_channels) {
	case 1:
		switch (desc->channel[0].size) {
		case 8:
			return V_0280A0_COLOR_8;
		case 16:
			if (is_float)
				return V_0280A0_COLOR_16_FLOAT;
			else
				return V_0280A0_COLOR_16;
		case 32:
			if (is_float)
				return V_0280A0_COLOR_32_FLOAT;
			else
				return V_0280A0_COLOR_32;
		}
		break;
	case 2:
		if (desc->channel[0].size == desc->channel[1].size) {
			switch (desc->channel[0].size) {
			case 4:
				if (chip <= R700)
					return V_0280A0_COLOR_4_4;
				else
					return ~0U; /* removed on Evergreen */
			case 8:
				return V_0280A0_COLOR_8_8;
			case 16:
				if (is_float)
					return V_0280A0_COLOR_16_16_FLOAT;
				else
					return V_0280A0_COLOR_16_16;
			case 32:
				if (is_float)
					return V_0280A0_COLOR_32_32_FLOAT;
				else
					return V_0280A0_COLOR_32_32;
			}
		} else if (HAS_SIZE(8,24,0,0)) {
			return V_0280A0_COLOR_24_8;
		} else if (HAS_SIZE(24,8,0,0)) {
			return V_0280A0_COLOR_8_24;
		}
		break;
	case 3:
		if (HAS_SIZE(5,6,5,0)) {
			return V_0280A0_COLOR_5_6_5;
		} else if (HAS_SIZE(32,8,24,0)) {
			return V_0280A0_COLOR_X24_8_32_FLOAT;
		}
		break;
	case 4:
		if (desc->channel[0].size == desc->channel[1].size &&
		    desc->channel[0].size == desc->channel[2].size &&
		    desc->channel[0].size == desc->channel[3].size) {
			switch (desc->channel[0].size) {
			case 4:
				return V_0280A0_COLOR_4_4_4_4;
			case 8:
				return V_0280A0_COLOR_8_8_8_8;
			case 16:
				if (is_float)
					return V_0280A0_COLOR_16_16_16_16_FLOAT;
				else
					return V_0280A0_COLOR_16_16_16_16;
			case 32:
				if (is_float)
					return V_0280A0_COLOR_32_32_32_32_FLOAT;
				else
					return V_0280A0_COLOR_32_32_32_32;
			}
		} else if (HAS_SIZE(5,5,5,1)) {
			return V_0280A0_COLOR_1_5_5_5;
		} else if (HAS_SIZE(10,10,10,2)) {
			return V_0280A0_COLOR_2_10_10_10;
		}
		break;
	}
	return ~0U;
}

uint32_t r600_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {
		/* 8-bit buffers. */
		case V_0280A0_COLOR_4_4:
		case V_0280A0_COLOR_8:
			return ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_0280A0_COLOR_5_6_5:
		case V_0280A0_COLOR_1_5_5_5:
		case V_0280A0_COLOR_4_4_4_4:
		case V_0280A0_COLOR_16:
		case V_0280A0_COLOR_8_8:
			return ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_0280A0_COLOR_8_8_8_8:
		case V_0280A0_COLOR_2_10_10_10:
		case V_0280A0_COLOR_8_24:
		case V_0280A0_COLOR_24_8:
		case V_0280A0_COLOR_32_FLOAT:
		case V_0280A0_COLOR_16_16_FLOAT:
		case V_0280A0_COLOR_16_16:
			return ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_0280A0_COLOR_16_16_16_16:
		case V_0280A0_COLOR_16_16_16_16_FLOAT:
			return ENDIAN_8IN16;

		case V_0280A0_COLOR_32_32_FLOAT:
		case V_0280A0_COLOR_32_32:
		case V_0280A0_COLOR_X24_8_32_FLOAT:
			return ENDIAN_8IN32;

		/* 128-bit buffers. */
		case V_0280A0_COLOR_32_32_32_32_FLOAT:
		case V_0280A0_COLOR_32_32_32_32:
			return ENDIAN_8IN32;
		default:
			return ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return ENDIAN_NONE;
	}
}

static void r600_invalidate_buffer(struct pipe_context *ctx, struct pipe_resource *buf)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_resource *rbuffer = r600_resource(buf);
	unsigned i, shader, mask, alignment = rbuffer->buf->alignment;

	/* Reallocate the buffer in the same pipe_resource. */
	r600_init_resource(&rctx->screen->b, rbuffer, rbuffer->b.b.width0,
			   alignment, TRUE);

	/* We changed the buffer, now we need to bind it where the old one was bound. */
	/* Vertex buffers. */
	mask = rctx->vertex_buffer_state.enabled_mask;
	while (mask) {
		i = u_bit_scan(&mask);
		if (rctx->vertex_buffer_state.vb[i].buffer == &rbuffer->b.b) {
			rctx->vertex_buffer_state.dirty_mask |= 1 << i;
			r600_vertex_buffers_dirty(rctx);
		}
	}
	/* Streamout buffers. */
	for (i = 0; i < rctx->b.streamout.num_targets; i++) {
		if (rctx->b.streamout.targets[i]->b.buffer == &rbuffer->b.b) {
			if (rctx->b.streamout.begin_emitted) {
				r600_emit_streamout_end(&rctx->b);
			}
			rctx->b.streamout.append_bitmask = rctx->b.streamout.enabled_mask;
			r600_streamout_buffers_dirty(&rctx->b);
		}
	}

	/* Constant buffers. */
	for (shader = 0; shader < PIPE_SHADER_TYPES; shader++) {
		struct r600_constbuf_state *state = &rctx->constbuf_state[shader];
		bool found = false;
		uint32_t mask = state->enabled_mask;

		while (mask) {
			unsigned i = u_bit_scan(&mask);
			if (state->cb[i].buffer == &rbuffer->b.b) {
				found = true;
				state->dirty_mask |= 1 << i;
			}
		}
		if (found) {
			r600_constant_buffers_dirty(rctx, state);
		}
	}

	/* XXX TODO: texture buffer objects */
}

static void r600_set_occlusion_query_state(struct pipe_context *ctx, bool enable)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (rctx->db_misc_state.occlusion_query_enabled != enable) {
		rctx->db_misc_state.occlusion_query_enabled = enable;
		rctx->db_misc_state.atom.dirty = true;
	}
}

static void r600_need_gfx_cs_space(struct pipe_context *ctx, unsigned num_dw,
                                   bool include_draw_vbo)
{
	r600_need_cs_space((struct r600_context*)ctx, num_dw, include_draw_vbo);
}

/* keep this at the end of this file, please */
void r600_init_common_state_functions(struct r600_context *rctx)
{
	rctx->b.b.create_fs_state = r600_create_ps_state;
	rctx->b.b.create_vs_state = r600_create_vs_state;
	rctx->b.b.create_gs_state = r600_create_gs_state;
	rctx->b.b.create_vertex_elements_state = r600_create_vertex_fetch_shader;
	rctx->b.b.bind_blend_state = r600_bind_blend_state;
	rctx->b.b.bind_depth_stencil_alpha_state = r600_bind_dsa_state;
	rctx->b.b.bind_sampler_states = r600_bind_sampler_states;
	rctx->b.b.bind_fs_state = r600_bind_ps_state;
	rctx->b.b.bind_rasterizer_state = r600_bind_rs_state;
	rctx->b.b.bind_vertex_elements_state = r600_bind_vertex_elements;
	rctx->b.b.bind_vs_state = r600_bind_vs_state;
	rctx->b.b.bind_gs_state = r600_bind_gs_state;
	rctx->b.b.delete_blend_state = r600_delete_blend_state;
	rctx->b.b.delete_depth_stencil_alpha_state = r600_delete_dsa_state;
	rctx->b.b.delete_fs_state = r600_delete_ps_state;
	rctx->b.b.delete_rasterizer_state = r600_delete_rs_state;
	rctx->b.b.delete_sampler_state = r600_delete_sampler_state;
	rctx->b.b.delete_vertex_elements_state = r600_delete_vertex_elements;
	rctx->b.b.delete_vs_state = r600_delete_vs_state;
	rctx->b.b.delete_gs_state = r600_delete_gs_state;
	rctx->b.b.set_blend_color = r600_set_blend_color;
	rctx->b.b.set_clip_state = r600_set_clip_state;
	rctx->b.b.set_constant_buffer = r600_set_constant_buffer;
	rctx->b.b.set_sample_mask = r600_set_sample_mask;
	rctx->b.b.set_stencil_ref = r600_set_pipe_stencil_ref;
	rctx->b.b.set_viewport_states = r600_set_viewport_states;
	rctx->b.b.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->b.b.set_index_buffer = r600_set_index_buffer;
	rctx->b.b.set_sampler_views = r600_set_sampler_views;
	rctx->b.b.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->b.b.texture_barrier = r600_texture_barrier;
	rctx->b.b.set_stream_output_targets = r600_set_streamout_targets;
	rctx->b.b.draw_vbo = r600_draw_vbo;
	rctx->b.invalidate_buffer = r600_invalidate_buffer;
	rctx->b.set_occlusion_query_state = r600_set_occlusion_query_state;
	rctx->b.need_gfx_cs_space = r600_need_gfx_cs_space;
}

void r600_trace_emit(struct r600_context *rctx)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	uint64_t va;
	uint32_t reloc;

	va = r600_resource_va(&rscreen->b.b, (void*)rscreen->b.trace_bo);
	reloc = r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, rscreen->b.trace_bo,
				      RADEON_USAGE_READWRITE, RADEON_PRIO_MIN);
	radeon_emit(cs, PKT3(PKT3_MEM_WRITE, 3, 0));
	radeon_emit(cs, va & 0xFFFFFFFFUL);
	radeon_emit(cs, (va >> 32UL) & 0xFFUL);
	radeon_emit(cs, cs->cdw);
	radeon_emit(cs, rscreen->b.cs_count);
	radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
	radeon_emit(cs, reloc);
}
