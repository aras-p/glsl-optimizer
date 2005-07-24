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

#ifndef DRI_NEW_INTERFACE_ONLY
# include <X11/X.h>
# include <GL/glx.h>
# include "GL/glxint.h"
#endif

#include <GL/internal/glcore.h>
#include <xf86drm.h>
#include <drm.h>

/**
 * \name DRI interface structures
 *
 * The following structures define the interface between the GLX client
 * side library and the DRI (direct rendering infrastructure).
 */
/*@{*/
typedef struct __DRIdisplayRec  __DRIdisplay;
typedef struct __DRIscreenRec   __DRIscreen;
typedef struct __DRIcontextRec  __DRIcontext;
typedef struct __DRIdrawableRec __DRIdrawable;
typedef struct __DRIdriverRec   __DRIdriver;
typedef struct __DRIframebufferRec __DRIframebuffer;
typedef struct __DRIversionRec     __DRIversion;
typedef unsigned long __DRIid;
typedef void __DRInativeDisplay;
/*@}*/


/**
 * \name Functions provided by the driver loader.
 */
/*@{*/
extern __DRIscreen *__glXFindDRIScreen(__DRInativeDisplay *dpy, int scrn);


/**
 * Type of a pointer to \c __glXGetInternalVersion, as returned by
 * \c glXGetProcAddress.
 *
 * \sa __glXGetInternalVersion, glXGetProcAddress
 */
typedef int (* PFNGLXGETINTERNALVERSIONPROC) ( void );

/**
 * Type of a pointer to \c __glXWindowExists, as returned by
 * \c glXGetProcAddress.
 *
 * \sa __glXWindowExists, glXGetProcAddress
 */
typedef GLboolean (* PFNGLXWINDOWEXISTSPROC) (__DRInativeDisplay *dpy, __DRIid draw);

/**
 * Type of a pointer to \c __glXGetUST, as returned by \c glXGetProcAddress.
 * 
 * \sa __glXGetUST, glXGetProcAddress
 */
typedef int (* PFNGLXGETUSTPROC) ( int64_t * ust );

/**
 * Type of pointer to \c __glXCreateContextModes, as returned by
 * \c glXGetProcAddress.
 * 
 * \sa _gl_context_modes_create, glXGetProcAddress
 */

typedef __GLcontextModes * (* PFNGLXCREATECONTEXTMODES) ( unsigned count,
    size_t minimum_bytes_per_struct );

/**
 * Type of a pointer to \c glXGetScreenDriver, as returned by
 * \c glXGetProcAddress.  This function is used to get the name of the DRI
 * driver for the specified screen of the specified display.  The driver
 * name is typically used with \c glXGetDriverConfig.
 *
 * \sa glXGetScreenDriver, glXGetProcAddress, glXGetDriverConfig
 */
typedef const char * (* PFNGLXGETSCREENDRIVERPROC) (__DRInativeDisplay *dpy, int scrNum);

/**
 * Type of a pointer to \c glXGetDriverConfig, as returned by
 * \c glXGetProcAddress.  This function is used to get the XML document
 * describing the configuration options available for the specified driver.
 *
 * \sa glXGetDriverConfig, glXGetProcAddress, glXGetScreenDriver
 */
typedef const char * (* PFNGLXGETDRIVERCONFIGPROC) (const char *driverName);

/**
 * Type of a pointer to \c __glXScrEnableExtension, as returned by
 * \c glXGetProcAddress.  This function is used to enable a GLX extension
 * on the specified screen.
 *
 * \sa __glXScrEnableExtension, glXGetProcAddress
 */
typedef void (* PFNGLXSCRENABLEEXTENSIONPROC) ( void *psc, const char * name );

/**
 * Type of a pointer to \c __glXGetDrawableInfo, as returned by
 * \c glXGetProcAddress.  This function is used to get information about the
 * position, size, and clip rects of a drawable.
 * 
 * \sa __glXGetDrawableInfo, glXGetProcAddress
 */
