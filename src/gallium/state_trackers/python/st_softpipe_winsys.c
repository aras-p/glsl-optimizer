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

#include "util/u_debug.h"
#include "softpipe/sp_public.h"
#include "llvmpipe/lp_public.h"
#include "state_tracker/sw_winsys.h"
#include "null/null_sw_winsys.h"
#include "st_winsys.h"


struct pipe_screen *
st_software_screen_create(void)
{
   struct sw_winsys *ws;
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;

#if defined(HAVE_LLVMPIPE)
   default_driver = "llvmpipe";
#elif defined(HAVE_SOFTPIPE)
   default_driver = "softpipe";
#endif

   ws = null_sw_create();
   if(!ws)
      return NULL;

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);

#ifdef HAVE_LLVMPIPE
   if (strcmp(driver, "llvmpipe") == 0) {
      screen = llvmpipe_create_screen(ws);
   }
#endif

#ifdef HAVE_SOFTPIPE
   if (strcmp(driver, "softpipe") == 0) {
      screen = softpipe_create_screen(ws);
   }
#endif

   return screen;
}
