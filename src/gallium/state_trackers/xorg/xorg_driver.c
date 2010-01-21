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
#ifndef XSERVER_LIBPCIACCESS
#error "libpciaccess needed"
#endif

#include <pciaccess.h>

#include "pipe/p_context.h"
#include "xorg_tracker.h"
#include "xorg_winsys.h"

#ifdef HAVE_LIBKMS
#include "libkms.h"
#endif

/*
 * Functions and symbols exported to Xorg via pointers.
 */

static Bool drv_pre_init(ScrnInfoPtr pScrn, int flags);
static Bool drv_screen_init(int scrnIndex, ScreenPtr pScreen, int argc,
			    char **argv);
static Bool drv_switch_mode(int scrnIndex, DisplayModePtr mode, int flags);
static void drv_adjust_frame(int scrnIndex, int x, int y, int flags);
static Bool drv_enter_vt(int scrnIndex, int flags);
static void drv_leave_vt(int scrnIndex, int flags);
static void drv_free_screen(int scrnIndex, int flags);
static ModeStatus drv_valid_mode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			         int flags);

typedef enum
{
    OPTION_SW_CURSOR,
    OPTION_2D_ACCEL,
} drv_option_enums;

static const OptionInfoRec drv_options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_2D_ACCEL, "2DAccel", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};


/*
 * Exported Xorg driver functions to winsys
 */

const OptionInfoRec *
xorg_tracker_available_options(int chipid, int busid)
{
    return drv_options;
}

void
xorg_tracker_set_functions(ScrnInfoPtr scrn)
{
    scrn->PreInit = drv_pre_init;
    scrn->ScreenInit = drv_screen_init;
    scrn->SwitchMode = drv_switch_mode;
    scrn->AdjustFrame = drv_adjust_frame;
    scrn->EnterVT = drv_enter_vt;
    scrn->LeaveVT = drv_leave_vt;
    scrn->FreeScreen = drv_free_screen;
    scrn->ValidMode = drv_valid_mode;
}


/*
 * Internal function definitions
 */

static Bool drv_init_front_buffer_functions(ScrnInfoPtr pScrn);
static Bool drv_close_screen(int scrnIndex, ScreenPtr pScreen);
static Bool drv_save_hw_state(ScrnInfoPtr pScrn);
static Bool drv_restore_hw_state(ScrnInfoPtr pScrn);


/*
 * Internal functions
 */

static Bool
drv_get_rec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(modesettingRec), 1);

    return TRUE;
}

static void
drv_free_rec(ScrnInfoPtr pScrn)
{
    if (!pScrn)
	return;

    if (!pScrn->driverPrivate)
	return;

    xfree(pScrn->driverPrivate);

    pScrn->driverPrivate = NULL;
}

static void
drv_probe_ddc(ScrnInfoPtr pScrn, int index)
{
    ConfiguredMonitor = NULL;
}

static Bool
drv_crtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    ScreenPtr pScreen = pScrn->pScreen;

    if (width == pScrn->virtualX && height == pScrn->virtualY)
	return TRUE;

    pScrn->virtualX = width;
    pScrn->virtualY = height;

    /*
     * Remove the old framebuffer & texture.
     */
    drmModeRmFB(ms->fd, ms->fb_id);
    if (!ms->destroy_front_buffer(pScrn))
	FatalError("failed to destroy front buffer\n");

    rootPixmap = pScreen->GetScreenPixmap(pScreen);
    if (!pScreen->ModifyPixmapHeader(rootPixmap, width, height, -1, -1, -1, NULL))
	return FALSE;

    pScrn->displayWidth = rootPixmap->devKind / (rootPixmap->drawable.bitsPerPixel / 8);

    /* now create new frontbuffer */
    return ms->create_front_buffer(pScrn) && ms->bind_front_buffer(pScrn);
}

static const xf86CrtcConfigFuncsRec crtc_config_funcs = {
    .resize = drv_crtc_resize
};

