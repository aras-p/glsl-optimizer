/* $Id: xm_tri.c,v 1.10 2000/11/16 21:05:40 keithw Exp $ */

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
 * This file contains "accelerated" triangle functions.  It should be
 * fairly easy to write new special-purpose triangle functions and hook
 * them into this module.
 */


#include "glxheader.h"
#include "depth.h"
#include "macros.h"
#include "types.h"
#include "xmesaP.h"

/* Internal swrast includes:
 */
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_triangle.h"




/**********************************************************************/
/***                   Triangle rendering                           ***/
/**********************************************************************/


/*
 * XImage, smooth, depth-buffered, PF_TRUECOLOR triangle.
 */
static void smooth_TRUECOLOR_z_triangle( GLcontext *ctx,
                                         SWvertex *v0,
					 SWvertex *v1,
					 SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer, Y);			\
   GLint len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         unsigned long p;						\
         PACK_TRUECOLOR(p, FixedToInt(ffr), FixedToInt(ffg), FixedToInt(ffb));\
         XMesaPutPixel( img, xx, yy, p );				\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}



/*
 * XImage, smooth, depth-buffered, PF_8A8B8G8R triangle.
 */
static void smooth_8A8B8G8R_z_triangle( GLcontext *ctx,
					SWvertex *v0,
					SWvertex *v1,
					SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_8R8G8B triangle.
 */
static void smooth_8R8G8B_z_triangle( GLcontext *ctx,
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_8R8G8B24 triangle.
 */
static void smooth_8R8G8B24_z_triangle( GLcontext *ctx,
                                        SWvertex *v0,
					SWvertex *v1,
					SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
	 PIXEL_TYPE *ptr = pRow + i;					\
         ptr->r = FixedToInt(ffr);					\
         ptr->g = FixedToInt(ffg);					\
         ptr->b = FixedToInt(ffb);					\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_TRUEDITHER triangle.
 */
static void smooth_TRUEDITHER_z_triangle( GLcontext *ctx,
					  SWvertex *v0,
					  SWvertex *v1,
					  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         unsigned long p;						\
         PACK_TRUEDITHER( p, xx, yy, FixedToInt(ffr),			\
                          FixedToInt(ffg), FixedToInt(ffb) );		\
         XMesaPutPixel( img, xx, yy, p );				\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_5R6G5B triangle.
 */
static void smooth_5R6G5B_z_triangle( GLcontext *ctx,
                                      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      DEPTH_TYPE z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_5R6G5B( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_DITHER_5R6G5B triangle.
 */
static void smooth_DITHER_5R6G5B_z_triangle( GLcontext *ctx,
                                             SWvertex *v0,
					     SWvertex *v1,
					     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         PACK_TRUEDITHER(pRow[i], LEFT+i, Y, FixedToInt(ffr),		\
			 FixedToInt(ffg), FixedToInt(ffb) );		\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, 8-bit, PF_DITHER8 triangle.
 */
static void smooth_DITHER8_z_triangle( GLcontext *ctx,
                                       SWvertex *v0,
				       SWvertex *v1,
				       SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   XDITHER_SETUP(yy);							\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = (PIXEL_TYPE) XDITHER( xx, FixedToInt(ffr),		\
			FixedToInt(ffg), FixedToInt(ffb) );		\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, PF_DITHER triangle.
 */
static void smooth_DITHER_z_triangle( GLcontext *ctx,
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   XDITHER_SETUP(yy);							\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
	 unsigned long p = XDITHER( xx, FixedToInt(ffr),		\
				 FixedToInt(ffg), FixedToInt(ffb) );	\
	 XMesaPutPixel( img, xx, yy, p );			       	\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, depth-buffered, 8-bit PF_LOOKUP triangle.
 */
static void smooth_LOOKUP8_z_triangle( GLcontext *ctx, 
				       SWvertex *v0,
				       SWvertex *v1,
				       SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   LOOKUP_SETUP;							\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = LOOKUP( FixedToInt(ffr), FixedToInt(ffg),		\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}



/*
 * XImage, smooth, depth-buffered, 8-bit PF_HPCR triangle.
 */
static void smooth_HPCR_z_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = DITHER_HPCR( xx, yy, FixedToInt(ffr),		\
				 FixedToInt(ffg), FixedToInt(ffb) );	\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_TRUECOLOR triangle.
 */
static void flat_TRUECOLOR_z_triangle( GLcontext *ctx,
                        	       SWvertex *v0,
				       SWvertex *v1,
				       SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE						\
   unsigned long pixel;						\
   PACK_TRUECOLOR(pixel, v0->color[0], v0->color[1], v0->color[2]);

#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         XMesaPutPixel( img, xx, yy, pixel );				\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_8A8B8G8R triangle.
 */
static void flat_8A8B8G8R_z_triangle( GLcontext *ctx, 
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE					\
   unsigned long p = PACK_8B8G8R( v0->color[0],	\
		 v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
	 pRow[i] = (PIXEL_TYPE) p;					\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_8R8G8B triangle.
 */
static void flat_8R8G8B_z_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE					\
   unsigned long p = PACK_8R8G8B( v0->color[0],	\
		 v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   (void) fffog; 					\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 pRow[i] = (PIXEL_TYPE) p;			\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_8R8G8B24 triangle.
 */
static void flat_8R8G8B24_z_triangle( GLcontext *ctx, 
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = v0->color;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   (void) fffog; 					\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 PIXEL_TYPE *ptr = pRow+i;			\
         ptr->r = color[RCOMP];				\
         ptr->g = color[GCOMP];				\
         ptr->b = color[BCOMP];				\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_TRUEDITHER triangle.
 */
static void flat_TRUEDITHER_z_triangle( GLcontext *ctx, 
					SWvertex *v0,
					SWvertex *v1,
					SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         unsigned long p;						\
         PACK_TRUEDITHER( p, xx, yy, v0->color[0],		\
            v0->color[1], v0->color[2] );	\
         XMesaPutPixel( img, xx, yy, p );				\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_5R6G5B triangle.
 */
static void flat_5R6G5B_z_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE							\
   unsigned long p = PACK_5R6G5B( v0->color[0],		\
            v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {				\
      DEPTH_TYPE z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 pRow[i] = (PIXEL_TYPE) p;			\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_DITHER_5R6G5B triangle.
 */
static void flat_DITHER_5R6G5B_z_triangle( GLcontext *ctx, 
					   SWvertex *v0,
					   SWvertex *v1,
					   SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = v0->color;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint i, len = RIGHT-LEFT;					\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {					\
      DEPTH_TYPE z = FixedToDepth(ffz);				\
      if (z < zRow[i]) {					\
	 PACK_TRUEDITHER(pRow[i], LEFT+i, Y, color[RCOMP],	\
			 color[GCOMP], color[BCOMP]);		\
         zRow[i] = z;						\
      }								\
      ffz += fdzdx;						\
   }								\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, 8-bit PF_DITHER triangle.
 */
static void flat_DITHER8_z_triangle( GLcontext *ctx, 
				     SWvertex *v0,
				     SWvertex *v1,
				     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE	\
   FLAT_DITHER_SETUP( v0->color[0], v0->color[1], v0->color[2] );

#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint i, xx = LEFT, len = RIGHT-LEFT;			\
   FLAT_DITHER_ROW_SETUP(FLIP(xmesa->xm_buffer, Y));		\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {					\
      GLdepth z = FixedToDepth(ffz);				\
      if (z < zRow[i]) {					\
	 pRow[i] = (PIXEL_TYPE) FLAT_DITHER(xx);		\
         zRow[i] = z;						\
      }								\
      ffz += fdzdx;						\
   }								\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, PF_DITHER triangle.
 */
static void flat_DITHER_z_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE	\
   FLAT_DITHER_SETUP( v0->color[0], v0->color[1], v0->color[2] );

#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   FLAT_DITHER_ROW_SETUP(yy);						\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         unsigned long p = FLAT_DITHER(xx);				\
	 XMesaPutPixel( img, xx, yy, p );				\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, 8-bit PF_HPCR triangle.
 */
static void flat_HPCR_z_triangle( GLcontext *ctx, 
				  SWvertex *v0,
				  SWvertex *v1,
				  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE				\
   GLubyte r = v0->color[0];	\
   GLubyte g = v0->color[1];	\
   GLubyte b = v0->color[2];
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, xx = LEFT, yy = FLIP(xmesa->xm_buffer,Y), len = RIGHT-LEFT;	\
   (void) fffog; 					        	\
   for (i=0;i<len;i++,xx++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
	 pRow[i] = (PIXEL_TYPE) DITHER_HPCR( xx, yy, r, g, b );		\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, depth-buffered, 8-bit PF_LOOKUP triangle.
 */
static void flat_LOOKUP8_z_triangle( GLcontext *ctx, 
				     SWvertex *v0,
				     SWvertex *v1,
				     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE				\
   LOOKUP_SETUP;				\
   GLubyte r = v0->color[0];	\
   GLubyte g = v0->color[1];	\
   GLubyte b = v0->color[2];	\
   GLubyte p = LOOKUP(r,g,b);
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   (void) fffog; 					        	\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 pRow[i] = p;					\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "swrast/s_tritemp.h"
}



/*
 * XImage, smooth, NON-depth-buffered, PF_TRUECOLOR triangle.
 */
static void smooth_TRUECOLOR_triangle( GLcontext *ctx, 
				       SWvertex *v0,
				       SWvertex *v1,
				       SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_RGB 1
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   for (xx=LEFT;xx<RIGHT;xx++) {					\
      unsigned long p;							\
      PACK_TRUECOLOR(p, FixedToInt(ffr), FixedToInt(ffg), FixedToInt(ffb));\
      XMesaPutPixel( img, xx, yy, p );					\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_8A8B8G8R triangle.
 */
static void smooth_8A8B8G8R_triangle( GLcontext *ctx, 
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),		\
				FixedToInt(ffb) );			\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_8R8G8B triangle.
 */
static void smooth_8R8G8B_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),		\
				FixedToInt(ffb) );			\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_8R8G8B triangle.
 */
static void smooth_8R8G8B24_triangle( GLcontext *ctx, 
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint xx;							\
   PIXEL_TYPE *pixel = pRow;					\
   for (xx=LEFT;xx<RIGHT;xx++) {				\
      pixel->r = FixedToInt(ffr);				\
      pixel->g = FixedToInt(ffg);				\
      pixel->b = FixedToInt(ffb);				\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;		\
      pixel++;							\
   }								\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_TRUEDITHER triangle.
 */
static void smooth_TRUEDITHER_triangle( GLcontext *ctx, 
					SWvertex *v0,
					SWvertex *v1,
					SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INTERP_RGB 1
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   for (xx=LEFT;xx<RIGHT;xx++) {					\
      unsigned long p;							\
      PACK_TRUEDITHER( p, xx, yy, FixedToInt(ffr), FixedToInt(ffg),	\
				FixedToInt(ffb) );			\
      XMesaPutPixel( img, xx, yy, p );					\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_5R6G5B triangle.
 */
static void smooth_5R6G5B_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = (PIXEL_TYPE) PACK_5R6G5B( FixedToInt(ffr),		\
				 FixedToInt(ffg), FixedToInt(ffb) );	\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_DITHER_5R6G5B triangle.
 */
static void smooth_DITHER_5R6G5B_triangle( GLcontext *ctx, 
					   SWvertex *v0,
					   SWvertex *v1,
					   SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      PACK_TRUEDITHER(*pixel, xx, Y, FixedToInt(ffr),			\
				 FixedToInt(ffg), FixedToInt(ffb) );	\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, 8-bit PF_DITHER triangle.
 */
static void smooth_DITHER8_triangle( GLcontext *ctx, 
				     SWvertex *v0,
				     SWvertex *v1,
				     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   PIXEL_TYPE *pixel = pRow;						\
   XDITHER_SETUP(yy);							\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = (PIXEL_TYPE) XDITHER( xx, FixedToInt(ffr),		\
				 FixedToInt(ffg), FixedToInt(ffb) );	\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, PF_DITHER triangle.
 */
static void smooth_DITHER_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;

#define INTERP_RGB 1
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   XDITHER_SETUP(yy);							\
   for (xx=LEFT;xx<RIGHT;xx++) {					\
      unsigned long p = XDITHER( xx, FixedToInt(ffr),			\
				FixedToInt(ffg), FixedToInt(ffb) );	\
      XMesaPutPixel( img, xx, yy, p );					\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, smooth, NON-depth-buffered, 8-bit PF_LOOKUP triangle.
 */
static void smooth_LOOKUP8_triangle( GLcontext *ctx, 
				     SWvertex *v0,
				     SWvertex *v1,
				     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;

#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   LOOKUP_SETUP;							\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = LOOKUP( FixedToInt(ffr), FixedToInt(ffg),		\
			FixedToInt(ffb) );				\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}



/*
 * XImage, smooth, NON-depth-buffered, 8-bit PF_HPCR triangle.
 */
static void smooth_HPCR_triangle( GLcontext *ctx, 
				  SWvertex *v0,
				  SWvertex *v1,
				  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;

#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = DITHER_HPCR( xx, yy, FixedToInt(ffr),			\
				FixedToInt(ffg), FixedToInt(ffb) );	\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_TRUECOLOR triangle.
 */
static void flat_TRUECOLOR_triangle( GLcontext *ctx, 
				     SWvertex *v0,
				     SWvertex *v1,
				     SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define SETUP_CODE						\
   unsigned long pixel;						\
   PACK_TRUECOLOR(pixel, v0->color[0], v0->color[1], v0->color[2]);

#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);			\
   for (xx=LEFT;xx<RIGHT;xx++) {				\
      XMesaPutPixel( img, xx, yy, pixel );			\
   }								\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_8A8B8G8R triangle.
 */
static void flat_8A8B8G8R_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE					\
   unsigned long p = PACK_8B8G8R( v0->color[0],	\
		 v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = (PIXEL_TYPE) p;				\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_8R8G8B triangle.
 */
static void flat_8R8G8B_triangle( GLcontext *ctx, 
				  SWvertex *v0,
				  SWvertex *v1,
				  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLuint
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE					\
   unsigned long p = PACK_8R8G8B( v0->color[0],	\
		 v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = (PIXEL_TYPE) p;				\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_8R8G8B24 triangle.
 */
static void flat_8R8G8B24_triangle( GLcontext *ctx, 
				    SWvertex *v0,
				    SWvertex *v1,
				    SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = v0->color;
#define PIXEL_ADDRESS(X,Y) PIXELADDR3(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE bgr_t
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++) {			\
      pixel->r = color[RCOMP];				\
      pixel->g = color[GCOMP];				\
      pixel->b = color[BCOMP];				\
      pixel++;						\
   }							\
}
#include "swrast/s_tritemp.h"
}

/*
 * XImage, flat, NON-depth-buffered, PF_TRUEDITHER triangle.
 */
static void flat_TRUEDITHER_triangle( GLcontext *ctx, 
				      SWvertex *v0,
				      SWvertex *v1,
				      SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);				\
   for (xx=LEFT;xx<RIGHT;xx++) {					\
      unsigned long p;							\
      PACK_TRUEDITHER( p, xx, yy, v0->color[0],		\
               v0->color[1], v0->color[2] );	\
      XMesaPutPixel( img, xx, yy, p );					\
   }									\
}
#include "swrast/s_tritemp.h"
}



/*
 * XImage, flat, NON-depth-buffered, PF_5R6G5B triangle.
 */
static void flat_5R6G5B_triangle( GLcontext *ctx, 
				  SWvertex *v0,
				  SWvertex *v1,
				  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE					\
   unsigned long p = PACK_5R6G5B( v0->color[0],	\
		 v0->color[1], v0->color[2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = (PIXEL_TYPE) p;				\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_DITHER_5R6G5B triangle.
 */
static void flat_DITHER_5R6G5B_triangle( GLcontext *ctx, 
					 SWvertex *v0,
					 SWvertex *v1,
					 SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte *color = v0->color;
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLushort
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      PACK_TRUEDITHER(*pixel, xx, Y, color[RCOMP],	\
                     color[GCOMP], color[BCOMP]);	\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, 8-bit PF_DITHER triangle.
 */
static void flat_DITHER8_triangle( GLcontext *ctx, 
				   SWvertex *v0,
				   SWvertex *v1,
				   SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE	\
   FLAT_DITHER_SETUP( v0->color[0], v0->color[1], v0->color[2] );

#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   FLAT_DITHER_ROW_SETUP(FLIP(xmesa->xm_buffer, Y));	\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = (PIXEL_TYPE) FLAT_DITHER(xx);		\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, PF_DITHER triangle.
 */
static void flat_DITHER_triangle( GLcontext *ctx, 
				  SWvertex *v0,
				  SWvertex *v1,
				  SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
#define SETUP_CODE	\
   FLAT_DITHER_SETUP( v0->color[0], v0->color[1], v0->color[2] );

#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);		\
   FLAT_DITHER_ROW_SETUP(yy);				\
   for (xx=LEFT;xx<RIGHT;xx++) {			\
      unsigned long p = FLAT_DITHER(xx);		\
      XMesaPutPixel( img, xx, yy, p );			\
   }							\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, 8-bit PF_HPCR triangle.
 */
static void flat_HPCR_triangle( GLcontext *ctx, 
				SWvertex *v0,
				SWvertex *v1,
				SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE				\
   GLubyte r = v0->color[0];	\
   GLubyte g = v0->color[1];	\
   GLubyte b = v0->color[2];
#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint xx, yy = FLIP(xmesa->xm_buffer, Y);			\
   PIXEL_TYPE *pixel = pRow;					\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {			\
      *pixel = (PIXEL_TYPE) DITHER_HPCR( xx, yy, r, g, b );	\
   }								\
}
#include "swrast/s_tritemp.h"
}


/*
 * XImage, flat, NON-depth-buffered, 8-bit PF_LOOKUP triangle.
 */
static void flat_LOOKUP8_triangle( GLcontext *ctx, 
				   SWvertex *v0,
				   SWvertex *v1,
				   SWvertex *v2 )
{
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(xmesa->xm_buffer,X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (xmesa->xm_buffer->backimage->bytes_per_line)
#define SETUP_CODE				\
   LOOKUP_SETUP;				\
   GLubyte r = v0->color[0];		\
   GLubyte g = v0->color[1];		\
   GLubyte b = v0->color[2];		\
   GLubyte p = LOOKUP(r,g,b);
#define INNER_LOOP( LEFT, RIGHT, Y )		\
{						\
   GLint xx;					\
   PIXEL_TYPE *pixel = pRow;			\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {	\
      *pixel = p;				\
   }						\
}
#include "swrast/s_tritemp.h"
}


#ifdef DEBUG
static void
_xmesa_print_triangle_func( swrast_tri_func triFunc )
{
   printf("XMesa tri func = ");
   if (triFunc ==smooth_TRUECOLOR_z_triangle)
      printf("smooth_TRUECOLOR_z_triangle\n");
   else if (triFunc ==smooth_8A8B8G8R_z_triangle)
      printf("smooth_8A8B8G8R_z_triangle\n");
   else if (triFunc ==smooth_8R8G8B_z_triangle)
      printf("smooth_8R8G8B_z_triangle\n");
   else if (triFunc ==smooth_8R8G8B24_z_triangle)
      printf("smooth_8R8G8B24_z_triangle\n");
   else if (triFunc ==smooth_TRUEDITHER_z_triangle)
      printf("smooth_TRUEDITHER_z_triangle\n");
   else if (triFunc ==smooth_5R6G5B_z_triangle)
      printf("smooth_5R6G5B_z_triangle\n");
   else if (triFunc ==smooth_DITHER_5R6G5B_z_triangle)
      printf("smooth_DITHER_5R6G5B_z_triangle\n");
   else if (triFunc ==smooth_HPCR_z_triangle)
      printf("smooth_HPCR_z_triangle\n");
   else if (triFunc ==smooth_DITHER8_z_triangle)
      printf("smooth_DITHER8_z_triangle\n");
   else if (triFunc ==smooth_LOOKUP8_z_triangle)
      printf("smooth_LOOKUP8_z_triangle\n");
   else if (triFunc ==flat_TRUECOLOR_z_triangle)
      printf("flat_TRUECOLOR_z_triangle\n");
   else if (triFunc ==flat_8A8B8G8R_z_triangle)
      printf("flat_8A8B8G8R_z_triangle\n");
   else if (triFunc ==flat_8R8G8B_z_triangle)
      printf("flat_8R8G8B_z_triangle\n");
   else if (triFunc ==flat_8R8G8B24_z_triangle)
      printf("flat_8R8G8B24_z_triangle\n");
   else if (triFunc ==flat_TRUEDITHER_z_triangle)
      printf("flat_TRUEDITHER_z_triangle\n");
   else if (triFunc ==flat_5R6G5B_z_triangle)
      printf("flat_5R6G5B_z_triangle\n");
   else if (triFunc ==flat_DITHER_5R6G5B_z_triangle)
      printf("flat_DITHER_5R6G5B_z_triangle\n");
   else if (triFunc ==flat_HPCR_z_triangle)
      printf("flat_HPCR_z_triangle\n");
   else if (triFunc ==flat_DITHER8_z_triangle)
      printf("flat_DITHER8_z_triangle\n");
   else if (triFunc ==flat_LOOKUP8_z_triangle)
      printf("flat_LOOKUP8_z_triangle\n");
   else if (triFunc ==smooth_TRUECOLOR_triangle)
      printf("smooth_TRUECOLOR_triangle\n");
   else if (triFunc ==smooth_8A8B8G8R_triangle)
      printf("smooth_8A8B8G8R_triangle\n");
   else if (triFunc ==smooth_8R8G8B_triangle)
      printf("smooth_8R8G8B_triangle\n");
   else if (triFunc ==smooth_8R8G8B24_triangle)
      printf("smooth_8R8G8B24_triangle\n");
   else if (triFunc ==smooth_TRUEDITHER_triangle)
      printf("smooth_TRUEDITHER_triangle\n");
   else if (triFunc ==smooth_5R6G5B_triangle)
      printf("smooth_5R6G5B_triangle\n");
   else if (triFunc ==smooth_DITHER_5R6G5B_triangle)
      printf("smooth_DITHER_5R6G5B_triangle\n");
   else if (triFunc ==smooth_HPCR_triangle)
      printf("smooth_HPCR_triangle\n");
   else if (triFunc ==smooth_DITHER8_triangle)
      printf("smooth_DITHER8_triangle\n");
   else if (triFunc ==smooth_LOOKUP8_triangle)
      printf("smooth_LOOKUP8_triangle\n");
   else if (triFunc ==flat_TRUECOLOR_triangle)
      printf("flat_TRUECOLOR_triangle\n");
   else if (triFunc ==flat_TRUEDITHER_triangle)
      printf("flat_TRUEDITHER_triangle\n");
   else if (triFunc ==flat_8A8B8G8R_triangle)
      printf("flat_8A8B8G8R_triangle\n");
   else if (triFunc ==flat_8R8G8B_triangle)
      printf("flat_8R8G8B_triangle\n");
   else if (triFunc ==flat_8R8G8B24_triangle)
      printf("flat_8R8G8B24_triangle\n");
   else if (triFunc ==flat_5R6G5B_triangle)
      printf("flat_5R6G5B_triangle\n");
   else if (triFunc ==flat_DITHER_5R6G5B_triangle)
      printf("flat_DITHER_5R6G5B_triangle\n");
   else if (triFunc ==flat_HPCR_triangle)
      printf("flat_HPCR_triangle\n");
   else if (triFunc ==flat_DITHER8_triangle)
      printf("flat_DITHER8_triangle\n");
   else if (triFunc ==flat_LOOKUP8_triangle)
      printf("flat_LOOKUP8_triangle\n");
   else
      printf("???\n");
}
#endif


static swrast_tri_func get_triangle_func( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   int depth = GET_VISUAL_DEPTH(xmesa->xm_visual);

   (void) kernel1;

   if (ctx->RenderMode != GL_RENDER)  return (swrast_tri_func) NULL;
   if (ctx->Polygon.SmoothFlag)       return (swrast_tri_func) NULL;
   if (ctx->Texture._ReallyEnabled)   return (swrast_tri_func) NULL;

   if (xmesa->xm_buffer->buffer==XIMAGE) {
      if (   ctx->Light.ShadeModel==GL_SMOOTH
          && swrast->_RasterMask==DEPTH_BIT
          && ctx->Depth.Func==GL_LESS
          && ctx->Depth.Mask==GL_TRUE
          && ctx->Visual.DepthBits == DEFAULT_SOFTWARE_DEPTH_BITS
          && ctx->Polygon.StippleFlag==GL_FALSE) {
         switch (xmesa->pixelformat) {
            case PF_TRUECOLOR:
	       return smooth_TRUECOLOR_z_triangle;
            case PF_8A8B8G8R:
               return smooth_8A8B8G8R_z_triangle;
            case PF_8R8G8B:
               return smooth_8R8G8B_z_triangle;
            case PF_8R8G8B24:
               return smooth_8R8G8B24_z_triangle;
            case PF_TRUEDITHER:
               return smooth_TRUEDITHER_z_triangle;
            case PF_5R6G5B:
               return smooth_5R6G5B_z_triangle;
            case PF_DITHER_5R6G5B:
               return smooth_DITHER_5R6G5B_z_triangle;
            case PF_HPCR:
	       return smooth_HPCR_z_triangle;
            case PF_DITHER:
               return (depth==8) ? smooth_DITHER8_z_triangle
                                        : smooth_DITHER_z_triangle;
            case PF_LOOKUP:
               return (depth==8) ? smooth_LOOKUP8_z_triangle : (swrast_tri_func)NULL;
            default:
               return (swrast_tri_func)NULL;
         }
      }
      if (   ctx->Light.ShadeModel==GL_FLAT
          && swrast->_RasterMask==DEPTH_BIT
          && ctx->Depth.Func==GL_LESS
          && ctx->Depth.Mask==GL_TRUE
          && ctx->Visual.DepthBits == DEFAULT_SOFTWARE_DEPTH_BITS
          && ctx->Polygon.StippleFlag==GL_FALSE) {
         switch (xmesa->pixelformat) {
            case PF_TRUECOLOR:
	       return flat_TRUECOLOR_z_triangle;
            case PF_8A8B8G8R:
               return flat_8A8B8G8R_z_triangle;
            case PF_8R8G8B:
               return flat_8R8G8B_z_triangle;
            case PF_8R8G8B24:
               return flat_8R8G8B24_z_triangle;
            case PF_TRUEDITHER:
               return flat_TRUEDITHER_z_triangle;
            case PF_5R6G5B:
               return flat_5R6G5B_z_triangle;
            case PF_DITHER_5R6G5B:
               return flat_DITHER_5R6G5B_z_triangle;
            case PF_HPCR:
	       return flat_HPCR_z_triangle;
            case PF_DITHER:
               return (depth==8) ? flat_DITHER8_z_triangle
                                        : flat_DITHER_z_triangle;
            case PF_LOOKUP:
               return (depth==8) ? flat_LOOKUP8_z_triangle : (swrast_tri_func)NULL;
            default:
               return (swrast_tri_func)NULL;
         }
      }
      if (   swrast->_RasterMask==0   /* no depth test */
          && ctx->Light.ShadeModel==GL_SMOOTH
          && ctx->Polygon.StippleFlag==GL_FALSE) {
         switch (xmesa->pixelformat) {
            case PF_TRUECOLOR:
	       return smooth_TRUECOLOR_triangle;
            case PF_8A8B8G8R:
               return smooth_8A8B8G8R_triangle;
            case PF_8R8G8B:
               return smooth_8R8G8B_triangle;
            case PF_8R8G8B24:
               return smooth_8R8G8B24_triangle;
            case PF_TRUEDITHER:
               return smooth_TRUEDITHER_triangle;
            case PF_5R6G5B:
               return smooth_5R6G5B_triangle;
            case PF_DITHER_5R6G5B:
               return smooth_DITHER_5R6G5B_triangle;
            case PF_HPCR:
	       return smooth_HPCR_triangle;
            case PF_DITHER:
               return (depth==8) ? smooth_DITHER8_triangle
                                        : smooth_DITHER_triangle;
            case PF_LOOKUP:
               return (depth==8) ? smooth_LOOKUP8_triangle : (swrast_tri_func)NULL;
            default:
               return (swrast_tri_func)NULL;
         }
      }

      if (   swrast->_RasterMask==0   /* no depth test */
          && ctx->Light.ShadeModel==GL_FLAT
          && ctx->Polygon.StippleFlag==GL_FALSE) {
         switch (xmesa->pixelformat) {
            case PF_TRUECOLOR:
	       return flat_TRUECOLOR_triangle;
            case PF_TRUEDITHER:
	       return flat_TRUEDITHER_triangle;
            case PF_8A8B8G8R:
               return flat_8A8B8G8R_triangle;
            case PF_8R8G8B:
               return flat_8R8G8B_triangle;
            case PF_8R8G8B24:
               return flat_8R8G8B24_triangle;
            case PF_5R6G5B:
               return flat_5R6G5B_triangle;
            case PF_DITHER_5R6G5B:
               return flat_DITHER_5R6G5B_triangle;
            case PF_HPCR:
	       return flat_HPCR_triangle;
            case PF_DITHER:
               return (depth==8) ? flat_DITHER8_triangle
                                        : flat_DITHER_triangle;
            case PF_LOOKUP:
               return (depth==8) ? flat_LOOKUP8_triangle : (swrast_tri_func)NULL;
            default:
               return (swrast_tri_func)NULL;
         }
      }

      return (swrast_tri_func)NULL;
   }
   else {
      /* draw to pixmap */
      return (swrast_tri_func) NULL;
   }
}


/* Override for the swrast tri-selection function.  Try to use one
 * of our internal tri functions, otherwise fall back to the
 * standard swrast functions.  
 */
void xmesa_choose_triangle( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (!(swrast->Triangle = get_triangle_func( ctx )))
      _swrast_choose_triangle( ctx );
}

