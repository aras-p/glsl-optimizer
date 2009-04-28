#include "util/u_memory.h"

#include "nouveau_drm_api.h"
#include "nouveau_winsys_pipe.h"

#include "nouveau_drmif.h"
#include "nouveau_channel.h"
#include "nouveau_bo.h"

static struct pipe_screen *
nouveau_drm_create_screen(int fd, struct drm_create_screen_arg *arg)
{
	struct pipe_winsys *ws;
	struct nouveau_winsys *nvws;
	struct nouveau_device *dev = NULL;
	struct pipe_screen *(*init)(struct pipe_winsys *,
				    struct nouveau_winsys *);
	int ret;

	ret = nouveau_device_open_existing(&dev, 0, fd, 0);
	if (ret)
		return NULL;

	switch (dev->chipset & 0xf0) {
	case 0x00:
		init = nv04_screen_create;
		break;
	case 0x10:
		init = nv10_screen_create;
		break;
	case 0x20:
		init = nv20_screen_create;
		break;
	case 0x30:
		init = nv30_screen_create;
		break;
	case 0x40:
	case 0x60:
		init = nv40_screen_create;
		break;
	case 0x80:
	case 0x90:
	case 0xa0:
		init = nv50_screen_create;
		break;
	default:
		debug_printf("%s: unknown chipset nv%02x\n", __func__,
			     dev->chipset);
		return NULL;
	}

	ws = nouveau_pipe_winsys_new(dev);
	if (!ws) {
		nouveau_device_close(&dev);
		return NULL;
	}

	nvws = nouveau_winsys_new(ws);
	if (!nvws) {
		ws->destroy(ws);
		return NULL;
	}

	nouveau_pipe_winsys(ws)->pscreen = init(ws, nvws);
	if (!nouveau_pipe_winsys(ws)->pscreen) {
		ws->destroy(ws);
		return NULL;
	}

	return nouveau_pipe_winsys(ws)->pscreen;
}

static struct pipe_context *
nouveau_drm_create_context(struct pipe_screen *pscreen)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_screen(pscreen);
	struct pipe_context *(*init)(struct pipe_screen *, unsigned);
	unsigned chipset = nvpws->channel->device->chipset;
	int i;

	switch (chipset & 0xf0) {
	case 0x00:
		init = nv04_create;
		break;
	case 0x10:
		init = nv10_create;
		break;
	case 0x20:
		init = nv20_create;
		break;
	case 0x30:
		init = nv30_create;
		break;
	case 0x40:
	case 0x60:
		init = nv40_create;
		break;
	case 0x80:
	case 0x90:
	case 0xa0:
		init = nv50_create;
		break;
	default:
		debug_printf("%s: unknown chipset nv%02x\n", __func__, chipset);
		return NULL;
	}

	/* Find a free slot for a pipe context, allocate a new one if needed */
	for (i = 0; i < nvpws->nr_pctx; i++) {
		if (nvpws->pctx[i] == NULL)
			break;
	}

	if (i == nvpws->nr_pctx) {
		nvpws->nr_pctx++;
		nvpws->pctx = realloc(nvpws->pctx,
				      sizeof(*nvpws->pctx) * nvpws->nr_pctx);
	}

	nvpws->pctx[i] = init(pscreen, i);
	return nvpws->pctx[i];
}

static boolean
nouveau_drm_pb_from_pt(struct pipe_texture *pt, struct pipe_buffer **ppb,
		       unsigned *stride)
{
	return false;
}

static struct pipe_buffer *
nouveau_drm_pb_from_handle(struct pipe_screen *pscreen, const char *name,
			   unsigned handle)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_screen(pscreen);
	struct nouveau_device *dev = nvpws->channel->device;
	struct nouveau_pipe_buffer *nvpb;
	int ret;

	nvpb = CALLOC_STRUCT(nouveau_pipe_buffer);
	if (!nvpb)
		return NULL;

	ret = nouveau_bo_handle_ref(dev, handle, &nvpb->bo);
	if (ret) {
		debug_printf("%s: ref name 0x%08x failed with %d\n",
			     __func__, handle, ret);
		FREE(nvpb);
		return NULL;
	}

	pipe_reference_init(&nvpb->base.reference, 1);
	nvpb->base.screen = pscreen;
	nvpb->base.alignment = 0;
	nvpb->base.usage = PIPE_BUFFER_USAGE_GPU_READ_WRITE |
			   PIPE_BUFFER_USAGE_CPU_READ_WRITE;
	nvpb->base.size = nvpb->bo->size;
	return &nvpb->base;
}

static boolean
nouveau_drm_handle_from_pb(struct pipe_screen *pscreen, struct pipe_buffer *pb,
			   unsigned *handle)
{
	struct nouveau_pipe_buffer *nvpb = nouveau_pipe_buffer(pb);

	if (!nvpb)
		return FALSE;

	*handle = nvpb->bo->handle;
	return TRUE;
}

static boolean
nouveau_drm_name_from_pb(struct pipe_screen *pscreen, struct pipe_buffer *pb,
			 unsigned *handle)
{
	struct nouveau_pipe_buffer *nvpb = nouveau_pipe_buffer(pb);

	if (!nvpb)
		return FALSE;

	return nouveau_bo_handle_get(nvpb->bo, handle) == 0;
}

struct drm_api drm_api_hooks = {
	.create_screen = nouveau_drm_create_screen,
	.create_context = nouveau_drm_create_context,
	.buffer_from_texture = nouveau_drm_pb_from_pt,
	.buffer_from_handle = nouveau_drm_pb_from_handle,
	.handle_from_buffer = nouveau_drm_handle_from_pb,
	.global_handle_from_buffer = nouveau_drm_name_from_pb,
};

