/**
 * Example of cooperative rendering into one window by two processes.
 * The first instance of the program creates the GLX window.
 * The second instance of the program gets the window ID from the first
 * and draws into it.
 * Socket IPC is used for synchronization.
 *
 * Usage:
 * 1. run 'corender &'
 * 2. run 'corender 2'  (any arg will do)
 *
 * Brian Paul
 * 11 Oct 2007
 */


#include <GL/gl.h>
#include <GL/glx.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>
#include <unistd.h>
#include "ipc.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int MyID = 0;  /* 0 or 1 */
static int WindowID = 0;
static GLXContext Context = 0;
static int Width = 700, Height = 350;
static int Rot = 0;
static int Sock = 0;

static GLfloat Red[4] = {1.0, 0.2, 0.2, 1.0};
static GLfloat Blue[4] = {0.2, 0.2, 1.0, 1.0};

static int Sync = 1;  /** synchronized rendering? */


static void
setup_ipc(void)
{
   int k, port = 10001;

   if (MyID == 0) {
      /* I'm the first one, wait for connection from second */
      k = CreatePort(&port);
      assert(k != -1);

      printf("Waiting for connection from another 'corender'\n");
      Sock = AcceptConnection(k);

      printf("Got connection, sending windowID\n");

      /* send windowID */
      SendData(Sock, &WindowID, sizeof(WindowID));
   }
   else {
      /* I'm the second one, connect to first */
      char hostname[1000];

      MyHostName(hostname, 1000);
      Sock = Connect(hostname, port);
      assert(Sock != -1);

      /* get windowID */
      ReceiveData(Sock, &WindowID, sizeof(WindowID));
      printf("Contacted first 'corender', getting WindowID\n");
   }
}



/** from GLUT */
static void
doughnut(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
  int i, j;
  GLfloat theta, phi, theta1;
  GLfloat cosTheta, sinTheta;
  GLfloat cosTheta1, sinTheta1;
  GLfloat ringDelta, sideDelta;

  ringDelta = 2.0 * M_PI / rings;
  sideDelta = 2.0 * M_PI / nsides;

  theta = 0.0;
  cosTheta = 1.0;
  sinTheta = 0.0;
  for (i = rings - 1; i >= 0; i--) {
    theta1 = theta + ringDelta;
    cosTheta1 = cos(theta1);
    sinTheta1 = sin(theta1);
    glBegin(GL_QUAD_STRIP);
    phi = 0.0;
    for (j = nsides; j >= 0; j--) {
      GLfloat cosPhi, sinPhi, dist;

      phi += sideDelta;
      cosPhi = cos(phi);
      sinPhi = sin(phi);
      dist = R + r * cosPhi;

      glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
      glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
      glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
      glVertex3f(cosTheta * dist, -sinTheta * dist,  r * sinPhi);
    }
    glEnd();
    theta = theta1;
    cosTheta = cosTheta1;
    sinTheta = sinTheta1;
  }
}


