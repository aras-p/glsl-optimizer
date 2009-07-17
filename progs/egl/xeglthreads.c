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
 *
 * Ported to EGL by Chia-I Wu <olvaffe@gmail.com>
 */


/*
 * This program tests EGL thread safety.
 * Command line options:
 *  -p                       Open a display connection for each thread
 *  -l                       Enable application-side locking
 *  -n <num threads>         Number of threads to create (default is 2)
 *  -display <display name>  Specify X display (default is $DISPLAY)
 *  -t                       Use texture mapping
 *
 * Brian Paul  20 July 2000
 */


/*
 * Notes:
 * - Each thread gets its own EGL context.
 *
 * - The EGL contexts share texture objects.
 *
 * - When 't' is pressed to update the texture image, the window/thread which
 *   has input focus is signalled to change the texture.  The other threads
 *   should see the updated texture the next time they call glBindTexture.
 */


#if defined(PTHREADS)   /* defined by Mesa on Linux and other platforms */

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <EGL/egl.h>
#include <math.h>
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
   EGLDisplay Display;
   EGLContext Context;
   EGLSurface Surface;
   float Angle;
   int WinWidth, WinHeight;
   GLboolean NewSize;
   GLboolean Initialized;
   GLboolean MakeNewTexture;
};


#define MAX_WINTHREADS 100
static struct winthread WinThreads[MAX_WINTHREADS];
static int NumWinThreads = 0;
static volatile GLboolean ExitFlag = GL_FALSE;

static GLboolean MultiDisplays = 0;
static GLboolean Locking = 0;
static GLboolean Texture = GL_FALSE;
static GLuint TexObj = 12;
static GLboolean Animate = GL_TRUE;

static pthread_mutex_t Mutex;
static pthread_cond_t CondVar;
static pthread_mutex_t CondMutex;


static void
Error(const char *msg)
{
   fprintf(stderr, "Error: %s\n", msg);
   exit(1);
}


static void
signal_redraw(void)
{
   pthread_mutex_lock(&CondMutex);
   pthread_cond_broadcast(&CondVar);
   pthread_mutex_unlock(&CondMutex);
}


static void
MakeNewTexture(struct winthread *wt)
{
#define TEX_SIZE 128
   static float step = 0.0;
   GLfloat image[TEX_SIZE][TEX_SIZE][4];
   GLint width;
   int i, j;

   for (j = 0; j < TEX_SIZE; j++) {
      for (i = 0; i < TEX_SIZE; i++) {
         float dt = 5.0 * (j - 0.5 * TEX_SIZE) / TEX_SIZE;
         float ds = 5.0 * (i - 0.5 * TEX_SIZE) / TEX_SIZE;
         float r = dt * dt + ds * ds + step;
         image[j][i][0] =
         image[j][i][1] =
         image[j][i][2] = 0.75 + 0.25 * cos(r);
         image[j][i][3] = 1.0;
      }
   }

   step += 0.5;

   glBindTexture(GL_TEXTURE_2D, TexObj);

   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
   if (width) {
      assert(width == TEX_SIZE);
      /* sub-tex replace */
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEX_SIZE, TEX_SIZE,
                   GL_RGBA, GL_FLOAT, image);
   }
   else {
      /* create new */
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0,
                   GL_RGBA, GL_FLOAT, image);
   }
}



/* draw a colored cube */
static void
draw_object(void)
{
   glPushMatrix();
   glScalef(0.75, 0.75, 0.75);

   glColor3f(1, 0, 0);

   if (Texture) {
      glBindTexture(GL_TEXTURE_2D, TexObj);
      glEnable(GL_TEXTURE_2D);
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }

   glBegin(GL_QUADS);

   /* -X */
   glColor3f(0, 1, 1);
   glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
   glTexCoord2f(1, 0);  glVertex3f(-1,  1, -1);
   glTexCoord2f(1, 1);  glVertex3f(-1,  1,  1);
   glTexCoord2f(0, 1);  glVertex3f(-1, -1,  1);

   /* +X */
   glColor3f(1, 0, 0);
   glTexCoord2f(0, 0);  glVertex3f(1, -1, -1);
   glTexCoord2f(1, 0);  glVertex3f(1,  1, -1);
   glTexCoord2f(1, 1);  glVertex3f(1,  1,  1);
   glTexCoord2f(0, 1);  glVertex3f(1, -1,  1);

   /* -Y */
   glColor3f(1, 0, 1);
   glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
   glTexCoord2f(1, 0);  glVertex3f( 1, -1, -1);
   glTexCoord2f(1, 1);  glVertex3f( 1, -1,  1);
   glTexCoord2f(0, 1);  glVertex3f(-1, -1,  1);

   /* +Y */
   glColor3f(0, 1, 0);
   glTexCoord2f(0, 0);  glVertex3f(-1, 1, -1);
   glTexCoord2f(1, 0);  glVertex3f( 1, 1, -1);
   glTexCoord2f(1, 1);  glVertex3f( 1, 1,  1);
   glTexCoord2f(0, 1);  glVertex3f(-1, 1,  1);

   /* -Z */
   glColor3f(1, 1, 0);
   glTexCoord2f(0, 0);  glVertex3f(-1, -1, -1);
   glTexCoord2f(1, 0);  glVertex3f( 1, -1, -1);
   glTexCoord2f(1, 1);  glVertex3f( 1,  1, -1);
   glTexCoord2f(0, 1);  glVertex3f(-1,  1, -1);

   /* +Y */
   glColor3f(0, 0, 1);
   glTexCoord2f(0, 0);  glVertex3f(-1, -1, 1);
   glTexCoord2f(1, 0);  glVertex3f( 1, -1, 1);
   glTexCoord2f(1, 1);  glVertex3f( 1,  1, 1);
   glTexCoord2f(0, 1);  glVertex3f(-1,  1, 1);

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
   if (!Animate)
      signal_redraw();
}


