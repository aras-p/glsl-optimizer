#include "driclient.h"
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <X11/Xlibint.h>

int driCreateScreen(Display *display, int screen, dri_screen_t **dri_screen, dri_framebuffer_t *dri_framebuf)
{
	int		evbase, errbase;
	char		*driver_name;
	int		newly_opened;
	drm_magic_t	magic;
	drmVersionPtr	drm_version;
	drm_handle_t	sarea_handle;
	char		*bus_id;
	dri_screen_t	*dri_scrn;

	assert(display);
	assert(dri_screen);

	if (!XF86DRIQueryExtension(display, &evbase, &errbase))
		return 1;

	dri_scrn = calloc(1, sizeof(dri_screen_t));

	if (!dri_scrn)
		return 1;

	if (!XF86DRIQueryVersion(display, &dri_scrn->dri.major, &dri_scrn->dri.minor, &dri_scrn->dri.patch))
		goto free_screen;

	dri_scrn->display = display;
	dri_scrn->num = screen;
	dri_scrn->draw_lock_id = 1;

	if (!XF86DRIOpenConnection(display, screen, &sarea_handle, &bus_id))
		goto free_screen;

	dri_scrn->fd = -1;
	dri_scrn->fd = drmOpenOnce(NULL, bus_id, &newly_opened);
	XFree(bus_id);

	if (dri_scrn->fd < 0)
		goto close_connection;

	if (drmGetMagic(dri_scrn->fd, &magic))
		goto close_drm;

	drm_version = drmGetVersion(dri_scrn->fd);

	if (!drm_version)
		goto close_drm;

	dri_scrn->drm.major = drm_version->version_major;
	dri_scrn->drm.minor = drm_version->version_minor;
	dri_scrn->drm.patch = drm_version->version_patchlevel;
	drmFreeVersion(drm_version);

	if (!XF86DRIAuthConnection(display, screen, magic))
		goto close_drm;

	if (!XF86DRIGetClientDriverName
	(
		display,
		screen,
		&dri_scrn->ddx.major,
		&dri_scrn->ddx.minor,
		&dri_scrn->ddx.patch,
		&driver_name
	))
		goto close_drm;

	if (drmMap(dri_scrn->fd, sarea_handle, SAREA_MAX, (drmAddress)&dri_scrn->sarea))
		goto close_drm;

	dri_scrn->drawable_hash = drmHashCreate();

	if (!dri_scrn->drawable_hash)
		goto unmap_sarea;

	if (dri_framebuf)
	{
		if (!XF86DRIGetDeviceInfo
		(
			display,
			screen, &dri_framebuf->drm_handle,
			&dri_framebuf->base,
			&dri_framebuf->size,
			&dri_framebuf->stride,
			&dri_framebuf->private_size,
			&dri_framebuf->private
		))
			goto destroy_hash;
	}

	*dri_screen = dri_scrn;

	return 0;

destroy_hash:
	drmHashDestroy(dri_scrn->drawable_hash);
unmap_sarea:
	drmUnmap(dri_scrn->sarea, SAREA_MAX);
close_drm:
	drmCloseOnce(dri_scrn->fd);
close_connection:
	XF86DRICloseConnection(display, screen);
free_screen:
	free(dri_scrn);

	return 1;
}

int driDestroyScreen(dri_screen_t *dri_screen)
{
	Drawable	draw;
	dri_drawable_t	*dri_draw;

	assert(dri_screen);

	if (drmHashFirst(dri_screen->drawable_hash, &draw, (void**)&dri_draw))
	{
		dri_draw->refcount = 1;
		driDestroyDrawable(dri_draw);

		while (drmHashNext(dri_screen->drawable_hash, &draw, (void**)&dri_draw))
		{
			dri_draw->refcount = 1;
			driDestroyDrawable(dri_draw);
		}
	}

	drmHashDestroy(dri_screen->drawable_hash);
	drmUnmap(dri_screen->sarea, SAREA_MAX);
	drmCloseOnce(dri_screen->fd);
	XF86DRICloseConnection(dri_screen->display, dri_screen->num);
	free(dri_screen);

	return 0;
}

