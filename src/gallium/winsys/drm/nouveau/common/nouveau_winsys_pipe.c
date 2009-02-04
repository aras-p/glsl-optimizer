#include "pipe/internal/p_winsys_screen.h"
#include <pipe/p_defines.h>
#include <pipe/p_inlines.h>
#include <util/u_memory.h>
#include "nouveau_context.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

static const char *
nouveau_get_name(struct pipe_winsys *pws)
{
	return "Nouveau/DRI";
}

static uint32_t
nouveau_flags_from_usage(struct nouveau_context *nv, unsigned usage)
{
	struct nouveau_device *dev = nv->nv_screen->device;
	uint32_t flags = NOUVEAU_BO_LOCAL;

	if (usage & PIPE_BUFFER_USAGE_PIXEL) {
		if (usage & NOUVEAU_BUFFER_USAGE_TEXTURE)
			flags |= NOUVEAU_BO_GART;
		if (!(usage & PIPE_BUFFER_USAGE_CPU_READ_WRITE))
			flags |= NOUVEAU_BO_VRAM;

		switch (dev->chipset & 0xf0) {
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
		if (nv->cap.hw_vertex_buffer)
			flags |= NOUVEAU_BO_GART;
	}

	if (usage & PIPE_BUFFER_USAGE_INDEX) {
		if (nv->cap.hw_index_buffer)
			flags |= NOUVEAU_BO_GART;
	}

	return flags;
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

	nvbuf = CALLOC_STRUCT(nouveau_pipe_buffer);
	if (!nvbuf)
		return NULL;
	nvbuf->base.refcount = 1;
	nvbuf->base.alignment = alignment;
	nvbuf->base.usage = usage;
	nvbuf->base.size = size;

	flags = nouveau_flags_from_usage(nv, usage);

	if (nouveau_bo_new(dev, flags, alignment, size, &nvbuf->bo)) {
		FREE(nvbuf);
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

	nvbuf = CALLOC_STRUCT(nouveau_pipe_buffer);
	if (!nvbuf)
		return NULL;
	nvbuf->base.refcount = 1;
	nvbuf->base.size = bytes;

	if (nouveau_bo_user(dev, ptr, bytes, &nvbuf->bo)) {
		FREE(nvbuf);
		return NULL;
	}

	return &nvbuf->base;
}

static void
nouveau_pipe_bo_del(struct pipe_winsys *ws, struct pipe_buffer *buf)
{
	struct nouveau_pipe_buffer *nvbuf = (void *)buf;

	nouveau_bo_ref(NULL, &nvbuf->bo);
	FREE(nvbuf);
}

static void *
nouveau_pipe_bo_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
		    unsigned flags)
{
	struct nouveau_pipe_buffer *nvbuf = (void *)buf;
	uint32_t map_flags = 0;

	if (flags & PIPE_BUFFER_USAGE_CPU_READ)
		map_flags |= NOUVEAU_BO_RD;
	if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
		map_flags |= NOUVEAU_BO_WR;

#if 0
	if (flags & PIPE_BUFFER_USAGE_DISCARD &&
	    !(flags & PIPE_BUFFER_USAGE_CPU_READ) &&
	    nouveau_bo_busy(nvbuf->bo, map_flags)) {
		struct nouveau_pipe_winsys *nvpws = (struct nouveau_pipe_winsys *)pws;
		struct nouveau_context *nv = nvpws->nv;
		struct nouveau_device *dev = nv->nv_screen->device;
		struct nouveau_bo *rename;
		uint32_t flags = nouveau_flags_from_usage(nv, buf->usage);

		if (!nouveau_bo_new(dev, flags, buf->alignment, buf->size, &rename)) {
			nouveau_bo_ref(NULL, &nvbuf->bo);
			nvbuf->bo = rename;
		}
	}
#endif

	if (nouveau_bo_map(nvbuf->bo, map_flags))
		return NULL;
	return nvbuf->bo->map;
}

static void
nouveau_pipe_bo_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
	struct nouveau_pipe_buffer *nvbuf = (void *)buf;

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

struct pipe_surface *
nouveau_surface_buffer_ref(struct nouveau_context *nv, struct pipe_buffer *pb,
			   enum pipe_format format, int w, int h,
			   unsigned pitch, struct pipe_texture **ppt)
{
	struct pipe_screen *pscreen = nv->nvc->pscreen;
	struct pipe_texture tmpl, *pt;
	struct pipe_surface *ps;

	memset(&tmpl, 0, sizeof(tmpl));
	tmpl.tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
			 NOUVEAU_TEXTURE_USAGE_LINEAR;
	tmpl.target = PIPE_TEXTURE_2D;
	tmpl.width[0] = w;
	tmpl.height[0] = h;
	tmpl.depth[0] = 1;
	tmpl.format = format;
	pf_get_block(tmpl.format, &tmpl.block);
	tmpl.nblocksx[0] = pf_get_nblocksx(&tmpl.block, w);
	tmpl.nblocksy[0] = pf_get_nblocksy(&tmpl.block, h);

	pt = pscreen->texture_blanket(pscreen, &tmpl, &pitch, pb);
	if (!pt)
		return NULL;

	ps = pscreen->get_tex_surface(pscreen, pt, 0, 0, 0,
				      PIPE_BUFFER_USAGE_GPU_WRITE);

	*ppt = pt;
	return ps;
}

static void
nouveau_destroy(struct pipe_winsys *pws)
{
	FREE(pws);
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

	pws->buffer_create = nouveau_pipe_bo_create;
	pws->buffer_destroy = nouveau_pipe_bo_del;
	pws->user_buffer_create = nouveau_pipe_bo_user_create;
	pws->buffer_map = nouveau_pipe_bo_map;
	pws->buffer_unmap = nouveau_pipe_bo_unmap;

	pws->fence_reference = nouveau_pipe_fence_reference;
	pws->fence_signalled = nouveau_pipe_fence_signalled;
	pws->fence_finish = nouveau_pipe_fence_finish;

	pws->get_name = nouveau_get_name;
	pws->destroy = nouveau_destroy;

	return &nvpws->pws;
}
