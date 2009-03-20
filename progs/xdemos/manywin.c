/*
 * Create N GLX windows/contexts and render to them in round-robin order.
 * Also, have the contexts share all texture objects.
 * Press 'd' to delete a texture, 'u' to unbind it.
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>


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


#define MAX_HEADS 200
static struct head Heads[MAX_HEADS];
static int NumHeads = 0;
static GLboolean SwapSeparate = GL_TRUE;
static GLuint TexObj = 0;


static void
Error(const char *display, const char *msg)
{
   fprintf(stderr, "Error on display %s - %s\n", XDisplayName(display), msg);
   exit(1);
}


static struct head *
AddHead(const char *displayName, const char *name)
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
   int width = 90, height = 90;
   int xpos = 0, ypos = 0;

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
   xpos = (NumHeads % 10) * 100;
   ypos = (NumHeads / 10) * 100;
   printf("%d, %d\n", xpos, ypos);
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(dpy, root, xpos, ypos, width, height,
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
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   if (NumHeads == 0) {
      ctx = glXCreateContext(dpy, visinfo, NULL, True);
   }
   else {
      /* share textures & dlists with 0th context */
      printf("sharing\n");
      ctx = glXCreateContext(dpy, visinfo, Heads[0].Context, True);
   }
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

   if (NumHeads == 0) {
      /* create texture object now */
      static const GLubyte checker[2][2][4] = {
         { {255, 255, 255, 255}, {  0,   0,   0, 255} },
         { {  0,   0,   0,   0}, {255, 255, 255, 255} }
      };
      glGenTextures(1, &TexObj);
      assert(TexObj);
      glBindTexture(GL_TEXTURE_2D, TexObj);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, checker);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }
   else {
      /* bind 0th context's texture in this context too */
      assert(TexObj);
      glBindTexture(GL_TEXTURE_2D, TexObj);
   }
   glEnable(GL_TEXTURE_2D);

   /* save the info for this head */
   {
      struct head *h = &Heads[NumHeads];
      strcpy(h->DisplayName, name);
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
DestroyHeads(void)
{
   int i;
   for (i = 0; i < NumHeads; i++) {
      XDestroyWindow(Heads[i].Dpy, Heads[i].Win);
      glXDestroyContext(Heads[i].Dpy, Heads[i].Context);
      XCloseDisplay(Heads[i].Dpy);
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
   glTexCoord2f(0.5, 1.0);   glVertex2f(0, 0.8);
   glTexCoord2f(0.0, 0.0);   glVertex2f(-0.8, -0.7);
   glTexCoord2f(1.0, 0.0);   glVertex2f(0.8, -0.7);
   glEnd();
   glPopMatrix();

   if (!SwapSeparate)
      glXSwapBuffers(h->Dpy, h->Win);
}


static void
Swap(struct head *h)
{
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
                     if (SwapSeparate)
                        Swap(h);
                     break;
                  case ConfigureNotify:
                     Resize(h, event.xconfigure.width, event.xconfigure.height);
                     break;
                  case KeyPress:
                     {
                        char buf[100];
                        KeySym keySym;
                        XComposeStatus stat;
                        XLookupString(&event.xkey, buf, sizeof(buf), &keySym, &stat);
                        switch (keySym) {
                           case XK_Escape:
                              return;
                              break;
                           case XK_d:
                           case XK_D:
                              printf("Delete Texture in window %d\n", i);
                              glXMakeCurrent(h->Dpy, h->Win, h->Context);
                              glDeleteTextures(1, &TexObj);
                              break;
                           case XK_u:
                           case XK_U:
                              printf("Unbind Texture in window %d\n", i);
                              glXMakeCurrent(h->Dpy, h->Win, h->Context);
                              glBindTexture(GL_TEXTURE_2D, 0);
                              break;
                        }
                     }
                     break;
                  default:
                     /*no-op*/ ;
               }
            }
            else {
               printf("window mismatch\n");
            }
         }
      }

      /* redraw all windows */
      for (i = 0; i < NumHeads; i++) {
         Redraw(&Heads[i]);
      }
      /* swapbuffers on all windows, if not already done */
      if (SwapSeparate) {
         for (i = 0; i < NumHeads; i++) {
            Swap(&Heads[i]);
         }
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
   char *dpyName = NULL;
   int i;

   if (argc == 1) {
      printf("manywin: open N simultaneous glx windows\n");
      printf("Usage:\n");
      printf("  manywin [-s] numWindows\n");
      printf("Options:\n");
      printf("  -s = swap immediately after drawing (see src code)\n");
      printf("Example:\n");
      printf("  manywin 10\n");
      return 0;
   }
   else {
      int n = 3;
      for (i = 1; i < argc; i++) {
         if (strcmp(argv[i], "-s") == 0) {
            SwapSeparate = GL_FALSE;
         }
         else if (strcmp(argv[i], "-display") == 0 && i < argc) {
            dpyName = argv[i+1];
            i++;
         }
         else {
            n = atoi(argv[i]);
         }
      }
      if (n < 1)
         n = 1;

      printf("%d windows\n", n);
      for (i = 0; i < n; i++) {
         char name[100];
         struct head *h;
         sprintf(name, "%d", i);
         h = AddHead(dpyName, name);
         if (h) {
            PrintInfo(h);
         }
      }
   }

   EventLoop();
   DestroyHeads();
   return 0;
}
