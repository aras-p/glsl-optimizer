/**
 * \file dri_util.c
 * \brief DRI utility functions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "sarea.h"
#include "dri_util.h"

/**
 * \brief Print message to \c stderr if the \c LIBGL_DEBUG environment variable
 * is set.
 * 
 * Is called from the drivers.
 * 
 * \param f \e printf like format.
 * 
 * \internal
 * This function is a wrapper around vfprintf().
 */
void
__driUtilMessage(const char *f, ...)
{
    va_list args;

    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL error: \n");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}


/*****************************************************************/
/** \name Visual utility functions                               */
/*****************************************************************/
/*@{*/


/*@}*/


/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/


/**
 * \brief Unbind context.
 * 
 * \param drawable __DRIdrawable
 * \param context __DRIcontext
 * \param will_rebind not used.
 *
 * \return GL_TRUE on success, or GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawablePrivateRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters into their
 * respective real types it also assures they are not null. 
 */
static GLboolean driUnbindContext(__DRIdrawable *drawable, 
				  __DRIcontext *context)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)drawable;
    __DRIcontextPrivate  *pcp =  (__DRIcontextPrivate  *)context;
    __DRIscreenPrivate *psp;

    if (pdp == NULL || pcp == NULL) 
	return GL_FALSE;

    if (!(psp = (__DRIscreenPrivate *)pdp->driScreenPriv)) 
	return GL_FALSE;

    /* Let driver unbind drawable from context */
    (*psp->DriverAPI.UnbindContext)(pcp);

    if (pdp->refcount == 0) 
	return GL_FALSE;

    --pdp->refcount;

    return GL_TRUE;
}


/**
 * \brief Unbind context.
 * 
 * \param pDRIScreen __DRIscreen
 * \param drawable __DRIdrawable
 * \param context __DRIcontext
 *
 * \internal
 * This function and increments __DRIdrawablePrivateRec::refcount and calls
 * __DriverAPIRec::MakeCurrent to binds the drawable.
 * 
 * While casting the opaque private pointers into their
 * respective real types it also assures they are not null. 
 */
static GLboolean driBindContext(__DRIscreen *screen, __DRIdrawable *drawable, 
				__DRIcontext *context)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *)screen;
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)drawable;
    __DRIcontextPrivate *pcp = (__DRIcontextPrivate *)context;
    
    if (psp == NULL)
	return GL_FALSE;
    
    if (pdp == NULL || pcp == NULL) {
	(*psp->DriverAPI.MakeCurrent)(0, 0, 0);
	return GL_TRUE;
    }

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pdp->driContextPriv = pcp;
    pdp->refcount++;

    /* Call device-specific MakeCurrent */
    (*psp->DriverAPI.MakeCurrent)(pcp, pdp, pdp);

    return GL_TRUE;
}

/*@}*/


/*****************************************************************/
/** \name Drawable handling functions                            */
/*****************************************************************/
/*@{*/


/**
 * \brief Update private drawable information.
 *
 * \param pdp pointer to the private drawable information to update.
 * 
 * \internal
 * This function is a no-op. Should never be called but is referenced as an
 * external symbol from client drivers.
 */
void __driUtilUpdateDrawableInfo(__DRIdrawablePrivate *pdp)
{
   __DRIscreenPrivate *psp = pdp->driScreenPriv;

   pdp->numClipRects = psp->pSAREA->drawableTable[pdp->index].flags ? 1 : 0;
   pdp->lastStamp = *(pdp->pStamp);
}


/**
 * \brief Swap buffers.
 *
 * \param pDRIscreen __DRIscreen
 * \param drawablePrivate opaque pointer to the per-drawable private info.
 * 
 * \internal
 * This function calls __DRIdrawablePrivate::swapBuffers.
 * 
 * Is called directly from glXSwapBuffers().
 */
static void driSwapBuffers(__DRIdrawable *drawable)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)drawable;
    if (pdp)
	pdp->swapBuffers(pdp);
}


