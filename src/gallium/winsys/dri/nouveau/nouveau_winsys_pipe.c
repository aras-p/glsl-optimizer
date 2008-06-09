#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"

#include "nouveau_context.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"
#include "nouveau_winsys_pipe.h"

static void
nouveau_flush_frontbuffer(struct pipe_winsys *pws, struct pipe_surface *surf,
			  void *context_private)
{
	struct nouveau_context *nv = context_private;
	__DRIdrawablePrivate *dPriv = nv->dri_drawable;

	nouveau_copy_buffer(dPriv, surf, NULL);
}

static const char *
nouveau_get_name(struct pipe_winsys *pws)
{
	return "Nouveau/DRI";
}

static struct pipe_surface *
nouveau_surface_alloc(struct pipe_winsys *ws)
{
	struct pipe_surface *surf;
	
	surf = CALLOC_STRUCT(pipe_surface);
	if (!surf)
		return NULL;

	surf->refcount = 1;
	surf->winsys = ws;
	return surf;
}

static void
nouveau_surface_release(struct pipe_winsys *ws, struct pipe_surface **s)
{
	struct pipe_surface *surf = *s;

	*s = NULL;
	if (--surf->refcount <= 0) {
		if (surf->buffer)
			pipe_buffer_reference(ws, &surf->buffer, NULL);
		free(surf);
	}
}

static struct pipe_buffer *
nouveau_pipe_bo_create(struct pipe_winsys *pws, unsigned alignment,
		       unsigned usage, unsigned size)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
	struct nouveau_context *nv = nvpws->nv;
	struct nouveau_device *dev = nv->nv_screen->device;
	struct nouveau_pipe_buffer *nvbuf;
	uint32_t flags;

	nvbuf = calloc(1, sizeof(*nvbuf));
	if (!nvbuf)
		return NULL;
	nvbuf->base.refcount = 1;
	nvbuf->base.alignment = alignment;
	nvbuf->base.usage = usage;
	nvbuf->base.size = size;

	flags = NOUVEAU_BO_LOCAL;

	if (usage & PIPE_BUFFER_USAGE_PIXEL) {
		if (usage & NOUVEAU_BUFFER_USAGE_TEXTURE)
			flags |= NOUVEAU_BO_GART;
		flags |= NOUVEAU_BO_VRAM;
	}

	if (usage & PIPE_BUFFER_USAGE_VERTEX) {
		if (nv->cap.hw_vertex_buffer)
			flags |= NOUVEAU_BO_GART;
	}

	if (usage & PIPE_BUFFER_USAGE_INDEX) {
		if (nv->cap.hw_index_buffer)
			flags |= NOUVEAU_BO_GART;
	}

	if (nouveau_bo_new(dev, flags, alignment, size, &nvbuf->bo)) {
		free(nvbuf);
		return NULL;
	}

	return &nvbuf->base;
}

static struct pipe_buffer *
nouveau_pipe_bo_user_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
	struct nouveau_device *dev = nvpws->nv->nv_screen->device;
	struct nouveau_pipe_buffer *nvbuf;

	nvbuf = calloc(1, sizeof(*nvbuf));
	if (!nvbuf)
		return NULL;
	nvbuf->base.refcount = 1;
	nvbuf->base.size = bytes;

	if (nouveau_bo_user(dev, ptr, bytes, &nvbuf->bo)) {
		free(nvbuf);
		return NULL;
	}

	return &nvbuf->base;
}

static void
nouveau_pipe_bo_del(struct pipe_winsys *ws, struct pipe_buffer *buf)
{
	struct nouveau_pipe_buffer *nvbuf = nouveau_buffer(buf);

	nouveau_bo_del(&nvbuf->bo);
	free(nvbuf);
}

static void *
nouveau_pipe_bo_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
		    unsigned flags)
{
	struct nouveau_pipe_buffer *nvbuf = nouveau_buffer(buf);
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
	struct nouveau_pipe_buffer *nvbuf = nouveau_buffer(buf);

	nouveau_bo_unmap(nvbuf->bo);
}

static INLINE struct nouveau_fence *
nouveau_pipe_fence(struct pipe_fence_handle *pfence)
{
	return (struct nouveau_fence *)pfence;
}

static void
nouveau_pipe_fence_reference(struct pipe_winsys *ws,
			     struct pipe_fence_handle **ptr,
			     struct pipe_fence_handle *pfence)
{
	nouveau_fence_ref((void *)pfence, (void *)ptr);
}

static int
nouveau_pipe_fence_signalled(struct pipe_winsys *ws,
			     struct pipe_fence_handle *pfence, unsigned flag)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)ws;
	struct nouveau_fence *fence = nouveau_pipe_fence(pfence);

	if (nouveau_fence(fence)->signalled == 0)
		nouveau_fence_flush(nvpws->nv->nvc->channel);

	return !nouveau_fence(fence)->signalled;
}

static int
nouveau_pipe_fence_finish(struct pipe_winsys *ws,
			  struct pipe_fence_handle *pfence, unsigned flag)
{
	struct nouveau_fence *fence = nouveau_pipe_fence(pfence);
	struct nouveau_fence *ref = NULL;

	nouveau_fence_ref(fence, &ref);
	return nouveau_fence_wait(&ref);
}

struct pipe_winsys *
nouveau_create_pipe_winsys(struct nouveau_context *nv)
{
	struct nouveau_pipe_winsys *nvpws;
	struct pipe_winsys *pws;

	nvpws = CALLOC_STRUCT(nouveau_pipe_winsys);
	if (!nvpws)
		return NULL;
	nvpws->nv = nv;
	pws = &nvpws->pws;

	pws->flush_frontbuffer = nouveau_flush_frontbuffer;

	pws->surface_alloc = nouveau_surface_alloc;
	pws->surface_release = nouveau_surface_release;

	pws->buffer_create = nouveau_pipe_bo_create;
	pws->buffer_destroy = nouveau_pipe_bo_del;
	pws->user_buffer_create = nouveau_pipe_bo_user_create;
	pws->buffer_map = nouveau_pipe_bo_map;
	pws->buffer_unmap = nouveau_pipe_bo_unmap;

	pws->fence_reference = nouveau_pipe_fence_reference;
	pws->fence_signalled = nouveau_pipe_fence_signalled;
	pws->fence_finish = nouveau_pipe_fence_finish;

	pws->get_name = nouveau_get_name;

	return &nvpws->pws;
}

