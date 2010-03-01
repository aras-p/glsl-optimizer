#include "pipe/p_context.h"
#include "pipe/p_format.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_util.h"
#include "nouveau/nouveau_screen.h"
#include "nv04_surface_2d.h"

static INLINE int
nv04_surface_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_B5G6R5_UNORM:
	case PIPE_FORMAT_Z16_UNORM:
	case PIPE_FORMAT_L8A8_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8;
	case PIPE_FORMAT_S8Z24_UNORM:
	case PIPE_FORMAT_X8Z24_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;
	default:
		return -1;
	}
}

static INLINE int
nv04_rect_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_A8_UNORM:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	case PIPE_FORMAT_B5G6R5_UNORM:
	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_Z16_UNORM:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_S8Z24_UNORM:
	case PIPE_FORMAT_X8Z24_UNORM:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}

static INLINE int
nv04_scaled_image_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_Y8;
	case PIPE_FORMAT_B5G5R5A1_UNORM:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A1R5G5B5;
	case PIPE_FORMAT_B8G8R8A8_UNORM:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A8R8G8B8;
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_X8R8G8B8;
	case PIPE_FORMAT_B5G6R5_UNORM:
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_L8A8_UNORM:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_R5G6B5;
	default:
		return -1;
	}
}

static INLINE unsigned
nv04_swizzle_bits_square(unsigned x, unsigned y)
{
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
	return v | u;
}

/* rectangular swizzled textures are linear concatenations of swizzled square tiles */
static INLINE unsigned
nv04_swizzle_bits(unsigned x, unsigned y, unsigned w, unsigned h)
{
	unsigned s = MIN2(w, h);
	unsigned m = s - 1;
	return (((x | y) & ~m) * s) | nv04_swizzle_bits_square(x & m, y & m);
}

static int
nv04_surface_copy_swizzle(struct nv04_surface_2d *ctx,
			  struct pipe_surface *dst, int dx, int dy,
			  struct pipe_surface *src, int sx, int sy,
			  int w, int h)
{
	struct nouveau_channel *chan = ctx->swzsurf->channel;
	struct nouveau_grobj *swzsurf = ctx->swzsurf;
	struct nouveau_grobj *sifm = ctx->sifm;
	struct nouveau_bo *src_bo = nouveau_bo(ctx->buf(src));
	struct nouveau_bo *dst_bo = nouveau_bo(ctx->buf(dst));
	const unsigned src_pitch = ((struct nv04_surface *)src)->pitch;
        /* Max width & height may not be the same on all HW, but must be POT */
	const unsigned max_w = 1024;
	const unsigned max_h = 1024;
	unsigned sub_w = w > max_w ? max_w : w;
	unsigned sub_h = h > max_h ? max_h : h;
	unsigned x;
	unsigned y;

        /* Swizzled surfaces must be POT  */
	assert(util_is_pot(dst->width) && util_is_pot(dst->height));

        /* If area is too large to copy in one shot we must copy it in POT chunks to meet alignment requirements */
	assert(sub_w == w || util_is_pot(sub_w));
	assert(sub_h == h || util_is_pot(sub_h));

	MARK_RING (chan, 8 + ((w+sub_w)/sub_w)*((h+sub_h)/sub_h)*17, 2 +
			 ((w+sub_w)/sub_w)*((h+sub_h)/sub_h)*2);

	BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_DMA_IMAGE, 1);
	OUT_RELOCo(chan, dst_bo,
	                 NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_FORMAT, 1);
	OUT_RING  (chan, nv04_surface_format(dst->format) |
	                 log2i(dst->width) << NV04_SWIZZLED_SURFACE_FORMAT_BASE_SIZE_U_SHIFT |
	                 log2i(dst->height) << NV04_SWIZZLED_SURFACE_FORMAT_BASE_SIZE_V_SHIFT);

	BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_DMA_IMAGE, 1);
	OUT_RELOCo(chan, src_bo,
	                 NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	BEGIN_RING(chan, sifm, NV04_SCALED_IMAGE_FROM_MEMORY_SURFACE, 1);
	OUT_RING  (chan, swzsurf->handle);

	for (y = 0; y < h; y += sub_h) {
	  sub_h = MIN2(sub_h, h - y);

	  for (x = 0; x < w; x += sub_w) {
	    sub_w = MIN2(sub_w, w - x);

	    assert(!(dst->offset & 63));

	    BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_OFFSET, 1);
	    OUT_RELOCl(chan, dst_bo, dst->offset,
                             NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	    BEGIN_RING(chan, sifm, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION, 9);
	    OUT_RING  (chan, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_TRUNCATE);
	    OUT_RING  (chan, nv04_scaled_image_format(src->format));
	    OUT_RING  (chan, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION_SRCCOPY);
	    OUT_RING  (chan, (x + dx) | ((y + dy) << NV03_SCALED_IMAGE_FROM_MEMORY_CLIP_POINT_Y_SHIFT));
	    OUT_RING  (chan, sub_h << NV03_SCALED_IMAGE_FROM_MEMORY_CLIP_SIZE_H_SHIFT | sub_w);
	    OUT_RING  (chan, (x + dx) | ((y + dy) << NV03_SCALED_IMAGE_FROM_MEMORY_OUT_POINT_Y_SHIFT));
	    OUT_RING  (chan, sub_h << NV03_SCALED_IMAGE_FROM_MEMORY_OUT_SIZE_H_SHIFT | sub_w);
	    OUT_RING  (chan, 1 << 20);
	    OUT_RING  (chan, 1 << 20);

	    BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_SIZE, 4);
	    OUT_RING  (chan, sub_h << NV03_SCALED_IMAGE_FROM_MEMORY_SIZE_H_SHIFT | sub_w);
	    OUT_RING  (chan, src_pitch |
			     NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_ORIGIN_CENTER |
			     NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_FILTER_POINT_SAMPLE);
	    OUT_RELOCl(chan, src_bo, src->offset + (sy+y) * src_pitch + (sx+x) * util_format_get_blocksize(src->texture->format),
                             NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	    OUT_RING  (chan, 0);
	  }
	}

	return 0;
}

