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
 *
 * Wrappers for DRM ioctl functionlaity used by the rest of the vmw
 * drm winsys.
 *
 * Based on svgaicd_escape.c
 */


#include "svga_cmd.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "svgadump/svga_dump.h"
#include "state_tracker/drm_driver.h"
#include "vmw_screen.h"
#include "vmw_context.h"
#include "vmw_fence.h"
#include "xf86drm.h"
#include "vmwgfx_drm.h"
#include "svga3d_caps.h"
#include "svga3d_reg.h"

#include "os/os_mman.h"

#include <errno.h>
#include <unistd.h>

struct vmw_region
{
   uint32_t handle;
   uint64_t map_handle;
   void *data;
   uint32_t map_count;
   int drm_fd;
   uint32_t size;
};

/* XXX: This isn't a real hardware flag, but just a hack for kernel to
 * know about primary surfaces. In newer versions of the kernel
 * interface the driver uses a special field.
 */
#define SVGA3D_SURFACE_HINT_SCANOUT (1 << 9)


uint32_t
vmw_region_size(struct vmw_region *region)
{
   return region->size;
}

uint32
vmw_ioctl_context_create(struct vmw_winsys_screen *vws)
{
   struct drm_vmw_context_arg c_arg;
   int ret;

   VMW_FUNC;

   ret = drmCommandRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_CONTEXT,
			&c_arg, sizeof(c_arg));

   if (ret)
      return -1;

   vmw_printf("Context id is %d\n", c_arg.cid);

   return c_arg.cid;
}

void
vmw_ioctl_context_destroy(struct vmw_winsys_screen *vws, uint32 cid)
{
   struct drm_vmw_context_arg c_arg;

   VMW_FUNC;

   memset(&c_arg, 0, sizeof(c_arg));
   c_arg.cid = cid;

   (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_CONTEXT,
			 &c_arg, sizeof(c_arg));

}

uint32
vmw_ioctl_surface_create(struct vmw_winsys_screen *vws,
                         SVGA3dSurfaceFlags flags,
                         SVGA3dSurfaceFormat format,
                         unsigned usage,
                         SVGA3dSize size,
                         uint32_t numFaces, uint32_t numMipLevels)
{
   union drm_vmw_surface_create_arg s_arg;
   struct drm_vmw_surface_create_req *req = &s_arg.req;
   struct drm_vmw_surface_arg *rep = &s_arg.rep;
   struct drm_vmw_size sizes[DRM_VMW_MAX_SURFACE_FACES*
			     DRM_VMW_MAX_MIP_LEVELS];
   struct drm_vmw_size *cur_size;
   uint32_t iFace;
   uint32_t iMipLevel;
   int ret;

   vmw_printf("%s flags %d format %d\n", __FUNCTION__, flags, format);

   memset(&s_arg, 0, sizeof(s_arg));
   if (vws->use_old_scanout_flag &&
       (flags & SVGA3D_SURFACE_HINT_SCANOUT)) {
      req->flags = (uint32_t) flags;
      req->scanout = false;
   } else if (flags & SVGA3D_SURFACE_HINT_SCANOUT) {
      req->flags = (uint32_t) (flags & ~SVGA3D_SURFACE_HINT_SCANOUT);
      req->scanout = true;
   } else {
      req->flags = (uint32_t) flags;
      req->scanout = false;
   }
   req->format = (uint32_t) format;
   req->shareable = !!(usage & SVGA_SURFACE_USAGE_SHARED);

   assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
	  DRM_VMW_MAX_MIP_LEVELS);
   cur_size = sizes;
   for (iFace = 0; iFace < numFaces; ++iFace) {
      SVGA3dSize mipSize = size;

      req->mip_levels[iFace] = numMipLevels;
      for (iMipLevel = 0; iMipLevel < numMipLevels; ++iMipLevel) {
	 cur_size->width = mipSize.width;
	 cur_size->height = mipSize.height;
	 cur_size->depth = mipSize.depth;
	 mipSize.width = MAX2(mipSize.width >> 1, 1);
	 mipSize.height = MAX2(mipSize.height >> 1, 1);
	 mipSize.depth = MAX2(mipSize.depth >> 1, 1);
	 cur_size++;
      }
   }
   for (iFace = numFaces; iFace < SVGA3D_MAX_SURFACE_FACES; ++iFace) {
      req->mip_levels[iFace] = 0;
   }

   req->size_addr = (unsigned long)&sizes;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_SURFACE,
			     &s_arg, sizeof(s_arg));

   if (ret)
      return -1;

   vmw_printf("Surface id is %d\n", rep->sid);

   return rep->sid;
}


