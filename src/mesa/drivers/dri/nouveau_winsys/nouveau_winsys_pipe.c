#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"

#include "nouveau_context.h"
#include "nouveau_device.h"
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

static void
nouveau_printf(struct pipe_winsys *pws, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

static const char *
nouveau_get_name(struct pipe_winsys *pws)
{
	return "Nouveau/DRI";
}

static struct pipe_region *
nouveau_region_alloc(struct pipe_winsys *ws, unsigned cpp,
		     unsigned width, unsigned height, unsigned flags)
{
	struct pipe_region *region;

	region = calloc(sizeof(*region), 1);
	region->cpp = cpp;
	region->pitch = ((cpp * width + 63) & ~63) / cpp;
	region->height = height;
	region->refcount = 1;
	region->buffer = ws->buffer_create(ws, 64);

	ws->buffer_data(ws, region->buffer, region->pitch * cpp * height, NULL,
			PIPE_BUFFER_USAGE_PIXEL);
	return region;
}

static void
nouveau_region_release(struct pipe_winsys *pws, struct pipe_region **pregion)
{
	struct pipe_region *region;

	if (!pregion || !*pregion)
		return;
	region = *pregion;
	*pregion = NULL;

	assert(region->refcount > 0);
	region->refcount--;

	if (region->refcount == 0) {
		assert(region->map_refcount == 0);
		pws->buffer_reference(pws, &region->buffer, NULL);
		free(region);
	}
}

static struct pipe_surface *
nouveau_surface_alloc(struct pipe_winsys *pws, unsigned format)
{
	struct pipe_surface *surf;
	
	surf = CALLOC_STRUCT(pipe_surface);
	if (!surf)
		return NULL;

	surf->format = format;
	surf->refcount = 1;
	surf->winsys = pws;

	return surf;
}

static void
nouveau_surface_release(struct pipe_winsys *pws, struct pipe_surface **s)
{
	struct pipe_surface *surf = *s; *s = NULL;

	if (surf->refcount-- == 0) {
		if (surf->region)
			pws->region_release(pws, &surf->region);
		free(surf);
	}
}

static struct pipe_buffer_handle *
nouveau_pipe_bo_create(struct pipe_winsys *pws, unsigned alignment)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
	struct nouveau_device *dev = nvpws->nv->nv_screen->device;
	struct nouveau_bo *nvbo = NULL;

	if (nouveau_bo_new(dev, NOUVEAU_BO_LOCAL, alignment, 0, &nvbo))
		return NULL;
	return (struct pipe_buffer_handle *)nvbo;
}

static struct pipe_buffer_handle *
nouveau_pipe_bo_user_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
	struct nouveau_device *dev = nvpws->nv->nv_screen->device;
	struct nouveau_bo *nvbo = NULL;

	if (nouveau_bo_user(dev, ptr, bytes, &nvbo))
		return NULL;
	return (struct pipe_buffer_handle *)nvbo;
}

static void *
nouveau_pipe_bo_map(struct pipe_winsys *pws, struct pipe_buffer_handle *bo,
		    unsigned flags)
{
	struct nouveau_bo *nvbo = (struct nouveau_bo *)bo;
	uint32_t map_flags = 0;

	if (flags & PIPE_BUFFER_FLAG_READ)
		map_flags |= NOUVEAU_BO_RD;
	if (flags & PIPE_BUFFER_FLAG_WRITE)
		map_flags |= NOUVEAU_BO_WR;

	if (nouveau_bo_map(nvbo, map_flags))
		return NULL;
	return nvbo->map;
}

static void
nouveau_pipe_bo_unmap(struct pipe_winsys *pws, struct pipe_buffer_handle *bo)
{
	struct nouveau_bo *nvbo = (struct nouveau_bo *)bo;

	nouveau_bo_unmap(nvbo);
}

static void
nouveau_pipe_bo_reference(struct pipe_winsys *pws,
			  struct pipe_buffer_handle **ptr,
			  struct pipe_buffer_handle *bo)
{
	struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
	struct nouveau_context *nv = nvpws->nv;
	struct nouveau_device *dev = nv->nv_screen->device;

	if (*ptr) {
		struct nouveau_bo *nvbo = (struct nouveau_bo *)*ptr;
		FIRE_RING();
		nouveau_bo_del(&nvbo);
		*ptr = NULL;
	}

	if (bo) {
		struct nouveau_bo *nvbo = (struct nouveau_bo *)bo, *new = NULL;
		nouveau_bo_ref(dev, nvbo->handle, &new);
		*ptr = bo;
	}
}

static void
nouveau_pipe_bo_data(struct pipe_winsys *pws, struct pipe_buffer_handle *bo,
		     unsigned size, const void *data, unsigned usage)
{
	struct nouveau_bo *nvbo = (struct nouveau_bo *)bo;

	if (nvbo->size != size)
		nouveau_bo_resize(nvbo, size);

	if (data) {
		if (nouveau_bo_map(nvbo, NOUVEAU_BO_WR))
			return;
		memcpy(nvbo->map, data, size);
		nouveau_bo_unmap(nvbo);
	}
}

static void
nouveau_pipe_bo_subdata(struct pipe_winsys *pws, struct pipe_buffer_handle *bo,
			unsigned long offset, unsigned long size,
			const void *data)
{
	struct nouveau_bo *nvbo = (struct nouveau_bo *)bo;

	if (nouveau_bo_map(nvbo, NOUVEAU_BO_WR))
		return;
	memcpy(nvbo->map + offset, data, size);
	nouveau_bo_unmap(nvbo);
}

static void
nouveau_pipe_bo_get_subdata(struct pipe_winsys *pws,
			    struct pipe_buffer_handle *bo, unsigned long offset,
			    unsigned long size, void *data)
{
	struct nouveau_bo *nvbo = (struct nouveau_bo *)bo;

	if (nouveau_bo_map(nvbo, NOUVEAU_BO_RD))
		return;
	memcpy(data, nvbo->map + offset, size);
	nouveau_bo_unmap(nvbo);
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
	pws->printf = nouveau_printf;

	pws->region_alloc = nouveau_region_alloc;
	pws->region_release = nouveau_region_release;

	pws->surface_alloc = nouveau_surface_alloc;
	pws->surface_release = nouveau_surface_release;

	pws->buffer_create = nouveau_pipe_bo_create;
	pws->user_buffer_create = nouveau_pipe_bo_user_create;
	pws->buffer_map = nouveau_pipe_bo_map;
	pws->buffer_unmap = nouveau_pipe_bo_unmap;
	pws->buffer_reference = nouveau_pipe_bo_reference;
	pws->buffer_data = nouveau_pipe_bo_data;
	pws->buffer_subdata = nouveau_pipe_bo_subdata;
	pws->buffer_get_subdata= nouveau_pipe_bo_get_subdata;

	pws->get_name = nouveau_get_name;

	return &nvpws->pws;
}