static int
nv04_surface_copy_m2mf(struct nv04_surface_2d *ctx,
		       struct pipe_surface *dst, int dx, int dy,
		       struct pipe_surface *src, int sx, int sy, int w, int h)
{
	struct nouveau_channel *chan = ctx->m2mf->channel;
	struct nouveau_grobj *m2mf = ctx->m2mf;
	struct nouveau_bo *src_bo = nouveau_bo(ctx->buf(src));
	struct nouveau_bo *dst_bo = nouveau_bo(ctx->buf(dst));
	unsigned src_pitch = ((struct nv04_surface *)src)->pitch;
	unsigned dst_pitch = ((struct nv04_surface *)dst)->pitch;
	unsigned dst_offset = dst->offset + dy * dst_pitch +
	                      dx * util_format_get_blocksize(dst->texture->format);
	unsigned src_offset = src->offset + sy * src_pitch +
	                      sx * util_format_get_blocksize(src->texture->format);

	MARK_RING (chan, 3 + ((h / 2047) + 1) * 9, 2 + ((h / 2047) + 1) * 2);
	BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
	OUT_RELOCo(chan, src_bo,
		   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, dst_bo,
		   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	while (h) {
		int count = (h > 2047) ? 2047 : h;

		BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RELOCl(chan, src_bo, src_offset,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCl(chan, dst_bo, dst_offset,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_WR);
		OUT_RING  (chan, src_pitch);
		OUT_RING  (chan, dst_pitch);
		OUT_RING  (chan, w * util_format_get_blocksize(src->texture->format));
		OUT_RING  (chan, count);
		OUT_RING  (chan, 0x0101);
		OUT_RING  (chan, 0);

		h -= count;
		src_offset += src_pitch * count;
		dst_offset += dst_pitch * count;
	}

	return 0;
}

static int
nv04_surface_copy_blit(struct nv04_surface_2d *ctx, struct pipe_surface *dst,
		       int dx, int dy, struct pipe_surface *src, int sx, int sy,
		       int w, int h)
{
	struct nouveau_channel *chan = ctx->surf2d->channel;
	struct nouveau_grobj *surf2d = ctx->surf2d;
	struct nouveau_grobj *blit = ctx->blit;
	struct nouveau_bo *src_bo = nouveau_bo(ctx->buf(src));
	struct nouveau_bo *dst_bo = nouveau_bo(ctx->buf(dst));
	unsigned src_pitch = ((struct nv04_surface *)src)->pitch;
	unsigned dst_pitch = ((struct nv04_surface *)dst)->pitch;
	int format;

	format = nv04_surface_format(dst->format);
	if (format < 0)
		return 1;

	MARK_RING (chan, 12, 4);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, src_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, dst_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, format);
	OUT_RING  (chan, (dst_pitch << 16) | src_pitch);
	OUT_RELOCl(chan, src_bo, src->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, dst_bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, blit, 0x0300, 3);
	OUT_RING  (chan, (sy << 16) | sx);
	OUT_RING  (chan, (dy << 16) | dx);
	OUT_RING  (chan, ( h << 16) |  w);

	return 0;
}

static void
nv04_surface_copy(struct nv04_surface_2d *ctx, struct pipe_surface *dst,
		  int dx, int dy, struct pipe_surface *src, int sx, int sy,
		  int w, int h)
{
	unsigned src_pitch = ((struct nv04_surface *)src)->pitch;
	unsigned dst_pitch = ((struct nv04_surface *)dst)->pitch;
	int src_linear = src->texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR;
	int dst_linear = dst->texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR;

	assert(src->format == dst->format);

	/* Setup transfer to swizzle the texture to vram if needed */
        if (src_linear && !dst_linear && w > 1 && h > 1) {
           nv04_surface_copy_swizzle(ctx, dst, dx, dy, src, sx, sy, w, h);
           return;
        }

	/* NV_CONTEXT_SURFACES_2D has buffer alignment restrictions, fallback
	 * to NV_MEMORY_TO_MEMORY_FORMAT in this case.
	 */
	if ((src->offset & 63) || (dst->offset & 63) ||
	    (src_pitch & 63) || (dst_pitch & 63)) {
		nv04_surface_copy_m2mf(ctx, dst, dx, dy, src, sx, sy, w, h);
		return;
	}

	nv04_surface_copy_blit(ctx, dst, dx, dy, src, sx, sy, w, h);
}

static void
nv04_surface_fill(struct nv04_surface_2d *ctx, struct pipe_surface *dst,
		  int dx, int dy, int w, int h, unsigned value)
{
	struct nouveau_channel *chan = ctx->surf2d->channel;
	struct nouveau_grobj *surf2d = ctx->surf2d;
	struct nouveau_grobj *rect = ctx->rect;
	struct nouveau_bo *dst_bo = nouveau_bo(ctx->buf(dst));
	unsigned dst_pitch = ((struct nv04_surface *)dst)->pitch;
	int cs2d_format, gdirect_format;

	cs2d_format = nv04_surface_format(dst->format);
	assert(cs2d_format >= 0);

	gdirect_format = nv04_rect_format(dst->format);
	assert(gdirect_format >= 0);

	MARK_RING (chan, 16, 4);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, dst_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCo(chan, dst_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, cs2d_format);
	OUT_RING  (chan, (dst_pitch << 16) | dst_pitch);
	OUT_RELOCl(chan, dst_bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, dst_bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
	OUT_RING  (chan, gdirect_format);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
	OUT_RING  (chan, value);
	BEGIN_RING(chan, rect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (dx << 16) | dy);
	OUT_RING  (chan, ( w << 16) |  h);
}

void
nv04_surface_2d_takedown(struct nv04_surface_2d **pctx)
{
	struct nv04_surface_2d *ctx;

	if (!pctx || !*pctx)
		return;
	ctx = *pctx;
	*pctx = NULL;

	nouveau_notifier_free(&ctx->ntfy);
	nouveau_grobj_free(&ctx->m2mf);
	nouveau_grobj_free(&ctx->surf2d);
	nouveau_grobj_free(&ctx->swzsurf);
	nouveau_grobj_free(&ctx->rect);
	nouveau_grobj_free(&ctx->blit);
	nouveau_grobj_free(&ctx->sifm);

	FREE(ctx);
}

struct nv04_surface_2d *
nv04_surface_2d_init(struct nouveau_screen *screen)
{
	struct nv04_surface_2d *ctx = CALLOC_STRUCT(nv04_surface_2d);
	struct nouveau_channel *chan = screen->channel;
	unsigned handle = 0x88000000, class;
	int ret;

	if (!ctx)
		return NULL;

	ret = nouveau_notifier_alloc(chan, handle++, 1, &ctx->ntfy);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	ret = nouveau_grobj_alloc(chan, handle++, 0x0039, &ctx->m2mf);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);

	if (chan->device->chipset < 0x10)
		class = NV04_CONTEXT_SURFACES_2D;
	else
		class = NV10_CONTEXT_SURFACES_2D;

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->surf2d);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->surf2d,
			 NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);

	if (chan->device->chipset < 0x10)
		class = NV04_IMAGE_BLIT;
	else
		class = NV12_IMAGE_BLIT;

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->blit);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->blit, NV01_IMAGE_BLIT_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);
	BEGIN_RING(chan, ctx->blit, NV04_IMAGE_BLIT_SURFACE, 1);
	OUT_RING  (chan, ctx->surf2d->handle);
	BEGIN_RING(chan, ctx->blit, NV01_IMAGE_BLIT_OPERATION, 1);
	OUT_RING  (chan, NV01_IMAGE_BLIT_OPERATION_SRCCOPY);

	ret = nouveau_grobj_alloc(chan, handle++, NV04_GDI_RECTANGLE_TEXT,
				  &ctx->rect);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);
	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, ctx->surf2d->handle);
	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_SRCCOPY);
	BEGIN_RING(chan, ctx->rect,
			 NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

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

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->swzsurf);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

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

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->sifm);
	if (ret) {
		nv04_surface_2d_takedown(&ctx);
		return NULL;
	}

	ctx->copy = nv04_surface_copy;
	ctx->fill = nv04_surface_fill;
	return ctx;
}

