/* $Id: s_zoom.c,v 1.13 2002/02/02 17:24:11 brianp Exp $ */

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


/*
 * Helper function called from _mesa_write_zoomed_rgba/rgb/index_span().
 */
static void
zoom_span( GLcontext *ctx, const struct sw_span *span,
           const GLvoid *src, GLint y0, GLenum format )
{
   GLint r0, r1, row;
   GLint c0, c1, skipCol;
   GLint i, j;
   const GLint maxWidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );
   GLchan rgbaSave[MAX_WIDTH][4];
   GLuint indexSave[MAX_WIDTH];
   struct sw_span zoomed;
   const GLchan (*rgba)[4] = (const GLchan (*)[4]) src;
   const GLchan (*rgb)[3] = (const GLchan (*)[3]) src;
   const GLuint *indexes = (const GLuint *) src;

   /* no pixel arrays! */
   ASSERT((span->arrayMask & SPAN_XY) == 0);

   INIT_SPAN(zoomed);
   if (format == GL_RGBA || format == GL_RGB) {
      zoomed.z = span->z;
      zoomed.zStep = span->z;
      zoomed.fog = span->fog;
      zoomed.fogStep = span->fogStep;
      zoomed.interpMask = span->interpMask & ~SPAN_RGBA;
      zoomed.arrayMask |= SPAN_RGBA;
   }
   else if (format == GL_COLOR_INDEX) {
      zoomed.z = span->z;
      zoomed.zStep = span->z;
      zoomed.fog = span->fog;
      zoomed.fogStep = span->fogStep;
      zoomed.interpMask = span->interpMask & ~SPAN_INDEX;
      zoomed.arrayMask |= SPAN_INDEX;
   }

   /*
    * Compute which columns to draw: [c0, c1)
    */
   c0 = (GLint) span->x;
   c1 = (GLint) (span->x + span->end * ctx->Pixel.ZoomX);
   if (c0 == c1) {
      return;
   }
   else if (c1 < c0) {
      /* swap */
      GLint ctmp = c1;
      c1 = c0;
      c0 = ctmp;
   }
   if (c0 < 0) {
      zoomed.x = 0;
      zoomed.start = 0;
      zoomed.end = c1;
      skipCol = -c0;
   }
   else {
      zoomed.x = c0;
      zoomed.start = 0;
      zoomed.end = c1 - c0;
      skipCol = 0;
   }
   if (zoomed.end > maxWidth)
      zoomed.end = maxWidth;

   /*
    * Compute which rows to draw: [r0, r1)
    */
   row = span->y - y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0 == r1) {
      return;
   }
   else if (r1 < r0) {
      /* swap */
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   ASSERT(r0 < r1);
   ASSERT(c0 < c1);

   /*
    * Trivial clip rejection testing.
    */
   if (r1 < 0) /* below window */
      return;
   if (r0 >= ctx->DrawBuffer->Height) /* above window */
      return;
   if (c1 < 0) /* left of window */
      return;
   if (c0 >= ctx->DrawBuffer->Width) /* right of window */
      return;

   /* zoom the span horizontally */
   if (format == GL_RGBA) {
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            COPY_CHAN4(zoomed.color.rgba[j], rgba[i]);
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (i < 0)
               i = span->end + i - 1;
            COPY_CHAN4(zoomed.color.rgba[j], rgba[i]);
         }
      }
   }
   else if (format == GL_RGB) {
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            zoomed.color.rgba[j][0] = rgb[i][0];
            zoomed.color.rgba[j][1] = rgb[i][1];
            zoomed.color.rgba[j][2] = rgb[i][2];
            zoomed.color.rgba[j][3] = CHAN_MAX;
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (i < 0)
               i = span->end + i - 1;
            zoomed.color.rgba[j][0] = rgb[i][0];
            zoomed.color.rgba[j][1] = rgb[i][1];
            zoomed.color.rgba[j][2] = rgb[i][2];
            zoomed.color.rgba[j][3] = CHAN_MAX;
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            zoomed.color.index[j] = indexes[i];
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = zoomed.start; j < zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (i < 0)
               i = span->end + i - 1;
            zoomed.color.index[j] = indexes[i];
         }
      }
   }

   /* write the span in rows [r0, r1) */
   if (format == GL_RGBA || format == GL_RGB) {
      /* Writing the span may modify the colors, so make a backup now if we're
       * going to call _mesa_write_zoomed_span() more than once.
       */
      if (r1 - r0 > 1) {
         MEMCPY(rgbaSave, zoomed.color.rgba, zoomed.end * 4 * sizeof(GLchan));
      }
      for (zoomed.y = r0; zoomed.y < r1; zoomed.y++) {
         _mesa_write_rgba_span(ctx, &zoomed, GL_BITMAP);
         if (r1 - r0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.color.rgba, rgbaSave, zoomed.end*4 * sizeof(GLchan));
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      if (r1 - r0 > 1) {
         MEMCPY(indexSave, zoomed.color.index, zoomed.end * sizeof(GLuint));
      }
      for (zoomed.y = r0; zoomed.y < r1; zoomed.y++) {
         _mesa_write_index_span(ctx, &zoomed, GL_BITMAP);
         if (r1 - r0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.color.index, indexSave, zoomed.end * sizeof(GLuint));
         }
      }
   }
}


void
_mesa_write_zoomed_rgba_span( GLcontext *ctx, const struct sw_span *span,
                              CONST GLchan rgba[][4], GLint y0 )
{
   zoom_span(ctx, span, (const GLvoid *) rgba, y0, GL_RGBA);
}


void
_mesa_write_zoomed_rgb_span( GLcontext *ctx, const struct sw_span *span,
                             CONST GLchan rgb[][3], GLint y0 )
{
   zoom_span(ctx, span, (const GLvoid *) rgb, y0, GL_RGB);
}


void
_mesa_write_zoomed_index_span( GLcontext *ctx, const struct sw_span *span,
                               GLint y0 )
{
  zoom_span(ctx, span, (const GLvoid *) span->color.index, y0, GL_COLOR_INDEX);
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
