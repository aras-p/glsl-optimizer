/* $Id: osmesa.c,v 1.52 2001/03/29 17:15:21 brianp Exp $ */

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


/*
 * Off-Screen Mesa rendering / Rendering into client memory space
 *
 * Note on thread safety:  this driver is thread safe.  All
 * functions are reentrant.  The notion of current context is
 * managed by the core _mesa_make_current() and _mesa_get_current_context()
 * functions.  Those functions are thread-safe.
 */


#include "glheader.h"
#include "GL/osmesa.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"



/*
 * This is the OS/Mesa context struct.
 * Notice how it includes a GLcontext.  By doing this we're mimicking
 * C++ inheritance/derivation.
 * Later, we can cast a GLcontext pointer into an OSMesaContext pointer
 * or vice versa.
 */
struct osmesa_context {
   GLcontext gl_ctx;		/* The core GL/Mesa context */
   GLvisual *gl_visual;		/* Describes the buffers */
   GLframebuffer *gl_buffer;	/* Depth, stencil, accum, etc buffers */
   GLenum format;		/* either GL_RGBA or GL_COLOR_INDEX */
   void *buffer;		/* the image buffer */
   GLint width, height;		/* size of image buffer */
   GLint rowlength;		/* number of pixels per row */
   GLint userRowLength;		/* user-specified number of pixels per row */
   GLint rshift, gshift;	/* bit shifts for RGBA formats */
   GLint bshift, ashift;
   GLint rInd, gInd, bInd, aInd;/* index offsets for RGBA formats */
   GLchan *rowaddr[MAX_HEIGHT];	/* address of first pixel in each image row */
   GLboolean yup;		/* TRUE  -> Y increases upward */
				/* FALSE -> Y increases downward */
};



/* A forward declaration: */
static void osmesa_update_state( GLcontext *ctx, GLuint newstate );
static void osmesa_register_swrast_functions( GLcontext *ctx );



#define OSMESA_CONTEXT(ctx)  ((OSMesaContext) (ctx->DriverCtx))



/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/*
 * Create an Off-Screen Mesa rendering context.  The only attribute needed is
 * an RGBA vs Color-Index mode flag.
 *
 * Input:  format - either GL_RGBA or GL_COLOR_INDEX
 *         sharelist - specifies another OSMesaContext with which to share
 *                     display lists.  NULL indicates no sharing.
 * Return:  an OSMesaContext or 0 if error
 */
OSMesaContext GLAPIENTRY
OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
   return OSMesaCreateContextExt(format, DEFAULT_SOFTWARE_DEPTH_BITS,
                                 8, 16, sharelist);
}



/*
 * New in Mesa 3.5
 *
 * Create context and specify size of ancillary buffers.
 */
OSMesaContext GLAPIENTRY
OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits,
                        GLint accumBits, OSMesaContext sharelist )
{
   OSMesaContext osmesa;
   GLint rshift, gshift, bshift, ashift;
   GLint rind, gind, bind, aind;
   GLint indexBits = 0, redBits = 0, greenBits = 0, blueBits = 0, alphaBits =0;
   GLboolean rgbmode;
   GLboolean swalpha;
   const GLuint i4 = 1;
   const GLubyte *i1 = (GLubyte *) &i4;
   const GLint little_endian = *i1;

   swalpha = GL_FALSE;
   rind = gind = bind = aind = 0;
   if (format==OSMESA_COLOR_INDEX) {
      indexBits = 8;
      rshift = gshift = bshift = ashift = 0;
      rgbmode = GL_FALSE;
   }
   else if (format==OSMESA_RGBA) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      rind = 0;
      gind = 1;
      bind = 2;
      aind = 3;
      if (little_endian) {
         rshift = 0;
         gshift = 8;
         bshift = 16;
         ashift = 24;
      }
      else {
         rshift = 24;
         gshift = 16;
         bshift = 8;
         ashift = 0;
      }
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_BGRA) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      rind = 2;
      gind = 1;
      bind = 0;
      aind = 3;
      if (little_endian) {
         ashift = 0;
         rshift = 8;
         gshift = 16;
         bshift = 24;
      }
      else {
         bshift = 24;
         gshift = 16;
         rshift = 8;
         ashift = 0;
      }
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_ARGB) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      rind = 1;
      gind = 2;
      bind = 3;
      aind = 0;
      if (little_endian) {
         bshift = 0;
         gshift = 8;
         rshift = 16;
         ashift = 24;
      }
      else {
         ashift = 24;
         rshift = 16;
         gshift = 8;
         bshift = 0;
      }
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_RGB) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      bshift = 0;
      gshift = 8;
      rshift = 16;
      ashift = 24;
      rind = 0;
      gind = 1;
      bind = 2;
      rgbmode = GL_TRUE;
      swalpha = GL_TRUE;
   }
   else if (format==OSMESA_BGR) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      bshift = 0;
      gshift = 8;
      rshift = 16;
      ashift = 24;
      rind = 2;
      gind = 1;
      bind = 0;
      rgbmode = GL_TRUE;
      swalpha = GL_TRUE;
   }
   else {
      return NULL;
   }


   osmesa = (OSMesaContext) CALLOC_STRUCT(osmesa_context);
   if (osmesa) {
      osmesa->gl_visual = _mesa_create_visual( rgbmode,
                                               GL_FALSE,    /* double buffer */
                                               GL_FALSE,    /* stereo */
                                               redBits,
                                               greenBits,
                                               blueBits,
                                               alphaBits,
                                               indexBits,
                                               depthBits,
                                               stencilBits,
                                               accumBits,
                                               accumBits,
                                               accumBits,
                                               alphaBits ? accumBits : 0,
                                               1            /* num samples */
                                               );
      if (!osmesa->gl_visual) {
         FREE(osmesa);
         return NULL;
      }

      if (!_mesa_initialize_context(&osmesa->gl_ctx,
                                    osmesa->gl_visual,
                                    sharelist ? &sharelist->gl_ctx
                                              : (GLcontext *) NULL,
                                    (void *) osmesa, GL_TRUE )) {
         _mesa_destroy_visual( osmesa->gl_visual );
         FREE(osmesa);
         return NULL;
      }

      _mesa_enable_sw_extensions(&(osmesa->gl_ctx));

      osmesa->gl_buffer = _mesa_create_framebuffer( osmesa->gl_visual,
                                          osmesa->gl_visual->depthBits > 0,
                                          osmesa->gl_visual->stencilBits > 0,
                                          osmesa->gl_visual->accumRedBits > 0,
                                          osmesa->gl_visual->alphaBits > 0 );

      if (!osmesa->gl_buffer) {
         _mesa_destroy_visual( osmesa->gl_visual );
         _mesa_free_context_data( &osmesa->gl_ctx );
         FREE(osmesa);
         return NULL;
      }
      osmesa->format = format;
      osmesa->buffer = NULL;
      osmesa->width = 0;
      osmesa->height = 0;
      osmesa->userRowLength = 0;
      osmesa->rowlength = 0;
      osmesa->yup = GL_TRUE;
      osmesa->rshift = rshift;
      osmesa->gshift = gshift;
      osmesa->bshift = bshift;
      osmesa->ashift = ashift;
      osmesa->rInd = rind;
      osmesa->gInd = gind;
      osmesa->bInd = bind;
      osmesa->aInd = aind;


      /* Initialize the software rasterizer and helper modules.
       */
      {
	 GLcontext *ctx = &osmesa->gl_ctx;

	 _swrast_CreateContext( ctx );
	 _ac_CreateContext( ctx );
	 _tnl_CreateContext( ctx );
	 _swsetup_CreateContext( ctx );
	
	 osmesa_register_swrast_functions( ctx );
      }
   }
   return osmesa;
}




