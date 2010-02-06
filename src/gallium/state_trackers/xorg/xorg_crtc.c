/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 *
 */

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "xorg-server.h"
#include <xf86.h>
#include <xf86i2c.h>
#include <xf86Crtc.h>
#include <cursorstr.h>
#include "xorg_tracker.h"
#include "xf86Modes.h"

#ifdef HAVE_XEXTPROTO_71
#include <X11/extensions/dpmsconst.h>
#else
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#endif

#include "util/u_inlines.h"
#include "util/u_rect.h"

#ifdef HAVE_LIBKMS
#include "libkms.h"
#endif

struct crtc_private
{
    drmModeCrtcPtr drm_crtc;

    /* hwcursor */
    struct pipe_texture *cursor_tex;
    struct kms_bo *cursor_bo;

    unsigned cursor_handle;
};

static void
crtc_dpms(xf86CrtcPtr crtc, int mode)
{
    /* ScrnInfoPtr pScrn = crtc->scrn; */

    switch (mode) {
    case DPMSModeOn:
    case DPMSModeStandby:
    case DPMSModeSuspend:
	break;
    case DPMSModeOff:
	break;
    }
}

static Bool
crtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
		    Rotation rotation, int x, int y)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    xf86OutputPtr output = NULL;
    drmModeConnectorPtr drm_connector;
    struct crtc_private *crtcp = crtc->driver_private;
    drmModeCrtcPtr drm_crtc = crtcp->drm_crtc;
    drmModeModeInfo drm_mode;
    int i, ret;

    for (i = 0; i < config->num_output; output = NULL, i++) {
	output = config->output[i];

	if (output->crtc == crtc)
	    break;
    }

    if (!output)
	return FALSE;

    drm_connector = output->driver_private;

    drm_mode.clock = mode->Clock;
    drm_mode.hdisplay = mode->HDisplay;
    drm_mode.hsync_start = mode->HSyncStart;
    drm_mode.hsync_end = mode->HSyncEnd;
    drm_mode.htotal = mode->HTotal;
    drm_mode.vdisplay = mode->VDisplay;
    drm_mode.vsync_start = mode->VSyncStart;
    drm_mode.vsync_end = mode->VSyncEnd;
    drm_mode.vtotal = mode->VTotal;
    drm_mode.flags = mode->Flags;
    drm_mode.hskew = mode->HSkew;
    drm_mode.vscan = mode->VScan;
    drm_mode.vrefresh = mode->VRefresh;
    if (!mode->name)
	xf86SetModeDefaultName(mode);
    strncpy(drm_mode.name, mode->name, DRM_DISPLAY_MODE_LEN - 1);
    drm_mode.name[DRM_DISPLAY_MODE_LEN - 1] = '\0';

    ret = drmModeSetCrtc(ms->fd, drm_crtc->crtc_id, ms->fb_id, x, y,
			 &drm_connector->connector_id, 1, &drm_mode);

    if (ret)
	return FALSE;

    crtc->x = x;
    crtc->y = y;
    crtc->mode = *mode;
    crtc->rotation = rotation;

    return TRUE;
}

static void
crtc_gamma_set(xf86CrtcPtr crtc, CARD16 * red, CARD16 * green, CARD16 * blue,
	       int size)
{
    /* XXX: hockup */
}

static void *
crtc_shadow_allocate(xf86CrtcPtr crtc, int width, int height)
{
    /* ScrnInfoPtr pScrn = crtc->scrn; */

    return NULL;
}

static PixmapPtr
crtc_shadow_create(xf86CrtcPtr crtc, void *data, int width, int height)
{
    /* ScrnInfoPtr pScrn = crtc->scrn; */

    return NULL;
}

static void
crtc_shadow_destroy(xf86CrtcPtr crtc, PixmapPtr rotate_pixmap, void *data)
{
    /* ScrnInfoPtr pScrn = crtc->scrn; */
}

/*
 * Cursor functions
 */

static void
crtc_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg)
{
    /* XXX: See if this one is needed, as we only support ARGB cursors */
}

static void
crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    drmModeMoveCursor(ms->fd, crtcp->drm_crtc->crtc_id, x, y);
}

static void
crtc_load_cursor_argb_ga3d(xf86CrtcPtr crtc, CARD32 * image)
{
    unsigned char *ptr;
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;
    struct pipe_transfer *transfer;

    if (!crtcp->cursor_tex) {
	struct pipe_texture templat;
	unsigned pitch;

	memset(&templat, 0, sizeof(templat));
	templat.tex_usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
	templat.tex_usage |= PIPE_TEXTURE_USAGE_PRIMARY;
	templat.target = PIPE_TEXTURE_2D;
	templat.last_level = 0;
	templat.depth0 = 1;
	templat.format = PIPE_FORMAT_B8G8R8A8_UNORM;
	templat.width0 = 64;
	templat.height0 = 64;

	crtcp->cursor_tex = ms->screen->texture_create(ms->screen,
						       &templat);
	ms->api->local_handle_from_texture(ms->api,
					   ms->screen,
					   crtcp->cursor_tex,
					   &pitch,
					   &crtcp->cursor_handle);
    }

    transfer = ms->screen->get_tex_transfer(ms->screen, crtcp->cursor_tex,
					    0, 0, 0,
					    PIPE_TRANSFER_WRITE,
					    0, 0, 64, 64);
    ptr = ms->screen->transfer_map(ms->screen, transfer);
    util_copy_rect(ptr, crtcp->cursor_tex->format,
		   transfer->stride, 0, 0,
		   64, 64, (void*)image, 64 * 4, 0, 0);
    ms->screen->transfer_unmap(ms->screen, transfer);
    ms->screen->tex_transfer_destroy(transfer);
}

