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

#include "si_pipe.h"
#include "util/u_format.h"
#include "util/u_surface.h"

enum si_blitter_op /* bitmask */
{
	SI_SAVE_TEXTURES      = 1,
	SI_SAVE_FRAMEBUFFER   = 2,
	SI_DISABLE_RENDER_COND = 4,

	SI_CLEAR         = 0,

	SI_CLEAR_SURFACE = SI_SAVE_FRAMEBUFFER,

	SI_COPY          = SI_SAVE_FRAMEBUFFER | SI_SAVE_TEXTURES |
			   SI_DISABLE_RENDER_COND,

	SI_BLIT          = SI_SAVE_FRAMEBUFFER | SI_SAVE_TEXTURES,

	SI_DECOMPRESS    = SI_SAVE_FRAMEBUFFER | SI_DISABLE_RENDER_COND,

	SI_COLOR_RESOLVE = SI_SAVE_FRAMEBUFFER
};

static void si_blitter_begin(struct pipe_context *ctx, enum si_blitter_op op)
{
	struct si_context *sctx = (struct si_context *)ctx;

	r600_suspend_nontimer_queries(&sctx->b);

	util_blitter_save_blend(sctx->blitter, sctx->queued.named.blend);
	util_blitter_save_depth_stencil_alpha(sctx->blitter, sctx->queued.named.dsa);
	util_blitter_save_stencil_ref(sctx->blitter, &sctx->stencil_ref);
	util_blitter_save_rasterizer(sctx->blitter, sctx->queued.named.rasterizer);
	util_blitter_save_fragment_shader(sctx->blitter, sctx->ps_shader);
	util_blitter_save_geometry_shader(sctx->blitter, sctx->gs_shader);
	util_blitter_save_vertex_shader(sctx->blitter, sctx->vs_shader);
	util_blitter_save_vertex_elements(sctx->blitter, sctx->vertex_elements);
	if (sctx->queued.named.sample_mask) {
		util_blitter_save_sample_mask(sctx->blitter,
					      sctx->queued.named.sample_mask->sample_mask);
	}
	if (sctx->queued.named.viewport) {
		util_blitter_save_viewport(sctx->blitter, &sctx->queued.named.viewport->viewport);
	}
	if (sctx->queued.named.scissor) {
		util_blitter_save_scissor(sctx->blitter, &sctx->queued.named.scissor->scissor);
	}
	util_blitter_save_vertex_buffer_slot(sctx->blitter, sctx->vertex_buffer);
	util_blitter_save_so_targets(sctx->blitter, sctx->b.streamout.num_targets,
				     (struct pipe_stream_output_target**)sctx->b.streamout.targets);

	if (op & SI_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(sctx->blitter, &sctx->framebuffer.state);

	if (op & SI_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			sctx->blitter, 2,
			sctx->samplers[PIPE_SHADER_FRAGMENT].states.saved_states);

		util_blitter_save_fragment_sampler_views(sctx->blitter, 2,
			sctx->samplers[PIPE_SHADER_FRAGMENT].views.views);
	}

	if ((op & SI_DISABLE_RENDER_COND) && sctx->b.current_render_cond) {
		util_blitter_save_render_condition(sctx->blitter,
                                                   sctx->b.current_render_cond,
                                                   sctx->b.current_render_cond_cond,
                                                   sctx->b.current_render_cond_mode);
	}
}

static void si_blitter_end(struct pipe_context *ctx)
{
	struct si_context *sctx = (struct si_context *)ctx;
	r600_resume_nontimer_queries(&sctx->b);
}

static unsigned u_max_sample(struct pipe_resource *r)
{
	return r->nr_samples ? r->nr_samples - 1 : 0;
}

