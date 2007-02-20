/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  Mesa Software WGL (WindowsGL)
*
****************************************************************************/

#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glheader.h"
#include "colors.h"
#include "context.h"
#include "colormac.h"
#include "dd.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
// #include "mem.h"
//#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "vbo/vbo.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "dglcontext.h"
#include "gld_driver.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

DGL_pixelFormat pfTemplateMesaSW =
{
    {
	sizeof(PIXELFORMATDESCRIPTOR),	// Size of the data structure
		1,							// Structure version - should be 1
									// Flags:
		PFD_DRAW_TO_WINDOW |		// The buffer can draw to a window or device surface.
		PFD_DRAW_TO_BITMAP |		// The buffer can draw to a bitmap. (DaveM)
		PFD_SUPPORT_GDI |			// The buffer supports GDI drawing. (DaveM)
		PFD_SUPPORT_OPENGL |		// The buffer supports OpenGL drawing.
		PFD_DOUBLEBUFFER |			// The buffer is double-buffered.
		0,							// Placeholder for easy commenting of above flags
		PFD_TYPE_RGBA,				// Pixel type RGBA.
		32,							// Total colour bitplanes (excluding alpha bitplanes)
		8, 0,						// Red bits, shift
		8, 8,						// Green bits, shift
		8, 16,						// Blue bits, shift
		8, 24,						// Alpha bits, shift (destination alpha)
		64,							// Accumulator bits (total)
		16, 16, 16, 16,				// Accumulator bits: Red, Green, Blue, Alpha
		16,							// Depth bits
		8,							// Stencil bits
		0,							// Number of auxiliary buffers
		0,							// Layer type
		0,							// Specifies the number of overlay and underlay planes.
		0,							// Layer mask
		0,							// Specifies the transparent color or index of an underlay plane.
		0							// Damage mask
	},
	0,	// Unused
};

//---------------------------------------------------------------------------
// Extensions
//---------------------------------------------------------------------------

typedef struct {
	PROC proc;
	char *name;
}  GLD_extension;

static GLD_extension GLD_extList[] = {
#ifdef GL_EXT_polygon_offset
    {	(PROC)glPolygonOffsetEXT,		"glPolygonOffsetEXT"		},
#endif
    {	(PROC)glBlendEquationEXT,		"glBlendEquationEXT"		},
    {	(PROC)glBlendColorEXT,			"glBlendColorExt"			},
    {	(PROC)glVertexPointerEXT,		"glVertexPointerEXT"		},
    {	(PROC)glNormalPointerEXT,		"glNormalPointerEXT"		},
    {	(PROC)glColorPointerEXT,		"glColorPointerEXT"			},
    {	(PROC)glIndexPointerEXT,		"glIndexPointerEXT"			},
    {	(PROC)glTexCoordPointerEXT,		"glTexCoordPointer"			},
    {	(PROC)glEdgeFlagPointerEXT,		"glEdgeFlagPointerEXT"		},
    {	(PROC)glGetPointervEXT,			"glGetPointervEXT"			},
    {	(PROC)glArrayElementEXT,		"glArrayElementEXT"			},
    {	(PROC)glDrawArraysEXT,			"glDrawArrayEXT"			},
    {	(PROC)glAreTexturesResidentEXT,	"glAreTexturesResidentEXT"	},
    {	(PROC)glBindTextureEXT,			"glBindTextureEXT"			},
    {	(PROC)glDeleteTexturesEXT,		"glDeleteTexturesEXT"		},
    {	(PROC)glGenTexturesEXT,			"glGenTexturesEXT"			},
    {	(PROC)glIsTextureEXT,			"glIsTextureEXT"			},
    {	(PROC)glPrioritizeTexturesEXT,	"glPrioritizeTexturesEXT"	},
    {	(PROC)glCopyTexSubImage3DEXT,	"glCopyTexSubImage3DEXT"	},
    {	(PROC)glTexImage3DEXT,			"glTexImage3DEXT"			},
    {	(PROC)glTexSubImage3DEXT,		"glTexSubImage3DEXT"		},
    {	(PROC)glPointParameterfEXT,		"glPointParameterfEXT"		},
    {	(PROC)glPointParameterfvEXT,	"glPointParameterfvEXT"		},
    {	(PROC)glLockArraysEXT,			"glLockArraysEXT"			},
    {	(PROC)glUnlockArraysEXT,		"glUnlockArraysEXT"			},
	{	NULL,							"\0"						}
};

//---------------------------------------------------------------------------
// WMesa Internal Functions
//---------------------------------------------------------------------------

#define PAGE_FILE	0xffffffff

#define REDBITS		0x03
#define REDSHIFT	0x00
#define GREENBITS	0x03
#define GREENSHIFT	0x03
#define BLUEBITS	0x02
#define BLUESHIFT	0x06

typedef struct _dibSection {
	HDC 	hDC;
	HANDLE	hFileMap;
	BOOL	fFlushed;
	LPVOID	base;
} WMDIBSECTION, *PWMDIBSECTION;

typedef struct wmesa_context {
	HWND				Window;
	HDC 				hDC;
	HPALETTE			hPalette;
	HPALETTE			hOldPalette;
	HPEN				hPen;
	HPEN				hOldPen;
	HCURSOR 			hOldCursor;
	COLORREF			crColor;
	// 3D projection stuff
	RECT				drawRect;
	UINT				uiDIBoffset;
	// OpenGL stuff
	HPALETTE			hGLPalette;
	GLuint				width;
	GLuint				height;
	GLuint				ScanWidth;
	GLboolean			db_flag;	//* double buffered?
	GLboolean			rgb_flag;	//* RGB mode?
	GLboolean			dither_flag;	//* use dither when 256 color mode for RGB?
	GLuint				depth;		//* bits per pixel (1, 8, 24, etc)
	ULONG				pixel;	// current color index or RGBA pixel value
	ULONG				clearpixel; //* pixel for clearing the color buffers
	PBYTE				ScreenMem; // WinG memory
	BITMAPINFO			*IndexFormat;
	HPALETTE			hPal; // Current Palette
	HPALETTE			hPalHalfTone;
	
	
	WMDIBSECTION		dib;
	BITMAPINFO			bmi;
	HBITMAP 			hbmDIB;
	HBITMAP 			hOldBitmap;
	HBITMAP 			Old_Compat_BM;
	HBITMAP 			Compat_BM;			  // Bitmap for double buffering
	PBYTE				pbPixels;
	int 				nColors;
	BYTE				cColorBits;
	int 				pixelformat;
	
	RECT					rectOffScreen;
	RECT					rectSurface;
//	HWND					hwnd;
	DWORD					pitch;
	PBYTE					addrOffScreen;

	// We always double-buffer, for performance reasons, but
	// we need to know which of SwapBuffers() or glFlush() to
	// handle. If we're emulating, then we update on Flush(),
	// otherwise we update on SwapBufers(). KeithH
	BOOL				bEmulateSingleBuffer;
} WMesaContext, *PWMC;

