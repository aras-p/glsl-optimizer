/* $Id: dd.h,v 1.6 1999/11/25 17:36:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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





/*
 * Device Driver function table.
 */
struct dd_function_table {

   /**********************************************************************
    *** Mandatory functions:  these functions must be implemented by   ***
    *** every device driver.                                           ***
    **********************************************************************/

   const char * (*RendererString)(void);
   /*
    * Return a string which uniquely identifies this device driver.
    * The string should contain no whitespace.  Examples: "X11", "OffScreen",
    * "MSWindows", "SVGA".
    * NOTE: This function will be obsolete in favor of GetString in the future!
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
    * 'mask' indicates which buffers need to be cleared.  Return a bitmask
    *    indicating which buffers weren't cleared by the driver function.
    * If 'all' is true then the clear the whole buffer, else clear the
    *    region defined by (x,y,width,height).
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

   const char * (*ExtensionString)( GLcontext *ctx );
   /* Return a space-separated list of extensions for this driver.
    * NOTE: This function will be obsolete in favor of GetString in the future!
    */

   const GLubyte * (*GetString)( GLcontext *ctx, GLenum name );
   /* Return a string as needed by glGetString().
    * NOTE: This will replace the ExtensionString and RendererString
    * functions in the future!
    */

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
    ***/

   void (*AllocDepthBuffer)( GLcontext *ctx );
   /*
    * Called when the depth buffer must be allocated or possibly resized.
    */

   GLuint (*DepthTestSpan)( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            GLubyte mask[] );
   void (*DepthTestPixels)( GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLdepth z[], GLubyte mask[] );
   /*
    * Apply the depth buffer test to an span/array of pixels and return
    * an updated pixel mask.  This function is not used when accelerated
    * point, line, polygon functions are used.
    */

   void (*ReadDepthSpanFloat)( GLcontext *ctx,
                               GLuint n, GLint x, GLint y, GLfloat depth[]);
   void (*ReadDepthSpanInt)( GLcontext *ctx,
                             GLuint n, GLint x, GLint y, GLdepth depth[] );
   /*
    * Return depth values as integers for glReadPixels.
    * Floats should be returned in the range [0,1].
    * Ints (GLdepth) values should be in the range [0,MAXDEPTH].
    */


   /***
    *** Accelerated point, line, polygon, glDrawPixels and glBitmap functions:
    ***/

   points_func   PointsFunc;
   line_func     LineFunc;
   triangle_func TriangleFunc;
   quad_func     QuadFunc;
   rect_func     RectFunc;    
   

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

   GLboolean (*Bitmap)( GLcontext *ctx,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        const struct gl_pixelstore_attrib *unpack,
                        const GLubyte *bitmap );
   /* This is called by glBitmap.  Works the same as DrawPixels, above.
    */

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
    *** Texture mapping functions:
    ***/

   void (*TexEnv)( GLcontext *ctx, GLenum pname, const GLfloat *param );
   /*
    * Called whenever glTexEnv*() is called.
    * Pname will be one of GL_TEXTURE_ENV_MODE or GL_TEXTURE_ENV_COLOR.
    * If pname is GL_TEXTURE_ENV_MODE then param will be one
    * of GL_MODULATE, GL_BLEND, GL_DECAL, or GL_REPLACE.
    */

   void (*TexImage)( GLcontext *ctx, GLenum target,
                     struct gl_texture_object *tObj, GLint level,
                     GLint internalFormat,
                     const struct gl_texture_image *image );
   /*
    * Called whenever a texture object's image is changed.
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
   /*
    * Called from glTexSubImage() to define a sub-region of a texture.
    */

   void (*TexParameter)( GLcontext *ctx, GLenum target,
                         struct gl_texture_object *tObj,
                         GLenum pname, const GLfloat *params );
   /*
    * Called whenever glTexParameter*() is called.
    *    target is GL_TEXTURE_1D or GL_TEXTURE_2D
    *    texObject is the texture object to modify
    *    pname is one of GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
    *       GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, or GL_TEXTURE_BORDER_COLOR.
    *    params is dependant on pname.  See man glTexParameter.
    */

   void (*BindTexture)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj );
   /*
    * Called whenever glBindTexture() is called.  This specifies which
    * texture is to be the current one.  No dirty flags will be set.
    */

   void (*DeleteTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /*
    * Called when a texture object is about to be deallocated.  Driver
    * should free anything attached to the DriverData pointers.
    */

   void (*UpdateTexturePalette)( GLcontext *ctx,
                                 struct gl_texture_object *tObj );
   /*
    * Called when the texture's color lookup table is changed.
    * If tObj is NULL then the shared texture palette ctx->Texture.Palette
    * was changed.
    */

   void (*UseGlobalTexturePalette)( GLcontext *ctx, GLboolean state );
   /*
    * Called via glEnable/Disable(GL_SHARED_TEXTURE_PALETTE_EXT)
    */

   void (*ActiveTexture)( GLcontext *ctx, GLuint texUnitNumber );
   /*
    * Called by glActiveTextureARB to set current texture unit.
    */


   GLboolean (*IsTextureResident)( GLcontext *ctx, 
				   struct gl_texture_object *t );
   /*
    * Allows the driver to implement the AreTexturesResident tests without
    * knowing about Mesa's internal hash tables for textures.
    */

   void (*PrioritizeTexture)( GLcontext *ctx, 
			      struct gl_texture_object *t,
			      GLclampf priority );
   /*
    * Notify driver of priority change for a texture.
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
   void (*PolygonMode)(GLcontext *ctx, GLenum face, GLenum mode);
   void (*Scissor)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   void (*ShadeModel)(GLcontext *ctx, GLenum mode);
   void (*ClearStencil)(GLcontext *ctx, GLint s);
   void (*StencilFunc)(GLcontext *ctx, GLenum func, GLint ref, GLuint mask);
   void (*StencilMask)(GLcontext *ctx, GLuint mask);
   void (*StencilOp)(GLcontext *ctx, GLenum fail, GLenum zfail, GLenum zpass);
   void (*Viewport)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
};



#endif

