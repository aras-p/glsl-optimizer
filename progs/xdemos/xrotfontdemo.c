/*
 * Mesa 3-D graphics library
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
 * Example of using glXUseRotatedXFontMESA().
 * 24 Jan 2004
 * Brian Paul
 */


#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xuserotfont.h"


static const char *ProgramName = "xfont";

static const char *FontName = "fixed";

static GLuint FontBase[4];


static void redraw( Display *dpy, Window w )
{
   static const char *text = "  Rotated bitmap text";
   int i;

   glClear( GL_COLOR_BUFFER_BIT );

   /* triangle */
   glColor3f( 0.2, 0.2, 1.0 );
   glBegin(GL_TRIANGLES);
   glVertex2f( -0.8,  0.7 );
   glVertex2f( -0.8, -0.7 );
   glVertex2f(  0.8,  0.0 );
   glEnd();

   /* marker */
   glColor3f( 0, 1, 0 );
   glBegin(GL_POINTS);
   glVertex2f(0, 0);
   glEnd();

   /* text */
   glColor3f( 1, 1, 1 );

   for (i = 0; i < 4; i++) {
      glRasterPos2f(0, 0); 
      glListBase(FontBase[i]);
      glCallLists(strlen(text), GL_UNSIGNED_BYTE, (GLubyte *) text);
   }

   glXSwapBuffers( dpy, w );
}



static void resize( unsigned int width, unsigned int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
}



static void setup_font( Display *dpy )
{
    XFontStruct *fontInfo;
    Font id;
    unsigned int first, last;
    int i;

    fontInfo = XLoadQueryFont(dpy, FontName);
    if (!fontInfo) {
        printf("Error: font %s not found\n", FontName);
	exit(0);
    }

    id = fontInfo->fid;
    first = fontInfo->min_char_or_byte2;
    last = fontInfo->max_char_or_byte2;

    for (i = 0; i < 4; i++) {
       FontBase[i] = glGenLists((GLuint) last + 1);
       if (!FontBase[i]) {
          printf("Error: unable to allocate display lists\n");
          exit(0);
       }
       glXUseRotatedXFontMESA(id, first, last - first + 1, FontBase[i] + first,
                              i * 90);
    }
}


static Window make_rgb_db_window( Display *dpy, int xpos, int ypos,
				  unsigned int width, unsigned int height )
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, ProgramName, ProgramName,
                              None, (char **)NULL, 0, &sizehints);
   }


   ctx = glXCreateContext( dpy, visinfo, NULL, True );

   glXMakeCurrent( dpy, win, ctx );

   return win;
}


static void event_loop( Display *dpy )
{
   XEvent event;

   while (1) {
      XNextEvent( dpy, &event );

      switch (event.type) {
	 case Expose:
	    redraw( dpy, event.xany.window );
	    break;
	 case ConfigureNotify:
	    resize( event.xconfigure.width, event.xconfigure.height );
	    break;
         case KeyPress:
            exit(0);
         default:
            ;  /* no-op */
      }
   }
}



int main( int argc, char *argv[] )
{
   Display *dpy;
   Window win;

   dpy = XOpenDisplay(NULL);

   win = make_rgb_db_window( dpy, 0, 0, 300, 300 );
   setup_font( dpy );

   glShadeModel( GL_FLAT );
   glClearColor( 0.5, 0.5, 1.0, 1.0 );

   XMapWindow( dpy, win );

   event_loop( dpy );
   return 0;
}
