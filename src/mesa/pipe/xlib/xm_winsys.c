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
#include "pipe/softpipe/sp_winsys.h"


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
xm_bo( struct pipe_buffer_handle *bo )
{
   return (struct xm_buffer *) bo;
}

static inline struct pipe_buffer_handle *
pipe_bo( struct xm_buffer *bo )
{
   return (struct pipe_buffer_handle *) bo;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *
xm_buffer_map(struct pipe_winsys *pws, struct pipe_buffer_handle *buf,
              unsigned flags)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = xm_buf->data;
   return xm_buf->mapped;
}

static void
xm_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer_handle *buf)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = NULL;
}

static void
xm_buffer_reference(struct pipe_winsys *pws,
                    struct pipe_buffer_handle **ptr,
                    struct pipe_buffer_handle *buf)
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
xm_buffer_data(struct pipe_winsys *pws, struct pipe_buffer_handle *buf,
               unsigned size, const void *data )
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
xm_buffer_subdata(struct pipe_winsys *pws, struct pipe_buffer_handle *buf,
                  unsigned long offset, unsigned long size, const void *data)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   GLubyte *b = (GLubyte *) xm_buf->data;
   assert(!xm_buf->userBuffer);
   assert(b);
   memcpy(b + offset, data, size);
}

static void
xm_buffer_get_subdata(struct pipe_winsys *pws, struct pipe_buffer_handle *buf,
                      unsigned long offset, unsigned long size, void *data)
{
   const struct xm_buffer *xm_buf = xm_bo(buf);
   const GLubyte *b = (GLubyte *) xm_buf->data;
   assert(!xm_buf->userBuffer);
   assert(b);
   memcpy(data, b + offset, size);
}

static void
xm_flush_frontbuffer(struct pipe_winsys *pws,
                     struct pipe_surface *surf )
{
   /* The Xlib driver's front color surfaces are actually X Windows so
    * this flush is a no-op.
    * If we instead did front buffer rendering to a temporary XImage,
    * this would be the place to copy the Ximage to the on-screen Window.
    */
}

static void
xm_wait_idle(struct pipe_winsys *pws)
{
   /* no-op */
}

static void
xm_printf(struct pipe_winsys *pws, const char *fmtString, ...)
{
   va_list args;
   va_start( args, fmtString );  
   vfprintf(stderr, fmtString, args);
   va_end( args );
}

static const char *
xm_get_name(struct pipe_winsys *pws)
{
   return "Xlib";
}


static struct pipe_buffer_handle *
xm_buffer_create(struct pipe_winsys *pws, unsigned alignment)
{
   struct xm_buffer *buffer = CALLOC_STRUCT(xm_buffer);
   buffer->refcount = 1;
   return pipe_bo(buffer);
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer_handle *
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

   region->buffer = winsys->buffer_create( winsys, alignment )
;

   /* NULL data --> just allocate the space */
   winsys->buffer_data( winsys,
                        region->buffer, 
                        region->pitch * cpp * height, 
                        NULL );
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




/**
 * Return pointer to a pipe_winsys object.
 * For Xlib, this is a singleton object.
 * Nothing special for the Xlib driver so no subclassing or anything.
 */
static struct pipe_winsys *
xmesa_get_pipe_winsys(void)
{
   static struct pipe_winsys *ws = NULL;

   if (!ws) {
      ws = CALLOC_STRUCT(pipe_winsys);
   
      /* Fill in this struct with callbacks that pipe will need to
       * communicate with the window system, buffer manager, etc. 
       */
      ws->buffer_create = xm_buffer_create;
      ws->user_buffer_create = xm_user_buffer_create;
      ws->buffer_map = xm_buffer_map;
      ws->buffer_unmap = xm_buffer_unmap;
      ws->buffer_reference = xm_buffer_reference;
      ws->buffer_data = xm_buffer_data;
      ws->buffer_subdata = xm_buffer_subdata;
      ws->buffer_get_subdata = xm_buffer_get_subdata;

      ws->region_alloc = xm_region_alloc;
      ws->region_release = xm_region_release;

      ws->surface_alloc = xm_surface_alloc;

      ws->flush_frontbuffer = xm_flush_frontbuffer;
      ws->wait_idle = xm_wait_idle;
      ws->printf = xm_printf;
      ws->get_name = xm_get_name;
   }

   return ws;
}


/**
 * XXX this depends on the depths supported by the screen (8/16/32/etc).
 * Maybe when we're about to create a context/drawable we create a new
 * softpipe_winsys object that corresponds to the specified screen...
 *
 * Also, this query only really matters for on-screen drawables.
 * For textures and FBOs we (softpipe) can support any format.o
 */
static boolean
xmesa_is_format_supported(struct softpipe_winsys *sws, uint format)
{
   switch (format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
   case PIPE_FORMAT_S_R16_G16_B16_A16:
   case PIPE_FORMAT_S8_Z24:
      return TRUE;
   default:
      return FALSE;
   };
}


/**
 * Return pointer to a softpipe_winsys object.
 * For Xlib, this is a singleton object.
 */
static struct softpipe_winsys *
xmesa_get_softpipe_winsys(void)
{
   static struct softpipe_winsys *spws = NULL;

   if (!spws) {
      spws = CALLOC_STRUCT(softpipe_winsys);
      if (spws) {
         spws->is_format_supported = xmesa_is_format_supported;
      }
   }

   return spws;
}


struct pipe_context *
xmesa_create_softpipe(XMesaContext xmesa)
{
   struct pipe_winsys *pws = xmesa_get_pipe_winsys();
   struct softpipe_winsys *spws = xmesa_get_softpipe_winsys();
   
   return softpipe_create( pws, spws );
}
