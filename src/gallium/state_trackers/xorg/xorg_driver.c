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
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "mipointer.h"
#include "micmap.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#ifndef XSERVER_LIBPCIACCESS
#error "libpciaccess needed"
#endif

#include <pciaccess.h>

#include "pipe/p_context.h"
#include "xorg_tracker.h"
#include "xorg_winsys.h"

static void AdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool CloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool EnterVT(int scrnIndex, int flags);
static Bool SaveHWState(ScrnInfoPtr pScrn);
static Bool RestoreHWState(ScrnInfoPtr pScrn);


static ModeStatus ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			    int flags);
static void FreeScreen(int scrnIndex, int flags);
static void LeaveVT(int scrnIndex, int flags);
static Bool SwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
static Bool ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc,
		       char **argv);
static Bool PreInit(ScrnInfoPtr pScrn, int flags);

typedef enum
{
    OPTION_SW_CURSOR,
} modesettingOpts;

static const OptionInfoRec Options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

/*
 * Exported Xorg driver functions to winsys
 */

const OptionInfoRec *
xorg_tracker_available_options(int chipid, int busid)
{
    return Options;
}

void
xorg_tracker_set_functions(ScrnInfoPtr scrn)
{
    scrn->PreInit = PreInit;
    scrn->ScreenInit = ScreenInit;
    scrn->SwitchMode = SwitchMode;
    scrn->AdjustFrame = AdjustFrame;
    scrn->EnterVT = EnterVT;
    scrn->LeaveVT = LeaveVT;
    scrn->FreeScreen = FreeScreen;
    scrn->ValidMode = ValidMode;
}

/*
 * Static Xorg funtctions
 */

static Bool
GetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(modesettingRec), 1);

    return TRUE;
}

static void
FreeRec(ScrnInfoPtr pScrn)
{
    if (!pScrn)
	return;

    if (!pScrn->driverPrivate)
	return;

    xfree(pScrn->driverPrivate);

    pScrn->driverPrivate = NULL;
}

static void
ProbeDDC(ScrnInfoPtr pScrn, int index)
{
    ConfiguredMonitor = NULL;
}

static Bool
CreateFrontBuffer(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    unsigned handle, stride;

    ms->noEvict = TRUE;
    xorg_exa_set_displayed_usage(rootPixmap);
    pScreen->ModifyPixmapHeader(rootPixmap,
				pScrn->virtualX, pScrn->virtualY,
				pScrn->depth, pScrn->bitsPerPixel,
				pScrn->displayWidth * pScrn->bitsPerPixel / 8,
				NULL);
    ms->noEvict = FALSE;

    handle = xorg_exa_get_pixmap_handle(rootPixmap, &stride);

    drmModeAddFB(ms->fd,
		 pScrn->virtualX,
		 pScrn->virtualY,
		 pScrn->depth,
		 pScrn->bitsPerPixel,
		 stride,
		 handle,
		 &ms->fb_id);

    pScrn->frameX0 = 0;
    pScrn->frameY0 = 0;
    AdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    return TRUE;
}

static Bool
crtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    /*
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    Bool fbAccessDisabled;
    CARD8 *fbstart;
     */

    if (width == pScrn->virtualX && height == pScrn->virtualY)
	return TRUE;

    ErrorF("RESIZING TO %dx%d\n", width, height);

    pScrn->virtualX = width;
    pScrn->virtualY = height;

    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;

    drmModeRmFB(ms->fd, ms->fb_id);

    /* now create new frontbuffer */
    return CreateFrontBuffer(pScrn);
}

static const xf86CrtcConfigFuncsRec crtc_config_funcs = {
    crtc_resize
};

