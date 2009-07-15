#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"

#include "nouveau_drm_api.h"

#include "nouveau_drmif.h"
#include "nouveau_channel.h"
#include "nouveau_bo.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

static struct pipe_surface *
dri_surface_from_handle(struct drm_api *api, struct pipe_screen *screen,
                        unsigned handle,
                        enum pipe_format format,
                        unsigned width,
                        unsigned height,
                        unsigned pitch)
{
   struct pipe_surface *surface = NULL;
   struct pipe_texture *texture = NULL;
   struct pipe_texture templat;
   struct pipe_buffer *buf = NULL;

   buf = api->buffer_from_handle(api, screen, "front buffer", handle);
   if (!buf)
      return NULL;

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage = PIPE_TEXTURE_USAGE_PRIMARY |
                       NOUVEAU_TEXTURE_USAGE_LINEAR;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth[0] = 1;
   templat.format = format;
   templat.width[0] = width;
   templat.height[0] = height;
   pf_get_block(templat.format, &templat.block);

   texture = screen->texture_blanket(screen,
                                     &templat,
                                     &pitch,
                                     buf);

   /* we don't need the buffer from this point on */
   pipe_buffer_reference(&buf, NULL);

   if (!texture)
      return NULL;

   surface = screen->get_tex_surface(screen, texture, 0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_READ |
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* we don't need the texture from this point on */
   pipe_texture_reference(&texture, NULL);
   return surface;
}

static struct pipe_surface *
nouveau_dri1_front_surface(struct pipe_context *pipe)
{
	return nouveau_winsys_screen(pipe->screen)->front;
}

static struct dri1_api nouveau_dri1_api = {
	nouveau_dri1_front_surface,
};

static struct pipe_screen *
nouveau_drm_create_screen(struct drm_api *api, int fd,
			  struct drm_create_screen_arg *arg)
{
	struct dri1_create_screen_arg *dri1 = (void *)arg;
	struct nouveau_winsys *nvws;
	struct pipe_winsys *ws;
	struct nouveau_device *dev = NULL;
	struct pipe_screen *(*init)(struct pipe_winsys *,
				    struct nouveau_device *);
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

	nvws = CALLOC_STRUCT(nouveau_winsys);
	if (!nvws) {
		nouveau_device_close(&dev);
		return NULL;
	}
	ws = &nvws->base;

	nvws->pscreen = init(ws, dev);
	if (!nvws->pscreen) {
		ws->destroy(ws);
		return NULL;
	}

	if (arg->mode == DRM_CREATE_DRI1) {
		struct nouveau_dri *nvdri = dri1->ddx_info;
		enum pipe_format format;

		if (nvdri->bpp == 16)
			format = PIPE_FORMAT_R5G6B5_UNORM;
		else
			format = PIPE_FORMAT_A8R8G8B8_UNORM;

		nvws->front = dri_surface_from_handle(api, nvws->pscreen,
						      nvdri->front_offset,
						      format, nvdri->width,
						      nvdri->height,
						      nvdri->front_pitch *
						      (nvdri->bpp / 8));
		if (!nvws->front) {
			debug_printf("%s: error referencing front buffer\n",
				     __func__);
			ws->destroy(ws);
			return NULL;
		}

		dri1->api = &nouveau_dri1_api;
	}

	return nvws->pscreen;
}

static struct pipe_context *
nouveau_drm_create_context(struct drm_api *api, struct pipe_screen *pscreen)
{
	struct nouveau_winsys *nvws = nouveau_winsys_screen(pscreen);
	struct pipe_context *(*init)(struct pipe_screen *, unsigned);
	unsigned chipset = nouveau_screen(pscreen)->device->chipset;
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
	for (i = 0; i < nvws->nr_pctx; i++) {
		if (nvws->pctx[i] == NULL)
			break;
	}

	if (i == nvws->nr_pctx) {
		nvws->nr_pctx++;
		nvws->pctx = realloc(nvws->pctx,
				      sizeof(*nvws->pctx) * nvws->nr_pctx);
	}

	nvws->pctx[i] = init(pscreen, i);
	return nvws->pctx[i];
}

static boolean
nouveau_drm_pb_from_pt(struct drm_api *api, struct pipe_texture *pt,
		       struct pipe_buffer **ppb, unsigned *stride)
{
	return false;
}

static struct pipe_buffer *
nouveau_drm_pb_from_handle(struct drm_api *api, struct pipe_screen *pscreen,
			   const char *name, unsigned handle)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct pipe_buffer *pb;
	int ret;

	pb = CALLOC(1, sizeof(struct pipe_buffer) + sizeof(struct nouveau_bo*));
	if (!pb)
		return NULL;

	ret = nouveau_bo_handle_ref(dev, handle, (struct nouveau_bo**)(pb+1));
	if (ret) {
		debug_printf("%s: ref name 0x%08x failed with %d\n",
			     __func__, handle, ret);
		FREE(pb);
		return NULL;
	}

	pipe_reference_init(&pb->reference, 1);
	pb->screen = pscreen;
	pb->alignment = 0;
	pb->usage = PIPE_BUFFER_USAGE_GPU_READ_WRITE |
		    PIPE_BUFFER_USAGE_CPU_READ_WRITE;
	pb->size = nouveau_bo(pb)->size;
	return pb;
}

static boolean
nouveau_drm_handle_from_pb(struct drm_api *api, struct pipe_screen *pscreen,
			   struct pipe_buffer *pb, unsigned *handle)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	if (!bo)
		return FALSE;

	*handle = bo->handle;
	return TRUE;
}

static boolean
nouveau_drm_name_from_pb(struct drm_api *api, struct pipe_screen *pscreen,
			 struct pipe_buffer *pb, unsigned *handle)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	if (!bo)
		return FALSE;

	return nouveau_bo_handle_get(bo, handle) == 0;
}

struct drm_api drm_api_hooks = {
	.create_screen = nouveau_drm_create_screen,
	.create_context = nouveau_drm_create_context,
	.buffer_from_texture = nouveau_drm_pb_from_pt,
	.buffer_from_handle = nouveau_drm_pb_from_handle,
	.handle_from_buffer = nouveau_drm_handle_from_pb,
	.global_handle_from_buffer = nouveau_drm_name_from_pb,
};

struct drm_api *
drm_api_create() {
	return &drm_api_hooks;
}

