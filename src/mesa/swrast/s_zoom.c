/* $Id: s_zoom.c,v 1.10 2002/01/28 00:07:33 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
#include "mem.h"
#include "colormac.h"

#include "s_context.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"


#ifdef DEBUG

#define SAVE_SPAN(span)    struct sw_span tmp_span = (span);

#define RESTORE_SPAN(span)						\
{									\
   GLint i;								\
   for (i=tmp_span.start; i<tmp_span.end; i++) {			\
      if (tmp_span.color.rgba[i][RCOMP] !=				\
          (span).color.rgba[i][RCOMP] ||	 			\
          tmp_span.color.rgba[i][GCOMP] != 				\
	  (span).color.rgba[i][GCOMP] || 				\
	  tmp_span.color.rgba[i][BCOMP] != 				\
	  (span).color.rgba[i][BCOMP]) {				\
         fprintf(stderr, "glZoom: Color-span changed in subfunction.");	\
      }									\
      if (tmp_span.zArray[i] != (span).zArray[i]) {			\
         fprintf(stderr, "glZoom: Depth-span changed in subfunction.");	\
      }									\
   }									\
   (span) = tmp_span;							\
}

#else /* DEBUG not defined */

#define SAVE_SPAN(span)    GLint start = (span).start, end = (span).end;
#define RESTORE_SPAN(span) (span).start = start, (span).end = end;      \
			   (span).writeAll = GL_TRUE;

#endif /* DEBUG */

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
   GLint r0, r1, row;
   GLint i, j;
   struct sw_span zoomed;
   const GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );

   SW_SPAN_RESET(zoomed);
   INIT_SPAN(zoomed);

   /* compute width of output row */
   zoomed.end = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (zoomed.end == 0) {
      return;
   }
   /*here ok or better latter? like it was before */
   else if (zoomed.end > maxwidth) {
     zoomed.end = maxwidth;
   }

   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      zoomed.x = x - zoomed.end;
   }
   else
     zoomed.x = x;


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
   if (zoomed.x < 0) {
      zoomed.start = -x;
   }

   /* make sure span isn't too long or short */
   /*   if (m>maxwidth) {
      m = maxwidth;
      }*/

   if (zoomed.end <= zoomed.start) {
      return;
   }

   ASSERT( zoomed.end <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      SW_SPAN_SET_FLAG(zoomed.filledColor);
      SW_SPAN_SET_FLAG(zoomed.filledAlpha);
      /* n==m */
      for (j=zoomed.start; j<zoomed.end; j++) {
         i = n - j - 1;
	 COPY_CHAN4(zoomed.color.rgba[j], rgba[i]);
         zoomed.zArray[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=zoomed.start; j<zoomed.end; j++) {
	    i = n - j - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
      }
   }
   else {
      const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      SW_SPAN_SET_FLAG(zoomed.filledColor);
      SW_SPAN_SET_FLAG(zoomed.filledAlpha);
      for (j=zoomed.start; j<zoomed.end; j++) {
         i = (GLint) (j * xscale);
         if (i<0)  i = n + i - 1;
	 COPY_CHAN4(zoomed.color.rgba[j], rgba[i]);
         zoomed.zArray[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=zoomed.start; j<zoomed.end; j++) {
	    i = (GLint) (j * xscale);
	    if (i<0)  i = n + i - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
      }
   }

   zoomed.arrayMask |= SPAN_RGBA;
   zoomed.arrayMask |= SPAN_Z;
   if (fog)
      zoomed.arrayMask |= SPAN_FOG;

   /* write the span */
   for (zoomed.y = r0; zoomed.y < r1; zoomed.y++) {
      SAVE_SPAN(zoomed);
      ASSERT((zoomed.interpMask & SPAN_RGBA) == 0);
      _mesa_write_rgba_span(ctx, &zoomed, GL_BITMAP);
      RESTORE_SPAN(zoomed);
      /* problem here: "zoomed" can change inside
	 "_mesa_write_rgba_span". Best solution: make copy "tmpspan"
	 and give to function, but too slow */
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
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );
   struct sw_span zoomed;

   INIT_SPAN(zoomed);
   zoomed.arrayMask |= SPAN_RGBA;

   if (fog && ctx->Fog.Enabled)
      zoomed.arrayMask |= SPAN_FOG;


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

   ASSERT( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zoomed.color.rgba[j][0] = rgb[i][0];
         zoomed.color.rgba[j][1] = rgb[i][1];
         zoomed.color.rgba[j][2] = rgb[i][2];
         zoomed.color.rgba[j][3] = CHAN_MAX;
         zoomed.zArray[j] = z[i];
      }
      if (zoomed.arrayMask & SPAN_FOG) {
	 for (j=0;j<m;j++) {
	    i = n - (j+skipcol) - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zoomed.color.rgba[j][0] = rgb[i][0];
         zoomed.color.rgba[j][1] = rgb[i][1];
         zoomed.color.rgba[j][2] = rgb[i][2];
         zoomed.color.rgba[j][3] = CHAN_MAX;
         zoomed.zArray[j] = z[i];
      }
      if (zoomed.arrayMask & SPAN_FOG) {
	 for (j=0;j<m;j++) {
	    i = (GLint) ((j+skipcol) * xscale);
	    if (i<0)  i = n + i - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      zoomed.x = x + skipcol;
      zoomed.y = r;
      zoomed.end = m;
      ASSERT((zoomed.interpMask & SPAN_RGBA) == 0);
      _mesa_write_rgba_span( ctx, &zoomed, GL_BITMAP );
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
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );
   struct sw_span zoomed;

   SW_SPAN_RESET(zoomed);
   INIT_SPAN(zoomed);
   zoomed.arrayMask |= SPAN_INDEX;

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

   ASSERT( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zoomed.color.index[j] = indexes[i];
         zoomed.zArray[j]   = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = n - (j+skipcol) - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
         zoomed.arrayMask |= SPAN_FOG;
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zoomed.color.index[j] = indexes[i];
         zoomed.zArray[j] = z[i];
      }
      if (fog && ctx->Fog.Enabled) {
	 for (j=0;j<m;j++) {
	    i = (GLint) ((j+skipcol) * xscale);
	    if (i<0)  i = n + i - 1;
	    zoomed.fogArray[j] = fog[i];
	 }
         zoomed.arrayMask |= SPAN_FOG;
      }
   }

   zoomed.arrayMask |= SPAN_Z;

   /* write the span */
   for (r=r0; r<r1; r++) {
      SAVE_SPAN(zoomed);
      ASSERT((zoomed.interpMask & SPAN_INDEX) == 0);
      zoomed.x = x + skipcol;
      zoomed.y = r;
      zoomed.end = m;
      _mesa_write_index_span(ctx, &zoomed, GL_BITMAP);
      RESTORE_SPAN(zoomed);
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

   ASSERT( m <= MAX_WIDTH );

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
