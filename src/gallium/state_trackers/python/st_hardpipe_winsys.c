/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/**
 * @file
 * Get a hardware accelerated Gallium screen/context from the OpenGL driver.
 */


#include "pipe/p_compiler.h"

#ifdef PIPE_OS_WINDOWS
#include <windows.h>
#include <GL/gl.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "st_winsys.h"


typedef struct pipe_screen * (GLAPIENTRY *PFNGETGALLIUMSCREENMESAPROC) (void);
typedef struct pipe_context * (GLAPIENTRY* PFNCREATEGALLIUMCONTEXTMESAPROC) (void);

static PFNGETGALLIUMSCREENMESAPROC pfnGetGalliumScreenMESA = NULL;
static PFNCREATEGALLIUMCONTEXTMESAPROC pfnCreateGalliumContextMESA = NULL;


/* XXX: Force init_gallium symbol to be linked */
extern void init_gallium(void);
void (*force_init_gallium_linkage)(void) = &init_gallium;


#ifdef PIPE_OS_WINDOWS

static INLINE boolean
st_hardpipe_load(void)
{
   WNDCLASS wc;
   HWND hwnd;
   HGLRC hglrc;
   HDC hdc;
   PIXELFORMATDESCRIPTOR pfd;
   int iPixelFormat;

   if(pfnGetGalliumScreenMESA && pfnCreateGalliumContextMESA)
      return TRUE;

   memset(&wc, 0, sizeof wc);
   wc.lpfnWndProc = DefWindowProc;
   wc.lpszClassName = "gallium";
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   RegisterClass(&wc);

   hwnd = CreateWindow(wc.lpszClassName, "gallium", 0, 0, 0, 0, 0, NULL, 0, wc.hInstance, NULL);
   if (!hwnd)
      return FALSE;

   hdc = GetDC(hwnd);
   if (!hdc)
      return FALSE;

   pfd.cColorBits = 3;
   pfd.cRedBits = 1;
   pfd.cGreenBits = 1;
   pfd.cBlueBits = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;

   iPixelFormat = ChoosePixelFormat(hdc, &pfd);
   if (!iPixelFormat) {
      pfd.dwFlags |= PFD_DOUBLEBUFFER;
      iPixelFormat = ChoosePixelFormat(hdc, &pfd);
   }
   if (!iPixelFormat)
      return FALSE;

   SetPixelFormat(hdc, iPixelFormat, &pfd);
   hglrc = wglCreateContext(hdc);
   if (!hglrc)
      return FALSE;

   if (!wglMakeCurrent(hdc, hglrc))
      return FALSE;

   pfnGetGalliumScreenMESA = (PFNGETGALLIUMSCREENMESAPROC)wglGetProcAddress("wglGetGalliumScreenMESA");
   if(!pfnGetGalliumScreenMESA)
      return FALSE;

   pfnCreateGalliumContextMESA = (PFNCREATEGALLIUMCONTEXTMESAPROC)wglGetProcAddress("wglCreateGalliumContextMESA");
   if(!pfnCreateGalliumContextMESA)
      return FALSE;

   DestroyWindow(hwnd);

   return TRUE;
}

#else

static INLINE boolean
st_hardpipe_load(void)
{
   Display *dpy;
   int scrnum;
   Window root;
   int attribSingle[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      None };
   int attribDouble[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None };
   XVisualInfo *visinfo;
   GLXContext ctx = NULL;
   XSetWindowAttributes attr;
   unsigned long mask;
   int width = 100, height = 100;
   Window win;

   dpy = XOpenDisplay(NULL);
   if (!dpy)
      return FALSE;

   scrnum = 0;
   
   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attribSingle);
   if (!visinfo)
      visinfo = glXChooseVisual(dpy, scrnum, attribDouble);
   if (!visinfo)
      return FALSE;
      
   ctx = glXCreateContext( dpy, visinfo, NULL, True );

   if (!ctx)
      return FALSE;

   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   
   win = XCreateWindow(dpy, root, 0, 0, width, height,
                       0, visinfo->depth, InputOutput,
                       visinfo->visual, mask, &attr);

   if (!glXMakeCurrent(dpy, win, ctx))
      return FALSE;

   pfnGetGalliumScreenMESA = (PFNGETGALLIUMSCREENMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXGetGalliumScreenMESA");
   if(!pfnGetGalliumScreenMESA)
      return FALSE;

   pfnCreateGalliumContextMESA = (PFNCREATEGALLIUMCONTEXTMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateGalliumContextMESA");
   if(!pfnCreateGalliumContextMESA)
      return FALSE;

   glXDestroyContext(dpy, ctx);
   XFree(visinfo);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return TRUE;
}

#endif


static struct pipe_screen *
st_hardpipe_screen_create(void)
{
   if(st_hardpipe_load())
      return pfnGetGalliumScreenMESA();
   else
      return st_softpipe_winsys.screen_create();
}


const struct st_winsys st_hardpipe_winsys = {
   &st_hardpipe_screen_create
};
