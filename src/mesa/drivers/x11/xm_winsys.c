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
#include "main/macros.h"

#include "pipe/p_winsys.h"
#include "softpipe/sp_winsys.h"


/**
 * XMesa winsys, derived from softpipe winsys.
 * NOTE: there's nothing really X-specific in this winsys layer so
 * we could probably lift it up somewhere.
 */
struct xm_winsys
{
   struct softpipe_winsys sws;
   int foo; /* placeholder */
};


/**
 * Low-level OS/window system memory buffer
 */
struct xm_buffer
{
   boolean userBuffer;  /** Is this a user-space buffer? */
   int refcount;
   unsigned size;
   void *data;
   void *mapped;
};



/* Turn the softpipe opaque buffer pointer into a dri_bufmgr opaque
 * buffer pointer...
 */
static inline struct xm_buffer *
xm_bo( struct pipe_buffer *bo )
{
   return (struct xm_buffer *) bo;
}

static inline struct pipe_buffer *
pipe_bo( struct xm_buffer *bo )
{
   return (struct pipe_buffer *) bo;
}

/* Turn a softpipe winsys into an xm/softpipe winsys:
 */
static inline struct xm_winsys *
xm_winsys(struct softpipe_winsys *sws)
{
   return (struct xm_winsys *) sws;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *
xm_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
              unsigned flags)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = xm_buf->data;
   return xm_buf->mapped;
}

static void
xm_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = NULL;
}

static void
xm_buffer_reference(struct pipe_winsys *pws,
                    struct pipe_buffer **ptr,
                    struct pipe_buffer *buf)
{
   if (*ptr) {
      struct xm_buffer *oldBuf = xm_bo(*ptr);
      oldBuf->refcount--;
      assert(oldBuf->refcount >= 0);
      if (oldBuf->refcount == 0) {
         if (oldBuf->data) {
            if (!oldBuf->userBuffer)
               free(oldBuf->data);
            oldBuf->data = NULL;
         }
         free(oldBuf);
      }
      *ptr = NULL;
   }

   assert(!(*ptr));

   if (buf) {
      struct xm_buffer *newBuf = xm_bo(buf);
      newBuf->refcount++;
      *ptr = buf;
   }
}

static void
xm_buffer_data(struct pipe_winsys *pws, struct pipe_buffer *buf,
               unsigned size, const void *data, unsigned usage)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   assert(!xm_buf->userBuffer);
   if (xm_buf->size != size) {
      if (xm_buf->data)
         free(xm_buf->data);
      xm_buf->data = malloc(size);
      xm_buf->size = size;
   }
   if (data)
      memcpy(xm_buf->data, data, size);
}

static void
xm_buffer_subdata(struct pipe_winsys *pws, struct pipe_buffer *buf,
                  unsigned long offset, unsigned long size, const void *data)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   GLubyte *b = (GLubyte *) xm_buf->data;
   assert(!xm_buf->userBuffer);
   assert(b);
   memcpy(b + offset, data, size);
}

static void
xm_buffer_get_subdata(struct pipe_winsys *pws, struct pipe_buffer *buf,
                      unsigned long offset, unsigned long size, void *data)
{
   const struct xm_buffer *xm_buf = xm_bo(buf);
   const GLubyte *b = (GLubyte *) xm_buf->data;
   assert(!xm_buf->userBuffer);
   assert(b);
   memcpy(data, b + offset, size);
}

static void
xm_flush_frontbuffer(struct pipe_winsys *pws)
{
   /*
   struct intel_context *intel = intel_pipe_winsys(sws)->intel;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   
   intelCopyBuffer(dPriv, NULL);
   */
}

static void
xm_wait_idle(struct pipe_winsys *pws)
{
   /* no-op */
}

static const char *
xm_get_name(struct pipe_winsys *pws)
{
   return "Xlib";
}


