#ifndef __NOUVEAU_SCREEN_DRI_H__
#define __NOUVEAU_SCREEN_DRI_H__

#include "xmlconfig.h"

struct nouveau_screen {
	__DRIscreenPrivate *driScrnPriv;
	driOptionCache      option_cache;

	struct nouveau_device *device;

	struct pipe_screen *pscreen;
	struct pipe_surface *fb;
};

#endif
