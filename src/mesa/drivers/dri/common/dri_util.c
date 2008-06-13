/**
 * \file dri_util.c
 * DRI utility functions.
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
 */


#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "main/imports.h"
#define None 0

#include "dri_util.h"
#include "drm_sarea.h"
#include "utils.h"

#ifndef GLX_OML_sync_control
typedef GLboolean ( * PFNGLXGETMSCRATEOMLPROC) (__DRIdrawable *drawable, int32_t *numerator, int32_t *denominator);
#endif

static void dri_get_drawable(__DRIdrawable *pdp);
static void dri_put_drawable(__DRIdrawable *pdp);

/**
 * This is just a token extension used to signal that the driver
 * supports setting a read drawable.
 */
const __DRIextension driReadDrawableExtension = {
    __DRI_READ_DRAWABLE, __DRI_READ_DRAWABLE_VERSION
};

/**
 * Print message to \c stderr if the \c LIBGL_DEBUG environment variable
 * is set. 
 * 
 * Is called from the drivers.
 * 
 * \param f \c printf like format string.
 */
void
__driUtilMessage(const char *f, ...)
{
    va_list args;

    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL: ");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

GLint
driIntersectArea( drm_clip_rect_t rect1, drm_clip_rect_t rect2 )
{
   if (rect2.x1 > rect1.x1) rect1.x1 = rect2.x1;
   if (rect2.x2 < rect1.x2) rect1.x2 = rect2.x2;
   if (rect2.y1 > rect1.y1) rect1.y1 = rect2.y1;
   if (rect2.y2 < rect1.y2) rect1.y2 = rect2.y2;

   if (rect1.x1 > rect1.x2 || rect1.y1 > rect1.y2) return 0;

   return (rect1.x2 - rect1.x1) * (rect1.y2 - rect1.y1);
}

/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

/**
 * Unbind context.
 * 
 * \param scrn the screen.
 * \param gc context.
 *
 * \return \c GL_TRUE on success, or \c GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawableRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters
 * into their respective real types it also assures they are not \c NULL. 
 */
static int driUnbindContext(__DRIcontext *pcp)
{
    __DRIscreen *psp;
    __DRIdrawable *pdp;
    __DRIdrawable *prp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (pcp == NULL)
        return GL_FALSE;

    psp = pcp->driScreenPriv;
    pdp = pcp->driDrawablePriv;
    prp = pcp->driReadablePriv;

    /* already unbound */
    if (!pdp && !prp)
      return GL_TRUE;
    /* Let driver unbind drawable from context */
    (*psp->DriverAPI.UnbindContext)(pcp);

    assert(pdp);
    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    dri_put_drawable(pdp);

    if (prp != pdp) {
        if (prp->refcount == 0) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

    	dri_put_drawable(prp);
    }


    /* XXX this is disabled so that if we call SwapBuffers on an unbound
     * window we can determine the last context bound to the window and
     * use that context's lock. (BrianP, 2-Dec-2000)
     */
    pcp->driDrawablePriv = pcp->driReadablePriv = NULL;

#if 0
    /* Unbind the drawable */
    pdp->driContextPriv = &psp->dummyContextPriv;
#endif

    return GL_TRUE;
}

/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static int driBindContext(__DRIcontext *pcp,
			  __DRIdrawable *pdp,
			  __DRIdrawable *prp)
{
    __DRIscreen *psp = NULL;

    /* Bind the drawable to the context */

    if (pcp) {
	psp = pcp->driScreenPriv;
	pcp->driDrawablePriv = pdp;
	pcp->driReadablePriv = prp;
	if (pdp) {
	    pdp->driContextPriv = pcp;
    	    dri_get_drawable(pdp);
	}
	if ( prp && pdp != prp ) {
    	    dri_get_drawable(prp);
	}
    }

    /*
    ** Now that we have a context associated with this drawable, we can
    ** initialize the drawable information if has not been done before.
    */

    assert(psp);
    if (!psp->dri2.enabled) {
	if (pdp && !pdp->pStamp) {
	    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	    __driUtilUpdateDrawableInfo(pdp);
	    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	}
	if (prp && pdp != prp && !prp->pStamp) {
	    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	    __driUtilUpdateDrawableInfo(prp);
	    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
        }
    }

    /* Call device-specific MakeCurrent */

    return (*psp->DriverAPI.MakeCurrent)(pcp, pdp, prp);
}

/*@}*/


/*****************************************************************/
/** \name Drawable handling functions                            */
/*****************************************************************/
/*@{*/

/**
 * Update private drawable information.
 *
 * \param pdp pointer to the private drawable information to update.
 * 
 * This function basically updates the __DRIdrawable struct's
 * cliprect information by calling \c __DRIinterfaceMethods::getDrawableInfo.
 * This is usually called by the DRI_VALIDATE_DRAWABLE_INFO macro which
 * compares the __DRIdrwablePrivate pStamp and lastStamp values.  If
 * the values are different that means we have to update the clipping
 * info.
 */
void
__driUtilUpdateDrawableInfo(__DRIdrawable *pdp)
{
    __DRIscreen *psp = pdp->driScreenPriv;
    __DRIcontext *pcp = pdp->driContextPriv;
    
    if (!pcp 
	|| ((pdp != pcp->driDrawablePriv) && (pdp != pcp->driReadablePriv))) {
	/* ERROR!!! 
	 * ...but we must ignore it. There can be many contexts bound to a
	 * drawable.
	 */
    }

    if (pdp->pClipRects) {
	free(pdp->pClipRects); 
	pdp->pClipRects = NULL;
    }

    if (pdp->pBackClipRects) {
	free(pdp->pBackClipRects); 
	pdp->pBackClipRects = NULL;
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (! (*psp->getDrawableInfo->getDrawableInfo)(pdp,
			  &pdp->index, &pdp->lastStamp,
			  &pdp->x, &pdp->y, &pdp->w, &pdp->h,
			  &pdp->numClipRects, &pdp->pClipRects,
			  &pdp->backX,
			  &pdp->backY,
			  &pdp->numBackClipRects,
			  &pdp->pBackClipRects,
			  pdp->loaderPrivate)) {
	/* Error -- eg the window may have been destroyed.  Keep going
	 * with no cliprects.
	 */
        pdp->pStamp = &pdp->lastStamp; /* prevent endless loop */
	pdp->numClipRects = 0;
	pdp->pClipRects = NULL;
	pdp->numBackClipRects = 0;
	pdp->pBackClipRects = NULL;
    }
    else
       pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);

    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
}

/*@}*/

/*****************************************************************/
/** \name GLX callbacks                                          */
/*****************************************************************/
/*@{*/

static void driReportDamage(__DRIdrawable *pdp,
			    struct drm_clip_rect *pClipRects, int numClipRects)
{
    __DRIscreen *psp = pdp->driScreenPriv;

    /* Check that we actually have the new damage report method */
    if (psp->damage) {
	/* Report the damage.  Currently, all our drivers draw
	 * directly to the front buffer, so we report the damage there
	 * rather than to the backing storein (if any).
	 */
	(*psp->damage->reportDamage)(pdp,
				     pdp->x, pdp->y,
				     pClipRects, numClipRects,
				     GL_TRUE, pdp->loaderPrivate);
    }
}


/**
 * Swap buffers.
 *
 * \param drawablePrivate opaque pointer to the per-drawable private info.
 * 
 * \internal
 * This function calls __DRIdrawable::swapBuffers.
 * 
 * Is called directly from glXSwapBuffers().
 */
static void driSwapBuffers(__DRIdrawable *dPriv)
{
    __DRIscreen *psp = dPriv->driScreenPriv;
    drm_clip_rect_t *rects;
    int i;

    psp->DriverAPI.SwapBuffers(dPriv);

    if (!dPriv->numClipRects)
        return;

    rects = malloc(sizeof(*rects) * dPriv->numClipRects);

    if (!rects)
        return;

    for (i = 0; i < dPriv->numClipRects; i++) {
        rects[i].x1 = dPriv->pClipRects[i].x1 - dPriv->x;
        rects[i].y1 = dPriv->pClipRects[i].y1 - dPriv->y;
        rects[i].x2 = dPriv->pClipRects[i].x2 - dPriv->x;
        rects[i].y2 = dPriv->pClipRects[i].y2 - dPriv->y;
    }

    driReportDamage(dPriv, rects, dPriv->numClipRects);
    free(rects);
}

static int driDrawableGetMSC( __DRIscreen *sPriv, __DRIdrawable *dPriv,
			      int64_t *msc )
{
    return sPriv->DriverAPI.GetDrawableMSC(sPriv, dPriv, msc);
}


static int driWaitForMSC(__DRIdrawable *dPriv, int64_t target_msc,
			 int64_t divisor, int64_t remainder,
			 int64_t * msc, int64_t * sbc)
{
    __DRIswapInfo  sInfo;
    int  status;

    status = dPriv->driScreenPriv->DriverAPI.WaitForMSC( dPriv, target_msc,
                                                         divisor, remainder,
                                                         msc );

    /* GetSwapInfo() may not be provided by the driver if GLX_SGI_video_sync
     * is supported but GLX_OML_sync_control is not.  Therefore, don't return
     * an error value if GetSwapInfo() is not implemented.
    */
    if ( status == 0
         && dPriv->driScreenPriv->DriverAPI.GetSwapInfo ) {
        status = dPriv->driScreenPriv->DriverAPI.GetSwapInfo( dPriv, & sInfo );
        *sbc = sInfo.swap_count;
    }

    return status;
}


const __DRImediaStreamCounterExtension driMediaStreamCounterExtension = {
    { __DRI_MEDIA_STREAM_COUNTER, __DRI_MEDIA_STREAM_COUNTER_VERSION },
    driWaitForMSC,
    driDrawableGetMSC,
};


static void driCopySubBuffer(__DRIdrawable *dPriv,
			      int x, int y, int w, int h)
{
    drm_clip_rect_t rect;

    rect.x1 = x;
    rect.y1 = dPriv->h - y - h;
    rect.x2 = x + w;
    rect.y2 = rect.y1 + h;
    driReportDamage(dPriv, &rect, 1);

    dPriv->driScreenPriv->DriverAPI.CopySubBuffer(dPriv, x, y, w, h);
}

const __DRIcopySubBufferExtension driCopySubBufferExtension = {
    { __DRI_COPY_SUB_BUFFER, __DRI_COPY_SUB_BUFFER_VERSION },
    driCopySubBuffer
};

static void driSetSwapInterval(__DRIdrawable *dPriv, unsigned int interval)
{
    dPriv->swap_interval = interval;
}

static unsigned int driGetSwapInterval(__DRIdrawable *dPriv)
{
    return dPriv->swap_interval;
}

const __DRIswapControlExtension driSwapControlExtension = {
    { __DRI_SWAP_CONTROL, __DRI_SWAP_CONTROL_VERSION },
    driSetSwapInterval,
    driGetSwapInterval
};


/**
 * This is called via __DRIscreenRec's createNewDrawable pointer.
 */
static __DRIdrawable *
driCreateNewDrawable(__DRIscreen *psp, const __DRIconfig *config,
		     drm_drawable_t hwDrawable, int renderType,
		     const int *attrs, void *data)
{
    __DRIdrawable *pdp;

    /* Since pbuffers are not yet supported, no drawable attributes are
     * supported either.
     */
    (void) attrs;

    pdp = malloc(sizeof *pdp);
    if (!pdp) {
	return NULL;
    }

    pdp->loaderPrivate = data;
    pdp->hHWDrawable = hwDrawable;
    pdp->refcount = 1;
    pdp->pStamp = NULL;
    pdp->lastStamp = 0;
    pdp->index = 0;
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = 0;
    pdp->h = 0;
    pdp->numClipRects = 0;
    pdp->numBackClipRects = 0;
    pdp->pClipRects = NULL;
    pdp->pBackClipRects = NULL;
    pdp->vblSeq = 0;
    pdp->vblFlags = 0;

    pdp->driScreenPriv = psp;
    pdp->driContextPriv = &psp->dummyContextPriv;

    if (!(*psp->DriverAPI.CreateBuffer)(psp, pdp, &config->modes,
					renderType == GLX_PIXMAP_BIT)) {
       free(pdp);
       return NULL;
    }

    pdp->msc_base = 0;

    /* This special default value is replaced with the configured
     * default value when the drawable is first bound to a direct
     * rendering context. 
     */
    pdp->swap_interval = (unsigned)-1;

    return pdp;
}


static __DRIdrawable *
dri2CreateNewDrawable(__DRIscreen *screen,
		      const __DRIconfig *config,
		      void *loaderPrivate)
{
    __DRIdrawable *pdraw;

    pdraw = driCreateNewDrawable(screen, config, 0, 0, NULL, loaderPrivate);
    if (!pdraw)
    	return NULL;

    pdraw->pClipRects = &pdraw->dri2.clipRect;
    pdraw->pBackClipRects = &pdraw->dri2.clipRect;

    pdraw->pStamp = &pdraw->dri2.stamp;
    *pdraw->pStamp = pdraw->lastStamp + 1;

    return pdraw;
}

static void dri_get_drawable(__DRIdrawable *pdp)
{
    pdp->refcount++;
}
	
static void dri_put_drawable(__DRIdrawable *pdp)
{
    __DRIscreen *psp;

    if (pdp) {
	pdp->refcount--;
	if (pdp->refcount)
	    return;

	psp = pdp->driScreenPriv;
        (*psp->DriverAPI.DestroyBuffer)(pdp);
	if (pdp->pClipRects && pdp->pClipRects != &pdp->dri2.clipRect) {
	    free(pdp->pClipRects);
	    pdp->pClipRects = NULL;
	}
	if (pdp->pBackClipRects && pdp->pClipRects != &pdp->dri2.clipRect) {
	    free(pdp->pBackClipRects);
	    pdp->pBackClipRects = NULL;
	}
	free(pdp);
    }
}

static void
driDestroyDrawable(__DRIdrawable *pdp)
{
    dri_put_drawable(pdp);
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-context private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void
driDestroyContext(__DRIcontext *pcp)
{
    if (pcp) {
	(*pcp->driScreenPriv->DriverAPI.DestroyContext)(pcp);
	free(pcp);
    }
}


/**
 * Create the per-drawable private driver information.
 * 
 * \param render_type   Type of rendering target.  \c GLX_RGBA is the only
 *                      type likely to ever be supported for direct-rendering.
 * \param shared        Context with which to share textures, etc. or NULL
 *
 * \returns An opaque pointer to the per-context private information on
 *          success, or \c NULL on failure.
 * 
 * \internal
 * This function allocates and fills a __DRIcontextRec structure.  It
 * performs some device independent initialization and passes all the
 * relevent information to __DriverAPIRec::CreateContext to create the
 * context.
 *
 */
static __DRIcontext *
driCreateNewContext(__DRIscreen *psp, const __DRIconfig *config,
		    int render_type, __DRIcontext *shared, 
		    drm_context_t hwContext, void *data)
{
    __DRIcontext *pcp;
    void * const shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;

    pcp = malloc(sizeof *pcp);
    if (!pcp)
	return NULL;

    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;
    pcp->loaderPrivate = data;
    
    pcp->dri2.draw_stamp = 0;
    pcp->dri2.read_stamp = 0;
    /* When the first context is created for a screen, initialize a "dummy"
     * context.
     */

    if (!psp->dri2.enabled && !psp->dummyContextPriv.driScreenPriv) {
        psp->dummyContextPriv.hHWContext = psp->pSAREA->dummy_context;
        psp->dummyContextPriv.driScreenPriv = psp;
        psp->dummyContextPriv.driDrawablePriv = NULL;
        psp->dummyContextPriv.driverPrivate = NULL;
	/* No other fields should be used! */
    }

    pcp->hHWContext = hwContext;

    if ( !(*psp->DriverAPI.CreateContext)(&config->modes, pcp, shareCtx) ) {
        free(pcp);
        return NULL;
    }

    return pcp;
}


static __DRIcontext *
dri2CreateNewContext(__DRIscreen *screen, const __DRIconfig *config,
		      __DRIcontext *shared, void *data)
{
    return driCreateNewContext(screen, config, 0, shared, 0, data);
}


static int
driCopyContext(__DRIcontext *dest, __DRIcontext *src, unsigned long mask)
{
    return GL_FALSE;
}

/*@}*/


/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-screen private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRIscreen *psp)
{
    if (psp) {
	/* No interaction with the X-server is possible at this point.  This
	 * routine is called after XCloseDisplay, so there is no protocol
	 * stream open to the X-server anymore.
	 */

	if (psp->DriverAPI.DestroyScreen)
	    (*psp->DriverAPI.DestroyScreen)(psp);

	if (!psp->dri2.enabled) {
	   (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	   (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	   (void)drmCloseOnce(psp->fd);
	}

	free(psp);
    }
}

static void
setupLoaderExtensions(__DRIscreen *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_GET_DRAWABLE_INFO) == 0)
	    psp->getDrawableInfo = (__DRIgetDrawableInfoExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_DAMAGE) == 0)
	    psp->damage = (__DRIdamageExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_SYSTEM_TIME) == 0)
	    psp->systemTime = (__DRIsystemTimeExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_DRI2_LOADER) == 0)
	    psp->dri2.loader = (__DRIdri2LoaderExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_IMAGE_LOOKUP) == 0)
	    psp->dri2.image = (__DRIimageLookupExtension *) extensions[i];
    }
}

