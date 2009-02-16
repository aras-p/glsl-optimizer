/*
 * GLX overlay test/demo.
 *
 * Brian Paul
 * 18 July 2005
 */

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/keysym.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int WinWidth = 300, WinHeight = 300;
static Window NormalWindow = 0;
static Window OverlayWindow = 0;
static GLXContext NormalContext = 0;
static GLXContext OverlayContext = 0;
static GLboolean RGBOverlay = GL_FALSE;
static GLfloat Angle = 0.0;


static void
RedrawNormal(Display *dpy)
{
   glXMakeCurrent(dpy, NormalWindow, NormalContext);
   glViewport(0, 0, WinWidth, WinHeight);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glColor3f(1.0, 1.0, 0.0);
   glPushMatrix();
   glRotatef(Angle, 0, 0, 1);
   glRectf(-0.8, -0.8, 0.8, 0.8);
   glPopMatrix();
   glXSwapBuffers(dpy, NormalWindow);
}


static void
RedrawOverlay(Display *dpy)
{
   glXMakeCurrent(dpy, OverlayWindow, OverlayContext);
   glViewport(0, 0, WinWidth, WinHeight);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glClear(GL_COLOR_BUFFER_BIT);
   if (RGBOverlay) {
      glColor3f(0.0, 1.0, 1.0);
   }
   else {
      glIndexi(2);
   }
   glBegin(GL_LINES);
   glVertex2f(-1, -1);
   glVertex2f(1, 1);
   glVertex2f(1, -1);
   glVertex2f(-1, 1);
   glEnd();
   glXSwapBuffers(dpy, OverlayWindow);
}


static Window
MakeWindow(Display *dpy, XVisualInfo *visinfo, Window parent,
             unsigned int width, unsigned int height)
{
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;

   scrnum = DefaultScreen(dpy);
   root = RootWindow(dpy, scrnum);

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(dpy, parent, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr);
   return win;
}


static void
MakeNormalWindow(Display *dpy)
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   Window root;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen(dpy);
   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attrib);
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   NormalWindow = MakeWindow(dpy, visinfo, root, WinWidth, WinHeight);
   assert(NormalWindow);

   NormalContext = glXCreateContext(dpy, visinfo, NULL, True);
   assert(NormalContext);
}


static void
MakeOverlayWindow(Display *dpy)
{
   int rgbAttribs[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      GLX_LEVEL, 1,
      None
   };
   int indexAttribs[] = {
      /*GLX_RGBA, leave this out */
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      GLX_LEVEL, 1,
      None
   };
   int scrnum;
   Window root;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen(dpy);
   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, rgbAttribs);
   if (visinfo) {
      printf("Found RGB overlay visual 0x%x\n", (int) visinfo->visualid);
      RGBOverlay = GL_TRUE;
   }
   else {
      visinfo = glXChooseVisual(dpy, scrnum, indexAttribs);
      if (visinfo) {
         printf("Found Color Index overlay visual 0x%x\n",
                (int) visinfo->visualid);
         /* XXX setup the colormap entries! */
      }
      else {
         printf("Couldn't get an overlay visual.\n");
         printf("Your hardware probably doesn't support framebuffer overlay planes.\n");
         exit(1);
      }
   }

   OverlayWindow = MakeWindow(dpy, visinfo, NormalWindow, WinWidth, WinHeight);
   assert(OverlayWindow);

   OverlayContext = glXCreateContext(dpy, visinfo, NULL, True);
   assert(OverlayContext);
}


static void
EventLoop(Display *dpy)
{
   XEvent event;

   while (1) {
      XNextEvent(dpy, &event);

      switch (event.type) {
	 case Expose:
	    RedrawNormal(dpy);
	    RedrawOverlay(dpy);
	    break;
	 case ConfigureNotify:
            WinWidth = event.xconfigure.width;
            WinHeight = event.xconfigure.height;
            if (event.xconfigure.window == NormalWindow)
               XResizeWindow(dpy, OverlayWindow, WinWidth, WinHeight);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
	       r = XLookupString(&event.xkey, buffer, sizeof(buffer),
				 NULL, NULL);
	       if (buffer[0] == 27) {
                  /* escape */
                  return;
               }
               else if (buffer[0] == ' ') {
                  Angle += 5.0;
                  RedrawNormal(dpy);
               }
            }
            break;
         default:
            ; /* nothing */
      }
   }
}


int
main(int argc, char *argv[])
{
   Display *dpy = XOpenDisplay(NULL);

   assert(dpy);

   MakeNormalWindow(dpy);
   MakeOverlayWindow(dpy);

   XMapWindow(dpy, NormalWindow);
   XMapWindow(dpy, OverlayWindow);

   EventLoop(dpy);

   glXDestroyContext(dpy, OverlayContext);
   glXDestroyContext(dpy, NormalContext);
   XDestroyWindow(dpy, OverlayWindow);
   XDestroyWindow(dpy, NormalWindow);

   return 0;
}