/**
 * \brief Destroy per-drawable private information.
 *
 * \param pDRIscreen __DRIscreen
 * \param drawablePrivate opaque pointer to the per-drawable private info.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyBuffer on \p drawablePrivate,
 * frees the clip rects if any, and finally frees \p drawablePrivate itself.
 */
static void driDestroyDrawable(__DRIdrawable *drawable)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)drawable;
    __DRIscreenPrivate *psp;

    if (pdp) {
	psp = pdp->driScreenPriv;
        (*psp->DriverAPI.DestroyBuffer)(pdp);
	if (pdp->pClipRects)
	    free(pdp->pClipRects);
	free(pdp);
    }
}


/**
 * \brief Create the per-drawable private driver information.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param draw the GLX drawable info.
 * \param vid visual ID.
 * \param pdraw will receive the drawable dependent methods.
 * 
 *
 * \returns a opaque pointer to the per-drawable private info on success, or NULL
 * on failure.
 * 
 * \internal
 * This function allocates and fills a __DRIdrawablePrivateRec structure,
 * initializing the invariant window dimensions and clip rects.  It obtains the
 * visual config, converts it into a __GLcontextModesRec and passes it to
 * __DriverAPIRec::CreateBuffer to create a buffer.
 */
static void *driCreateDrawable(__DRIscreen *screen,
                               int width, int height, int index,
                               const __GLcontextModes *glVisual)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *)screen;
    __DRIdrawablePrivate *pdp;

    if (!psp)
	return NULL;

    if (!(pdp = (__DRIdrawablePrivate *)malloc(sizeof(__DRIdrawablePrivate))))
	return NULL;

    pdp->index = index;
    pdp->refcount = 0;
    pdp->lastStamp = -1;
    pdp->numBackClipRects = 0;
    pdp->pBackClipRects = NULL;

    /* Initialize with the invariant window dimensions and clip rects here.
     */
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = width;
    pdp->h = height;
    pdp->numClipRects = 0;
    pdp->pClipRects = (XF86DRIClipRectPtr) malloc(sizeof(XF86DRIClipRectRec));
    (pdp->pClipRects)[0].x1 = 0;
    (pdp->pClipRects)[0].y1 = 0;
    (pdp->pClipRects)[0].x2 = width;
    (pdp->pClipRects)[0].y2 = height;

    pdp->driScreenPriv = psp;
    pdp->driContextPriv = 0;
    
    pdp->frontBuffer = psp->pFB;
    pdp->currentBuffer = pdp->frontBuffer;
    pdp->currentPitch = psp->fbStride;
    pdp->backBuffer = psp->pFB + psp->fbStride * psp->fbHeight;

    if (!(*psp->DriverAPI.CreateBuffer)(psp, pdp, glVisual, GL_FALSE)) {
       free(pdp);
       return NULL;
    }

    pdp->entry.destroyDrawable = driDestroyDrawable;
    pdp->entry.swapBuffers = driSwapBuffers;  /* called by glXSwapBuffers() */
    pdp->swapBuffers = psp->DriverAPI.SwapBuffers;

    pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);
    return (void *) pdp;
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/


/**
 * \brief Destroy the per-context private information.
 * 
 * \param contextPrivate opaque pointer to the per-drawable private info.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void driDestroyContext(__DRIcontext *context)
{
    __DRIcontextPrivate *pcp = (__DRIcontextPrivate *)context;
    __DRIscreenPrivate *psp = NULL;

    if (pcp) {
	(*pcp->driScreenPriv->DriverAPI.DestroyContext)(pcp);
        psp = pcp->driDrawablePriv->driScreenPriv;
	if (psp->fd) {
	   printf(">>> drmDestroyContext(0x%x)\n", (int) pcp->hHWContext);
	   drmDestroyContext(psp->fd, pcp->hHWContext);
	}
	free(pcp);
    }
}

/**
 * \brief Create the per-drawable private driver information.
 * 
 * \param dpy the display handle.
 * \param vis the visual information.
 * \param sharedPrivate the shared context dependent methods or NULL if non-existent.
 * \param pctx will receive the context dependent methods.
 *
 * \returns a opaque pointer to the per-context private information on success, or NULL
 * on failure.
 * 
 * \internal
 * This function allocates and fills a __DRIcontextPrivateRec structure.  It
 * gets the visual, converts it into a __GLcontextModesRec and passes it
 * to __DriverAPIRec::CreateContext to create the context.
 */
