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
#include "svga_screen_buffer.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/**
 * Vertex and index buffers have to be treated slightly differently from 
 * regular guest memory regions because the SVGA device sees them as 
 * surfaces, and the state tracker can create/destroy without the pipe 
 * driver, therefore we must do the uploads from the vws.
 */
static INLINE boolean
svga_buffer_needs_hw_storage(unsigned usage)
{
   return usage & (PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_INDEX);
}


static INLINE enum pipe_error
svga_buffer_create_host_surface(struct svga_screen *ss,
                                struct svga_buffer *sbuf)
{
   if(!sbuf->handle) {
      sbuf->key.flags = 0;
      
      sbuf->key.format = SVGA3D_BUFFER;
      if(sbuf->base.usage & PIPE_BUFFER_USAGE_VERTEX)
         sbuf->key.flags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;
      if(sbuf->base.usage & PIPE_BUFFER_USAGE_INDEX)
         sbuf->key.flags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;
      
      sbuf->key.size.width = sbuf->base.size;
      sbuf->key.size.height = 1;
      sbuf->key.size.depth = 1;
      
      sbuf->key.numFaces = 1;
      sbuf->key.numMipLevels = 1;
      sbuf->key.cachable = 1;
      
      SVGA_DBG(DEBUG_DMA, "surface_create for buffer sz %d\n", sbuf->base.size);

      sbuf->handle = svga_screen_surface_create(ss, &sbuf->key);
      if(!sbuf->handle)
         return PIPE_ERROR_OUT_OF_MEMORY;
   
      /* Always set the discard flag on the first time the buffer is written
       * as svga_screen_surface_create might have passed a recycled host
       * buffer.
       */
      sbuf->hw.flags.discard = TRUE;

      SVGA_DBG(DEBUG_DMA, "   --> got sid %p sz %d (buffer)\n", sbuf->handle, sbuf->base.size);
   }
   
   return PIPE_OK;
}   


static INLINE void
svga_buffer_destroy_host_surface(struct svga_screen *ss,
                                 struct svga_buffer *sbuf)
{
   if(sbuf->handle) {
      SVGA_DBG(DEBUG_DMA, " ungrab sid %p sz %d\n", sbuf->handle, sbuf->base.size);
      svga_screen_surface_destroy(ss, &sbuf->key, &sbuf->handle);
   }
}   


static INLINE void
svga_buffer_destroy_hw_storage(struct svga_screen *ss, struct svga_buffer *sbuf)
{
   struct svga_winsys_screen *sws = ss->sws;

   assert(!sbuf->map.count);
   assert(sbuf->hw.buf);
   if(sbuf->hw.buf) {
      sws->buffer_destroy(sws, sbuf->hw.buf);
      sbuf->hw.buf = NULL;
   }
}

struct svga_winsys_buffer *
svga_winsys_buffer_create( struct svga_screen *ss,
                           unsigned alignment, 
                           unsigned usage,
                           unsigned size )
{
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_winsys_buffer *buf;
   
   /* Just try */
   buf = sws->buffer_create(sws, alignment, usage, size);
   if(!buf) {

      SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "flushing screen to find %d bytes GMR\n", 
               size); 
      
      /* Try flushing all pending DMAs */
      svga_screen_flush(ss, NULL);
      buf = sws->buffer_create(sws, alignment, usage, size);

   }
   
   return buf;
}


/**
 * Allocate DMA'ble storage for the buffer. 
 * 
 * Called before mapping a buffer.
 */
static INLINE enum pipe_error
svga_buffer_create_hw_storage(struct svga_screen *ss,
                              struct svga_buffer *sbuf)
{
   if(!sbuf->hw.buf) {
      unsigned alignment = sbuf->base.alignment;
      unsigned usage = 0;
      unsigned size = sbuf->base.size;
      
      sbuf->hw.buf = svga_winsys_buffer_create(ss, alignment, usage, size);
      if(!sbuf->hw.buf)
         return PIPE_ERROR_OUT_OF_MEMORY;
      
      assert(!sbuf->needs_flush);
   }
   
   return PIPE_OK;
}


/**
 * Variant of SVGA3D_BufferDMA which leaves the copy box temporarily in blank.
 */
