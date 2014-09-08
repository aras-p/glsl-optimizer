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
 */
#include "r600_pipe.h"
#include "compute_memory_pool.h"
#include "evergreen_compute.h"
#include "util/u_surface.h"
#include "util/u_format.h"
#include "evergreend.h"

enum r600_blitter_op /* bitmask */
{
	R600_SAVE_FRAGMENT_STATE = 1,
	R600_SAVE_TEXTURES       = 2,
	R600_SAVE_FRAMEBUFFER    = 4,
	R600_DISABLE_RENDER_COND = 8,

	R600_CLEAR         = R600_SAVE_FRAGMENT_STATE,

	R600_CLEAR_SURFACE = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER,

	R600_COPY_BUFFER   = R600_DISABLE_RENDER_COND,

	R600_COPY_TEXTURE  = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_BLIT          = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES,

	R600_DECOMPRESS    = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,

	R600_COLOR_RESOLVE = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_suspend_nontimer_queries(&rctx->b);

	util_blitter_save_vertex_buffer_slot(rctx->blitter, rctx->vertex_buffer_state.vb);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_fetch_shader.cso);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_geometry_shader(rctx->blitter, rctx->gs_shader);
	util_blitter_save_so_targets(rctx->blitter, rctx->b.streamout.num_targets,
				     (struct pipe_stream_output_target**)rctx->b.streamout.targets);
	util_blitter_save_rasterizer(rctx->blitter, rctx->rasterizer_state.cso);

	if (op & R600_SAVE_FRAGMENT_STATE) {
		util_blitter_save_viewport(rctx->blitter, &rctx->viewport[0].state);
		util_blitter_save_scissor(rctx->blitter, &rctx->scissor[0].scissor);
		util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
		util_blitter_save_blend(rctx->blitter, rctx->blend_state.cso);
		util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->dsa_state.cso);
		util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref.pipe_state);
                util_blitter_save_sample_mask(rctx->blitter, rctx->sample_mask.sample_mask);
	}

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer.state);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, util_last_bit(rctx->samplers[PIPE_SHADER_FRAGMENT].states.enabled_mask),
			(void**)rctx->samplers[PIPE_SHADER_FRAGMENT].states.states);

		util_blitter_save_fragment_sampler_views(
			rctx->blitter, util_last_bit(rctx->samplers[PIPE_SHADER_FRAGMENT].views.enabled_mask),
			(struct pipe_sampler_view**)rctx->samplers[PIPE_SHADER_FRAGMENT].views.views);
	}

	if ((op & R600_DISABLE_RENDER_COND) && rctx->b.current_render_cond) {
           util_blitter_save_render_condition(rctx->blitter,
                                              rctx->b.current_render_cond,
                                              rctx->b.current_render_cond_cond,
                                              rctx->b.current_render_cond_mode);
        }
}

static void r600_blitter_end(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
        r600_resume_nontimer_queries(&rctx->b);
}

static unsigned u_max_sample(struct pipe_resource *r)
{
	return r->nr_samples ? r->nr_samples - 1 : 0;
}

