/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * DOS/DJGPP device driver v0.1 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef DMESAINT_H_included
#define DMESAINT_H_included



#define FLIP(y)  (c->Buffer->height - (y) - 1)
#define FLIP2(y) (h - (y) - 1)



/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/
#define WRITE_RGBA_SPAN(bpp) \
static void write_rgba_span_##bpp (const GLcontext *ctx, GLuint n, GLint x, GLint y, \
                                   const GLubyte rgba[][4], const GLubyte mask[])    \
{                                                                                    \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                      \
 void *b = c->Buffer->the_window;                                                    \
 GLuint i, offset;                                                                   \
                                                                                     \
 offset = c->Buffer->width * FLIP(y) + x;                                            \
 if (mask) {                                                                         \
    /* draw some pixels */                                                           \
    for (i=0; i<n; i++, offset++) {                                                  \
        if (mask[i]) {                                                               \
           dv_putpixel##bpp(b, offset, dv_color##bpp(rgba[i]));                      \
        }                                                                            \
    }                                                                                \
 } else {                                                                            \
    /* draw all pixels */                                                            \
    for (i=0; i<n; i++, offset++) {                                                  \
        dv_putpixel##bpp(b, offset, dv_color##bpp(rgba[i]));                         \
    }                                                                                \
 }                                                                                   \
}



#define WRITE_RGB_SPAN(bpp) \
static void write_rgb_span_##bpp (const GLcontext *ctx, GLuint n, GLint x, GLint y, \
                                  const GLubyte rgb[][3], const GLubyte mask[])     \
{                                                                                   \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                     \
 void *b = c->Buffer->the_window;                                                   \
 GLuint i, offset;                                                                  \
                                                                                    \
 offset = c->Buffer->width * FLIP(y) + x;                                           \
 if (mask) {                                                                        \
    /* draw some pixels */                                                          \
    for (i=0; i<n; i++, offset++) {                                                 \
        if (mask[i]) {                                                              \
           dv_putpixel##bpp(b, offset, dv_color##bpp(rgb[i]));                      \
        }                                                                           \
    }                                                                               \
 } else {                                                                           \
    /* draw all pixels */                                                           \
    for (i=0; i<n; i++, offset++) {                                                 \
        dv_putpixel##bpp(b, offset, dv_color##bpp(rgb[i]));                         \
    }                                                                               \
 }                                                                                  \
}



#define WRITE_MONO_RGBA_SPAN(bpp) \
static void write_mono_rgba_span_##bpp (const GLcontext *ctx,                        \
                                        GLuint n, GLint x, GLint y,                  \
                                        const GLchan color[4], const GLubyte mask[]) \
{                                                                                    \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                      \
 void *b = c->Buffer->the_window;                                                    \
 GLuint i, offset, rgba = dv_color##bpp(color);                                      \
                                                                                     \
 offset = c->Buffer->width * FLIP(y) + x;                                            \
 if (mask) {                                                                         \
    /* draw some pixels */                                                           \
    for (i=0; i<n; i++, offset++) {                                                  \
        if (mask[i]) {                                                               \
           dv_putpixel##bpp(b, offset, rgba);                                        \
        }                                                                            \
    }                                                                                \
 } else {                                                                            \
    /* draw all pixels */                                                            \
    for (i=0; i<n; i++, offset++) {                                                  \
        dv_putpixel##bpp(b, offset, rgba);                                           \
    }                                                                                \
 }                                                                                   \
}



/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/
#define READ_RGBA_SPAN(bpp) \
static void read_rgba_span_##bpp (const GLcontext *ctx, GLuint n, GLint x, GLint y, \
                                  GLubyte rgba[][4])                                \
{                                                                                   \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                     \
 void *b = c->Buffer->the_window;                                                   \
 GLuint i, offset;                                                                  \
                                                                                    \
 offset = c->Buffer->width * FLIP(y) + x;                                           \
 /* read all pixels */                                                              \
 for (i=0; i<n; i++, offset++) {                                                    \
     dv_getrgba##bpp(b, offset, rgba[i]);                                           \
 }                                                                                  \
}



/**********************************************************************/
/*****              Write arrays of pixels                        *****/
/**********************************************************************/
#define WRITE_RGBA_PIXELS(bpp) \
static void write_rgba_pixels_##bpp (const GLcontext *ctx,                          \
                                     GLuint n, const GLint x[], const GLint y[],    \
                                     const GLubyte rgba[][4], const GLubyte mask[]) \
{                                                                                   \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                     \
 void *b = c->Buffer->the_window;                                                   \
 GLuint i, w = c->Buffer->width, h = c->Buffer->height;                             \
                                                                                    \
 if (mask) {                                                                        \
    /* draw some pixels */                                                          \
    for (i=0; i<n; i++) {                                                           \
        if (mask[i]) {                                                              \
           dv_putpixel##bpp(b, FLIP2(y[i])*w + x[i], dv_color##bpp(rgba[i]));       \
        }                                                                           \
    }                                                                               \
 } else {                                                                           \
    /* draw all pixels */                                                           \
    for (i=0; i<n; i++) {                                                           \
        dv_putpixel##bpp(b, FLIP2(y[i])*w + x[i], dv_color##bpp(rgba[i]));          \
    }                                                                               \
 }                                                                                  \
}



#define WRITE_MONO_RGBA_PIXELS(bpp) \
static void write_mono_rgba_pixels_##bpp (const GLcontext *ctx,                        \
                                          GLuint n, const GLint x[], const GLint y[],  \
                                          const GLchan color[4], const GLubyte mask[]) \
{                                                                                      \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                        \
 void *b = c->Buffer->the_window;                                                      \
 GLuint i, w = c->Buffer->width, h = c->Buffer->height, rgba = dv_color##bpp(color);   \
                                                                                       \
 if (mask) {                                                                           \
    /* draw some pixels */                                                             \
    for (i=0; i<n; i++) {                                                              \
        if (mask[i]) {                                                                 \
           dv_putpixel##bpp(b, FLIP2(y[i])*w + x[i], rgba);                            \
        }                                                                              \
    }                                                                                  \
 } else {                                                                              \
    /* draw all pixels */                                                              \
    for (i=0; i<n; i++) {                                                              \
        dv_putpixel##bpp(b, FLIP2(y[i])*w + x[i], rgba);                               \
    }                                                                                  \
 }                                                                                     \
}




/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/
#define READ_RGBA_PIXELS(bpp) \
static void read_rgba_pixels_##bpp (const GLcontext *ctx,                       \
                                    GLuint n, const GLint x[], const GLint y[], \
                                    GLubyte rgba[][4], const GLubyte mask[])    \
{                                                                               \
 DMesaContext c = (DMesaContext)ctx->DriverCtx;                                 \
 void *b = c->Buffer->the_window;                                               \
 GLuint i, w = c->Buffer->width, h = c->Buffer->height;                         \
                                                                                \
 if (mask) {                                                                    \
    /* read some pixels */                                                      \
    for (i=0; i<n; i++) {                                                       \
        if (mask[i]) {                                                          \
           dv_getrgba##bpp(b, FLIP2(y[i])*w + x[i], rgba[i]);                   \
        }                                                                       \
    }                                                                           \
 } else {                                                                       \
    /* read all pixels */                                                       \
    for (i=0; i<n; i++) {                                                       \
        dv_getrgba##bpp(b, FLIP2(y[i])*w + x[i], rgba[i]);                      \
    }                                                                           \
 }                                                                              \
}



#endif
