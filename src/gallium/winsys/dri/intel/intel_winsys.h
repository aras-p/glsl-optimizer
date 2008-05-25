/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef INTEL_WINSYS_H
#define INTEL_WINSYS_H

#include "pipe/p_state.h"

struct intel_context;
struct pipe_context;
struct pipe_winsys;
struct pipe_buffer;
struct _DriBufferObject;

struct pipe_winsys *
intel_create_pipe_winsys( int fd, struct _DriFreeSlabManager *fMan );

void
intel_destroy_pipe_winsys( struct pipe_winsys *winsys );

struct pipe_context *
intel_create_softpipe( struct intel_context *intel,
                       struct pipe_winsys *winsys );

struct pipe_context *
intel_create_i915simple( struct intel_context *intel,
                       struct pipe_winsys *winsys );


struct intel_buffer {
   struct pipe_buffer base;
   struct _DriBufferPool *pool;
   struct _DriBufferObject *driBO;
};

static INLINE struct intel_buffer *
intel_buffer( struct pipe_buffer *buf )
{
   return (struct intel_buffer *)buf;
}

static INLINE struct _DriBufferObject *
dri_bo( struct pipe_buffer *buf )
{
   return intel_buffer(buf)->driBO;
}


#endif