#define GLD_GET_WMESA_DRIVER(c)	(WMesaContext*)(c)->glPriv

// TODO:
GLint stereo_flag = 0 ;

/* If we are double-buffering, we want to get the DC for the
 * off-screen DIB, otherwise the DC for the window.
 */
#define DD_GETDC ((Current->db_flag) ? Current->dib.hDC : Current->hDC )
#define DD_RELEASEDC

#define FLIP(Y)  (Current->height-(Y)-1)

struct DISPLAY_OPTIONS {
	int  stereo;
	int  fullScreen;
	int	 mode;
	int	 bpp;
};

struct DISPLAY_OPTIONS displayOptions;

//---------------------------------------------------------------------------

static unsigned char threeto8[8] = {
  0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

static unsigned char twoto8[4] = {
  0, 0x55, 0xaa, 0xff
};

static unsigned char oneto8[2] = {
  0, 255
};

//---------------------------------------------------------------------------

BYTE DITHER_RGB_2_8BIT( int red, int green, int blue, int pixel, int scanline)
{
  char unsigned redtemp, greentemp, bluetemp, paletteindex;
  
  //*** now, look up each value in the halftone matrix
  //*** using an 8x8 ordered dither.
  redtemp = aDividedBy51[red]
    + (aModulo51[red] > aHalftone8x8[(pixel%8)*8
				    + scanline%8]);
  greentemp = aDividedBy51[(char unsigned)green]
    + (aModulo51[green] > aHalftone8x8[
      (pixel%8)*8 + scanline%8]);
  bluetemp = aDividedBy51[(char unsigned)blue]
    + (aModulo51[blue] > aHalftone8x8[
      (pixel%8)*8 +scanline%8]);
  
  //*** recombine the halftoned rgb values into a palette index
  paletteindex =
    redtemp + aTimes6[greentemp] + aTimes36[bluetemp];
  
  //*** and translate through the wing halftone palette
  //*** translation vector to give the correct value.
  return aWinGHalftoneTranslation[paletteindex];
}

//---------------------------------------------------------------------------

static unsigned char componentFromIndex(UCHAR i, UINT nbits, UINT shift)
{
  unsigned char val;
  
  val = i >> shift;
  switch (nbits) {
    
  case 1:
    val &= 0x1;
    return oneto8[val];
    
  case 2:
    val &= 0x3;
    return twoto8[val];
    
  case 3:
    val &= 0x7;
    return threeto8[val];
    
  default:
    return 0;
  }
}

//---------------------------------------------------------------------------


void wmSetPixel(PWMC pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b)
{
	WMesaContext *Current = pwc;

	// Test for invalid scanline parameter. KeithH
	if ((iScanLine < 0) || (iScanLine >= pwc->height))
		return;

  if (Current->db_flag) {
    LPBYTE  lpb = pwc->pbPixels;
    UINT    nBypp = pwc->cColorBits >> 3;
    UINT    nOffset = iPixel % nBypp;
    
    lpb += pwc->ScanWidth * iScanLine;
    lpb += iPixel * nBypp;
    
    if(nBypp == 1){
      if(pwc->dither_flag)
	*lpb = DITHER_RGB_2_8BIT(r,g,b,iScanLine,iPixel);
      else
	*lpb = BGR8(r,g,b);
    }
    else if(nBypp == 2)
      *((LPWORD)lpb) = BGR16(r,g,b);
    else if (nBypp == 3)
      *((LPDWORD)lpb) = BGR24(r,g,b);
    else if (nBypp == 4)
      *((LPDWORD)lpb) = BGR32(r,g,b);
  }
  else{
    SetPixel(Current->hDC, iPixel, iScanLine, RGB(r,g,b));
  }
}

//---------------------------------------------------------------------------

void  wmCreateDIBSection(
  HDC   hDC,
  PWMC pwc,    // handle of device context
  CONST BITMAPINFO *pbmi,  // bitmap size, format, and color data
  UINT iUsage  // color data type indicator: RGB values or palette indices
  )
{
  DWORD   dwSize = 0;
  DWORD   dwScanWidth;
  UINT    nBypp = pwc->cColorBits / 8;
  HDC     hic;
  
  dwScanWidth = (((pwc->ScanWidth * nBypp)+ 3) & ~3);
  
  pwc->ScanWidth =pwc->pitch = dwScanWidth;
  
  if (stereo_flag)
    pwc->ScanWidth = 2* pwc->pitch;
  
  dwSize = sizeof(BITMAPINFO) + (dwScanWidth * pwc->height);
  
  pwc->dib.hFileMap = CreateFileMapping((HANDLE)PAGE_FILE,
					NULL,
					PAGE_READWRITE | SEC_COMMIT,
					0,
					dwSize,
					NULL);
  
  if (!pwc->dib.hFileMap)
    return;
  
  pwc->dib.base = MapViewOfFile(pwc->dib.hFileMap,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				0);
  
  if(!pwc->dib.base){
    CloseHandle(pwc->dib.hFileMap);
    return;
  }
  

  CopyMemory(pwc->dib.base, pbmi, sizeof(BITMAPINFO));
  
  hic = CreateIC("display", NULL, NULL, NULL);
  pwc->dib.hDC = CreateCompatibleDC(hic);
  

  pwc->hbmDIB = CreateDIBSection(hic,
				 &(pwc->bmi),
				 (iUsage ? DIB_PAL_COLORS : DIB_RGB_COLORS),
				 &(pwc->pbPixels),
				 pwc->dib.hFileMap,
				 0);
  pwc->ScreenMem = pwc->addrOffScreen = pwc->pbPixels;
  pwc->hOldBitmap = SelectObject(pwc->dib.hDC, pwc->hbmDIB);
  
  DeleteDC(hic);
  
  return;
  
}

//---------------------------------------------------------------------------

void wmCreatePalette( PWMC pwdc )
{
  /* Create a compressed and re-expanded 3:3:2 palette */
  int            i;
  LOGPALETTE     *pPal;
  BYTE           rb, rs, gb, gs, bb, bs;
  
  pwdc->nColors = 0x100;
  
  pPal = (PLOGPALETTE)malloc(sizeof(LOGPALETTE) + 
			     pwdc->nColors * sizeof(PALETTEENTRY));
  memset( pPal, 0, sizeof(LOGPALETTE) + pwdc->nColors * sizeof(PALETTEENTRY) );
  
  pPal->palVersion = 0x300;
  
  rb = REDBITS;
  rs = REDSHIFT;
  gb = GREENBITS;
  gs = GREENSHIFT;
  bb = BLUEBITS;
  bs = BLUESHIFT;
  
  if (pwdc->db_flag) {
    
    /* Need to make two palettes: one for the screen DC and one for the DIB. */
    pPal->palNumEntries = pwdc->nColors;
    for (i = 0; i < pwdc->nColors; i++) {
      pPal->palPalEntry[i].peRed = componentFromIndex( i, rb, rs );
      pPal->palPalEntry[i].peGreen = componentFromIndex( i, gb, gs );
      pPal->palPalEntry[i].peBlue = componentFromIndex( i, bb, bs );
      pPal->palPalEntry[i].peFlags = 0;
    }
    pwdc->hGLPalette = CreatePalette( pPal );
    pwdc->hPalette = CreatePalette( pPal );
  }
  
  else {
    pPal->palNumEntries = pwdc->nColors;
    for (i = 0; i < pwdc->nColors; i++) {
      pPal->palPalEntry[i].peRed = componentFromIndex( i, rb, rs );
      pPal->palPalEntry[i].peGreen = componentFromIndex( i, gb, gs );
      pPal->palPalEntry[i].peBlue = componentFromIndex( i, bb, bs );
      pPal->palPalEntry[i].peFlags = 0;
    }
    pwdc->hGLPalette = CreatePalette( pPal );
  }
  
  free(pPal);
  
}

//---------------------------------------------------------------------------

/* This function sets the color table of a DIB section
 * to match that of the destination DC
 */
BOOL wmSetDibColors(PWMC pwc)
{
  RGBQUAD         *pColTab, *pRGB;
  PALETTEENTRY    *pPal, *pPE;
  int             i, nColors;
  BOOL            bRet=TRUE;
  DWORD           dwErr=0;
  
  /* Build a color table in the DIB that maps to the
   *  selected palette in the DC.
   */
  nColors = 1 << pwc->cColorBits;
  pPal = (PALETTEENTRY *)malloc( nColors * sizeof(PALETTEENTRY));
  memset( pPal, 0, nColors * sizeof(PALETTEENTRY) );
  GetPaletteEntries( pwc->hGLPalette, 0, nColors, pPal );
  pColTab = (RGBQUAD *)malloc( nColors * sizeof(RGBQUAD));
  for (i = 0, pRGB = pColTab, pPE = pPal; i < nColors; i++, pRGB++, pPE++) {
    pRGB->rgbRed = pPE->peRed;
    pRGB->rgbGreen = pPE->peGreen;
    pRGB->rgbBlue = pPE->peBlue;
  }
  if(pwc->db_flag)
    bRet = SetDIBColorTable(pwc->dib.hDC, 0, nColors, pColTab );
  
  if(!bRet)
    dwErr = GetLastError();
  
  free( pColTab );
  free( pPal );
  
  return bRet;
}

//---------------------------------------------------------------------------

static void wmSetPixelFormat( PWMC wc, HDC hDC)
{
  if(wc->rgb_flag)
    wc->cColorBits = GetDeviceCaps(hDC, BITSPIXEL);
  else
    wc->cColorBits = 8;
  switch(wc->cColorBits){
  case 8:
    if(wc->dither_flag != GL_TRUE)
      wc->pixelformat = PF_INDEX8;
    else
      wc->pixelformat = PF_DITHER8;
    break;
  case 16:
    wc->pixelformat = PF_5R6G5B;
    break;
  case 32:
    wc->pixelformat = PF_8R8G8B;
    break;
  default:
    wc->pixelformat = PF_BADFORMAT;
  }
}

//---------------------------------------------------------------------------

/*
 * This function creates the DIB section that is used for combined
 * GL and GDI calls
 */
BOOL wmCreateBackingStore(PWMC pwc, long lxSize, long lySize)
{
  HDC hdc = pwc->hDC;
  LPBITMAPINFO pbmi = &(pwc->bmi);
  int     iUsage;
  
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = lxSize;
  pbmi->bmiHeader.biHeight= -lySize;
  pbmi->bmiHeader.biPlanes = 1;
  if(pwc->rgb_flag)
    pbmi->bmiHeader.biBitCount = GetDeviceCaps(pwc->hDC, BITSPIXEL);
  else
    pbmi->bmiHeader.biBitCount = 8;
  pbmi->bmiHeader.biCompression = BI_RGB;
  pbmi->bmiHeader.biSizeImage = 0;
  pbmi->bmiHeader.biXPelsPerMeter = 0;
  pbmi->bmiHeader.biYPelsPerMeter = 0;
  pbmi->bmiHeader.biClrUsed = 0;
  pbmi->bmiHeader.biClrImportant = 0;
  
  iUsage = (pbmi->bmiHeader.biBitCount <= 8) ? DIB_PAL_COLORS : DIB_RGB_COLORS;

  pwc->cColorBits = pbmi->bmiHeader.biBitCount;
  pwc->ScanWidth = pwc->pitch = lxSize;
  pwc->width = lxSize;
  pwc->height = lySize;
  
  wmCreateDIBSection(hdc, pwc, pbmi, iUsage);
  
  if ((iUsage == DIB_PAL_COLORS) && !(pwc->hGLPalette)) {
    wmCreatePalette( pwc );
    wmSetDibColors( pwc );
  }
  wmSetPixelFormat(pwc, pwc->hDC);
  return TRUE;
}

//---------------------------------------------------------------------------

/*
 * Free up the dib section that was created
 */
BOOL wmDeleteBackingStore(PWMC pwc)
{
  SelectObject(pwc->dib.hDC, pwc->hOldBitmap);
  DeleteDC(pwc->dib.hDC);
  DeleteObject(pwc->hbmDIB);
  UnmapViewOfFile(pwc->dib.base);
  CloseHandle(pwc->dib.hFileMap);
  return TRUE;
}

//---------------------------------------------------------------------------

/*
 * Blit memory DC to screen DC
 */
BOOL wmFlush(PWMC pwc, HDC hDC)
{
  BOOL    bRet = 0;
  DWORD   dwErr = 0;
  
// Now using bEmulateSingleBuffer in the calling function. KeithH

//  if(pwc->db_flag){
    bRet = BitBlt(hDC, 0, 0, pwc->width, pwc->height,
		  pwc->dib.hDC, 0, 0, SRCCOPY);
//  }
  
  return bRet;
  
}

//---------------------------------------------------------------------------
// Support Functions
//---------------------------------------------------------------------------

static void flush(GLcontext* ctx)
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
/*
  if((Current->rgb_flag &&!(Current->db_flag))
     ||(!Current->rgb_flag))
    {
      wmFlush(Current, Current->hDC);
    }
*/
	// Only flush if we're not in double-buffer mode. KeithH
	// The demo fractal.c calls glutSwapBuffers() then glFlush()!
	if (Current->bEmulateSingleBuffer) {
		wmFlush(Current, Current->hDC);
	}
}


