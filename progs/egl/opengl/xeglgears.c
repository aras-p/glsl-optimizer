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

/*
 * Ported to X/EGL/GLES.   XXX Actually, uses full OpenGL ATM.
 * Brian Paul
 * 30 May 2008
 */

/*
 * Command line options:
 *    -info      print GL implementation information
 *
 */


#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <EGL/egl.h>

#include <EGL/eglext.h>


#define BENCHMARK

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static double
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

#else /*BENCHMARK*/

/* dummy */
static double
current_time(void)
{
   /* update this function for other platforms! */
   static double t = 0.0;
   static int warn = 1;
   if (warn) {
      fprintf(stderr, "Warning: current_time() not implemented!!\n");
      warn = 0;
   }
   return t += 1.0;
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
   glClearColor(0.2, 0.2, 0.2, 0.2);
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
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);
   
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


struct egl_manager {
   EGLNativeDisplayType xdpy;
   EGLNativeWindowType xwin;
   EGLNativePixmapType xpix;

   EGLDisplay dpy;
   EGLConfig conf;
   EGLContext ctx;

   EGLSurface win;
   EGLSurface pix;
   EGLSurface pbuf;
   EGLImageKHR image;

   EGLBoolean verbose;
   EGLint major, minor;

   GC gc;
   GLuint fbo;
};

static struct egl_manager *
egl_manager_new(EGLNativeDisplayType xdpy, const EGLint *attrib_list,
                EGLBoolean verbose)
{
   struct egl_manager *eman;
   const char *ver;
   EGLint num_conf;

   eman = calloc(1, sizeof(*eman));
   if (!eman)
      return NULL;

   eman->verbose = verbose;
   eman->xdpy = xdpy;

   eman->dpy = eglGetDisplay(eman->xdpy);
   if (eman->dpy == EGL_NO_DISPLAY) {
      printf("eglGetDisplay() failed\n");
      free(eman);
      return NULL;
   }

   if (!eglInitialize(eman->dpy, &eman->major, &eman->minor)) {
      printf("eglInitialize() failed\n");
      free(eman);
      return NULL;
   }

   ver = eglQueryString(eman->dpy, EGL_VERSION);
   printf("EGL_VERSION = %s\n", ver);

   if (!eglChooseConfig(eman->dpy, attrib_list, &eman->conf, 1, &num_conf) ||
       !num_conf) {
      printf("eglChooseConfig() failed\n");
      eglTerminate(eman->dpy);
      free(eman);
      return NULL;
   }

   eman->ctx = eglCreateContext(eman->dpy, eman->conf, EGL_NO_CONTEXT, NULL);
   if (eman->ctx == EGL_NO_CONTEXT) {
      printf("eglCreateContext() failed\n");
      eglTerminate(eman->dpy);
      free(eman);
      return NULL;
   }

   return eman;
}

static EGLBoolean
egl_manager_create_window(struct egl_manager *eman, const char *name,
                          EGLint w, EGLint h, EGLBoolean need_surface,
                          EGLBoolean fullscreen, const EGLint *attrib_list)
{
   XVisualInfo vinfo_template, *vinfo = NULL;
   EGLint val, num_vinfo;
   Window root;
   XSetWindowAttributes attrs;
   unsigned long mask;
   EGLint x = 0, y = 0;

   if (!eglGetConfigAttrib(eman->dpy, eman->conf,
                           EGL_NATIVE_VISUAL_ID, &val)) {
      printf("eglGetConfigAttrib() failed\n");
      return EGL_FALSE;
   }
   if (val) {
      vinfo_template.visualid = (VisualID) val;
      vinfo = XGetVisualInfo(eman->xdpy, VisualIDMask, &vinfo_template, &num_vinfo);
   }
   /* try harder if window surface is not needed */
   if (!vinfo && !need_surface &&
       eglGetConfigAttrib(eman->dpy, eman->conf, EGL_BUFFER_SIZE, &val)) {
      if (val == 32)
         val = 24;
      vinfo_template.depth = val;
      vinfo = XGetVisualInfo(eman->xdpy, VisualDepthMask, &vinfo_template, &num_vinfo);
   }

   if (!vinfo) {
      printf("XGetVisualInfo() failed\n");
      return EGL_FALSE;
   }

   root = DefaultRootWindow(eman->xdpy);
   if (fullscreen) {
      x = y = 0;
      w = DisplayWidth(eman->xdpy, DefaultScreen(eman->xdpy));
      h = DisplayHeight(eman->xdpy, DefaultScreen(eman->xdpy));
   }

   /* window attributes */
   attrs.background_pixel = 0;
   attrs.border_pixel = 0;
   attrs.colormap = XCreateColormap(eman->xdpy, root, vinfo->visual, AllocNone);
   attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   attrs.override_redirect = fullscreen;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

   eman->xwin = XCreateWindow(eman->xdpy, root, x, y, w, h,
                              0, vinfo->depth, InputOutput,
                              vinfo->visual, mask, &attrs);
   XFree(vinfo);

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = w;
      sizehints.height = h;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(eman->xdpy, eman->xwin, &sizehints);
      XSetStandardProperties(eman->xdpy, eman->xwin, name, name,
                             None, (char **)NULL, 0, &sizehints);
   }

   if (need_surface) {
      eman->win = eglCreateWindowSurface(eman->dpy, eman->conf,
                                         eman->xwin, attrib_list);
      if (eman->win == EGL_NO_SURFACE) {
         printf("eglCreateWindowSurface() failed\n");
         XDestroyWindow(eman->xdpy, eman->xwin);
         eman->xwin = None;
         return EGL_FALSE;
      }
   }

   eman->gc = XCreateGC(eman->xdpy, eman->xwin, 0, NULL);

   XMapWindow(eman->xdpy, eman->xwin);

   return EGL_TRUE;
}

