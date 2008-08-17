
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"
#include "egllog.h"

#include "intel_egl.h"

#include "xf86drm.h"
#include "xf86drmMode.h"

#include "intel_context.h"

#include "state_tracker/st_public.h"

#define MAX_SCREENS 16

static void
drm_get_device_id(struct egl_drm_device *device)
{
	char path[512];
	FILE *file;

	/* TODO get the real minor */
	int minor = 0;

	snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/device", minor);
	file = fopen(path, "r");
	if (!file) {
		_eglLog(_EGL_WARNING, "Could not retrive device ID\n");
		return;
	}

	fgets(path, sizeof( path ), file);
	sscanf(path, "%x", &device->deviceID);
	fclose(file);
}

static struct egl_drm_device*
egl_drm_create_device(int drmFD)
{
	struct egl_drm_device *device = malloc(sizeof(*device));
	memset(device, 0, sizeof(*device));
	device->drmFD = drmFD;

	device->version = drmGetVersion(device->drmFD);

	drm_get_device_id(device);

	if (!intel_create_device(device)) {
		free(device);
		return NULL;
	}

	return device;
}

static void
_egl_context_modes_destroy(__GLcontextModes *modes)
{
   _eglLog(_EGL_DEBUG, "%s", __FUNCTION__);

   while (modes) {
      __GLcontextModes * const next = modes->next;
      free(modes);
      modes = next;
   }
}
/**
 * Create a linked list of 'count' GLcontextModes.
 * These are used during the client/server visual negotiation phase,
 * then discarded.
 */
static __GLcontextModes *
_egl_context_modes_create(unsigned count, size_t minimum_size)
{
   /* This code copied from libGLX, and modified */
   const size_t size = (minimum_size > sizeof(__GLcontextModes))
       ? minimum_size : sizeof(__GLcontextModes);
   __GLcontextModes * head = NULL;
   __GLcontextModes ** next;
   unsigned   i;

   _eglLog(_EGL_DEBUG, "%s %d %d", __FUNCTION__, count, minimum_size);

   next = & head;
   for (i = 0 ; i < count ; i++) {
      *next = (__GLcontextModes *) calloc(1, size);
      if (*next == NULL) {
	 _egl_context_modes_destroy(head);
	 head = NULL;
	 break;
      }
      
      (*next)->doubleBufferMode = 1;
      (*next)->visualID = GLX_DONT_CARE;
      (*next)->visualType = GLX_DONT_CARE;
      (*next)->visualRating = GLX_NONE;
      (*next)->transparentPixel = GLX_NONE;
      (*next)->transparentRed = GLX_DONT_CARE;
      (*next)->transparentGreen = GLX_DONT_CARE;
      (*next)->transparentBlue = GLX_DONT_CARE;
      (*next)->transparentAlpha = GLX_DONT_CARE;
      (*next)->transparentIndex = GLX_DONT_CARE;
      (*next)->xRenderable = GLX_DONT_CARE;
      (*next)->fbconfigID = GLX_DONT_CARE;
      (*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;
      (*next)->bindToTextureRgb = GLX_DONT_CARE;
      (*next)->bindToTextureRgba = GLX_DONT_CARE;
      (*next)->bindToMipmapTexture = GLX_DONT_CARE;
      (*next)->bindToTextureTargets = 0;
      (*next)->yInverted = GLX_DONT_CARE;

      next = & ((*next)->next);
   }

   return head;
}

struct drm_screen;

struct drm_driver
{
	_EGLDriver base;  /* base class/object */

	drmModeResPtr res;

	struct drm_screen *screens[MAX_SCREENS];
	size_t count_screens;

	struct egl_drm_device *device;
};

struct drm_surface
{
	_EGLSurface base;  /* base class/object */

	struct egl_drm_drawable *drawable;
};

struct drm_context
{
	_EGLContext base;  /* base class/object */

	struct egl_drm_context *context;
};

struct drm_screen
{
	_EGLScreen base;

	/* currently only support one connector */
	drmModeConnectorPtr connector;

	/* Has this screen been shown */
	int shown;

	/* Surface that is currently attached to this screen */
	struct drm_surface *surf;

	/* backing buffer */
	drmBO buffer;

	/* framebuffer */
	drmModeFBPtr fb;
	uint32_t fbID;

	/* crtc and mode used */
	drmModeCrtcPtr crtc;
	uint32_t crtcID;

	struct drm_mode_modeinfo *mode;

	/* geometry of the screen */
	struct egl_drm_frontbuffer front;
};

static void
drm_update_res(struct drm_driver *drm_drv)
{
	drmModeFreeResources(drm_drv->res);
	drm_drv->res = drmModeGetResources(drm_drv->device->drmFD);
}

static void
drm_add_modes_from_connector(_EGLScreen *screen, drmModeConnectorPtr connector)
{
	struct drm_mode_modeinfo *m;
	int i;

	for (i = 0; i < connector->count_modes; i++) {
		m = &connector->modes[i];
		_eglAddNewMode(screen, m->hdisplay, m->vdisplay, m->vrefresh, m->name);
	}
}


static EGLBoolean
drm_initialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	_EGLDisplay *disp = _eglLookupDisplay(dpy);
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	struct drm_screen *screen = NULL;
	drmModeConnectorPtr connector = NULL;
	drmModeResPtr res = NULL;
	unsigned count_connectors = 0;
	int num_screens = 0;

	EGLint i;
	int fd;

	fd = drmOpen("i915", NULL);
	if (fd < 0) {
		return EGL_FALSE;
	}

	drm_drv->device = egl_drm_create_device(fd);
	if (!drm_drv->device) {
		drmClose(fd);
		return EGL_FALSE;
	}

	drm_update_res(drm_drv);
	res = drm_drv->res;
	if (res)
		count_connectors = res->count_connectors;

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
		_eglInitScreen(&screen->base);
		_eglAddScreen(disp, &screen->base);
		drm_add_modes_from_connector(&screen->base, connector);
		drm_drv->screens[num_screens++] = screen;
	}
	drm_drv->count_screens = num_screens;

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
}

