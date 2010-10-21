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
#include <errno.h>
#include <pipe/p_screen.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include "state_tracker/drm_driver.h"
#include "r600_pipe.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"
#include "r600d.h"
#include "r600_formats.h"

extern struct u_resource_vtbl r600_texture_vtbl;

/* Copy from a tiled texture to a detiled one. */
static void r600_copy_from_tiled_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;
	struct pipe_subresource subdst;

	subdst.face = 0;
	subdst.level = 0;
	ctx->resource_copy_region(ctx, rtransfer->linear_texture,
				subdst, 0, 0, 0, texture, transfer->sr,
				transfer->box.x, transfer->box.y, transfer->box.z,
				transfer->box.width, transfer->box.height);
}


/* Copy from a detiled texture to a tiled one. */
static void r600_copy_into_tiled_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;
	struct pipe_subresource subsrc;

	subsrc.face = 0;
	subsrc.level = 0;
	ctx->resource_copy_region(ctx, texture, transfer->sr,
				  transfer->box.x, transfer->box.y, transfer->box.z,
				  rtransfer->linear_texture, subsrc,
				  0, 0, 0,
				  transfer->box.width, transfer->box.height);

	ctx->flush(ctx, 0, NULL);
}

static unsigned r600_texture_get_offset(struct r600_resource_texture *rtex,
					unsigned level, unsigned zslice,
					unsigned face)
{
	unsigned offset = rtex->offset[level];

	switch (rtex->resource.base.b.target) {
	case PIPE_TEXTURE_3D:
		assert(face == 0);
		return offset + zslice * rtex->layer_size[level];
	case PIPE_TEXTURE_CUBE:
		assert(zslice == 0);
		return offset + face * rtex->layer_size[level];
	default:
		assert(zslice == 0 && face == 0);
		return offset;
	}
}

static unsigned mip_minify(unsigned size, unsigned level)
{
	unsigned val;
	val = u_minify(size, level);
	if (level > 0)
		val = util_next_power_of_two(val);
	return val;
}

static unsigned r600_texture_get_stride(struct pipe_screen *screen,
					struct r600_resource_texture *rtex,
					unsigned level)
{
	struct pipe_resource *ptex = &rtex->resource.base.b;
	struct radeon *radeon = (struct radeon *)screen->winsys;
	enum chip_class chipc = r600_get_family_class(radeon);
	unsigned width, stride;
	
	if (rtex->pitch_override)
		return rtex->pitch_override;

	width = mip_minify(ptex->width0, level);

	stride = util_format_get_stride(ptex->format, align(width, 64));
	if (chipc == EVERGREEN)
		stride = align(stride, 512);
	else
		stride = align(stride, 256);
	return stride;
}

static unsigned r600_texture_get_nblocksy(struct pipe_screen *screen,
					  struct r600_resource_texture *rtex,
					  unsigned level)
{
	struct pipe_resource *ptex = &rtex->resource.base.b;
	unsigned height;

	height = mip_minify(ptex->height0, level);
	return util_format_get_nblocksy(ptex->format, height);
}

/* Get a width in pixels from a stride in bytes. */
static unsigned pitch_to_width(enum pipe_format format,
                                unsigned pitch_in_bytes)
{
    return (pitch_in_bytes / util_format_get_blocksize(format)) *
            util_format_get_blockwidth(format);
}

static void r600_setup_miptree(struct pipe_screen *screen,
			       struct r600_resource_texture *rtex)
{
	struct pipe_resource *ptex = &rtex->resource.base.b;
	struct radeon *radeon = (struct radeon *)screen->winsys;
	enum chip_class chipc = r600_get_family_class(radeon);
	unsigned pitch, size, layer_size, i, offset;
	unsigned nblocksy;

	for (i = 0, offset = 0; i <= ptex->last_level; i++) {
		pitch = r600_texture_get_stride(screen, rtex, i);
		nblocksy = r600_texture_get_nblocksy(screen, rtex, i);

		layer_size = pitch * nblocksy;

		if (ptex->target == PIPE_TEXTURE_CUBE) {
			if (chipc >= R700)
				size = layer_size * 8;
			else
				size = layer_size * 6;
		}
		else
			size = layer_size * u_minify(ptex->depth0, i);
		rtex->offset[i] = offset;
		rtex->layer_size[i] = layer_size;
		rtex->pitch_in_bytes[i] = pitch;
		rtex->pitch_in_pixels[i] = pitch_to_width(ptex->format, pitch);
		offset += size;
	}
	rtex->size = offset;
}

