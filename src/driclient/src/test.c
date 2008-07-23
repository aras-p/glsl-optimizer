#include <assert.h>
#include <stdio.h>
#include "driclient.h"

int main(int argc, char **argv)
{
	Display		*dpy;
	Window		root, window;
	
	dri_screen_t	*screen;
	dri_drawable_t	*dri_drawable;
	dri_context_t	*context;
	
	dpy = XOpenDisplay(NULL);
	root = XDefaultRootWindow(dpy);
	window = XCreateSimpleWindow(dpy, root, 0, 0, 100, 100, 0, 0, 0);

	XSelectInput(dpy, window, 0);
	XMapWindow(dpy, window);
	XSync(dpy, 0);
	
	assert(driCreateScreen(dpy, 0, &screen, NULL) == 0);
	assert(driCreateDrawable(screen, window, &dri_drawable) == 0);
	assert(driCreateContext(screen, XDefaultVisual(dpy, 0), &context) == 0);
	assert(driUpdateDrawableInfo(dri_drawable) == 0);
	
	DRI_VALIDATE_DRAWABLE_INFO(screen, dri_drawable);
	
	assert(drmGetLock(screen->fd, context->drm_context, 0) == 0);
	assert(drmUnlock(screen->fd, context->drm_context) == 0);
	
	assert(driDestroyContext(context) == 0);
	assert(driDestroyDrawable(dri_drawable) == 0);
	assert(driDestroyScreen(screen) == 0);
	
	XDestroyWindow(dpy, window);
	XCloseDisplay(dpy);
	
	return 0;
}

