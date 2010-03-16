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


#include "util/u_memory.h"
#include "pipebuffer/pb_buffer_fenced.h"

#include "vmw_screen.h"
#include "vmw_fence.h"



struct vmw_fence_ops 
{
   struct pb_fence_ops base;

   struct vmw_winsys_screen *vws;
};


static INLINE struct vmw_fence_ops *
vmw_fence_ops(struct pb_fence_ops *ops)
{
   assert(ops);
   return (struct vmw_fence_ops *)ops;
}


static void
vmw_fence_ops_fence_reference(struct pb_fence_ops *ops,
                              struct pipe_fence_handle **ptr,
                              struct pipe_fence_handle *fence)
{
   *ptr = fence;
}


static int
vmw_fence_ops_fence_signalled(struct pb_fence_ops *ops,
                              struct pipe_fence_handle *fence,
                              unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_fence_ops(ops)->vws;
   (void)flag;
   return vmw_ioctl_fence_signalled(vws, vmw_fence(fence));
}


static int
vmw_fence_ops_fence_finish(struct pb_fence_ops *ops,
                           struct pipe_fence_handle *fence,
                           unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_fence_ops(ops)->vws;
   (void)flag;
   return vmw_ioctl_fence_finish(vws, vmw_fence(fence));
}


static void
vmw_fence_ops_destroy(struct pb_fence_ops *ops)
{
   FREE(ops);
}


struct pb_fence_ops *
vmw_fence_ops_create(struct vmw_winsys_screen *vws) 
{
   struct vmw_fence_ops *ops;

   ops = CALLOC_STRUCT(vmw_fence_ops);
   if(!ops)
      return NULL;

   ops->base.destroy = &vmw_fence_ops_destroy;
   ops->base.fence_reference = &vmw_fence_ops_fence_reference;
   ops->base.fence_signalled = &vmw_fence_ops_fence_signalled;
   ops->base.fence_finish = &vmw_fence_ops_fence_finish;

   ops->vws = vws;

   return &ops->base;
}