static enum pipe_error
svga_buffer_upload_command(struct svga_context *svga,
                           struct svga_buffer *sbuf)
{
   struct svga_winsys_context *swc = svga->swc;
   struct svga_winsys_buffer *guest = sbuf->hw.buf;
   struct svga_winsys_surface *host = sbuf->handle;
   SVGA3dTransferType transfer = SVGA3D_WRITE_HOST_VRAM;
   SVGA3dSurfaceDMAFlags flags = sbuf->hw.flags;
   SVGA3dCmdSurfaceDMA *cmd;
   uint32 numBoxes = sbuf->hw.num_ranges;
   SVGA3dCopyBox *boxes;
   SVGA3dCmdSurfaceDMASuffix *pSuffix;
   unsigned region_flags;
   unsigned surface_flags;
   struct pipe_buffer *dummy;

   if(transfer == SVGA3D_WRITE_HOST_VRAM) {
      region_flags = PIPE_BUFFER_USAGE_GPU_READ;
      surface_flags = PIPE_BUFFER_USAGE_GPU_WRITE;
   }
   else if(transfer == SVGA3D_READ_HOST_VRAM) {
      region_flags = PIPE_BUFFER_USAGE_GPU_WRITE;
      surface_flags = PIPE_BUFFER_USAGE_GPU_READ;
   }
   else {
      assert(0);
      return PIPE_ERROR_BAD_INPUT;
   }

   assert(numBoxes);

   cmd = SVGA3D_FIFOReserve(swc,
                            SVGA_3D_CMD_SURFACE_DMA,
                            sizeof *cmd + numBoxes * sizeof *boxes + sizeof *pSuffix,
                            2);
   if(!cmd)
      return PIPE_ERROR_OUT_OF_MEMORY;

   swc->region_relocation(swc, &cmd->guest.ptr, guest, 0, region_flags);
   cmd->guest.pitch = 0;

   swc->surface_relocation(swc, &cmd->host.sid, host, surface_flags);
   cmd->host.face = 0;
   cmd->host.mipmap = 0;

   cmd->transfer = transfer;

   sbuf->hw.boxes = (SVGA3dCopyBox *)&cmd[1];
   sbuf->hw.svga = svga;

   /* Increment reference count */
   dummy = NULL;
   pipe_buffer_reference(&dummy, &sbuf->base);

   pSuffix = (SVGA3dCmdSurfaceDMASuffix *)((uint8_t*)cmd + sizeof *cmd + numBoxes * sizeof *boxes);
   pSuffix->suffixSize = sizeof *pSuffix;
   pSuffix->maximumOffset = sbuf->base.size;
   pSuffix->flags = flags;

   swc->commit(swc);

   return PIPE_OK;
}


/**
 * Patch up the upload DMA command reserved by svga_buffer_upload_command
 * with the final ranges.
 */
static void
svga_buffer_upload_flush(struct svga_context *svga,
                         struct svga_buffer *sbuf)
{
   SVGA3dCopyBox *boxes;
   unsigned i;

   assert(sbuf->handle); 
   assert(sbuf->hw.buf);
   assert(sbuf->hw.num_ranges);
   assert(sbuf->hw.svga == svga);
   assert(sbuf->hw.boxes);
   
   /*
    * Patch the DMA command with the final copy box.
    */

   SVGA_DBG(DEBUG_DMA, "dma to sid %p\n", sbuf->handle);

   boxes = sbuf->hw.boxes;
   for(i = 0; i < sbuf->hw.num_ranges; ++i) {
      SVGA_DBG(DEBUG_DMA, "  bytes %u - %u\n",
               sbuf->hw.ranges[i].start, sbuf->hw.ranges[i].end);

      boxes[i].x = sbuf->hw.ranges[i].start;
      boxes[i].y = 0;
      boxes[i].z = 0;
      boxes[i].w = sbuf->hw.ranges[i].end - sbuf->hw.ranges[i].start;
      boxes[i].h = 1;
      boxes[i].d = 1;
      boxes[i].srcx = sbuf->hw.ranges[i].start;
      boxes[i].srcy = 0;
      boxes[i].srcz = 0;
   }

   sbuf->hw.num_ranges = 0;
   memset(&sbuf->hw.flags, 0, sizeof sbuf->hw.flags);

   assert(sbuf->head.prev && sbuf->head.next);
   LIST_DEL(&sbuf->head);
#ifdef DEBUG
   sbuf->head.next = sbuf->head.prev = NULL; 
#endif
   sbuf->needs_flush = FALSE;

   sbuf->hw.svga = NULL;
   sbuf->hw.boxes = NULL;

   /* Decrement reference count */
   pipe_reference(&(sbuf->base.reference), NULL);
   sbuf = NULL;
}


/**
 * Queue a DMA upload of a range of this buffer to the host.
 *
 * This function only notes the range down. It doesn't actually emit a DMA
 * upload command. That only happens when a context tries to refer to this
 * buffer, and the DMA upload command is added to that context's command buffer.
 * 
 * We try to lump as many contiguous DMA transfers together as possible.
 */
