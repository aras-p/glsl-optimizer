/* $Id: fxwgl.c,v 1.14 2001/09/23 16:50:01 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Keith Whitwell
 */

/* fxwgl.c - Microsoft wgl functions emulation for
 *           3Dfx VooDoo/Mesa interface
 */


#ifdef __WIN32__

#ifdef __cplusplus
extern "C"
{
#endif

#include "fxdrv.h"
#include <windows.h>
#include "GL/gl.h"

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include "GL/fxmesa.h"

#define MAX_MESA_ATTRS  20

struct __extensions__
{
   PROC proc;
   char *name;
};

struct __pixelformat__
{
   PIXELFORMATDESCRIPTOR pfd;
   GLint mesaAttr[MAX_MESA_ATTRS];
};

WINGDIAPI void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint *);

static struct __extensions__ ext[] = {

#ifdef GL_EXT_polygon_offset
   {(PROC) glPolygonOffsetEXT, "glPolygonOffsetEXT"},
#endif
   {(PROC) glBlendEquationEXT, "glBlendEquationEXT"},
   {(PROC) glBlendColorEXT, "glBlendColorExt"},
   {(PROC) glVertexPointerEXT, "glVertexPointerEXT"},
   {(PROC) glNormalPointerEXT, "glNormalPointerEXT"},
   {(PROC) glColorPointerEXT, "glColorPointerEXT"},
   {(PROC) glIndexPointerEXT, "glIndexPointerEXT"},
   {(PROC) glTexCoordPointerEXT, "glTexCoordPointer"},
   {(PROC) glEdgeFlagPointerEXT, "glEdgeFlagPointerEXT"},
   {(PROC) glGetPointervEXT, "glGetPointervEXT"},
   {(PROC) glArrayElementEXT, "glArrayElementEXT"},
   {(PROC) glDrawArraysEXT, "glDrawArrayEXT"},
   {(PROC) glAreTexturesResidentEXT, "glAreTexturesResidentEXT"},
   {(PROC) glBindTextureEXT, "glBindTextureEXT"},
   {(PROC) glDeleteTexturesEXT, "glDeleteTexturesEXT"},
   {(PROC) glGenTexturesEXT, "glGenTexturesEXT"},
   {(PROC) glIsTextureEXT, "glIsTextureEXT"},
   {(PROC) glPrioritizeTexturesEXT, "glPrioritizeTexturesEXT"},
   {(PROC) glCopyTexSubImage3DEXT, "glCopyTexSubImage3DEXT"},
   {(PROC) glTexImage3DEXT, "glTexImage3DEXT"},
   {(PROC) glTexSubImage3DEXT, "glTexSubImage3DEXT"},
   {(PROC) gl3DfxSetPaletteEXT, "3DFX_set_global_palette"},
   {(PROC) glColorTableEXT, "glColorTableEXT"},
   {(PROC) glColorSubTableEXT, "glColorSubTableEXT"},
   {(PROC) glGetColorTableEXT, "glGetColorTableEXT"},
   {(PROC) glGetColorTableParameterfvEXT, "glGetColorTableParameterfvEXT"},
   {(PROC) glGetColorTableParameterivEXT, "glGetColorTableParameterivEXT"},
   {(PROC) glPointParameterfEXT, "glPointParameterfEXT"},
   {(PROC) glPointParameterfvEXT, "glPointParameterfvEXT"},
   {(PROC) glBlendFuncSeparateINGR, "glBlendFuncSeparateINGR"},
   {(PROC) glActiveTextureARB, "glActiveTextureARB"},
   {(PROC) glClientActiveTextureARB, "glClientActiveTextureARB"},
   {(PROC) glMultiTexCoord1dARB, "glMultiTexCoord1dARB"},
   {(PROC) glMultiTexCoord1dvARB, "glMultiTexCoord1dvARB"},
   {(PROC) glMultiTexCoord1fARB, "glMultiTexCoord1fARB"},
   {(PROC) glMultiTexCoord1fvARB, "glMultiTexCoord1fvARB"},
   {(PROC) glMultiTexCoord1iARB, "glMultiTexCoord1iARB"},
   {(PROC) glMultiTexCoord1ivARB, "glMultiTexCoord1ivARB"},
   {(PROC) glMultiTexCoord1sARB, "glMultiTexCoord1sARB"},
   {(PROC) glMultiTexCoord1svARB, "glMultiTexCoord1svARB"},
   {(PROC) glMultiTexCoord2dARB, "glMultiTexCoord2dARB"},
   {(PROC) glMultiTexCoord2dvARB, "glMultiTexCoord2dvARB"},
   {(PROC) glMultiTexCoord2fARB, "glMultiTexCoord2fARB"},
   {(PROC) glMultiTexCoord2fvARB, "glMultiTexCoord2fvARB"},
   {(PROC) glMultiTexCoord2iARB, "glMultiTexCoord2iARB"},
   {(PROC) glMultiTexCoord2ivARB, "glMultiTexCoord2ivARB"},
   {(PROC) glMultiTexCoord2sARB, "glMultiTexCoord2sARB"},
   {(PROC) glMultiTexCoord2svARB, "glMultiTexCoord2svARB"},
   {(PROC) glMultiTexCoord3dARB, "glMultiTexCoord3dARB"},
   {(PROC) glMultiTexCoord3dvARB, "glMultiTexCoord3dvARB"},
   {(PROC) glMultiTexCoord3fARB, "glMultiTexCoord3fARB"},
   {(PROC) glMultiTexCoord3fvARB, "glMultiTexCoord3fvARB"},
   {(PROC) glMultiTexCoord3iARB, "glMultiTexCoord3iARB"},
   {(PROC) glMultiTexCoord3ivARB, "glMultiTexCoord3ivARB"},
   {(PROC) glMultiTexCoord3sARB, "glMultiTexCoord3sARB"},
   {(PROC) glMultiTexCoord3svARB, "glMultiTexCoord3svARB"},
   {(PROC) glMultiTexCoord4dARB, "glMultiTexCoord4dARB"},
   {(PROC) glMultiTexCoord4dvARB, "glMultiTexCoord4dvARB"},
   {(PROC) glMultiTexCoord4fARB, "glMultiTexCoord4fARB"},
   {(PROC) glMultiTexCoord4fvARB, "glMultiTexCoord4fvARB"},
   {(PROC) glMultiTexCoord4iARB, "glMultiTexCoord4iARB"},
   {(PROC) glMultiTexCoord4ivARB, "glMultiTexCoord4ivARB"},
   {(PROC) glMultiTexCoord4sARB, "glMultiTexCoord4sARB"},
   {(PROC) glMultiTexCoord4svARB, "glMultiTexCoord4svARB"},
   {(PROC) glLockArraysEXT, "glLockArraysEXT"},
   {(PROC) glUnlockArraysEXT, "glUnlockArraysEXT"}
};

