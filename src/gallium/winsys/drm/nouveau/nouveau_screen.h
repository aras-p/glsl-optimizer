#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

#include "xmlconfig.h"

struct nouveau_screen {
	__DRIscreenPrivate *driScrnPriv;
	driOptionCache      option_cache;

	struct nouveau_device *device;

	uint32_t front_offset;
	uint32_t front_pitch;
	uint32_t front_cpp;
	uint32_t front_height;

	void *nvc;
};

#endif
