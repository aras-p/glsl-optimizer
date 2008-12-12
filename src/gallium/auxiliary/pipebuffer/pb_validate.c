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
#include "util/u_memory.h"
#include "pipe/p_debug.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"
#include "pb_validate.h"


#define PB_VALIDATE_INITIAL_SIZE 1 /* 512 */ 


struct pb_validate
{
   struct pb_buffer **buffers;
   unsigned used;
   unsigned size;
};


enum pipe_error
pb_validate_add_buffer(struct pb_validate *vl,
                       struct pb_buffer *buf)
{
   assert(buf);
   if(!buf)
      return PIPE_ERROR;

   /* We only need to store one reference for each buffer, so avoid storing
    * consecutive references for the same buffer. It might not be the more 
    * common pasttern, but it is easy to implement.
    */
   if(vl->used && vl->buffers[vl->used - 1] == buf) {
      return PIPE_OK;
   }
   
   /* Grow the table */
   if(vl->used == vl->size) {
      unsigned new_size;
      struct pb_buffer **new_buffers;
      
      new_size = vl->size * 2;
      if(!new_size)
	 return PIPE_ERROR_OUT_OF_MEMORY;

      new_buffers = (struct pb_buffer **)REALLOC(vl->buffers,
                                                 vl->size*sizeof(struct pb_buffer *),
                                                 new_size*sizeof(struct pb_buffer *));
      if(!new_buffers)
         return PIPE_ERROR_OUT_OF_MEMORY;
      
      memset(new_buffers + vl->size, 0, (new_size - vl->size)*sizeof(struct pb_buffer *));
      
      vl->size = new_size;
      vl->buffers = new_buffers;
   }
   
   assert(!vl->buffers[vl->used]);
   pb_reference(&vl->buffers[vl->used], buf);
   ++vl->used;
   
   return PIPE_OK;
}


enum pipe_error
pb_validate_validate(struct pb_validate *vl) 
{
   /* FIXME: go through each buffer, ensure its not mapped, its address is 
    * available -- requires a new pb_buffer interface */
   return PIPE_OK;
}


void
pb_validate_fence(struct pb_validate *vl,
                  struct pipe_fence_handle *fence)
{
   unsigned i;
   for(i = 0; i < vl->used; ++i) {
      buffer_fence(vl->buffers[i], fence);
      pb_reference(&vl->buffers[i], NULL);
   }
   vl->used = 0;
}


void
pb_validate_destroy(struct pb_validate *vl)
{
   unsigned i;
   for(i = 0; i < vl->used; ++i)
      pb_reference(&vl->buffers[i], NULL);
   FREE(vl->buffers);
   FREE(vl);
}


struct pb_validate *
pb_validate_create()
{
   struct pb_validate *vl;
   
   vl = CALLOC_STRUCT(pb_validate);
   if(!vl)
      return NULL;
   
   vl->size = PB_VALIDATE_INITIAL_SIZE;
   vl->buffers = (struct pb_buffer **)CALLOC(vl->size, sizeof(struct pb_buffer *));
   if(!vl->buffers) {
      FREE(vl);
      return NULL;
   }

   return vl;
}

