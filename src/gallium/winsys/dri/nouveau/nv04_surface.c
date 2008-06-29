#include "pipe/p_context.h"
#include "pipe/p_format.h"

#include "nouveau_context.h"

static INLINE int
nv04_surface_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_A8_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
	case PIPE_FORMAT_R5G6B5_UNORM:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_Z24S8_UNORM:
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
	case PIPE_FORMAT_R5G6B5_UNORM:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_Z24S8_UNORM:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}

static void
nv04_surface_copy_m2mf(struct nouveau_context *nv, unsigned dx, unsigned dy,
		       unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct pipe_surface *dst = nv->surf_dst;
	struct pipe_surface *src = nv->surf_src;
	unsigned dst_offset, src_offset;

	dst_offset = dst->offset + (dy * dst->stride) + (dx * dst->block.size);
	src_offset = src->offset + (sy * src->stride) + (sx * src->block.size);

	while (h) {
		int count = (h > 2047) ? 2047 : h;

		BEGIN_RING(chan, nv->nvc->NvM2MF,
			   NV04_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RELOCl(chan, nouveau_buffer(src->buffer)->bo, src_offset,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst_offset,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_WR);
		OUT_RING  (chan, src->stride);
		OUT_RING  (chan, dst->stride);
		OUT_RING  (chan, w * src->block.size);
		OUT_RING  (chan, count);
		OUT_RING  (chan, 0x0101);
		OUT_RING  (chan, 0);

		h -= count;
		src_offset += src->stride * count;
		dst_offset += dst->stride * count;
	}
}

static void
nv04_surface_copy_blit(struct nouveau_context *nv, unsigned dx, unsigned dy,
		       unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct nouveau_channel *chan = nv->nvc->channel;

	BEGIN_RING(chan, nv->nvc->NvImageBlit, 0x0300, 3);
	OUT_RING  (chan, (sy << 16) | sx);
	OUT_RING  (chan, (dy << 16) | dx);
	OUT_RING  (chan, ( h << 16) |  w);
}

static int
nv04_surface_copy_prep(struct nouveau_context *nv, struct pipe_surface *dst,
		       struct pipe_surface *src)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	int format;

	if (src->format != dst->format)
		return 1;

	/* NV_CONTEXT_SURFACES_2D has buffer alignment restrictions, fallback
	 * to NV_MEMORY_TO_MEMORY_FORMAT in this case.
	 */
	if ((src->offset & 63) || (dst->offset & 63)) {
		BEGIN_RING(nv->nvc->channel, nv->nvc->NvM2MF,
			   NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
		OUT_RELOCo(chan, nouveau_buffer(src->buffer)->bo,
			   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
		OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
			   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

		nv->surface_copy = nv04_surface_copy_m2mf;
		nv->surf_dst = dst;
		nv->surf_src = src;
		return 0;

	}

	if ((format = nv04_surface_format(dst->format)) < 0) {
		NOUVEAU_ERR("Bad surface format 0x%x\n", dst->format);
		return 1;
	}
	nv->surface_copy = nv04_surface_copy_blit;

	BEGIN_RING(chan, nv->nvc->NvCtxSurf2D,
		   NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, nouveau_buffer(src->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, nv->nvc->NvCtxSurf2D,
		   NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, format);
	OUT_RING  (chan, (dst->stride << 16) | src->stride);
	OUT_RELOCl(chan, nouveau_buffer(src->buffer)->bo, src->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	return 0;
}

static void
nv04_surface_copy_done(struct nouveau_context *nv)
{
	FIRE_RING(nv->nvc->channel);
}

static int
nv04_surface_fill(struct nouveau_context *nv, struct pipe_surface *dst,
		  unsigned dx, unsigned dy, unsigned w, unsigned h,
		  unsigned value)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *surf2d = nv->nvc->NvCtxSurf2D;
	struct nouveau_grobj *rect = nv->nvc->NvGdiRect;
	int cs2d_format, gdirect_format;

	if ((cs2d_format = nv04_surface_format(dst->format)) < 0) {
		NOUVEAU_ERR("Bad format = %d\n", dst->format);
		return 1;
	}

	if ((gdirect_format = nv04_rect_format(dst->format)) < 0) {
		NOUVEAU_ERR("Bad format = %d\n", dst->format);
		return 1;
	}

	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, cs2d_format);
	OUT_RING  (chan, (dst->stride << 16) | dst->stride);
	OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
	OUT_RING  (chan, gdirect_format);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
	OUT_RING  (chan, value);
	BEGIN_RING(chan, rect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (dx << 16) | dy);
	OUT_RING  (chan, ( w << 16) |  h);

	FIRE_RING(chan);
	return 0;
}

int
nouveau_surface_channel_create_nv04(struct nouveau_channel_context *nvc)
{
	struct nouveau_channel *chan = nvc->channel;
	unsigned chipset = nvc->channel->device->chipset, class;
	int ret;

	if ((ret = nouveau_grobj_alloc(chan, nvc->next_handle++, 0x39,
				       &nvc->NvM2MF))) {
		NOUVEAU_ERR("Error creating m2mf object: %d\n", ret);
		return 1;
	}
	BIND_RING (chan, nvc->NvM2MF, nvc->next_subchannel++);
	BEGIN_RING(chan, nvc->NvM2MF,
		   NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (chan, nvc->sync_notifier->handle);

	class = chipset < 0x10 ? NV04_CONTEXT_SURFACES_2D :
				 NV10_CONTEXT_SURFACES_2D;
	if ((ret = nouveau_grobj_alloc(chan, nvc->next_handle++, class,
				       &nvc->NvCtxSurf2D))) {
		NOUVEAU_ERR("Error creating 2D surface object: %d\n", ret);
		return 1;
	}
	BIND_RING (chan, nvc->NvCtxSurf2D, nvc->next_subchannel++);
	BEGIN_RING(chan, nvc->NvCtxSurf2D,
		   NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RING  (chan, nvc->channel->vram->handle);
	OUT_RING  (chan, nvc->channel->vram->handle);

	class = chipset < 0x10 ? NV04_IMAGE_BLIT : NV12_IMAGE_BLIT;
	if ((ret = nouveau_grobj_alloc(chan, nvc->next_handle++, class,
				       &nvc->NvImageBlit))) {
		NOUVEAU_ERR("Error creating blit object: %d\n", ret);
		return 1;
	}
	BIND_RING (chan, nvc->NvImageBlit, nvc->next_subchannel++);
	BEGIN_RING(chan, nvc->NvImageBlit, NV04_IMAGE_BLIT_DMA_NOTIFY, 1);
	OUT_RING  (chan, nvc->sync_notifier->handle);
	BEGIN_RING(chan, nvc->NvImageBlit, NV04_IMAGE_BLIT_SURFACE, 1);
	OUT_RING  (chan, nvc->NvCtxSurf2D->handle);
	BEGIN_RING(chan, nvc->NvImageBlit, NV04_IMAGE_BLIT_OPERATION, 1);
	OUT_RING  (chan, NV04_IMAGE_BLIT_OPERATION_SRCCOPY);

	class = NV04_GDI_RECTANGLE_TEXT;
	if ((ret = nouveau_grobj_alloc(chan, nvc->next_handle++, class,
				       &nvc->NvGdiRect))) {
		NOUVEAU_ERR("Error creating rect object: %d\n", ret);
		return 1;
	}
	BIND_RING (chan, nvc->NvGdiRect, nvc->next_subchannel++);
	BEGIN_RING(chan, nvc->NvGdiRect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, nvc->sync_notifier->handle);
	BEGIN_RING(chan, nvc->NvGdiRect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, nvc->NvCtxSurf2D->handle);
	BEGIN_RING(chan, nvc->NvGdiRect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_SRCCOPY);
	BEGIN_RING(chan, nvc->NvGdiRect,
		   NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	switch (chipset & 0xf0) {
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

	ret = nouveau_grobj_alloc(chan, nvc->next_handle++, class,
				  &nvc->NvSwzSurf);
	if (ret) {
		NOUVEAU_ERR("Error creating swizzled surface: %d\n", ret);
		return 1;
	}

	BIND_RING (chan, nvc->NvSwzSurf, nvc->next_subchannel++);

	if (chipset < 0x10) {
		class = NV04_SCALED_IMAGE_FROM_MEMORY;
	} else
	if (chipset < 0x40) {
		class = NV10_SCALED_IMAGE_FROM_MEMORY;
	} else {
		class = NV40_SCALED_IMAGE_FROM_MEMORY;
	}

	ret = nouveau_grobj_alloc(chan, nvc->next_handle++, class,
				  &nvc->NvSIFM);
	if (ret) {
		NOUVEAU_ERR("Error creating scaled image object: %d\n", ret);
		return 1;
	}

	BIND_RING (chan, nvc->NvSIFM, nvc->next_subchannel++);

	return 0;
}

int
nouveau_surface_init_nv04(struct nouveau_context *nv)
{
	nv->surface_copy_prep = nv04_surface_copy_prep;
	nv->surface_copy = nv04_surface_copy_blit;
	nv->surface_copy_done = nv04_surface_copy_done;
	nv->surface_fill = nv04_surface_fill;
	return 0;
}

