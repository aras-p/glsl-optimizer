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

#ifndef P_WINSYS_H
#define P_WINSYS_H


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

struct pipe_winsys {

   /* Do any special operations to ensure frontbuffer contents are
    * displayed, eg copy fake frontbuffer.
    */
   void (*flush_frontbuffer)( struct pipe_winsys *sws );

   /* debug output 
    */
   void (*printf)( struct pipe_winsys *sws,
		   const char *, ... );	


   /* The buffer manager is modeled after the dri_bugmgr interface,
    * but this is the subset that softpipe cares about.  Remember that
    * softpipe gets to choose the interface it needs, and the window
    * systems must then implement that interface (rather than the
    * other way around...).
    *
    * Softpipe only really wants to make system memory allocations,
    * right?? 
    */
   struct pipe_buffer_handle *(*buffer_create)(struct pipe_winsys *sws, 
					       unsigned alignment );

   void *(*buffer_map)( struct pipe_winsys *sws, 
			struct pipe_buffer_handle *buf,
			unsigned flags );
   
   void (*buffer_unmap)( struct pipe_winsys *sws, 
			 struct pipe_buffer_handle *buf );

   /** Set ptr = buf, with reference counting */
   void (*buffer_reference)( struct pipe_winsys *sws,
                             struct pipe_buffer_handle **ptr,
                             struct pipe_buffer_handle *buf );

   void (*buffer_data)(struct pipe_winsys *sws, 
		       struct pipe_buffer_handle *buf,
		       unsigned size, const void *data );

   void (*buffer_subdata)(struct pipe_winsys *sws, 
			  struct pipe_buffer_handle *buf,
			  unsigned long offset, 
			  unsigned long size, 
			  const void *data);

   void (*buffer_get_subdata)(struct pipe_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned long offset, 
			      unsigned long size, 
			      void *data);


   /* Wait for any hw swapbuffers, etc. to finish: 
    */
   void (*wait_idle)( struct pipe_winsys *sws );

   /* Queries:
    */
   const char *(*get_name)( struct pipe_winsys *sws );

};



#endif /* SP_WINSYS_H */