//---------------------------------------------------------------------------


/*
 * Set the color index used to clear the color buffer.
 */
static void clear_index(GLcontext* ctx, GLuint index)
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  Current->clearpixel = index;
}



//---------------------------------------------------------------------------

/*
 * Set the color used to clear the color buffer.
 */
//static void clear_color( GLcontext* ctx, const GLchan color[4] )
// Changed for Mesa 5.x. KeithH
static void clear_color(
	GLcontext* ctx,
	const GLfloat color[4])
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
	GLubyte col[4];
	CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
	Current->clearpixel = RGB(col[0], col[1], col[2]);
}


//---------------------------------------------------------------------------


/*
 * Clear the specified region of the color buffer using the clear color
 * or index as specified by one of the two functions above.
 *
 * This procedure clears either the front and/or the back COLOR buffers.
 * Only the "left" buffer is cleared since we are not stereo.
 * Clearing of the other non-color buffers is left to the swrast.
 * We also only clear the color buffers if the color masks are all 1's.
 * Otherwise, we let swrast do it.
 */

static clear(GLcontext* ctx, GLbitfield mask,
	     GLboolean all, GLint x, GLint y, GLint width, GLint height)
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  DWORD   dwColor;
  WORD    wColor;
  BYTE    bColor;
  LPDWORD lpdw = (LPDWORD)Current->pbPixels;
  LPWORD  lpw = (LPWORD)Current->pbPixels;
  LPBYTE  lpb = Current->pbPixels;
  int     lines;
  const   GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;
  
  if (all){
    x=y=0;
    width=Current->width;
    height=Current->height;
  }
  
  
  /* sanity check - can't have right(stereo) buffers */
  assert((mask & (DD_FRONT_RIGHT_BIT | DD_BACK_RIGHT_BIT)) == 0);
  
  /* clear alpha */
  if ((mask & (DD_FRONT_LEFT_BIT | DD_BACK_RIGHT_BIT)) &&
      ctx->DrawBuffer->UseSoftwareAlphaBuffers &&
      ctx->Color.ColorMask[ACOMP]) {
      _swrast_clear_alpha_buffers( ctx );
  }
  
  if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
      if (mask & DD_BACK_LEFT_BIT) {
	  /* Double-buffering - clear back buffer */
	  UINT    nBypp = Current->cColorBits / 8;
	  int     i = 0;
	  int     iSize = 0;
	  
	  assert(Current->db_flag==GL_TRUE); /* we'd better be double buffer */
	  if(nBypp ==1 ){
	      iSize = Current->width/4;
	      bColor  = BGR8(GetRValue(Current->clearpixel),
			     GetGValue(Current->clearpixel),
			     GetBValue(Current->clearpixel));
	      wColor  = MAKEWORD(bColor,bColor);
	      dwColor = MAKELONG(wColor, wColor);
	  }
	  if(nBypp == 2){
	      iSize = Current->width / 2;
	      wColor = BGR16(GetRValue(Current->clearpixel),
			     GetGValue(Current->clearpixel),
			     GetBValue(Current->clearpixel));
	      dwColor = MAKELONG(wColor, wColor);
	  }
	  else if(nBypp == 4){
	      iSize = Current->width;
	      dwColor = BGR32(GetRValue(Current->clearpixel),
			      GetGValue(Current->clearpixel),
			      GetBValue(Current->clearpixel));
	  }
	  
	  /* clear a line */
	  while(i < iSize){
	      *lpdw = dwColor;
	      lpdw++;
	      i++;
	  }
	  
	  /* This is the 24bit case */
	  if (nBypp == 3) {
	      iSize = Current->width *3/4;
	      dwColor = BGR24(GetRValue(Current->clearpixel),
			      GetGValue(Current->clearpixel),
			      GetBValue(Current->clearpixel));
	      while(i < iSize){
		  *lpdw = dwColor;
		  lpb += nBypp;
		  lpdw = (LPDWORD)lpb;
		  i++;
	      }
	  }
	  
	  i = 0;
	  if (stereo_flag)
	      lines = height /2;
	  else
	      lines = height;
	  /* copy cleared line to other lines in buffer */
	  do {
	      memcpy(lpb, Current->pbPixels, iSize*4);
	      lpb += Current->ScanWidth;
	      i++;
	  }
	  while (i<lines-1);
	  mask &= ~DD_BACK_LEFT_BIT;
      } /* double-buffer */
      
      if (mask & DD_FRONT_LEFT_BIT) {
	  /* single-buffer */
	  HDC DC=DD_GETDC;
	  HPEN Pen=CreatePen(PS_SOLID,1,Current->clearpixel);
	  HBRUSH Brush=CreateSolidBrush(Current->clearpixel);
	  HPEN Old_Pen=SelectObject(DC,Pen);
	  HBRUSH Old_Brush=SelectObject(DC,Brush);
	  Rectangle(DC,x,y,x+width,y+height);
	  SelectObject(DC,Old_Pen);
	  SelectObject(DC,Old_Brush);
	  DeleteObject(Pen);
	  DeleteObject(Brush);
	  DD_RELEASEDC;
	  mask &= ~DD_FRONT_LEFT_BIT;
      } /* single-buffer */
  } /* if masks are all 1's */
    
  /* Call swrast if there is anything left to clear (like DEPTH) */
  if (mask)
      _swrast_Clear( ctx, mask, all, x, y, width, height );
}
  

