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


#include "glxheader.h"
#include "xmesaP.h"

#undef ASSERT
#undef Elements

#include "pipe/p_winsys.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"

#ifdef GALLIUM_CELL
#include "cell/ppu/cell_context.h"
#include "cell/ppu/cell_screen.h"
#include "cell/ppu/cell_winsys.h"
#else
#define TILE_SIZE 32  /* avoid compilation errors */
#endif

#ifdef GALLIUM_TRACE
#include "trace/tr_screen.h"
#include "trace/tr_context.h"
#endif

#include "xm_winsys_aub.h"


/**
 * Subclass of pipe_buffer for Xlib winsys.
 * Low-level OS/window system memory buffer
 */
struct xm_buffer
{
   struct pipe_buffer base;
   boolean userBuffer;  /** Is this a user-space buffer? */
   void *data;
   void *mapped;
   
   XImage *tempImage;
   int shm;
#if defined(USE_XSHM) && !defined(XFree86Server)
   XShmSegmentInfo shminfo;
#endif
};


/**
 * Subclass of pipe_winsys for Xlib winsys
 */
struct xmesa_pipe_winsys
{
   struct pipe_winsys base;
   struct xmesa_visual *xm_visual;
   int shm;
};



/** Cast wrapper */
static INLINE struct xm_buffer *
xm_buffer( struct pipe_buffer *buf )
{
   return (struct xm_buffer *)buf;
}


/**
 * X Shared Memory Image extension code
 */
#if defined(USE_XSHM) && !defined(XFree86Server)

#define XSHM_ENABLED(b) ((b)->shm)

static volatile int mesaXErrorFlag = 0;

/**
 * Catches potential Xlib errors.
 */
static int
mesaHandleXError(XMesaDisplay *dpy, XErrorEvent *event)
{
   (void) dpy;
   (void) event;
   mesaXErrorFlag = 1;
   return 0;
}


static GLboolean alloc_shm(struct xm_buffer *buf, unsigned size)
{
   XShmSegmentInfo *const shminfo = & buf->shminfo;

   shminfo->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT|0777);
   if (shminfo->shmid < 0) {
      return GL_FALSE;
   }

   shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
   if (shminfo->shmaddr == (char *) -1) {
      shmctl(shminfo->shmid, IPC_RMID, 0);
      return GL_FALSE;
   }

   shminfo->readOnly = False;
   return GL_TRUE;
}


/**
 * Allocate a shared memory XImage back buffer for the given XMesaBuffer.
 */
static void
alloc_shm_ximage(struct xm_buffer *b, struct xmesa_buffer *xmb,
                 unsigned width, unsigned height)
{
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
#if 0
   GC gc;
#endif
   int (*old_handler)(XMesaDisplay *, XErrorEvent *);

   b->tempImage = XShmCreateImage(xmb->xm_visual->display,
                                  xmb->xm_visual->visinfo->visual,
                                  xmb->xm_visual->visinfo->depth,
                                  ZPixmap,
                                  NULL,
                                  &b->shminfo,
                                  width, height);
   if (b->tempImage == NULL) {
      b->shm = 0;
      return;
   }


   mesaXErrorFlag = 0;
   old_handler = XSetErrorHandler(mesaHandleXError);
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach(xmb->xm_visual->display, &b->shminfo);
   XSync(xmb->xm_visual->display, False);

   if (mesaXErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
      XFlush(xmb->xm_visual->display);
      mesaXErrorFlag = 0;
      XDestroyImage(b->tempImage);
      b->tempImage = NULL;
      b->shm = 0;
      (void) XSetErrorHandler(old_handler);
      return;
   }


   /* Finally, try an XShmPutImage to be really sure the extension works */
#if 0
   gc = XCreateGC(xmb->xm_visual->display, xmb->drawable, 0, NULL);
   XShmPutImage(xmb->xm_visual->display, xmb->drawable, gc,
                b->tempImage, 0, 0, 0, 0, 1, 1 /*one pixel*/, False);
   XSync(xmb->xm_visual->display, False);
   XFreeGC(xmb->xm_visual->display, gc);
   (void) XSetErrorHandler(old_handler);
   if (mesaXErrorFlag) {
      XFlush(xmb->xm_visual->display);
      mesaXErrorFlag = 0;
      XDestroyImage(b->tempImage);
      b->tempImage = NULL;
      b->shm = 0;
      return;
   }
#endif
}

#else

#define XSHM_ENABLED(b) 0

static void
alloc_shm_ximage(struct xm_buffer *b, struct xmesa_buffer *xmb,
                 unsigned width, unsigned height)
{
   b->shm = 0;
}
#endif /* USE_XSHM */




