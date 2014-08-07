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
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include "r600_pipe_common.h"
#include "r600_cs.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include <errno.h>
#include <inttypes.h>

/* Same as resource_copy_region, except that both upsampling and downsampling are allowed. */
static void r600_copy_region_with_blit(struct pipe_context *pipe,
				       struct pipe_resource *dst,
                                       unsigned dst_level,
                                       unsigned dstx, unsigned dsty, unsigned dstz,
                                       struct pipe_resource *src,
                                       unsigned src_level,
                                       const struct pipe_box *src_box)
{
	struct pipe_blit_info blit;

	memset(&blit, 0, sizeof(blit));
	blit.src.resource = src;
	blit.src.format = src->format;
	blit.src.level = src_level;
	blit.src.box = *src_box;
	blit.dst.resource = dst;
	blit.dst.format = dst->format;
	blit.dst.level = dst_level;
	blit.dst.box.x = dstx;
	blit.dst.box.y = dsty;
	blit.dst.box.z = dstz;
	blit.dst.box.width = src_box->width;
	blit.dst.box.height = src_box->height;
	blit.dst.box.depth = src_box->depth;
	blit.mask = util_format_get_mask(src->format) &
		    util_format_get_mask(dst->format);
	blit.filter = PIPE_TEX_FILTER_NEAREST;

	if (blit.mask) {
		pipe->blit(pipe, &blit);
	}
}

/* Copy from a full GPU texture to a transfer's staging one. */
static void r600_copy_to_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *dst = &rtransfer->staging->b.b;
	struct pipe_resource *src = transfer->resource;

	if (src->nr_samples > 1) {
		r600_copy_region_with_blit(ctx, dst, 0, 0, 0, 0,
					   src, transfer->level, &transfer->box);
		return;
	}

	rctx->dma_copy(ctx, dst, 0, 0, 0, 0, src, transfer->level,
		       &transfer->box);
}

/* Copy from a transfer's staging texture to a full GPU one. */
static void r600_copy_from_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *dst = transfer->resource;
	struct pipe_resource *src = &rtransfer->staging->b.b;
	struct pipe_box sbox;

	u_box_3d(0, 0, 0, transfer->box.width, transfer->box.height, transfer->box.depth, &sbox);

	if (dst->nr_samples > 1) {
		r600_copy_region_with_blit(ctx, dst, transfer->level,
					   transfer->box.x, transfer->box.y, transfer->box.z,
					   src, 0, &sbox);
		return;
	}

	rctx->dma_copy(ctx, dst, transfer->level,
		       transfer->box.x, transfer->box.y, transfer->box.z,
		       src, 0, &sbox);
}

static unsigned r600_texture_get_offset(struct r600_texture *rtex, unsigned level,
					const struct pipe_box *box)
{
	enum pipe_format format = rtex->resource.b.b.format;

	return rtex->surface.level[level].offset +
	       box->z * rtex->surface.level[level].slice_size +
	       box->y / util_format_get_blockheight(format) * rtex->surface.level[level].pitch_bytes +
	       box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
}

static int r600_init_surface(struct r600_common_screen *rscreen,
			     struct radeon_surface *surface,
			     const struct pipe_resource *ptex,
			     unsigned array_mode,
			     bool is_flushed_depth)
{
	const struct util_format_description *desc =
		util_format_description(ptex->format);
	bool is_depth, is_stencil;

	is_depth = util_format_has_depth(desc);
	is_stencil = util_format_has_stencil(desc);

	surface->npix_x = ptex->width0;
	surface->npix_y = ptex->height0;
	surface->npix_z = ptex->depth0;
	surface->blk_w = util_format_get_blockwidth(ptex->format);
	surface->blk_h = util_format_get_blockheight(ptex->format);
	surface->blk_d = 1;
	surface->array_size = 1;
	surface->last_level = ptex->last_level;

	if (rscreen->chip_class >= EVERGREEN && !is_flushed_depth &&
	    ptex->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
		surface->bpe = 4; /* stencil is allocated separately on evergreen */
	} else {
		surface->bpe = util_format_get_blocksize(ptex->format);
		/* align byte per element on dword */
		if (surface->bpe == 3) {
			surface->bpe = 4;
		}
	}

	surface->nsamples = ptex->nr_samples ? ptex->nr_samples : 1;
	surface->flags = RADEON_SURF_SET(array_mode, MODE);

	switch (ptex->target) {
	case PIPE_TEXTURE_1D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D, TYPE);
		break;
	case PIPE_TEXTURE_RECT:
	case PIPE_TEXTURE_2D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D, TYPE);
		break;
	case PIPE_TEXTURE_3D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_3D, TYPE);
		break;
	case PIPE_TEXTURE_1D_ARRAY:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D_ARRAY, TYPE);
		surface->array_size = ptex->array_size;
		break;
	case PIPE_TEXTURE_2D_ARRAY:
	case PIPE_TEXTURE_CUBE_ARRAY: /* cube array layout like 2d array */
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D_ARRAY, TYPE);
		surface->array_size = ptex->array_size;
		break;
	case PIPE_TEXTURE_CUBE:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_CUBEMAP, TYPE);
		break;
	case PIPE_BUFFER:
	default:
		return -EINVAL;
	}
	if (ptex->bind & PIPE_BIND_SCANOUT) {
		surface->flags |= RADEON_SURF_SCANOUT;
	}

	if (!is_flushed_depth && is_depth) {
		surface->flags |= RADEON_SURF_ZBUFFER;

		if (is_stencil) {
			surface->flags |= RADEON_SURF_SBUFFER |
					  RADEON_SURF_HAS_SBUFFER_MIPTREE;
		}
	}
	if (rscreen->chip_class >= SI) {
		surface->flags |= RADEON_SURF_HAS_TILE_MODE_INDEX;
	}
	return 0;
}

