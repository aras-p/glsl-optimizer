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
 * This is the interface that Gallium3D requires any window system
 * hosting it to implement.  This is the only include file in Gallium3D
 * which is public.
 */

#ifndef P_WINSYS_H
#define P_WINSYS_H


#include "p_format.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Opaque type */
struct pipe_fence_handle;

struct pipe_surface;


/**
 * Gallium3D drivers are (meant to be!) independent of both GL and the
 * window system.  The window system provides a buffer manager and a
 * set of additional hooks for things like command buffer submission,
 * etc.
 *
 * There clearly has to be some agreement between the window system
 * driver and the hardware driver about the format of command buffers,
 * etc.
 */
struct pipe_winsys
{
   void (*destroy)( struct pipe_winsys *ws );

   /** Returns name of this winsys interface */
   const char *(*get_name)( struct pipe_winsys *ws );

   /**
    * Do any special operations to ensure frontbuffer contents are
    * displayed, eg copy fake frontbuffer.
    */
   void (*flush_frontbuffer)( struct pipe_winsys *ws,
                              struct pipe_surface *surf,
                              void *context_private );


   /** allocate a new surface (no context dependency) */
   struct pipe_surface *(*surface_alloc)(struct pipe_winsys *ws);

   /**
    * Allocate storage for a pipe_surface.
    * \param flags XXX unused, remove someday
    * \return  0 if succeeds.
    */
   int (*surface_alloc_storage)(struct pipe_winsys *ws,
                                struct pipe_surface *surf,
                                unsigned width, unsigned height,
                                enum pipe_format format,
                                unsigned flags,
                                unsigned tex_usage);
   
   void (*surface_release)(struct pipe_winsys *ws, struct pipe_surface **s);


   /**
    * Buffer management. Buffer attributes are mostly fixed over its lifetime.
    *
    * Remember that gallium gets to choose the interface it needs, and the
    * window systems must then implement that interface (rather than the
    * other way around...).
    *
    * usage is a bitmask of PIPE_BUFFER_USAGE_PIXEL/VERTEX/INDEX/CONSTANT. This
    * usage argument is only an optimization hint, not a guarantee, therefore 
    * proper behavior must be observed in all circumstances.
    *
    * alignment indicates the client's alignment requirements, eg for
    * SSE instructions.
    */
   struct pipe_buffer *(*buffer_create)( struct pipe_winsys *ws, 
                                         unsigned alignment, 
                                         unsigned usage,
                                         unsigned size );

   /** 
    * Create a buffer that wraps user-space data.
    *
    * Effectively this schedules a delayed call to buffer_create
    * followed by an upload of the data at *some point in the future*,
    * or perhaps never.  Basically the allocate/upload is delayed
    * until the buffer is actually passed to hardware.
    *
    * The intention is to provide a quick way to turn regular data
    * into a buffer, and secondly to avoid a copy operation if that
    * data subsequently turns out to be only accessed by the CPU.  
    *
    * Common example is OpenGL vertex buffers that are subsequently
    * processed either by software TNL in the driver or by passing to
    * hardware.
    *
    * XXX: What happens if the delayed call to buffer_create() fails?
    *
    * Note that ptr may be accessed at any time upto the time when the
    * buffer is destroyed, so the data must not be freed before then.
    */
   struct pipe_buffer *(*user_buffer_create)(struct pipe_winsys *ws, 
                                                    void *ptr,
                                                    unsigned bytes);

   /** 
    * Map the entire data store of a buffer object into the client's address.
    * flags is bitmask of PIPE_BUFFER_USAGE_CPU_READ/WRITE flags. 
    */
   void *(*buffer_map)( struct pipe_winsys *ws, 
			struct pipe_buffer *buf,
			unsigned usage );
   
   void (*buffer_unmap)( struct pipe_winsys *ws, 
			 struct pipe_buffer *buf );

   void (*buffer_destroy)( struct pipe_winsys *ws,
			   struct pipe_buffer *buf );


   /** Set ptr = fence, with reference counting */
   void (*fence_reference)( struct pipe_winsys *ws,
                            struct pipe_fence_handle **ptr,
                            struct pipe_fence_handle *fence );

   /**
    * Checks whether the fence has been signalled.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_signalled)( struct pipe_winsys *ws,
                           struct pipe_fence_handle *fence,
                           unsigned flag );

   /**
    * Wait for the fence to finish.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_finish)( struct pipe_winsys *ws,
                        struct pipe_fence_handle *fence,
                        unsigned flag );

};


#ifdef __cplusplus
}
#endif

#endif /* P_WINSYS_H */
