/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 *   Brian Paul
 */




#undef ASSERT
#undef Elements

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "state_tracker/xlib_sw_winsys.h"

#include "xlib.h"

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

/**
 * Subclass of pipe_buffer for Xlib winsys.
 * Low-level OS/window system memory buffer
 */
struct xm_displaytarget
{
   enum pipe_format format;
   unsigned width;
   unsigned height;
   unsigned stride;

   void *data;
   void *mapped;

   Display *display;
   Visual *visual;
   XImage *tempImage;

   XShmSegmentInfo shminfo;
   int shm;
};


/**
 * Subclass of sw_winsys for Xlib winsys
 */
struct xlib_sw_winsys
{
   struct sw_winsys base;



   Display *display;
};



/** Cast wrapper */
static INLINE struct xm_displaytarget *
xm_displaytarget( struct sw_displaytarget *dt )
{
   return (struct xm_displaytarget *)dt;
}


/**
 * X Shared Memory Image extension code
 */

#ifdef USE_XSHM

static volatile int mesaXErrorFlag = 0;

/**
 * Catches potential Xlib errors.
 */
static int
mesaHandleXError(Display *dpy, XErrorEvent *event)
{
   (void) dpy;
   (void) event;
   mesaXErrorFlag = 1;
   return 0;
}


static char *alloc_shm(struct xm_displaytarget *buf, unsigned size)
{
   XShmSegmentInfo *const shminfo = & buf->shminfo;

   shminfo->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT|0777);
   if (shminfo->shmid < 0) {
      return NULL;
   }

   shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
   if (shminfo->shmaddr == (char *) -1) {
      shmctl(shminfo->shmid, IPC_RMID, 0);
      return NULL;
   }

   shminfo->readOnly = False;
   return shminfo->shmaddr;
}


/**
 * Allocate a shared memory XImage back buffer for the given XMesaBuffer.
 */
static void
alloc_shm_ximage(struct xm_displaytarget *xm_dt,
                 struct xlib_drawable *xmb,
                 unsigned width, unsigned height)
{
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
   int (*old_handler)(Display *, XErrorEvent *);

   xm_dt->tempImage = XShmCreateImage(xm_dt->display,
                                      xmb->visual,
                                      xmb->depth,
                                      ZPixmap,
                                      NULL,
                                      &xm_dt->shminfo,
                                      width, height);
   if (xm_dt->tempImage == NULL) {
      xm_dt->shm = 0;
      return;
   }


   mesaXErrorFlag = 0;
   old_handler = XSetErrorHandler(mesaHandleXError);
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach(xm_dt->display, &xm_dt->shminfo);
   XSync(xm_dt->display, False);

   if (mesaXErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
      XFlush(xm_dt->display);
      mesaXErrorFlag = 0;
      XDestroyImage(xm_dt->tempImage);
      xm_dt->tempImage = NULL;
      xm_dt->shm = 0;
      (void) XSetErrorHandler(old_handler);
      return;
   }

   xm_dt->shm = 1;
}

#endif /* USE_XSHM */

static boolean
xm_is_displaytarget_format_supported( struct sw_winsys *ws,
                                      enum pipe_format format )
{
   /* TODO: check visuals or other sensible thing here */
   return TRUE;
}


static void *
xm_displaytarget_map(struct sw_winsys *ws,
                     struct sw_displaytarget *dt,
                     unsigned flags)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = xm_dt->data;
   return xm_dt->mapped;
}

static void
xm_displaytarget_unmap(struct sw_winsys *ws,
                       struct sw_displaytarget *dt)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = NULL;
}

static void
xm_displaytarget_destroy(struct sw_winsys *ws,
                         struct sw_displaytarget *dt)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);

   if (xm_dt->data) {
#ifdef USE_XSHM
      if (xm_dt->shminfo.shmid >= 0) {
         shmdt(xm_dt->shminfo.shmaddr);
         shmctl(xm_dt->shminfo.shmid, IPC_RMID, 0);
         
         xm_dt->shminfo.shmid = -1;
         xm_dt->shminfo.shmaddr = (char *) -1;
      }
      else
#endif
         FREE(xm_dt->data);
   }

   FREE(xm_dt);
}


/**
 * Display/copy the image in the surface into the X window specified
 * by the XMesaBuffer.
 */
void
xlib_sw_display(struct xlib_drawable *xlib_drawable,
                struct sw_displaytarget *dt)
{
   XImage *ximage;
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   static boolean no_swap = 0;
   static boolean firsttime = 1;

