#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include <stdio.h>
#include <errno.h>

#include "nouveau/nouveau_bo.h"
#include "nouveau_winsys.h"
#include "nouveau_screen.h"

static const char *
nouveau_screen_get_name(struct pipe_screen *pscreen)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nouveau_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "nouveau";
}

static struct pipe_buffer *
nouveau_screen_bo_skel(struct pipe_screen *pscreen, struct nouveau_bo *bo,
		       unsigned alignment, unsigned usage, unsigned size)
{
	struct pipe_buffer *pb;

	pb = CALLOC(1, sizeof(struct pipe_buffer)+sizeof(struct nouveau_bo *));
	if (!pb) {
		nouveau_bo_ref(NULL, &bo);
		return NULL;
	}

	pipe_reference_init(&pb->reference, 1);
	pb->screen = pscreen;
	pb->alignment = alignment;
	pb->usage = usage;
	pb->size = size;
	*(struct nouveau_bo **)(pb + 1) = bo;
	return pb;
}

static struct pipe_buffer *
nouveau_screen_bo_new(struct pipe_screen *pscreen, unsigned alignment,
		      unsigned usage, unsigned size)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nouveau_bo *bo = NULL;
	uint32_t flags = NOUVEAU_BO_MAP, tile_mode = 0, tile_flags = 0;
	int ret;

	if (usage & NOUVEAU_BUFFER_USAGE_TRANSFER)
		flags |= NOUVEAU_BO_GART;
	else
	if (usage & PIPE_BUFFER_USAGE_VERTEX) {
		if (pscreen->get_param(pscreen, NOUVEAU_CAP_HW_VTXBUF))
			flags |= NOUVEAU_BO_GART;
	} else
	if (usage & PIPE_BUFFER_USAGE_INDEX) {
		if (pscreen->get_param(pscreen, NOUVEAU_CAP_HW_IDXBUF))
			flags |= NOUVEAU_BO_GART;
	}

	if (usage & PIPE_BUFFER_USAGE_PIXEL) {
		if (usage & NOUVEAU_BUFFER_USAGE_TEXTURE)
			flags |= NOUVEAU_BO_GART;
		if (!(usage & PIPE_BUFFER_USAGE_CPU_READ_WRITE))
			flags |= NOUVEAU_BO_VRAM;

		if (dev->chipset == 0x50 || dev->chipset >= 0x80) {
			if (usage & NOUVEAU_BUFFER_USAGE_ZETA)
				tile_flags = 0x2800;
			else
				tile_flags = 0x7000;
		}
	}

	ret = nouveau_bo_new_tile(dev, flags, alignment, size,
				  tile_mode, tile_flags, &bo);
	if (ret)
		return NULL;

	return nouveau_screen_bo_skel(pscreen, bo, alignment, usage, size);
}

static struct pipe_buffer *
nouveau_screen_bo_user(struct pipe_screen *pscreen, void *ptr, unsigned bytes)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nouveau_bo *bo = NULL;
	int ret;

	ret = nouveau_bo_user(dev, ptr, bytes, &bo);
	if (ret)
		return NULL;

	return nouveau_screen_bo_skel(pscreen, bo, 0, 0, bytes);
}

static inline uint32_t
nouveau_screen_map_flags(unsigned pipe)
{
	uint32_t flags = 0;

	if (pipe & PIPE_BUFFER_USAGE_CPU_READ)
		flags |= NOUVEAU_BO_RD;
	if (pipe & PIPE_BUFFER_USAGE_CPU_WRITE)
		flags |= NOUVEAU_BO_WR;
	if (pipe & PIPE_BUFFER_USAGE_DISCARD)
		flags |= NOUVEAU_BO_INVAL;
	if (pipe & PIPE_BUFFER_USAGE_DONTBLOCK)
		flags |= NOUVEAU_BO_NOWAIT;
	else
	if (pipe & 0 /*PIPE_BUFFER_USAGE_UNSYNCHRONIZED*/)
		flags |= NOUVEAU_BO_NOSYNC;

	return flags;
}

