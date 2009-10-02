/*
 * Copyright (C) 2008  Tunsgten Graphics,Inc.   All Rights Reserved.
 */

/*
 * Test GL_OES_draw_texture
 * Brian Paul
 * August 2008
 */

#define GL_GLEXT_PROTOTYPES

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



static GLfloat view_posx = 10.0, view_posy = 20.0;
static GLfloat width = 200, height = 200;


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glDrawTexfOES(view_posx, view_posy, 0.0, width, height);
}


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
   glTranslatef(0.0, 0.0, -15.0);
}


static float
dist(GLuint i, GLuint j, float x, float y)
{
   return sqrt((i-x) * (i-x) + (j-y) * (j-y));
}

static void
make_smile_texture(void)
{
#define SZ 128
   GLenum Filter = GL_LINEAR;
   GLubyte image[SZ][SZ][4];
   GLuint i, j;
   GLint cropRect[4];

   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         GLfloat d_mouth = dist(i, j, SZ/2, SZ/2);
         GLfloat d_rt_eye = dist(i, j, SZ*3/4, SZ*3/4);
         GLfloat d_lt_eye = dist(i, j, SZ*3/4, SZ*1/4);
         if (d_rt_eye < SZ / 8 || d_lt_eye < SZ / 8) {
            image[i][j][0] = 20;
            image[i][j][1] = 50;
            image[i][j][2] = 255;
            image[i][j][3] = 255;
         }
         else if (i < SZ/2 && d_mouth < SZ/3) {
            image[i][j][0] = 255;
            image[i][j][1] = 20;
            image[i][j][2] = 20;
            image[i][j][3] = 255;
         }
         else {
            image[i][j][0] = 200;
            image[i][j][1] = 200;
            image[i][j][2] = 200;
            image[i][j][3] = 255;
         }
      }
   }

   glActiveTexture(GL_TEXTURE0); /* unit 0 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   cropRect[0] = 0;
   cropRect[1] = 0;
   cropRect[2] = SZ;
   cropRect[3] = SZ;
   glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, cropRect);
#undef SZ
}



static void
init(void)
{
   const char *ext = (char *) glGetString(GL_EXTENSIONS);

   if (!strstr(ext, "GL_OES_draw_texture")) {
      fprintf(stderr, "Sorry, this program requires GL_OES_draw_texture");
      exit(1);
   }

   glClearColor(0.4, 0.4, 0.4, 0.0);

   make_smile_texture();
   glEnable(GL_TEXTURE_2D);
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

   eglBindAPI(EGL_OPENGL_ES_API);

   ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, NULL );
   if (!ctx) {
      printf("Error: eglCreateContext failed\n");
      exit(1);
   }

   *surfRet = eglCreateWindowSurface(egl_dpy, config, win, NULL);

   if (!*surfRet) {
      printf("Error: eglCreateWindowSurface failed\n");
      exit(1);
   }

   XFree(visInfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win,
           EGLDisplay egl_dpy, EGLSurface egl_surf)
{
   int anim = 0;

   while (1) {
      int redraw = 0;

      if (!anim || XPending(dpy)) {
         XEvent event;
         XNextEvent(dpy, &event);

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
               view_posx -= 1.0;
               }
               else if (code == XK_Right) {
                  view_posx += 1.0;
               }
               else if (code == XK_Up) {
                  view_posy += 1.0;
               }
               else if (code == XK_Down) {
                  view_posy -= 1.0;
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == ' ') {
                     anim = !anim;
                  }
                  else if (buffer[0] == 'w') {
                     width -= 1.0f;
                  }
                  else if (buffer[0] == 'W') {
                     width += 1.0f;
                  }
                  else if (buffer[0] == 'h') {
                     height -= 1.0f;
                  }
                  else if (buffer[0] == 'H') {
                     height += 1.0f;
                  }
                  else if (buffer[0] == 27) {
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

      if (anim) {
         view_posx += 1.0;
         view_posy += 2.0;
         redraw = 1;
      }

      if (redraw) {
         draw();
         eglSwapBuffers(egl_dpy, egl_surf);
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
   const int winWidth = 400, winHeight = 300;
   Display *x_dpy;
   Window win;
   EGLSurface egl_surf;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   EGLint egl_major, egl_minor;
   int i;
   const char *s;

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
                 "drawtex", 0, 0, winWidth, winHeight,
                 &win, &egl_ctx, &egl_surf);

   XMapWindow(x_dpy, win);
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
      printf("Error: eglMakeCurrent() failed\n");
      return -1;
   }

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   init();

   /* Set initial projection/viewing transformation.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   reshape(winWidth, winHeight);

   event_loop(x_dpy, win, egl_dpy, egl_surf);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);
   eglTerminate(egl_dpy);


   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
