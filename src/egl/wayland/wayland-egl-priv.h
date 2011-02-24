#ifndef _WAYLAND_EGL_PRIV_H
#define _WAYLAND_EGL_PRIV_H

#ifdef  __cplusplus
extern "C" {
#endif

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_EGL_EXPORT __attribute__ ((visibility("default")))
#else
#define WL_EGL_EXPORT
#endif

#include <stdbool.h>
#include <wayland-client.h>

struct wl_egl_display {
	struct wl_display *display;

	struct wl_drm *drm;
	int fd;
	char *device_name;
	bool authenticated;

	void (*glFlush)(void);
};

struct wl_egl_window {
	struct wl_surface *surface;
	struct wl_visual *visual;

	int width;
	int height;
	int dx;
	int dy;

	int attached_width;
	int attached_height;
};

struct wl_egl_pixmap {
	struct wl_egl_display *display;
	struct wl_visual      *visual;

	int name;
	int width;
	int height;
	int stride;

	void (*destroy) (struct wl_egl_pixmap *egl_pixmap);

	void *driver_private;
};

#ifdef  __cplusplus
}
#endif

#endif