/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 *
 * For legacy DRI.
 * 
 * \param scrn  Index of the screen
 * \param ddx_version Version of the 2D DDX.  This may not be meaningful for
 *                    all drivers.
 * \param dri_version Version of the "server-side" DRI.
 * \param drm_version Version of the kernel DRM.
 * \param frame_buffer Data describing the location and layout of the
 *                     framebuffer.
 * \param pSAREA       Pointer to the SAREA.
 * \param fd           Device handle for the DRM.
 * \param extensions   ??
 * \param driver_modes  Returns modes suppoted by the driver
 * \param loaderPrivate  ??
 * 
 * \note There is no need to check the minimum API version in this
 * function.  Since the name of this function is versioned, it is
 * impossible for a loader that is too old to even load this driver.
 */
static __DRIscreen *
driCreateNewScreen(int scrn,
		   const __DRIversion *ddx_version,
		   const __DRIversion *dri_version,
		   const __DRIversion *drm_version,
		   const __DRIframebuffer *frame_buffer,
		   drmAddress pSAREA, int fd, 
		   const __DRIextension **extensions,
		   const __DRIconfig ***driver_modes,
		   void *loaderPrivate)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;

    psp = calloc(1, sizeof *psp);
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    /*
    ** NOT_DONE: This is used by the X server to detect when the client
    ** has died while holding the drawable lock.  The client sets the
    ** drawable lock to this value.
    */
    psp->drawLockID = 1;

    psp->drm_version = *drm_version;
    psp->ddx_version = *ddx_version;
    psp->dri_version = *dri_version;

    psp->pSAREA = pSAREA;
    psp->lock = (drmLock *) &psp->pSAREA->lock;

    psp->pFB = frame_buffer->base;
    psp->fbSize = frame_buffer->size;
    psp->fbStride = frame_buffer->stride;
    psp->fbWidth = frame_buffer->width;
    psp->fbHeight = frame_buffer->height;
    psp->devPrivSize = frame_buffer->dev_priv_size;
    psp->pDevPriv = frame_buffer->dev_priv;
    psp->fbBPP = psp->fbStride * 8 / frame_buffer->width;

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;
    psp->dri2.enabled = GL_FALSE;

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psp->DriverAPI = driDriverAPI;

    *driver_modes = driDriverAPI.InitScreen(psp);
    if (*driver_modes == NULL) {
	free(psp);
	return NULL;
    }

    return psp;
}

