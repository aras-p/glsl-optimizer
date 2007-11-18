#include "nouveau_context.h"
#include "nouveau_winsys_pipe.h"

#include "pipe/nouveau/nouveau_winsys.h"

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

	return nouveau_grobj_alloc(nv->channel, nv->next_handle++,
				   grclass, grobj);
}

static uint32_t *
nouveau_pipe_dma_beginp(struct nouveau_grobj *grobj, int mthd, int size)
{
	struct nouveau_channel_priv *chan = nouveau_channel(grobj->channel);
	uint32_t *pushbuf;

	BEGIN_RING_CH(&chan->base, grobj, mthd, size);
	pushbuf = &chan->pushbuf[chan->dma.cur];
	chan->dma.cur += size;
#ifdef NOUVEAU_DMA_DEBUG
	chan->dma.push_free -= size;
#endif
	return pushbuf;
}

static void
nouveau_pipe_dma_kickoff(struct nouveau_channel *userchan)
{
	FIRE_RING_CH(userchan);
}

struct pipe_context *
nouveau_pipe_create(struct nouveau_context *nv)
{
	struct nouveau_winsys *nvws = CALLOC_STRUCT(nouveau_winsys);
	struct pipe_context *(*hw_create)(struct pipe_winsys *,
					  struct nouveau_winsys *,
					  unsigned);

	if (!nvws)
		return NULL;

	switch (nv->chipset & 0xf0) {
	case 0x40:
		hw_create = nv40_create;
		break;
	default:
		NOUVEAU_ERR("Unknown chipset NV%02x\n", (int)nv->chipset);
		return NULL;
	}

	nvws->nv		= nv;
	nvws->channel		= nv->channel;

	nvws->begin_ring        = nouveau_pipe_dma_beginp;
	nvws->out_reloc         = nouveau_bo_emit_reloc;
	nvws->fire_ring		= nouveau_pipe_dma_kickoff;

	nvws->grobj_alloc	= nouveau_pipe_grobj_alloc;
	nvws->grobj_free	= nouveau_grobj_free;

	nvws->notifier_alloc	= nouveau_pipe_notifier_alloc;
	nvws->notifier_free	= nouveau_notifier_free;
	nvws->notifier_reset	= nouveau_notifier_reset;
	nvws->notifier_status	= nouveau_notifier_status;
	nvws->notifier_retval	= nouveau_notifier_return_val;
	nvws->notifier_wait	= nouveau_notifier_wait_status;

	nvws->region_copy	= nv->region_copy;
	nvws->region_fill	= nv->region_fill;
	nvws->region_data	= nv->region_data;

	return hw_create(nouveau_create_pipe_winsys(nv), nvws, nv->chipset);
}

