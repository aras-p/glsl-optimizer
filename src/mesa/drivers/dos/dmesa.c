/*
 * Mesa 3-D graphics library
 * Version:  5.0
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
 * DOS/DJGPP device driver v1.3 for Mesa 5.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef FX

#include "glheader.h"
#include "context.h"
#include "GL/dmesa.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "texformat.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "video.h"

#else

#include "../FX/fxdrv.h"
#include "GL/dmesa.h"

#endif



/*
 * In C++ terms, this class derives from the GLvisual class.
 * Add system-specific fields to it.
 */
struct dmesa_visual {
   GLvisual *gl_visual;
   GLboolean db_flag;           /* double buffered? */
   GLboolean rgb_flag;          /* RGB mode? */
   GLuint depth;                /* bits per pixel (1, 8, 24, etc) */
};

/*
 * In C++ terms, this class derives from the GLframebuffer class.
 * Add system-specific fields to it.
 */
struct dmesa_buffer {
   GLframebuffer gl_buffer;     /* The depth, stencil, accum, etc buffers */
   void *the_window;            /* your window handle, etc */

   int xpos, ypos;              /* position */
   int width, height;           /* size in pixels */
};

/*
 * In C++ terms, this class derives from the GLcontext class.
 * Add system-specific fields to it.
 */
struct dmesa_context {
   GLcontext *gl_ctx;           /* the core library context */
   DMesaVisual visual;
   DMesaBuffer Buffer;
   GLuint ClearColor;
   GLuint ClearIndex;
   /* etc... */
};



#ifndef FX
/****************************************************************************
 * Read/Write pixels
 ***************************************************************************/
#define FLIP(y)  (c->Buffer->height - (y) - 1)
#define FLIP2(y) (b - (y))

/****************************************************************************
 * RGB[A]
 ***************************************************************************/
static void write_rgba_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLubyte rgba[][4], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, vl_mixrgba(rgba[i]));
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, vl_mixrgba(rgba[i]));
    }
 }
}



static void write_rgb_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte rgb[][3], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, vl_mixrgb(rgb[i]));
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, vl_mixrgb(rgb[i]));
    }
 }
}



static void write_mono_rgba_span (const GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLchan color[4], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset, rgba = vl_mixrgba(color);

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, rgba);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, rgba);
    }
 }
}



static void read_rgba_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLubyte rgba[][4])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 /* read all pixels */
 for (i=0; i<n; i++, offset++) {
     vl_getrgba(offset, rgba[i]);
 }
}



static void write_rgba_pixels (const GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLubyte rgba[][4], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1;

 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           vl_putpixel(FLIP2(y[i])*w + x[i], vl_mixrgba(rgba[i]));
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++) {
        vl_putpixel(FLIP2(y[i])*w + x[i], vl_mixrgba(rgba[i]));
    }
 }
}



static void write_mono_rgba_pixels (const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLchan color[4], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1, rgba = vl_mixrgba(color);

 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           vl_putpixel(FLIP2(y[i])*w + x[i], rgba);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++) {
        vl_putpixel(FLIP2(y[i])*w + x[i], rgba);
    }
 }
}



static void read_rgba_pixels (const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLubyte rgba[][4], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1;

 if (mask) {
    /* read some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           vl_getrgba(FLIP2(y[i])*w + x[i], rgba[i]);
        }
    }
 } else {
    /* read all pixels */
    for (i=0; i<n; i++) {
        vl_getrgba(FLIP2(y[i])*w + x[i], rgba[i]);
    }
 }
}

/****************************************************************************
 * Index
 ***************************************************************************/
static void write_index_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                              const GLuint index[], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, index[i]);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, index[i]);
    }
 }
}



static void write_index8_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                               const GLubyte index[], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, index[i]);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, index[i]);
    }
 }
}



static void write_mono_index_span (const GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++, offset++) {
        if (mask[i]) {
           vl_putpixel(offset, colorIndex);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++, offset++) {
        vl_putpixel(offset, colorIndex);
    }
 }
}



