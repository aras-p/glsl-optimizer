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

#include "xlib.h"

#ifdef GALLIUM_CELL

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

#include "cell/ppu/cell_context.h"
#include "cell/ppu/cell_screen.h"
#include "cell/ppu/cell_winsys.h"
#include "cell/ppu/cell_texture.h"


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
};


/**
 * Subclass of pipe_winsys for Xlib winsys
 */
struct xmesa_pipe_winsys
{
   struct pipe_winsys base;
};



/** Cast wrapper */
static INLINE struct xm_buffer *
xm_buffer( struct pipe_buffer *buf )
{
   return (struct xm_buffer *)buf;
}


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
xm_buffer_destroy(/*struct pipe_winsys *pws,*/
                  struct pipe_buffer *buf)
{
   struct xm_buffer *oldBuf = xm_buffer(buf);

   if (oldBuf) {
      if (oldBuf->data) {
         if (!oldBuf->userBuffer) {
            align_free(oldBuf->data);
         }

         oldBuf->data = NULL;
      }
      free(oldBuf);
   }
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
xlib_cell_display_surface(struct xmesa_buffer *b, struct pipe_surface *surf)
{
   XImage *ximage;
   struct xm_buffer *xm_buf = xm_buffer(
      cell_texture(surf->texture)->buffer);
   const uint tilesPerRow = (surf->width + TILE_SIZE - 1) / TILE_SIZE;
   uint x, y;

   ximage = b->tempImage;

   /* check that the XImage has been previously initialized */
   assert(ximage->format);
   assert(ximage->bitmap_unit);

   /* update XImage's fields */
   ximage->width = TILE_SIZE;
   ximage->height = TILE_SIZE;
   ximage->bytes_per_line = TILE_SIZE * 4;

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

         /* twiddle from ximage buffer to temp tile */
         twiddle_tile((uint *) xm_buf->data + offset, tmpTile);
         /* display temp tile data */
         ximage->data = (char *) tmpTile;
         XPutImage(b->xm_visual->display, b->drawable, b->gc,
                   ximage, 0, 0, x, y, w, h);
      }
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
   if (xmctx)
      xlib_cell_display_surface(xmctx->xm_buffer, surf);
}



static const char *
xm_get_name(struct pipe_winsys *pws)
{
   return "Xlib/Cell";
}


static struct pipe_buffer *
xm_buffer_create(struct pipe_winsys *pws, 
                 unsigned alignment, 
                 unsigned usage,
                 unsigned size)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;


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
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;
   buffer->shm = 0;

   return &buffer->base;
}



static struct pipe_buffer *
xm_surface_buffer_create(struct pipe_winsys *winsys,
                         unsigned width, unsigned height,
                         enum pipe_format format,
                         unsigned usage,
                         unsigned tex_usage,
                         unsigned *stride)
{
   const unsigned alignment = 64;
   unsigned nblocksy;

   nblocksy = util_format_get_nblocksy(format, height);
   *stride = align(util_format_get_stride(format, width), alignment);

   return winsys->buffer_create(winsys, alignment,
                                usage,
                                /* XXX a bit of a hack */
                                *stride * align(nblocksy, TILE_SIZE));
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



static struct pipe_winsys *
xlib_create_cell_winsys( void )
{
   static struct xmesa_pipe_winsys *ws = NULL;

   if (!ws) {
      ws = CALLOC_STRUCT(xmesa_pipe_winsys);

      /* Fill in this struct with callbacks that pipe will need to
       * communicate with the window system, buffer manager, etc. 
       */
      ws->base.buffer_create = xm_buffer_create;
      ws->base.user_buffer_create = xm_user_buffer_create;
      ws->base.buffer_map = xm_buffer_map;
      ws->base.buffer_unmap = xm_buffer_unmap;
      ws->base.buffer_destroy = xm_buffer_destroy;

      ws->base.surface_buffer_create = xm_surface_buffer_create;

      ws->base.fence_reference = xm_fence_reference;
      ws->base.fence_signalled = xm_fence_signalled;
      ws->base.fence_finish = xm_fence_finish;

      ws->base.flush_frontbuffer = xm_flush_frontbuffer;
      ws->base.get_name = xm_get_name;
   }

   return &ws->base;
}


static struct pipe_screen *
xlib_create_cell_screen( void )
{
   struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_cell_winsys();
   if (winsys == NULL)
      return NULL;

   screen = cell_create_screen(winsys);
   if (screen == NULL)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}



struct xm_driver xlib_cell_driver = 
{
   .create_pipe_screen = xlib_create_cell_screen,
   .display_surface = xlib_cell_display_surface,
};

#else

struct xm_driver xlib_cell_driver = 
{
   .create_pipe_screen = NULL,
   .display_surface = NULL,
};

#endif
