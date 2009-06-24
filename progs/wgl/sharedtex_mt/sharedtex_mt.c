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
 *
 * Port to windows by Michal Krol.
 */


#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "opengl32.lib")

struct thread_init_arg {
   int id;
};

struct window {
   CRITICAL_SECTION drawMutex;
   HDC hDC;
   HWND Win;
   HGLRC Context;
   float Angle;
   int Id;
   HGLRC sharedContext;
   HANDLE hEventInitialised;
};


#define MAX_WINDOWS 20
static struct window Windows[MAX_WINDOWS];
static int NumWindows = 0;
static HANDLE terminate = NULL;
static HGLRC gCtx = NULL;
static HDC gHDC = NULL;
static GLuint Textures[3];



static void
Error(const char *msg)
{
   fprintf(stderr, "Error - %s\n", msg);
   exit(1);
}

static void
Resize(struct window *h, unsigned int width, unsigned int height);

static LRESULT CALLBACK
WndProc(HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam )
{
   switch (uMsg) {
   case WM_KEYDOWN:
      SetEvent(terminate);
      break;
   case WM_SIZE:
      {
         LONG index = GetWindowLong(hWnd, GWL_USERDATA);

         if (index >= 0 && index < MAX_WINDOWS) {
            RECT r;

            GetClientRect(hWnd, &r);
            Resize(&Windows[index], r.right, r.bottom);
         }
      }
      break;
   case WM_CREATE:
      {
         CREATESTRUCT *pcs = (CREATESTRUCT *) lParam;

         SetWindowLong(hWnd, GWL_USERDATA, (LONG) pcs->lpCreateParams);
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

static int
initMainthread(void)
{
   WNDCLASS wc = {0};
   HWND win;
   PIXELFORMATDESCRIPTOR pfd = {0};
   int visinfo;

   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "sharedtex_mt.hidden";
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   RegisterClass(&wc);

   win = CreateWindowEx(0,
                        wc.lpszClassName,
                        "sharedtex_mt.hidden",
                        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        wc.hInstance,
                        (LPVOID) -1);
   if (!win) {
      Error("Couldn't create window");
   }

   gHDC = GetDC(win);
   if (!gHDC) {
      Error("Couldn't obtain HDC");
   }

   pfd.cColorBits = 24;
   pfd.cDepthBits = 24;
   pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;

   visinfo = ChoosePixelFormat(gHDC, &pfd);
   if (!visinfo) {
      Error("Unable to find RGB, Z, double-buffered visual");
   }

   SetPixelFormat(gHDC, visinfo, &pfd);
   gCtx = wglCreateContext(gHDC);
   if (!gCtx) {
      Error("Couldn't create WGL context");
   }

   return 0;
}

static struct window *
AddWindow(int xpos, int ypos, HGLRC sCtx)
{
   struct window *win = &Windows[NumWindows];
   WNDCLASS wc = {0};
   int width = 300, height = 300;

   if (NumWindows >= MAX_WINDOWS)
      return NULL;

   memset(win, 0, sizeof(*win));
   InitializeCriticalSection(&win->drawMutex);
   win->Angle = 0.0;
   win->Id = NumWindows++;

   wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "sharedtex_mt";
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   RegisterClass(&wc);

   win->Win = CreateWindowEx(0,
                             wc.lpszClassName,
                             "sharedtex_mt",
                             WS_SIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                             xpos,
                             ypos,
                             width,
                             height,
                             NULL,
                             NULL,
                             wc.hInstance,
                             (LPVOID) win->Id);
   if (!win->Win) {
      Error("Couldn't create window");
   }

   win->sharedContext = sCtx;

   ShowWindow(win->Win, SW_SHOW);

   return win;
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
   EnterCriticalSection(&h->drawMutex);
   if (!wglMakeCurrent(h->hDC, h->Context)) {
      LeaveCriticalSection(&h->drawMutex);
      Error("wglMakeCurrent failed in Redraw");
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

   SwapBuffers(h->hDC);

   if (!wglMakeCurrent(NULL, NULL)) {
      Error("wglMakeCurrent failed in Redraw");
   }
   LeaveCriticalSection(&h->drawMutex);
}

static DWORD WINAPI
threadRunner (void *arg)
{
   struct thread_init_arg *tia = (struct thread_init_arg *) arg;
   struct window *win;
   PIXELFORMATDESCRIPTOR pfd = {0};
   int visinfo;

   win = &Windows[tia->id];

   win->hDC = GetDC(win->Win);
   if (!win->hDC) {
      Error("Couldn't obtain HDC");
   }

   /* Wait for the previous thread */
   if(tia->id > 0)
      WaitForSingleObject(Windows[tia->id - 1].hEventInitialised, INFINITE);

   pfd.cColorBits = 24;
   pfd.cDepthBits = 24;
   pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;

   visinfo = ChoosePixelFormat(win->hDC, &pfd);
   if (!visinfo) {
      Error("Unable to find RGB, Z, double-buffered visual");
   }

   SetPixelFormat(win->hDC, visinfo, &pfd);
   win->Context = wglCreateContext(win->hDC);
   if (!win->Context) {
      Error("Couldn't create WGL context");
   }

   if (win->sharedContext) {
      if(!wglShareLists(win->sharedContext, win->Context))
         Error("Couldn't share WGL context lists");
   }

   SetEvent(win->hEventInitialised);

   /* Wait for all threads to initialize otherwise wglShareLists will fail */
   if(tia->id < NumWindows - 1)
      WaitForSingleObject(Windows[NumWindows - 1].hEventInitialised, INFINITE);

   SendMessage(win->Win, WM_SIZE, 0, 0);

   while (1) {
      MSG msg;

      /* wait 1 ms for signal either to exit or process messages */
      switch (MsgWaitForMultipleObjects(1, &terminate, FALSE, 1, QS_ALLINPUT)) {
      case WAIT_OBJECT_0:
         SendMessage(win->Win, WM_CLOSE, 0, 0);
         break;
      case WAIT_OBJECT_0 + 1:
         break;
      }

      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            return 0;
         }
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      Redraw(win);
   }

   return 0;
}

static void
Resize(struct window *h, unsigned int width, unsigned int height)
{
   if (!h->Context)
      return;

   EnterCriticalSection(&h->drawMutex);

   if (!wglMakeCurrent(h->hDC, h->Context)) {
      LeaveCriticalSection(&h->drawMutex);
      Error("wglMakeCurrent failed in Resize()");
      return;
   }

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 2, 10);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -4.5);
   if (!wglMakeCurrent(NULL, NULL)) {
      Error("wglMakeCurrent failed in Resize()");
   }
   LeaveCriticalSection(&h->drawMutex);
}

int
main(int argc, char *argv[])
{
   struct thread_init_arg tia[MAX_WINDOWS];
   struct window *h[MAX_WINDOWS];
   HANDLE threads[MAX_WINDOWS];
   int i;

   terminate = CreateEvent(NULL, TRUE, FALSE, NULL);

   if (initMainthread())
      return -1;

   /* four windows and contexts sharing display lists and texture objects */
   h[0] = AddWindow( 10,  10, gCtx);
   h[1] = AddWindow(330,  10, gCtx);
   h[2] = AddWindow( 10, 350, gCtx);
   h[3] = AddWindow(330, 350, gCtx);

   for (i = 0; i < NumWindows; i++) {
      Windows[i].hEventInitialised = CreateEvent(NULL, TRUE, FALSE, NULL);
   }

   for (i = 0; i < NumWindows; i++) {
      DWORD id;

      tia[i].id = i;
      threads[i] = CreateThread(NULL, 0, threadRunner, &tia[i], 0, &id);

      WaitForSingleObject(Windows[i].hEventInitialised, INFINITE);
   }

   if (!wglMakeCurrent(gHDC, gCtx)) {
      Error("wglMakeCurrent failed for init thread.");
      return -1;
   }

   InitGLstuff();

   while (1) {
      MSG msg;

      /* wait 1 ms for signal either to exit or process messages */
      switch (MsgWaitForMultipleObjects(NumWindows, threads, TRUE, 1, QS_ALLINPUT)) {
      case WAIT_OBJECT_0:
         return 0;
      }

      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            return 0;
         }
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return 0;
}
