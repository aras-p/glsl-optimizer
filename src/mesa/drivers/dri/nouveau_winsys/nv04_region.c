#include "pipe/p_context.h"

#include "nouveau_context.h"

static INLINE int
nv04_surface_format(int cpp)
{
	switch (cpp) {
	case 1: return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
	case 2: return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
	case 4: return NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;
	default:
		return -1;
	}
}

static INLINE int
nv04_rect_format(int cpp)
{
	switch (cpp) {
	case 1: return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	case 2: return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
	case 4: return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}

static void
nv04_region_copy_m2mf(struct nouveau_context *nv, unsigned dx, unsigned dy,
		      unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct pipe_region *dst = nv->region_dst;
	struct pipe_region *src = nv->region_dst;
	unsigned dst_offset, src_offset;
	
	dst_offset = nv->region_dst_offset + (dy * dst->pitch + dx) * dst->cpp;
	src_offset = nv->region_src_offset + (sy * src->pitch + sx) * src->cpp;

	while (h) {
		int count = (h > 2047) ? 2047 : h;

		BEGIN_RING(NvM2MF, NV_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RELOCl(src->buffer, src_offset, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCl(dst->buffer, dst_offset, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_WR);
		OUT_RING  (src->pitch * src->cpp);
		OUT_RING  (dst->pitch * dst->cpp);
		OUT_RING  (w * src->cpp);
		OUT_RING  (count);
		OUT_RING  (0x0101);
		OUT_RING  (0);

		h -= count;
		src_offset += src->pitch * count;
		dst_offset += dst->pitch * count;
	}
}

static void
nv04_region_copy_blit(struct nouveau_context *nv, unsigned dx, unsigned dy,
		      unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	BEGIN_RING(NvImageBlit, 0x0300, 3);
	OUT_RING  ((sy << 16) | sx);
	OUT_RING  ((dy << 16) | dx);
	OUT_RING  (( h << 16) |  w);
}

static int
nv04_region_copy_prep(struct nouveau_context *nv,
		      struct pipe_region *dst, unsigned dst_offset,
		      struct pipe_region *src, unsigned src_offset)
{
	int format;

	if (src->cpp != dst->cpp)
		return 1;

	/* NV_CONTEXT_SURFACES_2D has buffer alignment restrictions, fallback
	 * to NV_MEMORY_TO_MEMORY_FORMAT in this case.
	 */
	if ((src_offset & 63) || (dst_offset & 63)) {
		BEGIN_RING(NvM2MF, NV_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
		OUT_RELOCo(src->buffer, NOUVEAU_BO_GART | NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_RD);
		OUT_RELOCo(dst->buffer, NOUVEAU_BO_GART | NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_WR);

		nv->region_copy = nv04_region_copy_m2mf;
		nv->region_dst = dst;
		nv->region_dst_offset = dst_offset;
		nv->region_src = src;
		nv->region_src_offset = src_offset;
		return 0;

	}

	if ((format = nv04_surface_format(dst->cpp)) < 0) {
		NOUVEAU_ERR("Bad cpp = %d\n", dst->cpp);
		return 1;
	}
	nv->region_copy = nv04_region_copy_blit;

	BEGIN_RING(NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(src->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (format);
	OUT_RING  (((dst->pitch * dst->cpp) << 16) | (src->pitch * src->cpp));
	OUT_RELOCl(src->buffer, src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(dst->buffer, dst_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	return 0;
}

static void
nv04_region_copy_done(struct nouveau_context *nv)
{
	nouveau_notifier_reset(nv->sync_notifier, 0);
	BEGIN_RING(NvGdiRect, 0x104, 1);
	OUT_RING  (0);
	BEGIN_RING(NvGdiRect, 0x100, 1);
	OUT_RING  (0);
	FIRE_RING();
	nouveau_notifier_wait_status(nv->sync_notifier, 0, 0, 2000);
}

static int
nv04_region_fill(struct nouveau_context *nv,
		 struct pipe_region *dst, unsigned dst_offset,
		 unsigned dx, unsigned dy, unsigned w, unsigned h,
		 unsigned value)
{
	int cs2d_format, gdirect_format;

	if ((cs2d_format = nv04_surface_format(dst->cpp)) < 0) {
		NOUVEAU_ERR("Bad cpp = %d\n", dst->cpp);
		return 1;
	}

	if ((gdirect_format = nv04_rect_format(dst->cpp)) < 0) {
		NOUVEAU_ERR("Bad cpp = %d\n", dst->cpp);
		return 1;
	}

	BEGIN_RING(NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (cs2d_format);
	OUT_RING  (((dst->pitch * dst->cpp) << 16) | (dst->pitch * dst->cpp));
	OUT_RELOCl(dst->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(dst->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
	OUT_RING  (gdirect_format);
	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
	OUT_RING  (value);
	BEGIN_RING(NvGdiRect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  ((dx << 16) | dy);
	OUT_RING  (( w << 16) |  h);

	nouveau_notifier_reset(nv->sync_notifier, 0);
	BEGIN_RING(NvGdiRect, 0x104, 1);
	OUT_RING  (0);
	BEGIN_RING(NvGdiRect, 0x100, 1);
	OUT_RING  (0);
	FIRE_RING();
	nouveau_notifier_wait_status(nv->sync_notifier, 0, 0, 2000);

	return 0;
}

static int
nv04_region_data(struct nouveau_context *nv, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dx, unsigned dy,
		 const void *src, unsigned src_pitch,
		 unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	NOUVEAU_ERR("unimplemented!!\n");
	return 0;
}

int
nouveau_region_init_nv04(struct nouveau_context *nv)
{
	unsigned class;
	int ret;

	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, 0x39,
				       &nv->NvM2MF))) {
		NOUVEAU_ERR("Error creating m2mf object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvM2MF, NV_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (nv->sync_notifier->handle);

	class = nv->chipset < 0x10 ? NV04_CONTEXT_SURFACES_2D :
				     NV10_CONTEXT_SURFACES_2D;
	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, class,
				       &nv->NvCtxSurf2D))) {
		NOUVEAU_ERR("Error creating 2D surface object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RING  (nv->channel->vram->handle);
	OUT_RING  (nv->channel->vram->handle);

	class = nv->chipset < 0x10 ? NV_IMAGE_BLIT :
				     NV12_IMAGE_BLIT;
	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, class,
				       &nv->NvImageBlit))) {
		NOUVEAU_ERR("Error creating blit object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvImageBlit, NV_IMAGE_BLIT_DMA_NOTIFY, 1);
	OUT_RING  (nv->sync_notifier->handle);
	BEGIN_RING(NvImageBlit, NV_IMAGE_BLIT_SURFACE, 1);
	OUT_RING  (nv->NvCtxSurf2D->handle);
	BEGIN_RING(NvImageBlit, NV_IMAGE_BLIT_OPERATION, 1);
	OUT_RING  (NV_IMAGE_BLIT_OPERATION_SRCCOPY);

	class = NV04_GDI_RECTANGLE_TEXT;
	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, class,
				       &nv->NvGdiRect))) {
		NOUVEAU_ERR("Error creating rect object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (nv->sync_notifier->handle);
	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (nv->NvCtxSurf2D->handle);
	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (NV04_GDI_RECTANGLE_TEXT_OPERATION_SRCCOPY);
	BEGIN_RING(NvGdiRect, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	nv->region_copy_prep = nv04_region_copy_prep;
	nv->region_copy = nv04_region_copy_blit;
	nv->region_copy_done = nv04_region_copy_done;
	nv->region_fill = nv04_region_fill;
	nv->region_data = nv04_region_data;
	return 0;
}

