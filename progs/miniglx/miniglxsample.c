
#define USE_MINIGLX 1  /* 1 = use Mini GLX, 0 = use Xlib/GLX */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <GL/gl.h>

#if USE_MINIGLX
#include <GL/miniglx.h>
#else
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

static void _subset_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   glBegin( GL_QUADS );
   glVertex2f( x1, y1 );
   glVertex2f( x2, y1 );
   glVertex2f( x2, y2 );
   glVertex2f( x1, y2 );
   glEnd();
}


/*
 * Create a simple double-buffered RGBA window.
 */
static Window
MakeWindow(Display * dpy, unsigned int width, unsigned int height)
{
   int visAttributes[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None
   };
   XSetWindowAttributes attr;
   unsigned long attrMask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   root = RootWindow(dpy, 0);

   /* Choose GLX visual / pixel format */
   visinfo = glXChooseVisual(dpy, 0, visAttributes);
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   /* Create the window */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attrMask = CWBackPixel | CWBorderPixel | CWColormap;
   win = XCreateWindow(dpy, root, 0, 0, width, height,
		       0, visinfo->depth, InputOutput,
		       visinfo->visual, attrMask, &attr);
   if (!win) {
      printf("Error: XCreateWindow failed\n");
      exit(1);
   }

   /* Display the window */
   XMapWindow(dpy, win);

   /* Create GLX rendering context */
   ctx = glXCreateContext(dpy, visinfo, NULL, True);
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   /* Bind the rendering context and window */
   glXMakeCurrent(dpy, win, ctx);

   glViewport(0, 0, width, height);

   return win;
}


/*
 * Draw a few frames of a rotating square.
 */
static void
DrawFrames(Display * dpy, Window win)
{
   int angle;
   glShadeModel(GL_FLAT);
   glClearColor(0.5, 0.5, 0.5, 1.0);
   for (angle = 0; angle < 360; angle += 10) {
      glClear(GL_COLOR_BUFFER_BIT);
      glColor3f(1.0, 1.0, 0.0);
      glPushMatrix();
      glRotatef(angle, 0, 0, 1);
      _subset_Rectf(-0.8, -0.8, 0.8, 0.8);
      glPopMatrix();
      glXSwapBuffers(dpy, win);
      sleep(1);
   }
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   Window win;

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      printf("Error: XOpenDisplay failed\n");
      return 1;
   }

   win = MakeWindow(dpy, 300, 300);

   DrawFrames(dpy, win);

   return 0;
}
