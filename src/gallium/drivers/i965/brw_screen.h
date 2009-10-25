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

struct brw_winsys_screen;


/**
 * Subclass of pipe_screen
 */
struct brw_screen
{
   struct pipe_screen base;
   struct brw_chipset chipset;
   struct brw_winsys_screen *sws;
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
   struct brw_winsys_buffer *bo;
   void *ptr;
   boolean is_user_buffer;
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

static INLINE struct brw_buffer *
brw_buffer(struct pipe_buffer *buffer)
{
   return (struct brw_buffer *)buffer;
}


/* Pipe buffer helpers
 */
static INLINE boolean
brw_buffer_is_user_buffer( const struct pipe_buffer *buf )
{
   return ((const struct brw_buffer *)buf)->is_user_buffer;
}

struct brw_winsys_buffer *
brw_surface_bo( struct pipe_surface *surface );

unsigned
brw_surface_pitch( const struct pipe_surface *surface );


#endif /* BRW_SCREEN_H */