/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *
xm_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
              unsigned flags)
{
   struct xm_buffer *xm_buf = xm_buffer(buf);
   xm_buf->mapped = xm_buf->data;
   return xm_buf->mapped;
}

static void
xm_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
   struct xm_buffer *xm_buf = xm_buffer(buf);
   xm_buf->mapped = NULL;
}

static void
xm_buffer_destroy(struct pipe_winsys *pws,
                  struct pipe_buffer *buf)
{
   struct xm_buffer *oldBuf = xm_buffer(buf);

   if (oldBuf->data) {
#if defined(USE_XSHM) && !defined(XFree86Server)
      if (oldBuf->shminfo.shmid >= 0) {
         shmdt(oldBuf->shminfo.shmaddr);
         shmctl(oldBuf->shminfo.shmid, IPC_RMID, 0);
         
         oldBuf->shminfo.shmid = -1;
         oldBuf->shminfo.shmaddr = (char *) -1;
      }
      else
#endif
      {
         if (!oldBuf->userBuffer) {
            align_free(oldBuf->data);
         }
      }

      oldBuf->data = NULL;
   }

   free(oldBuf);
}


/**
 * For Cell.  Basically, rearrange the pixels/quads from this layout:
 *  +--+--+--+--+
 *  |p0|p1|p2|p3|....
 *  +--+--+--+--+
 *
 * to this layout:
 *  +--+--+
 *  |p0|p1|....
 *  +--+--+
 *  |p2|p3|
 *  +--+--+
 */
static void
twiddle_tile(const uint *tileIn, uint *tileOut)
{
   int y, x;

   for (y = 0; y < TILE_SIZE; y+=2) {
      for (x = 0; x < TILE_SIZE; x+=2) {
         int k = 4 * (y/2 * TILE_SIZE/2 + x/2);
         tileOut[y * TILE_SIZE + (x + 0)] = tileIn[k];
         tileOut[y * TILE_SIZE + (x + 1)] = tileIn[k+1];
         tileOut[(y + 1) * TILE_SIZE + (x + 0)] = tileIn[k+2];
         tileOut[(y + 1) * TILE_SIZE + (x + 1)] = tileIn[k+3];
      }
   }
}



/**
 * Display a surface that's in a tiled configuration.  That is, all the
 * pixels for a TILE_SIZExTILE_SIZE block are contiguous in memory.
 */
static void
xmesa_display_surface_tiled(XMesaBuffer b, const struct pipe_surface *surf)
{
   XImage *ximage;
   struct xm_buffer *xm_buf = xm_buffer(surf->buffer);
   const uint tilesPerRow = (surf->width + TILE_SIZE - 1) / TILE_SIZE;
   uint x, y;

   if (XSHM_ENABLED(xm_buf) && (xm_buf->tempImage == NULL)) {
      alloc_shm_ximage(xm_buf, b, TILE_SIZE, TILE_SIZE);
   }

   ximage = (XSHM_ENABLED(xm_buf)) ? xm_buf->tempImage : b->tempImage;

   /* check that the XImage has been previously initialized */
   assert(ximage->format);
   assert(ximage->bitmap_unit);

   if (!XSHM_ENABLED(xm_buf)) {
      /* update XImage's fields */
      ximage->width = TILE_SIZE;
      ximage->height = TILE_SIZE;
      ximage->bytes_per_line = TILE_SIZE * 4;
   }

   for (y = 0; y < surf->height; y += TILE_SIZE) {
      for (x = 0; x < surf->width; x += TILE_SIZE) {
         uint tmpTile[TILE_SIZE * TILE_SIZE];
         int tx = x / TILE_SIZE;
         int ty = y / TILE_SIZE;
         int offset = ty * tilesPerRow + tx;
         int w = TILE_SIZE;
         int h = TILE_SIZE;

         if (y + h > surf->height)
            h = surf->height - y;
         if (x + w > surf->width)
            w = surf->width - x;

         /* offset in pixels */
         offset *= TILE_SIZE * TILE_SIZE;

         if (XSHM_ENABLED(xm_buf)) {
            ximage->data = (char *) xm_buf->data + 4 * offset;
            /* make copy of tile data */
            memcpy(tmpTile, (uint *) ximage->data, sizeof(tmpTile));
            /* twiddle from temp to ximage in shared memory */
            twiddle_tile(tmpTile, (uint *) ximage->data);
            /* display image in shared memory */
#if defined(USE_XSHM) && !defined(XFree86Server)
            XShmPutImage(b->xm_visual->display, b->drawable, b->gc,
                         ximage, 0, 0, x, y, w, h, False);
#endif
         }
         else {
            /* twiddel from ximage buffer to temp tile */
            twiddle_tile((uint *) xm_buf->data + offset, tmpTile);
            /* display temp tile data */
            ximage->data = (char *) tmpTile;
            XPutImage(b->xm_visual->display, b->drawable, b->gc,
                      ximage, 0, 0, x, y, w, h);
         }
      }
   }
}


