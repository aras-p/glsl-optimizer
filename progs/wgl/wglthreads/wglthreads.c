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
 *
 * Port to windows done by Michal Krol.
 */


/*
 * This program tests WGL thread safety.
 * Command line options:
 *  -h                       Print usage
 *  -l                       Enable application-side locking
 *  -n <num threads>         Number of threads to create (default is 2)
 *  -t                       Use texture mapping
 *  -s                       Force single-threaded.
 *
 * Brian Paul  20 July 2000
 */


/*
 * Notes:
 * - Each thread gets its own WGL context.
 *
 * - The WGL contexts share texture objects.
 *
 * - When 't' is pressed to update the texture image, the window/thread which
 *   has input focus is signalled to change the texture.  The other threads
 *   should see the updated texture the next time they call glBindTexture.
 */


#include <assert.h>
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "opengl32.lib")


/*
 * Each window/thread/context:
 */
struct winthread {
   int Index;
   HANDLE Thread;
   HWND Win;
   HDC hDC;
   HGLRC Context;
   float Angle;
   int WinWidth, WinHeight;
   GLboolean NewSize;
   HANDLE hEventInitialised;
   GLboolean Initialized;
   GLboolean MakeNewTexture;
   HANDLE hEventRedraw;
};


#define MAX_WINTHREADS 100
static struct winthread WinThreads[MAX_WINTHREADS];
static int NumWinThreads = 2;
static HANDLE ExitEvent = NULL;

static GLboolean Locking = 0;
static GLboolean Texture = GL_FALSE;
static GLboolean SingleThreaded = GL_FALSE;
static GLuint TexObj = 12;
static GLboolean Animate = GL_TRUE;

static CRITICAL_SECTION Mutex;


static void
Error(const char *msg)
{
   fprintf(stderr, "Error: %s\n", msg);
   exit(1);
}


static void
signal_redraw(void)
{
   int i;

   for (i = 0; i < NumWinThreads; i++) {
      SetEvent(WinThreads[i].hEventRedraw);
   }
}