static void *
nouveau_screen_bo_map(struct pipe_screen *pscreen, struct pipe_buffer *pb,
		      unsigned usage)
{
	struct nouveau_bo *bo = nouveau_bo(pb);
	struct nouveau_screen *nscreen = nouveau_screen(pscreen);
	int ret;

	if (nscreen->pre_pipebuffer_map_callback) {
		ret = nscreen->pre_pipebuffer_map_callback(pscreen, pb, usage);
		if (ret) {
			debug_printf("pre_pipebuffer_map_callback failed %d\n",
				ret);
			return NULL;
		}
	}

	ret = nouveau_bo_map(bo, nouveau_screen_map_flags(usage));
	if (ret) {
		debug_printf("map failed: %d\n", ret);
		return NULL;
	}

	return bo->map;
}

static void *
nouveau_screen_bo_map_range(struct pipe_screen *pscreen, struct pipe_buffer *pb,
			    unsigned offset, unsigned length, unsigned usage)
{
	struct nouveau_bo *bo = nouveau_bo(pb);
	struct nouveau_screen *nscreen = nouveau_screen(pscreen);
	uint32_t flags = nouveau_screen_map_flags(usage);
	int ret;

	if (nscreen->pre_pipebuffer_map_callback) {
		ret = nscreen->pre_pipebuffer_map_callback(pscreen, pb, usage);
		if (ret) {
			debug_printf("pre_pipebuffer_map_callback failed %d\n",
				ret);
			return NULL;
		}
	}

	ret = nouveau_bo_map_range(bo, offset, length, flags);
	if (ret) {
		nouveau_bo_unmap(bo);
		if (!(flags & NOUVEAU_BO_NOWAIT) || ret != -EBUSY)
			debug_printf("map_range failed: %d\n", ret);
		return NULL;
	}

	return (char *)bo->map - offset; /* why gallium? why? */
}

static void
nouveau_screen_bo_map_flush(struct pipe_screen *pscreen, struct pipe_buffer *pb,
			    unsigned offset, unsigned length)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	nouveau_bo_map_flush(bo, offset, length);
}

static void
nouveau_screen_bo_unmap(struct pipe_screen *pscreen, struct pipe_buffer *pb)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	nouveau_bo_unmap(bo);
}

static void
nouveau_screen_bo_del(struct pipe_buffer *pb)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	nouveau_bo_ref(NULL, &bo);
	FREE(pb);
}

static void
nouveau_screen_fence_ref(struct pipe_screen *pscreen,
			 struct pipe_fence_handle **ptr,
			 struct pipe_fence_handle *pfence)
{
	*ptr = pfence;
}

static int
nouveau_screen_fence_signalled(struct pipe_screen *screen,
			       struct pipe_fence_handle *pfence,
			       unsigned flags)
{
	return 0;
}

static int
nouveau_screen_fence_finish(struct pipe_screen *screen,
			    struct pipe_fence_handle *pfence,
			    unsigned flags)
{
	return 0;
}

int
nouveau_screen_init(struct nouveau_screen *screen, struct nouveau_device *dev)
{
	struct pipe_screen *pscreen = &screen->base;
	int ret;

	ret = nouveau_channel_alloc(dev, 0xbeef0201, 0xbeef0202,
				    &screen->channel);
	if (ret)
		return ret;
	screen->device = dev;

	pscreen->get_name = nouveau_screen_get_name;
	pscreen->get_vendor = nouveau_screen_get_vendor;

	pscreen->buffer_create = nouveau_screen_bo_new;
	pscreen->user_buffer_create = nouveau_screen_bo_user;
	pscreen->buffer_map = nouveau_screen_bo_map;
	pscreen->buffer_map_range = nouveau_screen_bo_map_range;
	pscreen->buffer_flush_mapped_range = nouveau_screen_bo_map_flush;
	pscreen->buffer_unmap = nouveau_screen_bo_unmap;
	pscreen->buffer_destroy = nouveau_screen_bo_del;

	pscreen->fence_reference = nouveau_screen_fence_ref;
	pscreen->fence_signalled = nouveau_screen_fence_signalled;
	pscreen->fence_finish = nouveau_screen_fence_finish;

	return 0;
}

void
nouveau_screen_fini(struct nouveau_screen *screen)
{
	struct pipe_winsys *ws = screen->base.winsys;
	nouveau_channel_free(&screen->channel);
	ws->destroy(ws);
}

