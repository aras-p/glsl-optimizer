#include "pipe/p_context.h"

#include "nouveau_context.h"

static INLINE int
nv50_format(int cpp)
{
	switch (cpp) {
	case 4: return NV50_2D_DST_FORMAT_32BPP;
	case 3: return NV50_2D_DST_FORMAT_24BPP;
	case 2: return NV50_2D_DST_FORMAT_16BPP;
	case 1: return NV50_2D_DST_FORMAT_8BPP;
	default:
		return -1;
	}
}

static int
nv50_surface_copy_prep(struct nouveau_context *nv,
		       struct pipe_surface *dst, struct pipe_surface *src)
{
	int surf_format;

	assert(src->cpp == dst->cpp);

	surf_format = nv50_format(dst->cpp);
	assert(surf_format >= 0);

	BEGIN_RING(Nv2D, NV50_2D_DMA_IN_MEMORY0, 2);
	OUT_RELOCo(src->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(Nv2D, NV50_2D_DST_FORMAT, 2);
	OUT_RING  (surf_format);
	OUT_RING  (1);
	BEGIN_RING(Nv2D, NV50_2D_DST_PITCH, 5);
	OUT_RING  (dst->pitch * dst->cpp);
	OUT_RING  (dst->pitch);
	OUT_RING  (dst->height);
	OUT_RELOCh(dst->buffer, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(dst->buffer, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(Nv2D, NV50_2D_CLIP_X, 4);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (dst->pitch);
	OUT_RING  (dst->height);

	BEGIN_RING(Nv2D, NV50_2D_SRC_FORMAT, 2);
	OUT_RING  (surf_format);
	OUT_RING  (1);
	BEGIN_RING(Nv2D, NV50_2D_SRC_PITCH, 5);
	OUT_RING  (src->pitch * src->cpp);
	OUT_RING  (src->pitch);
	OUT_RING  (src->height);
	OUT_RELOCh(src->buffer, src->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(src->buffer, src->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

	return 0;
}

static void
nv50_surface_copy(struct nouveau_context *nv, unsigned dx, unsigned dy,
		  unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	BEGIN_RING(Nv2D, 0x0110, 1);
	OUT_RING  (0);
	BEGIN_RING(Nv2D, NV50_2D_BLIT_DST_X, 12);
	OUT_RING  (dx);
	OUT_RING  (dy);
	OUT_RING  (w);
	OUT_RING  (h);
	OUT_RING  (0);
	OUT_RING  (1);
	OUT_RING  (0);
	OUT_RING  (1);
	OUT_RING  (0);
	OUT_RING  (sx);
	OUT_RING  (0);
	OUT_RING  (sy);
}

static void
nv50_surface_copy_done(struct nouveau_context *nv)
{
	FIRE_RING();
}

static int
nv50_surface_fill(struct nouveau_context *nv, struct pipe_surface *dst,
		  unsigned dx, unsigned dy, unsigned w, unsigned h,
		  unsigned value)
{
	int surf_format, rect_format;

	surf_format = nv50_format(dst->cpp);
	if (surf_format < 0)
		return 1;

	rect_format = nv50_format(dst->cpp);
	if (rect_format < 0)
		return 1;

	BEGIN_RING(Nv2D, NV50_2D_DMA_IN_MEMORY1, 1);
	OUT_RELOCo(dst->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(Nv2D, NV50_2D_DST_FORMAT, 2);
	OUT_RING  (surf_format);
	OUT_RING  (1);
	BEGIN_RING(Nv2D, NV50_2D_DST_PITCH, 5);
	OUT_RING  (dst->pitch * dst->cpp);
	OUT_RING  (dst->pitch);
	OUT_RING  (dst->height);
	OUT_RELOCh(dst->buffer, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(dst->buffer, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(Nv2D, NV50_2D_CLIP_X, 4);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (dst->pitch);
	OUT_RING  (dst->height);

	BEGIN_RING(Nv2D, 0x0580, 3);
	OUT_RING  (4);
	OUT_RING  (rect_format);
	OUT_RING  (value);

	BEGIN_RING(Nv2D, NV50_2D_RECT_X1, 4);
	OUT_RING  (dx);
	OUT_RING  (dy);
	OUT_RING  (dx + w);
	OUT_RING  (dy + h);

	FIRE_RING();

	return 0;
}

static int
nv50_surface_data(struct nouveau_context *nv, struct pipe_surface *dst,
		  unsigned dx, unsigned dy, const void *src, unsigned src_pitch,
		  unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	NOUVEAU_ERR("unimplemented!!\n");
	return 0;
}

int
nouveau_surface_init_nv50(struct nouveau_context *nv)
{
	int ret;

	ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++, NV50_2D,
				  &nv->Nv2D);
	if (ret)
		return ret;

	BEGIN_RING(Nv2D, NV50_2D_DMA_NOTIFY, 1);
	OUT_RING  (nv->sync_notifier->handle);
	BEGIN_RING(Nv2D, NV50_2D_DMA_IN_MEMORY0, 2);
	OUT_RING  (nv->channel->vram->handle);
	OUT_RING  (nv->channel->vram->handle);
	BEGIN_RING(Nv2D, NV50_2D_OPERATION, 1);
	OUT_RING  (NV50_2D_OPERATION_SRCCOPY);

	nv->surface_copy_prep = nv50_surface_copy_prep;
	nv->surface_copy = nv50_surface_copy;
	nv->surface_copy_done = nv50_surface_copy_done;
	nv->surface_fill = nv50_surface_fill;
	nv->surface_data = nv50_surface_data;
	return 0;
}

