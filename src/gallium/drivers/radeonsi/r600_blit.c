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
	util_blitter_save_so_targets(rctx->blitter, rctx->num_so_targets,
				     (struct pipe_stream_output_target**)rctx->so_targets);

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, rctx->ps_samplers.n_samplers,
			(void**)rctx->ps_samplers.samplers);

		util_blitter_save_fragment_sampler_views(
			rctx->blitter, rctx->ps_samplers.n_views,
			(struct pipe_sampler_view**)rctx->ps_samplers.views);
	}

	if ((op & R600_DISABLE_RENDER_COND) && rctx->current_render_cond) {
		rctx->saved_render_cond = rctx->current_render_cond;
		rctx->saved_render_cond_mode = rctx->current_render_cond_mode;
		rctx->context.render_condition(&rctx->context, NULL, 0);
	}

}

static void r600_blitter_end(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	if (rctx->saved_render_cond) {
		rctx->context.render_condition(&rctx->context,
					       rctx->saved_render_cond,
					       rctx->saved_render_cond_mode);
		rctx->saved_render_cond = NULL;
	}
	r600_context_queries_resume(rctx);
}

void si_blit_uncompress_depth(struct pipe_context *ctx,
		struct r600_resource_texture *texture,
		struct r600_resource_texture *staging,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, checked_last_layer, max_layer;
	float depth = 1.0f;
	const struct util_format_description *desc;
	void *custom_dsa;
	struct r600_resource_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;

	if (!staging && !texture->dirty_db_mask)
		return;

	desc = util_format_description(flushed_depth_texture->resource.b.b.format);
	switch (util_format_has_depth(desc) | util_format_has_stencil(desc) << 1) {
	default:
		assert(!"No depth or stencil to uncompress");
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
		if (!staging && !(texture->dirty_db_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = util_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *zsurf, *cbsurf, surf_tmpl;

			surf_tmpl.format = texture->real_format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;

			zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

			surf_tmpl.format = flushed_depth_texture->real_format;
			cbsurf = ctx->create_surface(ctx,
					(struct pipe_resource*)flushed_depth_texture, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, ~0, custom_dsa, depth);
			r600_blitter_end(ctx);

			pipe_surface_reference(&zsurf, NULL);
			pipe_surface_reference(&cbsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case can occur though. */
		if (!staging && first_layer == 0 && last_layer == max_layer) {
			texture->dirty_db_mask &= ~(1 << level);
		}
	}
}

static void si_blit_decompress_depth_in_place(struct r600_context *rctx,
                                              struct r600_resource_texture *texture,
                                              unsigned first_level, unsigned last_level,
                                              unsigned first_layer, unsigned last_layer)
{
	struct pipe_surface *zsurf, surf_tmpl = {{0}};
	unsigned layer, max_layer, checked_last_layer, level;

	surf_tmpl.format = texture->resource.b.b.format;

	for (level = first_level; level <= last_level; level++) {
		if (!(texture->dirty_db_mask & (1 << level)))
			continue;

		surf_tmpl.u.tex.level = level;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = util_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;

			zsurf = rctx->context.create_surface(&rctx->context, &texture->resource.b.b, &surf_tmpl);

			r600_blitter_begin(&rctx->context, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, NULL, ~0,
							  rctx->custom_dsa_flush_inplace,
							  1.0f);
			r600_blitter_end(&rctx->context);

			pipe_surface_reference(&zsurf, NULL);
		}

		/* The texture will always be dirty if some layers aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			texture->dirty_db_mask &= ~(1 << level);
		}
	}
}

void si_flush_depth_textures(struct r600_context *rctx,
			     struct r600_textures_info *textures)
{
	unsigned i;

	for (i = 0; i < textures->n_views; ++i) {
		struct pipe_sampler_view *view;
		struct r600_resource_texture *tex;

		view = &textures->views[i]->base;
		if (!view) continue;

		tex = (struct r600_resource_texture *)view->texture;
		if (!tex->is_depth || tex->is_flushing_texture)
			continue;

		si_blit_decompress_depth_in_place(rctx, tex,
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
			   fb->nr_cbufs, buffers, fb->nr_cbufs ? fb->cbufs[0]->format : PIPE_FORMAT_NONE,
			   color, depth, stencil);
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
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)tex;
	unsigned pixsize = util_format_get_blocksize(rtex->real_format);
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
}

static void r600_change_format(struct pipe_resource *tex,
			       unsigned level,
			       struct texture_orig_info *orig,
			       enum pipe_format format)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)tex;

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
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)tex;

	tex->format = orig->format;
	tex->width0 = orig->width0;
	tex->height0 = orig->height0;
	rtex->surface.level[0].npix_x = orig->npix0_x;
	rtex->surface.level[0].npix_y = orig->npix0_y;
	rtex->surface.level[level].npix_x = orig->npix_x;
	rtex->surface.level[level].npix_y = orig->npix_y;
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
	struct r600_resource_texture *rsrc = (struct r600_resource_texture*)src;
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

	/* This must be done before entering u_blitter to avoid recursion. */
	if (rsrc->is_depth && !rsrc->is_flushing_texture) {
		si_blit_decompress_depth_in_place(rctx, rsrc,
						  src_level, src_level,
						  src_box->z, src_box->z + src_box->depth - 1);
	}

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

static void si_blit(struct pipe_context *ctx,
                      const struct pipe_blit_info *info)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_resource_texture *rsrc = (struct r600_resource_texture*)info->src.resource;

	assert(util_blitter_is_blit_supported(rctx->blitter, info));

	if (info->src.resource->nr_samples > 1 &&
	    info->dst.resource->nr_samples <= 1 &&
	    !util_format_is_depth_or_stencil(info->src.resource->format) &&
	    !util_format_is_pure_integer(info->src.resource->format)) {
		debug_printf("radeonsi: color resolve is unimplemented\n");
		return;
	}

	if (rsrc->is_depth && !rsrc->is_flushing_texture) {
		si_blit_decompress_depth_in_place(rctx, rsrc,
						  info->src.level, info->src.level,
						  info->src.box.z,
						  info->src.box.z + info->src.box.depth - 1);
	}

	r600_blitter_begin(ctx, R600_BLIT);
	util_blitter_blit(rctx->blitter, info);
	r600_blitter_end(ctx);
}

void si_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
	rctx->context.blit = si_blit;
}
