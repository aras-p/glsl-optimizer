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

#ifndef TR_WINSYS_H_
#define TR_WINSYS_H_


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_winsys.h"


/**
 * It often happens that new data is written directly to the user buffers 
 * without mapping/unmapping. This flag marks user buffers, so that their 
 * contents can be dumpped before being used by the pipe context.
 */
#define TRACE_BUFFER_USAGE_USER  (1 << 31)


struct hash_table;


struct trace_winsys
{
   struct pipe_winsys base;
   
   struct pipe_winsys *winsys;
   
   struct hash_table *buffer_maps;
};


static INLINE struct trace_winsys *
trace_winsys(struct pipe_winsys *winsys)
{
   assert(winsys);
   return (struct trace_winsys *)winsys;
}



struct pipe_winsys *
trace_winsys_create(struct pipe_winsys *winsys);


void
trace_winsys_user_buffer_update(struct pipe_winsys *winsys, 
                                struct pipe_buffer *buffer);


#endif /* TR_WINSYS_H_ */
