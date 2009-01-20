/* $XFree86: xc/lib/GL/dri/dri_util.h,v 1.1 2002/02/22 21:32:52 dawes Exp $ */
/**
 * \file dri_util.h
 * DRI utility functions definitions.
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
 * \sa dri_util.c.
 * 
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Brian Paul <brian@precisioninsight.com>
 */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef _DRI_UTIL_H_
#define _DRI_UTIL_H_

#include <GL/gl.h>
#include <drm.h>
#include <drm_sarea.h>
#include <xf86drm.h>
#include "main/glheader.h"
#include "GL/internal/glcore.h"
#include "GL/internal/dri_interface.h"

#define GLX_BAD_CONTEXT                    5

typedef struct __DRIswapInfoRec        __DRIswapInfo;

/* Typedefs to avoid rewriting the world. */
typedef struct __DRIscreenRec	__DRIscreenPrivate;
typedef struct __DRIdrawableRec	__DRIdrawablePrivate;
typedef struct __DRIcontextRec	__DRIcontextPrivate;

/**
 * Extensions.
 */
extern const __DRIlegacyExtension driLegacyExtension;
extern const __DRIcoreExtension driCoreExtension;
extern const __DRIextension driReadDrawableExtension;
extern const __DRIcopySubBufferExtension driCopySubBufferExtension;
extern const __DRIswapControlExtension driSwapControlExtension;
extern const __DRIframeTrackingExtension driFrameTrackingExtension;
extern const __DRImediaStreamCounterExtension driMediaStreamCounterExtension;

/**
 * Used by DRI_VALIDATE_DRAWABLE_INFO
 */
#define DRI_VALIDATE_DRAWABLE_INFO_ONCE(pDrawPriv)              \
    do {                                                        \
	if (*(pDrawPriv->pStamp) != pDrawPriv->lastStamp) {     \
	    __driUtilUpdateDrawableInfo(pDrawPriv);             \
	}                                                       \
    } while (0)


/**
 * Utility macro to validate the drawable information.
 *
 * See __DRIdrawable::pStamp and __DRIdrawable::lastStamp.
 */
#define DRI_VALIDATE_DRAWABLE_INFO(psp, pdp)                            \
do {                                                                    \
    while (*(pdp->pStamp) != pdp->lastStamp) {                          \
        register unsigned int hwContext = psp->pSAREA->lock.lock &      \
		     ~(DRM_LOCK_HELD | DRM_LOCK_CONT);                  \
	DRM_UNLOCK(psp->fd, &psp->pSAREA->lock, hwContext);             \
                                                                        \
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);     \
	DRI_VALIDATE_DRAWABLE_INFO_ONCE(pdp);                           \
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);   \
                                                                        \
	DRM_LIGHT_LOCK(psp->fd, &psp->pSAREA->lock, hwContext);         \
    }                                                                   \
} while (0)

/**
 * Same as above, but for two drawables simultaneously.
 *
 */

#define DRI_VALIDATE_TWO_DRAWABLES_INFO(psp, pdp, prp)			\
do {								\
    while (*((pdp)->pStamp) != (pdp)->lastStamp ||			\
	   *((prp)->pStamp) != (prp)->lastStamp) {			\
        register unsigned int hwContext = (psp)->pSAREA->lock.lock &	\
	    ~(DRM_LOCK_HELD | DRM_LOCK_CONT);				\
	DRM_UNLOCK((psp)->fd, &(psp)->pSAREA->lock, hwContext);		\
									\
	DRM_SPINLOCK(&(psp)->pSAREA->drawable_lock, (psp)->drawLockID);	\
	DRI_VALIDATE_DRAWABLE_INFO_ONCE(pdp);                           \
	DRI_VALIDATE_DRAWABLE_INFO_ONCE(prp);				\
	DRM_SPINUNLOCK(&(psp)->pSAREA->drawable_lock, (psp)->drawLockID); \
									\
	DRM_LIGHT_LOCK((psp)->fd, &(psp)->pSAREA->lock, hwContext);	\
    }                                                                   \
} while (0)


/**
 * Driver callback functions.
 *
 * Each DRI driver must have one of these structures with all the pointers set
 * to appropriate functions within the driver.
 * 
 * When glXCreateContext() is called, for example, it'll call a helper function
 * dri_util.c which in turn will jump through the \a CreateContext pointer in
 * this structure.
 */
struct __DriverAPIRec {
    const __DRIconfig **(*InitScreen) (__DRIscreen * priv);

    /**
     * Screen destruction callback
     */
    void (*DestroyScreen)(__DRIscreen *driScrnPriv);

    /**
     * Context creation callback
     */	    	    
    GLboolean (*CreateContext)(const __GLcontextModes *glVis,
                               __DRIcontext *driContextPriv,
                               void *sharedContextPrivate);

    /**
     * Context destruction callback
     */
    void (*DestroyContext)(__DRIcontext *driContextPriv);