static Bool
drv_init_drm(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    /* deal with server regeneration */
    if (ms->fd < 0) {
	char *BusID;

	BusID = xalloc(64);
	sprintf(BusID, "PCI:%d:%d:%d",
		((ms->PciInfo->domain << 8) | ms->PciInfo->bus),
		ms->PciInfo->dev, ms->PciInfo->func
	    );


	ms->api = drm_api_create();
	ms->fd = drmOpen(ms->api ? ms->api->driver_name : NULL, BusID);
	xfree(BusID);

	if (ms->fd >= 0)
	    return TRUE;

	if (ms->api && ms->api->destroy)
	    ms->api->destroy(ms->api);

	ms->api = NULL;

	return FALSE;
    }

    return TRUE;
}

static Bool
drv_close_drm(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    if (ms->api && ms->api->destroy)
	ms->api->destroy(ms->api);
    ms->api = NULL;

    drmClose(ms->fd);
    ms->fd = -1;

    return TRUE;
}

static Bool
drv_init_resource_management(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    /*
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    Bool fbAccessDisabled;
    CARD8 *fbstart;
     */

    if (ms->screen || ms->kms)
	return TRUE;

    if (ms->api) {
	ms->screen = ms->api->create_screen(ms->api, ms->fd, NULL);

	if (ms->screen)
	    return TRUE;

	if (ms->api->destroy)
	    ms->api->destroy(ms->api);

	ms->api = NULL;
    }

#ifdef HAVE_LIBKMS
    if (!kms_create(ms->fd, &ms->kms))
	return TRUE;
#endif

    return FALSE;
}

static Bool
drv_close_resource_management(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    int i;

    if (ms->screen) {
	assert(ms->ctx == NULL);

	for (i = 0; i < XORG_NR_FENCES; i++) {
	    if (ms->fence[i]) {
		ms->screen->fence_finish(ms->screen, ms->fence[i], 0);
		ms->screen->fence_reference(ms->screen, &ms->fence[i], NULL);
	    }
	}
	ms->screen->destroy(ms->screen);
    }
    ms->screen = NULL;

#ifdef HAVE_LIBKMS
    if (ms->kms)
	kms_destroy(&ms->kms);
#endif

    return TRUE;
}

