/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/
/*
 * Authors: Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include <stdlib.h>
#include "i915_context.h"
#include "i915_winsys.h"



/* Most callbacks map direcly onto winsys operations at the moment,
 * but this may change, especially as state_trackers and winsys's
 * evolve in separate directions...  Don't try and remove this yet.
 */
static struct pipe_buffer_handle *
i915_buffer_create(struct pipe_context *pipe, 
		 unsigned alignment,
		 unsigned flags)
{
   struct i915_context *i915 = i915_context( pipe );
   return i915->winsys->buffer_create( i915->winsys, alignment );
}

static void *i915_buffer_map(struct pipe_context *pipe, 
			     struct pipe_buffer_handle *buf,
			     unsigned flags )
{
   struct i915_context *i915 = i915_context( pipe );
   return i915->winsys->buffer_map( i915->winsys, buf );
}

static void i915_buffer_unmap(struct pipe_context *pipe, 
			    struct pipe_buffer_handle *buf)
{
   struct i915_context *i915 = i915_context( pipe );
   i915->winsys->buffer_unmap( i915->winsys, buf );
}

static struct pipe_buffer_handle *
i915_buffer_reference(struct pipe_context *pipe,
		    struct pipe_buffer_handle *buf)
{
   struct i915_context *i915 = i915_context( pipe );
   return i915->winsys->buffer_reference( i915->winsys, buf );
}

static void i915_buffer_unreference(struct pipe_context *pipe, 
				  struct pipe_buffer_handle **buf)
{
   struct i915_context *i915 = i915_context( pipe );
   i915->winsys->buffer_unreference( i915->winsys, buf );
}

static void i915_buffer_data(struct pipe_context *pipe, 
			   struct pipe_buffer_handle *buf,
			   unsigned size, const void *data )
{
   struct i915_context *i915 = i915_context( pipe );
   i915->winsys->buffer_data( i915->winsys, buf, size, data );
}

static void i915_buffer_subdata(struct pipe_context *pipe, 
				 struct pipe_buffer_handle *buf,
				 unsigned long offset, 
				 unsigned long size, 
				 const void *data)
{
   struct i915_context *i915 = i915_context( pipe );
   i915->winsys->buffer_subdata( i915->winsys, buf, offset, size, data );
}

static void i915_buffer_get_subdata(struct pipe_context *pipe, 
				  struct pipe_buffer_handle *buf,
				  unsigned long offset, 
				  unsigned long size, 
				  void *data)
{
   struct i915_context *i915 = i915_context( pipe );
   i915->winsys->buffer_get_subdata( i915->winsys, buf, offset, size, data );
}


void
i915_init_buffer_functions( struct i915_context *i915 )
{
   i915->pipe.create_buffer = i915_buffer_create;
   i915->pipe.buffer_map = i915_buffer_map;
   i915->pipe.buffer_unmap = i915_buffer_unmap;
   i915->pipe.buffer_reference = i915_buffer_reference;
   i915->pipe.buffer_unreference = i915_buffer_unreference;
   i915->pipe.buffer_data = i915_buffer_data;
   i915->pipe.buffer_subdata = i915_buffer_subdata;
   i915->pipe.buffer_get_subdata = i915_buffer_get_subdata;
}