static int qt_ext = sizeof(ext) / sizeof(ext[0]);

struct __pixelformat__ pix[] = {
   /* None */
   {
    {
     sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     32,
     8, 0, 8, 8, 8, 16, 0, 24,
     0, 0, 0, 0, 0,
     0,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 0,
     FXMESA_DEPTH_SIZE, 0,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
    }
   ,

   /* Alpha */
   {
    {
     sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     32,
     8, 0, 8, 8, 8, 16, 8, 24,
     0, 0, 0, 0, 0,
     0,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 8,
     FXMESA_DEPTH_SIZE, 0,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
    }
   ,

   /* Depth */
   {
    {
     sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     32,
     8, 0, 8, 8, 8, 16, 0, 24,
     0, 0, 0, 0, 0,
     16,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 0,
     FXMESA_DEPTH_SIZE, 16,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
    }
};
static int qt_pix = sizeof(pix) / sizeof(pix[0]);

static fxMesaContext ctx = NULL;
static WNDPROC hWNDOldProc;
static int curPFD = 0;
static HDC hDC;
static HWND hWND;

static GLboolean haveDualHead;

/* For the in-window-rendering hack */

static GLboolean gdiWindowHack;
static GLboolean gdiWindowHackEna;
static void *dibSurfacePtr;
static BITMAPINFO *dibBMI;
static HBITMAP dibHBM;
static HWND dibWnd;

LONG GLAPIENTRY
__wglMonitor(HWND hwnd, UINT message, UINT wParam, LONG lParam)
 {
   long ret;			/* Now gives the resized window at the end to hWNDOldProc */

   if (ctx && hwnd == hWND) {
      switch (message) {
      case WM_PAINT:
      case WM_MOVE:
	 break;
      case WM_DISPLAYCHANGE:
      case WM_SIZE:
	 if (wParam != SIZE_MINIMIZED) {
	    static int moving = 0;
	    if (!moving) {
	       if (fxQueryHardware() != GR_SSTTYPE_VOODOO) {
		  if (!FX_grSstControl(GR_CONTROL_RESIZE)) {
		     moving = 1;
		     SetWindowPos(hwnd, 0, 0, 0, 300, 300,
				  SWP_NOMOVE | SWP_NOZORDER);
		     moving = 0;
		     if (!FX_grSstControl(GR_CONTROL_RESIZE)) {
			/*MessageBox(0,_T("Error changing windowsize"),_T("fxMESA"),MB_OK); */
			PostMessage(hWND, WM_CLOSE, 0, 0);
		     }
		  }
	       }

	       /* Do the clipping in the glide library */
	       FX_grClipWindow(0, 0, FX_grSstScreenWidth(),
			       FX_grSstScreenHeight());
	       /* And let the new size set in the context */
	       fxMesaUpdateScreenSize(ctx);
	    }
	 }
	 break;
      case WM_ACTIVATE:
	 if ((fxQueryHardware() == GR_SSTTYPE_VOODOO) &&
	     (!gdiWindowHack) && (!haveDualHead)) {
	    WORD fActive = LOWORD(wParam);
	    BOOL fMinimized = (BOOL) HIWORD(wParam);

	    if ((fActive == WA_INACTIVE) || fMinimized)
	       FX_grSstControl(GR_CONTROL_DEACTIVATE);
	    else
	       FX_grSstControl(GR_CONTROL_ACTIVATE);
	 }
	 break;
      case WM_SHOWWINDOW:
	 break;
      case WM_SYSKEYDOWN:
      case WM_SYSCHAR:
	 if (gdiWindowHackEna && (VK_RETURN == wParam)) {
	    if (gdiWindowHack) {
	       gdiWindowHack = GL_FALSE;
	       FX_grSstControl(GR_CONTROL_ACTIVATE);
	    }
	    else {
	       gdiWindowHack = GL_TRUE;
	       FX_grSstControl(GR_CONTROL_DEACTIVATE);
	    }
	 }
	 break;
      }
   }

   /* Finaly call the hWNDOldProc, which handles the resize witch the
      now changed window sizes */
   ret = CallWindowProc(hWNDOldProc, hwnd, message, wParam, lParam);

   return (ret);
}

BOOL GLAPIENTRY
wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
   return (FALSE);
}

HGLRC GLAPIENTRY
wglCreateContext(HDC hdc)
{
   HWND hWnd;
   WNDPROC oldProc;
   int error;

   if (ctx) {
      SetLastError(0);
      return (NULL);
   }

   if (!(hWnd = WindowFromDC(hdc))) {
      SetLastError(0);
      return (NULL);
   }

   if (curPFD == 0) {
      SetLastError(0);
      return (NULL);
   }

   if ((oldProc = (WNDPROC) GetWindowLong(hWnd, GWL_WNDPROC)) != __wglMonitor) {
      hWNDOldProc = oldProc;
      SetWindowLong(hWnd, GWL_WNDPROC, (LONG) __wglMonitor);
   }

#ifndef FX_SILENT
   freopen("MESA.LOG", "w", stderr);
#endif

   ShowWindow(hWnd, SW_SHOWNORMAL);
   SetForegroundWindow(hWnd);
   Sleep(100);			/* an hack for win95 */

   if (fxQueryHardware() == GR_SSTTYPE_VOODOO) {
      RECT cliRect;

      GetClientRect(hWnd, &cliRect);
      error = !(ctx =
		fxMesaCreateBestContext((GLuint) hWnd, cliRect.right,
					cliRect.bottom,
					pix[curPFD - 1].mesaAttr));

      if (!error) {
	 /* create the DIB section for windowed rendering */
	 DWORD *p;

	 dibWnd = hWnd;

	 hDC = GetDC(dibWnd);

	 dibBMI =
	    (BITMAPINFO *) MALLOC(sizeof(BITMAPINFO) +
				  (256 * sizeof(RGBQUAD)));

	 memset(dibBMI, 0, sizeof(BITMAPINFO) + (256 * sizeof(RGBQUAD)));

	 dibBMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	 dibBMI->bmiHeader.biWidth = ctx->width;
	 dibBMI->bmiHeader.biHeight = -ctx->height;
	 dibBMI->bmiHeader.biPlanes = (short) 1;
	 dibBMI->bmiHeader.biBitCount = (short) 16;
	 dibBMI->bmiHeader.biCompression = BI_BITFIELDS;
	 dibBMI->bmiHeader.biSizeImage = 0;
	 dibBMI->bmiHeader.biXPelsPerMeter = 0;
	 dibBMI->bmiHeader.biYPelsPerMeter = 0;
	 dibBMI->bmiHeader.biClrUsed = 3;
	 dibBMI->bmiHeader.biClrImportant = 3;

	 p = (DWORD *) dibBMI->bmiColors;
	 p[0] = 0xF800;
	 p[1] = 0x07E0;
	 p[2] = 0x001F;

	 dibHBM =
	    CreateDIBSection(hDC, dibBMI, DIB_RGB_COLORS, &dibSurfacePtr,
			     NULL, 0);

	 ReleaseDC(dibWnd, hDC);

	 gdiWindowHackEna = (dibHBM != NULL ? GL_TRUE : GL_FALSE);

	 if (!getenv("MESA_WGL_FX")
	     || !strcmp(getenv("MESA_WGL_FX"), "fullscreen"))
	    gdiWindowHack = GL_FALSE;
	 else {
	    gdiWindowHack = GL_TRUE;
	    FX_grSstControl(GR_CONTROL_DEACTIVATE);
	 }
      }
   }
   else {
      /* For the Voodoo Rush */

      if (getenv("MESA_WGL_FX")
	  && !strcmp(getenv("MESA_WGL_FX"), "fullscreen")) {
	 RECT cliRect;

	 GetClientRect(hWnd, &cliRect);
	 error = !(ctx =
		   fxMesaCreateBestContext((GLuint) hWnd, cliRect.right,
					   cliRect.bottom,
					   pix[curPFD - 1].mesaAttr));
      }
      else
	 error = !(ctx =
		   fxMesaCreateContext((GLuint) hWnd, GR_RESOLUTION_NONE,
				       GR_REFRESH_75Hz,
				       pix[curPFD - 1].mesaAttr));
   }

   if (getenv("SST_DUALHEAD"))
      haveDualHead =
	 ((atoi(getenv("SST_DUALHEAD")) == 1) ? GL_TRUE : GL_FALSE);
   else
      haveDualHead = GL_FALSE;

   if (error) {
      SetLastError(0);
      return (NULL);
   }

   hDC = hdc;
   hWND = hWnd;

   /* Required by the OpenGL Optimizer 1.1 (is it a Optimizer bug ?) */
   wglMakeCurrent(hdc, (HGLRC) 1);

   return ((HGLRC) 1);
}

HGLRC GLAPIENTRY
wglCreateLayerContext(HDC hdc, int iLayerPlane)
{
   SetLastError(0);
   return (NULL);
}

BOOL GLAPIENTRY
wglDeleteContext(HGLRC hglrc)
{
   if (ctx && hglrc == (HGLRC) 1) {
      if (gdiWindowHackEna) {
	 DeleteObject(dibHBM);
	 FREE(dibBMI);

	 dibSurfacePtr = NULL;
	 dibBMI = NULL;
	 dibHBM = NULL;
	 dibWnd = NULL;
      }

      fxMesaDestroyContext(ctx);

      SetWindowLong(WindowFromDC(hDC), GWL_WNDPROC, (LONG) hWNDOldProc);

      ctx = NULL;
      hDC = 0;
      return (TRUE);
   }

   SetLastError(0);

   return (FALSE);
}

HGLRC GLAPIENTRY
wglGetCurrentContext(VOID)
{
   if (ctx)
      return ((HGLRC) 1);

   SetLastError(0);
   return (NULL);
}

HDC GLAPIENTRY
wglGetCurrentDC(VOID)
{
   if (ctx)
      return (hDC);

   SetLastError(0);
   return (NULL);
}

PROC GLAPIENTRY
wglGetProcAddress(LPCSTR lpszProc)
{
   int i;

   /*fprintf(stderr,"fxMesa: looking for extension %s\n",lpszProc);
      fflush(stderr); */

   for (i = 0; i < qt_ext; i++)
      if (!strcmp(lpszProc, ext[i].name)) {
	 /*fprintf(stderr,"fxMesa: found extension %s\n",lpszProc);
	    fflush(stderr); */

	 return (ext[i].proc);
      }
   SetLastError(0);
   return (NULL);
}

BOOL GLAPIENTRY
wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
   if ((hdc == NULL) && (hglrc == NULL))
      return (TRUE);

   if (!ctx || hglrc != (HGLRC) 1 || WindowFromDC(hdc) != hWND) {
      SetLastError(0);
      return (FALSE);
   }

   hDC = hdc;

   fxMesaMakeCurrent(ctx);

   return (TRUE);
}

BOOL GLAPIENTRY
wglShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
   if (!ctx || hglrc1 != (HGLRC) 1 || hglrc1 != hglrc2) {
      SetLastError(0);
      return (FALSE);
   }

