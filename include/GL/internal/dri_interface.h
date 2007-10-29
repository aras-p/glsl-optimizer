/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file dri_interface.h
 *
 * This file contains all the types and functions that define the interface
 * between a DRI driver and driver loader.  Currently, the most common driver
 * loader is the XFree86 libGL.so.  However, other loaders do exist, and in
 * the future the server-side libglx.a will also be a loader.
 * 
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef DRI_INTERFACE_H
#define DRI_INTERFACE_H

#include <GL/internal/glcore.h>
#include <drm.h>

/**
 * \name DRI interface structures
 *
 * The following structures define the interface between the GLX client
 * side library and the DRI (direct rendering infrastructure).
 */
/*@{*/
typedef struct __DRIdisplayRec		__DRIdisplay;
typedef struct __DRIscreenRec		__DRIscreen;
typedef struct __DRIcontextRec		__DRIcontext;
typedef struct __DRIdrawableRec		__DRIdrawable;
typedef struct __DRIdriverRec		__DRIdriver;
typedef struct __DRIframebufferRec	__DRIframebuffer;
typedef struct __DRIversionRec		__DRIversion;
typedef struct __DRIinterfaceMethodsRec	__DRIinterfaceMethods;

typedef struct __DRIextensionRec		__DRIextension;
typedef struct __DRIcopySubBufferExtensionRec	__DRIcopySubBufferExtension;
typedef struct __DRIswapControlExtensionRec	__DRIswapControlExtension;
typedef struct __DRIallocateExtensionRec	__DRIallocateExtension;
typedef struct __DRIframeTrackingExtensionRec	__DRIframeTrackingExtension;
typedef struct __DRImediaStreamCounterExtensionRec	__DRImediaStreamCounterExtension;
typedef struct __DRItexOffsetExtensionRec	__DRItexOffsetExtension;
/*@}*/


/**
 * Extension struct.  Drivers 'inherit' from this struct by embedding
 * it as the first element in the extension struct.  The
 * __DRIscreen::getExtensions entry point will return a list of these
 * structs and the loader can use the extensions it knows about by
 * casting it to a more specific extension and optionally advertising
 * the GLX extension.  See below for examples.
 *
 * We never break API in for a DRI extension.  If we need to change
 * the way things work in a non-backwards compatible manner, we
 * introduce a new extension.  During a transition period, we can
 * leave both the old and the new extension in the driver, which
 * allows us to move to the new interface without having to update the
 * loader(s) in lock step.
 *
 * However, we can add entry points to an extension over time as long
 * as we don't break the old ones.  As we add entry points to an
 * extension, we increase the version number.  The corresponding
 * #define can be used to guard code that accesses the new entry
 * points at compile time and the version field in the extension
 * struct can be used at run-time to determine how to use the
 * extension.
 */
struct __DRIextensionRec {
    const char *name;
    int version;
};

/**
 * Used by drivers to indicate support for setting the read drawable.
 */
#define __DRI_READ_DRAWABLE "DRI_ReadDrawable"
#define __DRI_READ_DRAWABLE_VERSION 1

/**
 * Used by drivers that implement the GLX_MESA_copy_sub_buffer extension.
 */
#define __DRI_COPY_SUB_BUFFER "DRI_CopySubBuffer"
#define __DRI_COPY_SUB_BUFFER_VERSION 1
struct __DRIcopySubBufferExtensionRec {
    __DRIextension base;
    void (*copySubBuffer)(__DRIdrawable *drawable, int x, int y, int w, int h);
};

/**
 * Used by drivers that implement the GLX_SGI_swap_control or
 * GLX_MESA_swap_control extension.
 */
#define __DRI_SWAP_CONTROL "DRI_SwapControl"
#define __DRI_SWAP_CONTROL_VERSION 1
struct __DRIswapControlExtensionRec {
    __DRIextension base;
    void (*setSwapInterval)(__DRIdrawable *drawable, unsigned int inteval);
    unsigned int (*getSwapInterval)(__DRIdrawable *drawable);
};

