/* $Id: glxapi.c,v 1.6 1999/11/23 19:54:27 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
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





/*
 * GLX API functions which either call fake or real GLX implementations
 *
 * To enable real GLX encoding the REALGLX preprocessor symbol should be
 * defined on the command line.
 */



#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "GL/glx.h"
#include "fakeglx.h"
#include "realglx.h"


#ifdef REALGLX
static Display *CurrentDisplay = NULL;
#endif


/*
 * This functions determines whether a call to a glX*() function should
 * be routed to the "fake" (Mesa) or "real" (GLX-encoder) functions.
 * Input:  dpy - the X display.
 * Return:   GL_TRUE if the given display supports the real GLX extension,
 *           GL_FALSE otherwise.
 */
static GLboolean display_has_glx( Display *dpy )
{
   /* TODO: we should use a lookup table to avoid calling XQueryExtension
    * every time.
    */
   int ignore;
   if (XQueryExtension( dpy, "GLX", &ignore, &ignore, &ignore )) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



XVisualInfo *glXChooseVisual( Display *dpy, int screen, int *list )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXChooseVisual( dpy, screen, list );
   else
#endif
      return Fake_glXChooseVisual( dpy, screen, list );
}



int glXGetConfig( Display *dpy, XVisualInfo *visinfo, int attrib, int *value )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXGetConfig( dpy, visinfo, attrib, value );
   else
#endif
      return Fake_glXGetConfig( dpy, visinfo, attrib, value );
}



GLXContext glXCreateContext( Display *dpy, XVisualInfo *visinfo,
			     GLXContext shareList, Bool direct )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXCreateContext( dpy, visinfo, shareList, direct );
   else
#endif
      return Fake_glXCreateContext( dpy, visinfo, shareList, direct );
}



void glXDestroyContext( Display *dpy, GLXContext ctx )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      Real_glXDestroyContext( dpy, ctx );
   else
#endif
      Fake_glXDestroyContext( dpy, ctx );
}



void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
		     GLuint mask )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      Real_glXCopyContext( dpy, src, dst, mask );
   else
#endif
      Fake_glXCopyContext( dpy, src, dst, mask );
}



Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx )
{
#ifdef REALGLX
   if (display_has_glx(dpy)) {
      if (Real_glXMakeCurrent( dpy, drawable, ctx )) {
         CurrentDisplay = dpy;
         return True;
      }
      else {
         return False;
      }
   }
   else {
      if (Fake_glXMakeCurrent( dpy, drawable, ctx )) {
         CurrentDisplay = dpy;
         return True;
      }
      else {
         return False;
      }
   }
#else
   return Fake_glXMakeCurrent( dpy, drawable, ctx );
#endif
}



GLXContext glXGetCurrentContext( void )
{
#ifdef REALGLX
   if (display_has_glx(CurrentDisplay))
      return Real_glXGetCurrentContext();
   else
#endif
      return Fake_glXGetCurrentContext();
}



GLXDrawable glXGetCurrentDrawable( void )
{
#ifdef REALGLX
   if (display_has_glx(CurrentDisplay))
      return Real_glXGetCurrentDrawable();
   else
#endif
      return Fake_glXGetCurrentDrawable();
}



GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo,
                              Pixmap pixmap )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXCreateGLXPixmap( dpy, visinfo, pixmap );
   else
#endif
      return Fake_glXCreateGLXPixmap( dpy, visinfo, pixmap );
}


void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      Real_glXDestroyGLXPixmap( dpy, pixmap );
   else
#endif
      Fake_glXDestroyGLXPixmap( dpy, pixmap );
}



Bool glXQueryExtension( Display *dpy, int *errorb, int *event )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXQueryExtension( dpy, errorb, event );
   else
#endif
      return Fake_glXQueryExtension( dpy, errorb, event );
}



Bool glXIsDirect( Display *dpy, GLXContext ctx )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXIsDirect( dpy, ctx );
   else
#endif
      return Fake_glXIsDirect( dpy, ctx );
}



void glXSwapBuffers( Display *dpy, GLXDrawable drawable )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      Real_glXSwapBuffers( dpy, drawable );
   else
#endif
      Fake_glXSwapBuffers( dpy, drawable );
}



void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                           int x, int y, int width, int height )
{
#ifdef REALGLX
   /* can't implement! */
   return;
#endif
   Fake_glXCopySubBufferMESA( dpy, drawable, x, y, width, height );
}



Bool glXQueryVersion( Display *dpy, int *maj, int *min )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXQueryVersion( dpy, maj, min );
   else
#endif
      return Fake_glXQueryVersion( dpy, maj, min );
}



void glXUseXFont( Font font, int first, int count, int listBase )
{
#ifdef REALGLX
   if (display_has_glx(CurrentDisplay))
      Real_glXUseXFont( font, first, count, listBase );
   else
#endif
      Fake_glXUseXFont( font, first, count, listBase );
}


void glXWaitGL( void )
{
#ifdef REALGLX
   if (display_has_glx(CurrentDisplay))
      Real_glXWaitGL();
   else
#endif
      Fake_glXWaitGL();
}



void glXWaitX( void )
{
#ifdef REALGLX
   if (display_has_glx(CurrentDisplay))
      Real_glXWaitX();
   else
#endif
      Fake_glXWaitX();
}