static void r600_blit_decompress_depth(struct pipe_context *ctx,
				       struct r600_texture *texture,
				       struct r600_texture *staging,
				       unsigned first_level, unsigned last_level,
				       unsigned first_layer, unsigned last_layer,
				       unsigned first_sample, unsigned last_sample)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, sample, checked_last_layer, max_layer, max_sample;
	struct r600_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;
	const struct util_format_description *desc =
		util_format_description(texture->resource.b.b.format);
	float depth;

	if (!staging && !texture->dirty_level_mask)
		return;

	max_sample = u_max_sample(&texture->resource.b.b);

	/* XXX Decompressing MSAA depth textures is broken on R6xx.
	 * There is also a hardlock if CMASK and FMASK are not present.
	 * Just skip this until we find out how to fix it. */
	if (rctx->b.chip_class == R600 && max_sample > 0) {
		texture->dirty_level_mask = 0;
		return;
	}

	if (rctx->b.family == CHIP_RV610 || rctx->b.family == CHIP_RV630 ||
	    rctx->b.family == CHIP_RV620 || rctx->b.family == CHIP_RV635)
		depth = 0.0f;
	else
		depth = 1.0f;

	/* Enable decompression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_through_cb = true;
	rctx->db_misc_state.copy_depth = util_format_has_depth(desc);
	rctx->db_misc_state.copy_stencil = util_format_has_stencil(desc);
	rctx->db_misc_state.copy_sample = first_sample;
	rctx->db_misc_state.atom.dirty = true;

	for (level = first_level; level <= last_level; level++) {
		if (!staging && !(texture->dirty_level_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = util_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			for (sample = first_sample; sample <= last_sample; sample++) {
				struct pipe_surface *zsurf, *cbsurf, surf_tmpl;

				if (sample != rctx->db_misc_state.copy_sample) {
					rctx->db_misc_state.copy_sample = sample;
					rctx->db_misc_state.atom.dirty = true;
				}

				surf_tmpl.format = texture->resource.b.b.format;
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;

				zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

				surf_tmpl.format = flushed_depth_texture->resource.b.b.format;
				cbsurf = ctx->create_surface(ctx,
						&flushed_depth_texture->resource.b.b, &surf_tmpl);

				r600_blitter_begin(ctx, R600_DECOMPRESS);
				util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, 1 << sample,
								  rctx->custom_dsa_flush, depth);
				r600_blitter_end(ctx);

				pipe_surface_reference(&zsurf, NULL);
				pipe_surface_reference(&cbsurf, NULL);
			}
		}

		/* The texture will always be dirty if some layers or samples aren't flushed.
		 * I don't think this case occurs often though. */
		if (!staging &&
		    first_layer == 0 && last_layer == max_layer &&
		    first_sample == 0 && last_sample == max_sample) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}

	/* reenable compression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_through_cb = false;
	rctx->db_misc_state.atom.dirty = true;
}

static void r600_blit_decompress_depth_in_place(struct r600_context *rctx,
                                                struct r600_texture *texture,
                                                unsigned first_level, unsigned last_level,
                                                unsigned first_layer, unsigned last_layer)
{
	struct pipe_surface *zsurf, surf_tmpl = {{0}};
	unsigned layer, max_layer, checked_last_layer, level;

	/* Enable decompression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_in_place = true;
	rctx->db_misc_state.atom.dirty = true;

	surf_tmpl.format = texture->resource.b.b.format;

	for (level = first_level; level <= last_level; level++) {
		if (!(texture->dirty_level_mask & (1 << level)))
			continue;

		surf_tmpl.u.tex.level = level;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = util_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;

			zsurf = rctx->b.b.create_surface(&rctx->b.b, &texture->resource.b.b, &surf_tmpl);

			r600_blitter_begin(&rctx->b.b, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, NULL, ~0,
							  rctx->custom_dsa_flush, 1.0f);
			r600_blitter_end(&rctx->b.b);

			pipe_surface_reference(&zsurf, NULL);
		}

		/* The texture will always be dirty if some layers or samples aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}

	/* Disable decompression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_in_place = false;
	rctx->db_misc_state.atom.dirty = true;
}

void r600_decompress_depth_textures(struct r600_context *rctx,
			       struct r600_samplerview_state *textures)
{
	unsigned i;
	unsigned depth_texture_mask = textures->compressed_depthtex_mask;

	while (depth_texture_mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&depth_texture_mask);

		view = &textures->views[i]->base;
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->is_depth && !tex->is_flushing_texture);

		if (rctx->b.chip_class >= EVERGREEN ||
		    r600_can_read_depth(tex)) {
			r600_blit_decompress_depth_in_place(rctx, tex,
						   view->u.tex.first_level, view->u.tex.last_level,
						   0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
		} else {
			r600_blit_decompress_depth(&rctx->b.b, tex, NULL,
						   view->u.tex.first_level, view->u.tex.last_level,
						   0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level),
						   0, u_max_sample(&tex->resource.b.b));
		}
	}
}

static void r600_blit_decompress_color(struct pipe_context *ctx,
		struct r600_texture *rtex,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, checked_last_layer, max_layer;

	if (!rtex->dirty_level_mask)
		return;

	for (level = first_level; level <= last_level; level++) {
		if (!(rtex->dirty_level_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = util_max_layer(&rtex->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *cbsurf, surf_tmpl;

			surf_tmpl.format = rtex->resource.b.b.format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			cbsurf = ctx->create_surface(ctx, &rtex->resource.b.b, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_color(rctx->blitter, cbsurf,
				rtex->fmask.size ? rctx->custom_blend_decompress : rctx->custom_blend_fastclear);
			r600_blitter_end(ctx);

			pipe_surface_reference(&cbsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			rtex->dirty_level_mask &= ~(1 << level);
		}
	}
}

void r600_decompress_color_textures(struct r600_context *rctx,
				    struct r600_samplerview_state *textures)
{
	unsigned i;
	unsigned mask = textures->compressed_colortex_mask;

	while (mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&mask);

		view = &textures->views[i]->base;
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->cmask.size);

		r600_blit_decompress_color(&rctx->b.b, tex,
					   view->u.tex.first_level, view->u.tex.last_level,
					   0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
	}
}

/* Helper for decompressing a portion of a color or depth resource before
 * blitting if any decompression is needed.
 * The driver doesn't decompress resources automatically while u_blitter is
 * rendering. */