   return (TRUE);
}

BOOL GLAPIENTRY
wglUseFontBitmaps(HDC fontDevice, DWORD firstChar, DWORD numChars,
		  DWORD listBase)
{
#define VERIFY(a) a

   TEXTMETRIC metric;
   BITMAPINFO *dibInfo;
   HDC bitDevice;
   COLORREF tempColor;
   int i;

   VERIFY(GetTextMetrics(fontDevice, &metric));

   dibInfo = (BITMAPINFO *) calloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD), 1);
   dibInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   dibInfo->bmiHeader.biPlanes = 1;
   dibInfo->bmiHeader.biBitCount = 1;
   dibInfo->bmiHeader.biCompression = BI_RGB;

   bitDevice = CreateCompatibleDC(fontDevice);
   // HDC bitDevice = CreateDC("DISPLAY", NULL, NULL, NULL);
   // VERIFY(bitDevice);

   // Swap fore and back colors so the bitmap has the right polarity
   tempColor = GetBkColor(bitDevice);
   SetBkColor(bitDevice, GetTextColor(bitDevice));
   SetTextColor(bitDevice, tempColor);

   // Place chars based on base line
   VERIFY(SetTextAlign(bitDevice, TA_BASELINE) >= 0 ? 1 : 0);

   for (i = 0; i < numChars; i++) {
      SIZE size;
      char curChar;
      int charWidth, charHeight, bmapWidth, bmapHeight, numBytes, res;
      HBITMAP bitObject;
      HGDIOBJ origBmap;
      unsigned char *bmap;

      curChar = i + firstChar;

      // Find how high/wide this character is
      VERIFY(GetTextExtentPoint32(bitDevice, &curChar, 1, &size));

      // Create the output bitmap
      charWidth = size.cx;
      charHeight = size.cy;
      bmapWidth = ((charWidth + 31) / 32) * 32;	// Round up to the next multiple of 32 bits
      bmapHeight = charHeight;
      bitObject = CreateCompatibleBitmap(bitDevice, bmapWidth, bmapHeight);
      //VERIFY(bitObject);

      // Assign the output bitmap to the device
      origBmap = SelectObject(bitDevice, bitObject);
      VERIFY(origBmap);

      VERIFY(PatBlt(bitDevice, 0, 0, bmapWidth, bmapHeight, BLACKNESS));

      // Use our source font on the device
      VERIFY(SelectObject(bitDevice, GetCurrentObject(fontDevice, OBJ_FONT)));

      // Draw the character
      VERIFY(TextOut(bitDevice, 0, metric.tmAscent, &curChar, 1));

      // Unselect our bmap object
      VERIFY(SelectObject(bitDevice, origBmap));

      // Convert the display dependant representation to a 1 bit deep DIB
      numBytes = (bmapWidth * bmapHeight) / 8;
      bmap = MALLOC(numBytes);
      dibInfo->bmiHeader.biWidth = bmapWidth;
      dibInfo->bmiHeader.biHeight = bmapHeight;
      res = GetDIBits(bitDevice, bitObject, 0, bmapHeight, bmap,
		      dibInfo, DIB_RGB_COLORS);
      //VERIFY(res);

      // Create the GL object
      glNewList(i + listBase, GL_COMPILE);
      glBitmap(bmapWidth, bmapHeight, 0.0, metric.tmDescent,
	       charWidth, 0.0, bmap);
      glEndList();
      // CheckGL();

      // Destroy the bmap object
      DeleteObject(bitObject);

      // Deallocate the bitmap data
      FREE(bmap);
   }

   // Destroy the DC
   VERIFY(DeleteDC(bitDevice));

   FREE(dibInfo);

   return TRUE;
