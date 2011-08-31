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

#include <wayland-client.h>

struct wl_egl_window {
	struct wl_surface *surface;

	int width;
	int height;
	int dx;
	int dy;

	int attached_width;
	int attached_height;
};

struct wl_egl_pixmap {
	struct wl_buffer *buffer;

	int width;
	int height;

	void (*destroy) (struct wl_egl_pixmap *egl_pixmap);

	void *driver_private;
};

#ifdef  __cplusplus
}
#endif

#endif
