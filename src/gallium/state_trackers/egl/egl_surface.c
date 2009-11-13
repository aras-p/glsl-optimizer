
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include "egllog.h"

#include "pipe/p_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

#include "state_tracker/drm_api.h"

#include "util/u_rect.h"

/*
 * Util functions
 */

static drmModeModeInfoPtr
drm_find_mode(drmModeConnectorPtr connector, _EGLMode *mode)
{
	int i;
	drmModeModeInfoPtr m = NULL;

	for (i = 0; i < connector->count_modes; i++) {
		m = &connector->modes[i];
		if (m->hdisplay == mode->Width && m->vdisplay == mode->Height && m->vrefresh == mode->RefreshRate)
			break;
		m = &connector->modes[0]; /* if we can't find one, return first */
	}

	return m;
}

static struct st_framebuffer *
drm_create_framebuffer(struct pipe_screen *screen,
                       const __GLcontextModes *visual,
                       unsigned width,
                       unsigned height,
                       void *priv)
{
	enum pipe_format color_format, depth_stencil_format;
	boolean d_depth_bits_last;
	boolean ds_depth_bits_last;

	d_depth_bits_last =
		screen->is_format_supported(screen, PIPE_FORMAT_X8Z24_UNORM,
		                            PIPE_TEXTURE_2D,
		                            PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
	ds_depth_bits_last =
		screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
		                            PIPE_TEXTURE_2D,
		                            PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);

	if (visual->redBits == 8) {
		if (visual->alphaBits == 8)
			color_format = PIPE_FORMAT_A8R8G8B8_UNORM;
		else
			color_format = PIPE_FORMAT_X8R8G8B8_UNORM;
	} else {
		color_format = PIPE_FORMAT_R5G6B5_UNORM;
	}

	switch(visual->depthBits) {
		default:
		case 0:
			depth_stencil_format = PIPE_FORMAT_NONE;
			break;
		case 16:
			depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
			break;
		case 24:
			if (visual->stencilBits == 0) {
				depth_stencil_format = (d_depth_bits_last) ?
					PIPE_FORMAT_X8Z24_UNORM:
					PIPE_FORMAT_Z24X8_UNORM;
			} else {
				depth_stencil_format = (ds_depth_bits_last) ?
					PIPE_FORMAT_S8Z24_UNORM:
					PIPE_FORMAT_Z24S8_UNORM;
			}
			break;
		case 32:
			depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
			break;
	}

	return st_create_framebuffer(visual,
	                             color_format,
	                             depth_stencil_format,
	                             depth_stencil_format,
	                             width,
	                             height,
	                             priv);
}

static void
drm_create_texture(_EGLDisplay *dpy,
                   struct drm_screen *scrn,
                   unsigned w, unsigned h)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	struct pipe_screen *screen = dev->screen;
	struct pipe_surface *surface;
	struct pipe_texture *texture;
	struct pipe_texture templat;
	struct pipe_buffer *buf = NULL;
	unsigned pitch = 0;

	memset(&templat, 0, sizeof(templat));
	templat.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
	templat.tex_usage |= PIPE_TEXTURE_USAGE_PRIMARY;
	templat.target = PIPE_TEXTURE_2D;
	templat.last_level = 0;
	templat.depth[0] = 1;
	templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	templat.width[0] = w;
	templat.height[0] = h;
	pf_get_block(templat.format, &templat.block);

	texture = screen->texture_create(dev->screen,
	                                 &templat);

	if (!texture)
		goto err_tex;

	surface = screen->get_tex_surface(screen,
	                                  texture,
	                                  0,
	                                  0,
	                                  0,
	                                  PIPE_BUFFER_USAGE_GPU_WRITE);

	if (!surface)
		goto err_surf;

	scrn->tex = texture;
	scrn->surface = surface;
	scrn->front.width = w;
	scrn->front.height = h;
	scrn->front.pitch = pitch;
	dev->api->local_handle_from_texture(dev->api, screen, texture,
	                                    &scrn->front.pitch, &scrn->front.handle);
	if (0)
		goto err_handle;

	return;

err_handle:
	pipe_surface_reference(&surface, NULL);
err_surf:
	pipe_texture_reference(&texture, NULL);
err_tex:
	pipe_buffer_reference(&buf, NULL);
	return;
}

/*
 * Exported functions
 */

void
drm_takedown_shown_screen(_EGLDisplay *dpy, struct drm_screen *screen)
{
	struct drm_device *dev = lookup_drm_device(dpy);

	screen->surf = NULL;

	drmModeSetCrtc(
		dev->drmFD,
		screen->crtcID,
		0, // FD
		0, 0,
		NULL, 0, // List of output ids
		NULL);

	drmModeRmFB(dev->drmFD, screen->fbID);
	drmModeFreeFB(screen->fb);
	screen->fb = NULL;

	pipe_surface_reference(&screen->surface, NULL);
	pipe_texture_reference(&screen->tex, NULL);

	screen->shown = 0;
}

/**
 * Called by libEGL's eglCreateWindowSurface().
 */
_EGLSurface *
drm_create_window_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativeWindowType window, const EGLint *attrib_list)
{
	return NULL;
}


/**
 * Called by libEGL's eglCreatePixmapSurface().
 */
_EGLSurface *
drm_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativePixmapType pixmap, const EGLint *attrib_list)
{
	return NULL;
}


/**
 * Called by libEGL's eglCreatePbufferSurface().
 */
