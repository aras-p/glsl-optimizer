/* $Id: swrast.h,v 1.16 2002/01/27 18:32:03 brianp Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#ifndef SWRAST_H
#define SWRAST_H

#include "mtypes.h"


/* The software rasterizer now uses this format for vertices.  Thus a
 * 'RasterSetup' stage or other translation is required between the
 * tnl module and the swrast rasterization functions.  This serves to
 * isolate the swrast module from the internals of the tnl module, and
 * improve its usefulness as a fallback mechanism for hardware
 * drivers.
 *
 * Full software drivers:
 *   - Register the rastersetup and triangle functions from
 *     utils/software_helper.
 *   - On statechange, update the rasterization pointers in that module.
 *
 * Rasterization hardware drivers:
 *   - Keep native rastersetup.
 *   - Implement native twoside,offset and unfilled triangle setup.
 *   - Implement a translator from native vertices to swrast vertices.
 *   - On partial fallback (mix of accelerated and unaccelerated
 *   prims), call a pass-through function which translates native
 *   vertices to SWvertices and calls the appropriate swrast function.
 *   - On total fallback (vertex format insufficient for state or all
 *     primitives unaccelerated), hook in swrast_setup instead.
 */
typedef struct {
   GLfloat win[4];
   GLfloat texcoord[MAX_TEXTURE_UNITS][4];
   GLchan color[4];
   GLchan specular[4];
   GLfloat fog;
   GLuint index;
   GLfloat pointSize;
} SWvertex;


/*
 * The sw_span structure is used by the triangle template code in
 * s_tritemp.h.  It describes how colors, Z, texcoords, etc are to be
 * interpolated across each scanline of triangle.
 * With this structure it's easy to hand-off span rasterization to a
 * subroutine instead of doing it all inline like we used to do.
 * It also cleans up the local variable namespace a great deal.
 *
 * It would be interesting to experiment with multiprocessor rasterization
 * with this structure.  The triangle rasterizer could simply emit a
 * stream of these structures which would be consumed by one or more
 * span-processing threads which could run in parallel.
 */


/* When the sw_span struct is initialized, these flags indicates
 * which values are needed for rendering the triangle.
 */
#define SPAN_RGBA         0x001
#define SPAN_SPEC         0x002
#define SPAN_INDEX        0x004
#define SPAN_Z            0x008
#define SPAN_FOG          0x010
#define SPAN_TEXTURE      0x020
#define SPAN_INT_TEXTURE  0x040
#define SPAN_LAMBDA       0x080
#define SPAN_COVERAGE     0x100
#define SPAN_FLAT         0x200  /* flat shading? */


struct sw_span {
   GLint x, y;

   /* only need to process pixels between start <= i < end */
   GLuint start, end;

   /* This flag indicates that only a part of the span is visible */
   GLboolean writeAll;

   /* This bitmask (bitwise-or of SPAN_* flags) indicates which of the
    * x/xStep variables are relevant.
    */
   GLuint interpMask;

#if CHAN_TYPE == GL_FLOAT
   GLfloat red, redStep;
   GLfloat green, greenStep;
   GLfloat blue, blueStep;
   GLfloat alpha, alphaStep;
   GLfloat specRed, specRedStep;
   GLfloat specGreen, specGreenStep;
   GLfloat specBlue, specBlueStep;
#else /* CHAN_TYPE == */
   GLfixed red, redStep;
   GLfixed green, greenStep;
   GLfixed blue, blueStep;
   GLfixed alpha, alphaStep;
   GLfixed specRed, specRedStep;
   GLfixed specGreen, specGreenStep;
   GLfixed specBlue, specBlueStep;
#endif
   GLfixed index, indexStep;
   GLfixed z, zStep;
   GLfloat fog, fogStep;
   GLfloat tex[MAX_TEXTURE_UNITS][4], texStep[MAX_TEXTURE_UNITS][4];
   GLfixed intTex[2], intTexStep[2];
   /* Needed for texture lambda (LOD) computation */
   GLfloat rho[MAX_TEXTURE_UNITS];
   GLfloat texWidth[MAX_TEXTURE_UNITS], texHeight[MAX_TEXTURE_UNITS];