static Bool
PreInit(ScrnInfoPtr pScrn, int flags)
{
    xf86CrtcConfigPtr xf86_config;
    modesettingPtr ms;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    EntPtr msEnt = NULL;
    char *BusID;
    int max_width, max_height;

    if (pScrn->numEntities != 1)
	return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    if (flags & PROBE_DETECT) {
	ProbeDDC(pScrn, pEnt->index);
	return TRUE;
    }

    /* Allocate driverPrivate */
    if (!GetRec(pScrn))
	return FALSE;

    ms = modesettingPTR(pScrn);
    ms->SaveGeneration = -1;
    ms->pEnt = pEnt;

    pScrn->displayWidth = 640;	       /* default it */

    if (ms->pEnt->location.type != BUS_PCI)
	return FALSE;

    ms->PciInfo = xf86GetPciInfoForEntity(ms->pEnt->index);

    /* Allocate an entity private if necessary */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
	FatalError("Entity");
#if 0
	msEnt = xf86GetEntityPrivate(pScrn->entityList[0],
				     modesettingEntityIndex)->ptr;
	ms->entityPrivate = msEnt;
#else
	(void)msEnt;
#endif
    } else
	ms->entityPrivate = NULL;

    if (xf86IsEntityShared(pScrn->entityList[0])) {
	if (xf86IsPrimInitDone(pScrn->entityList[0])) {
	    /* do something */
	} else {
	    xf86SetPrimInitDone(pScrn->entityList[0]);
	}
    }

    BusID = xalloc(64);
    sprintf(BusID, "PCI:%d:%d:%d",
	    ((ms->PciInfo->domain << 8) | ms->PciInfo->bus),
	    ms->PciInfo->dev, ms->PciInfo->func
	);

    ms->api = drm_api_create();
    ms->fd = drmOpen(NULL, BusID);

    if (ms->fd < 0)
	return FALSE;

    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

    if (!xf86SetDepthBpp
	(pScrn, 0, 0, 0,
	 PreferConvert24to32 | SupportConvert24to32 | Support32bppFb))
	return FALSE;

    switch (pScrn->depth) {
    case 15:
    case 16:
    case 24:
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by the driver\n",
		   pScrn->depth);
	return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
	return FALSE;
    if (!xf86SetDefaultVisual(pScrn, -1))
	return FALSE;

    /* Process the options */
    xf86CollectOptions(pScrn, NULL);
    if (!(ms->Options = xalloc(sizeof(Options))))
	return FALSE;
    memcpy(ms->Options, Options, sizeof(Options));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ms->Options);

    /* Allocate an xf86CrtcConfig */
    xf86CrtcConfigInit(pScrn, &crtc_config_funcs);
    xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

    max_width = 8192;
    max_height = 8192;
    xf86CrtcSetSizeRange(pScrn, 320, 200, max_width, max_height);

    if (xf86ReturnOptValBool(ms->Options, OPTION_SW_CURSOR, FALSE)) {
	ms->SWCursor = TRUE;
    }

    SaveHWState(pScrn);

    crtc_init(pScrn);
    output_init(pScrn);

    if (!xf86InitialConfiguration(pScrn, TRUE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes.\n");
	RestoreHWState(pScrn);
	return FALSE;
    }

    RestoreHWState(pScrn);

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma() here.
     */
    {
	Gamma zeros = { 0.0, 0.0, 0.0 };

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    if (pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
	return FALSE;
    }

    pScrn->currentMode = pScrn->modes;

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load the required sub modules */
    if (!xf86LoadSubModule(pScrn, "fb")) {
	return FALSE;
    }

    xf86LoadSubModule(pScrn, "exa");

#ifdef DRI2
    xf86LoadSubModule(pScrn, "dri2");
#endif

    return TRUE;
}

static Bool
SaveHWState(ScrnInfoPtr pScrn)
{
    /*xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);*/

    return TRUE;
}

static Bool
RestoreHWState(ScrnInfoPtr pScrn)
{
    /*xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);*/

    return TRUE;
}

static void xorgBlockHandler(int i, pointer blockData, pointer pTimeout,
                             pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[i];
    modesettingPtr ms = modesettingPTR(xf86Screens[pScreen->myNum]);

    pScreen->BlockHandler = ms->blockHandler;
    pScreen->BlockHandler(i, blockData, pTimeout, pReadmask);
    pScreen->BlockHandler = xorgBlockHandler;

    ms->ctx->flush(ms->ctx, PIPE_FLUSH_RENDER_CACHE, NULL);

#ifdef DRM_MODE_FEATURE_DIRTYFB
    {
	RegionPtr dirty = DamageRegion(ms->damage);
	unsigned num_cliprects = REGION_NUM_RECTS(dirty);

	if (num_cliprects) {
	    drmModeClip *clip = alloca(num_cliprects * sizeof(drmModeClip));
	    BoxPtr rect = REGION_RECTS(dirty);
	    int i;

	    for (i = 0; i < num_cliprects; i++, rect++) {
		clip[i].x = rect->x1;
		clip[i].y = rect->y1;
		clip[i].width = rect->x2 - rect->x1;
		clip[i].height = rect->y2 - rect->y1;
	    }

	    /* TODO query connector property to see if this is needed */
	    drmModeDirtyFB(ms->fd, ms->fb_id, clip, num_cliprects);

	    DamageEmpty(ms->damage);
	}
    }
#endif
}

static Bool
CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;
    unsigned handle, stride;

    ms->noEvict = TRUE;

    pScreen->CreateScreenResources = ms->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = CreateScreenResources;

    rootPixmap = pScreen->GetScreenPixmap(pScreen);

    xorg_exa_set_displayed_usage(rootPixmap);
    xorg_exa_set_shared_usage(rootPixmap);
    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, NULL))
	FatalError("Couldn't adjust screen pixmap\n");

    ms->noEvict = FALSE;

    handle = xorg_exa_get_pixmap_handle(rootPixmap, &stride);

    drmModeAddFB(ms->fd,
		 pScrn->virtualX,
		 pScrn->virtualY,
		 pScrn->depth,
		 pScrn->bitsPerPixel,
		 stride,
		 handle,
                 &ms->fb_id);

    AdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