static bool r600_decompress_subresource(struct pipe_context *ctx,
					struct pipe_resource *tex,
					unsigned level,
					unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_texture *rtex = (struct r600_texture*)tex;

	if (rtex->is_depth && !rtex->is_flushing_texture) {
		if (rctx->b.chip_class >= EVERGREEN ||
		    r600_can_read_depth(rtex)) {
			r600_blit_decompress_depth_in_place(rctx, rtex,
						   level, level,
						   first_layer, last_layer);
		} else {
			if (!r600_init_flushed_depth_texture(ctx, tex, NULL))
				return false; /* error */

			r600_blit_decompress_depth(ctx, rtex, NULL,
						   level, level,
						   first_layer, last_layer,
						   0, u_max_sample(tex));
		}
	} else if (rtex->cmask.size) {
		r600_blit_decompress_color(ctx, rtex, level, level,
					   first_layer, last_layer);
	}
	return true;
}

static void r600_clear(struct pipe_context *ctx, unsigned buffers,
		       const union pipe_color_union *color,
		       double depth, unsigned stencil)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_framebuffer_state *fb = &rctx->framebuffer.state;

	if (buffers & PIPE_CLEAR_COLOR && rctx->b.chip_class >= EVERGREEN) {
		evergreen_do_fast_color_clear(&rctx->b, fb, &rctx->framebuffer.atom,
					      &buffers, color);
	}

	if (buffers & PIPE_CLEAR_COLOR) {
		int i;

		/* These buffers cannot use fast clear, make sure to disable expansion. */
		for (i = 0; i < fb->nr_cbufs; i++) {
			struct r600_texture *tex;

			/* If not clearing this buffer, skip. */
			if (!(buffers & (PIPE_CLEAR_COLOR0 << i)))
				continue;

			if (!fb->cbufs[i])
				continue;

			tex = (struct r600_texture *)fb->cbufs[i]->texture;
			if (tex->fmask.size == 0)
				tex->dirty_level_mask &= ~(1 << fb->cbufs[i]->u.tex.level);
		}
	}

	/* if hyperz enabled just clear hyperz */
	if (fb->zsbuf && (buffers & PIPE_CLEAR_DEPTH)) {
		struct r600_texture *rtex;
		unsigned level = fb->zsbuf->u.tex.level;

		rtex = (struct r600_texture*)fb->zsbuf->texture;

		/* We can't use hyperz fast clear if each slice of a texture
		 * array are clear to different value. To simplify code just
		 * disable fast clear for texture array.
		 */
		/* Only use htile for first level */
		if (rtex->htile_buffer && !level &&
                   fb->zsbuf->u.tex.first_layer == 0 &&
                   fb->zsbuf->u.tex.last_layer == util_max_layer(&rtex->resource.b.b, level)) {
			if (rtex->depth_clear_value != depth) {
				rtex->depth_clear_value = depth;
				rctx->db_state.atom.dirty = true;
			}
			rctx->db_misc_state.htile_clear = true;
			rctx->db_misc_state.atom.dirty = true;
		}
	}

	r600_blitter_begin(ctx, R600_CLEAR);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
			   util_framebuffer_get_num_layers(fb),
			   buffers, color, depth, stencil);
	r600_blitter_end(ctx);

	/* disable fast clear */
	if (rctx->db_misc_state.htile_clear) {
		rctx->db_misc_state.htile_clear = false;
		rctx->db_misc_state.atom.dirty = true;
	}
}