static struct r600_resource_texture *
r600_texture_create_object(struct pipe_screen *screen,
			   const struct pipe_resource *base,
			   unsigned array_mode,
			   unsigned pitch_in_bytes_override,
			   unsigned max_buffer_size,
			   struct r600_bo *bo)
{
	struct r600_resource_texture *rtex;
	struct r600_resource *resource;
	struct radeon *radeon = (struct radeon *)screen->winsys;

	rtex = CALLOC_STRUCT(r600_resource_texture);
	if (rtex == NULL)
		return NULL;

	resource = &rtex->resource;
	resource->base.b = *base;
	resource->base.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->base.b.reference, 1);
	resource->base.b.screen = screen;
	resource->bo = bo;
	resource->domain = r600_domain_from_usage(resource->base.b.bind);
	rtex->pitch_override = pitch_in_bytes_override;
	rtex->array_mode = array_mode;

	if (array_mode)
		rtex->tiled = 1;
	r600_setup_miptree(screen, rtex);

	resource->size = rtex->size;

	if (!resource->bo) {
		resource->bo = r600_bo(radeon, rtex->size, 4096, 0);
		if (!resource->bo) {
			FREE(rtex);
			return NULL;
		}
	}
	return rtex;
}

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	unsigned array_mode = 0;

	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  0, 0, NULL);

}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;
	struct radeon *radeon = (struct radeon *)screen->winsys;

	if (rtex->flushed_depth_texture)
		pipe_resource_reference((struct pipe_resource **)&rtex->flushed_depth_texture, NULL);

	if (resource->bo) {
		r600_bo_reference(radeon, &resource->bo, NULL);
	}
	FREE(rtex);
}

static struct pipe_surface *r600_get_tex_surface(struct pipe_screen *screen,
						struct pipe_resource *texture,
						unsigned face, unsigned level,
						unsigned zslice, unsigned flags)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_surface *surface = CALLOC_STRUCT(pipe_surface);
	unsigned offset;

	if (surface == NULL)
		return NULL;
	offset = r600_texture_get_offset(rtex, level, zslice, face);
	pipe_reference_init(&surface->reference, 1);
	pipe_resource_reference(&surface->texture, texture);
	surface->format = texture->format;
	surface->width = mip_minify(texture->width0, level);
	surface->height = mip_minify(texture->height0, level);
	surface->offset = offset;
	surface->usage = flags;
	surface->zslice = zslice;
	surface->texture = texture;
	surface->face = face;
	surface->level = level;
	return surface;
}

static void r600_tex_surface_destroy(struct pipe_surface *surface)
{
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}


struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
					       const struct pipe_resource *templ,
					       struct winsys_handle *whandle)
{
	struct radeon *rw = (struct radeon*)screen->winsys;
	struct r600_bo *bo = NULL;
	unsigned array_mode = 0;

	/* Support only 2D textures without mipmaps */
	if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT) ||
	      templ->depth0 != 1 || templ->last_level != 0)
		return NULL;

	bo = r600_bo_handle(rw, whandle->handle, &array_mode);
	if (bo == NULL) {
		return NULL;
	}

	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  whandle->stride,
								  0,
								  bo);
}

static unsigned int r600_texture_is_referenced(struct pipe_context *context,
						struct pipe_resource *texture,
						unsigned face, unsigned level)
{
	/* FIXME */
	return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}

int (*r600_blit_uncompress_depth_ptr)(struct pipe_context *ctx, struct r600_resource_texture *texture);

int r600_texture_depth_flush(struct pipe_context *ctx,
			     struct pipe_resource *texture)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;

	if (rtex->flushed_depth_texture)
		goto out;

	resource.target = PIPE_TEXTURE_2D;
	resource.format = texture->format;
	resource.width0 = texture->width0;
	resource.height0 = texture->height0;
	resource.depth0 = 1;
	resource.last_level = 0;
	resource.nr_samples = 0;
	resource.usage = PIPE_USAGE_DYNAMIC;
	resource.bind = 0;
	resource.flags = 0;

	resource.bind |= PIPE_BIND_DEPTH_STENCIL;

	rtex->flushed_depth_texture = (struct r600_resource_texture *)ctx->screen->resource_create(ctx->screen, &resource);
	if (rtex->flushed_depth_texture == NULL) {
		R600_ERR("failed to create temporary texture to hold untiled copy\n");
		return -ENOMEM;
	}

