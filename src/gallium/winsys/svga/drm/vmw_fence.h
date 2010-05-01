/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#ifndef VMW_FENCE_H_
#define VMW_FENCE_H_


#include "pipe/p_compiler.h"


struct pipe_fence_handle;
struct pb_fence_ops;
struct vmw_winsys_screen;


/** Cast from a pipe_fence_handle pointer into a SVGA fence */
static INLINE uint32_t
vmw_fence( struct pipe_fence_handle *fence )
{
   return (uint32_t)(uintptr_t)fence;
}


/** Cast from a SVGA fence number to pipe_fence_handle pointer */
static INLINE struct pipe_fence_handle *
vmw_pipe_fence( uint32_t fence )
{
   return (struct pipe_fence_handle *)(uintptr_t)fence;
}


struct pb_fence_ops *
vmw_fence_ops_create(struct vmw_winsys_screen *vws); 


#endif /* VMW_FENCE_H_ */