#undef VERIFY
}

BOOL GLAPIENTRY
wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
   return (FALSE);
}

BOOL GLAPIENTRY
wglUseFontOutlinesA(HDC hdc, DWORD first, DWORD count,
		    DWORD listBase, FLOAT deviation,
		    FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
   SetLastError(0);
   return (FALSE);
}

BOOL GLAPIENTRY
wglUseFontOutlinesW(HDC hdc, DWORD first, DWORD count,
		    DWORD listBase, FLOAT deviation,
		    FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
   SetLastError(0);
   return (FALSE);
}


BOOL GLAPIENTRY
wglSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
   if (ctx && WindowFromDC(hdc) == hWND) {
      fxMesaSwapBuffers();

      return (TRUE);
   }

   SetLastError(0);
   return (FALSE);
}

int GLAPIENTRY
wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
   int i, best = -1, qt_valid_pix;

   qt_valid_pix = qt_pix;

   if (ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR) || ppfd->nVersion != 1) {
      SetLastError(0);
      return (0);
   }

   for (i = 0; i < qt_valid_pix; i++) {
      if ((ppfd->dwFlags & PFD_DRAW_TO_WINDOW)
	  && !(pix[i].pfd.dwFlags & PFD_DRAW_TO_WINDOW)) continue;
      if ((ppfd->dwFlags & PFD_DRAW_TO_BITMAP)
	  && !(pix[i].pfd.dwFlags & PFD_DRAW_TO_BITMAP)) continue;
      if ((ppfd->dwFlags & PFD_SUPPORT_GDI)
	  && !(pix[i].pfd.dwFlags & PFD_SUPPORT_GDI)) continue;
      if ((ppfd->dwFlags & PFD_SUPPORT_OPENGL)
	  && !(pix[i].pfd.dwFlags & PFD_SUPPORT_OPENGL)) continue;
      if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE)
	  && ((ppfd->dwFlags & PFD_DOUBLEBUFFER) !=
	      (pix[i].pfd.dwFlags & PFD_DOUBLEBUFFER))) continue;
      if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE)
	  && ((ppfd->dwFlags & PFD_STEREO) !=
	      (pix[i].pfd.dwFlags & PFD_STEREO))) continue;

      if (ppfd->cDepthBits > 0 && pix[i].pfd.cDepthBits == 0)
	 continue;		/* need depth buffer */

      if (ppfd->cAlphaBits > 0 && pix[i].pfd.cAlphaBits == 0)
	 continue;		/* need alpha buffer */

      if (ppfd->iPixelType == pix[i].pfd.iPixelType) {
	 best = i + 1;
	 break;
      }
   }

   if (best == -1) {
      SetLastError(0);
      return (0);
   }

   return (best);
}

