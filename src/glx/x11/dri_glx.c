/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xdamage.h>
#include "glheader.h"
#include "glxclient.h"
#include "glcontextmodes.h"
#include "xf86dri.h"
#include "sarea.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "xf86drm.h"
#include "dri_common.h"

typedef struct __GLXDRIdisplayPrivateRec __GLXDRIdisplayPrivate;
typedef struct __GLXDRIcontextPrivateRec __GLXDRIcontextPrivate;

struct __GLXDRIdisplayPrivateRec {
    __GLXDRIdisplay base;

    /*
    ** XFree86-DRI version information
    */
    int driMajor;
    int driMinor;
    int driPatch;
};

struct __GLXDRIcontextPrivateRec {
    __GLXDRIcontext base;
    __DRIcontext *driContext;
    XID hwContextID;
    __GLXscreenConfigs *psc;
};

/*
 * Given a display pointer and screen number, determine the name of
 * the DRI driver for the screen. (I.e. "r128", "tdfx", etc).
 * Return True for success, False for failure.
 */
static Bool driGetDriverName(Display *dpy, int scrNum, char **driverName)
{
   int directCapable;
   Bool b;
   int driverMajor, driverMinor, driverPatch;

   *driverName = NULL;

   if (!XF86DRIQueryDirectRenderingCapable(dpy, scrNum, &directCapable)) {
      ErrorMessageF("XF86DRIQueryDirectRenderingCapable failed\n");
      return False;
   }
   if (!directCapable) {
      ErrorMessageF("XF86DRIQueryDirectRenderingCapable returned false\n");
      return False;
   }

   b = XF86DRIGetClientDriverName(dpy, scrNum, &driverMajor, &driverMinor,
                                  &driverPatch, driverName);
   if (!b) {
      ErrorMessageF("Cannot determine driver name for screen %d\n", scrNum);
      return False;
   }

   InfoMessageF("XF86DRIGetClientDriverName: %d.%d.%d %s (screen %d)\n",
	     driverMajor, driverMinor, driverPatch, *driverName, scrNum);

   return True;
}

/*
 * Exported function for querying the DRI driver for a given screen.
 *
 * The returned char pointer points to a static array that will be
 * overwritten by subsequent calls.
 */
PUBLIC const char *glXGetScreenDriver (Display *dpy, int scrNum) {
   static char ret[32];
   char *driverName;
   if (driGetDriverName(dpy, scrNum, &driverName)) {
      int len;
      if (!driverName)
	 return NULL;
      len = strlen (driverName);
      if (len >= 31)
	 return NULL;
      memcpy (ret, driverName, len+1);
      Xfree(driverName);
      return ret;
   }
   return NULL;
}

/*
 * Exported function for obtaining a driver's option list (UTF-8 encoded XML).
 *
 * The returned char pointer points directly into the driver. Therefore
 * it should be treated as a constant.
 *
 * If the driver was not found or does not support configuration NULL is
 * returned.
 *
 * Note: The driver remains opened after this function returns.
 */
PUBLIC const char *glXGetDriverConfig (const char *driverName)
{
   void *handle = driOpenDriver (driverName);
   if (handle)
      return dlsym (handle, "__driConfigOptions");
   else
      return NULL;
}

#ifdef XDAMAGE_1_1_INTERFACE

static GLboolean has_damage_post(Display *dpy)
{
    static GLboolean inited = GL_FALSE;
    static GLboolean has_damage;

    if (!inited) {
	int major, minor;

	if (XDamageQueryVersion(dpy, &major, &minor) &&
	    major == 1 && minor >= 1)
	{
	    has_damage = GL_TRUE;
	} else {
	    has_damage = GL_FALSE;
	}
	inited = GL_TRUE;
    }

    return has_damage;
}

