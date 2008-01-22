
/*
 * Example of using the X "shape" extension with OpenGL:  render a spinning
 * cube inside of a non-rectangular window.
 *
 * Press ESC to exit.  Press up/down to change window shape.
 *
 * To compile add "shape" to the PROGS list in Makefile.
 *
 * Brian Paul
 * June 16, 1997
 *
 * This program is in the public domain.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>
#include <GL/glx.h>

#ifndef PI
#define PI 3.1415926
#endif


static int Width=500, Height=500;

static float Xangle = 0.0, Yangle = 0.0;
static int Sides = 5;
static int MinSides = 3;
static int MaxSides = 20;


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


/*
 * Draw the OpenGL stuff and do a SwapBuffers.
 */
static void display(Display *dpy, Window win)
{
   float scale = 1.7;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();

   glScalef(scale, scale, scale);
   glRotatef(Xangle, 1.0, 0.0, 0.0);
   glRotatef(Yangle, 0.0, 1.0, 0.0);

   /*
    * wireframe box
    */
   glColor3f(1.0, 1.0, 1.0);
   glBegin(GL_LINE_LOOP);
   glVertex3f(-1.0, -1.0, -1.0);
   glVertex3f( 1.0, -1.0, -1.0);
   glVertex3f( 1.0,  1.0, -1.0);
   glVertex3f(-1.0,  1.0, -1.0);
   glEnd();

   glBegin(GL_LINE_LOOP);
   glVertex3f(-1.0, -1.0, 1.0);
   glVertex3f( 1.0, -1.0, 1.0);
   glVertex3f( 1.0,  1.0, 1.0);
   glVertex3f(-1.0,  1.0, 1.0);
   glEnd();

   glBegin(GL_LINES);
   glVertex3f(-1.0, -1.0, -1.0);   glVertex3f(-1.0, -1.0, 1.0);
   glVertex3f( 1.0, -1.0, -1.0);   glVertex3f( 1.0, -1.0, 1.0);
   glVertex3f( 1.0,  1.0, -1.0);   glVertex3f( 1.0,  1.0, 1.0);
   glVertex3f(-1.0,  1.0, -1.0);   glVertex3f(-1.0,  1.0, 1.0);
   glEnd();

   /*
    * Solid box
    */
   glPushMatrix();
   glScalef(0.75, 0.75, 0.75);

   glColor3f(1, 0, 0);
   glBegin(GL_POLYGON);
   glVertex3f(1, -1, -1);
   glVertex3f(1,  1, -1);
   glVertex3f(1,  1,  1);
   glVertex3f(1, -1,  1);
   glEnd();

   glColor3f(0, 1, 1);
   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, -1);
   glVertex3f(-1,  1, -1);
   glVertex3f(-1,  1,  1);
   glVertex3f(-1, -1,  1);
   glEnd();

   glColor3f(0, 1, 0);
   glBegin(GL_POLYGON);
   glVertex3f(-1, 1, -1);
   glVertex3f( 1, 1, -1);
   glVertex3f( 1, 1,  1);
   glVertex3f(-1, 1,  1);
   glEnd();

   glColor3f(1, 0, 1);
   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, -1);
   glVertex3f( 1, -1, -1);
   glVertex3f( 1, -1,  1);
   glVertex3f(-1, -1,  1);
   glEnd();

   glColor3f(0, 0, 1);
   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, 1);
   glVertex3f( 1, -1, 1);
   glVertex3f( 1,  1, 1);
   glVertex3f(-1,  1, 1);
   glEnd();

   glColor3f(1, 1, 0);
   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, -1);
   glVertex3f( 1, -1, -1);
   glVertex3f( 1,  1, -1);
   glVertex3f(-1,  1, -1);
   glEnd();
   glPopMatrix();


   glPopMatrix();

   glXSwapBuffers(dpy, win);
}


/*
 * This is called when we have to recompute the window shape bitmask.
 * We just generate an n-sided regular polygon here but any other shape
 * would be possible.
 */
static void make_shape_mask(Display *dpy, Window win, int width, int height,
                            int sides)
{
   Pixmap shapeMask;
   XGCValues xgcv;
   GC gc;

   /* allocate 1-bit deep pixmap and a GC */
   shapeMask = XCreatePixmap(dpy, win, width, height, 1);
   gc = XCreateGC(dpy, shapeMask, 0, &xgcv);

   /* clear shapeMask to zeros */
   XSetForeground(dpy, gc, 0);
   XFillRectangle(dpy, shapeMask, gc, 0, 0, width, height);

   /* draw mask */
   XSetForeground(dpy, gc, 1);
   {
      int cx = width / 2;
      int cy = height / 2;
      float angle = 0.0;
      float step = 2.0 * PI / sides;
      float radius = width / 2;
      int i;
      XPoint points[100];
      for (i=0;i<sides;i++) {
         int x = cx + radius * sin(angle);
         int y = cy - radius * cos(angle);
         points[i].x = x;
         points[i].y = y;
         angle += step;
      }
      XFillPolygon(dpy, shapeMask, gc, points, sides, Convex, CoordModeOrigin);
   }

   /* This is the only SHAPE extension call- simple! */
   XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, shapeMask, ShapeSet);

   XFreeGC(dpy, gc);
   XFreePixmap(dpy, shapeMask);
}


