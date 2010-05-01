/*
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
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
 * Test drawing to two windows.
 * Brian Paul
 * August 2008
 */


#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>


static int WinWidth[2] = {150, 300}, WinHeight[2] = {150, 300};


static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
#ifdef GL_VERSION_ES_CM_1_0
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
#else
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);
#endif
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}


static void
draw(int win)
{
   static const GLfloat verts[3][2] = {
      { -1, -1 },
      {  1, -1 },
      {  0,  1 }
   };
   static const GLfloat colors[3][4] = {
      { 1, 0, 0, 1 },
      { 0, 1, 0, 1 },
      { 0, 0, 1, 1 }
   };

   assert(win == 0 || win == 1);

   reshape(WinWidth[win], WinHeight[win]);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_roty, 0, 1, 0);
   glRotatef(view_rotz, 0, 0, 1);

   /* draw triangle */
   {
      glVertexPointer(2, GL_FLOAT, 0, verts);
      glColorPointer(4, GL_FLOAT, 0, colors);

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_COLOR_ARRAY);

      glDrawArrays(GL_TRIANGLES, 0, 3);

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
   }

   glPopMatrix();
}


static void
init(void)
{
   glClearColor(0.4, 0.4, 0.4, 0.0);
}


/*
 * Create an RGB, double-buffered X window.
 * Return the window and context handles.
 */
