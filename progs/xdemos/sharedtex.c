/*
 * Test sharing of texture objects by two rendering contexts.
 * In particular, test that changing a texture object in one context
 * effects the texture in the second context.
 *
 * Brian Paul
 * 30 Apr 2008
 *
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
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


#define MAX_CONTEXTS 2

#define TEX_SIZE 32

static const char *DisplayName = NULL;
static Display *Dpy;
static XVisualInfo *VisInfo;
static Window Win;
static GLXContext Contexts[MAX_CONTEXTS];
static int WinWidth = 300, WinHeight = 300;

static int DrawContext = 0, TexContext = 1;

static GLuint TexObj = 0;
static GLboolean NewTexture = GL_FALSE;


static void
Error(const char *msg)
{
   fprintf(stderr, "sharedtex error: %s\n", msg);
   exit(1);
}


static void
CreateWindow(const char *name)
{
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
   int xpos = 0, ypos = 0;
   static int n = 0;

   scrnum = DefaultScreen(Dpy);
   root = RootWindow(Dpy, scrnum);

   VisInfo = glXChooseVisual(Dpy, scrnum, attrib);
   if (!VisInfo) {
      Error("Unable to find RGB, double-buffered visual");
   }

   /* window attributes */
   xpos = (n % 10) * 100;
   ypos = (n / 10) * 100;
   n++;

   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(Dpy, root, VisInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   Win = XCreateWindow(Dpy, root, xpos, ypos, WinWidth, WinHeight,
		        0, VisInfo->depth, InputOutput,
		        VisInfo->visual, mask, &attr);
   if (!Win) {
      Error("Couldn't create window");
   }

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = WinWidth;
      sizehints.height = WinHeight;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(Dpy, Win, &sizehints);
      XSetStandardProperties(Dpy, Win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   XMapWindow(Dpy, Win);
}


/**
 * Change texture image, using TexContext
 */
static void
ModifyTexture(void)
{
   GLuint tex[TEX_SIZE][TEX_SIZE];
   GLuint c0, c1;
   int i, j;

   if (Win && !glXMakeCurrent(Dpy, Win, Contexts[TexContext])) {
      Error("glXMakeCurrent failed");
   }

   /* choose two random colors */
   c0 = rand() & 0xffffffff;
   c1 = rand() & 0xffffffff;

   for (i = 0; i < TEX_SIZE; i++) {
      for (j = 0; j < TEX_SIZE; j++) {
         if (((i / 4) ^ (j / 4)) & 1) {
            tex[i][j] = c0;
         }
         else {
            tex[i][j] = c1;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, tex);

   NewTexture = GL_TRUE;
}


static void
InitContext(void)
{
   glGenTextures(1, &TexObj);
   assert(TexObj);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glEnable(GL_TEXTURE_2D);

   printf("GL_RENDERER = %s\n", (char*) glGetString(GL_RENDERER));
}


static void
Setup(void)
{
   int i;

   Dpy = XOpenDisplay(DisplayName);
   if (!Dpy) {
      Error("Unable to open display");
   }

   CreateWindow("sharedtex");

   for (i = 0; i < MAX_CONTEXTS; i++) {
      GLXContext share = i > 0 ? Contexts[0] : 0;

      Contexts[i] = glXCreateContext(Dpy, VisInfo, share, True);
      if (!Contexts[i]) {
         Error("Unable to create GLX context");
      }

      if (!glXMakeCurrent(Dpy, Win, Contexts[i])) {
         Error("glXMakeCurrent failed");
      }

      InitContext();
   }

   ModifyTexture();
}


/**
 * Redraw window, using DrawContext
 */
static void
Redraw(void)
{
   static float rot = 0.0;
   float ar;

   rot += 1.0;

   if (Win && !glXMakeCurrent(Dpy, Win, Contexts[DrawContext])) {
      Error("glXMakeCurrent failed");
   }

   glViewport(0, 0, WinWidth, WinHeight);
   ar = (float) WinWidth / (float) WinHeight;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-ar, ar, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);

   glShadeModel(GL_FLAT);
   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);

   glPushMatrix();
   glRotatef(rot, 0, 0, 1);
   glScalef(0.7, 0.7, 0.7);

   if (NewTexture) {
      /* rebind to get new contents */
      glBindTexture(GL_TEXTURE_2D, TexObj);
      NewTexture = GL_FALSE;
   }

   /* draw textured quad */
   glBegin(GL_POLYGON);
   glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0, -1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0, -1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0,  1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0,  1.0 );
   glEnd();

   glPopMatrix();

   if (Win)
      glXSwapBuffers(Dpy, Win);
}


static void
EventLoop(void)
{
   while (1) {
      while (XPending(Dpy) > 0) {
         XEvent event;
         XNextEvent(Dpy, &event);

         switch (event.type) {
         case Expose:
            Redraw();
            break;
         case ConfigureNotify:
            WinWidth = event.xconfigure.width;
            WinHeight = event.xconfigure.height;
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
               case XK_t:
               case XK_T:
                  ModifyTexture();
                  break;
               default:
                  ;
               }
            }
            Redraw();
            break;
         default:
            /*no-op*/ ;
         }
      }

      Redraw();
      usleep(10000);
   }
}




int
main(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0 && i < argc) {
         DisplayName = argv[i+1];
         i++;
      }
   }

   Setup();

   printf("Press 't' to change texture image/colors\n");

   EventLoop();

   return 0;
}