static void si_blit_decompress_depth(struct pipe_context *ctx,
				     struct r600_texture *texture,
				     struct r600_texture *staging,
				     unsigned first_level, unsigned last_level,
				     unsigned first_layer, unsigned last_layer,
				     unsigned first_sample, unsigned last_sample)
{
	struct si_context *sctx = (struct si_context *)ctx;
	unsigned layer, level, sample, checked_last_layer, max_layer, max_sample;
	float depth = 1.0f;
	const struct util_format_description *desc;
	struct r600_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;

	if (!staging && !texture->dirty_level_mask)
		return;

	max_sample = u_max_sample(&texture->resource.b.b);

	desc = util_format_description(flushed_depth_texture->resource.b.b.format);

	if (util_format_has_depth(desc))
		sctx->dbcb_depth_copy_enabled = true;
	if (util_format_has_stencil(desc))
		sctx->dbcb_stencil_copy_enabled = true;

	assert(sctx->dbcb_depth_copy_enabled || sctx->dbcb_stencil_copy_enabled);

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

				sctx->dbcb_copy_sample = sample;
				sctx->db_render_state.dirty = true;

				surf_tmpl.format = texture->resource.b.b.format;
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;

				zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

				surf_tmpl.format = flushed_depth_texture->resource.b.b.format;
				cbsurf = ctx->create_surface(ctx,
						(struct pipe_resource*)flushed_depth_texture, &surf_tmpl);

				si_blitter_begin(ctx, SI_DECOMPRESS);
				util_blitter_custom_depth_stencil(sctx->blitter, zsurf, cbsurf, 1 << sample,
								  sctx->custom_dsa_flush, depth);
				si_blitter_end(ctx);

				pipe_surface_reference(&zsurf, NULL);
				pipe_surface_reference(&cbsurf, NULL);
			}
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case can occur though. */
		if (!staging &&
		    first_layer == 0 && last_layer == max_layer &&
		    first_sample == 0 && last_sample == max_sample) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}

	sctx->dbcb_depth_copy_enabled = false;
	sctx->dbcb_stencil_copy_enabled = false;
	sctx->db_render_state.dirty = true;
}

static void si_blit_decompress_depth_in_place(struct si_context *sctx,
                                              struct r600_texture *texture,
                                              unsigned first_level, unsigned last_level,
                                              unsigned first_layer, unsigned last_layer)
{
	struct pipe_surface *zsurf, surf_tmpl = {{0}};
	unsigned layer, max_layer, checked_last_layer, level;

	sctx->db_inplace_flush_enabled = true;
	sctx->db_render_state.dirty = true;

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

			zsurf = sctx->b.b.create_surface(&sctx->b.b, &texture->resource.b.b, &surf_tmpl);

			si_blitter_begin(&sctx->b.b, SI_DECOMPRESS);
			util_blitter_custom_depth_stencil(sctx->blitter, zsurf, NULL, ~0,
							  sctx->custom_dsa_flush,
							  1.0f);
			si_blitter_end(&sctx->b.b);

			pipe_surface_reference(&zsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}

	sctx->db_inplace_flush_enabled = false;
	sctx->db_render_state.dirty = true;
}

void si_flush_depth_textures(struct si_context *sctx,
			     struct si_textures_info *textures)
{
	unsigned i;
	unsigned mask = textures->depth_texture_mask;

	while (mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&mask);

		view = textures->views.views[i];
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->is_depth && !tex->is_flushing_texture);

		si_blit_decompress_depth_in_place(sctx, tex,
						  view->u.tex.first_level, view->u.tex.last_level,
						  0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
	}
}

static void si_blit_decompress_color(struct pipe_context *ctx,
		struct r600_texture *rtex,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer)
{
	struct si_context *sctx = (struct si_context *)ctx;
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

			si_blitter_begin(ctx, SI_DECOMPRESS);
			util_blitter_custom_color(sctx->blitter, cbsurf,
				rtex->fmask.size ? sctx->custom_blend_decompress :
						   sctx->custom_blend_fastclear);
			si_blitter_end(ctx);

			pipe_surface_reference(&cbsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			rtex->dirty_level_mask &= ~(1 << level);
		}
	}
}