   if (firsttime) {
      no_swap = getenv("SP_NO_RAST") != NULL;
      firsttime = 0;
   }

   if (no_swap)
      return;

#ifdef USE_XSHM
   if (xm_dt->shm)
   {
      if (xm_dt->tempImage == NULL)
      {
         assert(util_format_get_blockwidth(xm_dt->format) == 1);
         assert(util_format_get_blockheight(xm_dt->format) == 1);
         alloc_shm_ximage(xm_dt,
                          xlib_drawable,
                          xm_dt->stride / util_format_get_blocksize(xm_dt->format),
                          xm_dt->height);
      }

      ximage = xm_dt->tempImage;
      ximage->data = xm_dt->data;

      /* _debug_printf("XSHM\n"); */
      XShmPutImage(xm_dt->display, xlib_drawable->drawable, xlib_drawable->gc,
                   ximage, 0, 0, 0, 0, xm_dt->width, xm_dt->height, False);
   }
   else
#endif
   {
      /* display image in Window */
      ximage = xm_dt->tempImage;
      ximage->data = xm_dt->data;

      /* check that the XImage has been previously initialized */
      assert(ximage->format);
      assert(ximage->bitmap_unit);

      /* update XImage's fields */
      ximage->width = xm_dt->width;
      ximage->height = xm_dt->height;
      ximage->bytes_per_line = xm_dt->stride;

      /* _debug_printf("XPUT\n"); */
      XPutImage(xm_dt->display, xlib_drawable->drawable, xlib_drawable->gc,
                ximage, 0, 0, 0, 0, xm_dt->width, xm_dt->height);
   }
}

/**
 * Display/copy the image in the surface into the X window specified
 * by the XMesaBuffer.
 */
static void
xm_displaytarget_display(struct sw_winsys *ws,
                         struct sw_displaytarget *dt,
                         void *context_private)
{
   struct xlib_drawable *xlib_drawable = (struct xlib_drawable *)context_private;
   xlib_sw_display(xlib_drawable, dt);
}


static struct sw_displaytarget *
xm_displaytarget_create(struct sw_winsys *winsys,
                        enum pipe_format format,
                        unsigned width, unsigned height,
                        unsigned alignment,
                        unsigned *stride)
{
   struct xm_displaytarget *xm_dt = CALLOC_STRUCT(xm_displaytarget);
   unsigned nblocksy, size;

   xm_dt = CALLOC_STRUCT(xm_displaytarget);
   if(!xm_dt)
      goto no_xm_dt;

   xm_dt->display = ((struct xlib_sw_winsys *)winsys)->display;
   xm_dt->format = format;
   xm_dt->width = width;
   xm_dt->height = height;

   nblocksy = util_format_get_nblocksy(format, height);
   xm_dt->stride = align(util_format_get_stride(format, width), alignment);
   size = xm_dt->stride * nblocksy;

#ifdef USE_XSHM
   if (!debug_get_bool_option("XLIB_NO_SHM", FALSE))
   {
      xm_dt->shminfo.shmid = -1;
      xm_dt->shminfo.shmaddr = (char *) -1;
      xm_dt->shm = TRUE;
         
      xm_dt->data = alloc_shm(xm_dt, size);
      if(!xm_dt->data)
         goto no_data;
   }
#endif

   if(!xm_dt->data) {
      xm_dt->data = align_malloc(size, alignment);
      if(!xm_dt->data)
         goto no_data;
   }

   *stride = xm_dt->stride;
   return (struct sw_displaytarget *)xm_dt;

no_data:
   FREE(xm_dt);
no_xm_dt:
   return NULL;
}


static void
xm_destroy( struct sw_winsys *ws )
{
   FREE(ws);
}


struct sw_winsys *
xlib_create_sw_winsys( Display *display )
{
   struct xlib_sw_winsys *ws;

   ws = CALLOC_STRUCT(xlib_sw_winsys);
   if (!ws)
      return NULL;

   ws->display = display;
   ws->base.destroy = xm_destroy;

   ws->base.is_displaytarget_format_supported = xm_is_displaytarget_format_supported;

   ws->base.displaytarget_create = xm_displaytarget_create;
   ws->base.displaytarget_map = xm_displaytarget_map;
   ws->base.displaytarget_unmap = xm_displaytarget_unmap;
   ws->base.displaytarget_destroy = xm_displaytarget_destroy;

   ws->base.displaytarget_display = xm_displaytarget_display;

   return &ws->base;
}

