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
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"
#include "r600d.h"

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

static unsigned long r600_texture_get_offset(struct r600_resource_texture *rtex,
					unsigned level, unsigned zslice,
					unsigned face)
{
	unsigned long offset = rtex->offset[level];

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

static void r600_setup_miptree(struct r600_screen *rscreen, struct r600_resource_texture *rtex)
{
	struct pipe_resource *ptex = &rtex->resource.base.b;
	unsigned long w, h, pitch, size, layer_size, i, offset;

	rtex->bpt = util_format_get_blocksize(ptex->format);
	for (i = 0, offset = 0; i <= ptex->last_level; i++) {
		w = u_minify(ptex->width0, i);
		h = u_minify(ptex->height0, i);
		h = util_next_power_of_two(h);
		pitch = util_format_get_stride(ptex->format, align(w, 64));
		pitch = align(pitch, 256);
		layer_size = pitch * h;
		if (ptex->target == PIPE_TEXTURE_CUBE)
			size = layer_size * 6;
		else
			size = layer_size * u_minify(ptex->depth0, i);
		rtex->offset[i] = offset;
		rtex->layer_size[i] = layer_size;
		rtex->pitch[i] = pitch;
		rtex->width[i] = w;
		rtex->height[i] = h;
		offset += size;
	}
	rtex->size = offset;
}

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	struct r600_resource_texture *rtex;
	struct r600_resource *resource;
	struct r600_screen *rscreen = r600_screen(screen);

	rtex = CALLOC_STRUCT(r600_resource_texture);
	if (!rtex) {
		return NULL;
	}
	resource = &rtex->resource;
	resource->base.b = *templ;
	resource->base.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->base.b.reference, 1);
	resource->base.b.screen = screen;
	r600_setup_miptree(rscreen, rtex);

	/* FIXME alignment 4096 enought ? too much ? */
	resource->domain = r600_domain_from_usage(resource->base.b.bind);
	resource->bo = radeon_bo(rscreen->rw, 0, rtex->size, 4096, NULL);
	if (resource->bo == NULL) {
		FREE(rtex);
		return NULL;
	}
	return &resource->base.b;
}

static void r600_texture_destroy_state(struct pipe_resource *ptexture)
{
	struct r600_resource_texture *rtexture = (struct r600_resource_texture*)ptexture;

	for (int i = 0; i < PIPE_MAX_TEXTURE_LEVELS; i++) {
		radeon_state_fini(&rtexture->scissor[i]);
		radeon_state_fini(&rtexture->db[i]);
		for (int j = 0; j < 8; j++) {
			radeon_state_fini(&rtexture->cb[j][i]);
		}
	}
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;
	struct r600_screen *rscreen = r600_screen(screen);

	if (resource->bo) {
		radeon_bo_decref(rscreen->rw, resource->bo);
	}
	if (rtex->uncompressed) {
		radeon_bo_decref(rscreen->rw, rtex->uncompressed);
	}
	r600_texture_destroy_state(ptex);
	FREE(rtex);
}

static struct pipe_surface *r600_get_tex_surface(struct pipe_screen *screen,
						struct pipe_resource *texture,
						unsigned face, unsigned level,
						unsigned zslice, unsigned flags)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_surface *surface = CALLOC_STRUCT(pipe_surface);
	unsigned long offset;

	if (surface == NULL)
		return NULL;
	offset = r600_texture_get_offset(rtex, level, zslice, face);
	pipe_reference_init(&surface->reference, 1);
	pipe_resource_reference(&surface->texture, texture);
	surface->format = texture->format;
	surface->width = u_minify(texture->width0, level);
	surface->height = u_minify(texture->height0, level);
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
	struct r600_resource_texture *rtex;
	struct r600_resource *resource;
	struct radeon_bo *bo = NULL;

	/* Support only 2D textures without mipmaps */
	if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT) ||
	      templ->depth0 != 1 || templ->last_level != 0)
		return NULL;

	rtex = CALLOC_STRUCT(r600_resource_texture);
	if (rtex == NULL)
		return NULL;

	bo = radeon_bo(rw, whandle->handle, 0, 0, NULL);
	if (bo == NULL) {
		FREE(rtex);
		return NULL;
	}

	resource = &rtex->resource;
	resource->base.b = *templ;
	resource->base.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->base.b.reference, 1);
	resource->base.b.screen = screen;
	resource->bo = bo;
	rtex->depth = 0;
	rtex->pitch_override = whandle->stride;
	rtex->bpt = util_format_get_blocksize(templ->format);
	rtex->pitch[0] = whandle->stride;
	rtex->width[0] = templ->width0;
	rtex->height[0] = templ->height0;
	rtex->offset[0] = 0;
	rtex->size = align(rtex->pitch[0] * templ->height0, 64);

	return &resource->base.b;
}

