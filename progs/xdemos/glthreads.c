/*
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


/*
 * This program tests GLX thread safety.
 * Command line options:
 *  -p                       Open a display connection for each thread
 *  -l                       Enable application-side locking
 *  -n <num threads>         Number of threads to create (default is 2)
 *  -display <display name>  Specify X display (default is :0.0)
 *
 * Brian Paul  20 July 2000
 */


#if defined(PTHREADS)   /* defined by Mesa on Linux and other platforms */

#include <assert.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


/*
 * Each window/thread/context:
 */
struct winthread {
   Display *Dpy;
   int Index;
   pthread_t Thread;
   Window Win;
   GLXContext Context;
   float Angle;
   int WinWidth, WinHeight;
   GLboolean NewSize;
};


#define MAX_WINTHREADS 100
static struct winthread WinThreads[MAX_WINTHREADS];
static int NumWinThreads = 0;
static volatile GLboolean ExitFlag = GL_FALSE;

static GLboolean MultiDisplays = 0;
static GLboolean Locking = 0;

static pthread_mutex_t Mutex;


static void
Error(const char *msg)
{
   fprintf(stderr, "Error: %s\n", msg);
   exit(1);
}


/* draw a colored cube */
static void
draw_object(void)
{
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
}


/* signal resize of given window */
static void
resize(struct winthread *wt, int w, int h)
{
   wt->NewSize = GL_TRUE;
   wt->WinWidth = w;
   wt->WinHeight = h;
}


/*
 * We have an instance of this for each thread.
 */
static void
draw_loop(struct winthread *wt)
{
   GLboolean firstIter = GL_TRUE;

   while (!ExitFlag) {

      if (Locking)
         pthread_mutex_lock(&Mutex);

      glXMakeCurrent(wt->Dpy, wt->Win, wt->Context);
      if (firstIter) {
         printf("glthreads: %d: GL_RENDERER = %s\n", wt->Index,
                (char *) glGetString(GL_RENDERER));
         firstIter = GL_FALSE;
      }

      if (Locking)
         pthread_mutex_unlock(&Mutex);

      glEnable(GL_DEPTH_TEST);

      if (wt->NewSize) {
         GLfloat w = (float) wt->WinWidth / (float) wt->WinHeight;
         glViewport(0, 0, wt->WinWidth, wt->WinHeight);
         glMatrixMode(GL_PROJECTION);
         glLoadIdentity();
         glFrustum(-w, w, -1.0, 1.0, 1.5, 10);
         glMatrixMode(GL_MODELVIEW);
         glLoadIdentity();
         glTranslatef(0, 0, -2.5);
         wt->NewSize = GL_FALSE;
      }

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glPushMatrix();
         glRotatef(wt->Angle, 0, 0, 1);
         glRotatef(wt->Angle, 1, 0, 0);
         glScalef(0.7, 0.7, 0.7);
         draw_object();
      glPopMatrix();

      if (Locking)
         pthread_mutex_lock(&Mutex);

      glXSwapBuffers(wt->Dpy, wt->Win);

      if (Locking)
         pthread_mutex_unlock(&Mutex);

      usleep(5000);
      wt->Angle += 1.0;
   }
}


/*
 * The main process thread runs this loop.
 * Single display connection for all threads.
 */
static void
event_loop(Display *dpy)
{
   XEvent event;
   int i;

   assert(!MultiDisplays);

   while (!ExitFlag) {

      if (Locking) {
         while (1) {
            int k;
            pthread_mutex_lock(&Mutex);
            k = XPending(dpy);
            if (k) {
               XNextEvent(dpy, &event);
               pthread_mutex_unlock(&Mutex);
               break;
            }
            pthread_mutex_unlock(&Mutex);
            usleep(5000);
         }
      }
      else {
         XNextEvent(dpy, &event);
      }

      switch (event.type) {
         case ConfigureNotify:
            /* Find winthread for this event's window */
            for (i = 0; i < NumWinThreads; i++) {
               struct winthread *wt = &WinThreads[i];
               if (event.xconfigure.window == wt->Win) {
                  resize(wt, event.xconfigure.width,
                         event.xconfigure.height);
                  break;
               }
            }
            break;
         case KeyPress:
            /* tell all threads to exit */
            ExitFlag = GL_TRUE;
            /*printf("exit draw_loop %d\n", wt->Index);*/
            return;
         default:
            /*no-op*/ ;
      }
   }
}


/*
 * Separate display connection for each thread.
 */
static void
event_loop_multi(void)
{
   XEvent event;
   int w = 0;

   assert(MultiDisplays);

   while (!ExitFlag) {
      struct winthread *wt = &WinThreads[w];
      if (XPending(wt->Dpy)) {
         XNextEvent(wt->Dpy, &event);
         switch (event.type) {
         case ConfigureNotify:
            resize(wt, event.xconfigure.width, event.xconfigure.height);
            break;
         case KeyPress:
            /* tell all threads to exit */
            ExitFlag = GL_TRUE;
            /*printf("exit draw_loop %d\n", wt->Index);*/
            return;
         default:
            /*no-op*/ ;
         }
      }
      w = (w + 1) % NumWinThreads;
      usleep(5000);
   }
}



/*
 * we'll call this once for each thread, before the threads are created.
 */
