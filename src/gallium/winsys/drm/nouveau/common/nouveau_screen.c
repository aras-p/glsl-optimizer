#include <util/u_memory.h>
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"

int
nouveau_screen_init(struct nouveau_dri *nv_dri, int dev_fd,
                    struct nouveau_screen *nv_screen)
{
	int ret;

	ret = nouveau_device_open_existing(&nv_screen->device, 0,
	                                   dev_fd, 0);
	if (ret) {
		NOUVEAU_ERR("Failed opening nouveau device: %d\n", ret);
		return 1;
	}

	nv_screen->front_offset = nv_dri->front_offset;
	nv_screen->front_pitch  = nv_dri->front_pitch * (nv_dri->bpp / 8);
	nv_screen->front_cpp = nv_dri->bpp / 8;
	nv_screen->front_height = nv_dri->height;

	return 0;
}

void
nouveau_screen_cleanup(struct nouveau_screen *nv_screen)
{
	nouveau_device_close(&nv_screen->device);
}
