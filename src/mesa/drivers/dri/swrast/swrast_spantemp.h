/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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


/*
 * Modified version of swrast/s_spantemp.h for front-buffer rendering. The
 * no-mask paths use a scratch row to avoid repeated calls to the loader.
 *
 * For the mask paths we always use an array of 4 elements of RB_TYPE. This is
 * to satisfy the xorg loader requirement of an image pitch of 32 bits and
 * should be ok for other loaders also.
 */


#ifndef _SWRAST_SPANTEMP_ONCE
#define _SWRAST_SPANTEMP_ONCE

static INLINE void
PUT_PIXEL( GLcontext *glCtx, GLint x, GLint y, GLvoid *p )
{
    __DRIcontext *ctx = swrast_context(glCtx);
    __DRIdrawable *draw = swrast_drawable(glCtx->DrawBuffer);

    __DRIscreen *screen = ctx->driScreenPriv;

    screen->swrast_loader->putImage(draw, __DRI_SWRAST_IMAGE_OP_DRAW,
				    x, y, 1, 1, (char *)p,
				    draw->loaderPrivate);
}


static INLINE void
GET_PIXEL( GLcontext *glCtx, GLint x, GLint y, GLubyte *p )
{
    __DRIcontext *ctx = swrast_context(glCtx);
    __DRIdrawable *read = swrast_drawable(glCtx->ReadBuffer);

    __DRIscreen *screen = ctx->driScreenPriv;

    screen->swrast_loader->getImage(read, x, y, 1, 1, (char *)p,
				    read->loaderPrivate);
}

static INLINE void
PUT_ROW( GLcontext *glCtx, GLint x, GLint y, GLuint n, char *row )
{
    __DRIcontext *ctx = swrast_context(glCtx);
    __DRIdrawable *draw = swrast_drawable(glCtx->DrawBuffer);

    __DRIscreen *screen = ctx->driScreenPriv;

    screen->swrast_loader->putImage(draw, __DRI_SWRAST_IMAGE_OP_DRAW,
				    x, y, n, 1, row,
				    draw->loaderPrivate);
}

static INLINE void
GET_ROW( GLcontext *glCtx, GLint x, GLint y, GLuint n, char *row )
{
    __DRIcontext *ctx = swrast_context(glCtx);
    __DRIdrawable *read = swrast_drawable(glCtx->ReadBuffer);

    __DRIscreen *screen = ctx->driScreenPriv;

    screen->swrast_loader->getImage(read, x, y, n, 1, row,
				    read->loaderPrivate);
}

#endif /* _SWRAST_SPANTEMP_ONCE */


/*
 * Templates for the span/pixel-array write/read functions called via
 * the gl_renderbuffer's GetRow, GetValues, PutRow, PutMonoRow, PutValues
 * and PutMonoValues functions.
 *
 * Define the following macros before including this file:
 *   NAME(BASE)  to generate the function name (i.e. add prefix or suffix)
 *   RB_TYPE  the renderbuffer DataType
 *   SPAN_VARS  to declare any local variables
 *   INIT_PIXEL_PTR(P, X, Y)  to initialize a pointer to a pixel
 *   INC_PIXEL_PTR(P)  to increment a pixel pointer by one pixel
 *   STORE_PIXEL(DST, X, Y, VALUE)  to store pixel values in buffer
 *   FETCH_PIXEL(DST, SRC)  to fetch pixel values from buffer
 *
 * Note that in the STORE_PIXEL macros, we also pass in the (X,Y) coordinates
 * for the pixels to be stored.  This is useful when dithering and probably
 * ignored otherwise.
 */

#include "main/macros.h"


#if !defined(RB_COMPONENTS)
#define RB_COMPONENTS 4
#endif