static int r600_setup_surface(struct pipe_screen *screen,
			      struct r600_texture *rtex,
			      unsigned pitch_in_bytes_override)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	int r;

	r = rscreen->ws->surface_init(rscreen->ws, &rtex->surface);
	if (r) {
		return r;
	}

	rtex->size = rtex->surface.bo_size;

	if (pitch_in_bytes_override && pitch_in_bytes_override != rtex->surface.level[0].pitch_bytes) {
		/* old ddx on evergreen over estimate alignment for 1d, only 1 level
		 * for those
		 */
		rtex->surface.level[0].nblk_x = pitch_in_bytes_override / rtex->surface.bpe;
		rtex->surface.level[0].pitch_bytes = pitch_in_bytes_override;
		rtex->surface.level[0].slice_size = pitch_in_bytes_override * rtex->surface.level[0].nblk_y;
		if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
			rtex->surface.stencil_offset =
			rtex->surface.stencil_level[0].offset = rtex->surface.level[0].slice_size;
		}
	}
	return 0;
}

static boolean r600_texture_get_handle(struct pipe_screen* screen,
				       struct pipe_resource *ptex,
				       struct winsys_handle *whandle)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;
	struct radeon_surface *surface = &rtex->surface;
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;

	rscreen->ws->buffer_set_tiling(resource->buf,
				       NULL,
				       surface->level[0].mode >= RADEON_SURF_MODE_1D ?
				       RADEON_LAYOUT_TILED : RADEON_LAYOUT_LINEAR,
				       surface->level[0].mode >= RADEON_SURF_MODE_2D ?
				       RADEON_LAYOUT_TILED : RADEON_LAYOUT_LINEAR,
				       surface->bankw, surface->bankh,
				       surface->tile_split,
				       surface->stencil_tile_split,
				       surface->mtilea,
				       surface->level[0].pitch_bytes,
				       (surface->flags & RADEON_SURF_SCANOUT) != 0);

	return rscreen->ws->buffer_get_handle(resource->buf,
						surface->level[0].pitch_bytes, whandle);
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;

	if (rtex->flushed_depth_texture)
		pipe_resource_reference((struct pipe_resource **)&rtex->flushed_depth_texture, NULL);

	pipe_resource_reference((struct pipe_resource**)&rtex->htile_buffer, NULL);
	if (rtex->cmask_buffer != &rtex->resource) {
	    pipe_resource_reference((struct pipe_resource**)&rtex->cmask_buffer, NULL);
	}
	pb_reference(&resource->buf, NULL);
	FREE(rtex);
}

static const struct u_resource_vtbl r600_texture_vtbl;

/* The number of samples can be specified independently of the texture. */
void r600_texture_get_fmask_info(struct r600_common_screen *rscreen,
				 struct r600_texture *rtex,
				 unsigned nr_samples,
				 struct r600_fmask_info *out)
{
	/* FMASK is allocated like an ordinary texture. */
	struct radeon_surface fmask = rtex->surface;

	memset(out, 0, sizeof(*out));

	fmask.bo_alignment = 0;
	fmask.bo_size = 0;
	fmask.nsamples = 1;
	fmask.flags |= RADEON_SURF_FMASK;

	/* Force 2D tiling if it wasn't set. This may occur when creating
	 * FMASK for MSAA resolve on R6xx. On R6xx, the single-sample
	 * destination buffer must have an FMASK too. */
	fmask.flags = RADEON_SURF_CLR(fmask.flags, MODE);
	fmask.flags |= RADEON_SURF_SET(RADEON_SURF_MODE_2D, MODE);

	if (rscreen->chip_class >= SI) {
		fmask.flags |= RADEON_SURF_HAS_TILE_MODE_INDEX;
	}

	switch (nr_samples) {
	case 2:
	case 4:
		fmask.bpe = 1;
		if (rscreen->chip_class <= CAYMAN) {
			fmask.bankh = 4;
		}
		break;
	case 8:
		fmask.bpe = 4;
		break;
	default:
		R600_ERR("Invalid sample count for FMASK allocation.\n");
		return;
	}

	/* Overallocate FMASK on R600-R700 to fix colorbuffer corruption.
	 * This can be fixed by writing a separate FMASK allocator specifically
	 * for R600-R700 asics. */
	if (rscreen->chip_class <= R700) {
		fmask.bpe *= 2;
	}

	if (rscreen->ws->surface_init(rscreen->ws, &fmask)) {
		R600_ERR("Got error in surface_init while allocating FMASK.\n");
		return;
	}

	assert(fmask.level[0].mode == RADEON_SURF_MODE_2D);

	out->slice_tile_max = (fmask.level[0].nblk_x * fmask.level[0].nblk_y) / 64;
	if (out->slice_tile_max)
		out->slice_tile_max -= 1;

	out->tile_mode_index = fmask.tiling_index[0];
	out->pitch = fmask.level[0].nblk_x;
	out->bank_height = fmask.bankh;
	out->alignment = MAX2(256, fmask.bo_alignment);
	out->size = fmask.bo_size;
}

static void r600_texture_allocate_fmask(struct r600_common_screen *rscreen,
					struct r600_texture *rtex)
{
	r600_texture_get_fmask_info(rscreen, rtex,
				    rtex->resource.b.b.nr_samples, &rtex->fmask);

	rtex->fmask.offset = align(rtex->size, rtex->fmask.alignment);
	rtex->size = rtex->fmask.offset + rtex->fmask.size;
}

void r600_texture_get_cmask_info(struct r600_common_screen *rscreen,
				 struct r600_texture *rtex,
				 struct r600_cmask_info *out)
{
	unsigned cmask_tile_width = 8;
	unsigned cmask_tile_height = 8;
	unsigned cmask_tile_elements = cmask_tile_width * cmask_tile_height;
	unsigned element_bits = 4;
	unsigned cmask_cache_bits = 1024;
	unsigned num_pipes = rscreen->tiling_info.num_channels;
	unsigned pipe_interleave_bytes = rscreen->tiling_info.group_bytes;

	unsigned elements_per_macro_tile = (cmask_cache_bits / element_bits) * num_pipes;
	unsigned pixels_per_macro_tile = elements_per_macro_tile * cmask_tile_elements;
	unsigned sqrt_pixels_per_macro_tile = sqrt(pixels_per_macro_tile);
	unsigned macro_tile_width = util_next_power_of_two(sqrt_pixels_per_macro_tile);
	unsigned macro_tile_height = pixels_per_macro_tile / macro_tile_width;