int GLAPIENTRY
ChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
   return wglChoosePixelFormat(hdc, ppfd);
}

int GLAPIENTRY
wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes,
		       LPPIXELFORMATDESCRIPTOR ppfd)
{
   int qt_valid_pix;

   qt_valid_pix = qt_pix;

   if (iPixelFormat < 1 || iPixelFormat > qt_valid_pix ||
       ((nBytes != sizeof(PIXELFORMATDESCRIPTOR)) && (nBytes != 0))) {
      SetLastError(0);
      return (0);
   }

   if (nBytes != 0)
      *ppfd = pix[iPixelFormat - 1].pfd;

   return (qt_valid_pix);
}

int GLAPIENTRY
DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes,
		    LPPIXELFORMATDESCRIPTOR ppfd)
{
   return wglDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

int GLAPIENTRY
wglGetPixelFormat(HDC hdc)
{
   if (curPFD == 0) {
      SetLastError(0);
      return (0);
   }

   return (curPFD);
}

int GLAPIENTRY
GetPixelFormat(HDC hdc)
{
   return wglGetPixelFormat(hdc);
}

BOOL GLAPIENTRY
wglSetPixelFormat(HDC hdc, int iPixelFormat,
		  CONST PIXELFORMATDESCRIPTOR * ppfd)
{
   int qt_valid_pix;

   qt_valid_pix = qt_pix;

   if (iPixelFormat < 1 || iPixelFormat > qt_valid_pix
       || ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR)) {
      SetLastError(0);
      return (FALSE);
   }
   curPFD = iPixelFormat;

   return (TRUE);
}