static void __glXReportDamage(__DRIdrawable *driDraw,
			      int x, int y,
			      drm_clip_rect_t *rects, int num_rects,
			      GLboolean front_buffer,
			      void *loaderPrivate)
{
    XRectangle *xrects;
    XserverRegion region;
    int i;
    int x_off, y_off;
    __GLXDRIdrawable *glxDraw = loaderPrivate;
    __GLXscreenConfigs *psc = glxDraw->psc;
    Display *dpy = psc->dpy;
    Drawable drawable;

    if (!has_damage_post(dpy))
	return;

    if (front_buffer) {
	x_off = x;
	y_off = y;
	drawable = RootWindow(dpy, psc->scr);
    } else{
	x_off = 0;
	y_off = 0;
	drawable = glxDraw->xDrawable;
    }

    xrects = malloc(sizeof(XRectangle) * num_rects);
    if (xrects == NULL)
	return;

    for (i = 0; i < num_rects; i++) {
	xrects[i].x = rects[i].x1 + x_off;
	xrects[i].y = rects[i].y1 + y_off;
	xrects[i].width = rects[i].x2 - rects[i].x1;
	xrects[i].height = rects[i].y2 - rects[i].y1;
    }
    region = XFixesCreateRegion(dpy, xrects, num_rects);
    free(xrects);
    XDamageAdd(dpy, drawable, region);
    XFixesDestroyRegion(dpy, region);
}

static const __DRIdamageExtension damageExtension = {
    { __DRI_DAMAGE, __DRI_DAMAGE_VERSION },
    __glXReportDamage,
};

#endif

static GLboolean
__glXDRIGetDrawableInfo(__DRIdrawable *drawable,
			unsigned int *index, unsigned int *stamp, 
			int *X, int *Y, int *W, int *H,
			int *numClipRects, drm_clip_rect_t ** pClipRects,
			int *backX, int *backY,
			int *numBackClipRects, drm_clip_rect_t **pBackClipRects,
			void *loaderPrivate)
{
    __GLXDRIdrawable *glxDraw = loaderPrivate;
    __GLXscreenConfigs *psc = glxDraw->psc;
    Display *dpy = psc->dpy;

    return XF86DRIGetDrawableInfo(dpy, psc->scr, glxDraw->drawable,
				  index, stamp, X, Y, W, H,
				  numClipRects, pClipRects,
				  backX, backY,
				  numBackClipRects, pBackClipRects);
}

static const __DRIgetDrawableInfoExtension getDrawableInfoExtension = {
    { __DRI_GET_DRAWABLE_INFO, __DRI_GET_DRAWABLE_INFO_VERSION },
    __glXDRIGetDrawableInfo
};

static const __DRIextension *loader_extensions[] = {
    &systemTimeExtension.base,
    &getDrawableInfoExtension.base,
#ifdef XDAMAGE_1_1_INTERFACE
    &damageExtension.base,
#endif
    NULL
};

#ifndef GLX_USE_APPLEGL

/**
 * Perform the required libGL-side initialization and call the client-side
 * driver's \c __driCreateNewScreen function.
 * 
 * \param dpy    Display pointer.
 * \param scrn   Screen number on the display.
 * \param psc    DRI screen information.
 * \param driDpy DRI display information.
 * \param createNewScreen  Pointer to the client-side driver's
 *               \c __driCreateNewScreen function.
 * \returns A pointer to the \c __DRIscreenPrivate structure returned by
 *          the client-side driver on success, or \c NULL on failure.
 */
