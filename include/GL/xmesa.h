/* $Id: xmesa.h,v 1.4 2000/01/13 17:40:12 brianp Exp $ */

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


/*
 * Mesa/X11 interface.  This header file serves as the documentation for
 * the Mesa/X11 interface functions.
 *
 * Note: this interface isn't intended for user programs.  It's primarily
 * just for implementing the pseudo-GLX interface.
 */


/* Sample Usage:

In addition to the usual X calls to select a visual, create a colormap
and create a window, you must do the following to use the X/Mesa interface:

1. Call XMesaCreateVisual() to make an XMesaVisual from an XVisualInfo.

2. Call XMesaCreateContext() to create an X/Mesa rendering context, given
   the XMesaVisual.

3. Call XMesaCreateWindowBuffer() to create an XMesaBuffer from an X window
   and XMesaVisual.

4. Call XMesaMakeCurrent() to bind the XMesaBuffer to an XMesaContext and
   to make the context the current one.

5. Make gl* calls to render your graphics.

6. Use XMesaSwapBuffers() when double buffering to swap front/back buffers.

7. Before the X window is destroyed, call XMesaDestroyBuffer().

8. Before exiting, call XMesaDestroyVisual and XMesaDestroyContext.

See the demos/xdemo.c and xmesa1.c files for examples.
*/




#ifndef XMESA_H
#define XMESA_H