uint32
vmw_ioctl_gb_surface_create(struct vmw_winsys_screen *vws,
			    SVGA3dSurfaceFlags flags,
			    SVGA3dSurfaceFormat format,
                            unsigned usage,
			    SVGA3dSize size,
			    uint32_t numFaces,
			    uint32_t numMipLevels,
                            uint32_t buffer_handle,
			    struct vmw_region **p_region)
{
   union drm_vmw_gb_surface_create_arg s_arg;
   struct drm_vmw_gb_surface_create_req *req = &s_arg.req;
   struct drm_vmw_gb_surface_create_rep *rep = &s_arg.rep;
   struct vmw_region *region = NULL;
   int ret;

   vmw_printf("%s flags %d format %d\n", __FUNCTION__, flags, format);

   if (p_region) {
      region = CALLOC_STRUCT(vmw_region);
      if (!region)
         return SVGA3D_INVALID_ID;
   }

   memset(&s_arg, 0, sizeof(s_arg));
   if (flags & SVGA3D_SURFACE_HINT_SCANOUT) {
      req->svga3d_flags = (uint32_t) (flags & ~SVGA3D_SURFACE_HINT_SCANOUT);
      req->drm_surface_flags = drm_vmw_surface_flag_scanout;
   } else {
      req->svga3d_flags = (uint32_t) flags;
   }
   req->format = (uint32_t) format;
   if (usage & SVGA_SURFACE_USAGE_SHARED)
      req->drm_surface_flags |= drm_vmw_surface_flag_shareable;
   req->drm_surface_flags |= drm_vmw_surface_flag_create_buffer; 

   assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
	  DRM_VMW_MAX_MIP_LEVELS);
   req->base_size.width = size.width;
   req->base_size.height = size.height;
   req->base_size.depth = size.depth;
   req->mip_levels = numMipLevels;
   req->multisample_count = 0;
   req->autogen_filter = SVGA3D_TEX_FILTER_NONE;
   if (buffer_handle)
      req->buffer_handle = buffer_handle;
   else
      req->buffer_handle = SVGA3D_INVALID_ID;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GB_SURFACE_CREATE,
			     &s_arg, sizeof(s_arg));

   if (ret)
      goto out_fail_create;

   if (p_region) {
      region->handle = rep->buffer_handle;
      region->map_handle = rep->buffer_map_handle;
      region->drm_fd = vws->ioctl.drm_fd;
      region->size = rep->backup_size;
      *p_region = region;
   }

   vmw_printf("Surface id is %d\n", rep->sid);
   return rep->handle;

out_fail_create:
   if (region)
      FREE(region);
   return SVGA3D_INVALID_ID;
}

/**
 * vmw_ioctl_surface_req - Fill in a struct surface_req
 *
 * @vws: Winsys screen
 * @whandle: Surface handle
 * @req: The struct surface req to fill in
 * @needs_unref: This call takes a kernel surface reference that needs to
 * be unreferenced.
 *
 * Returns 0 on success, negative error type otherwise.
 * Fills in the surface_req structure according to handle type and kernel
 * capabilities.
 */
static int
vmw_ioctl_surface_req(const struct vmw_winsys_screen *vws,
                      const struct winsys_handle *whandle,
                      struct drm_vmw_surface_arg *req,
                      boolean *needs_unref)
{
   int ret;

   switch(whandle->type) {
   case DRM_API_HANDLE_TYPE_SHARED:
   case DRM_API_HANDLE_TYPE_KMS:
      *needs_unref = FALSE;
      req->handle_type = DRM_VMW_HANDLE_LEGACY;
      req->sid = whandle->handle;
      break;
   case DRM_API_HANDLE_TYPE_FD:
      if (!vws->ioctl.have_drm_2_6) {
         uint32_t handle;

         ret = drmPrimeFDToHandle(vws->ioctl.drm_fd, whandle->handle, &handle);
         if (ret) {
            vmw_error("Failed to get handle from prime fd %d.\n",
                      (int) whandle->handle);
            return -EINVAL;
         }

         *needs_unref = TRUE;
         req->handle_type = DRM_VMW_HANDLE_LEGACY;
         req->sid = handle;
      } else {
         *needs_unref = FALSE;
         req->handle_type = DRM_VMW_HANDLE_PRIME;
         req->sid = whandle->handle;
      }
      break;
   default:
      vmw_error("Attempt to import unsupported handle type %d.\n",
                whandle->type);
      return -EINVAL;
   }

   return 0;
}