//---------------------------------------------------------------------------


static void enable( GLcontext* ctx, GLenum pname, GLboolean enable )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);

	if (!Current)
		return;
	
	if (pname == GL_DITHER) {
		if(enable == GL_FALSE){
			Current->dither_flag = GL_FALSE;
			if(Current->cColorBits == 8)
				Current->pixelformat = PF_INDEX8;
		}
		else{
			if (Current->rgb_flag && Current->cColorBits == 8){
				Current->pixelformat = PF_DITHER8;
				Current->dither_flag = GL_TRUE;
			}
			else
				Current->dither_flag = GL_FALSE;
		}
	}
}

//---------------------------------------------------------------------------

static GLboolean set_draw_buffer( GLcontext* ctx, GLenum mode )
{
  /* TODO: this could be better */
  if (mode==GL_FRONT_LEFT || mode==GL_BACK_LEFT) {
    return GL_TRUE;
  }
  else {
    return GL_FALSE;
  }
}

//---------------------------------------------------------------------------


static void set_read_buffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                            GLenum buffer )
{
  /* XXX todo */
  return;
}


//---------------------------------------------------------------------------


/* Return characteristics of the output buffer. */
//static void buffer_size( GLcontext* ctx, GLuint *width, GLuint *height )
// Altered for Mesa 5.x. KeithH
static void buffer_size(
	GLframebuffer *buffer,
	GLuint *width,
	GLuint *height)
{
	// For some reason the context is not passed into this function.
	// Therefore we have to explicitly retrieve it.
	GET_CURRENT_CONTEXT(ctx);

	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
	int New_Size;
	RECT CR;
	
	GetClientRect(Current->Window,&CR);
	
	*width=CR.right;
	*height=CR.bottom;
	
	New_Size=((*width)!=Current->width) || ((*height)!=Current->height);
	
	if (New_Size){
		Current->width=*width;
		Current->height=*height;
		Current->ScanWidth=Current->width;
		if ((Current->ScanWidth%sizeof(long))!=0)
			Current->ScanWidth+=(sizeof(long)-(Current->ScanWidth%sizeof(long)));
		
		if (Current->db_flag){
			if (Current->rgb_flag==GL_TRUE && Current->dither_flag!=GL_TRUE){
				wmDeleteBackingStore(Current);
				wmCreateBackingStore(Current, Current->width, Current->height);
			}
		}
		
	}
}



