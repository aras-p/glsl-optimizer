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
 * This file implements the SVGA interface into this winsys, defined
 * in drivers/svga/svga_winsys.h.
 *
 * @author Keith Whitwell
 * @author Jose Fonseca
 */


#include "svga_cmd.h"
#include "svga3d_caps.h"

#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"
#include "svga_winsys.h"
#include "vmw_context.h"
#include "vmw_screen.h"
#include "vmw_surface.h"
#include "vmw_buffer.h"
#include "vmw_fence.h"
#include "vmw_shader.h"
#include "svga3d_surfacedefs.h"

/**
 * Try to get a surface backing buffer from the cache
 * if it's this size or smaller.
 */
#define VMW_TRY_CACHED_SIZE (2*1024*1024)

static struct svga_winsys_buffer *
vmw_svga_winsys_buffer_create(struct svga_winsys_screen *sws,
                              unsigned alignment,
                              unsigned usage,
                              unsigned size)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_buffer_desc desc;
   struct pb_manager *provider;
   struct pb_buffer *buffer;

   memset(&desc, 0, sizeof desc);
   desc.pb_desc.alignment = alignment;
   desc.pb_desc.usage = usage;

   if (usage == SVGA_BUFFER_USAGE_PINNED) {
      if (vws->pools.query_fenced == NULL && !vmw_query_pools_init(vws))
	 return NULL;
      provider = vws->pools.query_fenced;
   } else if (usage == SVGA_BUFFER_USAGE_SHADER) {
      provider = vws->pools.mob_shader_slab_fenced;
   } else
      provider = vws->pools.gmr_fenced;

   assert(provider);
   buffer = provider->create_buffer(provider, size, &desc.pb_desc);

   if(!buffer && provider == vws->pools.gmr_fenced) {

      assert(provider);
      provider = vws->pools.gmr_slab_fenced;
      buffer = provider->create_buffer(provider, size, &desc.pb_desc);
   }

   if (!buffer)
      return NULL;

   return vmw_svga_winsys_buffer_wrap(buffer);
}


static void
vmw_svga_winsys_fence_reference(struct svga_winsys_screen *sws,
                                struct pipe_fence_handle **pdst,
                                struct pipe_fence_handle *src)
{
    struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

    vmw_fence_reference(vws, pdst, src);
}


static int
vmw_svga_winsys_fence_signalled(struct svga_winsys_screen *sws,
                                struct pipe_fence_handle *fence,
                                unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   return vmw_fence_signalled(vws, fence, flag);
}


static int
vmw_svga_winsys_fence_finish(struct svga_winsys_screen *sws,
                             struct pipe_fence_handle *fence,
                             unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   return vmw_fence_finish(vws, fence, flag);
}