static void
MakeNewTexture(struct winthread *wt)
{
#define TEX_SIZE 128
   static float step = 0.0f;
   GLfloat image[TEX_SIZE][TEX_SIZE][4];
   GLint width;
   int i, j;

   for (j = 0; j < TEX_SIZE; j++) {
      for (i = 0; i < TEX_SIZE; i++) {
         float dt = 5.0f * (j - 0.5f * TEX_SIZE) / TEX_SIZE;
         float ds = 5.0f * (i - 0.5f * TEX_SIZE) / TEX_SIZE;
         float r = dt * dt + ds * ds + step;
         image[j][i][0] = 
         image[j][i][1] = 
         image[j][i][2] = 0.75f + 0.25f * (float) cos(r);
         image[j][i][3] = 1.0f;
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
   glScalef(0.75f, 0.75f, 0.75f);

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
      SetEvent(wt->hEventRedraw);
}


/*
 * We have an instance of this for each thread.
 */
static void
draw_loop(struct winthread *wt)
{
   while (1) {
      GLboolean draw = Animate;
      MSG msg;

      if (Animate) {
         /* wait 5 ms for signal either to exit or process messages */
         switch (MsgWaitForMultipleObjects(1, &ExitEvent, FALSE, 5, QS_ALLINPUT)) {
         case WAIT_OBJECT_0:
            SendMessage(wt->Win, WM_CLOSE, 0, 0);
            break;
         case WAIT_OBJECT_0 + 1:
            break;
         }
      }
      else {
         HANDLE events[2];

         events[0] = wt->hEventRedraw;
         events[1] = ExitEvent;

         /* wait for signal either to draw, exit or process messages */
         switch (MsgWaitForMultipleObjects(2, events, FALSE, INFINITE, QS_ALLINPUT)) {
         case WAIT_OBJECT_0:
            draw = GL_TRUE;
            break;
         case WAIT_OBJECT_0 + 1:
            SendMessage(wt->Win, WM_CLOSE, 0, 0);
            break;
         case WAIT_OBJECT_0 + 2:
            break;
         }
      }

      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            return;
         }
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      if (!draw)
         continue;

      if (Locking)
         EnterCriticalSection(&Mutex);

      wglMakeCurrent(wt->hDC, wt->Context);

      if (!wt->Initialized) {
         printf("wglthreads: %d: GL_RENDERER = %s\n", wt->Index,
                (char *) glGetString(GL_RENDERER));
         if (Texture /*&& wt->Index == 0*/) {
            MakeNewTexture(wt);
         }
         wt->Initialized = GL_TRUE;
      }

      if (Locking)
         LeaveCriticalSection(&Mutex);

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
         glScalef(0.7f, 0.7f, 0.7f);
         draw_object();
      glPopMatrix();

      if (Locking)
         EnterCriticalSection(&Mutex);
      SwapBuffers(wt->hDC);
      if (Locking)
         LeaveCriticalSection(&Mutex);

      wt->Angle += 1.0;
   }
}


static void
keypress(WPARAM keySym, struct winthread *wt)
{
   switch (keySym) {
   case VK_ESCAPE:
      /* tell all threads to exit */
      SetEvent(ExitEvent);
      /*printf("exit draw_loop %d\n", wt->Index);*/
      return;
   case 't':
   case 'T':
      if (Texture) {
         wt->MakeNewTexture = GL_TRUE;
         if (!Animate)
            signal_redraw();
      }
      break;
   case 'a':
   case 'A':
      Animate = !Animate;
      if (Animate)
         signal_redraw();
      break;
   case 's':
   case 'S':
      if (!Animate)
         signal_redraw();
      break;
   default:
      ; /* nop */
   }
}


static LRESULT CALLBACK
WndProc(HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam )
{
   int i;

   switch (uMsg) {
   case WM_KEYDOWN:
      for (i = 0; i < NumWinThreads; i++) {
         struct winthread *wt = &WinThreads[i];

         if (hWnd == wt->Win) {
            keypress(wParam, wt);
            break;
         }
      }
      break;
   case WM_SIZE:
      for (i = 0; i < NumWinThreads; i++) {
         struct winthread *wt = &WinThreads[i];

         if (hWnd == wt->Win) {
            RECT r;

            GetClientRect(hWnd, &r);
            resize(wt, r.right, r.bottom);
            break;
         }
      }
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   return 0;
}

/*
 * we'll call this once for each thread, before the threads are created.
 */
static void
create_window(struct winthread *wt, HGLRC shareCtx)
{
   WNDCLASS wc = {0};
   int width = 160, height = 160;
   int xpos = (wt->Index % 8) * (width + 10);
   int ypos = (wt->Index / 8) * (width + 20);
   HWND win;
   HDC hdc;
   PIXELFORMATDESCRIPTOR pfd = {0};
   int visinfo;
   HGLRC ctx;

   wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "wglthreads";
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   RegisterClass(&wc);

   win = CreateWindowEx(0,
                        wc.lpszClassName,
                        "wglthreads",
                        WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                        xpos,
                        ypos,
                        width,
                        height,
                        NULL,
                        NULL,
                        wc.hInstance,
                        (LPVOID) wt);
   if (!win) {
      Error("Couldn't create window");
   }

   hdc = GetDC(win);
   if (!hdc) {
      Error("Couldn't obtain HDC");
   }

   pfd.cColorBits = 24;
   pfd.cDepthBits = 24;
   pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;

   visinfo = ChoosePixelFormat(hdc, &pfd);
   if (!visinfo) {
      Error("Unable to find RGB, Z, double-buffered visual");
   }

   SetPixelFormat(hdc, visinfo, &pfd);
   ctx = wglCreateContext(hdc);
   if (!ctx) {
      Error("Couldn't create WGL context");
   }

   if (shareCtx) {
      wglShareLists(shareCtx, ctx);
   }

   /* save the info for this window/context */
   wt->Win = win;
   wt->hDC = hdc;
   wt->Context = ctx;
   wt->Angle = 0.0;
   wt->WinWidth = width;
   wt->WinHeight = height;
   wt->NewSize = GL_TRUE;
}


/*
 * Called by pthread_create()
 */
static DWORD WINAPI
ThreadProc(void *p)
{
   struct winthread *wt = (struct winthread *) p;
   HGLRC share;

   share = (Texture && wt->Index > 0) ? WinThreads[0].Context : 0;
   create_window(wt, share);
   SetEvent(wt->hEventInitialised);

   draw_loop(wt);
   return 0;
}


static void
usage(void)
{
   printf("wglthreads: test of GL thread safety (any key = exit)\n");
   printf("Usage:\n");
   printf("  wglthreads [options]\n");
   printf("Options:\n");
   printf("   -h  Show this usage screen\n");
   printf("   -n NUMTHREADS  Number of threads to create\n");
   printf("   -l  Use application-side locking\n");
   printf("   -t  Enable texturing\n");
   printf("   -s  Force single-threaded\n");
   printf("Keyboard:\n");
   printf("   Esc  Exit\n");
   printf("   t    Change texture image (requires -t option)\n");
   printf("   a    Toggle animation\n");
   printf("   s    Step rotation (when not animating)\n");
}


int
main(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h") == 0) {
         usage();
         exit(0);
      }
      else if (strcmp(argv[i], "-l") == 0) {
         Locking = 1;
      }
      else if (strcmp(argv[i], "-t") == 0) {
         Texture = 1;
      }
      else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
         NumWinThreads = atoi(argv[i + 1]);
         if (NumWinThreads < 1)
            NumWinThreads = 1;
         else if (NumWinThreads > MAX_WINTHREADS)
            NumWinThreads = MAX_WINTHREADS;
         i++;
      }
      else if (strcmp(argv[i], "-s") == 0) {
         SingleThreaded = GL_TRUE;
      }
      else {
         usage();
         exit(1);
      }
   }

   if (SingleThreaded)
      printf("wglthreads: Forcing single-threaded, no other threads will be created.\n");
   
   if (Locking)
      printf("wglthreads: Using explicit locks around WGL calls.\n");
   else
      printf("wglthreads: No explict locking.\n");

   InitializeCriticalSection(&Mutex);
   ExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

   if (SingleThreaded) {
      NumWinThreads = 1;

      WinThreads[0].Index = 0;
      WinThreads[0].hEventInitialised = CreateEvent(NULL, TRUE, FALSE, NULL);
      WinThreads[0].hEventRedraw = CreateEvent(NULL, FALSE, FALSE, NULL);

      ThreadProc((void*) &WinThreads[0]);
   }
   else {
      HANDLE threads[MAX_WINTHREADS];

      printf("wglthreads: creating threads\n");

      /* Create the threads */
      for (i = 0; i < NumWinThreads; i++) {
         DWORD id;

         WinThreads[i].Index = i;
         WinThreads[i].hEventInitialised = CreateEvent(NULL, TRUE, FALSE, NULL);
         WinThreads[i].hEventRedraw = CreateEvent(NULL, FALSE, FALSE, NULL);
         WinThreads[i].Thread = CreateThread(NULL,
                                             0,
                                             ThreadProc,
                                             (void*) &WinThreads[i],
                                             0,
                                             &id);
         printf("wglthreads: Created thread %p\n", (void *) WinThreads[i].Thread);

         WaitForSingleObject(WinThreads[i].hEventInitialised, INFINITE);

         threads[i] = WinThreads[i].Thread;
      }

      /* Wait for all threads to finish. */
      WaitForMultipleObjects(NumWinThreads, threads, TRUE, INFINITE);
   }

   return 0;
}
