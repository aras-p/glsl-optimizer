
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include "egllog.h"

#include "pipe/p_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

#include "state_tracker/drm_api.h"

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
drm_create_framebuffer(const __GLcontextModes *visual,
                       unsigned width,
                       unsigned height,
                       void *priv)
{
	enum pipe_format colorFormat, depthFormat, stencilFormat;

	if (visual->redBits == 5)
		colorFormat = PIPE_FORMAT_R5G6B5_UNORM;
	else
		colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

	if (visual->depthBits == 16)
		depthFormat = PIPE_FORMAT_Z16_UNORM;
	else if (visual->depthBits == 24)
		depthFormat = PIPE_FORMAT_S8Z24_UNORM;
	else
		depthFormat = PIPE_FORMAT_NONE;

	if (visual->stencilBits == 8)
		stencilFormat = PIPE_FORMAT_S8Z24_UNORM;
	else
		stencilFormat = PIPE_FORMAT_NONE;

	return st_create_framebuffer(visual,
	                             colorFormat,
	                             depthFormat,
	                             stencilFormat,
	                             width,
	                             height,
	                             priv);
}

static void
drm_create_texture(_EGLDriver *drv,
                   struct drm_screen *scrn,
                   unsigned w, unsigned h)
{
	struct drm_device *dev = (struct drm_device *)drv;
	struct pipe_screen *screen = dev->screen;
	struct pipe_surface *surface;
	struct pipe_texture *texture;
	struct pipe_texture templat;
	struct pipe_buffer *buf;
	unsigned stride = 1024;
	unsigned pitch = 0;
	unsigned size = 0;

	/* ugly */
	if (stride < w)
		stride = 2048;

	pitch = stride * 4;
	size = h * 2 * pitch;

	buf = pipe_buffer_create(screen,
	                         0, /* alignment */
	                         PIPE_BUFFER_USAGE_GPU_READ_WRITE |
	                         PIPE_BUFFER_USAGE_CPU_READ_WRITE,
	                         size);

	if (!buf)
		goto err_buf;

	memset(&templat, 0, sizeof(templat));
	templat.tex_usage |= PIPE_TEXTURE_USAGE_DISPLAY_TARGET;
	templat.tex_usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
	templat.target = PIPE_TEXTURE_2D;
	templat.last_level = 0;
	templat.depth[0] = 1;
	templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	templat.width[0] = w;
	templat.height[0] = h;
	pf_get_block(templat.format, &templat.block);

	texture = screen->texture_blanket(dev->screen,
	                                  &templat,
	                                  &pitch,
	                                  buf);
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
	scrn->buffer = buf;
	scrn->front.width = w;
	scrn->front.height = h;
	scrn->front.pitch = pitch;
	drm_api_hooks.handle_from_buffer(screen, scrn->buffer, &scrn->front.handle);
	if (0)
		goto err_handle;

	return;

err_handle:
	pipe_surface_reference(&surface, NULL);
err_surf:
	pipe_texture_reference(&texture, NULL);
err_tex:
	pipe_buffer_reference(&buf, NULL);
err_buf:
	return;
}

/*
 * Exported functions
 */

void
drm_takedown_shown_screen(_EGLDriver *drv, struct drm_screen *screen)
{
	struct drm_device *dev = (struct drm_device *)drv;

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
	pipe_buffer_reference(&screen->buffer, NULL);

	screen->shown = 0;
}

EGLSurface
drm_create_window_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}


EGLSurface
drm_create_pixmap_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}


EGLSurface
drm_create_pbuffer_surface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                           const EGLint *attrib_list)
{
	int i;
	int width = -1;
	int height = -1;
	struct drm_surface *surf = NULL;
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

	surf->w = width;
	surf->h = height;

	visual = drm_visual_from_config(conf);
	surf->stfb = drm_create_framebuffer(visual,
	                                    width,
	                                    height,
	                                    (void*)surf);
	drm_visual_modes_destroy(visual);

	_eglSaveSurface(&surf->base);
	return surf->base.Handle;

err_surf:
	free(surf);
err:
	return EGL_NO_SURFACE;
}

EGLSurface
drm_create_screen_surface_mesa(_EGLDriver *drv, EGLDisplay dpy, EGLConfig cfg,
                               const EGLint *attrib_list)
{
	EGLSurface surf = drm_create_pbuffer_surface(drv, dpy, cfg, attrib_list);

	return surf;
}

EGLBoolean
drm_show_screen_surface_mesa(_EGLDriver *drv, EGLDisplay dpy,
                             EGLScreenMESA screen,
                             EGLSurface surface, EGLModeMESA m)
{
	struct drm_device *dev = (struct drm_device *)drv;
	struct drm_surface *surf = lookup_drm_surface(surface);
	struct drm_screen *scrn = lookup_drm_screen(dpy, screen);
	_EGLMode *mode = _eglLookupMode(dpy, m);
	int ret;
	unsigned int i, k;

	if (scrn->shown)
		drm_takedown_shown_screen(drv, scrn);


	drm_create_texture(drv, scrn, mode->Width, mode->Height);
	if (!scrn->buffer)
		return EGL_FALSE;

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
	pipe_buffer_reference(&scrn->buffer, NULL);

	return EGL_FALSE;
}

EGLBoolean
drm_destroy_surface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
	struct drm_surface *surf = lookup_drm_surface(surface);
	_eglRemoveSurface(&surf->base);
	if (surf->base.IsBound) {
		surf->base.DeletePending = EGL_TRUE;
	} else {
		if (surf->screen)
			drm_takedown_shown_screen(drv, surf->screen);
		st_unreference_framebuffer(surf->stfb);
		free(surf);
	}
	return EGL_TRUE;
}

EGLBoolean
drm_swap_buffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
	struct drm_surface *surf = lookup_drm_surface(draw);
	struct pipe_surface *back_surf;

	if (!surf)
		return EGL_FALSE;

	/* error checking */
	if (!_eglSwapBuffers(drv, dpy, draw))
		return EGL_FALSE;

	st_get_framebuffer_surface(surf->stfb, ST_SURFACE_BACK_LEFT, &back_surf);

	if (back_surf) {

		st_notify_swapbuffers(surf->stfb);

		if (surf->screen) {
			surf->user->pipe->flush(surf->user->pipe, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_TEXTURE_CACHE, NULL);
			surf->user->pipe->surface_copy(surf->user->pipe,
				surf->screen->surface,
				0, 0,
				back_surf,
				0, 0,
				surf->w, surf->h);
			surf->user->pipe->flush(surf->user->pipe, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_TEXTURE_CACHE, NULL);
			/* TODO stuff here */
		}
	}

	return EGL_TRUE;
}
