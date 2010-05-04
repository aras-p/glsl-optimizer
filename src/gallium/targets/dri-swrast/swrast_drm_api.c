/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "state_tracker/drm_api.h"
#include "state_tracker/sw_winsys.h"
#include "dri_sw_winsys.h"
#include "trace/tr_public.h"

/* Copied from targets/libgl-xlib */

#ifdef GALLIUM_SOFTPIPE
#include "softpipe/sp_public.h"
#endif

#ifdef GALLIUM_LLVMPIPE
#include "llvmpipe/lp_public.h"
#endif

#ifdef GALLIUM_CELL
#include "cell/ppu/cell_public.h"
#endif

static struct pipe_screen *
swrast_create_screen(struct sw_winsys *winsys)
{
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;

#if defined(GALLIUM_CELL)
   default_driver = "cell";
#elif defined(GALLIUM_LLVMPIPE)
   default_driver = "llvmpipe";
#elif defined(GALLIUM_SOFTPIPE)
   default_driver = "softpipe";
#else
   default_driver = "";
#endif

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);

#if defined(GALLIUM_CELL)
   if (screen == NULL && strcmp(driver, "cell") == 0)
      screen = cell_create_screen( winsys );
#endif

#if defined(GALLIUM_LLVMPIPE)
   if (screen == NULL && strcmp(driver, "llvmpipe") == 0)
      screen = llvmpipe_create_screen( winsys );
#endif

#if defined(GALLIUM_SOFTPIPE)
   if (screen == NULL)
      screen = softpipe_create_screen( winsys );
#endif

   return trace_screen_create(screen);;
}

struct pipe_screen *
drisw_create_screen(struct drisw_loader_funcs *lf)
{
   struct sw_winsys *winsys = NULL;
   struct pipe_screen *screen = NULL;

   winsys = dri_create_sw_winsys(lf);
   if (winsys == NULL)
      return NULL;

   screen = swrast_create_screen(winsys);
   if (!screen)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy(winsys);

   return NULL;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
