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

#ifndef LP_TEXTURE_H
#define LP_TEXTURE_H


#include "pipe/p_state.h"
#include "util/u_debug.h"


#define LP_MAX_TEXTURE_2D_LEVELS 12  /* 2K x 2K for now */
#define LP_MAX_TEXTURE_3D_LEVELS 10  /* 512 x 512 x 512 for now */


struct pipe_context;
struct pipe_screen;
struct llvmpipe_context;

struct sw_displaytarget;


struct llvmpipe_resource
{
   struct pipe_resource base;

   unsigned long level_offset[LP_MAX_TEXTURE_2D_LEVELS];
   unsigned stride[LP_MAX_TEXTURE_2D_LEVELS];

   /**
    * Display target, for textures with the PIPE_BIND_DISPLAY_TARGET
    * usage.
    */
   struct sw_displaytarget *dt;

   /**
    * Malloc'ed data for regular textures, or a mapping to dt above.
    */
   void *data;

   boolean userBuffer;  /** Is this a user-space buffer? */
   unsigned timestamp;
};


struct llvmpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};


/** cast wrappers */
static INLINE struct llvmpipe_resource *
llvmpipe_resource(struct pipe_resource *pt)
{
   return (struct llvmpipe_resource *) pt;
}


static INLINE const struct llvmpipe_resource *
llvmpipe_resource_const(const struct pipe_resource *pt)
{
   return (const struct llvmpipe_resource *) pt;
}


static INLINE struct llvmpipe_transfer *
llvmpipe_transfer(struct pipe_transfer *pt)
{
   return (struct llvmpipe_transfer *) pt;
}


void llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen);
void llvmpipe_init_context_resource_funcs(struct pipe_context *pipe);

static INLINE unsigned
llvmpipe_resource_stride(struct pipe_resource *texture,
                        unsigned level)
{
   struct llvmpipe_resource *lpt = llvmpipe_resource(texture);
   assert(level < LP_MAX_TEXTURE_2D_LEVELS);
   return lpt->stride[level];
}


void *
llvmpipe_resource_map(struct pipe_resource *texture,
		      unsigned usage,
		      unsigned face,
		      unsigned level,
		      unsigned zslice);

void
llvmpipe_resource_unmap(struct pipe_resource *texture,
                       unsigned face,
                       unsigned level,
                       unsigned zslice);


#endif /* LP_TEXTURE_H */
