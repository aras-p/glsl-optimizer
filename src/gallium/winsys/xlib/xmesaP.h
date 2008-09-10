/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#ifndef XMESAP_H
#define XMESAP_H


#include "GL/xmesa.h"
#include "mtypes.h"
#ifdef XFree86Server
#include "xm_image.h"
#endif

#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "pipe/p_thread.h"


extern pipe_mutex _xmesa_lock;

extern XMesaBuffer XMesaBufferList;

/*
 */
#define XMESA_SOFTPIPE 1
#define XMESA_AUB      2
extern int xmesa_mode;


/**
 * Visual inforation, derived from GLvisual.
 * Basically corresponds to an XVisualInfo.
 */
struct xmesa_visual {
   GLvisual mesa_visual;	/* Device independent visual parameters */
   XMesaDisplay *display;	/* The X11 display */
#ifdef XFree86Server
   GLint ColormapEntries;
   GLint nplanes;
#else
   XMesaVisualInfo visinfo;	/* X's visual info (pointer to private copy) */
   XVisualInfo *vishandle;	/* Only used in fakeglx.c */
#endif
   GLint BitsPerPixel;		/* True bits per pixel for XImages */

   GLboolean ximage_flag;	/* Use XImage for back buffer (not pixmap)? */
};


/**
 * Context info, derived from st_context.
 * Basically corresponds to a GLXContext.
 */
struct xmesa_context {
   struct st_context *st;
   XMesaVisual xm_visual;	/** pixel format info */
   XMesaBuffer xm_buffer;	/** current drawbuffer */
};


/**
 * Types of X/GLX drawables we might render into.
 */
typedef enum {
   WINDOW,          /* An X window */
   GLXWINDOW,       /* GLX window */
   PIXMAP,          /* GLX pixmap */
   PBUFFER          /* GLX Pbuffer */
} BufferType;


/**
 * Framebuffer information, derived from.
 * Basically corresponds to a GLXDrawable.
 */
struct xmesa_buffer {
   struct st_framebuffer *stfb;

   GLboolean wasCurrent;	/* was ever the current buffer? */
   XMesaVisual xm_visual;	/* the X/Mesa visual */
   XMesaDrawable drawable;	/* Usually the X window ID */
   XMesaColormap cmap;		/* the X colormap */
   BufferType type;             /* window, pixmap, pbuffer or glxwindow */

   XMesaImage *tempImage;
   unsigned long selectedEvents;/* for pbuffers only */

   GLuint shm;			/* X Shared Memory extension status:	*/
				/*    0 = not available			*/
				/*    1 = XImage support available	*/
				/*    2 = Pixmap support available too	*/
#if defined(USE_XSHM) && !defined(XFree86Server)
   XShmSegmentInfo shminfo;
#endif

   XMesaGC gc;			/* scratch GC for span, line, tri drawing */

   /* GLX_EXT_texture_from_pixmap */
   GLint TextureTarget; /** GLX_TEXTURE_1D_EXT, for example */
   GLint TextureFormat; /** GLX_TEXTURE_FORMAT_RGB_EXT, for example */
   GLint TextureMipmap; /** 0 or 1 */

   struct xmesa_buffer *Next;	/* Linked list pointer: */
};



/** cast wrapper */
static INLINE XMesaContext
xmesa_context(GLcontext *ctx)
{
   return (XMesaContext) ctx->DriverCtx;
}


/** cast wrapper */
static INLINE XMesaBuffer
xmesa_buffer(GLframebuffer *fb)
{
   struct st_framebuffer *stfb = (struct st_framebuffer *) fb;
   return (XMesaBuffer) st_framebuffer_private(stfb);
}


extern void
xmesa_delete_framebuffer(struct gl_framebuffer *fb);

extern XMesaBuffer
xmesa_find_buffer(XMesaDisplay *dpy, XMesaColormap cmap, XMesaBuffer notThis);

extern void
xmesa_check_and_update_buffer_size(XMesaContext xmctx, XMesaBuffer drawBuffer);

extern void
xmesa_destroy_buffers_on_display(XMesaDisplay *dpy);

extern struct pipe_context *
xmesa_create_pipe_context(XMesaContext xm, uint pixelformat);

static INLINE GLuint
xmesa_buffer_width(XMesaBuffer b)
{
   return b->stfb->Base.Width;
}

static INLINE GLuint
xmesa_buffer_height(XMesaBuffer b)
{
   return b->stfb->Base.Height;
}

extern void
xmesa_display_surface(XMesaBuffer b, const struct pipe_surface *surf);

extern int
xmesa_check_for_xshm(XMesaDisplay *display);

#endif
