/* $Id: glx.h,v 1.12 2000/02/25 12:35:57 joukj Exp $ */

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



#ifndef GLX_H
#define GLX_H


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "GL/gl.h"
#ifdef MESA
#include "GL/xmesa.h"
#endif


#if defined(USE_MGL_NAMESPACE)
#include "glx_mangle.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


#define GLX_VERSION_1_1		1
#define GLX_VERSION_1_2		1
#define GLX_VERSION_1_3		1

#define GLX_EXTENSION_NAME   "GLX"



/*
 * Tokens for glXChooseVisual and glXGetConfig:
 */
#define GLX_USE_GL		1
#define GLX_BUFFER_SIZE		2
#define GLX_LEVEL		3
#define GLX_RGBA		4
#define GLX_DOUBLEBUFFER	5
#define GLX_STEREO		6
#define GLX_AUX_BUFFERS		7
#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13
#define GLX_ACCUM_RED_SIZE	14
#define GLX_ACCUM_GREEN_SIZE	15
#define GLX_ACCUM_BLUE_SIZE	16
#define GLX_ACCUM_ALPHA_SIZE	17


/*
 * Error codes returned by glXGetConfig:
 */
#define GLX_BAD_SCREEN		1
#define GLX_BAD_ATTRIBUTE	2
#define GLX_NO_EXTENSION	3
#define GLX_BAD_VISUAL		4
#define GLX_BAD_CONTEXT		5
#define GLX_BAD_VALUE       	6
#define GLX_BAD_ENUM		7


/*
 * GLX 1.1 and later:
 */
#define GLX_VENDOR		1
#define GLX_VERSION		2
#define GLX_EXTENSIONS 		3


/*
 * GLX 1.3 and later:
 * XXX don't know the values of some of these enums!
 * XXX some 1.3 enums may be missing!
 */
#define GLX_CONFIG_CAVEAT		?
#define GLX_DONT_CARE			?
#define GLX_SLOW_CONFIG			?
#define GLX_NON_CONFORMANT_CONFIG	?
#define GLX_X_VISUAL_TYPE		0x22
#define GLX_TRANSPARENT_TYPE		0x23
#define GLX_TRANSPARENT_INDEX_VALUE	0x24
#define GLX_TRANSPARENT_RED_VALUE	0x25
#define GLX_TRANSPARENT_GREEN_VALUE	0x26
#define GLX_TRANSPARENT_BLUE_VALUE	0x27
#define GLX_TRANSPARENT_ALPHA_VALUE	0x28
#define GLX_MAX_PBUFFER_WIDTH		?
#define GLX_MAX_PBUFFER_HEIGHT		?
#define GLX_MAX_PBUFFER_PIXELS		?
#define GLX_PRESERVED_CONTENTS		?
#define GLX_LARGEST_BUFFER		?
#define GLX_DRAWABLE_TYPE		?
#define GLX_FBCONFIG_ID			?
#define GLX_VISUAL_ID			?
#define GLX_WINDOW_BIT			?
#define GLX_PIXMAP_BIT			?
#define GLX_PBUFFER_BIT			?
#define GLX_AUX_BUFFERS_BIT		?
#define GLX_FRONT_LEFT_BUFFER_BIT	?
#define GLX_FRONT_RIGHT_BUFFER_BIT	?
#define GLX_BACK_LEFT_BUFFER_BIT	?
#define GLX_BACK_RIGHT_BUFFER_BIT	?
#define GLX_AUX_BUFFERS_BIT		?
#define GLX_DEPTH_BUFFER_BIT		?
#define GLX_STENCIL_BUFFER_BIT		?
#define GLX_ACCUM_BUFFER_BIT		?
#define GLX_RENDER_TYPE			?
#define GLX_DRAWABLE_TYPE		?
#define GLX_X_RENDERABLE		?
#define GLX_NONE			0x8000
#define GLX_TRUE_COLOR			0x8002
#define GLX_DIRECT_COLOR		0x8003
#define GLX_PSEUDO_COLOR		0x8004
#define GLX_STATIC_COLOR		0x8005
#define GLX_GRAY_SCALE			0x8006
#define GLX_STATIC_GRAY			0x8007
#define GLX_TRANSPARENT_INDEX		0x8009
#define GLX_COLOR_INDEX_TYPE		?
#define GLX_COLOR_INDEX_BIT		?
#define GLX_SCREEN			?
#define GLX_PBUFFER_CLOBBER_MASK	?
#define GLX_DAMAGED			?
#define GLX_SAVED			?
#define GLX_WINDOW			?
#define GLX_PBUFFER			?


/*
 * GLX_EXT_visual_info extension
 */
#define GLX_X_VISUAL_TYPE_EXT		0x22
#define GLX_TRANSPARENT_TYPE_EXT	0x23
#define GLX_TRANSPARENT_INDEX_VALUE_EXT	0x24
#define GLX_TRANSPARENT_RED_VALUE_EXT	0x25
#define GLX_TRANSPARENT_GREEN_VALUE_EXT	0x26
#define GLX_TRANSPARENT_BLUE_VALUE_EXT	0x27
#define GLX_TRANSPARENT_ALPHA_VALUE_EXT	0x28


/*
 * GLX_visual_info extension
 */
