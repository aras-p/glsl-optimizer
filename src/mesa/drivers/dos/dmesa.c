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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "GL/dmesa.h"
#include "matrix.h"
#include "texformat.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#endif

#include "dvesa.h"
#include "dmesaint.h"



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
   GLframebuffer *gl_buffer;    /* The depth, stencil, accum, etc buffers */
   void *the_window;            /* your window handle, etc */

   int width, height;           /* size in pixels */
   int xpos, ypos;              /* buffer position */
   int xsize, len;              /* number of bytes in a line, then total */
   int delta;                   /* used to wrap around */
   int offset;                  /* offset in video */
   struct dvmode *video;
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
   /* etc... */
};



static void dmesa_update_state (GLcontext *ctx, GLuint new_state);



/**********************************************************************/
/*****            Read/Write pixels                               *****/
/**********************************************************************/



WRITE_RGBA_SPAN(15)
WRITE_RGBA_SPAN(16)
WRITE_RGBA_SPAN(24)
WRITE_RGBA_SPAN(32)

WRITE_RGB_SPAN(15)
WRITE_RGB_SPAN(16)
WRITE_RGB_SPAN(24)
WRITE_RGB_SPAN(32)

WRITE_MONO_RGBA_SPAN(15)
WRITE_MONO_RGBA_SPAN(16)
WRITE_MONO_RGBA_SPAN(24)
WRITE_MONO_RGBA_SPAN(32)

READ_RGBA_SPAN(15)
READ_RGBA_SPAN(16)
READ_RGBA_SPAN(24)
READ_RGBA_SPAN(32)

WRITE_RGBA_PIXELS(15)
WRITE_RGBA_PIXELS(16)
WRITE_RGBA_PIXELS(24)
WRITE_RGBA_PIXELS(32)

WRITE_MONO_RGBA_PIXELS(15)
WRITE_MONO_RGBA_PIXELS(16)
WRITE_MONO_RGBA_PIXELS(24)
WRITE_MONO_RGBA_PIXELS(32)

READ_RGBA_PIXELS(15)
READ_RGBA_PIXELS(16)
READ_RGBA_PIXELS(24)
READ_RGBA_PIXELS(32)



/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/**********************************************************************/



static void clear_color (GLcontext *ctx, const GLchan color[4])
{
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
 c->ClearColor = dv_color(color);
}



static void clear (GLcontext *ctx, GLbitfield mask, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height)
{
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
 const GLuint *colorMask = (GLuint *)&ctx->Color.ColorMask;
 DMesaBuffer b = c->Buffer;

/*
 * Clear the specified region of the buffers indicated by 'mask'
 * using the clear color or index as specified by one of the two
 * functions above.
 * If all==GL_TRUE, clear whole buffer, else just clear region defined
 * by x,y,width,height
 */

 /* we can't handle color or index masking */
 if (*colorMask==0xffffffff && ctx->Color.IndexMask==0xffffffff) {
    if (mask&DD_BACK_LEFT_BIT) {
       if (all) {
          dv_clear_virtual(b->the_window, b->len, c->ClearColor);
       } else {
          dv_fillrect(b->the_window, b->width, x, y, width, height, c->ClearColor);
       }
       mask &= ~DD_BACK_LEFT_BIT;
    }
 }

 if (mask) {
    _swrast_Clear(ctx, mask, all, x, y, width, height);
 }
}



/*
 * Set the current reading buffer.
 */
static void set_read_buffer (GLcontext *ctx, GLframebuffer *buffer,
                             GLenum mode)
{
/*
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
 dmesa_update_state(ctx);
*/
}



/*
 * Set the destination/draw buffer.
 */
static GLboolean set_draw_buffer (GLcontext *ctx, GLenum mode)
{
 if (mode==GL_BACK_LEFT) {
    return GL_TRUE;
 } else {
    return GL_FALSE;
 }
}



/*
 * Return the width and height of the current buffer.
 * If anything special has to been done when the buffer/window is
 * resized, do it now.
 */
static void get_buffer_size (GLcontext *ctx, GLuint *width, GLuint *height)
{
 DMesaContext c = (DMesaContext)ctx->DriverCtx;

 *width  = c->Buffer->width;
 *height = c->Buffer->height;
}



static const GLubyte* get_string (GLcontext *ctx, GLenum name)
{
 switch (name) {
        case GL_RENDERER:
             return (const GLubyte *)"DOS Mesa";
        default:
             return NULL;
 }
}



/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/*****           Note that these functions are mandatory          *****/
/**********************************************************************/



/* OPTIONAL FUNCTION: implements glFinish if possible */
static void finish (GLcontext *ctx)
{
/*
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
*/
}



/* OPTIONAL FUNCTION: implements glFlush if possible */
static void flush (GLcontext *ctx)
{
/*
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
*/
}



