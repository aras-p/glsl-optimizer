/* $Id: dd.h,v 1.52 2001/02/15 01:33:52 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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



#ifndef DD_INCLUDED
#define DD_INCLUDED

/* THIS FILE ONLY INCLUDED BY mtypes.h !!!!! */

struct gl_pixelstore_attrib;


/*
 *                      Device Driver (DD) interface
 *
 *
 * All device driver functions are accessed through pointers in the
 * dd_function_table struct (defined below) which is stored in the GLcontext
 * struct.  Since the device driver is strictly accessed trough a table of
 * function pointers we can:
 *   1. switch between a number of different device drivers at runtime.
 *   2. use optimized functions dependant on current rendering state or
 *      frame buffer configuration.
 *
 * The function pointers in the dd_function_table struct are divided into
 * two groups:  mandatory and optional.
 * Mandatory functions have to be implemented by every device driver.
 * Optional functions may or may not be implemented by the device driver.
 * The optional functions provide ways to take advantage of special hardware
 * or optimized algorithms.
 *
 * The function pointers in the dd_function_table struct should first be
 * initialized in the driver's "MakeCurrent" function.  The "MakeCurrent"
 * function is a little different in each device driver.  See the X/Mesa,
 * GLX, or OS/Mesa drivers for examples.
 *
 * Later, Mesa may call the dd_function_table's UpdateState() function.
 * This function should initialize the dd_function_table's pointers again.
 * The UpdateState() function is called whenever the core (GL) rendering
 * state is changed in a way which may effect rasterization.  For example,
 * the TriangleFunc() pointer may have to point to different functions
 * depending on whether smooth or flat shading is enabled.
 *
 * Note that the first argument to every device driver function is a
 * GLcontext *.  In turn, the GLcontext->DriverCtx pointer points to
 * the driver-specific context struct.  See the X/Mesa or OS/Mesa interface
 * for an example.
 *
 * For more information about writing a device driver see the ddsample.c
 * file and other device drivers (X/xmesa[1234].c, OSMesa/osmesa.c, etc)
 * for examples.
 *
 *
 * Look below in the dd_function_table struct definition for descriptions
 * of each device driver function.
 * 
 *
 * In the future more function pointers may be added for glReadPixels
 * glCopyPixels, etc.
 *
 *
 * Notes:
 * ------
 *   RGBA = red/green/blue/alpha
 *   CI = color index (color mapped mode)
 *   mono = all pixels have the same color or index
 *
 *   The write_ functions all take an array of mask flags which indicate
 *   whether or not the pixel should be written.  One special case exists
 *   in the write_color_span function: if the mask array is NULL, then
 *   draw all pixels.  This is an optimization used for glDrawPixels().
 *
 * IN ALL CASES:
 *      X coordinates start at 0 at the left and increase to the right
 *      Y coordinates start at 0 at the bottom and increase upward
 *
 */






/* Mask bits sent to the driver Clear() function */
#define DD_FRONT_LEFT_BIT  FRONT_LEFT_BIT         /* 1 */
#define DD_FRONT_RIGHT_BIT FRONT_RIGHT_BIT        /* 2 */
#define DD_BACK_LEFT_BIT   BACK_LEFT_BIT          /* 4 */
#define DD_BACK_RIGHT_BIT  BACK_RIGHT_BIT         /* 8 */
#define DD_DEPTH_BIT       GL_DEPTH_BUFFER_BIT    /* 0x00000100 */
#define DD_STENCIL_BIT     GL_STENCIL_BUFFER_BIT  /* 0x00000400 */
#define DD_ACCUM_BIT       GL_ACCUM_BUFFER_BIT    /* 0x00000200 */







/* Point, line, triangle, quadrilateral and rectangle rasterizer
 * functions.  These are specific to the tnl module and will shortly
 * move to a driver interface specific to that module.
 */
typedef void (*points_func)( GLcontext *ctx, GLuint first, GLuint last );

typedef void (*line_func)( GLcontext *ctx, GLuint v1, GLuint v2 );

typedef void (*triangle_func)( GLcontext *ctx,
                               GLuint v1, GLuint v2, GLuint v3 );

