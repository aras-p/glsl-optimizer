/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
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

/**
 * @file
 * Llvmpipe support. 
 * 
 * @author Jose Fonseca
 */


#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "llvmpipe/lp_winsys.h"
#include "st_winsys.h"


static boolean
llvmpipe_ws_is_displaytarget_format_supported( struct llvmpipe_winsys *ws,
      enum pipe_format format )
{
   return FALSE;
}


static void *
llvmpipe_ws_displaytarget_map(struct llvmpipe_winsys *ws,
                              struct llvmpipe_displaytarget *dt,
                              unsigned flags )
{
   assert(0);
   return NULL;
}


static void
llvmpipe_ws_displaytarget_unmap(struct llvmpipe_winsys *ws,
                                struct llvmpipe_displaytarget *dt )
{
   assert(0);
}


static void
llvmpipe_ws_displaytarget_destroy(struct llvmpipe_winsys *winsys,
                                  struct llvmpipe_displaytarget *dt)
{
   assert(0);
}


static struct llvmpipe_displaytarget *
llvmpipe_ws_displaytarget_create(struct llvmpipe_winsys *winsys,
                                 enum pipe_format format,
                                 unsigned width, unsigned height,
                                 unsigned alignment,
                                 unsigned *stride)
{
   return NULL;
}


static void
llvmpipe_ws_displaytarget_display(struct llvmpipe_winsys *winsys,
                                  struct llvmpipe_displaytarget *dt,
                                  void *context_private)
{
   assert(0);
}


static void
llvmpipe_ws_destroy(struct llvmpipe_winsys *winsys)
{
   FREE(winsys);
}


static struct pipe_screen *
st_llvmpipe_screen_create(void)
{
   static struct llvmpipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = CALLOC_STRUCT(llvmpipe_winsys);
   if (!winsys)
      goto no_winsys;

   winsys->destroy = llvmpipe_ws_destroy;
   winsys->is_displaytarget_format_supported = llvmpipe_ws_is_displaytarget_format_supported;
   winsys->displaytarget_create = llvmpipe_ws_displaytarget_create;
   winsys->displaytarget_map = llvmpipe_ws_displaytarget_map;
   winsys->displaytarget_unmap = llvmpipe_ws_displaytarget_unmap;
   winsys->displaytarget_display = llvmpipe_ws_displaytarget_display;
   winsys->displaytarget_destroy = llvmpipe_ws_displaytarget_destroy;

   screen = llvmpipe_create_screen(winsys);
   if (!screen)
      goto no_screen;

   return screen;

no_screen:
   FREE(winsys);
no_winsys:
   return NULL;
}



const struct st_winsys st_softpipe_winsys = {
   &st_llvmpipe_screen_create
};
