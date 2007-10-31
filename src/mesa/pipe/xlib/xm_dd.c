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
#include "bufferobj.h"
#include "buffers.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "drawpix.h"
#include "extensions.h"
#include "framebuffer.h"
#include "macros.h"
#include "image.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
#include "texformat.h"
#include "xmesaP.h"

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

const short xmesa_HPCR_DRGB[3][2][16] = {
   {
      { 16, -4,  1,-11, 14, -6,  3, -9, 15, -5,  2,-10, 13, -7,  4, -8},
      {-15,  5,  0, 12,-13,  7, -2, 10,-14,  6, -1, 11,-12,  8, -3,  9}
   },
   {
      {-11, 15, -7,  3, -8, 14, -4,  2,-10, 16, -6,  4, -9, 13, -5,  1},
      { 12,-14,  8, -2,  9,-13,  5, -1, 11,-15,  7, -3, 10,-12,  6,  0}
   },
   {
      {  6,-18, 26,-14,  2,-22, 30,-10,  8,-16, 28,-12,  4,-20, 32, -8},
      { -4, 20,-24, 16,  0, 24,-28, 12, -6, 18,-26, 14, -2, 22,-30, 10}
   }
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


/**********************************************************************/
/*** glClear implementations                                        ***/
/**********************************************************************/


/**
 * Clear the front or back color buffer, if it's implemented with a pixmap.
 */
static void
clear_pixmap(GLcontext *ctx, struct xmesa_renderbuffer *xrb, GLuint value)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

   assert(xmbuf);
   assert(xrb->pixmap);
   assert(xmesa);
   assert(xmesa->display);
   assert(xrb->pixmap);
   assert(xmbuf->cleargc);

   XMesaSetForeground( xmesa->display, xmbuf->cleargc, value );

   XMesaFillRectangle( xmesa->display, xrb->pixmap, xmbuf->cleargc,
                       0, 0, xrb->St.Base.Width, xrb->St.Base.Height);
}


static void
clear_8bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb, GLuint value)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   GLint i;
   for (i = 0; i < height; i++) {
      GLubyte *ptr = PIXEL_ADDR1(xrb, 0, i);
      MEMSET( ptr, xmesa->clearpixel, width );
   }
}


static void
clear_HPCR_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb, GLuint value)
                   
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   GLint i;
   for (i = 0; i < height; i++) {
      GLubyte *ptr = PIXEL_ADDR1( xrb, 0, i );
      int j;
      const GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
      if (i & 1) {
         sptr += 16;
      }
      for (j = 0; j < width; j++) {
         *ptr = sptr[j&15];
         ptr++;
      }
   }
}


static void
clear_16bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb, GLuint value)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   GLint i, j;

   if (xmesa->swapbytes) {
      value = ((value >> 8) & 0x00ff) | ((value << 8) & 0xff00);
   }

   for (j = 0; j < height; j++) {
      GLushort *ptr2 = PIXEL_ADDR2(xrb, 0, j);
      for (i = 0; i < width; i++) {
         ptr2[i] = value;
      }
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLuint value)
{
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   const GLubyte r = (value      ) & 0xff;
   const GLubyte g = (value >>  8) & 0xff;
   const GLubyte b = (value >> 16) & 0xff;

   if (r == g && g == b) {
      /* same value for all three components (gray) */
      GLint j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, 0, j);
         MEMSET(ptr3, r, 3 * width);
      }
   }
   else {
      /* non-gray clear color */
      GLint i, j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, 0, j);
         for (i = 0; i < width; i++) {
            ptr3->r = r;
            ptr3->g = g;
            ptr3->b = b;
            ptr3++;
         }
      }
   }
}


static void
clear_32bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLuint value)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   const GLuint n = width * height;
   GLuint *ptr4 = (GLuint *) xrb->ximage->data;

   if (!xrb->ximage)
      return;

   if (xmesa->swapbytes) {
      value = ((value >> 24) & 0x000000ff)
            | ((value >> 8)  & 0x0000ff00)
            | ((value << 8)  & 0x00ff0000)
            | ((value << 24) & 0xff000000);
   }

   if (value == 0) {
      /* common case */
      _mesa_memset(ptr4, value, 4 * n);
   }
   else {
      GLuint i;
      for (i = 0; i < n; i++)
         ptr4[i] = value;
   }
}


