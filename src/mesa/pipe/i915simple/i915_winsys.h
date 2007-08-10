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

#ifndef I915_WINSYS_H
#define I915_WINSYS_H

#include "main/mtypes.h"

/* This is the interface that softpipe requires any window system
 * hosting it to implement.  This is the only include file in softpipe
 * which is public.
 */


/* Pipe drivers are (meant to be!) independent of both GL and the
 * window system.  The window system provides a buffer manager and a
 * set of additional hooks for things like command buffer submission,
 * etc.
 *
 * There clearly has to be some agreement between the window system
 * driver and the hardware driver about the format of command buffers,
 * etc.
 */

struct pipe_buffer_handle;

struct i915_winsys {

   /* Do any special operations to ensure frontbuffer contents are
    * displayed, eg copy fake frontbuffer.
    */
   void (*flush_frontbuffer)( struct i915_winsys *sws );


   /* debug output 
    */
   void (*printf)( struct i915_winsys *sws, 
		   const char *, ... );	

   /* Many of the winsys's are probably going to have a similar
    * buffer-manager interface, as something almost identical is
    * currently exposed in the pipe interface.  Probably want to avoid
    * endless repetition of this code somehow.
    */
   struct pipe_buffer_handle *(*buffer_create)(struct i915_winsys *sws, 
					       unsigned alignment );

   void *(*buffer_map)( struct i915_winsys *sws, 
			struct pipe_buffer_handle *buf );
   
   void (*buffer_unmap)( struct i915_winsys *sws, 
			 struct pipe_buffer_handle *buf );

   struct pipe_buffer_handle *(*buffer_reference)( struct i915_winsys *sws,
						   struct pipe_buffer_handle *buf );

   void (*buffer_unreference)( struct i915_winsys *sws, 
			       struct pipe_buffer_handle **buf );

   void (*buffer_data)(struct i915_winsys *sws, 
		       struct pipe_buffer_handle *buf,
		       unsigned size, const void *data );

   void (*buffer_subdata)(struct i915_winsys *sws, 
			  struct pipe_buffer_handle *buf,
			  unsigned long offset, 
			  unsigned long size, 
			  const void *data);

   void (*buffer_get_subdata)(struct i915_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned long offset, 
			      unsigned long size, 
			      void *data);


   /* An over-simple batchbuffer mechanism.  Will want to improve the
    * performance of this, perhaps based on the cmdstream stuff.  It
    * would be pretty impossible to implement swz on top of this
    * interface.
    *
    * Will also need additions/changes to implement static/dynamic
    * indirect state.
    */
   unsigned *(*batch_start)( struct i915_winsys *sws,
			     unsigned dwords,
			     unsigned relocs );
   void (*batch_dword)( struct i915_winsys *sws,
			unsigned dword );
   void (*batch_reloc)( struct i915_winsys *sws,
			struct pipe_buffer_handle *buf,
			unsigned access_flags,
			unsigned delta );
   void (*batch_flush)( struct i915_winsys *sws );
   void (*batch_wait_idle)( struct i915_winsys *sws );
   

   /* Printf???
    */
   void (*dpf)( const char *fmt, ... );
   
};

#define I915_BUFFER_ACCESS_WRITE   0x1 
#define I915_BUFFER_ACCESS_READ    0x2


struct pipe_context *i915_create( struct i915_winsys *,
				  unsigned pci_id );


#endif 
