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
#include <xf86drm.h>
//#include "dri_bufpool.h"
//#include "dri_bufmgr.h"

#include "intel_context.h"
#include "intel_winsys.h"
#include "intel_swapbuffers.h"
#include "intel_batchbuffer.h"

#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"



struct intel_pipe_winsys {
   struct pipe_winsys winsys;
   struct _DriBufferPool *regionPool;
   struct _DriFreeSlabManager *fMan;
};



/* Turn a pipe winsys into an intel/pipe winsys:
 */
static inline struct intel_pipe_winsys *
intel_pipe_winsys( struct pipe_winsys *winsys )
{
   return (struct intel_pipe_winsys *)winsys;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *intel_buffer_map(struct pipe_winsys *winsys, 
			      struct pipe_buffer *buf,
			      unsigned flags )
{
   unsigned drm_flags = 0;
   
   if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
      drm_flags |= DRM_BO_FLAG_WRITE;

   if (flags & PIPE_BUFFER_USAGE_CPU_READ)
      drm_flags |= DRM_BO_FLAG_READ;

   return driBOMap( dri_bo(buf), drm_flags, 0 );
}

static void intel_buffer_unmap(struct pipe_winsys *winsys, 
			       struct pipe_buffer *buf)
{
   driBOUnmap( dri_bo(buf) );
}


static void
intel_buffer_destroy(struct pipe_winsys *winsys,
		     struct pipe_buffer *buf)
{
   driBOUnReference( dri_bo(buf) );
}


/* Pipe has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 * Grabs the hardware lock!
 */
static struct pipe_buffer *
intel_buffer_create(struct pipe_winsys *winsys, 
                    unsigned alignment, 
                    unsigned usage, 
                    unsigned size )
{
   struct intel_buffer *buffer = CALLOC_STRUCT( intel_buffer );
   struct intel_pipe_winsys *iws = intel_pipe_winsys(winsys);
   unsigned flags = 0;

   buffer->base.refcount = 1;
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;

   if (usage & (PIPE_BUFFER_USAGE_VERTEX /*| IWS_BUFFER_USAGE_LOCAL*/)) {
      flags |= DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_CACHED;
   } else {
      flags |= DRM_BO_FLAG_MEM_VRAM | DRM_BO_FLAG_MEM_TT;
   }

   if (usage & PIPE_BUFFER_USAGE_GPU_READ)
      flags |= DRM_BO_FLAG_READ;

   if (usage & PIPE_BUFFER_USAGE_GPU_WRITE)
      flags |= DRM_BO_FLAG_WRITE;

   /* drm complains if we don't set any read/write flags.
    */
   if ((flags & (DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE)) == 0)
      flags |= DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE;

#if 0
   if (flags & IWS_BUFFER_USAGE_EXE)
      flags |= DRM_BO_FLAG_EXE;

   if (usage & IWS_BUFFER_USAGE_CACHED)
      flags |= DRM_BO_FLAG_CACHED;
#endif

   driGenBuffers( iws->regionPool, 
		  "pipe buffer", 1, &buffer->driBO, alignment, flags, 0 );

   driBOData( buffer->driBO, size, NULL, iws->regionPool, 0 );

   return &buffer->base;
}


static struct pipe_buffer *
intel_user_buffer_create(struct pipe_winsys *winsys, void *ptr, unsigned bytes)
{
   struct intel_buffer *buffer = CALLOC_STRUCT( intel_buffer );
   struct intel_pipe_winsys *iws = intel_pipe_winsys(winsys);

   driGenUserBuffer( iws->regionPool, 
                     "pipe user buffer", &buffer->driBO, ptr, bytes );

   return &buffer->base;
}


/* The state tracker (should!) keep track of whether the fake
 * frontbuffer has been touched by any rendering since the last time
 * we copied its contents to the real frontbuffer.  Our task is easy:
 */
static void
intel_flush_frontbuffer( struct pipe_winsys *winsys,
                         struct pipe_surface *surf,
                         void *context_private)
{
   struct intel_context *intel = (struct intel_context *) context_private;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   intelDisplaySurface(dPriv, surf, NULL);
}


static struct pipe_surface *
intel_i915_surface_alloc(struct pipe_winsys *winsys)
{
   struct pipe_surface *surf = CALLOC_STRUCT(pipe_surface);
   if (surf) {
      surf->refcount = 1;
      surf->winsys = winsys;
   }
   return surf;
}


/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}