/*
 * Called when window is resized.  Do OpenGL viewport and projection stuff.
 */
static void reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 3.0, 20.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);

   glEnable(GL_DEPTH_TEST);
}


/*
 * Process X events.
 */
static void event_loop(Display *dpy, Window win)
{
   while (1) {
      XEvent event;
      if (XPending(dpy)) {
         XNextEvent(dpy, &event);
         switch (event.type) {
            case Expose:
               display(dpy, event.xexpose.window);
               break;
            case ConfigureNotify:
               Width = event.xconfigure.width;
               Height = event.xconfigure.height,
               make_shape_mask(dpy, win, Width, Height, Sides);
               reshape(Width, Height);
               break;
            case KeyPress:
               {
                  char buf[100];
                  KeySym keySym;
                  XComposeStatus stat;
                  XLookupString(&event.xkey, buf, sizeof(buf), &keySym, &stat);
                  switch (keySym) {
                     case XK_Escape:
                        exit(0);
                        break;
                     case XK_Up:
                        Sides++;
                        if (Sides>MaxSides) Sides = MaxSides;
                        make_shape_mask(dpy, win, Width, Height, Sides);
                        break;
                     case XK_Down:
                        Sides--;
                        if (Sides<MinSides) Sides = MinSides;
                        make_shape_mask(dpy, win, Width, Height, Sides);
                        break;
                  }
               }
               break;
            default:
               ;;
         }
      }
      else {
         static double t0 = -1.0;
         double dt, t = current_time();
         if (t0 < 0.0)
            t0 = t;
         dt = t - t0;
         Xangle += 90.0 * dt;  /* 90 degrees per second */
         Yangle += 70.0 * dt;
         t0 = t;
         display(dpy, win);
      }
   }
}


/*
 * Allocate a "nice" colormap.  This could be better (HP-CR support, etc).
 */
static Colormap alloc_colormap(Display *dpy, Window parent, Visual *vis)
{
   Screen *scr = DefaultScreenOfDisplay(dpy);
   int scrnum = DefaultScreen(dpy);

   if (MaxCmapsOfScreen(scr)==1 && vis==DefaultVisual(dpy, scrnum)) {
      /* The window and root are of the same visual type so */
      /* share the root colormap. */
      return DefaultColormap(dpy, scrnum);
   }
   else {
      return XCreateColormap(dpy, parent, vis, AllocNone);
   }
}


int main(int argc, char *argv[])
{
   static int glAttribs[] = {
      GLX_DOUBLEBUFFER,
      GLX_RGBA,
      GLX_DEPTH_SIZE, 1,
      None
   };
   Display *dpy;
   XVisualInfo *visInfo;
   int scrn;
   Window root;
   Colormap cmap;
   Window win;
   XSetWindowAttributes winAttribs;
   unsigned long winAttribsMask;
   GLXContext glCtx;
   int ignore;
   const char *name = "OpenGL in a Shaped Window";

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      fprintf(stderr, "Couldn't open default display\n");
      return 1;
   }

   /* check that we can use the shape extension */
   if (!XQueryExtension(dpy, "SHAPE", &ignore, &ignore, &ignore )) {
      fprintf(stderr, "Display doesn't support shape extension\n");
      return 1;
   }

   scrn = DefaultScreen(dpy);

   root = RootWindow(dpy, scrn);

   visInfo = glXChooseVisual(dpy, scrn, glAttribs);
   if (!visInfo) {
      fprintf(stderr, "Couldn't get RGB, DB, Z visual\n");
      return 1;
   }

   glCtx = glXCreateContext(dpy, visInfo, 0, True);
   if (!glCtx) {
      fprintf(stderr, "Couldn't create GL context\n");
      return 1;
   }

   cmap = alloc_colormap(dpy, root, visInfo->visual);
   if (!cmap) {
      fprintf(stderr, "Couln't create colormap\n");
      return 1;
   }

   winAttribs.border_pixel = 0;
   winAttribs.colormap = cmap;
   winAttribs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   winAttribsMask = CWBorderPixel | CWColormap | CWEventMask;
   win = XCreateWindow(dpy, root, 0, 0, Width, Height, 0,
                       visInfo->depth, InputOutput,
                       visInfo->visual,
                       winAttribsMask, &winAttribs);

   {
      XSizeHints sizehints;
      /*
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      */
      sizehints.flags = 0;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }


   XMapWindow(dpy, win);

   glXMakeCurrent(dpy, win, glCtx);

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("Press ESC to exit.\n");
   printf("Press up/down to change window shape.\n");

   event_loop(dpy, win);

   return 0;
}
