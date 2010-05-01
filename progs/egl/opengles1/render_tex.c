/*
 * Copyright (C) 2008  Tunsgten Graphics,Inc.   All Rights Reserved.
 */

/*
 * Test EGL render to texture.
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


static int TexWidth = 256, TexHeight = 256;

static int WinWidth = 300, WinHeight = 300;

static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;

static GLuint DotTexture, RenderTexture;


static void
Normal(GLfloat *n, GLfloat nx, GLfloat ny, GLfloat nz)
{
   n[0] = nx;
   n[1] = ny;
   n[2] = nz;
}

static void
Vertex(GLfloat *v, GLfloat vx, GLfloat vy, GLfloat vz)
{
   v[0] = vx;
   v[1] = vy;
   v[2] = vz;
}

static void
Texcoord(GLfloat *v, GLfloat s, GLfloat t)
{
   v[0] = s;
   v[1] = t;
}


/* Borrowed from glut, adapted */
static void
draw_torus(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
   int i, j;
   GLfloat theta, phi, theta1;
   GLfloat cosTheta, sinTheta;
   GLfloat cosTheta1, sinTheta1;
   GLfloat ringDelta, sideDelta;
   GLfloat varray[100][3], narray[100][3], tarray[100][2];
   int vcount;

   glVertexPointer(3, GL_FLOAT, 0, varray);
   glNormalPointer(GL_FLOAT, 0, narray);
   glTexCoordPointer(2, GL_FLOAT, 0, tarray);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   
   ringDelta = 2.0 * M_PI / rings;
   sideDelta = 2.0 * M_PI / nsides;

   theta = 0.0;
   cosTheta = 1.0;
   sinTheta = 0.0;
   for (i = rings - 1; i >= 0; i--) {
      theta1 = theta + ringDelta;
      cosTheta1 = cos(theta1);
      sinTheta1 = sin(theta1);

      vcount = 0; /* glBegin(GL_QUAD_STRIP); */

      phi = 0.0;
      for (j = nsides; j >= 0; j--) {
         GLfloat s0, s1, t;
         GLfloat cosPhi, sinPhi, dist;

         phi += sideDelta;
         cosPhi = cos(phi);
         sinPhi = sin(phi);
         dist = R + r * cosPhi;

         s0 = 20.0 * theta / (2.0 * M_PI);
         s1 = 20.0 * theta1 / (2.0 * M_PI);
         t = 8.0 * phi / (2.0 * M_PI);

         Normal(narray[vcount], cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
         Texcoord(tarray[vcount], s1, t);
         Vertex(varray[vcount], cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
         vcount++;

         Normal(narray[vcount], cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
         Texcoord(tarray[vcount], s0, t);
         Vertex(varray[vcount], cosTheta * dist, -sinTheta * dist,  r * sinPhi);
         vcount++;
      }

      /*glEnd();*/
      assert(vcount <= 100);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, vcount);

      theta = theta1;
      cosTheta = cosTheta1;
      sinTheta = sinTheta1;
   }

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


static void
draw_torus_to_texture(void)
{
   glViewport(0, 0, TexWidth, TexHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustumf(-1, 1, -1, 1, 5.0, 60.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   glClearColor(0.4, 0.4, 0.4, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindTexture(GL_TEXTURE_2D, DotTexture);

   glEnable(GL_LIGHTING);

   glPushMatrix();
   glRotatef(view_roty, 0, 1, 0);
   glScalef(0.5, 0.5, 0.5);

   draw_torus(1.0, 3.0, 30, 60);

   glPopMatrix();

   glDisable(GL_LIGHTING);

#if 0
   glBindTexture(GL_TEXTURE_2D, RenderTexture);
   glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, TexWidth, TexHeight);
#endif

   glFinish();
}


static void
draw_textured_quad(void)
{
   GLfloat ar = (GLfloat) WinWidth / (GLfloat) WinHeight;

   glViewport(0, 0, WinWidth, WinHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -8.0);

   glClearColor(0.4, 0.4, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindTexture(GL_TEXTURE_2D, RenderTexture);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_rotz, 0, 0, 1);

   {
      static const GLfloat texcoord[4][2] = {
         { 0, 0 },  { 1, 0 },  { 0, 1 },  { 1, 1 }
      };
      static const GLfloat vertex[4][2] = {
         { -1, -1 },  {  1, -1 },  { -1,  1 },  {  1,  1 }
      };

      glVertexPointer(2, GL_FLOAT, 0, vertex);
      glTexCoordPointer(2, GL_FLOAT, 0, texcoord);
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   }

   glPopMatrix();
}



static void
draw(EGLDisplay egl_dpy, EGLSurface egl_surf, EGLSurface egl_pbuf,
     EGLContext egl_ctx)
{
   /*printf("Begin draw\n");*/

   /* first draw torus to pbuffer /texture */
#if 01
   if (!eglMakeCurrent(egl_dpy, egl_pbuf, egl_pbuf, egl_ctx)) {
#else
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
#endif
      printf("Error: eglMakeCurrent(pbuf) failed\n");
      return;
   }
   draw_torus_to_texture();

   /* draw textured quad to window */
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
      printf("Error: eglMakeCurrent(pbuffer) failed\n");
      return;
   }

   glBindTexture(GL_TEXTURE_2D, RenderTexture);
   eglBindTexImage(egl_dpy, egl_pbuf, EGL_BACK_BUFFER);
   draw_textured_quad();
   eglReleaseTexImage(egl_dpy, egl_pbuf, EGL_BACK_BUFFER);

   eglSwapBuffers(egl_dpy, egl_surf);

   /*printf("End draw\n");*/
}



static void
make_dot_texture(void)
{
#define SZ 64
   GLenum Filter = GL_LINEAR;
   GLubyte image[SZ][SZ][4];
   GLuint i, j;

   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         GLfloat d = (i - SZ/2) * (i - SZ/2) + (j - SZ/2) * (j - SZ/2);
         d = sqrt(d);
         if (d < SZ/3) {
            image[i][j][0] = 255;
            image[i][j][1] = 255;
            image[i][j][2] = 255;
            image[i][j][3] = 255;
         }
         else {
            image[i][j][0] = 127;
            image[i][j][1] = 127;
            image[i][j][2] = 127;
            image[i][j][3] = 255;
         }
      }
   }

   glGenTextures(1, &DotTexture);
   glBindTexture(GL_TEXTURE_2D, DotTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#undef SZ
}


static void
make_render_texture(void)
{
   GLenum Filter = GL_LINEAR;
   glGenTextures(1, &RenderTexture);
   glBindTexture(GL_TEXTURE_2D, RenderTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}


static void
init(void)
{
   static const GLfloat red[4] = {1, 0, 0, 0};
   static const GLfloat white[4] = {1.0, 1.0, 1.0, 1.0};
   static const GLfloat diffuse[4] = {0.7, 0.7, 0.7, 1.0};
   static const GLfloat specular[4] = {0.001, 0.001, 0.001, 1.0};
   static const GLfloat pos[4] = {20, 20, 50, 1};

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 9.0);

   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

   glEnable(GL_DEPTH_TEST);

   make_dot_texture();
   make_render_texture();

   printf("DotTexture=%u RenderTexture=%u\n", DotTexture, RenderTexture);

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


static EGLSurface
make_pbuffer(Display *x_dpy, EGLDisplay egl_dpy, int width, int height)
{
   static const EGLint config_attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
      EGL_NONE
   };
   EGLConfig config;
   EGLSurface pbuf;
   EGLint num_configs;
   EGLint pbuf_attribs[15];
   int i = 0;

   pbuf_attribs[i++] = EGL_WIDTH;
   pbuf_attribs[i++] = width;
   pbuf_attribs[i++] = EGL_HEIGHT;
   pbuf_attribs[i++] = height;
   pbuf_attribs[i++] = EGL_TEXTURE_FORMAT;
   pbuf_attribs[i++] = EGL_TEXTURE_RGBA;
   pbuf_attribs[i++] = EGL_TEXTURE_TARGET;
   pbuf_attribs[i++] = EGL_TEXTURE_2D;
   pbuf_attribs[i++] = EGL_MIPMAP_TEXTURE;
   pbuf_attribs[i++] = EGL_FALSE;
   pbuf_attribs[i++] = EGL_NONE;
   assert(i <= 15);

   if (!eglChooseConfig( egl_dpy, config_attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL config for pbuffer\n");
      exit(1);
   }

   pbuf = eglCreatePbufferSurface(egl_dpy, config, pbuf_attribs);

   return pbuf;
}


static void
event_loop(Display *dpy, Window win,
           EGLDisplay egl_dpy, EGLSurface egl_surf, EGLSurface egl_pbuf,
           EGLContext egl_ctx)
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
            if (event.xconfigure.window == win) {
               WinWidth = event.xconfigure.width;
               WinHeight = event.xconfigure.height;
            }
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
                  if (buffer[0] == ' ') {
                     anim = !anim;
                  }
                  else if (buffer[0] == 'z') {
                     view_rotz += 5.0;
                  }
                  else if (buffer[0] == 'Z') {
                     view_rotz -= 5.0;
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
         view_rotx += 1.0;
         view_roty += 2.0;
         redraw = 1;
      }

      if (redraw) {
         draw(egl_dpy, egl_surf, egl_pbuf, egl_ctx);
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
   Window win;
   EGLSurface egl_surf, egl_pbuf;
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
                 "render_tex", 0, 0, WinWidth, WinHeight,
                 &win, &egl_ctx, &egl_surf);

   egl_pbuf = make_pbuffer(x_dpy, egl_dpy, TexWidth, TexHeight);
   if (!egl_pbuf) {
      printf("Error: eglCreatePBufferSurface() failed\n");
      return -1;
   }

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

   event_loop(x_dpy, win, egl_dpy, egl_surf, egl_pbuf, egl_ctx);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);
   eglTerminate(egl_dpy);


   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