static void read_index_span (const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             GLuint index[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, offset;

 offset = c->Buffer->width * FLIP(y) + x;
 /* read all pixels */
 for (i=0; i<n; i++, offset++) {
     index[i] = vl_getpixel(offset);
 }
}



static void write_index_pixels (const GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLuint index[], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1;

 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           vl_putpixel(FLIP2(y[i])*w + x[i], index[i]);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++) {
        vl_putpixel(FLIP2(y[i])*w + x[i], index[i]);
    }
 }
}



static void write_mono_index_pixels (const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1;

 if (mask) {
    /* draw some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           vl_putpixel(FLIP2(y[i])*w + x[i], colorIndex);
        }
    }
 } else {
    /* draw all pixels */
    for (i=0; i<n; i++) {
        vl_putpixel(FLIP2(y[i])*w + x[i], colorIndex);
    }
 }
}



static void read_index_pixels (const GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLuint index[], const GLubyte mask[])
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint i, w = c->Buffer->width, b = c->Buffer->height - 1;

 if (mask) {
    /* read some pixels */
    for (i=0; i<n; i++) {
        if (mask[i]) {
           index[i] = vl_getpixel(FLIP2(y[i])*w + x[i]);
        }
    }
 } else {
    /* read all pixels */
    for (i=0; i<n; i++) {
        index[i] = vl_getpixel(FLIP2(y[i])*w + x[i]);
    }
 }
}



/****************************************************************************
 * Optimized triangle rendering
 ***************************************************************************/

/*
 * flat, NON-depth-buffered, triangle.
 */
static void tri_rgb_flat (GLcontext *ctx,
                          const SWvertex *v0,
                          const SWvertex *v1,
                          const SWvertex *v2)
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint w = c->Buffer->width, b = c->Buffer->height - 1;

#define SETUP_CODE GLuint rgb = vl_mixrgb(v2->color);

#define RENDER_SPAN(span)					\
 GLuint i, offset = FLIP2(span.y)*w + span.x;			\
 for (i = 0; i < span.end; i++, offset++) {			\
     vl_putpixel(offset, rgb);					\
 }

#include "swrast/s_tritemp.h"
}



/*
 * flat, depth-buffered, triangle.
 */
static void tri_rgb_flat_z (GLcontext *ctx,
                            const SWvertex *v0,
                            const SWvertex *v1,
                            const SWvertex *v2)
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint w = c->Buffer->width, b = c->Buffer->height - 1;

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE GLuint rgb = vl_mixrgb(v2->color);

#define RENDER_SPAN(span)					\
 GLuint i, offset = FLIP2(span.y)*w + span.x;			\
 for (i = 0; i < span.end; i++, offset++) {			\
     const DEPTH_TYPE z = FixedToDepth(span.z);			\
     if (z < zRow[i]) {						\
        vl_putpixel(offset, rgb);				\
        zRow[i] = z;						\
     }								\
     span.z += span.zStep;					\
 }

#include "swrast/s_tritemp.h"
}



/*
 * smooth, NON-depth-buffered, triangle.
 */
static void tri_rgb_smooth (GLcontext *ctx,
                            const SWvertex *v0,
                            const SWvertex *v1,
                            const SWvertex *v2)
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint w = c->Buffer->width, b = c->Buffer->height - 1;

#define INTERP_RGB 1
#define RENDER_SPAN(span)						\
 GLuint i, offset = FLIP2(span.y)*w + span.x;				\
 for (i = 0; i < span.end; i++, offset++) {				\
     vl_putpixel(offset, vl_mixfix(span.red, span.green, span.blue));	\
     span.red += span.redStep;						\
     span.green += span.greenStep;					\
     span.blue += span.blueStep;					\
 }

#include "swrast/s_tritemp.h"
}



/*
 * smooth, depth-buffered, triangle.
 */
