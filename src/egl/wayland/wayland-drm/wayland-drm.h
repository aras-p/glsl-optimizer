#ifndef WAYLAND_DRM_H
#define WAYLAND_DRM_H

#include "egldisplay.h"
#include "eglimage.h"

#include <wayland-server.h>

struct wl_drm;

typedef int (*authenticate_t) (_EGLDisplay *disp, uint32_t id);

struct wl_drm_buffer {
	struct wl_buffer buffer;
	struct wl_drm *drm;
	_EGLImage *image;
};

struct wl_drm *
wayland_drm_init(struct wl_display *display, _EGLDisplay *disp,
		 authenticate_t authenticate, char *device_name);

void
wayland_drm_destroy(struct wl_drm *drm);

#endif
