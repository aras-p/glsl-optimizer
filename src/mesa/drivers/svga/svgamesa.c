/* $Id: svgamesa.c,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



/*
 * Linux SVGA/Mesa interface.
 *
 * This interface is not finished!  Still have to implement pixel
 * reading functions and double buffering.  Then, look into accelerated
 * line and polygon rendering.  And, clean up a bunch of other stuff.
 * Any volunteers?
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef SVGA


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include "GL/svgamesa.h"
#include "context.h"
#include "matrix.h"
#include "types.h"
#endif


struct svgamesa_context {
   GLcontext *gl_ctx;		/* the core Mesa context */
   GLvisual *gl_vis;		/* describes the color buffer */
   GLframebuffer *gl_buffer;	/* the ancillary buffers */
   GLuint index;		/* current color index */
   GLint red, green, blue;	/* current rgb color */
   GLint width, height;		/* size of color buffer */
   GLint depth;			/* bits per pixel (8,16,24 or 32) */
};


static SVGAMesaContext SVGAMesa = NULL;    /* the current context */



/*
 * Convert Mesa window Y coordinate to VGA screen Y coordinate:
 */
#define FLIP(Y)  (SVGAMesa->height-(Y)-1)



/**********************************************************************/
/*****                 Miscellaneous functions                    *****/
/**********************************************************************/


static void get_buffer_size( GLcontext *ctx, GLuint *width, GLuint *height )
{
   *width = SVGAMesa->width = vga_getxdim();
   *height = SVGAMesa->height = vga_getydim();
}


/* Set current color index */
static void set_index( GLcontext *ctx, GLuint index )
{
   SVGAMesa->index = index;
   vga_setcolor( index );
}


/* Set current drawing color */
static void set_color( GLcontext *ctx,
                       GLubyte red, GLubyte green,
                       GLubyte blue, GLubyte alpha )
{
   SVGAMesa->red = red;
   SVGAMesa->green = green;
   SVGAMesa->blue = blue;
   vga_setrgbcolor( red, green, blue );
}


static void clear_index( GLcontext *ctx, GLuint index )
{
   /* TODO: Implements glClearIndex() */
}


static void clear_color( GLcontext *ctx,
                         GLubyte red, GLubyte green,
                         GLubyte blue, GLubyte alpha )
{
   /* TODO: Implements glClearColor() */
}


static GLbitfield clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
                         GLint x, GLint y, GLint width, GLint height )
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      vga_clear();
   }
   return mask & (~GL_COLOR_BUFFER_BIT);
}


static GLboolean set_buffer( GLcontext *ctx, GLenum buffer )
{
   /* TODO: implement double buffering and use this function to select */
   /* between front and back buffers. */
   if (buffer == GL_FRONT_LEFT)
      return GL_TRUE;
   else if (buffer == GL_BACK_LEFT)
      return GL_TRUE;
   else
      return GL_FALSE;
}




/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/


static void write_ci32_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
   int i;
   y = FLIP(y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         vga_setcolor( index[i] );
         vga_drawpixel( x, y );
      }
   }
}

static void write_ci8_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
   int i;
   y = FLIP(y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         vga_setcolor( index[i] );
         vga_drawpixel( x, y );
      }
   }
}



static void write_mono_ci_span( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, const GLubyte mask[] )
{
   int i;
   y = FLIP(y);
   /* use current color index */
   vga_setcolor( SVGAMesa->index );
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         vga_drawpixel( x, y );
      }
   }
}



static void write_rgba_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   y=FLIP(y);
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
            vga_setrgbcolor( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
            vga_drawpixel( x, y );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
         vga_setrgbcolor( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         vga_drawpixel( x, y );
      }
   }
}



static void write_mono_rgba_span( const GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLubyte mask[])
{
   int i;
   y=FLIP(y);
   /* use current rgb color */
   vga_setrgbcolor( SVGAMesa->red, SVGAMesa->green, SVGAMesa->blue );
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
         vga_drawpixel( x, y );
      }
   }
}



/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


static void read_ci32_span( const GLcontext *ctx,
                            GLuint n, GLint x, GLint y, GLuint index[])
{
   int i;
   y = FLIP(y);
   for (i=0; i<n; i++,x++) {
      index[i] = vga_getpixel( x, y );
   }
}



static void read_rgba_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLubyte rgba[][4] )
{
   int i;
   for (i=0; i<n; i++, x++) {
      /* TODO */
   }
}



/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/


static void write_ci32_pixels( const GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         vga_setcolor( index[i] );
         vga_drawpixel( x[i], FLIP(y[i]) );
      }
   }
}


static void write_mono_ci_pixels( const GLcontext *ctx, GLuint n,
                                  const GLint x[], const GLint y[],
                                  const GLubyte mask[] )
{
   int i;
   /* use current color index */
   vga_setcolor( SVGAMesa->index );
   for (i=0; i<n; i++) {
      if (mask[i]) {
         vga_drawpixel( x[i], FLIP(y[i]) );
      }
   }
}



static void write_rgba_pixels( const GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         vga_setrgbcolor( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         vga_drawpixel( x[i], FLIP(y[i]) );
      }
   }
}



static void write_mono_rgba_pixels( const GLcontext *ctx,
                                    GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLubyte mask[] )
{
   int i;
   /* use current rgb color */
   vga_setrgbcolor( SVGAMesa->red, SVGAMesa->green, SVGAMesa->blue );
   for (i=0; i<n; i++) {
      if (mask[i]) {
         vga_drawpixel( x[i], FLIP(y[i]) );
      }
   }
}




/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

/* Read an array of color index pixels. */
static void read_ci32_pixels( const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
      index[i] = vga_getpixel( x[i], FLIP(y[i]) );
   }
}



