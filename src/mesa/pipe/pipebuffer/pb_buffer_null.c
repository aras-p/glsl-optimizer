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
 * Null buffer implementation.
 * 
 * We have a special null buffer object so that we can safely call buffer 
 * operations without having to check whether the buffer pointer is null or not.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"

#include "pb_buffer.h"


static void
null_buffer_reference(struct pipe_buffer *buf)
{
   /* No-op */
}


static void
null_buffer_release(struct pipe_buffer *buf)
{
   /* No-op */
}


static void *
null_buffer_map(struct pipe_buffer *buf, 
                unsigned flags)
{
   assert(0);
   return NULL;
}


static void
null_buffer_unmap(struct pipe_buffer *buf)
{
   assert(0);
}


static void
null_buffer_get_base_buffer(struct pipe_buffer *buf,
                            struct pipe_buffer **base_buf,
                            unsigned *offset)
{
   *base_buf = buf;
   *offset = 0;
}


const struct pipe_buffer_vtbl 
pipe_buffer_vtbl = {
      null_buffer_reference,
      null_buffer_release,
      null_buffer_map,
      null_buffer_unmap,
      null_buffer_get_base_buffer
};


struct pipe_buffer
null_buffer = {
      &pipe_buffer_vtbl
};
