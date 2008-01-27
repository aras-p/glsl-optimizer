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
 * Implementation of client buffer (also designated as "user buffers"), which
 * are just state-tracker owned data masqueraded as buffers.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_util.h"
#include "pb_buffer.h"


struct pb_user_buffer 
{
   struct pb_buffer base;
   void *data;
};


extern const struct pb_vtbl pb_user_buffer_vtbl;


static INLINE struct pb_user_buffer *
pb_user_buffer(struct pb_buffer *buf)
{
   assert(buf);
   assert(buf->vtbl == &pb_user_buffer_vtbl);
   return (struct pb_user_buffer *)buf;
}


static void
pb_user_buffer_destroy(struct pb_buffer *buf)
{
   assert(buf);
   FREE(buf);
}


static void *
pb_user_buffer_map(struct pb_buffer *buf, 
                  unsigned flags)
{
   return pb_user_buffer(buf)->data;
}


static void
pb_user_buffer_unmap(struct pb_buffer *buf)
{
   /* No-op */
}


static void
pb_user_buffer_get_base_buffer(struct pb_buffer *buf,
                              struct pb_buffer **base_buf,
                              unsigned *offset)
{
   *base_buf = buf;
   *offset = 0;
}


const struct pb_vtbl 
pb_user_buffer_vtbl = {
      pb_user_buffer_destroy,
      pb_user_buffer_map,
      pb_user_buffer_unmap,
      pb_user_buffer_get_base_buffer
};


struct pb_buffer *
pb_user_buffer_create(void *data, unsigned bytes) 
{
   struct pb_user_buffer *buf = CALLOC_STRUCT(pb_user_buffer);

   if(!buf)
      return NULL;
   
   buf->base.vtbl = &pb_user_buffer_vtbl;   
   buf->base.base.size = bytes;
   buf->data = data;
   
   return &buf->base;
}
