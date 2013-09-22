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
#include "util/u_surface.h"
#include "util/u_blitter.h"
#include "util/u_format.h"
#include "radeonsi_pipe.h"
#include "si_state.h"

enum r600_blitter_op /* bitmask */
{
	R600_SAVE_TEXTURES      = 1,
	R600_SAVE_FRAMEBUFFER   = 2,
	R600_DISABLE_RENDER_COND = 4,

	R600_CLEAR         = 0,

	R600_CLEAR_SURFACE = R600_SAVE_FRAMEBUFFER,

	R600_COPY          = R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_BLIT          = R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_DECOMPRESS    = R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,

	R600_COLOR_RESOLVE = R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_context_queries_suspend(rctx);

	util_blitter_save_blend(rctx->blitter, rctx->queued.named.blend);
	util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->queued.named.dsa);
	util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref);
	util_blitter_save_rasterizer(rctx->blitter, rctx->queued.named.rasterizer);
	util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_elements);
	if (rctx->queued.named.viewport) {
		util_blitter_save_viewport(rctx->blitter, &rctx->queued.named.viewport->viewport);
	}
	util_blitter_save_vertex_buffer_slot(rctx->blitter, rctx->vertex_buffer);
	util_blitter_save_so_targets(rctx->blitter, rctx->b.streamout.num_targets,
				     (struct pipe_stream_output_target**)rctx->b.streamout.targets);

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, rctx->samplers[PIPE_SHADER_FRAGMENT].n_samplers,
			(void**)rctx->samplers[PIPE_SHADER_FRAGMENT].samplers);

		util_blitter_save_fragment_sampler_views(rctx->blitter,
			util_last_bit(rctx->samplers[PIPE_SHADER_FRAGMENT].views.desc.enabled_mask &
				      ((1 << NUM_TEX_UNITS) - 1)),
			rctx->samplers[PIPE_SHADER_FRAGMENT].views.views);
	}

	if ((op & R600_DISABLE_RENDER_COND) && rctx->current_render_cond) {
		rctx->saved_render_cond = rctx->current_render_cond;
		rctx->saved_render_cond_cond = rctx->current_render_cond_cond;
		rctx->saved_render_cond_mode = rctx->current_render_cond_mode;
		rctx->b.b.render_condition(&rctx->b.b, NULL, FALSE, 0);
	}

}

static void r600_blitter_end(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	if (rctx->saved_render_cond) {
		rctx->b.b.render_condition(&rctx->b.b,
					       rctx->saved_render_cond,
					       rctx->saved_render_cond_cond,
					       rctx->saved_render_cond_mode);
		rctx->saved_render_cond = NULL;
	}
	r600_context_queries_resume(rctx);
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
	float depth = 1.0f;
	const struct util_format_description *desc;
	void **custom_dsa;
	struct r600_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;

	if (!staging && !texture->dirty_level_mask)
		return;

	max_sample = u_max_sample(&texture->resource.b.b);

	desc = util_format_description(flushed_depth_texture->resource.b.b.format);
	switch (util_format_has_depth(desc) | util_format_has_stencil(desc) << 1) {
	default:
		assert(!"No depth or stencil to uncompress");
		return;
	case 3:
		custom_dsa = rctx->custom_dsa_flush_depth_stencil;
		break;
	case 2:
		custom_dsa = rctx->custom_dsa_flush_stencil;
		break;
	case 1:
		custom_dsa = rctx->custom_dsa_flush_depth;
		break;
	}

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

				surf_tmpl.format = texture->resource.b.b.format;
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;

				zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

				surf_tmpl.format = flushed_depth_texture->resource.b.b.format;
				cbsurf = ctx->create_surface(ctx,
						(struct pipe_resource*)flushed_depth_texture, &surf_tmpl);

				r600_blitter_begin(ctx, R600_DECOMPRESS);
				util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, 1 << sample,
								  custom_dsa[sample], depth);
				r600_blitter_end(ctx);

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
}

