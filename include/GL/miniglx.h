/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


/**
 * \file miniglx.h
 * \brief Mini GLX interface functions.
 * \author Brian Paul
 *
 * See comments miniglx.c for more information.
 */

#ifndef MINIGLX_H
#define MINIGLX_H

#include <GL/gl.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \name Replacement Xlib/GLX types
 */
/*@{*/
/**
 * \brief Boolean type.
 * 
 * It can have the values #True or #False.
 */
#ifndef MINIGLX_NO_XTYPES
typedef int Bool;
#endif
typedef int MINI_Bool;

/**
 * \brief Color map.
 *
 * Alias for private ::MiniGLXColormapRec structure.
 */
typedef struct MiniGLXColormapRec *MINI_Colormap;
#ifndef MINIGLX_NO_XTYPES
typedef struct MiniGLXColormapRec *Colormap;
#endif

/**
 * \brief Window attributes.
 */
typedef struct MiniGLXSetWindowAttributesRec {
   int background_pixel; /**< \brief background pixel */
   int border_pixel;     /**< \brief border pixel value */
   MINI_Colormap colormap;    /**< \brief color map to be associated with window */
   int event_mask;       /**< \brief set of events that should be saved */
} XSetWindowAttributes;

/**
 * \brief Visual.
 *
 * Alias for the private ::MiniGLXVisualRec structure.
 *
 * \sa \ref datatypes.
 */
typedef struct MiniGLXVisualRec Visual;

/**
 * \brief Visual information.
 *
 * \sa \ref datatypes.
 */
#ifndef MINIGLX_NO_XTYPES
typedef unsigned long VisualID;
#endif
typedef unsigned long MINI_VisualID;
typedef struct MiniGLXXVisualInfoRec {
   Visual *visual;	/**< \brief pointer to the GLX Visual */
   MINI_VisualID visualid;	/**< \brief visual ID */
   int screen;		/**< \brief screen number */
   int depth;		/**< \brief bit depth */
#if defined(__cplusplus) || defined(c_plusplus)
   int c_class;		/**< \brief class */
#else
   int class;		/**< \brief class */
#endif
   int bits_per_rgb;	/**< \brief total bits per pixel */
} XVisualInfo;

/**
 * \brief GLX Frame Buffer Configuration (for pbuffers)
 * \sa \ref datatypes.
 */
typedef struct MiniGLXFBConfigRec {
   XVisualInfo *visInfo;
} GLXFBConfig;


/**
 * \brief Display handle.
 *
 * Alias for the private ::MiniGLXDisplayRec structure.
 *
 * \sa \ref datatypes.
 */
#ifndef MINIGLX_NO_XTYPES
typedef struct MiniGLXDisplayRec   Display;
#endif
typedef struct MiniGLXDisplayRec   MINI_Display;

/**
 * \brief Window handle.
 *
 * Alias for the private ::MiniGLXWindowRec structure.
 *
 * \sa \ref datatypes.
 */
#ifndef MINIGLX_NO_XTYPES
typedef struct MiniGLXWindowRec   *Window;
#endif
typedef struct MiniGLXWindowRec   *MINI_Window;

/**
 * \brief Drawable.
 *
 * Alias for the private ::MiniGLXWindowRec structure.
 * 
 * For Mini GLX only the full-screen window can be used as source and
 * destination in graphics operations.
 *
 * \sa \ref datatypes.
 */
#ifndef MINIGLX_NO_XTYPES
typedef struct MiniGLXWindowRec   *Drawable;
#endif
typedef struct MiniGLXWindowRec   *MINI_Drawable;

/**
 * \brief GLX drawable.
 *
 * Alias for the private ::MiniGLXWindowRec structure.
 * 
 * Same as #Drawable.
 *
 * \sa \ref datatypes.
 */
typedef struct MiniGLXWindowRec   *GLXDrawable;

/**
 * \brief GLX pbuffer.
 *
 * Alias for the private ::MiniGLXWindowRec structure.
 * 
 * Same as #Drawable.
 *
 * \sa \ref datatypes.
 */
typedef struct MiniGLXWindowRec   *GLXPbuffer;

/**
 * \brief GLX context.
 * 
 * Alias for the private ::MiniGLXContext structure.
 *
 * \sa \ref datatypes.
 */
typedef struct MiniGLXContextRec  *GLXContext;
/*@}*/


typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window window;
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
} XExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window parent;		/* parent of the window */
        MINI_Window window;		/* window id of window created */
	int x, y;		/* window location */
	int width, height;	/* size of window */
	int border_width;	/* border width */
	MINI_Bool override_redirect;	/* creation should be overridden */
} XCreateWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window event;
	MINI_Window window;
} XDestroyWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window event;
	MINI_Window window;
	MINI_Bool from_configure;
} XUnmapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window event;
	MINI_Window window;
	MINI_Bool override_redirect;	/* boolean, is override set... */
} XMapEvent;


typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	MINI_Bool send_event;	/* true if this came from a SendEvent request */
	MINI_Display *display;	/* Display the event was read from */
	MINI_Window parent;
	MINI_Window window;
} XMapRequestEvent;

typedef union _XEvent {
        int type;		/* must not be changed; first element */
	XExposeEvent xexpose;
        XCreateWindowEvent xcreatewindow;
	XDestroyWindowEvent xdestroywindow;
	XUnmapEvent xunmap;
	XMapEvent xmap;
	XMapRequestEvent xmaprequest;
	long pad[24];
} XEvent;


/**
 * \name Xlib constants
 */