/*
 * Destroy an Off-Screen Mesa rendering context.
 *
 * Input:  ctx - the context to destroy
 */
void GLAPIENTRY OSMesaDestroyContext( OSMesaContext ctx )
{
   if (ctx) {
      _swsetup_DestroyContext( &ctx->gl_ctx );
      _tnl_DestroyContext( &ctx->gl_ctx );
      _ac_DestroyContext( &ctx->gl_ctx );
      _swrast_DestroyContext( &ctx->gl_ctx );

      _mesa_destroy_visual( ctx->gl_visual );
      _mesa_destroy_framebuffer( ctx->gl_buffer );
      _mesa_free_context_data( &ctx->gl_ctx );
      FREE( ctx );
   }
}



/*
 * Recompute the values of the context's rowaddr array.
 */
static void compute_row_addresses( OSMesaContext ctx )
{
   GLint bytesPerPixel, bytesPerRow, i;
   GLubyte *origin = (GLubyte *) ctx->buffer;

   if (ctx->format == OSMESA_COLOR_INDEX) {
      /* CI mode */
      bytesPerPixel = 1 * sizeof(GLchan);
   }
   else if ((ctx->format == OSMESA_RGB) || (ctx->format == OSMESA_BGR)) {
      /* RGB mode */
      bytesPerPixel = 3 * sizeof(GLchan);
   }
   else {
      /* RGBA mode */
      bytesPerPixel = 4 * sizeof(GLchan);
   }

   bytesPerRow = ctx->rowlength * bytesPerPixel;

   if (ctx->yup) {
      /* Y=0 is bottom line of window */
      for (i = 0; i < MAX_HEIGHT; i++) {
         ctx->rowaddr[i] = (GLchan *) ((GLubyte *) origin + i * bytesPerRow);
      }
   }
   else {
      /* Y=0 is top line of window */
      for (i = 0; i < MAX_HEIGHT; i++) {
         GLint j = ctx->height - i - 1;
         ctx->rowaddr[i] = (GLchan *) ((GLubyte *) origin + j * bytesPerRow);
      }
   }
}


/*
 * Bind an OSMesaContext to an image buffer.  The image buffer is just a
 * block of memory which the client provides.  Its size must be at least
 * as large as width*height*sizeof(type).  Its address should be a multiple
 * of 4 if using RGBA mode.
 *
 * Image data is stored in the order of glDrawPixels:  row-major order
 * with the lower-left image pixel stored in the first array position
 * (ie. bottom-to-top).
 *
 * Since the only type initially supported is GL_UNSIGNED_BYTE, if the
 * context is in RGBA mode, each pixel will be stored as a 4-byte RGBA
 * value.  If the context is in color indexed mode, each pixel will be
 * stored as a 1-byte value.
 *
 * If the context's viewport hasn't been initialized yet, it will now be
 * initialized to (0,0,width,height).
 *
 * Input:  ctx - the rendering context
 *         buffer - the image buffer memory
 *         type - data type for pixel components, only GL_UNSIGNED_BYTE
 *                supported now
 *         width, height - size of image buffer in pixels, at least 1
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid ctx,
 *          invalid buffer address, type!=GL_UNSIGNED_BYTE, width<1, height<1,
 *          width>internal limit or height>internal limit.
 */
GLboolean GLAPIENTRY
OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type,
                   GLsizei width, GLsizei height )
{
   if (!ctx || !buffer || type != CHAN_TYPE ||
       width < 1 || height < 1 ||
       width > MAX_WIDTH || height > MAX_HEIGHT) {
      return GL_FALSE;
   }

   osmesa_update_state( &ctx->gl_ctx, 0 );
   _mesa_make_current( &ctx->gl_ctx, ctx->gl_buffer );

   ctx->buffer = buffer;
   ctx->width = width;
   ctx->height = height;
   if (ctx->userRowLength)
      ctx->rowlength = ctx->userRowLength;
   else
      ctx->rowlength = width;

   compute_row_addresses( ctx );

   /* init viewport */
   if (ctx->gl_ctx.Viewport.Width==0) {
      /* initialize viewport and scissor box to buffer size */
      _mesa_Viewport( 0, 0, width, height );
      ctx->gl_ctx.Scissor.Width = width;
      ctx->gl_ctx.Scissor.Height = height;
   }

   return GL_TRUE;
}



OSMesaContext GLAPIENTRY OSMesaGetCurrentContext( void )
{
   GLcontext *ctx = _mesa_get_current_context();
   if (ctx)
      return (OSMesaContext) ctx;
   else
      return NULL;
}