static void tri_rgb_smooth_z (GLcontext *ctx,
                              const SWvertex *v0,
                              const SWvertex *v1,
                              const SWvertex *v2)
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 GLuint w = c->Buffer->width, b = c->Buffer->height - 1;

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1

#define RENDER_SPAN(span)						\
 GLuint i, offset = FLIP2(span.y)*w + span.x;				\
 for (i = 0; i < span.end; i++, offset++) {				\
     const DEPTH_TYPE z = FixedToDepth(span.z);				\
     if (z < zRow[i]) {							\
        vl_putpixel(offset, vl_mixfix(span.red, span.green, span.blue));\
        zRow[i] = z;							\
     }									\
     span.red += span.redStep;						\
     span.green += span.greenStep;					\
     span.blue += span.blueStep;					\
     span.z += span.zStep;						\
 }

#include "swrast/s_tritemp.h"
}



/*
 * Analyze context state to see if we can provide a fast triangle function
 * Otherwise, return NULL.
 */
static swrast_tri_func dmesa_choose_tri_function (GLcontext *ctx)
{
 const SWcontext *swrast = SWRAST_CONTEXT(ctx);

 if ((ctx->RenderMode != GL_RENDER)
     || (ctx->Polygon.SmoothFlag)
     || (ctx->Polygon.StippleFlag)
     || (ctx->Texture._EnabledUnits)
     || (swrast->_RasterMask & MULTI_DRAW_BIT)
     || ((ctx->Polygon.CullFlag && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK))) {
    return (swrast_tri_func)NULL;
 }

 if (swrast->_RasterMask==DEPTH_BIT
     && ctx->Depth.Func==GL_LESS
     && ctx->Depth.Mask==GL_TRUE
     && ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
    return (ctx->Light.ShadeModel==GL_SMOOTH) ? tri_rgb_smooth_z : tri_rgb_flat_z;
 }

 if (swrast->_RasterMask==0) { /* no depth test */
    return (ctx->Light.ShadeModel==GL_SMOOTH) ? tri_rgb_smooth : tri_rgb_flat;
 }

 return (swrast_tri_func)NULL;
}



/* Override for the swrast triangle-selection function.  Try to use one
 * of our internal triangle functions, otherwise fall back to the
 * standard swrast functions.
 */
static void dmesa_choose_tri (GLcontext *ctx)
{
 SWcontext *swrast = SWRAST_CONTEXT(ctx);

 if (!(swrast->Triangle=dmesa_choose_tri_function(ctx)))
    _swrast_choose_triangle(ctx);
}



/****************************************************************************
 * Miscellaneous device driver funcs
 ***************************************************************************/

static void clear_index (GLcontext *ctx, GLuint index)
{
 DMesaContext c = (DMesaContext)ctx->DriverCtx;

 c->ClearIndex = index;
}

static void clear_color (GLcontext *ctx, const GLfloat color[4])
{
 GLubyte col[4];
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
 CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
 CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
 CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
 CLAMPED_FLOAT_TO_UBYTE(col[3], color[3]);
 c->ClearColor = vl_mixrgba(col);
}



static void clear (GLcontext *ctx, GLbitfield mask, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height)
{
 const DMesaContext c = (DMesaContext)ctx->DriverCtx;
 const GLuint *colorMask = (GLuint *)&ctx->Color.ColorMask;

/*
 * Clear the specified region of the buffers indicated by 'mask'
 * using the clear color or index as specified by one of the two
 * functions above.
 * If all==GL_TRUE, clear whole buffer, else just clear region defined
 * by x,y,width,height
 */

 /* we can't handle color or index masking */
 if ((*colorMask == 0xffffffff) && (ctx->Color.IndexMask == 0xffffffff)) {
    if (mask & DD_BACK_LEFT_BIT) {
       int color = c->visual->rgb_flag ? c->ClearColor : c->ClearIndex;

       if (all) {
          vl_clear(color);
       } else {
          vl_rect(x, y, width, height, color);
       }

       mask &= ~DD_BACK_LEFT_BIT;
    }
 }

 if (mask) {
    _swrast_Clear(ctx, mask, all, x, y, width, height);
 }
}