static struct pipe_buffer *
xm_buffer_create(struct pipe_winsys *pws, 
                 unsigned alignment, 
                 unsigned flags, 
                 unsigned hint)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);
   buffer->refcount = 1;
   return pipe_bo(buffer);
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
xm_user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);
   buffer->userBuffer = TRUE;
   buffer->refcount = 1;
   buffer->data = ptr;
   buffer->size = bytes;
   return pipe_bo(buffer);
}



/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}


static struct pipe_region *
xm_region_alloc(struct pipe_winsys *winsys,
                unsigned cpp, unsigned width, unsigned height, unsigned flags)
{
   struct pipe_region *region = CALLOC_STRUCT(pipe_region);
   const unsigned alignment = 64;

   region->cpp = cpp;
   region->pitch = round_up(width, alignment / cpp);
   region->height = height;
   region->refcount = 1;

   assert(region->pitch > 0);

   region->buffer = winsys->buffer_create( winsys, alignment, 0, 0 )
;

   /* NULL data --> just allocate the space */
   winsys->buffer_data( winsys,
                        region->buffer, 
                        region->pitch * cpp * height, 
                        NULL,
                        PIPE_BUFFER_USAGE_PIXEL );
   return region;
}


static void
xm_region_release(struct pipe_winsys *winsys, struct pipe_region **region)
{
   if (!*region)
      return;

   assert((*region)->refcount > 0);
   (*region)->refcount--;

   if ((*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);

      winsys->buffer_reference( winsys, &((*region)->buffer), NULL );
      free(*region);
   }
   *region = NULL;
}


/**
 * Called via pipe->surface_alloc() to create new surfaces (textures,
 * renderbuffers, etc.
 */
static struct pipe_surface *
xm_surface_alloc(struct pipe_winsys *ws, GLuint pipeFormat)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);

   assert(ws);
   assert(pipeFormat);

   xms->surface.format = pipeFormat;
   xms->surface.refcount = 1;
#if 0
   /*
    * This is really just a softpipe surface, not an XImage/Pixmap surface.
    */
   softpipe_init_surface_funcs(&xms->surface);
#endif
   return &xms->surface;
}




struct xmesa_pipe_winsys
{
   struct pipe_winsys winsys;
   XMesaContext xmesa;
};

static struct pipe_winsys *
xmesa_create_pipe_winsys( XMesaContext xmesa )
{
   struct xmesa_pipe_winsys *xws = CALLOC_STRUCT(xmesa_pipe_winsys);
   
   /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    *
    * Pipe would be happy with a malloc based memory manager, but
    * the SwapBuffers implementation in this winsys driver requires
    * that rendering be done to an appropriate _DriBufferObject.  
    */
   xws->winsys.buffer_create = xm_buffer_create;
   xws->winsys.user_buffer_create = xm_user_buffer_create;
   xws->winsys.buffer_map = xm_buffer_map;
   xws->winsys.buffer_unmap = xm_buffer_unmap;
   xws->winsys.buffer_reference = xm_buffer_reference;
   xws->winsys.buffer_data = xm_buffer_data;
   xws->winsys.buffer_subdata = xm_buffer_subdata;
   xws->winsys.buffer_get_subdata = xm_buffer_get_subdata;

   xws->winsys.region_alloc = xm_region_alloc;
   xws->winsys.region_release = xm_region_release;

   xws->winsys.surface_alloc = xm_surface_alloc;

   xws->winsys.flush_frontbuffer = xm_flush_frontbuffer;
   xws->winsys.wait_idle = xm_wait_idle;
   xws->winsys.get_name = xm_get_name;
   xws->xmesa = xmesa;

   return &xws->winsys;
}


struct pipe_context *
xmesa_create_softpipe(XMesaContext xmesa)
{
   struct xm_winsys *xm_ws = CALLOC_STRUCT( xm_winsys );
   
   /* Create the softpipe context:
    */
   return softpipe_create( xmesa_create_pipe_winsys(xmesa), &xm_ws->sws );
}