static struct svga_winsys_surface *
vmw_svga_winsys_surface_create(struct svga_winsys_screen *sws,
                               SVGA3dSurfaceFlags flags,
                               SVGA3dSurfaceFormat format,
                               unsigned usage,
                               SVGA3dSize size,
                               uint32 numFaces,
                               uint32 numMipLevels)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_svga_winsys_surface *surface;
   struct vmw_buffer_desc desc;
   struct pb_manager *provider;
   uint32_t buffer_size;


   memset(&desc, 0, sizeof(desc));
   surface = CALLOC_STRUCT(vmw_svga_winsys_surface);
   if(!surface)
      goto no_surface;

   pipe_reference_init(&surface->refcnt, 1);
   p_atomic_set(&surface->validated, 0);
   surface->screen = vws;
   pipe_mutex_init(surface->mutex);
   surface->shared = !!(usage & SVGA_SURFACE_USAGE_SHARED);
   provider = (surface->shared) ? vws->pools.gmr : vws->pools.mob_fenced;

   /*
    * Used for the backing buffer GB surfaces, and to approximate
    * when to flush on non-GB hosts.
    */
   buffer_size = svga3dsurface_get_serialized_size(format, size, numMipLevels, (numFaces == 6));
   if (sws->have_gb_objects) {
      SVGAGuestPtr ptr = {0,0};

      /*
       * If the backing buffer size is small enough, try to allocate a
       * buffer out of the buffer cache. Otherwise, let the kernel allocate
       * a suitable buffer for us.
       */
      if (buffer_size < VMW_TRY_CACHED_SIZE && !surface->shared) {
         struct pb_buffer *pb_buf;

         surface->size = buffer_size;
         desc.pb_desc.alignment = 4096;
         desc.pb_desc.usage = 0;
         pb_buf = provider->create_buffer(provider, buffer_size, &desc.pb_desc);
         surface->buf = vmw_svga_winsys_buffer_wrap(pb_buf);
         if (surface->buf && !vmw_gmr_bufmgr_region_ptr(pb_buf, &ptr))
            assert(0);
      }

      surface->sid = vmw_ioctl_gb_surface_create(vws, flags, format, usage,
                                                 size, numFaces,
                                                 numMipLevels, ptr.gmrId,
                                                 surface->buf ? NULL :
						 &desc.region);

      if (surface->sid == SVGA3D_INVALID_ID && surface->buf) {

         /*
          * Kernel refused to allocate a surface for us.
          * Perhaps something was wrong with our buffer?
          * This is really a guard against future new size requirements
          * on the backing buffers.
          */
         vmw_svga_winsys_buffer_destroy(sws, surface->buf);
         surface->buf = NULL;
         surface->sid = vmw_ioctl_gb_surface_create(vws, flags, format, usage,
                                                    size, numFaces,
                                                    numMipLevels, 0,
                                                    &desc.region);
         if (surface->sid == SVGA3D_INVALID_ID)
            goto no_sid;
      }

      /*
       * If the kernel created the buffer for us, wrap it into a
       * vmw_svga_winsys_buffer.
       */
      if (surface->buf == NULL) {
         struct pb_buffer *pb_buf;

         surface->size = vmw_region_size(desc.region);
         desc.pb_desc.alignment = 4096;
         desc.pb_desc.usage = VMW_BUFFER_USAGE_SHARED;
         pb_buf = provider->create_buffer(provider, surface->size,
                                          &desc.pb_desc);
         surface->buf = vmw_svga_winsys_buffer_wrap(pb_buf);
         if (surface->buf == NULL) {
            vmw_ioctl_region_destroy(desc.region);
            vmw_ioctl_surface_destroy(vws, surface->sid);
            goto no_sid;
         }
      }
   } else {
      surface->sid = vmw_ioctl_surface_create(vws, flags, format, usage,
                                              size, numFaces, numMipLevels);
      if(surface->sid == SVGA3D_INVALID_ID)
         goto no_sid;

      /* Best estimate for surface size, used for early flushing. */
      surface->size = buffer_size;
      surface->buf = NULL; 
   }      

   return svga_winsys_surface(surface);

no_sid:
   if (surface->buf)
      vmw_svga_winsys_buffer_destroy(sws, surface->buf);

   FREE(surface);
no_surface:
   return NULL;
}


static boolean
vmw_svga_winsys_surface_is_flushed(struct svga_winsys_screen *sws,
                                   struct svga_winsys_surface *surface)
{
   struct vmw_svga_winsys_surface *vsurf = vmw_svga_winsys_surface(surface);
   return (p_atomic_read(&vsurf->validated) == 0);
}


static void
vmw_svga_winsys_surface_ref(struct svga_winsys_screen *sws,
			    struct svga_winsys_surface **pDst,
			    struct svga_winsys_surface *src)
{
   struct vmw_svga_winsys_surface *d_vsurf = vmw_svga_winsys_surface(*pDst);
   struct vmw_svga_winsys_surface *s_vsurf = vmw_svga_winsys_surface(src);

   vmw_svga_winsys_surface_reference(&d_vsurf, s_vsurf);
   *pDst = svga_winsys_surface(d_vsurf);
}


