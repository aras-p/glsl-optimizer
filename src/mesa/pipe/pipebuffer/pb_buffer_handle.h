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
 * Buffer handle interface.
 *
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_BUFFER_HANDLE_H_
#define PB_BUFFER_HANDLE_H_


#include <assert.h>

#include "pb_buffer.h"


/**
 * Buffer handle.
 *
 * The buffer handle and the buffer data storage are separate entities. This
 * is modelled after ARB_vertex_buffer_object, which is the interface that
 * Gallium requires. See p_winsys.h for more information.
 */
struct pipe_buffer_handle 
{
   /** Reference count */
   unsigned refcount;

   /** Allocation characteristics */
   unsigned alignment;
   unsigned flags;
   unsigned hint;
   
   /** 
    * The actual buffer.
    * 
    * It should never be NULL. Use null_buffer instead. 
    */
   struct pipe_buffer *buf;
};


/**
 * Set buffer storage.
 * 
 * NOTE: this will not increase the buffer reference count.
 */
static inline void
buffer_handle_data(struct pipe_buffer_handle *handle,
                   struct pipe_buffer *buf)
{
   assert(handle);
   assert(handle->buf);
   buffer_release(handle->buf);
   assert(buf);
   handle->buf = buf;
}


static inline void
buffer_handle_clear(struct pipe_buffer_handle *handle)
{
   buffer_handle_data(handle, &null_buffer);
}


static inline int
buffer_handle_has_data(struct pipe_buffer_handle *handle) {
   assert(handle);
   assert(handle->buf);
   return handle->buf != &null_buffer;
}


/**
 * Fill in the pipe_winsys' buffer-related callbacks.
 * 
 * Specifically, the fullfilled functions are:
 * - buffer_create
 * - user_buffer_create
 * - buffer_map
 * - buffer_unmap
 * - buffer_reference
 * - buffer_subdata
 * - buffer_get_subdata
 * 
 * NOTE: buffer_data is left untouched.
 */
void 
buffer_handle_init_winsys(struct pipe_winsys *winsys);


#endif /*PB_BUFFER_HANDLE_H_*/
