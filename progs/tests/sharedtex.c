
/*
 * Test sharing of display lists and texture objects between GLX contests.
 * Brian Paul
 * Summer 2000
 *
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
#include <unistd.h>
#include <string.h>


struct window {
   char DisplayName[1000];
   Display *Dpy;
   Window Win;
   GLXContext Context;
   float Angle;
   int Id;
};


#define MAX_WINDOWS 20
static struct window Windows[MAX_WINDOWS];
static int NumWindows = 0;


static GLuint Textures[3];
static GLuint CubeList;



static void
Error(const char *display, const char *msg)
{
   fprintf(stderr, "Error on display %s - %s\n", display, msg);
   exit(1);
}


static struct window *
AddWindow(const char *displayName, int xpos, int ypos,
          const struct window *shareWindow)
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
                    GLX_DEPTH_SIZE, 1,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visinfo;
   int width = 300, height = 300;

   if (NumWindows >= MAX_WINDOWS)
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
      XSetStandardProperties(dpy, win, displayName, displayName,
                              None, (char **)NULL, 0, &sizehints);
   }


   ctx = glXCreateContext(dpy, visinfo,
                          shareWindow ? shareWindow->Context : NULL,
                          True);
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

   /* save the info for this window */
   {
      static int id = 0;
      struct window *h = &Windows[NumWindows];
      strcpy(h->DisplayName, displayName);
      h->Dpy = dpy;
      h->Win = win;
      h->Context = ctx;
      h->Angle = 0.0;
      h->Id = id++;
      NumWindows++;
      return &Windows[NumWindows-1];
   }

}


static void
InitGLstuff(struct window *h)
{
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed in InitGLstuff");
      return;
   }

   glGenTextures(3, Textures);

   /* setup first texture object */
   {
      GLubyte image[16][16][4];
      GLint i, j;
      glBindTexture(GL_TEXTURE_2D, Textures[0]);

      /* red/white checkerboard */
      for (i = 0; i < 16; i++) {
         for (j = 0; j < 16; j++) {
            if ((i ^ j) & 1) {
               image[i][j][0] = 255;
               image[i][j][1] = 255;
               image[i][j][2] = 255;
               image[i][j][3] = 255;
            }
            else {
               image[i][j][0] = 255;
               image[i][j][1] = 0;
               image[i][j][2] = 0;
               image[i][j][3] = 255;
            }
         }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }

   /* setup second texture object */
   {
      GLubyte image[8][8][3];
      GLint i, j;
      glBindTexture(GL_TEXTURE_2D, Textures[1]);

      /* green/yellow checkerboard */
      for (i = 0; i < 8; i++) {
         for (j = 0; j < 8; j++) {
            if ((i ^ j) & 1) {
               image[i][j][0] = 0;
               image[i][j][1] = 255;
               image[i][j][2] = 0;
            }
            else {
               image[i][j][0] = 255;
               image[i][j][1] = 255;
               image[i][j][2] = 0;
            }
         }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }

   /* setup second texture object */
   {
      GLubyte image[4][4][3];
      GLint i, j;
      glBindTexture(GL_TEXTURE_2D, Textures[2]);

      /* blue/gray checkerboard */
      for (i = 0; i < 4; i++) {
         for (j = 0; j < 4; j++) {
            if ((i ^ j) & 1) {
               image[i][j][0] = 0;
               image[i][j][1] = 0;
               image[i][j][2] = 255;
            }
            else {
               image[i][j][0] = 200;
               image[i][j][1] = 200;
               image[i][j][2] = 200;
            }
         }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }

   /* Now make the cube object display list */
   CubeList = glGenLists(1);
   glNewList(CubeList, GL_COMPILE);
   {
      glBindTexture(GL_TEXTURE_2D, Textures[0]);
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
      glTexCoord2f(1, 0);  glVertex3f(-1,  1, -1);
      glTexCoord2f(1, 1);  glVertex3f(-1,  1,  1);
      glTexCoord2f(0, 1);  glVertex3f(-1, -1,  1);
      glEnd();
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(1, -1, -1);
      glTexCoord2f(1, 0);  glVertex3f(1,  1, -1);
      glTexCoord2f(1, 1);  glVertex3f(1,  1,  1);
      glTexCoord2f(0, 1);  glVertex3f(1, -1,  1);
      glEnd();

      glBindTexture(GL_TEXTURE_2D, Textures[1]);
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
      glTexCoord2f(1, 0);  glVertex3f( 1, -1, -1);
      glTexCoord2f(1, 1);  glVertex3f( 1, -1,  1);
      glTexCoord2f(0, 1);  glVertex3f(-1, -1,  1);
      glEnd();
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(-1, 1, -1);
      glTexCoord2f(1, 0);  glVertex3f( 1, 1, -1);
      glTexCoord2f(1, 1);  glVertex3f( 1, 1,  1);
      glTexCoord2f(0, 1);  glVertex3f(-1, 1,  1);
      glEnd();

      glBindTexture(GL_TEXTURE_2D, Textures[2]);
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
      glTexCoord2f(1, 0);  glVertex3f( 1, -1, -1);
      glTexCoord2f(1, 1);  glVertex3f( 1,  1, -1);
      glTexCoord2f(0, 1);  glVertex3f(-1,  1, -1);
      glEnd();
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex3f(-1, -1, 1);
      glTexCoord2f(1, 0);  glVertex3f( 1, -1, 1);
      glTexCoord2f(1, 1);  glVertex3f( 1,  1, 1);
      glTexCoord2f(0, 1);  glVertex3f(-1,  1, 1);
      glEnd();
   }
   glEndList();

   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_VENDOR: %s\n", (char *) glGetString(GL_VENDOR));
}



static void
Redraw(struct window *h)
{
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed");
      printf("glXMakeCurrent failed in Redraw()\n");
      return;
   }

   h->Angle += 1.0;

   glShadeModel(GL_FLAT);
   glClearColor(0.25, 0.25, 0.25, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glColor3f(1, 1, 1);

   glPushMatrix();
      if (h->Id == 0)
         glRotatef(h->Angle, 0, 1, -1);
      else if (h->Id == 1)
         glRotatef(-(h->Angle), 0, 1, -1);
      else if (h->Id == 2)
         glRotatef(h->Angle, 0, 1, 1);
      else if (h->Id == 3)
         glRotatef(-(h->Angle), 0, 1, 1);
      glCallList(CubeList);
   glPopMatrix();

   glXSwapBuffers(h->Dpy, h->Win);
}



static void
Resize(const struct window *h, unsigned int width, unsigned int height)
{
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Resize()");
      return;
   }
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 2, 10);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -4.5);
}



static void
EventLoop(void)
{
   while (1) {
      int i;
      for (i = 0; i < NumWindows; i++) {
         struct window *h = &Windows[i];
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


#if 0
static void
PrintInfo(const struct window *h)
{
   printf("Name: %s\n", h->DisplayName);
   printf("  Display:     %p\n", (void *) h->Dpy);
   printf("  Window:      0x%x\n", (int) h->Win);
   printf("  Context:     0x%x\n", (int) h->Context);
}
#endif


int
main(int argc, char *argv[])
{
   const char *dpyName = XDisplayName(NULL);

   struct window *h0;

   /* four windows and contexts sharing display lists and texture objects */
   h0 = AddWindow(dpyName,  10,  10, NULL);
   (void) AddWindow(dpyName, 330,  10, h0);
   (void) AddWindow(dpyName,  10, 350, h0);
   (void) AddWindow(dpyName, 330, 350, h0);

   InitGLstuff(h0);

   EventLoop();
   return 0;
}
