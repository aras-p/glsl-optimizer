#ifndef _GGIMESA_DISPLAY_FBDEV_H
#define _GGIMESA_DISPLAY_FBDEV_H

#include <ggi/internal/ggi-dl.h>
#include <ggi/display/fbdev.h>

ggifunc_setmode GGIMesa_fbdev_setmode;
ggifunc_getapi GGIMesa_fbdev_getapi;

#define FBDEV_PRIV_MESA(vis) ((fbdev_hook_mesa *)(FBDEV_PRIV(vis)->accelpriv))

typedef struct fbdev_hook_mesa
{
	char *accel;
	int have_accel;
	void *accelpriv;
	fbdev_hook *oldpriv;	// Hooks back to the LibGGI fbdev target's private data
} fbdev_hook_mesa;

#endif /* _GGIMESA_DISPLAY_FBDEV_H */
