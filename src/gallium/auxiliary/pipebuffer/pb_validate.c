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
 * @file
 * Buffer validation.
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_error.h"
#include "pipe/p_util.h"
#include "pipe/p_debug.h"

#include "util/u_hash_table.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"


struct pb_validate
{
   struct hash_table *buffer_table;
};


static unsigned buffer_table_hash(void *pb_buf) 
{
   return (unsigned)(uintptr_t)pb_buf;
}


static int buffer_table_compare(void *pb_buf1, void *pb_buf2)
{
   return (char *)pb_buf2 - (char *)pb_buf1; 
}


enum pipe_error
pb_validate_add_buffer(struct pb_validate *vl,
                       struct pb_buffer *buf)
{
   enum pipe_error ret;
   
   assert(buf);
   if(!buf)
      return PIPE_ERROR;
   
   if(!hash_table_get(vl->buffer_table, buf)) {
      struct pb_buffer *tmp = NULL;
      
      ret = hash_table_set(vl->buffer_table, buf, buf);
      if(ret != PIPE_OK)
	 return ret;
      
      /* Increment reference count */
      pb_reference(&tmp, buf);
   }
   
   return PIPE_OK;
}


enum pipe_error
pb_validate_validate(struct pb_validate *vl) 
{
   /* FIXME: go through each buffer, ensure its not mapped, its address is 
    * available -- requires a new pb_buffer interface */
   return PIPE_OK;
}


struct pb_validate_fence_data {
   struct pb_validate *vl;
   struct pipe_fence_handle *fence;
};


static enum pipe_error
pb_validate_fence_cb(void *key, void *value, void *_data) 
{
   struct pb_buffer *buf = (struct pb_buffer *)key;
   struct pb_validate_fence_data *data = (struct pb_validate_fence_data *)_data;
   struct pb_validate *vl = data->vl;
   struct pipe_fence_handle *fence = data->fence;
   
   assert(value == key);
   
   buffer_fence(buf, fence);
   
   /* Decrement the reference count -- table entry destroyed later */
   pb_reference(&buf, NULL);
   
   return PIPE_OK;
}


void
pb_validate_fence(struct pb_validate *vl,
                  struct pipe_fence_handle *fence)
{
   struct pb_validate_fence_data data;
   
   data.vl = vl;
   data.fence = fence;
   
   hash_table_foreach(vl->buffer_table, 
                      pb_validate_fence_cb, 
                      &data);

   /* FIXME: cso_hash shrinks here, which is not desirable in this use case, 
    * as it will be refilled right soon */ 
   hash_table_clear(vl->buffer_table);
}


void
pb_validate_destroy(struct pb_validate *vl)
{
   pb_validate_fence(vl, NULL);
   hash_table_destroy(vl->buffer_table);
   FREE(vl);
}


struct pb_validate *
pb_validate_create()
{
   struct pb_validate *vl;
   
   vl = CALLOC_STRUCT(pb_validate);
   if(!vl)
      return NULL;
   
   vl->buffer_table = hash_table_create(buffer_table_hash,
                                        buffer_table_compare);
   if(!vl->buffer_table) {
      FREE(vl);
      return NULL;
   }

   return vl;
}