static void read_rgba_pixels( const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLubyte rgba[][4], const GLubyte mask[] )
{
   /* TODO */
}



static void svgamesa_update_state( GLcontext *ctx )
{
   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */

   ctx->Driver.UpdateState = svgamesa_update_state;

   ctx->Driver.ClearIndex = clear_index;
   ctx->Driver.ClearColor = clear_color;
   ctx->Driver.Clear = clear;

   ctx->Driver.Index = set_index;
   ctx->Driver.Color = set_color;

   ctx->Driver.SetBuffer = set_buffer;
   ctx->Driver.GetBufferSize = get_buffer_size;

   ctx->Driver.PointsFunc = NULL;
   ctx->Driver.LineFunc = NULL;
   ctx->Driver.TriangleFunc = NULL;

   /* Pixel/span writing functions: */
   /* TODO: use different funcs for 8, 16, 32-bit depths */
   ctx->Driver.WriteRGBASpan        = write_rgba_span;
   ctx->Driver.WriteMonoRGBASpan    = write_mono_rgba_span;
   ctx->Driver.WriteRGBAPixels      = write_rgba_pixels;
   ctx->Driver.WriteMonoRGBAPixels  = write_mono_rgba_pixels;
   ctx->Driver.WriteCI32Span        = write_ci32_span;
   ctx->Driver.WriteCI8Span         = write_ci8_span;
   ctx->Driver.WriteMonoCISpan      = write_mono_ci_span;
   ctx->Driver.WriteCI32Pixels      = write_ci32_pixels;
   ctx->Driver.WriteMonoCIPixels    = write_mono_ci_pixels;

   /* Pixel/span reading functions: */
   /* TODO: use different funcs for 8, 16, 32-bit depths */
   ctx->Driver.ReadCI32Span   = read_ci32_span;
   ctx->Driver.ReadRGBASpan   = read_rgba_span;
   ctx->Driver.ReadCI32Pixels = read_ci32_pixels;
   ctx->Driver.ReadRGBAPixels = read_rgba_pixels;
}



/*
 * Create a new VGA/Mesa context and return a handle to it.
 */
SVGAMesaContext SVGAMesaCreateContext( GLboolean doubleBuffer )
{
   SVGAMesaContext ctx;
   GLboolean rgb_flag;
   GLfloat redscale, greenscale, bluescale, alphascale;
   GLboolean alpha_flag = GL_FALSE;
   int colors;
   GLint index_bits;
   GLint redbits, greenbits, bluebits, alphabits;

   /* determine if we're in RGB or color index mode */
   colors = vga_getcolors();
   if (colors==32768) {
      rgb_flag = GL_TRUE;
      redscale = greenscale = bluescale = alphascale = 255.0;
      redbits = greenbits = bluebits = 8;
      alphabits = 0;
      index_bits = 0;
   }
   else if (colors==256) {
      rgb_flag = GL_FALSE;
      redscale = greenscale = bluescale = alphascale = 0.0;
      redbits = greenbits = bluebits = alphabits = 0;
      index_bits = 8;
   }
   else {
      printf(">16 bit color not implemented yet!\n");
      return NULL;
   }

   ctx = (SVGAMesaContext) calloc( 1, sizeof(struct svgamesa_context) );
   if (!ctx) {
      return NULL;
   }

   ctx->gl_vis = gl_create_visual( rgb_flag,
                                   alpha_flag,
                                   doubleBuffer,
                                   GL_FALSE,  /* stereo */
                                   16,   /* depth_size */
                                   8,    /* stencil_size */
                                   16,   /* accum_size */
                                   index_bits,
                                   redbits, greenbits,
                                   bluebits, alphabits );

   ctx->gl_ctx = gl_create_context( ctx->gl_vis,
                                    NULL,  /* share list context */
                                    (void *) ctx, GL_TRUE );

   ctx->gl_buffer = gl_create_framebuffer( ctx->gl_vis );

   ctx->index = 1;
   ctx->red = ctx->green = ctx->blue = 255;

   ctx->width = ctx->height = 0;  /* temporary until first "make-current" */

   return ctx;
}




/*
 * Destroy the given VGA/Mesa context.
 */
void SVGAMesaDestroyContext( SVGAMesaContext ctx )
{
   if (ctx) {
      gl_destroy_visual( ctx->gl_vis );
      gl_destroy_context( ctx->gl_ctx );
      gl_destroy_framebuffer( ctx->gl_buffer );
      free( ctx );
      if (ctx==SVGAMesa) {
         SVGAMesa = NULL;
      }
   }
}



/*
 * Make the specified VGA/Mesa context the current one.
 */
void SVGAMesaMakeCurrent( SVGAMesaContext ctx )
{
   SVGAMesa = ctx;
   svgamesa_update_state( ctx->gl_ctx );
   gl_make_current( ctx->gl_ctx, ctx->gl_buffer );

   if (ctx->width==0 || ctx->height==0) {
      /* setup initial viewport */
      ctx->width = vga_getxdim();
      ctx->height = vga_getydim();
      gl_Viewport( ctx->gl_ctx, 0, 0, ctx->width, ctx->height );
   }
}



/*
 * Return a handle to the current VGA/Mesa context.
 */
SVGAMesaContext SVGAMesaGetCurrentContext( void )
{
   return SVGAMesa;
}


/*
 * Swap front/back buffers for current context if double buffered.
 */
void SVGAMesaSwapBuffers( void )
{
   FLUSH_VB( SVGAMesa->gl_ctx, "swap buffers" );
   if (SVGAMesa->gl_vis->DBflag) {
      vga_flip();
   }
}


#else

/*
 * Need this to provide at least one external definition when SVGA is
 * not defined on the compiler command line.
 */

int gl_svga_dummy_function(void)
{
   return 0;
}

#endif  /*SVGA*/