void si_decompress_color_textures(struct si_context *sctx,
				  struct si_textures_info *textures)
{
	unsigned i;
	unsigned mask = textures->compressed_colortex_mask;

	while (mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&mask);

		view = textures->views.views[i];
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->cmask.size || tex->fmask.size);

		si_blit_decompress_color(&sctx->b.b, tex,
					 view->u.tex.first_level, view->u.tex.last_level,
					 0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
	}
}

static void si_clear(struct pipe_context *ctx, unsigned buffers,
		     const union pipe_color_union *color,
		     double depth, unsigned stencil)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct pipe_framebuffer_state *fb = &sctx->framebuffer.state;
	struct pipe_surface *zsbuf = fb->zsbuf;
	struct r600_texture *zstex =
		zsbuf ? (struct r600_texture*)zsbuf->texture : NULL;

	if (buffers & PIPE_CLEAR_COLOR) {
		evergreen_do_fast_color_clear(&sctx->b, fb, &sctx->framebuffer.atom,
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

	if (buffers & PIPE_CLEAR_DEPTH &&
	    zstex && zstex->htile_buffer &&
	    zsbuf->u.tex.level == 0 &&
	    zsbuf->u.tex.first_layer == 0 &&
	    zsbuf->u.tex.last_layer == util_max_layer(&zstex->resource.b.b, 0)) {
		/* Need to disable EXPCLEAR temporarily if clearing
		 * to a new value. */
		if (zstex->depth_cleared && zstex->depth_clear_value != depth) {
			sctx->db_depth_disable_expclear = true;
		}

		zstex->depth_clear_value = depth;
		sctx->framebuffer.atom.dirty = true; /* updates DB_DEPTH_CLEAR */
		sctx->db_depth_clear = true;
		sctx->db_render_state.dirty = true;
	}

	si_blitter_begin(ctx, SI_CLEAR);
	util_blitter_clear(sctx->blitter, fb->width, fb->height,
			   util_framebuffer_get_num_layers(fb),
			   buffers, color, depth, stencil);
	si_blitter_end(ctx);

	if (sctx->db_depth_clear) {
		sctx->db_depth_clear = false;
		sctx->db_depth_disable_expclear = false;
		zstex->depth_cleared = true;
		sctx->db_render_state.dirty = true;
	}
}

static void si_clear_render_target(struct pipe_context *ctx,
				   struct pipe_surface *dst,
				   const union pipe_color_union *color,
				   unsigned dstx, unsigned dsty,
				   unsigned width, unsigned height)
{
	struct si_context *sctx = (struct si_context *)ctx;

	si_blitter_begin(ctx, SI_CLEAR_SURFACE);
	util_blitter_clear_render_target(sctx->blitter, dst, color,
					 dstx, dsty, width, height);
	si_blitter_end(ctx);
}

static void si_clear_depth_stencil(struct pipe_context *ctx,
				   struct pipe_surface *dst,
				   unsigned clear_flags,
				   double depth,
				   unsigned stencil,
				   unsigned dstx, unsigned dsty,
				   unsigned width, unsigned height)
{
	struct si_context *sctx = (struct si_context *)ctx;

	si_blitter_begin(ctx, SI_CLEAR_SURFACE);
	util_blitter_clear_depth_stencil(sctx->blitter, dst, clear_flags, depth, stencil,
					 dstx, dsty, width, height);
	si_blitter_end(ctx);
}

/* Helper for decompressing a portion of a color or depth resource before
 * blitting if any decompression is needed.
 * The driver doesn't decompress resources automatically while u_blitter is
 * rendering. */
static void si_decompress_subresource(struct pipe_context *ctx,
				      struct pipe_resource *tex,
				      unsigned level,
				      unsigned first_layer, unsigned last_layer)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct r600_texture *rtex = (struct r600_texture*)tex;

	if (rtex->is_depth && !rtex->is_flushing_texture) {
		si_blit_decompress_depth_in_place(sctx, rtex,
						  level, level,
						  first_layer, last_layer);
	} else if (rtex->fmask.size || rtex->cmask.size) {
		si_blit_decompress_color(ctx, rtex, level, level,
					 first_layer, last_layer);
	}
}

