/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_tex_layout.h"
#include "brw_winsys.h"

#include "pipe/p_winsys.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "pipe/p_screen.h"


#ifndef BRW_DEBUG
int BRW_DEBUG = (0);
#endif


static void brw_destroy(struct pipe_context *pipe)
{
   struct brw_context *brw = brw_context(pipe);

   if(brw->winsys->destroy)
      brw->winsys->destroy(brw->winsys);
   
   FREE(brw);
}


static void brw_clear(struct pipe_context *pipe, struct pipe_surface *ps,
                      unsigned clearValue)
{
   int x, y, w, h;
   /* FIXME: corny... */

   x = 0;
   y = 0;
   w = ps->width;
   h = ps->height;

   pipe->surface_fill(pipe, ps, x, y, w, h, clearValue);
}


struct pipe_context *brw_create(struct pipe_screen *screen,
                                struct brw_winsys *brw_winsys,
                                unsigned pci_id)
{
   struct brw_context *brw;

   debug_printf("%s: creating brw_context with pci id 0x%x\n",
                __FUNCTION__, pci_id);

   brw = CALLOC_STRUCT(brw_context);
   if (brw == NULL)
      return NULL;

   brw->winsys = brw_winsys;
   brw->pipe.winsys = screen->winsys;
   brw->pipe.screen = screen;

   brw->pipe.destroy = brw_destroy;
   brw->pipe.clear = brw_clear;

   brw_init_surface_functions(brw);
   brw_init_texture_functions(brw);
   brw_init_state_functions(brw);
   brw_init_flush_functions(brw);
   brw_init_draw_functions( brw );


   brw_init_state( brw );

   brw->pci_id = pci_id;
   brw->dirty = ~0;
   brw->hardware_dirty = ~0;

   memset(&brw->wm.bind, ~0, sizeof(brw->wm.bind));

   return &brw->pipe;
}

