/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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

#ifndef DRI_DRAWABLE_H
#define DRI_DRAWABLE_H

#include "pipe/p_compiler.h"

struct pipe_surface;
struct pipe_fence;
struct st_framebuffer;


struct dri_drawable
{
   __DRIdrawablePrivate *dPriv;
   unsigned stamp;

   struct pipe_fence *last_swap_fence;
   struct pipe_fence *first_swap_fence;

   struct st_framebuffer *stfb;
};


static INLINE struct dri_drawable *
dri_drawable(__DRIdrawablePrivate * driDrawPriv)
{
   return (struct dri_drawable *) driDrawPriv->driverPrivate;
}


/***********************************************************************
 * dri_drawable.c
 */

void 
dri_swap_buffers(__DRIdrawablePrivate * dPriv);

void 
dri_copy_sub_buffer(__DRIdrawablePrivate * dPriv,
                    int x, int y, 
                    int w, int h);

void 
dri_update_window_size(__DRIdrawablePrivate *dPriv);


#endif