typedef void (*quad_func)( GLcontext *ctx, GLuint v1, GLuint v2,
                           GLuint v3, GLuint v4 );

typedef void (*render_func)( GLcontext *ctx, GLuint start, GLuint count, 
			     GLuint flags );

typedef void (*interp_func)( GLcontext *ctx,
			     GLfloat t, GLuint dst, GLuint in, GLuint out,
			     GLboolean force_boundary );

typedef void (*copy_pv_func)( GLcontext *ctx, GLuint dst, GLuint src );


/*
 * Device Driver function table.
 */
struct dd_function_table {

   /**********************************************************************
    *** Mandatory functions:  these functions must be implemented by   ***
    *** every device driver.                                           ***
    **********************************************************************/

   const GLubyte * (*GetString)( GLcontext *ctx, GLenum name );
   /* Return a string as needed by glGetString().
    * Only the GL_RENDERER token must be implemented.  Otherwise,
    * NULL can be returned.
    */

   void (*UpdateState)( GLcontext *ctx, GLuint new_state );
   /*
    * UpdateState() is called whenver Mesa thinks the device driver should
    * update its state and/or the other pointers (such as PointsFunc,
    * LineFunc, or TriangleFunc).
    */

   void (*Clear)( GLcontext *ctx, GLbitfield mask, GLboolean all,
		  GLint x, GLint y, GLint width, GLint height );
   /* Clear the color/depth/stencil/accum buffer(s).
    * 'mask' is a bitmask of the DD_*_BIT values defined above that indicates
    * which buffers need to be cleared. 
    * If 'all' is true then the clear the whole buffer, else clear only the
    * region defined by (x,y,width,height).
    * This function must obey the glColorMask, glIndexMask and glStencilMask
    * settings!  Software Mesa can do masked clears if the device driver can't.
    */

   GLboolean (*SetDrawBuffer)( GLcontext *ctx, GLenum buffer );
   /*
    * Specifies the current buffer for writing.
    * The following values must be accepted when applicable:
    *    GL_FRONT_LEFT - this buffer always exists
    *    GL_BACK_LEFT - when double buffering
    *    GL_FRONT_RIGHT - when using stereo
    *    GL_BACK_RIGHT - when using stereo and double buffering
    * The folowing values may optionally be accepted.  Return GL_TRUE
    * if accepted, GL_FALSE if not accepted.  In practice, only drivers
    * which can write to multiple color buffers at once should accept
    * these values.
    *    GL_FRONT - write to front left and front right if it exists
    *    GL_BACK - write to back left and back right if it exists
    *    GL_LEFT - write to front left and back left if it exists
    *    GL_RIGHT - write to right left and back right if they exist
    *    GL_FRONT_AND_BACK - write to all four buffers if they exist
    *    GL_NONE - disable buffer write in device driver.
    */

   void (*SetReadBuffer)( GLcontext *ctx, GLframebuffer *colorBuffer,
                          GLenum buffer );
   /*
    * Specifies the current buffer for reading.
    * colorBuffer will be one of:
    *    GL_FRONT_LEFT - this buffer always exists
    *    GL_BACK_LEFT - when double buffering
    *    GL_FRONT_RIGHT - when using stereo
    *    GL_BACK_RIGHT - when using stereo and double buffering
    */

   void (*GetBufferSize)( GLcontext *ctx, GLuint *width, GLuint *height );
   /*
    * Returns the width and height of the current color buffer.
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


   /**********************************************************************
    *** Optional functions:  these functions may or may not be         ***
    *** implemented by the device driver.  If the device driver        ***
    *** doesn't implement them it should never touch these pointers    ***
    *** since Mesa will either set them to NULL or point them at a     ***
    *** fall-back function.                                            ***
    **********************************************************************/

   void (*Finish)( GLcontext *ctx );
   /*
    * This is called whenever glFinish() is called.
    */

   void (*Flush)( GLcontext *ctx );
   /*
    * This is called whenever glFlush() is called.
    */

   void (*Error)( GLcontext *ctx );
   /*
    * Called whenever an error is generated.  ctx->ErrorValue contains
    * the error value.
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
  

   /***
    *** For hardware accumulation buffer:
    ***/
   void (*Accum)( GLcontext *ctx, GLenum op, GLfloat value,
		  GLint xpos, GLint ypos, GLint width, GLint height );
   /* Execute glAccum command within the given scissor region.
    */


