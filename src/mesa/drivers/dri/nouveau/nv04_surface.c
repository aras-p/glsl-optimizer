/*
 * Copyright (C) 2007-2010 The Nouveau Project.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_class.h"
#include "nouveau_context.h"
#include "nouveau_util.h"
#include "nv04_driver.h"

static inline int
swzsurf_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
	case MESA_FORMAT_CI8:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_Y8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_A8R8G8B8;

	default:
		assert(0);
	}
}

static inline int
surf2d_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
	case MESA_FORMAT_CI8:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;

	default:
		assert(0);
	}
}

static inline int
rect_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
	case MESA_FORMAT_CI8:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;

	default:
		assert(0);
	}
}

static inline int
sifm_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
	case MESA_FORMAT_CI8:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_AY8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A8R8G8B8;

	default:
		assert(0);
	}
}

static void
nv04_surface_copy_swizzle(GLcontext *ctx,
			  struct nouveau_surface *dst,
			  struct nouveau_surface *src,
			  int dx, int dy, int sx, int sy,
			  int w, int h)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_grobj *swzsurf = hw->swzsurf;
	struct nouveau_grobj *sifm = hw->sifm;
	struct nouveau_bo_context *bctx = context_bctx(ctx, SURFACE);
	const unsigned bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART;
	/* Max width & height may not be the same on all HW, but must be POT */
	const unsigned max_w = 1024;
	const unsigned max_h = 1024;
	unsigned sub_w = w > max_w ? max_w : w;
	unsigned sub_h = h > max_h ? max_h : h;
	unsigned x, y;

        /* Swizzled surfaces must be POT  */
	assert(_mesa_is_pow_two(dst->width) &&
	       _mesa_is_pow_two(dst->height));

        /* If area is too large to copy in one shot we must copy it in
	 * POT chunks to meet alignment requirements */
	assert(sub_w == w || _mesa_is_pow_two(w));
	assert(sub_h == h || _mesa_is_pow_two(h));

	nouveau_bo_marko(bctx, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_DMA_IMAGE,
			 src->bo, bo_flags | NOUVEAU_BO_RD);
	nouveau_bo_marko(bctx, swzsurf, NV04_SWIZZLED_SURFACE_DMA_IMAGE,
			 dst->bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	nouveau_bo_markl(bctx, swzsurf, NV04_SWIZZLED_SURFACE_OFFSET,
			 dst->bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_FORMAT, 1);
	OUT_RING  (chan, swzsurf_format(dst->format) |
		   log2i(dst->width) << 16 |
		   log2i(dst->height) << 24);

	BEGIN_RING(chan, sifm, NV04_SCALED_IMAGE_FROM_MEMORY_SURFACE, 1);
	OUT_RING  (chan, swzsurf->handle);

	for (y = 0; y < h; y += sub_h) {
		sub_h = MIN2(sub_h, h - y);

		for (x = 0; x < w; x += sub_w) {
			sub_w = MIN2(sub_w, w - x);

			MARK_RING(chan, 15, 1);

			BEGIN_RING(chan, sifm,
				   NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT, 8);
			OUT_RING(chan, sifm_format(src->format));
			OUT_RING(chan, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION_SRCCOPY);
			OUT_RING(chan, (y + dy) << 16 | (x + dx));
			OUT_RING(chan, sub_h << 16 | sub_w);
			OUT_RING(chan, (y + dy) << 16 | (x + dx));
			OUT_RING(chan, sub_h << 16 | sub_w);
			OUT_RING(chan, 1 << 20);
			OUT_RING(chan, 1 << 20);

			BEGIN_RING(chan, sifm,
				   NV03_SCALED_IMAGE_FROM_MEMORY_SIZE, 4);
			OUT_RING(chan, sub_h << 16 | sub_w);
			OUT_RING(chan, src->pitch  |
				 NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_ORIGIN_CENTER |
				 NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_FILTER_POINT_SAMPLE);
			OUT_RELOCl(chan, src->bo, src->offset +
				   (y + sy) * src->pitch +
				   (x + sx) * src->cpp,
				   bo_flags | NOUVEAU_BO_RD);
			OUT_RING(chan, 0);
		}
	}

	nouveau_bo_context_reset(bctx);

	if (context_chipset(ctx) < 0x10)
		FIRE_RING(chan);
}

