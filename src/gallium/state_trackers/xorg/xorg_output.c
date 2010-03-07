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

#ifdef HAVE_XEXTPROTO_71
#include <X11/extensions/dpmsconst.h>
#else
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#endif

#include "xorg_tracker.h"

static char *output_enum_list[] = {
    "Unknown",
    "VGA",
    "DVI",
    "DVI",
    "DVI",
    "Composite",
    "SVIDEO",
    "LVDS",
    "CTV",
    "DIN",
    "DP",
    "HDMI",
    "HDMI",
};

static void
output_create_resources(xf86OutputPtr output)
{
#ifdef RANDR_12_INTERFACE
#endif /* RANDR_12_INTERFACE */
}

static void
output_dpms(xf86OutputPtr output, int mode)
{
}

static xf86OutputStatus
output_detect(xf86OutputPtr output)
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
output_get_modes(xf86OutputPtr output)
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
	    mode->type = 0;
	    if (drm_mode->type & DRM_MODE_TYPE_PREFERRED)
		mode->type |= M_T_PREFERRED;
	    if (drm_mode->type & DRM_MODE_TYPE_DRIVER)
		mode->type |= M_T_DRIVER;
	    xf86SetModeDefaultName(mode);
	    modes = xf86ModesAdd(modes, mode);
	    xf86PrintModeline(0, mode);
	}
    }

    return modes;
}

static int
output_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    return MODE_OK;
}

#ifdef RANDR_12_INTERFACE
static Bool
output_set_property(xf86OutputPtr output, Atom property, RRPropertyValuePtr value)
{
    return TRUE;
}
#endif /* RANDR_12_INTERFACE */

#ifdef RANDR_13_INTERFACE
static Bool
output_get_property(xf86OutputPtr output, Atom property)
{
    return TRUE;
}
#endif /* RANDR_13_INTERFACE */

static void
output_destroy(xf86OutputPtr output)
{
    drmModeFreeConnector(output->driver_private);
}

static const xf86OutputFuncsRec output_funcs = {
    .create_resources = output_create_resources,
#ifdef RANDR_12_INTERFACE
    .set_property = output_set_property,
#endif
#ifdef RANDR_13_INTERFACE
    .get_property = output_get_property,
#endif
    .dpms = output_dpms,
    .detect = output_detect,

    .get_modes = output_get_modes,
    .mode_valid = output_mode_valid,
    .destroy = output_destroy,
};

void
xorg_output_init(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86OutputPtr output;
    drmModeResPtr res;
    drmModeConnectorPtr drm_connector = NULL;
    drmModeEncoderPtr drm_encoder = NULL;
    char name[32];
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

	snprintf(name, 32, "%s%d",
		 output_enum_list[drm_connector->connector_type],
		 drm_connector->connector_type_id);


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