static void
drm_takedown_shown_screen(_EGLDriver *drv, struct drm_screen *screen)
{
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	unsigned int i;

	intel_bind_frontbuffer(screen->surf->drawable, NULL);
	screen->surf = NULL;

	for (i = 0; i < drm_drv->res->count_crtcs; i++) {
		drmModeSetCrtc(
			drm_drv->device->drmFD,
			drm_drv->res->crtcs[i],
			0, // FD
			0, 0,
			NULL, 0, // List of output ids
			NULL);
	}

	drmModeRmFB(drm_drv->device->drmFD, screen->fbID);
	drmModeFreeFB(screen->fb);
	screen->fb = NULL;

	drmBOUnreference(drm_drv->device->drmFD, &screen->buffer);

	screen->shown = 0;
}

static EGLBoolean
drm_terminate(_EGLDriver *drv, EGLDisplay dpy)
{
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	struct drm_screen *screen;
	int i = 0;

	intel_destroy_device(drm_drv->device);
	drmFreeVersion(drm_drv->device->version);

	for (i = 0; i < drm_drv->count_screens; i++) {
		screen = drm_drv->screens[i];

		if (screen->shown)
			drm_takedown_shown_screen(drv, screen);

		drmModeFreeConnector(screen->connector);
		_eglDestroyScreen(&screen->base);
		drm_drv->screens[i] = NULL;
	}

	drmClose(drm_drv->device->drmFD);

	free(drm_drv->device);

	_eglCleanupDisplay(_eglLookupDisplay(dpy));
	free(drm_drv);

	return EGL_TRUE;
}


static struct drm_context *
lookup_drm_context(EGLContext context)
{
	_EGLContext *c = _eglLookupContext(context);
	return (struct drm_context *) c;
}


