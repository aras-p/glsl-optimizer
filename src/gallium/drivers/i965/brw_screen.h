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
   struct brw_chipset chipset;
   struct brw_winsys_screen *sws;
   boolean no_tiling;
};

/**
 * Subclass of pipe_transfer
 */
struct brw_transfer
{
   struct pipe_transfer base;

   unsigned offset;
};

struct brw_buffer
{
   struct pipe_buffer base;

   /* One of either bo or user_buffer will be non-null, depending on
    * whether this is a hardware or user buffer.
    */
   struct brw_winsys_buffer *bo;
   void *user_buffer;

   /* Mapped pointer??
    */
   void *ptr;
};


union brw_surface_id {
   struct {
      unsigned face:3;
      unsigned zslice:13;
      unsigned level:16;
   } bits;
   unsigned value;
};


struct brw_surface
{
   struct pipe_surface base;
   
   union brw_surface_id id;
   unsigned cpp;
   unsigned pitch;
   unsigned draw_offset;
   unsigned tiling;

   struct brw_surface_state ss;
   struct brw_winsys_buffer *bo;
   struct brw_surface *next, *prev;
};



struct brw_texture
{
   struct pipe_texture base;
   struct brw_winsys_buffer *bo;
   struct brw_surface_state ss;

   unsigned *image_offset[PIPE_MAX_TEXTURE_LEVELS];
   unsigned nr_images[PIPE_MAX_TEXTURE_LEVELS];
   unsigned level_offset[PIPE_MAX_TEXTURE_LEVELS];

   boolean compressed;
   unsigned brw_target;
   unsigned pitch;
   unsigned tiling;
   unsigned cpp;
   unsigned total_height;

   struct brw_surface views[2];
};



/*
 * Cast wrappers
 */
static INLINE struct brw_screen *
brw_screen(struct pipe_screen *pscreen)
{
   return (struct brw_screen *) pscreen;
}

static INLINE struct brw_transfer *
brw_transfer(struct pipe_transfer *transfer)
{
   return (struct brw_transfer *)transfer;
}

static INLINE struct brw_surface *
brw_surface(struct pipe_surface *surface)
{
   return (struct brw_surface *)surface;
}

static INLINE struct brw_buffer *
brw_buffer(struct pipe_buffer *buffer)
{
   return (struct brw_buffer *)buffer;
}

static INLINE struct brw_texture *
brw_texture(struct pipe_texture *texture)
{
   return (struct brw_texture *)texture;
}


/* Pipe buffer helpers
 */
static INLINE boolean
brw_buffer_is_user_buffer( const struct pipe_buffer *buf )
{
   return ((const struct brw_buffer *)buf)->user_buffer != NULL;
}

unsigned
brw_surface_pitch( const struct pipe_surface *surface );

/***********************************************************************
 * Internal functions 
 */
GLboolean brw_texture_layout(struct brw_screen *brw_screen,
			     struct brw_texture *tex );

void brw_update_texture( struct brw_screen *brw_screen,
			 struct brw_texture *tex );


void brw_screen_tex_init( struct brw_screen *brw_screen );
void brw_screen_tex_surface_init( struct brw_screen *brw_screen );

void brw_screen_buffer_init(struct brw_screen *brw_screen);


boolean brw_is_texture_referenced_by_bo( struct brw_screen *brw_screen,
                                         struct pipe_texture *texture,
                                         unsigned face, 
                                         unsigned level,
                                         struct brw_winsys_buffer *bo );

boolean brw_is_buffer_referenced_by_bo( struct brw_screen *brw_screen,
                                        struct pipe_buffer *buffer,
                                        struct brw_winsys_buffer *bo );



#endif /* BRW_SCREEN_H */
