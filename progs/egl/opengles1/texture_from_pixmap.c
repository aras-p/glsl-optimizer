/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

/*
 * This demo uses EGL_KHR_image_pixmap and GL_OES_EGL_image to demonstrate
 * texture-from-pixmap.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> /* for usleep */
#include <sys/time.h> /* for gettimeofday */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct app_data {
   /* native */
   Display *xdpy;
   Window canvas, cube;
   Pixmap pix;
   unsigned int width, height, depth;
   GC fg, bg;

   /* EGL */
   EGLDisplay dpy;
   EGLContext ctx;
   EGLSurface surf;
   EGLImageKHR img;

   /* OpenGL ES */
   GLuint texture;

   PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
   PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
   PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

   /* app state */
   Bool loop;
   Bool redraw, reshape;

   struct {
      Bool active;
      unsigned long next_frame; /* in ms */
      float view_rotx;
      float view_roty;
      float view_rotz;

   } animate;

   struct {
      Bool active;
      int x1, y1;
      int x2, y2;
   } paint;
};

static void
gl_redraw(void)
{
   const GLfloat verts[4][2] = {
      { -1, -1 },
      {  1, -1 },
      {  1,  1 },
      { -1,  1 }
   };
   const GLfloat texcoords[4][2] = {
      { 0, 1 },
      { 1, 1 },
      { 1, 0 },
      { 0, 0 }
   };
   const GLfloat faces[6][4] = {
      {   0, 0, 1, 0 },
      {  90, 0, 1, 0 },
      { 180, 0, 1, 0 },
      { 270, 0, 1, 0 },
      {  90, 1, 0, 0 },
      { -90, 1, 0, 0 }
   };
   GLint i;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   for (i = 0; i < 6; i++) {
      glPushMatrix();
      glRotatef(faces[i][0], faces[i][1], faces[i][2], faces[i][3]);
      glTranslatef(0, 0, 1.0);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glPopMatrix();
   }

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

static void
gl_reshape(int width, int height)
{
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   glViewport(0, 0, width, height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}

static void
app_redraw(struct app_data *data)
{
   /* pixmap has changed */
   if (data->reshape || data->paint.active) {
      eglWaitNative(EGL_CORE_NATIVE_ENGINE);

      if (data->reshape) {
         data->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D,
               (GLeglImageOES) data->img);
      }
   }

   XCopyArea(data->xdpy, data->pix, data->canvas, data->fg,
         0, 0, data->width, data->height, 0, 0);

   glPushMatrix();
   glRotatef(data->animate.view_rotx, 1, 0, 0);
   glRotatef(data->animate.view_roty, 0, 1, 0);
   glRotatef(data->animate.view_rotz, 0, 0, 1);
   gl_redraw();
   glPopMatrix();

   eglSwapBuffers(data->dpy, data->surf);
}

static void
app_reshape(struct app_data *data)
{
   const EGLint img_attribs[] = {
      EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
      EGL_NONE
   };

   XResizeWindow(data->xdpy, data->cube, data->width, data->height);
   XMoveWindow(data->xdpy, data->cube, data->width, 0);

   if (data->img)
      data->eglDestroyImageKHR(data->dpy, data->img);
   if (data->pix)
      XFreePixmap(data->xdpy, data->pix);

   data->pix = XCreatePixmap(data->xdpy, data->canvas, data->width, data->height, data->depth);
   XFillRectangle(data->xdpy, data->pix, data->bg, 0, 0, data->width, data->height);

   data->img = data->eglCreateImageKHR(data->dpy, EGL_NO_CONTEXT,
         EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer) data->pix, img_attribs);

   gl_reshape(data->width, data->height);
}

static void
app_toggle_animate(struct app_data *data)
{
   data->animate.active = !data->animate.active;

   if (data->animate.active) {
      struct timeval tv;

      gettimeofday(&tv, NULL);
      data->animate.next_frame = tv.tv_sec * 1000 + tv.tv_usec / 1000;
   }
}

