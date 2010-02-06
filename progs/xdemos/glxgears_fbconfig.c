/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * \file glxgears_fbconfig.c
 * Yet-another-version of gears.  Originally ported to GLX by Brian Paul on
 * 23 March 2001.  Modified to use fbconfigs by Ian Romanick on 10 Feb 2004.
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 * \author Brian Paul
 * \author Ian Romanick <idr@us.ibm.com>
 */


#define GLX_GLXEXT_PROTOTYPES

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <assert.h>
#include "pbutil.h"

static PFNGLXCHOOSEFBCONFIGPROC choose_fbconfig = NULL;
static PFNGLXGETVISUALFROMFBCONFIGPROC get_visual_from_fbconfig = NULL;
static PFNGLXCREATENEWCONTEXTPROC create_new_context = NULL;
static PFNGLXCREATEWINDOWPROC create_window = NULL;
static PFNGLXDESTROYWINDOWPROC destroy_window = NULL;

#define BENCHMARK

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static int
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (int) tv.tv_sec;
}

#else /*BENCHMARK*/

/* dummy */
static int
current_time(void)
{
   return 0;
}

#endif /*BENCHMARK*/



#ifndef M_PI
#define M_PI 3.14159265
#endif


static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;


/*
 *
 *  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat angle, da;
   GLfloat u, v, len;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   glShadeModel(GL_FLAT);

   glNormal3f(0.0, 0.0, 1.0);

   /* draw front face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      if (i < teeth) {
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    width * 0.5);
      }
   }
   glEnd();

   /* draw front sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
   }
   glEnd();

   glNormal3f(0.0, 0.0, -1.0);

   /* draw back face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      if (i < teeth) {
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    -width * 0.5);
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      }
   }
   glEnd();

   /* draw back sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
   }
   glEnd();

   /* draw outward faces of teeth */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      u = r2 * cos(angle + da) - r1 * cos(angle);
      v = r2 * sin(angle + da) - r1 * sin(angle);
      len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      glNormal3f(v, -u, 0.0);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
   }

   glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
   glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

   glEnd();

   glShadeModel(GL_SMOOTH);

   /* draw inside radius cylinder */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glNormal3f(-cos(angle), -sin(angle), 0.0);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
   }
   glEnd();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glCallList(gear1);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
   glCallList(gear2);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
   glCallList(gear3);
   glPopMatrix();

   glPopMatrix();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat h = (GLfloat) height / (GLfloat) width;

   glViewport(0, 0, (GLint) width, (GLint) height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);
}


