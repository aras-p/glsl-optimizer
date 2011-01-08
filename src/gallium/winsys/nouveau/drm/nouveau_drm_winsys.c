#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "nouveau_drm_winsys.h"
#include "nouveau_drm_public.h"

#include "nouveau_drmif.h"
#include "nouveau_channel.h"
#include "nouveau_bo.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

static void
nouveau_drm_destroy_winsys(struct pipe_winsys *s)
{
	struct nouveau_winsys *nv_winsys = nouveau_winsys(s);
	struct nouveau_screen *nv_screen= nouveau_screen(nv_winsys->pscreen);
	if (nv_screen)
		nouveau_device_close(&nv_screen->device);
	FREE(nv_winsys);
}

struct pipe_screen *
nouveau_drm_screen_create(int fd)
{
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
	case 0xc0:
		init = nvc0_screen_create;
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

	return nvws->pscreen;
}