#ifdef DRM_MODE_FEATURE_DIRTYFB
    ms->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
                              pScreen, rootPixmap);

    if (ms->damage) {
       DamageRegister(&rootPixmap->drawable, ms->damage);

       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Damage tracking initialized\n");
    } else {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                  "Failed to create screen damage record\n");
       return FALSE;
    }
#endif

    return ret;
}

static Bool
ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    VisualPtr visual;

    /* deal with server regeneration */
    if (ms->fd < 0) {
	char *BusID;

	BusID = xalloc(64);
	sprintf(BusID, "PCI:%d:%d:%d",
		((ms->PciInfo->domain << 8) | ms->PciInfo->bus),
		ms->PciInfo->dev, ms->PciInfo->func
	    );

	ms->fd = drmOpen(NULL, BusID);

	if (ms->fd < 0)
	    return FALSE;
    }

    if (!ms->screen) {
	ms->screen = ms->api->create_screen(ms->api, ms->fd, NULL);

	if (!ms->screen) {
	    FatalError("Could not init pipe_screen\n");
	    return FALSE;
	}
    }

    pScrn->pScreen = pScreen;

    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;

    miClearVisualTypes();

    if (!miSetVisualTypes(pScrn->depth,
			  miGetDefaultVisualMask(pScrn->depth),
			  pScrn->rgbBits, pScrn->defaultVisual))
	return FALSE;

    if (!miSetPixmapDepths())
	return FALSE;

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    if (!fbScreenInit(pScreen, NULL,
		      pScrn->virtualX, pScrn->virtualY,
		      pScrn->xDpi, pScrn->yDpi,
		      pScrn->displayWidth, pScrn->bitsPerPixel))
	return FALSE;

    if (pScrn->bitsPerPixel > 8) {
	/* Fixup RGB ordering */
	visual = pScreen->visuals + pScreen->numVisuals;
	while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    fbPictureInit(pScreen, NULL, 0);

    ms->blockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = xorgBlockHandler;
    ms->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = CreateScreenResources;

    xf86SetBlackWhitePixels(pScreen);

    ms->exa = xorg_exa_init(pScrn);

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Need to extend HWcursor support to handle mask interleave */
    if (!ms->SWCursor)
	xf86_cursors_init(pScreen, 64, 64,
			  HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
			  HARDWARE_CURSOR_ARGB);

    /* Must force it before EnterVT, so we are in control of VT and
     * later memory should be bound when allocating, e.g rotate_mem */
    pScrn->vtSema = TRUE;

    pScreen->SaveScreen = xf86SaveScreen;
    ms->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CloseScreen;

    if (!xf86CrtcScreenInit(pScreen))
	return FALSE;

    if (!miCreateDefColormap(pScreen))
	return FALSE;

    xf86DPMSInit(pScreen, xf86DPMSSet, 0);

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

#if 1
#ifdef DRI2
    driScreenInit(pScreen);
#endif
#endif

    return EnterVT(scrnIndex, 1);
}