    /**
     * Buffer (drawable) creation callback
     */
    GLboolean (*CreateBuffer)(__DRIscreen *driScrnPriv,
                              __DRIdrawable *driDrawPriv,
                              const __GLcontextModes *glVis,
                              GLboolean pixmapBuffer);
    
    /**
     * Buffer (drawable) destruction callback
     */
    void (*DestroyBuffer)(__DRIdrawable *driDrawPriv);

    /**
     * Buffer swapping callback 
     */
    void (*SwapBuffers)(__DRIdrawable *driDrawPriv);

    /**
     * Context activation callback
     */
    GLboolean (*MakeCurrent)(__DRIcontext *driContextPriv,
                             __DRIdrawable *driDrawPriv,
                             __DRIdrawable *driReadPriv);

    /**
     * Context unbinding callback
     */
    GLboolean (*UnbindContext)(__DRIcontext *driContextPriv);
  
    /**
     * Retrieves statistics about buffer swap operations.  Required if
     * GLX_OML_sync_control or GLX_MESA_swap_frame_usage is supported.
     */
    int (*GetSwapInfo)( __DRIdrawable *dPriv, __DRIswapInfo * sInfo );


    /**
     * These are required if GLX_OML_sync_control is supported.
     */
    /*@{*/
    int (*WaitForMSC)( __DRIdrawable *priv, int64_t target_msc, 
		       int64_t divisor, int64_t remainder,
		       int64_t * msc );
    int (*WaitForSBC)( __DRIdrawable *priv, int64_t target_sbc,
		       int64_t * msc, int64_t * sbc );

    int64_t (*SwapBuffersMSC)( __DRIdrawable *priv, int64_t target_msc,
			       int64_t divisor, int64_t remainder );
    /*@}*/
    void (*CopySubBuffer)(__DRIdrawable *driDrawPriv,
			  int x, int y, int w, int h);

    /**
     * New version of GetMSC so we can pass drawable data to the low
     * level DRM driver (e.g. pipe info).  Required if
     * GLX_SGI_video_sync or GLX_OML_sync_control is supported.
     */
    int (*GetDrawableMSC) ( __DRIscreen * priv,
			    __DRIdrawable *drawablePrivate,
			    int64_t *count);



    /* DRI2 Entry point */
    const __DRIconfig **(*InitScreen2) (__DRIscreen * priv);
};

extern const struct __DriverAPIRec driDriverAPI;


struct __DRIswapInfoRec {
    /** 
     * Number of swapBuffers operations that have been *completed*. 
     */
    uint64_t swap_count;

    /**
     * Unadjusted system time of the last buffer swap.  This is the time
     * when the swap completed, not the time when swapBuffers was called.
     */
    int64_t   swap_ust;

    /**
     * Number of swap operations that occurred after the swap deadline.  That
     * is if a swap happens more than swap_interval frames after the previous
     * swap, it has missed its deadline.  If swap_interval is 0, then the
     * swap deadline is 1 frame after the previous swap.
     */
    uint64_t swap_missed_count;

    /**
     * Amount of time used by the last swap that missed its deadline.  This
     * is calculated as (__glXGetUST() - swap_ust) / (swap_interval * 
     * time_for_single_vrefresh)).  If the actual value of swap_interval is
     * 0, then 1 is used instead.  If swap_missed_count is non-zero, this
     * should be greater-than 1.0.
     */
    float     swap_missed_usage;
};


/**
 * Per-drawable private DRI driver information.
 */
struct __DRIdrawableRec {
    /**
     * Kernel drawable handle
     */
    drm_drawable_t hHWDrawable;

    /**
     * Driver's private drawable information.  
     *
     * This structure is opaque.
     */
    void *driverPrivate;

    /**
     * Private data from the loader.  We just hold on to it and pass
     * it back when calling into loader provided functions.
     */
    void *loaderPrivate;

    /**
     * Reference count for number of context's currently bound to this
     * drawable.  
     *
     * Once it reaches zero, the drawable can be destroyed.
     *
     * \note This behavior will change with GLX 1.3.
     */
    int refcount;

    /**
     * Index of this drawable information in the SAREA.
     */
    unsigned int index;

    /**
     * Pointer to the "drawable has changed ID" stamp in the SAREA.
     */
    unsigned int *pStamp;

    /**
     * Last value of the stamp.
     *
     * If this differs from the value stored at __DRIdrawable::pStamp,
     * then the drawable information has been modified by the X server, and the
     * drawable information (below) should be retrieved from the X server.
     */
    unsigned int lastStamp;

    /**
     * \name Drawable 
     *
     * Drawable information used in software fallbacks.
     */
    /*@{*/
    int x;
    int y;
    int w;
    int h;
    int numClipRects;
    drm_clip_rect_t *pClipRects;
    /*@}*/

    /**
     * \name Back and depthbuffer
     *
     * Information about the back and depthbuffer where different from above.
     */
    /*@{*/
    int backX;
    int backY;
    int backClipRectType;
    int numBackClipRects;
    drm_clip_rect_t *pBackClipRects;
    /*@}*/