void GLAPIENTRY OSMesaPixelStore( GLint pname, GLint value )
{
   OSMesaContext ctx = OSMesaGetCurrentContext();

   switch (pname) {
      case OSMESA_ROW_LENGTH:
         if (value<0) {
            _mesa_error( &ctx->gl_ctx, GL_INVALID_VALUE,
                      "OSMesaPixelStore(value)" );
            return;
         }
         ctx->userRowLength = value;
         ctx->rowlength = value;
         break;
      case OSMESA_Y_UP:
         ctx->yup = value ? GL_TRUE : GL_FALSE;
         break;
      default:
         _mesa_error( &ctx->gl_ctx, GL_INVALID_ENUM, "OSMesaPixelStore(pname)" );
         return;
   }

   compute_row_addresses( ctx );
}


void GLAPIENTRY OSMesaGetIntegerv( GLint pname, GLint *value )
{
   OSMesaContext ctx = OSMesaGetCurrentContext();

   switch (pname) {
      case OSMESA_WIDTH:
         *value = ctx->width;
         return;
      case OSMESA_HEIGHT:
         *value = ctx->height;
         return;
      case OSMESA_FORMAT:
         *value = ctx->format;
         return;
      case OSMESA_TYPE:
         *value = CHAN_TYPE;
         return;
      case OSMESA_ROW_LENGTH:
         *value = ctx->rowlength;
         return;
      case OSMESA_Y_UP:
         *value = ctx->yup;
         return;
      default:
         _mesa_error(&ctx->gl_ctx, GL_INVALID_ENUM, "OSMesaGetIntergerv(pname)");
         return;
   }
}

/*
 * Return the depth buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLboolean GLAPIENTRY
OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height,
                      GLint *bytesPerValue, void **buffer )
{
   if ((!c->gl_buffer) || (!c->gl_buffer->DepthBuffer)) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = c->gl_buffer->Width;
      *height = c->gl_buffer->Height;
      if (c->gl_visual->depthBits <= 16)
         *bytesPerValue = sizeof(GLushort);
      else
         *bytesPerValue = sizeof(GLuint);
      *buffer = c->gl_buffer->DepthBuffer;
      return GL_TRUE;
   }
}

/*
 * Return the color buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          format - the pixel format (OSMESA_FORMAT)
 *          buffer - pointer to color buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLboolean GLAPIENTRY
OSMesaGetColorBuffer( OSMesaContext c, GLint *width,
                      GLint *height, GLint *format, void **buffer )
{
   if (!c->buffer) {
      *width = 0;
      *height = 0;
      *format = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = c->width;
      *height = c->height;
      *format = c->format;
      *buffer = c->buffer;
      return GL_TRUE;
   }
}

/**********************************************************************/
/*** Device Driver Functions                                        ***/
/**********************************************************************/


/*
 * Useful macros:
 */

#define PACK_RGBA(DST, R, G, B, A)	\
do {					\
   (DST)[osmesa->rInd] = R;		\
   (DST)[osmesa->gInd] = G;		\
   (DST)[osmesa->bInd] = B;		\
   (DST)[osmesa->aInd] = A;		\
} while (0)

#define PACK_RGB(DST, R, G, B)  \
do {				\
   (DST)[0] = R;		\
   (DST)[1] = G;		\
   (DST)[2] = B;		\
} while (0)

#define PACK_BGR(DST, R, G, B)  \
do {				\
   (DST)[0] = B;		\
   (DST)[1] = G;		\
   (DST)[2] = R;		\
} while (0)


#define UNPACK_RED(P)      ( (P)[osmesa->rInd] )
#define UNPACK_GREEN(P)    ( (P)[osmesa->gInd] )
#define UNPACK_BLUE(P)     ( (P)[osmesa->bInd] )
#define UNPACK_ALPHA(P)    ( (P)[osmesa->aInd] )


#define PIXELADDR1(X,Y)  (osmesa->rowaddr[Y] + (X))
#define PIXELADDR3(X,Y)  (osmesa->rowaddr[Y] + 3 * (X))
#define PIXELADDR4(X,Y)  (osmesa->rowaddr[Y] + 4 * (X))