/**
 * Copied from xm_winsys.c
 */
static int
intel_i915_surface_alloc_storage(struct pipe_winsys *winsys,
                                 struct pipe_surface *surf,
                                 unsigned width, unsigned height,
                                 enum pipe_format format, 
                                 unsigned flags)
{
   const unsigned alignment = 64;
   //int ret;

   surf->width = width;
   surf->height = height;
   surf->format = format;
   surf->cpp = pf_get_size(format);
   surf->pitch = round_up(width, alignment / surf->cpp);

   assert(!surf->buffer);
   surf->buffer = winsys->buffer_create(winsys, alignment,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        surf->pitch * surf->cpp * height);
   if(!surf->buffer)
      return -1;

   return 0;
}


static void
intel_i915_surface_release(struct pipe_winsys *winsys, struct pipe_surface **s)
{
   struct pipe_surface *surf = *s;
   surf->refcount--;
   if (surf->refcount == 0) {
      if (surf->buffer)
	 pipe_buffer_reference(winsys, &surf->buffer, NULL);
      free(surf);
   }
   *s = NULL;
}



static const char *
intel_get_name( struct pipe_winsys *winsys )
{
   return "Intel/DRI/ttm";
}

static void
intel_fence_reference( struct pipe_winsys *sws,
                       struct pipe_fence_handle **ptr,
                       struct pipe_fence_handle *fence )
{
   if (*ptr)
      driFenceUnReference((struct _DriFenceObject **)ptr);

   if (fence)
      *ptr = (struct pipe_fence_handle *)driFenceReference((struct _DriFenceObject *)fence);
}

static int
intel_fence_signalled( struct pipe_winsys *sws,
                       struct pipe_fence_handle *fence,
                       unsigned flag )
{
   return driFenceSignaled((struct _DriFenceObject *)fence, flag);
}

static int
intel_fence_finish( struct pipe_winsys *sws,
                    struct pipe_fence_handle *fence,
                    unsigned flag )
{
   /* JB: Lets allways lazy wait */
   return driFenceFinish((struct _DriFenceObject *)fence, flag, 1);
}

struct pipe_winsys *
intel_create_pipe_winsys( int fd, struct _DriFreeSlabManager *fMan )
{
   struct intel_pipe_winsys *iws = CALLOC_STRUCT( intel_pipe_winsys );
   
   /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    *
    * Pipe would be happy with a malloc based memory manager, but
    * the SwapBuffers implementation in this winsys driver requires
    * that rendering be done to an appropriate _DriBufferObject.  
    */
   iws->winsys.buffer_create = intel_buffer_create;
   iws->winsys.user_buffer_create = intel_user_buffer_create;
   iws->winsys.buffer_map = intel_buffer_map;
   iws->winsys.buffer_unmap = intel_buffer_unmap;
   iws->winsys.buffer_destroy = intel_buffer_destroy;
   iws->winsys.flush_frontbuffer = intel_flush_frontbuffer;
   iws->winsys.get_name = intel_get_name;
   iws->winsys.surface_alloc = intel_i915_surface_alloc;
   iws->winsys.surface_alloc_storage = intel_i915_surface_alloc_storage;
   iws->winsys.surface_release = intel_i915_surface_release;

   iws->winsys.fence_reference = intel_fence_reference;
   iws->winsys.fence_signalled = intel_fence_signalled;
   iws->winsys.fence_finish = intel_fence_finish;

   if (fd)
      iws->regionPool = driSlabPoolInit(fd,
			DRM_BO_FLAG_READ |
			DRM_BO_FLAG_WRITE |
			DRM_BO_FLAG_MEM_TT,
			DRM_BO_FLAG_READ |
			DRM_BO_FLAG_WRITE |
			DRM_BO_FLAG_MEM_TT,
			64, 6, 16, 4096, 0,
			fMan);

   return &iws->winsys;
}


void
intel_destroy_pipe_winsys( struct pipe_winsys *winsys )
{
   struct intel_pipe_winsys *iws = intel_pipe_winsys(winsys);
   if (iws->regionPool) {
      driPoolTakeDown(iws->regionPool);
   }
   free(iws);
}

