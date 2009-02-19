#include "util/u_memory.h"

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

	BEGIN_RING(chan, *grobj, 0x0000, 1);
	OUT_RING  (chan, (*grobj)->handle);
	(*grobj)->bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;
	return 0;
}

static int
nouveau_pipe_push_reloc(struct nouveau_winsys *nvws, void *ptr,
			struct pipe_buffer *buf, uint32_t data,
			uint32_t flags, uint32_t vor, uint32_t tor)
{
	struct nouveau_bo *bo = nouveau_pipe_buffer(buf)->bo;

	return nouveau_pushbuf_emit_reloc(nvws->channel, ptr, bo,
					  data, flags, vor, tor);
}

static int
nouveau_pipe_push_flush(struct nouveau_winsys *nvws, unsigned size,
			struct pipe_fence_handle **fence)
{
	if (fence)
		*fence = NULL;

	return nouveau_pushbuf_flush(nvws->channel, size);
}

static struct nouveau_bo *
nouveau_pipe_get_bo(struct pipe_buffer *pb)
{
	return nouveau_pipe_buffer(pb)->bo;
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
#if 0
	case 0x00:
		hws_create = nv04_screen_create;
		hw_create = nv04_create;
		break;
	case 0x10:
		hws_create = nv10_screen_create;
		hw_create = nv10_create;
		break;
	case 0x20:
		hws_create = nv20_screen_create;
		hw_create = nv20_create;
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
#endif
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

	nvws->get_bo		= nouveau_pipe_get_bo;

	ws = nouveau_create_pipe_winsys(nv);

	if (!nvc->pscreen)
		nvc->pscreen = hws_create(ws, nvws);
	nvc->pctx[nv->pctx_id] = hw_create(nvc->pscreen, nv->pctx_id);
	return nvc->pctx[nv->pctx_id];
}