/**
 * vmw_ioctl_gb_surface_ref - Put a reference on a guest-backed surface and
 * get surface information
 *
 * @vws: Screen to register the reference on
 * @handle: Kernel handle of the guest-backed surface
 * @flags: flags used when the surface was created
 * @format: Format used when the surface was created
 * @numMipLevels: Number of mipmap levels of the surface
 * @p_region: On successful return points to a newly allocated
 * struct vmw_region holding a reference to the surface backup buffer.
 *
 * Returns 0 on success, a system error on failure.
 */
int
vmw_ioctl_gb_surface_ref(struct vmw_winsys_screen *vws,
                         const struct winsys_handle *whandle,
                         SVGA3dSurfaceFlags *flags,
                         SVGA3dSurfaceFormat *format,
                         uint32_t *numMipLevels,
                         uint32_t *handle,
                         struct vmw_region **p_region)
{
   union drm_vmw_gb_surface_reference_arg s_arg;
   struct drm_vmw_surface_arg *req = &s_arg.req;
   struct drm_vmw_gb_surface_ref_rep *rep = &s_arg.rep;
   struct vmw_region *region = NULL;
   boolean needs_unref = FALSE;
   int ret;

   vmw_printf("%s flags %d format %d\n", __FUNCTION__, flags, format);

   assert(p_region != NULL);
   region = CALLOC_STRUCT(vmw_region);
   if (!region)
      return -ENOMEM;

   memset(&s_arg, 0, sizeof(s_arg));
   ret = vmw_ioctl_surface_req(vws, whandle, req, &needs_unref);
   if (ret)
      goto out_fail_req;

   *handle = req->sid;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GB_SURFACE_REF,
			     &s_arg, sizeof(s_arg));

   if (ret)
      goto out_fail_ref;

   region->handle = rep->crep.buffer_handle;
   region->map_handle = rep->crep.buffer_map_handle;
   region->drm_fd = vws->ioctl.drm_fd;
   region->size = rep->crep.backup_size;
   *p_region = region;

   *handle = rep->crep.handle;
   *flags = rep->creq.svga3d_flags;
   *format = rep->creq.format;
   *numMipLevels = rep->creq.mip_levels;

   if (needs_unref)
      vmw_ioctl_surface_destroy(vws, *handle);

   return 0;
out_fail_ref:
   if (needs_unref)
      vmw_ioctl_surface_destroy(vws, *handle);
out_fail_req:
   if (region)
      FREE(region);
   return ret;
}

void
vmw_ioctl_surface_destroy(struct vmw_winsys_screen *vws, uint32 sid)
{
   struct drm_vmw_surface_arg s_arg;

   VMW_FUNC;

   memset(&s_arg, 0, sizeof(s_arg));
   s_arg.sid = sid;

   (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_SURFACE,
			 &s_arg, sizeof(s_arg));
}

void
vmw_ioctl_command(struct vmw_winsys_screen *vws, int32_t cid,
		  uint32_t throttle_us, void *commands, uint32_t size,
		  struct pipe_fence_handle **pfence)
{
   struct drm_vmw_execbuf_arg arg;
   struct drm_vmw_fence_rep rep;
   int ret;

#ifdef DEBUG
   {
      static boolean firsttime = TRUE;
      static boolean debug = FALSE;
      static boolean skip = FALSE;
      if (firsttime) {
         debug = debug_get_bool_option("SVGA_DUMP_CMD", FALSE);
         skip = debug_get_bool_option("SVGA_SKIP_CMD", FALSE);
      }
      if (debug) {
         VMW_FUNC;
         svga_dump_commands(commands, size);
      }
      firsttime = FALSE;
      if (skip) {
         size = 0;
      }
   }
#endif

