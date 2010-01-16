/* $Id: sharedtex.c,v 1.2 2002/01/16 14:32:46 joukj Exp $ */

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
 *
 *
 * Modified 2009 for multithreading by Thomas Hellstrom.
 */


#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <X11/X.h>

struct thread_init_arg {
   int id;
};

struct window {
   pthread_mutex_t drawMutex;
   char DisplayName[1000];
   Display *Dpy;
   Window Win;
   GLXContext Context;
   float Angle;
   int Id;
   XVisualInfo *visInfo;
};


#define MAX_WINDOWS 20
static struct window Windows[MAX_WINDOWS];
static int NumWindows = 0;
static int terminate = 0;
static GLXContext gCtx;
static Display *gDpy;
static GLuint Textures[3];



static void
Error(const char *display, const char *msg)
{
   fprintf(stderr, "Error on display %s - %s\n", display, msg);
   exit(1);
}


static int
initMainthread(Display *dpy, const char *displayName)
{
   int scrnum;
   XVisualInfo *visinfo;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
                    GLX_DEPTH_SIZE, 1,
		    None };

   scrnum = DefaultScreen(dpy);
   visinfo = glXChooseVisual(dpy, scrnum, attrib);
   if (!visinfo) {
      Error(displayName, "Unable to find RGB, double-buffered visual");
      return -1;
   }
   gCtx = glXCreateContext(dpy, visinfo, NULL, True);
   if (!gCtx) {
      Error(displayName, "Couldn't create GLX context");
      return -1;
   }
   return 0;
}

static struct window *
AddWindow(Display *dpy, const char *displayName, int xpos, int ypos,
          GLXContext sCtx)
{
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
   mask = CWBorderPixel | CWColormap | CWEventMask;

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
                          sCtx ? sCtx : NULL, True);

   if (!ctx) {
      Error(displayName, "Couldn't create GLX context");
      return NULL;
   }

   XMapWindow(dpy, win);

   /* save the info for this window */
   {
      static int id = 0;
      struct window *h = &Windows[NumWindows];
      if (strlen(displayName) + 1 > sizeof(h->DisplayName)) {
         Error(displayName, "string overflow");
         return NULL;
      }
      strcpy(h->DisplayName, displayName);
      h->Dpy = dpy;
      h->Win = win;
      h->Context = ctx;
      h->Angle = 0.0;
      h->Id = id++;
      h->visInfo = visinfo;
      pthread_mutex_init(&h->drawMutex, NULL);
      NumWindows++;
      return &Windows[NumWindows-1];
   }
}


static void
InitGLstuff(void)

{
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

   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_VENDOR: %s\n", (char *) glGetString(GL_VENDOR));
}

static void
Redraw(struct window *h)
{
   pthread_mutex_lock(&h->drawMutex);
   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Redraw");
      pthread_mutex_unlock(&h->drawMutex);
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

   glPopMatrix();

   glXSwapBuffers(h->Dpy, h->Win);

   if (!glXMakeCurrent(h->Dpy, None, NULL)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Redraw");
   }
   pthread_mutex_unlock(&h->drawMutex);
}

static void *threadRunner (void *arg)
{
   struct thread_init_arg *tia = (struct thread_init_arg *) arg;
   struct window *win;

   win = &Windows[tia->id];

   while(!terminate) {
      usleep(1000);
      Redraw(win);
   }

   return NULL;
}

static void
Resize(struct window *h, unsigned int width, unsigned int height)
{
   pthread_mutex_lock(&h->drawMutex);

   if (!glXMakeCurrent(h->Dpy, h->Win, h->Context)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Resize()");
      pthread_mutex_unlock(&h->drawMutex);
      return;
   }

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 2, 10);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -4.5);
   if (!glXMakeCurrent(h->Dpy, None, NULL)) {
      Error(h->DisplayName, "glXMakeCurrent failed in Resize()");
   }
   pthread_mutex_unlock(&h->drawMutex);
}


static void
EventLoop(void)
{
   while (1) {
      int i;
      XEvent event;
      XNextEvent(gDpy, &event);
      for (i = 0; i < NumWindows; i++) {
	 struct window *h = &Windows[i];
	 if (event.xany.window == h->Win) {
	    switch (event.type) {
	    case Expose:
	       Redraw(h);
	       break;
	    case ConfigureNotify:
	       Resize(h, event.xconfigure.width, event.xconfigure.height);
	       break;
	    case KeyPress:
	       terminate = 1;
	       return;
	    default:
	       /*no-op*/ ;
	    }
	 }
      }
   }
}

int
main(int argc, char *argv[])
{
   const char *dpyName = XDisplayName(NULL);
   pthread_t t0, t1, t2, t3;
   struct thread_init_arg tia0, tia1, tia2, tia3;
   struct window *h0;

   XInitThreads();

   gDpy = XOpenDisplay(dpyName);
   if (!gDpy) {
      Error(dpyName, "Unable to open display");
      return -1;
   }

   if (initMainthread(gDpy, dpyName))
      return -1;

   /* four windows and contexts sharing display lists and texture objects */
   h0 = AddWindow(gDpy, dpyName,  10,  10, gCtx);
   (void) AddWindow(gDpy, dpyName, 330,  10, gCtx);
   (void) AddWindow(gDpy, dpyName,  10, 350, gCtx);
   (void) AddWindow(gDpy, dpyName, 330, 350, gCtx);

   if (!glXMakeCurrent(gDpy, h0->Win, gCtx)) {
      Error(dpyName, "glXMakeCurrent failed for init thread.");
      return -1;
   }

   InitGLstuff();

   tia0.id = 0;
   pthread_create(&t0, NULL, threadRunner, &tia0);
   tia1.id = 1;
   pthread_create(&t1, NULL, threadRunner, &tia1);
   tia2.id = 2;
   pthread_create(&t2, NULL, threadRunner, &tia2);
   tia3.id = 3;
   pthread_create(&t3, NULL, threadRunner, &tia3);
   EventLoop();
   return 0;
}
