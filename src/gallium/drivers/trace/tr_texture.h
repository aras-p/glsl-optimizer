/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef TR_TEXTURE_H_
#define TR_TEXTURE_H_


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "tr_screen.h"


struct trace_texture
{
   struct pipe_texture base;

   struct pipe_texture *texture;

   struct tr_list list;
};


struct trace_surface
{
   struct pipe_surface base;

   struct pipe_surface *surface;

   struct tr_list list;
};


struct trace_transfer
{
   struct pipe_transfer base;

   struct pipe_transfer *transfer;

   struct tr_list list;

   void *map;
};


static INLINE struct trace_texture *
trace_texture(struct pipe_texture *texture)
{
   if(!texture)
      return NULL;
   (void)trace_screen(texture->screen);
   return (struct trace_texture *)texture;
}


static INLINE struct trace_surface *
trace_surface(struct pipe_surface *surface)
{
   if(!surface)
      return NULL;
   (void)trace_texture(surface->texture);
   return (struct trace_surface *)surface;
}


static INLINE struct trace_transfer *
trace_transfer(struct pipe_transfer *transfer)
{
   if(!transfer)
      return NULL;
   (void)trace_texture(transfer->texture);
   return (struct trace_transfer *)transfer;
}


struct pipe_texture *
trace_texture_create(struct trace_screen *tr_scr,
                     struct pipe_texture *texture);

void
trace_texture_destroy(struct trace_texture *tr_tex);

struct pipe_surface *
trace_surface_create(struct trace_texture *tr_tex,
                     struct pipe_surface *surface);

void
trace_surface_destroy(struct trace_surface *tr_surf);

struct pipe_transfer *
trace_transfer_create(struct trace_texture *tr_tex,
                      struct pipe_transfer *transfer);

void
trace_transfer_destroy(struct trace_transfer *tr_trans);


#endif /* TR_TEXTURE_H_ */
