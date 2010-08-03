
/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_context.h"
#include "pipe/p_format.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "util/u_rect.h"
#include "util/u_blitter.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_util.h"
#include "nouveau/nouveau_screen.h"
#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_resource.h"
#include "nv04_2d.h"

#include <nouveau/nouveau_bo.h>

static INLINE void
nvfx_region_set_format(struct nv04_region* rgn, enum pipe_format format)
{
	unsigned bits = util_format_get_blocksizebits(format);
	switch(bits)
	{
	case 8:
		rgn->bpps = 0;
		break;
	case 16:
		rgn->bpps = 1;
		break;
	case 32:
		rgn->bpps = 2;
		break;
	default:
		assert(util_is_pot(bits));
		int shift = log2i(bits) - 3;
		assert(shift >= 2);
		rgn->bpps = 2;
		shift -= 2;

		rgn->x = util_format_get_nblocksx(format, rgn->x) << shift;
		rgn->y = util_format_get_nblocksy(format, rgn->y);
	}
}

static INLINE void
nvfx_region_fixup_swizzled(struct nv04_region* rgn, unsigned zslice, unsigned width, unsigned height, unsigned depth)
{
	// TODO: move this code to surface creation?
	if((depth <= 1) && (height <= 1 || width <= 2))
		rgn->pitch = width << rgn->bpps;
	else if(depth > 1 && height <= 2 && width <= 2)
	{
		rgn->pitch = width << rgn->bpps;
		rgn->offset += (zslice * width * height) << rgn->bpps;
	}
	else
	{
		rgn->pitch = 0;
		rgn->z = zslice;
		rgn->w = width;
		rgn->h = height;
		rgn->d = depth;
	}
}

static INLINE void
nvfx_region_init_for_surface(struct nv04_region* rgn, struct nvfx_surface* surf, unsigned x, unsigned y)
{
	rgn->bo = ((struct nvfx_resource*)surf->base.texture)->bo;
	rgn->offset = surf->base.offset;
	rgn->pitch = surf->pitch;
	rgn->x = x;
	rgn->y = y;
	rgn->z = 0;

	nvfx_region_set_format(rgn, surf->base.format);
	if(!(surf->base.texture->flags & NVFX_RESOURCE_FLAG_LINEAR))
		nvfx_region_fixup_swizzled(rgn, surf->base.zslice, surf->base.width, surf->base.height, u_minify(surf->base.texture->depth0, surf->base.level));
}

static INLINE void
nvfx_region_init_for_subresource(struct nv04_region* rgn, struct pipe_resource* pt, struct pipe_subresource sub, unsigned x, unsigned y, unsigned z)
{
	rgn->bo = ((struct nvfx_resource*)pt)->bo;
	rgn->offset = nvfx_subresource_offset(pt, sub.face, sub.level, z);
	rgn->pitch = nvfx_subresource_pitch(pt, sub.level);
	rgn->x = x;
	rgn->y = y;
	rgn->z = 0;

	nvfx_region_set_format(rgn, pt->format);
	if(!(pt->flags & NVFX_RESOURCE_FLAG_LINEAR))
		nvfx_region_fixup_swizzled(rgn, z, u_minify(pt->width0, sub.level), u_minify(pt->height0, sub.level), u_minify(pt->depth0, sub.level));
}

// TODO: actually test this for all formats, it's probably wrong for some...

static INLINE int
nvfx_surface_format(enum pipe_format format)
{
	switch(util_format_get_blocksize(format)) {
	case 1:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
	case 2:
		//return NV04_CONTEXT_SURFACES_2D_FORMAT_Y16;
		return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
	case 4:
		//if(format == PIPE_FORMAT_B8G8R8X8_UNORM || format == PIPE_FORMAT_B8G8R8A8_UNORM)
			return NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8;
		//else
		//	return NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;
	default:
		return -1;
	}
}