static void
svga_buffer_upload_queue(struct svga_buffer *sbuf,
                         unsigned start,
                         unsigned end)
{
   unsigned i;
   unsigned nearest_range;
   unsigned nearest_dist;

   assert(sbuf->hw.buf);
   assert(end > start);
   
   if (sbuf->hw.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      nearest_range = sbuf->hw.num_ranges;
      nearest_dist = ~0;
   } else {
      nearest_range = SVGA_BUFFER_MAX_RANGES - 1;
      nearest_dist = 0;
   }

   /*
    * Try to grow one of the ranges.
    *
    * Note that it is not this function task to care about overlapping ranges,
    * as the GMR was already given so it is too late to do anything. Situations
    * where overlapping ranges may pose a problem should be detected via
    * pipe_context::is_buffer_referenced and the context that refers to the
    * buffer should be flushed.
    */

   for(i = 0; i < sbuf->hw.num_ranges; ++i) {
      int left_dist;
      int right_dist;
      int dist;

      left_dist = start - sbuf->hw.ranges[i].end;
      right_dist = sbuf->hw.ranges[i].start - end;
      dist = MAX2(left_dist, right_dist);

      if (dist <= 0) {
         /*
          * Ranges are contiguous or overlapping -- extend this one and return.
          */

         sbuf->hw.ranges[i].start = MIN2(sbuf->hw.ranges[i].start, start);
         sbuf->hw.ranges[i].end   = MAX2(sbuf->hw.ranges[i].end,   end);
         return;
      }
      else {
         /*
          * Discontiguous ranges -- keep track of the nearest range.
          */

         if (dist < nearest_dist) {
            nearest_range = i;
            nearest_dist = dist;
         }
      }
   }

   /*
    * We cannot add a new range to an existing DMA command, so patch-up the
    * pending DMA upload and start clean.
    */

   if(sbuf->needs_flush)
      svga_buffer_upload_flush(sbuf->hw.svga, sbuf);

   assert(!sbuf->needs_flush);
   assert(!sbuf->hw.svga);
   assert(!sbuf->hw.boxes);

   if (sbuf->hw.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      /*
       * Add a new range.
       */

      sbuf->hw.ranges[sbuf->hw.num_ranges].start = start;
      sbuf->hw.ranges[sbuf->hw.num_ranges].end = end;
      ++sbuf->hw.num_ranges;
   } else {
      /*
       * Everything else failed, so just extend the nearest range.
       *
       * It is OK to do this because we always keep a local copy of the
       * host buffer data, for SW TNL, and the host never modifies the buffer.
       */

      assert(nearest_range < SVGA_BUFFER_MAX_RANGES);
      assert(nearest_range < sbuf->hw.num_ranges);
      sbuf->hw.ranges[nearest_range].start = MIN2(sbuf->hw.ranges[nearest_range].start, start);
      sbuf->hw.ranges[nearest_range].end   = MAX2(sbuf->hw.ranges[nearest_range].end,   end);
   }
}


static void *
svga_buffer_map_range( struct pipe_screen *screen,
                       struct pipe_buffer *buf,
                       unsigned offset, unsigned length,
                       unsigned usage )
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_buffer *sbuf = svga_buffer( buf );
   void *map;

   if (!sbuf->swbuf && !sbuf->hw.buf) {
      if (svga_buffer_create_hw_storage(ss, sbuf) != PIPE_OK) {
         /*
          * We can't create a hardware buffer big enough, so create a malloc
          * buffer instead.
          */

         debug_printf("%s: failed to allocate %u KB of DMA, splitting DMA transfers\n",
                      __FUNCTION__,
                      (sbuf->base.size + 1023)/1024);

         sbuf->swbuf = align_malloc(sbuf->base.size, sbuf->base.alignment);
      }
   }

   if (sbuf->swbuf) {
      /* User/malloc buffer */
      map = sbuf->swbuf;
   }
   else if (sbuf->hw.buf) {
      map = sws->buffer_map(sws, sbuf->hw.buf, usage);
   }
   else {
      map = NULL;
   }

   if(map) {
      pipe_mutex_lock(ss->swc_mutex);

      ++sbuf->map.count;

      if (usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
         assert(sbuf->map.count <= 1);
         sbuf->map.writing = TRUE;
         if (usage & PIPE_BUFFER_USAGE_FLUSH_EXPLICIT)
            sbuf->map.flush_explicit = TRUE;
      }
      
      pipe_mutex_unlock(ss->swc_mutex);
   }
   
   return map;
}

