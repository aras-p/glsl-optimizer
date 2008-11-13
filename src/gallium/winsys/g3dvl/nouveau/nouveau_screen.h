#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

/* TODO: Investigate using DRI options for interesting things */
/*#include "xmlconfig.h"*/

struct nouveau_screen {
	dri_screen_t			*dri_screen;
	struct nouveau_device		*device;
	struct nouveau_channel_context	*nvc;

	uint32_t			front_offset;
	uint32_t			front_pitch;
	uint32_t			front_cpp;
	uint32_t			front_height;
	
	/*driOptionCache		option_cache;*/
};

int nouveau_screen_create(dri_screen_t *dri_screen, dri_framebuffer_t *dri_framebuf);
void nouveau_screen_destroy(dri_screen_t *dri_screen);

#endif