typedef GLboolean (* PFNGLXGETDRAWABLEINFOPROC) ( __DRInativeDisplay *dpy, int scrn,
    __DRIid draw, unsigned int * index, unsigned int * stamp,
    int * x, int * y, int * width, int * height,
    int * numClipRects, drm_clip_rect_t ** pClipRects,
    int * backX, int * backY,
    int * numBackClipRects, drm_clip_rect_t ** pBackClipRects );

/* Test for the xf86dri.h header file */
#ifndef _XF86DRI_H_
extern GLboolean XF86DRIDestroyContext( __DRInativeDisplay *dpy, int screen,
    __DRIid context_id );

extern GLboolean XF86DRICreateDrawable( __DRInativeDisplay *dpy, int screen,
    __DRIid drawable, drm_drawable_t *hHWDrawable );

extern GLboolean XF86DRIDestroyDrawable( __DRInativeDisplay *dpy, int screen, 
    __DRIid drawable);
#endif
/*@}*/


/**
 * \name Functions and data provided by the driver.
 */
/*@{*/

typedef void *(CREATENEWSCREENFUNC)(__DRInativeDisplay *dpy, int scrn,
    __DRIscreen *psc, const __GLcontextModes * modes,
    const __DRIversion * ddx_version, const __DRIversion * dri_version,
    const __DRIversion * drm_version, const __DRIframebuffer * frame_buffer,
    void * pSAREA, int fd, int internal_api_version,
    __GLcontextModes ** driver_modes);
typedef CREATENEWSCREENFUNC* PFNCREATENEWSCREENFUNC;
extern CREATENEWSCREENFUNC __driCreateNewScreen;

#ifndef DRI_NEW_INTERFACE_ONLY

extern void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
    int numConfigs, __GLXvisualConfig *config);

#endif /* DRI_NEW_INTERFACE_ONLY */


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
    void (*destroyScreen)(__DRInativeDisplay *dpy, int scrn, void *screenPrivate);

    /**
     * Method to create the private DRI drawable data and initialize the
     * drawable dependent methods.
     */
    void *(*createNewDrawable)(__DRInativeDisplay *dpy, const __GLcontextModes *modes,
			       __DRIid draw, __DRIdrawable *pdraw,
			       int renderType, const int *attrs);

    /**
     * Method to return a pointer to the DRI drawable data.
     */
    __DRIdrawable *(*getDrawable)(__DRInativeDisplay *dpy, __DRIid draw,
				  void *drawablePrivate);

    /**
     * Opaque pointer to private per screen direct rendering data.  \c NULL
     * if direct rendering is not supported on this screen.  Never
     * dereferenced in libGL.
     */
    void *private;

    /**
     * Get the number of vertical refreshes since some point in time before
     * this function was first called (i.e., system start up).
     * 
     * \since Internal API version 20030317.
     */
    int (*getMSC)( void *screenPrivate, int64_t *msc );

    /**
     * Opaque pointer that points back to the containing 
     * \c __GLXscreenConfigs.  This data structure is shared with DRI drivers
     * but \c __GLXscreenConfigs is not. However, they are needed by some GLX
     * functions called by DRI drivers.
     *
     * \since Internal API version 20030813.
     */
    void *screenConfigs;

    /**
     * Functions associated with MESA_allocate_memory.
     *
     * \since Internal API version 20030815.
     */
    /*@{*/
    void *(*allocateMemory)(__DRInativeDisplay *dpy, int scrn, GLsizei size,
			    GLfloat readfreq, GLfloat writefreq,
			    GLfloat priority);
   
    void (*freeMemory)(__DRInativeDisplay *dpy, int scrn, GLvoid *pointer);
   
    GLuint (*memoryOffset)(__DRInativeDisplay *dpy, int scrn, const GLvoid *pointer);
    /*@}*/

    /**
     * Method to create the private DRI context data and initialize the
     * context dependent methods.
     *
     * \since Internal API version 20031201.
     */
    void * (*createNewContext)(__DRInativeDisplay *dpy, const __GLcontextModes *modes,
			       int render_type,
			       void *sharedPrivate, __DRIcontext *pctx);
};

