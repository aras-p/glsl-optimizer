/* $Id: glxapi.c,v 1.3 1999/09/16 15:54:21 brianp Exp $ */

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



#ifdef GLX_EXT_get_proc_address
void (*glXGetProcAddressEXT( const GLubyte *procName ))()
{
#ifdef REALGLX
   return NULL;
#else
   return Fake_glXGetProcAddress( procName );
#endif
}
#endif
