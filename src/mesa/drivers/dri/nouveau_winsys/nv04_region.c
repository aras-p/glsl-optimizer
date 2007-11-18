#include "pipe/p_context.h"

#include "nouveau_context.h"

static INLINE int
nv04_surface_format(int cpp)
{
	switch (cpp) {
	case 1: return 0x1;
	case 2: return 0x4;
	case 4: return 0xb;
	default:
		return -1;
	}
}

static INLINE int
nv04_rect_format(int cpp)
{
	switch (cpp) {
	case 1: return 0x3;
	case 2: return 0x1;
	case 4: return 0x3;
	default:
		return -1;
	}
}

static int
nv04_region_display(void)
{
	NOUVEAU_ERR("unimplemented\n");
	return 0;
}

static int
nv04_region_copy_m2mf(struct nouveau_context *nv, struct pipe_region *dst,
		      unsigned dst_offset, struct pipe_region *src,
		      unsigned src_offset, unsigned line_len, unsigned height)
{
	BEGIN_RING(NvM2MF, NV_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
	OUT_RELOCo(src->buffer, NOUVEAU_BO_GART | NOUVEAU_BO_VRAM |
		   NOUVEAU_BO_RD);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_GART | NOUVEAU_BO_VRAM |
		   NOUVEAU_BO_WR);

	while (height) {
		int count = (height > 2047) ? 2047 : height;

		BEGIN_RING(NvM2MF, NV_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RELOCl(src->buffer, src_offset, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCl(dst->buffer, dst_offset, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_WR);
		OUT_RING  (src->pitch * src->cpp);
		OUT_RING  (dst->pitch * dst->cpp);
		OUT_RING  (line_len);
		OUT_RING  (count);
		OUT_RING  (0x0101);
		OUT_RING  (0);

		height -= count;
		src_offset += src->pitch * count;
		dst_offset += dst->pitch * count;
	}

	return 0;
}

static int
nv04_region_copy(struct nouveau_context *nv, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dx, unsigned dy,
		 struct pipe_region *src, unsigned src_offset,
		 unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	int format;

	if (src->cpp != dst->cpp)
		return 1;

	NOUVEAU_ERR("preg\n");

	/* NV_CONTEXT_SURFACES_2D has buffer alignment restrictions, fallback
	 * to NV_MEMORY_TO_MEMORY_FORMAT in this case.
	 */
	if ((src_offset & 63) || (dst_offset & 63)) {
		dst_offset += (dy * dst->pitch + dx) * dst->cpp;
		src_offset += (sy * src->pitch + sx) * src->cpp;
		return nv04_region_copy_m2mf(nv, dst, dst_offset, src,
					     src_offset, w * src->cpp, h);

	}

	if ((format = nv04_surface_format(dst->cpp)) < 0) {
		NOUVEAU_ERR("Bad cpp = %d\n", dst->cpp);
		return 1;
	}

	BEGIN_RING(NvCtxSurf2D, 0x0184, 2);
	OUT_RELOCo(src->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(NvCtxSurf2D, 0x0300, 4);
	OUT_RING  (format);
	OUT_RING  (((dst->pitch * dst->cpp) << 16) | (src->pitch * src->cpp));
	OUT_RELOCl(src->buffer, src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(dst->buffer, dst_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(NvImageBlit, 0x0300, 3);
	OUT_RING  ((sy << 16) | sx);
	OUT_RING  ((dy << 16) | dx);
	OUT_RING  (( h << 16) |  w);

	return 0;
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

	BEGIN_RING(NvCtxSurf2D, 0x0184, 2);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(NvCtxSurf2D, 0x0300, 4);
	OUT_RING  (cs2d_format);
	OUT_RING  (((dst->pitch * dst->cpp) << 16) | (dst->pitch * dst->cpp));
	OUT_RELOCl(dst->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(dst->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(NvGdiRect, 0x0300, 1);
	OUT_RING  (gdirect_format);
	BEGIN_RING(NvGdiRect, 0x03fc, 1);
	OUT_RING  (value);
	BEGIN_RING(NvGdiRect, 0x0400, 2);
	OUT_RING  ((dx << 16) | dy);
	OUT_RING  (( w << 16) |  h);

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
	int ret;

	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, 0x39,
				       &nv->NvM2MF))) {
		NOUVEAU_ERR("Error creating m2mf object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvM2MF, 0x0180, 1);
	OUT_RING  (nv->sync_notifier->handle);

	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, 0x62,
				       &nv->NvCtxSurf2D))) {
		NOUVEAU_ERR("Error creating 2D surface object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvCtxSurf2D, 0x0184, 2);
	OUT_RING  (nv->channel->vram->handle);
	OUT_RING  (nv->channel->vram->handle);

	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, 0x9f,
				       &nv->NvImageBlit))) {
		NOUVEAU_ERR("Error creating blit object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvImageBlit, 0x0180, 1);
	OUT_RING  (nv->sync_notifier->handle);
	BEGIN_RING(NvImageBlit, 0x019c, 1);
	OUT_RING  (nv->NvCtxSurf2D->handle);
	BEGIN_RING(NvImageBlit, 0x02fc, 1);
	OUT_RING  (3);

	if ((ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, 0x4a,
				       &nv->NvGdiRect))) {
		NOUVEAU_ERR("Error creating rect object: %d\n", ret);
		return 1;
	}
	BEGIN_RING(NvGdiRect, 0x0180, 1);
	OUT_RING  (nv->sync_notifier->handle);
	BEGIN_RING(NvGdiRect, 0x0198, 1);
	OUT_RING  (nv->NvCtxSurf2D->handle);
	BEGIN_RING(NvGdiRect, 0x02fc, 1);
	OUT_RING  (3);
	BEGIN_RING(NvGdiRect, 0x0304, 1);
	OUT_RING  (2);

	nv->region_display = nv04_region_display;
	nv->region_copy = nv04_region_copy;
	nv->region_fill = nv04_region_fill;
	nv->region_data = nv04_region_data;
	return 0;
}

