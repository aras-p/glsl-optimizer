
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include "egllog.h"


#include "pipe/p_context.h"
#include "pipe/p_screen.h"

#include "state_tracker/st_public.h"
#include "state_tracker/drm_api.h"

#include "GL/internal/glcore.h"

_EGLContext *
drm_create_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	struct drm_context *ctx;
	struct drm_context *share = NULL;
	struct st_context *st_share = NULL;
	int i;
	__GLcontextModes *visual;

	for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
		switch (attrib_list[i]) {
			/* no attribs defined for now */
			default:
				_eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
				return EGL_NO_CONTEXT;
		}
	}

	ctx = (struct drm_context *) calloc(1, sizeof(struct drm_context));
	if (!ctx)
		goto err_c;

	_eglInitContext(drv, &ctx->base, conf, attrib_list);

	ctx->pipe = dev->api->create_context(dev->api, dev->screen);
	if (!ctx->pipe)
		goto err_pipe;

	if (share)
		st_share = share->st;

	visual = drm_visual_from_config(conf);
	ctx->st = st_create_context(ctx->pipe, visual, st_share);
	drm_visual_modes_destroy(visual);

	if (!ctx->st)
		goto err_gl;

	return &ctx->base;

err_gl:
	ctx->pipe->destroy(ctx->pipe);
err_pipe:
	free(ctx);
err_c:
	return NULL;
}

EGLBoolean
drm_destroy_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *context)
{
	struct drm_context *c = lookup_drm_context(context);
	if (!_eglIsContextBound(&c->base)) {
		st_destroy_context(c->st);
		free(c);
	}
	return EGL_TRUE;
}

EGLBoolean
drm_make_current(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw, _EGLSurface *read, _EGLContext *context)
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

		st_make_current(ctx->st, drawSurf->stfb, readSurf->stfb);

		/* st_resize_framebuffer needs a bound context to work */
		st_resize_framebuffer(drawSurf->stfb, drawSurf->w, drawSurf->h);
		st_resize_framebuffer(readSurf->stfb, readSurf->w, readSurf->h);
	} else {
		st_make_current(NULL, NULL, NULL);
	}

	return EGL_TRUE;
}
