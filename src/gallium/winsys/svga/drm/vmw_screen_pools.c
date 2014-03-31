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

/**
 * vmw_pools_cleanup - Destroy the buffer pools.
 *
 * @vws: pointer to a struct vmw_winsys_screen.
 */
void
vmw_pools_cleanup(struct vmw_winsys_screen *vws)
{
   if (vws->pools.mob_shader_slab_fenced)
      vws->pools.mob_shader_slab_fenced->destroy
         (vws->pools.mob_shader_slab_fenced);
   if (vws->pools.mob_shader_slab)
      vws->pools.mob_shader_slab->destroy(vws->pools.mob_shader_slab);
   if (vws->pools.mob_fenced)
      vws->pools.mob_fenced->destroy(vws->pools.mob_fenced);
   if (vws->pools.mob_cache)
      vws->pools.mob_cache->destroy(vws->pools.mob_cache);

   if (vws->pools.query_fenced)
      vws->pools.query_fenced->destroy(vws->pools.query_fenced);
   if (vws->pools.query_mm)
      vws->pools.query_mm->destroy(vws->pools.query_mm);

   if(vws->pools.gmr_fenced)
      vws->pools.gmr_fenced->destroy(vws->pools.gmr_fenced);
   if (vws->pools.gmr_mm)
      vws->pools.gmr_mm->destroy(vws->pools.gmr_mm);
   if (vws->pools.gmr_slab_fenced)
      vws->pools.gmr_slab_fenced->destroy(vws->pools.gmr_slab_fenced);
   if (vws->pools.gmr_slab)
      vws->pools.gmr_slab->destroy(vws->pools.gmr_slab);

   if(vws->pools.gmr)
      vws->pools.gmr->destroy(vws->pools.gmr);
}


/**
 * vmw_query_pools_init - Create a pool of query buffers.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 *
 * Typically this pool should be created on demand when we
 * detect that the app will be using queries. There's nothing
 * special with this pool other than the backing kernel buffer sizes,
 * which are limited to 8192.
 * If there is a performance issue with allocation and freeing of the
 * query slabs, it should be easily fixable by allocating them out
 * of a buffer cache.
 */
boolean
vmw_query_pools_init(struct vmw_winsys_screen *vws)
{
   struct pb_desc desc;

   desc.alignment = 16;
   desc.usage = ~(VMW_BUFFER_USAGE_SHARED | VMW_BUFFER_USAGE_SYNC);

   vws->pools.query_mm = pb_slab_range_manager_create(vws->pools.gmr, 16, 128,
                                                      VMW_QUERY_POOL_SIZE,
                                                      &desc);
   if (!vws->pools.query_mm)
      return FALSE;

   vws->pools.query_fenced = simple_fenced_bufmgr_create(
      vws->pools.query_mm, vws->fence_ops);

   if(!vws->pools.query_fenced)
      goto out_no_query_fenced;

   return TRUE;

  out_no_query_fenced:
   vws->pools.query_mm->destroy(vws->pools.query_mm);
   return FALSE;
}

/**
 * vmw_mob_pool_init - Create a pool of fenced kernel buffers.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 *
 * Typically this pool should be created on demand when we
 * detect that the app will be using MOB buffers.
 */
boolean
vmw_mob_pools_init(struct vmw_winsys_screen *vws)
{
   struct pb_desc desc;

   vws->pools.mob_cache = 
      pb_cache_manager_create(vws->pools.gmr, 100000, 2.0f,
                              VMW_BUFFER_USAGE_SHARED);
   if (!vws->pools.mob_cache)
      return FALSE;

   vws->pools.mob_fenced = 
      simple_fenced_bufmgr_create(vws->pools.mob_cache,
                                  vws->fence_ops);
   if(!vws->pools.mob_fenced)
      goto out_no_mob_fenced;
   
   desc.alignment = 64;
   desc.usage = ~(SVGA_BUFFER_USAGE_PINNED | VMW_BUFFER_USAGE_SHARED |
                  VMW_BUFFER_USAGE_SYNC);
   vws->pools.mob_shader_slab =
      pb_slab_range_manager_create(vws->pools.mob_cache,
                                   64,
                                   8192,
                                   16384,
                                   &desc);
   if(!vws->pools.mob_shader_slab)
      goto out_no_mob_shader_slab;

   vws->pools.mob_shader_slab_fenced =
      simple_fenced_bufmgr_create(vws->pools.mob_shader_slab,
				  vws->fence_ops);
   if(!vws->pools.mob_fenced)
      goto out_no_mob_shader_slab_fenced;

   return TRUE;

 out_no_mob_shader_slab_fenced:
   vws->pools.mob_shader_slab->destroy(vws->pools.mob_shader_slab);
 out_no_mob_shader_slab:
   vws->pools.mob_fenced->destroy(vws->pools.mob_fenced);
 out_no_mob_fenced:
   vws->pools.mob_cache->destroy(vws->pools.mob_cache);
   return FALSE;
}

/**
 * vmw_pools_init - Create a pool of GMR buffers.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 */
boolean
vmw_pools_init(struct vmw_winsys_screen *vws)
{
   struct pb_desc desc;

   vws->pools.gmr = vmw_gmr_bufmgr_create(vws);
   if(!vws->pools.gmr)
      goto error;

   if ((vws->base.have_gb_objects && vws->base.have_gb_dma) ||
       !vws->base.have_gb_objects) {
      /*
       * A managed pool for DMA buffers.
       */
      vws->pools.gmr_mm = mm_bufmgr_create(vws->pools.gmr,
                                           VMW_GMR_POOL_SIZE,
                                           12 /* 4096 alignment */);
      if(!vws->pools.gmr_mm)
         goto error;

      vws->pools.gmr_fenced = simple_fenced_bufmgr_create
         (vws->pools.gmr_mm, vws->fence_ops);

#ifdef DEBUG
      vws->pools.gmr_fenced = pb_debug_manager_create(vws->pools.gmr_fenced,
                                                      4096,
                                                      4096);
#endif
      if(!vws->pools.gmr_fenced)
         goto error;

   /*
    * The slab pool allocates buffers directly from the kernel except
    * for very small buffers which are allocated from a slab in order
    * not to waste memory, since a kernel buffer is a minimum 4096 bytes.
    *
    * Here we use it only for emergency in the case our pre-allocated
    * managed buffer pool runs out of memory.
    */

      desc.alignment = 64;
      desc.usage = ~(SVGA_BUFFER_USAGE_PINNED | SVGA_BUFFER_USAGE_SHADER |
                     VMW_BUFFER_USAGE_SHARED | VMW_BUFFER_USAGE_SYNC);
      vws->pools.gmr_slab = pb_slab_range_manager_create(vws->pools.gmr,
                                                         64,
                                                         8192,
                                                         16384,
                                                         &desc);
      if (!vws->pools.gmr_slab)
         goto error;

      vws->pools.gmr_slab_fenced =
         simple_fenced_bufmgr_create(vws->pools.gmr_slab, vws->fence_ops);

      if (!vws->pools.gmr_slab_fenced)
         goto error;
   }

   vws->pools.query_fenced = NULL;
   vws->pools.query_mm = NULL;
   vws->pools.mob_cache = NULL;

   if (vws->base.have_gb_objects && !vmw_mob_pools_init(vws))
      goto error;

   return TRUE;

error:
   vmw_pools_cleanup(vws);
   return FALSE;
}
