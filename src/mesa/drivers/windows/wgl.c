/* $Id: wgl.c,v 1.4 2000/11/22 08:55:53 joukj Exp $ */

/*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*/

/*
* File name 	: wgl.c
* WGL stuff. Added by Oleg Letsinsky, ajl@ultersys.ru
* Some things originated from the 3Dfx WGL functions
*/

#ifdef WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
//#include <GL/glu.h>

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <tchar.h>
#include "wmesadef.h"
#include "GL/wmesa.h"
#include "mtypes.h"

#define MAX_MESA_ATTRS	20

struct __extensions__
{
    PROC	proc;
    char	*name;
};

struct __pixelformat__
{
    PIXELFORMATDESCRIPTOR	pfd;
    GLboolean doubleBuffered;
};

struct __extensions__	ext[] = {

#ifdef GL_EXT_polygon_offset
   { (PROC)glPolygonOffsetEXT,			"glPolygonOffsetEXT"		},
#endif
   { (PROC)glBlendEquationEXT,			"glBlendEquationEXT"		},
   { (PROC)glBlendColorEXT,			"glBlendColorExt"		},
   { (PROC)glVertexPointerEXT,			"glVertexPointerEXT"		},
   { (PROC)glNormalPointerEXT,			"glNormalPointerEXT"		},
   { (PROC)glColorPointerEXT,			"glColorPointerEXT"		},
   { (PROC)glIndexPointerEXT,			"glIndexPointerEXT"		},
   { (PROC)glTexCoordPointerEXT,		"glTexCoordPointer"		},
   { (PROC)glEdgeFlagPointerEXT,		"glEdgeFlagPointerEXT"		},
   { (PROC)glGetPointervEXT,			"glGetPointervEXT"		},
   { (PROC)glArrayElementEXT,			"glArrayElementEXT"		},
   { (PROC)glDrawArraysEXT,			"glDrawArrayEXT"		},
   { (PROC)glAreTexturesResidentEXT,		"glAreTexturesResidentEXT"	},
   { (PROC)glBindTextureEXT,			"glBindTextureEXT"		},
   { (PROC)glDeleteTexturesEXT,			"glDeleteTexturesEXT"		},
   { (PROC)glGenTexturesEXT,			"glGenTexturesEXT"		},
   { (PROC)glIsTextureEXT,			"glIsTextureEXT"		},
   { (PROC)glPrioritizeTexturesEXT,		"glPrioritizeTexturesEXT"	},
   { (PROC)glCopyTexSubImage3DEXT,		"glCopyTexSubImage3DEXT"	},
   { (PROC)glTexImage3DEXT,			"glTexImage3DEXT"		},
   { (PROC)glTexSubImage3DEXT,			"glTexSubImage3DEXT"		},
   { (PROC)glColorTableEXT,			"glColorTableEXT"		},
   { (PROC)glColorSubTableEXT,			"glColorSubTableEXT"		},
   { (PROC)glGetColorTableEXT,			"glGetColorTableEXT"		},
   { (PROC)glGetColorTableParameterfvEXT,	"glGetColorTableParameterfvEXT"	},
   { (PROC)glGetColorTableParameterivEXT,	"glGetColorTableParameterivEXT"	},
   { (PROC)glPointParameterfEXT,		"glPointParameterfEXT"		},
   { (PROC)glPointParameterfvEXT,		"glPointParameterfvEXT"		},
   { (PROC)glBlendFuncSeparateEXT,		"glBlendFuncSeparateEXT"	},
   { (PROC)glLockArraysEXT,			"glLockArraysEXT"		},
   { (PROC)glUnlockArraysEXT,			"glUnlockArraysEXT"		}
};

int qt_ext = sizeof(ext) / sizeof(ext[0]);

struct __pixelformat__	pix[] =
{
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_TRUE
    },
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_FALSE
    },
};

int				qt_pix = sizeof(pix) / sizeof(pix[0]);

typedef struct {
    WMesaContext ctx;
    HDC hdc;
} MesaWglCtx;

#define MESAWGL_CTX_MAX_COUNT 20

static MesaWglCtx wgl_ctx[MESAWGL_CTX_MAX_COUNT];

static unsigned ctx_count = 0;
static unsigned ctx_current = -1;
static unsigned curPFD = 0;

GLAPI BOOL GLWINAPI wglCopyContext(HGLRC hglrcSrc,HGLRC hglrcDst,UINT mask)
{
    return(FALSE);
}

GLAPI HGLRC GLWINAPI wglCreateContext(HDC hdc)
{
    HWND		hWnd;
    int i = 0;
    if(!(hWnd = WindowFromDC(hdc)))
    {
        SetLastError(0);
        return(NULL);
    }
    if (!ctx_count)
    {
    	for(i=0;i<MESAWGL_CTX_MAX_COUNT;i++)
    	{
    		wgl_ctx[i].ctx = NULL;
    		wgl_ctx[i].hdc = NULL;
    	}
    }
    for( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
        if ( wgl_ctx[i].ctx == NULL )
        {
            wgl_ctx[i].ctx = WMesaCreateContext( hWnd, NULL, GL_TRUE,
                pix[curPFD-1].doubleBuffered );
            if (wgl_ctx[i].ctx == NULL)
                break;
            wgl_ctx[i].hdc = hdc;
            ctx_count++;
            return ((HGLRC)wgl_ctx[i].ctx);
        }
    }
    SetLastError(0);
    return(NULL);
}