#ifdef __cplusplus
extern "C" {
#endif


#ifdef XFree86Server
#include "xmesa_xf86.h"
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "xmesa_x.h"
#ifdef GLX_DIRECT_RENDERING
#include "dri_mesa.h"
#endif
#endif
#include "GL/gl.h"

#ifdef AMIWIN
#include <pragmas/xlib_pragmas.h>
extern struct Library *XLibBase;
#endif


#define XMESA_MAJOR_VERSION 3
#define XMESA_MINOR_VERSION 3



/*
 * Values passed to XMesaGetString:
 */
#define XMESA_VERSION 1
#define XMESA_EXTENSIONS 2


/*
 * Values passed to XMesaSetFXmode:
 */
#define XMESA_FX_WINDOW       1
#define XMESA_FX_FULLSCREEN   2



typedef struct xmesa_context *XMesaContext;

typedef struct xmesa_visual *XMesaVisual;

typedef struct xmesa_buffer *XMesaBuffer;

#if defined(GLX_DIRECT_RENDERING) && !defined(XFree86Server)
/*
 * Initialize the XMesa driver.
 */
extern GLboolean XMesaInitDriver( __DRIscreenPrivate *driScrnPriv );

/*
 * Reset the XMesa driver when the X server resets.
 */
extern void XMesaResetDriver( __DRIscreenPrivate *driScrnPriv );
#endif



/*
 * Create a new X/Mesa visual.
 * Input:  display - X11 display
 *         visinfo - an XVisualInfo pointer
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         alpha_flag - alpha buffer requested?
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         stereo_flag - stereo visual?
 *         depth_size - requested bits/depth values, or zero
 *         stencil_size - requested bits/stencil values, or zero
 *         accum_size - requested bits/component values, or zero
 *         ximage_flag - GL_TRUE = use an XImage for back buffer,
 *                       GL_FALSE = use an off-screen pixmap for back buffer
 * Return;  a new XMesaVisual or 0 if error.
 */
extern XMesaVisual XMesaCreateVisual( XMesaDisplay *display,
				      XMesaVisualInfo visinfo,
				      GLboolean rgb_flag,
				      GLboolean alpha_flag,
				      GLboolean db_flag,
				      GLboolean stereo_flag,
				      GLboolean ximage_flag,
				      GLint depth_size,
				      GLint stencil_size,
				      GLint accum_size,
				      GLint level );

/*
 * Destroy an XMesaVisual, but not the associated XVisualInfo.
 */
extern void XMesaDestroyVisual( XMesaVisual v );



/*
 * Create a new XMesaContext for rendering into an X11 window.
 *
 * Input:  visual - an XMesaVisual
 *         share_list - another XMesaContext with which to share display
 *                      lists or NULL if no sharing is wanted.
 * Return:  an XMesaContext or NULL if error.
 */
extern XMesaContext XMesaCreateContext( XMesaVisual v,
					XMesaContext share_list
#if defined(GLX_DIRECT_RENDERING) && !defined(XFree86Server)
					, __DRIcontextPrivate *driContextPriv
#endif
				      );


/*
 * Destroy a rendering context as returned by XMesaCreateContext()
 */
extern void XMesaDestroyContext( XMesaContext c );


/*
 * Create an XMesaBuffer from an X window.
 */
extern XMesaBuffer XMesaCreateWindowBuffer( XMesaVisual v,
					    XMesaWindow w
#if defined(GLX_DIRECT_RENDERING) && !defined(XFree86Server)
					    , __DRIdrawablePrivate *driDrawPriv
#endif
					  );


/*
 * Create an XMesaBuffer from an X pixmap.
 */
extern XMesaBuffer XMesaCreatePixmapBuffer( XMesaVisual v,
					    XMesaPixmap p,
					    XMesaColormap cmap
#if defined(GLX_DIRECT_RENDERING) && !defined(XFree86Server)
					    , __DRIdrawablePrivate *driDrawPriv
#endif
					  );


/*
 * Destroy an XMesaBuffer, but not the corresponding window or pixmap.
 */
extern void XMesaDestroyBuffer( XMesaBuffer b );


/*
 * Return the XMesaBuffer handle which corresponds to an X drawable, if any.
 *
 * New in Mesa 2.3.
 */
extern XMesaBuffer XMesaFindBuffer( XMesaDisplay *dpy,
				    XMesaDrawable d );



/*
 * Bind a buffer to a context and make the context the current one.
 */
extern GLboolean XMesaMakeCurrent( XMesaContext c,
				   XMesaBuffer b );


/*
 * Bind two buffers (read and draw) to a context and make the
 * context the current one.
 * New in Mesa 3.3
 */
extern GLboolean XMesaMakeCurrent2( XMesaContext c,
                                    XMesaBuffer drawBuffer,
                                    XMesaBuffer readBuffer );


/*
 * Return a handle to the current context.
 */
extern XMesaContext XMesaGetCurrentContext( void );


/*
 * Return handle to the current (draw) buffer.
 */
extern XMesaBuffer XMesaGetCurrentBuffer( void );


/*
 * Return handle to the current read buffer.
 * New in Mesa 3.3
 */
extern XMesaBuffer XMesaGetCurrentReadBuffer( void );


/*
 * Swap the front and back buffers for the given buffer.  No action is
 * taken if the buffer is not double buffered.
 */
extern void XMesaSwapBuffers( XMesaBuffer b );


/*
 * Copy a sub-region of the back buffer to the front buffer.
 *
 * New in Mesa 2.6
 */
extern void XMesaCopySubBuffer( XMesaBuffer b,
				int x,
				int y,
				int width,
				int height );


/*
 * Return a pointer to the the Pixmap or XImage being used as the back
 * color buffer of an XMesaBuffer.  This function is a way to get "under
 * the hood" of X/Mesa so one can manipulate the back buffer directly.
 * Input:  b - the XMesaBuffer
 * Output:  pixmap - pointer to back buffer's Pixmap, or 0
 *          ximage - pointer to back buffer's XImage, or NULL
 * Return:  GL_TRUE = context is double buffered
 *          GL_FALSE = context is single buffered
 */
extern GLboolean XMesaGetBackBuffer( XMesaBuffer b,
				     XMesaPixmap *pixmap,
				     XMesaImage **ximage );



/*
 * Return the depth buffer associated with an XMesaBuffer.
 * Input:  b - the XMesa buffer handle
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 *
 * New in Mesa 2.4.
 */
extern GLboolean XMesaGetDepthBuffer( XMesaBuffer b,
				      GLint *width,
				      GLint *height,
				      GLint *bytesPerValue,
				      void **buffer );



/*
 * Flush/sync a context
 */
extern void XMesaFlush( XMesaContext c );



/*
 * Get an X/Mesa-specific string.
 * Input:  name - either XMESA_VERSION or XMESA_EXTENSIONS
 */
extern const char *XMesaGetString( XMesaContext c, int name );



/*
 * Scan for XMesaBuffers whose window/pixmap has been destroyed, then free
 * any memory used by that buffer.
 *
 * New in Mesa 2.3.
 */
extern void XMesaGarbageCollect( void );



/*
 * Return a dithered pixel value.
 * Input:  c - XMesaContext
 *         x, y - window coordinate
 *         red, green, blue, alpha - color components in [0,1]
 * Return:  pixel value
 *
 * New in Mesa 2.3.
 */
extern unsigned long XMesaDitherColor( XMesaContext xmesa,
				       GLint x,
				       GLint y,
				       GLfloat red,
				       GLfloat green,
				       GLfloat blue,
				       GLfloat alpha );



/*
 * 3Dfx Glide driver only!
 * Set 3Dfx/Glide full-screen or window rendering mode.
 * Input:  mode - either XMESA_FX_WINDOW (window rendering mode) or
 *                XMESA_FX_FULLSCREEN (full-screen rendering mode)
 * Return:  GL_TRUE if success
 *          GL_FALSE if invalid mode or if not using 3Dfx driver
 *
 * New in Mesa 2.6.
 */
extern GLboolean XMesaSetFXmode( GLint mode );



#ifdef __cplusplus
}
#endif


#endif