static struct drm_surface *
lookup_drm_surface(EGLSurface surface)
{
	_EGLSurface *s = _eglLookupSurface(surface);
	return (struct drm_surface *) s;
}

static struct drm_screen *
lookup_drm_screen(EGLDisplay dpy, EGLScreenMESA screen)
{
	_EGLScreen *s = _eglLookupScreen(dpy, screen);
	return (struct drm_screen *) s;
}

static __GLcontextModes*
visual_from_config(_EGLConfig *conf)
{
	__GLcontextModes *visual;
	(void)conf;

	visual = _egl_context_modes_create(1, sizeof(*visual));
	visual->redBits = 8;
	visual->greenBits = 8;
	visual->blueBits = 8;
	visual->alphaBits = 8;

	visual->rgbBits = 32;
	visual->doubleBufferMode = 1;

	visual->depthBits = 24;
	visual->haveDepthBuffer = visual->depthBits > 0;
	visual->stencilBits = 8;
	visual->haveStencilBuffer = visual->stencilBits > 0;

	return visual;
}



static EGLContext
drm_create_context(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	struct drm_context *c;
	struct drm_egl_context *share = NULL;
	_EGLConfig *conf;
	int i;
	int ret;
	__GLcontextModes *visual;
	struct egl_drm_context *context;

	conf = _eglLookupConfig(drv, dpy, config);
	if (!conf) {
		_eglError(EGL_BAD_CONFIG, "eglCreateContext");
		return EGL_NO_CONTEXT;
	}

	for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
		switch (attrib_list[i]) {
			/* no attribs defined for now */
			default:
				_eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
				return EGL_NO_CONTEXT;
		}
	}

	c = (struct drm_context *) calloc(1, sizeof(struct drm_context));
	if (!c)
		return EGL_NO_CONTEXT;

	_eglInitContext(drv, dpy, &c->base, config, attrib_list);

	context = malloc(sizeof(*context));
	memset(context, 0, sizeof(*context));

	if (!context)
		goto err_c;

	context->device = drm_drv->device;
	visual = visual_from_config(conf);

	ret = intel_create_context(context, visual, share);
	free(visual);

	if (!ret)
		goto err_gl;

	c->context = context;

	/* generate handle and insert into hash table */
	_eglSaveContext(&c->base);
	assert(_eglGetContextHandle(&c->base));

	return _eglGetContextHandle(&c->base);
err_gl:
	free(context);
err_c:
	free(c);
	return EGL_NO_CONTEXT;
}

static EGLBoolean
drm_destroy_context(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
	struct drm_context *fc = lookup_drm_context(context);
	_eglRemoveContext(&fc->base);
	if (fc->base.IsBound) {
		fc->base.DeletePending = EGL_TRUE;
	} else {
		intel_destroy_context(fc->context);
		free(fc->context);
		free(fc);
	}
	return EGL_TRUE;
}


static EGLSurface
drm_create_window_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}


static EGLSurface
drm_create_pixmap_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}


static EGLSurface
drm_create_pbuffer_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	int i;
	int ret;
	int width = -1;
	int height = -1;
	struct drm_surface *surf = NULL;
	struct egl_drm_drawable *drawable = NULL;
	__GLcontextModes *visual;
	_EGLConfig *conf;

	conf = _eglLookupConfig(drv, dpy, config);
	if (!conf) {
		_eglError(EGL_BAD_CONFIG, "eglCreatePbufferSurface");
		return EGL_NO_CONTEXT;
	}

	for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
		switch (attrib_list[i]) {
			case EGL_WIDTH:
				width = attrib_list[++i];
				break;
			case EGL_HEIGHT:
				height = attrib_list[++i];
				break;
			default:
				_eglError(EGL_BAD_ATTRIBUTE, "eglCreatePbufferSurface");
				return EGL_NO_SURFACE;
		}
	}

	if (width < 1 || height < 1) {
		_eglError(EGL_BAD_ATTRIBUTE, "eglCreatePbufferSurface");
		return EGL_NO_SURFACE;
	}

	surf = (struct drm_surface *) calloc(1, sizeof(struct drm_surface));
	if (!surf)
		goto err;

	if (!_eglInitSurface(drv, dpy, &surf->base, EGL_PBUFFER_BIT, config, attrib_list))
		goto err_surf;

	drawable = malloc(sizeof(*drawable));
	memset(drawable, 0, sizeof(*drawable));

	drawable->w = width;
	drawable->h = height;

	visual = visual_from_config(conf);

	drawable->device = drm_drv->device;
	ret = intel_create_drawable(drawable, visual);
	free(visual);

	if (!ret)
		goto err_draw;

	surf->drawable = drawable;

	_eglSaveSurface(&surf->base);
	return surf->base.Handle;

