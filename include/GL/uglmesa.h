/* uglmesa.h - Public header UGL/Mesa */

/* Copyright (C) 2001 by Wind River Systems, Inc */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * The MIT License
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
 * THE AUTHORS OR COPYRIGHT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Author:
 * Stephane Raimbault <stephane.raimbault@windriver.com> 
 */

#ifndef UGLMESA_H
#define UGLMESA_H

#ifdef __cplusplus
extern "C" {
#endif

#define UGL_MESA_MAJOR_VERSION 1
#define UGL_MESA_MINOR_VERSION 0

#include <GL/gl.h>
#include <ugl/ugl.h>

/*
 * Values for display mode of uglMesaCreateContext ()
 */

#define UGL_MESA_SINGLE            0x00
#define UGL_MESA_DOUBLE            0x01
#define UGL_MESA_DOUBLE_SW         0x02
#define UGL_MESA_DOUBLE_HW         0x03
    
#define UGL_MESA_FULLSCREEN_WIDTH  0x0
#define UGL_MESA_FULLSCREEN_HEIGHT 0x0

/*
 * Pixel format
 */
#define UGL_MESA_ARGB8888          0x01
#define UGL_MESA_RGB565            0x02
#define UGL_MESA_RGB888            0x03
#define UGL_MESA_ARGB4444          0x04
#define UGL_MESA_CI                0x05
#define UGL_MESA_DITHER_RGB        0x10

/*
 * uglMesaPixelStore() parameters:
 */
    
#define UGL_MESA_ROW_LENGTH	0x10
#define UGL_MESA_Y_UP           0x11

/* 
 * Accepted by uglMesaGetIntegerv:
 */

#define UGL_MESA_LEFT_X		0x18
#define UGL_MESA_TOP_Y		0x19    
#define UGL_MESA_WIDTH		0x20
#define UGL_MESA_HEIGHT		0x21
#define UGL_MESA_DISPLAY_WIDTH  0x22
#define UGL_MESA_DISPLAY_HEIGHT 0x23
#define UGL_MESA_COLOR_FORMAT	0x24
#define UGL_MESA_COLOR_MODEL    0x25
#define UGL_MESA_PIXEL_FORMAT   0x26
#define UGL_MESA_TYPE		0x27
#define UGL_MESA_RGB		0x28
#define UGL_MESA_COLOR_INDEXED  0x29

/*
 * typedefs
 */

typedef struct uglMesaContext * UGL_MESA_CONTEXT;
    
/*
 * Create an Mesa/UGL rendering context. The attributes needed are
 * double buffer flag and a context sharelist.
 *
 * It's necessary to first call this function before use uglMakeCurrentContext.
 * This function provides neither stencil nor accumulation buffer only
 * a depth buffer to reduce memory footprint.
 *
 * Input:  db_mode - UGL_MESA_SINGLE = single buffer mode
 *                   UGL_MESA_DOUBLE = double buffer mode (HW fallback -> SW)
 *                   UGL_MESA_DOUBLE_SW = double buffer software
 *                   UGL_MESA_DOUBLE_HW = double buffer hardware
 *         share_list - specifies another UGL_MESA_CONTEXT with which to share
 *         display lists. NULL indicates no sharing.
 *
 * Return:  a UGL_MESA_CONTEXT, or zero if error
 */

UGL_MESA_CONTEXT uglMesaCreateNewContext (GLenum db_mode,
					  UGL_MESA_CONTEXT share_list);

/*
 * Create an UGL/Mesa rendering context and specify desired
 * size of depth buffer, stencil buffer and accumulation buffer.
 * If you specify zero for depth_bits, stencil_bits,
 * accum_[red|gren|blue]_bits, you can save some memory.
 *
 * INPUT:  db_mode - double buffer mode
 *         depth_bits - depth buffer size
 *         stencil_bits - stencil buffer size
 *         accum_red_bits - accumulation red buffer size
 *         accum_green_bits - accumulation green buffer size
 *         accum_blue_bits -accumulation blue buffer size
 *         accum_alpha_bits -accumulation alpha buffer size
 *         share_list - specifies another UGL_MESA_CONTEXT with which to share
 *                     display lists.  NULL indicates no sharing.
 */
UGL_MESA_CONTEXT  uglMesaCreateNewContextExt (GLenum db_flag,
					      GLint depth_bits,
					      GLint stencil_bits,
					      GLint accum_red_bits,
					      GLint accum_green_bits,
					      GLint accum_blue_bits,
					      GLint accum_alpha_bits,
					      UGL_MESA_CONTEXT share_list);

/* 
 * Bind an UGL_MESA_CONTEXT to an image buffer. The image buffer is
 * just a block of memory which the client provides.  Its size must be
 * at least as large as width*height*sizeof(type).  Its address should
 * be a multiple of 4 if using RGBA mode.
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
 * initialized to (0, 0, width, height).
 *
 * Input:  umc - a rendering context
 *         left, top - coordinates in pixels of (left,top) pixel
 *         (0,0) in fullscreen mode.
 *         width, height - size of image buffer in pixels, at least 1
 *         else fullscreen dimensions are used (UGL_MESA_DISPLAY_WIDTH
 *         and UGL_MESA_DISPLAY_HEIGHT).
 *
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid umc,
 *          width<1, height<1, width>internal limit or height>internal limit.
 */

GLboolean uglMesaMakeCurrentContext (UGL_MESA_CONTEXT umc,
				     GLsizei left, GLsizei top,
				     GLsizei width, GLsizei height);

/*
 * Move an OpenGL window by a delta value   
 *
 * Input:  dx, dy - delta values in pixels
 *
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid
 *          coordinates.
 */
GLboolean uglMesaMoveWindow (GLsizei dx, GLsizei dy);

/*
 * Move an OpenGL window to an absolute position   
 *
 * Input:  left, top - new coordinates in pixels
 *
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid
 *          coordinates.
 */
GLboolean uglMesaMoveToWindow (GLsizei left, GLsizei top);

/*
 * Resize an OpenGL window by a delta value
 *
 * Input:  dw, dh - delta values in pixels
 *
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid
 *          coordinates.
 */
GLboolean uglMesaResizeWindow (GLsizei dw, GLsizei dh);

/*
 * Resize an OpenGL window to an absolute size
 *
 * Input:  width, height - new dimensions in pixels
 *
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid
 *          coordinates.
 */
GLboolean uglMesaResizeToWindow (GLsizei width, GLsizei height);

/*
 * Destroy the current UGL/Mesa rendering context
 *
 */
void uglMesaDestroyContext (void);

/*
 * Return the current UGL/Mesa context 
 *
 * Return:  a UGL/Mesa context, or NULL if error
 *
 */
UGL_MESA_CONTEXT uglMesaGetCurrentContext (void);

/*
 * Swap front and back buffers in double buffering mode. This
 * function is a no-op if there's no back buffer. In case of software
 * double buffering a copy occurs from off-screen buffer to front
 * buffer.  Works faster with an hardware support.
 */

void uglMesaSwapBuffers (void);

/*
 * Set pixel store/packing parameters for the current context.  This
 * is similar to glPixelStore. UGL uses Y coordinates increase
 * downward.
 *
 * Input:  pname - UGL_MESA_ROW_LENGTH
 *                 zero, same as image width (default).
 *                 value specify actual pixels per row in image buffer
 *                 UGL_MESA_Y_UP:
 *                 zero = Y coordinates increase downward (default)
 *                 non-zero = Y coordinates increase upward 
 *         value - value for the parameter pname
 */

void uglMesaPixelStore (GLint pname, GLint value);

/*
 * Return an integer value like glGetIntegerv.
 *
 * Input:  pname - UGL_MESA_LEFT_X        return the x axis value
 *                                       of the most left pixel
 *                 UGL_MESA_TOP_Y         return the y axis value
 *                                       of the topper pixel
 *                 UGL_MESA_WIDTH         return current image width
 *                 UGL_MESA_HEIGHT        return current image height
 *                 UGL_MESA_COLOR_FORMAT  return image color format
 *                 UGL_MESA_COLOR_MODEL   return image color model
 *                 UGL_MESA_PIXEL_FORMAT  return pixel format
 *                 UGL_MESA_ROW_LENGTH    return row length in pixels
 *                 UGL_MESA_RGB           return true if RGB
 *                 UGL_MESA_COLOR_INDEXED return true if color indexed
 *         value - pointer to integer in which to return result.
 */
void uglMesaGetIntegerv (GLint pname, GLint *value);

/*
 * Return the depth buffer associated with an UGL/Mesa context.
 *
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 *
 */
GLboolean uglMesaGetDepthBuffer (GLint *width, GLint *height,
				 GLint *bytesPerValue, void **buffer);

/*
 * Return the color buffer associated with an UGL/Mesa context.
 * Input:  c - the UGL/Mesa context
 * Output:  width, height - size of buffer in pixels
 *          format - buffer format (UGLMESA_FORMAT)
 *          buffer - pointer to color buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 *
 */
GLboolean uglMesaGetColorBuffer (GLint *width, GLint *height,
				 GLint *format, void **buffer);

/*
 * Color allocation in indexed mode.
 * This function does nothing in RGB mode.
 *
 * Input:  index - Value for the current color index
 *         red - Red component (between 0 and 1)
 *         green - Green component (between 0 and 1)
 *         blue - Blue component (between 0 and 1)
 *
 * Return: GL_TRUE if success, or GL_FALSE if index<0 or * clutSize<index,
 *         red, green and blue are not between 0.0 and 1.0.
 * 
 */

GLboolean uglMesaSetColor (GLubyte index, GLfloat red,
			   GLfloat green, GLfloat blue);
  
#ifdef __cplusplus
}
#endif


#endif