static void
NAME(get_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y, void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   RB_TYPE (*dest)[RB_COMPONENTS] = (RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   char *row = swrast_drawable(ctx->ReadBuffer)->row;
   INIT_PIXEL_PTR(pixel, x, y);
   GET_ROW( ctx, x, YFLIP(xrb, y), count, row );
   for (i = 0; i < count; i++) {
      FETCH_PIXEL(dest[i], pixel);
      INC_PIXEL_PTR(pixel);
   }
   (void) rb;
}


static void
NAME(get_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[], void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   RB_TYPE (*dest)[RB_COMPONENTS] = (RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   for (i = 0; i < count; i++) {
      RB_TYPE pixel[4];
      GET_PIXEL(ctx, x[i], YFLIP(xrb, y[i]), pixel);
      FETCH_PIXEL(dest[i], pixel);
   }
   (void) rb;
}


static void
NAME(put_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y,
               const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[RB_COMPONENTS] = (const RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            RB_TYPE row[4];
            INIT_PIXEL_PTR(pixel, x, y);
            STORE_PIXEL(pixel, x + i, y, src[i]);
            PUT_PIXEL(ctx, x + i, YFLIP(xrb, y), pixel);
         }
      }
   }
   else {
      char *row = swrast_drawable(ctx->DrawBuffer)->row;
      INIT_PIXEL_PTR(pixel, x, y);
      for (i = 0; i < count; i++) {
         STORE_PIXEL(pixel, x + i, y, src[i]);
         INC_PIXEL_PTR(pixel);
      }
      PUT_ROW( ctx, x, YFLIP(xrb, y), count, row );
   }
   (void) rb;
}


static void
NAME(put_row_rgb)( GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, GLint x, GLint y,
                   const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[3] = (const RB_TYPE (*)[3]) values;
   GLuint i;
   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            RB_TYPE row[4];
            INIT_PIXEL_PTR(pixel, x, y);
#ifdef STORE_PIXEL_RGB
            STORE_PIXEL_RGB(pixel, x + i, y, src[i]);
#else
            STORE_PIXEL(pixel, x + i, y, src[i]);
#endif
            PUT_PIXEL(ctx, x + i, YFLIP(xrb, y), pixel);
         }
      }
   }
   else {
      char *row = swrast_drawable(ctx->DrawBuffer)->row;
      INIT_PIXEL_PTR(pixel, x, y);
      for (i = 0; i < count; i++) {
#ifdef STORE_PIXEL_RGB
         STORE_PIXEL_RGB(pixel, x + i, y, src[i]);
#else
         STORE_PIXEL(pixel, x + i, y, src[i]);
#endif
         INC_PIXEL_PTR(pixel);
      }
      PUT_ROW( ctx, x, YFLIP(xrb, y), count, row );
   }
   (void) rb;
}


static void
NAME(put_mono_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
                    GLuint count, GLint x, GLint y,
                    const void *value, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE *src = (const RB_TYPE *) value;
   GLuint i;
   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            RB_TYPE row[4];
            INIT_PIXEL_PTR(pixel, x, y);
            STORE_PIXEL(pixel, x + i, y, src);
            PUT_PIXEL(ctx, x + i, YFLIP(xrb, y), pixel);
         }
      }
   }
   else {
      char *row = swrast_drawable(ctx->DrawBuffer)->row;
      INIT_PIXEL_PTR(pixel, x, y);
      for (i = 0; i < count; i++) {
         STORE_PIXEL(pixel, x + i, y, src);
         INC_PIXEL_PTR(pixel);
      }
      PUT_ROW( ctx, x, YFLIP(xrb, y), count, row );
   }
   (void) rb;
}


static void
NAME(put_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[],
                  const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[RB_COMPONENTS] = (const RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         RB_TYPE row[4];
         INIT_PIXEL_PTR(pixel, x, y);
         STORE_PIXEL(pixel, x[i], y[i], src[i]);
         PUT_PIXEL(ctx, x[i], YFLIP(xrb, y[i]), pixel);
      }
   }
   (void) rb;
}


static void
NAME(put_mono_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE *src = (const RB_TYPE *) value;
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         RB_TYPE row[4];
         INIT_PIXEL_PTR(pixel, x, y);
         STORE_PIXEL(pixel, x[i], y[i], src);
         PUT_PIXEL(ctx, x[i], YFLIP(xrb, y[i]), pixel);
      }
   }
   (void) rb;
}


#undef NAME
#undef RB_TYPE
#undef RB_COMPONENTS
#undef SPAN_VARS
#undef INIT_PIXEL_PTR
#undef INC_PIXEL_PTR
#undef STORE_PIXEL
#undef STORE_PIXEL_RGB
#undef FETCH_PIXEL
