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
#include "xorg_tracker.h"
#include "xf86Modes.h"

#define DPMS_SERVER
#include <X11/extensions/dpms.h>

#include "pipe/p_inlines.h"

struct crtc_private
{
    drmModeCrtcPtr drm_crtc;

    /* hwcursor */
    struct pipe_buffer *cursor_buf;
    unsigned cursor_handle;
};

static void
crtc_dpms(xf86CrtcPtr crtc, int mode)
{
    //ScrnInfoPtr pScrn = crtc->scrn;

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
crtc_lock(xf86CrtcPtr crtc)
{
    return FALSE;
}

static void
crtc_unlock(xf86CrtcPtr crtc)
{
}

static void
crtc_prepare(xf86CrtcPtr crtc)
{
}

static void
crtc_commit(xf86CrtcPtr crtc)
{
}

static Bool
crtc_mode_fixup(xf86CrtcPtr crtc, DisplayModePtr mode,
		DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
crtc_mode_set(xf86CrtcPtr crtc, DisplayModePtr mode,
	      DisplayModePtr adjusted_mode, int x, int y)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    xf86OutputPtr output = config->output[config->compat_output];
    drmModeConnectorPtr drm_connector = output->driver_private;
    struct crtc_private *crtcp = crtc->driver_private;
    drmModeCrtcPtr drm_crtc = crtcp->drm_crtc;
    drmModeModeInfo drm_mode;

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
    strncpy(drm_mode.name, mode->name, DRM_DISPLAY_MODE_LEN);

    drmModeSetCrtc(ms->fd, drm_crtc->crtc_id, ms->fb_id, x, y,
		   &drm_connector->connector_id, 1, &drm_mode);
}

#if 0
static void
crtc_load_lut(xf86CrtcPtr crtc)
{
    //ScrnInfoPtr pScrn = crtc->scrn;
}
#endif

static void
crtc_gamma_set(xf86CrtcPtr crtc, CARD16 * red, CARD16 * green, CARD16 * blue,
	       int size)
{
}

static void *
crtc_shadow_allocate(xf86CrtcPtr crtc, int width, int height)
{
    //ScrnInfoPtr pScrn = crtc->scrn;

    return NULL;
}

static PixmapPtr
crtc_shadow_create(xf86CrtcPtr crtc, void *data, int width, int height)
{
    //ScrnInfoPtr pScrn = crtc->scrn;

    return NULL;
}

static void
crtc_shadow_destroy(xf86CrtcPtr crtc, PixmapPtr rotate_pixmap, void *data)
{
    //ScrnInfoPtr pScrn = crtc->scrn;
}

static void
crtc_destroy(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    if (crtcp->cursor_buf)
	pipe_buffer_reference(&crtcp->cursor_buf, NULL);

    drmModeFreeCrtc(crtcp->drm_crtc);
    xfree(crtcp);
}

static void
crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32 * image)
{
    unsigned char *ptr;
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    if (!crtcp->cursor_buf) {
	crtcp->cursor_buf = pipe_buffer_create(ms->screen,
					       0,
					       PIPE_BUFFER_USAGE_CPU_WRITE |
					       PIPE_BUFFER_USAGE_GPU_READ,
					       64*64*4);
	drm_api_hooks.handle_from_buffer(ms->screen,
					 crtcp->cursor_buf,
					 &crtcp->cursor_handle);
    }

    ptr = pipe_buffer_map(ms->screen, crtcp->cursor_buf, PIPE_BUFFER_USAGE_CPU_WRITE);

    if (ptr)
	memcpy(ptr, image, 64 * 64 * 4);

    pipe_buffer_unmap(ms->screen, crtcp->cursor_buf);
}

static void
crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    drmModeMoveCursor(ms->fd, crtcp->drm_crtc->crtc_id, x, y);
}

static void
crtc_show_cursor(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    if (crtcp->cursor_buf)
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

static const xf86CrtcFuncsRec crtc_funcs = {
    .dpms = crtc_dpms,
    .save = NULL,
    .restore = NULL,
    .lock = crtc_lock,
    .unlock = crtc_unlock,
    .mode_fixup = crtc_mode_fixup,
    .prepare = crtc_prepare,
    .mode_set = crtc_mode_set,
    .commit = crtc_commit,
    .gamma_set = crtc_gamma_set,
    .shadow_create = crtc_shadow_create,
    .shadow_allocate = crtc_shadow_allocate,
    .shadow_destroy = crtc_shadow_destroy,
    .set_cursor_position = crtc_set_cursor_position,
    .show_cursor = crtc_show_cursor,
    .hide_cursor = crtc_hide_cursor,
    .load_cursor_image = NULL,	       /* lets convert to argb only */
    .set_cursor_colors = NULL,	       /* using argb only */
    .load_cursor_argb = crtc_load_cursor_argb,
    .destroy = crtc_destroy,
};

void
cursor_destroy(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    struct crtc_private *crtcp = crtc->driver_private;

    if (crtcp->cursor_buf) {
	pipe_buffer_reference(&crtcp->cursor_buf, NULL);
    }
}

void
crtc_init(ScrnInfoPtr pScrn)
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
