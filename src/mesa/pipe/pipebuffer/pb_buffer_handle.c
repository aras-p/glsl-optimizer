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
 * Drop-in implementation of the winsys driver functions for buffer handles.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include <assert.h>
#include <stdlib.h>

#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"

#include "pb_buffer.h"
#include "pb_buffer_handle.h"


static struct pipe_buffer_handle *
buffer_handle_create(struct pipe_winsys *winsys,
                     unsigned alignment,
                     unsigned flags,
                     unsigned hint)
{
   struct pipe_buffer_handle *handle;
  
   handle = (struct pipe_buffer_handle *)malloc(sizeof(struct pipe_buffer_handle));
   if(!handle)
      return NULL;
   
   handle->refcount = 1;
   handle->alignment = alignment;
   handle->flags = flags;
   handle->hint = hint;
   
   handle->buf = &null_buffer;
   
   return handle;
}


static struct pipe_buffer_handle *
buffer_handle_create_user(struct pipe_winsys *winsys, 
                          void *data, unsigned size)
{
   struct pipe_buffer_handle *handle;
   struct pipe_buffer *buf;
   
   handle = buffer_handle_create(winsys, 1, 0, 0);
   if(!handle)
      return NULL;
   
   buf = client_buffer_create(data);
   if(!buf) {
      free(handle);
      return NULL;
   }
   
   buffer_handle_data(handle, buf);
   
   return handle;
}


static void *
buffer_handle_map(struct pipe_winsys *winsys,
                  struct pipe_buffer_handle *handle, 
                  unsigned flags)
{
   return buffer_map(handle->buf, flags);
}


static void 
buffer_handle_unmap(struct pipe_winsys *winsys,
                    struct pipe_buffer_handle *handle)
{
   buffer_unmap(handle->buf);
}


static void
buffer_handle_reference(struct pipe_winsys *winsys,
                        struct pipe_buffer_handle **dst,
		        struct pipe_buffer_handle *src)
{
   /* XXX: should this be thread safe? */
   
   if (src) {
      src->refcount++;
   }
   
   if (*dst) {
      (*dst)->refcount--;
      if ((*dst)->refcount == 0) {
	 buffer_release((*dst)->buf);
	 free(*dst);
      }
   }

   *dst = src;
}


static int 
buffer_handle_subdata(struct pipe_winsys *winsys, 
                      struct pipe_buffer_handle *handle,
                      unsigned long offset, 
                      unsigned long size, 
                      const void *data)
{
   void *map;
   assert(handle);
   assert(data);
   map = buffer_handle_map(winsys, handle, PIPE_BUFFER_FLAG_WRITE);
   if(map) {
      memcpy((char *)map + offset, data, size);
      buffer_handle_unmap(winsys, handle);
      return 0;
   }
   return -1; 
}


static int  
buffer_handle_get_subdata(struct pipe_winsys *winsys,
                          struct pipe_buffer_handle *handle,
                          unsigned long offset, 
                          unsigned long size, 
                          void *data)
{
   void *map;
   assert(handle);
   assert(data);
   map = buffer_handle_map(winsys, handle, PIPE_BUFFER_FLAG_READ);
   if(map) {
      memcpy(data, (char *)map + offset, size);
      buffer_handle_unmap(winsys, handle);
      return 0;
   }
   return -1; 
}


void 
buffer_handle_init_winsys(struct pipe_winsys *winsys)
{
   winsys->buffer_create = buffer_handle_create;
   winsys->user_buffer_create = buffer_handle_create_user;
   winsys->buffer_map = buffer_handle_map;
   winsys->buffer_unmap = buffer_handle_unmap;
   winsys->buffer_reference = buffer_handle_reference;
   winsys->buffer_subdata = buffer_handle_subdata;
   winsys->buffer_get_subdata = buffer_handle_get_subdata;
}
