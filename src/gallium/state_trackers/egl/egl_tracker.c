
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include "egllog.h"
#include "state_tracker/drm_api.h"

#include "pipe/p_screen.h"
#include "pipe/internal/p_winsys_screen.h"

/** HACK */
void* driDriverAPI;
extern const struct dri_extension card_extensions[];


/*
 * Exported functions
 */

/**
 * The bootstrap function.  Return a new drm_driver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy, const char *args)
{
	struct drm_device *drm;

	drm = (struct drm_device *) calloc(1, sizeof(struct drm_device));
	if (!drm) {
		return NULL;
	}

	/* First fill in the dispatch table with defaults */
	_eglInitDriverFallbacks(&drm->base);
	/* then plug in our Drm-specific functions */
	drm->base.API.Initialize = drm_initialize;
	drm->base.API.Terminate = drm_terminate;
	drm->base.API.CreateContext = drm_create_context;
	drm->base.API.MakeCurrent = drm_make_current;
	drm->base.API.CreateWindowSurface = drm_create_window_surface;
	drm->base.API.CreatePixmapSurface = drm_create_pixmap_surface;
	drm->base.API.CreatePbufferSurface = drm_create_pbuffer_surface;
	drm->base.API.DestroySurface = drm_destroy_surface;
	drm->base.API.DestroyContext = drm_destroy_context;
	drm->base.API.CreateScreenSurfaceMESA = drm_create_screen_surface_mesa;
	drm->base.API.ShowScreenSurfaceMESA = drm_show_screen_surface_mesa;
	drm->base.API.SwapBuffers = drm_swap_buffers;

	drm->base.ClientAPIsMask = EGL_OPENGL_BIT /*| EGL_OPENGL_ES_BIT*/;
	drm->base.Name = "DRM/Gallium/Win";

	/* enable supported extensions */
	drm->base.Extensions.MESA_screen_surface = EGL_TRUE;
	drm->base.Extensions.MESA_copy_context = EGL_TRUE;

	return &drm->base;
}

static void
drm_get_device_id(struct drm_device *device)
{
	char path[512];
	FILE *file;
	char *ret;

	/* TODO get the real minor */
	int minor = 0;

	device->deviceID = 0;

	snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/device", minor);
	file = fopen(path, "r");
	if (!file) {
		_eglLog(_EGL_WARNING, "Could not retrive device ID\n");
		return;
	}

	ret = fgets(path, sizeof( path ), file);
	if (!ret)
		return;

	sscanf(path, "%x", &device->deviceID);
	fclose(file);
}

static void
drm_update_res(struct drm_device *dev)
{
	drmModeFreeResources(dev->res);
	dev->res = drmModeGetResources(dev->drmFD);
}

static void
drm_add_modes_from_connector(_EGLScreen *screen, drmModeConnectorPtr connector)
{
	drmModeModeInfoPtr m = NULL;
	int i;

	for (i = 0; i < connector->count_modes; i++) {
		m = &connector->modes[i];
		_eglAddNewMode(screen, m->hdisplay, m->vdisplay, m->vrefresh, m->name);
	}
}

static void
drm_find_dpms(struct drm_device *dev, struct drm_screen *screen)
{
	drmModeConnectorPtr c = screen->connector;
	drmModePropertyPtr p;
	int i;

	for (i = 0; i < c->count_props; i++) {
		p = drmModeGetProperty(dev->drmFD, c->props[i]);
		if (!strcmp(p->name, "DPMS"))
			break;

		drmModeFreeProperty(p);
		p = NULL;
	}

	screen->dpms = p;
}

EGLBoolean
drm_initialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	_EGLDisplay *disp = _eglLookupDisplay(dpy);
	struct drm_device *dev = (struct drm_device *)drv;
	struct drm_screen *screen = NULL;
	drmModeConnectorPtr connector = NULL;
	drmModeResPtr res = NULL;
	unsigned count_connectors = 0;
	int num_screens = 0;
	EGLint i;
	int fd;

	fd = drmOpen("i915", NULL);
	if (fd < 0)
		goto err_fd;

	dev->drmFD = fd;
	drm_get_device_id(dev);

	dev->screen = drm_api_hooks.create_screen(dev->drmFD, NULL);
	if (!dev->screen)
		goto err_screen;
	dev->winsys = dev->screen->winsys;

	/* TODO HACK */
	driInitExtensions(NULL, card_extensions, GL_FALSE);

	drm_update_res(dev);
	res = dev->res;
	if (res)
		count_connectors = res->count_connectors;
	else
		_eglLog(_EGL_WARNING, "Could not retrive kms information\n");

	for(i = 0; i < count_connectors && i < MAX_SCREENS; i++) {
		connector = drmModeGetConnector(fd, res->connectors[i]);

		if (!connector)
			continue;

		if (connector->connection != DRM_MODE_CONNECTED) {
			drmModeFreeConnector(connector);
			continue;
		}

		screen = malloc(sizeof(struct drm_screen));
		memset(screen, 0, sizeof(*screen));
		screen->connector = connector;
		screen->connectorID = connector->connector_id;
		_eglInitScreen(&screen->base);
		_eglAddScreen(disp, &screen->base);
		drm_add_modes_from_connector(&screen->base, connector);
		drm_find_dpms(dev, screen);
		dev->screens[num_screens++] = screen;
	}
	dev->count_screens = num_screens;

	/* for now we only have one config */
	_EGLConfig *config = calloc(1, sizeof(*config));
	memset(config, 1, sizeof(*config));
	_eglInitConfig(config, 1);
	_eglSetConfigAttrib(config, EGL_RED_SIZE, 8);
	_eglSetConfigAttrib(config, EGL_GREEN_SIZE, 8);
	_eglSetConfigAttrib(config, EGL_BLUE_SIZE, 8);
	_eglSetConfigAttrib(config, EGL_ALPHA_SIZE, 8);
	_eglSetConfigAttrib(config, EGL_BUFFER_SIZE, 32);
	_eglSetConfigAttrib(config, EGL_DEPTH_SIZE, 24);
	_eglSetConfigAttrib(config, EGL_STENCIL_SIZE, 8);
	_eglSetConfigAttrib(config, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT);
	_eglAddConfig(disp, config);

	drv->Initialized = EGL_TRUE;

	*major = 1;
	*minor = 4;

	return EGL_TRUE;

err_screen:
	drmClose(fd);
err_fd:
	return EGL_FALSE;
}

EGLBoolean
drm_terminate(_EGLDriver *drv, EGLDisplay dpy)
{
	struct drm_device *dev = (struct drm_device *)drv;
	struct drm_screen *screen;
	int i = 0;

	drmFreeVersion(dev->version);

	for (i = 0; i < dev->count_screens; i++) {
		screen = dev->screens[i];

		if (screen->shown)
			drm_takedown_shown_screen(drv, screen);

		drmModeFreeProperty(screen->dpms);
		drmModeFreeConnector(screen->connector);
		_eglDestroyScreen(&screen->base);
		dev->screens[i] = NULL;
	}

	dev->screen->destroy(dev->screen);
	dev->winsys = NULL;

	drmClose(dev->drmFD);

	_eglCleanupDisplay(_eglLookupDisplay(dpy));
	free(dev);

	return EGL_TRUE;
}
