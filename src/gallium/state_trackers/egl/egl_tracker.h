
#ifndef _EGL_TRACKER_H_
#define _EGL_TRACKER_H_

#include <stdint.h>

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"

#include "xf86drm.h"
#include "xf86drmMode.h"

#include "pipe/p_compiler.h"

#include "state_tracker/st_public.h"

#define MAX_SCREENS 16

struct pipe_winsys;
struct pipe_screen;
struct pipe_context;
struct state_tracker;

struct drm_screen;
struct drm_context;

struct drm_device
{
	/*
	 * pipe
	 */

	struct drm_api *api;
	struct pipe_winsys *winsys;
	struct pipe_screen *screen;

	/*
	 * drm
	 */

	int drmFD;
	drmVersionPtr version;
	int deviceID;

	drmModeResPtr res;

	struct drm_screen *screens[MAX_SCREENS];
	size_t count_screens;
};

struct drm_surface
{
	_EGLSurface base; /* base class/object */

	/*
	 * pipe
	 */


	struct st_framebuffer *stfb;

	/*
	 * drm
	 */

	struct drm_screen *screen;

	int w;
	int h;
};

struct drm_context
{
	_EGLContext base; /* base class/object */

	/* pipe */

	struct pipe_context *pipe;
	struct st_context *st;
};

struct drm_screen
{
	_EGLScreen base;

	/*
	 * pipe
	 */

	struct pipe_texture *tex;
	struct pipe_surface *surface;

	/*
	 * drm
	 */

	struct {
		unsigned height;
		unsigned width;
		unsigned pitch;
		unsigned handle;
	} front;

	/* currently only support one connector */
	drmModeConnectorPtr connector;
	uint32_t connectorID;

	/* dpms property */
	drmModePropertyPtr dpms;

	/* Has this screen been shown */
	int shown;

	/* Surface that is currently attached to this screen */
	struct drm_surface *surf;

	/* framebuffer */
	drmModeFBPtr fb;
	uint32_t fbID;

	/* crtc and mode used */
	/*drmModeCrtcPtr crtc;*/
	uint32_t crtcID;

	drmModeModeInfoPtr mode;
};


static INLINE struct drm_device *
lookup_drm_device(_EGLDisplay *d)
{
	return (struct drm_device *) d->DriverData;
}


static INLINE struct drm_context *
lookup_drm_context(_EGLContext *c)
{
	return (struct drm_context *) c;
}


static INLINE struct drm_surface *
lookup_drm_surface(_EGLSurface *s)
{
	return (struct drm_surface *) s;
}

static INLINE struct drm_screen *
lookup_drm_screen(_EGLScreen *s)
{
	return (struct drm_screen *) s;
}

/**
 * egl_visual.h
 */
/*@{*/
void drm_visual_modes_destroy(__GLcontextModes *modes);
__GLcontextModes* drm_visual_modes_create(unsigned count, size_t minimum_size);
__GLcontextModes* drm_visual_from_config(_EGLConfig *conf);
/*@}*/

/**
 * egl_surface.h
 */
/*@{*/
void drm_takedown_shown_screen(_EGLDisplay *dpy, struct drm_screen *screen);
/*@}*/

/**
 * All function exported to the egl side.
 */
/*@{*/
EGLBoolean drm_initialize(_EGLDriver *drv, _EGLDisplay *dpy, EGLint *major, EGLint *minor);
EGLBoolean drm_terminate(_EGLDriver *drv, _EGLDisplay *dpy);
_EGLContext *drm_create_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list);
EGLBoolean drm_destroy_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *context);
_EGLSurface *drm_create_window_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativeWindowType window, const EGLint *attrib_list);
_EGLSurface *drm_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativePixmapType pixmap, const EGLint *attrib_list);
_EGLSurface *drm_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, const EGLint *attrib_list);
_EGLSurface *drm_create_screen_surface_mesa(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, const EGLint *attrib_list);
EGLBoolean drm_show_screen_surface_mesa(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *screen, _EGLSurface *surface, _EGLMode *mode);
EGLBoolean drm_destroy_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface);
EGLBoolean drm_make_current(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw, _EGLSurface *read, _EGLContext *context);
EGLBoolean drm_swap_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw);
/*@}*/

#endif
