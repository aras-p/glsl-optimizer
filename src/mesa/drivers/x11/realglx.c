/* $Id: realglx.c,v 1.2 1999/11/22 21:52:23 brianp Exp $ */

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
 * Real GLX-encoder functions.  Called from glxapi.c
 *
 * Steven Parker's code for the GLX client API functions should be
 * put in this file.
 *
 * Also, the main API functions in api.c should somehow hook into the
 * GLX-encoding functions...
 */



#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "realglx.h"



XVisualInfo *Real_glXChooseVisual( Display *dpy, int screen, int *list )
{
   (void) dpy;
   (void) screen;
   (void) list;
   return 0;
}



int Real_glXGetConfig( Display *dpy, XVisualInfo *visinfo,
                       int attrib, int *value )
{
   (void) dpy;
   (void) visinfo;
   (void) attrib;
   (void) value;
   return 0;
}



GLXContext Real_glXCreateContext( Display *dpy, XVisualInfo *visinfo,
                                  GLXContext shareList, Bool direct )
{
   (void) dpy;
   (void) visinfo;
   (void) shareList;
   (void) direct;
   return 0;
}



void Real_glXDestroyContext( Display *dpy, GLXContext ctx )
{
   (void) dpy;
   (void) ctx;
}



void Real_glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
                          GLuint mask )
{
   (void) dpy;
   (void) src;
   (void) dst;
   (void) mask;
}



Bool Real_glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx )
{
   (void) dpy;
   (void) drawable;
   (void) ctx;
   return 0;
}



GLXContext Real_glXGetCurrentContext( void )
{
   return 0;
}



GLXDrawable Real_glXGetCurrentDrawable( void )
{
   return 0;
}



GLXPixmap Real_glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo,
				   Pixmap pixmap )
{
   (void) dpy;
   (void) visinfo;
   (void) pixmap;
   return 0;
}


void Real_glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap )
{
   (void) dpy;
   (void) pixmap;
}



Bool Real_glXQueryExtension( Display *dpy, int *errorb, int *event )
{
   (void) dpy;
   (void) errorb;
   (void) event;
   return 0;
}



Bool Real_glXIsDirect( Display *dpy, GLXContext ctx )
{
   (void) dpy;
   (void) ctx;
   return 0;
}



void Real_glXSwapBuffers( Display *dpy, GLXDrawable drawable )
{
   (void) dpy;
   (void) drawable;
}



Bool Real_glXQueryVersion( Display *dpy, int *maj, int *min )
{
   (void) dpy;
   (void) maj;
   (void) min;
   return 0;
}



void Real_glXUseXFont( Font font, int first, int count, int listBase )
{
   (void) font;
   (void) first;
   (void) count;
   (void) listBase;
}


typedef struct {
   struct {
      int major_opcode;
   } codes;



} XExtDisplayInfo;


void Real_glXWaitGL( void )
{
}



void Real_glXWaitX( void )
{
}



/* GLX 1.1 and later */
const char *Real_glXQueryExtensionsString( Display *dpy, int screen )
{
   (void) dpy;
   (void) screen;
   return 0;
}



/* GLX 1.1 and later */
const char *Real_glXQueryServerString( Display *dpy, int screen, int name )
{
   (void) dpy;
   (void) screen;
   (void) name;
   return 0;
}



/* GLX 1.1 and later */
const char *Real_glXGetClientString( Display *dpy, int name )
{
   (void) dpy;
   (void) name;
   return 0;
}



/* GLX 1.2 and later */
Display *Real_glXGetCurrentDisplay( void )
{
   return 0;
}


