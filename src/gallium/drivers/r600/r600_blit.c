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
#include "util/u_surface.h"
#include "util/u_blitter.h"
#include "util/u_format.h"

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

	R600_BLIT          = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_DECOMPRESS    = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,

	R600_COLOR_RESOLVE = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_suspend_nontimer_queries(rctx);

	util_blitter_save_vertex_buffer_slot(rctx->blitter, rctx->vertex_buffer_state.vb);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_fetch_shader.cso);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_so_targets(rctx->blitter, rctx->num_so_targets,
				     (struct pipe_stream_output_target**)rctx->so_targets);
	util_blitter_save_rasterizer(rctx->blitter, rctx->rasterizer_state.cso);

	if (op & R600_SAVE_FRAGMENT_STATE) {
		util_blitter_save_viewport(rctx->blitter, &rctx->viewport.state);
		util_blitter_save_scissor(rctx->blitter, &rctx->scissor.scissor);
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

	if ((op & R600_DISABLE_RENDER_COND) && rctx->current_render_cond) {
           util_blitter_save_render_condition(rctx->blitter,
                                              rctx->current_render_cond,
                                              rctx->current_render_cond_mode);
        }
}

static void r600_blitter_end(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
        r600_resume_nontimer_queries(rctx);
}

static unsigned u_max_sample(struct pipe_resource *r)
{
	return r->nr_samples ? r->nr_samples - 1 : 0;
}

