/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "svga_cmd.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "os/os_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_resource_buffer.h"
#include "svga_resource_buffer_upload.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/**
 * Vertex and index buffers need hardware backing.  Constant buffers
 * do not.  No other types of buffers currently supported.
 */
static INLINE boolean
svga_buffer_needs_hw_storage(unsigned usage)
{
   return usage & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER);
}


static unsigned int
svga_buffer_is_referenced( struct pipe_context *pipe,
                           struct pipe_resource *buf,
                           unsigned level, int layer)
{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(buf);

   /**
    * XXX: Check this.
    * The screen may cache buffer writes, but when we map, we map out
    * of those cached writes, so we don't need to set a
    * PIPE_REFERENCED_FOR_WRITE flag for cached buffers.
    */

   if (!sbuf->handle || ss->sws->surface_is_flushed(ss->sws, sbuf->handle))
     return PIPE_UNREFERENCED;

   /**
    * sws->surface_is_flushed() does not distinguish between read references
    * and write references. So assume a reference is both,
    * however, we make an exception for index- and vertex buffers, to avoid
    * a flush in st_bufferobj_get_subdata, during display list replay.
    */

   if (sbuf->b.b.bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
      return PIPE_REFERENCED_FOR_READ;

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}






static void *
svga_buffer_map_range( struct pipe_screen *screen,
                       struct pipe_resource *buf,
                       unsigned offset,
		       unsigned length,
                       unsigned usage )
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_buffer *sbuf = svga_buffer( buf );
   void *map;

   if (!sbuf->swbuf && !sbuf->hwbuf) {
      if (svga_buffer_create_hw_storage(ss, sbuf) != PIPE_OK) {
         /*
          * We can't create a hardware buffer big enough, so create a malloc
          * buffer instead.
          */
         debug_printf("%s: failed to allocate %u KB of DMA, splitting DMA transfers\n",
                      __FUNCTION__,
                      (sbuf->b.b.width0 + 1023)/1024);

         sbuf->swbuf = align_malloc(sbuf->b.b.width0, 16);
      }
   }

   if (sbuf->swbuf) {
      /* User/malloc buffer */
      map = sbuf->swbuf;
   }
   else if (sbuf->hwbuf) {
      map = sws->buffer_map(sws, sbuf->hwbuf, usage);
   }
   else {
      map = NULL;
   }

   if(map) {
      ++sbuf->map.count;

      if (usage & PIPE_TRANSFER_WRITE) {
         assert(sbuf->map.count <= 1);
         sbuf->map.writing = TRUE;
         if (usage & PIPE_TRANSFER_FLUSH_EXPLICIT)
            sbuf->map.flush_explicit = TRUE;
      }
   }
   
   return map;
}



static void 
svga_buffer_flush_mapped_range( struct pipe_screen *screen,
                                struct pipe_resource *buf,
                                unsigned offset, unsigned length)
{
   struct svga_buffer *sbuf = svga_buffer( buf );
   struct svga_screen *ss = svga_screen(screen);
   
   pipe_mutex_lock(ss->swc_mutex);
   assert(sbuf->map.writing);
   if(sbuf->map.writing) {
      assert(sbuf->map.flush_explicit);
      svga_buffer_add_range(sbuf, offset, offset + length);
   }
   pipe_mutex_unlock(ss->swc_mutex);
}

static void 
svga_buffer_unmap( struct pipe_screen *screen,
                   struct pipe_resource *buf)
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_buffer *sbuf = svga_buffer( buf );
   
   pipe_mutex_lock(ss->swc_mutex);
   
   assert(sbuf->map.count);
   if(sbuf->map.count)
      --sbuf->map.count;

   if(sbuf->hwbuf)
      sws->buffer_unmap(sws, sbuf->hwbuf);

   if(sbuf->map.writing) {
      if(!sbuf->map.flush_explicit) {
         /* No mapped range was flushed -- flush the whole buffer */
         SVGA_DBG(DEBUG_DMA, "flushing the whole buffer\n");
   
         svga_buffer_add_range(sbuf, 0, sbuf->b.b.width0);
      }
      
      sbuf->map.writing = FALSE;
      sbuf->map.flush_explicit = FALSE;
   }

   pipe_mutex_unlock(ss->swc_mutex);
}