struct texture_orig_info {
	unsigned format;
	unsigned width0;
	unsigned height0;
	unsigned npix_x;
	unsigned npix_y;
	unsigned npix0_x;
	unsigned npix0_y;
};

static void si_compressed_to_blittable(struct pipe_resource *tex,
				       unsigned level,
				       struct texture_orig_info *orig)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;
	unsigned pixsize = util_format_get_blocksize(rtex->resource.b.b.format);
	int new_format;
	int new_height, new_width;

	orig->format = tex->format;
	orig->width0 = tex->width0;
	orig->height0 = tex->height0;
	orig->npix0_x = rtex->surface.level[0].npix_x;
	orig->npix0_y = rtex->surface.level[0].npix_y;
	orig->npix_x = rtex->surface.level[level].npix_x;
	orig->npix_y = rtex->surface.level[level].npix_y;

	if (pixsize == 8)
		new_format = PIPE_FORMAT_R16G16B16A16_UINT; /* 64-bit block */
	else
		new_format = PIPE_FORMAT_R32G32B32A32_UINT; /* 128-bit block */

	new_width = util_format_get_nblocksx(tex->format, orig->width0);
	new_height = util_format_get_nblocksy(tex->format, orig->height0);

	tex->width0 = new_width;
	tex->height0 = new_height;
	tex->format = new_format;
	rtex->surface.level[0].npix_x = util_format_get_nblocksx(orig->format, orig->npix0_x);
	rtex->surface.level[0].npix_y = util_format_get_nblocksy(orig->format, orig->npix0_y);
	rtex->surface.level[level].npix_x = util_format_get_nblocksx(orig->format, orig->npix_x);
	rtex->surface.level[level].npix_y = util_format_get_nblocksy(orig->format, orig->npix_y);

	/* By dividing the dimensions by 4, we effectively decrement
	 * last_level by 2, therefore the last 2 mipmap levels disappear and
	 * aren't blittable. Note that the last 3 mipmap levels (4x4, 2x2,
	 * 1x1) have equal slice sizes, which is an important assumption
	 * for this to work.
	 *
	 * In order to make the last 2 mipmap levels blittable, we have to
	 * add the slice size of the last mipmap level to the texture
	 * address, so that even though the hw thinks it reads last_level-2,
	 * it will actually read last_level-1, and if we add the slice size*2,
	 * it will read last_level. That's how this workaround works.
	 */
	if (level > rtex->resource.b.b.last_level-2)
		rtex->mipmap_shift = level - (rtex->resource.b.b.last_level-2);
}

static void si_change_format(struct pipe_resource *tex,
			     unsigned level,
			     struct texture_orig_info *orig,
			     enum pipe_format format)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;

	orig->format = tex->format;
	orig->width0 = tex->width0;
	orig->height0 = tex->height0;
	orig->npix0_x = rtex->surface.level[0].npix_x;
	orig->npix0_y = rtex->surface.level[0].npix_y;
	orig->npix_x = rtex->surface.level[level].npix_x;
	orig->npix_y = rtex->surface.level[level].npix_y;

	tex->format = format;
}

static void si_reset_blittable_to_orig(struct pipe_resource *tex,
				       unsigned level,
				       struct texture_orig_info *orig)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;

	tex->format = orig->format;
	tex->width0 = orig->width0;
	tex->height0 = orig->height0;
	rtex->surface.level[0].npix_x = orig->npix0_x;
	rtex->surface.level[0].npix_y = orig->npix0_y;
	rtex->surface.level[level].npix_x = orig->npix_x;
	rtex->surface.level[level].npix_y = orig->npix_y;
	rtex->mipmap_shift = 0;
}

