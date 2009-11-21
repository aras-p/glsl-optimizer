
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include <fcntl.h>

#include "egllog.h"
#include "state_tracker/drm_api.h"

#include "pipe/p_screen.h"
#include "pipe/internal/p_winsys_screen.h"

/** HACK */
void* driDriverAPI;


/*
 * Exported functions
 */

/** Called by libEGL just prior to unloading/closing the driver.
 */
static void
drm_unload(_EGLDriver *drv)
{
	free(drv);
}

/**
 * The bootstrap function.  Return a new drm_driver object and
 * plug in API functions.
 * libEGL finds this function with dlopen()/dlsym() and calls it from
 * "load driver" function.
 */
_EGLDriver *
_eglMain(const char *args)
{
	_EGLDriver *drv;

	drv = (_EGLDriver *) calloc(1, sizeof(_EGLDriver));
	if (!drv) {
		return NULL;
	}

	/* First fill in the dispatch table with defaults */
	_eglInitDriverFallbacks(drv);
	/* then plug in our Drm-specific functions */
	drv->API.Initialize = drm_initialize;
	drv->API.Terminate = drm_terminate;
	drv->API.CreateContext = drm_create_context;
	drv->API.MakeCurrent = drm_make_current;
	drv->API.CreateWindowSurface = drm_create_window_surface;
	drv->API.CreatePixmapSurface = drm_create_pixmap_surface;
	drv->API.CreatePbufferSurface = drm_create_pbuffer_surface;
	drv->API.DestroySurface = drm_destroy_surface;
	drv->API.DestroyContext = drm_destroy_context;
	drv->API.CreateScreenSurfaceMESA = drm_create_screen_surface_mesa;
	drv->API.ShowScreenSurfaceMESA = drm_show_screen_surface_mesa;
	drv->API.SwapBuffers = drm_swap_buffers;

	drv->Name = "DRM/Gallium/Win";
	drv->Unload = drm_unload;

	return drv;
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
	fclose(file);
	if (!ret)
		return;

	sscanf(path, "%x", &device->deviceID);
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

static int drm_open_minor(int minor)
{
	char buf[64];

	sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, minor);
	return open(buf, O_RDWR, 0);
}

EGLBoolean
drm_initialize(_EGLDriver *drv, _EGLDisplay *disp, EGLint *major, EGLint *minor)
{
	struct drm_device *dev;
	struct drm_screen *screen = NULL;
	drmModeConnectorPtr connector = NULL;
	drmModeResPtr res = NULL;
	unsigned count_connectors = 0;
	int num_screens = 0;
	EGLint i;
	int fd;

	dev = (struct drm_device *) calloc(1, sizeof(struct drm_device));
	if (!dev)
		return EGL_FALSE;
	dev->api = drm_api_create();

	/* try the first node */
	fd = drm_open_minor(0);
	if (fd < 0)
		goto err_fd;

	dev->drmFD = fd;
	drm_get_device_id(dev);

	dev->screen = dev->api->create_screen(dev->api, dev->drmFD, NULL);
	if (!dev->screen)
		goto err_screen;
	dev->winsys = dev->screen->winsys;

	driInitExtensions(NULL, NULL, GL_FALSE);

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

	disp->DriverData = dev;

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

	disp->ClientAPIsMask = EGL_OPENGL_BIT /*| EGL_OPENGL_ES_BIT*/;
	/* enable supported extensions */
	disp->Extensions.MESA_screen_surface = EGL_TRUE;
	disp->Extensions.MESA_copy_context = EGL_TRUE;

	*major = 1;
	*minor = 4;

	return EGL_TRUE;

err_screen:
	drmClose(fd);
err_fd:
	free(dev);
	return EGL_FALSE;
}

EGLBoolean
drm_terminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	struct drm_screen *screen;
	int i = 0;

	_eglReleaseDisplayResources(drv, dpy);
	_eglCleanupDisplay(dpy);

	drmFreeVersion(dev->version);

	for (i = 0; i < dev->count_screens; i++) {
		screen = dev->screens[i];

		if (screen->shown)
			drm_takedown_shown_screen(dpy, screen);

		drmModeFreeProperty(screen->dpms);
		drmModeFreeConnector(screen->connector);
		_eglDestroyScreen(&screen->base);
		dev->screens[i] = NULL;
	}

	dev->screen->destroy(dev->screen);
	dev->winsys = NULL;

	drmClose(dev->drmFD);

	dev->api->destroy(dev->api);
	free(dev);
	dpy->DriverData = NULL;

	return EGL_TRUE;
}
