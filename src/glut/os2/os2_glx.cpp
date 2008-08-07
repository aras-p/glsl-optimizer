/* os2_glx.c */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "gl/gl.h"
#include "WarpGL.h"
#include "GL/os2mesa.h"

#define POKA 0
/* global current HDC */

XVisualInfo *wglDescribePixelFormat(int iPixelFormat);

extern HDC XHDC;
extern HWND XHWND;
//extern HPS hpsCurrent;
extern HAB   hab;      /* PM anchor block handle */

GLXContext
glXCreateContext(HPS hps, XVisualInfo * visinfo,
  GLXContext share, Bool direct)
{
  /* KLUDGE: GLX really expects a display pointer to be passed
     in as the first parameter, but Win32 needs an HDC instead,
     so BE SURE that the global XHDC is set before calling this
     routine. */
  HGLRC context;

  context = wglCreateContext(XHDC,hps,hab);


  /* Since direct rendering is implicit, the direct flag is
     ignored. */

  return context;
}


int
glXGetConfig(XVisualInfo * visual, int attrib, int *value)
{
  if (!visual)
    return GLX_BAD_VISUAL;

  switch (attrib) {
  case GLX_USE_GL:
    if (visual->dwFlags & (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW)) {
      /* XXX Brad's Matrix Millenium II has problems creating
         color index windows in 24-bit mode (lead to GDI crash)
         and 32-bit mode (lead to black window).  The cColorBits
         filed of the PIXELFORMATDESCRIPTOR returned claims to
         have 24 and 32 bits respectively of color indices. 2^24
         and 2^32 are ridiculously huge writable colormaps.
         Assume that if we get back a color index
         PIXELFORMATDESCRIPTOR with 24 or more bits, the
         PIXELFORMATDESCRIPTOR doesn't really work and skip it.
         -mjk */
      if (visual->iPixelType == PFD_TYPE_COLORINDEX
        && visual->cColorBits >= 24) {
        *value = 0;
      } else {
       *value = 1;
      }
    } else {
      *value = 0;
    }
    break;
  case GLX_BUFFER_SIZE:
    /* KLUDGE: if we're RGBA, return the number of bits/pixel,
       otherwise, return 8 (we guessed at 256 colors in CI
       mode). */
    if (visual->iPixelType == PFD_TYPE_RGBA)
      *value = visual->cColorBits;
    else
      *value = 8;
    break;
  case GLX_LEVEL:
    /* The bReserved flag of the pfd contains the
       overlay/underlay info. */
    *value = visual->bReserved;
    break;
  case GLX_RGBA:
    *value = visual->iPixelType == PFD_TYPE_RGBA;
    break;
  case GLX_DOUBLEBUFFER:
    *value = visual->dwFlags & PFD_DOUBLEBUFFER;
    break;
  case GLX_STEREO:
    *value = visual->dwFlags & PFD_STEREO;
    break;
  case GLX_AUX_BUFFERS:
    *value = visual->cAuxBuffers;
    break;
  case GLX_RED_SIZE:
    *value = visual->cRedBits;
    break;
  case GLX_GREEN_SIZE:
    *value = visual->cGreenBits;
    break;
  case GLX_BLUE_SIZE:
    *value = visual->cBlueBits;
    break;
  case GLX_ALPHA_SIZE:
    *value = visual->cAlphaBits;
    break;
  case GLX_DEPTH_SIZE:
    *value = visual->cDepthBits;
    break;
  case GLX_STENCIL_SIZE:
    *value = visual->cStencilBits;
    break;
  case GLX_ACCUM_RED_SIZE:
    *value = visual->cAccumRedBits;
    break;
  case GLX_ACCUM_GREEN_SIZE:
    *value = visual->cAccumGreenBits;
    break;
  case GLX_ACCUM_BLUE_SIZE:
    *value = visual->cAccumBlueBits;
    break;
  case GLX_ACCUM_ALPHA_SIZE:
    *value = visual->cAccumAlphaBits;
    break;
#if POKA == 100
#endif /*  POKA == 100 */
  default:
    return GLX_BAD_ATTRIB;
  }
  return 0;
}


XVisualInfo * glXChooseVisual(int mode)
{  int imode = 2;
   if(mode & GLUT_DOUBLE)
            imode = 1;
   return
         wglDescribePixelFormat(imode);
}


#if POKA
#endif /* POKA */

