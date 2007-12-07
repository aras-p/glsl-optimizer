/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file xm_surface.c
 * Code to allow the softpipe code to write to X windows/buffers.
 * This is a bit of a hack for now.  We've basically got two different
 * abstractions for color buffers: gl_renderbuffer and pipe_surface.
 * They'll need to get merged someday...
 * For now, they're separate things that point to each other.
 */


#include "GL/xmesa.h"
#include "glxheader.h"
#include "xmesaP.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/softpipe/sp_context.h"
#include "pipe/softpipe/sp_clear.h"
#include "pipe/softpipe/sp_tile_cache.h"
#include "pipe/softpipe/sp_surface.h"
#include "state_tracker/st_context.h"


/*
 * Dithering kernels and lookup tables.
 */

const int xmesa_kernel8[DITH_DY * DITH_DX] = {
    0 * MAXC,  8 * MAXC,  2 * MAXC, 10 * MAXC,
   12 * MAXC,  4 * MAXC, 14 * MAXC,  6 * MAXC,
    3 * MAXC, 11 * MAXC,  1 * MAXC,  9 * MAXC,
   15 * MAXC,  7 * MAXC, 13 * MAXC,  5 * MAXC,
};

const int xmesa_kernel1[16] = {
   0*47,  9*47,  4*47, 12*47,     /* 47 = (255*3)/16 */
   6*47,  2*47, 14*47,  8*47,
  10*47,  1*47,  5*47, 11*47,
   7*47, 13*47,  3*47, 15*47
};




/** XXX unfinished sketch... */
struct pipe_surface *
xmesa_create_front_surface(XMesaVisual vis, Window win)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);
   if (!xms) {
      return NULL;
   }

   xms->display = vis->display;
   xms->drawable = win;

   xms->surface.format = PIPE_FORMAT_U_A8_R8_G8_B8;
   xms->surface.refcount = 1;
#if 0
   xms->surface.region = pipe->winsys->region_alloc(pipe->winsys,
                                                    1, 0, 0, 0x0);
#endif
   return &xms->surface;
}

