
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
#include "util/u_blitter.h"
#include "util/u_surface.h"

#include "nouveau/nouveau_winsys.h"
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
	unsigned shift = 0;
	rgn->one_bits = 0;

	switch(bits)
	{
	case 8:
		rgn->bpps = 0;
		break;
	case 16:
		rgn->bpps = 1;
		if(format == PIPE_FORMAT_B5G5R5X1_UNORM)
			rgn->one_bits = 1;
		break;
	case 32:
		rgn->bpps = 2;
		if(format == PIPE_FORMAT_R8G8B8X8_UNORM || format == PIPE_FORMAT_B8G8R8X8_UNORM)
			rgn->one_bits = 8;
		break;
	case 64:
		rgn->bpps = 2;
		shift = 1;
		break;
	case 128:
		rgn->bpps = 2;
		shift = 2;
		break;
	}

	if(shift) {
		rgn->x = util_format_get_nblocksx(format, rgn->x) << shift;
		rgn->y = util_format_get_nblocksy(format, rgn->y);
		rgn->w <<= shift;
	}
}

static INLINE void
nvfx_region_init_for_surface(struct nv04_region* rgn, struct nvfx_surface* surf, unsigned x, unsigned y, boolean for_write)
{
	rgn->x = x;
	rgn->y = y;
	rgn->z = 0;

	if(surf->temp)
	{
		rgn->bo = surf->temp->base.bo;
		rgn->offset = 0;
		rgn->pitch = surf->temp->linear_pitch;

		if(for_write)
			util_dirty_surface_set_dirty(nvfx_surface_get_dirty_surfaces(&surf->base.base), &surf->base);
	} else {
		rgn->bo = ((struct nvfx_resource*)surf->base.base.texture)->bo;
		rgn->offset = surf->offset;

		if(surf->base.base.texture->flags & NVFX_RESOURCE_FLAG_LINEAR)
			rgn->pitch = surf->pitch;
	        else
	        {
		        rgn->pitch = 0;
		        rgn->z = surf->base.base.u.tex.first_layer;
		        rgn->w = surf->base.base.width;
		        rgn->h = surf->base.base.height;
		        rgn->d = u_minify(surf->base.base.texture->depth0, surf->base.base.u.tex.level);
	        }
	}

	nvfx_region_set_format(rgn, surf->base.base.format);
	if(!rgn->pitch)
		nv04_region_try_to_linearize(rgn);
}

static INLINE void
nvfx_region_init_for_subresource(struct nv04_region* rgn, struct pipe_resource* pt, unsigned level, unsigned x, unsigned y, unsigned z, boolean for_write)
{
	if(pt->target != PIPE_BUFFER)
	{
		struct nvfx_surface* ns = (struct nvfx_surface*)util_surfaces_peek(&((struct nvfx_miptree*)pt)->surfaces, pt, level, z);
		if(ns && util_dirty_surface_is_dirty(&ns->base))
		{
			nvfx_region_init_for_surface(rgn, ns, x, y, for_write);
			return;
		}
	}

	rgn->bo = ((struct nvfx_resource*)pt)->bo;
	rgn->offset = nvfx_subresource_offset(pt, z, level, z);
	rgn->x = x;
	rgn->y = y;

	if(pt->flags & NVFX_RESOURCE_FLAG_LINEAR)
	{
		rgn->pitch = nvfx_subresource_pitch(pt, level);
		rgn->z = 0;
	}
	else
	{
		rgn->pitch = 0;
		rgn->z = z;
		rgn->w = u_minify(pt->width0, level);
		rgn->h = u_minify(pt->height0, level);
		rgn->d = u_minify(pt->depth0, level);
	}

	nvfx_region_set_format(rgn, pt->format);
	if(!rgn->pitch)
		nv04_region_try_to_linearize(rgn);
}