int driCreateDrawable(dri_screen_t *dri_screen, Drawable drawable, dri_drawable_t **dri_drawable)
{
	int		evbase, errbase;
	dri_drawable_t	*dri_draw;

	assert(dri_screen);
	assert(dri_drawable);

	if (!XF86DRIQueryExtension(dri_screen->display, &evbase, &errbase))
		return 1;

	if (!drmHashLookup(dri_screen->drawable_hash, drawable, (void**)dri_drawable))
	{
		/* Found */
		(*dri_drawable)->refcount++;
		return 0;
	}

	dri_draw = calloc(1, sizeof(dri_drawable_t));

	if (!dri_draw)
		return 1;

	if (!XF86DRICreateDrawable(dri_screen->display, 0, drawable, &dri_draw->drm_drawable))
	{
		free(dri_draw);
		return 1;
	}

	dri_draw->x_drawable = drawable;
	dri_draw->sarea_index = 0;
	dri_draw->sarea_stamp = NULL;
	dri_draw->last_sarea_stamp = 0;
	dri_draw->dri_screen = dri_screen;
	dri_draw->refcount = 1;

	if (drmHashInsert(dri_screen->drawable_hash, drawable, dri_draw))
	{
		XF86DRIDestroyDrawable(dri_screen->display, dri_screen->num, drawable);
		free(dri_draw);
		return 1;
	}

	if (!dri_draw->sarea_stamp || *dri_draw->sarea_stamp != dri_draw->last_sarea_stamp)
	{
		DRM_SPINLOCK(&dri_screen->sarea->drawable_lock, dri_screen->draw_lock_id);

		if (driUpdateDrawableInfo(dri_draw))
		{
			XF86DRIDestroyDrawable(dri_screen->display, dri_screen->num, drawable);
			free(dri_draw);
			DRM_SPINUNLOCK(&dri_screen->sarea->drawable_lock, dri_screen->draw_lock_id);
			return 1;
		}

		DRM_SPINUNLOCK(&dri_screen->sarea->drawable_lock, dri_screen->draw_lock_id);
	}

	*dri_drawable = dri_draw;

	return 0;
}

int driUpdateDrawableInfo(dri_drawable_t *dri_drawable)
{
	assert(dri_drawable);

	if (dri_drawable->cliprects)
	{
		XFree(dri_drawable->cliprects);
		dri_drawable->cliprects = NULL;
	}
	if (dri_drawable->back_cliprects)
	{
		XFree(dri_drawable->back_cliprects);
		dri_drawable->back_cliprects = NULL;
	}

	DRM_SPINUNLOCK(&dri_drawable->dri_screen->sarea->drawable_lock, dri_drawable->dri_screen->draw_lock_id);

	if (!XF86DRIGetDrawableInfo
	(
		dri_drawable->dri_screen->display,
		dri_drawable->dri_screen->num,
		dri_drawable->x_drawable,
		&dri_drawable->sarea_index,
		&dri_drawable->last_sarea_stamp,
		&dri_drawable->x,
		&dri_drawable->y,
		&dri_drawable->w,
		&dri_drawable->h,
		&dri_drawable->num_cliprects,
		&dri_drawable->cliprects,
		&dri_drawable->back_x,
		&dri_drawable->back_y,
		&dri_drawable->num_back_cliprects,
		&dri_drawable->back_cliprects
	))
	{
		dri_drawable->sarea_stamp = &dri_drawable->last_sarea_stamp;
		dri_drawable->num_cliprects = 0;
		dri_drawable->cliprects = NULL;
		dri_drawable->num_back_cliprects = 0;
		dri_drawable->back_cliprects = 0;

		return 1;
	}
	else
		dri_drawable->sarea_stamp = &dri_drawable->dri_screen->sarea->drawableTable[dri_drawable->sarea_index].stamp;

	DRM_SPINLOCK(&dri_drawable->dri_screen->sarea->drawable_lock, dri_drawable->dri_screen->draw_lock_id);

	return 0;
}