/**
 * Used by drivers that implement the GLX_MESA_allocate_memory.
 */
#define __DRI_ALLOCATE "DRI_Allocate"
#define __DRI_ALLOCATE_VERSION 1
struct __DRIallocateExtensionRec {
    __DRIextension base;

    void *(*allocateMemory)(__DRIscreen *screen, GLsizei size,
			    GLfloat readfreq, GLfloat writefreq,
			    GLfloat priority);
   
    void (*freeMemory)(__DRIscreen *screen, GLvoid *pointer);
   
    GLuint (*memoryOffset)(__DRIscreen *screen, const GLvoid *pointer);
};

/**
 * Used by drivers that implement the GLX_MESA_swap_frame_usage extension.
 */
#define __DRI_FRAME_TRACKING "DRI_FrameTracking"
#define __DRI_FRAME_TRACKING_VERSION 1
struct __DRIframeTrackingExtensionRec {
    __DRIextension base;

    /**
     * Enable or disable frame usage tracking.
     * 
     * \since Internal API version 20030317.
     */
    int (*frameTracking)(__DRIdrawable *drawable, GLboolean enable);

    /**
     * Retrieve frame usage information.
     * 
     * \since Internal API version 20030317.
     */
    int (*queryFrameTracking)(__DRIdrawable *drawable,
			      int64_t * sbc, int64_t * missedFrames,
			      float * lastMissedUsage, float * usage);
};


/**
 * Used by drivers that implement the GLX_SGI_video_sync extension.
 */
#define __DRI_MEDIA_STREAM_COUNTER "DRI_MediaStreamCounter"
#define __DRI_MEDIA_STREAM_COUNTER_VERSION 2
struct __DRImediaStreamCounterExtensionRec {
    __DRIextension base;

    /**
     * Get the number of vertical refreshes since some point in time before
     * this function was first called (i.e., system start up).
     */
    int (*getMSC)(__DRIscreen *screen, int64_t *msc);

    /**
     * Wait for the MSC to equal target_msc, or, if that has already passed,
     * the next time (MSC % divisor) is equal to remainder.  If divisor is
     * zero, the function will return as soon as MSC is greater than or equal
     * to target_msc.
     */
    int (*waitForMSC)(__DRIdrawable *drawable,
		      int64_t target_msc, int64_t divisor, int64_t remainder,
		      int64_t * msc, int64_t * sbc);

    /**
     * Like the screen version of getMSC, but also takes a drawable so that
     * the appropriate pipe's counter can be retrieved.
     *
     * Get the number of vertical refreshes since some point in time before
     * this function was first called (i.e., system start up).
     *
     * \since Internal API version 2
     */
    int (*getDrawableMSC)(__DRIscreen *screen, void *drawablePrivate,
			  int64_t *msc);
};


#define __DRI_TEX_OFFSET "DRI_TexOffset"
#define __DRI_TEX_OFFSET_VERSION 1
struct __DRItexOffsetExtensionRec {
    __DRIextension base;

    /**
     * Method to override base texture image with a driver specific 'offset'.
     * The depth passed in allows e.g. to ignore the alpha channel of texture
     * images where the non-alpha components don't occupy a whole texel.
     *
     * For GLX_EXT_texture_from_pixmap with AIGLX.
     */
    void (*setTexOffset)(__DRIcontext *pDRICtx, GLint texname,
			 unsigned long long offset, GLint depth, GLuint pitch);
};


/**
 * Macros for building symbol and strings.  Standard CPP two step...
 */

#define __DRI_REAL_STRINGIFY(x) # x
#define __DRI_STRINGIFY(x) __DRI_REAL_STRINGIFY(x)
#define __DRI_REAL_MAKE_VERSION(name, version) name ## _ ## version
#define __DRI_MAKE_VERSION(name, version) __DRI_REAL_MAKE_VERSION(name, version)

#define __DRI_CREATE_NEW_SCREEN \
    __DRI_MAKE_VERSION(__driCreateNewScreen, __DRI_INTERFACE_VERSION)