static GLboolean set_draw_buffer( GLcontext *ctx, GLenum mode )
{
   (void) ctx;
   if (mode==GL_FRONT_LEFT) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


static void set_read_buffer( GLcontext *ctx, GLframebuffer *buffer, GLenum mode )
{
   /* separate read buffer not supported */
   ASSERT(buffer == ctx->DrawBuffer);
   ASSERT(mode == GL_FRONT_LEFT);
}


static void clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		   GLint x, GLint y, GLint width, GLint height )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;

   /* sanity check - we only have a front-left buffer */
   ASSERT((mask & (DD_FRONT_RIGHT_BIT | DD_BACK_LEFT_BIT | DD_BACK_RIGHT_BIT)) == 0);
   if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
      if (mask & DD_FRONT_LEFT_BIT) {
	 if (osmesa->format == OSMESA_COLOR_INDEX) {
	    if (all) {
	       /* Clear whole CI buffer */
#if CHAN_TYPE == GL_UNSIGNED_BYTE
	       MEMSET(osmesa->buffer, ctx->Color.ClearIndex,
		      osmesa->rowlength * osmesa->height);
#else
	       const GLint n = osmesa->rowlength * osmesa->height;
	       GLchan *buffer = (GLchan *) osmesa->buffer;
	       GLint i;
	       for (i = 0; i < n; i ++) {
		  buffer[i] = ctx->Color.ClearIndex;
	       }
#endif
	    }
	    else {
	       /* Clear part of CI buffer */
	       const GLchan clearIndex = (GLchan) ctx->Color.ClearIndex;
	       GLint i, j;
	       for (i = 0; i < height; i++) {
		  GLchan *ptr1 = PIXELADDR1(x, (y + i));
		  for (j = 0; j < width; j++) {
		     *ptr1++ = clearIndex;
		  }
	       }
	    }
	 }
	 else if (osmesa->format == OSMESA_RGB) {
	    const GLchan r = ctx->Color.ClearColor[0];
	    const GLchan g = ctx->Color.ClearColor[1];
	    const GLchan b = ctx->Color.ClearColor[2];
	    if (all) {
	       /* Clear whole RGB buffer */
	       GLuint n = osmesa->rowlength * osmesa->height;
	       GLchan *ptr3 = (GLchan *) osmesa->buffer;
	       GLuint i;
	       for (i = 0; i < n; i++) {
		  PACK_RGB(ptr3, r, g, b);
		  ptr3 += 3;
	       }
	    }
	    else {
	       /* Clear part of RGB buffer */
	       GLint i, j;
	       for (i = 0; i < height; i++) {
		  GLchan *ptr3 = PIXELADDR3(x, (y + i));
		  for (j = 0; j < width; j++) {
		     PACK_RGB(ptr3, r, g, b);
		     ptr3 += 3;
		  }
	       }
	    }
	 }
	 else if (osmesa->format == OSMESA_BGR) {
	    const GLchan r = ctx->Color.ClearColor[0];
	    const GLchan g = ctx->Color.ClearColor[1];
	    const GLchan b = ctx->Color.ClearColor[2];
	    if (all) {
	       /* Clear whole RGB buffer */
	       const GLint n = osmesa->rowlength * osmesa->height;
	       GLchan *ptr3 = (GLchan *) osmesa->buffer;
	       GLint i;
	       for (i = 0; i < n; i++) {
		  PACK_BGR(ptr3, r, g, b);
		  ptr3 += 3;
	       }
	    }
	    else {
	       /* Clear part of RGB buffer */
	       GLint i, j;
	       for (i = 0; i < height; i++) {
		  GLchan *ptr3 = PIXELADDR3(x, (y + i));
		  for (j = 0; j < width; j++) {
		     PACK_BGR(ptr3, r, g, b);
		     ptr3 += 3;
		  }
	       }
	    }
	 }
	 else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
	    /* 4-byte pixel value */
	    GLuint clearPixel;
	    GLchan *clr = (GLchan *) &clearPixel;
	    clr[osmesa->rInd] = ctx->Color.ClearColor[0];
	    clr[osmesa->gInd] = ctx->Color.ClearColor[1];
	    clr[osmesa->bInd] = ctx->Color.ClearColor[2];
	    clr[osmesa->aInd] = ctx->Color.ClearColor[3];
	    if (all) {
	       /* Clear whole RGBA buffer */
	       const GLuint n = osmesa->rowlength * osmesa->height;
	       GLuint *ptr4 = (GLuint *) osmesa->buffer;
	       GLuint i;
	       if (clearPixel) {
		  for (i = 0; i < n; i++) {
		     *ptr4++ = clearPixel;
		  }
	       }
	       else {
		  BZERO(ptr4, n * sizeof(GLuint));
	       }
	    }
	    else {
	       /* Clear part of RGBA buffer */
	       GLint i, j;
	       for (i = 0; i < height; i++) {
		  GLuint *ptr4 = (GLuint *) PIXELADDR4(x, (y + i));
		  for (j = 0; j < width; j++) {
		     *ptr4++ = clearPixel;
		  }
	       }
	    }
#else
	    const GLchan r = ctx->Color.ClearColor[0];
	    const GLchan g = ctx->Color.ClearColor[1];
	    const GLchan b = ctx->Color.ClearColor[2];
	    const GLchan a = ctx->Color.ClearColor[3];
	    if (all) {
	       /* Clear whole RGBA buffer */
	       const GLuint n = osmesa->rowlength * osmesa->height;
	       GLchan *p = (GLchan *) osmesa->buffer;
	       GLuint i;
	       for (i = 0; i < n; i++) {
		  PACK_RGBA(p, r, g, b, a);
		  p += 4;
	       }
	    }
	    else {
	       /* Clear part of RGBA buffer */
	       GLint i, j;
	       for (i = 0; i < height; i++) {
		  GLchan *p = PIXELADDR4(x, (y + i));
		  for (j = 0; j < width; j++) {
		     PACK_RGBA(p, r, g, b, a);
		     p += 4;
		  }
	       }
	    }

#endif
	 }
	 mask &= ~DD_FRONT_LEFT_BIT;
      }
   }
   
   if (mask)
      _swrast_Clear( ctx, mask, all, x, y, width, height );
}



static void buffer_size( GLcontext *ctx, GLuint *width, GLuint *height )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   *width = osmesa->width;
   *height = osmesa->height;
}


/**********************************************************************/
/*****        Read/write spans/arrays of RGBA pixels              *****/
/**********************************************************************/

/* Write RGBA pixels to an RGBA (or permuted) buffer. */
static void
write_rgba_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                 CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR4(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p += 4) {
         if (mask[i]) {
            PACK_RGBA(p, rgba[i][RCOMP], rgba[i][GCOMP],
                         rgba[i][BCOMP], rgba[i][ACOMP]);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p += 4) {
         PACK_RGBA(p, rgba[i][RCOMP], rgba[i][GCOMP],
                      rgba[i][BCOMP], rgba[i][ACOMP]);
      }
   }
}


/* Write RGBA pixels to an RGBA buffer.  This is the fastest span-writer. */
static void
write_rgba_span_rgba( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      CONST GLchan rgba[][4], const GLubyte mask[] )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint *ptr4 = (GLuint *) PIXELADDR4(x, y);
   const GLuint *rgba4 = (const GLuint *) rgba;
   GLuint i;
   ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            ptr4[i] = rgba4[i];
         }
      }
   }
   else {
      MEMCPY( ptr4, rgba4, n * 4 );
   }
}


/* Write RGB pixels to an RGBA (or permuted) buffer. */
static void
write_rgb_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                CONST GLchan rgb[][3], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR4(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p+=4) {
         if (mask[i]) {
            PACK_RGBA(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP], 255);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p+=4) {
         PACK_RGBA(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP], 255);
      }
   }
}



static void
write_monocolor_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR4(x, y);
   GLuint i;
   for (i = 0; i < n; i++, p += 4) {
      if (mask[i]) {
         PACK_RGBA(p, color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP]);
      }
   }
}