out:
	r600_blit_uncompress_depth_ptr(ctx, rtex);
	return 0;
}

struct pipe_transfer* r600_texture_get_transfer(struct pipe_context *ctx,
						struct pipe_resource *texture,
						struct pipe_subresource sr,
						unsigned usage,
						const struct pipe_box *box)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;
	struct r600_transfer *trans;
	int r;

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	pipe_resource_reference(&trans->transfer.resource, texture);
	trans->transfer.sr = sr;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;
	if (rtex->depth) {
		r = r600_texture_depth_flush(ctx, texture);
		if (r < 0) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}
	} else if (rtex->tiled) {
		resource.target = PIPE_TEXTURE_2D;
		resource.format = texture->format;
		resource.width0 = box->width;
		resource.height0 = box->height;
		resource.depth0 = 1;
		resource.last_level = 0;
		resource.nr_samples = 0;
		resource.usage = PIPE_USAGE_DYNAMIC;
		resource.bind = 0;
		resource.flags = 0;
		/* For texture reading, the temporary (detiled) texture is used as
		 * a render target when blitting from a tiled texture. */
		if (usage & PIPE_TRANSFER_READ) {
			resource.bind |= PIPE_BIND_RENDER_TARGET;
		}
		/* For texture writing, the temporary texture is used as a sampler
		 * when blitting into a tiled texture. */
		if (usage & PIPE_TRANSFER_WRITE) {
			resource.bind |= PIPE_BIND_SAMPLER_VIEW;
		}
		/* Create the temporary texture. */
		trans->linear_texture = ctx->screen->resource_create(ctx->screen, &resource);
		if (trans->linear_texture == NULL) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}

		trans->transfer.stride =
		  ((struct r600_resource_texture *)trans->linear_texture)->pitch_in_bytes[0];
		if (usage & PIPE_TRANSFER_READ) {
			/* We cannot map a tiled texture directly because the data is
			 * in a different order, therefore we do detiling using a blit. */
			r600_copy_from_tiled_texture(ctx, trans);
			/* Always referenced in the blit. */
			ctx->flush(ctx, 0, NULL);
		}
		return &trans->transfer;
	}
	trans->transfer.stride = rtex->pitch_in_bytes[sr.level];
	trans->offset = r600_texture_get_offset(rtex, sr.level, box->z, sr.face);
	return &trans->transfer;
}

void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

	if (rtransfer->linear_texture) {
		if (transfer->usage & PIPE_TRANSFER_WRITE) {
			r600_copy_into_tiled_texture(ctx, rtransfer);
		}
		pipe_resource_reference(&rtransfer->linear_texture, NULL);
	}
	if (rtex->flushed_depth_texture) {
		pipe_resource_reference((struct pipe_resource **)&rtex->flushed_depth_texture, NULL);
	}
	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_bo *bo;
	enum pipe_format format = transfer->resource->format;
	struct radeon *radeon = (struct radeon *)ctx->screen->winsys;
	unsigned offset = 0;
	char *map;

	if (rtransfer->linear_texture) {
		bo = ((struct r600_resource *)rtransfer->linear_texture)->bo;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture)
			bo = ((struct r600_resource *)rtex->flushed_depth_texture)->bo;
		else
			bo = ((struct r600_resource *)transfer->resource)->bo;

		offset = rtransfer->offset +
			transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
			transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
	}
	map = r600_bo_map(radeon, bo, 0, ctx);
	if (!map) {
		return NULL;
	}

	return map + offset;
}

void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct radeon *radeon = (struct radeon *)ctx->screen->winsys;
	struct r600_bo *bo;

	if (rtransfer->linear_texture) {
		bo = ((struct r600_resource *)rtransfer->linear_texture)->bo;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture) {
			bo = ((struct r600_resource *)rtex->flushed_depth_texture)->bo;
		} else {
			bo = ((struct r600_resource *)transfer->resource)->bo;
		}
	}
	r600_bo_unmap(radeon, bo);
}

