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

#ifndef CELL_TEXTURE_H
#define CELL_TEXTURE_H

#include "cell/common.h"

struct cell_context;
struct pipe_resource;


/**
 * Subclass of pipe_resource
 */
struct cell_resource
{
   struct pipe_resource base;

   unsigned long level_offset[CELL_MAX_TEXTURE_LEVELS];
   unsigned long stride[CELL_MAX_TEXTURE_LEVELS];

   /**
    * Display target, for textures with the PIPE_BIND_DISPLAY_TARGET
    * usage.
    */
   struct sw_displaytarget *dt;
   unsigned dt_stride;

   /**
    * Malloc'ed data for regular textures, or a mapping to dt above.
    */
   void *data;
   boolean userBuffer;

   /* Size of the linear buffer??
    */
   unsigned long buffer_size;

   /** The buffer above, mapped.  This is the memory from which the
    * SPUs will fetch texels.  This texture data is in the tiled layout.
    */
   ubyte *mapped;
};


struct cell_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
   void *map;
};


/** cast wrapper */
static INLINE struct cell_resource *
cell_resource(struct pipe_resource *pt)
{
   return (struct cell_resource *) pt;
}


/** cast wrapper */
static INLINE struct cell_transfer *
cell_transfer(struct pipe_transfer *pt)
{
   return (struct cell_transfer *) pt;
}


extern void
cell_init_screen_texture_funcs(struct pipe_screen *screen);

extern void
cell_init_texture_transfer_funcs(struct cell_context *cell);

#endif /* CELL_TEXTURE_H */
