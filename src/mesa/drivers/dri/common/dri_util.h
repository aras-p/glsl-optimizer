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

#ifndef _DRI_UTIL_H_
#define _DRI_UTIL_H_

#include <GL/gl.h>
#include <drm.h>
#include <drm_sarea.h>
#include <xf86drm.h>
#include "xmlconfig.h"
#include "main/glheader.h"
#include "main/mtypes.h"
#include "GL/internal/dri_interface.h"

#define GLX_BAD_CONTEXT                    5

/**
 * Extensions.
 */
extern const __DRIcoreExtension driCoreExtension;
extern const __DRIdri2Extension driDRI2Extension;
extern const __DRI2configQueryExtension dri2ConfigQueryExtension;

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
    /**
     * Screen destruction callback
     */
    void (*DestroyScreen)(__DRIscreen *driScrnPriv);

    /**
     * Context creation callback
     */	    	    
    GLboolean (*CreateContext)(gl_api api,
			       const struct gl_config *glVis,
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
                              const struct gl_config *glVis,
                              GLboolean pixmapBuffer);
    
    /**
     * Buffer (drawable) destruction callback
     */
    void (*DestroyBuffer)(__DRIdrawable *driDrawPriv);

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

    /* DRI2 Entry point */
    const __DRIconfig **(*InitScreen2) (__DRIscreen * priv);

    __DRIbuffer *(*AllocateBuffer) (__DRIscreen *screenPrivate,
				    unsigned int attachment,
				    unsigned int format,
				    int width, int height);
    void (*ReleaseBuffer) (__DRIscreen *screenPrivate, __DRIbuffer *buffer);
};

extern const struct __DriverAPIRec driDriverAPI;


/**
 * Per-drawable private DRI driver information.
 */
struct __DRIdrawableRec {
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
     * Last value of the stamp.
     *
     * If this differs from the value stored at __DRIdrawable::dri2.stamp,
     * then the drawable information has been modified by the X server, and the
     * drawable information (below) should be retrieved from the X server.
     */
    unsigned int lastStamp;

    int w, h;

    /**
     * Pointer to context to which this drawable is currently bound.
     */
    __DRIcontext *driContextPriv;

    /**
     * Pointer to screen on which this drawable was created.
     */
    __DRIscreen *driScreenPriv;

    /**
     * Drawable timestamp.  Increased when the loader calls invalidate.
     */
    struct {
	unsigned int stamp;
    } dri2;
};

/**
 * Per-context private driver information.
 */
struct __DRIcontextRec {
    /**
     * Device driver's private context data.  This structure is opaque.
     */
    void *driverPrivate;

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

    /**
     * The loaders's private context data.  This structure is opaque.
     */
    void *loaderPrivate;

    struct {
	int draw_stamp;
	int read_stamp;
    } dri2;
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
     * DRM (kernel module) version information.
     */
    __DRIversion drm_version;

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
     * Device-dependent private information (not stored in the SAREA).
     * 
     * This pointer is never touched by the DRI layer.
     */
#ifdef __cplusplus
    void *priv;
#else
    void *private;
#endif

    struct {
	/* Flag to indicate that this is a DRI2 screen.  Many of the above
	 * fields will not be valid or initializaed in that case. */
	__DRIdri2LoaderExtension *loader;
	__DRIimageLookupExtension *image;
	__DRIuseInvalidateExtension *useInvalidate;
    } dri2;

    driOptionCache optionInfo;
    driOptionCache optionCache;
   unsigned int api_mask;
   void *loaderPrivate;
};

extern void
dri2InvalidateDrawable(__DRIdrawable *drawable);

extern void
driUpdateFramebufferSize(struct gl_context *ctx, const __DRIdrawable *dPriv);

#endif /* _DRI_UTIL_H_ */
