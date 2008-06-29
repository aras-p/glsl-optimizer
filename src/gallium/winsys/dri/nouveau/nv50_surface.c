#include "pipe/p_context.h"
#include "pipe/p_format.h"

#include "nouveau_context.h"

static INLINE int
nv50_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_Z24S8_UNORM:
		return NV50_2D_DST_FORMAT_32BPP;
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return NV50_2D_DST_FORMAT_24BPP;
	case PIPE_FORMAT_R5G6B5_UNORM:
		return NV50_2D_DST_FORMAT_16BPP;
	case PIPE_FORMAT_A8_UNORM:
		return NV50_2D_DST_FORMAT_8BPP;
	default:
		return -1;
	}
}

static int
nv50_surface_copy_prep(struct nouveau_context *nv,
		       struct pipe_surface *dst, struct pipe_surface *src)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *eng2d = nv->nvc->Nv2D;
	int surf_format;

	assert(src->format == dst->format);

	surf_format = nv50_format(dst->format);
	assert(surf_format >= 0);

	BEGIN_RING(chan, eng2d, NV50_2D_DMA_IN_MEMORY0, 2);
	OUT_RELOCo(chan, nouveau_buffer(src->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, eng2d, NV50_2D_DST_FORMAT, 2);
	OUT_RING  (chan, surf_format);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, NV50_2D_DST_PITCH, 5);
	OUT_RING  (chan, dst->stride);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);
	OUT_RELOCh(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);

	BEGIN_RING(chan, eng2d, NV50_2D_SRC_FORMAT, 2);
	OUT_RING  (chan, surf_format);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, NV50_2D_SRC_PITCH, 5);
	OUT_RING  (chan, src->stride);
	OUT_RING  (chan, src->width);
	OUT_RING  (chan, src->height);
	OUT_RELOCh(chan, nouveau_buffer(src->buffer)->bo, src->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, nouveau_buffer(src->buffer)->bo, src->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

	return 0;
}

static void
nv50_surface_copy(struct nouveau_context *nv, unsigned dx, unsigned dy,
		  unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *eng2d = nv->nvc->Nv2D;

	BEGIN_RING(chan, eng2d, 0x0110, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, NV50_2D_BLIT_DST_X, 12);
	OUT_RING  (chan, dx);
	OUT_RING  (chan, dy);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, sx);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, sy);
}

static void
nv50_surface_copy_done(struct nouveau_context *nv)
{
	FIRE_RING(nv->nvc->channel);
}

static int
nv50_surface_fill(struct nouveau_context *nv, struct pipe_surface *dst,
		  unsigned dx, unsigned dy, unsigned w, unsigned h,
		  unsigned value)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *eng2d = nv->nvc->Nv2D;
	int surf_format, rect_format;

	surf_format = nv50_format(dst->format);
	if (surf_format < 0)
		return 1;

	rect_format = nv50_format(dst->format);
	if (rect_format < 0)
		return 1;

	BEGIN_RING(chan, eng2d, NV50_2D_DMA_IN_MEMORY1, 1);
	OUT_RELOCo(chan, nouveau_buffer(dst->buffer)->bo,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, eng2d, NV50_2D_DST_FORMAT, 2);
	OUT_RING  (chan, surf_format);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, NV50_2D_DST_PITCH, 5);
	OUT_RING  (chan, dst->stride);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);
	OUT_RELOCh(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, nouveau_buffer(dst->buffer)->bo, dst->offset,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);

	BEGIN_RING(chan, eng2d, 0x0580, 3);
	OUT_RING  (chan, 4);
	OUT_RING  (chan, rect_format);
	OUT_RING  (chan, value);

	BEGIN_RING(chan, eng2d, NV50_2D_RECT_X1, 4);
	OUT_RING  (chan, dx);
	OUT_RING  (chan, dy);
	OUT_RING  (chan, dx + w);
	OUT_RING  (chan, dy + h);

	FIRE_RING(chan);

	return 0;
}

int
nouveau_surface_channel_create_nv50(struct nouveau_channel_context *nvc)
{
	int ret;

	ret = nouveau_grobj_alloc(nvc->channel, nvc->next_handle++, NV50_2D,
				  &nvc->Nv2D);
	if (ret)
		return ret;
	BIND_RING (nvc->channel, nvc->Nv2D, nvc->next_subchannel++);
	BEGIN_RING(nvc->channel, nvc->Nv2D, NV50_2D_DMA_NOTIFY, 1);
	OUT_RING  (nvc->channel, nvc->sync_notifier->handle);
	BEGIN_RING(nvc->channel, nvc->Nv2D, NV50_2D_DMA_IN_MEMORY0, 2);
	OUT_RING  (nvc->channel, nvc->channel->vram->handle);
	OUT_RING  (nvc->channel, nvc->channel->vram->handle);
	BEGIN_RING(nvc->channel, nvc->Nv2D, NV50_2D_OPERATION, 1);
	OUT_RING  (nvc->channel, NV50_2D_OPERATION_SRCCOPY);

	return 0;
}

int
nouveau_surface_init_nv50(struct nouveau_context *nv)
{
	nv->surface_copy_prep = nv50_surface_copy_prep;
	nv->surface_copy = nv50_surface_copy;
	nv->surface_copy_done = nv50_surface_copy_done;
	nv->surface_fill = nv50_surface_fill;
	return 0;
}