static void 
svga_buffer_flush_mapped_range( struct pipe_screen *screen,
                                struct pipe_buffer *buf,
                                unsigned offset, unsigned length)
{
   struct svga_buffer *sbuf = svga_buffer( buf );
   struct svga_screen *ss = svga_screen(screen);
   
   pipe_mutex_lock(ss->swc_mutex);
   assert(sbuf->map.writing);
   if(sbuf->map.writing) {
      assert(sbuf->map.flush_explicit);
      if(sbuf->hw.buf)
         svga_buffer_upload_queue(sbuf, offset, offset + length);
   }
   pipe_mutex_unlock(ss->swc_mutex);
}

static void 
svga_buffer_unmap( struct pipe_screen *screen,
                   struct pipe_buffer *buf)
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_buffer *sbuf = svga_buffer( buf );
   
   pipe_mutex_lock(ss->swc_mutex);
   
   assert(sbuf->map.count);
   if(sbuf->map.count)
      --sbuf->map.count;

   if(sbuf->hw.buf)
      sws->buffer_unmap(sws, sbuf->hw.buf);

   if(sbuf->map.writing) {
      if(!sbuf->map.flush_explicit) {
         /* No mapped range was flushed -- flush the whole buffer */
         SVGA_DBG(DEBUG_DMA, "flushing the whole buffer\n");
   
         if(sbuf->hw.buf)
            svga_buffer_upload_queue(sbuf, 0, sbuf->base.size);
      }
      
      sbuf->map.writing = FALSE;
      sbuf->map.flush_explicit = FALSE;
   }

   pipe_mutex_unlock(ss->swc_mutex);
}

static void
svga_buffer_destroy( struct pipe_buffer *buf )
{
   struct svga_screen *ss = svga_screen(buf->screen); 
   struct svga_buffer *sbuf = svga_buffer( buf );

   assert(!p_atomic_read(&buf->reference.count));
   
   assert(!sbuf->needs_flush);

   if(sbuf->handle)
      svga_buffer_destroy_host_surface(ss, sbuf);
   
   if(sbuf->hw.buf)
      svga_buffer_destroy_hw_storage(ss, sbuf);
   
   if(sbuf->swbuf && !sbuf->user)
      align_free(sbuf->swbuf);
   
   FREE(sbuf);
}

static struct pipe_buffer *
svga_buffer_create(struct pipe_screen *screen,
                   unsigned alignment,
                   unsigned usage,
                   unsigned size)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
   
   assert(size);
   assert(alignment);

   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto error1;
      
   sbuf->magic = SVGA_BUFFER_MAGIC;
   
   pipe_reference_init(&sbuf->base.reference, 1);
   sbuf->base.screen = screen;
   sbuf->base.alignment = alignment;
   sbuf->base.usage = usage;
   sbuf->base.size = size;

   if(svga_buffer_needs_hw_storage(usage)) {
      if(svga_buffer_create_host_surface(ss, sbuf) != PIPE_OK)
         goto error2;
   }
   else {
      if(alignment < sizeof(void*))
         alignment = sizeof(void*);

      usage |= PIPE_BUFFER_USAGE_CPU_READ_WRITE;
      
      sbuf->swbuf = align_malloc(size, alignment);
      if(!sbuf->swbuf)
         goto error2;
   }
      
   return &sbuf->base; 

error2:
   FREE(sbuf);
error1:
   return NULL;
}

static struct pipe_buffer *
svga_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes)
{
   struct svga_buffer *sbuf;
   
   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto no_sbuf;
      
   sbuf->magic = SVGA_BUFFER_MAGIC;
   
   sbuf->swbuf = ptr;
   sbuf->user = TRUE;
   
   pipe_reference_init(&sbuf->base.reference, 1);
   sbuf->base.screen = screen;
   sbuf->base.alignment = 1;
   sbuf->base.usage = 0;
   sbuf->base.size = bytes;
   
   return &sbuf->base; 

no_sbuf:
   return NULL;
}

   
void
svga_screen_init_buffer_functions(struct pipe_screen *screen)
{
   screen->buffer_create = svga_buffer_create;
   screen->user_buffer_create = svga_user_buffer_create;
   screen->buffer_map_range = svga_buffer_map_range;
   screen->buffer_flush_mapped_range = svga_buffer_flush_mapped_range;
   screen->buffer_unmap = svga_buffer_unmap;
   screen->buffer_destroy = svga_buffer_destroy;
}


/** 
 * Copy the contents of the user buffer / malloc buffer to a hardware buffer.
 */