	unsigned pitch_elements = align(rtex->surface.npix_x, macro_tile_width);
	unsigned height = align(rtex->surface.npix_y, macro_tile_height);

	unsigned base_align = num_pipes * pipe_interleave_bytes;
	unsigned slice_bytes =
		((pitch_elements * height * element_bits + 7) / 8) / cmask_tile_elements;

	assert(macro_tile_width % 128 == 0);
	assert(macro_tile_height % 128 == 0);

	out->slice_tile_max = ((pitch_elements * height) / (128*128)) - 1;
	out->alignment = MAX2(256, base_align);
	out->size = (util_max_layer(&rtex->resource.b.b, 0) + 1) *
		    align(slice_bytes, base_align);
}

static void si_texture_get_cmask_info(struct r600_common_screen *rscreen,
				      struct r600_texture *rtex,
				      struct r600_cmask_info *out)
{
	unsigned pipe_interleave_bytes = rscreen->tiling_info.group_bytes;
	unsigned num_pipes = rscreen->tiling_info.num_channels;
	unsigned cl_width, cl_height;

	switch (num_pipes) {
	case 2:
		cl_width = 32;
		cl_height = 16;
		break;
	case 4:
		cl_width = 32;
		cl_height = 32;
		break;
	case 8:
		cl_width = 64;
		cl_height = 32;
		break;
	case 16: /* Hawaii */
		cl_width = 64;
		cl_height = 64;
		break;
	default:
		assert(0);
		return;
	}

	unsigned base_align = num_pipes * pipe_interleave_bytes;

	unsigned width = align(rtex->surface.npix_x, cl_width*8);
	unsigned height = align(rtex->surface.npix_y, cl_height*8);
	unsigned slice_elements = (width * height) / (8*8);

	/* Each element of CMASK is a nibble. */
	unsigned slice_bytes = slice_elements / 2;

	out->slice_tile_max = (width * height) / (128*128);
	if (out->slice_tile_max)
		out->slice_tile_max -= 1;

	out->alignment = MAX2(256, base_align);
	out->size = (util_max_layer(&rtex->resource.b.b, 0) + 1) *
		    align(slice_bytes, base_align);
}

static void r600_texture_allocate_cmask(struct r600_common_screen *rscreen,
					struct r600_texture *rtex)
{
	if (rscreen->chip_class >= SI) {
		si_texture_get_cmask_info(rscreen, rtex, &rtex->cmask);
	} else {
		r600_texture_get_cmask_info(rscreen, rtex, &rtex->cmask);
	}

	rtex->cmask.offset = align(rtex->size, rtex->cmask.alignment);
	rtex->size = rtex->cmask.offset + rtex->cmask.size;

	if (rscreen->chip_class >= SI)
		rtex->cb_color_info |= SI_S_028C70_FAST_CLEAR(1);
	else
		rtex->cb_color_info |= EG_S_028C70_FAST_CLEAR(1);
}

static void r600_texture_alloc_cmask_separate(struct r600_common_screen *rscreen,
					      struct r600_texture *rtex)
{
	if (rtex->cmask_buffer)
                return;

	assert(rtex->cmask.size == 0);

	if (rscreen->chip_class >= SI) {
		si_texture_get_cmask_info(rscreen, rtex, &rtex->cmask);
	} else {
		r600_texture_get_cmask_info(rscreen, rtex, &rtex->cmask);
	}

	rtex->cmask_buffer = (struct r600_resource *)
		pipe_buffer_create(&rscreen->b, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_DEFAULT, rtex->cmask.size);
	if (rtex->cmask_buffer == NULL) {
		rtex->cmask.size = 0;
		return;
	}

	/* update colorbuffer state bits */
	rtex->cmask.base_address_reg = rtex->cmask_buffer->gpu_address >> 8;

	if (rscreen->chip_class >= SI)
		rtex->cb_color_info |= SI_S_028C70_FAST_CLEAR(1);
	else
		rtex->cb_color_info |= EG_S_028C70_FAST_CLEAR(1);
}

static unsigned si_texture_htile_alloc_size(struct r600_common_screen *rscreen,
					    struct r600_texture *rtex)
{
	unsigned cl_width, cl_height, width, height;
	unsigned slice_elements, slice_bytes, pipe_interleave_bytes, base_align;
	unsigned num_pipes = rscreen->tiling_info.num_channels;

	/* HTILE is broken with 1D tiling on old kernels and CIK. */
	if (rtex->surface.level[0].mode == RADEON_SURF_MODE_1D &&
	    rscreen->chip_class >= CIK && rscreen->info.drm_minor < 38)
		return 0;

	switch (num_pipes) {
	case 2:
		cl_width = 32;
		cl_height = 32;
		break;
	case 4:
		cl_width = 64;
		cl_height = 32;
		break;
	case 8:
		cl_width = 64;
		cl_height = 64;
		break;
	case 16:
		cl_width = 128;
		cl_height = 64;
		break;
	default:
		assert(0);
		return 0;
	}

	width = align(rtex->surface.npix_x, cl_width * 8);
	height = align(rtex->surface.npix_y, cl_height * 8);

	slice_elements = (width * height) / (8 * 8);
	slice_bytes = slice_elements * 4;

	pipe_interleave_bytes = rscreen->tiling_info.group_bytes;
	base_align = num_pipes * pipe_interleave_bytes;

	return (util_max_layer(&rtex->resource.b.b, 0) + 1) *
		align(slice_bytes, base_align);
}

static unsigned r600_texture_htile_alloc_size(struct r600_common_screen *rscreen,
					      struct r600_texture *rtex)
{
	unsigned sw = rtex->surface.level[0].nblk_x * rtex->surface.blk_w;
	unsigned sh = rtex->surface.level[0].nblk_y * rtex->surface.blk_h;
	unsigned npipes = rscreen->info.r600_num_tile_pipes;
	unsigned htile_size;