GLAPI BOOL GLWINAPI wglDeleteContext(HGLRC hglrc)
{
    int i;
    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
    	if ( wgl_ctx[i].ctx == (PWMC) hglrc )
    	{
            WMesaMakeCurrent((PWMC) hglrc);
            WMesaDestroyContext();
            wgl_ctx[i].ctx = NULL;
            wgl_ctx[i].hdc = NULL;
            ctx_count--;
            return(TRUE);
    	}
    }
    SetLastError(0);
    return(FALSE);
}

GLAPI HGLRC GLWINAPI wglCreateLayerContext(HDC hdc,int iLayerPlane)
{
    SetLastError(0);
    return(NULL);
}

GLAPI HGLRC GLWINAPI wglGetCurrentContext(VOID)
{
   if (ctx_current < 0)
      return 0;
   else
      return (HGLRC) wgl_ctx[ctx_current].ctx;
}

GLAPI HDC GLWINAPI wglGetCurrentDC(VOID)
{
   if (ctx_current < 0)
      return 0;
   else
      return wgl_ctx[ctx_current].hdc;
}

GLAPI BOOL GLWINAPI wglMakeCurrent(HDC hdc,HGLRC hglrc)
{
    int i;

    /* new code suggested by Andy Sy */
    if (!hdc || !hglrc) {
       WMesaMakeCurrent(NULL);
       ctx_current = -1;
       return TRUE;
    }

    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
        if ( wgl_ctx[i].ctx == (PWMC) hglrc )
        {
            wgl_ctx[i].hdc = hdc;
            WMesaMakeCurrent( (WMesaContext) hglrc );
            ctx_current = i;
            return TRUE;
        }
    }
    return FALSE;
}

GLAPI BOOL GLWINAPI wglShareLists(HGLRC hglrc1,HGLRC hglrc2)
{
    return(TRUE);
}


static FIXED FixedFromDouble(double d)
{
   long l = (long) (d * 65536L);
   return *(FIXED *)&l;
}


GLAPI BOOL GLWINAPI wglUseFontBitmapsA(HDC hdc, DWORD first,
                                       DWORD count, DWORD listBase)
{
   int i;
   GLuint font_list;
   DWORD size;
   GLYPHMETRICS gm;
   HANDLE hBits;
   LPSTR lpBits;
   MAT2 mat;

   if (first<0)
      return FALSE;
   if (count<0)
      return FALSE;
   if (listBase<0)
      return FALSE;

   font_list = glGenLists( count );
   if(font_list == 0)
      return FALSE;

   mat.eM11 = FixedFromDouble(1);
   mat.eM12 = FixedFromDouble(0);
   mat.eM21 = FixedFromDouble(0);
   mat.eM22 = FixedFromDouble(1);

   memset(&gm,0,sizeof(gm));

   for (i = 0; i < count; i++)
   {
      glNewList( font_list+i, GL_COMPILE );

      /* allocate space for the bitmap/outline */
      size = GetGlyphOutline(hdc, first + i, GGO_BITMAP, &gm, 0, NULL, &mat);
      if (size == GDI_ERROR)
      {
         DWORD err;
         err = GetLastError();
         return(FALSE);
      }

      hBits  = GlobalAlloc(GHND, size);
      lpBits = GlobalLock(hBits);

      GetGlyphOutline(hdc,    /* handle to device context */
                      first + i,          /* character to query */
                      GGO_BITMAP,         /* format of data to return */
                      &gm,                /* pointer to structure for metrics */
                      size,               /* size of buffer for data */
                      lpBits,             /* pointer to buffer for data */
                      &mat                /* pointer to transformation */
                                          /* matrix structure */
                  );

      if (*lpBits == GDI_ERROR)
      {
         DWORD err;
         err = GetLastError();

         GlobalUnlock(hBits);
         GlobalFree(hBits);

         return(FALSE);
      }

      glBitmap(gm.gmBlackBoxX,gm.gmBlackBoxY,
               gm.gmptGlyphOrigin.x,
               gm.gmptGlyphOrigin.y,
               gm.gmCellIncX,gm.gmCellIncY,
               (const GLubyte * )lpBits);

      GlobalUnlock(hBits);
      GlobalFree(hBits);

      glEndList( );
   }

    return TRUE;
}

GLAPI BOOL GLWINAPI wglUseFontBitmapsW(HDC hdc,DWORD first,DWORD count,DWORD listBase)
{
    return FALSE;
}

GLAPI BOOL GLWINAPI wglUseFontOutlinesA(HDC hdc,DWORD first,DWORD count,
                                  DWORD listBase,FLOAT deviation,
                                  FLOAT extrusion,int format,
                                  LPGLYPHMETRICSFLOAT lpgmf)
{
    SetLastError(0);
    return(FALSE);
}