   /***
    *** glDraw/Read/CopyPixels and glBitmap functions:
    ***/

   void (*DrawPixels)( GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       const GLvoid *pixels );
   /* This is called by glDrawPixels.
    * 'unpack' describes how to unpack the source image data.
    */

   void (*ReadPixels)( GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       GLvoid *dest );
   /* Called by glReadPixels.
    */

   void (*CopyPixels)( GLcontext *ctx,
                            GLint srcx, GLint srcy,
                            GLsizei width, GLsizei height,
                            GLint dstx, GLint dsty, GLenum type );
   /* Do a glCopyPixels.  This function must respect all rasterization
    * state, glPixelTransfer, glPixelZoom, etc.
    */

   void (*Bitmap)( GLcontext *ctx,
		   GLint x, GLint y, GLsizei width, GLsizei height,
		   const struct gl_pixelstore_attrib *unpack,
		   const GLubyte *bitmap );
   /* This is called by glBitmap.  Works the same as DrawPixels, above.
    */

   void (*ResizeBuffersMESA)( GLcontext *ctx );


   /***
    *** Texture image functions:
    ***/
   void (*TexImage1D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );
   void (*TexImage2D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );
   void (*TexImage3D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );
   /* Called by glTexImage1/2/3D.
    * Will not be called if any glPixelTransfer operations are enabled.
    * Arguments:
    *   <target>, <level>, <format>, <type> and <pixels> are user specified.
    *   <packing> indicates the image packing of pixels.
    *   <texObj> is the target texture object.
    *   <texImage> is the target texture image.  It will have the texture
    *      width, height, depth, border and internalFormat information.
    *   <retainInternalCopy> is returned by this function and indicates whether
    *      core Mesa should keep an internal copy of the texture image.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.  If GL_FALSE is returned, this function will be
    * called a second time after the texture image has been unpacked into
    * GLubytes.  It may be easier for the driver to handle then.
    */

   void (*TexSubImage1D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLsizei width,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );
   void (*TexSubImage2D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );
   void (*TexSubImage3D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLint depth,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );
   /* Called by glTexSubImage1/2/3D.
    * Will not be called if any glPixelTransfer operations are enabled.
    * Arguments:
    *   <target>, <level>, <xoffset>, <yoffset>, <zoffset>, <width>, <height>,
    *      <depth>, <format>, <type> and <pixels> are user specified.
    *   <packing> indicates the image packing of pixels.
    *   <texObj> is the target texture object.
    *   <texImage> is the target texture image.  It will have the texture
    *      width, height, border and internalFormat information.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.  If GL_FALSE is returned, then TexImage1/2/3D will
    * be called with the complete texture image.
    */
      
   GLboolean (*CopyTexImage1D)( GLcontext *ctx, GLenum target, GLint level,
                                GLenum internalFormat, GLint x, GLint y,
                                GLsizei width, GLint border );
   GLboolean (*CopyTexImage2D)( GLcontext *ctx, GLenum target, GLint level,
                                GLenum internalFormat, GLint x, GLint y,
                                GLsizei width, GLsizei height, GLint border );
   /* Called by glCopyTexImage1D and glCopyTexImage2D.
    * Will not be called if any glPixelTransfer operations are enabled.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.
    */

   GLboolean (*CopyTexSubImage1D)( GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset,
                                   GLint x, GLint y, GLsizei width );
   GLboolean (*CopyTexSubImage2D)( GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height );
   GLboolean (*CopyTexSubImage3D)( GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height );
   /* Called by glCopyTexSubImage1/2/3D.
    * Will not be called if any glPixelTransfer operations are enabled.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.
    */

   GLboolean (*TestProxyTexImage)(GLcontext *ctx, GLenum target,
                                  GLint level, GLint internalFormat,
                                  GLenum format, GLenum type,
                                  GLint width, GLint height,
                                  GLint depth, GLint border);
   /* Called by glTexImage[123]D when user specifies a proxy texture
    * target.  Return GL_TRUE if the proxy test passes, return GL_FALSE
    * if the test fails.
    */