static void
init(void)
{
   static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
   static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   /* make the gears */
   gear1 = glGenLists(1);
   glNewList(gear1, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
   gear(1.0, 4.0, 1.0, 20, 0.7);
   glEndList();

   gear2 = glGenLists(1);
   glNewList(gear2, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
   gear(0.5, 2.0, 2.0, 10, 0.7);
   glEndList();

   gear3 = glGenLists(1);
   glNewList(gear3, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
   gear(1.3, 2.0, 0.5, 10, 0.7);
   glEndList();

   glEnable(GL_NORMALIZE);
}


static GLXWindow
dummy_create_window(Display *dpy, GLXFBConfig config, Window win,
                   const int *attrib_list)
{
   (void) dpy;
   (void) config;
   (void) attrib_list;

   return (GLXWindow) win;
}


static void
dummy_destroy_window(Display *dpy, GLXWindow win)
{
   (void) dpy;
   (void) win;
}


/**
 * Initialize fbconfig related function pointers.
 */
static void
init_fbconfig_functions(Display *dpy, int scrnum)
{
   const char * glx_extensions;
   const char * match;
   static const char ext_name[] = "GLX_SGIX_fbconfig";
   const size_t len = strlen( ext_name );
   int major;
   int minor;
   GLboolean ext_version_supported;
   GLboolean glx_1_3_supported;


   /* Determine if GLX 1.3 or greater is supported.
    */
   glXQueryVersion(dpy, & major, & minor);
   glx_1_3_supported = (major == 1) && (minor >= 3);

   /* Determine if GLX_SGIX_fbconfig is supported.
    */
   glx_extensions = glXQueryExtensionsString(dpy, scrnum);
   match = strstr( glx_extensions, ext_name );

   ext_version_supported = (match != NULL)
       && ((match[len] == '\0') || (match[len] == ' '));

   printf( "GLX 1.3 is %ssupported.\n",
	   (glx_1_3_supported) ? "" : "not " );
   printf( "%s is %ssupported.\n",
	   ext_name, (ext_version_supported) ? "" : "not " );

   if ( glx_1_3_supported ) {
      choose_fbconfig = (PFNGLXCHOOSEFBCONFIGPROC)
	 glXGetProcAddressARB((GLubyte *) "glXChooseFBConfig");
      get_visual_from_fbconfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)
	 glXGetProcAddressARB((GLubyte *) "glXGetVisualFromFBConfig");
      create_new_context = (PFNGLXCREATENEWCONTEXTPROC)
	 glXGetProcAddressARB((GLubyte *) "glXCreateNewContext");
      create_window = (PFNGLXCREATEWINDOWPROC)
	 glXGetProcAddressARB((GLubyte *) "glXCreateWindow");
      destroy_window = (PFNGLXDESTROYWINDOWPROC)
	 glXGetProcAddressARB((GLubyte *) "glXDestroyWindow");
   }
   else if ( ext_version_supported ) {
      choose_fbconfig = (PFNGLXCHOOSEFBCONFIGPROC)
	 glXGetProcAddressARB((GLubyte *) "glXChooseFBConfigSGIX");
      get_visual_from_fbconfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)
	 glXGetProcAddressARB((GLubyte *) "glXGetVisualFromFBConfigSGIX");
      create_new_context = (PFNGLXCREATENEWCONTEXTPROC)
	 glXGetProcAddressARB((GLubyte *) "glXCreateContextWithConfigSGIX");
      create_window = dummy_create_window;
      destroy_window = dummy_destroy_window;
   }
   else {
      printf( "This demo requires either GLX 1.3 or %s be supported.\n",
	      ext_name );
      exit(1);
   }

   if ( choose_fbconfig == NULL ) {
      printf( "glXChooseFBConfig not found!\n" );
      exit(1);
   }

   if ( get_visual_from_fbconfig == NULL ) {
      printf( "glXGetVisualFromFBConfig not found!\n" );
      exit(1);
   }

   if ( create_new_context == NULL ) {
      printf( "glXCreateNewContext not found!\n" );
      exit(1);
   }
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window( Display *dpy, const char *name,
             int x, int y, int width, int height,
             Window *winRet, GLXWindow *glxWinRet, GLXContext *ctxRet)
{
   int attrib[] = { GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		    GLX_RED_SIZE,      1,
		    GLX_GREEN_SIZE,    1,
		    GLX_BLUE_SIZE,     1,
		    GLX_DOUBLEBUFFER,  GL_TRUE,
		    GLX_DEPTH_SIZE,    1,
		    None };
   GLXFBConfig * fbconfig;
   int num_configs;
   int scrnum;
   int i;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXWindow glxWin;
   GLXContext ctx;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   init_fbconfig_functions(dpy, scrnum);
   fbconfig = (*choose_fbconfig)(dpy, scrnum, attrib, & num_configs);
   if (fbconfig == NULL) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   printf("\nThe following fbconfigs meet the requirements.  The first one "
	  "will be used.\n\n");
   for ( i = 0 ; i < num_configs ; i++ ) {
      PrintFBConfigInfo(dpy, scrnum, fbconfig[i], GL_TRUE);
   }

   /* window attributes */
   visinfo = (*get_visual_from_fbconfig)(dpy, fbconfig[0]);
   assert(visinfo != NULL);
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   glxWin = (*create_window)(dpy, fbconfig[0], win, NULL);

   ctx = (*create_new_context)(dpy, fbconfig[0], GLX_RGBA_TYPE, NULL, GL_TRUE);
   if (!ctx) {
      printf("Error: glXCreateNewContext failed\n");
      exit(1);
   }

   XFree(fbconfig);

   *glxWinRet = glxWin;
   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, GLXWindow win)
{
   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         switch (event.type) {
	 case Expose:
            /* we'll redraw below */
	    break;
	 case ConfigureNotify:
	    reshape(event.xconfigure.width, event.xconfigure.height);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
               if (code == XK_Left) {
                  view_roty += 5.0;
               }
               else if (code == XK_Right) {
                  view_roty -= 5.0;
               }
               else if (code == XK_Up) {
                  view_rotx += 5.0;
               }
               else if (code == XK_Down) {
                  view_rotx -= 5.0;
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == 27) {
                     /* escape */
                     return;
                  }
               }
            }
         }
      }

      /* next frame */
      angle += 2.0;

      draw();
      glXSwapBuffers(dpy, win);

      /* calc framerate */
      {
         static int t0 = -1;
         static int frames = 0;
         int t = current_time();

         if (t0 < 0)
            t0 = t;

         frames++;

         if (t - t0 >= 5.0) {
            GLfloat seconds = t - t0;
            GLfloat fps = frames / seconds;
            printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
                   fps);
            t0 = t;
            frames = 0;
         }
      }
   }
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   Window win;
   GLXWindow glxWin;
   GLXContext ctx;
   const char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
   }

   dpy = XOpenDisplay(dpyName);
   if (!dpy) {
      printf("Error: couldn't open display %s\n", XDisplayName(dpyName));
      return -1;
   }

   make_window(dpy, "glxgears", 0, 0, 300, 300, &win, &glxWin, &ctx);
   XMapWindow(dpy, win);
   glXMakeCurrent(dpy, glxWin, ctx);

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   init();

   event_loop(dpy, glxWin);

   glXDestroyContext(dpy, ctx);
   destroy_window(dpy, glxWin);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