   memset(&arg, 0, sizeof(arg));
   memset(&rep, 0, sizeof(rep));

   rep.error = -EFAULT;
   if (pfence)
      arg.fence_rep = (unsigned long)&rep;
   arg.commands = (unsigned long)commands;
   arg.command_size = size;
   arg.throttle_us = throttle_us;
   arg.version = DRM_VMW_EXECBUF_VERSION;

   do {
       ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_EXECBUF, &arg, sizeof(arg));
   } while(ret == -ERESTART);
   if (ret) {
      vmw_error("%s error %s.\n", __FUNCTION__, strerror(-ret));
   }

   if (rep.error) {

      /*
       * Kernel has already synced, or caller requested no fence.
       */
      if (pfence)
	 *pfence = NULL;
   } else {
      if (pfence) {
         vmw_fences_signal(vws->fence_ops, rep.passed_seqno, rep.seqno,
                           TRUE);

	 *pfence = vmw_fence_create(vws->fence_ops, rep.handle,
				    rep.seqno, rep.mask);
	 if (*pfence == NULL) {
	    /*
	     * Fence creation failed. Need to sync.
	     */
	    (void) vmw_ioctl_fence_finish(vws, rep.handle, rep.mask);
	    vmw_ioctl_fence_unref(vws, rep.handle);
	 }
      }
   }
}


struct vmw_region *
vmw_ioctl_region_create(struct vmw_winsys_screen *vws, uint32_t size)
{
   struct vmw_region *region;
   union drm_vmw_alloc_dmabuf_arg arg;
   struct drm_vmw_alloc_dmabuf_req *req = &arg.req;
   struct drm_vmw_dmabuf_rep *rep = &arg.rep;
   int ret;

   vmw_printf("%s: size = %u\n", __FUNCTION__, size);

   region = CALLOC_STRUCT(vmw_region);
   if (!region)
      goto out_err1;

   memset(&arg, 0, sizeof(arg));
   req->size = size;
   do {
      ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_ALLOC_DMABUF, &arg,
				sizeof(arg));
   } while (ret == -ERESTART);

   if (ret) {
      vmw_error("IOCTL failed %d: %s\n", ret, strerror(-ret));
      goto out_err1;
   }

   region->data = NULL;
   region->handle = rep->handle;
   region->map_handle = rep->map_handle;
   region->map_count = 0;
   region->size = size;
   region->drm_fd = vws->ioctl.drm_fd;

   vmw_printf("   gmrId = %u, offset = %u\n",
              region->ptr.gmrId, region->ptr.offset);

   return region;

 out_err1:
   FREE(region);
   return NULL;
}

void
vmw_ioctl_region_destroy(struct vmw_region *region)
{
   struct drm_vmw_unref_dmabuf_arg arg;

   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);

   if (region->data) {
      os_munmap(region->data, region->size);
      region->data = NULL;
   }

   memset(&arg, 0, sizeof(arg));
   arg.handle = region->handle;
   drmCommandWrite(region->drm_fd, DRM_VMW_UNREF_DMABUF, &arg, sizeof(arg));

   FREE(region);
}

SVGAGuestPtr
vmw_ioctl_region_ptr(struct vmw_region *region)
{
   SVGAGuestPtr ptr = {region->handle, 0};
   return ptr;
}

void *
vmw_ioctl_region_map(struct vmw_region *region)
{
   void *map;

   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);

   if (region->data == NULL) {
      map = os_mmap(NULL, region->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		 region->drm_fd, region->map_handle);
      if (map == MAP_FAILED) {
	 vmw_error("%s: Map failed.\n", __FUNCTION__);
	 return NULL;
      }

      region->data = map;
   }

   ++region->map_count;

   return region->data;
}

void
vmw_ioctl_region_unmap(struct vmw_region *region)
{
   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);
   --region->map_count;
}

/**
 * vmw_ioctl_syncforcpu - Synchronize a buffer object for CPU usage
 *
 * @region: Pointer to a struct vmw_region representing the buffer object.
 * @dont_block: Dont wait for GPU idle, but rather return -EBUSY if the
 * GPU is busy with the buffer object.
 * @readonly: Hint that the CPU access is read-only.
 * @allow_cs: Allow concurrent command submission while the buffer is
 * synchronized for CPU. If FALSE command submissions referencing the
 * buffer will block until a corresponding call to vmw_ioctl_releasefromcpu.
 *
 * This function idles any GPU activities touching the buffer and blocks
 * command submission of commands referencing the buffer, even from
 * other processes.
 */
