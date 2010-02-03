/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * Totally software-based winsys layer.
 * Note that the one winsys function that we can't implement here
 * is flush_frontbuffer().
 * Whoever uses this code will have to provide that.
 *
 * Authors: Brian Paul
 */


#include "util/u_simple_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "sw_winsys.h"



/** Subclass of pipe_winsys */
struct sw_pipe_winsys
{
   struct pipe_winsys Base;
   /* no extra fields for now */
};


/** subclass of pipe_buffer */
struct sw_pipe_buffer
{
   struct pipe_buffer Base;
   boolean UserBuffer;  /** Is this a user-space buffer? */
   void *Data;
   void *Mapped;
};


/** cast wrapper */
static INLINE struct sw_pipe_buffer *
sw_pipe_buffer(struct pipe_buffer *b)
{
   return (struct sw_pipe_buffer *) b;
}


static const char *
get_name(struct pipe_winsys *pws)
{
   return "software";
}


/** Create new pipe_buffer and allocate storage of given size */
static struct pipe_buffer *
buffer_create(struct pipe_winsys *pws, 
              unsigned alignment, 
              unsigned usage,
              unsigned size)
{
   struct sw_pipe_buffer *buffer = CALLOC_STRUCT(sw_pipe_buffer);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->Base.reference, 1);
   buffer->Base.alignment = alignment;
   buffer->Base.usage = usage;
   buffer->Base.size = size;

   /* align to 16-byte multiple for Cell */
   buffer->Data = align_malloc(size, MAX2(alignment, 16));

   return &buffer->Base;
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct sw_pipe_buffer *buffer = CALLOC_STRUCT(sw_pipe_buffer);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->Base.reference, 1);
   buffer->Base.size = bytes;
   buffer->UserBuffer = TRUE;
   buffer->Data = ptr;

   return &buffer->Base;
}


static void *
buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf, unsigned flags)
{
   struct sw_pipe_buffer *buffer = sw_pipe_buffer(buf);
   buffer->Mapped = buffer->Data;
   return buffer->Mapped;
}


static void
buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
   struct sw_pipe_buffer *buffer = sw_pipe_buffer(buf);
   buffer->Mapped = NULL;
}


static void
buffer_destroy(struct pipe_buffer *buf)
{
   struct sw_pipe_buffer *buffer = sw_pipe_buffer(buf);

   if (buffer->Data && !buffer->UserBuffer) {
      align_free(buffer->Data);
      buffer->Data = NULL;
   }

   free(buffer);
}


static struct pipe_buffer *
surface_buffer_create(struct pipe_winsys *winsys,
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
                                *stride * nblocksy);
}


static void
fence_reference(struct pipe_winsys *sws, struct pipe_fence_handle **ptr,
                struct pipe_fence_handle *fence)
{
   /* no-op */
}


static int
fence_signalled(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
                unsigned flag)
{
   /* no-op */
   return 0;
}


static int
fence_finish(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
             unsigned flag)
{
   /* no-op */
   return 0;
}


/**
 * Create/return a new pipe_winsys object.
 */
struct pipe_winsys *
create_sw_winsys(void)
{
   struct sw_pipe_winsys *ws = CALLOC_STRUCT(sw_pipe_winsys);
   if (!ws)
      return NULL;

   /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    */
   ws->Base.buffer_create = buffer_create;
   ws->Base.user_buffer_create = user_buffer_create;
   ws->Base.buffer_map = buffer_map;
   ws->Base.buffer_unmap = buffer_unmap;
   ws->Base.buffer_destroy = buffer_destroy;

   ws->Base.surface_buffer_create = surface_buffer_create;

   ws->Base.fence_reference = fence_reference;
   ws->Base.fence_signalled = fence_signalled;
   ws->Base.fence_finish = fence_finish;

   ws->Base.flush_frontbuffer = NULL; /* not implemented here! */

   ws->Base.get_name = get_name;

   return &ws->Base;
}
