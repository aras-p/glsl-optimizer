#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "nouveau_drm_api.h"

#include "nouveau_drmif.h"
#include "nouveau_channel.h"
#include "nouveau_bo.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

static struct pipe_surface *
dri_surface_from_handle(struct drm_api *api, struct pipe_screen *pscreen,
                        unsigned handle, enum pipe_format format,
                        unsigned width, unsigned height, unsigned pitch)
{
	struct pipe_surface *ps = NULL;
	struct pipe_texture *pt = NULL;
	struct pipe_texture tmpl;
	struct winsys_handle whandle;

	memset(&tmpl, 0, sizeof(tmpl));
	tmpl.tex_usage = PIPE_TEXTURE_USAGE_SCANOUT;
	tmpl.target = PIPE_TEXTURE_2D;
	tmpl.last_level = 0;
	tmpl.depth0 = 1;
	tmpl.format = format;
	tmpl.width0 = width;
	tmpl.height0 = height;

	memset(&whandle, 0, sizeof(whandle));
	whandle.stride = pitch;
	whandle.handle = handle;

	pt = pscreen->texture_from_handle(pscreen, &tmpl, &whandle);
	if (!pt)
		return NULL;

	ps = pscreen->get_tex_surface(pscreen, pt, 0, 0, 0,
				      PIPE_BUFFER_USAGE_GPU_READ |
				      PIPE_BUFFER_USAGE_GPU_WRITE);

	/* we don't need the texture from this point on */
	pipe_texture_reference(&pt, NULL);
	return ps;
}

static struct pipe_surface *
nouveau_dri1_front_surface(struct pipe_context *pipe)
{
	return nouveau_winsys_screen(pipe->screen)->front;
}

static struct dri1_api nouveau_dri1_api = {
	nouveau_dri1_front_surface,
};

static void
nouveau_drm_destroy_winsys(struct pipe_winsys *s)
{
	struct nouveau_winsys *nv_winsys = nouveau_winsys(s);
	struct nouveau_screen *nv_screen= nouveau_screen(nv_winsys->pscreen);
	nouveau_device_close(&nv_screen->device);
	FREE(nv_winsys);
}

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
	case 0x30:
	case 0x40:
	case 0x60:
		init = nvfx_screen_create;
		break;
	case 0x50:
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
	ws->destroy = nouveau_drm_destroy_winsys;

	nvws->pscreen = init(ws, dev);
	if (!nvws->pscreen) {
		ws->destroy(ws);
		return NULL;
	}

	if (arg && arg->mode == DRM_CREATE_DRI1) {
		struct nouveau_dri *nvdri = dri1->ddx_info;
		enum pipe_format format;

		if (nvdri->bpp == 16)
			format = PIPE_FORMAT_B5G6R5_UNORM;
		else
			format = PIPE_FORMAT_B8G8R8A8_UNORM;

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

struct drm_api drm_api_hooks = {
	.name = "nouveau",
	.driver_name = "nouveau",
	.create_screen = nouveau_drm_create_screen,
};

struct drm_api *
drm_api_create() {
	return &drm_api_hooks;
}

