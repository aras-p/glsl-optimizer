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
#include "dri_bufpool.h"
#include "dri_bufmgr.h"

#include "intel_context.h"
#include "intel_winsys.h"
#include "intel_swapbuffers.h"
#include "intel_batchbuffer.h"

#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"



struct intel_pipe_winsys {
   struct pipe_winsys winsys;
   struct intel_context *intel;
};



/* Turn a pipe winsys into an intel/pipe winsys:
 */
static inline struct intel_pipe_winsys *
intel_pipe_winsys( struct pipe_winsys *sws )
{
   return (struct intel_pipe_winsys *)sws;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *intel_buffer_map(struct pipe_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned flags )
{
   unsigned drm_flags = 0;
   
   if (flags & PIPE_BUFFER_FLAG_WRITE)
      drm_flags |= DRM_BO_FLAG_WRITE;

   if (flags & PIPE_BUFFER_FLAG_READ)
      drm_flags |= DRM_BO_FLAG_READ;

   return driBOMap( dri_bo(buf), drm_flags, 0 );
}

static void intel_buffer_unmap(struct pipe_winsys *sws, 
			       struct pipe_buffer_handle *buf)
{
   driBOUnmap( dri_bo(buf) );
}


static void
intel_buffer_reference(struct pipe_winsys *sws,
		       struct pipe_buffer_handle **ptr,
		       struct pipe_buffer_handle *buf)
{
   if (*ptr) {
      driBOUnReference( dri_bo(*ptr) );
      *buf = NULL;
   }

   driBOReference( dri_bo(buf) );
   *ptr = buf;
}


/* Grabs the hardware lock!
 */
static void intel_buffer_data(struct pipe_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned size, const void *data )
{
   struct intel_context *intel = intel_pipe_winsys(sws)->intel;

   LOCK_HARDWARE( intel );
   driBOData( dri_bo(buf), size, data, 0 );
   UNLOCK_HARDWARE( intel );
}

static void intel_buffer_subdata(struct pipe_winsys *sws, 
				 struct pipe_buffer_handle *buf,
				 unsigned long offset, 
				 unsigned long size, 
				 const void *data)
{
   driBOSubData( dri_bo(buf), offset, size, data );
}

static void intel_buffer_get_subdata(struct pipe_winsys *sws, 
				     struct pipe_buffer_handle *buf,
				     unsigned long offset, 
				     unsigned long size, 
				     void *data)
{
   driBOGetSubData( dri_bo(buf), offset, size, data );
}

/* Pipe has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 */
static struct pipe_buffer_handle *
intel_buffer_create(struct pipe_winsys *sws, 
		    unsigned alignment)
{
   struct intel_context *intel = intel_pipe_winsys(sws)->intel;
   struct _DriBufferObject *buffer;

   LOCK_HARDWARE( intel );
   driGenBuffers( intel->intelScreen->regionPool, 
		  "pipe buffer", 1, &buffer, alignment, 0, 0 );
   UNLOCK_HARDWARE( intel );

   return pipe_bo(buffer);
}


static void intel_wait_idle( struct pipe_winsys *sws )
{
   struct intel_context *intel = intel_pipe_winsys(sws)->intel;

   if (intel->batch->last_fence) {
      driFenceFinish(intel->batch->last_fence, 
		     DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, GL_FALSE);
      driFenceUnReference(intel->batch->last_fence);
      intel->batch->last_fence = NULL;
   }
}


/* The state tracker (should!) keep track of whether the fake
 * frontbuffer has been touched by any rendering since the last time
 * we copied its contents to the real frontbuffer.  Our task is easy:
 */
static void
intel_flush_frontbuffer( struct pipe_winsys *sws )
{
   struct intel_context *intel = intel_pipe_winsys(sws)->intel;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   
   intelCopyBuffer(dPriv, NULL);
}

static void
intel_printf( struct pipe_winsys *sws, const char *fmtString, ... )
{
   va_list args;
   va_start( args, fmtString );  
   vfprintf(stderr, fmtString, args);
   va_end( args );
}

static const char *
intel_get_name( struct pipe_winsys *sws )
{
   return "Intel/DRI/ttm";
}


struct pipe_winsys *
intel_create_pipe_winsys( struct intel_context *intel )
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
   iws->winsys.buffer_map = intel_buffer_map;
   iws->winsys.buffer_unmap = intel_buffer_unmap;
   iws->winsys.buffer_reference = intel_buffer_reference;
   iws->winsys.buffer_data = intel_buffer_data;
   iws->winsys.buffer_subdata = intel_buffer_subdata;
   iws->winsys.buffer_get_subdata = intel_buffer_get_subdata;
   iws->winsys.flush_frontbuffer = intel_flush_frontbuffer;
   iws->winsys.wait_idle = intel_wait_idle;
   iws->winsys.printf = intel_printf;
   iws->winsys.get_name = intel_get_name;
   iws->intel = intel;

   return &iws->winsys;
}