   /* This bitmask (bitwise-or of SPAN_* flags) indicates which of the
    * fragment arrays are relevant.
    */
   GLuint arrayMask;

   /**
    * Arrays of fragment values.  These will either be computed from the
    * x/xStep values above or loadd from glDrawPixels, etc.
    */
   union {
      GLchan rgb[MAX_WIDTH][3];
      GLchan rgba[MAX_WIDTH][4];
      GLuint index[MAX_WIDTH];
   } color;
   GLchan  specArray[MAX_WIDTH][4];
   GLdepth zArray[MAX_WIDTH];
   GLfloat fogArray[MAX_WIDTH];
   /* Texture (s,t,r).  4th component only used for pixel texture */
   GLfloat texcoords[MAX_TEXTURE_UNITS][MAX_WIDTH][4];
   GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat coverage[MAX_WIDTH];

   /* This mask indicates if fragment is alive or culled */
   GLubyte mask[MAX_WIDTH];

#ifdef DEBUG
   GLboolean filledDepth, filledAlpha;
   GLboolean filledColor, filledSpecular;
   GLboolean filledLambda[MAX_TEXTURE_UNITS], filledTex[MAX_TEXTURE_UNITS];
#endif
};


#define INIT_SPAN(S)	\
do {			\
   S.interpMask = 0;	\
   S.arrayMask = 0;	\
   S.start = S.end = 0;	\
} while (0)


#ifdef DEBUG
#define SW_SPAN_SET_FLAG(flag) {ASSERT((flag) == GL_FALSE);(flag) = GL_TRUE;}
#define SW_SPAN_RESET(span) {                                        \
         (span).filledDepth = (span).filledAlpha \
         = (span).filledColor = (span).filledSpecular = GL_FALSE;    \
         MEMSET((span).filledTex, GL_FALSE,                          \
		MAX_TEXTURE_UNITS*sizeof(GLboolean));                \
         MEMSET((span).filledLambda, GL_FALSE,                       \
		MAX_TEXTURE_UNITS*sizeof(GLboolean));                \
         (span).start = 0; (span).writeAll = GL_TRUE;}
#else
#define SW_SPAN_SET_FLAG(flag) ;
#define SW_SPAN_RESET(span) {(span).start = 0;(span).writeAll = GL_TRUE;}
#endif

struct swrast_device_driver;


/* These are the public-access functions exported from swrast.
 */
extern void
_swrast_alloc_buffers( GLcontext *ctx );

extern GLboolean
_swrast_CreateContext( GLcontext *ctx );

extern void
_swrast_DestroyContext( GLcontext *ctx );

/* Get a (non-const) reference to the device driver struct for swrast.
 */
extern struct swrast_device_driver *
_swrast_GetDeviceDriverReference( GLcontext *ctx );

extern void
_swrast_Bitmap( GLcontext *ctx,
		GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap );

extern void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy,
		    GLint destx, GLint desty,
		    GLsizei width, GLsizei height,
		    GLenum type );

extern void
_swrast_DrawPixels( GLcontext *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels );

extern void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    GLvoid *pixels );

extern void
_swrast_Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
	       GLint x, GLint y, GLint width, GLint height );

extern void
_swrast_Accum( GLcontext *ctx, GLenum op,
	       GLfloat value, GLint xpos, GLint ypos,
	       GLint width, GLint height );


/* Reset the stipple counter
 */
extern void
_swrast_ResetLineStipple( GLcontext *ctx );

/* These will always render the correct point/line/triangle for the
 * current state.
 *
 * For flatshaded primitives, the provoking vertex is the final one.
 */
extern void
_swrast_Point( GLcontext *ctx, const SWvertex *v );

extern void
_swrast_Line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 );

extern void
_swrast_Triangle( GLcontext *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 );

