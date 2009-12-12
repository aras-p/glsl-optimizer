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

#ifndef SVGA_BUFFER_H
#define SVGA_BUFFER_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "util/u_double_list.h"

#include "svga_screen_cache.h"


#define SVGA_BUFFER_MAGIC 0x344f9005

/**
 * Maximum number of discontiguous ranges
 */
#define SVGA_BUFFER_MAX_RANGES 32


struct svga_screen;
struct svga_context;
struct svga_winsys_buffer;
struct svga_winsys_surface;


struct svga_buffer_range
{
   unsigned start;
   unsigned end;
};


/**
 * Describe a
 *
 * This holds the information to emit a SVGA3dCmdSurfaceDMA.
 */
struct svga_buffer_upload
{
   /**
    * Guest memory region.
    */
   struct svga_winsys_buffer *buf;

   struct svga_buffer_range ranges[SVGA_BUFFER_MAX_RANGES];
   unsigned num_ranges;

   SVGA3dSurfaceDMAFlags flags;

   /**
    * Pointer to the DMA copy box *inside* the command buffer.
    */
   SVGA3dCopyBox *boxes;

   /**
    * Context that has the pending DMA to this buffer.
    */
   struct svga_context *svga;
};


/**
 * SVGA pipe buffer.
 */
struct svga_buffer 
{
   struct pipe_buffer base;

   /** 
    * Marker to detect bad casts in runtime.
    */ 
   uint32_t magic;

   /**
    * Regular (non DMA'able) memory.
    * 
    * Used for user buffers or for buffers which we know before hand that can
    * never be used by the virtual hardware directly, such as constant buffers.
    */
   void *swbuf;
   
   /** 
    * Whether swbuf was created by the user or not.
    */
   boolean user;
   
   /**
    * DMA'ble memory.
    * 
    * A piece of GMR memory. It is created when mapping the buffer, and will be
    * used to upload/download vertex data from the host.
    */
   struct svga_buffer_upload hw;

   /**
    * Creation key for the host surface handle.
    * 
    * This structure describes all the host surface characteristics so that it 
    * can be looked up in cache, since creating a host surface is often a slow
    * operation.
    */
   struct svga_host_surface_cache_key key;
   
   /**
    * Host surface handle.
    * 
    * This is a platform independent abstraction for host SID. We create when 
    * trying to bind
    */
   struct svga_winsys_surface *handle;
   
   struct {
      unsigned count;
      boolean writing;
      boolean flush_explicit;
   } map;
   
   boolean needs_flush;
   struct list_head head;
};


static INLINE struct svga_buffer *
svga_buffer(struct pipe_buffer *buffer)
{
   if (buffer) {
      assert(((struct svga_buffer *)buffer)->magic == SVGA_BUFFER_MAGIC);
      return (struct svga_buffer *)buffer;
   }
   return NULL;
}


/**
 * Returns TRUE for user buffers.  We may
 * decide to use an alternate upload path for these buffers.
 */
static INLINE boolean 
svga_buffer_is_user_buffer( struct pipe_buffer *buffer )
{
   return svga_buffer(buffer)->user;
}


void
svga_screen_init_buffer_functions(struct pipe_screen *screen);

struct svga_winsys_surface *
svga_buffer_handle(struct svga_context *svga,
                   struct pipe_buffer *buf);

void
svga_context_flush_buffers(struct svga_context *svga);

boolean
svga_buffer_free_cached_hw_storage(struct svga_screen *ss);

struct svga_winsys_buffer *
svga_winsys_buffer_create(struct svga_screen *ss,
                          unsigned alignment, 
                          unsigned usage,
                          unsigned size);

#endif /* SVGA_BUFFER_H */