int
vmw_ioctl_syncforcpu(struct vmw_region *region,
                     boolean dont_block,
                     boolean readonly,
                     boolean allow_cs)
{
   struct drm_vmw_synccpu_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.op = drm_vmw_synccpu_grab;
   arg.handle = region->handle;
   arg.flags = drm_vmw_synccpu_read;
   if (!readonly)
      arg.flags |= drm_vmw_synccpu_write;
   if (dont_block)
      arg.flags |= drm_vmw_synccpu_dontblock;
   if (allow_cs)
      arg.flags |= drm_vmw_synccpu_allow_cs;

   return drmCommandWrite(region->drm_fd, DRM_VMW_SYNCCPU, &arg, sizeof(arg));
}

/**
 * vmw_ioctl_releasefromcpu - Undo a previous syncforcpu.
 *
 * @region: Pointer to a struct vmw_region representing the buffer object.
 * @readonly: Should hold the same value as the matching syncforcpu call.
 * @allow_cs: Should hold the same value as the matching syncforcpu call.
 */
void
vmw_ioctl_releasefromcpu(struct vmw_region *region,
                         boolean readonly,
                         boolean allow_cs)
{
   struct drm_vmw_synccpu_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.op = drm_vmw_synccpu_release;
   arg.handle = region->handle;
   arg.flags = drm_vmw_synccpu_read;
   if (!readonly)
      arg.flags |= drm_vmw_synccpu_write;
   if (allow_cs)
      arg.flags |= drm_vmw_synccpu_allow_cs;

   (void) drmCommandWrite(region->drm_fd, DRM_VMW_SYNCCPU, &arg, sizeof(arg));
}

void
vmw_ioctl_fence_unref(struct vmw_winsys_screen *vws,
		      uint32_t handle)
{
   struct drm_vmw_fence_arg arg;
   int ret;
   
   memset(&arg, 0, sizeof(arg));
   arg.handle = handle;

   ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_FENCE_UNREF,
			 &arg, sizeof(arg));
   if (ret != 0)
      vmw_error("%s Failed\n", __FUNCTION__);
}

static INLINE uint32_t
vmw_drm_fence_flags(uint32_t flags)
{
    uint32_t dflags = 0;

    if (flags & SVGA_FENCE_FLAG_EXEC)
	dflags |= DRM_VMW_FENCE_FLAG_EXEC;
    if (flags & SVGA_FENCE_FLAG_QUERY)
	dflags |= DRM_VMW_FENCE_FLAG_QUERY;

    return dflags;
}


int
vmw_ioctl_fence_signalled(struct vmw_winsys_screen *vws,
			  uint32_t handle,
			  uint32_t flags)
{
   struct drm_vmw_fence_signaled_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
   int ret;

   memset(&arg, 0, sizeof(arg));
   arg.handle = handle;
   arg.flags = vflags;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_SIGNALED,
			     &arg, sizeof(arg));

   if (ret != 0)
      return ret;

   vmw_fences_signal(vws->fence_ops, arg.passed_seqno, 0, FALSE);

   return (arg.signaled) ? 0 : -1;
}



int
vmw_ioctl_fence_finish(struct vmw_winsys_screen *vws,
                       uint32_t handle,
		       uint32_t flags)
{
   struct drm_vmw_fence_wait_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
   int ret;

   memset(&arg, 0, sizeof(arg));

   arg.handle = handle;
   arg.timeout_us = 10*1000000;
   arg.lazy = 0;
   arg.flags = vflags;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_WAIT,
			     &arg, sizeof(arg));

   if (ret != 0)
      vmw_error("%s Failed\n", __FUNCTION__);
   
   return 0;
}

