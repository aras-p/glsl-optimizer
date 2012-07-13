#ifndef WAYLAND_DRM_H
#define WAYLAND_DRM_H

#include <wayland-server.h>
#include "wayland-drm-server-protocol.h"

struct wl_drm;

struct wl_drm_buffer {
	struct wl_buffer buffer;
	struct wl_drm *drm;
	uint32_t format;
        const void *driver_format;
        int32_t offset[3];
        int32_t stride[3];
	void *driver_buffer;
};

struct wayland_drm_callbacks {
	int (*authenticate)(void *user_data, uint32_t id);

	void (*reference_buffer)(void *user_data, uint32_t name,
                                 struct wl_drm_buffer *buffer);

	void (*release_buffer)(void *user_data, struct wl_drm_buffer *buffer);
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
