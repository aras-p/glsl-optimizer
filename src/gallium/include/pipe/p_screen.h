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
 * @file
 * 
 * Screen, Adapter or GPU
 *
 * These are driver functions/facilities that are context independent.
 */


#ifndef P_SCREEN_H
#define P_SCREEN_H


#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_defines.h"



#ifdef __cplusplus
extern "C" {
#endif


/** Opaque type */
struct pipe_fence_handle;
struct pipe_winsys;
struct pipe_buffer;
struct pipe_texture;
struct pipe_surface;
struct pipe_video_surface;
struct pipe_transfer;


/**
 * Gallium screen/adapter context.  Basically everything
 * hardware-specific that doesn't actually require a rendering
 * context.
 */
struct pipe_screen {
   struct pipe_winsys *winsys;

   void (*destroy)( struct pipe_screen * );


   const char *(*get_name)( struct pipe_screen * );

   const char *(*get_vendor)( struct pipe_screen * );

   /**
    * Query an integer-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   int (*get_param)( struct pipe_screen *, int param );

   /**
    * Query a float-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   float (*get_paramf)( struct pipe_screen *, int param );

   struct pipe_context * (*context_create)( struct pipe_screen *,
					    void *priv );
   
   /**
    * Check if the given pipe_format is supported as a texture or
    * drawing surface.
    * \param tex_usage  bitmask of PIPE_TEXTURE_USAGE_*
    * \param geom_flags  bitmask of PIPE_TEXTURE_GEOM_*
    */
   boolean (*is_format_supported)( struct pipe_screen *,
                                   enum pipe_format format,
                                   enum pipe_texture_target target,
                                   unsigned tex_usage, 
                                   unsigned geom_flags );

   /**
    * Create a new texture object, using the given template info.
    */
   struct pipe_texture * (*texture_create)(struct pipe_screen *,
                                           const struct pipe_texture *templat);

   /**
    * Create a new texture object, using the given template info, but on top of
    * existing memory.
    * 
    * It is assumed that the buffer data is layed out according to the expected
    * by the hardware. NULL will be returned if any inconsistency is found.  
    */
   struct pipe_texture * (*texture_blanket)(struct pipe_screen *,
                                            const struct pipe_texture *templat,
                                            const unsigned *stride,
                                            struct pipe_buffer *buffer);

   void (*texture_destroy)(struct pipe_texture *pt);

   /** Get a 2D surface which is a "view" into a texture
    * \param usage  bitmaks of PIPE_BUFFER_USAGE_* read/write flags
    */
   struct pipe_surface *(*get_tex_surface)(struct pipe_screen *,
                                           struct pipe_texture *texture,
                                           unsigned face, unsigned level,
                                           unsigned zslice,
                                           unsigned usage );

   void (*tex_surface_destroy)(struct pipe_surface *);
   

   /** Get a transfer object for transferring data to/from a texture */
   struct pipe_transfer *(*get_tex_transfer)(struct pipe_screen *,
                                             struct pipe_texture *texture,
                                             unsigned face, unsigned level,
                                             unsigned zslice,
                                             enum pipe_transfer_usage usage,
                                             unsigned x, unsigned y,
                                             unsigned w, unsigned h);

   void (*tex_transfer_destroy)(struct pipe_transfer *);
   
   void *(*transfer_map)( struct pipe_screen *,
                          struct pipe_transfer *transfer );

   void (*transfer_unmap)( struct pipe_screen *,
                           struct pipe_transfer *transfer );


   /**
    * Create a new buffer.
    * \param alignment  buffer start address alignment in bytes
    * \param usage  bitmask of PIPE_BUFFER_USAGE_x
    * \param size  size in bytes
    */
   struct pipe_buffer *(*buffer_create)( struct pipe_screen *screen,
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
   struct pipe_buffer *(*user_buffer_create)(struct pipe_screen *screen,
                                             void *ptr,
                                             unsigned bytes);

   /**
    * Allocate storage for a display target surface.
    *
    * Often surfaces which are meant to be blitted to the front screen (i.e.,
    * display targets) must be allocated with special characteristics, memory
    * pools, or obtained directly from the windowing system.
    *
    * This callback is invoked by the pipe_screenwhen creating a texture marked
    * with the PIPE_TEXTURE_USAGE_DISPLAY_TARGET flag  to get the underlying
    * buffer storage.
    */
   struct pipe_buffer *(*surface_buffer_create)(struct pipe_screen *screen,
						unsigned width, unsigned height,
						enum pipe_format format,
						unsigned usage,
						unsigned tex_usage,
						unsigned *stride);


   /**
    * Map the entire data store of a buffer object into the client's address.
    * flags is bitmask of PIPE_BUFFER_USAGE_CPU_READ/WRITE flags.
    */
   void *(*buffer_map)( struct pipe_screen *screen,
			struct pipe_buffer *buf,
			unsigned usage );
   /**
    * Map a subrange of the buffer data store into the client's address space.
    *
    * The returned pointer is always relative to buffer start, regardless of 
    * the specified range. This is different from the ARB_map_buffer_range
    * semantics because we don't forbid multiple mappings of the same buffer
    * (yet).
    */
   void *(*buffer_map_range)( struct pipe_screen *screen,
                              struct pipe_buffer *buf,
                              unsigned offset,
                              unsigned length,
                              unsigned usage);

   /**
    * Notify a range that was actually written into.
    * 
    * Can only be used if the buffer was mapped with the 
    * PIPE_BUFFER_USAGE_CPU_WRITE and PIPE_BUFFER_USAGE_FLUSH_EXPLICIT flags 
    * set.
    * 
    * The range is relative to the buffer start, regardless of the range 
    * specified to buffer_map_range. This is different from the 
    * ARB_map_buffer_range semantics because we don't forbid multiple mappings 
    * of the same buffer (yet).
    * 
    */
   void (*buffer_flush_mapped_range)( struct pipe_screen *screen,
                                      struct pipe_buffer *buf,
                                      unsigned offset,
                                      unsigned length);

   /**
    * Unmap buffer.
    * 
    * If the buffer was mapped with PIPE_BUFFER_USAGE_CPU_WRITE flag but not
    * PIPE_BUFFER_USAGE_FLUSH_EXPLICIT then the pipe driver will
    * assume that the whole buffer was written. This is mostly for backward 
    * compatibility purposes and may affect performance -- the state tracker 
    * should always specify exactly what got written while the buffer was 
    * mapped.
    */
   void (*buffer_unmap)( struct pipe_screen *screen,
                         struct pipe_buffer *buf );

   void (*buffer_destroy)( struct pipe_buffer *buf );

   /**
    * Create a video surface suitable for use as a decoding target by the
    * driver's pipe_video_context.
    */
   struct pipe_video_surface*
   (*video_surface_create)( struct pipe_screen *screen,
                            enum pipe_video_chroma_format chroma_format,
                            unsigned width, unsigned height );

   void (*video_surface_destroy)( struct pipe_video_surface *vsfc );

   /**
    * Do any special operations to ensure buffer size is correct
    */
   void (*update_buffer)( struct pipe_screen *ws,
                          void *context_private );

   /**
    * Do any special operations to ensure frontbuffer contents are
    * displayed, eg copy fake frontbuffer.
    */
   void (*flush_frontbuffer)( struct pipe_screen *screen,
                              struct pipe_surface *surf,
                              void *context_private );



   /** Set ptr = fence, with reference counting */
   void (*fence_reference)( struct pipe_screen *screen,
                            struct pipe_fence_handle **ptr,
                            struct pipe_fence_handle *fence );

   /**
    * Checks whether the fence has been signalled.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_signalled)( struct pipe_screen *screen,
                           struct pipe_fence_handle *fence,
                           unsigned flags );

   /**
    * Wait for the fence to finish.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_finish)( struct pipe_screen *screen,
                        struct pipe_fence_handle *fence,
                        unsigned flags );

};


#ifdef __cplusplus
}
#endif

#endif /* P_SCREEN_H */