    /**
     * \name Vertical blank tracking information
     * Used for waiting on vertical blank events.
     */
    /*@{*/
    unsigned int vblSeq;
    unsigned int vblFlags;
    /*@}*/

    /**
     * \name Monotonic MSC tracking
     *
     * Low level driver is responsible for updating msc_base and
     * vblSeq values so that higher level code can calculate
     * a new msc value or msc target for a WaitMSC call.  The new value
     * will be:
     *   msc = msc_base + get_vblank_count() - vblank_base;
     *
     * And for waiting on a value, core code will use:
     *   actual_target = target_msc - msc_base + vblank_base;
     */
    /*@{*/
    int64_t vblank_base;
    int64_t msc_base;
    /*@}*/

    /**
     * Pointer to context to which this drawable is currently bound.
     */
    __DRIcontext *driContextPriv;

    /**
     * Pointer to screen on which this drawable was created.
     */
    __DRIscreen *driScreenPriv;

    /**
     * Controls swap interval as used by GLX_SGI_swap_control and
     * GLX_MESA_swap_control.
     */
    unsigned int swap_interval;
};

/**
 * Per-context private driver information.
 */
struct __DRIcontextRec {
    /**
     * Kernel context handle used to access the device lock.
     */
    drm_context_t hHWContext;

    /**
     * Device driver's private context data.  This structure is opaque.
     */
    void *driverPrivate;

    /**
     * Pointer back to the \c __DRIcontext that contains this structure.
     */
    __DRIcontext *pctx;

    /**
     * Pointer to drawable currently bound to this context for drawing.
     */
    __DRIdrawable *driDrawablePriv;

    /**
     * Pointer to drawable currently bound to this context for reading.
     */
    __DRIdrawable *driReadablePriv;

    /**
     * Pointer to screen on which this context was created.
     */
    __DRIscreen *driScreenPriv;
};

/**
 * Per-screen private driver information.
 */
struct __DRIscreenRec {
    /**
     * Current screen's number
     */
    int myNum;

    /**
     * Callback functions into the hardware-specific DRI driver code.
     */
    struct __DriverAPIRec DriverAPI;

    const __DRIextension **extensions;
    /**
     * DDX / 2D driver version information.
     */
    __DRIversion ddx_version;

    /**
     * DRI X extension version information.
     */
    __DRIversion dri_version;

    /**
     * DRM (kernel module) version information.
     */
    __DRIversion drm_version;

    /**
     * ID used when the client sets the drawable lock.
     *
     * The X server uses this value to detect if the client has died while
     * holding the drawable lock.
     */
    int drawLockID;

    /**
     * File descriptor returned when the kernel device driver is opened.
     * 
     * Used to:
     *   - authenticate client to kernel
     *   - map the frame buffer, SAREA, etc.
     *   - close the kernel device driver
     */
    int fd;

    /**
     * SAREA pointer 
     *
     * Used to access:
     *   - the device lock
     *   - the device-independent per-drawable and per-context(?) information
     */
    drm_sarea_t *pSAREA;

    /**
     * \name Direct frame buffer access information 
     * Used for software fallbacks.
     */
    /*@{*/
    unsigned char *pFB;
    int fbSize;
    int fbOrigin;
    int fbStride;
    int fbWidth;
    int fbHeight;
    int fbBPP;
    /*@}*/

    /**
     * \name Device-dependent private information (stored in the SAREA).
     *
     * This data is accessed by the client driver only.
     */
    /*@{*/
    void *pDevPriv;
    int devPrivSize;
    /*@}*/

    /**
     * Dummy context to which drawables are bound when not bound to any
     * other context. 
     *
     * A dummy hHWContext is created for this context, and is used by the GL
     * core when a hardware lock is required but the drawable is not currently
     * bound (e.g., potentially during a SwapBuffers request).  The dummy
     * context is created when the first "real" context is created on this
     * screen.
     */
    __DRIcontext dummyContextPriv;

    /**
     * Device-dependent private information (not stored in the SAREA).
     * 
     * This pointer is never touched by the DRI layer.
     */
    void *private;

    /**
     * Pointer back to the \c __DRIscreen that contains this structure.
     */
    __DRIscreen *psc;

    /* Extensions provided by the loader. */
    const __DRIgetDrawableInfoExtension *getDrawableInfo;
    const __DRIsystemTimeExtension *systemTime;
    const __DRIdamageExtension *damage;

    struct {
	/* Flag to indicate that this is a DRI2 screen.  Many of the above
	 * fields will not be valid or initializaed in that case. */
	int enabled;
	__DRIdri2LoaderExtension *loader;
    } dri2;

    /* The lock actually in use, old sarea or DRI2 */
    drmLock *lock;
};

extern void
__driUtilMessage(const char *f, ...);


extern void
__driUtilUpdateDrawableInfo(__DRIdrawable *pdp);

extern float
driCalculateSwapUsage( __DRIdrawable *dPriv,
		       int64_t last_swap_ust, int64_t current_ust );

extern GLint
driIntersectArea( drm_clip_rect_t rect1, drm_clip_rect_t rect2 );

#endif /* _DRI_UTIL_H_ */
