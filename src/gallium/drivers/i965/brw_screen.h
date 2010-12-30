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

#ifndef BRW_SCREEN_H
#define BRW_SCREEN_H

#include "pipe/p_state.h"
#include "pipe/p_screen.h"

#include "brw_reg.h"
#include "brw_structs.h"

struct brw_winsys_screen;


/**
 * Subclass of pipe_screen
 */
struct brw_screen
{
   struct pipe_screen base;
   int gen;
   boolean has_negative_rhw_bug;
   boolean needs_ff_sync;
   boolean is_g4x;
   int pci_id;
   struct brw_winsys_screen *sws;
   boolean no_tiling;
};



union brw_surface_id {
   struct {
      unsigned level:16;
      unsigned layer:16;
   } bits;
   unsigned value;
};


struct brw_surface
{
   struct pipe_surface base;

   union brw_surface_id id;
   unsigned offset;
   unsigned cpp;
   unsigned pitch;
   unsigned draw_offset;
   unsigned tiling;

   struct brw_surface_state ss;
   struct brw_winsys_buffer *bo;
   struct brw_surface *next, *prev;
};



/*
 * Cast wrappers
 */
static INLINE struct brw_screen *
brw_screen(struct pipe_screen *pscreen)
{
   return (struct brw_screen *) pscreen;
}


static INLINE struct brw_surface *
brw_surface(struct pipe_surface *surface)
{
   return (struct brw_surface *)surface;
}

unsigned
brw_surface_pitch( const struct pipe_surface *surface );


#endif /* BRW_SCREEN_H */