static void *driCreateContext(__DRIscreen *screen, 
                              const __GLcontextModes *glVisual,
                              void *sharedPrivate)
{
   __DRIscreenPrivate *psp = (__DRIscreenPrivate *)screen;
   __DRIcontextPrivate *pcp;
   __DRIcontextPrivate *pshare = (__DRIcontextPrivate *) sharedPrivate;
   void *shareCtx;

   if (!psp) 
      return NULL;

   if (!(pcp = (__DRIcontextPrivate *)malloc(sizeof(__DRIcontextPrivate))))
      return NULL;

   pcp->driScreenPriv = psp;
   pcp->driDrawablePriv = NULL;

   if (psp->fd) {
      if (drmCreateContext(psp->fd, &pcp->hHWContext)) {
	 fprintf(stderr, ">>> drmCreateContext failed\n");
	 free(pcp);
	 return NULL;
      }
   }

   shareCtx = pshare ? pshare->driverPrivate : NULL;
   
   if (!(*psp->DriverAPI.CreateContext)(glVisual, pcp, shareCtx)) {
      if (psp->fd) 
	 (void) drmDestroyContext(psp->fd, pcp->hHWContext);
      free(pcp);
      return NULL;
   }

   pcp->entry.destroyContext = driDestroyContext;
   pcp->entry.bindContext    = driBindContext;
   pcp->entry.unbindContext  = driUnbindContext;

   return pcp;
}

/*@}*/


/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/


/**
 * \brief Destroy the per-screen private information.
 * 
 * \param pDRIscreen __DRIscreen 
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRIscreen *screen)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *)screen;
    if (psp) {
	if (psp->DriverAPI.DestroyScreen)
	    (*psp->DriverAPI.DestroyScreen)(psp);
	
	if (psp->fd) 
	    (void)drmClose(psp->fd);
	
	free(psp->pDevPriv);
	free(psp);
    }
}


/**
 * \brief Create the per-screen private information.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param psc will receive the screen dependent methods.
 * \param numConfigs number of visuals.
 * \param config visuals.
 * \param driverAPI driver callbacks structure.
 *
 * \return a pointer to the per-screen private information.
 * 
 * \internal
 * This function allocates and fills a __DRIscreenPrivateRec structure. It
 * opens the DRM device verifying that the exported version matches the
 * expected.  It copies the driver callback functions and calls
 * __DriverAPIRec::InitDriver.
 *
 * If a client maps the framebuffer and SAREA regions.
 */