static void
redraw(Display *dpy)
{
   int dbg = 0;

   glXMakeCurrent(dpy, WindowID, Context);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glClearColor(0.5, 0.5, 0.5, 0.0);

   if (MyID == 0) {
      /* First process */

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glPushMatrix();
      glTranslatef(-1, 0, 0);
      glRotatef(Rot, 1, 0, 0);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Red);
      doughnut(0.5, 2.0, 20, 30);
      glPopMatrix();

      glFinish();
      if (!Sync) {
         usleep(1000*10);
      }

      /* signal second process to render */
      if (Sync) {
         int code = 1;
         if (dbg) printf("0: send signal\n");
         SendData(Sock, &code, sizeof(code));
         SendData(Sock, &Rot, sizeof(Rot));
      }

      /* wait for second process to finish rendering */
      if (Sync) {
         int code = 0;
         if (dbg) printf("0: wait signal\n");
         ReceiveData(Sock, &code, sizeof(code));
         if (dbg) printf("0: got signal\n");
         assert(code == 2);
      }

   }
   else {
      /* Second process */

      /* wait for first process's signal for me to render */
      if (Sync) {
         int code = 0;
         if (dbg) printf("1: wait signal\n");
         ReceiveData(Sock, &code, sizeof(code));
         ReceiveData(Sock, &Rot, sizeof(Rot));

         if (dbg) printf("1: got signal\n");
         assert(code == 1);
      }

      /* XXX this clear should not be here, but for some reason, it
       * makes things _mostly_ work correctly w/ NVIDIA's driver.
       * There's only occasional glitches.
       * Without this glClear(), depth buffer for the second process
       * is pretty much broken.
       */
      //glClear(GL_DEPTH_BUFFER_BIT);

      glPushMatrix();
      glTranslatef(1, 0, 0);
      glRotatef(Rot + 90 , 1, 0, 0);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Blue);
      doughnut(0.5, 2.0, 20, 30);
      glPopMatrix();
      glFinish();

      glXSwapBuffers(dpy, WindowID);
      usleep(1000*10);

      /* signal first process that I'm done rendering */
      if (Sync) {
         int code = 2;
         if (dbg) printf("1: send signal\n");
         SendData(Sock, &code, sizeof(code));
      }
   }
}


static void
resize(Display *dpy, int width, int height)
{
   float ar = (float) width / height;

   glXMakeCurrent(dpy, WindowID, Context);

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, 1.0, -1.0, 5.0, 200.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -15);

   Width = width;
   Height = height;
}



static void
set_window_title(Display *dpy, Window win, const char *title)
{
   XSizeHints sizehints;
   sizehints.flags = 0;
   XSetStandardProperties(dpy, win, title, title,
                          None, (char **)NULL, 0, &sizehints);
}


static Window
make_gl_window(Display *dpy, XVisualInfo *visinfo, int width, int height)
{
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   int x = 0, y = 0;
   char *name = NULL;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, x, y, width, height,
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

   return win;
}


static void
set_event_mask(Display *dpy, Window win)
{
   XSetWindowAttributes attr;
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   XChangeWindowAttributes(dpy, win, CWEventMask, &attr);
}


static void
event_loop(Display *dpy)
{
   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);

         switch (event.type) {
         case Expose:
            redraw(dpy);
            break;
         case ConfigureNotify:
            resize(dpy, event.xconfigure.width, event.xconfigure.height);
            break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
               if (code == XK_Left) {
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == 27) {
                     exit(0);
                  }
               }
            }
         default:
            /* nothing */
            ;
         }
      }

      if (MyID == 0 || !Sync)
         Rot += 1;
      redraw(dpy);
   }
}


static XVisualInfo *
choose_visual(Display *dpy)
{
   int attribs[] = { GLX_RGBA,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DOUBLEBUFFER,
                     GLX_DEPTH_SIZE, 1,
                     None };
   int scrnum = DefaultScreen( dpy );
   return glXChooseVisual(dpy, scrnum, attribs);
}


static void
parse_opts(int argc, char *argv[])
{
   if (argc > 1) {
      MyID = 1;
   }
}


int
main( int argc, char *argv[] )
{
   Display *dpy;
   XVisualInfo *visinfo;

   parse_opts(argc, argv);

   dpy = XOpenDisplay(NULL);

   visinfo = choose_visual(dpy);

   Context = glXCreateContext( dpy, visinfo, NULL, True );
   if (!Context) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   if (MyID == 0) {
      WindowID = make_gl_window(dpy, visinfo, Width, Height);
      set_window_title(dpy, WindowID, "corender");
      XMapWindow(dpy, WindowID);
      /*printf("WindowID 0x%x\n", (int) WindowID);*/
   }

   /* do ipc hand-shake here */
   setup_ipc();
   assert(Sock);
   assert(WindowID);

   if (MyID == 1) {
      set_event_mask(dpy, WindowID);
   }

   resize(dpy, Width, Height);

   event_loop(dpy);

   return 0;
}