#define __DRI_CREATE_NEW_SCREEN_STRING \
    __DRI_STRINGIFY(__DRI_CREATE_NEW_SCREEN)

/**
 * \name Functions and data provided by the driver.
 */
/*@{*/

#define __DRI_INTERFACE_VERSION 20070105

typedef void *(CREATENEWSCREENFUNC)(int scr, __DRIscreen *psc,
    const __DRIversion * ddx_version, const __DRIversion * dri_version,
    const __DRIversion * drm_version, const __DRIframebuffer * frame_buffer,
    void * pSAREA, int fd, int internal_api_version,
    const __DRIinterfaceMethods * interface,
    __GLcontextModes ** driver_modes);
typedef CREATENEWSCREENFUNC* PFNCREATENEWSCREENFUNC;
extern CREATENEWSCREENFUNC __DRI_CREATE_NEW_SCREEN;



/**
 * XML document describing the configuration options supported by the
 * driver.
 */
extern const char __driConfigOptions[];

/*@}*/


/**
 * Stored version of some component (i.e., server-side DRI module, kernel-side
 * DRM, etc.).
 * 
 * \todo
 * There are several data structures that explicitly store a major version,
 * minor version, and patch level.  These structures should be modified to
 * have a \c __DRIversionRec instead.
 */
struct __DRIversionRec {
    int    major;        /**< Major version number. */
    int    minor;        /**< Minor version number. */
    int    patch;        /**< Patch-level. */
};


typedef void (*__DRIfuncPtr)(void);

struct __DRIinterfaceMethodsRec {
    /**
     * Create a list of \c __GLcontextModes structures.
     */
    __GLcontextModes * (*createContextModes)(unsigned count,
        size_t minimum_bytes_per_struct);

    /**
     * Destroy a list of \c __GLcontextModes structures.
     *
     * \todo
     * Determine if the drivers actually need to call this.
     */
    void (*destroyContextModes)( __GLcontextModes * modes );


    /**
     * \name Client/server protocol functions.
     *
     * These functions implement the DRI client/server protocol for
     * context and drawable operations.  Platforms that do not implement
     * the wire protocol (e.g., EGL) will implement glorified no-op functions.
     */
    /*@{*/

    /**
     * This function is used to get information about the position, size, and
     * clip rects of a drawable.
     */
    GLboolean (* getDrawableInfo) ( __DRIdrawable *drawable,
	unsigned int * index, unsigned int * stamp,
        int * x, int * y, int * width, int * height,
        int * numClipRects, drm_clip_rect_t ** pClipRects,
        int * backX, int * backY,
        int * numBackClipRects, drm_clip_rect_t ** pBackClipRects );
    /*@}*/


    /**
     * \name Timing related functions.
     */
    /*@{*/
    /**
     * Get the 64-bit unadjusted system time (UST).
     */
    int (*getUST)(int64_t * ust);

    /**
     * Get the media stream counter (MSC) rate.
     * 
     * Matching the definition in GLX_OML_sync_control, this function returns
     * the rate of the "media stream counter".  In practical terms, this is
     * the frame refresh rate of the display.
     */
    GLboolean (*getMSCRate)(__DRIdrawable *draw,
			    int32_t * numerator, int32_t * denominator);
    /*@}*/

    /**
     * Reports areas of the given drawable which have been modified by the
     * driver.
     *
     * \param drawable which the drawing was done to.
     * \param rects rectangles affected, with the drawable origin as the
     *	      origin.
     * \param x X offset of the drawable within the screen (used in the
     *	      front_buffer case)
     * \param y Y offset of the drawable within the screen.
     * \param front_buffer boolean flag for whether the drawing to the
     * 	      drawable was actually done directly to the front buffer (instead
     *	      of backing storage, for example)
     */
    void (*reportDamage)(__DRIdrawable *draw,
			 int x, int y,
			 drm_clip_rect_t *rects, int num_rects,
			 GLboolean front_buffer);
};

   
/**
 * Framebuffer information record.  Used by libGL to communicate information
 * about the framebuffer to the driver's \c __driCreateNewScreen function.
 * 
 * In XFree86, most of this information is derrived from data returned by
 * calling \c XF86DRIGetDeviceInfo.
 *
 * \sa XF86DRIGetDeviceInfo __DRIdisplayRec::createNewScreen
 *     __driUtilCreateNewScreen CallCreateNewScreen
 *
 * \bug This structure could be better named.
 */