GLAPI BOOL GLWINAPI wglUseFontOutlinesW(HDC hdc,DWORD first,DWORD count,
                                  DWORD listBase,FLOAT deviation,
                                  FLOAT extrusion,int format,
                                  LPGLYPHMETRICSFLOAT lpgmf)
{
    SetLastError(0);
    return(FALSE);
}

GLAPI BOOL GLWINAPI wglDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane,UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    SetLastError(0);
    return(FALSE);
}

GLAPI int GLWINAPI wglSetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       CONST COLORREF *pcr)
{
    SetLastError(0);
    return(0);
}

GLAPI int GLWINAPI wglGetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       COLORREF *pcr)
{
    SetLastError(0);
    return(0);
}

GLAPI BOOL GLWINAPI wglRealizeLayerPalette(HDC hdc,int iLayerPlane,BOOL bRealize)
{
    SetLastError(0);
    return(FALSE);
}

GLAPI BOOL GLWINAPI wglSwapLayerBuffers(HDC hdc,UINT fuPlanes)
{
    if( !hdc )
    {
        WMesaSwapBuffers();
        return(TRUE);
    }
    SetLastError(0);
    return(FALSE);
}

GLAPI int GLWINAPI wglChoosePixelFormat(HDC hdc,
                                  CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    int		i,best = -1,bestdelta = 0x7FFFFFFF,delta,qt_valid_pix;

    qt_valid_pix = qt_pix;
    if(ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR) || ppfd->nVersion != 1)
    {
        SetLastError(0);
        return(0);
    }
    for(i = 0;i < qt_valid_pix;i++)
    {
        delta = 0;
        if(
            (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) &&
            !(pix[i].pfd.dwFlags & PFD_DRAW_TO_WINDOW))
            continue;
        if(
            (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) &&
            !(pix[i].pfd.dwFlags & PFD_DRAW_TO_BITMAP))
            continue;
        if(
            (ppfd->dwFlags & PFD_SUPPORT_GDI) &&
            !(pix[i].pfd.dwFlags & PFD_SUPPORT_GDI))
            continue;
        if(
            (ppfd->dwFlags & PFD_SUPPORT_OPENGL) &&
            !(pix[i].pfd.dwFlags & PFD_SUPPORT_OPENGL))
            continue;
        if(
            !(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
            ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != (pix[i].pfd.dwFlags & PFD_DOUBLEBUFFER)))
            continue;
        if(
            !(ppfd->dwFlags & PFD_STEREO_DONTCARE) &&
            ((ppfd->dwFlags & PFD_STEREO) != (pix[i].pfd.dwFlags & PFD_STEREO)))
            continue;
        if(ppfd->iPixelType != pix[i].pfd.iPixelType)
            delta++;
        if(delta < bestdelta)
        {
            best = i + 1;
            bestdelta = delta;
            if(bestdelta == 0)
                break;
        }
    }
    if(best == -1)
    {
        SetLastError(0);
        return(0);
    }
    return(best);
}

GLAPI int GLWINAPI wglDescribePixelFormat(HDC hdc,int iPixelFormat,UINT nBytes,
                                    LPPIXELFORMATDESCRIPTOR ppfd)
{
    int		qt_valid_pix;

    qt_valid_pix = qt_pix;
    if(iPixelFormat < 1 || iPixelFormat > qt_valid_pix || nBytes != sizeof(PIXELFORMATDESCRIPTOR))
    {
        SetLastError(0);
        return(0);
    }
    *ppfd = pix[iPixelFormat - 1].pfd;
    return(qt_valid_pix);
}

/*
* GetProcAddress - return the address of an appropriate extension
*/
GLAPI PROC GLWINAPI wglGetProcAddress(LPCSTR lpszProc)
{
    int		i;
    for(i = 0;i < qt_ext;i++)
        if(!strcmp(lpszProc,ext[i].name))
            return(ext[i].proc);

        SetLastError(0);
        return(NULL);
}

GLAPI int GLWINAPI wglGetPixelFormat(HDC hdc)
{
    if(curPFD == 0)
    {
        SetLastError(0);
        return(0);
    }
    return(curPFD);
}

GLAPI BOOL GLWINAPI wglSetPixelFormat(HDC hdc,int iPixelFormat,
                                PIXELFORMATDESCRIPTOR *ppfd)
{
    int		qt_valid_pix;

    qt_valid_pix = qt_pix;
    if(iPixelFormat < 1 || iPixelFormat > qt_valid_pix || ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR))
    {
        SetLastError(0);
        return(FALSE);
    }
    curPFD = iPixelFormat;
    return(TRUE);
}

GLAPI BOOL GLWINAPI wglSwapBuffers(HDC hdc)
{
   if (ctx_current < 0)
      return FALSE;

   if(wgl_ctx[ctx_current].ctx == NULL) {
      SetLastError(0);
      return(FALSE);
   }
   WMesaSwapBuffers();
   return(TRUE);
}

#endif /* WIN32 */
