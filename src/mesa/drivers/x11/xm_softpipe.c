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

#include "pipe/softpipe/sp_winsys.h"


struct xm_softpipe_winsys
{
   struct softpipe_winsys sws;
   XMesaContext xmctx; /* not really needed */
};


struct xm_buffer
{
   int refcount;
   void *data;
   void *mapped;
};



/* Turn the softpipe opaque buffer pointer into a dri_bufmgr opaque
 * buffer pointer...
 */
static inline struct xm_buffer *
xm_bo( struct pipe_buffer_handle *bo )
{
   return (struct xm_buffer *)bo;
}

static inline struct pipe_buffer_handle *
pipe_bo( struct xm_buffer *bo )
{
   return (struct pipe_buffer_handle *)bo;
}

/* Turn a softpipe winsys into an xm/softpipe winsys:
 */
static inline struct xm_softpipe_winsys *
xm_softpipe_winsys( struct softpipe_winsys *sws )
{
   return (struct xm_softpipe_winsys *)sws;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *xm_buffer_map(struct softpipe_winsys *sws, 
                           struct pipe_buffer_handle *buf )
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = xm_buf->data;
   return xm_buf->mapped;
}

static void xm_buffer_unmap(struct softpipe_winsys *sws, 
			       struct pipe_buffer_handle *buf)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->mapped = NULL;
}


static struct pipe_buffer_handle *
xm_buffer_reference(struct softpipe_winsys *sws,
                    struct pipe_buffer_handle *buf)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   xm_buf->refcount++;
   return buf;
}

static void xm_buffer_unreference(struct softpipe_winsys *sws, 
                                  struct pipe_buffer_handle **buf)
{
   struct xm_buffer *xm_buf = xm_bo(*buf);
   xm_buf->refcount--;
   if (xm_buf->refcount == 0) {
      if (xm_buf->data) {
         free(xm_buf->data);
         xm_buf->data = NULL;
      }
      free(xm_buf);
   }
   *buf = NULL;
}


static void xm_buffer_data(struct softpipe_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned size, const void *data )
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   assert(!xm_buf->data);
   xm_buf->data = malloc(size);
}

static void xm_buffer_subdata(struct softpipe_winsys *sws, 
                              struct pipe_buffer_handle *buf,
                              unsigned long offset, 
                              unsigned long size, 
                              const void *data)
{
   struct xm_buffer *xm_buf = xm_bo(buf);
   GLubyte *b = (GLubyte *) xm_buf->data;
   assert(b);
   memcpy(b + offset, data, size);
}

static void xm_buffer_get_subdata(struct softpipe_winsys *sws, 
				     struct pipe_buffer_handle *buf,
				     unsigned long offset, 
				     unsigned long size, 
				     void *data)
{
   const struct xm_buffer *xm_buf = xm_bo(buf);
   const GLubyte *b = (GLubyte *) xm_buf->data;
   assert(b);
   memcpy(data, b + offset, size);
}


/* Softpipe has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 */
static struct pipe_buffer_handle *
xm_create_buffer(struct softpipe_winsys *sws, 
                 unsigned alignment)
{
   struct xm_buffer *buffer;

   buffer = CALLOC_STRUCT(xm_buffer);
   return pipe_bo(buffer);
}


struct pipe_context *
xmesa_create_softpipe(XMesaContext xmesa)
{
   struct xm_softpipe_winsys *isws = CALLOC_STRUCT( xm_softpipe_winsys );
   
   /* Fill in this struct with callbacks that softpipe will need to
    * communicate with the window system, buffer manager, etc. 
    *
    * Softpipe would be happy with a malloc based memory manager, but
    * the SwapBuffers implementation in this winsys driver requires
    * that rendering be done to an appropriate xm_buffer.  
    */
   isws->sws.create_buffer = xm_create_buffer;
   isws->sws.buffer_map = xm_buffer_map;
   isws->sws.buffer_unmap = xm_buffer_unmap;
   isws->sws.buffer_reference = xm_buffer_reference;
   isws->sws.buffer_unreference = xm_buffer_unreference;
   isws->sws.buffer_data = xm_buffer_data;
   isws->sws.buffer_subdata = xm_buffer_subdata;
   isws->sws.buffer_get_subdata = xm_buffer_get_subdata;

   /* Create the softpipe context:
    */
   return softpipe_create( &isws->sws );
}