	/* XXX also use it for other texture targets */
	if (rscreen->info.drm_minor < 26 ||
	    rtex->resource.b.b.target != PIPE_TEXTURE_2D ||
	    rtex->surface.level[0].nblk_x < 32 ||
	    rtex->surface.level[0].nblk_y < 32) {
		return 0;
	}

	/* HW bug on R6xx. */
	if (rscreen->chip_class == R600 &&
	    (rtex->surface.level[0].npix_x > 7680 ||
	     rtex->surface.level[0].npix_y > 7680))
		return 0;

	/* this alignment and htile size only apply to linear htile buffer */
	sw = align(sw, 16 << 3);
	sh = align(sh, npipes << 3);
	htile_size = (sw >> 3) * (sh >> 3) * 4;
	/* must be aligned with 2K * npipes */
	htile_size = align(htile_size, (2 << 10) * npipes);
	return htile_size;
}

static void r600_texture_allocate_htile(struct r600_common_screen *rscreen,
					struct r600_texture *rtex)
{
	unsigned htile_size;
	if (rscreen->chip_class >= SI) {
		htile_size = si_texture_htile_alloc_size(rscreen, rtex);
	} else {
		htile_size = r600_texture_htile_alloc_size(rscreen, rtex);
	}

	if (!htile_size)
		return;

	/* XXX don't allocate it separately */
	rtex->htile_buffer = (struct r600_resource*)
			     pipe_buffer_create(&rscreen->b, PIPE_BIND_CUSTOM,
						PIPE_USAGE_DEFAULT, htile_size);
	if (rtex->htile_buffer == NULL) {
		/* this is not a fatal error as we can still keep rendering
		 * without htile buffer */
		R600_ERR("Failed to create buffer object for htile buffer.\n");
	} else {
		r600_screen_clear_buffer(rscreen, &rtex->htile_buffer->b.b, 0, htile_size, 0);
	}
}

/* Common processing for r600_texture_create and r600_texture_from_handle */
static struct r600_texture *
r600_texture_create_object(struct pipe_screen *screen,
			   const struct pipe_resource *base,
			   unsigned pitch_in_bytes_override,
			   struct pb_buffer *buf,
			   struct radeon_surface *surface)
{
	struct r600_texture *rtex;
	struct r600_resource *resource;
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;

	rtex = CALLOC_STRUCT(r600_texture);
	if (rtex == NULL)
		return NULL;

	resource = &rtex->resource;
	resource->b.b = *base;
	resource->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->b.b.reference, 1);
	resource->b.b.screen = screen;
	rtex->pitch_override = pitch_in_bytes_override;

	/* don't include stencil-only formats which we don't support for rendering */
	rtex->is_depth = util_format_has_depth(util_format_description(rtex->resource.b.b.format));

	rtex->surface = *surface;
	if (r600_setup_surface(screen, rtex, pitch_in_bytes_override)) {
		FREE(rtex);
		return NULL;
	}

	/* Tiled depth textures utilize the non-displayable tile order.
	 * This must be done after r600_setup_surface.
	 * Applies to R600-Cayman. */
	rtex->non_disp_tiling = rtex->is_depth && rtex->surface.level[0].mode >= RADEON_SURF_MODE_1D;

	if (rtex->is_depth) {
		if (!(base->flags & (R600_RESOURCE_FLAG_TRANSFER |
				     R600_RESOURCE_FLAG_FLUSHED_DEPTH)) &&
		    (rscreen->debug_flags & DBG_HYPERZ)) {

			r600_texture_allocate_htile(rscreen, rtex);
		}
	} else {
		if (base->nr_samples > 1) {
			if (!buf) {
				r600_texture_allocate_fmask(rscreen, rtex);
				r600_texture_allocate_cmask(rscreen, rtex);
				rtex->cmask_buffer = &rtex->resource;
			}
			if (!rtex->fmask.size || !rtex->cmask.size) {
				FREE(rtex);
				return NULL;
			}
		}
	}

	/* Now create the backing buffer. */
	if (!buf) {
		if (!r600_init_resource(rscreen, resource, rtex->size,
					rtex->surface.bo_alignment, TRUE)) {
			FREE(rtex);
			return NULL;
		}
	} else {
		resource->buf = buf;
		resource->cs_buf = rscreen->ws->buffer_get_cs_handle(buf);
		resource->gpu_address = rscreen->ws->buffer_get_virtual_address(resource->cs_buf);
		resource->domains = rscreen->ws->buffer_get_initial_domain(resource->cs_buf);
	}

	if (rtex->cmask.size) {
		/* Initialize the cmask to 0xCC (= compressed state). */
		r600_screen_clear_buffer(rscreen, &rtex->cmask_buffer->b.b,
					 rtex->cmask.offset, rtex->cmask.size, 0xCCCCCCCC);
	}

	/* Initialize the CMASK base register value. */
	rtex->cmask.base_address_reg =
		(rtex->resource.gpu_address + rtex->cmask.offset) >> 8;

	if (rscreen->debug_flags & DBG_VM) {
		fprintf(stderr, "VM start=0x%"PRIX64"  end=0x%"PRIX64" | Texture %ix%ix%i, %i levels, %i samples, %s\n",
			rtex->resource.gpu_address,
			rtex->resource.gpu_address + rtex->resource.buf->size,
			base->width0, base->height0, util_max_layer(base, 0)+1, base->last_level+1,
			base->nr_samples ? base->nr_samples : 1, util_format_short_name(base->format));
	}

	if (rscreen->debug_flags & DBG_TEX ||
	    (rtex->resource.b.b.last_level > 0 && rscreen->debug_flags & DBG_TEXMIP)) {
		printf("Texture: npix_x=%u, npix_y=%u, npix_z=%u, blk_w=%u, "
		       "blk_h=%u, blk_d=%u, array_size=%u, last_level=%u, "
		       "bpe=%u, nsamples=%u, flags=0x%x, %s\n",
		       rtex->surface.npix_x, rtex->surface.npix_y,
		       rtex->surface.npix_z, rtex->surface.blk_w,
		       rtex->surface.blk_h, rtex->surface.blk_d,
		       rtex->surface.array_size, rtex->surface.last_level,
		       rtex->surface.bpe, rtex->surface.nsamples,
		       rtex->surface.flags, util_format_short_name(base->format));
		for (int i = 0; i <= rtex->surface.last_level; i++) {
			printf("  L %i: offset=%"PRIu64", slice_size=%"PRIu64", npix_x=%u, "
			       "npix_y=%u, npix_z=%u, nblk_x=%u, nblk_y=%u, "
			       "nblk_z=%u, pitch_bytes=%u, mode=%u\n",
			       i, rtex->surface.level[i].offset,
			       rtex->surface.level[i].slice_size,
			       u_minify(rtex->resource.b.b.width0, i),
			       u_minify(rtex->resource.b.b.height0, i),
			       u_minify(rtex->resource.b.b.depth0, i),
			       rtex->surface.level[i].nblk_x,
			       rtex->surface.level[i].nblk_y,
			       rtex->surface.level[i].nblk_z,
			       rtex->surface.level[i].pitch_bytes,
			       rtex->surface.level[i].mode);
		}
		if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
			for (int i = 0; i <= rtex->surface.last_level; i++) {
				printf("  S %i: offset=%"PRIu64", slice_size=%"PRIu64", npix_x=%u, "
				       "npix_y=%u, npix_z=%u, nblk_x=%u, nblk_y=%u, "
				       "nblk_z=%u, pitch_bytes=%u, mode=%u\n",
				       i, rtex->surface.stencil_level[i].offset,
				       rtex->surface.stencil_level[i].slice_size,
				       u_minify(rtex->resource.b.b.width0, i),
				       u_minify(rtex->resource.b.b.height0, i),
				       u_minify(rtex->resource.b.b.depth0, i),
				       rtex->surface.stencil_level[i].nblk_x,
				       rtex->surface.stencil_level[i].nblk_y,
				       rtex->surface.stencil_level[i].nblk_z,
				       rtex->surface.stencil_level[i].pitch_bytes,
				       rtex->surface.stencil_level[i].mode);
			}
		}
	}
	return rtex;
}