static void
nv04_surface_copy_m2mf(GLcontext *ctx,
		       struct nouveau_surface *dst,
		       struct nouveau_surface *src,
		       int dx, int dy, int sx, int sy,
		       int w, int h)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_grobj *m2mf = hw->m2mf;
	struct nouveau_bo_context *bctx = context_bctx(ctx, SURFACE);
	const unsigned bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART;
	unsigned dst_offset = dst->offset + dy * dst->pitch + dx * dst->cpp;
	unsigned src_offset = src->offset + sy * src->pitch + sx * src->cpp;

	nouveau_bo_marko(bctx, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN,
			 src->bo, bo_flags | NOUVEAU_BO_RD);
	nouveau_bo_marko(bctx, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_OUT,
			 dst->bo, bo_flags | NOUVEAU_BO_WR);

	while (h) {
		int count = (h > 2047) ? 2047 : h;

		MARK_RING(chan, 9, 2);

		BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RELOCl(chan, src->bo, src_offset,
			   bo_flags | NOUVEAU_BO_RD);
		OUT_RELOCl(chan, dst->bo, dst_offset,
			   bo_flags | NOUVEAU_BO_WR);
		OUT_RING  (chan, src->pitch);
		OUT_RING  (chan, dst->pitch);
		OUT_RING  (chan, w * src->cpp);
		OUT_RING  (chan, count);
		OUT_RING  (chan, 0x0101);
		OUT_RING  (chan, 0);

		h -= count;
		src_offset += src->pitch * count;
		dst_offset += dst->pitch * count;
	}

	nouveau_bo_context_reset(bctx);

	if (context_chipset(ctx) < 0x10)
		FIRE_RING(chan);
}

typedef unsigned (*get_offset_t)(struct nouveau_surface *s,
				 unsigned x, unsigned y);

static unsigned
get_linear_offset(struct nouveau_surface *s, unsigned x, unsigned y)
{
	return x * s->cpp + y * s->pitch;
}

static unsigned
get_swizzled_offset(struct nouveau_surface *s, unsigned x, unsigned y)
{
	unsigned k = log2i(MIN2(s->width, s->height));

	unsigned u = (x & 0x001) << 0 |
		(x & 0x002) << 1 |
		(x & 0x004) << 2 |
		(x & 0x008) << 3 |
		(x & 0x010) << 4 |
		(x & 0x020) << 5 |
		(x & 0x040) << 6 |
		(x & 0x080) << 7 |
		(x & 0x100) << 8 |
		(x & 0x200) << 9 |
		(x & 0x400) << 10 |
		(x & 0x800) << 11;

	unsigned v = (y & 0x001) << 1 |
		(y & 0x002) << 2 |
		(y & 0x004) << 3 |
		(y & 0x008) << 4 |
		(y & 0x010) << 5 |
		(y & 0x020) << 6 |
		(y & 0x040) << 7 |
		(y & 0x080) << 8 |
		(y & 0x100) << 9 |
		(y & 0x200) << 10 |
		(y & 0x400) << 11 |
		(y & 0x800) << 12;

	return s->cpp * (((u | v) & ~(~0 << 2*k)) |
			 (x & (~0 << k)) << k |
			 (y & (~0 << k)) << k);
}

static void
nv04_surface_copy_cpu(GLcontext *ctx,
		      struct nouveau_surface *dst,
		      struct nouveau_surface *src,
		      int dx, int dy, int sx, int sy,
		      int w, int h)
{
	int x, y;
	get_offset_t get_dst = (dst->layout == SWIZZLED ?
				get_swizzled_offset : get_linear_offset);
	get_offset_t get_src = (src->layout == SWIZZLED ?
				get_swizzled_offset : get_linear_offset);
	void *dp, *sp;

	nouveau_bo_map(dst->bo, NOUVEAU_BO_WR);
	nouveau_bo_map(src->bo, NOUVEAU_BO_RD);

	dp = dst->bo->map + dst->offset;
	sp = src->bo->map + src->offset;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			memcpy(dp + get_dst(dst, dx + x, dy + y),
			       sp + get_src(src, sx + x, sy + y), dst->cpp);
		}
	}

	nouveau_bo_unmap(src->bo);
	nouveau_bo_unmap(dst->bo);
}

void
nv04_surface_copy(GLcontext *ctx,
		  struct nouveau_surface *dst,
		  struct nouveau_surface *src,
		  int dx, int dy, int sx, int sy,
		  int w, int h)
{
	/* Linear texture copy. */
	if ((src->layout == LINEAR && dst->layout == LINEAR) ||
	    dst->width <= 2 || dst->height <= 1) {
		nv04_surface_copy_m2mf(ctx, dst, src, dx, dy, sx, sy, w, h);
		return;
	}

	/* Swizzle using sifm+swzsurf. */
        if (src->layout == LINEAR && dst->layout == SWIZZLED &&
	    dst->cpp != 1 && !(dst->offset & 63)) {
		nv04_surface_copy_swizzle(ctx, dst, src, dx, dy, sx, sy, w, h);
		return;
	}

	/* Fallback to CPU copy. */
	nv04_surface_copy_cpu(ctx, dst, src, dx, dy, sx, sy, w, h);
}