/**
 * DRI2
 */
static __DRIscreen *
dri2CreateNewScreen(int scrn, int fd,
		    const __DRIextension **extensions,
		    const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;
    drmVersionPtr version;

    if (driDriverAPI.InitScreen2 == NULL)
        return NULL;

    psp = calloc(1, sizeof(*psp));
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    version = drmGetVersion(fd);
    if (version) {
	psp->drm_version.major = version->version_major;
	psp->drm_version.minor = version->version_minor;
	psp->drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;
    psp->dri2.enabled = GL_TRUE;

    psp->DriverAPI = driDriverAPI;
    *driver_configs = driDriverAPI.InitScreen2(psp);
    if (*driver_configs == NULL) {
	free(psp);
	return NULL;
    }

    psp->DriverAPI = driDriverAPI;

    return psp;
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    return psp->extensions;
}

/** Core interface */
const __DRIcoreExtension driCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    NULL,
    driDestroyScreen,
    driGetExtensions,
    driGetConfigAttrib,
    driIndexConfigAttrib,
    NULL,
    driDestroyDrawable,
    driSwapBuffers,
    NULL,
    driCopyContext,
    driDestroyContext,
    driBindContext,
    driUnbindContext
};

/** Legacy DRI interface */
const __DRIlegacyExtension driLegacyExtension = {
    { __DRI_LEGACY, __DRI_LEGACY_VERSION },
    driCreateNewScreen,
    driCreateNewDrawable,
    driCreateNewContext,
};

