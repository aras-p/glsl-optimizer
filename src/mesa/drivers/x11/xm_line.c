/* $Id: xm_line.c,v 1.6 2000/11/05 18:26:12 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


/*
 * This file contains "accelerated" point, line, and triangle functions.
 * It should be fairly easy to write new special-purpose point, line or
 * triangle functions and hook them into this module.
 */


#include "glxheader.h"
#include "depth.h"
#include "macros.h"
#include "vb.h"
#include "types.h"
#include "xmesaP.h"

/* Internal swrast includes:
 */
#include "swrast/s_depth.h"
#include "swrast/s_points.h"
#include "swrast/s_lines.h"
#include "swrast/s_context.h"


/**********************************************************************/
/***                    Point rendering                             ***/
/**********************************************************************/


/*
 * Render an array of points into a pixmap, any pixel format.
 */
static void draw_points_ANY_pixmap( GLcontext *ctx, SWvertex *vert )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc2;

   if (xmesa->xm_visual->gl_visual->RGBAflag) {
      register int x, y;
      const GLubyte *color = vert->color;
      unsigned long pixel = xmesa_color_to_pixel( xmesa,
						  color[0], color[1], 
						  color[2], color[3], 
						  xmesa->pixelformat);
      XMesaSetForeground( dpy, gc, pixel );
      x =                         (GLint) vert->win[0];
      y = FLIP( xmesa->xm_buffer, (GLint) vert->win[1] );
      XMesaDrawPoint( dpy, buffer, gc, x, y);
   }
   else {
      /* Color index mode */
      register int x, y;
      XMesaSetForeground( dpy, gc, vert->index );
      x =                         (GLint) vert->win[0];
      y = FLIP( xmesa->xm_buffer, (GLint) vert->win[1] );
      XMesaDrawPoint( dpy, buffer, gc, x, y);
   }
}



/* Override the swrast point-selection function.  Try to use one of
 * our internal point functions, otherwise fall back to the standard
 * swrast functions.
 */
void xmesa_choose_point( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->Point.Size==1.0F && !ctx->Point.SmoothFlag 
       && swrast->_RasterMask==0
       && !ctx->Texture._ReallyEnabled) {
      swrast->Point = draw_points_ANY_pixmap;
   }
   else { 
      _swrast_choose_point( ctx );
   }
}



/**********************************************************************/
/***                      Line rendering                            ***/
/**********************************************************************/


#if 0
/*
 * Render a line into a pixmap, any pixel format.
 */
static void flat_pixmap_line( GLcontext *ctx,
                              SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   register int x0, y0, x1, y1;
   XMesaGC gc;
   unsigned long pixel;
   if (xmesa->xm_visual->gl_visual->RGBAflag) {
      const GLubyte *color = vert0->color;
      pixel = xmesa_color_to_pixel( xmesa, color[0], color[1], color[2], color[3],
                                    xmesa->pixelformat );
   }
   else {
      pixel = vert0->index;
   }
   gc = xmesa->xm_buffer->gc2;
   XMesaSetForeground( xmesa->display, gc, pixel );

   x0 =                         (GLint) vert0->win[0];
   y0 = FLIP( xmesa->xm_buffer, (GLint) vert0->win[1] );
   x1 =                         (GLint) vert1->win[0];
   y1 = FLIP( xmesa->xm_buffer, (GLint) vert1->win[1] );
   XMesaDrawLine( xmesa->display, xmesa->xm_buffer->buffer, gc,
		  x0, y0, x1, y1 );
}
#endif


/*
 * Draw a flat-shaded, PF_TRUECOLOR line into an XImage.
 */
static void flat_TRUECOLOR_line( GLcontext *ctx,
                                 SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   XMesaImage *img = xmesa->xm_buffer->backimage;
   unsigned long pixel;
   PACK_TRUECOLOR( pixel, color[0], color[1], color[2] );

#define INTERP_XY 1
#define CLIP_HACK 1
#define PLOT(X,Y) XMesaPutPixel( img, X, FLIP(xmesa->xm_buffer, Y), pixel );

#include "swrast/s_linetemp.h"
}



/*
 * Draw a flat-shaded, PF_8A8B8G8R line into an XImage.
 */
