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

/* #include "errno.h" */
/* #include "string.h" */
/* #include "imports.h" */

#include "intel_context.h"

#include "pipe/softpipe/sp_winsys.h"


struct intel_softpipe_winsys {
   struct softpipe_winsys sws;
   struct intel_context *intel;
};



/* Turn the softpipe opaque buffer pointer into a dri_bufmgr opaque
 * buffer pointer...
 */
static inline struct _DriBufferObject *
dri_bo( struct softpipe_buffer_handle *bo )
{
   return (struct _DriBufferObject *)bo;
}

static inline struct softpipe_buffer_handle *
pipe_bo( struct _DriBufferObject *bo )
{
   return (struct softpipe_buffer_handle *)bo;
}

/* Turn a softpipe winsys into an intel/softpipe winsys:
 */
static inline struct intel_softpipe_winsys *
intel_softpipe_winsys( struct softpipe_winsys *sws )
{
   return (struct intel_softpipe_winsys *)sws;
}


/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *intel_buffer_map(struct softpipe_winsys *sws, 
			      struct softpipe_buffer_handle *buf )
{
   return driBOMap( dri_bo(buf), 
		    DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0 );
}

static void intel_buffer_unmap(struct softpipe_winsys *sws, 
			       struct softpipe_buffer_handle *buf)
{
   driBOUnmap( dri_bo(buf) );
}


static struct softpipe_buffer_handle *
intel_buffer_reference(struct softpipe_winsys *sws,
		       struct softpipe_buffer_handle *buf)
{
   return pipe_bo( driBOReference( dri_bo(buf) ) );
}

static void intel_buffer_unreference(struct softpipe_winsys *sws, 
				     struct softpipe_buffer_handle *buf)
{
   driBOUnReference( dri_bo(buf) );
}

/* Grabs the hardware lock!
 */
static void intel_buffer_data(struct softpipe_winsys *sws, 
			      struct softpipe_buffer_handle *buf,
			      unsigned size, const void *data )
{
   struct intel_context *intel = intel_softpipe_winsys(sws)->intel;

   LOCK_HARDWARE( intel );
   driBOData( dri_bo(buf), size, data, 0 );
   UNLOCK_HARDWARE( intel );
}

static void intel_buffer_subdata(struct softpipe_winsys *sws, 
				 struct softpipe_buffer_handle *buf,
				 unsigned long offset, 
				 unsigned long size, 
				 const void *data)
{
   driBOSubData( dri_bo(buf), offset, size, data );
}

static void intel_buffer_get_subdata(struct softpipe_winsys *sws, 
				     struct softpipe_buffer_handle *buf,
				     unsigned long offset, 
				     unsigned long size, 
				     void *data)
{
   driBOGetSubData( dri_bo(buf), offset, size, data );
}

/* Softpipe has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 */
static struct softpipe_buffer_handle *
intel_create_buffer(struct softpipe_winsys *sws, 
		    const char *name,
		    unsigned alignment)
{
   struct intel_context *intel = intel_softpipe_winsys(sws)->intel;
   struct _DriBufferObject *buffer;

   LOCK_HARDWARE( intel );
   driGenBuffers( intel->intelScreen->regionPool, 
		  name, 1, &buffer, alignment, 0, 0 );
   UNLOCK_HARDWARE( intel );

   return pipe_bo(buffer);
}


struct pipe_context *
intel_create_softpipe( struct intel_context *intel )
{
   struct intel_softpipe_winsys *isws = CALLOC_STRUCT( intel_softpipe_winsys );
   
   /* Fill in this struct with callbacks that softpipe will need to
    * communicate with the window system, buffer manager, etc. 
    *
    * Softpipe would be happy with a malloc based memory manager, but
    * the SwapBuffers implementation in this winsys driver requires
    * that rendering be done to an appropriate _DriBufferObject.  
    */
   isws->sws.create_buffer = intel_create_buffer;
   isws->sws.buffer_map = intel_buffer_map;
   isws->sws.buffer_unmap = intel_buffer_unmap;
   isws->sws.buffer_reference = intel_buffer_reference;
   isws->sws.buffer_unreference = intel_buffer_unreference;
   isws->sws.buffer_data = intel_buffer_data;
   isws->sws.buffer_subdata = intel_buffer_subdata;
   isws->sws.buffer_get_subdata = intel_buffer_get_subdata;
   isws->intel = intel;

   /* Create the softpipe context:
    */
   return softpipe_create( &isws->sws );
}