/** DRI2 interface */
const __DRIdri2Extension driDRI2Extension = {
    { __DRI_DRI2, __DRI_DRI2_VERSION },
    dri2CreateNewScreen,
    dri2CreateNewDrawable,
    dri2CreateNewContext,
};

static int
driFrameTracking(__DRIdrawable *drawable, GLboolean enable)
{
    return GLX_BAD_CONTEXT;
}

static int
driQueryFrameTracking(__DRIdrawable *dpriv,
		      int64_t * sbc, int64_t * missedFrames,
		      float * lastMissedUsage, float * usage)
{
   __DRIswapInfo   sInfo;
   int             status;
   int64_t         ust;
   __DRIscreen *psp = dpriv->driScreenPriv;

   status = dpriv->driScreenPriv->DriverAPI.GetSwapInfo( dpriv, & sInfo );
   if ( status == 0 ) {
      *sbc = sInfo.swap_count;
      *missedFrames = sInfo.swap_missed_count;
      *lastMissedUsage = sInfo.swap_missed_usage;

      (*psp->systemTime->getUST)( & ust );
      *usage = driCalculateSwapUsage( dpriv, sInfo.swap_ust, ust );
   }

   return status;
}

const __DRIframeTrackingExtension driFrameTrackingExtension = {
    { __DRI_FRAME_TRACKING, __DRI_FRAME_TRACKING_VERSION },
    driFrameTracking,
    driQueryFrameTracking    
};