static void
write_rgba_pixels( const GLcontext *ctx, GLuint n,
                   const GLint x[], const GLint y[],
                   CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR4(x[i], y[i]);
         PACK_RGBA(p, rgba[i][RCOMP], rgba[i][GCOMP],
                      rgba[i][BCOMP], rgba[i][ACOMP]);
      }
   }
}



static void
write_monocolor_pixels( const GLcontext *ctx, GLuint n,
                        const GLint x[], const GLint y[],
                        const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR4(x[i], y[i]);
         PACK_RGBA(p, color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP]);
      }
   }
}


static void
read_rgba_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                GLchan rgba[][4] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   GLchan *p = PIXELADDR4(x, y);
   for (i = 0; i < n; i++, p += 4) {
      rgba[i][RCOMP] = UNPACK_RED(p);
      rgba[i][GCOMP] = UNPACK_GREEN(p);
      rgba[i][BCOMP] = UNPACK_BLUE(p);
      rgba[i][ACOMP] = UNPACK_ALPHA(p);
   }
}


/* Read RGBA pixels from an RGBA buffer */
static void
read_rgba_span_rgba( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                     GLchan rgba[][4] )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint *ptr4 = (GLuint *) PIXELADDR4(x, y);
   MEMCPY( rgba, ptr4, n * 4 * sizeof(GLchan) );
}


static void
read_rgba_pixels( const GLcontext *ctx,
                  GLuint n, const GLint x[], const GLint y[],
                  GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLchan *p = PIXELADDR4(x[i], y[i]);
         rgba[i][RCOMP] = UNPACK_RED(p);
         rgba[i][GCOMP] = UNPACK_GREEN(p);
         rgba[i][BCOMP] = UNPACK_BLUE(p);
         rgba[i][ACOMP] = UNPACK_ALPHA(p);
      }
   }
}

/**********************************************************************/
/*****                3 byte RGB pixel support funcs              *****/
/**********************************************************************/

/* Write RGBA pixels to an RGB buffer. */
static void
write_rgba_span_RGB( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                     CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p += 3) {
         if (mask[i]) {
            PACK_RGB(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p += 3) {
         PACK_RGB(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
   }
}

/* Write RGBA pixels to an BGR buffer. */
static void
write_rgba_span_BGR( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                     CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p += 3) {
         if (mask[i]) {
            PACK_BGR(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p += 3) {
         PACK_BGR(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
   }
}

/* Write RGB pixels to an RGB buffer. */
static void
write_rgb_span_RGB( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                    CONST GLchan rgb[][3], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p += 3) {
         if (mask[i]) {
            PACK_RGB(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p += 3) {
         PACK_RGB(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
      }
   }
}

/* Write RGB pixels to an BGR buffer. */
static void
write_rgb_span_BGR( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                    CONST GLchan rgb[][3], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   if (mask) {
      for (i = 0; i < n; i++, p += 3) {
         if (mask[i]) {
            PACK_BGR(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         }
      }
   }
   else {
      for (i = 0; i < n; i++, p += 3) {
         PACK_BGR(p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
      }
   }
}


static void
write_monocolor_span_RGB( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   for (i = 0; i < n; i++, p += 3) {
      if (mask[i]) {
         PACK_RGB(p, color[RCOMP], color[GCOMP], color[BCOMP]);
      }
   }
}

static void
write_monocolor_span_BGR( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *p = PIXELADDR3(x, y);
   GLuint i;
   for (i = 0; i < n; i++, p += 3) {
      if (mask[i]) {
         PACK_BGR(p, color[RCOMP], color[GCOMP], color[BCOMP]);
      }
   }
}

static void
write_rgba_pixels_RGB( const GLcontext *ctx, GLuint n,
                       const GLint x[], const GLint y[],
                       CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = (const OSMesaContext) ctx;
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR3(x[i], y[i]);
         PACK_RGB(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
   }
}

static void
write_rgba_pixels_BGR( const GLcontext *ctx, GLuint n,
                       const GLint x[], const GLint y[],
                       CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = (const OSMesaContext) ctx;
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR3(x[i], y[i]);
         PACK_BGR(p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
   }
}

static void
write_monocolor_pixels_RGB( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR3(x[i], y[i]);
         PACK_RGB(p, color[RCOMP], color[GCOMP], color[BCOMP]);
      }
   }
}

static void
write_monocolor_pixels_BGR( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLchan color[4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         GLchan *p = PIXELADDR3(x[i], y[i]);
         PACK_BGR(p, color[RCOMP], color[GCOMP], color[BCOMP]);
      }
   }
}

static void
read_rgba_span3( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                 GLchan rgba[][4] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   const GLchan *p = PIXELADDR3(x, y);
   for (i = 0; i < n; i++, p += 3) {
      rgba[i][RCOMP] = UNPACK_RED(p);
      rgba[i][GCOMP] = UNPACK_GREEN(p);
      rgba[i][BCOMP] = UNPACK_BLUE(p);
      rgba[i][ACOMP] = 255;
   }
}

static void
read_rgba_pixels3( const GLcontext *ctx,
                   GLuint n, const GLint x[], const GLint y[],
                   GLchan rgba[][4], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLchan *p = PIXELADDR3(x[i], y[i]);
         rgba[i][RCOMP] = UNPACK_RED(p);
         rgba[i][GCOMP] = UNPACK_GREEN(p);
         rgba[i][BCOMP] = UNPACK_BLUE(p);
         rgba[i][ACOMP] = 255;
      }
   }
}


/**********************************************************************/
/*****        Read/write spans/arrays of CI pixels                *****/
/**********************************************************************/

/* Write 32-bit color index to buffer */
static void
write_index32_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                    const GLuint index[], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *ptr1 = PIXELADDR1(x, y);
   GLuint i;
   if (mask) {
      for (i=0;i<n;i++,ptr1++) {
         if (mask[i]) {
            *ptr1 = (GLchan) index[i];
         }
      }
   }
   else {
      for (i=0;i<n;i++,ptr1++) {
         *ptr1 = (GLchan) index[i];
      }
   }
}


/* Write 8-bit color index to buffer */
static void
write_index8_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                   const GLubyte index[], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *ptr1 = PIXELADDR1(x, y);
   GLuint i;
   if (mask) {
      for (i=0;i<n;i++,ptr1++) {
         if (mask[i]) {
            *ptr1 = (GLchan) index[i];
         }
      }
   }
   else {
      MEMCPY(ptr1, index, n * sizeof(GLchan));
   }
}


