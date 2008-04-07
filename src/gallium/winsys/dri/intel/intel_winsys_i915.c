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
#include "ws_dri_bufpool.h"
#include "ws_dri_bufmgr.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_winsys.h"

#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "i915simple/i915_winsys.h"
#include "i915simple/i915_screen.h"


struct intel_i915_winsys {
   struct i915_winsys winsys;   /**< batch buffer funcs */
   struct pipe_winsys *pws;
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
			     struct pipe_buffer *buf,
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

#if 0 /* JB old */
   intel_batchbuffer_emit_reloc( intel->batch,
				 dri_bo( buf ),
				 flags, mask,
				 delta );
#else /* new */
   intel_offset_relocation( intel->batch,
				delta,
				dri_bo( buf ),
				flags,
				mask );
#endif
}



static void intel_i915_batch_flush( struct i915_winsys *sws,
                                    struct pipe_fence_handle **fence )
{
   struct intel_i915_winsys *iws = intel_i915_winsys(sws);
   struct intel_context *intel = iws->intel;
   union {
      struct _DriFenceObject *dri;
      struct pipe_fence_handle *pipe;
   } fu;

   fu.dri = intel_batchbuffer_flush( intel->batch );

   if (!fu.dri && fence) {
      *fence = NULL;
      return;
   }

   if (fu.dri) {
      if (fence)
	 *fence = fu.pipe;
      else
	 iws->pws->fence_reference(iws->pws, &fu.dri, NULL);
   }


//   if (0) intel_i915_batch_wait_idle( sws );
}


/**
 * Create i915 hardware rendering context.
 */
struct pipe_context *
intel_create_i915simple( struct intel_context *intel,
                         struct pipe_winsys *winsys )
{
   struct intel_i915_winsys *iws = CALLOC_STRUCT( intel_i915_winsys );
   struct pipe_screen *screen;
   
   /* Fill in this struct with callbacks that i915simple will need to
    * communicate with the window system, buffer manager, etc. 
    */
   iws->winsys.batch_start = intel_i915_batch_start;
   iws->winsys.batch_dword = intel_i915_batch_dword;
   iws->winsys.batch_reloc = intel_i915_batch_reloc;
   iws->winsys.batch_flush = intel_i915_batch_flush;
   iws->pws = winsys;
   iws->intel = intel;

   screen = i915_create_screen(winsys, intel->intelScreen->deviceID);

   /* Create the i915simple context:
    */
   return i915_create_context( screen,
                               winsys,
                               &iws->winsys );
}