static void set_buffer (GLcontext *ctx, GLframebuffer *colorBuffer, GLuint bufferBit)
{
 /*
  * XXX todo - examine bufferBit and set read/write pointers
  */
}



/*
 * Return the width and height of the current buffer.
 * If anything special has to been done when the buffer/window is
 * resized, do it now.
 */
static void get_buffer_size (GLframebuffer *buffer, GLuint *width, GLuint *height)
{
 DMesaBuffer b = (DMesaBuffer)buffer;

 *width  = b->width;
 *height = b->height;
}



static const GLubyte* get_string (GLcontext *ctx, GLenum name)
{
 switch (name) {
        case GL_RENDERER:
             return (const GLubyte *)"Mesa DJGPP\0port (c) Borca Daniel dec-2002";
        default:
             return NULL;
 }
}



static void finish (GLcontext *ctx)
{
 /*
  * XXX todo - OPTIONAL FUNCTION: implements glFinish if possible
  */
}



static void flush (GLcontext *ctx)
{
 /*
  * XXX todo - OPTIONAL FUNCTION: implements glFlush if possible
  */
}



/****************************************************************************
 * State
 ***************************************************************************/
#define DMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                            _NEW_TEXTURE | \
                            _NEW_LIGHT | \
                            _NEW_DEPTH | \
                            _NEW_RENDERMODE | \
                            _SWRAST_NEW_RASTERMASK)

/* Extend the software rasterizer with our line and triangle
 * functions.
 */
static void dmesa_register_swrast_functions (GLcontext *ctx)
{
 SWcontext *swrast = SWRAST_CONTEXT(ctx);

 swrast->choose_triangle = dmesa_choose_tri;

 swrast->invalidate_triangle |= DMESA_NEW_TRIANGLE;
}



/* Setup pointers and other driver state that is constant for the life
 * of a context.
 */
static void dmesa_init_pointers (GLcontext *ctx)
{
 TNLcontext *tnl;
 struct swrast_device_driver *dd = _swrast_GetDeviceDriverReference(ctx);

 ctx->Driver.GetString = get_string;
 ctx->Driver.GetBufferSize = get_buffer_size;
 ctx->Driver.Flush = flush;
 ctx->Driver.Finish = finish;
    
 /* Software rasterizer pixel paths:
  */
 ctx->Driver.Accum = _swrast_Accum;
 ctx->Driver.Bitmap = _swrast_Bitmap;
 ctx->Driver.Clear = clear;
 ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
 ctx->Driver.CopyPixels = _swrast_CopyPixels;
 ctx->Driver.DrawPixels = _swrast_DrawPixels;
 ctx->Driver.ReadPixels = _swrast_ReadPixels;
 ctx->Driver.DrawBuffer = _swrast_DrawBuffer;

 /* Software texture functions:
  */
 ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
 ctx->Driver.TexImage1D = _mesa_store_teximage1d;
 ctx->Driver.TexImage2D = _mesa_store_teximage2d;
 ctx->Driver.TexImage3D = _mesa_store_teximage3d;
 ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
 ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
 ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
 ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

 ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
 ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
 ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
 ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
 ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;

 ctx->Driver.CompressedTexImage1D = _mesa_store_compressed_teximage1d;
 ctx->Driver.CompressedTexImage2D = _mesa_store_compressed_teximage2d;
 ctx->Driver.CompressedTexImage3D = _mesa_store_compressed_teximage3d;
 ctx->Driver.CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
 ctx->Driver.CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
 ctx->Driver.CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;

 /* Swrast hooks for imaging extensions:
  */
 ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
 ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
 ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
 ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

 /* Statechange callbacks:
  */
 ctx->Driver.ClearColor = clear_color;
 ctx->Driver.ClearIndex = clear_index;

 /* Initialize the TNL driver interface:
  */
 tnl = TNL_CONTEXT(ctx);
 tnl->Driver.RunPipeline = _tnl_run_pipeline;

 dd->SetBuffer = set_buffer;
   
 /* Install swsetup for tnl->Driver.Render.*:
  */
 _swsetup_Wakeup(ctx);
}