static unsigned r600_choose_tiling(struct r600_common_screen *rscreen,
				   const struct pipe_resource *templ)
{
	const struct util_format_description *desc = util_format_description(templ->format);

	/* MSAA resources must be 2D tiled. */
	if (templ->nr_samples > 1)
		return RADEON_SURF_MODE_2D;

	/* Transfer resources should be linear. */
	if (templ->flags & R600_RESOURCE_FLAG_TRANSFER)
		return RADEON_SURF_MODE_LINEAR_ALIGNED;

	/* Handle common candidates for the linear mode.
	 * Compressed textures must always be tiled. */
	if (!(templ->flags & R600_RESOURCE_FLAG_FORCE_TILING) &&
	    !util_format_is_compressed(templ->format)) {
		/* Not everything can be linear, so we cannot enforce it
		 * for all textures. */
		if ((rscreen->debug_flags & DBG_NO_TILING) &&
		    (!util_format_is_depth_or_stencil(templ->format) ||
		     !(templ->flags & R600_RESOURCE_FLAG_FLUSHED_DEPTH)))
			return RADEON_SURF_MODE_LINEAR_ALIGNED;

		/* Tiling doesn't work with the 422 (SUBSAMPLED) formats on R600+. */
		if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
			return RADEON_SURF_MODE_LINEAR_ALIGNED;

		/* Cursors are linear on SI.
		 * (XXX double-check, maybe also use RADEON_SURF_SCANOUT) */
		if (rscreen->chip_class >= SI &&
		    (templ->bind & PIPE_BIND_CURSOR))
			return RADEON_SURF_MODE_LINEAR_ALIGNED;

		if (templ->bind & PIPE_BIND_LINEAR)
			return RADEON_SURF_MODE_LINEAR_ALIGNED;

		/* Textures with a very small height are recommended to be linear. */
		if (templ->target == PIPE_TEXTURE_1D ||
		    templ->target == PIPE_TEXTURE_1D_ARRAY ||
		    templ->height0 <= 4)
			return RADEON_SURF_MODE_LINEAR_ALIGNED;

		/* Textures likely to be mapped often. */
		if (templ->usage == PIPE_USAGE_STAGING ||
		    templ->usage == PIPE_USAGE_STREAM)
			return RADEON_SURF_MODE_LINEAR_ALIGNED;
	}

	/* Make small textures 1D tiled. */
	if (templ->width0 <= 16 || templ->height0 <= 16 ||
	    (rscreen->debug_flags & DBG_NO_2D_TILING))
		return RADEON_SURF_MODE_1D;

	/* The allocator will switch to 1D if needed. */
	return RADEON_SURF_MODE_2D;
}

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					  const struct pipe_resource *templ)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	struct radeon_surface surface = {0};
	int r;

	r = r600_init_surface(rscreen, &surface, templ,
			      r600_choose_tiling(rscreen, templ),
			      templ->flags & R600_RESOURCE_FLAG_FLUSHED_DEPTH);
	if (r) {
		return NULL;
	}
	r = rscreen->ws->surface_best(rscreen->ws, &surface);
	if (r) {
		return NULL;
	}
	return (struct pipe_resource *)r600_texture_create_object(screen, templ,
								  0, NULL, &surface);
}

static struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
						      const struct pipe_resource *templ,
						      struct winsys_handle *whandle)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	struct pb_buffer *buf = NULL;
	unsigned stride = 0;
	unsigned array_mode;
	enum radeon_bo_layout micro, macro;
	struct radeon_surface surface;
	bool scanout;
	int r;

	/* Support only 2D textures without mipmaps */
	if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT) ||
	      templ->depth0 != 1 || templ->last_level != 0)
		return NULL;

	buf = rscreen->ws->buffer_from_handle(rscreen->ws, whandle, &stride);
	if (!buf)
		return NULL;

	rscreen->ws->buffer_get_tiling(buf, &micro, &macro,
				       &surface.bankw, &surface.bankh,
				       &surface.tile_split,
				       &surface.stencil_tile_split,
				       &surface.mtilea, &scanout);

	if (macro == RADEON_LAYOUT_TILED)
		array_mode = RADEON_SURF_MODE_2D;
	else if (micro == RADEON_LAYOUT_TILED)
		array_mode = RADEON_SURF_MODE_1D;
	else
		array_mode = RADEON_SURF_MODE_LINEAR_ALIGNED;

	r = r600_init_surface(rscreen, &surface, templ, array_mode, false);
	if (r) {
		return NULL;
	}

	if (scanout)
		surface.flags |= RADEON_SURF_SCANOUT;

	return (struct pipe_resource *)r600_texture_create_object(screen, templ,
								  stride, buf, &surface);
}