struct nv04_surface*
nv04_surface_wrap_for_render(struct pipe_screen *pscreen, struct nv04_surface_2d* eng2d, struct nv04_surface* ns)
{
	int temp_flags;

	// printf("creating temp, flags is %i!\n", flags);

	if(ns->base.usage & PIPE_BUFFER_USAGE_DISCARD)
	{
		temp_flags = ns->base.usage | PIPE_BUFFER_USAGE_GPU_READ;
		ns->base.usage = PIPE_BUFFER_USAGE_GPU_WRITE | NOUVEAU_BUFFER_USAGE_NO_RENDER | PIPE_BUFFER_USAGE_DISCARD;
	}
	else
	{
		temp_flags = ns->base.usage | PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE;
		ns->base.usage = PIPE_BUFFER_USAGE_GPU_WRITE | NOUVEAU_BUFFER_USAGE_NO_RENDER | PIPE_BUFFER_USAGE_GPU_READ;
	}

	struct nv40_screen* screen = (struct nv40_screen*)pscreen;
	ns->base.usage = PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE;

	struct pipe_texture templ;
	memset(&templ, 0, sizeof(templ));
	templ.format = ns->base.texture->format;
	templ.target = PIPE_TEXTURE_2D;
	templ.width0 = ns->base.width;
	templ.height0 = ns->base.height;
	templ.depth0 = 1;
	templ.last_level = 0;

	// TODO: this is probably wrong and we should specifically handle multisampling somehow once it is implemented
	templ.nr_samples = ns->base.texture->nr_samples;

	templ.tex_usage = ns->base.texture->tex_usage | PIPE_TEXTURE_USAGE_RENDER_TARGET;

	struct pipe_texture* temp_tex = pscreen->texture_create(pscreen, &templ);
	struct nv04_surface* temp_ns = (struct nv04_surface*)pscreen->get_tex_surface(pscreen, temp_tex, 0, 0, 0, temp_flags);
	temp_ns->backing = ns;

	if(ns->base.usage & PIPE_BUFFER_USAGE_GPU_READ)
		eng2d->copy(eng2d, &temp_ns->backing->base, 0, 0, &ns->base, 0, 0, ns->base.width, ns->base.height);

	return temp_ns;
}