static void dmesa_update_state (GLcontext *ctx, GLuint new_state)
{
 struct swrast_device_driver *dd = _swrast_GetDeviceDriverReference(ctx);

 /* Propogate statechange information to swrast and swrast_setup
  * modules. The DMesa driver has no internal GL-dependent state.
  */
 _swrast_InvalidateState( ctx, new_state );
 _ac_InvalidateState( ctx, new_state );
 _tnl_InvalidateState( ctx, new_state );
 _swsetup_InvalidateState( ctx, new_state );

 /* Index span/pixel functions */
 dd->WriteCI32Span = write_index_span;
 dd->WriteCI8Span = write_index8_span;
 dd->WriteMonoCISpan = write_mono_index_span;
 dd->WriteCI32Pixels = write_index_pixels;
 dd->WriteMonoCIPixels = write_mono_index_pixels;
 dd->ReadCI32Span = read_index_span;
 dd->ReadCI32Pixels = read_index_pixels;

 /* RGB(A) span/pixel functions */
 dd->WriteRGBASpan = write_rgba_span;
 dd->WriteRGBSpan = write_rgb_span;
 dd->WriteMonoRGBASpan = write_mono_rgba_span;
 dd->WriteRGBAPixels = write_rgba_pixels;
 dd->WriteMonoRGBAPixels = write_mono_rgba_pixels;
 dd->ReadRGBASpan = read_rgba_span;
 dd->ReadRGBAPixels = read_rgba_pixels;
}
#endif



/****************************************************************************
 * DMesa Public API Functions
 ***************************************************************************/

/*
 * The exact arguments to this function will depend on your window system
 */
DMesaVisual DMesaCreateVisual (GLint width,
                               GLint height,
                               GLint colDepth,
                               GLint refresh,
                               GLboolean dbFlag,
                               GLboolean rgbFlag,
                               GLboolean alphaFlag,
                               GLint depthSize,
                               GLint stencilSize,
                               GLint accumSize)
{
#ifndef FX
 DMesaVisual v;
 GLint redBits, greenBits, blueBits, alphaBits, indexBits;

 if (!dbFlag) {
    return NULL;
 }

 alphaBits = 0;

 if (!rgbFlag) {
    indexBits = 8;
    redBits = 0;
    greenBits = 0;
    blueBits = 0;
 } else {
    indexBits = 0;
    switch (colDepth) {
           case 8:
                redBits = 8;
                greenBits = 8;
                blueBits = 8;
                break;
           case 15:
                redBits = 5;
                greenBits = 5;
                blueBits = 5;
                break;
           case 16:
                redBits = 5;
                greenBits = 6;
                blueBits = 5;
                break;
           case 32:
                alphaBits = 8;
           case 24:
                redBits = 8;
                greenBits = 8;
                blueBits = 8;
                break;
           default:
                return NULL;
    }
 }

 if ((colDepth=vl_video_init(width, height, colDepth, rgbFlag, refresh)) <= 0) {
    return NULL;
 }

 if (alphaFlag && (alphaBits==0)) {
    alphaBits = 8;
 }

 if ((v=(DMesaVisual)calloc(1, sizeof(struct dmesa_visual))) != NULL) {
    /* Create core visual */
    v->gl_visual = _mesa_create_visual(rgbFlag,			/* rgb */
                                       dbFlag,
                                       GL_FALSE,		/* stereo */
                                       redBits,
                                       greenBits,
                                       blueBits,
                                       alphaBits,
                                       indexBits,		/* indexBits */
                                       depthSize,
                                       stencilSize,
                                       accumSize,		/* accumRed */
                                       accumSize,		/* accumGreen */
                                       accumSize,		/* accumBlue */
                                       alphaFlag?accumSize:0,	/* accumAlpha */
                                       1);			/* numSamples */

    v->depth = colDepth;
    v->db_flag = dbFlag;
    v->rgb_flag = rgbFlag;
 }

 return v;

#else

 int i = 0, fx_attrib[32];

 if (!rgbFlag) {
    return NULL;
 }

 if (dbFlag) fx_attrib[i++] = FXMESA_DOUBLEBUFFER;
 if (depthSize > 0) { fx_attrib[i++] = FXMESA_DEPTH_SIZE; fx_attrib[i++] = depthSize; }
 if (stencilSize > 0) { fx_attrib[i++] = FXMESA_STENCIL_SIZE; fx_attrib[i++] = stencilSize; }
 if (accumSize > 0) { fx_attrib[i++] = FXMESA_ACCUM_SIZE; fx_attrib[i++] = accumSize; }
 if (alphaFlag) { fx_attrib[i++] = FXMESA_ALPHA_SIZE; fx_attrib[i++] = 1; }
 fx_attrib[i] = FXMESA_NONE;

 return (DMesaVisual)fxMesaCreateBestContext(-1, width, height, fx_attrib);
#endif
}



