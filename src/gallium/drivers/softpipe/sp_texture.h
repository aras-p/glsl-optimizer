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

#ifndef SP_TEXTURE_H
#define SP_TEXTURE_H


#include "pipe/p_state.h"
#include "pipe/p_video_state.h"


struct pipe_context;
struct pipe_screen;
struct softpipe_context;


struct softpipe_texture
{
   struct pipe_texture base;

   unsigned long level_offset[PIPE_MAX_TEXTURE_LEVELS];
   unsigned stride[PIPE_MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct pipe_buffer *buffer;

   /* True if texture images are power-of-two in all dimensions:
    */
   boolean pot;

   unsigned timestamp;
};

struct softpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};

struct softpipe_video_surface
{
   struct pipe_video_surface base;

   /* The data is held here:
    */
   struct pipe_texture *tex;
};


/** cast wrappers */
static INLINE struct softpipe_texture *
softpipe_texture(struct pipe_texture *pt)
{
   return (struct softpipe_texture *) pt;
}

static INLINE struct softpipe_transfer *
softpipe_transfer(struct pipe_transfer *pt)
{
   return (struct softpipe_transfer *) pt;
}

static INLINE struct softpipe_video_surface *
softpipe_video_surface(struct pipe_video_surface *pvs)
{
   return (struct softpipe_video_surface *) pvs;
}


extern void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen);


#endif /* SP_TEXTURE */