_EGLSurface *
drm_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                           const EGLint *attrib_list)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	int i;
	int width = -1;
	int height = -1;
	struct drm_surface *surf = NULL;
	__GLcontextModes *visual;

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
		return NULL;
	}

	surf = (struct drm_surface *) calloc(1, sizeof(struct drm_surface));
	if (!surf)
		goto err;

	if (!_eglInitSurface(drv, &surf->base, EGL_PBUFFER_BIT, conf, attrib_list))
		goto err_surf;

	surf->w = width;
	surf->h = height;

	visual = drm_visual_from_config(conf);
	surf->stfb = drm_create_framebuffer(dev->screen, visual,
	                                    width, height,
	                                    (void*)surf);
	drm_visual_modes_destroy(visual);

	return &surf->base;

err_surf:
	free(surf);
err:
	return NULL;
}

/**
 * Called by libEGL's eglCreateScreenSurfaceMESA().
 */
_EGLSurface *
drm_create_screen_surface_mesa(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *cfg,
                               const EGLint *attrib_list)
{
	EGLSurface surf = drm_create_pbuffer_surface(drv, dpy, cfg, attrib_list);

	return surf;
}

/**
 * Called by libEGL's eglShowScreenSurfaceMESA().
 */
EGLBoolean
drm_show_screen_surface_mesa(_EGLDriver *drv, _EGLDisplay *dpy,
                             _EGLScreen *screen,
                             _EGLSurface *surface, _EGLMode *mode)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	struct drm_surface *surf = lookup_drm_surface(surface);
	struct drm_screen *scrn = lookup_drm_screen(screen);
	int ret;
	unsigned int i, k;

	if (scrn->shown)
		drm_takedown_shown_screen(dpy, scrn);


	drm_create_texture(dpy, scrn, mode->Width, mode->Height);
	if (!scrn->tex)
		goto err_tex;

	ret = drmModeAddFB(dev->drmFD,
	                   scrn->front.width, scrn->front.height,
	                   32, 32, scrn->front.pitch,
	                   scrn->front.handle,
	                   &scrn->fbID);

	if (ret)
		goto err_bo;

	scrn->fb = drmModeGetFB(dev->drmFD, scrn->fbID);
	if (!scrn->fb)
		goto err_bo;

	/* find a fitting crtc */
	{
		drmModeConnector *con = scrn->connector;

		scrn->mode = drm_find_mode(con, mode);
		if (!scrn->mode)
			goto err_fb;

		for (k = 0; k < con->count_encoders; k++) {
			drmModeEncoder *enc = drmModeGetEncoder(dev->drmFD, con->encoders[k]);
			for (i = 0; i < dev->res->count_crtcs; i++) {
				if (enc->possible_crtcs & (1<<i)) {
					/* save the ID */
					scrn->crtcID = dev->res->crtcs[i];

					/* skip the rest */
					i = dev->res->count_crtcs;
					k = dev->res->count_encoders;
				}
			}
			drmModeFreeEncoder(enc);
		}
	}

	ret = drmModeSetCrtc(dev->drmFD,
	                     scrn->crtcID,
	                     scrn->fbID,
	                     0, 0,
	                     &scrn->connectorID, 1,
	                     scrn->mode);

	if (ret)
		goto err_crtc;


	if (scrn->dpms)
		drmModeConnectorSetProperty(dev->drmFD,
		                            scrn->connectorID,
		                            scrn->dpms->prop_id,
		                            DRM_MODE_DPMS_ON);

	surf->screen = scrn;

	scrn->surf = surf;
	scrn->shown = 1;

	return EGL_TRUE;

err_crtc:
	scrn->crtcID = 0;

err_fb:
	drmModeRmFB(dev->drmFD, scrn->fbID);
	drmModeFreeFB(scrn->fb);
	scrn->fb = NULL;

err_bo:
	pipe_surface_reference(&scrn->surface, NULL);
	pipe_texture_reference(&scrn->tex, NULL);

err_tex:
	return EGL_FALSE;
}

/**
 * Called by libEGL's eglDestroySurface().
 */
EGLBoolean
drm_destroy_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface)
{
	struct drm_surface *surf = lookup_drm_surface(surface);
	if (!_eglIsSurfaceBound(&surf->base)) {
		if (surf->screen)
			drm_takedown_shown_screen(dpy, surf->screen);
		st_unreference_framebuffer(surf->stfb);
		free(surf);
	}
	return EGL_TRUE;
}

/**
 * Called by libEGL's eglSwapBuffers().
 */
EGLBoolean
drm_swap_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw)
{
	struct drm_device *dev = lookup_drm_device(dpy);
	struct drm_surface *surf = lookup_drm_surface(draw);
	struct pipe_surface *back_surf;

	if (!surf)
		return EGL_FALSE;

	st_get_framebuffer_surface(surf->stfb, ST_SURFACE_BACK_LEFT, &back_surf);

	if (back_surf) {
		struct drm_context *ctx = lookup_drm_context(draw->Binding);

		st_notify_swapbuffers(surf->stfb);

		if (ctx && surf->screen) {
            if (ctx->pipe->surface_copy) {
                ctx->pipe->surface_copy(ctx->pipe,
                    surf->screen->surface,
                    0, 0,
                    back_surf,
                    0, 0,
                    surf->w, surf->h);
            } else {
                util_surface_copy(ctx->pipe, FALSE,
                    surf->screen->surface,
                    0, 0,
                    back_surf,
                    0, 0,
                    surf->w, surf->h);
            }
			ctx->pipe->flush(ctx->pipe, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_TEXTURE_CACHE, NULL);

#ifdef DRM_MODE_FEATURE_DIRTYFB
			/* TODO query connector property to see if this is needed */
			drmModeDirtyFB(dev->drmFD, surf->screen->fbID, NULL, 0);
#else
			(void)dev;
#endif

			/* TODO more stuff here */
		}
	}

	return EGL_TRUE;
}
