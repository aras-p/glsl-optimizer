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

#include "imports.h"
#include "intel_context.h"
#include "intel_winsys.h"
#include "pipe/softpipe/sp_winsys.h"
#include "pipe/p_defines.h"



struct intel_softpipe_winsys {
   struct softpipe_winsys sws;
   struct intel_context *intel;
};

/**
 * Return list of surface formats supported by this driver.
 */
static boolean
intel_is_format_supported(struct softpipe_winsys *sws, uint format)
{
   switch(format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
   case PIPE_FORMAT_U_R5_G6_B5:
   case PIPE_FORMAT_S8_Z24:
      return TRUE;
   default:
      return FALSE;
   }
}


struct pipe_context *
intel_create_softpipe( struct intel_context *intel )
{
   struct intel_softpipe_winsys *isws = CALLOC_STRUCT( intel_softpipe_winsys );
   
   /* Fill in this struct with callbacks that softpipe will need to
    * communicate with the window system, buffer manager, etc. 
    */
   isws->sws.is_format_supported = intel_is_format_supported;
   isws->intel = intel;

   /* Create the softpipe context:
    */
   return softpipe_create( intel_create_pipe_winsys( intel ),
			   &isws->sws );
}