bool r600_init_flushed_depth_texture(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     struct r600_texture **staging)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct pipe_resource resource;
	struct r600_texture **flushed_depth_texture = staging ?
			staging : &rtex->flushed_depth_texture;

	if (!staging && rtex->flushed_depth_texture)
		return true; /* it's ready */

	resource.target = texture->target;
	resource.format = texture->format;
	resource.width0 = texture->width0;
	resource.height0 = texture->height0;
	resource.depth0 = texture->depth0;
	resource.array_size = texture->array_size;
	resource.last_level = texture->last_level;
	resource.nr_samples = texture->nr_samples;
	resource.usage = staging ? PIPE_USAGE_STAGING : PIPE_USAGE_DEFAULT;
	resource.bind = texture->bind & ~PIPE_BIND_DEPTH_STENCIL;
	resource.flags = texture->flags | R600_RESOURCE_FLAG_FLUSHED_DEPTH;

	if (staging)
		resource.flags |= R600_RESOURCE_FLAG_TRANSFER;

	*flushed_depth_texture = (struct r600_texture *)ctx->screen->resource_create(ctx->screen, &resource);
	if (*flushed_depth_texture == NULL) {
		R600_ERR("failed to create temporary texture to hold flushed depth\n");
		return false;
	}

	(*flushed_depth_texture)->is_flushing_texture = TRUE;
	(*flushed_depth_texture)->non_disp_tiling = false;
	return true;
}

/**
 * Initialize the pipe_resource descriptor to be of the same size as the box,
 * which is supposed to hold a subregion of the texture "orig" at the given
 * mipmap level.
 */
static void r600_init_temp_resource_from_box(struct pipe_resource *res,
					     struct pipe_resource *orig,
					     const struct pipe_box *box,
					     unsigned level, unsigned flags)
{
	memset(res, 0, sizeof(*res));
	res->format = orig->format;
	res->width0 = box->width;
	res->height0 = box->height;
	res->depth0 = 1;
	res->array_size = 1;
	res->usage = flags & R600_RESOURCE_FLAG_TRANSFER ? PIPE_USAGE_STAGING : PIPE_USAGE_DEFAULT;
	res->flags = flags;

	/* We must set the correct texture target and dimensions for a 3D box. */
	if (box->depth > 1 && util_max_layer(orig, level) > 0)
		res->target = orig->target;
	else
		res->target = PIPE_TEXTURE_2D;

	switch (res->target) {
	case PIPE_TEXTURE_1D_ARRAY:
	case PIPE_TEXTURE_2D_ARRAY:
	case PIPE_TEXTURE_CUBE_ARRAY:
		res->array_size = box->depth;
		break;
	case PIPE_TEXTURE_3D:
		res->depth0 = box->depth;
		break;
	default:;
	}
}