static void si_blit_decompress_depth_in_place(struct r600_context *rctx,
                                              struct r600_texture *texture,
                                              unsigned first_level, unsigned last_level,
                                              unsigned first_layer, unsigned last_layer)
{
	struct pipe_surface *zsurf, surf_tmpl = {{0}};
	unsigned layer, max_layer, checked_last_layer, level;

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
							  rctx->custom_dsa_flush_inplace,
							  1.0f);
			r600_blitter_end(&rctx->b.b);

			pipe_surface_reference(&zsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}
}

void si_flush_depth_textures(struct r600_context *rctx,
			     struct r600_textures_info *textures)
{
	unsigned i;

	for (i = 0; i < textures->n_views; ++i) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		view = textures->views.views[i];
		if (!view) continue;

		tex = (struct r600_texture *)view->texture;
		if (!tex->is_depth || tex->is_flushing_texture)
			continue;

		si_blit_decompress_depth_in_place(rctx, tex,
						  view->u.tex.first_level, view->u.tex.last_level,
						  0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
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
						  rctx->custom_blend_decompress);
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
				    struct r600_textures_info *textures)
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

		r600_blit_decompress_color(&rctx->b.b, tex,
					   view->u.tex.first_level, view->u.tex.last_level,
					   0, util_max_layer(&tex->resource.b.b, view->u.tex.first_level));
	}
}

static void r600_clear(struct pipe_context *ctx, unsigned buffers,
		       const union pipe_color_union *color,
		       double depth, unsigned stencil)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_framebuffer_state *fb = &rctx->framebuffer;

	r600_blitter_begin(ctx, R600_CLEAR);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
			   buffers, color, depth, stencil);
	r600_blitter_end(ctx);
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

/* Helper for decompressing a portion of a color or depth resource before
 * blitting if any decompression is needed.
 * The driver doesn't decompress resources automatically while u_blitter is
 * rendering. */
