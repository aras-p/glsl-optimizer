/* $Id: realglx.h,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

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





#ifndef REALGLX_H
#define REALGLX_H


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "GL/glx.h"



extern XVisualInfo *Real_glXChooseVisual( Display *dpy,
                                          int screen, int *list );


extern int Real_glXGetConfig( Display *dpy, XVisualInfo *visinfo,
                              int attrib, int *value );


extern GLXContext Real_glXCreateContext( Display *dpy, XVisualInfo *visinfo,
                                         GLXContext shareList, Bool direct );


extern void Real_glXDestroyContext( Display *dpy, GLXContext ctx );


extern void Real_glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
                                 GLuint mask );


extern Bool Real_glXMakeCurrent( Display *dpy, GLXDrawable drawable,
                                 GLXContext ctx );


extern GLXContext Real_glXGetCurrentContext( void );


extern GLXDrawable Real_glXGetCurrentDrawable( void );


extern GLXPixmap Real_glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo,
                                          Pixmap pixmap );


extern void Real_glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );


extern Bool Real_glXQueryExtension( Display *dpy, int *errorb, int *event );


extern Bool Real_glXIsDirect( Display *dpy, GLXContext ctx );


extern void Real_glXSwapBuffers( Display *dpy, GLXDrawable drawable );


extern Bool Real_glXQueryVersion( Display *dpy, int *maj, int *min );


extern void Real_glXUseXFont( Font font, int first, int count, int listBase );


extern void Real_glXWaitGL( void );


extern void Real_glXWaitX( void );


/* GLX 1.1 and later */
extern const char *Real_glXQueryExtensionsString( Display *dpy, int screen );


/* GLX 1.1 and later */
extern const char *Real_glXQueryServerString( Display *dpy, int screen,
                                              int name );


/* GLX 1.1 and later */
extern const char *Real_glXGetClientString( Display *dpy, int name );


#endif
