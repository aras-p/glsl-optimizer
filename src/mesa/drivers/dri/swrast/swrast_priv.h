/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/*
 * Authors:
 *    George Sapountzis <gsap7@yahoo.gr>
 */


#ifndef _SWRAST_PRIV_H
#define _SWRAST_PRIV_H

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include "mtypes.h"


/**
 * Debugging
 */
#define DEBUG_CORE	0
#define DEBUG_SPAN	0

#if DEBUG_CORE
#define TRACE _mesa_printf("--> %s\n", __FUNCTION__)
#else
#define TRACE
#endif

#if DEBUG_SPAN
#define TRACE_SPAN _mesa_printf("--> %s\n", __FUNCTION__)
#else
#define TRACE_SPAN
#endif


/**
 * Data types
 */
struct __DRIscreenRec {
    int num;

    const __DRIextension **extensions;

    const __DRIswrastLoaderExtension *swrast_loader;
};

struct __DRIcontextRec {
    GLcontext Base;

    void *loaderPrivate;

    __DRIscreen *driScreenPriv;
};

struct __DRIdrawableRec {
    GLframebuffer Base;

    void *loaderPrivate;

    __DRIscreen *driScreenPriv;

    /* scratch row for optimized front-buffer rendering */
    char *row;
};

struct swrast_renderbuffer {
    struct gl_renderbuffer Base;

    /* renderbuffer pitch (in bytes) */
    GLuint pitch;
};

static INLINE __DRIcontext *
swrast_context(GLcontext *ctx)
{
    return (__DRIcontext *) ctx;
}

static INLINE __DRIdrawable *
swrast_drawable(GLframebuffer *fb)
{
    return (__DRIdrawable *) fb;
}

static INLINE struct swrast_renderbuffer *
swrast_renderbuffer(struct gl_renderbuffer *rb)
{
    return (struct swrast_renderbuffer *) rb;
}


/**
 * Pixel formats we support
 */
#define PF_CI8        1		/**< Color Index mode */
#define PF_A8R8G8B8   2		/**< 32-bit TrueColor:  8-A, 8-R, 8-G, 8-B bits */
#define PF_R5G6B5     3		/**< 16-bit TrueColor:  5-R, 6-G, 5-B bits */
#define PF_R3G3B2     4		/**<  8-bit TrueColor:  3-R, 3-G, 2-B bits */


/**
 * Renderbuffer pitch alignment (in bits).
 *
 * The xorg loader requires padding images to 32 bits. However, this should
 * become a screen/drawable parameter XXX
 */
#define PITCH_ALIGN_BITS 32


/* swrast_span.c */

extern void
swrast_set_span_funcs_back(struct swrast_renderbuffer *xrb,
			   GLuint pixel_format);

extern void
swrast_set_span_funcs_front(struct swrast_renderbuffer *xrb,
			    GLuint pixel_format);

#endif /* _SWRAST_PRIV_H_ */
