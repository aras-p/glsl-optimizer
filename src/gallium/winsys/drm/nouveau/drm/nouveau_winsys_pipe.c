#include "pipe/internal/p_winsys_screen.h"
#include <pipe/p_defines.h>
#include <pipe/p_inlines.h>
#include <util/u_memory.h>

#include "nouveau_winsys_pipe.h"

#include "nouveau_drmif.h"
#include "nouveau_bo.h"

static const char *
nouveau_get_name(struct pipe_winsys *pws)
{
	return "Nouveau/DRI";
}

static uint32_t
nouveau_flags_from_usage(struct pipe_winsys *ws, unsigned usage)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);
	struct pipe_screen *pscreen = nvpws->pscreen;
	uint32_t flags = NOUVEAU_BO_LOCAL;

	if (usage & NOUVEAU_BUFFER_USAGE_TRANSFER)
		flags |= NOUVEAU_BO_GART;

	if (usage & PIPE_BUFFER_USAGE_PIXEL) {
		if (usage & NOUVEAU_BUFFER_USAGE_TEXTURE)
			flags |= NOUVEAU_BO_GART;
		if (!(usage & PIPE_BUFFER_USAGE_CPU_READ_WRITE))
			flags |= NOUVEAU_BO_VRAM;

		switch (nvpws->channel->device->chipset & 0xf0) {
		case 0x50:
		case 0x80:
		case 0x90:
			flags |= NOUVEAU_BO_TILED;
			if (usage & NOUVEAU_BUFFER_USAGE_ZETA)
				flags |= NOUVEAU_BO_ZTILE;
			break;
		default:
			break;
		}
	}

	if (usage & PIPE_BUFFER_USAGE_VERTEX) {
		if (pscreen->get_param(pscreen, NOUVEAU_CAP_HW_VTXBUF))
			flags |= NOUVEAU_BO_GART;
	}

	if (usage & PIPE_BUFFER_USAGE_INDEX) {
		if (pscreen->get_param(pscreen, NOUVEAU_CAP_HW_IDXBUF))
			flags |= NOUVEAU_BO_GART;
	}

	return flags;
}

static struct pipe_buffer *
nouveau_pipe_bo_create(struct pipe_winsys *ws, unsigned alignment,
		       unsigned usage, unsigned size)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);
	struct nouveau_device *dev = nvpws->channel->device;
	struct nouveau_pipe_buffer *nvbuf;
	uint32_t flags;

	nvbuf = CALLOC_STRUCT(nouveau_pipe_buffer);
	if (!nvbuf)
		return NULL;
	pipe_reference_init(&nvbuf->base.reference, 1);
	nvbuf->base.alignment = alignment;
	nvbuf->base.usage = usage;
	nvbuf->base.size = size;

	flags = nouveau_flags_from_usage(ws, usage);
	if (nouveau_bo_new(dev, flags, alignment, size, &nvbuf->bo)) {
		FREE(nvbuf);
		return NULL;
	}

	return &nvbuf->base;
}

static struct pipe_buffer *
nouveau_pipe_bo_user_create(struct pipe_winsys *ws, void *ptr, unsigned bytes)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);
	struct nouveau_device *dev = nvpws->channel->device;
	struct nouveau_pipe_buffer *nvbuf;

	nvbuf = CALLOC_STRUCT(nouveau_pipe_buffer);
	if (!nvbuf)
		return NULL;
	pipe_reference_init(&nvbuf->base.reference, 1);
	nvbuf->base.size = bytes;

	if (nouveau_bo_user(dev, ptr, bytes, &nvbuf->bo)) {
		FREE(nvbuf);
		return NULL;
	}

	return &nvbuf->base;
}

static void
nouveau_pipe_bo_del(struct pipe_buffer *buf)
{
	struct nouveau_pipe_buffer *nvbuf = nouveau_pipe_buffer(buf);

	nouveau_bo_ref(NULL, &nvbuf->bo);
	FREE(nvbuf);
}

static void *
nouveau_pipe_bo_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
		    unsigned flags)
{
	struct nouveau_pipe_buffer *nvbuf = nouveau_pipe_buffer(buf);
	uint32_t map_flags = 0;

	if (flags & PIPE_BUFFER_USAGE_CPU_READ)
		map_flags |= NOUVEAU_BO_RD;
	if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
		map_flags |= NOUVEAU_BO_WR;

	if (nouveau_bo_map(nvbuf->bo, map_flags))
		return NULL;
	return nvbuf->bo->map;
}

static void
nouveau_pipe_bo_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
	struct nouveau_pipe_buffer *nvbuf = nouveau_pipe_buffer(buf);

	nouveau_bo_unmap(nvbuf->bo);
}

static void
nouveau_pipe_fence_reference(struct pipe_winsys *ws,
			     struct pipe_fence_handle **ptr,
			     struct pipe_fence_handle *pfence)
{
	*ptr = pfence;
}

static int
nouveau_pipe_fence_signalled(struct pipe_winsys *ws,
			     struct pipe_fence_handle *pfence, unsigned flag)
{
	return 0;
}

static int
nouveau_pipe_fence_finish(struct pipe_winsys *ws,
			  struct pipe_fence_handle *pfence, unsigned flag)
{
	return 0;
}

static void
nouveau_destroy(struct pipe_winsys *ws)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);

	nouveau_device_close(&nvpws->channel->device);
	FREE(nvpws);
}

struct pipe_winsys *
nouveau_pipe_winsys_new(struct nouveau_device *dev)
{
	struct nouveau_pipe_winsys *nvpws;
	int ret;

	nvpws = CALLOC_STRUCT(nouveau_pipe_winsys);
	if (!nvpws)
		return NULL;

	ret = nouveau_channel_alloc(dev, 0xbeef0201, 0xbeef0202,
				    &nvpws->channel);
	if (ret) {
		debug_printf("%s: error opening GPU channel: %d\n",
			     __func__, ret);
		FREE(nvpws);
		return NULL;
	}
	nvpws->next_handle = 0x77000000;

	nvpws->base.buffer_create = nouveau_pipe_bo_create;
	nvpws->base.buffer_destroy = nouveau_pipe_bo_del;
	nvpws->base.user_buffer_create = nouveau_pipe_bo_user_create;
	nvpws->base.buffer_map = nouveau_pipe_bo_map;
	nvpws->base.buffer_unmap = nouveau_pipe_bo_unmap;

	nvpws->base.fence_reference = nouveau_pipe_fence_reference;
	nvpws->base.fence_signalled = nouveau_pipe_fence_signalled;
	nvpws->base.fence_finish = nouveau_pipe_fence_finish;

	nvpws->base.get_name = nouveau_get_name;
	nvpws->base.destroy = nouveau_destroy;
	return &nvpws->base;
}
