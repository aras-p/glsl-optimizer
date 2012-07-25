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
#include "util/u_framebuffer.h"
#include "util/u_blitter.h"
#include "tgsi/tgsi_parse.h"
#include "radeonsi_pipe.h"
#include "si_state.h"
#include "sid.h"

/*
 * Shaders
 */

static void si_pipe_shader_vs(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4;
	unsigned num_sgprs, num_user_sgprs;
	unsigned nparams, i;
	uint64_t va;

	if (si_pipe_shader_create(ctx, shader))
		return;

	si_pm4_delete_state(rctx, vs, shader->pm4);
	pm4 = shader->pm4 = CALLOC_STRUCT(si_pm4_state);

	si_pm4_inval_shader_cache(pm4);

	/* Certain attributes (position, psize, etc.) don't count as params.
	 * VS is required to export at least one param and r600_shader_from_tgsi()
	 * takes care of adding a dummy export.
	 */
	for (nparams = 0, i = 0 ; i < shader->shader.noutput; i++) {
		if (shader->shader.output[i].name != TGSI_SEMANTIC_POSITION)
			nparams++;
	}
	if (nparams < 1)
		nparams = 1;

	si_pm4_set_reg(pm4, R_0286C4_SPI_VS_OUT_CONFIG,
		       S_0286C4_VS_EXPORT_COUNT(nparams - 1));

	si_pm4_set_reg(pm4, R_02870C_SPI_SHADER_POS_FORMAT,
		       S_02870C_POS0_EXPORT_FORMAT(V_02870C_SPI_SHADER_4COMP) |
		       S_02870C_POS1_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE) |
		       S_02870C_POS2_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE) |
		       S_02870C_POS3_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE));

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ);
	si_pm4_set_reg(pm4, R_00B120_SPI_SHADER_PGM_LO_VS, va >> 8);
	si_pm4_set_reg(pm4, R_00B124_SPI_SHADER_PGM_HI_VS, va >> 40);

	num_user_sgprs = 8;
	num_sgprs = shader->num_sgprs;
	if (num_user_sgprs > num_sgprs)
		num_sgprs = num_user_sgprs;
	/* Last 2 reserved SGPRs are used for VCC */
	num_sgprs += 2;
	assert(num_sgprs <= 104);

	si_pm4_set_reg(pm4, R_00B128_SPI_SHADER_PGM_RSRC1_VS,
		       S_00B128_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B128_SGPRS((num_sgprs - 1) / 8));
	si_pm4_set_reg(pm4, R_00B12C_SPI_SHADER_PGM_RSRC2_VS,
		       S_00B12C_USER_SGPR(num_user_sgprs));

	si_pm4_bind_state(rctx, vs, shader->pm4);
}

