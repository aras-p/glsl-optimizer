/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

#include "glxheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "xmesaP.h"

#include "swrast/swrast.h"


/*
 * The following functions are used to trap XGetImage() calls which
 * generate BadMatch errors if the drawable isn't mapped.
 */

static int caught_xgetimage_error = 0;
static int (*old_xerror_handler)( XMesaDisplay *dpy, XErrorEvent *ev );
static unsigned long xgetimage_serial;

/*
 * This is the error handler which will be called if XGetImage fails.
 */
static int xgetimage_error_handler( XMesaDisplay *dpy, XErrorEvent *ev )
{
   if (ev->serial==xgetimage_serial && ev->error_code==BadMatch) {
      /* caught the expected error */
      caught_xgetimage_error = 0;
   }
   else {
      /* call the original X error handler, if any.  otherwise ignore */
      if (old_xerror_handler) {
         (*old_xerror_handler)( dpy, ev );
      }
   }
   return 0;
}


/*
 * Call this right before XGetImage to setup error trap.
 */
static void catch_xgetimage_errors( XMesaDisplay *dpy )
{
   xgetimage_serial = NextRequest( dpy );
   old_xerror_handler = XSetErrorHandler( xgetimage_error_handler );
   caught_xgetimage_error = 0;
}


/*
 * Call this right after XGetImage to check if an error occured.
 */
static int check_xgetimage_errors( void )
{
   /* restore old handler */
   (void) XSetErrorHandler( old_xerror_handler );
   /* return 0=no error, 1=error caught */
   return caught_xgetimage_error;
}


/*
 * Read a pixel from an X drawable.
 */
static unsigned long read_pixel( XMesaDisplay *dpy,
                                 XMesaDrawable d, int x, int y )
{
   unsigned long p;
   XMesaImage *pixel = NULL;
   int error;

   catch_xgetimage_errors( dpy );
   pixel = XGetImage( dpy, d, x, y, 1, 1, AllPlanes, ZPixmap );
   error = check_xgetimage_errors();
   if (pixel && !error) {
      p = XMesaGetPixel( pixel, 0, 0 );
   }
   else {
      p = 0;
   }
   if (pixel) {
      XMesaDestroyImage( pixel );
   }
   return p;
}



/*
 * The Mesa library needs to be able to draw pixels in a number of ways:
 *   1. RGB vs Color Index
 *   2. as horizontal spans (polygons, images) vs random locations (points,
 *      lines)
 *   3. different color per-pixel or same color for all pixels
 *
 * Furthermore, the X driver needs to support rendering to 3 possible
 * "buffers", usually one, but sometimes two at a time:
 *   1. The front buffer as an X window
 *   2. The back buffer as a Pixmap
 *   3. The back buffer as an XImage
 *
 * Finally, if the back buffer is an XImage, we can avoid using XPutPixel and
 * optimize common cases such as 24-bit and 8-bit modes.
 *
 * By multiplication, there's at least 48 possible combinations of the above.
 *
 * Below are implementations of the most commonly used combinations.  They are
 * accessed through function pointers which get initialized here and are used
 * directly from the Mesa library.  The 8 function pointers directly correspond
 * to the first 3 cases listed above.
 *
 *
 * The function naming convention is:
 *
 *   [put|get]_[row|values]_[format]_[pixmap|ximage]
 *
 * New functions optimized for specific cases can be added without too much
 * trouble.  An example might be the 24-bit TrueColor mode 8A8R8G8B which is
 * found on IBM RS/6000 X servers.
 */







/**********************************************************************/
/*****                      Pixel reading                         *****/
/**********************************************************************/

/**
 * Do clip testing prior to calling XGetImage.  If any of the region lies
 * outside the screen's bounds, XGetImage will return NULL.
 * We use XTranslateCoordinates() to check if that's the case and
 * adjust the x, y and length parameters accordingly.
 * \return  -1 if span is totally clipped away,
 *          else return number of pixels to skip in the destination array.
 */
static int
clip_for_xgetimage(struct gl_context *ctx, XMesaPixmap pixmap, GLuint *n, GLint *x, GLint *y)
{
   XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer source = XMESA_BUFFER(ctx->DrawBuffer);
   Window rootWin = RootWindow(xmesa->display, 0);
   Window child;
   GLint screenWidth = WidthOfScreen(DefaultScreenOfDisplay(xmesa->display));
   GLint dx, dy;
   if (source->type == PBUFFER || source->type == PIXMAP)
      return 0;
   XTranslateCoordinates(xmesa->display, pixmap, rootWin,
                         *x, *y, &dx, &dy, &child);
   if (dx >= screenWidth) {
      /* totally clipped on right */
      return -1;
   }
   if (dx < 0) {
      /* clipped on left */
      GLint clip = -dx;
      if (clip >= (GLint) *n)
         return -1;  /* totally clipped on left */
      *x += clip;
      *n -= clip;
      dx = 0;
      return clip;
   }
   if ((GLint) (dx + *n) > screenWidth) {
      /* clipped on right */
      GLint clip = dx + *n - screenWidth;
      *n -= clip;
   }
   return 0;
}


#define GET_XRB(XRB) \
   struct xmesa_renderbuffer *XRB = xmesa_renderbuffer(rb)





/**
 * Initialize the renderbuffer's PutRow, GetRow, etc. functions.
 * This would generally only need to be called once when the renderbuffer
 * is created.  However, we can change pixel formats on the fly if dithering
 * is enabled/disabled.  Therefore, we may call this more often than that.
 */
void
xmesa_set_renderbuffer_funcs(struct xmesa_renderbuffer *xrb,
                             enum pixel_format pixelformat, GLint depth)
{
   /* XXX remove this */
}

