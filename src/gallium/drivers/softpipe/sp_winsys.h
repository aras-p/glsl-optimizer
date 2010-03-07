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

/* This is the interface that softpipe requires any window system
 * hosting it to implement.  This is the only include file in softpipe
 * which is public.
 */


#ifndef SP_WINSYS_H
#define SP_WINSYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pipe/p_defines.h"

struct pipe_screen;
struct pipe_winsys;
struct pipe_context;
struct pipe_texture;
struct pipe_buffer;



/**
 * Create a softpipe screen that uses the
 * given winsys for allocating buffers.
 */
struct pipe_screen *softpipe_create_screen( struct pipe_winsys * );

/**
 * Create a softpipe screen that uses
 * regular malloc to create all its buffers.
 */
struct pipe_screen *softpipe_create_screen_malloc(void);

boolean
softpipe_get_texture_buffer( struct pipe_texture *texture,
                             struct pipe_buffer **buf,
                             unsigned *stride );


#ifdef __cplusplus
}
#endif

#endif /* SP_WINSYS_H */