/**********************************************************************/
/**********************************************************************/



/* Setup pointers and other driver state that is constant for the life
 * of a context.
 */
void dmesa_init_pointers (GLcontext *ctx)
{
 TNLcontext *tnl;

 ctx->Driver.UpdateState = dmesa_update_state;

 ctx->Driver.GetString = get_string;
 ctx->Driver.GetBufferSize = get_buffer_size;
 ctx->Driver.Flush = flush;
 ctx->Driver.Finish = finish;
    
 /* Software rasterizer pixel paths:
  */
 ctx->Driver.Accum = _swrast_Accum;
 ctx->Driver.Bitmap = _swrast_Bitmap;
 ctx->Driver.Clear = clear;
 ctx->Driver.ResizeBuffersMESA = _swrast_alloc_buffers;
 ctx->Driver.CopyPixels = _swrast_CopyPixels;
 ctx->Driver.DrawPixels = _swrast_DrawPixels;
 ctx->Driver.ReadPixels = _swrast_ReadPixels;

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

 ctx->Driver.BaseCompressedTexFormat = _mesa_base_compressed_texformat;
 ctx->Driver.CompressedTextureSize = _mesa_compressed_texture_size;
 ctx->Driver.GetCompressedTexImage = _mesa_get_compressed_teximage;

 /* Swrast hooks for imaging extensions:
  */
 ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
 ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
 ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
 ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

 /* Statechange callbacks:
  */
 ctx->Driver.SetDrawBuffer = set_draw_buffer;
 ctx->Driver.ClearColor = clear_color;

 /* Initialize the TNL driver interface:
  */
 tnl = TNL_CONTEXT(ctx);
 tnl->Driver.RunPipeline = _tnl_run_pipeline;
   
 /* Install swsetup for tnl->Driver.Render.*:
  */
 _swsetup_Wakeup(ctx);
}



static void dmesa_update_state (GLcontext *ctx, GLuint new_state)
{
 DMesaContext c = (DMesaContext)ctx->DriverCtx;
 struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

 /* Initialize all the pointers in the DD struct.  Do this whenever */
 /* a new context is made current or we change buffers via set_buffer! */

 _swrast_InvalidateState(ctx, new_state);
 _swsetup_InvalidateState(ctx, new_state);
 _ac_InvalidateState(ctx, new_state);
 _tnl_InvalidateState(ctx, new_state);

 swdd->SetReadBuffer = set_read_buffer;

 /* RGB(A) span/pixel functions */
 switch (c->visual->depth) {
        case 15:
             swdd->WriteRGBASpan = write_rgba_span_15;
             swdd->WriteRGBSpan = write_rgb_span_15;
             swdd->WriteMonoRGBASpan = write_mono_rgba_span_15;
             swdd->WriteRGBAPixels = write_rgba_pixels_15;
             swdd->WriteMonoRGBAPixels = write_mono_rgba_pixels_15;
             swdd->ReadRGBASpan = read_rgba_span_15;
             swdd->ReadRGBAPixels = read_rgba_pixels_15;
             break;
        case 16:
             swdd->WriteRGBASpan = write_rgba_span_16;
             swdd->WriteRGBSpan = write_rgb_span_16;
             swdd->WriteMonoRGBASpan = write_mono_rgba_span_16;
             swdd->WriteRGBAPixels = write_rgba_pixels_16;
             swdd->WriteMonoRGBAPixels = write_mono_rgba_pixels_16;
             swdd->ReadRGBASpan = read_rgba_span_16;
             swdd->ReadRGBAPixels = read_rgba_pixels_16;
             break;
        case 24:
             swdd->WriteRGBASpan = write_rgba_span_24;
             swdd->WriteRGBSpan = write_rgb_span_24;
             swdd->WriteMonoRGBASpan = write_mono_rgba_span_24;
             swdd->WriteRGBAPixels = write_rgba_pixels_24;
             swdd->WriteMonoRGBAPixels = write_mono_rgba_pixels_24;
             swdd->ReadRGBASpan = read_rgba_span_24;
             swdd->ReadRGBAPixels = read_rgba_pixels_24;
             break;
        case 32:
             swdd->WriteRGBASpan = write_rgba_span_32;
             swdd->WriteRGBSpan = write_rgb_span_32;
             swdd->WriteMonoRGBASpan = write_mono_rgba_span_32;
             swdd->WriteRGBAPixels = write_rgba_pixels_32;
             swdd->WriteMonoRGBAPixels = write_mono_rgba_pixels_32;
             swdd->ReadRGBASpan = read_rgba_span_32;
             swdd->ReadRGBAPixels = read_rgba_pixels_32;
             break;
 }
}



/**********************************************************************/
/*****               DMesa Public API Functions                   *****/
/**********************************************************************/



/*
 * The exact arguments to this function will depend on your window system
 */
