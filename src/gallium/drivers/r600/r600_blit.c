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

	R600_DECOMPRESS    = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_suspend_nontimer_queries(rctx);

	util_blitter_save_vertex_buffers(rctx->blitter,
					 util_last_bit(rctx->vertex_buffer_state.enabled_mask),
					 rctx->vertex_buffer_state.vb);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_elements);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_so_targets(rctx->blitter, rctx->num_so_targets,
				     (struct pipe_stream_output_target**)rctx->so_targets);
	util_blitter_save_rasterizer(rctx->blitter, rctx->states[R600_PIPE_STATE_RASTERIZER]);

	if (op & R600_SAVE_FRAGMENT_STATE) {
		if (rctx->states[R600_PIPE_STATE_VIEWPORT]) {
			util_blitter_save_viewport(rctx->blitter, &rctx->viewport);
		}
		util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
		util_blitter_save_blend(rctx->blitter, rctx->states[R600_PIPE_STATE_BLEND]);
		util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->states[R600_PIPE_STATE_DSA]);
		if (rctx->states[R600_PIPE_STATE_STENCIL_REF]) {
			util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref);
		}
	}

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, rctx->ps_samplers.n_samplers,
			(void**)rctx->ps_samplers.samplers);

		util_blitter_save_fragment_sampler_views(
			rctx->blitter, util_last_bit(rctx->ps_samplers.views.enabled_mask),
			(struct pipe_sampler_view**)rctx->ps_samplers.views.views);
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
	r600_resume_nontimer_queries(rctx);
}

static unsigned u_max_layer(struct pipe_resource *r, unsigned level)
{
	switch (r->target) {
	case PIPE_TEXTURE_CUBE:
		return 6 - 1;
	case PIPE_TEXTURE_3D:
		return u_minify(r->depth0, level) - 1;
	case PIPE_TEXTURE_1D_ARRAY:
	case PIPE_TEXTURE_2D_ARRAY:
		return r->array_size - 1;
	default:
		return 0;
	}
}

void r600_blit_uncompress_depth(struct pipe_context *ctx,
		struct r600_resource_texture *texture,
		struct r600_resource_texture *staging,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, checked_last_layer, max_layer;
	float depth = 1.0f;
	struct r600_resource_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;

	if (!staging && !texture->dirty_db_mask)
		return;

	if (rctx->family == CHIP_RV610 || rctx->family == CHIP_RV630 ||
	    rctx->family == CHIP_RV620 || rctx->family == CHIP_RV635)
		depth = 0.0f;

	if (!rctx->db_misc_state.flush_depthstencil_through_cb) {
		/* Enable decompression in DB_RENDER_CONTROL */
		rctx->db_misc_state.flush_depthstencil_through_cb = true;
		r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
	}

	for (level = first_level; level <= last_level; level++) {
		if (!staging && !(texture->dirty_db_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = u_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *zsurf, *cbsurf, surf_tmpl;

			surf_tmpl.format = texture->real_format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			surf_tmpl.usage = PIPE_BIND_DEPTH_STENCIL;

			zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

			surf_tmpl.format = flushed_depth_texture->real_format;
			surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
			cbsurf = ctx->create_surface(ctx,
					(struct pipe_resource*)flushed_depth_texture, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, rctx->custom_dsa_flush, depth);
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

	/* reenable compression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_through_cb = false;
	r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
}

void r600_flush_depth_textures(struct r600_context *rctx,
			       struct r600_samplerview_state *textures)
{
	unsigned i;
	unsigned depth_texture_mask = textures->depth_texture_mask;

	while (depth_texture_mask) {
		struct pipe_sampler_view *view;
		struct r600_resource_texture *tex;

		i = u_bit_scan(&depth_texture_mask);

		view = &textures->views[i]->base;
		assert(view);

		tex = (struct r600_resource_texture *)view->texture;
		assert(tex->is_depth && !tex->is_flushing_texture);

		r600_blit_uncompress_depth(&rctx->context, tex, NULL,
					   view->u.tex.first_level,
					   view->u.tex.last_level,
					   0,
					   u_max_layer(&tex->resource.b.b, view->u.tex.first_level));
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

void r600_copy_buffer(struct pipe_context *ctx, struct
		      pipe_resource *dst, unsigned dstx,
		      struct pipe_resource *src, const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (rctx->screen->has_streamout &&
	    /* Require dword alignment. */
	    dstx % 4 == 0 && src_box->x % 4 == 0 && src_box->width % 4 == 0) {
		r600_blitter_begin(ctx, R600_COPY_BUFFER);
		util_blitter_copy_buffer(rctx->blitter, dst, dstx, src, src_box->x, src_box->width);
		r600_blitter_end(ctx);
	} else {
		util_resource_copy_region(ctx, dst, 0, dstx, 0, 0, src, 0, src_box);
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

static void r600_reset_blittable_to_compressed(struct pipe_resource *tex,
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
	const struct pipe_box *psbox;
	boolean restore_orig[2];

	memset(orig_info, 0, sizeof(orig_info));

	/* Handle buffers first. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		r600_copy_buffer(ctx, dst, dstx, src, src_box);
		return;
	}

	/* This must be done before entering u_blitter to avoid recursion. */
	if (rsrc->is_depth && !rsrc->is_flushing_texture) {
		if (!r600_init_flushed_depth_texture(ctx, src, NULL))
			return; /* error */

		r600_blit_uncompress_depth(ctx, rsrc, NULL,
					   src_level, src_level,
					   src_box->z, src_box->z + src_box->depth - 1);
	}

	restore_orig[0] = restore_orig[1] = FALSE;

	if (util_format_is_compressed(src->format)) {
		r600_compressed_to_blittable(src, src_level, &orig_info[0]);
		restore_orig[0] = TRUE;
		sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
		sbox.y = util_format_get_nblocksy(orig_info[0].format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
		sbox.height = util_format_get_nblocksy(orig_info[0].format, src_box->height);
		sbox.depth = src_box->depth;
		psbox=&sbox;
	} else
		psbox=src_box;

	if (util_format_is_compressed(dst->format)) {
		r600_compressed_to_blittable(dst, dst_level, &orig_info[1]);
		restore_orig[1] = TRUE;
		/* translate the dst box as well */
		dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		dsty = util_format_get_nblocksy(orig_info[1].format, dsty);
	}

	r600_blitter_begin(ctx, R600_COPY_TEXTURE);
	util_blitter_copy_texture(rctx->blitter, dst, dst_level, dstx, dsty, dstz,
				  src, src_level, psbox);
	r600_blitter_end(ctx);

	if (restore_orig[0])
		r600_reset_blittable_to_compressed(src, src_level, &orig_info[0]);

	if (restore_orig[1])
		r600_reset_blittable_to_compressed(dst, dst_level, &orig_info[1]);
}

void r600_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
}