struct __DRIframebufferRec {
    unsigned char *base;    /**< Framebuffer base address in the CPU's
			     * address space.  This value is calculated by
			     * calling \c drmMap on the framebuffer handle
			     * returned by \c XF86DRIGetDeviceInfo (or a
			     * similar function).
			     */
    int size;               /**< Framebuffer size, in bytes. */
    int stride;             /**< Number of bytes from one line to the next. */
    int width;              /**< Pixel width of the framebuffer. */
    int height;             /**< Pixel height of the framebuffer. */
    int dev_priv_size;      /**< Size of the driver's dev-priv structure. */
    void *dev_priv;         /**< Pointer to the driver's dev-priv structure. */
};


/**
 * Screen dependent methods.  This structure is initialized during the
 * \c __DRIdisplayRec::createScreen call.
 */
struct __DRIscreenRec {
    /**
     * Method to destroy the private DRI screen data.
     */
    void (*destroyScreen)(__DRIscreen *screen);

    /**
     * Method to get screen extensions.
     */
    const __DRIextension **(*getExtensions)(__DRIscreen *screen);

    /**
     * Method to create the private DRI drawable data and initialize the
     * drawable dependent methods.
     */
    void *(*createNewDrawable)(__DRIscreen *screen,
			       const __GLcontextModes *modes,
			       __DRIdrawable *pdraw,
			       drm_drawable_t hwDrawable,
			       int renderType, const int *attrs);

    /**
     * Opaque pointer to private per screen direct rendering data.  \c NULL
     * if direct rendering is not supported on this screen.  Never
     * dereferenced in libGL.
     */
    void *private;

    /**
     * Method to create the private DRI context data and initialize the
     * context dependent methods.
     *
     * \since Internal API version 20031201.
     */
    void * (*createNewContext)(__DRIscreen *screen,
			       const __GLcontextModes *modes,
			       int render_type,
			       __DRIcontext *shared,
			       drm_context_t hwContext, __DRIcontext *pctx);
};

/**
 * Context dependent methods.  This structure is initialized during the
 * \c __DRIscreenRec::createContext call.
 */
struct __DRIcontextRec {
    /**
     * Method to destroy the private DRI context data.
     */
    void (*destroyContext)(__DRIcontext *context);

    /**
     * Opaque pointer to private per context direct rendering data.
     * \c NULL if direct rendering is not supported on the display or
     * screen used to create this context.  Never dereferenced in libGL.
     */
    void *private;

    /**
     * Method to bind a DRI drawable to a DRI graphics context.
     *
     * \since Internal API version 20050727.
     */
    GLboolean (*bindContext)(__DRIcontext *ctx,
			     __DRIdrawable *pdraw,
			     __DRIdrawable *pread);

    /**
     * Method to unbind a DRI drawable from a DRI graphics context.
     *
     * \since Internal API version 20050727.
     */
    GLboolean (*unbindContext)(__DRIcontext *ctx);
};

/**
 * Drawable dependent methods.  This structure is initialized during the
 * \c __DRIscreenRec::createDrawable call.  \c createDrawable is not called
 * by libGL at this time.  It's currently used via the dri_util.c utility code
 * instead.
 */
struct __DRIdrawableRec {
    /**
     * Method to destroy the private DRI drawable data.
     */
    void (*destroyDrawable)(__DRIdrawable *drawable);

    /**
     * Method to swap the front and back buffers.
     */
    void (*swapBuffers)(__DRIdrawable *drawable);

    /**
     * Opaque pointer to private per drawable direct rendering data.
     * \c NULL if direct rendering is not supported on the display or
     * screen used to create this drawable.  Never dereferenced in libGL.
     */
    void *private;
};

#endif