static void r600_clear_render_target(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     const union pipe_color_union *color,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_blitter_begin(ctx, R600_CLEAR_SURFACE);
	util_blitter_clear_render_target(rctx->blitter, dst, color,
					 dstx, dsty, width, height);
	r600_blitter_end(ctx);
}

static void r600_clear_depth_stencil(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     unsigned clear_flags,
				     double depth,
				     unsigned stencil,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_blitter_begin(ctx, R600_CLEAR_SURFACE);
	util_blitter_clear_depth_stencil(rctx->blitter, dst, clear_flags, depth, stencil,
					 dstx, dsty, width, height);
	r600_blitter_end(ctx);
}

static void r600_copy_buffer(struct pipe_context *ctx, struct pipe_resource *dst, unsigned dstx,
			     struct pipe_resource *src, const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (rctx->screen->b.has_cp_dma) {
		r600_cp_dma_copy_buffer(rctx, dst, dstx, src, src_box->x, src_box->width);
	}
	else if (rctx->screen->b.has_streamout &&
		 /* Require 4-byte alignment. */
		 dstx % 4 == 0 && src_box->x % 4 == 0 && src_box->width % 4 == 0) {

		r600_blitter_begin(ctx, R600_COPY_BUFFER);
		util_blitter_copy_buffer(rctx->blitter, dst, dstx, src, src_box->x, src_box->width);
		r600_blitter_end(ctx);
	} else {
		util_resource_copy_region(ctx, dst, 0, dstx, 0, 0, src, 0, src_box);
	}

	/* The index buffer (VGT) doesn't seem to see the result of the copying.
	 * Can we somehow flush the index buffer cache? Starting a new IB seems
	 * to do the trick. */
	if (rctx->b.chip_class <= R700)
		rctx->b.rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
}

/**
 * Global buffers are not really resources, they are are actually offsets
 * into a single global resource (r600_screen::global_pool).  The means
 * they don't have their own cs_buf handle, so they cannot be passed
 * to r600_copy_buffer() and must be handled separately.
 */
static void r600_copy_global_buffer(struct pipe_context *ctx,
				    struct pipe_resource *dst, unsigned
				    dstx, struct pipe_resource *src,
				    const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct compute_memory_pool *pool = rctx->screen->global_pool;
	struct pipe_box new_src_box = *src_box;

	if (src->bind & PIPE_BIND_GLOBAL) {
		struct r600_resource_global *rsrc =
			(struct r600_resource_global *)src;
		struct compute_memory_item *item = rsrc->chunk;

		if (is_item_in_pool(item)) {
			new_src_box.x += 4 * item->start_in_dw;
			src = (struct pipe_resource *)pool->bo;
		} else {
			if (item->real_buffer == NULL) {
				item->real_buffer = (struct r600_resource*)
					r600_compute_buffer_alloc_vram(pool->screen,
								       item->size_in_dw * 4);
			}
			src = (struct pipe_resource*)item->real_buffer;
		}
	}
	if (dst->bind & PIPE_BIND_GLOBAL) {
		struct r600_resource_global *rdst =
			(struct r600_resource_global *)dst;
		struct compute_memory_item *item = rdst->chunk;

		if (is_item_in_pool(item)) {
			dstx += 4 * item->start_in_dw;
			dst = (struct pipe_resource *)pool->bo;
		} else {
			if (item->real_buffer == NULL) {
				item->real_buffer = (struct r600_resource*)
					r600_compute_buffer_alloc_vram(pool->screen,
								       item->size_in_dw * 4);
			}
			dst = (struct pipe_resource*)item->real_buffer;
		}
	}

	r600_copy_buffer(ctx, dst, dstx, src, &new_src_box);
}