extern void
_swrast_Quad( GLcontext *ctx,
              const SWvertex *v0, const SWvertex *v1,
	      const SWvertex *v2,  const SWvertex *v3);

extern void
_swrast_flush( GLcontext *ctx );


/* Tell the software rasterizer about core state changes.
 */
extern void
_swrast_InvalidateState( GLcontext *ctx, GLuint new_state );

/* Configure software rasterizer to match hardware rasterizer characteristics:
 */
extern void
_swrast_allow_vertex_fog( GLcontext *ctx, GLboolean value );

extern void
_swrast_allow_pixel_fog( GLcontext *ctx, GLboolean value );

/* Debug:
 */
extern void
_swrast_print_vertex( GLcontext *ctx, const SWvertex *v );


/*
 * Imaging fallbacks (a better solution should be found, perhaps
 * moving all the imaging fallback code to a new module) 
 */
void
_swrast_CopyConvolutionFilter2D(GLcontext *ctx, GLenum target, 
				GLenum internalFormat, 
				GLint x, GLint y, GLsizei width, 
				GLsizei height);
void
_swrast_CopyConvolutionFilter1D(GLcontext *ctx, GLenum target, 
				GLenum internalFormat, 
				GLint x, GLint y, GLsizei width);
void
_swrast_CopyColorSubTable( GLcontext *ctx,GLenum target, GLsizei start,
			   GLint x, GLint y, GLsizei width);
void
_swrast_CopyColorTable( GLcontext *ctx, 
			GLenum target, GLenum internalformat,
			GLint x, GLint y, GLsizei width);


/*
 * Texture fallbacks, Brian Paul.  Could also live in a new module
 * with the rest of the texture store fallbacks?
 */
extern void
_swrast_copy_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLint border);

extern void
_swrast_copy_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border);


extern void
_swrast_copy_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width);

extern void
_swrast_copy_texsubimage2d(GLcontext *ctx,
                         GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height);

extern void
_swrast_copy_texsubimage3d(GLcontext *ctx,
                         GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height);



/* The driver interface for the software rasterizer.  Unless otherwise
 * noted, all functions are mandatory.  
 */
struct swrast_device_driver {

   void (*SetReadBuffer)( GLcontext *ctx, GLframebuffer *colorBuffer,
                          GLenum buffer );
   /*
    * Specifies the current buffer for span/pixel reading.
    * colorBuffer will be one of:
    *    GL_FRONT_LEFT - this buffer always exists
    *    GL_BACK_LEFT - when double buffering
    *    GL_FRONT_RIGHT - when using stereo
    *    GL_BACK_RIGHT - when using stereo and double buffering
    */


   /***
    *** Functions for synchronizing access to the framebuffer:
    ***/

   void (*SpanRenderStart)(GLcontext *ctx);
   void (*SpanRenderFinish)(GLcontext *ctx);
   /* OPTIONAL.
    *
    * Called before and after all rendering operations, including DrawPixels,
    * ReadPixels, Bitmap, span functions, and CopyTexImage, etc commands.
    * These are a suitable place for grabbing/releasing hardware locks.
    *
    * NOTE: The swrast triangle/line/point routines *DO NOT* call
    * these functions.  Locking in that case must be organized by the
    * driver by other mechanisms.
    */

   /***
    *** Functions for writing pixels to the frame buffer:
    ***/

   void (*WriteRGBASpan)( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          CONST GLchan rgba[][4], const GLubyte mask[] );
   void (*WriteRGBSpan)( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         CONST GLchan rgb[][3], const GLubyte mask[] );
   /* Write a horizontal run of RGBA or RGB pixels.
    * If mask is NULL, draw all pixels.
    * If mask is not null, only draw pixel [i] when mask [i] is true.
    */