__DRIscreenPrivate *    
__driUtilCreateScreen(struct DRIDriverRec *driver,
 		      struct DRIDriverContextRec *driverContext,
		      const struct __DriverAPIRec *driverAPI)
{
    __DRIscreenPrivate *psp;
    
    if(!(psp = (__DRIscreenPrivate *)malloc(sizeof(__DRIscreenPrivate))))
	return NULL;
    
    psp->fd = drmOpen(NULL, driverContext->pciBusID);
    if (psp->fd < 0) {
	fprintf(stderr, "libGL error: failed to open DRM: %s\n", 
		strerror(-psp->fd));
	free(psp);
	return NULL;
    }
    
    {
	drmVersionPtr version = drmGetVersion(psp->fd);
	if (version) {
	    psp->drmMajor = version->version_major;
	    psp->drmMinor = version->version_minor;
	    psp->drmPatch = version->version_patchlevel;
	    drmFreeVersion(version);
	}
	else {
	    fprintf(stderr, "libGL error: failed to get drm version: %s\n", 
		    strerror(-psp->fd));
	    free(psp);
	    return NULL;
	}
    }
    
    /*
    * Fake various version numbers.
    */
    psp->ddxMajor = 4;
    psp->ddxMinor = 0;
    psp->ddxPatch = 1;
    psp->driMajor = 4;
    psp->driMinor = 1;
    psp->driPatch = 0;
    
    /* install driver's callback functions */
    psp->DriverAPI = *driverAPI;
    
    /*
    * Get device-specific info.  pDevPriv will point to a struct
    * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h)
    * that has information about the screen size, depth, pitch,
    * ancilliary buffers, DRM mmap handles, etc.
    */
    psp->fbOrigin = driverContext->shared.fbOrigin;  
    psp->fbSize = driverContext->shared.fbSize; 
    psp->fbStride = driverContext->shared.fbStride;
    psp->devPrivSize = driverContext->driverClientMsgSize;
    psp->pDevPriv = driverContext->driverClientMsg;
    psp->fbWidth = driverContext->shared.virtualWidth;
    psp->fbHeight = driverContext->shared.virtualHeight;
    psp->fbBPP = driverContext->bpp;
    
    if ((driverContext->FBAddress != NULL) && (driverContext->pSAREA != NULL)) {
	/* Already mapped in server */
	psp->pFB = driverContext->FBAddress;
	psp->pSAREA = driverContext->pSAREA;
    } else {
	/*
       * Map the framebuffer region.  
       */
	if (drmMap(psp->fd, driverContext->shared.hFrameBuffer, psp->fbSize, 
		   (drmAddressPtr)&psp->pFB)) {
	    fprintf(stderr, "libGL error: drmMap of framebuffer failed\n");
	    (void)drmClose(psp->fd);
	    free(psp);
	    return NULL;
	}
	
	/*
       * Map the SAREA region.  Further mmap regions may be setup in
       * each DRI driver's "createScreen" function.
       */
	if (drmMap(psp->fd, driverContext->shared.hSAREA,
		   driverContext->shared.SAREASize, 
		   (drmAddressPtr)&psp->pSAREA)) {
	    fprintf(stderr, "libGL error: drmMap of sarea failed\n");
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    (void)drmClose(psp->fd);
	    free(psp);
	    return NULL;
	}
	
#ifdef _EMBEDDED
	mprotect(psp->pSAREA, driverContext->shared.SAREASize, PROT_READ);
#endif
    }
    
    
    /* Initialize the screen specific GLX driver */
    if (psp->DriverAPI.InitDriver) {
	if (!(*psp->DriverAPI.InitDriver)(psp)) {
	    fprintf(stderr, "libGL error: InitDriver failed\n");
	    free(psp->pDevPriv);
	    (void)drmClose(psp->fd);
	    free(psp);
	    return NULL;
	}
    }
    
    psp->entry.destroyScreen  = driDestroyScreen;
    psp->entry.createContext  = driCreateContext;
    psp->entry.createDrawable = driCreateDrawable;
    
    return psp;
}



/**
 * \brief Create the per-screen private information.
 *
 * Version for drivers without a DRM module.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param numConfigs number of visuals.
 * \param config visuals.
 * \param driverAPI driver callbacks structure.
 * 
 * \internal
 * Same as __driUtilCreateScreen() but without opening the DRM device.
 */
