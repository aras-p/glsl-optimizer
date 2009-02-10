#ifndef __NOUVEAU_SCREEN_DRI_H__
#define __NOUVEAU_SCREEN_DRI_H__

#include "../common/nouveau_screen.h"
#include "xmlconfig.h"

struct nouveau_screen_dri {
	struct nouveau_screen base;
	__DRIscreenPrivate *driScrnPriv;
	driOptionCache      option_cache;
};

#endif