void r600_blit_decompress_depth(struct pipe_context *ctx,
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
	if (rctx->chip_class == R600 && max_sample > 0) {
		texture->dirty_level_mask = 0;
		return;
	}

	if (rctx->family == CHIP_RV610 || rctx->family == CHIP_RV630 ||
	    rctx->family == CHIP_RV620 || rctx->family == CHIP_RV635)
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
		max_layer = u_max_layer(&texture->resource.b.b, level);
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
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;
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
		max_layer = u_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;

			zsurf = rctx->context.create_surface(&rctx->context, &texture->resource.b.b, &surf_tmpl);

			r600_blitter_begin(&rctx->context, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, NULL, ~0,
							  rctx->custom_dsa_flush, 1.0f);
			r600_blitter_end(&rctx->context);

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

		if (rctx->chip_class >= EVERGREEN ||
		    r600_can_read_depth(tex)) {
			r600_blit_decompress_depth_in_place(rctx, tex,
						   view->u.tex.first_level, view->u.tex.last_level,
						   0, u_max_layer(&tex->resource.b.b, view->u.tex.first_level));
		} else {
			r600_blit_decompress_depth(&rctx->context, tex, NULL,
						   view->u.tex.first_level, view->u.tex.last_level,
						   0, u_max_layer(&tex->resource.b.b, view->u.tex.first_level),
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
	void *blend_decompress;

	if (!rtex->dirty_level_mask)
		return;

	switch (rctx->screen->msaa_texture_support) {
	case MSAA_TEXTURE_DECOMPRESSED:
		blend_decompress = rctx->custom_blend_decompress;
		break;
	case MSAA_TEXTURE_COMPRESSED:
		/* XXX the 2x and 4x cases are broken. */
		if (rtex->resource.b.b.nr_samples == 8)
			blend_decompress = rctx->custom_blend_fmask_decompress;
		else
			blend_decompress = rctx->custom_blend_decompress;
		break;
	case MSAA_TEXTURE_SAMPLE_ZERO:
	default:
		/* Nothing to do. */
		rtex->dirty_level_mask = 0;
		return;
	}

	for (level = first_level; level <= last_level; level++) {
		if (!(rtex->dirty_level_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = u_max_layer(&rtex->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *cbsurf, surf_tmpl;

			surf_tmpl.format = rtex->resource.b.b.format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			cbsurf = ctx->create_surface(ctx, &rtex->resource.b.b, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_color(rctx->blitter, cbsurf, blend_decompress);
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
		assert(tex->cmask_size && tex->fmask_size);

		r600_blit_decompress_color(&rctx->context, tex,
					   view->u.tex.first_level, view->u.tex.last_level,
					   0, u_max_layer(&tex->resource.b.b, view->u.tex.first_level));
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
		if (rctx->chip_class >= EVERGREEN ||
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
	} else if (rtex->fmask_size && rtex->cmask_size) {
		r600_blit_decompress_color(ctx, rtex, level, level,
					   first_layer, last_layer);
	}
	return true;
}

static boolean is_simple_msaa_resolve(const struct pipe_blit_info *info)
{
	unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
	unsigned dst_height = u_minify(info->dst.resource->height0, info->dst.level);
	struct r600_texture *dst = (struct r600_texture*)info->dst.resource;
	unsigned dst_tile_mode = dst->surface.level[info->dst.level].mode;

	return info->dst.resource->format == info->src.resource->format &&
		info->dst.resource->format == info->dst.format &&
		info->src.resource->format == info->src.format &&
		!info->scissor_enable &&
		info->mask == PIPE_MASK_RGBA &&
		dst_width == info->src.resource->width0 &&
		dst_height == info->src.resource->height0 &&
		info->dst.box.x == 0 &&
		info->dst.box.y == 0 &&
		info->dst.box.width == dst_width &&
		info->dst.box.height == dst_height &&
		info->src.box.x == 0 &&
		info->src.box.y == 0 &&
		info->src.box.width == dst_width &&
		info->src.box.height == dst_height &&
		/* Dst must be tiled. If it's not, we have to use a temporary
		 * resource which is tiled. */
		dst_tile_mode >= RADEON_SURF_MODE_1D;
}

static void r600_clear(struct pipe_context *ctx, unsigned buffers,
		       const union pipe_color_union *color,
		       double depth, unsigned stencil)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_framebuffer_state *fb = &rctx->framebuffer.state;

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
		if (rtex->htile && !level && rtex->surface.array_size == 1) {
			if (rtex->depth_clear != depth) {
				rtex->depth_clear = depth;
				rctx->db_state.atom.dirty = true;
			}
			rctx->db_misc_state.htile_clear = true;
			rctx->db_misc_state.atom.dirty = true;
		}
	}

	r600_blitter_begin(ctx, R600_CLEAR);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
			   fb->nr_cbufs, buffers, fb->nr_cbufs ? fb->cbufs[0]->format : PIPE_FORMAT_NONE,
			   color, depth, stencil);
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

void r600_copy_buffer(struct pipe_context *ctx, struct pipe_resource *dst, unsigned dstx,
		      struct pipe_resource *src, const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	/* CP DMA doesn't work on R600 (flushing seems to be unreliable). */
	if (rctx->screen->info.drm_minor >= 27 && rctx->chip_class >= R700) {
		r600_cp_dma_copy_buffer(rctx, dst, dstx, src, src_box->x, src_box->width);
	}
	else if (rctx->screen->has_streamout &&
		 /* Require 4-byte alignment. */
		 dstx % 4 == 0 && src_box->x % 4 == 0 && src_box->width % 4 == 0) {
		r600_blitter_begin(ctx, R600_COPY_BUFFER);
		util_blitter_copy_buffer(rctx->blitter, dst, dstx, src, src_box->x, src_box->width);
		r600_blitter_end(ctx);
	} else {
		util_resource_copy_region(ctx, dst, 0, dstx, 0, 0, src, 0, src_box);
	}
}

static bool util_format_is_subsampled_2x1_32bpp(enum pipe_format format)
{
	const struct util_format_description *desc = util_format_description(format);

	return desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED &&
	       desc->block.width == 2 &&
	       desc->block.height == 1 &&
	       desc->block.bits == 32;
}

static void r600_resource_copy_region(struct pipe_context *ctx,
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
	struct pipe_box sbox, dstbox;
	bool copy_all_samples;

	/* Handle buffers first. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		r600_copy_buffer(ctx, dst, dstx, src, src_box);
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
	} else if (!util_blitter_is_copy_supported(rctx->blitter, dst, src,
						   PIPE_MASK_RGBAZS)) {
		if (util_format_is_subsampled_2x1_32bpp(src->format)) {

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

	if (rctx->chip_class >= EVERGREEN) {
		src_view = evergreen_create_sampler_view_custom(ctx, src, &src_templ,
								src_width0, src_height0);
	} else {
		src_view = r600_create_sampler_view_custom(ctx, src, &src_templ,
							   src_widthFL, src_heightFL);
	}

	copy_all_samples = rctx->screen->msaa_texture_support != MSAA_TEXTURE_SAMPLE_ZERO;

        u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height),
                 abs(src_box->depth), &dstbox);

	/* Copy. */
	r600_blitter_begin(ctx, R600_COPY_TEXTURE);
	util_blitter_blit_generic(rctx->blitter, dst_view, &dstbox,
				  src_view, src_box, src_width0, src_height0,
				  PIPE_MASK_RGBAZS, PIPE_TEX_FILTER_NEAREST, NULL,
				  copy_all_samples);
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
	REPLACE_FORMAT(R8G8B8);
	REPLACE_FORMAT(R8G8B8A8);
	REPLACE_FORMAT(A8);
	REPLACE_FORMAT(I8);
	REPLACE_FORMAT(L8);
	REPLACE_FORMAT(L8A8);
	REPLACE_FORMAT(R16);
	REPLACE_FORMAT(R16G16);
	REPLACE_FORMAT(R16G16B16);
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

static void r600_msaa_color_resolve(struct pipe_context *ctx,
			      const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_screen *screen = ctx->screen;
	struct pipe_resource *tmp, templ;
	struct pipe_blit_info blit;
	unsigned sample_mask =
		rctx->chip_class == CAYMAN ? ~0 :
		((1ull << MAX2(1, info->src.resource->nr_samples)) - 1);

	assert(info->src.level == 0);
	assert(info->src.box.depth == 1);
	assert(info->dst.box.depth == 1);

	if (is_simple_msaa_resolve(info)) {
		r600_blitter_begin(ctx, R600_COLOR_RESOLVE);
		util_blitter_custom_resolve_color(rctx->blitter,
						  info->dst.resource, info->dst.level,
						  info->dst.box.z,
						  info->src.resource, info->src.box.z,
						  sample_mask, rctx->custom_blend_resolve,
                                                  int_to_norm_format(info->dst.format));
		r600_blitter_end(ctx);
		return;
	}

	/* resolve into a temporary texture, then blit */
	templ.target = PIPE_TEXTURE_2D;
	templ.format = info->src.resource->format;
	templ.width0 = info->src.resource->width0;
	templ.height0 = info->src.resource->height0;
	templ.depth0 = 1;
	templ.array_size = 1;
	templ.last_level = 0;
	templ.nr_samples = 0;
	templ.usage = PIPE_USAGE_STATIC;
	templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
	templ.flags = R600_RESOURCE_FLAG_FORCE_TILING; /* dst must not have a linear layout */

	tmp = screen->resource_create(screen, &templ);

	/* resolve */
	r600_blitter_begin(ctx, R600_COLOR_RESOLVE);
	util_blitter_custom_resolve_color(rctx->blitter,
					  tmp, 0, 0,
					  info->src.resource, info->src.box.z,
					  sample_mask, rctx->custom_blend_resolve,
                                          int_to_norm_format(tmp->format));
	r600_blitter_end(ctx);

	/* blit */
	blit = *info;
	blit.src.resource = tmp;
	blit.src.box.z = 0;

	r600_blitter_begin(ctx, R600_BLIT);
	util_blitter_blit(rctx->blitter, &blit);
	r600_blitter_end(ctx);

	pipe_resource_reference(&tmp, NULL);
}

static void r600_blit(struct pipe_context *ctx,
                      const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	assert(util_blitter_is_blit_supported(rctx->blitter, info));

	if (info->src.resource->nr_samples > 1 &&
	    info->dst.resource->nr_samples <= 1 &&
	    !util_format_is_depth_or_stencil(info->src.resource->format) &&
	    !util_format_is_pure_integer(int_to_norm_format(info->src.resource->format))) {
		r600_msaa_color_resolve(ctx, info);
		return;
	}

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	if (!r600_decompress_subresource(ctx, info->src.resource, info->src.level,
					 info->src.box.z,
					 info->src.box.z + info->src.box.depth - 1)) {
		return; /* error */
	}

	r600_blitter_begin(ctx, R600_BLIT);
	util_blitter_blit(rctx->blitter, info);
	r600_blitter_end(ctx);
}

void r600_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
	rctx->context.blit = r600_blit;
}