#define GLX_TRUE_COLOR_EXT		0x8002
#define GLX_DIRECT_COLOR_EXT		0x8003
#define GLX_PSEUDO_COLOR_EXT		0x8004
#define GLX_STATIC_COLOR_EXT		0x8005
#define GLX_GRAY_SCALE_EXT		0x8006
#define GLX_STATIC_GRAY_EXT		0x8007
#define GLX_NONE_EXT			0x8000
#define GLX_TRANSPARENT_RGB_EXT		0x8008
#define GLX_TRANSPARENT_INDEX_EXT	0x8009


/*
 * Compile-time extension tests
 */
#define GLX_EXT_visual_info		1
#define GLX_MESA_pixmap_colormap	1
#define GLX_MESA_release_buffers	1
#define GLX_MESA_copy_sub_buffer	1
#define GLX_MESA_set_3dfx_mode		1
#define GLX_SGI_video_sync		1
#define GLX_ARB_get_proc_address	1



#ifdef MESA
   typedef XMesaContext GLXContext;
   typedef Pixmap GLXPixmap;
   typedef Drawable GLXDrawable;
#else
   typedef void * GLXContext;
   typedef XID GLXPixmap;
   typedef XID GLXDrawable;
   /* GLX 1.3 and later */
   typedef XID GLXFBConfigID;
   typedef XID GLXPfuffer;
   typedef XID GLXWindow;
   typedef XID GLXPbuffer;
   typedef XID GLXFBConfig;
#endif
typedef XID GLXContextID;



extern XVisualInfo* glXChooseVisual( Display *dpy, int screen,
				     int *attribList );

extern GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis,
				    GLXContext shareList, Bool direct );

extern void glXDestroyContext( Display *dpy, GLXContext ctx );

extern Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable,
			    GLXContext ctx);

extern void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
			    GLuint mask );

extern void glXSwapBuffers( Display *dpy, GLXDrawable drawable );

extern GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual,
				     Pixmap pixmap );

extern void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );

extern Bool glXQueryExtension( Display *dpy, int *errorb, int *event );

extern Bool glXQueryVersion( Display *dpy, int *maj, int *min );

extern Bool glXIsDirect( Display *dpy, GLXContext ctx );

extern int glXGetConfig( Display *dpy, XVisualInfo *visual,
			 int attrib, int *value );

extern GLXContext glXGetCurrentContext( void );

extern GLXDrawable glXGetCurrentDrawable( void );

extern void glXWaitGL( void );

extern void glXWaitX( void );

extern void glXUseXFont( Font font, int first, int count, int list );



/* GLX 1.1 and later */
extern const char *glXQueryExtensionsString( Display *dpy, int screen );

extern const char *glXQueryServerString( Display *dpy, int screen, int name );

extern const char *glXGetClientString( Display *dpy, int name );


/* GLX 1.2 and later */
extern Display *glXGetCurrentDisplay( void );


/* GLX 1.3 and later */
extern GLXFBConfig glXChooseFBConfig( Display *dpy, int screen,
                                      const int *attribList, int *nitems );

extern int glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config,
                                 int attribute, int *value );

extern XVisualInfo *glXGetVisualFromFBConfig( Display *dpy,
                                              GLXFBConfig config );

extern GLXWindow glXCreateWindow( Display *dpy, GLXFBConfig config,
                                  Window win, const int *attribList );

extern void glXDestroyWindow( Display *dpy, GLXWindow window );

extern GLXPixmap glXCreatePixmap( Display *dpy, GLXFBConfig config,
                                  Pixmap pixmap, const int *attribList );

extern void glXDestroyPixmap( Display *dpy, GLXPixmap pixmap );

extern GLXPbuffer glXCreatePbuffer( Display *dpy, GLXFBConfig config,
                                    const int *attribList );

extern void glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf );

extern void glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute,
                              unsigned int *value );

extern GLXContext glXCreateNewContext( Display *dpy, GLXFBConfig config,
                                       int renderType, GLXContext shareList,
                                       Bool direct );

extern Bool glXMakeContextCurrent( Display *dpy, GLXDrawable draw,
                                   GLXDrawable read, GLXContext ctx );

extern GLXDrawable glXGetCurrentReadDrawable( void );

extern int glXQueryContext( Display *dpy, GLXContext ctx, int attribute,
                            int *value );

extern void glXSelectEvent( Display *dpy, GLXDrawable drawable,
                            unsigned long mask );

extern void glXGetSelectedEvent( Display *dpy, GLXDrawable drawable,
                                 unsigned long *mask );



/* GLX_MESA_pixmap_colormap */
extern GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visual,
                                         Pixmap pixmap, Colormap cmap );


/* GLX_MESA_release_buffers */
extern Bool glXReleaseBuffersMESA( Display *dpy, GLXDrawable d );


/* GLX_MESA_copy_sub_buffer */
extern void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                                  int x, int y, int width, int height );


/* GLX_MESA_set_3dfx_mode */
extern GLboolean glXSet3DfxModeMESA( GLint mode );


/* GLX_SGI_video_sync */
extern int glXGetVideoSyncSGI(unsigned int *count);
extern int glXWaitVideoSyncSGI(int divisor, int remainder,
                               unsigned int *count);


/* GLX_ARB_get_proc_address */
extern void (*glXGetProcAddressARB(const GLubyte *procName))();



#ifdef __cplusplus
}
#endif

#endif
