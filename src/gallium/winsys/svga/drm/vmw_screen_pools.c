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


#include "vmw_screen.h"

#include "vmw_buffer.h"
#include "vmw_fence.h"

#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"

void
vmw_pools_cleanup(struct vmw_winsys_screen *vws)
{
   if(vws->pools.gmr_fenced)
      vws->pools.gmr_fenced->destroy(vws->pools.gmr_fenced);

   /* gmr_mm pool is already destroyed above */

   if(vws->pools.gmr)
      vws->pools.gmr->destroy(vws->pools.gmr);
}


boolean
vmw_pools_init(struct vmw_winsys_screen *vws)
{
   vws->pools.gmr = vmw_gmr_bufmgr_create(vws);
   if(!vws->pools.gmr)
      goto error;

   vws->pools.gmr_mm = mm_bufmgr_create(vws->pools.gmr,
                                        VMW_GMR_POOL_SIZE,
                                        12 /* 4096 alignment */);
   if(!vws->pools.gmr_mm)
      goto error;

   /*
    * GMR buffers are typically shortlived, but it's possible that at a given
    * instance a buffer is mapped. So to avoid stalling we tell pipebuffer to
    * forbid creation of buffers beyond half the GMR pool size,
    *
    * XXX: It is unclear weather we want to limit the total amount of temporary
    * malloc memory used to backup unvalidated GMR buffers. On one hand it is
    * preferrable to fail an allocation than exhausting the guest memory with
    * temporary data, but on the other hand it is possible that a stupid
    * application creates large vertex buffers and does not use them for a long
    * time -- since the svga pipe driver only emits the DMA uploads when a
    * buffer is used for drawing this would effectively disabling swapping GMR
    * buffers to memory. So far, the preemptively flush already seems to keep
    * total allocated memory within relatively small numbers, so we don't
    * limit.
    */
   vws->pools.gmr_fenced = fenced_bufmgr_create(
      vws->pools.gmr_mm,
      vmw_fence_ops_create(vws),
      VMW_GMR_POOL_SIZE/2,
      ~0);

#ifdef DEBUG
   vws->pools.gmr_fenced = pb_debug_manager_create(vws->pools.gmr_fenced,
						   4096,
						   4096);
#endif
   if(!vws->pools.gmr_fenced)
      goto error;

   return TRUE;

error:
   vmw_pools_cleanup(vws);
   return FALSE;
}

