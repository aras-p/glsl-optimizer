/* prototypes for the Mesa WGL functions */
/* relocated here so that I could make GLUT get them properly */

#ifndef GL_H
#include <gl/gl.h>
#endif

WGLAPI int   GLAPIENTRY wglDeleteContext(HGLRC);
WGLAPI int   GLAPIENTRY wglMakeCurrent(HDC,HGLRC);
WGLAPI int   GLAPIENTRY wglSetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR *);
WGLAPI int   GLAPIENTRY wglSwapBuffers(HDC hdc);
WGLAPI HDC   GLAPIENTRY wglGetCurrentDC(void);
WGLAPI HGLRC GLAPIENTRY wglCreateContext(HDC);
WGLAPI HGLRC GLAPIENTRY wglCreateLayerContext(HDC,int);
WGLAPI HGLRC GLAPIENTRY wglGetCurrentContext(void);
WGLAPI PROC  GLAPIENTRY wglGetProcAddress(const char*);
WGLAPI int   GLAPIENTRY SwapBuffers(HDC);
WGLAPI int   GLAPIENTRY wglChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *);
WGLAPI int   GLAPIENTRY wglCopyContext(HGLRC, HGLRC, unsigned int);
WGLAPI int   GLAPIENTRY wglDeleteContext(HGLRC);
WGLAPI int   GLAPIENTRY wglDescribeLayerPlane(HDC, int, int, unsigned int,LPLAYERPLANEDESCRIPTOR);
WGLAPI int   GLAPIENTRY wglDescribePixelFormat(HDC,int, unsigned int, LPPIXELFORMATDESCRIPTOR);
WGLAPI int   GLAPIENTRY wglGetLayerPaletteEntries(HDC, int, int, int,COLORREF *);
WGLAPI int   GLAPIENTRY wglGetPixelFormat(HDC hdc);
WGLAPI int   GLAPIENTRY wglMakeCurrent(HDC, HGLRC);
WGLAPI int   GLAPIENTRY wglRealizeLayerPalette(HDC, int, int);
WGLAPI int   GLAPIENTRY wglSetLayerPaletteEntries(HDC, int, int, int,const COLORREF *);
WGLAPI int   GLAPIENTRY wglShareLists(HGLRC, HGLRC);
WGLAPI int   GLAPIENTRY wglSwapLayerBuffers(HDC, unsigned int);
WGLAPI int   GLAPIENTRY wglUseFontBitmapsA(HDC, unsigned long, unsigned long, unsigned long);
WGLAPI int   GLAPIENTRY wglUseFontBitmapsW(HDC, unsigned long, unsigned long, unsigned long);
WGLAPI int   GLAPIENTRY wglUseFontOutlinesA(HDC, unsigned long, unsigned long, unsigned long, float,float, int, LPGLYPHMETRICSFLOAT);
WGLAPI int   GLAPIENTRY wglUseFontOutlinesW(HDC, unsigned long, unsigned long, unsigned long, float,float, int, LPGLYPHMETRICSFLOAT);