DMesaVisual DMesaCreateVisual (GLint colDepth, GLboolean dbFlag,
                               GLint depthSize, GLint stencilSize,
                               GLint accumSize)
{
 DMesaVisual v;
 GLint redBits, greenBits, blueBits, alphaBits;

 if (!dbFlag) {
    return NULL;
 }
 switch (colDepth) {
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
        case 24:
        case 32:
             redBits = 8;
             greenBits = 8;
             blueBits = 8;
             break;
        default:
             return NULL;
 }
 alphaBits = 8;

 if ((v=(DMesaVisual)calloc(1, sizeof(struct dmesa_visual)))!=NULL) {
    /* Create core visual */
    v->gl_visual = _mesa_create_visual(colDepth>8,		/* rgb */
                                       dbFlag,
                                       GL_FALSE,		/* stereo */
                                       redBits,
                                       greenBits,
                                       blueBits,
                                       alphaBits,
                                       0,			/* indexBits */
                                       depthSize,
                                       stencilSize,
                                       accumSize,		/* accumRed */
                                       accumSize,		/* accumGreen */
                                       accumSize,		/* accumBlue */
                                       alphaBits?accumSize:0,	/* accumAlpha */
                                       1);			/* numSamples */

    v->depth = colDepth;
    v->db_flag = dbFlag;
 }

 return v;
}



void DMesaDestroyVisual (DMesaVisual v)
{
 _mesa_destroy_visual(v->gl_visual);
 free(v);
}



DMesaBuffer DMesaCreateBuffer (DMesaVisual visual,
                               GLint width, GLint height,
                               GLint xpos, GLint ypos)
{
 DMesaBuffer b;

 if ((b=(DMesaBuffer)calloc(1, sizeof(struct dmesa_buffer)))!=NULL) {
    if (visual->db_flag) {
       if ((b->the_window=calloc(1, width*height*((visual->depth+7)/8)))==NULL) {
          return NULL;
       }
    }

    b->gl_buffer = _mesa_create_framebuffer(visual->gl_visual,
                                            visual->gl_visual->depthBits > 0,
                                            visual->gl_visual->stencilBits > 0,
                                            visual->gl_visual->accumRedBits > 0,
                                            visual->gl_visual->alphaBits > 0);
    b->width = width;
    b->height = height;
    b->xpos = xpos;
    b->ypos = ypos;
 }

 return b;
}



void DMesaDestroyBuffer (DMesaBuffer b)
{
 free(b->the_window);
 _mesa_destroy_framebuffer(b->gl_buffer);
 free(b);
}



DMesaContext DMesaCreateContext (DMesaVisual visual,
                                 DMesaContext share)
{
 DMesaContext c;
 GLboolean direct = GL_FALSE;

 if ((c=(DMesaContext)calloc(1, sizeof(struct dmesa_context)))!=NULL) {
    c->gl_ctx = _mesa_create_context(visual->gl_visual,
                                     share ? share->gl_ctx : NULL,
                                     (void *)c, direct);

   /* you probably have to do a bunch of other initializations here. */
    c->visual = visual;

   /* Initialize the software rasterizer and helper modules.
    */
    _swrast_CreateContext(c->gl_ctx);
    _ac_CreateContext(c->gl_ctx);
    _tnl_CreateContext(c->gl_ctx);
    _swsetup_CreateContext(c->gl_ctx);
    dmesa_init_pointers(c->gl_ctx);
 }

 return c;
}



void DMesaDestroyContext (DMesaContext c)
{
 _mesa_destroy_context(c->gl_ctx);
 free(c);
}



/*
 * Make the specified context and buffer the current one.
 */
GLboolean DMesaMakeCurrent (DMesaContext c, DMesaBuffer b)
{
 if (c&&b) {
    c->Buffer = b;
    if ((b->video=dv_select_mode(b->xpos, b->ypos, b->width, b->height, c->visual->depth, &b->delta, &b->offset))==NULL) {
       return GL_FALSE;
    }

    b->xsize = b->width*((c->visual->depth+7)/8);
    b->len = b->xsize*b->height;

    dmesa_update_state(c->gl_ctx, 0);
    _mesa_make_current(c->gl_ctx, b->gl_buffer);
    if (c->gl_ctx->Viewport.Width==0) {
       /* initialize viewport to window size */
       _mesa_Viewport(0, 0, c->Buffer->width, c->Buffer->height);
    }
 } else {
    /* Detach */
    _mesa_make_current(NULL, NULL);
 }

 return GL_TRUE;
}



void DMesaSwapBuffers (DMesaBuffer b)
{
 /* copy/swap back buffer to front if applicable */
 if (b->the_window) {
    dv_dump_virtual(b->the_window, b->xsize, b->height, b->offset, b->delta);
 }
}