static void
write_monoindex_span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      GLuint colorIndex, const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLchan *ptr1 = PIXELADDR1(x, y);
   GLuint i;
   for (i=0;i<n;i++,ptr1++) {
      if (mask[i]) {
         *ptr1 = (GLchan) colorIndex;
      }
   }
}


static void
write_index_pixels( const GLcontext *ctx,
                    GLuint n, const GLint x[], const GLint y[],
                    const GLuint index[], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLchan *ptr1 = PIXELADDR1(x[i], y[i]);
         *ptr1 = (GLchan) index[i];
      }
   }
}


static void
write_monoindex_pixels( const GLcontext *ctx,
                        GLuint n, const GLint x[], const GLint y[],
                        GLuint colorIndex, const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLchan *ptr1 = PIXELADDR1(x[i], y[i]);
         *ptr1 = (GLchan) colorIndex;
      }
   }
}


static void
read_index_span( const GLcontext *ctx,
                 GLuint n, GLint x, GLint y, GLuint index[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   const GLchan *ptr1 = (const GLchan *) PIXELADDR1(x, y);
   for (i=0;i<n;i++,ptr1++) {
      index[i] = (GLuint) *ptr1;
   }
}


static void
read_index_pixels( const GLcontext *ctx,
                   GLuint n, const GLint x[], const GLint y[],
                   GLuint index[], const GLubyte mask[] )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i] ) {
         const GLchan *ptr1 = PIXELADDR1(x[i], y[i]);
         index[i] = (GLuint) *ptr1;
      }
   }
}



/**********************************************************************/
/*****                   Optimized line rendering                 *****/
/**********************************************************************/


/*
 * Draw a flat-shaded, RGB line into an osmesa buffer.
 */
static void
flat_rgba_line( GLcontext *ctx, const SWvertex *vert0, const SWvertex *vert1 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLchan *color = vert0->color;

#define INTERP_XY 1
#define CLIP_HACK 1
#define PLOT(X, Y)						\
do {								\
   GLchan *p = PIXELADDR4(X, Y);				\
   PACK_RGBA(p, color[0], color[1], color[2], color[3]);	\
} while (0)

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif
}


/*
 * Draw a flat-shaded, Z-less, RGB line into an osmesa buffer.
 */
static void
flat_rgba_z_line(GLcontext *ctx, const SWvertex *vert0, const SWvertex *vert1)
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLchan *color = vert0->color;

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define CLIP_HACK 1
#define PLOT(X, Y)					\
do {							\
   if (Z < *zPtr) {					\
      GLchan *p = PIXELADDR4(X, Y);			\
      PACK_RGBA(p, color[RCOMP], color[GCOMP],		\
                   color[BCOMP], color[ACOMP]);		\
      *zPtr = Z;					\
   }							\
} while (0)


#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif
}


/*
 * Draw a flat-shaded, alpha-blended, RGB line into an osmesa buffer.
 * XXX update for GLchan
 */
static void
flat_blend_rgba_line( GLcontext *ctx,
                      const SWvertex *vert0, const SWvertex *vert1 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLint rshift = osmesa->rshift;
   const GLint gshift = osmesa->gshift;
   const GLint bshift = osmesa->bshift;
   const GLint avalue = vert0->color[3];
   const GLint msavalue = 255 - avalue;
   const GLint rvalue = vert0->color[0]*avalue;
   const GLint gvalue = vert0->color[1]*avalue;
   const GLint bvalue = vert0->color[2]*avalue;

#define INTERP_XY 1
#define CLIP_HACK 1
#define PLOT(X,Y)					\
   { GLuint *ptr4 = (GLuint *) PIXELADDR4(X, Y);		\
     GLuint  pixel = 0;					\
     pixel |=((((((*ptr4) >> rshift) & 0xff)*msavalue+rvalue)>>8) << rshift);\
     pixel |=((((((*ptr4) >> gshift) & 0xff)*msavalue+gvalue)>>8) << gshift);\
     pixel |=((((((*ptr4) >> bshift) & 0xff)*msavalue+bvalue)>>8) << bshift);\
     *ptr4 = pixel;					\
   }

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif
}


/*
 * Draw a flat-shaded, Z-less, alpha-blended, RGB line into an osmesa buffer.
 * XXX update for GLchan
 */
static void
flat_blend_rgba_z_line( GLcontext *ctx,
                        const SWvertex *vert0, const SWvertex *vert1 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLint rshift = osmesa->rshift;
   const GLint gshift = osmesa->gshift;
   const GLint bshift = osmesa->bshift;
   const GLint avalue = vert0->color[3];
   const GLint msavalue = 256 - avalue;
   const GLint rvalue = vert0->color[0]*avalue;
   const GLint gvalue = vert0->color[1]*avalue;
   const GLint bvalue = vert0->color[2]*avalue;

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define CLIP_HACK 1
#define PLOT(X,Y)							\
	if (Z < *zPtr) {						\
	   GLuint *ptr4 = (GLuint *) PIXELADDR4(X, Y);			\
	   GLuint  pixel = 0;						\
	   pixel |=((((((*ptr4) >> rshift) & 0xff)*msavalue+rvalue)>>8) << rshift);	\
	   pixel |=((((((*ptr4) >> gshift) & 0xff)*msavalue+gvalue)>>8) << gshift);	\
	   pixel |=((((((*ptr4) >> bshift) & 0xff)*msavalue+bvalue)>>8) << bshift);	\
	   *ptr4 = pixel; 						\
	}

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif
}


/*
 * Draw a flat-shaded, Z-less, alpha-blended, RGB line into an osmesa buffer.
 * XXX update for GLchan
 */