// don't save index buffer because blitter doesn't setit
static struct blitter_context*
nvfx_get_blitter(struct pipe_context* pipe, int copy)
{
	struct nvfx_context* nvfx = nvfx_context(pipe);
	struct blitter_context** pblitter;
	struct blitter_context* blitter;

	assert(nvfx->blitters_in_use < Elements(nvfx->blitter));

	if(nvfx->query && !nvfx->blitters_in_use)
	{
		struct nouveau_channel* chan = nvfx->screen->base.channel;
		struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
		BEGIN_RING(chan, eng3d, NV30_3D_QUERY_ENABLE, 1);
		OUT_RING(chan, 0);
	}

	pblitter = &nvfx->blitter[nvfx->blitters_in_use++];
	if(!*pblitter)
		*pblitter = util_blitter_create(pipe);
	blitter = *pblitter;

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

static inline void
nvfx_put_blitter(struct pipe_context* pipe, struct blitter_context* blitter)
{
	struct nvfx_context* nvfx = nvfx_context(pipe);
	--nvfx->blitters_in_use;
	assert(nvfx->blitters_in_use >= 0);

	if(nvfx->query && !nvfx->blitters_in_use)
	{
		struct nouveau_channel* chan = nvfx->screen->base.channel;
		struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
		BEGIN_RING(chan, eng3d, NV30_3D_QUERY_ENABLE, 1);
		OUT_RING(chan, 1);
	}
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
			  struct pipe_resource *dstr, unsigned dst_level,
			  unsigned dstx, unsigned dsty, unsigned dstz,
			  struct pipe_resource *srcr, unsigned src_level,
			  const struct pipe_box *src_box)
{
	static int copy_threshold = -1;
	struct nv04_2d_context *ctx = nvfx_screen(pipe->screen)->eng2d;
	struct nv04_region dst, src;
	int dst_to_gpu;
	int src_on_gpu;
	boolean small;
	int ret;
	unsigned w = src_box->width;
	unsigned h = src_box->height;

	if(!w || !h)
		return;

        /* Fallback for buffers. */
        if (dstr->target == PIPE_BUFFER && srcr->target == PIPE_BUFFER) {
                util_resource_copy_region(pipe, dstr, dst_level, dstx, dsty, dstz,
                                          srcr, src_level, src_box);
                return;
        }

	if(copy_threshold < 0)
		copy_threshold = debug_get_num_option("NOUVEAU_COPY_THRESHOLD", 4);

	dst_to_gpu = dstr->usage != PIPE_USAGE_DYNAMIC && dstr->usage != PIPE_USAGE_STAGING;
	src_on_gpu = nvfx_resource_on_gpu(srcr);

	nvfx_region_init_for_subresource(&dst, dstr, dst_level, dstx, dsty, dstz, TRUE);
	nvfx_region_init_for_subresource(&src, srcr, src_level, src_box->x, src_box->y, src_box->z, FALSE);
	w = util_format_get_stride(dstr->format, w) >> dst.bpps;
	h = util_format_get_nblocksy(dstr->format, h);

	small = (w * h <= copy_threshold);
	if((!dst_to_gpu || !src_on_gpu) && small)
		ret = -1; /* use the CPU */
	else
		ret = nv04_region_copy_2d(ctx, &dst, &src, w, h, dst_to_gpu, src_on_gpu);
	if(!ret)
	{}
	else if(ret > 0
			&& dstr->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL)
			&& srcr->bind & PIPE_BIND_SAMPLER_VIEW)
	{
		/* this currently works because we hack the bind flags on resource creation to be
		 * the maximum set that the resource type actually supports
		 *
		 * TODO: perhaps support reinterpreting the formats
		 */
		struct blitter_context* blitter = nvfx_get_blitter(pipe, 1);
		util_blitter_copy_region(blitter, dstr, dst_level, dstx, dsty, dstz, srcr, src_level, src_box, TRUE);
		nvfx_put_blitter(pipe, blitter);
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
	int ret;
	/* Always try to use the GPU right now, if possible
	 * If the user wanted the surface data on the CPU, he would have cleared with memset (hopefully) */

	// we don't care about interior pixel order since we set all them to the same value
	nvfx_region_init_for_surface(&dst, (struct nvfx_surface*)dsts, dx, dy, TRUE);

	w = util_format_get_stride(dsts->format, w) >> dst.bpps;
	h = util_format_get_nblocksy(dsts->format, h);

	ret = nv04_region_fill_2d(ctx, &dst, w, h, value);
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
nvfx_surface_copy_temp(struct pipe_context* pipe, struct pipe_surface* surf, int to_temp)
{
	struct nvfx_surface* ns = (struct nvfx_surface*)surf;
	struct pipe_box box;
	struct nvfx_context* nvfx = nvfx_context(pipe);
	struct nvfx_miptree* temp;
	unsigned use_vertex_buffers;
	boolean use_index_buffer;
	unsigned base_vertex;

	/* temporarily detach the temp, so it isn't used in place of the actual resource */
	temp = ns->temp;
	ns->temp = 0;

	// TODO: we really should do this validation before setting these variable in draw calls
	use_vertex_buffers = nvfx->use_vertex_buffers;
	use_index_buffer = nvfx->use_index_buffer;
	base_vertex = nvfx->base_vertex;

	box.x = box.y = 0;
	assert(surf->u.tex.first_layer == surf->u.tex.last_layer);
	box.width = surf->width;
	box.height = surf->height;
	box.depth = 1;

	if(to_temp) {
	        box.z = surf->u.tex.first_layer;
		nvfx_resource_copy_region(pipe, &temp->base.base, 0, 0, 0, 0, surf->texture, surf->u.tex.level, &box);
	}
	else {
		box.z = 0;
		nvfx_resource_copy_region(pipe, surf->texture, surf->u.tex.level, 0, 0, surf->u.tex.first_layer, &temp->base.base, 0, &box);
	}

	/* If this triggers, it probably means we attempted to use the blitter
	 * but failed due to non-renderability of the target.
	 * Obviously, this would lead to infinite recursion if supported. */
	assert(!ns->temp);

	ns->temp = temp;

	nvfx->use_vertex_buffers = use_vertex_buffers;
	nvfx->use_index_buffer = use_index_buffer;
        nvfx->base_vertex = base_vertex;

	nvfx->dirty |= NVFX_NEW_ARRAYS;
	nvfx->draw_dirty |= NVFX_NEW_ARRAYS;
}

void
nvfx_surface_create_temp(struct pipe_context* pipe, struct pipe_surface* surf)
{
	struct nvfx_surface* ns = (struct nvfx_surface*)surf;
	struct pipe_resource template;
	memset(&template, 0, sizeof(struct pipe_resource));
	template.target = PIPE_TEXTURE_2D;
	template.format = surf->format;
	template.width0 = surf->width;
	template.height0 = surf->height;
	template.depth0 = 1;
	template.nr_samples = surf->texture->nr_samples;
	template.flags = NVFX_RESOURCE_FLAG_LINEAR;

	assert(!ns->temp && !util_dirty_surface_is_dirty(&ns->base));

	ns->temp = (struct nvfx_miptree*)nvfx_miptree_create(pipe->screen, &template);
	nvfx_surface_copy_temp(pipe, surf, 1);
}

void
nvfx_surface_flush(struct pipe_context* pipe, struct pipe_surface* surf)
{
	struct nvfx_context* nvfx = (struct nvfx_context*)pipe;
	struct nvfx_surface* ns = (struct nvfx_surface*)surf;
	boolean bound = FALSE;

	nvfx_surface_copy_temp(pipe, surf, 0);

	util_dirty_surface_set_clean(nvfx_surface_get_dirty_surfaces(surf), &ns->base);

	if(nvfx->framebuffer.zsbuf == surf)
		bound = TRUE;
	else
	{
		for(unsigned i = 0; i < nvfx->framebuffer.nr_cbufs; ++i)
		{
			if(nvfx->framebuffer.cbufs[i] == surf)
			{
				bound = TRUE;
				break;
			}
		}
	}

	if(!bound)
		pipe_resource_reference((struct pipe_resource**)&ns->temp, 0);
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
		nvfx_put_blitter(pipe, blitter);
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
		nvfx_put_blitter(pipe, blitter);
	}
}


void
nvfx_init_surface_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.resource_copy_region = nvfx_resource_copy_region;
	nvfx->pipe.clear_render_target = nvfx_clear_render_target;
	nvfx->pipe.clear_depth_stencil = nvfx_clear_depth_stencil;
}
