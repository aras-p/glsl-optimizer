#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_string.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_mm.h"
#include "nouveau_winsys.h"
#include "nouveau_screen.h"
#include "nouveau_fence.h"

/* XXX this should go away */
#include "state_tracker/drm_driver.h"
#include "util/u_simple_screen.h"

#include "nouveau_drmif.h"

int nouveau_mesa_debug = 0;

static const char *
nouveau_screen_get_name(struct pipe_screen *pscreen)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	static char buffer[128];

	util_snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nouveau_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "nouveau";
}



struct nouveau_bo *
nouveau_screen_bo_new(struct pipe_screen *pscreen, unsigned alignment,
		      unsigned usage, unsigned bind, unsigned size)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nouveau_bo *bo = NULL;
	uint32_t flags = NOUVEAU_BO_MAP, tile_mode = 0, tile_flags = 0;
	int ret;

	if (bind & PIPE_BIND_VERTEX_BUFFER)
		flags |= nouveau_screen(pscreen)->vertex_buffer_flags;
	else if (bind & PIPE_BIND_INDEX_BUFFER)
		flags |= nouveau_screen(pscreen)->index_buffer_flags;

	if (bind & (PIPE_BIND_RENDER_TARGET |
			PIPE_BIND_DEPTH_STENCIL |
			PIPE_BIND_SCANOUT |
			PIPE_BIND_DISPLAY_TARGET |
			PIPE_BIND_SAMPLER_VIEW))
	{
		/* TODO: this may be incorrect or suboptimal */
		if (!(bind & PIPE_BIND_SCANOUT))
			flags |= NOUVEAU_BO_GART;
		if (usage != PIPE_USAGE_DYNAMIC)
			flags |= NOUVEAU_BO_VRAM;

		if (dev->chipset == 0x50 || dev->chipset >= 0x80) {
			if (bind & PIPE_BIND_DEPTH_STENCIL)
				tile_flags = 0x2800;
			else
				tile_flags = 0x7000;
		}
	}

	ret = nouveau_bo_new_tile(dev, flags, alignment, size,
				  tile_mode, tile_flags, &bo);
	if (ret)
		return NULL;

	return bo;
}

void *
nouveau_screen_bo_map(struct pipe_screen *pscreen,
		      struct nouveau_bo *bo,
		      unsigned map_flags)
{
	int ret;

	ret = nouveau_bo_map(bo, map_flags);
	if (ret) {
		debug_printf("map failed: %d\n", ret);
		return NULL;
	}

	return bo->map;
}

void *
nouveau_screen_bo_map_range(struct pipe_screen *pscreen, struct nouveau_bo *bo,
			    unsigned offset, unsigned length, unsigned flags)
{
	int ret;

	ret = nouveau_bo_map_range(bo, offset, length, flags);
	if (ret) {
		nouveau_bo_unmap(bo);
		if (!(flags & NOUVEAU_BO_NOWAIT) || ret != -EBUSY)
			debug_printf("map_range failed: %d\n", ret);
		return NULL;
	}

	return (char *)bo->map - offset; /* why gallium? why? */
}

void
nouveau_screen_bo_map_flush_range(struct pipe_screen *pscreen, struct nouveau_bo *bo,
				  unsigned offset, unsigned length)
{
	nouveau_bo_map_flush(bo, offset, length);
}

void
nouveau_screen_bo_unmap(struct pipe_screen *pscreen, struct nouveau_bo *bo)
{
	nouveau_bo_unmap(bo);
}

void
nouveau_screen_bo_release(struct pipe_screen *pscreen, struct nouveau_bo *bo)
{
	nouveau_bo_ref(NULL, &bo);
}

static void
nouveau_screen_fence_ref(struct pipe_screen *pscreen,
			 struct pipe_fence_handle **ptr,
			 struct pipe_fence_handle *pfence)
{
	nouveau_fence_ref(nouveau_fence(pfence), (struct nouveau_fence **)ptr);
}

static boolean
nouveau_screen_fence_signalled(struct pipe_screen *screen,
                               struct pipe_fence_handle *pfence)
{
        return nouveau_fence_signalled(nouveau_fence(pfence));
}

static boolean
nouveau_screen_fence_finish(struct pipe_screen *screen,
			    struct pipe_fence_handle *pfence,
                            uint64_t timeout)
{
        return nouveau_fence_wait(nouveau_fence(pfence));
}


struct nouveau_bo *
nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
			      struct winsys_handle *whandle,
			      unsigned *out_stride)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nouveau_bo *bo = 0;
	int ret;
 
	ret = nouveau_bo_handle_ref(dev, whandle->handle, &bo);
	if (ret) {
		debug_printf("%s: ref name 0x%08x failed with %d\n",
			     __FUNCTION__, whandle->handle, ret);
		return NULL;
	}

	*out_stride = whandle->stride;
	return bo;
}


boolean
nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
			     struct nouveau_bo *bo,
			     unsigned stride,
			     struct winsys_handle *whandle)
{
	whandle->stride = stride;

	if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) { 
		return nouveau_bo_handle_get(bo, &whandle->handle) == 0;
	} else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
		whandle->handle = bo->handle;
		return TRUE;
	} else {
		return FALSE;
	}
}

int
nouveau_screen_init(struct nouveau_screen *screen, struct nouveau_device *dev)
{
	struct pipe_screen *pscreen = &screen->base;
	int ret;

	char *nv_dbg = getenv("NOUVEAU_MESA_DEBUG");
	if (nv_dbg)
	   nouveau_mesa_debug = atoi(nv_dbg);

	ret = nouveau_channel_alloc(dev, 0xbeef0201, 0xbeef0202,
				    512*1024, &screen->channel);
	if (ret)
		return ret;
	screen->device = dev;

	pscreen->get_name = nouveau_screen_get_name;
	pscreen->get_vendor = nouveau_screen_get_vendor;

	pscreen->fence_reference = nouveau_screen_fence_ref;
	pscreen->fence_signalled = nouveau_screen_fence_signalled;
	pscreen->fence_finish = nouveau_screen_fence_finish;

	util_format_s3tc_init();

	screen->mm_GART = nouveau_mm_create(dev,
					    NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
					    0x000);
	screen->mm_VRAM = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, 0x000);
	return 0;
}

void
nouveau_screen_fini(struct nouveau_screen *screen)
{
	nouveau_mm_destroy(screen->mm_GART);
	nouveau_mm_destroy(screen->mm_VRAM);

	nouveau_channel_free(&screen->channel);

	nouveau_device_close(&screen->device);
}