void si_resource_copy_region(struct pipe_context *ctx,
			     struct pipe_resource *dst,
			     unsigned dst_level,
			     unsigned dstx, unsigned dsty, unsigned dstz,
			     struct pipe_resource *src,
			     unsigned src_level,
			     const struct pipe_box *src_box)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct r600_texture *rdst = (struct r600_texture*)dst;
	struct pipe_surface *dst_view, dst_templ;
	struct pipe_sampler_view src_templ, *src_view;
	struct texture_orig_info orig_info[2];
	struct pipe_box sbox, dstbox;
	boolean restore_orig[2];

	/* Fallback for buffers. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		si_copy_buffer(sctx, dst, src, dstx, src_box->x, src_box->width);
		return;
	}

	memset(orig_info, 0, sizeof(orig_info));

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	si_decompress_subresource(ctx, src, src_level,
				  src_box->z, src_box->z + src_box->depth - 1);

	restore_orig[0] = restore_orig[1] = FALSE;

	if (util_format_is_compressed(src->format) &&
	    util_format_is_compressed(dst->format)) {
		si_compressed_to_blittable(src, src_level, &orig_info[0]);
		restore_orig[0] = TRUE;
		sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
		sbox.y = util_format_get_nblocksy(orig_info[0].format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
		sbox.height = util_format_get_nblocksy(orig_info[0].format, src_box->height);
		sbox.depth = src_box->depth;
		src_box = &sbox;

		si_compressed_to_blittable(dst, dst_level, &orig_info[1]);
		restore_orig[1] = TRUE;
		/* translate the dst box as well */
		dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		dsty = util_format_get_nblocksy(orig_info[1].format, dsty);
	} else if (!util_blitter_is_copy_supported(sctx->blitter, dst, src)) {
		if (util_format_is_subsampled_422(src->format)) {
			/* XXX untested */
			si_change_format(src, src_level, &orig_info[0],
					 PIPE_FORMAT_R8G8B8A8_UINT);
			si_change_format(dst, dst_level, &orig_info[1],
					 PIPE_FORMAT_R8G8B8A8_UINT);

			sbox = *src_box;
			sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
			sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
			src_box = &sbox;
			dstx = util_format_get_nblocksx(orig_info[1].format, dstx);

			restore_orig[0] = TRUE;
			restore_orig[1] = TRUE;
		} else {
			unsigned blocksize = util_format_get_blocksize(src->format);

			switch (blocksize) {
			case 1:
				si_change_format(src, src_level, &orig_info[0],
						PIPE_FORMAT_R8_UNORM);
				si_change_format(dst, dst_level, &orig_info[1],
						PIPE_FORMAT_R8_UNORM);
				break;
			case 2:
				si_change_format(src, src_level, &orig_info[0],
						PIPE_FORMAT_R8G8_UNORM);
				si_change_format(dst, dst_level, &orig_info[1],
						PIPE_FORMAT_R8G8_UNORM);
				break;
			case 4:
				si_change_format(src, src_level, &orig_info[0],
						PIPE_FORMAT_R8G8B8A8_UNORM);
				si_change_format(dst, dst_level, &orig_info[1],
						PIPE_FORMAT_R8G8B8A8_UNORM);
				break;
			case 8:
				si_change_format(src, src_level, &orig_info[0],
						PIPE_FORMAT_R16G16B16A16_UINT);
				si_change_format(dst, dst_level, &orig_info[1],
						PIPE_FORMAT_R16G16B16A16_UINT);
				break;
			case 16:
				si_change_format(src, src_level, &orig_info[0],
						PIPE_FORMAT_R32G32B32A32_UINT);
				si_change_format(dst, dst_level, &orig_info[1],
						PIPE_FORMAT_R32G32B32A32_UINT);
				break;
			default:
				fprintf(stderr, "Unhandled format %s with blocksize %u\n",
					util_format_short_name(src->format), blocksize);
				assert(0);
			}
			restore_orig[0] = TRUE;
			restore_orig[1] = TRUE;
		}
	}

	/* Initialize the surface. */
	util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstz);
	dst_view = r600_create_surface_custom(ctx, dst, &dst_templ,
					      rdst->surface.level[dst_level].npix_x,
					      rdst->surface.level[dst_level].npix_y);

	/* Initialize the sampler view. */
	util_blitter_default_src_texture(&src_templ, src, src_level);
	src_view = ctx->create_sampler_view(ctx, src, &src_templ);

	u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height),
		 abs(src_box->depth), &dstbox);

	/* Copy. */
	si_blitter_begin(ctx, SI_COPY);
	util_blitter_blit_generic(sctx->blitter, dst_view, &dstbox,
				  src_view, src_box, src->width0, src->height0,
				  PIPE_MASK_RGBAZS, PIPE_TEX_FILTER_NEAREST, NULL);
	si_blitter_end(ctx);

	pipe_surface_reference(&dst_view, NULL);
	pipe_sampler_view_reference(&src_view, NULL);

	if (restore_orig[0])
		si_reset_blittable_to_orig(src, src_level, &orig_info[0]);

	if (restore_orig[1])
		si_reset_blittable_to_orig(dst, dst_level, &orig_info[1]);
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
	struct si_context *sctx = (struct si_context*)ctx;
	struct r600_texture *dst = (struct r600_texture*)info->dst.resource;
	unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
	unsigned dst_height = u_minify(info->dst.resource->height0, info->dst.level);
	enum pipe_format format = int_to_norm_format(info->dst.format);
	unsigned sample_mask = ~0;

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
	    !(dst->surface.flags & RADEON_SURF_SCANOUT) &&
	    (!dst->cmask.size || !dst->dirty_level_mask) /* dst cannot be fast-cleared */) {
		si_blitter_begin(ctx, SI_COLOR_RESOLVE |
				 (info->render_condition_enable ? 0 : SI_DISABLE_RENDER_COND));
		util_blitter_custom_resolve_color(sctx->blitter,
						  info->dst.resource, info->dst.level,
						  info->dst.box.z,
						  info->src.resource, info->src.box.z,
						  sample_mask, sctx->custom_blend_resolve,
						  format);
		si_blitter_end(ctx);
		return true;
	}
	return false;
}

