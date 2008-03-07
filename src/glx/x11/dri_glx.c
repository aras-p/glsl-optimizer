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
/* $XFree86: xc/lib/GL/dri/dri_glx.c,v 1.14 2003/07/16 00:54:00 dawes Exp $ */

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include "glheader.h"
#include "glxclient.h"
#include "xf86dri.h"
#include "sarea.h"
#include <stdio.h>
#include <dlfcn.h>
#include "dri_glx.h"
#include <sys/types.h>
#include <stdarg.h>
#include "glcontextmodes.h"
#include <sys/mman.h>
#include "xf86drm.h"


#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif


#ifndef DEFAULT_DRIVER_DIR
/* this is normally defined in Mesa/configs/default with DRI_DRIVER_SEARCH_PATH */
#define DEFAULT_DRIVER_DIR "/usr/X11R6/lib/modules/dri"
#endif

static void InfoMessageF(const char *f, ...)
{
    va_list args;
    const char *env;

    if ((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose")) {
	fprintf(stderr, "libGL: ");
	va_start(args, f);
	vfprintf(stderr, f, args);
	va_end(args);
    }
}

/**
 * Print error to stderr, unless LIBGL_DEBUG=="quiet".
 */
static void ErrorMessageF(const char *f, ...)
{
    va_list args;
    const char *env;

    if ((env = getenv("LIBGL_DEBUG")) && !strstr(env, "quiet")) {
	fprintf(stderr, "libGL error: ");
	va_start(args, f);
	vfprintf(stderr, f, args);
	va_end(args);
    }
}


/**
 * Extract the ith directory path out of a colon-separated list of paths.  No
 * more than \c dirLen characters, including the terminating \c NUL, will be
 * written to \c dir.
 *
 * \param index  Index of path to extract (starting at zero)
 * \param paths  The colon-separated list of paths
 * \param dirLen Maximum length of result to store in \c dir
 * \param dir    Buffer to hold the extracted directory path
 *
 * \returns
 * The number of characters that would have been written to \c dir had there
 * been enough room.  This does not include the terminating \c NUL.  When
 * extraction fails, zero will be returned.
 * 
 * \todo
 * It seems like this function could be rewritten to use \c strchr.
 */
static size_t
ExtractDir(int index, const char *paths, int dirLen, char *dir)
{
   int i, len;
   const char *start, *end;

   /* find ith colon */
   start = paths;
   i = 0;
   while (i < index) {
      if (*start == ':') {
         i++;
         start++;
      }
      else if (*start == 0) {
         /* end of string and couldn't find ith colon */
         dir[0] = 0;
         return 0;
      }
      else {
         start++;
      }
   }

   while (*start == ':')
      start++;

   /* find next colon, or end of string */
   end = start + 1;
   while (*end != ':' && *end != 0) {
      end++;
   }

   /* copy string between <start> and <end> into result string */
   len = end - start;
   if (len > dirLen - 1)
      len = dirLen - 1;
   strncpy(dir, start, len);
   dir[len] = 0;

   return( end - start );
}


/**
 * Versioned name of the expected \c __driCreateNewScreen function.
 * 
 * The version of the last incompatible loader/driver inteface change is
 * appended to the name of the \c __driCreateNewScreen function.  This
 * prevents loaders from trying to load drivers that are too old.
 */
static const char createNewScreenName[] = __DRI_CREATE_NEW_SCREEN_STRING;


/**
 * Try to \c dlopen the named driver.
 *
 * This function adds the "_dri.so" suffix to the driver name and searches the
 * directories specified by the \c LIBGL_DRIVERS_PATH environment variable in
 * order to find the driver.
 *
 * \param driverName - a name like "tdfx", "i810", "mga", etc.
 *
 * \returns
 * A handle from \c dlopen, or \c NULL if driver file not found.
 */
static void *OpenDriver(const char *driverName)
{
   void *glhandle = NULL;
   char *libPaths = NULL;
   char libDir[1000];
   char realDriverName[200];
   void *handle = NULL;
   int i;

   /* Attempt to make sure libGL symbols will be visible to the driver */
   glhandle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);

   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use LIBGL_DRIVERS_PATH */
      libPaths = getenv("LIBGL_DRIVERS_PATH");
      if (!libPaths)
         libPaths = getenv("LIBGL_DRIVERS_DIR"); /* deprecated */
   }
   if (!libPaths)
      libPaths = DEFAULT_DRIVER_DIR;

   for ( i = 0 ; ExtractDir(i, libPaths, 1000, libDir) != 0 ; i++ ) {
      /* If TLS support is enabled, try to open the TLS version of the driver
       * binary first.  If that fails, try the non-TLS version.
       */
#ifdef GLX_USE_TLS
      snprintf(realDriverName, 200, "%s/tls/%s_dri.so", libDir, driverName);
      InfoMessageF("OpenDriver: trying %s\n", realDriverName);
      handle = dlopen(realDriverName, RTLD_NOW | RTLD_GLOBAL);
#endif

      if ( handle == NULL ) {
	 snprintf(realDriverName, 200, "%s/%s_dri.so", libDir, driverName);
	 InfoMessageF("OpenDriver: trying %s\n", realDriverName);
	 handle = dlopen(realDriverName, RTLD_NOW | RTLD_GLOBAL);
      }

      if ( handle != NULL )
	  break;
      else
	 ErrorMessageF("dlopen %s failed (%s)\n", realDriverName, dlerror());
   }

   if (!handle)
      ErrorMessageF("unable to load driver: %s_dri.so\n", driverName);

   if (glhandle)
      dlclose(glhandle);

   return handle;
}


