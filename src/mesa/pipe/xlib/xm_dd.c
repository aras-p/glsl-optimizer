/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file xm_dd.h
 * General device driver functions for Xlib driver.
 */

#include "glxheader.h"
#include "xmesaP.h"
#include "main/imports.h"
#include "main/mtypes.h"

#include "pipe/softpipe/sp_context.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_draw.h"


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


static void
finish_or_flush( GLcontext *ctx )
{
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (xmesa) {
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XSync( xmesa->display, False );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
#endif
   abort();
}


/**
 * Called when the driver should update its state, based on the new_state
 * flags.
 */
void
xmesa_update_state( GLcontext *ctx, GLbitfield new_state )
{
#if 0
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
#endif

   st_invalidate_state( ctx, new_state );


   if (ctx->DrawBuffer->Name != 0)
      return;

   /*
    * GL_DITHER, GL_READ/DRAW_BUFFER, buffer binding state, etc. effect
    * renderbuffer span/clear funcs.
    */
   if (new_state & (_NEW_COLOR | _NEW_BUFFERS)) {
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      struct xmesa_renderbuffer *front_xrb, *back_xrb;

      front_xrb = xmbuf->frontxrb;
      if (front_xrb) {
#if 0
         xmesa_set_renderbuffer_funcs(front_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         front_xrb->clearFunc = clear_pixmap;
#endif
      }

      back_xrb = xmbuf->backxrb;
      if (back_xrb) {
#if 0
         xmesa_set_renderbuffer_funcs(back_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         if (xmbuf->backxrb->pixmap) {
            back_xrb->clearFunc = clear_pixmap;
         }
         else {
            switch (xmesa->xm_visual->BitsPerPixel) {
            case 8:
               back_xrb->clearFunc = clear_8bit_ximage;
               break;
            case 16:
               back_xrb->clearFunc = clear_16bit_ximage;
               break;
            case 24:
               back_xrb->clearFunc = clear_24bit_ximage;
               break;
            case 32:
               back_xrb->clearFunc = clear_32bit_ximage;
               break;
            default:
               back_xrb->clearFunc = clear_nbit_ximage;
               break;
            }
         }
#endif
      }
   }
}



/**
 * Called by glViewport.
 * This is a good time for us to poll the current X window size and adjust
 * our renderbuffers to match the current window size.
 * Remember, we have no opportunity to respond to conventional
 * X Resize/StructureNotify events since the X driver has no event loop.
 * Thus, we poll.
 * Note that this trick isn't fool-proof.  If the application never calls
 * glViewport, our notion of the current window size may be incorrect.
 * That problem led to the GLX_MESA_resize_buffers extension.
 */
static void
xmesa_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   XMesaBuffer xmdrawbuf = XMESA_BUFFER(ctx->WinSysDrawBuffer);
   XMesaBuffer xmreadbuf = XMESA_BUFFER(ctx->WinSysReadBuffer);
   xmesa_check_and_update_buffer_size(xmctx, xmdrawbuf);
   xmesa_check_and_update_buffer_size(xmctx, xmreadbuf);
   (void) x;
   (void) y;
   (void) w;
   (void) h;
}


/**
 * Initialize the device driver function table with the functions
 * we implement in this driver.
 */
void
xmesa_init_driver_functions( XMesaVisual xmvisual,
                             struct dd_function_table *driver )
{
   driver->UpdateState = xmesa_update_state;
   driver->Flush = finish_or_flush;
   driver->Finish = finish_or_flush;
   driver->Viewport = xmesa_viewport;
}