/**
 * Calculate amount of swap interval used between GLX buffer swaps.
 * 
 * The usage value, on the range [0,max], is the fraction of total swap
 * interval time used between GLX buffer swaps is calculated.
 *
 *            \f$p = t_d / (i * t_r)\f$
 * 
 * Where \f$t_d\f$ is the time since the last GLX buffer swap, \f$i\f$ is the
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
 * \sa glXSwapIntervalSGI glXGetMscRateOML
 * 
 * \todo Instead of caching the \c glXGetMscRateOML function pointer, would it
 *       be possible to cache the sync rate?
 */
float
driCalculateSwapUsage( __DRIdrawable *dPriv, int64_t last_swap_ust,
		       int64_t current_ust )
{
   int32_t   n;
   int32_t   d;
   int       interval;
   float     usage = 1.0;
   __DRIscreen *psp = dPriv->driScreenPriv;

   if ( (*psp->systemTime->getMSCRate)(dPriv, &n, &d, dPriv->loaderPrivate) ) {
      interval = (dPriv->swap_interval != 0) ? dPriv->swap_interval : 1;


      /* We want to calculate
       * (current_UST - last_swap_UST) / (interval * us_per_refresh).  We get
       * current_UST by calling __glXGetUST.  last_swap_UST is stored in
       * dPriv->swap_ust.  interval has already been calculated.
       *
       * The only tricky part is us_per_refresh.  us_per_refresh is
       * 1000000 / MSC_rate.  We know the MSC_rate is n / d.  We can flip it
       * around and say us_per_refresh = 1000000 * d / n.  Since this goes in
       * the denominator of the final calculation, we calculate
       * (interval * 1000000 * d) and move n into the numerator.
       */

      usage = (current_ust - last_swap_ust);
      usage *= n;
      usage /= (interval * d);
      usage /= 1000000.0;
   }
   
   return usage;
}

void
dri2InvalidateDrawable(__DRIdrawable *drawable)
{
    drawable->dri2.stamp++;
}

/*@}*/
