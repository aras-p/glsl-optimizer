/* $Id: dd.h,v 1.22 2000/03/21 16:10:22 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#include "macros.h"


struct gl_pixelstore_attrib;


struct vertex_buffer;
struct immediate;
struct gl_pipeline_stage;


/* THIS FILE ONLY INCLUDED BY types.h !!!!! */


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




/* Used by the GetParameteri device driver function */
#define DD_HAVE_HARDWARE_FOG         3



/* Mask bits sent to the driver Clear() function */
#define DD_FRONT_LEFT_BIT  FRONT_LEFT_BIT         /* 1 */
#define DD_FRONT_RIGHT_BIT FRONT_RIGHT_BIT        /* 2 */
#define DD_BACK_LEFT_BIT   BACK_LEFT_BIT          /* 4 */
#define DD_BACK_RIGHT_BIT  BACK_RIGHT_BIT         /* 8 */
#define DD_DEPTH_BIT       GL_DEPTH_BUFFER_BIT    /* 0x00000100 */
#define DD_STENCIL_BIT     GL_STENCIL_BUFFER_BIT  /* 0x00000400 */
#define DD_ACCUM_BIT       GL_ACCUM_BUFFER_BIT    /* 0x00000200 */



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

   void (*UpdateState)( GLcontext *ctx );
   /*
    * UpdateState() is called whenver Mesa thinks the device driver should
    * update its state and/or the other pointers (such as PointsFunc,
    * LineFunc, or TriangleFunc).
    */

   void (*ClearIndex)( GLcontext *ctx, GLuint index );
   /*
    * Called whenever glClearIndex() is called.  Set the index for clearing
    * the color buffer when in color index mode.
    */

   void (*ClearColor)( GLcontext *ctx, GLubyte red, GLubyte green,
                                        GLubyte blue, GLubyte alpha );
   /*
    * Called whenever glClearColor() is called.  Set the color for clearing
    * the color buffer when in RGBA mode.
    */

   GLbitfield (*Clear)( GLcontext *ctx, GLbitfield mask, GLboolean all,
                        GLint x, GLint y, GLint width, GLint height );
   /* Clear the color/depth/stencil/accum buffer(s).
    * 'mask' is a bitmask of the DD_*_BIT values defined above that indicates
    * which buffers need to be cleared.  The driver should clear those
    * buffers then return a new bitmask indicating which buffers should be
    * cleared by software Mesa.
    * If 'all' is true then the clear the whole buffer, else clear only the
    * region defined by (x,y,width,height).
    * This function must obey the glColorMask, glIndexMask and glStencilMask
    * settings!  Software Mesa can do masked clears if the device driver can't.
    */

   void (*Index)( GLcontext *ctx, GLuint index );
   /*
    * Sets current color index for drawing flat-shaded primitives.
    * This index should also be used in the "mono" drawing functions.
    */

   void (*Color)( GLcontext *ctx,
                  GLubyte red, GLubyte green, GLubyte glue, GLubyte alpha );
   /*
    * Sets current color for drawing flat-shaded primitives.
    * This color should also be used in the "mono" drawing functions.
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
                          CONST GLubyte rgba[][4], const GLubyte mask[] );
   void (*WriteRGBSpan)( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         CONST GLubyte rgb[][3], const GLubyte mask[] );
   /* Write a horizontal run of RGBA or RGB pixels.
    * If mask is NULL, draw all pixels.
    * If mask is not null, only draw pixel [i] when mask [i] is true.
    */

   void (*WriteMonoRGBASpan)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                              const GLubyte mask[] );
   /* Write a horizontal run of RGBA pixels all with the color last
    * specified by the Color function.
    */

   void (*WriteRGBAPixels)( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            CONST GLubyte rgba[][4], const GLubyte mask[] );
   /* Write array of RGBA pixels at random locations.
    */

   void (*WriteMonoRGBAPixels)( const GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLubyte mask[] );
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
                            const GLubyte mask[] );
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
                              const GLubyte mask[] );
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
                         GLubyte rgba[][4] );
   /* Read a horizontal run of RGBA pixels.
    */

   void (*ReadCI32Pixels)( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint indx[], const GLubyte mask[] );
   /* Read a random array of CI pixels.
    */

   void (*ReadRGBAPixels)( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] );
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

   GLboolean (*IndexMask)( GLcontext *ctx, GLuint mask );
   /*
    * Implements glIndexMask() if possible, else return GL_FALSE.
    */

   GLboolean (*ColorMask)( GLcontext *ctx,
                           GLboolean rmask, GLboolean gmask,
                           GLboolean bmask, GLboolean amask );
   /*
    * Implements glColorMask() if possible, else return GL_FALSE.
    */

   GLboolean (*LogicOp)( GLcontext *ctx, GLenum op );
   /*
    * Implements glLogicOp() if possible, else return GL_FALSE.
    */

   void (*Dither)( GLcontext *ctx, GLboolean enable );
   /*
    * Enable/disable dithering.
    * NOTE: This function will be removed in the future in favor
    * of the "Enable" driver function.
    */

   void (*Error)( GLcontext *ctx );
   /*
    * Called whenever an error is generated.  ctx->ErrorValue contains
    * the error value.
    */

   void (*NearFar)( GLcontext *ctx, GLfloat nearVal, GLfloat farVal );
   /*
    * Called from glFrustum and glOrtho to tell device driver the
    * near and far clipping plane Z values.  The 3Dfx driver, for example,
    * uses this.
    */

   GLint (*GetParameteri)( const GLcontext *ctx, GLint param );
   /* Query the device driver to get an integer parameter.
    * Current parameters:
    *     DD_MAX_TEXTURE_SIZE         return maximum texture size
    *
    *     DD_MAX_TEXTURES             number of texture sets/stages, usually 1
    *
    *     DD_HAVE_HARDWARE_FOG        the driver should return 1 (0 otherwise)
    *                                 when the hardware support per fragment
    *                                 fog for free (like the Voodoo Graphics)
    *                                 so the Mesa core will start to ever use
    *                                 per fragment fog
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
    *** glDraw/Read/CopyPixels and glBitmap functions:
    ***/

   GLboolean (*DrawPixels)( GLcontext *ctx,
                            GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            const struct gl_pixelstore_attrib *unpack,
                            const GLvoid *pixels );
   /* This is called by glDrawPixels.
    * 'unpack' describes how to unpack the source image data.
    * Return GL_TRUE if the driver succeeds, return GL_FALSE if core Mesa
    * must do the job.
    */

   GLboolean (*ReadPixels)( GLcontext *ctx,
                            GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            const struct gl_pixelstore_attrib *unpack,
                            GLvoid *dest );
   /* Called by glReadPixels.
    * Return GL_TRUE if operation completed, else return GL_FALSE.
    * This function must respect all glPixelTransfer settings.
    */

   GLboolean (*CopyPixels)( GLcontext *ctx,
                            GLint srcx, GLint srcy,
                            GLsizei width, GLsizei height,
                            GLint dstx, GLint dsty, GLenum type );
   /* Do a glCopyPixels.  Return GL_TRUE if operation completed, else
    * return GL_FALSE.  This function must respect all rasterization
    * state, glPixelTransfer, glPixelZoom, etc.
    */

   GLboolean (*Bitmap)( GLcontext *ctx,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        const struct gl_pixelstore_attrib *unpack,
                        const GLubyte *bitmap );
   /* This is called by glBitmap.  Works the same as DrawPixels, above.
    */


   /***
    *** Texture mapping functions:
    ***/

   void (*TexEnv)( GLcontext *ctx, GLenum target, GLenum pname,
                   const GLfloat *param );
   /* Called whenever glTexEnv*() is called.
    */

   void (*TexImage)( GLcontext *ctx, GLenum target,
                     struct gl_texture_object *tObj, GLint level,
                     GLint internalFormat,
                     const struct gl_texture_image *image );
   /* XXX this function is obsolete */
   /* Called whenever a texture object's image is changed.
    *    texObject is the number of the texture object being changed.
    *    level indicates the mipmap level.
    *    internalFormat is the format in which the texture is to be stored.
    *    image is a pointer to a gl_texture_image struct which contains
    *       the actual image data.
    */

   void (*TexSubImage)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj, GLint level,
                        GLint xoffset, GLint yoffset,
                        GLsizei width, GLsizei height,
                        GLint internalFormat,
                        const struct gl_texture_image *image );
   /* XXX this function is obsolete */
   /* Called from glTexSubImage() to define a sub-region of a texture.
    */


   GLboolean (*TexImage1D)( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, const GLvoid *pixels,
                            const struct gl_pixelstore_attrib *packing,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage,
                            GLboolean *retainInternalCopy );
   GLboolean (*TexImage2D)( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, const GLvoid *pixels,
                            const struct gl_pixelstore_attrib *packing,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage,
                            GLboolean *retainInternalCopy );
   GLboolean (*TexImage3D)( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, const GLvoid *pixels,
                            const struct gl_pixelstore_attrib *packing,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage,
                            GLboolean *retainInternalCopy );
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

   GLboolean (*TexSubImage1D)( GLcontext *ctx, GLenum target, GLint level,
                               GLint xoffset, GLsizei width,
                               GLenum format, GLenum type,
                               const GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing,
                               struct gl_texture_object *texObj,
                               struct gl_texture_image *texImage );
   GLboolean (*TexSubImage2D)( GLcontext *ctx, GLenum target, GLint level,
                               GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing,
                               struct gl_texture_object *texObj,
                               struct gl_texture_image *texImage );
   GLboolean (*TexSubImage3D)( GLcontext *ctx, GLenum target, GLint level,
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

   GLvoid *(*GetTexImage)( GLcontext *ctx, GLenum target, GLint level,
                           GLenum *formatOut, GLenum *typeOut,
                           GLboolean *freeImageOut );
   /* Called by glGetTexImage or by core Mesa when a texture image
    * is needed for software fallback rendering.
    * Return the address of the texture image or NULL if failure.
    * The image must be tightly packed (i.e. row stride = image width)
    * Return the image's format and type in formatOut and typeOut.
    * The format and type must be values which are accepted by glTexImage.
    * Set the freeImageOut flag if the returned image should be deallocated
    * with FREE() when finished.
    * The size of the image can be deduced from the target and level.
    * Core Mesa will perform any image format/type conversions that are needed.
    */

   void (*TexParameter)( GLcontext *ctx, GLenum target,
                         struct gl_texture_object *tObj,
                         GLenum pname, const GLfloat *params );
   /* Called whenever glTexParameter*() is called.
    *    target is GL_TEXTURE_1D or GL_TEXTURE_2D
    *    texObject is the texture object to modify
    *    pname is one of GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
    *       GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, or GL_TEXTURE_BORDER_COLOR.
    *    params is dependant on pname.  See man glTexParameter.
    */

   void (*BindTexture)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj );
   /* Called whenever glBindTexture() is called.  This specifies which
    * texture is to be the current one.  No dirty flags will be set.
    */

   void (*DeleteTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /* Called when a texture object is about to be deallocated.  Driver
    * should free anything attached to the DriverData pointers.
    */

   void (*UpdateTexturePalette)( GLcontext *ctx,
                                 struct gl_texture_object *tObj );
   /* Called when the texture's color lookup table is changed.
    * If tObj is NULL then the shared texture palette ctx->Texture.Palette
    * was changed.
    */

   void (*ActiveTexture)( GLcontext *ctx, GLuint texUnitNumber );
   /* Called by glActiveTextureARB to set current texture unit.
    */


   GLboolean (*IsTextureResident)( GLcontext *ctx, 
                                   struct gl_texture_object *t );
   /* Allows the driver to implement the AreTexturesResident tests without
    * knowing about Mesa's internal hash tables for textures.
    */

   void (*PrioritizeTexture)( GLcontext *ctx,  struct gl_texture_object *t,
                              GLclampf priority );
   /* Notify driver of priority change for a texture.
    */


   /***
    *** Accelerated point, line, polygon, glDrawPixels and glBitmap functions:
    ***/

   points_func   PointsFunc;
   line_func     LineFunc;
   triangle_func TriangleFunc;
   quad_func     QuadFunc;
   rect_func     RectFunc;    
   

   /***
    *** Transformation/Rendering functions
    ***/

   void (*RenderStart)( GLcontext *ctx );
   void (*RenderFinish)( GLcontext *ctx );
    /* KW: These replace Begin and End, and have more relaxed semantics.
     * They are called prior-to and after one or more vb flush, and are
     * thus decoupled from the gl_begin/gl_end pairs, which are possibly 
     * more frequent.  If a begin/end pair covers >1 vertex buffer, these
     * are called at most once for the pair. (a bit broken at present)
     */

   void (*RasterSetup)( struct vertex_buffer *VB, GLuint start, GLuint end );
   /* This function, if not NULL, is called whenever new window coordinates
    * are put in the vertex buffer.  The vertices in question are those n
    * such that start <= n < end.
    * The device driver can convert the window coords to its own specialized
    * format.  The 3Dfx driver uses this.
    *
    * Note: Deprecated in favour of RegisterPipelineStages, below.
    */

   render_func *RenderVBClippedTab;
   render_func *RenderVBCulledTab;
   render_func *RenderVBRawTab;
   /* These function tables allow the device driver to rasterize an
    * entire begin/end group of primitives at once.  See the
    * gl_render_vb() function in vbrender.c for more details.  
    */

   void (*ReducedPrimitiveChange)( GLcontext *ctx, GLenum primitive );
   /* If registered, this will be called when rendering transitions between
    * points, lines and triangles.  It is not called on transitions between 
    * primtives such as GL_TRIANGLES and GL_TRIANGLE_STRIPS, or between
    * triangles and quads or triangles and polygons.
    */

   GLuint TriangleCaps;
   /* Holds a list of the reasons why we might normally want to call
    * render_triangle, but which are in fact implemented by the
    * driver.  The FX driver sets this to DD_TRI_CULL, and will soon
    * implement DD_TRI_OFFSET.
    */

   GLboolean (*MultipassFunc)( struct vertex_buffer *VB, GLuint passno );
   /* Driver may request additional render passes by returning GL_TRUE
    * when this function is called.  This function will be called
    * after the first pass, and passes will be made until the function
    * returns GL_FALSE.  If no function is registered, only one pass
    * is made.  
    * 
    * This function will be first invoked with passno == 1.
    */

   /***
    *** NEW in Mesa 3.x
    ***/

   void (*RegisterVB)( struct vertex_buffer *VB );
   void (*UnregisterVB)( struct vertex_buffer *VB );
   /* Do any processing (eg allocate memory) required to set up a new
    * vertex_buffer.  
    */


   void (*ResetVB)( struct vertex_buffer *VB );
   void (*ResetCvaVB)( struct vertex_buffer *VB, GLuint stages );
   /* Do any reset operations necessary to the driver data associated
    * with these vertex buffers.
    */

   GLuint RenderVectorFlags;
   /* What do the render tables require of the vectors they deal
    * with?  
    */

   GLuint (*RegisterPipelineStages)( struct gl_pipeline_stage *out,
				     const struct gl_pipeline_stage *in,
				     GLuint nr );
   /* Register new pipeline stages, or modify existing ones.  See also
    * the OptimizePipeline() functions.
    */


   GLboolean (*BuildPrecalcPipeline)( GLcontext *ctx );
   GLboolean (*BuildEltPipeline)( GLcontext *ctx );
   /* Perform the full pipeline build, or return false.
    */


   void (*OptimizePrecalcPipeline)( GLcontext *ctx, struct gl_pipeline *pipe );
   void (*OptimizeImmediatePipeline)( GLcontext *ctx, struct gl_pipeline *pipe);
   /* Check to see if a fast path exists for this combination of stages 
    * in the precalc and immediate (elt) pipelines.
    */


   /*
    * State-changing functions (drawing functions are above)
    *
    * These functions are called by their corresponding OpenGL API functions.
    * They're ALSO called by the gl_PopAttrib() function!!!
    * May add more functions like these to the device driver in the future.
    * This should reduce the amount of state checking that
    * the driver's UpdateState() function must do.
    */
   void (*AlphaFunc)(GLcontext *ctx, GLenum func, GLclampf ref);
   void (*BlendEquation)(GLcontext *ctx, GLenum mode);
   void (*BlendFunc)(GLcontext *ctx, GLenum sfactor, GLenum dfactor);
   void (*BlendFuncSeparate)( GLcontext *ctx, GLenum sfactorRGB, 
			      GLenum dfactorRGB, GLenum sfactorA,
			      GLenum dfactorA );
   void (*ClearDepth)(GLcontext *ctx, GLclampd d);
   void (*CullFace)(GLcontext *ctx, GLenum mode);
   void (*FrontFace)(GLcontext *ctx, GLenum mode);
   void (*DepthFunc)(GLcontext *ctx, GLenum func);
   void (*DepthMask)(GLcontext *ctx, GLboolean flag);
   void (*DepthRange)(GLcontext *ctx, GLclampd nearval, GLclampd farval);
   void (*Enable)(GLcontext* ctx, GLenum cap, GLboolean state);
   void (*Fogfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   void (*Hint)(GLcontext *ctx, GLenum target, GLenum mode);
   void (*Lightfv)(GLcontext *ctx, GLenum light,
		   GLenum pname, const GLfloat *params, GLint nparams );
   void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   void (*LineStipple)(GLcontext *ctx, GLint factor, GLushort pattern );
   void (*LineWidth)(GLcontext *ctx, GLfloat width);
   void (*LogicOpcode)(GLcontext *ctx, GLenum opcode);
   void (*PolygonMode)(GLcontext *ctx, GLenum face, GLenum mode);
   void (*PolygonStipple)(GLcontext *ctx, const GLubyte *mask );
   void (*Scissor)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   void (*ShadeModel)(GLcontext *ctx, GLenum mode);
   void (*ClearStencil)(GLcontext *ctx, GLint s);
   void (*StencilFunc)(GLcontext *ctx, GLenum func, GLint ref, GLuint mask);
   void (*StencilMask)(GLcontext *ctx, GLuint mask);
   void (*StencilOp)(GLcontext *ctx, GLenum fail, GLenum zfail, GLenum zpass);
   void (*Viewport)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
};



#endif