static Bool
drv_pre_init(ScrnInfoPtr pScrn, int flags)
{
    xf86CrtcConfigPtr xf86_config;
    modesettingPtr ms;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    EntPtr msEnt = NULL;
    int max_width, max_height;

    if (pScrn->numEntities != 1)
	return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    if (flags & PROBE_DETECT) {
	drv_probe_ddc(pScrn, pEnt->index);
	return TRUE;
    }

    /* Allocate driverPrivate */
    if (!drv_get_rec(pScrn))
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

    ms->fd = -1;
    ms->api = NULL;
    if (!drv_init_drm(pScrn))
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
    if (!(ms->Options = xalloc(sizeof(drv_options))))
	return FALSE;
    memcpy(ms->Options, drv_options, sizeof(drv_options));
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

    drv_save_hw_state(pScrn);

    xorg_crtc_init(pScrn);
    xorg_output_init(pScrn);

    if (!xf86InitialConfiguration(pScrn, TRUE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes.\n");
	drv_restore_hw_state(pScrn);
	return FALSE;
    }

    drv_restore_hw_state(pScrn);

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
    if (!xf86LoadSubModule(pScrn, "fb"))
	return FALSE;

    /* XXX: these aren't needed when we are using libkms */
    if (!xf86LoadSubModule(pScrn, "exa"))
	return FALSE;

#ifdef DRI2
    if (!xf86LoadSubModule(pScrn, "dri2"))
	return FALSE;
#endif

    return TRUE;
}

static Bool
drv_save_hw_state(ScrnInfoPtr pScrn)
{
    /*xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);*/

    return TRUE;
}

static Bool
drv_restore_hw_state(ScrnInfoPtr pScrn)
{
    /*xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);*/

    return TRUE;
}

static void drv_block_handler(int i, pointer blockData, pointer pTimeout,
                              pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[i];
    modesettingPtr ms = modesettingPTR(xf86Screens[pScreen->myNum]);

    pScreen->BlockHandler = ms->blockHandler;
    pScreen->BlockHandler(i, blockData, pTimeout, pReadmask);
    pScreen->BlockHandler = drv_block_handler;

    if (ms->ctx) {
       int j;

       ms->ctx->flush(ms->ctx, PIPE_FLUSH_RENDER_CACHE, &ms->fence[XORG_NR_FENCES-1]);
       
       if (ms->fence[0])
          ms->ctx->screen->fence_finish(ms->ctx->screen, ms->fence[0], 0);
  
       /* The amount of rendering generated by a block handler can be
        * quite small.  Let us get a fair way ahead of hardware before
        * throttling.
        */
       for (j = 0; j < XORG_NR_FENCES - 1; j++)
          ms->screen->fence_reference(ms->screen,
                                      &ms->fence[j],
                                      ms->fence[j+1]);

       ms->screen->fence_reference(ms->screen,
                                   &ms->fence[XORG_NR_FENCES-1],
                                   NULL);
    }
        

#ifdef DRM_MODE_FEATURE_DIRTYFB
    {
	RegionPtr dirty = DamageRegion(ms->damage);
	unsigned num_cliprects = REGION_NUM_RECTS(dirty);

	if (num_cliprects) {
	    drmModeClip *clip = alloca(num_cliprects * sizeof(drmModeClip));
	    BoxPtr rect = REGION_RECTS(dirty);
	    int i, ret;

	    /* XXX no need for copy? */
	    for (i = 0; i < num_cliprects; i++, rect++) {
		clip[i].x1 = rect->x1;
		clip[i].y1 = rect->y1;
		clip[i].x2 = rect->x2;
		clip[i].y2 = rect->y2;
	    }

	    /* TODO query connector property to see if this is needed */
	    ret = drmModeDirtyFB(ms->fd, ms->fb_id, clip, num_cliprects);
	    if (ret) {
		debug_printf("%s: failed to send dirty (%i, %s)\n",
			     __func__, ret, strerror(-ret));
	    }

	    DamageEmpty(ms->damage);
	}
    }
#endif
}

static Bool
drv_create_screen_resources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;

    ms->noEvict = TRUE;

    pScreen->CreateScreenResources = ms->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = drv_create_screen_resources;

    ms->bind_front_buffer(pScrn);

    ms->noEvict = FALSE;

    drv_adjust_frame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

#ifdef DRM_MODE_FEATURE_DIRTYFB
    rootPixmap = pScreen->GetScreenPixmap(pScreen);
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
#else
    (void)rootPixmap;
#endif

    return ret;
}

static Bool
drv_screen_init(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    VisualPtr visual;

    if (!drv_init_drm(pScrn)) {
	FatalError("Could not init DRM");
	return FALSE;
    }

    if (!drv_init_resource_management(pScrn)) {
	FatalError("Could not init resource management (!pipe_screen && !libkms)");
	return FALSE;
    }

    if (!drv_init_front_buffer_functions(pScrn)) {
	FatalError("Could not init front buffer manager");
	return FALSE;
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
    pScreen->BlockHandler = drv_block_handler;
    ms->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = drv_create_screen_resources;

    xf86SetBlackWhitePixels(pScreen);

    if (ms->screen) {
	ms->exa = xorg_exa_init(pScrn, xf86ReturnOptValBool(ms->Options,
							    OPTION_2D_ACCEL, TRUE));
	ms->debug_fallback = debug_get_bool_option("XORG_DEBUG_FALLBACK", TRUE);

	xorg_xv_init(pScreen);
#ifdef DRI2
	xorg_dri2_init(pScreen);
#endif
    }

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
    pScreen->CloseScreen = drv_close_screen;

    if (!xf86CrtcScreenInit(pScreen))
	return FALSE;

    if (!miCreateDefColormap(pScreen))
	return FALSE;

    xf86DPMSInit(pScreen, xf86DPMSSet, 0);

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    if (ms->winsys_screen_init)
	ms->winsys_screen_init(pScrn);

    return drv_enter_vt(scrnIndex, 1);
}

static void
drv_adjust_frame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86OutputPtr output = config->output[config->compat_output];
    xf86CrtcPtr crtc = output->crtc;

    if (crtc && crtc->enabled) {
	crtc->funcs->set_mode_major(crtc, pScrn->currentMode,
				    RR_Rotate_0, x, y);
	crtc->x = output->initial_x + x;
	crtc->y = output->initial_y + y;
    }
}

