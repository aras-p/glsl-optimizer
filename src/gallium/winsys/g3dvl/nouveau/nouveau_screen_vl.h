#ifndef __NOUVEAU_SCREEN_VL_H__
#define __NOUVEAU_SCREEN_VL_H__

#include <driclient.h>
#include <common/nouveau_screen.h>

/* TODO: Investigate using DRI options for interesting things */
/*#include "xmlconfig.h"*/

struct nouveau_screen_vl
{
	struct nouveau_screen		base;
	dri_screen_t			*dri_screen;
	/*driOptionCache		option_cache;*/
};

int nouveau_screen_create(dri_screen_t *dri_screen, dri_framebuffer_t *dri_framebuf);
void nouveau_screen_destroy(dri_screen_t *dri_screen);

#endif