static void
svga_buffer_destroy( struct pipe_screen *screen,
		     struct pipe_resource *buf )
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_buffer *sbuf = svga_buffer( buf );

   assert(!p_atomic_read(&buf->reference.count));
   
   assert(!sbuf->dma.pending);

   if(sbuf->handle)
      svga_buffer_destroy_host_surface(ss, sbuf);
   
   if(sbuf->uploaded.buffer)
      pipe_resource_reference(&sbuf->uploaded.buffer, NULL);

   if(sbuf->hwbuf)
      svga_buffer_destroy_hw_storage(ss, sbuf);
   
   if(sbuf->swbuf && !sbuf->user)
      align_free(sbuf->swbuf);
   
   FREE(sbuf);
}


/* Keep the original code more or less intact, implement transfers in
 * terms of the old functions.
 */
static void *
svga_buffer_transfer_map( struct pipe_context *pipe,
			  struct pipe_transfer *transfer )
{
   uint8_t *map = svga_buffer_map_range( pipe->screen,
					 transfer->resource,
					 transfer->box.x,
					 transfer->box.width,
					 transfer->usage );
   if (map == NULL)
      return NULL;

   /* map_buffer() returned a pointer to the beginning of the buffer,
    * but transfers are expected to return a pointer to just the
    * region specified in the box.
    */
   return map + transfer->box.x;
}



static void svga_buffer_transfer_flush_region( struct pipe_context *pipe,
					       struct pipe_transfer *transfer,
					       const struct pipe_box *box)
{
   assert(box->x + box->width <= transfer->box.width);

   svga_buffer_flush_mapped_range(pipe->screen,
				  transfer->resource,
				  transfer->box.x + box->x,
				  box->width);
}

static void svga_buffer_transfer_unmap( struct pipe_context *pipe,
			    struct pipe_transfer *transfer )
{
   svga_buffer_unmap(pipe->screen,
		     transfer->resource);
}







struct u_resource_vtbl svga_buffer_vtbl = 
{
   u_default_resource_get_handle,      /* get_handle */
   svga_buffer_destroy,		     /* resource_destroy */
   svga_buffer_is_referenced,	     /* is_resource_referenced */
   u_default_get_transfer,	     /* get_transfer */
   u_default_transfer_destroy,	     /* transfer_destroy */
   svga_buffer_transfer_map,	     /* transfer_map */
   svga_buffer_transfer_flush_region,  /* transfer_flush_region */
   svga_buffer_transfer_unmap,	     /* transfer_unmap */
   u_default_transfer_inline_write   /* transfer_inline_write */
};



struct pipe_resource *
svga_buffer_create(struct pipe_screen *screen,
		   const struct pipe_resource *template)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
   
   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto error1;
   
   sbuf->b.b = *template;
   sbuf->b.vtbl = &svga_buffer_vtbl;
   pipe_reference_init(&sbuf->b.b.reference, 1);
   sbuf->b.b.screen = screen;

   if(svga_buffer_needs_hw_storage(template->bind)) {
      if(svga_buffer_create_host_surface(ss, sbuf) != PIPE_OK)
         goto error2;
   }
   else {
      sbuf->swbuf = align_malloc(template->width0, 64);
      if(!sbuf->swbuf)
         goto error2;
   }
      
   return &sbuf->b.b; 

error2:
   FREE(sbuf);
error1:
   return NULL;
}

struct pipe_resource *
svga_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
			unsigned bind)
{
   struct svga_buffer *sbuf;
   
   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto no_sbuf;
      
   pipe_reference_init(&sbuf->b.b.reference, 1);
   sbuf->b.vtbl = &svga_buffer_vtbl;
   sbuf->b.b.screen = screen;
   sbuf->b.b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   sbuf->b.b.usage = PIPE_USAGE_IMMUTABLE;
   sbuf->b.b.bind = bind;
   sbuf->b.b.width0 = bytes;
   sbuf->b.b.height0 = 1;
   sbuf->b.b.depth0 = 1;
   sbuf->b.b.array_size = 1;

   sbuf->swbuf = ptr;
   sbuf->user = TRUE;
   
   return &sbuf->b.b; 

no_sbuf:
   return NULL;
}