static void
flat_blend_rgba_z_line_write( GLcontext *ctx,
                              const SWvertex *vert0, const SWvertex *vert1 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLint rshift = osmesa->rshift;
   const GLint gshift = osmesa->gshift;
   const GLint bshift = osmesa->bshift;
   const GLint avalue = vert0->color[3];
   const GLint msavalue = 256 - avalue;
   const GLint rvalue = vert0->color[0]*avalue;
   const GLint gvalue = vert0->color[1]*avalue;
   const GLint bvalue = vert0->color[2]*avalue;

#define INTERP_XY 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define CLIP_HACK 1
#define PLOT(X,Y)							\
	if (Z < *zPtr) {						\
	   GLuint *ptr4 = (GLuint *) PIXELADDR4(X, Y);			\
	   GLuint  pixel = 0;						\
	   pixel |=((((((*ptr4) >> rshift) & 0xff)*msavalue+rvalue)>>8) << rshift);	\
	   pixel |=((((((*ptr4) >> gshift) & 0xff)*msavalue+gvalue)>>8) << gshift);	\
	   pixel |=((((((*ptr4) >> bshift) & 0xff)*msavalue+bvalue)>>8) << bshift);	\
	   *ptr4 = pixel;						\
	   *zPtr = Z;							\
	}

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif
}


/*
 * Analyze context state to see if we can provide a fast line drawing
 * function, like those in lines.c.  Otherwise, return NULL.
 */
static swrast_line_func
osmesa_choose_line_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (CHAN_BITS != 8)                    return NULL;
   if (ctx->RenderMode != GL_RENDER)      return NULL;
   if (ctx->Line.SmoothFlag)              return NULL;
   if (ctx->Texture._ReallyEnabled)       return NULL;
   if (ctx->Light.ShadeModel != GL_FLAT)  return NULL;
   if (ctx->Line.Width != 1.0F)           return NULL;
   if (ctx->Line.StippleFlag)             return NULL;
   if (ctx->Line.SmoothFlag)              return NULL;
   if (osmesa->format != OSMESA_RGBA &&
       osmesa->format != OSMESA_BGRA &&
       osmesa->format != OSMESA_ARGB)     return NULL;

   if (swrast->_RasterMask==DEPTH_BIT
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_TRUE
       && ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
      return flat_rgba_z_line;
   }

   if (swrast->_RasterMask == 0) {
      return flat_rgba_line;
   }

   if (swrast->_RasterMask==(DEPTH_BIT|BLEND_BIT)
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_TRUE
       && ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS
       && ctx->Color.BlendSrcRGB==GL_SRC_ALPHA
       && ctx->Color.BlendDstRGB==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendSrcA==GL_SRC_ALPHA
       && ctx->Color.BlendDstA==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendEquation==GL_FUNC_ADD_EXT) {
      return flat_blend_rgba_z_line_write;
   }

   if (swrast->_RasterMask==(DEPTH_BIT|BLEND_BIT)
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_FALSE
       && ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS
       && ctx->Color.BlendSrcRGB==GL_SRC_ALPHA
       && ctx->Color.BlendDstRGB==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendSrcA==GL_SRC_ALPHA
       && ctx->Color.BlendDstA==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendEquation==GL_FUNC_ADD_EXT) {
      return flat_blend_rgba_z_line;
   }

   if (swrast->_RasterMask==BLEND_BIT
       && ctx->Color.BlendSrcRGB==GL_SRC_ALPHA
       && ctx->Color.BlendDstRGB==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendSrcA==GL_SRC_ALPHA
       && ctx->Color.BlendDstA==GL_ONE_MINUS_SRC_ALPHA
       && ctx->Color.BlendEquation==GL_FUNC_ADD_EXT) {
      return flat_blend_rgba_line;
   }

   return NULL;
}


/**********************************************************************/
/*****                 Optimized triangle rendering               *****/
/**********************************************************************/


/*
 * Smooth-shaded, z-less triangle, RGBA color.
 */
static void smooth_rgba_z_triangle( GLcontext *ctx,
				    const SWvertex *v0,
                                    const SWvertex *v1,
                                    const SWvertex *v2 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);

#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INNER_LOOP( LEFT, RIGHT, Y )				\
{								\
   GLint i, len = RIGHT-LEFT;					\
   GLchan *img = PIXELADDR4(LEFT, Y); 				\
   for (i = 0; i < len; i++, img += 4) {			\
      GLdepth z = FixedToDepth(ffz);				\
      if (z < zRow[i]) {					\
         PACK_RGBA(img, FixedToInt(ffr), FixedToInt(ffg),	\
		        FixedToInt(ffb), FixedToInt(ffa));	\
         zRow[i] = z;						\
      }								\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;  ffa += fdadx;\
      ffz += fdzdx;						\
   }								\
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif
}




/*
 * Flat-shaded, z-less triangle, RGBA color.
 */
static void flat_rgba_z_triangle( GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1,
                                  const SWvertex *v2 )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE						\
   GLuint pixel;						\
   PACK_RGBA((GLchan *) &pixel, v0->color[0], v0->color[1],	\
                                v0->color[2], v0->color[3]);

#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   GLuint *img = (GLuint *) PIXELADDR4(LEFT, Y);   	\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
         img[i] = pixel;				\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif
}



/*
 * Return pointer to an accelerated triangle function if possible.
 */
static swrast_tri_func
osmesa_choose_triangle_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (CHAN_BITS != 8)                  return (swrast_tri_func) NULL;
   if (ctx->RenderMode != GL_RENDER)    return (swrast_tri_func) NULL;
   if (ctx->Polygon.SmoothFlag)         return (swrast_tri_func) NULL;
   if (ctx->Polygon.StippleFlag)        return (swrast_tri_func) NULL;
   if (ctx->Texture._ReallyEnabled)     return (swrast_tri_func) NULL;
   if (osmesa->format != OSMESA_RGBA &&
       osmesa->format != OSMESA_BGRA &&
       osmesa->format != OSMESA_ARGB)   return (swrast_tri_func) NULL;

   if (swrast->_RasterMask == DEPTH_BIT &&
       ctx->Depth.Func == GL_LESS &&
       ctx->Depth.Mask == GL_TRUE &&
       ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         return smooth_rgba_z_triangle;
      }
      else {
         return flat_rgba_z_triangle;
      }
   }
   return (swrast_tri_func) NULL;
}