static unsigned int r600_texture_is_referenced(struct pipe_context *context,
						struct pipe_resource *texture,
						unsigned face, unsigned level)
{
	/* FIXME */
	return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
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

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	pipe_resource_reference(&trans->transfer.resource, texture);
	trans->transfer.sr = sr;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;
	trans->transfer.stride = rtex->pitch[sr.level];
	trans->offset = r600_texture_get_offset(rtex, sr.level, box->z, sr.face);
	if (rtex->tilled && !rtex->depth) {
		resource.target = PIPE_TEXTURE_2D;
		resource.format = texture->format;
		resource.width0 = box->width;
		resource.height0 = box->height;
		resource.depth0 = 0;
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
		if (usage & PIPE_TRANSFER_READ) {
			/* We cannot map a tiled texture directly because the data is
			 * in a different order, therefore we do detiling using a blit. */
			r600_copy_from_tiled_texture(ctx, trans);
			/* Always referenced in the blit. */
			ctx->flush(ctx, 0, NULL);
		}
	}
	return &trans->transfer;
}

void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;

	if (rtransfer->linear_texture) {
		pipe_resource_reference(&rtransfer->linear_texture, NULL);
	}
	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct radeon_bo *bo;
	enum pipe_format format = transfer->resource->format;
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_resource_texture *rtex;
	unsigned long offset = 0;
	char *map;
	int r;

	r600_flush(ctx, 0, NULL);
	if (rtransfer->linear_texture) {
		bo = ((struct r600_resource *)rtransfer->linear_texture)->bo;
	} else {
		rtex = (struct r600_resource_texture*)transfer->resource;
		if (rtex->depth) {
			r = r600_texture_from_depth(ctx, rtex, transfer->sr.level);
			if (r) {
				return NULL;
			}
			r600_flush(ctx, 0, NULL);
			bo = rtex->uncompressed;
		} else {
			bo = ((struct r600_resource *)transfer->resource)->bo;
		}
		offset = rtransfer->offset +
			transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
			transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
	}
	if (radeon_bo_map(rscreen->rw, bo)) {
		return NULL;
	}
	radeon_bo_wait(rscreen->rw, bo);

	map = bo->data;
	return map + offset;
}

void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_resource_texture *rtex;
	struct radeon_bo *bo;

	if (rtransfer->linear_texture) {
		bo = ((struct r600_resource *)rtransfer->linear_texture)->bo;
	} else {
		rtex = (struct r600_resource_texture*)transfer->resource;
		if (rtex->depth) {
			bo = rtex->uncompressed;
		} else {
			bo = ((struct r600_resource *)transfer->resource)->bo;
		}
	}
	radeon_bo_unmap(rscreen->rw, bo);
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
			result = V_0280A0_COLOR_16;
			goto out_word4;
		case PIPE_FORMAT_Z24X8_UNORM:
			result = V_0280A0_COLOR_8_24;
			goto out_word4;
		case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
			result = V_0280A0_COLOR_8_24;
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
				result = V_0280A0_COLOR_5_6_5;
				goto out_word4;
			}
			goto out_unknown;
		case 4:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 5 &&
			    desc->channel[2].size == 5 &&
			    desc->channel[3].size == 1) {
				result = V_0280A0_COLOR_1_5_5_5;
				goto out_word4;
			}
			if (desc->channel[0].size == 10 &&
			    desc->channel[1].size == 10 &&
			    desc->channel[2].size == 10 &&
			    desc->channel[3].size == 2) {
				result = V_0280A0_COLOR_10_10_10_2;
				goto out_word4;
			}
			goto out_unknown;
		}
		goto out_unknown;
	}

	/* uniform formats */
	switch (desc->channel[0].type) {
	case UTIL_FORMAT_TYPE_UNSIGNED:
	case UTIL_FORMAT_TYPE_SIGNED:
		if (!desc->channel[0].normalized &&
		    desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
			goto out_unknown;
		}

		switch (desc->channel[0].size) {
		case 4:
			switch (desc->nr_channels) {
			case 2:
				result = V_0280A0_COLOR_4_4;
				goto out_word4;
			case 4:
				result = V_0280A0_COLOR_4_4_4_4;
				goto out_word4;
			}
			goto out_unknown;
		case 8:
			switch (desc->nr_channels) {
			case 1:
				result = V_0280A0_COLOR_8;
				goto out_word4;
			case 2:
				result = V_0280A0_COLOR_8_8;
				goto out_word4;
			case 4:
				result = V_0280A0_COLOR_8_8_8_8;
				goto out_word4;
			}
			goto out_unknown;
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = V_0280A0_COLOR_16;
				goto out_word4;
			case 2:
				result = V_0280A0_COLOR_16_16;
				goto out_word4;
			case 4:
				result = V_0280A0_COLOR_16_16_16_16;
				goto out_word4;
			}
		}
		goto out_unknown;

	case UTIL_FORMAT_TYPE_FLOAT:
		switch (desc->channel[0].size) {
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = V_0280A0_COLOR_16_FLOAT;
				goto out_word4;
			case 2:
				result = V_0280A0_COLOR_16_16_FLOAT;
				goto out_word4;
			case 4:
				result = V_0280A0_COLOR_16_16_16_16_FLOAT;
				goto out_word4;
			}
			goto out_unknown;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				result = V_0280A0_COLOR_32_FLOAT;
				goto out_word4;
			case 2:
				result = V_0280A0_COLOR_32_32_FLOAT;
				goto out_word4;
			case 4:
				result = V_0280A0_COLOR_32_32_32_32_FLOAT;
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

