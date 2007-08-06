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
#include "sp_context.h"
#include "sp_winsys.h"



/* Most callbacks map direcly onto winsys operations:
 */
static struct pipe_buffer_handle *
sp_create_buffer(struct pipe_context *pipe, 
		 unsigned alignment,
		 unsigned flags)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   return sp->winsys->create_buffer( sp->winsys, alignment );
}

static void *sp_buffer_map(struct pipe_context *pipe, 
			   struct pipe_buffer_handle *buf,
			   unsigned flags )
{
   struct softpipe_context *sp = softpipe_context( pipe );
   return sp->winsys->buffer_map( sp->winsys, buf );
}

static void sp_buffer_unmap(struct pipe_context *pipe, 
			    struct pipe_buffer_handle *buf)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   sp->winsys->buffer_unmap( sp->winsys, buf );
}

static struct pipe_buffer_handle *
sp_buffer_reference(struct pipe_context *pipe,
		    struct pipe_buffer_handle *buf)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   return sp->winsys->buffer_reference( sp->winsys, buf );
}

static void sp_buffer_unreference(struct pipe_context *pipe, 
				  struct pipe_buffer_handle **buf)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   sp->winsys->buffer_unreference( sp->winsys, buf );
}

static void sp_buffer_data(struct pipe_context *pipe, 
			   struct pipe_buffer_handle *buf,
			   unsigned size, const void *data )
{
   struct softpipe_context *sp = softpipe_context( pipe );
   sp->winsys->buffer_data( sp->winsys, buf, size, data );
}

static void sp_buffer_subdata(struct pipe_context *pipe, 
				 struct pipe_buffer_handle *buf,
				 unsigned long offset, 
				 unsigned long size, 
				 const void *data)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   sp->winsys->buffer_subdata( sp->winsys, buf, offset, size, data );
}

static void sp_buffer_get_subdata(struct pipe_context *pipe, 
				  struct pipe_buffer_handle *buf,
				  unsigned long offset, 
				  unsigned long size, 
				  void *data)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   sp->winsys->buffer_get_subdata( sp->winsys, buf, offset, size, data );
}


void
sp_init_buffer_functions( struct softpipe_context *sp )
{
   sp->pipe.create_buffer = sp_create_buffer;
   sp->pipe.buffer_map = sp_buffer_map;
   sp->pipe.buffer_unmap = sp_buffer_unmap;
   sp->pipe.buffer_reference = sp_buffer_reference;
   sp->pipe.buffer_unreference = sp_buffer_unreference;
   sp->pipe.buffer_data = sp_buffer_data;
   sp->pipe.buffer_subdata = sp_buffer_subdata;
   sp->pipe.buffer_get_subdata = sp_buffer_get_subdata;
}