/*@{*/
#define False                0
#define True                 1
#define None                 0
#define AllocNone            0
#define InputOutput          1
#define ExposureMask         (1L<<15) 
#define StructureNotifyMask  (1L<<17) 
#define CWBackPixel          (1L<<1)
#define CWBorderPixel        (1L<<3)
#define CWEventMask          (1L<<11)
#define CWColormap           (1L<<13)
#define PseudoColor          3
#define TrueColor            4
#define VisualIDMask     0x1
#define VisualScreenMask     0x2
#define Expose		     12
#define CreateNotify	     16
#define DestroyNotify	     17
#define UnmapNotify	     18
#define MapNotify	     19
#define MapRequest	     20

/*@}*/

/**
 * \name Standard GLX tokens
 */
/*@{*/
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
#define GLX_BAD_ATTRIBUTE       1
#define GLX_BAD_VISUAL          4
/*@}*/


/**
 * \name Unique to Mini GLX
 *
 * At compile time, the Mini GLX interface version can be tested with the
 * MINI_GLX_VERSION_1_x preprocessor tokens.
 *
 * \sa glXQueryVersion()
 */
/*@{*/
/** \brief Defined if version 1.0 of Mini GLX is supported. */
#define MINI_GLX_VERSION_1_0    1
/** \brief Defined if version 1.1 of Mini GLX is supported. */
#define MINI_GLX_VERSION_1_1    1
/*@}*/


/**
 * \name Server-specific functions
 */
extern MINI_Display *
__miniglx_StartServer( const char *display_name );

extern int
__miniglx_Select( MINI_Display *dpy, int maxfd,
		  fd_set *rfds, fd_set *wfds, fd_set *xfds,
		  struct timeval *tv );


/**
 * \name Simulated Xlib functions
 */
/*@{*/
extern MINI_Display *
XOpenDisplay( const char *dpy_name );


extern void
XCloseDisplay( MINI_Display *display );

extern MINI_Window
XCreateWindow( MINI_Display *display, MINI_Window parent, int x, int y,
               unsigned int width, unsigned int height,
               unsigned int border_width, int depth, unsigned int winclass,
               Visual *visual, unsigned long valuemask,
               XSetWindowAttributes *attributes );

extern int 
XNextEvent(MINI_Display *display, XEvent *event_return);

extern MINI_Bool 
XCheckMaskEvent( MINI_Display *dpy, long event_mask, XEvent *event_return );

/**
 * \brief Return the root window.
 * 
 * \param display the display handle. It is ignored by Mini GLX, but should be
 * the value returned by XOpenDisplay().
 * \param screen the screen number on the host server. It is ignored by Mini
 * GLX but should be zero.
 *
 * \return the root window. Always zero on Mini GLX.
 */
#define RootWindow(display, screen) 0
#define DefaultScreen(dpy)  0

extern void
XDestroyWindow( MINI_Display *display, MINI_Window w );

extern void
XMapWindow( MINI_Display *display, MINI_Window w );

/* Should clients have access to this?
 */
extern void
XUnmapWindow( MINI_Display *display, MINI_Window w );

extern MINI_Colormap
XCreateColormap( MINI_Display *display, MINI_Window w, Visual *visual, int alloc );

extern void
XFreeColormap( MINI_Display *display, MINI_Colormap cmap );

extern void
XFree( void *data );

extern XVisualInfo *
XGetVisualInfo( MINI_Display *display, long vinfo_mask,
                XVisualInfo *vinfo_template, int *nitems_return );
/*@}*/



/**
 * \name GLX functions
 */
/*@{*/
extern XVisualInfo*
glXChooseVisual( MINI_Display *dpy, int screen, int *attribList );

extern int
glXGetConfig( MINI_Display *dpy, XVisualInfo *vis, int attrib, int *value );

extern GLXContext
glXCreateContext( MINI_Display *dpy, XVisualInfo *vis,
                  GLXContext shareList, MINI_Bool direct );

extern void
glXDestroyContext( MINI_Display *dpy, GLXContext ctx );

extern MINI_Bool
glXMakeCurrent( MINI_Display *dpy, GLXDrawable drawable, GLXContext ctx);

extern void
glXSwapBuffers( MINI_Display *dpy, GLXDrawable drawable );

extern GLXContext
glXGetCurrentContext( void );

extern GLXDrawable
glXGetCurrentDrawable( void );

extern void 
(*glXGetProcAddress(const GLubyte *procname))( void );

extern MINI_Bool
glXQueryVersion( MINI_Display *dpy, int *major, int *minor );

/* Added in MiniGLX 1.1 */
extern GLXPbuffer
glXCreatePbuffer( MINI_Display *dpy, GLXFBConfig config, const int *attribList );

extern void
glXDestroyPbuffer( MINI_Display *dpy, GLXPbuffer pbuf );

extern GLXFBConfig *
glXChooseFBConfig( MINI_Display *dpy, int screen, const int *attribList,
                   int *nitems );

extern XVisualInfo *
glXGetVisualFromFBConfig( MINI_Display *dpy, GLXFBConfig config );

extern void *glXAllocateMemoryMESA(Display *dpy, int scrn,
				   size_t size, float readFreq,
				   float writeFreq, float priority);

extern void glXFreeMemoryMESA(Display *dpy, int scrn, void *pointer);

extern GLuint glXGetMemoryOffsetMESA( Display *dpy, int scrn,
				      const void *pointer );
/*@}*/

extern void
__glXScrEnableExtension( void *, const char * name );

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* MINIGLX_H */
