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
	struct nouveau_screen *screen = to_nouveau_context(ctx)->screen;
	struct nouveau_grobj *swzsurf = screen->swzsurf;
	struct nouveau_grobj *sifm = screen->sifm;
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
	assert(sub_w == w || _mesa_is_pow_two(sub_w));
	assert(sub_h == h || _mesa_is_pow_two(sub_h));

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
			/* Must be 64-byte aligned */
			assert(!(dst->offset & 63));

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
	struct nouveau_screen *screen = to_nouveau_context(ctx)->screen;
	struct nouveau_grobj *m2mf = screen->m2mf;
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

void
nv04_surface_copy(GLcontext *ctx,
		  struct nouveau_surface *dst,
		  struct nouveau_surface *src,
		  int dx, int dy, int sx, int sy,
		  int w, int h)
{
	/* Setup transfer to swizzle the texture to vram if needed */
        if (src->layout != SWIZZLED &&
	    dst->layout == SWIZZLED &&
	    dst->width > 2 && dst->height > 1) {
		nv04_surface_copy_swizzle(ctx, dst, src,
					  dx, dy, sx, sy, w, h);
		return;
	}

	nv04_surface_copy_m2mf(ctx, dst, src, dx, dy, sx, sy, w, h);
}

void
nv04_surface_fill(GLcontext *ctx,
		  struct nouveau_surface *dst,
		  unsigned mask, unsigned value,
		  int dx, int dy, int w, int h)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_screen *screen = to_nouveau_context(ctx)->screen;
	struct nouveau_grobj *surf2d = screen->surf2d;
	struct nouveau_grobj *patt = screen->patt;
	struct nouveau_grobj *rect = screen->rect;
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
	OUT_RING  (chan, mask | ~0 << (8 * dst->cpp));

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
nv04_surface_takedown(struct nouveau_screen *screen)
{
	nouveau_grobj_free(&screen->swzsurf);
	nouveau_grobj_free(&screen->sifm);
	nouveau_grobj_free(&screen->rect);
	nouveau_grobj_free(&screen->rop);
	nouveau_grobj_free(&screen->patt);
	nouveau_grobj_free(&screen->surf2d);
	nouveau_grobj_free(&screen->m2mf);
	nouveau_notifier_free(&screen->ntfy);
}

GLboolean
nv04_surface_init(struct nouveau_screen *screen)
{
	struct nouveau_channel *chan = screen->chan;
	const unsigned chipset = screen->device->chipset;
	unsigned handle = 0x88000000, class;
	int ret;

	/* Notifier object. */
	ret = nouveau_notifier_alloc(chan, handle++, 1, &screen->ntfy);
	if (ret)
		goto fail;

	/* Memory to memory format. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_MEMORY_TO_MEMORY_FORMAT,
				  &screen->m2mf);
	if (ret)
		goto fail;

	BEGIN_RING(chan, screen->m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->ntfy->handle);

	/* Context surfaces 2D. */
	if (chan->device->chipset < 0x10)
		class = NV04_CONTEXT_SURFACES_2D;
	else
		class = NV10_CONTEXT_SURFACES_2D;

	ret = nouveau_grobj_alloc(chan, handle++, class, &screen->surf2d);
	if (ret)
		goto fail;

	/* Raster op. */
	ret = nouveau_grobj_alloc(chan, handle++, NV03_CONTEXT_ROP,
				  &screen->rop);
	if (ret)
		goto fail;

	BEGIN_RING(chan, screen->rop, NV03_CONTEXT_ROP_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->ntfy->handle);

	BEGIN_RING(chan, screen->rop, NV03_CONTEXT_ROP_ROP, 1);
	OUT_RING  (chan, 0xca); /* DPSDxax in the GDI speech. */

	/* Image pattern. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_IMAGE_PATTERN,
				  &screen->patt);
	if (ret)
		goto fail;

	BEGIN_RING(chan, screen->patt,
		   NV04_IMAGE_PATTERN_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->ntfy->handle);

	BEGIN_RING(chan, screen->patt,
		   NV04_IMAGE_PATTERN_MONOCHROME_FORMAT, 3);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT_LE);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_SHAPE_8X8);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_PATTERN_SELECT_MONO);

	BEGIN_RING(chan, screen->patt,
		   NV04_IMAGE_PATTERN_MONOCHROME_COLOR0, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, ~0);
	OUT_RING  (chan, ~0);

	/* GDI rectangle text. */
	ret = nouveau_grobj_alloc(chan, handle++, NV04_GDI_RECTANGLE_TEXT,
				  &screen->rect);
	if (ret)
		goto fail;

	BEGIN_RING(chan, screen->rect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->ntfy->handle);
	BEGIN_RING(chan, screen->rect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, screen->surf2d->handle);
	BEGIN_RING(chan, screen->rect, NV04_GDI_RECTANGLE_TEXT_ROP, 1);
	OUT_RING  (chan, screen->rop->handle);
	BEGIN_RING(chan, screen->rect, NV04_GDI_RECTANGLE_TEXT_PATTERN, 1);
	OUT_RING  (chan, screen->patt->handle);

	BEGIN_RING(chan, screen->rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_ROP_AND);
	BEGIN_RING(chan, screen->rect,
		   NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	/* Swizzled surface. */
	switch (chan->device->chipset & 0xf0) {
	case 0x00:
	case 0x10:
		class = NV04_SWIZZLED_SURFACE;
		break;
	case 0x20:
		class = NV20_SWIZZLED_SURFACE;
		break;
	case 0x30:
		class = NV30_SWIZZLED_SURFACE;
		break;
	case 0x40:
	case 0x60:
		class = NV40_SWIZZLED_SURFACE;
		break;
	default:
		/* Famous last words: this really can't happen.. */
		assert(0);
		break;
	}

	ret = nouveau_grobj_alloc(chan, handle++, class, &screen->swzsurf);
	if (ret)
		goto fail;

	/* Scaled image from memory. */
	switch (chan->device->chipset & 0xf0) {
	case 0x10:
	case 0x20:
		class = NV10_SCALED_IMAGE_FROM_MEMORY;
		break;
	case 0x30:
		class = NV30_SCALED_IMAGE_FROM_MEMORY;
		break;
	case 0x40:
	case 0x60:
		class = NV40_SCALED_IMAGE_FROM_MEMORY;
		break;
	default:
		class = NV04_SCALED_IMAGE_FROM_MEMORY;
		break;
	}

	ret = nouveau_grobj_alloc(chan, handle++, class, &screen->sifm);
	if (ret)
		goto fail;

	if (chipset >= 0x10) {
		BEGIN_RING(chan, screen->sifm,
			   NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION, 1);
		OUT_RING(chan, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_TRUNCATE);
	}

	return GL_TRUE;

fail:
	nv04_surface_takedown(screen);
	return GL_FALSE;
}