static void
vmw_svga_winsys_destroy(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   vmw_winsys_destroy(vws);
}


static SVGA3dHardwareVersion
vmw_svga_winsys_get_hw_version(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   if (sws->have_gb_objects)
      return SVGA3D_HWVERSION_WS8_B1;

   return (SVGA3dHardwareVersion) vws->ioctl.hwversion;
}


static boolean
vmw_svga_winsys_get_cap(struct svga_winsys_screen *sws,
                        SVGA3dDevCapIndex index,
                        SVGA3dDevCapResult *result)
{   
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   if (index > vws->ioctl.num_cap_3d || !vws->ioctl.cap_3d[index].has_cap)      
      return FALSE;

   *result = vws->ioctl.cap_3d[index].result;
   return TRUE;
}

static struct svga_winsys_gb_shader *
vmw_svga_winsys_shader_create(struct svga_winsys_screen *sws,
			      SVGA3dShaderType type,
			      const uint32 *bytecode,
			      uint32 bytecodeLen)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_svga_winsys_shader *shader;
   void *code;

   shader = CALLOC_STRUCT(vmw_svga_winsys_shader);
   if(!shader)
      goto out_no_shader;

   pipe_reference_init(&shader->refcnt, 1);
   p_atomic_set(&shader->validated, 0);
   shader->screen = vws;
   shader->buf = vmw_svga_winsys_buffer_create(sws, 64,
					       SVGA_BUFFER_USAGE_SHADER,
					       bytecodeLen);
   if (!shader->buf)
      goto out_no_buf;

   code = vmw_svga_winsys_buffer_map(sws, shader->buf, PIPE_TRANSFER_WRITE);
   if (!code)
      goto out_no_buf;

   memcpy(code, bytecode, bytecodeLen);
   vmw_svga_winsys_buffer_unmap(sws, shader->buf);

   shader->shid = vmw_ioctl_shader_create(vws, type, bytecodeLen);
   if(shader->shid == SVGA3D_INVALID_ID)
      goto out_no_shid;

   return svga_winsys_shader(shader);

out_no_shid:
   vmw_svga_winsys_buffer_destroy(sws, shader->buf);
out_no_buf:
   FREE(shader);
out_no_shader:
   return NULL;
}

static void
vmw_svga_winsys_shader_destroy(struct svga_winsys_screen *sws,
			       struct svga_winsys_gb_shader *shader)
{
   struct vmw_svga_winsys_shader *d_shader =
      vmw_svga_winsys_shader(shader);

   vmw_svga_winsys_shader_reference(&d_shader, NULL);
}

boolean
vmw_winsys_screen_init_svga(struct vmw_winsys_screen *vws)
{
   vws->base.destroy = vmw_svga_winsys_destroy;
   vws->base.get_hw_version = vmw_svga_winsys_get_hw_version;
   vws->base.get_cap = vmw_svga_winsys_get_cap;
   vws->base.context_create = vmw_svga_winsys_context_create;
   vws->base.surface_create = vmw_svga_winsys_surface_create;
   vws->base.surface_is_flushed = vmw_svga_winsys_surface_is_flushed;
   vws->base.surface_reference = vmw_svga_winsys_surface_ref;
   vws->base.buffer_create = vmw_svga_winsys_buffer_create;
   vws->base.buffer_map = vmw_svga_winsys_buffer_map;
   vws->base.buffer_unmap = vmw_svga_winsys_buffer_unmap;
   vws->base.buffer_destroy = vmw_svga_winsys_buffer_destroy;
   vws->base.fence_reference = vmw_svga_winsys_fence_reference;
   vws->base.fence_signalled = vmw_svga_winsys_fence_signalled;
   vws->base.shader_create = vmw_svga_winsys_shader_create;
   vws->base.shader_destroy = vmw_svga_winsys_shader_destroy;
   vws->base.fence_finish = vmw_svga_winsys_fence_finish;

   return TRUE;
}