void DMesaDestroyVisual (DMesaVisual v)
{
#ifndef FX
 _mesa_destroy_visual(v->gl_visual);
 free(v);

 vl_video_exit();
#else
 fxMesaDestroyContext((fxMesaContext)v);
#endif
}



DMesaBuffer DMesaCreateBuffer (DMesaVisual visual,
                               GLint xpos, GLint ypos,
                               GLint width, GLint height)
{
#ifndef FX
 DMesaBuffer b;

 if ((b=(DMesaBuffer)calloc(1, sizeof(struct dmesa_buffer))) != NULL) {

    _mesa_initialize_framebuffer(&b->gl_buffer,
                                 visual->gl_visual,
                                 visual->gl_visual->depthBits > 0,
                                 visual->gl_visual->stencilBits > 0,
                                 visual->gl_visual->accumRedBits > 0,
                                 visual->gl_visual->alphaBits > 0);
    b->xpos = xpos;
    b->ypos = ypos;
    b->width = width;
    b->height = height;
 }

 return b;
#else
 return (DMesaBuffer)visual;
#endif
}



void DMesaDestroyBuffer (DMesaBuffer b)
{
#ifndef FX
 free(b->the_window);
 _mesa_free_framebuffer_data(&b->gl_buffer);
 free(b);
#endif
}



DMesaContext DMesaCreateContext (DMesaVisual visual,
                                 DMesaContext share)
{
#ifndef FX
 DMesaContext c;
 GLboolean direct = GL_FALSE;

 if ((c=(DMesaContext)calloc(1, sizeof(struct dmesa_context))) != NULL) {
    c->gl_ctx = _mesa_create_context(visual->gl_visual,
                                     share ? share->gl_ctx : NULL,
                                     (void *)c, direct);

    _mesa_enable_sw_extensions(c->gl_ctx);
    _mesa_enable_1_3_extensions(c->gl_ctx);
    _mesa_enable_1_4_extensions(c->gl_ctx);

    /* you probably have to do a bunch of other initializations here. */
    c->visual = visual;

    c->gl_ctx->Driver.UpdateState = dmesa_update_state;

    /* Initialize the software rasterizer and helper modules.
     */
    _swrast_CreateContext(c->gl_ctx);
    _ac_CreateContext(c->gl_ctx);
    _tnl_CreateContext(c->gl_ctx);
    _swsetup_CreateContext(c->gl_ctx);
    if (visual->rgb_flag) dmesa_register_swrast_functions(c->gl_ctx);
    dmesa_init_pointers(c->gl_ctx);
 }

 return c;

#else

 return (DMesaContext)visual;
#endif
}