static void
make_x_window(Display *x_dpy, EGLDisplay egl_dpy,
              const char *name,
              int x, int y, int width, int height,
              Window *winRet,
              EGLContext *ctxRet,
              EGLSurface *surfRet)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
      EGL_NONE
   };

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visInfo, visTemplate;
   int num_visuals;
   EGLContext ctx;
   EGLConfig config;
   EGLint num_configs;
   EGLint vid;

   scrnum = DefaultScreen( x_dpy );
   root = RootWindow( x_dpy, scrnum );

   if (!eglChooseConfig( egl_dpy, attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL visual config\n");
      exit(1);
   }

   assert(config);
   assert(num_configs > 0);

   if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      printf("Error: eglGetConfigAttrib() failed\n");
      exit(1);
   }

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo) {
      printf("Error: couldn't get X visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( x_dpy, root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( x_dpy, root, x, y, width, height,
		        0, visInfo->depth, InputOutput,
		        visInfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(x_dpy, win, &sizehints);
      XSetStandardProperties(x_dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

#if USE_FULL_GL
   eglBindAPI(EGL_OPENGL_API);
#else
   eglBindAPI(EGL_OPENGL_ES_API);
#endif

   if (ctxRet) {
      ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, NULL );
      if (!ctx) {
         printf("Error: eglCreateContext failed\n");
         exit(1);
      }
      *ctxRet = ctx;
   }

   *surfRet = eglCreateWindowSurface(egl_dpy, config, win, NULL);

   if (!*surfRet) {
      printf("Error: eglCreateWindowSurface failed\n");
      exit(1);
   }

   XFree(visInfo);

   *winRet = win;
}


static void
event_loop(Display *dpy, Window win1, Window win2,
           EGLDisplay egl_dpy, EGLSurface egl_surf1, EGLSurface egl_surf2,
           EGLContext egl_ctx)
{
   while (1) {
      int redraw = 0;
      int win;
      XEvent event;

      XNextEvent(dpy, &event);

      switch (event.type) {
      case Expose:
         redraw = 1;
         break;
      case ConfigureNotify:
         if (event.xconfigure.window == win1)
            win = 0;
         else
            win = 1;
         WinWidth[win] = event.xconfigure.width;
         WinHeight[win] = event.xconfigure.height;
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
         redraw = 1;
         break;
      default:
         ; /*no-op*/
      }

      if (redraw) {
         /* win 1 */
         if (!eglMakeCurrent(egl_dpy, egl_surf1, egl_surf1, egl_ctx)) {
            printf("Error: eglMakeCurrent(1) failed\n");
            return;
         }
         draw(0);
         eglSwapBuffers(egl_dpy, egl_surf1);

         /* win 2 */
         if (!eglMakeCurrent(egl_dpy, egl_surf2, egl_surf2, egl_ctx)) {
            printf("Error: eglMakeCurrent(2) failed\n");
            return;
         }
         draw(1);
         eglSwapBuffers(egl_dpy, egl_surf2);
      }
   }
}


static void
usage(void)
{
   printf("Usage:\n");
   printf("  -display <displayname>  set the display to run on\n");
   printf("  -info                   display OpenGL renderer info\n");
}
 

int
main(int argc, char *argv[])
{
   Display *x_dpy;
   Window win1, win2;
   EGLSurface egl_surf1, egl_surf2;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   EGLint egl_major, egl_minor;
   int i;
   const char *s;

   static struct {
      char *name;
      GLenum value;
      enum {GetString, GetInteger} type;
   } info_items[] = {
      {"GL_RENDERER", GL_RENDERER, GetString},
      {"GL_VERSION", GL_VERSION, GetString},
      {"GL_VENDOR", GL_VENDOR, GetString},
      {"GL_EXTENSIONS", GL_EXTENSIONS, GetString},
      {"GL_MAX_PALETTE_MATRICES_OES", GL_MAX_PALETTE_MATRICES_OES, GetInteger},
      {"GL_MAX_VERTEX_UNITS_OES", GL_MAX_VERTEX_UNITS_OES, GetInteger},
   };

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else {
         usage();
         return -1;
      }
   }

   x_dpy = XOpenDisplay(dpyName);
   if (!x_dpy) {
      printf("Error: couldn't open display %s\n",
	     dpyName ? dpyName : getenv("DISPLAY"));
      return -1;
   }

   egl_dpy = eglGetDisplay(x_dpy);
   if (!egl_dpy) {
      printf("Error: eglGetDisplay() failed\n");
      return -1;
   }

   if (!eglInitialize(egl_dpy, &egl_major, &egl_minor)) {
      printf("Error: eglInitialize() failed\n");
      return -1;
   }

   s = eglQueryString(egl_dpy, EGL_VERSION);
   printf("EGL_VERSION = %s\n", s);

   s = eglQueryString(egl_dpy, EGL_VENDOR);
   printf("EGL_VENDOR = %s\n", s);

   s = eglQueryString(egl_dpy, EGL_EXTENSIONS);
   printf("EGL_EXTENSIONS = %s\n", s);

   s = eglQueryString(egl_dpy, EGL_CLIENT_APIS);
   printf("EGL_CLIENT_APIS = %s\n", s);

   make_x_window(x_dpy, egl_dpy,
                 "xegl_two_win #1", 0, 0, WinWidth[0], WinHeight[0],
                 &win1, &egl_ctx, &egl_surf1);

   make_x_window(x_dpy, egl_dpy,
                 "xegl_two_win #2", WinWidth[0] + 50, 0,
                 WinWidth[1], WinHeight[1],
                 &win2, NULL, &egl_surf2);

   XMapWindow(x_dpy, win1);

   XMapWindow(x_dpy, win2);

   if (!eglMakeCurrent(egl_dpy, egl_surf1, egl_surf1, egl_ctx)) {
      printf("Error: eglMakeCurrent() failed\n");
      return -1;
   }

   if (printInfo) {
      for (i = 0; i < sizeof(info_items)/sizeof(info_items[0]); i++) {
         switch (info_items[i].type) {
            case GetString:
               printf("%s = %s\n", info_items[i].name, (char *)glGetString(info_items[i].value));
               break;
            case GetInteger: {
               GLint rv = -1;
               glGetIntegerv(info_items[i].value, &rv);
               printf("%s = %d\n", info_items[i].name, rv);
               break;
            }
         }
      }
   };

   init();

   event_loop(x_dpy, win1, win2, egl_dpy, egl_surf1, egl_surf2, egl_ctx);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf1);
   eglDestroySurface(egl_dpy, egl_surf2);
   eglTerminate(egl_dpy);

   XDestroyWindow(x_dpy, win1);
   XDestroyWindow(x_dpy, win2);
   XCloseDisplay(x_dpy);

   return 0;
}
