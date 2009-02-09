/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/
/*
 * Authors: Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include "intel_context.h"
#include "intel_winsys_softpipe.h"
#include "pipe/p_defines.h"
#include "pipe/p_format.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"


struct intel_softpipe_winsys {
   struct softpipe_winsys sws;
   struct intel_context *intel;
};

/**
 * Return list of surface formats supported by this driver.
 */
static boolean
intel_is_format_supported(struct softpipe_winsys *sws,
                          enum pipe_format format)
{
   switch(format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_R5G6B5_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
      return TRUE;
   default:
      return FALSE;
   }
}


/**
 * Create rendering context which uses software rendering.
 */
struct pipe_context *
intel_create_softpipe( struct intel_context *intel,
                       struct pipe_winsys *winsys )
{
   struct intel_softpipe_winsys *isws = CALLOC_STRUCT( intel_softpipe_winsys );
   struct pipe_screen *screen = softpipe_create_screen(winsys);

   /* Fill in this struct with callbacks that softpipe will need to
    * communicate with the window system, buffer manager, etc.
    */
   isws->sws.is_format_supported = intel_is_format_supported;
   isws->intel = intel;

   /* Create the softpipe context:
    */
   return softpipe_create( screen, winsys, &isws->sws );
}
