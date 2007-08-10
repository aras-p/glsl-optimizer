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
#include "intel_batchbuffer.h"
#include "intel_pipe.h"
#include "intel_blit.h"

#include "pipe/i915simple/i915_winsys.h"


struct intel_i915_winsys {
   struct i915_winsys winsys;
   struct intel_context *intel;
};


/* Turn a i915simple winsys into an intel/i915simple winsys:
 */
static inline struct intel_i915_winsys *
intel_i915_winsys( struct i915_winsys *sws )
{
   return (struct intel_i915_winsys *)sws;
}



/* Many of the winsys's are probably going to have a similar
 * buffer-manager interface, as something almost identical is
 * currently exposed in the pipe interface.  Probably want to avoid
 * endless repetition of this code somehow.
 */
static inline struct _DriBufferObject *
dri_bo( struct pipe_buffer_handle *bo )
{
   return (struct _DriBufferObject *)bo;
}

static inline struct pipe_buffer_handle *
pipe_bo( struct _DriBufferObject *bo )
{
   return (struct pipe_buffer_handle *)bo;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *intel_i915_buffer_map(struct i915_winsys *sws, 
				   struct pipe_buffer_handle *buf )
{
   return driBOMap( dri_bo(buf), 
		    DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0 );
}

static void intel_i915_buffer_unmap(struct i915_winsys *sws, 
				    struct pipe_buffer_handle *buf)
{
   driBOUnmap( dri_bo(buf) );
}


static struct pipe_buffer_handle *
intel_i915_buffer_reference(struct i915_winsys *sws,
		       struct pipe_buffer_handle *buf)
{
   return pipe_bo( driBOReference( dri_bo(buf) ) );
}

static void intel_i915_buffer_unreference(struct i915_winsys *sws, 
				     struct pipe_buffer_handle **buf)
{
   if (*buf) {
      driBOUnReference( dri_bo(*buf) );
      *buf = NULL;
   }
}

/* Grabs the hardware lock!
 */
static void intel_i915_buffer_data(struct i915_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned size, const void *data )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;

   LOCK_HARDWARE( intel );
   driBOData( dri_bo(buf), size, data, 0 );
   UNLOCK_HARDWARE( intel );
}

static void intel_i915_buffer_subdata(struct i915_winsys *sws, 
				 struct pipe_buffer_handle *buf,
				 unsigned long offset, 
				 unsigned long size, 
				 const void *data)
{
   driBOSubData( dri_bo(buf), offset, size, data );
}

static void intel_i915_buffer_get_subdata(struct i915_winsys *sws, 
				     struct pipe_buffer_handle *buf,
				     unsigned long offset, 
				     unsigned long size, 
				     void *data)
{
   driBOGetSubData( dri_bo(buf), offset, size, data );
}

/* I915simple has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 */
static struct pipe_buffer_handle *
intel_i915_buffer_create(struct i915_winsys *sws, 
			 unsigned alignment)
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;
   struct _DriBufferObject *buffer;

   LOCK_HARDWARE( intel );
   driGenBuffers( intel->intelScreen->regionPool, 
		  "i915simple buffer", 1, &buffer, alignment, 0, 0 );
   UNLOCK_HARDWARE( intel );

   return pipe_bo(buffer);
}


/* Simple batchbuffer interface:
 */

static unsigned *intel_i915_batch_start( struct i915_winsys *sws,
					 unsigned dwords,
					 unsigned relocs )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;

   /* XXX: check relocs. 
    */
   if (intel_batchbuffer_space( intel->batch ) >= dwords * 4) {
      /* XXX: Hmm, the driver can't really do much with this pointer: 
       */
      return (unsigned *)intel->batch->ptr;	
   }
   else 
      return NULL;
}

static void intel_i915_batch_dword( struct i915_winsys *sws,
				    unsigned dword )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;
   intel_batchbuffer_emit_dword( intel->batch, dword );
}

static void intel_i915_batch_reloc( struct i915_winsys *sws,
			     struct pipe_buffer_handle *buf,
			     unsigned access_flags,
			     unsigned delta )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;
   unsigned flags = DRM_BO_FLAG_MEM_TT;
   unsigned mask = DRM_BO_MASK_MEM;

   if (access_flags & I915_BUFFER_ACCESS_WRITE) {
      flags |= DRM_BO_FLAG_WRITE;
      mask |= DRM_BO_FLAG_WRITE;
   }

   if (access_flags & I915_BUFFER_ACCESS_READ) {
      flags |= DRM_BO_FLAG_READ;
      mask |= DRM_BO_FLAG_READ;
   }

   intel_batchbuffer_emit_reloc( intel->batch, 
				 dri_bo( buf ),
				 flags, mask, 
				 delta );
}


static void intel_i915_batch_wait_idle( struct i915_winsys *sws )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;

   if (intel->batch->last_fence) {
      driFenceFinish(intel->batch->last_fence, 
		     DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, GL_FALSE);
      driFenceUnReference(intel->batch->last_fence);
      intel->batch->last_fence = NULL;
   }
}


static void intel_i915_batch_flush( struct i915_winsys *sws )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;

   intel_batchbuffer_flush( intel->batch );
   if (0) intel_i915_batch_wait_idle( sws );
}


static void intel_i915_printf( struct i915_winsys *sws,
			       const char *fmtString, ... )
{
   va_list args;
   va_start( args, fmtString );  
   vfprintf(stderr, fmtString, args);
   va_end( args );
}


static void
intel_i915_flush_frontbuffer( struct i915_winsys *sws )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   
   intelCopyBuffer(dPriv, NULL);
}


struct pipe_context *
intel_create_i915simple( struct intel_context *intel )
{
   struct intel_i915_winsys *iws = CALLOC_STRUCT( intel_i915_winsys );
   
   /* Fill in this struct with callbacks that i915simple will need to
    * communicate with the window system, buffer manager, etc. 
    */
   iws->winsys.printf = intel_i915_printf;
   iws->winsys.buffer_create = intel_i915_buffer_create;
   iws->winsys.buffer_map = intel_i915_buffer_map;
   iws->winsys.buffer_unmap = intel_i915_buffer_unmap;
   iws->winsys.buffer_reference = intel_i915_buffer_reference;
   iws->winsys.buffer_unreference = intel_i915_buffer_unreference;
   iws->winsys.buffer_data = intel_i915_buffer_data;
   iws->winsys.buffer_subdata = intel_i915_buffer_subdata;
   iws->winsys.buffer_get_subdata = intel_i915_buffer_get_subdata;
   iws->winsys.batch_start = intel_i915_batch_start;
   iws->winsys.batch_dword = intel_i915_batch_dword;
   iws->winsys.batch_reloc = intel_i915_batch_reloc;
   iws->winsys.batch_flush = intel_i915_batch_flush;
   iws->winsys.batch_wait_idle = intel_i915_batch_wait_idle;
   iws->winsys.flush_frontbuffer = intel_i915_flush_frontbuffer;
   iws->intel = intel;

   /* Create the i915simple context:
    */
   return i915_create( &iws->winsys, intel->intelScreen->deviceID );
}