static void *
CallCreateNewScreen(Display *dpy, int scrn, __GLXscreenConfigs *psc,
		    __GLXDRIdisplayPrivate * driDpy)
{
    void *psp = NULL;
    drm_handle_t hSAREA;
    drmAddress pSAREA = MAP_FAILED;
    char *BusID;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    __DRIframebuffer  framebuffer;
    int   fd = -1;
    int   status;

    drm_magic_t magic;
    drmVersionPtr version;
    int newlyopened;
    char *driverName;
    drm_handle_t  hFB;
    int        junk;
    const __DRIconfig **driver_configs;

    /* DRI protocol version. */
    dri_version.major = driDpy->driMajor;
    dri_version.minor = driDpy->driMinor;
    dri_version.patch = driDpy->driPatch;

    framebuffer.base = MAP_FAILED;
    framebuffer.dev_priv = NULL;

    if (!XF86DRIOpenConnection(dpy, scrn, &hSAREA, &BusID)) {
	ErrorMessageF("XF86DRIOpenConnection failed\n");
	goto handle_error;
    }

    fd = drmOpenOnce(NULL, BusID, &newlyopened);

    Xfree(BusID); /* No longer needed */

    if (fd < 0) {
	ErrorMessageF("drmOpenOnce failed (%s)\n", strerror(-fd));
	goto handle_error;
    }

    if (drmGetMagic(fd, &magic)) {
	ErrorMessageF("drmGetMagic failed\n");
	goto handle_error;
    }

    version = drmGetVersion(fd);
    if (version) {
	drm_version.major = version->version_major;
	drm_version.minor = version->version_minor;
	drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }
    else {
	drm_version.major = -1;
	drm_version.minor = -1;
	drm_version.patch = -1;
    }

    if (newlyopened && !XF86DRIAuthConnection(dpy, scrn, magic)) {
	ErrorMessageF("XF86DRIAuthConnection failed\n");
	goto handle_error;
    }

    /* Get device name (like "tdfx") and the ddx version numbers.
     * We'll check the version in each DRI driver's "createNewScreen"
     * function. */
    if (!XF86DRIGetClientDriverName(dpy, scrn,
				    &ddx_version.major,
				    &ddx_version.minor,
				    &ddx_version.patch,
				    &driverName)) {
	ErrorMessageF("XF86DRIGetClientDriverName failed\n");
	goto handle_error;
    }

    Xfree(driverName); /* No longer needed. */

    /*
     * Get device-specific info.  pDevPriv will point to a struct
     * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h) that
     * has information about the screen size, depth, pitch, ancilliary
     * buffers, DRM mmap handles, etc.
     */
    if (!XF86DRIGetDeviceInfo(dpy, scrn, &hFB, &junk,
			      &framebuffer.size, &framebuffer.stride,
			      &framebuffer.dev_priv_size, &framebuffer.dev_priv)) {
	ErrorMessageF("XF86DRIGetDeviceInfo failed");
	goto handle_error;
    }

    framebuffer.width = DisplayWidth(dpy, scrn);
    framebuffer.height = DisplayHeight(dpy, scrn);

    /* Map the framebuffer region. */
    status = drmMap(fd, hFB, framebuffer.size, 
		    (drmAddressPtr)&framebuffer.base);
    if (status != 0) {
	ErrorMessageF("drmMap of framebuffer failed (%s)", strerror(-status));
	goto handle_error;
    }

    /* Map the SAREA region.  Further mmap regions may be setup in
     * each DRI driver's "createNewScreen" function.
     */
    status = drmMap(fd, hSAREA, SAREA_MAX, &pSAREA);
    if (status != 0) {
	ErrorMessageF("drmMap of SAREA failed (%s)", strerror(-status));
	goto handle_error;
    }

    psp = (*psc->legacy->createNewScreen)(scrn,
					  &ddx_version,
					  &dri_version,
					  &drm_version,
					  &framebuffer,
					  pSAREA,
					  fd,
					  loader_extensions,
					  &driver_configs,
					  psc);

    if (psp == NULL) {
	ErrorMessageF("Calling driver entry point failed");
	goto handle_error;
    }

    psc->configs = driConvertConfigs(psc->core, psc->configs, driver_configs);
    psc->visuals = driConvertConfigs(psc->core, psc->visuals, driver_configs);

    return psp;

 handle_error:
    if (pSAREA != MAP_FAILED)
	drmUnmap(pSAREA, SAREA_MAX);

    if (framebuffer.base != MAP_FAILED)
	drmUnmap((drmAddress)framebuffer.base, framebuffer.size);

    if (framebuffer.dev_priv != NULL)
	Xfree(framebuffer.dev_priv);

    if (fd >= 0)
	drmCloseOnce(fd);

    XF86DRICloseConnection(dpy, scrn);

    ErrorMessageF("reverting to software direct rendering\n");

    return NULL;
}