/* GLX 1.1 and later */
const char *glXQueryExtensionsString( Display *dpy, int screen )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXQueryExtensionsString( dpy, screen );
   else
#endif
      return Fake_glXQueryExtensionsString( dpy, screen );
}



/* GLX 1.1 and later */
const char *glXQueryServerString( Display *dpy, int screen, int name )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXQueryServerString( dpy, screen, name );
   else
#endif
      return Fake_glXQueryServerString( dpy, screen, name );
}



/* GLX 1.1 and later */
const char *glXGetClientString( Display *dpy, int name )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXGetClientString( dpy, name );
   else
#endif
      return Fake_glXGetClientString( dpy, name );
}



/* GLX 1.2 and later */
Display *glXGetCurrentDisplay( void )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return Real_glXGetCurrentDisplay();
   else
#endif
      return Fake_glXGetCurrentDisplay();
}



/*
 * GLX 1.3 and later
 * XXX these are just no-op stubs for now.
 */
GLXFBConfig glXChooseFBConfig( Display *dpy, int screen,
                               const int *attribList, int *nitems )
{
   (void) dpy;
   (void) screen;
   (void) attribList;
   (void) nitems;
   return 0;
}


int glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config,
                          int attribute, int *value )
{
   (void) dpy;
   (void) config;
   (void) attribute;
   (void) value;
   return 0;
}


XVisualInfo *glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )
{
   (void) dpy;
   (void) config;
   return 0;
}


GLXWindow glXCreateWindow( Display *dpy, GLXFBConfig config, Window win,
                           const int *attribList )
{
   (void) dpy;
   (void) config;
   (void) win;
   (void) attribList;
   return 0;
}


void glXDestroyWindow( Display *dpy, GLXWindow window )
{
   (void) dpy;
   (void) window;
   return;
}


GLXPixmap glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap,
                           const int *attribList )
{
   (void) dpy;
   (void) config;
   (void) pixmap;
   (void) attribList;
   return 0;
}


void glXDestroyPixmap( Display *dpy, GLXPixmap pixmap )
{
   (void) dpy;
   (void) pixmap;
   return;
}


GLXPbuffer glXCreatePbuffer( Display *dpy, GLXFBConfig config,
                             const int *attribList )
{
   (void) dpy;
   (void) config;
   (void) attribList;
   return 0;
}


void glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf )
{
   (void) dpy;
   (void) pbuf;
}


void glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute,
                       unsigned int *value )
{
   (void) dpy;
   (void) draw;
   (void) attribute;
   (void) value;
}


GLXContext glXCreateNewContext( Display *dpy, GLXFBConfig config,
                                int renderType, GLXContext shareList,
                                Bool direct )
{
   (void) dpy;
   (void) config;
   (void) renderType;
   (void) shareList;
   (void) direct;
   return 0;
}


Bool glXMakeContextCurrent( Display *dpy, GLXDrawable draw, GLXDrawable read,
                            GLXContext ctx )
{
   (void) dpy;
   (void) draw;
   (void) read;
   (void) ctx;
   return 0;
}


GLXDrawable glXGetCurrentReadDrawable( void )
{
   return 0;
}


int glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value )
{
   (void) dpy;
   (void) ctx;
   (void) attribute;
   (void) value;
   return 0;
}


void glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask )
{
   (void) dpy;
   (void) drawable;
   (void) mask;
}


void glXGetSelectedEvent( Display *dpy, GLXDrawable drawable,
                          unsigned long *mask )
{
   (void) dpy;
   (void) drawable;
   (void) mask;
}




#ifdef GLX_MESA_release_buffers
Bool glXReleaseBuffersMESA( Display *dpy, Window w )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return GL_FALSE;
   else
#endif
      return Fake_glXReleaseBuffersMESA( dpy, w );
}
#endif


#ifdef GLX_MESA_pixmap_colormap
GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visinfo,
                                  Pixmap pixmap, Colormap cmap )
{
#ifdef REALGLX
   if (display_has_glx(dpy))
      return 0;
   else
#endif
      return Fake_glXCreateGLXPixmapMESA( dpy, visinfo, pixmap, cmap );
}
#endif



#ifdef GLX_SGI_video_sync

/*
 * This function doesn't really do anything.  But, at least one
 * application uses the function so this stub is useful.
 */
int glXGetVideoSyncSGI(unsigned int *count)
{
   static unsigned int counter = 0;
   *count = counter++;
   return 0;
}


/*
 * Again, this is really just a stub function.
 */
int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   static unsigned int counter = 0;
   while (counter % divisor != remainder)
      counter++;
   *count = counter;
   return 0;
}

#endif



#ifdef GLX_MESA_set_3dfx_mode
GLboolean glXSet3DfxModeMESA( GLint mode )
{
#ifdef REALGLX
   return GL_FALSE;
#else
   return Fake_glXSet3DfxModeMESA( mode );
#endif
}
#endif



#if 0  /* spec for this not finalized yet */
void (*glXGetProcAddressEXT( const GLubyte *procName ))()
{
#ifdef REALGLX
   return NULL;
#else
   return Fake_glXGetProcAddress( procName );
#endif
}
#endif