err_draw:
	free(drawable);
err_surf:
	free(surf);
err:
	return EGL_NO_SURFACE;
}

static EGLSurface
drm_create_screen_surface_mesa(_EGLDriver *drv, EGLDisplay dpy, EGLConfig cfg,
                               const EGLint *attrib_list)
{
	EGLSurface surf = drm_create_pbuffer_surface(drv, dpy, cfg, attrib_list);

	return surf;
}

static struct drm_mode_modeinfo *
drm_find_mode(drmModeConnectorPtr connector, _EGLMode *mode)
{
	int i;
	struct drm_mode_modeinfo *m;

	for (i = 0; i < connector->count_modes; i++) {
		m = &connector->modes[i];
		if (m->hdisplay == mode->Width && m->vdisplay == mode->Height && m->vrefresh == mode->RefreshRate)
			break;
		m = &connector->modes[0]; /* if we can't find one, return first */
	}

	return m;
}
static void
draw(size_t x, size_t y, size_t w, size_t h, size_t pitch, size_t v, unsigned int *ptr)
{
    int i, j;

    for (i = x; i < x + w; i++)
        for(j = y; j < y + h; j++)
            ptr[(i * pitch / 4) + j] = v;

}

static void
prettyColors(int fd, unsigned int handle, size_t pitch)
{
	drmBO bo;
	unsigned int *ptr;
	void *p;
	int i;

	drmBOReference(fd, handle, &bo);
	drmBOMap(fd, &bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0, &p);
	ptr = (unsigned int*)p;

	for (i = 0; i < (bo.size / 4); i++)
		ptr[i] = 0xFFFFFFFF;

	for (i = 0; i < 4; i++)
		draw(i * 40, i * 40, 40, 40, pitch, 0, ptr);


	draw(200, 100, 40, 40, pitch, 0xff00ff, ptr);
	draw(100, 200, 40, 40, pitch, 0xff00ff, ptr);

	drmBOUnmap(fd, &bo);
}