int r600_texture_from_depth(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	int r;

	if (!rtexture->depth) {
		/* This shouldn't happen maybe print a warning */
		return 0;
	}
	if (rtexture->uncompressed && !rtexture->dirty) {
		/* Uncompressed bo already in good state */
		return 0;
	}

	/* allocate uncompressed texture */
	if (rtexture->uncompressed == NULL) {
		rtexture->uncompressed = radeon_bo(rscreen->rw, 0, rtexture->size, 4096, NULL);
		if (rtexture->uncompressed == NULL) {
			return -ENOMEM;
		}
	}

	/* render a rectangle covering whole buffer to uncompress depth */
	r = r600_blit_uncompress_depth(ctx, rtexture, level);
	if (r) {
		return r;
	}

	rtexture->dirty = 0;
	return 0;
}

static void r600_texture_state_scissor(struct r600_screen *rscreen,
					struct r600_resource_texture *rtexture,
					unsigned level)
{
	struct radeon_state *rstate = &rtexture->scissor[level];

	radeon_state_init(rstate, rscreen->rw, R600_STATE_SCISSOR, 0, 0);
	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_0_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_0_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_1_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_1_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_2_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_2_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_3_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_3_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_RULE] = 0x0000FFFF;
	rstate->states[R600_SCISSOR__PA_SC_EDGERULE] = 0xAAAAAAAA;
	rstate->states[R600_SCISSOR__PA_SC_GENERIC_SCISSOR_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_GENERIC_SCISSOR_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_SCREEN_SCISSOR_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_SCREEN_SCISSOR_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_TL] = 0x80000000;
	rstate->states[R600_SCISSOR__PA_SC_WINDOW_SCISSOR_BR] = S_028244_BR_X(rtexture->width[level]) | S_028244_BR_Y(rtexture->height[level]);
	rstate->states[R600_SCISSOR__PA_SC_WINDOW_SCISSOR_TL] = 0x80000000;

	radeon_state_pm4(rstate);
}