static void
app_next_event(struct app_data *data)
{
   XEvent event;

   data->reshape = False;
   data->redraw = False;
   data->paint.active = False;

   if (data->animate.active) {
      struct timeval tv;
      unsigned long now;

      gettimeofday(&tv, NULL);
      now = tv.tv_sec * 1000 + tv.tv_usec / 1000;

      /* wait for next frame */
      if (!XPending(data->xdpy) && now < data->animate.next_frame) {
         usleep((data->animate.next_frame - now) * 1000);
         gettimeofday(&tv, NULL);
         now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
      }

      while (now >= data->animate.next_frame) {
         data->animate.view_rotx += 1.0;
         data->animate.view_roty += 2.0;
         data->animate.view_rotz += 1.5;

         /* 30fps */
         data->animate.next_frame += 1000 / 30;
      }

      /* check again in case there were events when sleeping */
      if (!XPending(data->xdpy)) {
         data->redraw = True;
         return;
      }
   }

   XNextEvent(data->xdpy, &event);

   switch (event.type) {
   case ConfigureNotify:
      data->width = event.xconfigure.width / 2;
      data->height = event.xconfigure.height;
      data->reshape = True;
      break;
   case Expose:
      data->redraw = True;
      break;
   case KeyPress:
      {
         int code;

         code = XLookupKeysym(&event.xkey, 0);
         switch (code) {
         case XK_a:
            app_toggle_animate(data);
            break;
         case XK_Escape:
            data->loop = False;
            break;
         default:
            break;
         }
      }
      break;
   case ButtonPress:
      data->paint.x1 = data->paint.x2 = event.xbutton.x;
      data->paint.y1 = data->paint.y2 = event.xbutton.y;
      break;
   case ButtonRelease:
      data->paint.active = False;
      break;
   case MotionNotify:
      data->paint.x1 = data->paint.x2;
      data->paint.y1 = data->paint.y2;
      data->paint.x2 = event.xmotion.x;
      data->paint.y2 = event.xmotion.y;
      data->paint.active = True;
      break;
   default:
      break;
   }

   if (data->paint.active || data->reshape)
      data->redraw = True;
}