/* Override for the swrast triangle-selection function.  Try to use one
 * of our internal triangle functions, otherwise fall back to the
 * standard swrast functions.
 */
static void osmesa_choose_triangle( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Triangle = osmesa_choose_triangle_function( ctx );
   if (!swrast->Triangle)
      _swrast_choose_triangle( ctx );
}

static void osmesa_choose_line( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Line = osmesa_choose_line_function( ctx );
   if (!swrast->Line)
      _swrast_choose_line( ctx );
}


#define OSMESA_NEW_LINE   (_NEW_LINE | \
                           _NEW_TEXTURE | \
                           _NEW_LIGHT | \
                           _NEW_DEPTH | \
                           _NEW_RENDERMODE | \
                           _SWRAST_NEW_RASTERMASK)

#define OSMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                             _NEW_TEXTURE | \
                             _NEW_LIGHT | \
                             _NEW_DEPTH | \
                             _NEW_RENDERMODE | \
                             _SWRAST_NEW_RASTERMASK)


/* Extend the software rasterizer with our line and triangle
 * functions.
 */
static void osmesa_register_swrast_functions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );

   swrast->choose_line = osmesa_choose_line;
   swrast->choose_triangle = osmesa_choose_triangle;

   swrast->invalidate_line |= OSMESA_NEW_LINE;
   swrast->invalidate_triangle |= OSMESA_NEW_TRIANGLE;
}


static const GLubyte *get_string( GLcontext *ctx, GLenum name )
{
   (void) ctx;
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa OffScreen";
      default:
         return NULL;
   }
}


static void osmesa_update_state( GLcontext *ctx, GLuint new_state )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   ASSERT((void *) osmesa == (void *) ctx->DriverCtx);

   /*
    * XXX these function pointers could be initialized just once during
    * context creation since they don't depend on any state changes.
    */

   ctx->Driver.GetString = get_string;
   ctx->Driver.UpdateState = osmesa_update_state;
   ctx->Driver.SetDrawBuffer = set_draw_buffer;
   ctx->Driver.ResizeBuffersMESA = _swrast_alloc_buffers;
   ctx->Driver.GetBufferSize = buffer_size;

   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.Clear = clear;
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;

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
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;


   /* RGB(A) span/pixel functions */
   if (osmesa->format == OSMESA_RGB) {
      swdd->WriteRGBASpan = write_rgba_span_RGB;
      swdd->WriteRGBSpan = write_rgb_span_RGB;
      swdd->WriteMonoRGBASpan = write_monocolor_span_RGB;
      swdd->WriteRGBAPixels = write_rgba_pixels_RGB;
      swdd->WriteMonoRGBAPixels = write_monocolor_pixels_RGB;
      swdd->ReadRGBASpan = read_rgba_span3;
      swdd->ReadRGBAPixels = read_rgba_pixels3;
   }
   else if (osmesa->format == OSMESA_BGR) {
      swdd->WriteRGBASpan = write_rgba_span_BGR;
      swdd->WriteRGBSpan = write_rgb_span_BGR;
      swdd->WriteMonoRGBASpan = write_monocolor_span_BGR;
      swdd->WriteRGBAPixels = write_rgba_pixels_BGR;
      swdd->WriteMonoRGBAPixels = write_monocolor_pixels_BGR;
      swdd->ReadRGBASpan = read_rgba_span3;
      swdd->ReadRGBAPixels = read_rgba_pixels3;
   }
   else {
      /* 4 bytes / pixel in frame buffer */
      swdd->WriteRGBSpan = write_rgb_span;
      swdd->WriteRGBAPixels = write_rgba_pixels;
      swdd->WriteMonoRGBASpan = write_monocolor_span;
      swdd->WriteMonoRGBAPixels = write_monocolor_pixels;
      if (osmesa->format == OSMESA_RGBA &&
          CHAN_TYPE == GL_UNSIGNED_BYTE &&
          RCOMP==0 && GCOMP==1 && BCOMP==2 && ACOMP==3) {
         /* special, fast case */
         swdd->WriteRGBASpan = write_rgba_span_rgba;
         swdd->ReadRGBASpan = read_rgba_span_rgba;
      }
      else {
         swdd->WriteRGBASpan = write_rgba_span;
         swdd->ReadRGBASpan = read_rgba_span;
      }
      swdd->ReadRGBAPixels = read_rgba_pixels;
   }

   /* CI span/pixel functions */
   swdd->WriteCI32Span = write_index32_span;
   swdd->WriteCI8Span = write_index8_span;
   swdd->WriteMonoCISpan = write_monoindex_span;
   swdd->WriteCI32Pixels = write_index_pixels;
   swdd->WriteMonoCIPixels = write_monoindex_pixels;
   swdd->ReadCI32Span = read_index_span;
   swdd->ReadCI32Pixels = read_index_pixels;

   swdd->SetReadBuffer = set_read_buffer;

   tnl->Driver.RenderStart = _swsetup_RenderStart;
   tnl->Driver.RenderFinish = _swsetup_RenderFinish;
   tnl->Driver.BuildProjectedVertices = _swsetup_BuildProjectedVertices;
   tnl->Driver.RenderPrimitive = _swsetup_RenderPrimitive;
   tnl->Driver.PointsFunc = _swsetup_Points;
   tnl->Driver.LineFunc = _swsetup_Line;
   tnl->Driver.TriangleFunc = _swsetup_Triangle;
   tnl->Driver.QuadFunc = _swsetup_Quad;
   tnl->Driver.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.RenderInterp = _swsetup_RenderInterp;
   tnl->Driver.RenderCopyPV = _swsetup_RenderCopyPV;
   tnl->Driver.RenderClippedLine = _swsetup_RenderClippedLine;
   tnl->Driver.RenderClippedPolygon = _swsetup_RenderClippedPolygon;


   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}