static void r600_decompress_subresource(struct pipe_context *ctx,
					struct pipe_resource *tex,
					unsigned level,
					unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_texture *rtex = (struct r600_texture*)tex;

	if (rtex->is_depth && !rtex->is_flushing_texture) {
		si_blit_decompress_depth_in_place(rctx, rtex,
						  level, level,
						  first_layer, last_layer);
	} else if (rtex->fmask.size || rtex->cmask.size) {
		r600_blit_decompress_color(ctx, rtex, level, level,
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

static void r600_compressed_to_blittable(struct pipe_resource *tex,
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

static void r600_change_format(struct pipe_resource *tex,
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

static void r600_reset_blittable_to_orig(struct pipe_resource *tex,
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

static void r600_resource_copy_region(struct pipe_context *ctx,
				      struct pipe_resource *dst,
				      unsigned dst_level,
				      unsigned dstx, unsigned dsty, unsigned dstz,
				      struct pipe_resource *src,
				      unsigned src_level,
				      const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct texture_orig_info orig_info[2];
	struct pipe_box sbox;
	const struct pipe_box *psbox = src_box;
	boolean restore_orig[2];

	memset(orig_info, 0, sizeof(orig_info));

	/* Fallback for buffers. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		util_resource_copy_region(ctx, dst, dst_level, dstx, dsty, dstz,
                                          src, src_level, src_box);
		return;
	}

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	r600_decompress_subresource(ctx, src, src_level,
				    src_box->z, src_box->z + src_box->depth - 1);

	restore_orig[0] = restore_orig[1] = FALSE;

	if (util_format_is_compressed(src->format) &&
	    util_format_is_compressed(dst->format)) {
		r600_compressed_to_blittable(src, src_level, &orig_info[0]);
		restore_orig[0] = TRUE;
		sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
		sbox.y = util_format_get_nblocksy(orig_info[0].format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
		sbox.height = util_format_get_nblocksy(orig_info[0].format, src_box->height);
		sbox.depth = src_box->depth;
		psbox=&sbox;

		r600_compressed_to_blittable(dst, dst_level, &orig_info[1]);
		restore_orig[1] = TRUE;
		/* translate the dst box as well */
		dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		dsty = util_format_get_nblocksy(orig_info[1].format, dsty);
	} else if (!util_blitter_is_copy_supported(rctx->blitter, dst, src,
						   PIPE_MASK_RGBAZS)) {
		unsigned blocksize = util_format_get_blocksize(src->format);

		switch (blocksize) {
		case 1:
			r600_change_format(src, src_level, &orig_info[0],
					   PIPE_FORMAT_R8_UNORM);
			r600_change_format(dst, dst_level, &orig_info[1],
					   PIPE_FORMAT_R8_UNORM);
			break;
		case 2:
			r600_change_format(src, src_level, &orig_info[0],
					   PIPE_FORMAT_R8G8_UNORM);
			r600_change_format(dst, dst_level, &orig_info[1],
					   PIPE_FORMAT_R8G8_UNORM);
			break;
		case 4:
			r600_change_format(src, src_level, &orig_info[0],
					   PIPE_FORMAT_R8G8B8A8_UNORM);
			r600_change_format(dst, dst_level, &orig_info[1],
					   PIPE_FORMAT_R8G8B8A8_UNORM);
			break;
		case 8:
			r600_change_format(src, src_level, &orig_info[0],
					   PIPE_FORMAT_R16G16B16A16_UINT);
			r600_change_format(dst, dst_level, &orig_info[1],
					   PIPE_FORMAT_R16G16B16A16_UINT);
			break;
		case 16:
			r600_change_format(src, src_level, &orig_info[0],
					   PIPE_FORMAT_R32G32B32A32_UINT);
			r600_change_format(dst, dst_level, &orig_info[1],
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

	r600_blitter_begin(ctx, R600_COPY);
	util_blitter_copy_texture(rctx->blitter, dst, dst_level, dstx, dsty, dstz,
				  src, src_level, psbox, PIPE_MASK_RGBAZS, TRUE);
	r600_blitter_end(ctx);

	if (restore_orig[0])
		r600_reset_blittable_to_orig(src, src_level, &orig_info[0]);

	if (restore_orig[1])
		r600_reset_blittable_to_orig(dst, dst_level, &orig_info[1]);
}

static boolean is_simple_msaa_resolve(const struct pipe_blit_info *info)
{
	unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
	unsigned dst_height = u_minify(info->dst.resource->height0, info->dst.level);
	struct r600_texture *dst = (struct r600_texture*)info->dst.resource;
	unsigned dst_tile_mode = dst->surface.level[info->dst.level].mode;
	bool dst_is_scanout = (dst->surface.flags & RADEON_SURF_SCANOUT) != 0;

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
		dst_tile_mode >= RADEON_SURF_MODE_1D &&
		!dst_is_scanout;
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

static void si_msaa_color_resolve(struct pipe_context *ctx,
				  const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_screen *screen = ctx->screen;
	struct pipe_resource *tmp, templ;
	struct pipe_blit_info blit;
	unsigned sample_mask = ~0;

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

static void si_blit(struct pipe_context *ctx,
                      const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (info->src.resource->nr_samples > 1 &&
	    info->dst.resource->nr_samples <= 1 &&
	    !util_format_is_depth_or_stencil(info->src.resource->format) &&
	    !util_format_is_pure_integer(info->src.resource->format)) {
		si_msaa_color_resolve(ctx, info);
		return;
	}

	assert(util_blitter_is_blit_supported(rctx->blitter, info));

	/* The driver doesn't decompress resources automatically while
	 * u_blitter is rendering. */
	r600_decompress_subresource(ctx, info->src.resource, info->src.level,
				    info->src.box.z,
				    info->src.box.z + info->src.box.depth - 1);

	r600_blitter_begin(ctx, R600_BLIT);
	util_blitter_blit(rctx->blitter, info);
	r600_blitter_end(ctx);
}

static void si_flush_resource(struct pipe_context *ctx,
			      struct pipe_resource *resource)
{
}

void si_init_blit_functions(struct r600_context *rctx)
{
	rctx->b.b.clear = r600_clear;
	rctx->b.b.clear_render_target = r600_clear_render_target;
	rctx->b.b.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->b.b.resource_copy_region = r600_resource_copy_region;
	rctx->b.b.blit = si_blit;
	rctx->b.b.flush_resource = si_flush_resource;
	rctx->b.blit_decompress_depth = r600_blit_decompress_depth;
}