static void si_pipe_shader_ps(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4;
	unsigned i, exports_ps, num_cout, spi_ps_in_control, db_shader_control;
	unsigned num_sgprs, num_user_sgprs;
	int ninterp = 0;
	boolean have_linear = FALSE, have_centroid = FALSE, have_perspective = FALSE;
	unsigned spi_baryc_cntl;
	uint64_t va;

	if (si_pipe_shader_create(ctx, shader))
		return;

	si_pm4_delete_state(rctx, ps, shader->pm4);
	pm4 = shader->pm4 = CALLOC_STRUCT(si_pm4_state);

	si_pm4_inval_shader_cache(pm4);

	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	for (i = 0; i < shader->shader.ninput; i++) {
		ninterp++;
		/* XXX: Flat shading hangs the GPU */
		if (shader->shader.input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
		    (shader->shader.input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
		     rctx->queued.named.rasterizer->flatshade))
			have_linear = TRUE;
		if (shader->shader.input[i].interpolate == TGSI_INTERPOLATE_LINEAR)
			have_linear = TRUE;
		if (shader->shader.input[i].interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
			have_perspective = TRUE;
		if (shader->shader.input[i].centroid)
			have_centroid = TRUE;
	}

	for (i = 0; i < shader->shader.noutput; i++) {
		if (shader->shader.output[i].name == TGSI_SEMANTIC_POSITION)
			db_shader_control |= S_02880C_Z_EXPORT_ENABLE(1);
		if (shader->shader.output[i].name == TGSI_SEMANTIC_STENCIL)
			db_shader_control |= 0; // XXX OP_VAL or TEST_VAL?
	}
	if (shader->shader.uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	exports_ps = 0;
	num_cout = 0;
	for (i = 0; i < shader->shader.noutput; i++) {
		if (shader->shader.output[i].name == TGSI_SEMANTIC_POSITION ||
		    shader->shader.output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
		else if (shader->shader.output[i].name == TGSI_SEMANTIC_COLOR) {
			if (shader->shader.fs_write_all)
				num_cout = shader->shader.nr_cbufs;
			else
				num_cout++;
		}
	}
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	spi_ps_in_control = S_0286D8_NUM_INTERP(ninterp);

	spi_baryc_cntl = 0;
	if (have_perspective)
		spi_baryc_cntl |= have_centroid ?
			S_0286E0_PERSP_CENTROID_CNTL(1) : S_0286E0_PERSP_CENTER_CNTL(1);
	if (have_linear)
		spi_baryc_cntl |= have_centroid ?
			S_0286E0_LINEAR_CENTROID_CNTL(1) : S_0286E0_LINEAR_CENTER_CNTL(1);

	si_pm4_set_reg(pm4, R_0286E0_SPI_BARYC_CNTL, spi_baryc_cntl);
	si_pm4_set_reg(pm4, R_0286CC_SPI_PS_INPUT_ENA, shader->spi_ps_input_ena);
	si_pm4_set_reg(pm4, R_0286D0_SPI_PS_INPUT_ADDR, shader->spi_ps_input_ena);
	si_pm4_set_reg(pm4, R_0286D8_SPI_PS_IN_CONTROL, spi_ps_in_control);

	/* XXX: Depends on Z buffer format? */
	si_pm4_set_reg(pm4, R_028710_SPI_SHADER_Z_FORMAT, 0);

	/* XXX: Depends on color buffer format? */
	si_pm4_set_reg(pm4, R_028714_SPI_SHADER_COL_FORMAT,
		       S_028714_COL0_EXPORT_FORMAT(V_028714_SPI_SHADER_32_ABGR));

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ);
	si_pm4_set_reg(pm4, R_00B020_SPI_SHADER_PGM_LO_PS, va >> 8);
	si_pm4_set_reg(pm4, R_00B024_SPI_SHADER_PGM_HI_PS, va >> 40);

	num_user_sgprs = 6;
	num_sgprs = shader->num_sgprs;
	if (num_user_sgprs > num_sgprs)
		num_sgprs = num_user_sgprs;
	/* Last 2 reserved SGPRs are used for VCC */
	num_sgprs += 2;
	assert(num_sgprs <= 104);

	si_pm4_set_reg(pm4, R_00B028_SPI_SHADER_PGM_RSRC1_PS,
		       S_00B028_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B028_SGPRS((num_sgprs - 1) / 8));
	si_pm4_set_reg(pm4, R_00B02C_SPI_SHADER_PGM_RSRC2_PS,
		       S_00B02C_USER_SGPR(num_user_sgprs));

	si_pm4_set_reg(pm4, R_02880C_DB_SHADER_CONTROL, db_shader_control);

	shader->sprite_coord_enable = rctx->sprite_coord_enable;
	si_pm4_bind_state(rctx, ps, shader->pm4);
}

/*
 * Drawing
 */

static unsigned si_conv_pipe_prim(unsigned pprim)
{
        static const unsigned prim_conv[] = {
		[PIPE_PRIM_POINTS]			= V_008958_DI_PT_POINTLIST,
		[PIPE_PRIM_LINES]			= V_008958_DI_PT_LINELIST,
		[PIPE_PRIM_LINE_LOOP]			= V_008958_DI_PT_LINELOOP,
		[PIPE_PRIM_LINE_STRIP]			= V_008958_DI_PT_LINESTRIP,
		[PIPE_PRIM_TRIANGLES]			= V_008958_DI_PT_TRILIST,
		[PIPE_PRIM_TRIANGLE_STRIP]		= V_008958_DI_PT_TRISTRIP,
		[PIPE_PRIM_TRIANGLE_FAN]		= V_008958_DI_PT_TRIFAN,
		[PIPE_PRIM_QUADS]			= V_008958_DI_PT_QUADLIST,
		[PIPE_PRIM_QUAD_STRIP]			= V_008958_DI_PT_QUADSTRIP,
		[PIPE_PRIM_POLYGON]			= V_008958_DI_PT_POLYGON,
		[PIPE_PRIM_LINES_ADJACENCY]		= ~0,
		[PIPE_PRIM_LINE_STRIP_ADJACENCY]	= ~0,
		[PIPE_PRIM_TRIANGLES_ADJACENCY]		= ~0,
		[PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]	= ~0
        };
	unsigned result = prim_conv[pprim];
        if (result == ~0) {
		R600_ERR("unsupported primitive type %d\n", pprim);
        }
	return result;
}

static bool si_update_draw_info_state(struct r600_context *rctx,
			       const struct pipe_draw_info *info)
{
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	unsigned prim = si_conv_pipe_prim(info->mode);
	unsigned ls_mask = 0;

	if (pm4 == NULL)
		return false;

	if (prim == ~0) {
		FREE(pm4);
		return false;
	}

	si_pm4_set_reg(pm4, R_008958_VGT_PRIMITIVE_TYPE, prim);
	si_pm4_set_reg(pm4, R_028400_VGT_MAX_VTX_INDX, ~0);
	si_pm4_set_reg(pm4, R_028404_VGT_MIN_VTX_INDX, 0);
	si_pm4_set_reg(pm4, R_028408_VGT_INDX_OFFSET, info->index_bias);
	si_pm4_set_reg(pm4, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, info->restart_index);
	si_pm4_set_reg(pm4, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, info->primitive_restart);
#if 0
	si_pm4_set_reg(pm4, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);
	si_pm4_set_reg(pm4, R_03CFF4_SQ_VTX_START_INST_LOC, info->start_instance);
#endif

        if (prim == V_008958_DI_PT_LINELIST)
                ls_mask = 1;
        else if (prim == V_008958_DI_PT_LINESTRIP)
                ls_mask = 2;
	si_pm4_set_reg(pm4, R_028A0C_PA_SC_LINE_STIPPLE,
		       S_028A0C_AUTO_RESET_CNTL(ls_mask) |
		       rctx->pa_sc_line_stipple);

        if (info->mode == PIPE_PRIM_QUADS || info->mode == PIPE_PRIM_QUAD_STRIP || info->mode == PIPE_PRIM_POLYGON) {
		si_pm4_set_reg(pm4, R_028814_PA_SU_SC_MODE_CNTL,
			       S_028814_PROVOKING_VTX_LAST(1) | rctx->pa_su_sc_mode_cntl);
        } else {
		si_pm4_set_reg(pm4, R_028814_PA_SU_SC_MODE_CNTL, rctx->pa_su_sc_mode_cntl);
        }
	si_pm4_set_reg(pm4, R_02881C_PA_CL_VS_OUT_CNTL,
		       prim == PIPE_PRIM_POINTS ? rctx->pa_cl_vs_out_cntl : 0
		       /*| (rctx->rasterizer->clip_plane_enable &
		       rctx->vs_shader->shader.clip_dist_write)*/);
	si_pm4_set_reg(pm4, R_028810_PA_CL_CLIP_CNTL, rctx->pa_cl_clip_cntl
			/*| (rctx->vs_shader->shader.clip_dist_write ||
			rctx->vs_shader->shader.vs_prohibit_ucps ?
			0 : rctx->rasterizer->clip_plane_enable & 0x3F)*/);

	si_pm4_set_state(rctx, draw_info, pm4);
	return true;
}

static void si_update_alpha_ref(struct r600_context *rctx)
{
#if 0
        unsigned alpha_ref;
        struct r600_pipe_state rstate;

        alpha_ref = rctx->alpha_ref;
        rstate.nregs = 0;
        if (rctx->export_16bpc)
                alpha_ref &= ~0x1FFF;
        si_pm4_set_reg(&rstate, R_028438_SX_ALPHA_REF, alpha_ref);

	si_pm4_set_state(rctx, TODO, pm4);
        rctx->alpha_ref_dirty = false;
#endif
}

static void si_update_spi_map(struct r600_context *rctx)
{
	struct si_shader *ps = &rctx->ps_shader->shader;
	struct si_shader *vs = &rctx->vs_shader->shader;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	unsigned i, j, tmp;

	for (i = 0; i < ps->ninput; i++) {
		tmp = 0;

#if 0
		/* XXX: Flat shading hangs the GPU */
		if (ps->input[i].name == TGSI_SEMANTIC_POSITION ||
		    ps->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
		    (ps->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
		     rctx->rasterizer && rctx->rasterizer->flatshade)) {
			tmp |= S_028644_FLAT_SHADE(1);
		}
#endif

		if (ps->input[i].name == TGSI_SEMANTIC_GENERIC &&
		    rctx->sprite_coord_enable & (1 << ps->input[i].sid)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}

		for (j = 0; j < vs->noutput; j++) {
			if (ps->input[i].name == vs->output[j].name &&
			    ps->input[i].sid == vs->output[j].sid) {
				tmp |= S_028644_OFFSET(vs->output[j].param_offset);
				break;
			}
		}

		if (j == vs->noutput) {
			/* No corresponding output found, load defaults into input */
			tmp |= S_028644_OFFSET(0x20);
		}

		si_pm4_set_reg(pm4, R_028644_SPI_PS_INPUT_CNTL_0 + i * 4, tmp);
	}

	si_pm4_set_state(rctx, spi, pm4);
}

static void si_update_derived_state(struct r600_context *rctx)
{
	struct pipe_context * ctx = (struct pipe_context*)rctx;

	if (!rctx->blitter->running) {
		if (rctx->have_depth_fb || rctx->have_depth_texture)
			r600_flush_depth_textures(rctx);
	}

	if ((rctx->ps_shader->shader.fs_write_all &&
	     (rctx->ps_shader->shader.nr_cbufs != rctx->framebuffer.nr_cbufs)) ||
	    (rctx->sprite_coord_enable &&
	     (rctx->ps_shader->sprite_coord_enable != rctx->sprite_coord_enable))) {
		si_pipe_shader_destroy(&rctx->context, rctx->ps_shader);
	}

	if (rctx->alpha_ref_dirty) {
		si_update_alpha_ref(rctx);
	}

	if (!rctx->vs_shader->bo) {
		si_pipe_shader_vs(ctx, rctx->vs_shader);
	}

	if (!rctx->ps_shader->bo) {
		si_pipe_shader_ps(ctx, rctx->ps_shader);
	}

	if (rctx->shader_dirty) {
		si_update_spi_map(rctx);
		rctx->shader_dirty = false;
	}
}

static void si_vertex_buffer_update(struct r600_context *rctx)
{
	struct pipe_context *ctx = &rctx->context;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	bool bound[PIPE_MAX_ATTRIBS] = {};
	struct si_resource *t_list_buffer;
	unsigned i, count;
	uint32_t *ptr;
	uint64_t va;

	si_pm4_inval_vertex_cache(pm4);

	/* bind vertex buffer once */
	count = rctx->vertex_elements->count;
	assert(count <= 256 / 4);

	t_list_buffer = si_resource_create_custom(ctx->screen, PIPE_USAGE_IMMUTABLE,
						  4 * 4 * count);
	if (t_list_buffer == NULL) {
		FREE(pm4);
		return;
	}
	si_pm4_add_bo(pm4, t_list_buffer, RADEON_USAGE_READ);

	ptr = (uint32_t*)rctx->ws->buffer_map(t_list_buffer->cs_buf,
					      rctx->cs,
					      PIPE_TRANSFER_WRITE);

	for (i = 0 ; i < count; i++, ptr += 4) {
		struct pipe_vertex_element *ve = &rctx->vertex_elements->elements[i];
		struct pipe_vertex_buffer *vb;
		struct si_resource *rbuffer;
		unsigned offset;

		if (ve->vertex_buffer_index >= rctx->nr_vertex_buffers)
			continue;

		vb = &rctx->vertex_buffer[ve->vertex_buffer_index];
		rbuffer = (struct si_resource*)vb->buffer;
		if (rbuffer == NULL)
			continue;

		offset = 0;
		offset += vb->buffer_offset;
		offset += ve->src_offset;

		va = r600_resource_va(ctx->screen, (void*)rbuffer);
		va += offset;

		/* Fill in T# buffer resource description */
		ptr[0] = va & 0xFFFFFFFF;
		ptr[1] = (S_008F04_BASE_ADDRESS_HI(va >> 32) |
			  S_008F04_STRIDE(vb->stride));
		if (vb->stride > 0)
			ptr[2] = (vb->buffer->width0 - offset) / vb->stride;
		else
			ptr[2] = vb->buffer->width0 - offset;
		ptr[3] = rctx->vertex_elements->rsrc_word3[i];

		if (!bound[ve->vertex_buffer_index]) {
			si_pm4_add_bo(pm4, rbuffer, RADEON_USAGE_READ);
			bound[ve->vertex_buffer_index] = true;
		}
	}

	va = r600_resource_va(ctx->screen, (void*)t_list_buffer);
	si_pm4_set_reg(pm4, R_00B148_SPI_SHADER_USER_DATA_VS_6, va);
	si_pm4_set_reg(pm4, R_00B14C_SPI_SHADER_USER_DATA_VS_7, va >> 32);
	si_pm4_set_state(rctx, vertex_buffers, pm4);
}

void si_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_state_dsa *dsa = rctx->queued.named.dsa;
	struct pipe_draw_info info = *dinfo;
	struct r600_draw rdraw = {};
	struct pipe_index_buffer ib = {};
	struct r600_atom *state = NULL, *next_state = NULL;

	if ((!info.count && (info.indexed || !info.count_from_stream_output)) ||
	    (info.indexed && !rctx->index_buffer.buffer)) {
		return;
	}

	if (!rctx->ps_shader || !rctx->vs_shader)
		return;

	si_update_derived_state(rctx);
	si_vertex_buffer_update(rctx);

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
		rdraw.indices = (struct si_resource*)ib.buffer;
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

	if (!si_update_draw_info_state(rctx, &info))
		return;

	rdraw.db_render_override = dsa->db_render_override;
	rdraw.db_render_control = dsa->db_render_control;

	/* Emit states. */
	rctx->pm4_dirty_cdwords += si_pm4_dirty_dw(rctx);

	r600_need_cs_space(rctx, 0, TRUE);

	LIST_FOR_EACH_ENTRY_SAFE(state, next_state, &rctx->dirty_states, head) {
		r600_emit_atom(rctx, state);
	}
	si_pm4_emit_dirty(rctx);
	rctx->pm4_dirty_cdwords = 0;

	/* Enable stream out if needed. */
	if (rctx->streamout_start) {
		r600_context_streamout_begin(rctx);
		rctx->streamout_start = FALSE;
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