static void r600_clear_buffer(struct pipe_context *ctx, struct pipe_resource *dst,
			      unsigned offset, unsigned size, unsigned value)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (rctx->screen->b.has_cp_dma &&
	    rctx->b.chip_class >= EVERGREEN &&
	    offset % 4 == 0 && size % 4 == 0) {
		evergreen_cp_dma_clear_buffer(rctx, dst, offset, size, value);
	} else if (rctx->screen->b.has_streamout && offset % 4 == 0 && size % 4 == 0) {
		union pipe_color_union clear_value;
		clear_value.ui[0] = value;

		r600_blitter_begin(ctx, R600_DISABLE_RENDER_COND);
		util_blitter_clear_buffer(rctx->blitter, dst, offset, size,
					  1, &clear_value);
		r600_blitter_end(ctx);
	} else {
		uint32_t *map = r600_buffer_map_sync_with_rings(&rctx->b, r600_resource(dst),
								 PIPE_TRANSFER_WRITE);
		size /= 4;
		for (unsigned i = 0; i < size; i++)
			*map++ = value;
	}
}

void r600_resource_copy_region(struct pipe_context *ctx,
			       struct pipe_resource *dst,
			       unsigned dst_level,
			       unsigned dstx, unsigned dsty, unsigned dstz,
			       struct pipe_resource *src,
			       unsigned src_level,
			       const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_surface *dst_view, dst_templ;
	struct pipe_sampler_view src_templ, *src_view;
	unsigned dst_width, dst_height, src_width0, src_height0, src_widthFL, src_heightFL;
	unsigned src_force_level = 0;
	struct pipe_box sbox, dstbox;

	/* Handle buffers first. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		if ((src->bind & PIPE_BIND_GLOBAL) ||
					(dst->bind & PIPE_BIND_GLOBAL)) {
			r600_copy_global_buffer(ctx, dst, dstx, src, src_box);
		} else {
			r600_copy_buffer(ctx, dst, dstx, src, src_box);
		}
		return;
	}

	assert(u_max_sample(dst) == u_max_sample(src));

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	if (!r600_decompress_subresource(ctx, src, src_level,
					 src_box->z, src_box->z + src_box->depth - 1)) {
		return; /* error */
	}

	dst_width = u_minify(dst->width0, dst_level);
        dst_height = u_minify(dst->height0, dst_level);
	src_width0 = src->width0;
	src_height0 = src->height0;
        src_widthFL = u_minify(src->width0, src_level);
        src_heightFL = u_minify(src->height0, src_level);

	util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstz);
	util_blitter_default_src_texture(&src_templ, src, src_level);

	if (util_format_is_compressed(src->format)) {
		unsigned blocksize = util_format_get_blocksize(src->format);

		if (blocksize == 8)
			src_templ.format = PIPE_FORMAT_R16G16B16A16_UINT; /* 64-bit block */
		else
			src_templ.format = PIPE_FORMAT_R32G32B32A32_UINT; /* 128-bit block */
		dst_templ.format = src_templ.format;

		dst_width = util_format_get_nblocksx(dst->format, dst_width);
		dst_height = util_format_get_nblocksy(dst->format, dst_height);
		src_width0 = util_format_get_nblocksx(src->format, src_width0);
		src_height0 = util_format_get_nblocksy(src->format, src_height0);
		src_widthFL = util_format_get_nblocksx(src->format, src_widthFL);
		src_heightFL = util_format_get_nblocksy(src->format, src_heightFL);

		dstx = util_format_get_nblocksx(dst->format, dstx);
		dsty = util_format_get_nblocksy(dst->format, dsty);

		sbox.x = util_format_get_nblocksx(src->format, src_box->x);
		sbox.y = util_format_get_nblocksy(src->format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(src->format, src_box->width);
		sbox.height = util_format_get_nblocksy(src->format, src_box->height);
		sbox.depth = src_box->depth;
		src_box = &sbox;

		src_force_level = src_level;
	} else if (!util_blitter_is_copy_supported(rctx->blitter, dst, src)) {
		if (util_format_is_subsampled_422(src->format)) {

			src_templ.format = PIPE_FORMAT_R8G8B8A8_UINT;
			dst_templ.format = PIPE_FORMAT_R8G8B8A8_UINT;

			dst_width = util_format_get_nblocksx(dst->format, dst_width);
			src_width0 = util_format_get_nblocksx(src->format, src_width0);
			src_widthFL = util_format_get_nblocksx(src->format, src_widthFL);

			dstx = util_format_get_nblocksx(dst->format, dstx);

			sbox = *src_box;
			sbox.x = util_format_get_nblocksx(src->format, src_box->x);
			sbox.width = util_format_get_nblocksx(src->format, src_box->width);
			src_box = &sbox;
		} else {
			unsigned blocksize = util_format_get_blocksize(src->format);

			switch (blocksize) {
			case 1:
				dst_templ.format = PIPE_FORMAT_R8_UNORM;
				src_templ.format = PIPE_FORMAT_R8_UNORM;
				break;
                        case 2:
				dst_templ.format = PIPE_FORMAT_R8G8_UNORM;
				src_templ.format = PIPE_FORMAT_R8G8_UNORM;
				break;
			case 4:
				dst_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
				src_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
				break;
                        case 8:
                                dst_templ.format = PIPE_FORMAT_R16G16B16A16_UINT;
                                src_templ.format = PIPE_FORMAT_R16G16B16A16_UINT;
                                break;
                        case 16:
                                dst_templ.format = PIPE_FORMAT_R32G32B32A32_UINT;
                                src_templ.format = PIPE_FORMAT_R32G32B32A32_UINT;
                                break;
			default:
				fprintf(stderr, "Unhandled format %s with blocksize %u\n",
					util_format_short_name(src->format), blocksize);
				assert(0);
			}
		}
	}

	dst_view = r600_create_surface_custom(ctx, dst, &dst_templ, dst_width, dst_height);

	if (rctx->b.chip_class >= EVERGREEN) {
		src_view = evergreen_create_sampler_view_custom(ctx, src, &src_templ,
								src_width0, src_height0,
								src_force_level);
	} else {
		src_view = r600_create_sampler_view_custom(ctx, src, &src_templ,
							   src_widthFL, src_heightFL);
	}

        u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height),
                 abs(src_box->depth), &dstbox);

	/* Copy. */
	r600_blitter_begin(ctx, R600_COPY_TEXTURE);
	util_blitter_blit_generic(rctx->blitter, dst_view, &dstbox,
				  src_view, src_box, src_width0, src_height0,
				  PIPE_MASK_RGBAZS, PIPE_TEX_FILTER_NEAREST, NULL);
	r600_blitter_end(ctx);

	pipe_surface_reference(&dst_view, NULL);
	pipe_sampler_view_reference(&src_view, NULL);
}

