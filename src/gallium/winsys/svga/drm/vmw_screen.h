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

/**
 * @file
 * Common definitions for the VMware SVGA winsys.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef VMW_SCREEN_H_
#define VMW_SCREEN_H_


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "svga_winsys.h"


#define VMW_GMR_POOL_SIZE (16*1024*1024)


struct pb_manager;
struct vmw_region;


struct vmw_winsys_screen
{
   struct svga_winsys_screen base;

   boolean use_old_scanout_flag;

   struct {
      volatile uint32_t *fifo_map;
      uint64_t last_fence;
      int drm_fd;
   } ioctl;

   struct {
      struct pb_manager *gmr;
      struct pb_manager *gmr_mm;
      struct pb_manager *gmr_fenced;
   } pools;
};


static INLINE struct vmw_winsys_screen *
vmw_winsys_screen(struct svga_winsys_screen *base)
{
   return (struct vmw_winsys_screen *)base;
}

/*  */
uint32
vmw_ioctl_context_create(struct vmw_winsys_screen *vws);

void
vmw_ioctl_context_destroy(struct vmw_winsys_screen *vws,
                          uint32 cid);

uint32
vmw_ioctl_surface_create(struct vmw_winsys_screen *vws,
                              SVGA3dSurfaceFlags flags,
                              SVGA3dSurfaceFormat format,
                              SVGA3dSize size,
                              uint32 numFaces,
                              uint32 numMipLevels);

void
vmw_ioctl_surface_destroy(struct vmw_winsys_screen *vws,
                          uint32 sid);

void
vmw_ioctl_command(struct vmw_winsys_screen *vws,
                       void *commands,
                       uint32_t size,
                       uint32_t *fence);

struct vmw_region *
vmw_ioctl_region_create(struct vmw_winsys_screen *vws, uint32_t size);

void
vmw_ioctl_region_destroy(struct vmw_region *region);

struct SVGAGuestPtr
vmw_ioctl_region_ptr(struct vmw_region *region);

void *
vmw_ioctl_region_map(struct vmw_region *region);
void
vmw_ioctl_region_unmap(struct vmw_region *region);


int
vmw_ioctl_fence_finish(struct vmw_winsys_screen *vws,
                       uint32_t fence);

int
vmw_ioctl_fence_signalled(struct vmw_winsys_screen *vws,
                          uint32_t fence);


/* Initialize parts of vmw_winsys_screen at startup:
 */
boolean vmw_ioctl_init(struct vmw_winsys_screen *vws);
boolean vmw_pools_init(struct vmw_winsys_screen *vws);
boolean vmw_winsys_screen_init_svga(struct vmw_winsys_screen *vws);

void vmw_ioctl_cleanup(struct vmw_winsys_screen *vws);
void vmw_pools_cleanup(struct vmw_winsys_screen *vws);

struct vmw_winsys_screen *vmw_winsys_create(int fd, boolean use_old_scanout_flag);
void vmw_winsys_destroy(struct vmw_winsys_screen *sws);


#endif /* VMW_SCREEN_H_ */