__DRIscreenPrivate *     
__driUtilCreateScreenNoDRM(struct DRIDriverRec *driver,
			   struct DRIDriverContextRec *driverContext,
			   const struct __DriverAPIRec *driverAPI)
{
    __DRIscreenPrivate *psp;
    
    psp = (__DRIscreenPrivate *)calloc(1, sizeof(__DRIscreenPrivate));
    if (!psp) 
	return NULL;
    
    psp->ddxMajor = 4;
    psp->ddxMinor = 0;
    psp->ddxPatch = 1;
    psp->driMajor = 4;
    psp->driMinor = 1;
    psp->driPatch = 0;
    psp->fd = 0;
    
    psp->fbOrigin = driverContext->shared.fbOrigin; 
    psp->fbSize = driverContext->shared.fbSize; 
    psp->fbStride = driverContext->shared.fbStride;
    psp->devPrivSize = driverContext->driverClientMsgSize;
    psp->pDevPriv = driverContext->driverClientMsg;
    psp->fbWidth = driverContext->shared.virtualWidth;
    psp->fbHeight = driverContext->shared.virtualHeight;
    psp->fbBPP = driverContext->bpp;
    
    psp->pFB = driverContext->FBAddress;
    
    /* install driver's callback functions */
    psp->DriverAPI = *driverAPI;
    
    if ((driverContext->FBAddress != NULL) && (driverContext->pSAREA != NULL)) {
	/* Already mapped in server */
	psp->pFB = driverContext->FBAddress;
	psp->pSAREA = driverContext->pSAREA;
    } else {
	psp->fd = open("/dev/mem", O_RDWR, 0);
	/*
       * Map the framebuffer region.  
       */
	if (drmMap(psp->fd, driverContext->shared.hFrameBuffer, psp->fbSize, 
		   (drmAddressPtr)&psp->pFB)) {
	    fprintf(stderr, "libGL error: drmMap of framebuffer failed\n");
	    (void)drmClose(psp->fd);
	    free(psp);
	    return NULL;
	}
	driverContext->FBAddress = psp->pFB;
	
	/*
       * Map the SAREA region.  Non-DRM drivers use a shmem SAREA
       */
	int id;
	id = shmget(driverContext->shared.hSAREA, driverContext->shared.SAREASize, 0);
	driverContext->pSAREA = shmat(id, NULL, 0);
	if (!driverContext->pSAREA) {
	    fprintf(stderr, "libGL error: shmget of sarea failed\n");
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    (void)drmClose(psp->fd);
	    free(psp);
	    return NULL;
	}
	
	close(psp->fd);
	psp->fd = 0;
    }
    
    /* Initialize the screen specific GLX driver */
    if (psp->DriverAPI.InitDriver) {
	if (!(*psp->DriverAPI.InitDriver)(psp)) {
	    fprintf(stderr, "libGL error: InitDriver failed\n");
	    free(psp->pDevPriv);
	    free(psp);
	    return NULL;
	}
    }
    
    psp->entry.destroyScreen  = driDestroyScreen;
    psp->entry.createContext  = driCreateContext;
    psp->entry.createDrawable = driCreateDrawable;
    
    return psp;
}

/**
 * Calculate amount of swap interval used between GLX buffer swaps.
 * 
 * The usage value, on the range [0,max], is the fraction of total swap
 * interval time used between GLX buffer swaps is calculated.
 *
 *            \f$p = t_d / (i * t_r)\f$
 * 
 * Where \f$t_d\$f is the time since the last GLX buffer swap, \f$i\f$ is the
 * swap interval (as set by \c glXSwapIntervalSGI), and \f$t_r\f$ time
 * required for a single vertical refresh period (as returned by \c
 * glXGetMscRateOML).
 * 
 * See the documentation for the GLX_MESA_swap_frame_usage extension for more
 * details.
 *
 * \param   dPriv  Pointer to the private drawable structure.
 * \return  If less than a single swap interval time period was required
 *          between GLX buffer swaps, a number greater than 0 and less than
 *          1.0 is returned.  If exactly one swap interval time period is
 *          required, 1.0 is returned, and if more than one is required then
 *          a number greater than 1.0 will be returned.
 *
 * \sa glXSwapIntervalSGI(), glXGetMscRateOML().
 */
float
driCalculateSwapUsage( __DRIdrawablePrivate *dPriv, int64_t last_swap_ust,
		       int64_t current_ust )
{
   return 0.0f;
}

/**
 * Compare the current GLX API version with a driver supplied required version.
 * 
 * The minimum required version is compared with the API version exported by
 * the \c __glXGetInternalVersion function (in libGL.so).
 * 
 * \param   required_version Minimum required internal GLX API version.
 * \return  A tri-value return, as from strcmp is returned.  A value less
 *          than, equal to, or greater than zero will be returned if the
 *          internal GLX API version is less than, equal to, or greater
 *          than \c required_version.
 *
 * \sa __glXGetInternalVersion().
 */
int driCompareGLXAPIVersion( GLuint required_version )
{
   return 0;
}

/*@}*/