void
nv04_surface_fill(GLcontext *ctx,
		  struct nouveau_surface *dst,
		  unsigned mask, unsigned value,
		  int dx, int dy, int w, int h)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_grobj *surf2d = hw->surf2d;
	struct nouveau_grobj *patt = hw->patt;
	struct nouveau_grobj *rect = hw->rect;
	unsigned bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART;

	MARK_RING (chan, 19, 4);

	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, dst->bo, bo_flags | NOUVEAU_BO_WR);
	OUT_RELOCo(chan, dst->bo, bo_flags | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, surf2d_format(dst->format));
	OUT_RING  (chan, (dst->pitch << 16) | dst->pitch);
	OUT_RELOCl(chan, dst->bo, dst->offset, bo_flags | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, dst->bo, dst->offset, bo_flags | NOUVEAU_BO_WR);

	BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_COLOR_FORMAT, 1);
	OUT_RING  (chan, rect_format(dst->format));
	BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_MONOCHROME_COLOR1, 1);
	OUT_RING  (chan, mask | ~0ll << (8 * dst->cpp));

	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
	OUT_RING  (chan, rect_format(dst->format));
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
	OUT_RING  (chan, value);
	BEGIN_RING(chan, rect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (dx << 16) | dy);
	OUT_RING  (chan, ( w << 16) |  h);

	if (context_chipset(ctx) < 0x10)
		FIRE_RING(chan);
}

void
nv04_surface_takedown(GLcontext *ctx)
{
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;

	nouveau_grobj_free(&hw->swzsurf);
	nouveau_grobj_free(&hw->sifm);
	nouveau_grobj_free(&hw->rect);
	nouveau_grobj_free(&hw->rop);
	nouveau_grobj_free(&hw->patt);
	nouveau_grobj_free(&hw->surf2d);
	nouveau_grobj_free(&hw->m2mf);
	nouveau_notifier_free(&hw->ntfy);
}

GLboolean
nv04_surface_init(GLcontext *ctx)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	unsigned handle = 0x88000000, class;
	int ret;

	/* Notifier object. */
	ret = nouveau_notifier_alloc(chan, handle++, 1, &hw->ntfy);
	if (ret)
		goto fail;

	/* Memory to memory format. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_MEMORY_TO_MEMORY_FORMAT,
				  &hw->m2mf);
	if (ret)
		goto fail;

	BEGIN_RING(chan, hw->m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (chan, hw->ntfy->handle);

	/* Context surfaces 2D. */
	if (context_chipset(ctx) < 0x10)
		class = NV04_CONTEXT_SURFACES_2D;
	else
		class = NV10_CONTEXT_SURFACES_2D;

	ret = nouveau_grobj_alloc(chan, handle++, class, &hw->surf2d);
	if (ret)
		goto fail;

	/* Raster op. */
	ret = nouveau_grobj_alloc(chan, handle++, NV03_CONTEXT_ROP, &hw->rop);
	if (ret)
		goto fail;

	BEGIN_RING(chan, hw->rop, NV03_CONTEXT_ROP_DMA_NOTIFY, 1);
	OUT_RING  (chan, hw->ntfy->handle);

	BEGIN_RING(chan, hw->rop, NV03_CONTEXT_ROP_ROP, 1);
	OUT_RING  (chan, 0xca); /* DPSDxax in the GDI speech. */

	/* Image pattern. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_IMAGE_PATTERN,
				  &hw->patt);
	if (ret)
		goto fail;

	BEGIN_RING(chan, hw->patt, NV04_IMAGE_PATTERN_DMA_NOTIFY, 1);
	OUT_RING  (chan, hw->ntfy->handle);

	BEGIN_RING(chan, hw->patt, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT, 3);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT_LE);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_SHAPE_8X8);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_PATTERN_SELECT_MONO);

	BEGIN_RING(chan, hw->patt, NV04_IMAGE_PATTERN_MONOCHROME_COLOR0, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, ~0);
	OUT_RING  (chan, ~0);

	/* GDI rectangle text. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_GDI_RECTANGLE_TEXT,
				  &hw->rect);
	if (ret)
		goto fail;

	BEGIN_RING(chan, hw->rect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, hw->ntfy->handle);
	BEGIN_RING(chan, hw->rect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, hw->surf2d->handle);
	BEGIN_RING(chan, hw->rect, NV04_GDI_RECTANGLE_TEXT_ROP, 1);
	OUT_RING  (chan, hw->rop->handle);
	BEGIN_RING(chan, hw->rect, NV04_GDI_RECTANGLE_TEXT_PATTERN, 1);
	OUT_RING  (chan, hw->patt->handle);

	BEGIN_RING(chan, hw->rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_ROP_AND);
	BEGIN_RING(chan, hw->rect,
		   NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	/* Swizzled surface. */
	if (context_chipset(ctx) < 0x20)
		class = NV04_SWIZZLED_SURFACE;
	else
		class = NV20_SWIZZLED_SURFACE;

	ret = nouveau_grobj_alloc(chan, handle++, class, &hw->swzsurf);
	if (ret)
		goto fail;

	/* Scaled image from memory. */
	if  (context_chipset(ctx) < 0x10)
		class = NV04_SCALED_IMAGE_FROM_MEMORY;
	else
		class = NV10_SCALED_IMAGE_FROM_MEMORY;

	ret = nouveau_grobj_alloc(chan, handle++, class, &hw->sifm);
	if (ret)
		goto fail;

	if (context_chipset(ctx) >= 0x10) {
		BEGIN_RING(chan, hw->sifm,
			   NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION, 1);
		OUT_RING(chan, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_TRUNCATE);
	}

	return GL_TRUE;

fail:
	nv04_surface_takedown(ctx);
	return GL_FALSE;
}