static void flat_8A8B8G8R_line( GLcontext *ctx,
                                SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLuint pixel = PACK_8B8G8R( color[0], color[1], color[2] );

#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = pixel;

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_8R8G8B line into an XImage.
 */
static void flat_8R8G8B_line( GLcontext *ctx,
                              SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLuint pixel = PACK_8R8G8B( color[0], color[1], color[2] );

#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = pixel;

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_8R8G8B24 line into an XImage.
 */
static void flat_8R8G8B24_line( GLcontext *ctx,
                              SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;

#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) {			\
      pixelPtr->r = color[RCOMP];	\
      pixelPtr->g = color[GCOMP];	\
      pixelPtr->b = color[BCOMP];	\
}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_5R6G5B line into an XImage.
 */
static void flat_5R6G5B_line( GLcontext *ctx,
                              SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLushort pixel = PACK_5R6G5B( color[0], color[1], color[2] );

#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = pixel;

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_DITHER_5R6G5B line into an XImage.
 */
static void flat_DITHER_5R6G5B_line( GLcontext *ctx,
                                     SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;

#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) PACK_TRUEDITHER( *pixelPtr, X, Y, color[0], color[1], color[2] );

#include "swrast/s_linetemp.h"
}



/*
 * Draw a flat-shaded, PF_DITHER 8-bit line into an XImage.
 */
static void flat_DITHER8_line( GLcontext *ctx,
                               SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLint r = color[0], g = color[1], b = color[2];
   DITHER_SETUP;

#define INTERP_XY 1
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = DITHER(X,Y,r,g,b);

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_LOOKUP 8-bit line into an XImage.
 */
static void flat_LOOKUP8_line( GLcontext *ctx,
                               SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLubyte pixel;
   LOOKUP_SETUP;
   pixel = (GLubyte) LOOKUP( color[0], color[1], color[2] );

#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = pixel;

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, PF_HPCR line into an XImage.
 */
static void flat_HPCR_line( GLcontext *ctx,
                            SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLint r = color[0], g = color[1], b = color[2];

#define INTERP_XY 1
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y) *pixelPtr = (GLubyte) DITHER_HPCR(X,Y,r,g,b);

#include "swrast/s_linetemp.h"
}



/*
 * Draw a flat-shaded, Z-less, PF_TRUECOLOR line into an XImage.
 */
static void flat_TRUECOLOR_z_line( GLcontext *ctx,
                                   SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   XMesaImage *img = xmesa->xm_buffer->backimage;
   unsigned long pixel;
   PACK_TRUECOLOR( pixel, color[0], color[1], color[2] );

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define CLIP_HACK 1
#define PLOT(X,Y)							\
	if (Z < *zPtr) {						\
	   *zPtr = Z;							\
           XMesaPutPixel( img, X, FLIP(xmesa->xm_buffer, Y), pixel );	\
	}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_8A8B8G8R line into an XImage.
 */
static void flat_8A8B8G8R_z_line( GLcontext *ctx,
                                  SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLuint pixel = PACK_8B8G8R( color[0], color[1], color[2] );

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)		\
	if (Z < *zPtr) {	\
	   *zPtr = Z;		\
	   *pixelPtr = pixel;	\
	}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_8R8G8B line into an XImage.
 */
static void flat_8R8G8B_z_line( GLcontext *ctx,
                                SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLuint pixel = PACK_8R8G8B( color[0], color[1], color[2] );

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)		\
	if (Z < *zPtr) {	\
	   *zPtr = Z;		\
	   *pixelPtr = pixel;	\
	}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_8R8G8B24 line into an XImage.
 */
static void flat_8R8G8B24_z_line( GLcontext *ctx,
                                    SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)			\
	if (Z < *zPtr) {		\
	   *zPtr = Z;			\
           pixelPtr->r = color[RCOMP];	\
           pixelPtr->g = color[GCOMP];	\
           pixelPtr->b = color[BCOMP];	\
	}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_5R6G5B line into an XImage.
 */
static void flat_5R6G5B_z_line( GLcontext *ctx,
                                SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLushort pixel = PACK_5R6G5B( color[0], color[1], color[2] );

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)		\
	if (Z < *zPtr) {	\
	   *zPtr = Z;		\
	   *pixelPtr = pixel;	\
	}
#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_DITHER_5R6G5B line into an XImage.
 */
static void flat_DITHER_5R6G5B_z_line( GLcontext *ctx,
                                       SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)		\
	if (Z < *zPtr) {	\
	   *zPtr = Z;		\
	   PACK_TRUEDITHER(*pixelPtr, X, Y, color[0], color[1], color[2]); \
	}
#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_DITHER 8-bit line into an XImage.
 */
static void flat_DITHER8_z_line( GLcontext *ctx,
                                 SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLint r = color[0], g = color[1], b = color[2];
   DITHER_SETUP;

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)						\
	if (Z < *zPtr) {					\
	   *zPtr = Z;						\
	   *pixelPtr = (GLubyte) DITHER( X, Y, r, g, b);	\
	}
#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_LOOKUP 8-bit line into an XImage.
 */
static void flat_LOOKUP8_z_line( GLcontext *ctx,
                                 SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLubyte pixel;
   LOOKUP_SETUP;
   pixel = (GLubyte) LOOKUP( color[0], color[1], color[2] );

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)		\
	if (Z < *zPtr) {	\
	   *zPtr = Z;		\
	   *pixelPtr = pixel;	\
	}

#include "swrast/s_linetemp.h"
}


/*
 * Draw a flat-shaded, Z-less, PF_HPCR line into an XImage.
 */
static void flat_HPCR_z_line( GLcontext *ctx,
                              SWvertex *vert0, SWvertex *vert1 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = vert0->color;
   GLint r = color[0], g = color[1], b = color[2];

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define CLIP_HACK 1
#define PLOT(X,Y)						\
	if (Z < *zPtr) {					\
	   *zPtr = Z;						\
	   *pixelPtr = (GLubyte) DITHER_HPCR( X, Y, r, g, b);	\
	}

#include "swrast/s_linetemp.h"
}


#if 0
/*
 * Examine ctx->Line attributes and set xmesa->xm_buffer->gc1
 * and xmesa->xm_buffer->gc2 appropriately.
 */
static void setup_x_line_options( GLcontext *ctx )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   int i, state, state0, new_state, len, offs;
   int tbit;
   char *dptr;
   int n_segments = 0;
   char dashes[20];
   int line_width, line_style;

   /*** Line Stipple ***/
   if (ctx->Line.StippleFlag) {
      const int pattern = ctx->Line.StipplePattern;

      dptr = dashes;
      state0 = state = ((pattern & 1) != 0);

      /* Decompose the pattern */
      for (i=1,tbit=2,len=1;i<16;++i,tbit=(tbit<<1))
	{
	  new_state = ((tbit & pattern) != 0);
	  if (state != new_state)
	    {
	      *dptr++ = ctx->Line.StippleFactor * len;
	      len = 1;
	      state = new_state;
	    }
	  else
	    ++len;
	}
      *dptr = ctx->Line.StippleFactor * len;
      n_segments = 1 + (dptr - dashes);

      /* ensure an even no. of segments, or X may toggle on/off for consecutive patterns */
      /* if (n_segments & 1)  dashes [n_segments++] = 0;  value of 0 not allowed in dash list */

      /* Handle case where line style starts OFF */
      if (state0 == 0)
        offs = dashes[0];
      else
        offs = 0;

#if 0
fprintf (stderr, "input pattern: 0x%04x, offset %d, %d segments:", pattern, offs, n_segments);
for (i = 0;  i < n_segments;  i++)
fprintf (stderr, " %d", dashes[i]);
fprintf (stderr, "\n");
#endif

      XMesaSetDashes( xmesa->display, xmesa->xm_buffer->gc1,
		      offs, dashes, n_segments );
      XMesaSetDashes( xmesa->display, xmesa->xm_buffer->gc2,
		      offs, dashes, n_segments );

      line_style = LineOnOffDash;
   }
   else {
      line_style = LineSolid;
   }

   /*** Line Width ***/
   line_width = (int) (ctx->Line.Width+0.5F);
   if (line_width < 2) {
      /* Use fast lines when possible */
      line_width = 0;
   }

   /*** Set GC attributes ***/
   XMesaSetLineAttributes( xmesa->display, xmesa->xm_buffer->gc1,
			   line_width, line_style, CapButt, JoinBevel);
   XMesaSetLineAttributes( xmesa->display, xmesa->xm_buffer->gc2,
			   line_width, line_style, CapButt, JoinBevel);
   XMesaSetFillStyle( xmesa->display, xmesa->xm_buffer->gc1, FillSolid );
   XMesaSetFillStyle( xmesa->display, xmesa->xm_buffer->gc2, FillSolid );
}
#endif

 
static swrast_line_func get_line_func( GLcontext *ctx )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   int depth = GET_VISUAL_DEPTH(xmesa->xm_visual);

   (void) DitherValues;  /* silence unused var warning */
   (void) kernel1;  /* silence unused var warning */

   if (ctx->Line.SmoothFlag)              return (swrast_line_func)NULL;
   if (ctx->Texture._ReallyEnabled)        return (swrast_line_func)NULL;
   if (ctx->Light.ShadeModel!=GL_FLAT)    return (swrast_line_func)NULL;
   /* X line stippling doesn't match OpenGL stippling */
   if (ctx->Line.StippleFlag==GL_TRUE)    return (swrast_line_func)NULL;

   if (xmesa->xm_buffer->buffer==XIMAGE
       && swrast->_RasterMask==DEPTH_BIT
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_TRUE
       && ctx->Visual.DepthBits == DEFAULT_SOFTWARE_DEPTH_BITS
       && ctx->Line.Width==1.0F) {
      switch (xmesa->pixelformat) {
         case PF_TRUECOLOR:
            return flat_TRUECOLOR_z_line;
         case PF_8A8B8G8R:
            return flat_8A8B8G8R_z_line;
         case PF_8R8G8B:
            return flat_8R8G8B_z_line;
         case PF_8R8G8B24:
            return flat_8R8G8B24_z_line;
         case PF_5R6G5B:
            return flat_5R6G5B_z_line;
         case PF_DITHER_5R6G5B:
            return flat_DITHER_5R6G5B_z_line;
         case PF_DITHER:
            return (depth==8) ? flat_DITHER8_z_line : (swrast_line_func)NULL;
         case PF_LOOKUP:
            return (depth==8) ? flat_LOOKUP8_z_line : (swrast_line_func)NULL;
         case PF_HPCR:
            return flat_HPCR_z_line;
         default:
            return (swrast_line_func)NULL;
      }
   }
   if (xmesa->xm_buffer->buffer==XIMAGE
       && swrast->_RasterMask==0
       && ctx->Line.Width==1.0F) {
      switch (xmesa->pixelformat) {
         case PF_TRUECOLOR:
            return flat_TRUECOLOR_line;
         case PF_8A8B8G8R:
            return flat_8A8B8G8R_line;
         case PF_8R8G8B:
            return flat_8R8G8B_line;
         case PF_8R8G8B24:
            return flat_8R8G8B24_line;
         case PF_5R6G5B:
            return flat_5R6G5B_line;
         case PF_DITHER_5R6G5B:
            return flat_DITHER_5R6G5B_line;
         case PF_DITHER:
            return (depth==8) ? flat_DITHER8_line : (swrast_line_func)NULL;
         case PF_LOOKUP:
            return (depth==8) ? flat_LOOKUP8_line : (swrast_line_func)NULL;
         case PF_HPCR:
            return flat_HPCR_line;
	 default:
	    return (swrast_line_func)NULL;
      }
   }
#if 0
   /* XXX have to disable this because X's rasterization rules don't match
    * software Mesa's.  This causes the linehv.c conformance test to fail.
    * In the future, we might provide a config option to enable this.
    */
   if (xmesa->xm_buffer->buffer!=XIMAGE && ctx->_RasterMask==0) {
      setup_x_line_options( ctx );
      return flat_pixmap_line;
   }
#endif
   return (swrast_line_func)NULL;
}

/* Override for the swrast line-selection function.  Try to use one
 * of our internal line functions, otherwise fall back to the
 * standard swrast functions.  
 */
void xmesa_choose_line( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (!(swrast->Line = get_line_func( ctx )))
      _swrast_choose_line( ctx );
}


#define XMESA_NEW_POINT  (_NEW_POINT|_SWRAST_NEW_RASTERMASK)
#define XMESA_NEW_LINE   (_NEW_LINE|_NEW_TEXTURE|_NEW_LIGHT|\
                          _NEW_DEPTH|_SWRAST_NEW_RASTERMASK)
#define XMESA_NEW_TRIANGLE (_NEW_POLYGON|_NEW_TEXTURE|_NEW_LIGHT|\
                            _SWRAST_NEW_RASTERMASK|_NEW_DEPTH)


/* Extend the software rasterizer with our line/point/triangle 
 * functions.
 */
void xmesa_register_swrast_functions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );
   
   swrast->choose_point = xmesa_choose_point;
   swrast->choose_line = xmesa_choose_line;
   swrast->choose_triangle = xmesa_choose_triangle;
   
   swrast->invalidate_point |= XMESA_NEW_POINT;
   swrast->invalidate_line |= XMESA_NEW_LINE;
   swrast->invalidate_triangle |= XMESA_NEW_TRIANGLE;
}
