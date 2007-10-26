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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef SP_SURFACE_H
#define SP_SURFACE_H

#include "sp_headers.h"
#include "pipe/p_state.h"

struct pipe_context;
struct softpipe_surface;
struct softpipe_context;
struct softpipe_tile_cache;


/**
 * Softpipe surface is derived from pipe_surface.
 */
struct softpipe_surface {
   struct pipe_surface surface;

#if 0
   /* XXX these are temporary here */
   void (*get_tile)(struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, float *p);
   void (*put_tile)(struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, const float *p);
#endif
};

extern struct pipe_surface *
softpipe_get_tex_surface(struct pipe_context *pipe,
                         struct pipe_mipmap_tree *mt,
                         unsigned face, unsigned level, unsigned zslice);


extern void
softpipe_get_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       float *p);

extern void
softpipe_put_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       const float *p);

extern void
softpipe_init_surface_funcs(struct softpipe_surface *sps);


/** Cast wrapper */
static INLINE struct softpipe_surface *
softpipe_surface(struct pipe_surface *ps)
{
   return (struct softpipe_surface *) ps;
}


extern void
sp_init_surface_functions(struct softpipe_context *sp);


#endif /* SP_SURFACE_H */