   /***
    *** Compressed texture functions:
    ***/

   void (*CompressedTexImage1D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );
   void (*CompressedTexImage2D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );
   void (*CompressedTexImage3D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLsizei height, GLsizei depth,
                                 GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );
   /* Called by glCompressedTexImage1/2/3D.
    * Arguments:
    *   <target>, <level>, <internalFormat>, <data> are user specified.
    *   <texObj> is the target texture object.
    *   <texImage> is the target texture image.  It will have the texture
    *      width, height, depth, border and internalFormat information.
    *   <retainInternalCopy> is returned by this function and indicates whether
    *      core Mesa should keep an internal copy of the texture image.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.
    */

   void (*CompressedTexSubImage1D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLsizei width,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);
   void (*CompressedTexSubImage2D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLsizei width, GLint height,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);
   void (*CompressedTexSubImage3D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLsizei width, GLint height, GLint depth,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);
   /* Called by glCompressedTexSubImage1/2/3D.
    * Arguments:
    *   <target>, <level>, <x/z/zoffset>, <width>, <height>, <depth>,
    *      <imageSize>, and <data> are user specified.
    *   <texObj> is the target texture object.
    *   <texImage> is the target texture image.  It will have the texture
    *      width, height, depth, border and internalFormat information.
    * Return GL_TRUE if operation completed, return GL_FALSE if core Mesa
    * should do the job.
    */

   GLboolean (*IsCompressedFormat)(GLcontext *ctx, GLint internalFormat);
   /* Called to tell if a format is a compressed format.
    */

   void (*GetCompressedTexImage)( GLcontext *ctx, GLenum target,
                                  GLint lod, void *image,
                                  const struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage );
   /* Called by glGetCompressedTexImageARB.
    * <target>, <lod>, <image> are specified by user.
    * <texObj> is the source texture object.
    * <texImage> is the source texture image.
    */

   GLint (*BaseCompressedTexFormat)(GLcontext *ctx,
                                    GLint internalFormat);
   /* Called to compute the base format for a specific compressed
    * format.  Return -1 if the internalFormat is not a specific
    * compressed format that the driver recognizes.
    * Example: if internalFormat==GL_COMPRESSED_RGB_FXT1_3DFX, return GL_RGB.
    */

#if 000
   /* ... Note the
    * return value differences between this function and
    * SpecificCompressedTexFormat below.
    */

   GLint (*SpecificCompressedTexFormat)(GLcontext *ctx,
                                        GLint      internalFormat,
                                        GLint      numDimensions,
                                        GLint     *levelp,
                                        GLsizei   *widthp,
                                        GLsizei   *heightp,
                                        GLsizei   *depthp,
                                        GLint     *borderp,
                                        GLenum    *formatp,
                                        GLenum    *typep);
   /* Called to turn a generic texture format into a specific
    * texture format.  For example, if a driver implements
    * GL_3DFX_texture_compression_FXT1, this would map
    * GL_COMPRESSED_RGBA_ARB to GL_COMPRESSED_RGBA_FXT1_3DFX.
    *
    * If the driver does not know how to handle the compressed
    * format, then just return the generic format, and Mesa will
    * do the right thing with it.
    */

   GLsizei (*CompressedImageSize)(GLcontext *ctx,
                                  GLenum internalFormat,
                                  GLuint numDimensions,
                                  GLuint width,
                                  GLuint height,
                                  GLuint depth);
   /* Calculate the size of a compressed image, given the image's
    * format and dimensions.
    */
#endif

   /***
    *** Texture object functions:
    ***/