uint32
vmw_ioctl_shader_create(struct vmw_winsys_screen *vws,
			SVGA3dShaderType type,
			uint32 code_len)
{
   struct drm_vmw_shader_create_arg sh_arg;
   int ret;

   VMW_FUNC;

   memset(&sh_arg, 0, sizeof(sh_arg));

   sh_arg.size = code_len;
   sh_arg.buffer_handle = SVGA3D_INVALID_ID;
   sh_arg.shader_handle = SVGA3D_INVALID_ID;
   switch (type) {
   case SVGA3D_SHADERTYPE_VS:
      sh_arg.shader_type = drm_vmw_shader_type_vs;
      break;
   case SVGA3D_SHADERTYPE_PS:
      sh_arg.shader_type = drm_vmw_shader_type_ps;
      break;
   default:
      assert(!"Invalid shader type.");
      break;
   }

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_SHADER,
			     &sh_arg, sizeof(sh_arg));

   if (ret)
      return SVGA3D_INVALID_ID;

   return sh_arg.shader_handle;
}

void
vmw_ioctl_shader_destroy(struct vmw_winsys_screen *vws, uint32 shid)
{
   struct drm_vmw_shader_arg sh_arg;

   VMW_FUNC;

   memset(&sh_arg, 0, sizeof(sh_arg));
   sh_arg.handle = shid;

   (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_SHADER,
			 &sh_arg, sizeof(sh_arg));

}

static int
vmw_ioctl_parse_caps(struct vmw_winsys_screen *vws,
		     const uint32_t *cap_buffer)
{
   int i;

   if (vws->base.have_gb_objects) {
      for (i = 0; i < vws->ioctl.num_cap_3d; ++i) {
	 vws->ioctl.cap_3d[i].has_cap = TRUE;
	 vws->ioctl.cap_3d[i].result.u = cap_buffer[i];
      }
      return 0;
   } else {
      const uint32 *capsBlock;
      const SVGA3dCapsRecord *capsRecord = NULL;
      uint32 offset;
      const SVGA3dCapPair *capArray;
      int numCaps, index;

      /*
       * Search linearly through the caps block records for the specified type.
       */
      capsBlock = cap_buffer;
      for (offset = 0; capsBlock[offset] != 0; offset += capsBlock[offset]) {
	 const SVGA3dCapsRecord *record;
	 assert(offset < SVGA_FIFO_3D_CAPS_SIZE);
	 record = (const SVGA3dCapsRecord *) (capsBlock + offset);
	 if ((record->header.type >= SVGA3DCAPS_RECORD_DEVCAPS_MIN) &&
	     (record->header.type <= SVGA3DCAPS_RECORD_DEVCAPS_MAX) &&
	     (!capsRecord || (record->header.type > capsRecord->header.type))) {
	    capsRecord = record;
	 }
      }

      if(!capsRecord)
	 return -1;

      /*
       * Calculate the number of caps from the size of the record.
       */
      capArray = (const SVGA3dCapPair *) capsRecord->data;
      numCaps = (int) ((capsRecord->header.length * sizeof(uint32) -
			sizeof capsRecord->header) / (2 * sizeof(uint32)));

      for (i = 0; i < numCaps; i++) {
	 index = capArray[i][0];
	 if (index < vws->ioctl.num_cap_3d) {
	    vws->ioctl.cap_3d[index].has_cap = TRUE;
	    vws->ioctl.cap_3d[index].result.u = capArray[i][1];
	 } else {
	    debug_printf("Unknown devcaps seen: %d\n", index);
	 }
      }
   }
   return 0;
}

