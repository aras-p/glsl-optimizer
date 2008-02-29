#include "pipe/p_util.h"

#include "nouveau_context.h"
#include "nouveau_winsys_pipe.h"

#include "nouveau/nouveau_winsys.h"

static int
nouveau_pipe_notifier_alloc(struct nouveau_winsys *nvws, int count,
			    struct nouveau_notifier **notify)
{
	struct nouveau_context *nv = nvws->nv;

	return nouveau_notifier_alloc(nv->channel, nv->next_handle++,
				      count, notify);
}

static int
nouveau_pipe_grobj_alloc(struct nouveau_winsys *nvws, int grclass,
			 struct nouveau_grobj **grobj)
{
	struct nouveau_context *nv = nvws->nv;
	int ret;

	ret = nouveau_grobj_alloc(nv->channel, nv->next_handle++,
				  grclass, grobj);
	if (ret)
		return ret;

	(*grobj)->subc = nv->next_subchannel++;
	assert((*grobj)->subc <= 7);
	BEGIN_RING_GR(*grobj, 0x0000, 1);
	OUT_RING     ((*grobj)->handle);
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

int
nouveau_pipe_emit_reloc(struct nouveau_channel *chan, void *ptr,
			struct pipe_buffer *buf, uint32_t data,
			uint32_t flags, uint32_t vor, uint32_t tor)
{
	return nouveau_pushbuf_emit_reloc(chan, ptr, nouveau_buffer(buf)->bo,
					  data, flags, vor, tor);
}

struct pipe_context *
nouveau_pipe_create(struct nouveau_context *nv)
{
	struct nouveau_winsys *nvws = CALLOC_STRUCT(nouveau_winsys);
	struct pipe_screen *(*hws_create)(struct pipe_winsys *,
					  struct nouveau_winsys *,
					  unsigned chipset);
	struct pipe_context *(*hw_create)(struct pipe_screen *);
	struct pipe_winsys *ws;
	struct pipe_screen *pscreen;

	if (!nvws)
		return NULL;

	switch (nv->chipset & 0xf0) {
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
		hws_create = nv50_screen_create;
		hw_create = nv50_create;
		break;
	default:
		NOUVEAU_ERR("Unknown chipset NV%02x\n", (int)nv->chipset);
		return NULL;
	}

	nvws->nv		= nv;
	nvws->channel		= nv->channel;

	nvws->res_init		= nouveau_resource_init;
	nvws->res_alloc		= nouveau_resource_alloc;
	nvws->res_free		= nouveau_resource_free;

	nvws->push_reloc        = nouveau_pipe_emit_reloc;
	nvws->push_flush	= nouveau_pushbuf_flush;

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
	pscreen = hws_create(ws, nvws, nv->chipset);
	return hw_create(pscreen);
}