static void
drv_free_screen(int scrnIndex, int flags)
{
    drv_free_rec(xf86Screens[scrnIndex]);
}

static void
drv_leave_vt(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int o;

    if (ms->winsys_leave_vt)
	ms->winsys_leave_vt(pScrn);

    for (o = 0; o < config->num_crtc; o++) {
	xf86CrtcPtr crtc = config->crtc[o];

	xorg_crtc_cursor_destroy(crtc);

	if (crtc->rotatedPixmap || crtc->rotatedData) {
	    crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
					crtc->rotatedData);
	    crtc->rotatedPixmap = NULL;
	    crtc->rotatedData = NULL;
	}
    }

    drmModeRmFB(ms->fd, ms->fb_id);

    drv_restore_hw_state(pScrn);

    if (drmDropMaster(ms->fd))
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "drmDropMaster failed: %s\n", strerror(errno));

    pScrn->vtSema = FALSE;
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
drv_enter_vt(int scrnIndex, int flags)
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
	drv_save_hw_state(pScrn);
    }

    if (!ms->create_front_buffer(pScrn))
	return FALSE;

    if (!flags && !ms->bind_front_buffer(pScrn))
	return FALSE;

    if (!xf86SetDesiredModes(pScrn))
	return FALSE;

    if (ms->winsys_enter_vt)
	ms->winsys_enter_vt(pScrn);

    return TRUE;
}

static Bool
drv_switch_mode(int scrnIndex, DisplayModePtr mode, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static Bool
drv_close_screen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (pScrn->vtSema) {
	drv_leave_vt(scrnIndex, 0);
    }

    if (ms->winsys_screen_close)
	ms->winsys_screen_close(pScrn);

#ifdef DRI2
    if (ms->screen)
	xorg_dri2_close(pScreen);
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

    drmModeRmFB(ms->fd, ms->fb_id);
    ms->destroy_front_buffer(pScrn);

    if (ms->exa)
	xorg_exa_close(pScrn);
    ms->exa = NULL;

    drv_close_resource_management(pScrn);

    drv_close_drm(pScrn);

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = ms->CloseScreen;
    return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static ModeStatus
drv_valid_mode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}


/*
 * Front buffer backing store functions.
 */

static Bool
drv_destroy_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    pipe_texture_reference(&ms->root_texture, NULL);
    return TRUE;
}

static Bool
drv_create_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    unsigned handle, stride;
    struct pipe_texture *tex;
    int ret;

    ms->noEvict = TRUE;

    tex = xorg_exa_create_root_texture(pScrn, pScrn->virtualX, pScrn->virtualY,
				       pScrn->depth, pScrn->bitsPerPixel);

    if (!tex)
	return FALSE;

    if (!ms->api->local_handle_from_texture(ms->api, ms->screen,
					    tex,
					    &stride,
					    &handle))
	goto err_destroy;

    ret = drmModeAddFB(ms->fd,
		       pScrn->virtualX,
		       pScrn->virtualY,
		       pScrn->depth,
		       pScrn->bitsPerPixel,
		       stride,
		       handle,
		       &ms->fb_id);
    if (ret) {
	debug_printf("%s: failed to create framebuffer (%i, %s)",
		     __func__, ret, strerror(-ret));
	goto err_destroy;
    }

    pScrn->frameX0 = 0;
    pScrn->frameY0 = 0;
    drv_adjust_frame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    pipe_texture_reference(&ms->root_texture, tex);
    pipe_texture_reference(&tex, NULL);

    return TRUE;