static void *r600_texture_transfer_map(struct pipe_context *ctx,
				       struct pipe_resource *texture,
				       unsigned level,
				       unsigned usage,
				       const struct pipe_box *box,
				       struct pipe_transfer **ptransfer)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct r600_transfer *trans;
	boolean use_staging_texture = FALSE;
	struct r600_resource *buf;
	unsigned offset = 0;
	char *map;

	/* We cannot map a tiled texture directly because the data is
	 * in a different order, therefore we do detiling using a blit.
	 *
	 * Also, use a temporary in GTT memory for read transfers, as
	 * the CPU is much happier reading out of cached system memory
	 * than uncached VRAM.
	 */
	if (rtex->surface.level[level].mode >= RADEON_SURF_MODE_1D)
		use_staging_texture = TRUE;

	/* Untiled buffers in VRAM, which is slow for CPU reads */
	if ((usage & PIPE_TRANSFER_READ) && !(usage & PIPE_TRANSFER_MAP_DIRECTLY) &&
	    (rtex->resource.domains == RADEON_DOMAIN_VRAM)) {
		use_staging_texture = TRUE;
	}

	/* Use a staging texture for uploads if the underlying BO is busy. */
	if (!(usage & PIPE_TRANSFER_READ) &&
	    (r600_rings_is_buffer_referenced(rctx, rtex->resource.cs_buf, RADEON_USAGE_READWRITE) ||
	     rctx->ws->buffer_is_busy(rtex->resource.buf, RADEON_USAGE_READWRITE))) {
		use_staging_texture = TRUE;
	}

	if (texture->flags & R600_RESOURCE_FLAG_TRANSFER) {
		use_staging_texture = FALSE;
	}

	if (use_staging_texture && (usage & PIPE_TRANSFER_MAP_DIRECTLY)) {
		return NULL;
	}

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	trans->transfer.resource = texture;
	trans->transfer.level = level;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;

	if (rtex->is_depth) {
		struct r600_texture *staging_depth;

		if (rtex->resource.b.b.nr_samples > 1) {
			/* MSAA depth buffers need to be converted to single sample buffers.
			 *
			 * Mapping MSAA depth buffers can occur if ReadPixels is called
			 * with a multisample GLX visual.
			 *
			 * First downsample the depth buffer to a temporary texture,
			 * then decompress the temporary one to staging.
			 *
			 * Only the region being mapped is transfered.
			 */
			struct pipe_resource resource;

			r600_init_temp_resource_from_box(&resource, texture, box, level, 0);

			if (!r600_init_flushed_depth_texture(ctx, &resource, &staging_depth)) {
				R600_ERR("failed to create temporary texture to hold untiled copy\n");
				FREE(trans);
				return NULL;
			}

			if (usage & PIPE_TRANSFER_READ) {
				struct pipe_resource *temp = ctx->screen->resource_create(ctx->screen, &resource);

				r600_copy_region_with_blit(ctx, temp, 0, 0, 0, 0, texture, level, box);
				rctx->blit_decompress_depth(ctx, (struct r600_texture*)temp, staging_depth,
							    0, 0, 0, box->depth, 0, 0);
				pipe_resource_reference((struct pipe_resource**)&temp, NULL);
			}
		}
		else {
			/* XXX: only readback the rectangle which is being mapped? */
			/* XXX: when discard is true, no need to read back from depth texture */
			if (!r600_init_flushed_depth_texture(ctx, texture, &staging_depth)) {
				R600_ERR("failed to create temporary texture to hold untiled copy\n");
				FREE(trans);
				return NULL;
			}

			rctx->blit_decompress_depth(ctx, rtex, staging_depth,
						    level, level,
						    box->z, box->z + box->depth - 1,
						    0, 0);

			offset = r600_texture_get_offset(staging_depth, level, box);
		}

		trans->transfer.stride = staging_depth->surface.level[level].pitch_bytes;
		trans->transfer.layer_stride = staging_depth->surface.level[level].slice_size;
		trans->staging = (struct r600_resource*)staging_depth;
	} else if (use_staging_texture) {
		struct pipe_resource resource;
		struct r600_texture *staging;

		r600_init_temp_resource_from_box(&resource, texture, box, level,
						 R600_RESOURCE_FLAG_TRANSFER);
		resource.usage = (usage & PIPE_TRANSFER_READ) ?
			PIPE_USAGE_STAGING : PIPE_USAGE_STREAM;

		/* Create the temporary texture. */
		staging = (struct r600_texture*)ctx->screen->resource_create(ctx->screen, &resource);
		if (staging == NULL) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			FREE(trans);
			return NULL;
		}
		trans->staging = &staging->resource;
		trans->transfer.stride = staging->surface.level[0].pitch_bytes;
		trans->transfer.layer_stride = staging->surface.level[0].slice_size;
		if (usage & PIPE_TRANSFER_READ) {
			r600_copy_to_staging_texture(ctx, trans);
		}
	} else {
		/* the resource is mapped directly */
		trans->transfer.stride = rtex->surface.level[level].pitch_bytes;
		trans->transfer.layer_stride = rtex->surface.level[level].slice_size;
		offset = r600_texture_get_offset(rtex, level, box);
	}

	if (trans->staging) {
		buf = trans->staging;
		if (!rtex->is_depth && !(usage & PIPE_TRANSFER_READ))
			usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
	} else {
		buf = &rtex->resource;
	}

	if (!(map = r600_buffer_map_sync_with_rings(rctx, buf, usage))) {
		pipe_resource_reference((struct pipe_resource**)&trans->staging, NULL);
		FREE(trans);
		return NULL;
	}

	*ptransfer = &trans->transfer;
	return map + offset;
}

static void r600_texture_transfer_unmap(struct pipe_context *ctx,
					struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct radeon_winsys_cs_handle *buf;
	struct pipe_resource *texture = transfer->resource;
	struct r600_texture *rtex = (struct r600_texture*)texture;

	if (rtransfer->staging) {
		buf = rtransfer->staging->cs_buf;
	} else {
		buf = r600_resource(transfer->resource)->cs_buf;
	}
	rctx->ws->buffer_unmap(buf);

	if ((transfer->usage & PIPE_TRANSFER_WRITE) && rtransfer->staging) {
		if (rtex->is_depth && rtex->resource.b.b.nr_samples <= 1) {
			ctx->resource_copy_region(ctx, texture, transfer->level,
						  transfer->box.x, transfer->box.y, transfer->box.z,
						  &rtransfer->staging->b.b, transfer->level,
						  &transfer->box);
		} else {
			r600_copy_from_staging_texture(ctx, rtransfer);
		}
	}

	if (rtransfer->staging)
		pipe_resource_reference((struct pipe_resource**)&rtransfer->staging, NULL);

	FREE(transfer);
}

static const struct u_resource_vtbl r600_texture_vtbl =
{
	NULL,				/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	r600_texture_transfer_map,	/* transfer_map */
	NULL,				/* transfer_flush_region */
	r600_texture_transfer_unmap,	/* transfer_unmap */
	NULL				/* transfer_inline_write */
};

struct pipe_surface *r600_create_surface_custom(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *templ,
						unsigned width, unsigned height)
{
	struct r600_surface *surface = CALLOC_STRUCT(r600_surface);

	if (surface == NULL)
		return NULL;

	assert(templ->u.tex.first_layer <= util_max_layer(texture, templ->u.tex.level));
	assert(templ->u.tex.last_layer <= util_max_layer(texture, templ->u.tex.level));

	pipe_reference_init(&surface->base.reference, 1);
	pipe_resource_reference(&surface->base.texture, texture);
	surface->base.context = pipe;
	surface->base.format = templ->format;
	surface->base.width = width;
	surface->base.height = height;
	surface->base.u = templ->u;
	return &surface->base;
}

static struct pipe_surface *r600_create_surface(struct pipe_context *pipe,
						struct pipe_resource *tex,
						const struct pipe_surface *templ)
{
	unsigned level = templ->u.tex.level;

	return r600_create_surface_custom(pipe, tex, templ,
					  u_minify(tex->width0, level),
					  u_minify(tex->height0, level));
}

static void r600_surface_destroy(struct pipe_context *pipe,
				 struct pipe_surface *surface)
{
	struct r600_surface *surf = (struct r600_surface*)surface;
	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_fmask, NULL);
	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_cmask, NULL);
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}