struct u_resource_vtbl r600_texture_vtbl =
{
	u_default_resource_get_handle,	/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	r600_texture_is_referenced,	/* is_resource_referenced */
	r600_texture_get_transfer,	/* get_transfer */
	r600_texture_transfer_destroy,	/* transfer_destroy */
	r600_texture_transfer_map,	/* transfer_map */
	u_default_transfer_flush_region,/* transfer_flush_region */
	r600_texture_transfer_unmap,	/* transfer_unmap */
	u_default_transfer_inline_write	/* transfer_inline_write */
};

void r600_init_screen_texture_functions(struct pipe_screen *screen)
{
	screen->get_tex_surface = r600_get_tex_surface;
	screen->tex_surface_destroy = r600_tex_surface_destroy;
}

static unsigned r600_get_swizzle_combined(const unsigned char *swizzle_format,
		const unsigned char *swizzle_view)
{
	unsigned i;
	unsigned char swizzle[4];
	unsigned result = 0;
	const uint32_t swizzle_shift[4] = {
		16, 19, 22, 25,
	};
	const uint32_t swizzle_bit[4] = {
		0, 1, 2, 3,
	};

	if (swizzle_view) {
		/* Combine two sets of swizzles. */
		for (i = 0; i < 4; i++) {
			swizzle[i] = swizzle_view[i] <= UTIL_FORMAT_SWIZZLE_W ?
				swizzle_format[swizzle_view[i]] : swizzle_view[i];
		}
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
uint32_t r600_translate_texformat(enum pipe_format format,
				  const unsigned char *swizzle_view, 
				  uint32_t *word4_p, uint32_t *yuv_format_p)
{
	uint32_t result = 0, word4 = 0, yuv_format = 0;
	const struct util_format_description *desc;
	boolean uniform = TRUE;
	int i;
	const uint32_t sign_bit[4] = {
		S_038010_FORMAT_COMP_X(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Y(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Z(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_W(V_038010_SQ_FORMAT_COMP_SIGNED)
	};
	desc = util_format_description(format);

	word4 |= r600_get_swizzle_combined(desc->swizzle, swizzle_view);

	/* Colorspace (return non-RGB formats directly). */
	switch (desc->colorspace) {
		/* Depth stencil formats */
	case UTIL_FORMAT_COLORSPACE_ZS:
		switch (format) {
		case PIPE_FORMAT_Z16_UNORM:
			result = FMT_16;
			goto out_word4;
		case PIPE_FORMAT_X24S8_USCALED:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
			result = FMT_8_24;
			goto out_word4;
		case PIPE_FORMAT_S8X24_USCALED:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
			result = FMT_24_8;
			goto out_word4;
		case PIPE_FORMAT_S8_USCALED:
			result = V_0280A0_COLOR_8;
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
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
		goto out_unknown; /* TODO */
		
	case UTIL_FORMAT_COLORSPACE_SRGB:
		word4 |= S_038010_FORCE_DEGAMMA(1);
		if (format == PIPE_FORMAT_L8A8_SRGB || format == PIPE_FORMAT_L8_SRGB)
			goto out_unknown; /* fails for some reason - TODO */
		break;

	default:
		break;
	}

	/* S3TC formats. TODO */
	if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
		goto out_unknown;
	}


	for (i = 0; i < desc->nr_channels; i++) {	
		if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
			word4 |= sign_bit[i];
		}
	}

	/* R8G8Bx_SNORM - TODO CxV8U8 */

	/* RGTC - TODO */

	/* See whether the components are of the same size. */
	for (i = 1; i < desc->nr_channels; i++) {
		uniform = uniform && desc->channel[0].size == desc->channel[i].size;
	}
	
	/* Non-uniform formats. */
	if (!uniform) {
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
				result = FMT_10_10_10_2;
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
		if (!desc->channel[i].normalized &&
		    desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
			goto out_unknown;
		}

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
		
	}
out_word4:
	if (word4_p)
		*word4_p = word4;
	if (yuv_format_p)
		*yuv_format_p = yuv_format;
	return result;
out_unknown:
//	R600_ERR("Unable to handle texformat %d %s\n", format, util_format_name(format));
	return ~0;
}
