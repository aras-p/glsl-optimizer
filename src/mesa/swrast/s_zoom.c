/* $Id: s_zoom.c,v 1.6 2001/05/15 21:30:27 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

#include "glheader.h"
#include "macros.h"

#include "s_context.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"



/*
 * Write a span of pixels to the frame buffer while applying a pixel zoom.
 * This is only used by glDrawPixels and glCopyPixels.
 * Input:  n - number of pixels in input row
 *         x, y - destination of the span
 *         z - depth values for the span
 *         red, green, blue, alpha - array of colors
 *         y0 - location of first row in the image we're drawing.
 */
void
_mesa_write_zoomed_rgba_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y, const GLdepth z[],
                              const GLfloat *fog,
                              CONST GLchan rgba[][4], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLchan zrgba[MAX_WIDTH][4];  /* zoomed pixel colors */
   GLdepth zdepth[MAX_WIDTH];  /* zoomed depth values */
   GLfloat zfog[MAX_WIDTH];  /* zoomed fog values */
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );
   const GLuint *srcRGBA32 = (const GLuint *) rgba;
   GLuint *dstRGBA32 = (GLuint *) zrgba;

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->DrawBuffer->Height && r1>=ctx->DrawBuffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         dstRGBA32[j] = srcRGBA32[i];
         zdepth[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = n - (j+skipcol) - 1;
	    zfog[j] = fog[i];
	 }
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         dstRGBA32[j] = srcRGBA32[i];
         zdepth[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = (GLint) ((j+skipcol) * xscale);
	    if (i<0)  i = n + i - 1;
	    zfog[j] = fog[i];
	 }
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      _mesa_write_rgba_span( ctx, m, x+skipcol, r, zdepth,
			  (fog ? zfog : 0), zrgba, NULL, GL_BITMAP );
   }
}



void
_mesa_write_zoomed_rgb_span( GLcontext *ctx,
                             GLuint n, GLint x, GLint y, const GLdepth z[],
                             const GLfloat *fog,
                             CONST GLchan rgb[][3], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLchan zrgba[MAX_WIDTH][4];  /* zoomed pixel colors */
   GLdepth zdepth[MAX_WIDTH];  /* zoomed depth values */
   GLfloat zfog[MAX_WIDTH];  /* zoomed fog values */
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->DrawBuffer->Height && r1>=ctx->DrawBuffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zrgba[j][0] = rgb[i][0];
         zrgba[j][1] = rgb[i][1];
         zrgba[j][2] = rgb[i][2];
         zrgba[j][3] = CHAN_MAX;
         zdepth[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = n - (j+skipcol) - 1;
	    zfog[j] = fog[i];
	 }
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zrgba[j][0] = rgb[i][0];
         zrgba[j][1] = rgb[i][1];
         zrgba[j][2] = rgb[i][2];
         zrgba[j][3] = CHAN_MAX;
         zdepth[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = (GLint) ((j+skipcol) * xscale);
	    if (i<0)  i = n + i - 1;
	    zfog[j] = fog[i];
	 }
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      _mesa_write_rgba_span( ctx, m, x+skipcol, r, zdepth,
			  (fog ? zfog : 0), zrgba, NULL, GL_BITMAP );
   }
}



/*
 * As above, but write CI pixels.
 */
void
_mesa_write_zoomed_index_span( GLcontext *ctx,
                               GLuint n, GLint x, GLint y, const GLdepth z[],
                               const GLfloat *fog,
                               const GLuint indexes[], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLuint zindexes[MAX_WIDTH];  /* zoomed color indexes */
   GLdepth zdepth[MAX_WIDTH];  /* zoomed depth values */
   GLfloat zfog[MAX_WIDTH];  /* zoomed fog values */
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->DrawBuffer->Height && r1>=ctx->DrawBuffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zindexes[j] = indexes[i];
         zdepth[j]   = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = n - (j+skipcol) - 1;
	    zfog[j] = fog[i];
	 }
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zindexes[j] = indexes[i];
         zdepth[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = (GLint) ((j+skipcol) * xscale);
	    if (i<0)  i = n + i - 1;
	    zfog[j] = fog[i];
	 }
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      _mesa_write_index_span( ctx, m, x+skipcol, r, zdepth,
                              (fog ? zfog : 0), zindexes, NULL, GL_BITMAP );
   }
}



/*
 * As above, but write stencil values.
 */
void
_mesa_write_zoomed_stencil_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y,
                                 const GLstencil stencil[], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLstencil zstencil[MAX_WIDTH];  /* zoomed stencil values */
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->DrawBuffer->Height && r1>=ctx->DrawBuffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zstencil[j] = stencil[i];
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zstencil[j] = stencil[i];
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      _mesa_write_stencil_span( ctx, m, x+skipcol, r, zstencil );
   }
}