/* For MSAA integer resolving to work, we change the format to NORM using this function. */
static enum pipe_format int_to_norm_format(enum pipe_format format)
{
	switch (format) {
#define REPLACE_FORMAT_SIGN(format,sign) \
	case PIPE_FORMAT_##format##_##sign##INT: \
		return PIPE_FORMAT_##format##_##sign##NORM
#define REPLACE_FORMAT(format) \
		REPLACE_FORMAT_SIGN(format, U); \
		REPLACE_FORMAT_SIGN(format, S)

	REPLACE_FORMAT_SIGN(B10G10R10A2, U);
	REPLACE_FORMAT(R8);
	REPLACE_FORMAT(R8G8);
	REPLACE_FORMAT(R8G8B8X8);
	REPLACE_FORMAT(R8G8B8A8);
	REPLACE_FORMAT(A8);
	REPLACE_FORMAT(I8);
	REPLACE_FORMAT(L8);
	REPLACE_FORMAT(L8A8);
	REPLACE_FORMAT(R16);
	REPLACE_FORMAT(R16G16);
	REPLACE_FORMAT(R16G16B16X16);
	REPLACE_FORMAT(R16G16B16A16);
	REPLACE_FORMAT(A16);
	REPLACE_FORMAT(I16);
	REPLACE_FORMAT(L16);
	REPLACE_FORMAT(L16A16);

#undef REPLACE_FORMAT
#undef REPLACE_FORMAT_SIGN
	default:
		return format;
	}
}