#else /* !GLX_USE_APPLEGL */

static void *
CallCreateNewScreen(Display *dpy, int scrn, __GLXscreenConfigs *psc,
		    __GLXDRIdisplayPrivate * driDpy)
{
    return NULL;
}

#endif /* !GLX_USE_APPLEGL */

static void driDestroyContext(__GLXDRIcontext *context,
			      __GLXscreenConfigs *psc, Display *dpy)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
			
    (*psc->core->destroyContext)(pcp->driContext);

    XF86DRIDestroyContext(psc->dpy, psc->scr, pcp->hwContextID);
    Xfree(pcp);
}

static Bool driBindContext(__GLXDRIcontext *context,
			   __GLXDRIdrawable *draw, __GLXDRIdrawable *read)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
    const __DRIcoreExtension *core = pcp->psc->core;

    return (*core->bindContext)(pcp->driContext,
				draw->driDrawable,
				read->driDrawable);
}

static void driUnbindContext(__GLXDRIcontext *context)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
    const __DRIcoreExtension *core = pcp->psc->core;

    (*core->unbindContext)(pcp->driContext);
}

static __GLXDRIcontext *driCreateContext(__GLXscreenConfigs *psc,
					 const __GLcontextModes *mode,
					 GLXContext gc,
					 GLXContext shareList, int renderType)
{
    __GLXDRIcontextPrivate *pcp, *pcp_shared;
    drm_context_t hwContext;
    __DRIcontext *shared = NULL;
    __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) mode;

    if (!psc || !psc->driScreen)
	return NULL;

    if (shareList) {
	pcp_shared = (__GLXDRIcontextPrivate *) shareList->driContext;
	shared = pcp_shared->driContext;
    }

    pcp = Xmalloc(sizeof *pcp);
    if (pcp == NULL)
	return NULL;

    pcp->psc = psc;
    if (!XF86DRICreateContextWithConfig(psc->dpy, psc->scr,
					mode->visualID,
					&pcp->hwContextID, &hwContext)) {
	Xfree(pcp);
	return NULL;
    }

    pcp->driContext =
        (*psc->legacy->createNewContext)(psc->__driScreen,
					 config->driConfig,
					 renderType,
					 shared,
					 hwContext,
					 pcp);
    if (pcp->driContext == NULL) {
	XF86DRIDestroyContext(psc->dpy, psc->scr, pcp->hwContextID);
	Xfree(pcp);
	return NULL;
    }

    pcp->base.destroyContext = driDestroyContext;
    pcp->base.bindContext = driBindContext;
    pcp->base.unbindContext = driUnbindContext;

    return &pcp->base;
}

static void driDestroyDrawable(__GLXDRIdrawable *pdraw)
{
    __GLXscreenConfigs *psc = pdraw->psc;

    (*psc->core->destroyDrawable)(pdraw->driDrawable);
    XF86DRIDestroyDrawable(psc->dpy, psc->scr, pdraw->drawable);
    Xfree(pdraw);
}