int driDestroyDrawable(dri_drawable_t *dri_drawable)
{
	assert(dri_drawable);

	if (--dri_drawable->refcount == 0)
	{
		if (dri_drawable->cliprects)
			XFree(dri_drawable->cliprects);
		if (dri_drawable->back_cliprects)
			XFree(dri_drawable->back_cliprects);
		drmHashDelete(dri_drawable->dri_screen->drawable_hash, dri_drawable->x_drawable);
		XF86DRIDestroyDrawable(dri_drawable->dri_screen->display, dri_drawable->dri_screen->num, dri_drawable->x_drawable);
		free(dri_drawable);
	}

	return 0;
}

int driCreateContext(dri_screen_t *dri_screen, Visual *visual, dri_context_t **dri_context)
{
	int		evbase, errbase;
	dri_context_t	*dri_ctx;

	assert(dri_screen);
	assert(visual);
	assert(dri_context);

	if (!XF86DRIQueryExtension(dri_screen->display, &evbase, &errbase))
		return 1;

	dri_ctx = calloc(1, sizeof(dri_context_t));

	if (!dri_ctx)
		return 1;

	if (!XF86DRICreateContext(dri_screen->display, dri_screen->num, visual, &dri_ctx->id, &dri_ctx->drm_context))
	{
		free(dri_ctx);
		return 1;
	}

	dri_ctx->dri_screen = dri_screen;
	*dri_context = dri_ctx;

	return 0;
}

int driDestroyContext(dri_context_t *dri_context)
{
	assert(dri_context);

	XF86DRIDestroyContext(dri_context->dri_screen->display, dri_context->dri_screen->num, dri_context->id);
	free(dri_context);

	return 0;
}

int dri2CreateScreen(Display *display, int screen, dri_screen_t **dri_screen)
{
	dri_screen_t	*dri_scrn;
	drm_magic_t	magic;
	char		*drvName;
	char		*devName;

	dri_scrn = calloc(1, sizeof(dri_screen_t));

	if (!dri_scrn)
		return 1;

	if (!DRI2Connect(display, XRootWindow(display, screen), &drvName, &devName))
		goto free_screen;

	dri_scrn->fd = open(devName, O_RDWR);
	Xfree(drvName);
	Xfree(devName);
	if (dri_scrn->fd < 0)
		goto free_screen;

	if (drmGetMagic(dri_scrn->fd, &magic))
		goto free_screen;

	if (!DRI2Authenticate(display, RootWindow(display, screen), magic))
		goto free_screen;

	dri_scrn->display = display;
	dri_scrn->num = screen;
	*dri_screen = dri_scrn;

	return 0;

free_screen:
	free(dri_scrn);

	return 1;
}

int dri2DestroyScreen(dri_screen_t *dri_screen)
{
	/* Not much to do here apparently... */
	assert(dri_screen);
	free(dri_screen);
	return 0;
}

int dri2CreateDrawable(dri_screen_t *dri_screen, XID drawable)
{
	assert(dri_screen);
	DRI2CreateDrawable(dri_screen->display, drawable);
	return 0;
}

int dri2DestroyDrawable(dri_screen_t *dri_screen, XID drawable)
{
	assert(dri_screen);
	DRI2DestroyDrawable(dri_screen->display, drawable);
	return 0;
}

int dri2CopyDrawable(dri_screen_t *dri_screen, XID drawable, int dest, int src)
{
	XserverRegion region;

	assert(dri_screen);
	assert(dest >= DRI_BUFFER_FRONT_LEFT && dest <= DRI_BUFFER_DEPTH_STENCIL);
	assert(src >= DRI_BUFFER_FRONT_LEFT && src <= DRI_BUFFER_DEPTH_STENCIL);

	region = XFixesCreateRegionFromWindow(dri_screen->display, drawable, WindowRegionBounding);
	DRI2CopyRegion(dri_screen->display, drawable, region, dest, src);
	XFixesDestroyRegion(dri_screen->display, region);

	return 0;
}
