#ifndef WAYLAND_DRM_H
#define WAYLAND_DRM_H

#include "egldisplay.h"
#include "eglimage.h"

#include <wayland-server.h>
#include "wayland-drm-server-protocol.h"

struct wl_drm;

struct wayland_drm_callbacks {
	int (*authenticate)(void *user_data, uint32_t id);

	void *(*reference_buffer)(void *user_data, uint32_t name,
				  int32_t width, int32_t height,
				  uint32_t stride, uint32_t format);

	void (*release_buffer)(void *user_data, void *buffer);
};

struct wl_drm *
wayland_drm_init(struct wl_display *display, char *device_name,
		 struct wayland_drm_callbacks *callbacks, void *user_data);

void
wayland_drm_uninit(struct wl_drm *drm);

int
wayland_buffer_is_drm(struct wl_buffer *buffer);

uint32_t
wayland_drm_buffer_get_format(struct wl_buffer *buffer_base);

void *
wayland_drm_buffer_get_buffer(struct wl_buffer *buffer);

#endif
