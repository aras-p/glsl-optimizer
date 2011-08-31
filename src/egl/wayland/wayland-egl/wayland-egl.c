#include <stdlib.h>

#include <wayland-client.h>
#include "wayland-egl.h"
#include "wayland-egl-priv.h"

WL_EGL_EXPORT void
wl_egl_window_resize(struct wl_egl_window *egl_window,
		     int width, int height,
		     int dx, int dy)
{
	egl_window->width  = width;
	egl_window->height = height;
	egl_window->dx     = dx;
	egl_window->dy     = dy;
}

WL_EGL_EXPORT struct wl_egl_window *
wl_egl_window_create(struct wl_surface *surface,
		     int width, int height)
{
	struct wl_egl_window *egl_window;

	egl_window = malloc(sizeof *egl_window);
	if (!egl_window)
		return NULL;

	egl_window->surface = surface;
	wl_egl_window_resize(egl_window, width, height, 0, 0);
	egl_window->attached_width  = 0;
	egl_window->attached_height = 0;
	
	return egl_window;
}

WL_EGL_EXPORT void
wl_egl_window_destroy(struct wl_egl_window *egl_window)
{
	free(egl_window);
}

WL_EGL_EXPORT void
wl_egl_window_get_attached_size(struct wl_egl_window *egl_window,
				int *width, int *height)
{
	if (width)
		*width = egl_window->attached_width;
	if (height)
		*height = egl_window->attached_height;
}

WL_EGL_EXPORT struct wl_egl_pixmap *
wl_egl_pixmap_create(int width, int height, uint32_t flags)
{
	struct wl_egl_pixmap *egl_pixmap;

	egl_pixmap = malloc(sizeof *egl_pixmap);
	if (egl_pixmap == NULL)
		return NULL;

	egl_pixmap->width   = width;
	egl_pixmap->height  = height;

	egl_pixmap->destroy = NULL;
	egl_pixmap->buffer  = NULL;
	egl_pixmap->driver_private = NULL;

	return egl_pixmap;
}

WL_EGL_EXPORT void
wl_egl_pixmap_destroy(struct wl_egl_pixmap *egl_pixmap)
{
	if (egl_pixmap->destroy)
		egl_pixmap->destroy(egl_pixmap);
	free(egl_pixmap);
}

WL_EGL_EXPORT struct wl_buffer *
wl_egl_pixmap_create_buffer(struct wl_egl_pixmap *egl_pixmap)
{
	return egl_pixmap->buffer;
}
