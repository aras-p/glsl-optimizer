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
nv50_surface_set(struct nouveau_context *nv, struct pipe_surface *surf, int dst)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *eng2d = nv->nvc->Nv2D;
 	struct nouveau_bo *bo = nouveau_buffer(surf->buffer)->bo;
 	int surf_format, mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
 	int flags = NOUVEAU_BO_VRAM | (dst ? NOUVEAU_BO_WR : NOUVEAU_BO_RD);
  
 	surf_format = nv50_format(surf->format);
 	if (surf_format < 0)
 		return 1;
  
 	if (!nouveau_bo(bo)->tiled) {
 		BEGIN_RING(chan, eng2d, mthd, 2);
 		OUT_RING  (chan, surf_format);
 		OUT_RING  (chan, 1);
 		BEGIN_RING(chan, eng2d, mthd + 0x14, 5);
 		OUT_RING  (chan, surf->stride);
 		OUT_RING  (chan, surf->width);
 		OUT_RING  (chan, surf->height);
 		OUT_RELOCh(chan, bo, surf->offset, flags);
 		OUT_RELOCl(chan, bo, surf->offset, flags);
 	} else {
 		BEGIN_RING(chan, eng2d, mthd, 5);
 		OUT_RING  (chan, surf_format);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, 1);
 		OUT_RING  (chan, 0);
 		BEGIN_RING(chan, eng2d, mthd + 0x18, 4);
 		OUT_RING  (chan, surf->width);
 		OUT_RING  (chan, surf->height);
 		OUT_RELOCh(chan, bo, surf->offset, flags);
 		OUT_RELOCl(chan, bo, surf->offset, flags);
 	}
 
#if 0
 	if (dst) {
 		BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, surf->width);
 		OUT_RING  (chan, surf->height);
 	}
#endif
  
 	return 0;
}

static int
nv50_surface_copy_prep(struct nouveau_context *nv,
		       struct pipe_surface *dst, struct pipe_surface *src)
{
	int ret;

	assert(src->format == dst->format);

	ret = nv50_surface_set(nv, dst, 1);
	if (ret)
		return ret;

	ret = nv50_surface_set(nv, src, 0);
	if (ret)
		return ret;

	return 0;
}

static void
nv50_surface_copy(struct nouveau_context *nv, unsigned dx, unsigned dy,
		  unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct nouveau_channel *chan = nv->nvc->channel;
	struct nouveau_grobj *eng2d = nv->nvc->Nv2D;

	BEGIN_RING(chan, eng2d, 0x088c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, NV50_2D_BLIT_DST_X, 4);
	OUT_RING  (chan, dx);
	OUT_RING  (chan, dy);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
	BEGIN_RING(chan, eng2d, 0x08c0, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, 0x08d0, 4);
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
	int rect_format, ret;

	rect_format = nv50_format(dst->format);
	if (rect_format < 0)
		return 1;

	ret = nv50_surface_set(nv, dst, 1);
	if (ret)
		return ret;

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
	struct nouveau_channel *chan = nvc->channel;
	struct nouveau_grobj *eng2d = NULL;
	int ret;

	ret = nouveau_grobj_alloc(chan, nvc->next_handle++, NV50_2D, &eng2d);
	if (ret)
		return ret;
	nvc->Nv2D = eng2d;

	BIND_RING (chan, eng2d, nvc->next_subchannel++);
	BEGIN_RING(chan, eng2d, NV50_2D_DMA_NOTIFY, 4);
	OUT_RING  (chan, nvc->sync_notifier->handle);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);
	BEGIN_RING(chan, eng2d, NV50_2D_OPERATION, 1);
	OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
	BEGIN_RING(chan, eng2d, 0x0290, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, 0x0888, 1);
	OUT_RING  (chan, 1);

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