static __GLXDRIdrawable *driCreateDrawable(__GLXscreenConfigs *psc,
					   XID xDrawable,
					   GLXDrawable drawable,
					   const __GLcontextModes *modes)
{
    __GLXDRIdrawable *pdraw;
    drm_drawable_t hwDrawable;
    void *empty_attribute_list = NULL;
    __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;

    /* Old dri can't handle GLX 1.3+ drawable constructors. */
    if (xDrawable != drawable)
	return NULL;

    pdraw = Xmalloc(sizeof(*pdraw));
    if (!pdraw)
	return NULL;

    pdraw->drawable = drawable;
    pdraw->psc = psc;

    if (!XF86DRICreateDrawable(psc->dpy, psc->scr, drawable, &hwDrawable))
	return NULL;

    /* Create a new drawable */
    pdraw->driDrawable =
	(*psc->legacy->createNewDrawable)(psc->__driScreen,
					  config->driConfig,
					  hwDrawable,
					  GLX_WINDOW_BIT,
					  empty_attribute_list,
					  pdraw);

    if (!pdraw->driDrawable) {
	XF86DRIDestroyDrawable(psc->dpy, psc->scr, drawable);
	Xfree(pdraw);
	return NULL;
    }

    pdraw->destroyDrawable = driDestroyDrawable;

    return pdraw;
}

static void driDestroyScreen(__GLXscreenConfigs *psc)
{
    /* Free the direct rendering per screen data */
    if (psc->__driScreen)
	(*psc->core->destroyScreen)(psc->__driScreen);
    psc->__driScreen = NULL;
    if (psc->driver)
	dlclose(psc->driver);
}

static __GLXDRIscreen *driCreateScreen(__GLXscreenConfigs *psc, int screen,
				       __GLXdisplayPrivate *priv)
{
    __GLXDRIdisplayPrivate *pdp;
    __GLXDRIscreen *psp;
    const __DRIextension **extensions;
    char *driverName;
    int i;

    psp = Xmalloc(sizeof *psp);
    if (psp == NULL)
	return NULL;

    /* Initialize per screen dynamic client GLX extensions */
    psc->ext_list_first_time = GL_TRUE;

    if (!driGetDriverName(priv->dpy, screen, &driverName)) {
	Xfree(psp);
	return NULL;
    }

    psc->driver = driOpenDriver(driverName);
    Xfree(driverName);
    if (psc->driver == NULL) {
	Xfree(psp);
	return NULL;
    }

    extensions = dlsym(psc->driver, __DRI_DRIVER_EXTENSIONS);
    if (extensions == NULL) {
 	ErrorMessageF("driver exports no extensions (%s)\n", dlerror());
	Xfree(psp);
	return NULL;
    }

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_CORE) == 0)
	    psc->core = (__DRIcoreExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_LEGACY) == 0)
	    psc->legacy = (__DRIlegacyExtension *) extensions[i];
    }

    if (psc->core == NULL || psc->legacy == NULL) {
	Xfree(psp);
	return NULL;
    }
  
    pdp = (__GLXDRIdisplayPrivate *) priv->driDisplay;
    psc->__driScreen =
 	CallCreateNewScreen(psc->dpy, screen, psc, pdp);
    if (psc->__driScreen == NULL) {
 	dlclose(psc->driver);
 	Xfree(psp);
 	return NULL;
    }

    driBindExtensions(psc, 0);

    psp->destroyScreen = driDestroyScreen;
    psp->createContext = driCreateContext;
    psp->createDrawable = driCreateDrawable;

    return psp;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void driDestroyDisplay(__GLXDRIdisplay *dpy)
{
    Xfree(dpy);
}

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *driCreateDisplay(Display *dpy)
{
    __GLXDRIdisplayPrivate *pdpyp;
    int eventBase, errorBase;
    int major, minor, patch;

    if (!XF86DRIQueryExtension(dpy, &eventBase, &errorBase)) {
	return NULL;
    }

    if (!XF86DRIQueryVersion(dpy, &major, &minor, &patch)) {
	return NULL;
    }

    pdpyp = Xmalloc(sizeof *pdpyp);
    if (!pdpyp) {
	return NULL;
    }

    pdpyp->driMajor = major;
    pdpyp->driMinor = minor;
    pdpyp->driPatch = patch;

    pdpyp->base.destroyDisplay = driDestroyDisplay;
    pdpyp->base.createScreen = driCreateScreen;

    return &pdpyp->base;
}

#endif /* GLX_DIRECT_RENDERING */