err_destroy:
    pipe_texture_reference(&tex, NULL);
    return FALSE;
}

static Bool
drv_bind_front_buffer_ga3d(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct pipe_texture *check;

    xorg_exa_set_displayed_usage(rootPixmap);
    xorg_exa_set_shared_usage(rootPixmap);
    xorg_exa_set_texture(rootPixmap, ms->root_texture);
    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, NULL))
	FatalError("Couldn't adjust screen pixmap\n");

    check = xorg_exa_get_texture(rootPixmap);
    if (ms->root_texture != check)
	FatalError("Created new root texture\n");

    pipe_texture_reference(&check, NULL);
    return TRUE;
}

#ifdef HAVE_LIBKMS
static Bool
drv_destroy_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);

    /* XXX Do something with the rootPixmap.
     * This currently works fine but if we are getting crashes in
     * the fb functions after VT switches maybe look more into it.
     */
    (void)rootPixmap;

    if (!ms->root_bo)
	return TRUE;

    kms_bo_unmap(ms->root_bo);
    kms_bo_destroy(&ms->root_bo);
    return TRUE;
}

static Bool
drv_create_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    unsigned handle, stride;
    struct kms_bo *bo;
    unsigned attr[8];
    int ret;

    attr[0] = KMS_BO_TYPE;
    attr[1] = KMS_BO_TYPE_SCANOUT;
    attr[2] = KMS_WIDTH;
    attr[3] = pScrn->virtualX;
    attr[4] = KMS_HEIGHT;
    attr[5] = pScrn->virtualY;
    attr[6] = 0;

    if (kms_bo_create(ms->kms, attr, &bo))
	return FALSE;

    if (kms_bo_get_prop(bo, KMS_PITCH, &stride))
	goto err_destroy;

    if (kms_bo_get_prop(bo, KMS_HANDLE, &handle))
	goto err_destroy;

    ret = drmModeAddFB(ms->fd,
		       pScrn->virtualX,
		       pScrn->virtualY,
		       pScrn->depth,
		       pScrn->bitsPerPixel,
		       stride,
		       handle,
		       &ms->fb_id);
    if (ret) {
	debug_printf("%s: failed to create framebuffer (%i, %s)",
		     __func__, ret, strerror(-ret));
	goto err_destroy;
    }

    pScrn->frameX0 = 0;
    pScrn->frameY0 = 0;
    drv_adjust_frame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    ms->root_bo = bo;

    return TRUE;

err_destroy:
    kms_bo_destroy(&bo);
    return FALSE;
}

static Bool
drv_bind_front_buffer_kms(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    unsigned stride;
    void *ptr;

    if (kms_bo_get_prop(ms->root_bo, KMS_PITCH, &stride))
	return FALSE;

    if (kms_bo_map(ms->root_bo, &ptr))
	goto err_destroy;

    pScreen->ModifyPixmapHeader(rootPixmap,
				pScreen->width,
				pScreen->height,
				pScreen->rootDepth,
				pScrn->bitsPerPixel,
				stride,
				ptr);
    return TRUE;

err_destroy:
    kms_bo_destroy(&ms->root_bo);
    return FALSE;
}
#endif /* HAVE_LIBKMS */

static Bool drv_init_front_buffer_functions(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    if (ms->screen) {
	ms->destroy_front_buffer = drv_destroy_front_buffer_ga3d;
	ms->create_front_buffer = drv_create_front_buffer_ga3d;
	ms->bind_front_buffer = drv_bind_front_buffer_ga3d;
#ifdef HAVE_LIBKMS
    } else if (ms->kms) {
	ms->destroy_front_buffer = drv_destroy_front_buffer_kms;
	ms->create_front_buffer = drv_create_front_buffer_kms;
	ms->bind_front_buffer = drv_bind_front_buffer_kms;
#endif
    } else
	return FALSE;

    return TRUE;
}

/* vim: set sw=4 ts=8 sts=4: */