   void (*BindTexture)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj );
   /* Called by glBindTexture().
    */

   void (*CreateTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /* Called when a texture object is created.
    */

   void (*DeleteTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /* Called when a texture object is about to be deallocated.  Driver
    * should free anything attached to the DriverData pointers.
    */

   GLboolean (*IsTextureResident)( GLcontext *ctx, 
                                   struct gl_texture_object *t );
   /* Called by glAreTextureResident().
    */

   void (*PrioritizeTexture)( GLcontext *ctx,  struct gl_texture_object *t,
                              GLclampf priority );
   /* Called by glPrioritizeTextures().
    */

   void (*ActiveTexture)( GLcontext *ctx, GLuint texUnitNumber );
   /* Called by glActiveTextureARB to set current texture unit.
    */

   void (*UpdateTexturePalette)( GLcontext *ctx,
                                 struct gl_texture_object *tObj );
   /* Called when the texture's color lookup table is changed.
    * If tObj is NULL then the shared texture palette ctx->Texture.Palette
    * is to be updated.
    */


   /***
    *** State-changing functions (drawing functions are above)
    ***
    *** These functions are called by their corresponding OpenGL API functions.
    *** They're ALSO called by the gl_PopAttrib() function!!!
    *** May add more functions like these to the device driver in the future.
    ***/
   void (*AlphaFunc)(GLcontext *ctx, GLenum func, GLclampf ref);
   void (*BlendEquation)(GLcontext *ctx, GLenum mode);
   void (*BlendFunc)(GLcontext *ctx, GLenum sfactor, GLenum dfactor);
   void (*BlendFuncSeparate)(GLcontext *ctx,
                             GLenum sfactorRGB, GLenum dfactorRGB,
                             GLenum sfactorA, GLenum dfactorA);
   void (*ClearColor)(GLcontext *ctx, const GLchan color[4]);
   void (*ClearDepth)(GLcontext *ctx, GLclampd d);
   void (*ClearIndex)(GLcontext *ctx, GLuint index);
   void (*ClearStencil)(GLcontext *ctx, GLint s);
   void (*ColorMask)(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
                     GLboolean bmask, GLboolean amask );
   void (*CullFace)(GLcontext *ctx, GLenum mode);
   void (*ClipPlane)(GLcontext *ctx, GLenum plane, const GLfloat *equation );
   void (*FrontFace)(GLcontext *ctx, GLenum mode);
   void (*DepthFunc)(GLcontext *ctx, GLenum func);
   void (*DepthMask)(GLcontext *ctx, GLboolean flag);
   void (*DepthRange)(GLcontext *ctx, GLclampd nearval, GLclampd farval);
   void (*Enable)(GLcontext* ctx, GLenum cap, GLboolean state);
   void (*Fogfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   void (*Hint)(GLcontext *ctx, GLenum target, GLenum mode);
   void (*IndexMask)(GLcontext *ctx, GLuint mask);
   void (*Lightfv)(GLcontext *ctx, GLenum light,
		   GLenum pname, const GLfloat *params );
   void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   void (*LineStipple)(GLcontext *ctx, GLint factor, GLushort pattern );
   void (*LineWidth)(GLcontext *ctx, GLfloat width);
   void (*LogicOpcode)(GLcontext *ctx, GLenum opcode);
   void (*PointParameterfv)(GLcontext *ctx, GLenum pname,
                            const GLfloat *params);
   void (*PointSize)(GLcontext *ctx, GLfloat size);
   void (*PolygonMode)(GLcontext *ctx, GLenum face, GLenum mode);
   void (*PolygonStipple)(GLcontext *ctx, const GLubyte *mask );
   void (*RenderMode)(GLcontext *ctx, GLenum mode );
   void (*Scissor)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   void (*ShadeModel)(GLcontext *ctx, GLenum mode);
   void (*StencilFunc)(GLcontext *ctx, GLenum func, GLint ref, GLuint mask);
   void (*StencilMask)(GLcontext *ctx, GLuint mask);
   void (*StencilOp)(GLcontext *ctx, GLenum fail, GLenum zfail, GLenum zpass);
   void (*TexGen)(GLcontext *ctx, GLenum coord, GLenum pname, 
		  const GLfloat *params);
   void (*TexEnv)(GLcontext *ctx, GLenum target, GLenum pname,
                  const GLfloat *param);
   void (*TexParameter)(GLcontext *ctx, GLenum target,
                        struct gl_texture_object *texObj,
                        GLenum pname, const GLfloat *params);
   void (*TextureMatrix)(GLcontext *ctx, GLuint unit, const GLmatrix *mat);
   void (*Viewport)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);


   /*** State-query functions
    ***
    *** Return GL_TRUE if query was completed, GL_FALSE otherwise.
    ***/
   GLboolean (*GetBooleanv)(GLcontext *ctx, GLenum pname, GLboolean *result);
   GLboolean (*GetDoublev)(GLcontext *ctx, GLenum pname, GLdouble *result);
   GLboolean (*GetFloatv)(GLcontext *ctx, GLenum pname, GLfloat *result);
   GLboolean (*GetIntegerv)(GLcontext *ctx, GLenum pname, GLint *result);
   GLboolean (*GetPointerv)(GLcontext *ctx, GLenum pname, GLvoid **result);


   /***
    *** Vertex array functions
    ***
    *** Called by the corresponding OpenGL functions.
    ***/
   void (*VertexPointer)(GLcontext *ctx, GLint size, GLenum type, 
			 GLsizei stride, const GLvoid *ptr);
   void (*NormalPointer)(GLcontext *ctx, GLenum type, 
			 GLsizei stride, const GLvoid *ptr);
   void (*ColorPointer)(GLcontext *ctx, GLint size, GLenum type, 
			GLsizei stride, const GLvoid *ptr);
   void (*FogCoordPointer)(GLcontext *ctx, GLenum type, 
			   GLsizei stride, const GLvoid *ptr);
   void (*IndexPointer)(GLcontext *ctx, GLenum type, 
			GLsizei stride, const GLvoid *ptr);
   void (*SecondaryColorPointer)(GLcontext *ctx, GLint size, GLenum type, 
				 GLsizei stride, const GLvoid *ptr);
   void (*TexCoordPointer)(GLcontext *ctx, GLint size, GLenum type, 
			   GLsizei stride, const GLvoid *ptr);
   void (*EdgeFlagPointer)(GLcontext *ctx, GLsizei stride, const GLvoid *ptr);


   /***
    *** TNL Pipeline
    ***/

   void (*PipelineStart)(GLcontext *ctx);
   void (*PipelineFinish)(GLcontext *ctx);
   /* Called before and after all pipeline stages.
    * These are a suitable place for grabbing/releasing hardware locks.
    */

   /***
    *** Rendering
    ***/

   void (*RenderStart)(GLcontext *ctx);
   void (*RenderFinish)(GLcontext *ctx);
   /* Called before and after all rendering operations, including DrawPixels,
    * ReadPixels, Bitmap, span functions, and CopyTexImage, etc commands.
    * These are a suitable place for grabbing/releasing hardware locks.
    */

   void (*RenderPrimitive)(GLcontext *ctx, GLenum mode);
   /* Called between RednerStart() and RenderFinish() to indicate the
    * type of primitive we're about to draw.  Mode will be one of the
    * modes accepted by glBegin().
    */

   interp_func RenderInterp;
   copy_pv_func RenderCopyPV;
   void (*RenderClippedPolygon)( GLcontext *ctx, const GLuint *elts, GLuint n );
   void (*RenderClippedLine)( GLcontext *ctx, GLuint v0, GLuint v1 );
   /* Functions to interpolate between prebuilt vertices, copy flat-shade
    * provoking color, and to render clipped primitives.
    */

   /***
    *** Parameters for _tnl_render_stage
    ***/
   points_func           PointsFunc; /* must now respect vb->elts */
   line_func             LineFunc;
   triangle_func         TriangleFunc;
   quad_func             QuadFunc;
   /* These functions are called in order to render points, lines,
    * triangles and quads.  These are only called via the T&L module.
    */

   render_func          *RenderTabVerts;
   render_func          *RenderTabElts;
   /* XXX Description???
    */

   void (*ResetLineStipple)( GLcontext *ctx );
   /* Reset the hardware's line stipple counter.
    */
   
   void (*BuildProjectedVertices)( GLcontext *ctx, 
				   GLuint start, GLuint end,
				   GLuint new_inputs);
   /* This function is called whenever new vertices are required for
    * rendering.  The vertices in question are those n such that start
    * <= n < end.  The new_inputs parameter indicates those fields of
    * the vertex which need to be updated, if only a partial repair of
    * the vertex is required.
    *
    * This function is called only from _tnl_render_stage in tnl/t_render.c.  
    */


   GLboolean (*MultipassFunc)( GLcontext *ctx, GLuint passno );
   /* Driver may request additional render passes by returning GL_TRUE
    * when this function is called.  This function will be called
    * after the first pass, and passes will be made until the function
    * returns GL_FALSE.  If no function is registered, only one pass
    * is made.  
    * 
    * This function will be first invoked with passno == 1.
    */


   /***
    *** Support for multiple t&l engines
    ***/
#define PRIM_OUTSIDE_BEGIN_END   GL_POLYGON+1
#define PRIM_INSIDE_UNKNOWN_PRIM GL_POLYGON+2
#define PRIM_UNKNOWN             GL_POLYGON+3
   
   GLuint CurrentExecPrimitive;
   /* Set by the driver-supplied t&l engine.  Set to
    * PRIM_OUTSIDE_BEGIN_END when outside begin/end.  
    */

   GLuint CurrentSavePrimitive;
   /* Current state of an in-progress compilation.  May take on any of
    * the additional values defined above.
    */

   

#define FLUSH_STORED_VERTICES 0x1
#define FLUSH_UPDATE_CURRENT  0x2
   GLuint NeedFlush;
   /* Set by the driver-supplied t&l engine whenever vertices are
    * buffered between begin/end objects or ctx->Current is not uptodate.
    *
    * The FlushVertices() call below may be used to resolve 
    * these conditions.  
    */

   void (*FlushVertices)( GLcontext *ctx, GLuint flags );
   /* If inside begin/end, ASSERT(0).  
    * Otherwise, 
    *   if (flags & FLUSH_STORED_VERTICES) flushes any buffered vertices,
    *   if (flags & FLUSH_UPDATE_CURRENT) updates ctx->Current 
    *                                     and ctx->Light.Material
    *
    * Note that the default t&l engine never clears the
    * FLUSH_UPDATE_CURRENT bit, even after performing the update.
    */

   void (*LightingSpaceChange)( GLcontext *ctx );
   /* Notify driver that the special derived value _NeedEyeCoords has
    * changed.  
    */

   void (*NewList)( GLcontext *ctx, GLuint list, GLenum mode );
   void (*EndList)( GLcontext *ctx );
   /* Let the t&l component know what is going on with display lists
    * in time to make changes to dispatch tables, etc.
    * Called by glNewList() and glEndList(), respectively.
    */

   void (*BeginCallList)( GLcontext *ctx, GLuint list );
   void (*EndCallList)( GLcontext *ctx );
   /* Notify the t&l component before and after calling a display list.
    * Called by glCallList(s), but not recursively.
    */

   void (*MakeCurrent)( GLcontext *ctx, GLframebuffer *drawBuffer, 
			GLframebuffer *readBuffer );
   /* Let the t&l component know when the context becomes current.
    */


   void (*LockArraysEXT)( GLcontext *ctx, GLint first, GLsizei count );
   void (*UnlockArraysEXT)( GLcontext *ctx );
   /* Called by glLockArraysEXT() and glUnlockArraysEXT(), respectively.
    */

};



/*
 * Transform/Clip/Lighting interface
 */
typedef struct {
   void (*ArrayElement)( GLint ); /* NOTE */
   void (*Color3f)( GLfloat, GLfloat, GLfloat );
   void (*Color3fv)( const GLfloat * );
   void (*Color3ub)( GLubyte, GLubyte, GLubyte );
   void (*Color3ubv)( const GLubyte * );
   void (*Color4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (*Color4fv)( const GLfloat * );
   void (*Color4ub)( GLubyte, GLubyte, GLubyte, GLubyte );
   void (*Color4ubv)( const GLubyte * );
   void (*EdgeFlag)( GLboolean );
   void (*EdgeFlagv)( const GLboolean * );
   void (*EvalCoord1f)( GLfloat );          /* NOTE */
   void (*EvalCoord1fv)( const GLfloat * ); /* NOTE */
   void (*EvalCoord2f)( GLfloat, GLfloat ); /* NOTE */
   void (*EvalCoord2fv)( const GLfloat * ); /* NOTE */
   void (*EvalPoint1)( GLint );             /* NOTE */
   void (*EvalPoint2)( GLint, GLint );      /* NOTE */
   void (*FogCoordfEXT)( GLfloat );
   void (*FogCoordfvEXT)( const GLfloat * );
   void (*Indexi)( GLint );
   void (*Indexiv)( const GLint * );
   void (*Materialfv)( GLenum face, GLenum pname, const GLfloat * ); /* NOTE */ 
   void (*MultiTexCoord1fARB)( GLenum, GLfloat );
   void (*MultiTexCoord1fvARB)( GLenum, const GLfloat * );
   void (*MultiTexCoord2fARB)( GLenum, GLfloat, GLfloat );
   void (*MultiTexCoord2fvARB)( GLenum, const GLfloat * );
   void (*MultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat );
   void (*MultiTexCoord3fvARB)( GLenum, const GLfloat * );
   void (*MultiTexCoord4fARB)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*MultiTexCoord4fvARB)( GLenum, const GLfloat * );
   void (*Normal3f)( GLfloat, GLfloat, GLfloat );
   void (*Normal3fv)( const GLfloat * );
   void (*SecondaryColor3fEXT)( GLfloat, GLfloat, GLfloat );
   void (*SecondaryColor3fvEXT)( const GLfloat * );
   void (*SecondaryColor3ubEXT)( GLubyte, GLubyte, GLubyte );
   void (*SecondaryColor3ubvEXT)( const GLubyte * );
   void (*TexCoord1f)( GLfloat );
   void (*TexCoord1fv)( const GLfloat * );
   void (*TexCoord2f)( GLfloat, GLfloat );
   void (*TexCoord2fv)( const GLfloat * );
   void (*TexCoord3f)( GLfloat, GLfloat, GLfloat );
   void (*TexCoord3fv)( const GLfloat * );
   void (*TexCoord4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (*TexCoord4fv)( const GLfloat * );
   void (*Vertex2f)( GLfloat, GLfloat );
   void (*Vertex2fv)( const GLfloat * );
   void (*Vertex3f)( GLfloat, GLfloat, GLfloat );
   void (*Vertex3fv)( const GLfloat * );
   void (*Vertex4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (*Vertex4fv)( const GLfloat * );
   void (*CallList)( GLuint );	/* NOTE */
   void (*Begin)( GLenum );
   void (*End)( void );
   /* Drivers present a reduced set of the functions possible in
    * begin/end objects.  Core mesa provides translation stubs for the
    * remaining functions to map down to these entrypoints.  
    *
    * These are the initial values to be installed into dispatch by
    * mesa.  If the t&l driver wants to modify the dispatch table
    * while installed, it must do so itself.  It would be possible for
    * the vertexformat to install it's own initial values for these
    * functions, but this way there is an obvious list of what is
    * expected of the driver.
    *
    * If the driver wants to hook in entrypoints other than those
    * listed above, it must restore them to their original values in
    * the disable() callback, below.  
    */

   void (*Rectf)( GLfloat, GLfloat, GLfloat, GLfloat );
   /* 
    */


   void (*DrawArrays)( GLenum mode, GLint start, GLsizei count );
   void (*DrawElements)( GLenum mode, GLsizei count, GLenum type, 
			 const GLvoid *indices );
   void (*DrawRangeElements)(GLenum mode, GLuint start,
			     GLuint end, GLsizei count,
			     GLenum type, const GLvoid *indices);
   /* These may or may not belong here.  Heuristic: If an array is
    * enabled, the installed vertex format should support that array and
    * it's current size natively.
    */

   void (*EvalMesh1)( GLenum mode, GLint i1, GLint i2 );
   void (*EvalMesh2)( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
   /* If you don't support eval, fallback to the default vertex format
    * on receiving an eval call and use the pipeline mechanism to
    * provide partial t&l acceleration.
    *
    * Mesa will provide a set of helper functions to do eval within
    * accelerated vertex formats, eventually...  
    * 
    * Update: There seem to be issues re. maintaining correct values
    * for 'ctx->Current' in the face of Eval and T&L fallbacks...  
    */

   GLboolean prefer_float_colors;
   /* Should core send non-standard colors to glColor4f or glColor4ub
    */


} GLvertexformat;


#endif