static void r600_texture_state_cb(struct r600_screen *rscreen, struct r600_resource_texture *rtexture, unsigned cb, unsigned level)
{
	struct radeon_state *rstate;
	struct r600_resource *rbuffer;
	unsigned pitch, slice;
	unsigned color_info;
	unsigned format, swap, ntype;
	const struct util_format_description *desc;

	rstate = &rtexture->cb[cb][level];
	radeon_state_init(rstate, rscreen->rw, R600_STATE_CB0 + cb, 0, 0);
	rbuffer = &rtexture->resource;

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	pitch = (rtexture->pitch[level] / rtexture->bpt) / 8 - 1;
	slice = (rtexture->pitch[level] / rtexture->bpt) * rtexture->height[level] / 64 - 1;
	ntype = 0;
	desc = util_format_description(rbuffer->base.b.format);
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_0280A0_NUMBER_SRGB;
	format = r600_translate_colorformat(rtexture->resource.base.b.format);
	swap = r600_translate_colorswap(rtexture->resource.base.b.format);
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
		rstate->bo[0] = radeon_bo_incref(rscreen->rw, rtexture->uncompressed);
		rstate->bo[1] = radeon_bo_incref(rscreen->rw, rtexture->uncompressed);
		rstate->bo[2] = radeon_bo_incref(rscreen->rw, rtexture->uncompressed);
		rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
		rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;
		rstate->placement[4] = RADEON_GEM_DOMAIN_GTT;
		rstate->nbo = 3;
		color_info = 0;
	} else {
		rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		rstate->bo[1] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		rstate->bo[2] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
		rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;
		rstate->placement[4] = RADEON_GEM_DOMAIN_GTT;
		rstate->nbo = 3;
		color_info = S_0280A0_SOURCE_FORMAT(1);
	}
	color_info |= S_0280A0_FORMAT(format) |
		S_0280A0_COMP_SWAP(swap) |
		S_0280A0_BLEND_CLAMP(1) |
		S_0280A0_NUMBER_TYPE(ntype);
	rstate->states[R600_CB0__CB_COLOR0_BASE] = rtexture->offset[level] >> 8;
	rstate->states[R600_CB0__CB_COLOR0_INFO] = color_info;
	rstate->states[R600_CB0__CB_COLOR0_SIZE] = S_028060_PITCH_TILE_MAX(pitch) |
						S_028060_SLICE_TILE_MAX(slice);

	radeon_state_pm4(rstate);
}

static void r600_texture_state_db(struct r600_screen *rscreen, struct r600_resource_texture *rtexture, unsigned level)
{
	struct radeon_state *rstate = &rtexture->db[level];
	struct r600_resource *rbuffer;
	unsigned pitch, slice, format;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_DB, 0, 0);
	rbuffer = &rtexture->resource;
	rtexture->tilled = 1;
	rtexture->array_mode = 2;
	rtexture->tile_type = 1;
	rtexture->depth = 1;

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	pitch = (rtexture->pitch[level] / rtexture->bpt) / 8 - 1;
	slice = (rtexture->pitch[level] / rtexture->bpt) * rtexture->height[level] / 64 - 1;
	format = r600_translate_dbformat(rbuffer->base.b.format);
	rstate->states[R600_DB__DB_DEPTH_BASE] = rtexture->offset[level] >> 8;
	rstate->states[R600_DB__DB_DEPTH_INFO] = S_028010_ARRAY_MODE(rtexture->array_mode) |
					S_028010_FORMAT(format);
	rstate->states[R600_DB__DB_DEPTH_VIEW] = 0x00000000;
	rstate->states[R600_DB__DB_PREFETCH_LIMIT] = (rtexture->height[level] / 8) -1;
	rstate->states[R600_DB__DB_DEPTH_SIZE] = S_028000_PITCH_TILE_MAX(pitch) |
						S_028000_SLICE_TILE_MAX(slice);
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rstate->nbo = 1;

	radeon_state_pm4(rstate);
}

int r600_texture_scissor(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);

	if (!rtexture->scissor[level].cpm4) {
		r600_texture_state_scissor(rscreen, rtexture, level);
	}
	return 0;
}

static void r600_texture_state_viewport(struct r600_screen *rscreen, struct r600_resource_texture *rtexture, unsigned level)
{
	struct radeon_state *rstate = &rtexture->viewport[level];

	radeon_state_init(rstate, rscreen->rw, R600_STATE_VIEWPORT, 0, 0);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XOFFSET_0] = fui((float)rtexture->width[level]/2.0);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XSCALE_0] = fui((float)rtexture->width[level]/2.0);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YOFFSET_0] = fui((float)rtexture->height[level]/2.0);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YSCALE_0] = fui((float)-rtexture->height[level]/2.0);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZOFFSET_0] = 0x3F000000;
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZSCALE_0] = 0x3F000000;
	rstate->states[R600_VIEWPORT__PA_CL_VTE_CNTL] = 0x0000043F;
	rstate->states[R600_VIEWPORT__PA_SC_VPORT_ZMAX_0] = 0x3F800000;

	radeon_state_pm4(rstate);
}

int r600_texture_cb(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned cb, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);

	if (!rtexture->cb[cb][level].cpm4) {
		r600_texture_state_cb(rscreen, rtexture, cb, level);
	}
	return 0;
}

int r600_texture_db(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);

	if (!rtexture->db[level].cpm4) {
		r600_texture_state_db(rscreen, rtexture, level);
	}
	return 0;
}

int r600_texture_viewport(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);

	if (!rtexture->viewport[level].cpm4) {
		r600_texture_state_viewport(rscreen, rtexture, level);
	}
	return 0;
}
