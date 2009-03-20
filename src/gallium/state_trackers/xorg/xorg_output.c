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

#include "xorg-server.h"
#include <xf86.h>
#include <xf86i2c.h>
#include <xf86Crtc.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DPMS_SERVER
#include <X11/extensions/dpms.h>

#include "X11/Xatom.h"

#include "xorg_tracker.h"

static char *connector_enum_list[] = {
    "Unknown",
    "VGA",
    "DVI-I",
    "DVI-D",
    "DVI-A",
    "Composite",
    "SVIDEO",
    "LVDS",
    "Component",
    "9-pin DIN",
    "DisplayPort",
    "HDMI Type A",
    "HDMI Type B",
};

static void
dpms(xf86OutputPtr output, int mode)
{
}

static void
save(xf86OutputPtr output)
{
}

static void
restore(xf86OutputPtr output)
{
}

static int
mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    return MODE_OK;
}

static Bool
mode_fixup(xf86OutputPtr output, DisplayModePtr mode,
	   DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
prepare(xf86OutputPtr output)
{
    dpms(output, DPMSModeOff);
}

static void
mode_set(xf86OutputPtr output, DisplayModePtr mode,
	 DisplayModePtr adjusted_mode)
{
}

static void
commit(xf86OutputPtr output)
{
    dpms(output, DPMSModeOn);

    if (output->scrn->pScreen != NULL)
	xf86_reload_cursors(output->scrn->pScreen);
}

static xf86OutputStatus
detect(xf86OutputPtr output)
{
    drmModeConnectorPtr drm_connector = output->driver_private;

    switch (drm_connector->connection) {
    case DRM_MODE_CONNECTED:
	return XF86OutputStatusConnected;
    case DRM_MODE_DISCONNECTED:
	return XF86OutputStatusDisconnected;
    default:
	return XF86OutputStatusUnknown;
    }
}

static DisplayModePtr
get_modes(xf86OutputPtr output)
{
    drmModeConnectorPtr drm_connector = output->driver_private;
    drmModeModeInfoPtr drm_mode = NULL;
    DisplayModePtr modes = NULL, mode = NULL;
    int i;

    for (i = 0; i < drm_connector->count_modes; i++) {
	drm_mode = &drm_connector->modes[i];
	if (drm_mode) {
	    mode = xcalloc(1, sizeof(DisplayModeRec));
	    if (!mode)
		continue;
	    mode->type = 0;
	    mode->Clock = drm_mode->clock;
	    mode->HDisplay = drm_mode->hdisplay;
	    mode->HSyncStart = drm_mode->hsync_start;
	    mode->HSyncEnd = drm_mode->hsync_end;
	    mode->HTotal = drm_mode->htotal;
	    mode->VDisplay = drm_mode->vdisplay;
	    mode->VSyncStart = drm_mode->vsync_start;
	    mode->VSyncEnd = drm_mode->vsync_end;
	    mode->VTotal = drm_mode->vtotal;
	    mode->Flags = drm_mode->flags;
	    mode->HSkew = drm_mode->hskew;
	    mode->VScan = drm_mode->vscan;
	    mode->VRefresh = xf86ModeVRefresh(mode);
	    mode->Private = (void *)drm_mode;
	    xf86SetModeDefaultName(mode);
	    modes = xf86ModesAdd(modes, mode);
	    xf86PrintModeline(0, mode);
	}
    }

    return modes;
}

static void
destroy(xf86OutputPtr output)
{
    drmModeFreeConnector(output->driver_private);
}

static void
create_resources(xf86OutputPtr output)
{
#ifdef RANDR_12_INTERFACE
#endif /* RANDR_12_INTERFACE */
}

#ifdef RANDR_12_INTERFACE
static Bool
set_property(xf86OutputPtr output, Atom property, RRPropertyValuePtr value)
{
    return TRUE;
}
#endif /* RANDR_12_INTERFACE */

#ifdef RANDR_13_INTERFACE
static Bool
get_property(xf86OutputPtr output, Atom property)
{
    return TRUE;
}
#endif /* RANDR_13_INTERFACE */

#ifdef RANDR_GET_CRTC_INTERFACE
static xf86CrtcPtr
get_crtc(xf86OutputPtr output)
{
    return NULL;
}
#endif

static const xf86OutputFuncsRec output_funcs = {
    .create_resources = create_resources,
    .dpms = dpms,
    .save = save,
    .restore = restore,
    .mode_valid = mode_valid,
    .mode_fixup = mode_fixup,
    .prepare = prepare,
    .mode_set = mode_set,
    .commit = commit,
    .detect = detect,
    .get_modes = get_modes,
#ifdef RANDR_12_INTERFACE
    .set_property = set_property,
#endif
#ifdef RANDR_13_INTERFACE
    .get_property = get_property,
#endif
    .destroy = destroy,
#ifdef RANDR_GET_CRTC_INTERFACE
    .get_crtc = get_crtc,
#endif
};

void
output_init(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86OutputPtr output;
    drmModeResPtr res;
    drmModeConnectorPtr drm_connector = NULL;
    drmModeEncoderPtr drm_encoder = NULL;
    char *name;
    int c, v, p;

    res = drmModeGetResources(ms->fd);
    if (res == 0) {
	DRV_ERROR("Failed drmModeGetResources\n");
	return;
    }

    for (c = 0; c < res->count_connectors; c++) {
	drm_connector = drmModeGetConnector(ms->fd, res->connectors[c]);
	if (!drm_connector)
	    goto out;

#if 0
	for (p = 0; p < drm_connector->count_props; p++) {
	    drmModePropertyPtr prop;

	    prop = drmModeGetProperty(ms->fd, drm_connector->props[p]);

	    name = NULL;
	    if (prop) {
		ErrorF("VALUES %d\n", prop->count_values);

		for (v = 0; v < prop->count_values; v++)
		    ErrorF("%s %lld\n", prop->name, prop->values[v]);
	    }
	}
#else
	(void)p;
	(void)v;
#endif

	name = connector_enum_list[drm_connector->connector_type];

	output = xf86OutputCreate(pScrn, &output_funcs, name);
	if (!output)
	    continue;

	drm_encoder = drmModeGetEncoder(ms->fd, drm_connector->encoders[0]);
	if (drm_encoder) {
	    output->possible_crtcs = drm_encoder->possible_crtcs;
	    output->possible_clones = drm_encoder->possible_clones;
	} else {
	    output->possible_crtcs = 0;
	    output->possible_clones = 0;
	}
	output->driver_private = drm_connector;
	output->subpixel_order = SubPixelHorizontalRGB;
	output->interlaceAllowed = FALSE;
	output->doubleScanAllowed = FALSE;
    }

  out:
    drmModeFreeResources(res);
}

/* vim: set sw=4 ts=8 sts=4: */