static void
clear_nbit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb, GLuint value)
{
   XMesaImage *img = xrb->ximage;
   GLint width = xrb->St.Base.Width;
   GLint height = xrb->St.Base.Height;
   GLint i, j;
   for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
         XMesaPutPixel(img, i, j, value);
      }
   }
}


static void
clear_color_HPCR_ximage( GLcontext *ctx, const GLfloat color[4] )
{
   int i;
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);

   if (color[0] == 0.0 && color[1] == 0.0 && color[2] == 0.0) {
      /* black is black */
      MEMSET( xmesa->xm_visual->hpcr_clear_ximage_pattern, 0x0 ,
              sizeof(xmesa->xm_visual->hpcr_clear_ximage_pattern));
   }
   else {
      /* build clear pattern */
      for (i=0; i<16; i++) {
         xmesa->xm_visual->hpcr_clear_ximage_pattern[0][i] =
            DITHER_HPCR(i, 0,
                        xmesa->clearcolor[0],
                        xmesa->clearcolor[1],
                        xmesa->clearcolor[2]);
         xmesa->xm_visual->hpcr_clear_ximage_pattern[1][i]    =
            DITHER_HPCR(i, 1,
                        xmesa->clearcolor[0],
                        xmesa->clearcolor[1],
                        xmesa->clearcolor[2]);
      }
   }
}


static void
clear_color_HPCR_pixmap( GLcontext *ctx, const GLfloat color[4] )
{
   int i;
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);

   if (color[0] == 0.0 && color[1] == 0.0 && color[2] == 0.0) {
      /* black is black */
      for (i=0; i<16; i++) {
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 0, 0);
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 1, 0);
      }
   }
   else {
      for (i=0; i<16; i++) {
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 0,
                       DITHER_HPCR(i, 0,
                                   xmesa->clearcolor[0],
                                   xmesa->clearcolor[1],
                                   xmesa->clearcolor[2]));
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 1,
                       DITHER_HPCR(i, 1,
                                   xmesa->clearcolor[0],
                                   xmesa->clearcolor[1],
                                   xmesa->clearcolor[2]));
      }
   }
   /* change tile pixmap content */
   XMesaPutImage(xmesa->display,
		 (XMesaDrawable)xmesa->xm_visual->hpcr_clear_pixmap,
		 XMESA_BUFFER(ctx->DrawBuffer)->cleargc,
		 xmesa->xm_visual->hpcr_clear_ximage, 0, 0, 0, 0, 16, 2);
}


/**
 * Called when the driver should update its state, based on the new_state
 * flags.
 */
void
xmesa_update_state( GLcontext *ctx, GLbitfield new_state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

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
         xmesa_set_renderbuffer_funcs(front_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         front_xrb->clearFunc = clear_pixmap;
      }

      back_xrb = xmbuf->backxrb;
      if (back_xrb) {
         xmesa_set_renderbuffer_funcs(back_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         if (xmbuf->backxrb->pixmap) {
            back_xrb->clearFunc = clear_pixmap;
         }
         else {
            switch (xmesa->xm_visual->BitsPerPixel) {
            case 8:
               if (xmesa->xm_visual->hpcr_clear_flag) {
                  back_xrb->clearFunc = clear_HPCR_ximage;
               }
               else {
                  back_xrb->clearFunc = clear_8bit_ximage;
               }
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
      }
   }

   if (xmesa->xm_visual->hpcr_clear_flag) {
      /* this depends on whether we're drawing to the front or back buffer */
      /* XXX FIX THIS! */
#if 0
      if (pixmap) {
         ctx->Driver.ClearColor = clear_color_HPCR_pixmap;
      }
      else {
         ctx->Driver.ClearColor = clear_color_HPCR_ximage;
      }
#else
      (void) clear_color_HPCR_pixmap;
      (void) clear_color_HPCR_ximage;
#endif
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
