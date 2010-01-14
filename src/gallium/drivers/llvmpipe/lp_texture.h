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


struct pipe_context;
struct pipe_screen;
struct llvmpipe_context;
struct llvmpipe_displaytarget;


struct llvmpipe_texture
{
   struct pipe_texture base;

   unsigned long level_offset[PIPE_MAX_TEXTURE_LEVELS];
   unsigned stride[PIPE_MAX_TEXTURE_LEVELS];

   /**
    * Display target, for textures with the PIPE_TEXTURE_USAGE_DISPLAY_TARGET
    * usage.
    */
   struct llvmpipe_displaytarget *dt;

   /**
    * Malloc'ed data for regular textures, or a mapping to dt above.
    */
   void *data;

   unsigned timestamp;
};


struct llvmpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};


/** cast wrappers */
static INLINE struct llvmpipe_texture *
llvmpipe_texture(struct pipe_texture *pt)
{
   return (struct llvmpipe_texture *) pt;
}


static INLINE const struct llvmpipe_texture *
llvmpipe_texture_const(const struct pipe_texture *pt)
{
   return (const struct llvmpipe_texture *) pt;
}


static INLINE struct llvmpipe_transfer *
llvmpipe_transfer(struct pipe_transfer *pt)
{
   return (struct llvmpipe_transfer *) pt;
}


extern void
llvmpipe_init_screen_texture_funcs(struct pipe_screen *screen);


#endif /* LP_TEXTURE_H */
