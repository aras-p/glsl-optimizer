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


#if defined(GALLIUM_LLVMPIPE)

#include "xm_api.h"

#undef ASSERT
#undef Elements

#include "util/u_simple_screen.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "llvmpipe/lp_winsys.h"
#include "llvmpipe/lp_texture.h"

#include "xlib.h"

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

   XImage *tempImage;
#ifdef USE_XSHM
   int shm;
   XShmSegmentInfo shminfo;
#endif
};


/**
 * Subclass of llvmpipe_winsys for Xlib winsys
 */
struct xmesa_llvmpipe_winsys
{
   struct llvmpipe_winsys base;
/*   struct xmesa_visual *xm_visual; */
};



/** Cast wrapper */
static INLINE struct xm_displaytarget *
xm_displaytarget( struct llvmpipe_displaytarget *dt )
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
alloc_shm_ximage(struct xm_displaytarget *xm_buffer,
                 struct xmesa_buffer *xmb,
                 unsigned width, unsigned height)
{
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
   int (*old_handler)(Display *, XErrorEvent *);

   xm_buffer->tempImage = XShmCreateImage(xmb->xm_visual->display,
                                  xmb->xm_visual->visinfo->visual,
                                  xmb->xm_visual->visinfo->depth,
                                  ZPixmap,
                                  NULL,
                                  &xm_buffer->shminfo,
                                  width, height);
   if (xm_buffer->tempImage == NULL) {
      xm_buffer->shm = 0;
      return;
   }


   mesaXErrorFlag = 0;
   old_handler = XSetErrorHandler(mesaHandleXError);
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach(xmb->xm_visual->display, &xm_buffer->shminfo);
   XSync(xmb->xm_visual->display, False);

   if (mesaXErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
      XFlush(xmb->xm_visual->display);
      mesaXErrorFlag = 0;
      XDestroyImage(xm_buffer->tempImage);
      xm_buffer->tempImage = NULL;
      xm_buffer->shm = 0;
      (void) XSetErrorHandler(old_handler);
      return;
   }

   xm_buffer->shm = 1;
}

#endif /* USE_XSHM */

static boolean
xm_is_displaytarget_format_supported( struct llvmpipe_winsys *ws,
                                      enum pipe_format format )
{
   /* TODO: check visuals or other sensible thing here */
   return TRUE;
}


static void *
xm_displaytarget_map(struct llvmpipe_winsys *ws,
                     struct llvmpipe_displaytarget *dt,
                     unsigned flags)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = xm_dt->data;
   return xm_dt->mapped;
}

static void
xm_displaytarget_unmap(struct llvmpipe_winsys *ws,
                       struct llvmpipe_displaytarget *dt)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = NULL;
}

static void
xm_displaytarget_destroy(struct llvmpipe_winsys *ws,
                         struct llvmpipe_displaytarget *dt)
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
static void
xm_llvmpipe_display(struct xmesa_buffer *xm_buffer,
                    struct llvmpipe_displaytarget *dt)
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
         alloc_shm_ximage(xm_dt, xm_buffer,
                          xm_dt->stride / util_format_get_blocksize(xm_dt->format),
                          xm_dt->height);
      }

      ximage = xm_dt->tempImage;
      ximage->data = xm_dt->data;

      /* _debug_printf("XSHM\n"); */
      XShmPutImage(xm_buffer->xm_visual->display, xm_buffer->drawable, xm_buffer->gc,
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
      XPutImage(xm_buffer->xm_visual->display, xm_buffer->drawable, xm_buffer->gc,
                ximage, 0, 0, 0, 0, xm_dt->width, xm_dt->height);
   }
}

/**
 * Display/copy the image in the surface into the X window specified
 * by the XMesaBuffer.
 */
static void
xm_displaytarget_display(struct llvmpipe_winsys *ws,
                         struct llvmpipe_displaytarget *dt,
                         void *context_private)
{
   XMesaContext xmctx = (XMesaContext) context_private;
   struct xmesa_buffer *xm_buffer = xmctx->xm_buffer;
   xm_llvmpipe_display(xm_buffer, dt);
}


static struct llvmpipe_displaytarget *
xm_displaytarget_create(struct llvmpipe_winsys *winsys,
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
   return (struct llvmpipe_displaytarget *)xm_dt;

no_data:
   FREE(xm_dt);
no_xm_dt:
   return NULL;
}


static void
xm_destroy( struct llvmpipe_winsys *ws )
{
   FREE(ws);
}


static struct llvmpipe_winsys *
xlib_create_llvmpipe_winsys( void )
{
   struct xmesa_llvmpipe_winsys *ws;

   ws = CALLOC_STRUCT(xmesa_llvmpipe_winsys);
   if (!ws)
      return NULL;

   ws->base.destroy = xm_destroy;

   ws->base.is_displaytarget_format_supported = xm_is_displaytarget_format_supported;

   ws->base.displaytarget_create = xm_displaytarget_create;
   ws->base.displaytarget_map = xm_displaytarget_map;
   ws->base.displaytarget_unmap = xm_displaytarget_unmap;
   ws->base.displaytarget_destroy = xm_displaytarget_destroy;

   ws->base.displaytarget_display = xm_displaytarget_display;

   return &ws->base;
}


static struct pipe_screen *
xlib_create_llvmpipe_screen( void )
{
   struct llvmpipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_llvmpipe_winsys();
   if (winsys == NULL)
      return NULL;

   screen = llvmpipe_create_screen(winsys);
   if (screen == NULL)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}


static void
xlib_llvmpipe_display_surface(struct xmesa_buffer *xm_buffer,
                              struct pipe_surface *surf)
{
   struct llvmpipe_texture *texture = llvmpipe_texture(surf->texture);

   assert(texture->dt);
   if (texture->dt)
      xm_llvmpipe_display(xm_buffer, texture->dt);
}


struct xm_driver xlib_llvmpipe_driver = 
{
   .create_pipe_screen = xlib_create_llvmpipe_screen,
   .display_surface = xlib_llvmpipe_display_surface
};



#endif /* GALLIUM_LLVMPIPE */