boolean
vmw_ioctl_init(struct vmw_winsys_screen *vws)
{
   struct drm_vmw_getparam_arg gp_arg;
   struct drm_vmw_get_3d_cap_arg cap_arg;
   unsigned int size;
   int ret;
   uint32_t *cap_buffer;
   drmVersionPtr version;
   boolean have_drm_2_5;

   VMW_FUNC;

   version = drmGetVersion(vws->ioctl.drm_fd);
   if (!version)
      goto out_no_version;

   have_drm_2_5 = version->version_major > 2 ||
      (version->version_major == 2 && version->version_minor > 4);
   vws->ioctl.have_drm_2_6 = version->version_major > 2 ||
      (version->version_major == 2 && version->version_minor > 5);

   memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_3D;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
			     &gp_arg, sizeof(gp_arg));
   if (ret || gp_arg.value == 0) {
      vmw_error("No 3D enabled (%i, %s).\n", ret, strerror(-ret));
      goto out_no_3d;
   }

   memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_FIFO_HW_VERSION;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
			     &gp_arg, sizeof(gp_arg));
   if (ret) {
      vmw_error("Failed to get fifo hw version (%i, %s).\n",
                ret, strerror(-ret));
      goto out_no_3d;
   }
   vws->ioctl.hwversion = gp_arg.value;

   memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_HW_CAPS;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
                             &gp_arg, sizeof(gp_arg));
   if (ret)
      vws->base.have_gb_objects = FALSE;
   else
      vws->base.have_gb_objects =
         !!(gp_arg.value & (uint64_t) SVGA_CAP_GBOBJECTS);
   
   if (vws->base.have_gb_objects && !have_drm_2_5)
      goto out_no_3d;

   if (vws->base.have_gb_objects) {
      memset(&gp_arg, 0, sizeof(gp_arg));
      gp_arg.param = DRM_VMW_PARAM_3D_CAPS_SIZE;
      ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
                                &gp_arg, sizeof(gp_arg));
      if (ret)
         size = SVGA_FIFO_3D_CAPS_SIZE * sizeof(uint32_t);
      else
         size = gp_arg.value;
   
      if (vws->base.have_gb_objects)
         vws->ioctl.num_cap_3d = size / sizeof(uint32_t);
      else
         vws->ioctl.num_cap_3d = SVGA3D_DEVCAP_MAX;


      memset(&gp_arg, 0, sizeof(gp_arg));
      gp_arg.param = DRM_VMW_PARAM_MAX_MOB_MEMORY;
      ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
                                &gp_arg, sizeof(gp_arg));
      if (ret) {
         /* Just guess a large enough value. */
         vws->ioctl.max_mob_memory = 256*1024*1024;
      } else {
         vws->ioctl.max_mob_memory = gp_arg.value;
      }
      /* Never early flush surfaces, mobs do accounting. */
      vws->ioctl.max_surface_memory = -1;
   } else {
      vws->ioctl.num_cap_3d = SVGA3D_DEVCAP_MAX;

      memset(&gp_arg, 0, sizeof(gp_arg));
      gp_arg.param = DRM_VMW_PARAM_MAX_SURF_MEMORY;
      if (have_drm_2_5)
         ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
                                   &gp_arg, sizeof(gp_arg));
      if (!have_drm_2_5 || ret) {
         /* Just guess a large enough value, around 800mb. */
         vws->ioctl.max_surface_memory = 0x30000000;
      } else {
         vws->ioctl.max_surface_memory = gp_arg.value;
      }
      size = SVGA_FIFO_3D_CAPS_SIZE * sizeof(uint32_t);
   }

   cap_buffer = calloc(1, size);
   if (!cap_buffer) {
      debug_printf("Failed alloc fifo 3D caps buffer.\n");
      goto out_no_3d;
   }

   vws->ioctl.cap_3d = calloc(vws->ioctl.num_cap_3d, 
			      sizeof(*vws->ioctl.cap_3d));
   if (!vws->ioctl.cap_3d) {
      debug_printf("Failed alloc fifo 3D caps buffer.\n");
      goto out_no_caparray;
   }
      
   memset(&cap_arg, 0, sizeof(cap_arg));
   cap_arg.buffer = (uint64_t) (unsigned long) (cap_buffer);
   cap_arg.max_size = size;

   ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_GET_3D_CAP,
			 &cap_arg, sizeof(cap_arg));

   if (ret) {
      debug_printf("Failed to get 3D capabilities"
		   " (%i, %s).\n", ret, strerror(-ret));
      goto out_no_caps;
   }

   ret = vmw_ioctl_parse_caps(vws, cap_buffer);
   if (ret) {
      debug_printf("Failed to parse 3D capabilities"
		   " (%i, %s).\n", ret, strerror(-ret));
      goto out_no_caps;
   }
   free(cap_buffer);
   drmFreeVersion(version);
   vmw_printf("%s OK\n", __FUNCTION__);
   return TRUE;
  out_no_caps:
   free(vws->ioctl.cap_3d);
  out_no_caparray:
   free(cap_buffer);
  out_no_3d:
   drmFreeVersion(version);
  out_no_version:
   vws->ioctl.num_cap_3d = 0;
   debug_printf("%s Failed\n", __FUNCTION__);
   return FALSE;
}



void
vmw_ioctl_cleanup(struct vmw_winsys_screen *vws)
{
   VMW_FUNC;
}