static void si_blit(struct pipe_context *ctx,
		    const struct pipe_blit_info *info)
{
	struct si_context *sctx = (struct si_context*)ctx;

	if (do_hardware_msaa_resolve(ctx, info)) {
		return;
	}

	assert(util_blitter_is_blit_supported(sctx->blitter, info));

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	si_decompress_subresource(ctx, info->src.resource, info->src.level,
				  info->src.box.z,
				  info->src.box.z + info->src.box.depth - 1);

	if (sctx->screen->b.debug_flags & DBG_FORCE_DMA &&
	    util_try_blit_via_copy_region(ctx, info))
		return;

	si_blitter_begin(ctx, SI_BLIT |
			 (info->render_condition_enable ? 0 : SI_DISABLE_RENDER_COND));
	util_blitter_blit(sctx->blitter, info);
	si_blitter_end(ctx);
}

static void si_flush_resource(struct pipe_context *ctx,
			      struct pipe_resource *res)
{
	struct r600_texture *rtex = (struct r600_texture*)res;

	assert(res->target != PIPE_BUFFER);

	if (!rtex->is_depth && rtex->cmask.size) {
		si_blit_decompress_color(ctx, rtex, 0, res->last_level,
					 0, util_max_layer(res, 0));
	}
}

void si_init_blit_functions(struct si_context *sctx)
{
	sctx->b.b.clear = si_clear;
	sctx->b.b.clear_render_target = si_clear_render_target;
	sctx->b.b.clear_depth_stencil = si_clear_depth_stencil;
	sctx->b.b.resource_copy_region = si_resource_copy_region;
	sctx->b.b.blit = si_blit;
	sctx->b.b.flush_resource = si_flush_resource;
	sctx->b.blit_decompress_depth = si_blit_decompress_depth;
}
