/*
 * Copyright (C) 2008  Tunsgten Graphics,Inc.   All Rights Reserved.
 */

/*
 * Draw a lit, textured torus with X/EGL and OpenGL ES 1.x
 * Brian Paul
 * July 2008
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


static const struct {
   GLenum internalFormat;
   const char *name;
   GLuint num_entries;
   GLuint size;
} cpal_formats[] = {
   { GL_PALETTE4_RGB8_OES,     "GL_PALETTE4_RGB8_OES",      16, 3 },
   { GL_PALETTE4_RGBA8_OES,    "GL_PALETTE4_RGBA8_OES",     16, 4 },
   { GL_PALETTE4_R5_G6_B5_OES, "GL_PALETTE4_R5_G6_B5_OES",  16, 2 },
   { GL_PALETTE4_RGBA4_OES,    "GL_PALETTE4_RGBA4_OES",     16, 2 },
   { GL_PALETTE4_RGB5_A1_OES,  "GL_PALETTE4_RGB5_A1_OES",   16, 2 },
   { GL_PALETTE8_RGB8_OES,     "GL_PALETTE8_RGB8_OES",     256, 3 },
   { GL_PALETTE8_RGBA8_OES,    "GL_PALETTE8_RGBA8_OES",    256, 4 },
   { GL_PALETTE8_R5_G6_B5_OES, "GL_PALETTE8_R5_G6_B5_OES", 256, 2 },
   { GL_PALETTE8_RGBA4_OES,    "GL_PALETTE8_RGBA4_OES",    256, 2 },
   { GL_PALETTE8_RGB5_A1_OES,  "GL_PALETTE8_RGB5_A1_OES",  256, 2 }
};
#define NUM_CPAL_FORMATS (sizeof(cpal_formats) / sizeof(cpal_formats[0]))

static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
static GLint tex_format = NUM_CPAL_FORMATS;


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
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_roty, 0, 1, 0);
   glRotatef(view_rotz, 0, 0, 1);
   glScalef(0.5, 0.5, 0.5);

   draw_torus(1.0, 3.0, 30, 60);

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

#ifdef GL_VERSION_ES_CM_1_0
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
#else
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);
#endif
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static GLint
make_cpal_texture(GLint idx)
{
#define SZ 64
   GLenum internalFormat = GL_PALETTE4_RGB8_OES + idx;
   GLenum Filter = GL_LINEAR;
   GLubyte palette[256 * 4 + SZ * SZ];
   GLubyte *indices;
   GLsizei image_size;
   GLuint i, j;
   GLuint packed_indices = 0;

   assert(cpal_formats[idx].internalFormat == internalFormat);

   /* init palette */
   switch (internalFormat) {
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE8_RGB8_OES:
      /* first entry */
      palette[0] = 255;
      palette[1] = 255;
      palette[2] = 255;
      /* second entry */
      palette[3] = 127;
      palette[4] = 127;
      palette[5] = 127;
      break;
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE8_RGBA8_OES:
      /* first entry */
      palette[0] = 255;
      palette[1] = 255;
      palette[2] = 255;
      palette[3] = 255;
      /* second entry */
      palette[4] = 127;
      palette[5] = 127;
      palette[6] = 127;
      palette[7] = 255;
      break;
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
      {
         GLushort *pal = (GLushort *) palette;
         /* first entry */
         pal[0] = (31 << 11 | 63 << 5 | 31);
         /* second entry */
         pal[1] = (15 << 11 | 31 << 5 | 15);
      }
      break;
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE8_RGBA4_OES:
      {
         GLushort *pal = (GLushort *) palette;
         /* first entry */
         pal[0] = (15 << 12 | 15 << 8 | 15 << 4 | 15);
         /* second entry */
         pal[1] = (7 << 12 | 7 << 8 | 7 << 4 | 15);
      }
      break;
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB5_A1_OES:
      {
         GLushort *pal = (GLushort *) palette;
         /* first entry */
         pal[0] = (31 << 11 | 31 << 6 | 31 << 1 | 1);
         /* second entry */
         pal[1] = (15 << 11 | 15 << 6 | 15 << 1 | 1);
      }
      break;
   }

   image_size = cpal_formats[idx].size * cpal_formats[idx].num_entries;
   indices = palette + image_size;
   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         GLfloat d;
         GLint index;
         d = (i - SZ/2) * (i - SZ/2) + (j - SZ/2) * (j - SZ/2);
         d = sqrt(d);
         index = (d < SZ / 3) ? 0 : 1;

         if (cpal_formats[idx].num_entries == 16) {
            /* 4-bit indices packed in GLubyte */
            packed_indices |= index << (4 * (1 - (j % 2)));
            if (j % 2) {
               *(indices + (i * SZ + j - 1) / 2) = packed_indices & 0xff;
               packed_indices = 0;
               image_size += 1;
            }
         }
         else {
            /* 8-bit indices */
            *(indices + i * SZ + j) = index;
            image_size += 1;
         }
      }
   }

   glActiveTexture(GL_TEXTURE0); /* unit 0 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, SZ, SZ, 0,
                          image_size, palette);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#undef SZ

   return image_size;
}


static GLint
make_texture(void)
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

   glActiveTexture(GL_TEXTURE0); /* unit 0 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#undef SZ

   return sizeof(image);
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

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

   glClearColor(0.4, 0.4, 0.4, 0.0);
   glEnable(GL_DEPTH_TEST);

   make_texture();
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


static void
event_loop(Display *dpy, Window win,
           EGLDisplay egl_dpy, EGLSurface egl_surf)
{
   int anim = 1;

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
               else if (code == XK_t) {
                  GLint size;
                  tex_format = (tex_format + 1) % (NUM_CPAL_FORMATS + 1);
                  if (tex_format < NUM_CPAL_FORMATS) {
                     size = make_cpal_texture(tex_format);
                     printf("Using %s (%d bytes)\n",
                           cpal_formats[tex_format].name, size);
                  }
                  else {
                     size = make_texture();
                     printf("Using uncompressed texture (%d bytes)\n", size);
                  }
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == ' ') {
                     anim = !anim;
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
   const int winWidth = 300, winHeight = 300;
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
                 "torus", 0, 0, winWidth, winHeight,
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