static void
app_init_gl(struct app_data *data)
{
   glClearColor(0.1, 0.1, 0.3, 0.0);
   glColor4f(1.0, 1.0, 1.0, 1.0);

   glGenTextures(1, &data->texture);

   glBindTexture(GL_TEXTURE_2D, data->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
}

static Bool
app_init_exts(struct app_data *data)
{
   const char *exts;

   exts = eglQueryString(data->dpy, EGL_EXTENSIONS);
   data->eglCreateImageKHR =
      (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
   data->eglDestroyImageKHR =
      (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
   if (!exts || !strstr(exts, "EGL_KHR_image_pixmap") ||
       !data->eglCreateImageKHR || !data->eglDestroyImageKHR) {
      printf("EGL does not support EGL_KHR_image_pixmap\n");
      return False;
   }

   exts = (const char *) glGetString(GL_EXTENSIONS);
   data->glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
      eglGetProcAddress("glEGLImageTargetTexture2DOES");
   if (!exts || !strstr(exts, "GL_OES_EGL_image") ||
       !data->glEGLImageTargetTexture2DOES) {
      printf("OpenGL ES does not support GL_OES_EGL_image\n");
      return False;
   }

   return True;
}

static void
app_run(struct app_data *data)
{
   Window root;
   int x, y;
   unsigned int border;

   if (!eglMakeCurrent(data->dpy, data->surf, data->surf, data->ctx))
      return;

   if (!app_init_exts(data))
      return;

   printf("Draw something on the left with the mouse!\n");

   app_init_gl(data);

   if (!XGetGeometry(data->xdpy, data->canvas, &root, &x, &y,
            &data->width, &data->height, &border, &data->depth))
      return;
   data->width /= 2;

   app_reshape(data);

   XMapWindow(data->xdpy, data->canvas);
   XMapWindow(data->xdpy, data->cube);

   app_toggle_animate(data);
   data->loop = True;

   while (data->loop) {
      app_next_event(data);

      if (data->reshape)
         app_reshape(data);
      if (data->paint.active) {
         XDrawLine(data->xdpy, data->pix, data->fg,
               data->paint.x1, data->paint.y1,
               data->paint.x2, data->paint.y2);
      }

      if (data->redraw)
         app_redraw(data);
   }

   eglMakeCurrent(data->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

static Bool
make_x_window(struct app_data *data, const char *name,
              int x, int y, int width, int height)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
      EGL_NONE
   };
   static const EGLint ctx_attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 1,
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
   EGLint num_configs;
   EGLint vid;

   scrnum = DefaultScreen( data->xdpy );
   root = RootWindow( data->xdpy, scrnum );

   if (!eglChooseConfig( data->dpy, attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL visual config\n");
      exit(1);
   }

   assert(config);
   assert(num_configs > 0);

   if (!eglGetConfigAttrib(data->dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      printf("Error: eglGetConfigAttrib() failed\n");
      exit(1);
   }

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo(data->xdpy, VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo) {
      printf("Error: couldn't get X visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( data->xdpy, root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask |
                     KeyPressMask | ButtonPressMask | ButtonMotionMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( data->xdpy, root, 0, 0, width * 2, height,
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
      XSetNormalHints(data->xdpy, win, &sizehints);
      XSetStandardProperties(data->xdpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   data->canvas = win;

   attr.event_mask = 0x0;
   win = XCreateWindow( data->xdpy, win, width, 0, width, height,
		        0, visInfo->depth, InputOutput,
		        visInfo->visual, mask, &attr );
   data->cube = win;

   eglBindAPI(EGL_OPENGL_ES_API);

   data->ctx = eglCreateContext(data->dpy, config, EGL_NO_CONTEXT, ctx_attribs );
   if (!data->ctx) {
      printf("Error: eglCreateContext failed\n");
      exit(1);
   }

   data->surf = eglCreateWindowSurface(data->dpy, config, data->cube, NULL);
   if (!data->surf) {
      printf("Error: eglCreateWindowSurface failed\n");
      exit(1);
   }

   XFree(visInfo);

   return (data->canvas && data->cube && data->ctx && data->surf);
}

static void
app_fini(struct app_data *data)
{
   if (data->img)
      data->eglDestroyImageKHR(data->dpy, data->img);
   if (data->pix)
      XFreePixmap(data->xdpy, data->pix);

   if (data->fg)
      XFreeGC(data->xdpy, data->fg);
   if (data->bg)
      XFreeGC(data->xdpy, data->bg);

   if (data->surf)
      eglDestroySurface(data->dpy, data->surf);
   if (data->ctx)
      eglDestroyContext(data->dpy, data->ctx);

   if (data->cube)
      XDestroyWindow(data->xdpy, data->cube);
   if (data->canvas)
      XDestroyWindow(data->xdpy, data->canvas);

   if (data->dpy)
      eglTerminate(data->dpy);
   if (data->xdpy)
      XCloseDisplay(data->xdpy);
}

static Bool
app_init(struct app_data *data, int argc, char **argv)
{
   XGCValues gc_vals;

   memset(data, 0, sizeof(*data));

   data->xdpy = XOpenDisplay(NULL);
   if (!data->xdpy)
      goto fail;

   data->dpy = eglGetDisplay(data->xdpy);
   if (!data->dpy || !eglInitialize(data->dpy, NULL, NULL))
      goto fail;

   if (!make_x_window(data, "EGLImage TFP", 0, 0, 300, 300))
      goto fail;

   gc_vals.function = GXcopy;
   gc_vals.foreground = WhitePixel(data->xdpy, DefaultScreen(data->xdpy));
   gc_vals.line_width = 3;
   gc_vals.line_style = LineSolid;
   gc_vals.fill_style = FillSolid;

   data->fg = XCreateGC(data->xdpy, data->canvas,
         GCFunction | GCForeground | GCLineWidth | GCLineStyle | GCFillStyle,
         &gc_vals);
   gc_vals.foreground = BlackPixel(data->xdpy, DefaultScreen(data->xdpy));
   data->bg = XCreateGC(data->xdpy, data->canvas,
         GCFunction | GCForeground | GCLineWidth | GCLineStyle | GCFillStyle,
         &gc_vals);
   if (!data->fg || !data->bg)
      goto fail;

   return True;

fail:
   app_fini(data);
   return False;
}

int
main(int argc, char **argv)
{
   struct app_data data;

   if (app_init(&data, argc, argv)) {
      app_run(&data);
      app_fini(&data);
   }

   return 0;
}