/**
 * Context dependent methods.  This structure is initialized during the
 * \c __DRIscreenRec::createContext call.
 */
struct __DRIcontextRec {
    /**
     * Method to destroy the private DRI context data.
     */
    void (*destroyContext)(__DRInativeDisplay *dpy, int scrn, void *contextPrivate);

    /**
     * Opaque pointer to private per context direct rendering data.
     * \c NULL if direct rendering is not supported on the display or
     * screen used to create this context.  Never dereferenced in libGL.
     */
    void *private;

    /**
     * Pointer to the mode used to create this context.
     *
     * \since Internal API version 20040317.
     */
    const __GLcontextModes * mode;

    /**
     * Method to bind a DRI drawable to a DRI graphics context.
     *
     * \since Internal API version 20050722.
     */
    GLboolean (*bindContext)(__DRInativeDisplay *dpy, int scrn, __DRIid draw,
			 __DRIid read, __DRIcontext *ctx);

    /**
     * Method to unbind a DRI drawable from a DRI graphics context.
     *
     * \since Internal API version 20050722.
     */
    GLboolean (*unbindContext)(__DRInativeDisplay *dpy, int scrn, __DRIid draw,
			   __DRIid read, __DRIcontext *ctx);
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
    void (*destroyDrawable)(__DRInativeDisplay *dpy, void *drawablePrivate);

    /**
     * Method to swap the front and back buffers.
     */
    void (*swapBuffers)(__DRInativeDisplay *dpy, void *drawablePrivate);

    /**
     * Opaque pointer to private per drawable direct rendering data.
     * \c NULL if direct rendering is not supported on the display or
     * screen used to create this drawable.  Never dereferenced in libGL.
     */
    void *private;

    /**
     * Get the number of completed swap buffers for this drawable.
     *
     * \since Internal API version 20030317.
     */
    int (*getSBC)(__DRInativeDisplay *dpy, void *drawablePrivate, int64_t *sbc );

    /**
     * Wait for the SBC to be greater than or equal target_sbc.
     *
     * \since Internal API version 20030317.
     */
    int (*waitForSBC)( __DRInativeDisplay * dpy, void *drawablePriv,
		       int64_t target_sbc,
		       int64_t * msc, int64_t * sbc );

    /**
     * Wait for the MSC to equal target_msc, or, if that has already passed,
     * the next time (MSC % divisor) is equal to remainder.  If divisor is
     * zero, the function will return as soon as MSC is greater than or equal
     * to target_msc.
     * 
     * \since Internal API version 20030317.
     */
    int (*waitForMSC)( __DRInativeDisplay * dpy, void *drawablePriv,
		       int64_t target_msc, int64_t divisor, int64_t remainder,
		       int64_t * msc, int64_t * sbc );

    /**
     * Like \c swapBuffers, but does NOT have an implicit \c glFlush.  Once
     * rendering is complete, waits until MSC is equal to target_msc, or
     * if that has already passed, waits until (MSC % divisor) is equal
     * to remainder.  If divisor is zero, the swap will happen as soon as
     * MSC is greater than or equal to target_msc.
     * 
     * \since Internal API version 20030317.
     */
    int64_t (*swapBuffersMSC)(__DRInativeDisplay *dpy, void *drawablePrivate,
			      int64_t target_msc,
			      int64_t divisor, int64_t remainder);

    /**
     * Enable or disable frame usage tracking.
     * 
     * \since Internal API version 20030317.
     */
    int (*frameTracking)(__DRInativeDisplay *dpy, void *drawablePrivate, GLboolean enable);

    /**
     * Retrieve frame usage information.
     * 
     * \since Internal API version 20030317.
     */
    int (*queryFrameTracking)(__DRInativeDisplay *dpy, void *drawablePrivate,
			      int64_t * sbc, int64_t * missedFrames,
			      float * lastMissedUsage, float * usage );

    /**
     * Used by drivers that implement the GLX_SGI_swap_control or
     * GLX_MESA_swap_control extension.
     *
     * \since Internal API version 20030317.
     */
    unsigned swap_interval;
};

#endif