   void (*WriteMonoRGBASpan)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                              const GLchan color[4], const GLubyte mask[] );
   /* Write a horizontal run of RGBA pixels all with the same color.
    */

   void (*WriteRGBAPixels)( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            CONST GLchan rgba[][4], const GLubyte mask[] );
   /* Write array of RGBA pixels at random locations.
    */

   void (*WriteMonoRGBAPixels)( const GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLchan color[4], const GLubyte mask[] );
   /* Write an array of mono-RGBA pixels at random locations.
    */

   void (*WriteCI32Span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLuint index[], const GLubyte mask[] );
   void (*WriteCI8Span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLubyte index[], const GLubyte mask[] );
   /* Write a horizontal run of CI pixels.  One function is for 32bpp
    * indexes and the other for 8bpp pixels (the common case).  You mus
    * implement both for color index mode.
    */

   void (*WriteMonoCISpan)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLuint colorIndex, const GLubyte mask[] );
   /* Write a horizontal run of color index pixels using the color index
    * last specified by the Index() function.
    */

   void (*WriteCI32Pixels)( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLuint index[], const GLubyte mask[] );
   /*
    * Write a random array of CI pixels.
    */

   void (*WriteMonoCIPixels)( const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint colorIndex, const GLubyte mask[] );
   /* Write a random array of color index pixels using the color index
    * last specified by the Index() function.
    */


   /***
    *** Functions to read pixels from frame buffer:
    ***/

   void (*ReadCI32Span)( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint index[] );
   /* Read a horizontal run of color index pixels.
    */

   void (*ReadRGBASpan)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLchan rgba[][4] );
   /* Read a horizontal run of RGBA pixels.
    */

   void (*ReadCI32Pixels)( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint indx[], const GLubyte mask[] );
   /* Read a random array of CI pixels.
    */

   void (*ReadRGBAPixels)( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLchan rgba[][4], const GLubyte mask[] );
   /* Read a random array of RGBA pixels.
    */



   /***
    *** For supporting hardware Z buffers:
    *** Either ALL or NONE of these functions must be implemented!
    *** NOTE that Each depth value is a 32-bit GLuint.  If the depth
    *** buffer is less than 32 bits deep then the extra upperbits are zero.
    ***/

   void (*WriteDepthSpan)( GLcontext *ctx, GLuint n, GLint x, GLint y,
                           const GLdepth depth[], const GLubyte mask[] );
   /* Write a horizontal span of values into the depth buffer.  Only write
    * depth[i] value if mask[i] is nonzero.
    */

   void (*ReadDepthSpan)( GLcontext *ctx, GLuint n, GLint x, GLint y,
                          GLdepth depth[] );
   /* Read a horizontal span of values from the depth buffer.
    */


   void (*WriteDepthPixels)( GLcontext *ctx, GLuint n,
                             const GLint x[], const GLint y[],
                             const GLdepth depth[], const GLubyte mask[] );
   /* Write an array of randomly positioned depth values into the
    * depth buffer.  Only write depth[i] value if mask[i] is nonzero.
    */

   void (*ReadDepthPixels)( GLcontext *ctx, GLuint n,
                            const GLint x[], const GLint y[],
                            GLdepth depth[] );
   /* Read an array of randomly positioned depth values from the depth buffer.
    */



   /***
    *** For supporting hardware stencil buffers:
    *** Either ALL or NONE of these functions must be implemented!
    ***/

   void (*WriteStencilSpan)( GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLstencil stencil[], const GLubyte mask[] );
   /* Write a horizontal span of stencil values into the stencil buffer.
    * If mask is NULL, write all stencil values.
    * Else, only write stencil[i] if mask[i] is non-zero.
    */

   void (*ReadStencilSpan)( GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLstencil stencil[] );
   /* Read a horizontal span of stencil values from the stencil buffer.
    */

   void (*WriteStencilPixels)( GLcontext *ctx, GLuint n,
                               const GLint x[], const GLint y[],
                               const GLstencil stencil[],
                               const GLubyte mask[] );
   /* Write an array of stencil values into the stencil buffer.
    * If mask is NULL, write all stencil values.
    * Else, only write stencil[i] if mask[i] is non-zero.
    */

   void (*ReadStencilPixels)( GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              GLstencil stencil[] );
   /* Read an array of stencil values from the stencil buffer.
    */
};



#endif