#if HAVE_LIBKMS
static void
crtc_load_cursor_argb_kms(xf86CrtcPtr crtc, CARD32 * image)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;
    unsigned char *ptr;

    if (!crtcp->cursor_bo) {
	unsigned attr[8];

	attr[0] = KMS_BO_TYPE;
#ifdef KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8
	attr[1] = KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8;
#else
	attr[1] = KMS_BO_TYPE_CURSOR;
#endif
	attr[2] = KMS_WIDTH;
	attr[3] = 64;
	attr[4] = KMS_HEIGHT;
	attr[5] = 64;
	attr[6] = 0;

        if (kms_bo_create(ms->kms, attr, &crtcp->cursor_bo))
	   return;

	if (kms_bo_get_prop(crtcp->cursor_bo, KMS_HANDLE,
			    &crtcp->cursor_handle))
	    goto err_bo_destroy;
    }

    kms_bo_map(crtcp->cursor_bo, (void**)&ptr);
    memcpy(ptr, image, 64*64*4);
    kms_bo_unmap(crtcp->cursor_bo);

    return;

err_bo_destroy:
    kms_bo_destroy(&crtcp->cursor_bo);
}
#endif

static void
crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32 * image)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    modesettingPtr ms = modesettingPTR(crtc->scrn);

    /* Older X servers have cursor reference counting bugs leading to use of
     * freed memory and consequently random crashes. Should be fixed as of
     * xserver 1.8, but this workaround shouldn't hurt anyway.
     */
    if (config->cursor)
       config->cursor->refcnt++;

    if (ms->cursor)
       FreeCursor(ms->cursor, None);

    ms->cursor = config->cursor;

    if (ms->screen)
	crtc_load_cursor_argb_ga3d(crtc, image);
#ifdef HAVE_LIBKMS
    else if (ms->kms)
	crtc_load_cursor_argb_kms(crtc, image);
#endif
}

static void
crtc_show_cursor(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    if (crtcp->cursor_tex || crtcp->cursor_bo)
	drmModeSetCursor(ms->fd, crtcp->drm_crtc->crtc_id,
			 crtcp->cursor_handle, 64, 64);
}

static void
crtc_hide_cursor(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    drmModeSetCursor(ms->fd, crtcp->drm_crtc->crtc_id, 0, 0, 0);
}

/**
 * Called at vt leave
 */
void
xorg_crtc_cursor_destroy(xf86CrtcPtr crtc)
{
    struct crtc_private *crtcp = crtc->driver_private;

    if (crtcp->cursor_tex)
	pipe_texture_reference(&crtcp->cursor_tex, NULL);
#ifdef HAVE_LIBKMS
    if (crtcp->cursor_bo)
	kms_bo_destroy(&crtcp->cursor_bo);
#endif
}

/*
 * Misc functions
 */

static void
crtc_destroy(xf86CrtcPtr crtc)
{
    struct crtc_private *crtcp = crtc->driver_private;

    xorg_crtc_cursor_destroy(crtc);

    drmModeFreeCrtc(crtcp->drm_crtc);

    xfree(crtcp);
    crtc->driver_private = NULL;
}

static const xf86CrtcFuncsRec crtc_funcs = {
    .dpms = crtc_dpms,
    .set_mode_major = crtc_set_mode_major,

    .set_cursor_colors = crtc_set_cursor_colors,
    .set_cursor_position = crtc_set_cursor_position,
    .show_cursor = crtc_show_cursor,
    .hide_cursor = crtc_hide_cursor,
    .load_cursor_argb = crtc_load_cursor_argb,

    .shadow_create = crtc_shadow_create,
    .shadow_allocate = crtc_shadow_allocate,
    .shadow_destroy = crtc_shadow_destroy,

    .gamma_set = crtc_gamma_set,
    .destroy = crtc_destroy,
};

void
xorg_crtc_init(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86CrtcPtr crtc;
    drmModeResPtr res;
    drmModeCrtcPtr drm_crtc = NULL;
    struct crtc_private *crtcp;
    int c;

    res = drmModeGetResources(ms->fd);
    if (res == 0) {
	ErrorF("Failed drmModeGetResources %d\n", errno);
	return;
    }

    for (c = 0; c < res->count_crtcs; c++) {
	drm_crtc = drmModeGetCrtc(ms->fd, res->crtcs[c]);

	if (!drm_crtc)
	    continue;

	crtc = xf86CrtcCreate(pScrn, &crtc_funcs);
	if (crtc == NULL)
	    goto out;

	crtcp = xcalloc(1, sizeof(struct crtc_private));
	if (!crtcp) {
	    xf86CrtcDestroy(crtc);
	    goto out;
	}

	crtcp->drm_crtc = drm_crtc;

	crtc->driver_private = crtcp;
    }

  out:
    drmModeFreeResources(res);
}

/* vim: set sw=4 ts=8 sts=4: */
