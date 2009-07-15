
/*
 * Exercise multiple GLX connections on multiple X displays.
 * Direct GLX contexts are attempted first, then indirect.
 * Each window will display a spinning green triangle.
 *
 * Copyright (C) 2000  Brian Paul   All Rights Reserved.
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


#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/*
 * Each display/window/context:
 */
struct head {
   char DisplayName[1000];
   Display *Dpy;
   Window Win;
   GLXContext Context;
   float Angle;
   char Renderer[1000];
   char Vendor[1000];
   char Version[1000];
};


#define MAX_HEADS 20
static struct head Heads[MAX_HEADS];
static int NumHeads = 0;


static void
Error(const char *display, const char *msg)
{
   fprintf(stderr, "Error on display %s - %s\n", XDisplayName(display), msg);
   exit(1);
}


static struct head *
AddHead(const char *displayName)
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visinfo;
   int width = 300, height = 300;
   int xpos = 10, ypos = 10;

   if (NumHeads >= MAX_HEADS)
      return NULL;

   dpy = XOpenDisplay(displayName);
   if (!dpy) {
      Error(displayName, "Unable to open display");
      return NULL;
   }

   scrnum = DefaultScreen(dpy);
   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attrib);
   if (!visinfo) {
      Error(displayName, "Unable to find RGB, double-buffered visual");
      return NULL;
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr);
   if (!win) {
      Error(displayName, "Couldn't create window");
      return NULL;
   }

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, displayName, displayName,
                              None, (char **)NULL, 0, &sizehints);
   }


   ctx = glXCreateContext(dpy, visinfo, NULL, True);
   if (!ctx) {
      Error(displayName, "Couldn't create GLX context");
      return NULL;
   }

   XMapWindow(dpy, win);

   if (!glXMakeCurrent(dpy, win, ctx)) {
      Error(displayName, "glXMakeCurrent failed");
      printf("glXMakeCurrent failed in Redraw()\n");
      return NULL;
   }

   /* save the info for this head */
   {
      struct head *h = &Heads[NumHeads];
      strcpy(h->DisplayName, displayName);
      h->Dpy = dpy;
      h->Win = win;
      h->Context = ctx;
      h->Angle = 0.0;
      strcpy(h->Version, (char *) glGetString(GL_VERSION));
      strcpy(h->Vendor, (char *) glGetString(GL_VENDOR));
      strcpy(h->Renderer, (char *) glGetString(GL_RENDERER));
      NumHeads++;
      return &Heads[NumHeads-1];
   }

}


static void
Redraw(struct head *h)
{
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed");
      printf("glXMakeCurrent failed in Redraw()\n");
      return;
   }

   h->Angle += 1.0;

   glShadeModel(GL_FLAT);
   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* draw green triangle */
   glColor3f(0.0, 1.0, 0.0);
   glPushMatrix();
   glRotatef(h->Angle, 0, 0, 1);
   glBegin(GL_TRIANGLES);
   glVertex2f(0, 0.8);
   glVertex2f(-0.8, -0.7);
   glVertex2f(0.8, -0.7);
   glEnd();
   glPopMatrix();

   glXSwapBuffers(h->Dpy, h->Win);
}



static void
Resize(const struct head *h, unsigned int width, unsigned int height)
{
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Resize()");
      return;
   }
   glFlush();
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
}



static void
EventLoop(void)
{
   while (1) {
      int i;
      for (i = 0; i < NumHeads; i++) {
         struct head *h = &Heads[i];
         while (XPending(h->Dpy) > 0) {
            XEvent event;
            XNextEvent(h->Dpy, &event);
            if (event.xany.window == h->Win) {
               switch (event.type) {
                  case Expose:
                     Redraw(h);
                     break;
                  case ConfigureNotify:
                     Resize(h, event.xconfigure.width, event.xconfigure.height);
                     break;
                  case KeyPress:
                     return;
                  default:
                     /*no-op*/ ;
               }
            }
            else {
               printf("window mismatch\n");
            }
         }
         Redraw(h);
      }
      usleep(1);
   }
}



static void
PrintInfo(const struct head *h)
{
   printf("Name: %s\n", h->DisplayName);
   printf("  Display:     %p\n", (void *) h->Dpy);
   printf("  Window:      0x%x\n", (int) h->Win);
   printf("  Context:     0x%lx\n", (long) h->Context);
   printf("  GL_VERSION:  %s\n", h->Version);
   printf("  GL_VENDOR:   %s\n", h->Vendor);
   printf("  GL_RENDERER: %s\n", h->Renderer);
}


int
main(int argc, char *argv[])
{
   int i;
   if (argc == 1) {
      struct head *h;
      printf("glxheads: exercise multiple GLX connections (any key = exit)\n");
      printf("Usage:\n");
      printf("  glxheads xdisplayname ...\n");
      printf("Example:\n");
      printf("  glxheads :0 mars:0 venus:1\n");

      h = AddHead(XDisplayName(NULL));
      if (h)
         PrintInfo(h);
   }
   else {
      for (i = 1; i < argc; i++) {
         const char *name = argv[i];
         struct head *h = AddHead(name);
         if (h) {
            PrintInfo(h);
         }
      }
   }

   EventLoop();
   return 0;
}
