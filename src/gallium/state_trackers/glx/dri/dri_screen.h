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

#ifndef DRI_SCREEN_H
#define DRI_SCREEN_H

#include "dri_util.h"
#include "xmlconfig.h"

#include "pipe/p_compiler.h"

struct dri_screen
{
   __DRIScreenPrivate *sPriv;
   struct pipe_winsys *pipe_winsys;
   struct pipe_screen *pipe_screen;

   struct {
      /* Need a pipe_surface pointer to do client-side swapbuffers:
       */
      unsigned long buffer_handle;
      struct pipe_surface *surface;
      struct pipe_texture *texture;

      int pitch;                   /* row stride, in bytes */
      int width;
      int height;
      int size;
      int cpp;                     /* for front and back buffers */
   } front;

   int deviceID;
   int drmMinor;


   /**
    * Configuration cache with default values for all contexts
    */
   driOptionCache optionCache;

   /**
    * Temporary(?) context to use for SwapBuffers or other situations in
    * which we need a rendering context, but none is currently bound.
    */
   struct dri_context *dummyContext;
};



/** cast wrapper */
static INLINE struct dri_screen *
dri_screen(__DRIscreenPrivate *sPriv)
{
   return (struct dri_screen *) sPriv->private;
}



#endif
