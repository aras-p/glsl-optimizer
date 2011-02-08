#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <dlfcn.h>

#include <wayland-client.h>
#include <xf86drm.h>

#include "wayland-egl.h"
#include "wayland-egl-priv.h"

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
	struct wl_egl_display *egl_display = data;
	drm_magic_t magic;

	egl_display->device_name = strdup(device);

	egl_display->fd = open(egl_display->device_name, O_RDWR);

	if (egl_display->fd == -1) {
		fprintf(stderr, "wayland-egl: could not open %s (%s)",
			egl_display->device_name, strerror(errno));
		return;
	}
	drmGetMagic(egl_display->fd, &magic);
	wl_drm_authenticate(egl_display->drm, magic);
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
	struct wl_egl_display *egl_display = data;

	egl_display->authenticated = true;
}

static const struct wl_drm_listener drm_listener = {
	drm_handle_device,
	drm_handle_authenticated
};

static void
wl_display_handle_global(struct wl_display *display, uint32_t id,
			 const char *interface, uint32_t version, void *data)
{
	struct wl_egl_display *egl_display = data;

	if (strcmp(interface, "drm") == 0) {
		egl_display->drm = wl_drm_create(display, id);
		wl_drm_add_listener(egl_display->drm, &drm_listener,
				    egl_display);
	}
}

/* stolen from egl_dri2:dri2_load() */
static void *
get_flush_address() {
	void *handle;
	void *(*get_proc_address)(const char *procname);

	handle = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);
	if (handle) {
		get_proc_address = (void* (*)(const char *))
			dlsym(handle, "_glapi_get_proc_address");
		/* no need to keep a reference */
		dlclose(handle);
	}

	/*
	 * If glapi is not available, loading DRI drivers will fail.  Ideally, we
	 * should load one of libGL, libGLESv1_CM, or libGLESv2 and go on.  But if
	 * the app has loaded another one of them with RTLD_LOCAL, there may be
	 * unexpected behaviors later because there will be two copies of glapi
	 * (with global variables of the same names!) in the memory.
	 */
	if (!get_proc_address) {
		fprintf(stderr, "failed to find _glapi_get_proc_address");
		return NULL;
	}

	return get_proc_address("glFlush");
}

WL_EGL_EXPORT struct wl_egl_display *
wl_egl_display_create(struct wl_display *display)
{
	struct wl_egl_display *egl_display;

	egl_display = malloc(sizeof *egl_display);
	if (!egl_display)
		return NULL;

	egl_display->display = display;
	egl_display->drm = NULL;
	egl_display->fd = -1;
	egl_display->device_name = NULL;
	egl_display->authenticated = false;

	egl_display->glFlush = (void (*)(void)) get_flush_address();

	wl_display_add_global_listener(display, wl_display_handle_global,
			               egl_display);

	return egl_display;
}

WL_EGL_EXPORT void
wl_egl_display_destroy(struct wl_egl_display *egl_display)
{

	free(egl_display->device_name);
	close(egl_display->fd);

	wl_drm_destroy(egl_display->drm);

	free(egl_display);
}

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
wl_egl_window_create(struct wl_egl_display *egl_display,
		     struct wl_surface *surface,
		     int width, int height,
		     struct wl_visual *visual)
{
	struct wl_egl_window *egl_window;

	egl_window = malloc(sizeof *egl_window);
	if (!egl_window)
		return NULL;

	egl_window->surface = surface;
	egl_window->visual  = visual;
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
wl_egl_pixmap_create(struct wl_egl_display *egl_display,
		     int width, int height,
		     struct wl_visual *visual, uint32_t flags)
{
	struct wl_egl_pixmap *egl_pixmap;

	egl_pixmap = malloc(sizeof *egl_pixmap);
	if (egl_pixmap == NULL)
		return NULL;

	egl_pixmap->display = egl_display;
	egl_pixmap->width   = width;
	egl_pixmap->height  = height;
	egl_pixmap->visual  = visual;
	egl_pixmap->name    = 0;
	egl_pixmap->stride  = 0;

	egl_pixmap->destroy = NULL;

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
wl_egl_pixmap_create_buffer(struct wl_egl_display *egl_display,
			    struct wl_egl_pixmap *egl_pixmap)
{
	if (egl_pixmap->name == 0)
		return NULL;

	return wl_drm_create_buffer(egl_display->drm, egl_pixmap->name,
				    egl_pixmap->width, egl_pixmap->height,
				    egl_pixmap->stride, egl_pixmap->visual);
}

WL_EGL_EXPORT void
wl_egl_pixmap_flush(struct wl_egl_display *egl_display,
		    struct wl_egl_pixmap *egl_pixmap)
{
	if (egl_display->glFlush)
		egl_display->glFlush();
}