static bool do_hardware_msaa_resolve(struct pipe_context *ctx,
				     const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_texture *dst = (struct r600_texture*)info->dst.resource;
	unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
	unsigned dst_height = u_minify(info->dst.resource->height0, info->dst.level);
	enum pipe_format format = int_to_norm_format(info->dst.format);
	unsigned sample_mask =
		rctx->b.chip_class == CAYMAN ? ~0 :
		((1ull << MAX2(1, info->src.resource->nr_samples)) - 1);

	if (info->src.resource->nr_samples > 1 &&
	    info->dst.resource->nr_samples <= 1 &&
	    util_max_layer(info->src.resource, 0) == 0 &&
	    util_max_layer(info->dst.resource, info->dst.level) == 0 &&
	    info->dst.format == info->src.format &&
	    !util_format_is_pure_integer(format) &&
	    !util_format_is_depth_or_stencil(format) &&
	    !info->scissor_enable &&
	    (info->mask & PIPE_MASK_RGBA) == PIPE_MASK_RGBA &&
	    dst_width == info->src.resource->width0 &&
	    dst_height == info->src.resource->height0 &&
	    info->dst.box.x == 0 &&
	    info->dst.box.y == 0 &&
	    info->dst.box.width == dst_width &&
	    info->dst.box.height == dst_height &&
	    info->dst.box.depth == 1 &&
	    info->src.box.x == 0 &&
	    info->src.box.y == 0 &&
	    info->src.box.width == dst_width &&
	    info->src.box.height == dst_height &&
	    info->src.box.depth == 1 &&
	    dst->surface.level[info->dst.level].mode >= RADEON_SURF_MODE_1D &&
	    (!dst->cmask.size || !dst->dirty_level_mask) /* dst cannot be fast-cleared */) {
		r600_blitter_begin(ctx, R600_COLOR_RESOLVE |
				   (info->render_condition_enable ? 0 : R600_DISABLE_RENDER_COND));
		util_blitter_custom_resolve_color(rctx->blitter,
						  info->dst.resource, info->dst.level,
						  info->dst.box.z,
						  info->src.resource, info->src.box.z,
						  sample_mask, rctx->custom_blend_resolve,
						  format);
		r600_blitter_end(ctx);
		return true;
	}
	return false;
}

static void r600_blit(struct pipe_context *ctx,
                      const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (do_hardware_msaa_resolve(ctx, info)) {
		return;
	}

	assert(util_blitter_is_blit_supported(rctx->blitter, info));

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	if (!r600_decompress_subresource(ctx, info->src.resource, info->src.level,
					 info->src.box.z,
					 info->src.box.z + info->src.box.depth - 1)) {
		return; /* error */
	}

	if (rctx->screen->b.debug_flags & DBG_FORCE_DMA &&
	    util_try_blit_via_copy_region(ctx, info))
		return;

	r600_blitter_begin(ctx, R600_BLIT |
			   (info->render_condition_enable ? 0 : R600_DISABLE_RENDER_COND));
	util_blitter_blit(rctx->blitter, info);
	r600_blitter_end(ctx);
}

static void r600_flush_resource(struct pipe_context *ctx,
				struct pipe_resource *res)
{
	struct r600_texture *rtex = (struct r600_texture*)res;

	assert(res->target != PIPE_BUFFER);

	if (!rtex->is_depth && rtex->cmask.size) {
		r600_blit_decompress_color(ctx, rtex, 0, res->last_level,
					   0, util_max_layer(res, 0));
	}
}

void r600_init_blit_functions(struct r600_context *rctx)
{
	rctx->b.b.clear = r600_clear;
	rctx->b.b.clear_render_target = r600_clear_render_target;
	rctx->b.b.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->b.b.resource_copy_region = r600_resource_copy_region;
	rctx->b.b.blit = r600_blit;
	rctx->b.b.flush_resource = r600_flush_resource;
	rctx->b.clear_buffer = r600_clear_buffer;
	rctx->b.blit_decompress_depth = r600_blit_decompress_depth;
}