void DMesaDestroyContext (DMesaContext c)
{
#ifndef FX
 if (c->gl_ctx) {
    _swsetup_DestroyContext(c->gl_ctx);
    _swrast_DestroyContext(c->gl_ctx);
    _tnl_DestroyContext(c->gl_ctx);
    _ac_DestroyContext(c->gl_ctx);
    _mesa_destroy_context(c->gl_ctx);
 }
 free(c);
#endif
}



GLboolean DMesaMoveBuffer (GLint xpos, GLint ypos)
{
#ifndef FX
 GET_CURRENT_CONTEXT(ctx);
 DMesaBuffer b = ((DMesaContext)ctx->DriverCtx)->Buffer;

 if (vl_sync_buffer(&b->the_window, xpos, ypos, b->width, b->height) != 0) {
    return GL_FALSE;
 } else {
    b->xpos = xpos;
    b->ypos = ypos;
    return GL_TRUE;
 }

#else

 return GL_FALSE;
#endif
}



GLboolean DMesaResizeBuffer (GLint width, GLint height)
{
#ifndef FX
 GET_CURRENT_CONTEXT(ctx);
 DMesaBuffer b = ((DMesaContext)ctx->DriverCtx)->Buffer;

 if (vl_sync_buffer(&b->the_window, b->xpos, b->ypos, width, height) != 0) {
    return GL_FALSE;
 } else {
    b->width = width;
    b->height = height;
    return GL_TRUE;
 }

#else

 return GL_FALSE;
#endif
}



/*
 * Make the specified context and buffer the current one.
 */
GLboolean DMesaMakeCurrent (DMesaContext c, DMesaBuffer b)
{
#ifndef FX
 if ((c != NULL) && (b != NULL)) {
    if (vl_sync_buffer(&b->the_window, b->xpos, b->ypos, b->width, b->height) != 0) {
       return GL_FALSE;
    }

    c->Buffer = b;

    _mesa_make_current(c->gl_ctx, &b->gl_buffer);
    if (c->gl_ctx->Viewport.Width == 0) {
       /* initialize viewport to window size */
       _mesa_Viewport(0, 0, b->width, b->height);
    }
 } else {
    /* Detach */
    _mesa_make_current(NULL, NULL);
 }

#else

 fxMesaMakeCurrent((fxMesaContext)c);
#endif

 return GL_TRUE;
}



void DMesaSwapBuffers (DMesaBuffer b)
{
 /* copy/swap back buffer to front if applicable */
#ifndef FX
 GET_CURRENT_CONTEXT(ctx);
 _mesa_notifySwapBuffers(ctx);
 vl_flip();
#else
 fxMesaSwapBuffers();
#endif
}



void DMesaSetCI (int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
#ifndef FX
 vl_setCI(ndx, red, green, blue);
#endif
}



void DMesaGetIntegerv (GLenum pname, GLint *params)
{
#ifndef FX
 GET_CURRENT_CONTEXT(ctx);
 const DMesaContext c = (ctx == NULL) ? NULL : (DMesaContext)ctx->DriverCtx;
#else
 const fxMesaContext c = fxMesaGetCurrentContext();
#endif

 if (c == NULL) {
    return;
 }

 switch (pname) {
        case DMESA_Y_ORIGIN:
             #ifndef FX
             params[0] = GL_FALSE;
             #else
             params[0] = GL_TRUE;
             #endif
             break;
        case DMESA_SCREEN_SIZE:
             #ifndef FX
             vl_get_screen_size(&params[0], &params[1]);
             #else
             params[0] = c->screen_width;
             params[1] = c->screen_height;
             #endif
             break;
        case DMESA_ARGB_ORDER:
             #ifndef FX
             params[0] = GL_FALSE;
             #else
             params[0] = !c->bgrOrder;
             #endif
             break;
        default:
             break;
 }
}
