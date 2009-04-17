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

#ifndef TR_BUFFER_H_
#define TR_BUFFER_H_


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "tr_screen.h"


struct trace_buffer
{
   struct pipe_buffer base;

   struct pipe_buffer *buffer;

   struct tr_list list;

   void *map;
   boolean range_flushed;
};


static INLINE struct trace_buffer *
trace_buffer(struct pipe_buffer *buffer)
{
   if(!buffer)
      return NULL;
   (void)trace_screen(buffer->screen);
   return (struct trace_buffer *)buffer;
}


struct pipe_buffer *
trace_buffer_create(struct trace_screen *tr_scr,
                    struct pipe_buffer *buffer);

void
trace_buffer_destroy(struct trace_screen *tr_scr,
                     struct pipe_buffer *buffer);


#endif