/**
 * Display/copy the image in the surface into the X window specified
 * by the XMesaBuffer.
 */
void
xmesa_display_surface(XMesaBuffer b, const struct pipe_surface *surf)
{
   XImage *ximage;
   struct xm_buffer *xm_buf = xm_buffer(surf->buffer);
   static boolean no_swap = 0;
   static boolean firsttime = 1;
   static int tileSize = 0;

   if (firsttime) {
      no_swap = getenv("SP_NO_RAST") != NULL;
#ifdef GALLIUM_CELL
      if (!getenv("GALLIUM_NOCELL")) {
         tileSize = 32; /** probably temporary */
      }
#endif
      firsttime = 0;
   }

   if (no_swap)
      return;

   if (tileSize) {
      xmesa_display_surface_tiled(b, surf);
      return;
   }

   if (XSHM_ENABLED(xm_buf) && (xm_buf->tempImage == NULL)) {
      assert(surf->block.width == 1);
      assert(surf->block.height == 1);
      alloc_shm_ximage(xm_buf, b, surf->stride/surf->block.size, surf->height);
   }

   ximage = (XSHM_ENABLED(xm_buf)) ? xm_buf->tempImage : b->tempImage;
   ximage->data = xm_buf->data;

   /* display image in Window */
   if (XSHM_ENABLED(xm_buf)) {
#if defined(USE_XSHM) && !defined(XFree86Server)
      XShmPutImage(b->xm_visual->display, b->drawable, b->gc,
                   ximage, 0, 0, 0, 0, surf->width, surf->height, False);
#endif
   } else {
      /* check that the XImage has been previously initialized */
      assert(ximage->format);
      assert(ximage->bitmap_unit);

      /* update XImage's fields */
      ximage->width = surf->width;
      ximage->height = surf->height;
      ximage->bytes_per_line = surf->stride;

      XPutImage(b->xm_visual->display, b->drawable, b->gc,
                ximage, 0, 0, 0, 0, surf->width, surf->height);
   }
}


static void
xm_flush_frontbuffer(struct pipe_winsys *pws,
                     struct pipe_surface *surf,
                     void *context_private)
{
   /*
    * The front color buffer is actually just another XImage buffer.
    * This function copies that XImage to the actual X Window.
    */
   XMesaContext xmctx = (XMesaContext) context_private;
   xmesa_display_surface(xmctx->xm_buffer, surf);
}



static const char *
xm_get_name(struct pipe_winsys *pws)
{
   return "Xlib";
}


static struct pipe_buffer *
xm_buffer_create(struct pipe_winsys *pws, 
                 unsigned alignment, 
                 unsigned usage,
                 unsigned size)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);
#if defined(USE_XSHM) && !defined(XFree86Server)
   struct xmesa_pipe_winsys *xpws = (struct xmesa_pipe_winsys *) pws;
#endif

   buffer->base.refcount = 1;
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;


#if defined(USE_XSHM) && !defined(XFree86Server)
   buffer->shminfo.shmid = -1;
   buffer->shminfo.shmaddr = (char *) -1;

   if (xpws->shm && (usage & PIPE_BUFFER_USAGE_PIXEL) != 0) {
      buffer->shm = xpws->shm;

      if (alloc_shm(buffer, size)) {
         buffer->data = buffer->shminfo.shmaddr;
      }
   }
#endif

   if (buffer->data == NULL) {
      buffer->shm = 0;

      /* align to 16-byte multiple for Cell */
      buffer->data = align_malloc(size, max(alignment, 16));
   }

   return &buffer->base;
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
xm_user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);
   buffer->base.refcount = 1;
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;
   buffer->shm = 0;

   return &buffer->base;
}



/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}

static int
xm_surface_alloc_storage(struct pipe_winsys *winsys,
                         struct pipe_surface *surf,
                         unsigned width, unsigned height,
                         enum pipe_format format, 
                         unsigned flags,
                         unsigned tex_usage)
{
   const unsigned alignment = 64;

   surf->width = width;
   surf->height = height;
   surf->format = format;
   pf_get_block(format, &surf->block);
   surf->nblocksx = pf_get_nblocksx(&surf->block, width);
   surf->nblocksy = pf_get_nblocksy(&surf->block, height);
   surf->stride = round_up(surf->nblocksx * surf->block.size, alignment);
   surf->usage = flags;

   assert(!surf->buffer);
   surf->buffer = winsys->buffer_create(winsys, alignment,
                                        PIPE_BUFFER_USAGE_PIXEL,
#ifdef GALLIUM_CELL /* XXX a bit of a hack */
                                        surf->stride * round_up(surf->nblocksy, TILE_SIZE));
#else
                                        surf->stride * surf->nblocksy);