unsigned r600_translate_colorswap(enum pipe_format format)
{
	const struct util_format_description *desc = util_format_description(format);

#define HAS_SWIZZLE(chan,swz) (desc->swizzle[chan] == UTIL_FORMAT_SWIZZLE_##swz)

	if (format == PIPE_FORMAT_R11G11B10_FLOAT) /* isn't plain */
		return V_0280A0_SWAP_STD;

	if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
		return ~0U;

	switch (desc->nr_channels) {
	case 1:
		if (HAS_SWIZZLE(0,X))
			return V_0280A0_SWAP_STD; /* X___ */
		else if (HAS_SWIZZLE(3,X))
			return V_0280A0_SWAP_ALT_REV; /* ___X */
		break;
	case 2:
		if ((HAS_SWIZZLE(0,X) && HAS_SWIZZLE(1,Y)) ||
		    (HAS_SWIZZLE(0,X) && HAS_SWIZZLE(1,NONE)) ||
		    (HAS_SWIZZLE(0,NONE) && HAS_SWIZZLE(1,Y)))
			return V_0280A0_SWAP_STD; /* XY__ */
		else if ((HAS_SWIZZLE(0,Y) && HAS_SWIZZLE(1,X)) ||
			 (HAS_SWIZZLE(0,Y) && HAS_SWIZZLE(1,NONE)) ||
		         (HAS_SWIZZLE(0,NONE) && HAS_SWIZZLE(1,X)))
			return V_0280A0_SWAP_STD_REV; /* YX__ */
		else if (HAS_SWIZZLE(0,X) && HAS_SWIZZLE(3,Y))
			return V_0280A0_SWAP_ALT; /* X__Y */
		else if (HAS_SWIZZLE(0,Y) && HAS_SWIZZLE(3,X))
			return V_0280A0_SWAP_ALT_REV; /* Y__X */
		break;
	case 3:
		if (HAS_SWIZZLE(0,X))
			return V_0280A0_SWAP_STD; /* XYZ */
		else if (HAS_SWIZZLE(0,Z))
			return V_0280A0_SWAP_STD_REV; /* ZYX */
		break;
	case 4:
		/* check the middle channels, the 1st and 4th channel can be NONE */
		if (HAS_SWIZZLE(1,Y) && HAS_SWIZZLE(2,Z))
			return V_0280A0_SWAP_STD; /* XYZW */
		else if (HAS_SWIZZLE(1,Z) && HAS_SWIZZLE(2,Y))
			return V_0280A0_SWAP_STD_REV; /* WZYX */
		else if (HAS_SWIZZLE(1,Y) && HAS_SWIZZLE(2,X))
			return V_0280A0_SWAP_ALT; /* ZYXW */
		else if (HAS_SWIZZLE(1,X) && HAS_SWIZZLE(2,Y))
			return V_0280A0_SWAP_ALT_REV; /* WXYZ */
		break;
	}
	return ~0U;
}

static void evergreen_set_clear_color(struct r600_texture *rtex,
				      enum pipe_format surface_format,
				      const union pipe_color_union *color)
{
	union util_color uc;

	memset(&uc, 0, sizeof(uc));

	if (util_format_is_pure_uint(surface_format)) {
		util_format_write_4ui(surface_format, color->ui, 0, &uc, 0, 0, 0, 1, 1);
	} else if (util_format_is_pure_sint(surface_format)) {
		util_format_write_4i(surface_format, color->i, 0, &uc, 0, 0, 0, 1, 1);
	} else {
		util_pack_color(color->f, surface_format, &uc);
	}

	memcpy(rtex->color_clear_value, &uc, 2 * sizeof(uint32_t));
}

void evergreen_do_fast_color_clear(struct r600_common_context *rctx,
				   struct pipe_framebuffer_state *fb,
				   struct r600_atom *fb_state,
				   unsigned *buffers,
				   const union pipe_color_union *color)
{
	int i;

	if (rctx->current_render_cond)
		return;

	for (i = 0; i < fb->nr_cbufs; i++) {
		struct r600_texture *tex;
		unsigned clear_bit = PIPE_CLEAR_COLOR0 << i;

		if (!fb->cbufs[i])
			continue;

		/* if this colorbuffer is not being cleared */
		if (!(*buffers & clear_bit))
			continue;

		tex = (struct r600_texture *)fb->cbufs[i]->texture;

		/* 128-bit formats are unusupported */
		if (util_format_get_blocksizebits(fb->cbufs[i]->format) > 64) {
			continue;
		}

		/* the clear is allowed if all layers are bound */
		if (fb->cbufs[i]->u.tex.first_layer != 0 ||
		    fb->cbufs[i]->u.tex.last_layer != util_max_layer(&tex->resource.b.b, 0)) {
			continue;
		}

		/* cannot clear mipmapped textures */
		if (fb->cbufs[i]->texture->last_level != 0) {
			continue;
		}

		/* only supported on tiled surfaces */
		if (tex->surface.level[0].mode < RADEON_SURF_MODE_1D) {
			continue;
		}

		/* fast color clear with 1D tiling doesn't work on old kernels and CIK */
		if (tex->surface.level[0].mode == RADEON_SURF_MODE_1D &&
		    rctx->chip_class >= CIK && rctx->screen->info.drm_minor < 38) {
			continue;
		}

		/* ensure CMASK is enabled */
		r600_texture_alloc_cmask_separate(rctx->screen, tex);
		if (tex->cmask.size == 0) {
			continue;
		}

		/* Do the fast clear. */
		evergreen_set_clear_color(tex, fb->cbufs[i]->format, color);
		rctx->clear_buffer(&rctx->b, &tex->cmask_buffer->b.b,
				   tex->cmask.offset, tex->cmask.size, 0);

		tex->dirty_level_mask |= 1 << fb->cbufs[i]->u.tex.level;
		fb_state->dirty = true;
		*buffers &= ~clear_bit;
	}
}

void r600_init_screen_texture_functions(struct r600_common_screen *rscreen)
{
	rscreen->b.resource_from_handle = r600_texture_from_handle;
	rscreen->b.resource_get_handle = r600_texture_get_handle;
}

void r600_init_context_texture_functions(struct r600_common_context *rctx)
{
	rctx->b.create_surface = r600_create_surface;
	rctx->b.surface_destroy = r600_surface_destroy;
}