static void
AdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86OutputPtr output = config->output[config->compat_output];
    xf86CrtcPtr crtc = output->crtc;

    if (crtc && crtc->enabled) {
	crtc->funcs->mode_set(crtc, pScrn->currentMode, pScrn->currentMode, x,
			      y);
	crtc->x = output->initial_x + x;
	crtc->y = output->initial_y + y;
    }
}

static void
FreeScreen(int scrnIndex, int flags)
{
    FreeRec(xf86Screens[scrnIndex]);
}

static void
LeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int o;

    for (o = 0; o < config->num_crtc; o++) {
	xf86CrtcPtr crtc = config->crtc[o];

	crtc_cursor_destroy(crtc);

	if (crtc->rotatedPixmap || crtc->rotatedData) {
	    crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
					crtc->rotatedData);
	    crtc->rotatedPixmap = NULL;
	    crtc->rotatedData = NULL;
	}
    }

    drmModeRmFB(ms->fd, ms->fb_id);

    RestoreHWState(pScrn);

    if (drmDropMaster(ms->fd))
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "drmDropMaster failed: %s\n", strerror(errno));

    pScrn->vtSema = FALSE;
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
EnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (drmSetMaster(ms->fd)) {
	if (errno == EINVAL) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "drmSetMaster failed: 2.6.29 or newer kernel required for "
		       "multi-server DRI\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "drmSetMaster failed: %s\n", strerror(errno));
	}
    }

    /*
     * Only save state once per server generation since that's what most
     * drivers do.  Could change this to save state at each VT enter.
     */
    if (ms->SaveGeneration != serverGeneration) {
	ms->SaveGeneration = serverGeneration;
	SaveHWState(pScrn);
    }

    if (!flags)			       /* signals startup as we'll do this in CreateScreenResources */
	CreateFrontBuffer(pScrn);

    if (!xf86SetDesiredModes(pScrn))
	return FALSE;

    return TRUE;
}

static Bool
SwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static Bool
CloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (pScrn->vtSema) {
	LeaveVT(scrnIndex, 0);
    }
#ifdef DRI2
    driCloseScreen(pScreen);
#endif

    pScreen->BlockHandler = ms->blockHandler;
    pScreen->CreateScreenResources = ms->createScreenResources;

#ifdef DRM_MODE_FEATURE_DIRTYFB
    if (ms->damage) {
	DamageUnregister(&pScreen->GetScreenPixmap(pScreen)->drawable, ms->damage);
	DamageDestroy(ms->damage);
	ms->damage = NULL;
    }
#endif

    if (ms->exa)
	xorg_exa_close(pScrn);

	ms->api->destroy(ms->api);
	ms->api = NULL;
    drmClose(ms->fd);
    ms->fd = -1;

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = ms->CloseScreen;
    return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static ModeStatus
ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}

/* vim: set sw=4 ts=8 sts=4: */
