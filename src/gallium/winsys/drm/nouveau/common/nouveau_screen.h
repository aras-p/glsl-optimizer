#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

#include <stdint.h>

struct nouveau_device;
struct nouveau_dri;

struct nouveau_screen {
	struct nouveau_device *device;

	uint32_t front_offset;
	uint32_t front_pitch;
	uint32_t front_cpp;
	uint32_t front_height;

	void *nvc;
};

int
nouveau_screen_init(struct nouveau_dri *nv_dri, int dev_fd,
                    struct nouveau_screen *nv_screen);

void
nouveau_screen_cleanup(struct nouveau_screen *nv_screen);

#endif
