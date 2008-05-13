#include "pipe/p_util.h"

#include "nouveau_context.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

#include "nouveau/nouveau_winsys.h"

static int
nouveau_pipe_notifier_alloc(struct nouveau_winsys *nvws, int count,
			    struct nouveau_notifier **notify)
{
	struct nouveau_context *nv = nvws->nv;

	return nouveau_notifier_alloc(nv->nvc->channel, nv->nvc->next_handle++,
				      count, notify);
}

static int
nouveau_pipe_grobj_alloc(struct nouveau_winsys *nvws, int grclass,
			 struct nouveau_grobj **grobj)
{
	struct nouveau_context *nv = nvws->nv;
	struct nouveau_channel *chan = nv->nvc->channel;
	int ret;

	ret = nouveau_grobj_alloc(chan, nv->nvc->next_handle++,
				  grclass, grobj);
	if (ret)
		return ret;

	assert(nv->nvc->next_subchannel < 7);
	BIND_RING(chan, *grobj, nv->nvc->next_subchannel++);
	return 0;
}

static int
nouveau_pipe_surface_copy(struct nouveau_winsys *nvws, struct pipe_surface *dst,
			  unsigned dx, unsigned dy, struct pipe_surface *src,
			  unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	struct nouveau_context *nv = nvws->nv;

	if (nv->surface_copy_prep(nv, dst, src))
		return 1;
	nv->surface_copy(nv, dx, dy, sx, sy, w, h);
	nv->surface_copy_done(nv);

	return 0;
}

static int
nouveau_pipe_surface_fill(struct nouveau_winsys *nvws, struct pipe_surface *dst,
			  unsigned dx, unsigned dy, unsigned w, unsigned h,
			  unsigned value)
{
	if (nvws->nv->surface_fill(nvws->nv, dst, dx, dy, w, h, value))
		return 1;
	return 0;
}

static int
nouveau_pipe_push_reloc(struct nouveau_winsys *nvws, void *ptr,
			struct pipe_buffer *buf, uint32_t data,
			uint32_t flags, uint32_t vor, uint32_t tor)
{
	return nouveau_pushbuf_emit_reloc(nvws->channel, ptr,
					  nouveau_buffer(buf)->bo,
					  data, flags, vor, tor);
}

static int
nouveau_pipe_push_flush(struct nouveau_winsys *nvws, unsigned size,
			struct pipe_fence_handle **fence)
{
	if (fence) {
		struct nouveau_pushbuf *pb = nvws->channel->pushbuf;
		struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(pb);
		struct nouveau_fence *ref = NULL;

		nouveau_fence_ref(nvpb->fence, &ref);
		*fence = (struct pipe_fence_handle *)ref;
	}

	return nouveau_pushbuf_flush(nvws->channel, size);
}

struct pipe_context *
nouveau_pipe_create(struct nouveau_context *nv)
{
	struct nouveau_channel_context *nvc = nv->nvc;
	struct nouveau_winsys *nvws = CALLOC_STRUCT(nouveau_winsys);
	struct pipe_screen *(*hws_create)(struct pipe_winsys *,
					  struct nouveau_winsys *);
	struct pipe_context *(*hw_create)(struct pipe_screen *, unsigned);
	struct pipe_winsys *ws;
	unsigned chipset = nv->nv_screen->device->chipset;

	if (!nvws)
		return NULL;

	switch (chipset & 0xf0) {
	case 0x10:
	case 0x20:
		hws_create = nv10_screen_create;
		hw_create = nv10_create;
		break;
	case 0x30:
		hws_create = nv30_screen_create;
		hw_create = nv30_create;
		break;
	case 0x40:
	case 0x60:
		hws_create = nv40_screen_create;
		hw_create = nv40_create;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
		hws_create = nv50_screen_create;
		hw_create = nv50_create;
		break;
	default:
		NOUVEAU_ERR("Unknown chipset NV%02x\n", chipset);
		return NULL;
	}

	nvws->nv		= nv;
	nvws->channel		= nv->nvc->channel;

	nvws->res_init		= nouveau_resource_init;
	nvws->res_alloc		= nouveau_resource_alloc;
	nvws->res_free		= nouveau_resource_free;

	nvws->push_reloc        = nouveau_pipe_push_reloc;
	nvws->push_flush	= nouveau_pipe_push_flush;

	nvws->grobj_alloc	= nouveau_pipe_grobj_alloc;
	nvws->grobj_free	= nouveau_grobj_free;

	nvws->notifier_alloc	= nouveau_pipe_notifier_alloc;
	nvws->notifier_free	= nouveau_notifier_free;
	nvws->notifier_reset	= nouveau_notifier_reset;
	nvws->notifier_status	= nouveau_notifier_status;
	nvws->notifier_retval	= nouveau_notifier_return_val;
	nvws->notifier_wait	= nouveau_notifier_wait_status;

	nvws->surface_copy	= nouveau_pipe_surface_copy;
	nvws->surface_fill	= nouveau_pipe_surface_fill;

	ws = nouveau_create_pipe_winsys(nv);

	if (!nvc->pscreen)
		nvc->pscreen = hws_create(ws, nvws);
	nvc->pctx[nv->pctx_id] = hw_create(nvc->pscreen, nv->pctx_id);
	return nvc->pctx[nv->pctx_id];
}