/**********************************************************************/
/*****           Accelerated point, line, polygon rendering       *****/
/**********************************************************************/

/* Accelerated routines are not implemented in 4.0. See OSMesa for ideas. */

static void fast_rgb_points( GLcontext* ctx, GLuint first, GLuint last )
{
}

//---------------------------------------------------------------------------

/* Return pointer to accelerated points function */
extern tnl_points_func choose_points_function( GLcontext* ctx )
{
  return NULL;
}

//---------------------------------------------------------------------------

static void fast_flat_rgb_line( GLcontext* ctx, GLuint v0, 
				GLuint v1, GLuint pv )
{
}

//---------------------------------------------------------------------------

static tnl_line_func choose_line_function( GLcontext* ctx )
{
}


/**********************************************************************/
/*****                 Span-based pixel drawing                   *****/
/**********************************************************************/


/* Write a horizontal span of 32-bit color-index pixels with a boolean mask. */
static void write_ci32_span( const GLcontext* ctx,
                             GLuint n, GLint x, GLint y,
                             const GLuint index[],
                             const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  PBYTE Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=index[i];
}


//---------------------------------------------------------------------------

/* Write a horizontal span of 8-bit color-index pixels with a boolean mask. */
static void write_ci8_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            const GLubyte index[],
                            const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  PBYTE Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=index[i];
}


//---------------------------------------------------------------------------


/*
 * Write a horizontal span of pixels with a boolean mask.  The current
 * color index is used for all pixels.
 */
static void write_mono_ci_span(const GLcontext* ctx,
                               GLuint n,GLint x,GLint y,
                               GLuint colorIndex, const GLubyte mask[])
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=colorIndex;
}

//---------------------------------------------------------------------------

/*
 * To improve the performance of this routine, frob the data into an actual
 * scanline and call bitblt on the complete scan line instead of SetPixel.
 */