static EGLBoolean
drm_show_screen_surface_mesa(_EGLDriver *drv, EGLDisplay dpy,
                         EGLScreenMESA screen,
                         EGLSurface surface, EGLModeMESA m)
{
	struct drm_driver *drm_drv = (struct drm_driver *)drv;
	struct drm_surface *surf = lookup_drm_surface(surface);
	struct drm_screen *scrn = lookup_drm_screen(dpy, screen);
	_EGLMode *mode = _eglLookupMode(dpy, m);
	size_t pitch = mode->Width * 4;
	size_t size = mode->Height * pitch;
	int ret;
	unsigned int i,j,k;

	if (scrn->shown)
		drm_takedown_shown_screen(drv, scrn);

	ret = drmBOCreate(drm_drv->device->drmFD, size, 0, 0,
		DRM_BO_FLAG_READ |
		DRM_BO_FLAG_WRITE |
		DRM_BO_FLAG_MEM_TT |
		DRM_BO_FLAG_MEM_VRAM |
		DRM_BO_FLAG_NO_EVICT,
		DRM_BO_HINT_DONT_FENCE, &scrn->buffer);

	if (ret)
		return EGL_FALSE;

	prettyColors(drm_drv->device->drmFD, scrn->buffer.handle, pitch);

	ret = drmModeAddFB(drm_drv->device->drmFD, mode->Width, mode->Height,
			32, 32, pitch,
			scrn->buffer.handle,
			&scrn->fbID);

	if (ret)
		goto err_bo;

	scrn->fb = drmModeGetFB(drm_drv->device->drmFD, scrn->fbID);
	if (!scrn->fb)
		goto err_bo;

	for (j = 0; j < drm_drv->res->count_connectors; j++) {
		drmModeConnector *con = drmModeGetConnector(drm_drv->device->drmFD, drm_drv->res->connectors[j]);
		scrn->mode = drm_find_mode(con, mode);
		if (!scrn->mode)
			goto err_fb;

		for (k = 0; k < con->count_encoders; k++) {
			drmModeEncoder *enc = drmModeGetEncoder(drm_drv->device->drmFD, con->encoders[k]);
			for (i = 0; i < drm_drv->res->count_crtcs; i++) {
				if (enc->possible_crtcs & (1<<i)) {
					ret = drmModeSetCrtc(
						drm_drv->device->drmFD,
						drm_drv->res->crtcs[i],
						scrn->fbID,
						0, 0,
						&drm_drv->res->connectors[j], 1,
						scrn->mode);	
					/* skip the other crtcs now */
					i = drm_drv->res->count_crtcs;
				}
			}
		}
	}

	scrn->front.handle = scrn->buffer.handle;
	scrn->front.pitch = pitch;
	scrn->front.width = mode->Width;
	scrn->front.height = mode->Height;

	scrn->surf = surf;
	intel_bind_frontbuffer(surf->drawable, &scrn->front);

	scrn->shown = 1;

	return EGL_TRUE;

err_fb:
	/* TODO remove fb */

err_bo:
	drmBOUnreference(drm_drv->device->drmFD, &scrn->buffer);
	return EGL_FALSE;
}

static EGLBoolean
drm_destroy_surface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
	struct drm_surface *fs = lookup_drm_surface(surface);
	_eglRemoveSurface(&fs->base);
	if (fs->base.IsBound) {
		fs->base.DeletePending = EGL_TRUE;
	} else {
		intel_bind_frontbuffer(fs->drawable, NULL);
		intel_destroy_drawable(fs->drawable);
		free(fs->drawable);
		free(fs);
	}
	return EGL_TRUE;
}


static EGLBoolean
drm_make_current(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext context)
{
	struct drm_surface *readSurf = lookup_drm_surface(read);
	struct drm_surface *drawSurf = lookup_drm_surface(draw);
	struct drm_context *ctx = lookup_drm_context(context);
	EGLBoolean b;

	b = _eglMakeCurrent(drv, dpy, draw, read, context);
	if (!b)
		return EGL_FALSE;

	if (ctx) {
		if (!drawSurf || !readSurf)
			return EGL_FALSE;

		intel_make_current(ctx->context, drawSurf->drawable, readSurf->drawable);
	} else {
		intel_make_current(NULL, NULL, NULL);
	}

	return EGL_TRUE;
}

static EGLBoolean
drm_swap_buffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
	struct drm_surface *surf = lookup_drm_surface(draw);
	if (!surf)
		return EGL_FALSE;

	/* error checking */
	if (!_eglSwapBuffers(drv, dpy, draw))
		return EGL_FALSE;

	intel_swap_buffers(surf->drawable);
	return EGL_TRUE;
}


/**
 * The bootstrap function.  Return a new drm_driver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy, const char *args)
{
	struct drm_driver *drm;

	drm = (struct drm_driver *) calloc(1, sizeof(struct drm_driver));
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
	drm->base.Name = "DRM/Gallium";

	/* enable supported extensions */
	drm->base.Extensions.MESA_screen_surface = EGL_TRUE;
	drm->base.Extensions.MESA_copy_context = EGL_TRUE;

	return &drm->base;
}