static INLINE enum pipe_error
svga_buffer_update_hw(struct svga_screen *ss, struct svga_buffer *sbuf)
{
   if(!sbuf->hw.buf) {
      enum pipe_error ret;
      void *map;
      
      assert(sbuf->swbuf);
      if(!sbuf->swbuf)
         return PIPE_ERROR;
      
      ret = svga_buffer_create_hw_storage(ss, sbuf);
      assert(ret == PIPE_OK);
      if(ret != PIPE_OK)
         return ret;

      pipe_mutex_lock(ss->swc_mutex);
      map = ss->sws->buffer_map(ss->sws, sbuf->hw.buf, PIPE_BUFFER_USAGE_CPU_WRITE);
      assert(map);
      if(!map) {
	 pipe_mutex_unlock(ss->swc_mutex);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      memcpy(map, sbuf->swbuf, sbuf->base.size);
      ss->sws->buffer_unmap(ss->sws, sbuf->hw.buf);

      /* This user/malloc buffer is now indistinguishable from a gpu buffer */
      assert(!sbuf->map.count);
      if(!sbuf->map.count) {
         if(sbuf->user)
            sbuf->user = FALSE;
         else
            align_free(sbuf->swbuf);
         sbuf->swbuf = NULL;
      }
      
      svga_buffer_upload_queue(sbuf, 0, sbuf->base.size);
   }
   
   pipe_mutex_unlock(ss->swc_mutex);
   return PIPE_OK;
}


struct svga_winsys_surface *
svga_buffer_handle(struct svga_context *svga,
                   struct pipe_buffer *buf)
{
   struct pipe_screen *screen = svga->pipe.screen;
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
   enum pipe_error ret;

   if(!buf)
      return NULL;

   sbuf = svga_buffer(buf);
   
   assert(!sbuf->map.count);
   
   if(!sbuf->handle) {
      ret = svga_buffer_create_host_surface(ss, sbuf);
      if(ret != PIPE_OK)
	 return NULL;

      ret = svga_buffer_update_hw(ss, sbuf);
      if(ret != PIPE_OK)
	 return NULL;
   }

   if(!sbuf->needs_flush && sbuf->hw.num_ranges) {
      /* Queue the buffer for flushing */
      ret = svga_buffer_upload_command(svga, sbuf);
      if(ret != PIPE_OK)
         /* XXX: Should probably have a richer return value */
         return NULL;

      assert(sbuf->hw.svga == svga);

      sbuf->needs_flush = TRUE;
      assert(!sbuf->head.prev && !sbuf->head.next);
      LIST_ADDTAIL(&sbuf->head, &svga->dirty_buffers);
   }

   return sbuf->handle;
}

struct pipe_buffer *
svga_screen_buffer_wrap_surface(struct pipe_screen *screen,
				enum SVGA3dSurfaceFormat format,
				struct svga_winsys_surface *srf)
{
   struct pipe_buffer *buf;
   struct svga_buffer *sbuf;
   struct svga_winsys_screen *sws = svga_winsys_screen(screen);

   buf = svga_buffer_create(screen, 0, SVGA_BUFFER_USAGE_WRAPPED, 0);
   if (!buf)
      return NULL;

   sbuf = svga_buffer(buf);

   /*
    * We are not the creator of this surface and therefore we must not
    * cache it for reuse. Set the cacheable flag to zero in the key to
    * prevent this.
    */
   sbuf->key.format = format;
   sbuf->key.cachable = 0;
   sws->surface_reference(sws, &sbuf->handle, srf);

   return buf;
}


struct svga_winsys_surface *
svga_screen_buffer_get_winsys_surface(struct pipe_buffer *buffer)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(buffer->screen);
   struct svga_winsys_surface *vsurf = NULL;

   assert(svga_buffer(buffer)->key.cachable == 0);
   svga_buffer(buffer)->key.cachable = 0;
   sws->surface_reference(sws, &vsurf, svga_buffer(buffer)->handle);
   return vsurf;
}

void
svga_context_flush_buffers(struct svga_context *svga)
{
   struct list_head *curr, *next;
   struct svga_buffer *sbuf;

   curr = svga->dirty_buffers.next;
   next = curr->next;
   while(curr != &svga->dirty_buffers) {
      sbuf = LIST_ENTRY(struct svga_buffer, curr, head);

      assert(p_atomic_read(&sbuf->base.reference.count) != 0);
      assert(sbuf->needs_flush);
      
      svga_buffer_upload_flush(svga, sbuf);

      curr = next; 
      next = curr->next;
   }
}