/* Write a horizontal span of RGBA color pixels with a boolean mask. */
static void write_rgba_span( const GLcontext* ctx, GLuint n, GLint x, GLint y,
                             const GLubyte rgba[][4], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  PWMC    pwc = Current;
  
  if (pwc->rgb_flag==GL_TRUE)
    {
      GLuint i;
      HDC DC=DD_GETDC;
      y=FLIP(y);
      if (mask) {
	for (i=0; i<n; i++)
	  if (mask[i])
	    wmSetPixel(pwc, y, x + i, 
		       rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
      else {
	for (i=0; i<n; i++)
	  wmSetPixel(pwc, y, x + i, 
		     rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
      DD_RELEASEDC;
    }
  else
    {
      GLuint i;
      BYTE *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
      y = FLIP(y);
      if (mask) {
	for (i=0; i<n; i++)
	  if (mask[i])
	    Mem[i] = GetNearestPaletteIndex(Current->hPal, 
					    RGB(rgba[i][RCOMP], 
						rgba[i][GCOMP], 
						rgba[i][BCOMP]));
      }
      else {
	for (i=0; i<n; i++)
	  Mem[i] = GetNearestPaletteIndex(Current->hPal,
					  RGB(rgba[i][RCOMP], 
					      rgba[i][GCOMP], 
					      rgba[i][BCOMP]));
      }
    }
}

//---------------------------------------------------------------------------

/* Write a horizontal span of RGB color pixels with a boolean mask. */
static void write_rgb_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            const GLubyte rgb[][3], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  PWMC    pwc = Current;
  
  if (pwc->rgb_flag==GL_TRUE)
    {
      GLuint i;
      HDC DC=DD_GETDC;
      y=FLIP(y);
      if (mask) {
	for (i=0; i<n; i++)
	  if (mask[i])
	    wmSetPixel(pwc, y, x + i, 
		       rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
      }
      else {
	for (i=0; i<n; i++)
	  wmSetPixel(pwc, y, x + i, 
		     rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
      DD_RELEASEDC;
    }
  else
    {
      GLuint i;
      BYTE *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
      y = FLIP(y);
      if (mask) {
	for (i=0; i<n; i++)
	  if (mask[i])
	    Mem[i] = GetNearestPaletteIndex(Current->hPal,
					    RGB(rgb[i][RCOMP], 
						rgb[i][GCOMP], 
						rgb[i][BCOMP]));
      }
      else {
	for (i=0; i<n; i++)
	  Mem[i] = GetNearestPaletteIndex(Current->hPal,
					  RGB(rgb[i][RCOMP], 
					      rgb[i][GCOMP], 
					      rgb[i][BCOMP]));
      }
    }
}

//---------------------------------------------------------------------------

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_span( const GLcontext* ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLchan color[4], const GLubyte mask[])
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  ULONG pixel =  RGB( color[RCOMP], color[GCOMP], color[BCOMP] );
  GLuint i;
  HDC DC=DD_GETDC;
  PWMC pwc = Current;
  assert(Current->rgb_flag==GL_TRUE);
  y=FLIP(y);
  if(Current->rgb_flag==GL_TRUE){
    for (i=0; i<n; i++)
      if (mask[i])
	wmSetPixel(pwc,y,x+i,color[RCOMP], color[GCOMP], color[BCOMP]);
  }
  else {
    for (i=0; i<n; i++)
      if (mask[i])
	SetPixel(DC, y, x+i, pixel);
  }
  DD_RELEASEDC;
}



/**********************************************************************/
/*****                   Array-based pixel drawing                *****/
/**********************************************************************/


/* Write an array of 32-bit index pixels with a boolean mask. */
static void write_ci32_pixels( const GLcontext* ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLuint index[], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
      *Mem = index[i];
    }
  }
}


//---------------------------------------------------------------------------


/*
 * Write an array of pixels with a boolean mask.  The current color
 * index is used for all pixels.
 */
static void write_mono_ci_pixels( const GLcontext* ctx,
                                  GLuint n,
                                  const GLint x[], const GLint y[],
                                  GLuint colorIndex, const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
      *Mem = colorIndex;
    }
  }
}


//---------------------------------------------------------------------------


/* Write an array of RGBA pixels with a boolean mask. */
static void write_rgba_pixels( const GLcontext* ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLubyte rgba[][4], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  PWMC    pwc = Current;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  for (i=0; i<n; i++)
    if (mask[i])
      wmSetPixel(pwc, FLIP(y[i]), x[i],
		 rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
  DD_RELEASEDC;
}


//---------------------------------------------------------------------------


/*
 * Write an array of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_pixels( const GLcontext* ctx,
                                    GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLchan color[4],
                                    const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  PWMC    pwc = Current;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  for (i=0; i<n; i++)
    if (mask[i])
      wmSetPixel(pwc, FLIP(y[i]),x[i],color[RCOMP],
		 color[GCOMP], color[BCOMP]);
  DD_RELEASEDC;
}

/**********************************************************************/
/*****            Read spans/arrays of pixels                     *****/
/**********************************************************************/

/* Read a horizontal span of color-index pixels. */
static void read_ci32_span( const GLcontext* ctx, GLuint n, GLint x, GLint y,
                            GLuint index[])
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    index[i]=Mem[i];
}

//---------------------------------------------------------------------------

/* Read an array of color index pixels. */
static void read_ci32_pixels( const GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint indx[], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      indx[i]=*(Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i]);
    }
  }
}

//---------------------------------------------------------------------------

/* Read a horizontal span of color pixels. */
static void read_rgba_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            GLubyte rgba[][4] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  UINT i;
  COLORREF Color;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  y = Current->height - y - 1;
  for (i=0; i<n; i++) {
    Color=GetPixel(DC,x+i,y);
    rgba[i][RCOMP] = GetRValue(Color);
    rgba[i][GCOMP] = GetGValue(Color);
    rgba[i][BCOMP] = GetBValue(Color);
    rgba[i][ACOMP] = 255;
  }
  DD_RELEASEDC;
}

//---------------------------------------------------------------------------

/* Read an array of color pixels. */
static void read_rgba_pixels( const GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLubyte rgba[][4], const GLubyte mask[] )
{
	GLD_context *gldCtx = GLD_GET_CONTEXT(ctx);
	WMesaContext *Current = GLD_GET_WMESA_DRIVER(gldCtx);
  GLuint i;
  COLORREF Color;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      GLint y2 = Current->height - y[i] - 1;
      Color=GetPixel(DC,x[i],y2);
      rgba[i][RCOMP] = GetRValue(Color);
      rgba[i][GCOMP] = GetGValue(Color);
      rgba[i][BCOMP] = GetBValue(Color);
      rgba[i][ACOMP] = 255;
    }
  }
  DD_RELEASEDC;
}

//---------------------------------------------------------------------------

static void wmesa_update_state(
	GLcontext *ctx,
	GLuint new_state)
{
    _swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_vbo_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );
}

//---------------------------------------------------------------------------

