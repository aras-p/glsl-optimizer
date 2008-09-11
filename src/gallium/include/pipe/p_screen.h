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
 * Screen, Adapter or GPU
 *
 * These are driver functions/facilities that are context independent.
 */


#ifndef P_SCREEN_H
#define P_SCREEN_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"



#ifdef __cplusplus
extern "C" {
#endif



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

   /**
    * Check if the given pipe_format is supported as a texture or
    * drawing surface.
    * \param tex_usage  bitmask of PIPE_TEXTURE_USAGE_*
    * \param flags  bitmask of PIPE_TEXTURE_GEOM_*
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
                                            const unsigned *pitch,
                                            struct pipe_buffer *buffer);

   void (*texture_release)(struct pipe_screen *,
                           struct pipe_texture **pt);

   /** Get a surface which is a "view" into a texture */
   struct pipe_surface *(*get_tex_surface)(struct pipe_screen *,
                                           struct pipe_texture *texture,
                                           unsigned face, unsigned level,
                                           unsigned zslice,
                                           unsigned usage );

   /* Surfaces allocated by the above must be released here:
    */
   void (*tex_surface_release)( struct pipe_screen *,
                                struct pipe_surface ** );
   

   void *(*surface_map)( struct pipe_screen *,
                         struct pipe_surface *surface,
                         unsigned flags );

   void (*surface_unmap)( struct pipe_screen *,
                          struct pipe_surface *surface );
   
};


#ifdef __cplusplus
}
#endif

#endif /* P_SCREEN_H */
