/*
 * Mesa 3-D graphics library
 * Version:  4.1
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * DOS/DJGPP glut driver v1.2 for Mesa 4.1
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdio.h>
#include <GL/gl.h>
#include "GL/glut.h"
#include "internal.h"

#define USE_MINI_GLX 1
#if USE_MINI_GLX
#include "GL/miniglx.h"
#else
#include <GL/glx.h>
#endif



static GLXContext context = 0;
static Window win;
static XVisualInfo *visinfo = 0;
static Display *dpy = 0;


int APIENTRY glutCreateWindow (const char *title)
{
   XSetWindowAttributes attr;
   unsigned long mask;
   GLXContext ctx;
   int scrnum = 0;
   Window root = RootWindow( dpy, scrnum );

   if (win)
      return 0;

   if (!dpy) {
      dpy = XOpenDisplay(NULL);
      if (!dpy) {
	 printf("Error: XOpenDisplay failed\n");
	 exit(1);
      }
   }

   if (!visinfo) {
      int attrib[] = {GLX_RGBA,
		      GLX_RED_SIZE, 1,
		      GLX_GREEN_SIZE, 1,
		      GLX_BLUE_SIZE, 1,
		      GLX_DEPTH_SIZE, 1,
		      GLX_DOUBLEBUFFER, 
		      None };

    
      visinfo = glXChooseVisual( dpy, scrnum, attrib );
      if (!visinfo) {
	 printf("Error: couldn't get an RGB, Double-buffered visual\n");
	 exit(1);
      }
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, g_xpos, g_ypos, g_width, g_height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );
   if (!win) {
      printf("Error: XCreateWindow failed\n");
      exit(1);
   }

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   if (!glXMakeCurrent( dpy, win, ctx )) {
      printf("Error: glXMakeCurrent failed\n");
      exit(1);
   }

   if (!(g_display_mode & GLUT_DOUBLE))
      glDrawBuffer( GL_FRONT );
      

   XMapWindow( dpy, win );

#if !USE_MINI_GLX
   {
      XEvent e;
      while (1) {
	 XNextEvent( dpy, &e );
	 if (e.type == MapNotify && e.xmap.window == win) {
	    break;
	 }
      }
   }
#endif

   return 1;
}


int APIENTRY glutCreateSubWindow (int win, int x, int y, int width, int height)
{
   return GL_FALSE;
}


void APIENTRY glutDestroyWindow (int idx)
{
   if (dpy && win)
      XDestroyWindow( dpy, win );

   if (dpy) 
      XCloseDisplay( dpy );

   win = 0;
   dpy = 0;
}


void APIENTRY glutPostRedisplay (void)
{
 g_redisplay = GL_TRUE;
}


void APIENTRY glutSwapBuffers (void)
{
/*  if (g_mouse) pc_scare_mouse(); */
   if (dpy && win) glXSwapBuffers( dpy, win );
/*  if (g_mouse) pc_unscare_mouse(); */
}


int APIENTRY glutGetWindow (void)
{
   return 0;
}


void APIENTRY glutSetWindow (int win)
{
}


void APIENTRY glutSetWindowTitle (const char *title)
{
}


void APIENTRY glutSetIconTitle (const char *title)
{
}


void APIENTRY glutPositionWindow (int x, int y)
{
}


void APIENTRY glutReshapeWindow (int width, int height)
{
}


void APIENTRY glutPopWindow (void)
{
}


void APIENTRY glutPushWindow (void)
{
}


void APIENTRY glutIconifyWindow (void)
{
}


void APIENTRY glutShowWindow (void)
{
}


void APIENTRY glutHideWindow (void)
{
}

void APIENTRY glutMainLoop (void)
{
   GLboolean idle;
   GLboolean have_event;
   XEvent evt;
   int visible = 0;

   glutPostRedisplay();
   if (reshape_func) reshape_func(g_width, g_height);

   while (GL_TRUE) {
      idle = GL_TRUE;


      if (visible && idle_func) 
	 have_event = XCheckMaskEvent( dpy, ~0, &evt );
      else 
	 have_event = XNextEvent( dpy, &evt );

      if (have_event) {
	 idle = GL_FALSE;
	 switch(evt.type) {
	 case MapNotify:
	    if (visibility_func) {
	       visibility_func(GLUT_VISIBLE);
	    }
	    visible = 1;
	    break;
	 case UnmapNotify:
	    if (visibility_func) {
	       visibility_func(GLUT_NOT_VISIBLE);
	    }
	    visible = 0;
	    break;
	 case Expose:
	    g_redisplay = 1;
	    break;
	 }
      }

      if (visible && g_redisplay && display_func) {
	 idle        = GL_FALSE;
	 g_redisplay = GL_FALSE;

	 display_func();
      }

      if (visible && idle && idle_func) {
	 idle_func();
      }
   }
}