static void wmesa_viewport(
	GLcontext *ctx,
	GLint x,
	GLint y,
	GLsizei w,
	GLsizei h)
{
//	ctx->Driver.ResizeBuffersMESA(ctx);
}

//---------------------------------------------------------------------------

static void wmesa_update_state_first_time(
	GLcontext *ctx,
	GLuint new_state)
{
	struct swrast_device_driver	*swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext					*tnl = TNL_CONTEXT(ctx);
	
	/*
	* XXX these function pointers could be initialized just once during
	* context creation since they don't depend on any state changes.
	* kws - This is true - this function gets called a lot and it
	* would be good to minimize setting all this when not needed.
	*/
	// Good idea, so I'll do it. KeithH. :-)

	ctx->Driver.GetString				= _gldGetStringGeneric;
	ctx->Driver.UpdateState				= wmesa_update_state;
	ctx->Driver.DrawBuffer				= set_draw_buffer;
	ctx->Driver.ResizeBuffers			= _swrast_alloc_buffers;
	ctx->Driver.GetBufferSize			= buffer_size;

	ctx->Driver.Viewport				= wmesa_viewport;
	
	ctx->Driver.Accum					= _swrast_Accum;
	ctx->Driver.Bitmap					= _swrast_Bitmap;
	ctx->Driver.Clear					= clear;
	
	ctx->Driver.Flush					= flush;
	ctx->Driver.ClearIndex				= clear_index;
	ctx->Driver.ClearColor				= clear_color;
	ctx->Driver.Enable					= enable;
	
	ctx->Driver.CopyPixels				= _swrast_CopyPixels;
	ctx->Driver.DrawPixels				= _swrast_DrawPixels;
	ctx->Driver.ReadPixels				= _swrast_ReadPixels;
	
	ctx->Driver.ChooseTextureFormat		= _mesa_choose_tex_format;
	ctx->Driver.TexImage1D				= _mesa_store_teximage1d;
	ctx->Driver.TexImage2D				= _mesa_store_teximage2d;
	ctx->Driver.TexImage3D				= _mesa_store_teximage3d;
	ctx->Driver.TexSubImage1D			= _mesa_store_texsubimage1d;
	ctx->Driver.TexSubImage2D			= _mesa_store_texsubimage2d;
	ctx->Driver.TexSubImage3D			= _mesa_store_texsubimage3d;
	ctx->Driver.TestProxyTexImage		= _mesa_test_proxy_teximage;
	
	ctx->Driver.CopyTexImage1D			= _swrast_copy_teximage1d;
	ctx->Driver.CopyTexImage2D			= _swrast_copy_teximage2d;
	ctx->Driver.CopyTexSubImage1D		= _swrast_copy_texsubimage1d;
	ctx->Driver.CopyTexSubImage2D		= _swrast_copy_texsubimage2d;
	ctx->Driver.CopyTexSubImage3D		= _swrast_copy_texsubimage3d;
	ctx->Driver.CopyColorTable			= _swrast_CopyColorTable;
	ctx->Driver.CopyColorSubTable		= _swrast_CopyColorSubTable;
	ctx->Driver.CopyConvolutionFilter1D	= _swrast_CopyConvolutionFilter1D;
	ctx->Driver.CopyConvolutionFilter2D	= _swrast_CopyConvolutionFilter2D;
	
	// Does not apply for Mesa 5.x
	//ctx->Driver.BaseCompressedTexFormat	= _mesa_base_compressed_texformat;
	//ctx->Driver.CompressedTextureSize	= _mesa_compressed_texture_size;
	//ctx->Driver.GetCompressedTexImage	= _mesa_get_compressed_teximage;
	
	swdd->SetBuffer					= set_read_buffer;
	
	
	/* Pixel/span writing functions: */
	swdd->WriteRGBASpan        = write_rgba_span;
	swdd->WriteRGBSpan         = write_rgb_span;
	swdd->WriteMonoRGBASpan    = write_mono_rgba_span;
	swdd->WriteRGBAPixels      = write_rgba_pixels;
	swdd->WriteMonoRGBAPixels  = write_mono_rgba_pixels;
	swdd->WriteCI32Span        = write_ci32_span;
	swdd->WriteCI8Span         = write_ci8_span;
	swdd->WriteMonoCISpan      = write_mono_ci_span;
	swdd->WriteCI32Pixels      = write_ci32_pixels;
	swdd->WriteMonoCIPixels    = write_mono_ci_pixels;
	
	swdd->ReadCI32Span        = read_ci32_span;
	swdd->ReadRGBASpan        = read_rgba_span;
	swdd->ReadCI32Pixels      = read_ci32_pixels;
	swdd->ReadRGBAPixels      = read_rgba_pixels;
	
	
	tnl->Driver.RunPipeline = _tnl_run_pipeline;
	
	wmesa_update_state(ctx, new_state);
}

//---------------------------------------------------------------------------
// Driver interface functions
//---------------------------------------------------------------------------

