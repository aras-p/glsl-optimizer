/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * Authors:
 *   Keith Whitwell
 */

#include "swrast_xlib.h"

#include "state_tracker/xlib_sw_winsys.h"
#include "util/u_debug.h"
#include "softpipe/sp_public.h"
#include "llvmpipe/lp_public.h"
#include "identity/id_public.h"
#include "trace/tr_public.h"
#include "cell/ppu/cell_public.h"


/* Helper function to build a subset of a driver stack consisting of
 * one of the software rasterizers (cell, llvmpipe, softpipe) and the
 * xlib winsys.
 *
 * This can be called by any target that builds on top of this
 * combination.
 */
struct pipe_screen *
swrast_xlib_create_screen( Display *display )
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen = NULL;

   /* Create the underlying winsys, which performs presents to Xlib
    * drawables:
    */
   winsys = xlib_create_sw_winsys( display );
   if (winsys == NULL)
      return NULL;

   /* Create a software rasterizer on top of that winsys:
    */
#if defined(GALLIUM_CELL)
   if (screen == NULL &&
       !debug_get_bool_option("GALLIUM_NO_CELL", FALSE))
      screen = cell_create_screen( winsys );
#endif

#if defined(GALLIUM_LLVMPIPE)
   if (screen == NULL &&
       !debug_get_bool_option("GALLIUM_NO_LLVM", FALSE))
      screen = llvmpipe_create_screen( winsys );
#endif

   if (screen == NULL)
      screen = softpipe_create_screen( winsys );

   if (screen == NULL)
      goto fail;

   /* Inject any wrapping layers we want to here:
    */
   if (debug_get_bool_option("GALLIUM_WRAP", FALSE)) {
      screen = identity_screen_create(screen);
   }

   if (debug_get_bool_option("GALLIUM_TRACE", FALSE)) {
      screen = trace_screen_create( screen );
   }

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}