#endif

   if(!surf->buffer)
      return -1;
   
   return 0;
}


/**
 * Called via winsys->surface_alloc() to create new surfaces.
 */
static struct pipe_surface *
xm_surface_alloc(struct pipe_winsys *ws)
{
   struct pipe_surface *surface = CALLOC_STRUCT(pipe_surface);

   assert(ws);

   surface->refcount = 1;
   surface->winsys = ws;

   return surface;
}



static void
xm_surface_release(struct pipe_winsys *winsys, struct pipe_surface **s)
{
   struct pipe_surface *surf = *s;
   assert(!surf->texture);
   surf->refcount--;
   if (surf->refcount == 0) {
      if (surf->buffer)
	winsys_buffer_reference(winsys, &surf->buffer, NULL);
      free(surf);
   }
   *s = NULL;
}


/*
 * Fence functions - basically nothing to do, as we don't create any actual
 * fence objects.
 */

static void
xm_fence_reference(struct pipe_winsys *sws, struct pipe_fence_handle **ptr,
                   struct pipe_fence_handle *fence)
{
}


static int
xm_fence_signalled(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
                   unsigned flag)
{
   return 0;
}


static int
xm_fence_finish(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
                unsigned flag)
{
   return 0;
}


/**
 * Return pointer to a pipe_winsys object.
 * For Xlib, this is a singleton object.
 * Nothing special for the Xlib driver so no subclassing or anything.
 */
struct pipe_winsys *
xmesa_get_pipe_winsys_aub(struct xmesa_visual *xm_vis)
{
   static struct xmesa_pipe_winsys *ws = NULL;

   if (!ws) {
      ws = (struct xmesa_pipe_winsys *) xmesa_create_pipe_winsys_aub();
   }
   return &ws->base;
}


static struct pipe_winsys *
xmesa_get_pipe_winsys(struct xmesa_visual *xm_vis)
{
   static struct xmesa_pipe_winsys *ws = NULL;

   if (!ws) {
      ws = CALLOC_STRUCT(xmesa_pipe_winsys);

      ws->xm_visual = xm_vis;
      ws->shm = xmesa_check_for_xshm(xm_vis->display);

      /* Fill in this struct with callbacks that pipe will need to
       * communicate with the window system, buffer manager, etc. 
       */
      ws->base.buffer_create = xm_buffer_create;
      ws->base.user_buffer_create = xm_user_buffer_create;
      ws->base.buffer_map = xm_buffer_map;
      ws->base.buffer_unmap = xm_buffer_unmap;
      ws->base.buffer_destroy = xm_buffer_destroy;

      ws->base.surface_alloc = xm_surface_alloc;
      ws->base.surface_alloc_storage = xm_surface_alloc_storage;
      ws->base.surface_release = xm_surface_release;

      ws->base.fence_reference = xm_fence_reference;
      ws->base.fence_signalled = xm_fence_signalled;
      ws->base.fence_finish = xm_fence_finish;

      ws->base.flush_frontbuffer = xm_flush_frontbuffer;
      ws->base.get_name = xm_get_name;
   }

   return &ws->base;
}


struct pipe_context *
xmesa_create_pipe_context(XMesaContext xmesa, uint pixelformat)
{
   struct pipe_winsys *pws;
   struct pipe_context *pipe;
   
   if (getenv("XM_AUB")) {
      pws = xmesa_get_pipe_winsys_aub(xmesa->xm_visual);
   }
   else {
      pws = xmesa_get_pipe_winsys(xmesa->xm_visual);
   }

#ifdef GALLIUM_CELL
   if (!getenv("GALLIUM_NOCELL")) {
      struct cell_winsys *cws = cell_get_winsys(pixelformat);
      struct pipe_screen *screen = cell_create_screen(pws);

      pipe = cell_create_context(screen, cws);
   }
   else
#endif
   {
      struct pipe_screen *screen = softpipe_create_screen(pws);

      pipe = softpipe_create(screen, pws, NULL);

#ifdef GALLIUM_TRACE
      screen = trace_screen_create(screen);
      
      pipe = trace_context_create(screen, pipe);
#endif
   }

   if (pipe)
      pipe->priv = xmesa;

   return pipe;
}