BOOL gldCreateDrawable_MesaSW(
	DGL_ctx *pCtx,
	BOOL bPersistantInterface,
	BOOL bPersistantBuffers)
{
	WMesaContext *c;
	GLboolean true_color_flag;
	GLboolean rgb_flag = GL_TRUE;
	GLboolean db_flag = GL_TRUE;

	if (pCtx == NULL)
		return FALSE;

	c = (struct wmesa_context * ) calloc(1,sizeof(struct wmesa_context));
	if (!c)
		return FALSE;

	pCtx->glPriv = c;

	c->hDC		= pCtx->hDC;
	c->Window	= pCtx->hWnd;

	true_color_flag = GetDeviceCaps(pCtx->hDC, BITSPIXEL) > 8;
	
	
#ifdef DITHER
	if ((true_color_flag==GL_FALSE) && (rgb_flag == GL_TRUE)){
		c->dither_flag = GL_TRUE;
		c->hPalHalfTone = WinGCreateHalftonePalette();
	}
	else
		c->dither_flag = GL_FALSE;
#else
	c->dither_flag = GL_FALSE;
#endif
	
	
	if (rgb_flag==GL_FALSE)
    {
		c->rgb_flag = GL_FALSE;
#if 0
		/* Old WinG stuff???? */
		c->db_flag = db_flag =GL_TRUE; /* WinG requires double buffering */
		printf("Single buffer is not supported in color index mode, ",
			"setting to double buffer.\n");
#endif
    }
	else
    {
		c->rgb_flag = GL_TRUE;
    }

//	db_flag = pCtx->lpPF->pfd.dwFlags & PFD_DOUBLEBUFFER ? GL_TRUE : GL_FALSE;
	db_flag = GL_TRUE; // Force double-buffer
	if (db_flag) {
		c->db_flag = 1;
		/* Double buffered */
		{
			wmCreateBackingStore(c, pCtx->dwWidth, pCtx->dwHeight);
			
		}
    } else {
		/* Single Buffered */
		if (c->rgb_flag)
			c->db_flag = 0;
    }	

	c->bEmulateSingleBuffer = (pCtx->lpPF->pfd.dwFlags & PFD_DOUBLEBUFFER)
		? FALSE : TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldResizeDrawable_MesaSW(
	DGL_ctx *ctx,
	BOOL bDefaultDriver,
	BOOL bPersistantInterface,
	BOOL bPersistantBuffers)
{
	WMesaContext *c;

	if (ctx == NULL)
		return FALSE;

	c = ctx->glPriv;
	if (c == NULL)
		return FALSE;

	c->hDC = ctx->hDC;
	c->Window = ctx->hWnd;
//	c->width = ctx->dwWidth;
//	c->height = ctx->dwHeight;

	if (c->db_flag) {
		wmDeleteBackingStore(c);
		wmCreateBackingStore(c, ctx->dwWidth, ctx->dwHeight);
	}

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldDestroyDrawable_MesaSW(
	DGL_ctx *ctx)
{
	WMesaContext *c;

	if (ctx == NULL)
		return FALSE;

	c = ctx->glPriv;
	if (c == NULL)
		return FALSE;

	if (c->hPalHalfTone != NULL)
		DeleteObject(c->hPalHalfTone);
    
	if (c->db_flag)
		wmDeleteBackingStore(c);

	free(c);

	ctx->glPriv = NULL;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldCreatePrivateGlobals_MesaSW(void)
{
	// Mesa Software driver needs no private globals
	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldDestroyPrivateGlobals_MesaSW(void)
{
	// Mesa Software driver needs no private globals
	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldBuildPixelformatList_MesaSW(void)
{
	// Release any existing pixelformat list
	if (glb.lpPF) {
		free(glb.lpPF);
	}

	glb.nPixelFormatCount	= 0;
	glb.lpPF				= NULL;

	glb.lpPF = (DGL_pixelFormat *)calloc(2, sizeof(DGL_pixelFormat));
	if (glb.lpPF == NULL)
		return FALSE;
	// Single-buffered
	memcpy(&glb.lpPF[0], &pfTemplateMesaSW, sizeof(DGL_pixelFormat));
	glb.lpPF[0].pfd.dwFlags &= ~PFD_DOUBLEBUFFER; // Remove doublebuffer flag
	// Double-buffered
	memcpy(&glb.lpPF[1], &pfTemplateMesaSW, sizeof(DGL_pixelFormat));
	glb.nPixelFormatCount = 2;

	// Mark list as 'current'
	glb.bPixelformatsDirty = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldInitialiseMesa_MesaSW(
	DGL_ctx *gld)
{
	GLcontext *ctx;

	if (gld == NULL)
		return FALSE;

	ctx = gld->glCtx;

	// Set max texture size to 256
	ctx->Const.MaxTextureLevels = 8;

	// Multitexture enable/disable
	ctx->Const.MaxTextureUnits = (glb.bMultitexture) ? MAX_TEXTURE_UNITS : 1;

	/* Initialize the software rasterizer and helper modules.*/

	// Added this to force max texture diminsion to 256. KeithH
	ctx->Const.MaxTextureLevels = 8;

	_mesa_enable_sw_extensions(ctx);
	_mesa_enable_imaging_extensions(ctx);
	_mesa_enable_1_3_extensions(ctx);
	
//	_swrast_CreateContext( ctx );
//	_vbo_CreateContext( ctx );
//	_tnl_CreateContext( ctx );
//	_swsetup_CreateContext( ctx );
	
	_swsetup_Wakeup( ctx );
	
	wmesa_update_state_first_time(ctx, ~0);

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldSwapBuffers_MesaSW(
	DGL_ctx *ctx,
	HDC hDC,
	HWND hWnd)
{
	WMesaContext *c;

	if (ctx == NULL)
		return FALSE;

	c = ctx->glPriv;
	if (c == NULL)
		return FALSE;
	
	/* If we're swapping the buffer associated with the current context
	* we have to flush any pending rendering commands first.
	*/

	// Altered to respect bEmulateSingleBuffer. KeithH
//	if (c->db_flag)
	if (!c->bEmulateSingleBuffer)
		wmFlush(c, hDC);

	return TRUE;
}

//---------------------------------------------------------------------------

PROC gldGetProcAddress_MesaSW(
	LPCSTR a)
{
	int		i;
	PROC	proc = NULL;

	for (i=0; GLD_extList[i].proc; i++) {
		if (!strcmp(a, GLD_extList[i].name)) {
			proc = GLD_extList[i].proc;
			break;
		}
	}

	gldLogPrintf(GLDLOG_INFO, "GetProcAddress: %s (%s)", a, proc ? "OK" : "Failed");

	return proc;
}

//---------------------------------------------------------------------------

BOOL gldGetDisplayMode_MesaSW(
	DGL_ctx *ctx,
	GLD_displayMode *glddm)
{
	HDC hdcDesktop;

	if (glddm == NULL)
		return FALSE;

	//
	// A bit hacky... KeithH
	//

	hdcDesktop = GetDC(NULL);
	glddm->Width	= GetDeviceCaps(hdcDesktop, HORZRES);
	glddm->Height	= GetDeviceCaps(hdcDesktop, VERTRES);
	glddm->BPP		= GetDeviceCaps(hdcDesktop, BITSPIXEL);
	glddm->Refresh	= 0;
	ReleaseDC(0, hdcDesktop);

	return TRUE;
}

//---------------------------------------------------------------------------