BOOL GLAPIENTRY
wglSwapBuffers(HDC hdc)
{
   if (!ctx) {
      SetLastError(0);
      return (FALSE);
   }

   fxMesaSwapBuffers();

   if (gdiWindowHack) {
      GLuint width = ctx->width;
      GLuint height = ctx->height;

      HDC hdcScreen = GetDC(dibWnd);
      HDC hdcDIBSection = CreateCompatibleDC(hdcScreen);
      HBITMAP holdBitmap = (HBITMAP) SelectObject(hdcDIBSection, dibHBM);

      FX_grLfbReadRegion(GR_BUFFER_FRONTBUFFER, 0, 0,
			 width, height, width * 2, dibSurfacePtr);

      /* Since the hardware is configured for GR_COLORFORMAT_ABGR the pixel data is
       * going to come out as BGR 565, which is reverse of what we need for blitting
       * to screen, so we need to convert it here pixel-by-pixel (ick). This loop would NOT
       * be required if the color format was changed to GR_COLORFORMAT_ARGB, but I do
       * not know the ramifications of that, so this will work until that is resolved.
       *
       * This routine CRIES out for MMX implementation, however since that's not
       * guaranteed to be running on MMX enabled hardware so I'm not going to do
       * that. I'm just going to try to make a reasonably efficient C
       * version. -TAJ
       *
       * This routine drops frame rate by <1 fps on a 200Mhz MMX processor with a 640x480
       * display. Obviously, it's performance hit will be higher on larger displays and
       * less on smaller displays. To support the window-hack display this is probably fine.
       */
#if  FXMESA_USE_ARGB
      {
	 unsigned long *pixel = dibSurfacePtr;
	 unsigned long count = (width * height) / 2;

	 while (count--) {
	    *pixel++ = (*pixel & 0x07e007e0)	/* greens */
	       |((*pixel & 0xf800f800) >> 11)	/* swap blues */
	       |((*pixel & 0x001f001f) << 11)	/* swap reds */
	       ;
	 }
      }
#endif

      BitBlt(hdcScreen, 0, 0, width, height, hdcDIBSection, 0, 0, SRCCOPY);

      ReleaseDC(dibWnd, hdcScreen);
      SelectObject(hdcDIBSection, holdBitmap);
      DeleteDC(hdcDIBSection);
   }

   return (TRUE);
}

BOOL GLAPIENTRY
SetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
   return wglSetPixelFormat(hdc, iPixelFormat, ppfd);
}

BOOL GLAPIENTRY
SwapBuffers(HDC hdc)
{
   return wglSwapBuffers(hdc);
}

#endif /* FX */
