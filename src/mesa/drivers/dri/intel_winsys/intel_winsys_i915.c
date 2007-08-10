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
#include "intel_winsys.h"
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



static void intel_i915_batch_flush( struct i915_winsys *sws )
{
   struct intel_context *intel = intel_i915_winsys(sws)->intel;

   intel_batchbuffer_flush( intel->batch );
//   if (0) intel_i915_batch_wait_idle( sws );
}




struct pipe_context *
intel_create_i915simple( struct intel_context *intel )
{
   struct intel_i915_winsys *iws = CALLOC_STRUCT( intel_i915_winsys );
   
   /* Fill in this struct with callbacks that i915simple will need to
    * communicate with the window system, buffer manager, etc. 
    */
   iws->winsys.batch_start = intel_i915_batch_start;
   iws->winsys.batch_dword = intel_i915_batch_dword;
   iws->winsys.batch_reloc = intel_i915_batch_reloc;
   iws->winsys.batch_flush = intel_i915_batch_flush;
   iws->intel = intel;

   /* Create the i915simple context:
    */
   return i915_create( intel_create_pipe_winsys(intel),
		       &iws->winsys, 
		       intel->intelScreen->deviceID );
}
