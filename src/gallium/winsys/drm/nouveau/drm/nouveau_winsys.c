#include "util/u_memory.h"

#include "nouveau_winsys_pipe.h"

static int
nouveau_pipe_notifier_alloc(struct nouveau_winsys *nvws, int count,
			    struct nouveau_notifier **notify)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(nvws->ws);

	return nouveau_notifier_alloc(nvpws->channel, nvpws->next_handle++,
				      count, notify);
}

static int
nouveau_pipe_grobj_alloc(struct nouveau_winsys *nvws, int grclass,
			 struct nouveau_grobj **grobj)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(nvws->ws);
	struct nouveau_channel *chan = nvpws->channel;
	int ret;

	ret = nouveau_grobj_alloc(chan, nvpws->next_handle++, grclass, grobj);
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

struct nouveau_winsys *
nouveau_winsys_new(struct pipe_winsys *ws)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);
	struct nouveau_winsys *nvws;

	nvws = CALLOC_STRUCT(nouveau_winsys);
	if (!nvws)
		return NULL;

	nvws->ws		= ws;
	nvws->channel		= nvpws->channel;

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

	return nvws;
}