static EGLBoolean
egl_manager_create_pixmap(struct egl_manager *eman, EGLNativeWindowType xwin,
                          EGLBoolean need_surface, const EGLint *attrib_list)
{
   XWindowAttributes attrs;

   if (!XGetWindowAttributes(eman->xdpy, xwin, &attrs)) {
      printf("XGetWindowAttributes() failed\n");
      return EGL_FALSE;
   }

   eman->xpix = XCreatePixmap(eman->xdpy, xwin,
                              attrs.width, attrs.height, attrs.depth);

   if (need_surface) {
      eman->pix = eglCreatePixmapSurface(eman->dpy, eman->conf,
                                         eman->xpix, attrib_list);
      if (eman->pix == EGL_NO_SURFACE) {
         printf("eglCreatePixmapSurface() failed\n");
         XFreePixmap(eman->xdpy, eman->xpix);
         eman->xpix = None;
         return EGL_FALSE;
      }
   }

   return EGL_TRUE;
}

static EGLBoolean
egl_manager_create_pbuffer(struct egl_manager *eman, const EGLint *attrib_list)
{
   eman->pbuf = eglCreatePbufferSurface(eman->dpy, eman->conf, attrib_list);
   if (eman->pbuf == EGL_NO_SURFACE) {
      printf("eglCreatePbufferSurface() failed\n");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

static void
egl_manager_destroy(struct egl_manager *eman)
{
   eglMakeCurrent(eman->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglTerminate(eman->dpy);

   if (eman->xwin != None)
      XDestroyWindow(eman->xdpy, eman->xwin);
   if (eman->xpix != None)
      XFreePixmap(eman->xdpy, eman->xpix);

   XFreeGC(eman->xdpy, eman->gc);

   free(eman);
}

enum {
   GEARS_WINDOW,
   GEARS_PIXMAP,
   GEARS_PIXMAP_TEXTURE,
   GEARS_PBUFFER,
   GEARS_PBUFFER_TEXTURE,
   GEARS_RENDERBUFFER
};

static void
texture_gears(struct egl_manager *eman, int surface_type)
{
   static const GLint verts[12] =
      { -5, -6, -10,  5, -6, -10,  -5, 4, 10,  5, 4, 10 };
   static const GLint tex_coords[8] = { 0, 0,  1, 0,  0, 1,  1, 1 };

   eglMakeCurrent(eman->dpy, eman->win, eman->win, eman->ctx);

   glClearColor(0, 0, 0, 0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3, GL_INT, 0, verts);
   glTexCoordPointer(2, GL_INT, 0, tex_coords);

   if (surface_type == GEARS_PBUFFER_TEXTURE)
      eglBindTexImage(eman->dpy, eman->pbuf, EGL_BACK_BUFFER);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
   glDisable(GL_TEXTURE_2D);

   if (surface_type == GEARS_PBUFFER_TEXTURE)
      eglReleaseTexImage(eman->dpy, eman->pbuf, EGL_BACK_BUFFER);

   eglSwapBuffers(eman->dpy, eman->win); 
}

static void
copy_gears(struct egl_manager *eman,
	   EGLint tile_w, EGLint tile_h, EGLint w, EGLint h)
{
   int x, y;

   eglWaitClient();

   for (x = 0; x < w; x += tile_w) {
      for (y = 0; y < h; y += tile_h) {

	 XCopyArea(eman->xdpy, eman->xpix, eman->xwin, eman->gc,
		   0, 0, tile_w, tile_h, x, y);
      }
   }
}

static void
event_loop(struct egl_manager *eman, EGLint surface_type, EGLint w, EGLint h)
{
   int window_w = w, window_h = h;

   if (surface_type == EGL_PBUFFER_BIT)
      printf("there will be no screen update if "
             "eglCopyBuffers() is not implemented\n");

   while (1) {
      while (XPending(eman->xdpy) > 0) {
         XEvent event;
         XNextEvent(eman->xdpy, &event);
         switch (event.type) {
         case Expose:
            /* we'll redraw below */
            break;
         case ConfigureNotify:
            window_w = event.xconfigure.width;
            window_h = event.xconfigure.height;
            if (surface_type == EGL_WINDOW_BIT)
               reshape(window_w, window_h);
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

      {
         static int frames = 0;
         static double tRot0 = -1.0, tRate0 = -1.0;
         double dt, t = current_time();
         if (tRot0 < 0.0)
            tRot0 = t;
         dt = t - tRot0;
         tRot0 = t;

         /* advance rotation for next frame */
         angle += 70.0 * dt;  /* 70 degrees per second */
         if (angle > 3600.0)
             angle -= 3600.0;

         switch (surface_type) {
         case GEARS_WINDOW:
	    draw();
            eglSwapBuffers(eman->dpy, eman->win);
            break;

	 case GEARS_PBUFFER:
	    draw();
	    if (!eglCopyBuffers(eman->dpy, eman->pbuf, eman->xpix))
	       break;
	    copy_gears(eman, w, h, window_w, window_h);
	    break;

	 case GEARS_PBUFFER_TEXTURE:
            eglMakeCurrent(eman->dpy, eman->pbuf, eman->pbuf, eman->ctx);
	    draw();
	    texture_gears(eman, surface_type);
	    break;

	 case GEARS_PIXMAP:
	    draw();
	    copy_gears(eman, w, h, window_w, window_h);
	    break;

	 case GEARS_PIXMAP_TEXTURE:
            eglMakeCurrent(eman->dpy, eman->pix, eman->pix, eman->ctx);
	    draw();
	    texture_gears(eman, surface_type);
	    break;

	 case GEARS_RENDERBUFFER:
	    glBindFramebuffer(GL_FRAMEBUFFER_EXT, eman->fbo);
	    draw();
	    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	    texture_gears(eman, surface_type);
	    break;
	 }

         frames++;

         if (tRate0 < 0.0)
            tRate0 = t;
         if (t - tRate0 >= 5.0) {
            GLfloat seconds = t - tRate0;
            GLfloat fps = frames / seconds;
            printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
                   fps);
            tRate0 = t;
            frames = 0;
         }
      }
   }
}


static void
usage(void)
{
   printf("Usage:\n");
   printf("  -display <displayname>  set the display to run on\n");
   printf("  -fullscreen             run in fullscreen mode\n");
   printf("  -info                   display OpenGL renderer info\n");
   printf("  -pixmap                 use pixmap surface\n");
   printf("  -pixmap-texture         use pixmap surface and texture using EGLImage\n");
   printf("  -pbuffer                use pbuffer surface and eglCopyBuffers\n");
   printf("  -pbuffer-texture        use pbuffer surface and eglBindTexImage\n");
   printf("  -renderbuffer           renderbuffer as EGLImage and bind as texture from\n");
}
 
static const char *names[] = {
   "window",
   "pixmap",
   "pixmap_texture",
   "pbuffer",
   "pbuffer_texture",
   "renderbuffer"
};

int
main(int argc, char *argv[])
{
   const int winWidth = 300, winHeight = 300;
   Display *x_dpy;
   char *dpyName = NULL;
   struct egl_manager *eman;
   EGLint attribs[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT, /* may be changed later */
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_NONE
   };
   char win_title[] = "xeglgears (window/pixmap/pbuffer)";
   EGLint surface_type = GEARS_WINDOW;
   GLboolean printInfo = GL_FALSE;
   GLboolean fullscreen = GL_FALSE;
   EGLBoolean ret;
   GLuint texture, color_rb, depth_rb;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else if (strcmp(argv[i], "-fullscreen") == 0) {
         fullscreen = GL_TRUE;
      }
      else if (strcmp(argv[i], "-pixmap") == 0) {
         surface_type = GEARS_PIXMAP;
	 attribs[1] = EGL_PIXMAP_BIT;
      }
      else if (strcmp(argv[i], "-pixmap-texture") == 0) {
         surface_type = GEARS_PIXMAP_TEXTURE;
	 attribs[1] = EGL_PIXMAP_BIT;
      }
      else if (strcmp(argv[i], "-pbuffer") == 0) {
         surface_type = GEARS_PBUFFER;
	 attribs[1] = EGL_PBUFFER_BIT;
      }
      else if (strcmp(argv[i], "-pbuffer-texture") == 0) {
         surface_type = GEARS_PBUFFER_TEXTURE;
	 attribs[1] = EGL_PBUFFER_BIT;
      }
      else if (strcmp(argv[i], "-renderbuffer") == 0) {
         surface_type = GEARS_RENDERBUFFER;
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

   eglBindAPI(EGL_OPENGL_API);

   eman = egl_manager_new(x_dpy, attribs, printInfo);
   if (!eman) {
      XCloseDisplay(x_dpy);
      return -1;
   }

   snprintf(win_title, sizeof(win_title),
	    "xeglgears (%s)", names[surface_type]);

   ret = egl_manager_create_window(eman, win_title, winWidth, winHeight,
				   EGL_TRUE, fullscreen, NULL);
   if (!ret)
      return -1;

   /* create surface(s) */
   switch (surface_type) {
   case GEARS_WINDOW:
      if (ret)
         ret = eglMakeCurrent(eman->dpy, eman->win, eman->win, eman->ctx);
      break;
   case GEARS_PIXMAP:
   case GEARS_PIXMAP_TEXTURE:
      ret = egl_manager_create_pixmap(eman, eman->xwin, EGL_TRUE, NULL);
      if (surface_type == GEARS_PIXMAP_TEXTURE)
	 eman->image = eglCreateImageKHR (eman->dpy, eman->ctx,
					  EGL_NATIVE_PIXMAP_KHR,
					  (EGLClientBuffer) eman->xpix, NULL);
      if (ret)
         ret = eglMakeCurrent(eman->dpy, eman->pix, eman->pix, eman->ctx);
      break;
   case GEARS_PBUFFER:
   case GEARS_PBUFFER_TEXTURE:
      {
         EGLint pbuf_attribs[] = {
            EGL_WIDTH, winWidth,
            EGL_HEIGHT, winHeight,
	    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB,
	    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
            EGL_NONE
         };
         ret = (egl_manager_create_pixmap(eman, eman->xwin,
					  EGL_TRUE, NULL) &&
                egl_manager_create_pbuffer(eman, pbuf_attribs));
         if (ret)
            ret = eglMakeCurrent(eman->dpy, eman->pbuf, eman->pbuf, eman->ctx);
      }
      break;


   case GEARS_RENDERBUFFER:
      ret = eglMakeCurrent(eman->dpy, eman->win, eman->win, eman->ctx);
      if (ret == EGL_FALSE)
	 printf("failed to make context current\n");

      glGenFramebuffers(1, &eman->fbo);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, eman->fbo);
      
      glGenRenderbuffers(1, &color_rb);
      glBindRenderbuffer(GL_RENDERBUFFER_EXT, color_rb);
      glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_RGBA, winWidth, winHeight);
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				   GL_COLOR_ATTACHMENT0_EXT,
				   GL_RENDERBUFFER_EXT,
				   color_rb);

      eman->image = eglCreateImageKHR(eman->dpy, eman->ctx,
				      EGL_GL_RENDERBUFFER_KHR,
				      (EGLClientBuffer) color_rb, NULL);

      glGenRenderbuffers(1, &depth_rb);
      glBindRenderbuffer(GL_RENDERBUFFER_EXT, depth_rb);
      glRenderbufferStorage(GL_RENDERBUFFER_EXT,
			    GL_DEPTH_COMPONENT, winWidth, winHeight);
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				   GL_DEPTH_ATTACHMENT_EXT,
				   GL_RENDERBUFFER_EXT,
				   depth_rb);

      if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE) {
	 printf("framebuffer not complete\n");
	 exit(1);
      }

      break;

   default:
      ret = EGL_FALSE;
      break;
   }

   switch (surface_type) {
   case GEARS_PIXMAP_TEXTURE:
   case GEARS_RENDERBUFFER:
	   glGenTextures(1, &texture);
   	   glBindTexture(GL_TEXTURE_2D, texture);
	   glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eman->image);
	   break;
   case GEARS_PBUFFER_TEXTURE:
	   glGenTextures(1, &texture);
   	   glBindTexture(GL_TEXTURE_2D, texture);
	   break;
   }

   if (!ret) {
      egl_manager_destroy(eman);
      XCloseDisplay(x_dpy);
      return -1;
   }

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   }

   init();

   /* Set initial projection/viewing transformation.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   reshape(winWidth, winHeight);

   event_loop(eman, surface_type, winWidth, winHeight);

   glDeleteLists(gear1, 1);
   glDeleteLists(gear2, 1);
   glDeleteLists(gear3, 1);

   egl_manager_destroy(eman);
   XCloseDisplay(x_dpy);

   return 0;
}