/*
 * We have an instance of this for each thread.
 */
static void
draw_loop(struct winthread *wt)
{
   while (!ExitFlag) {

      if (Locking)
         pthread_mutex_lock(&Mutex);

      if (!wt->Initialized) {
         eglMakeCurrent(wt->Display, wt->Surface, wt->Surface, wt->Context);
         printf("xeglthreads: %d: GL_RENDERER = %s\n", wt->Index,
                (char *) glGetString(GL_RENDERER));
         if (Texture /*&& wt->Index == 0*/) {
            MakeNewTexture(wt);
         }
         wt->Initialized = GL_TRUE;
      }

      if (Locking)
         pthread_mutex_unlock(&Mutex);

      eglBindAPI(EGL_OPENGL_API);
      if (eglGetCurrentContext() != wt->Context) {
         printf("xeglthreads: current context %p != %p\n",
               eglGetCurrentContext(), wt->Context);
      }

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

      if (wt->MakeNewTexture) {
         MakeNewTexture(wt);
         wt->MakeNewTexture = GL_FALSE;
      }

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glPushMatrix();
         glRotatef(wt->Angle, 0, 1, 0);
         glRotatef(wt->Angle, 1, 0, 0);
         glScalef(0.7, 0.7, 0.7);
         draw_object();
      glPopMatrix();

      if (Locking)
         pthread_mutex_lock(&Mutex);

      eglSwapBuffers(wt->Display, wt->Surface);

      if (Locking)
         pthread_mutex_unlock(&Mutex);

      if (Animate) {
         usleep(5000);
      }
      else {
         /* wait for signal to draw */
         pthread_mutex_lock(&CondMutex);
         pthread_cond_wait(&CondVar, &CondMutex);
         pthread_mutex_unlock(&CondMutex);
      }
      wt->Angle += 1.0;
   }
   eglMakeCurrent(wt->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}


static void
keypress(XEvent *event, struct winthread *wt)
{
   char buf[100];
   KeySym keySym;
   XComposeStatus stat;

   XLookupString(&event->xkey, buf, sizeof(buf), &keySym, &stat);

   switch (keySym) {
   case XK_Escape:
      /* tell all threads to exit */
      if (!Animate) {
         signal_redraw();
      }
      ExitFlag = GL_TRUE;
      /*printf("exit draw_loop %d\n", wt->Index);*/
      return;
   case XK_t:
   case XK_T:
      if (Texture) {
         wt->MakeNewTexture = GL_TRUE;
         if (!Animate)
            signal_redraw();
      }
      break;
   case XK_a:
   case XK_A:
      Animate = !Animate;
      if (Animate)  /* yes, prev Animate state! */
         signal_redraw();
      break;
   case XK_s:
   case XK_S:
      if (!Animate)
         signal_redraw();
      break;
   default:
      ; /* nop */
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
            for (i = 0; i < NumWinThreads; i++) {
               struct winthread *wt = &WinThreads[i];
               if (event.xkey.window == wt->Win) {
                  keypress(&event, wt);
                  break;
               }
            }
            break;
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
            keypress(&event, wt);
            break;
         default:
            ; /* nop */
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
create_window(struct winthread *wt, EGLContext shareCtx)
{
   Window win;
   EGLContext ctx;
   EGLSurface surf;
   EGLint attribs[] = { EGL_RED_SIZE, 1,
                        EGL_GREEN_SIZE, 1,
                        EGL_BLUE_SIZE, 1,
                        EGL_DEPTH_SIZE, 1,
                        EGL_NONE };
   EGLConfig config;
   EGLint num_configs;
   EGLint vid;
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visinfo, visTemplate;
   int num_visuals;
   int width = 160, height = 160;
   int xpos = (wt->Index % 8) * (width + 10);
   int ypos = (wt->Index / 8) * (width + 20);

   scrnum = DefaultScreen(wt->Dpy);
   root = RootWindow(wt->Dpy, scrnum);

   if (!eglChooseConfig(wt->Display, attribs, &config, 1, &num_configs)) {
      Error("Unable to choose an EGL config");
   }

   assert(config);
   assert(num_configs > 0);

   if (!eglGetConfigAttrib(wt->Display, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      Error("Unable to get visual id of EGL config\n");
   }

   visTemplate.visualid = vid;
   visinfo = XGetVisualInfo(wt->Dpy, VisualIDMask,
                        &visTemplate, &num_visuals);
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

   XFree(visinfo);

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(wt->Dpy, win, &sizehints);
      XSetStandardProperties(wt->Dpy, win, "xeglthreads", "xeglthreads",
                              None, (char **)NULL, 0, &sizehints);
   }

   eglBindAPI(EGL_OPENGL_API);

   ctx = eglCreateContext(wt->Display, config, shareCtx, NULL);
   if (!ctx) {
      Error("Couldn't create EGL context");
   }
   surf = eglCreateWindowSurface(wt->Display, config, win, NULL);
   if (!surf) {
      Error("Couldn't create EGL surface");
   }

   XMapWindow(wt->Dpy, win);
   XSync(wt->Dpy, 0);

   /* save the info for this window/context */
   wt->Win = win;
   wt->Context = ctx;
   wt->Surface = surf;
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
      eglDestroyContext(WinThreads[i].Display, WinThreads[i].Context);
      XDestroyWindow(WinThreads[i].Dpy, WinThreads[i].Win);
   }
}


static void
usage(void)
{
   printf("xeglthreads: test of EGL/GL thread safety (any key = exit)\n");
   printf("Usage:\n");
   printf("  xeglthreads [options]\n");
   printf("Options:\n");
   printf("   -display DISPLAYNAME  Specify display string\n");
   printf("   -n NUMTHREADS  Number of threads to create\n");
   printf("   -p  Use a separate display connection for each thread\n");
   printf("   -l  Use application-side locking\n");
   printf("   -t  Enable texturing\n");
   printf("Keyboard:\n");
   printf("   Esc  Exit\n");
   printf("   t    Change texture image (requires -t option)\n");
   printf("   a    Toggle animation\n");
   printf("   s    Step rotation (when not animating)\n");
}


int
main(int argc, char *argv[])
{
   char *displayName = NULL;
   int numThreads = 2;
   Display *dpy = NULL;
   EGLDisplay *egl_dpy = NULL;
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
         else if (strcmp(argv[i], "-t") == 0) {
            Texture = 1;
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
      printf("xeglthreads: Using explicit locks around Xlib calls.\n");
   else
      printf("xeglthreads: No explict locking.\n");

   if (MultiDisplays)
      printf("xeglthreads: Per-thread display connections.\n");
   else
      printf("xeglthreads: Single display connection.\n");

   /*
    * VERY IMPORTANT: call XInitThreads() before any other Xlib functions.
    */
   if (!MultiDisplays) {
       if (!Locking) {
           threadStat = XInitThreads();
           if (threadStat) {
               printf("XInitThreads() returned %d (success)\n",
                      (int) threadStat);
           }
           else {
               printf("XInitThreads() returned 0 "
                      "(failure- this program may fail)\n");
           }
       }

      dpy = XOpenDisplay(displayName);
      if (!dpy) {
         fprintf(stderr, "Unable to open display %s\n",
                 XDisplayName(displayName));
         return -1;
      }
      egl_dpy = eglGetDisplay(dpy);
      if (!egl_dpy) {
         fprintf(stderr, "Unable to get EGL display\n");
         XCloseDisplay(dpy);
         return -1;
      }
      if (!eglInitialize(egl_dpy, NULL, NULL)) {
          fprintf(stderr, "Unable to initialize EGL display\n");
          return -1;
      }
   }

   pthread_mutex_init(&Mutex, NULL);
   pthread_mutex_init(&CondMutex, NULL);
   pthread_cond_init(&CondVar, NULL);

   printf("xeglthreads: creating windows\n");

   NumWinThreads = numThreads;

   /* Create the EGL windows and contexts */
   for (i = 0; i < numThreads; i++) {
      EGLContext share;

      if (MultiDisplays) {
         WinThreads[i].Dpy = XOpenDisplay(displayName);
         assert(WinThreads[i].Dpy);
         WinThreads[i].Display = eglGetDisplay(WinThreads[i].Dpy);
         assert(eglInitialize(WinThreads[i].Display, NULL, NULL));
      }
      else {
         WinThreads[i].Dpy = dpy;
         WinThreads[i].Display = egl_dpy;
      }
      WinThreads[i].Index = i;
      WinThreads[i].Initialized = GL_FALSE;

      share = (Texture && i > 0) ? WinThreads[0].Context : 0;

      create_window(&WinThreads[i], share);
   }

   printf("xeglthreads: creating threads\n");

   /* Create the threads */
   for (i = 0; i < numThreads; i++) {
      pthread_create(&WinThreads[i].Thread, NULL, thread_function,
                     (void*) &WinThreads[i]);
      printf("xeglthreads: Created thread %p\n",
              (void *) WinThreads[i].Thread);
   }

   if (MultiDisplays)
      event_loop_multi();
   else
      event_loop(dpy);

   clean_up();

   if (MultiDisplays) {
      for (i = 0; i < numThreads; i++) {
          eglTerminate(WinThreads[i].Display);
          XCloseDisplay(WinThreads[i].Dpy);
      }
   }
   else {
      eglTerminate(egl_dpy);
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