/*
 * Given a display pointer and screen number, determine the name of
 * the DRI driver for the screen. (I.e. "r128", "tdfx", etc).
 * Return True for success, False for failure.
 */
static Bool GetDriverName(Display *dpy, int scrNum, char **driverName)
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
 * Given a display pointer and screen number, return a __DRIdriver handle.
 * Return NULL if anything goes wrong.
 */
static void *driGetDriver(Display *dpy, int scrNum)
{
   char *driverName;
   void *ret;

   if (GetDriverName(dpy, scrNum, &driverName)) {
      ret = OpenDriver(driverName);
      if (driverName)
     	 Xfree(driverName);
      return ret;
   }
   return NULL;
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
   if (GetDriverName(dpy, scrNum, &driverName)) {
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
PUBLIC const char *glXGetDriverConfig (const char *driverName) {
   void *handle = OpenDriver (driverName);
   if (handle)
      return dlsym (handle, "__driConfigOptions");
   else
      return NULL;
}

static void
filter_modes( __GLcontextModes ** server_modes,
	      const __GLcontextModes * driver_modes )
{
    __GLcontextModes * m;
    __GLcontextModes ** prev_next;
    const __GLcontextModes * check;

    if (driver_modes == NULL) {
	fprintf(stderr, "libGL warning: 3D driver returned no fbconfigs.\n");
	return;
    }

    /* For each mode in server_modes, check to see if a matching mode exists
     * in driver_modes.  If not, then the mode is not available.
     */

    prev_next = server_modes;
    for ( m = *prev_next ; m != NULL ; m = *prev_next ) {
	GLboolean do_delete = GL_TRUE;

	for ( check = driver_modes ; check != NULL ; check = check->next ) {
	    if ( _gl_context_modes_are_same( m, check ) ) {
		do_delete = GL_FALSE;
		break;
	    }
	}

	/* The 3D has to support all the modes that match the GLX visuals
	 * sent from the X server.
	 */
	if ( do_delete && (m->visualID != 0) ) {
	    do_delete = GL_FALSE;

	    /* don't warn for this visual (Novell #247471 / X.Org #6689) */
	    if (m->visualRating != GLX_NON_CONFORMANT_CONFIG) {
		fprintf(stderr, "libGL warning: 3D driver claims to not "
			"support visual 0x%02x\n", m->visualID);
	    }
	}

	if ( do_delete ) {
	    *prev_next = m->next;

	    m->next = NULL;
	    _gl_context_modes_destroy( m );
	}
	else {
	    prev_next = & m->next;
	}
    }
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
#endif /* XDAMAGE_1_1_INTERFACE */

static void __glXReportDamage(__DRIdrawable *driDraw,
			      int x, int y,
			      drm_clip_rect_t *rects, int num_rects,
			      GLboolean front_buffer)
{
#ifdef XDAMAGE_1_1_INTERFACE
    XRectangle *xrects;
    XserverRegion region;
    int i;
    int x_off, y_off;
    __GLXdrawable *glxDraw =
	containerOf(driDraw, __GLXdrawable, driDrawable);
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
	drawable = glxDraw->drawable;
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
#endif
}

static GLboolean
__glXDRIGetDrawableInfo(__DRIdrawable *drawable,
			unsigned int *index, unsigned int *stamp, 
			int *X, int *Y, int *W, int *H,
			int *numClipRects, drm_clip_rect_t ** pClipRects,
			int *backX, int *backY,
			int *numBackClipRects, drm_clip_rect_t **pBackClipRects)
{
    __GLXdrawable *glxDraw =
	containerOf(drawable, __GLXdrawable, driDrawable);
    __GLXscreenConfigs *psc = glxDraw->psc;
    Display *dpy = psc->dpy;

    return XF86DRIGetDrawableInfo(dpy, psc->scr, glxDraw->drawable,
				  index, stamp, X, Y, W, H,
				  numClipRects, pClipRects,
				  backX, backY,
				  numBackClipRects, pBackClipRects);
}


/**
 * Table of functions exported by the loader to the driver.
 */
static const __DRIcontextModesExtension contextModesExtension = {
    { __DRI_CONTEXT_MODES, __DRI_CONTEXT_MODES_VERSION },
    _gl_context_modes_create,
    _gl_context_modes_destroy,
};

static const __DRIsystemTimeExtension systemTimeExtension = {
    { __DRI_SYSTEM_TIME, __DRI_SYSTEM_TIME_VERSION },
    __glXGetUST,
    __driGetMscRateOML,
};

static const __DRIgetDrawableInfoExtension getDrawableInfoExtension = {
    { __DRI_GET_DRAWABLE_INFO, __DRI_GET_DRAWABLE_INFO_VERSION },
    __glXDRIGetDrawableInfo
};

static const __DRIdamageExtension damageExtension = {
    { __DRI_DAMAGE, __DRI_DAMAGE_VERSION },
    __glXReportDamage,
};

static const __DRIextension *loader_extensions[] = {
    &contextModesExtension.base,
    &systemTimeExtension.base,
    &getDrawableInfoExtension.base,
    &damageExtension.base,
    NULL
};


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
 * 
 * \todo This function needs to be modified to remove context-modes from the
 *       list stored in the \c __GLXscreenConfigsRec to match the list
 *       returned by the client-side driver.
 */
static void *
CallCreateNewScreen(Display *dpy, int scrn, __GLXscreenConfigs *psc,
		    __DRIdisplay * driDpy,
		    PFNCREATENEWSCREENFUNC createNewScreen)
{
    __DRIscreenPrivate *psp = NULL;
#ifndef GLX_USE_APPLEGL
    drm_handle_t hSAREA;
    drmAddress pSAREA = MAP_FAILED;
    char *BusID;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    __DRIframebuffer  framebuffer;
    int   fd = -1;
    int   status;
    const char * err_msg;
    const char * err_extra;

    dri_version.major = driDpy->private->driMajor;
    dri_version.minor = driDpy->private->driMinor;
    dri_version.patch = driDpy->private->driPatch;


    err_msg = "XF86DRIOpenConnection";
    err_extra = NULL;

    framebuffer.base = MAP_FAILED;
    framebuffer.dev_priv = NULL;

    if (XF86DRIOpenConnection(dpy, scrn, &hSAREA, &BusID)) {
        int newlyopened;
	fd = drmOpenOnce(NULL,BusID, &newlyopened);
	Xfree(BusID); /* No longer needed */

	err_msg = "open DRM";
	err_extra = strerror( -fd );

	if (fd >= 0) {
	    drm_magic_t magic;

	    err_msg = "drmGetMagic";
	    err_extra = NULL;

	    if (!drmGetMagic(fd, &magic)) {
		drmVersionPtr version = drmGetVersion(fd);
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

		err_msg = "XF86DRIAuthConnection";
		if (!newlyopened || XF86DRIAuthConnection(dpy, scrn, magic)) {
		    char *driverName;

		    /*
		     * Get device name (like "tdfx") and the ddx version
		     * numbers.  We'll check the version in each DRI driver's
		     * "createNewScreen" function.
		     */
		    err_msg = "XF86DRIGetClientDriverName";
		    if (XF86DRIGetClientDriverName(dpy, scrn,
						   &ddx_version.major,
						   &ddx_version.minor,
						   &ddx_version.patch,
						   &driverName)) {
			drm_handle_t  hFB;
			int        junk;

			/* No longer needed. */
			Xfree( driverName );


			/*
			 * Get device-specific info.  pDevPriv will point to a struct
			 * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h)
			 * that has information about the screen size, depth, pitch,
			 * ancilliary buffers, DRM mmap handles, etc.
			 */
			err_msg = "XF86DRIGetDeviceInfo";
			if (XF86DRIGetDeviceInfo(dpy, scrn,
						 &hFB,
						 &junk,
						 &framebuffer.size,
						 &framebuffer.stride,
						 &framebuffer.dev_priv_size,
						 &framebuffer.dev_priv)) {
			    framebuffer.width = DisplayWidth(dpy, scrn);
			    framebuffer.height = DisplayHeight(dpy, scrn);

			    /*
			     * Map the framebuffer region.
			     */
			    status = drmMap(fd, hFB, framebuffer.size, 
					    (drmAddressPtr)&framebuffer.base);

			    err_msg = "drmMap of framebuffer";
			    err_extra = strerror( -status );

			    if ( status == 0 ) {
				/*
				 * Map the SAREA region.  Further mmap regions
				 * may be setup in each DRI driver's
				 * "createNewScreen" function.
				 */
				status = drmMap(fd, hSAREA, SAREA_MAX, 
						&pSAREA);

				err_msg = "drmMap of sarea";
				err_extra = strerror( -status );

				if ( status == 0 ) {
				    __GLcontextModes * driver_modes = NULL;

				    err_msg = "InitDriver";
				    err_extra = NULL;
				    psp = (*createNewScreen)(scrn,
							     &psc->driScreen,
							     & ddx_version,
							     & dri_version,
							     & drm_version,
							     & framebuffer,
							     pSAREA,
							     fd,
							     loader_extensions,
							     & driver_modes );

				    filter_modes(&psc->configs, driver_modes);
				    filter_modes(&psc->visuals, driver_modes);
				    _gl_context_modes_destroy(driver_modes);
				}
			    }
			}
		    }
		}
	    }
	}
    }

    if ( psp == NULL ) {
	if ( pSAREA != MAP_FAILED ) {
	    (void)drmUnmap(pSAREA, SAREA_MAX);
	}

	if ( framebuffer.base != MAP_FAILED ) {
	    (void)drmUnmap((drmAddress)framebuffer.base, framebuffer.size);
	}

	if ( framebuffer.dev_priv != NULL ) {
	    Xfree(framebuffer.dev_priv);
	}

	if ( fd >= 0 ) {
	    (void)drmCloseOnce(fd);
	}

	(void)XF86DRICloseConnection(dpy, scrn);

	if ( err_extra != NULL ) {
	    fprintf(stderr, "libGL error: %s failed (%s)\n", err_msg,
		    err_extra);
	}
	else {
	    fprintf(stderr, "libGL error: %s failed\n", err_msg );
	}

        fprintf(stderr, "libGL error: reverting to (slow) indirect rendering\n");
    }
#endif /* !GLX_USE_APPLEGL */

    return psp;
}

void
driCreateScreen(__GLXscreenConfigs *psc, int screen,
		__GLXdisplayPrivate *priv)
{
    PFNCREATENEWSCREENFUNC createNewScreen;

    if (priv->driDisplay.private == NULL)
	return;

    /* Create drawable hash */
    psc->drawHash = __glxHashCreate();
    if ( psc->drawHash == NULL )
	return;

    /* Initialize per screen dynamic client GLX extensions */
    psc->ext_list_first_time = GL_TRUE;

    psc->driver = driGetDriver(priv->dpy, screen);
    createNewScreen = dlsym(psc->driver, createNewScreenName);
    if (createNewScreenName == NULL)
	return;

    psc->driScreen.private =
	CallCreateNewScreen(psc->dpy, screen, psc,
			    &priv->driDisplay, createNewScreen);
    if (psc->driScreen.private != NULL)
	__glXScrEnableDRIExtension(psc);
}

void driDestroyScreen(__GLXscreenConfigs *psc)
{
    /* Free the direct rendering per screen data */
    if (psc->driScreen.private)
	(*psc->driScreen.destroyScreen)(&psc->driScreen);
    psc->driScreen.private = NULL;
    if (psc->drawHash)
	__glxHashDestroy(psc->drawHash);
    if (psc->driver)
	dlclose(psc->driver);
}

/* Called from __glXFreeDisplayPrivate.
 */
static void driDestroyDisplay(Display *dpy, void *private)
{
    __DRIdisplayPrivate *pdpyp = (__DRIdisplayPrivate *)private;

    if (pdpyp)
	Xfree(pdpyp);
}


/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
void *driCreateDisplay(Display *dpy, __DRIdisplay *pdisp)
{
    __DRIdisplayPrivate *pdpyp;
    int eventBase, errorBase;
    int major, minor, patch;

    /* Initialize these fields to NULL in case we fail.
     * If we don't do this we may later get segfaults trying to free random
     * addresses when the display is closed.
     */
    pdisp->private = NULL;
    pdisp->destroyDisplay = NULL;

    if (!XF86DRIQueryExtension(dpy, &eventBase, &errorBase)) {
	return NULL;
    }

    if (!XF86DRIQueryVersion(dpy, &major, &minor, &patch)) {
	return NULL;
    }

    pdpyp = (__DRIdisplayPrivate *)Xmalloc(sizeof(__DRIdisplayPrivate));
    if (!pdpyp) {
	return NULL;
    }

    pdpyp->driMajor = major;
    pdpyp->driMinor = minor;
    pdpyp->driPatch = patch;

    pdisp->destroyDisplay = driDestroyDisplay;

    return (void *)pdpyp;
}

#endif /* GLX_DIRECT_RENDERING */