static INLINE int
nv04_scaled_image_format(enum pipe_format format)
{
	switch(util_format_get_blocksize(format)) {
	case 1:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_Y8;
	case 2:
		//if(format == PIPE_FORMAT_B5G5R5A1_UNORM)
		//	return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A1R5G5B5;
		//else
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_R5G6B5;
	case 4:
		if(format == PIPE_FORMAT_B8G8R8X8_UNORM)
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_X8R8G8B8;
		else
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}

static struct blitter_context*
nvfx_get_blitter(struct pipe_context* pipe, int copy)
{
	struct nvfx_context* nvfx = nvfx_context(pipe);

	struct blitter_context* blitter = nvfx->blitter;
	if(!blitter)
		nvfx->blitter = blitter = util_blitter_create(pipe);

	util_blitter_save_blend(blitter, nvfx->blend);
	util_blitter_save_depth_stencil_alpha(blitter, nvfx->zsa);
	util_blitter_save_stencil_ref(blitter, &nvfx->stencil_ref);
	util_blitter_save_rasterizer(blitter, nvfx->rasterizer);
	util_blitter_save_fragment_shader(blitter, nvfx->fragprog);
	util_blitter_save_vertex_shader(blitter, nvfx->vertprog);
	util_blitter_save_viewport(blitter, &nvfx->viewport);
	util_blitter_save_framebuffer(blitter, &nvfx->framebuffer);
	util_blitter_save_clip(blitter, &nvfx->clip);
	util_blitter_save_vertex_elements(blitter, nvfx->vtxelt);
	util_blitter_save_vertex_buffers(blitter, nvfx->vtxbuf_nr, nvfx->vtxbuf);

	if(copy)
	{
		util_blitter_save_fragment_sampler_states(blitter, nvfx->nr_samplers, (void**)nvfx->tex_sampler);
		util_blitter_save_fragment_sampler_views(blitter, nvfx->nr_textures, nvfx->fragment_sampler_views);
	}

	return blitter;
}

static unsigned
nvfx_region_clone(struct nv04_2d_context* ctx, struct nv04_region* rgn, unsigned w, unsigned h, boolean for_read)
{
	unsigned begin = nv04_region_begin(rgn, w, h);
	unsigned end = nv04_region_end(rgn, w, h);
	unsigned size = end - begin;
	struct nouveau_bo* bo = 0;
	nouveau_bo_new(rgn->bo->device, NOUVEAU_BO_MAP | NOUVEAU_BO_GART, 256, size, &bo);

	if(for_read || (size > ((w * h) << rgn->bpps)))
		nv04_memcpy(ctx, bo, 0, rgn->bo, rgn->offset + begin, size);

	rgn->bo = bo;
	rgn->offset = -begin;
	return begin;
}

static void
nvfx_resource_copy_region(struct pipe_context *pipe,
		  struct pipe_resource *dstr, struct pipe_subresource subdst,
		  unsigned dstx, unsigned dsty, unsigned dstz,
		  struct pipe_resource *srcr, struct pipe_subresource subsrc,
		  unsigned srcx, unsigned srcy, unsigned srcz,
		  unsigned w, unsigned h)
{
	struct nv04_2d_context *ctx = nvfx_screen(pipe->screen)->eng2d;
	struct nv04_region dst, src;

	if(!w || !h)
		return;

	static int copy_threshold = -1;
	if(copy_threshold < 0)
	{
		copy_threshold = debug_get_num_option("NOUVEAU_COPY_THRESHOLD", 0);
		if(copy_threshold < 0)
			copy_threshold = 0;
	}

	int dst_to_gpu = dstr->usage != PIPE_USAGE_DYNAMIC && dstr->usage != PIPE_USAGE_STAGING;
	int src_on_gpu = nvfx_resource_on_gpu(srcr);

	nvfx_region_init_for_subresource(&dst, dstr, subdst, dstx, dsty, dstz);
	nvfx_region_init_for_subresource(&src, srcr, subsrc, srcx, srcy, srcz);
	w = util_format_get_stride(dstr->format, w) >> dst.bpps;
	h = util_format_get_nblocksy(dstr->format, h);

	int ret;
	boolean small = (w * h <= copy_threshold);
	if((!dst_to_gpu || !src_on_gpu) && small)
		ret = -1; /* use the CPU */
	else
		ret = nv04_region_copy_2d(ctx, &dst, &src, w, h,
			dstr->target == PIPE_BUFFER ? -1 : nvfx_surface_format(dstr->format),
			dstr->target == PIPE_BUFFER ? -1 : nv04_scaled_image_format(dstr->format),
			dst_to_gpu, src_on_gpu);
	if(!ret)
	{}
	else if(ret > 0 && dstr->bind & PIPE_BIND_RENDER_TARGET && srcr->bind & PIPE_BIND_SAMPLER_VIEW)
	{
		struct blitter_context* blitter = nvfx_get_blitter(pipe, 1);
		util_blitter_copy_region(blitter, dstr, subdst, dstx, dsty, dstz, srcr, subsrc, srcx, srcy, srcz, w, h, TRUE);
	}
	else
	{
		struct nv04_region dstt = dst;
		struct nv04_region srct = src;
		unsigned dstbegin = 0;

		if(!small)
		{
			if(src_on_gpu)
				nvfx_region_clone(ctx, &srct, w, h, TRUE);

			if(dst_to_gpu)
				dstbegin = nvfx_region_clone(ctx, &dstt, w, h, FALSE);
		}

		nv04_region_copy_cpu(&dstt, &srct, w, h);

		if(srct.bo != src.bo)
			nouveau_screen_bo_release(pipe->screen, srct.bo);

		if(dstt.bo != dst.bo)
		{
			nv04_memcpy(ctx, dst.bo, dst.offset + dstbegin, dstt.bo, 0, dstt.bo->size);
			nouveau_screen_bo_release(pipe->screen, dstt.bo);
		}
	}
}

static int
nvfx_surface_fill(struct pipe_context* pipe, struct pipe_surface *dsts,
		  unsigned dx, unsigned dy, unsigned w, unsigned h, unsigned value)
{
	struct nv04_2d_context *ctx = nvfx_screen(pipe->screen)->eng2d;
	struct nv04_region dst;
	/* Always try to use the GPU right now, if possible
	 * If the user wanted the surface data on the CPU, he would have cleared with memset */

	// we don't care about interior pixel order since we set all them to the same value
	nvfx_region_init_for_surface(&dst, (struct nvfx_surface*)dsts, dx, dy);
	w = util_format_get_stride(dsts->format, w) >> dst.bpps;
	h = util_format_get_nblocksy(dsts->format, h);

	int ret = nv04_region_fill_2d(ctx, &dst, w, h, value);
	if(ret > 0 && dsts->texture->bind & PIPE_BIND_RENDER_TARGET)
		return 1;
	else if(ret)
	{
		struct nv04_region dstt = dst;
		unsigned dstbegin = 0;

		if(nvfx_resource_on_gpu(dsts->texture))
			dstbegin = nvfx_region_clone(ctx, &dstt, w, h, FALSE);

		nv04_region_fill_cpu(&dstt, w, h, value);

		if(dstt.bo != dst.bo)
		{
			nv04_memcpy(ctx, dst.bo, dst.offset + dstbegin, dstt.bo, 0, dstt.bo->size);
			nouveau_screen_bo_release(pipe->screen, dstt.bo);
		}
	}

	return 0;
}


void
nvfx_screen_surface_takedown(struct pipe_screen *pscreen)
{
	nv04_2d_context_takedown(nvfx_screen(pscreen)->eng2d);
	nvfx_screen(pscreen)->eng2d = 0;
}

int
nvfx_screen_surface_init(struct pipe_screen *pscreen)
{
	struct nv04_2d_context* ctx = nv04_2d_context_init(nouveau_screen(pscreen)->channel);
	if(!ctx)
		return -1;
	nvfx_screen(pscreen)->eng2d = ctx;
	return 0;
}

static void
nvfx_clear_render_target(struct pipe_context *pipe,
			 struct pipe_surface *dst,
			 const float *rgba,
			 unsigned dstx, unsigned dsty,
			 unsigned width, unsigned height)
{
	union util_color uc;
	util_pack_color(rgba, dst->format, &uc);

	if(util_format_get_blocksizebits(dst->format) > 32
		|| nvfx_surface_fill(pipe, dst, dstx, dsty, width, height, uc.ui))
	{
		// TODO: probably should use hardware clear here instead if possible
		struct blitter_context* blitter = nvfx_get_blitter(pipe, 0);
		util_blitter_clear_render_target(blitter, dst, rgba, dstx, dsty, width, height);
	}
}

static void
nvfx_clear_depth_stencil(struct pipe_context *pipe,
			 struct pipe_surface *dst,
			 unsigned clear_flags,
			 double depth,
			 unsigned stencil,
			 unsigned dstx, unsigned dsty,
			 unsigned width, unsigned height)
{
	if(util_format_get_blocksizebits(dst->format) > 32
		|| nvfx_surface_fill(pipe, dst, dstx, dsty, width, height, util_pack_z_stencil(dst->format, depth, stencil)))
	{
		// TODO: probably should use hardware clear here instead if possible
		struct blitter_context* blitter = nvfx_get_blitter(pipe, 0);
		util_blitter_clear_depth_stencil(blitter, dst, clear_flags, depth, stencil, dstx, dsty, width, height);
	}
}


void
nvfx_init_surface_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.resource_copy_region = nvfx_resource_copy_region;
	nvfx->pipe.clear_render_target = nvfx_clear_render_target;
	nvfx->pipe.clear_depth_stencil = nvfx_clear_depth_stencil;
}
