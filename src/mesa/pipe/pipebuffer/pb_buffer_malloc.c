/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * \file
 * Implementation of malloc-based buffers to store data that can't be processed
 * by the hardware. 
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include <assert.h>
#include <stdlib.h>

#include "pb_buffer.h"


struct malloc_buffer 
{
   struct pipe_buffer base;
   void *data;
};


extern const struct pipe_buffer_vtbl malloc_buffer_vtbl;

static inline struct malloc_buffer *
malloc_buffer(struct pipe_buffer *buf)
{
   assert(buf);
   assert(buf->vtbl == &malloc_buffer_vtbl);
   return (struct malloc_buffer *)buf;
}


static void
malloc_buffer_reference(struct pipe_buffer *buf)
{
   /* no-op */
}


static void
malloc_buffer_release(struct pipe_buffer *buf)
{
   free(malloc_buffer(buf)->data);
   free(buf);
}


static void *
malloc_buffer_map(struct pipe_buffer *buf, 
                  unsigned flags)
{
   return malloc_buffer(buf)->data;
}


static void
malloc_buffer_unmap(struct pipe_buffer *buf)
{
   /* No-op */
}


static void
malloc_buffer_get_base_buffer(struct pipe_buffer *buf,
                              struct pipe_buffer **base_buf,
                              unsigned *offset)
{
   *base_buf = buf;
   *offset = 0;
}


const struct pipe_buffer_vtbl 
malloc_buffer_vtbl = {
      malloc_buffer_reference,
      malloc_buffer_release,
      malloc_buffer_map,
      malloc_buffer_unmap,
      malloc_buffer_get_base_buffer
};


struct pipe_buffer *
malloc_buffer_create(unsigned size) 
{
   struct malloc_buffer *buf;
   
   /* TODO: accept an alignment parameter */
   /* TODO: do a single allocation */
   
   buf = (struct malloc_buffer *)malloc(sizeof(struct malloc_buffer));
   if(!buf)
      return NULL;
   
   buf->base.vtbl = &malloc_buffer_vtbl;
   
   buf->data = malloc(size);
   if(!buf->data) {
      free(buf);
      return NULL;
   }

   return &buf->base;
}
