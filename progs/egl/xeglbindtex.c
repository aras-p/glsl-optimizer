/*
 * Simple demo for eglBindTexImage.  Based on xegl_tri.c by
 *
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
 * The spec says that eglBindTexImage supports only OpenGL ES context, but this
 * demo uses OpenGL context.  Keep in mind that this is non-standard.
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <EGL/egl.h>

static EGLDisplay dpy;
static EGLContext ctx_win, ctx_pbuf;
static EGLSurface surf_win, surf_pbuf;
static GLuint tex_pbuf;

static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
static GLboolean blend = GL_TRUE;
static GLuint color_flow;

static void
make_pbuffer(int width, int height)
{
   static const EGLint config_attribs[] = {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE,
      EGL_NONE
   };
   EGLint pbuf_attribs[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB,
      EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
      EGL_NONE
   };
   EGLConfig config;
   EGLint num_configs;

   if (!eglChooseConfig(dpy, config_attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL visual config for pbuffer\n");
      exit(1);
   }

   eglBindAPI(EGL_OPENGL_API);
   ctx_pbuf = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL );
   surf_pbuf = eglCreatePbufferSurface(dpy, config, pbuf_attribs);
   if (surf_pbuf == EGL_NO_SURFACE) {
      printf("failed to allocate pbuffer\n");
      exit(1);
   }

   glGenTextures(1, &tex_pbuf);
}

static void
use_pbuffer(void)
{
   static int initialized;

   eglMakeCurrent(dpy, surf_pbuf, surf_pbuf, ctx_pbuf);
   if (!initialized) {
      EGLint width, height;
      GLfloat ar;

      initialized = 1;

      eglQuerySurface(dpy, surf_pbuf, EGL_WIDTH, &width);
      eglQuerySurface(dpy, surf_pbuf, EGL_WIDTH, &height);
      ar = (GLfloat) width / (GLfloat) height;

      glViewport(0, 0, (GLint) width, (GLint) height);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-ar, ar, -1, 1, 1.0, 10.0);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      /* y-inverted */
      glScalef(1.0, -1.0, 1.0);

      glTranslatef(0.0, 0.0, -5.0);

      glClearColor(0.2, 0.2, 0.2, 0.0);
   }
}

static void
make_window(Display *x_dpy, const char *name,
            int x, int y, int width, int height,
            Window *winRet)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 8,
      EGL_NONE
   };

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visInfo, visTemplate;
   int num_visuals;
   EGLConfig config;
   EGLint num_configs, vid;

   scrnum = DefaultScreen( x_dpy );
   root = RootWindow( x_dpy, scrnum );

   if (!eglChooseConfig(dpy, attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL visual config\n");
      exit(1);
   }

   if (!eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
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
   attr.override_redirect = 0;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

   win = XCreateWindow( x_dpy, root, 0, 0, width, height,
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

   eglBindAPI(EGL_OPENGL_API);
   ctx_win = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL );
   if (!ctx_win) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   surf_win = eglCreateWindowSurface(dpy, config, win, NULL);

   XFree(visInfo);

   *winRet = win;
}

static void
use_window(void)
{
   static int initialized;

   eglMakeCurrent(dpy, surf_win, surf_win, ctx_win);
   if (!initialized) {
      initialized = 1;
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex_pbuf);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   }
}

static void
draw_triangle(void)
{
   static const GLfloat verts[3][2] = {
      { -3, -3 },
      {  3, -3 },
      {  0,  3 }
   };
   GLfloat colors[3][3] = {
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }
   };
   GLint i;

   /* flow the color */
   for (i = 0; i < 3; i++) {
      GLint first = (i + color_flow / 256) % 3;
      GLint second = (first + 1) % 3;
      GLint third = (second + 1) % 3;
      GLfloat c = (color_flow % 256) / 256.0f;

      c = c * c * c;
      colors[i][first] = 1.0f - c;
      colors[i][second] = c;
      colors[i][third] = 0.0f;
   }

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glColorPointer(3, GL_FLOAT, 0, colors);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glDrawArrays(GL_TRIANGLES, 0, 3);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
}

static void
draw_textured_cube(void)
{
   static const GLfloat verts[][2] = {
      { -4, -4 },
      {  4, -4 },
      {  4,  4 },
      { -4,  4 }
   };
   static const GLfloat colors[][4] = {
      { 1, 1, 1, 0.5 },
      { 1, 1, 1, 0.5 },
      { 1, 1, 1, 0.5 },
      { 1, 1, 1, 0.5 }
   };
   static const GLfloat texs[][2] = {
      { 0, 0 },
      { 1, 0 },
      { 1, 1 },
      { 0, 1 }
   };
   static const GLfloat xforms[6][4] = {
      {   0, 0, 1, 0 },
      {  90, 0, 1, 0 },
      { 180, 0, 1, 0 },
      { 270, 0, 1, 0 },
      {  90, 1, 0, 0 },
      { -90, 1, 0, 0 }
   };
   GLint i;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (blend) {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
   } else {
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
   }

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glColorPointer(4, GL_FLOAT, 0, colors);
   glTexCoordPointer(2, GL_FLOAT, 0, texs);

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   for (i = 0; i < 6; i++) {
      glPushMatrix();
      glRotatef(xforms[i][0], xforms[i][1], xforms[i][2], xforms[i][3]);
      glTranslatef(0, 0, 4.1);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glPopMatrix();
   }

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

static void
draw(void)
{
   use_pbuffer();
   draw_triangle();

   use_window();

   eglBindTexImage(dpy, surf_pbuf, EGL_BACK_BUFFER);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_roty, 0, 1, 0);
   glRotatef(view_rotz, 0, 0, 1);

   draw_textured_cube();

   glPopMatrix();

   eglReleaseTexImage(dpy, surf_pbuf, EGL_BACK_BUFFER);
}

/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   use_window();

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);
}

static void
event_loop(Display *x_dpy, Window win)
{
   while (1) {
      int redraw = 1;

      if (XPending(x_dpy) > 0) {
         XEvent event;
         XNextEvent(x_dpy, &event);

         switch (event.type) {
         case Expose:
            redraw = 1;
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
               else if (code == XK_b) {
                  blend = !blend;
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
      }

      if (redraw) {
         view_rotx += 1.0;
         view_roty += 2.0;
         view_rotz += 1.5;
         color_flow += 20;
         draw();
         eglSwapBuffers(dpy, surf_win);
      }
   }
}

int
main(int argc, char *argv[])
{
   const int winWidth = 300, winHeight = 300;
   Display *x_dpy;
   Window win;
   char *dpyName = NULL;
   EGLint egl_major, egl_minor;
   const char *s;

   x_dpy = XOpenDisplay(dpyName);
   if (!x_dpy) {
      printf("Error: couldn't open display %s\n",
	     dpyName ? dpyName : getenv("DISPLAY"));
      return -1;
   }

   dpy = eglGetDisplay(x_dpy);
   if (!dpy) {
      printf("Error: eglGetDisplay() failed\n");
      return -1;
   }

   if (!eglInitialize(dpy, &egl_major, &egl_minor)) {
      printf("Error: eglInitialize() failed\n");
      return -1;
   }

   s = eglQueryString(dpy, EGL_VERSION);
   printf("EGL_VERSION = %s\n", s);

   make_window(x_dpy, "color flow", 0, 0, winWidth, winHeight, &win);
   make_pbuffer(winWidth, winHeight);

   XMapWindow(x_dpy, win);

   reshape(winWidth, winHeight);
   event_loop(x_dpy, win);

   glDeleteTextures(1, &tex_pbuf);

   eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglTerminate(dpy);

   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
