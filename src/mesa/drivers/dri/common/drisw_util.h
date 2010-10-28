/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file
 * Binding of the DRI interface (dri_interface.h) for DRISW.
 *
 * The DRISW structs are 'base classes' of the corresponding DRI1 / DRI2 (DRM)
 * structs. The bindings for SW and DRM can be unified by making the DRM structs
 * 'sub-classes' of the SW structs, either proper or with field re-ordering.
 *
 * The code can also be unified but that requires cluttering the common code
 * with ifdef's and guarding with (__DRIscreen::fd >= 0) for DRM.
 */

#ifndef _DRISW_UTIL_H
#define _DRISW_UTIL_H

#include "main/mtypes.h"

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
typedef struct _drmLock drmLock;


/**
 * Extensions
 */
extern const __DRIcoreExtension driCoreExtension;
extern const __DRIswrastExtension driSWRastExtension;


/**
 * Driver callback functions
 */
struct __DriverAPIRec {
    const __DRIconfig **(*InitScreen) (__DRIscreen * priv);

    void (*DestroyScreen)(__DRIscreen *driScrnPriv);

    GLboolean (*CreateContext)(gl_api glapi,
                               const struct gl_config *glVis,
                               __DRIcontext *driContextPriv,
                               void *sharedContextPrivate);

    void (*DestroyContext)(__DRIcontext *driContextPriv);

    GLboolean (*CreateBuffer)(__DRIscreen *driScrnPriv,
                              __DRIdrawable *driDrawPriv,
                              const struct gl_config *glVis,
                              GLboolean pixmapBuffer);

    void (*DestroyBuffer)(__DRIdrawable *driDrawPriv);

    void (*SwapBuffers)(__DRIdrawable *driDrawPriv);

    GLboolean (*MakeCurrent)(__DRIcontext *driContextPriv,
                             __DRIdrawable *driDrawPriv,
                             __DRIdrawable *driReadPriv);

    GLboolean (*UnbindContext)(__DRIcontext *driContextPriv);
};

extern const struct __DriverAPIRec driDriverAPI;


/**
 * Data types
 */
struct __DRIscreenRec {
    int myNum;

    int fd;

    void *private;

    const __DRIextension **extensions;

    const __DRIswrastLoaderExtension *swrast_loader;
};

struct __DRIcontextRec {

    void *driverPrivate;

    void *loaderPrivate;

    __DRIdrawable *driDrawablePriv;

    __DRIdrawable *driReadablePriv;

    __DRIscreen *driScreenPriv;
};

struct __DRIdrawableRec {

    void *driverPrivate;

    void *loaderPrivate;

    __DRIcontext *driContextPriv;

    __DRIscreen *driScreenPriv;

    int refcount;

    /* gallium */
    unsigned int lastStamp;

    int w;
    int h;
};

#endif /* _DRISW_UTIL_H */
