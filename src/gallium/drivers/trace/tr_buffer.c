/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tr_buffer.h"

struct pipe_buffer *
trace_buffer_create(struct trace_screen *tr_scr,
                    struct pipe_buffer *buffer)
{
   struct trace_buffer *tr_buf;

   if(!buffer)
      goto error;

   assert(buffer->screen == tr_scr->screen);

   tr_buf = CALLOC_STRUCT(trace_buffer);
   if(!tr_buf)
      goto error;

   memcpy(&tr_buf->base, buffer, sizeof(struct pipe_buffer));

   pipe_reference_init(&tr_buf->base.reference, 1);
   tr_buf->base.screen = &tr_scr->base;
   tr_buf->buffer = buffer;

   trace_screen_add_to_list(tr_scr, buffers, tr_buf);

   return &tr_buf->base;

error:
   pipe_buffer_reference(&buffer, NULL);
   return NULL;
}


void
trace_buffer_destroy(struct trace_screen *tr_scr,
                     struct pipe_buffer *buffer)
{
   struct trace_buffer *tr_buf = trace_buffer(buffer);

   trace_screen_remove_from_list(tr_scr, buffers, tr_buf);

   pipe_buffer_reference(&tr_buf->buffer, NULL);
   FREE(tr_buf);
}
