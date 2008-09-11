/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * \file
 * This is the interface that i915simple requires any window system
 * hosting it to implement.  This is the only include file in i915simple
 * which is public.
 * 
 */

#ifndef I915_WINSYS_H
#define I915_WINSYS_H


#include "pipe/p_defines.h"


#ifdef __cplusplus
extern "C" {
#endif

   
/* Pipe drivers are (meant to be!) independent of both GL and the
 * window system.  The window system provides a buffer manager and a
 * set of additional hooks for things like command buffer submission,
 * etc.
 *
 * There clearly has to be some agreement between the window system
 * driver and the hardware driver about the format of command buffers,
 * etc.
 */

struct i915_batchbuffer;
struct pipe_buffer;
struct pipe_fence_handle;
struct pipe_winsys;
struct pipe_screen;


/**
 * Additional winsys interface for i915simple.
 * 
 * It is an over-simple batchbuffer mechanism.  Will want to improve the
 * performance of this, perhaps based on the cmdstream stuff.  It
 * would be pretty impossible to implement swz on top of this
 * interface.
 *
 * Will also need additions/changes to implement static/dynamic
 * indirect state.
 */
struct i915_winsys {

   void (*destroy)( struct i915_winsys *sws );
   
   /**
    * Get the current batch buffer from the winsys.
    */
   struct i915_batchbuffer *(*batch_get)( struct i915_winsys *sws );

   /**
    * Emit a relocation to a buffer.
    * 
    * Used not only when the buffer addresses are not pinned, but also to 
    * ensure refered buffers will not be destroyed until the current batch 
    * buffer execution is finished.
    *
    * The access flags is a combination of I915_BUFFER_ACCESS_WRITE and 
    * I915_BUFFER_ACCESS_READ macros.
    */
   void (*batch_reloc)( struct i915_winsys *sws,
			struct pipe_buffer *buf,
			unsigned access_flags,
			unsigned delta );

   /**
    * Flush the batch.
    */
   void (*batch_flush)( struct i915_winsys *sws,
                        struct pipe_fence_handle **fence );
};

#define I915_BUFFER_ACCESS_WRITE   0x1 
#define I915_BUFFER_ACCESS_READ    0x2

#define I915_BUFFER_USAGE_LIT_VERTEX  (PIPE_BUFFER_USAGE_CUSTOM << 0)


struct pipe_context *i915_create_context( struct pipe_screen *,
                                          struct pipe_winsys *,
                                          struct i915_winsys * );

#ifdef __cplusplus
}
#endif

#endif 