static void
create_window(struct winthread *wt)
{
   Window win;
   GLXContext ctx;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
                    GLX_DEPTH_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visinfo;
   int width = 80, height = 80;
   int xpos = (wt->Index % 10) * 90;
   int ypos = (wt->Index / 10) * 100;

   scrnum = DefaultScreen(wt->Dpy);
   root = RootWindow(wt->Dpy, scrnum);

   visinfo = glXChooseVisual(wt->Dpy, scrnum, attrib);
   if (!visinfo) {
      Error("Unable to find RGB, Z, double-buffered visual");
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(wt->Dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(wt->Dpy, root, xpos, ypos, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr);
   if (!win) {
      Error("Couldn't create window");
   }

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(wt->Dpy, win, &sizehints);
      XSetStandardProperties(wt->Dpy, win, "glthreads", "glthreads",
                              None, (char **)NULL, 0, &sizehints);
   }


   ctx = glXCreateContext(wt->Dpy, visinfo, NULL, True);
   if (!ctx) {
      Error("Couldn't create GLX context");
   }

   XMapWindow(wt->Dpy, win);
   XSync(wt->Dpy, 0);

   /* save the info for this window/context */
   wt->Win = win;
   wt->Context = ctx;
   wt->Angle = 0.0;
   wt->WinWidth = width;
   wt->WinHeight = height;
   wt->NewSize = GL_TRUE;
}


/*
 * Called by pthread_create()
 */
static void *
thread_function(void *p)
{
   struct winthread *wt = (struct winthread *) p;
   draw_loop(wt);
   return NULL;
}


/*
 * called before exit to wait for all threads to finish
 */
static void
clean_up(void)
{
   int i;

   /* wait for threads to finish */
   for (i = 0; i < NumWinThreads; i++) {
      pthread_join(WinThreads[i].Thread, NULL);
   }

   for (i = 0; i < NumWinThreads; i++) {
      glXDestroyContext(WinThreads[i].Dpy, WinThreads[i].Context);
      XDestroyWindow(WinThreads[i].Dpy, WinThreads[i].Win);
   }
}


static void
usage(void)
{
   printf("glthreads: test of GL thread safety (any key = exit)\n");
   printf("Usage:\n");
   printf("  glthreads [options]\n");
   printf("Options:\n");
   printf("   -display DISPLAYNAME  Specify display string\n");
   printf("   -n NUMTHREADS  Number of threads to create\n");
   printf("   -p  Use a separate display connection for each thread\n");
   printf("   -l  Use application-side locking\n");
}


int
main(int argc, char *argv[])
{
   char *displayName = ":0.0";
   int numThreads = 2;
   Display *dpy = NULL;
   int i;
   Status threadStat;

   if (argc == 1) {
      usage();
   }
   else {
      int i;
      for (i = 1; i < argc; i++) {
         if (strcmp(argv[i], "-display") == 0 && i + 1 < argc) {
            displayName = argv[i + 1];
            i++;
         }
         else if (strcmp(argv[i], "-p") == 0) {
            MultiDisplays = 1;
         }
         else if (strcmp(argv[i], "-l") == 0) {
            Locking = 1;
         }
         else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            numThreads = atoi(argv[i + 1]);
            if (numThreads < 1)
               numThreads = 1;
            else if (numThreads > MAX_WINTHREADS)
               numThreads = MAX_WINTHREADS;
            i++;
         }
         else {
            usage();
            exit(1);
         }
      }
   }
   
   if (Locking)
      printf("glthreads: Using explicit locks around Xlib calls.\n");
   else
      printf("glthreads: No explict locking.\n");

   if (MultiDisplays)
      printf("glthreads: Per-thread display connections.\n");
   else
      printf("glthreads: Single display connection.\n");

   /*
    * VERY IMPORTANT: call XInitThreads() before any other Xlib functions.
    */
   if (!MultiDisplays) {
      if (!Locking) {
         threadStat = XInitThreads();
      if (threadStat) {
         printf("XInitThreads() returned %d (success)\n", (int) threadStat);
      }
      else {
         printf("XInitThreads() returned 0 (failure- this program may fail)\n");
      }
      }

      dpy = XOpenDisplay(displayName);
      if (!dpy) {
         fprintf(stderr, "Unable to open display %s\n", XDisplayName(displayName));
         return -1;
      }
   }

   if (Locking) {
      pthread_mutex_init(&Mutex, NULL);
   }

   printf("glthreads: creating windows\n");

   NumWinThreads = numThreads;

   /* Create the GLX windows and contexts */
   for (i = 0; i < numThreads; i++) {
      if (MultiDisplays) {
         WinThreads[i].Dpy = XOpenDisplay(displayName);
         assert(WinThreads[i].Dpy);
      }
      else {
         WinThreads[i].Dpy = dpy;
      }
      WinThreads[i].Index = i;
      create_window(&WinThreads[i]);
   }

   printf("glthreads: creating threads\n");

   /* Create the threads */
   for (i = 0; i < numThreads; i++) {
      pthread_create(&WinThreads[i].Thread, NULL, thread_function,
                     (void*) &WinThreads[i]);
      printf("glthreads: Created thread %p\n", (void *) WinThreads[i].Thread);
   }

   if (MultiDisplays)
      event_loop_multi();
   else
      event_loop(dpy);

   clean_up();

   if (MultiDisplays) {
      for (i = 0; i < numThreads; i++) {
         XCloseDisplay(WinThreads[i].Dpy);
      }
   }
   else {
      XCloseDisplay(dpy);
   }

   return 0;
}


#else /* PTHREADS */


#include <stdio.h>

int
main(int argc, char *argv[])
{
   printf("Sorry, this program wasn't compiled with PTHREADS defined.\n");
   return 0;
}


#endif /* PTHREADS */
