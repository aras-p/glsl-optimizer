/* $Id: wmesa.c,v 1.21 2001/10/04 19:04:56 kschultz Exp $ */

/*
 * Windows (Win32) device driver for Mesa 3.4
 *
 * Original author:
 *
 *  Copyright (C) 1996-  Li Wei
 *  Address      :       Institute of Artificial Intelligence
 *               :           & Robotics
 *               :       Xi'an Jiaotong University
 *  Email        :       liwei@aiar.xjtu.edu.cn
 *  Web page :       http://sun.aiar.xjtu.edu.cn
 *
 *  This file and its associations are partially borrowed from the
 *  Windows NT driver for Mesa 1.8 , written by Mark Leaming
 *  (mark@rsinc.com).
 *
 * Updated for Mesa 4.0 by Karl Schultz (kschultz@sourceforge.net)
 */



#include "wmesadef.h"
#include <GL/wmesa.h>
#include "mesa_extend.h"

#include "glheader.h"
#include "colors.h"
#include "context.h"
#include "colormac.h"
#include "dd.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"
#include "array_cache/acache.h"
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

/* Dither not tested for Mesa 4.0 */ 
#ifdef DITHER
#include <wing.h>
#endif

#ifdef __CYGWIN32__
#include "macros.h"
#include <string.h>
#define CopyMemory memcpy
#endif

/* Stereo and parallel not tested for Mesa 4.0. */
#if !defined(NO_STEREO)
#include "gl\glu.h"
#include "stereo.h"
#endif

#if !defined(NO_PARALLEL)
#include "parallel.h"
#endif

/* File global varaibles */
struct DISPLAY_OPTIONS displayOptions;
GLenum stereoCompile = GL_FALSE ;
GLenum stereoShowing  = GL_FALSE ;
GLenum stereoBuffer = GL_FALSE;
#if !defined(NO_STEREO)
GLint displayList = MAXIMUM_DISPLAY_LIST ;
#endif
GLint stereo_flag = 0 ;

static PWMC Current = NULL;
WMesaContext WC = NULL;

/* If we are double-buffering, we want to get the DC for the
 * off-screen DIB, otherwise the DC for the window.
 */
#define DD_GETDC ((Current->db_flag) ? Current->dib.hDC : Current->hDC )
#define DD_RELEASEDC

#define FLIP(Y)  (Current->height-(Y)-1)

#define DITHER_RGB_TO_8BIT_SETUP            \
GLubyte pixelDithered;

#define DITHER_RGB_TO_8BIT(red, green, blue, pixel, scanline)        \
{                                                                    \
    char unsigned redtemp, greentemp, bluetemp, paletteindex;        \
    redtemp = aDividedBy51[red]                                      \
    + (aModulo51[red] > aHalftone8x8[(pixel%8)*8                     \
    + scanline%8]);                                                  \
    greentemp = aDividedBy51[(char unsigned)green]                   \
    + (aModulo51[green] > aHalftone8x8[                              \
    (pixel%8)*8 + scanline%8]);                                      \
    bluetemp = aDividedBy51[(char unsigned)blue]                     \
    + (aModulo51[blue] > aHalftone8x8[                               \
    (pixel%8)*8 +scanline%8]);                                       \
    paletteindex = redtemp + aTimes6[greentemp] + aTimes36[bluetemp];\
    pixelDithered = aWinGHalftoneTranslation[paletteindex];          \
}


#ifdef DDRAW
static BOOL DDInit( WMesaContext wc, HWND hwnd);
static void DDFree( WMesaContext wc);
static HRESULT DDRestoreAll( WMesaContext wc );
static void DDDeleteOffScreen(WMesaContext wc);
static BOOL DDCreateOffScreen(WMesaContext wc);
#endif

static void FlushToFile(PWMC pwc, PSTR  szFile);
BOOL wmCreateBackingStore(PWMC pwc, long lxSize, long lySize);
BOOL wmDeleteBackingStore(PWMC pwc);
void wmCreatePalette( PWMC pwdc );
BOOL wmSetDibColors(PWMC pwc);
void wmSetPixel(PWMC pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b);
BOOL wmFlush(PWMC pwc);
void wmCreateDIBSection(
    HDC  hDC,
    PWMC pwc,   // handle of device context
    CONST BITMAPINFO *pbmi, // bitmap size, format, and color data
    UINT iUsage // color data type indicator: RGB values or palette indices
    );
void WMesaViewport( GLcontext *ctx,
                    GLint x, GLint y, GLsizei width, GLsizei height );


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
  
  wmCreateDIBSection(hdc, pwc, pbmi, iUsage);
  
  if ((iUsage == DIB_PAL_COLORS) && !(pwc->hGLPalette)) {
    wmCreatePalette( pwc );
    wmSetDibColors( pwc );
  }
  wmSetPixelFormat(pwc, pwc->hDC);
  return TRUE;
}


/*
 * This function copies one scan line in a DIB section to another
 */
BOOL wmSetDIBits(PWMC pwc, UINT uiScanWidth, UINT uiNumScans, 
			UINT nBypp, UINT uiNewWidth, LPBYTE pBits)
{
  UINT    uiScans = 0;
  LPBYTE  pDest = pwc->pbPixels;
  DWORD   dwNextScan = uiScanWidth;
  DWORD   dwNewScan = uiNewWidth;
  DWORD   dwScanWidth = (uiScanWidth * nBypp);
  
  /*
   * We need to round up to the nearest DWORD
   * and multiply by the number of bytes per
   * pixel
   */
  dwNextScan = (((dwNextScan * nBypp)+ 3) & ~3);
  dwNewScan = (((dwNewScan * nBypp)+ 3) & ~3);
  
  for(uiScans = 0; uiScans < uiNumScans; uiScans++){
    CopyMemory(pDest, pBits, dwScanWidth);
    pBits += dwNextScan;
    pDest += dwNewScan;
  }
  return TRUE;
}



#define PIXELADDR(X,Y)  \
  ((GLubyte *)Current->pbPixels + (Current->height-Y-1)* \
  Current->ScanWidth + (X)*nBypp)
#define PIXELADDR1( X, Y )  \
((GLubyte *)wmesa->pbPixels + (wmesa->height-Y-1)* wmesa->ScanWidth + (X))
#define PIXELADDR2( X, Y )  \
((GLubyte *)wmesa->pbPixels + (wmesa->height-Y-1)* wmesa->ScanWidth + (X)*2)
#define PIXELADDR4( X, Y )  \
((GLubyte *)wmesa->pbPixels + (wmesa->height-Y-1)* wmesa->ScanWidth + (X)*4)


BYTE DITHER_RGB_2_8BIT( int r, int g, int b, int x, int y);

/* Finish all pending operations and synchronize. */
static void finish(GLcontext* ctx)
{
  /* No op */
}


static void flush(GLcontext* ctx)
{
  if((Current->rgb_flag &&!(Current->db_flag))
     ||(!Current->rgb_flag))
    {
      wmFlush(Current);
    }
  
}



/*
 * Set the color index used to clear the color buffer.
 */
static void clear_index(GLcontext* ctx, GLuint index)
{
  Current->clearpixel = index;
}



/*
 * Set the color used to clear the color buffer.
 */
static void clear_color( GLcontext* ctx, const GLchan color[4] )
{
  Current->clearpixel = RGB(color[0], color[1], color[2]);
}



/*
 * Clear the specified region of the color buffer using the clear color
 * or index as specified by one of the two functions above.
 */

static clear(GLcontext* ctx, GLbitfield mask,
	     GLboolean all, GLint x, GLint y, GLint width, GLint height)
{
  DWORD   dwColor;
  WORD    wColor;
  BYTE    bColor;
  LPDWORD lpdw = (LPDWORD)Current->pbPixels;
  LPWORD  lpw = (LPWORD)Current->pbPixels;
  LPBYTE  lpb = Current->pbPixels;
  int     lines;
  
  if (all){
    x=y=0;
    width=Current->width;
    height=Current->height;
  }
  if(Current->db_flag==GL_TRUE){
    UINT    nBypp = Current->cColorBits / 8;
    int     i = 0;
    int     iSize = 0;
    
    if(nBypp ==1 ){
      /* Need rectification */
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
    do {
      memcpy(lpb, Current->pbPixels, iSize*4);
      lpb += Current->ScanWidth;
      i++;
    }
    while (i<lines-1);
  }
  else { /* For single buffer */
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
  }
 
  /* TODO need to check masks - see osmesa */
  mask &= ~DD_FRONT_LEFT_BIT;
  if (mask)
    _swrast_Clear( ctx, mask, all, x, y, width, height );
  
}



static void enable( GLcontext* ctx, GLenum pname, GLboolean enable )
{
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


static void set_read_buffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                            GLenum buffer )
{
  /* XXX todo */
  return;
}



/* Return characteristics of the output buffer. */
static void buffer_size( GLcontext* ctx, GLuint *width, GLuint *height )
{
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
#ifdef DDRAW
       DDDeleteOffScreen(Current);
       DDCreateOffScreen(Current);
#else
       if (Current->rgb_flag==GL_TRUE && Current->dither_flag!=GL_TRUE){
	 wmDeleteBackingStore(Current);
	 wmCreateBackingStore(Current, Current->width, Current->height);
       }
#endif
     }
     
     /*  Resize OsmesaBuffer if in Parallel mode */
#if !defined(NO_PARALLEL)
     if(parallelFlag)
       PRSizeRenderBuffer(Current->width, Current->height,Current->ScanWidth,
			  Current->rgb_flag == GL_TRUE ? Current->pbPixels: 
			  Current->ScreenMem);
#endif
   }
}



/**********************************************************************/
/*****           Accelerated point, line, polygon rendering       *****/
/**********************************************************************/

/* Accelerated routines are not implemented in 4.0. See OSMesa for ideas. */

static void fast_rgb_points( GLcontext* ctx, GLuint first, GLuint last )
{
}

/* Return pointer to accelerated points function */
extern points_func choose_points_function( GLcontext* ctx )
{
  return NULL;
}

static void fast_flat_rgb_line( GLcontext* ctx, GLuint v0, 
				GLuint v1, GLuint pv )
{
}

static line_func choose_line_function( GLcontext* ctx )
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
  GLuint i;
  PBYTE Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=index[i];
}


/* Write a horizontal span of 8-bit color-index pixels with a boolean mask. */
static void write_ci8_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            const GLubyte index[],
                            const GLubyte mask[] )
{
  GLuint i;
  PBYTE Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=index[i];
}



/*
 * Write a horizontal span of pixels with a boolean mask.  The current
 * color index is used for all pixels.
 */
static void write_mono_ci_span(const GLcontext* ctx,
                               GLuint n,GLint x,GLint y,
                               GLuint colorIndex, const GLubyte mask[])
{
  GLuint i;
  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    if (mask[i])
      Mem[i]=colorIndex;
}

/*
 * To improve the performance of this routine, frob the data into an actual
 * scanline and call bitblt on the complete scan line instead of SetPixel.
 */

/* Write a horizontal span of RGBA color pixels with a boolean mask. */
static void write_rgba_span( const GLcontext* ctx, GLuint n, GLint x, GLint y,
                             const GLubyte rgba[][4], const GLubyte mask[] )
{
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

/* Write a horizontal span of RGB color pixels with a boolean mask. */
static void write_rgb_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            const GLubyte rgb[][3], const GLubyte mask[] )
{
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

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_span( const GLcontext* ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLchan color[4], const GLubyte mask[])
{
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
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
      *Mem = index[i];
    }
  }
}



/*
 * Write an array of pixels with a boolean mask.  The current color
 * index is used for all pixels.
 */
static void write_mono_ci_pixels( const GLcontext* ctx,
                                  GLuint n,
                                  const GLint x[], const GLint y[],
                                  GLuint colorIndex, const GLubyte mask[] )
{
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
      *Mem = colorIndex;
    }
  }
}



/* Write an array of RGBA pixels with a boolean mask. */
static void write_rgba_pixels( const GLcontext* ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLubyte rgba[][4], const GLubyte mask[] )
{
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
  GLuint i;
  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    index[i]=Mem[i];
}




/* Read an array of color index pixels. */
static void read_ci32_pixels( const GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint indx[], const GLubyte mask[] )
{
  GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
    if (mask[i]) {
      indx[i]=*(Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i]);
    }
  }
}



/* Read a horizontal span of color pixels. */
static void read_rgba_span( const GLcontext* ctx,
                            GLuint n, GLint x, GLint y,
                            GLubyte rgba[][4] )
{
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


/* Read an array of color pixels. */
static void read_rgba_pixels( const GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLubyte rgba[][4], const GLubyte mask[] )
{
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



/**********************************************************************/
/**********************************************************************/


static const GLubyte *get_string(GLcontext *ctx, GLenum name)
{
  if (name == GL_RENDERER) {
    return (GLubyte *) "Mesa Windows";
  }
  else {
    return NULL;
  }
}

static void wmesa_update_state( GLcontext *ctx, GLuint new_state )
{
  struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
  TNLcontext *tnl = TNL_CONTEXT(ctx);
  
  /*
   * XXX these function pointers could be initialized just once during
   * context creation since they don't depend on any state changes.
   * kws - This is true - this function gets called a lot and it
   * would be good to minimize setting all this when not needed.
   */
  
  ctx->Driver.GetString = get_string;
  ctx->Driver.UpdateState = wmesa_update_state;
  ctx->Driver.SetDrawBuffer = set_draw_buffer;
  ctx->Driver.ResizeBuffersMESA = _swrast_alloc_buffers;
  ctx->Driver.GetBufferSize = buffer_size;
  
  ctx->Driver.Accum = _swrast_Accum;
  ctx->Driver.Bitmap = _swrast_Bitmap;
  ctx->Driver.Clear = clear;
  
  ctx->Driver.Flush = flush;
  ctx->Driver.ClearIndex = clear_index;
  ctx->Driver.ClearColor = clear_color;
  ctx->Driver.Enable = enable;
  
  ctx->Driver.CopyPixels = _swrast_CopyPixels;
  ctx->Driver.DrawPixels = _swrast_DrawPixels;
  ctx->Driver.ReadPixels = _swrast_ReadPixels;
  
  ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
  ctx->Driver.TexImage1D = _mesa_store_teximage1d;
  ctx->Driver.TexImage2D = _mesa_store_teximage2d;
  ctx->Driver.TexImage3D = _mesa_store_teximage3d;
  ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
  ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
  ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
  ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;
  
  ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
  ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
  ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
  ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
  ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
  ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
  ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
  ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
  ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
  
  ctx->Driver.BaseCompressedTexFormat = _mesa_base_compressed_texformat;
  ctx->Driver.CompressedTextureSize = _mesa_compressed_texture_size;
  ctx->Driver.GetCompressedTexImage = _mesa_get_compressed_teximage;
  
  
  swdd->SetReadBuffer = set_read_buffer;
  
  
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
  
  _swrast_InvalidateState( ctx, new_state );
  _swsetup_InvalidateState( ctx, new_state );
  _ac_InvalidateState( ctx, new_state );
  _tnl_InvalidateState( ctx, new_state );
}




/**********************************************************************/
/*****                  WMesa API Functions                       *****/
/**********************************************************************/



#define PAL_SIZE 256
static void GetPalette(HPALETTE Pal,RGBQUAD *aRGB)
{
  int i;
  HDC hdc;
  struct
  {
    WORD Version;
    WORD NumberOfEntries;
    PALETTEENTRY aEntries[PAL_SIZE];
  } Palette =
    {
      0x300,
      PAL_SIZE
    };
  hdc=GetDC(NULL);
  if (Pal!=NULL)
    GetPaletteEntries(Pal,0,PAL_SIZE,Palette.aEntries);
  else
    GetSystemPaletteEntries(hdc,0,PAL_SIZE,Palette.aEntries);
  if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)
    {
      for(i = 0; i <PAL_SIZE; i++)
	Palette.aEntries[i].peFlags = PC_RESERVED;
      Palette.aEntries[255].peRed = 255;
      Palette.aEntries[255].peGreen = 255;
      Palette.aEntries[255].peBlue = 255;
      Palette.aEntries[255].peFlags = 0;
      Palette.aEntries[0].peRed = 0;
      Palette.aEntries[0].peGreen = 0;
      Palette.aEntries[0].peBlue = 0;
      Palette.aEntries[0].peFlags = 0;
    }
  else
    {
      int nStaticColors;
      int nUsableColors;
      nStaticColors = GetDeviceCaps(hdc, NUMCOLORS)/2;
      for (i=0; i<nStaticColors; i++)
	Palette.aEntries[i].peFlags = 0;
      nUsableColors = PAL_SIZE-nStaticColors;
      for (; i<nUsableColors; i++)
	Palette.aEntries[i].peFlags = PC_RESERVED;
      for (; i<PAL_SIZE-nStaticColors; i++)
	Palette.aEntries[i].peFlags = PC_RESERVED;
      for (i=PAL_SIZE-nStaticColors; i<PAL_SIZE; i++)
	Palette.aEntries[i].peFlags = 0;
    }
  ReleaseDC(NULL,hdc);
  for (i=0; i<PAL_SIZE; i++)
    {
      aRGB[i].rgbRed=Palette.aEntries[i].peRed;
      aRGB[i].rgbGreen=Palette.aEntries[i].peGreen;
      aRGB[i].rgbBlue=Palette.aEntries[i].peBlue;
      aRGB[i].rgbReserved=Palette.aEntries[i].peFlags;
    }
}


WMesaContext WMesaCreateContext( HWND hWnd, HPALETTE* Pal,
				 GLboolean rgb_flag,
				 GLboolean db_flag )
{
  RECT CR;
  WMesaContext c;
  GLboolean true_color_flag;
  c = (struct wmesa_context * ) calloc(1,sizeof(struct wmesa_context));
  if (!c)
    return NULL;
  
  c->Window=hWnd;
  c->hDC = GetDC(hWnd);
  true_color_flag = GetDeviceCaps(c->hDC, BITSPIXEL) > 8;
#ifdef DDRAW
  if(true_color_flag) c->rgb_flag = rgb_flag = GL_TRUE;
#endif
  
  
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
  GetClientRect(c->Window,&CR);
  c->width=CR.right;
  c->height=CR.bottom;
  if (db_flag)
    {
      c->db_flag = 1;
      /* Double buffered */
#ifndef DDRAW
      {
	wmCreateBackingStore(c, c->width, c->height);
	
      }
#endif
    }
  else
    {
      /* Single Buffered */
      if (c->rgb_flag)
	c->db_flag = 0;
    }
#ifdef DDRAW
  if (DDInit(c,hWnd) == GL_FALSE) {
    free( (void *) c );
    exit(1);
  }
#endif
  
  
  c->gl_visual = _mesa_create_visual(rgb_flag,
				     db_flag,    /* db_flag */
				     GL_FALSE,   /* stereo */
				     8,8,8,8,    /* r, g, b, a bits */
				     0,          /* index bits */
				     16,         /* depth_bits */
				     8,          /* stencil_bits */
				     16,16,16,16,/* accum_bits */
				     1);
  
  if (!c->gl_visual) {
    return NULL;
  }
  
  /* allocate a new Mesa context */
  c->gl_ctx = _mesa_create_context( c->gl_visual, NULL, c, GL_TRUE);
  
  if (!c->gl_ctx) {
    _mesa_destroy_visual( c->gl_visual );
    free(c);
    return NULL;
  }
  
  if (!_mesa_initialize_context(c->gl_ctx,
				c->gl_visual,
				(GLcontext *) NULL,
				(void *) c, GL_TRUE )) {
    _mesa_destroy_visual( c->gl_visual );
    free(c);
    return NULL;
  }
  
  
  _mesa_enable_sw_extensions(c->gl_ctx);
  _mesa_enable_1_3_extensions(c->gl_ctx);

  c->gl_buffer = _mesa_create_framebuffer( c->gl_visual,
					   c->gl_visual->depthBits > 0,
					   c->gl_visual->stencilBits > 0,
					   c->gl_visual->accumRedBits > 0,
					   GL_FALSE /* s/w alpha */ );
  if (!c->gl_buffer) {
    _mesa_destroy_visual( c->gl_visual );
    _mesa_free_context_data( c->gl_ctx );
    free(c);
    return NULL;
  }
  
  /* Initialize the software rasterizer and helper modules.
   */
  {
    GLcontext *ctx = c->gl_ctx;
    
    _swrast_CreateContext( ctx );
    _ac_CreateContext( ctx );
    _tnl_CreateContext( ctx );
    _swsetup_CreateContext( ctx );
    
    _swsetup_Wakeup( ctx );
  }
  return c;
}

void WMesaDestroyContext( void )
{
  WMesaContext c = Current;
  ReleaseDC(c->Window,c->hDC);
  WC = c;
  if(c->hPalHalfTone != NULL)
    DeleteObject(c->hPalHalfTone);

  _swsetup_DestroyContext( c->gl_ctx );
  _tnl_DestroyContext( c->gl_ctx );
  _ac_DestroyContext( c->gl_ctx );
  _swrast_DestroyContext( c->gl_ctx );
  
  _mesa_destroy_visual( c->gl_visual );
  _mesa_destroy_framebuffer( c->gl_buffer );
  _mesa_free_context_data( c->gl_ctx );
    
  if (c->db_flag)
#ifdef DDRAW
    DDFree(c);
#else
  wmDeleteBackingStore(c);
#endif
  free( (void *) c );
#if !defined(NO_PARALLEL)
  if(parallelMachine)
    PRDestroyRenderBuffer();
#endif
}


void WMesaMakeCurrent( WMesaContext c )
{
  if(!c){
    Current = c;
    return;
  }
  
  if(Current == c)
    return;
  
  wmesa_update_state(c->gl_ctx, 0);
  _mesa_make_current(c->gl_ctx, c->gl_buffer);
  Current = c;
  if (Current->gl_ctx->Viewport.Width==0) {
    /* initialize viewport to window size */
    _mesa_Viewport( 0, 0, Current->width, Current->height );
    Current->gl_ctx->Scissor.Width = Current->width;
    Current->gl_ctx->Scissor.Height = Current->height;
  }
  if ((c->cColorBits <= 8 ) && (c->rgb_flag == GL_TRUE)){
    WMesaPaletteChange(c->hPalHalfTone);
  }
}



void WMesaSwapBuffers( void )
{
  HDC DC = Current->hDC;
  GET_CURRENT_CONTEXT(ctx);
  
  /* If we're swapping the buffer associated with the current context
   * we have to flush any pending rendering commands first.
   */
  if (Current && Current->gl_ctx == ctx)
    _mesa_swapbuffers(ctx);
  
  if (Current->db_flag)
    wmFlush(Current);
}



void WMesaPaletteChange(HPALETTE Pal)
{
  int vRet;
  LPPALETTEENTRY pPal;
  if (Current && (Current->rgb_flag==GL_FALSE || 
		  Current->dither_flag == GL_TRUE))
    {
      pPal = (PALETTEENTRY *)malloc( 256 * sizeof(PALETTEENTRY));
      Current->hPal=Pal;
      GetPaletteEntries( Pal, 0, 256, pPal );
#ifdef DDRAW
      Current->lpDD->lpVtbl->CreatePalette(Current->lpDD,DDPCAPS_8BIT,
					   pPal, &(Current->lpDDPal), NULL);
      if (Current->lpDDPal)
	Current->lpDDSPrimary->lpVtbl->SetPalette(Current->lpDDSPrimary,
						  Current->lpDDPal);
#else
      vRet = SetDIBColorTable(Current->dib.hDC, 0, 256, (RGBQUAD*)pPal);
#endif
      free( pPal );
    }
}




static unsigned char threeto8[8] = {
  0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

static unsigned char twoto8[4] = {
  0, 0x55, 0xaa, 0xff
};

static unsigned char oneto8[2] = {
  0, 255
};

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

void wmSetPixel(PWMC pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b)
{
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
    DD_RELEASEDC;
  }
}

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

/*
 * Blit memory DC to screen DC
 */
BOOL wmFlush(PWMC pwc)
{
  BOOL    bRet = 0;
  DWORD   dwErr = 0;
#ifdef DDRAW
  HRESULT             ddrval;
#endif
  
  if(pwc->db_flag){
#ifdef DDRAW
    if (pwc->lpDDSOffScreen == NULL)
      if(DDCreateOffScreen(pwc) == GL_FALSE)
	return;
    
    pwc->lpDDSOffScreen->lpVtbl->Unlock(pwc->lpDDSOffScreen, NULL);
    
    while( 1 )
      {
	ddrval = pwc->lpDDSPrimary->lpVtbl->Blt( pwc->lpDDSPrimary,
						 &(pwc->rectSurface), 
						 pwc->lpDDSOffScreen, 
						 &(pwc->rectOffScreen), 
						 0, NULL );
	
	if( ddrval == DD_OK )
	  {
	    break;
	  }
	if( ddrval == DDERR_SURFACELOST )
	  {
	    if(!DDRestoreAll(pwc))
	      {
		break;
	      }
	  }
	if( ddrval != DDERR_WASSTILLDRAWING )
	  {
	    break;
	  }
      }
    
    while (pwc->lpDDSOffScreen->lpVtbl->Lock(pwc->lpDDSOffScreen,
					     NULL, &(pwc->ddsd), 0, NULL) == 
	   DDERR_WASSTILLDRAWING)
      ;
    
    if(ddrval != DD_OK)
      dwErr = GetLastError();
#else
    bRet = BitBlt(pwc->hDC, 0, 0, pwc->width, pwc->height,
		  pwc->dib.hDC, 0, 0, SRCCOPY);
#endif
  }
  
  return(TRUE);
  
}


/* The following code is added by Li Wei to enable stereo display */

#if !defined(NO_STEREO)

void WMesaShowStereo(GLuint list)
{
  
  GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
  GLfloat cm[16];
  GLint matrix_mode;
  /* Must use double Buffer */
  if( ! Current-> db_flag )
    return;
  
  
  glGetIntegerv(GL_MATRIX_MODE,&matrix_mode);
  
  WMesaViewport(Current->gl_ctx,0,Current->height/2,
		Current->width,Current->height/2);
  if(matrix_mode!=GL_MODELVIEW)
    glMatrixMode(GL_MODELVIEW);
  
  glGetFloatv(GL_MODELVIEW_MATRIX,cm);
  glLoadIdentity();
  gluLookAt(viewDistance/2,0.0,0.0 ,
	    viewDistance/2,0.0,-1.0,
	    0.0,1.0,0.0 );
  glMultMatrixf( cm );
  
  Current->ScreenMem = Current->pbPixels = Current->addrOffScreen;
  glCallList( list );
  
  glGetFloatv(GL_MODELVIEW_MATRIX,cm);
  glLoadIdentity();
  gluLookAt(-viewDistance/2,0.0,0.0 ,
	    -viewDistance/2,0.0,-1.0,
	    0.0,1.0,0.0 );
  glMultMatrixf(cm);
  
  Current->ScreenMem = Current->pbPixels = Current->addrOffScreen + Current->pitch;
  glCallList(list);
  if(matrix_mode!=GL_MODELVIEW)
    glMatrixMode(matrix_mode);
  
  glFlush();
  
  WMesaViewport(Current->gl_ctx,0,0,Current->width,Current->height);
  WMesaSwapBuffers();
}

void toggleStereoMode()
{
  if(!Current->db_flag)
    return;
  if(!stereo_flag){
    stereo_flag = 1;
    if(stereoBuffer==GL_FALSE)
#if !defined(NO_PARALLEL)
      if(!parallelFlag)
#endif
	{
	  Current->ScanWidth = Current->pitch*2;
	}
  }
  else {
    stereo_flag = 0;
#if !defined(NO_PARALLEL)
    if(!parallelFlag)
#endif
      Current->ScanWidth = Current->pitch;
    Current->pbPixels = Current->addrOffScreen;
  }
}

/* if in stereo mode, the following function is called */
void glShowStereo(GLuint list)
{
  WMesaShowStereo(list);
}

#endif /* NO_STEREO */

#if !defined(NO_PARALLEL)

void toggleParallelMode(void)
{
  if(!parallelFlag){
    parallelFlag = GL_TRUE;
    if(parallelMachine==GL_FALSE){
      PRCreateRenderBuffer( Current->rgb_flag? GL_RGBA :GL_COLOR_INDEX,
			    Current->cColorBits/8,
			    Current->width ,Current->height,
			    Current->ScanWidth,
			    Current->rgb_flag? Current->pbPixels: 
			    Current->ScreenMem);
      parallelMachine = GL_TRUE;
    }
  }
  else {
    parallelFlag = GL_FALSE;
    if(parallelMachine==GL_TRUE){
      PRDestroyRenderBuffer();
      parallelMachine=GL_FALSE;
      ReadyForNextFrame = GL_TRUE;
    }
    
    /***********************************************
     * Seems something wrong!!!!
     ************************************************/
    
    WMesaMakeCurrent(Current);
#if !defined(NO_STEREO)
    stereo_flag = GL_FALSE ;
#endif
  }
}

void PRShowRenderResult(void)
{
  int flag = 0;
  if(!glImageRendered())
    return;
  
  if (parallelFlag)
    {
      WMesaSwapBuffers();
    }
  
}
#endif /* NO_PARALLEL */

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

#ifdef DDRAW
/*
 * restoreAll
 *
 * restore all lost objects
 */
HRESULT DDRestoreAll( WMesaContext wc )
{
  HRESULT     ddrval;
  
  ddrval = wc->lpDDSPrimary->lpVtbl->Restore(wc->lpDDSPrimary);
  if( ddrval == DD_OK )
    {
      ddrval = wc->lpDDSOffScreen->lpVtbl->Restore(wc->lpDDSOffScreen);
    }
  return ddrval;
  
} /* restoreAll */


/*
 * This function is called if the initialization function fails
 */
BOOL initFail( HWND hwnd, WMesaContext wc )
{
  DDFree(wc);
  MessageBox( hwnd, "DirectDraw Init FAILED", "", MB_OK );
  return FALSE;
  
} /* initFail */


static void DDDeleteOffScreen(WMesaContext wc)
{
  if( wc->lpDDSOffScreen != NULL )
    {
      wc->lpDDSOffScreen->lpVtbl->Unlock(wc->lpDDSOffScreen,NULL);
      wc->lpDDSOffScreen->lpVtbl->Release(wc->lpDDSOffScreen);
      wc->lpDDSOffScreen = NULL;
    }
  
}

static void DDFreePrimarySurface(WMesaContext wc)
{
  if( wc->lpDDSPrimary != NULL )
    {
      if(wc->db_flag == GL_FALSE)
	wc->lpDDSPrimary->lpVtbl->ReleaseDC(wc->lpDDSPrimary, wc->hDC);
      wc->lpDDSPrimary->lpVtbl->Release(wc->lpDDSPrimary);
      wc->lpDDSPrimary = NULL;
    }
}

static BOOL DDCreatePrimarySurface(WMesaContext wc)
{
  HRESULT ddrval;
  DDSCAPS             ddscaps;
  wc->ddsd.dwSize = sizeof( wc->ddsd );
  wc->ddsd.dwFlags = DDSD_CAPS;
  wc->ddsd.ddsCaps.dwCaps =   DDSCAPS_PRIMARYSURFACE;
  
  ddrval = wc->lpDD->lpVtbl->CreateSurface( wc->lpDD,&(wc->ddsd), 
					    &(wc->lpDDSPrimary), NULL );
  if( ddrval != DD_OK )
    {
      return initFail(wc->hwnd , wc);
    }
  if(wc->db_flag == GL_FALSE)
    wc->lpDDSPrimary->lpVtbl->GetDC(wc->lpDDSPrimary, wc->hDC);
  return TRUE;
}

static BOOL DDCreateOffScreen(WMesaContext wc)
{
  POINT   pt;
  HRESULT     ddrval;
  if(wc->lpDD == NULL)
    return FALSE;
  GetClientRect( wc->hwnd, &(wc->rectOffScreen) );
  wc->ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  wc->ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  wc->ddsd.dwHeight = wc->rectOffScreen.bottom - wc->rectOffScreen.top;
  wc->ddsd.dwWidth = wc->rectOffScreen.right - wc->rectOffScreen.left;
  
  ddrval = wc->lpDD->lpVtbl->CreateSurface( wc->lpDD, &(wc->ddsd), 
					    &(wc->lpDDSOffScreen), NULL );
  if( ddrval != DD_OK )
    {
      return FALSE;
    }
  
  while (wc->lpDDSOffScreen->lpVtbl->Lock(wc->lpDDSOffScreen,NULL, 
					  &(wc->ddsd), 0, NULL) == 
	 DDERR_WASSTILLDRAWING)
    ;
  
  if(wc->ddsd.lpSurface==NULL)
    return initFail(wc->hwnd, wc);
  
  wc->ScreenMem = wc->pbPixels = wc->addrOffScreen = 
    (PBYTE)(wc->ddsd.lpSurface);
  wc->ScanWidth = wc->pitch = wc->ddsd.lPitch;
  if (stereo_flag)
    wc->ScanWidth = wc->ddsd.lPitch*2;
  
  GetClientRect( wc->hwnd, &(wc->rectSurface) );
  pt.x = pt.y = 0;
  ClientToScreen( wc->hwnd, &pt );
  OffsetRect(&(wc->rectSurface), pt.x, pt.y);
  wmSetPixelFormat(wc, wc->hDC);
  return TRUE;
}

/*
 * doInit - do work required for every instance of the application:
 *                create the window, initialize data
 */
static BOOL DDInit( WMesaContext wc, HWND hwnd)
{
  HRESULT             ddrval;
  DWORD dwFrequency;
  
  LPDIRECTDRAW            lpDD;           // DirectDraw object
  LPDIRECTDRAW2            lpDD2;
  
  
  wc->fullScreen = displayOptions.fullScreen;
  wc->gMode = displayOptions.mode;
  wc->hwnd = hwnd;
  stereo_flag = displayOptions.stereo;
  if(wc->db_flag!= GL_TRUE)
    stereo_flag = GL_FALSE;
  /*
   * create the main DirectDraw object
   */
  ddrval = DirectDrawCreate( NULL, &(wc->lpDD), NULL );
  if( ddrval != DD_OK )
    {
      return initFail(hwnd,wc);
    }
  
  // Get exclusive mode if requested
  if(wc->fullScreen)
    {
      ddrval = wc->lpDD->lpVtbl->SetCooperativeLevel( wc->lpDD, hwnd, 
						      DDSCL_EXCLUSIVE | 
						      DDSCL_FULLSCREEN );
    }
  else
    {
      ddrval = wc->lpDD->lpVtbl->SetCooperativeLevel( wc->lpDD, hwnd, 
						      DDSCL_NORMAL );
    }
  if( ddrval != DD_OK )
    {
      return initFail(hwnd , wc);
    }
  
  
  if(ddrval != DD_OK)
    return initFail(hwnd , wc);
  
  
  switch( wc->gMode )
    {
    case 1:  
      ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 640, 480, 
						 displayOptions.bpp); 
      break;
    case 2:  
      ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 800, 600, 
						 displayOptions.bpp); 
      break;
    case 3:  
      ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1024, 768, 
						 displayOptions.bpp); 
      break;
    case 4:  
      ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1152, 864, 
						 displayOptions.bpp); 
      break;
    case 5:  
      ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1280, 1024, 
						 displayOptions.bpp); 
      break;
    }

  if( ddrval != DD_OK )
    {
      printf("Can't modify display mode, current mode used\n");
    }
  switch(ddrval){
  case DDERR_INVALIDOBJECT:
    break;
  case DDERR_INVALIDPARAMS:
    break;
  case DDERR_UNSUPPORTEDMODE:
    ;
  }
  
  if(DDCreatePrimarySurface(wc) == GL_FALSE)
    return initFail(hwnd, wc);
  
  if(wc->db_flag)
    return DDCreateOffScreen(wc);
} /* DDInit */

static void DDFree( WMesaContext wc)
{
  if( wc->lpDD != NULL )
    {
      DDFreePrimarySurface(wc);
      DDDeleteOffScreen(wc);
      wc->lpDD->lpVtbl->Release(wc->lpDD);
      wc->lpDD = NULL;
    }
  // Clean up the screen on exit
  RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
		RDW_ALLCHILDREN );
  
}
#endif

void WMesaMove(void)
{
  WMesaContext wc = Current;
  POINT   pt;
  if (Current != NULL){
    GetClientRect( wc->hwnd, &(wc->rectSurface) );
    pt.x = pt.y = 0;
    ClientToScreen( wc->hwnd, &pt );
    OffsetRect(&(wc->rectSurface), pt.x, pt.y);
  }
}


/************************************************
 * Mesa 4.0 - These triangle rasterizers are not
 * implemented in this version of the Windows
 * driver.  They could be implemented for a
 * potential performance improvement.
 * See OSMesa for an example of the approach
 * to use.
 * This old code is left in this file in case
 * it is useful.  However, it may end up looking
 * a lot more like the OSMesa code.
 ************************************************/


#if 0


/*
 * Like PACK_8A8B8G8R() but don't use alpha.  This is usually an acceptable
 * shortcut.
 */
#define PACK_8B8G8R( R, G, B )   ( ((B) << 16) | ((G) << 8) | (R) )

/**********************************************************************/
/***                   Triangle rendering                            ***/
/**********************************************************************/

/*
 * XImage, smooth, depth-buffered, PF_8A8B8G8R triangle.
 */
static void smooth_8A8B8G8R_z_triangle( GLcontext *ctx,
                                       GLuint v0, GLuint v1, GLuint v2,
                                       GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint i, len = RIGHT-LEFT;                      \
    for (i=0;i<len;i++) {                       \
    GLdepth z = FixedToDepth(ffz);                  \
    if (z < zRow[i]) {                      \
    pRow[i] = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),    \
    FixedToInt(ffb) );          \
    zRow[i] = z;                            \
    }                                   \
    ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    ffz += fdzdx;                           \
    }                                   \
    }

#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, smooth, depth-buffered, PF_8R8G8B triangle.
*/
static void smooth_8R8G8B_z_triangle( GLcontext *ctx,
                                     GLuint v0, GLuint v1, GLuint v2,
                                     GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint i, len = RIGHT-LEFT;                      \
    for (i=0;i<len;i++) {                       \
    GLdepth z = FixedToDepth(ffz);                  \
    if (z < zRow[i]) {                      \
    pRow[i] = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),    \
    FixedToInt(ffb) );          \
    zRow[i] = z;                            \
    }                                   \
    ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    ffz += fdzdx;                           \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}



/*
* XImage, smooth, depth-buffered, PF_5R6G5B triangle.
*/
static void smooth_5R6G5B_z_triangle( GLcontext *ctx,
                                     GLuint v0, GLuint v1, GLuint v2,
                                     GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(X,Y)
#define PIXEL_TYPE GLushort
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint i, len = RIGHT-LEFT;                      \
    for (i=0;i<len;i++) {                       \
    GLdepth z = FixedToDepth(ffz);                  \
    if (z < zRow[i]) {                      \
    pRow[i] = PACK_5R6G5B( FixedToInt(ffr), FixedToInt(ffg),    \
    FixedToInt(ffb) );          \
    zRow[i] = z;                            \
    }                                   \
    ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    ffz += fdzdx;                           \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

/*
* XImage, flat, depth-buffered, PF_8A8B8G8R triangle.
*/
static void flat_8A8B8G8R_z_triangle( GLcontext *ctx, GLuint v0,
                                     GLuint v1, GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_8B8G8R( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint i, len = RIGHT-LEFT;                      \
    for (i=0;i<len;i++) {                       \
    GLdepth z = FixedToDepth(ffz);                  \
    if (z < zRow[i]) {                      \
    pRow[i] = p;                            \
    zRow[i] = z;                            \
    }                                   \
    ffz += fdzdx;                           \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, depth-buffered, PF_8R8G8B triangle.
*/
static void flat_8R8G8B_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                   GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_8R8G8B( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )            \
    {                           \
    GLint i, len = RIGHT-LEFT;              \
    for (i=0;i<len;i++) {               \
    GLdepth z = FixedToDepth(ffz);          \
    if (z < zRow[i]) {              \
    pRow[i] = p;                    \
    zRow[i] = z;                    \
    }                           \
    ffz += fdzdx;                   \
    }                           \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, depth-buffered, PF_5R6G5B triangle.
*/
static void flat_5R6G5B_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                   GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(X,Y)
#define PIXEL_TYPE GLushort
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_5R6G5B( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )            \
    {                           \
    GLint i, len = RIGHT-LEFT;              \
    for (i=0;i<len;i++) {               \
    GLdepth z = FixedToDepth(ffz);          \
    if (z < zRow[i]) {              \
    pRow[i] = p;                    \
    zRow[i] = z;                    \
    }                           \
    ffz += fdzdx;                   \
    }                           \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, smooth, NON-depth-buffered, PF_8A8B8G8R triangle.
*/
static void smooth_8A8B8G8R_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                     GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint xx;                               \
    PIXEL_TYPE *pixel = pRow;                       \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {               \
    *pixel = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),     \
                FixedToInt(ffb) );          \
                ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, smooth, NON-depth-buffered, PF_8R8G8B triangle.
*/
static void smooth_8R8G8B_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                   GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint xx;                               \
    PIXEL_TYPE *pixel = pRow;                       \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {               \
    *pixel = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),     \
                FixedToInt(ffb) );          \
                ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, smooth, NON-depth-buffered, PF_5R6G5B triangle.
*/
static void smooth_5R6G5B_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                   GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(X,Y)
#define PIXEL_TYPE GLushort
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint xx;                               \
    PIXEL_TYPE *pixel = pRow;                       \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {               \
    *pixel = PACK_5R6G5B( FixedToInt(ffr), FixedToInt(ffg),     \
                FixedToInt(ffb) );          \
                ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;         \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}



/*
* XImage, flat, NON-depth-buffered, PF_8A8B8G8R triangle.
*/
static void flat_8A8B8G8R_triangle( GLcontext *ctx, GLuint v0,
                                   GLuint v1, GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_8B8G8R( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )            \
    {                           \
    GLint xx;                       \
    PIXEL_TYPE *pixel = pRow;               \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {       \
    *pixel = p;                 \
    }                           \
    }

#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, NON-depth-buffered, PF_8R8G8B triangle.
*/
static void flat_8R8G8B_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                 GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_8R8G8B( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )            \
    {                           \
    GLint xx;                       \
    PIXEL_TYPE *pixel = pRow;               \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {       \
    *pixel = p;                 \
    }                           \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, NON-depth-buffered, PF_5R6G5B triangle.
*/
static void flat_5R6G5B_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                 GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define PIXEL_ADDRESS(X,Y) PIXELADDR2(X,Y)
#define PIXEL_TYPE GLushort
    //#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE                  \
    unsigned long p = PACK_5R6G5B( VB->ColorPtr->data[pv][0],    \
    VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )            \
    {                           \
    GLint xx;                       \
    PIXEL_TYPE *pixel = pRow;               \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {       \
    *pixel = p;                 \
    }                           \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, smooth, depth-buffered, 8-bit PF_LOOKUP triangle.
*/

static void smooth_ci_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                 GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define INTERP_INDEX 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                                \
    {                                                                   \
    GLint i, len = RIGHT-LEFT;                                      \
    for (i=0;i<len;i++) {                                           \
    GLdepth z = FixedToDepth(ffz);                              \
    if (z < zRow[i]) {                                          \
    pRow[i] = FixedToInt(ffi);                                  \
    zRow[i] = z;                                                \
    }                                                               \
    ffi += fdidx;                                                   \
    ffz += fdzdx;                                                   \
    }                                                               \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, depth-buffered, 8-bit PF_LOOKUP triangle.
*/

static void flat_ci_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                               GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE			\
   GLuint index = VB->IndexPtr->data[pv];	\
   (*ctx->Driver.Index)( ctx, index );
#define INNER_LOOP( LEFT, RIGHT, Y )	\
   {					\
      GLint i, len = RIGHT-LEFT;	\
      for (i=0;i<len;i++) {		\
         GLdepth z = FixedToDepth(ffz);	\
         if (z < zRow[i]) {		\
            pRow[i] = index;		\
            zRow[i] = z;		\
         }				\
         ffz += fdzdx;			\
      }					\
   }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}



/*
* XImage, smooth, NON-depth-buffered, 8-bit PF_LOOKUP triangle.
*/

static void smooth_ci_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                               GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define INTERP_INDEX 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                    \
    {                                   \
    GLint xx;                               \
    PIXEL_TYPE *pixel = pRow;                       \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {               \
    *pixel = FixedToInt(ffi);           \
    ffi += fdidx;           \
    }                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}


/*
* XImage, flat, NON-depth-buffered, 8-bit PF_LOOKUP triangle.
*/
static void flat_ci_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                             GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define SETUP_CODE			\
   GLuint index = VB->IndexPtr->data[pv];	\
   (*ctx->Driver.Index)( ctx, index );
#define INNER_LOOP( LEFT, RIGHT, Y )		\
   {						\
      GLint xx;					\
      PIXEL_TYPE *pixel = pRow;			\
      for (xx=LEFT;xx<RIGHT;xx++,pixel++) {	\
         *pixel = index;			\
      }						\
   }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

/*
* XImage, smooth, depth-buffered, 8-bit, PF_DITHER8 triangle.
*/
static void smooth_DITHER8_z_triangle( GLcontext *ctx,
                                      GLuint v0, GLuint v1, GLuint v2,
                                      GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
    DITHER_RGB_TO_8BIT_SETUP
#define INTERP_Z 1
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                                    \
    {                                                                       \
    GLint i, xx = LEFT, yy = FLIP(Y), len = RIGHT-LEFT;                 \
    for (i=0;i<len;i++,xx++) {                                          \
    GLdepth z = FixedToDepth(ffz);                                  \
    if (z < zRow[i]) {                                              \
    DITHER_RGB_TO_8BIT( FixedToInt(ffr), FixedToInt(ffg),           \
    FixedToInt(ffb), xx, yy);               \
    pRow[i] = pixelDithered;                                        \
    zRow[i] = z;                                                    \
    }                                                                   \
    ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;                     \
    ffz += fdzdx;                                                       \
    }                                                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

/*
* XImage, flat, depth-buffered, 8-bit PF_DITHER triangle.
*/
static void flat_DITHER8_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                    GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
    DITHER_RGB_TO_8BIT_SETUP
#define INTERP_Z 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)

#define INNER_LOOP( LEFT, RIGHT, Y )                                    \
    {                                                                       \
    GLint i, xx = LEFT, yy = FLIP(Y), len = RIGHT-LEFT;                 \
    for (i=0;i<len;i++,xx++) {                                          \
    GLdepth z = FixedToDepth(ffz);                                  \
    if (z < zRow[i]) {                                              \
    DITHER_RGB_TO_8BIT( VB->ColorPtr->data[pv][0],                           \
             VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2], xx, yy);               \
             pRow[i] = pixelDithered;                                       \
             zRow[i] = z;                                                   \
    }                                                                   \
    ffz += fdzdx;                                                       \
    }                                                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

/*
* XImage, smooth, NON-depth-buffered, 8-bit PF_DITHER triangle.
*/
static void smooth_DITHER8_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                    GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
    DITHER_RGB_TO_8BIT_SETUP
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )                                    \
    {                                                                       \
    GLint xx, yy = FLIP(Y);                                             \
    PIXEL_TYPE *pixel = pRow;                                           \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {                               \
    DITHER_RGB_TO_8BIT( VB->ColorPtr->data[pv][0],   VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2], xx, yy);\
    *pixel = pixelDithered;                                         \
    ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;                     \
    }                                                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

/*
* XImage, flat, NON-depth-buffered, 8-bit PF_DITHER triangle.
*/

static void flat_DITHER8_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                  GLuint v2, GLuint pv )
{
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
    DITHER_RGB_TO_8BIT_SETUP
#define PIXEL_ADDRESS(X,Y) PIXELADDR1(X,Y)
#define PIXEL_TYPE GLubyte
#define BYTES_PER_ROW (wmesa->ScanWidth)

#define INNER_LOOP( LEFT, RIGHT, Y )                                    \
    {                                                                       \
    GLint xx, yy = FLIP(Y);                                             \
    PIXEL_TYPE *pixel = pRow;                                           \
    for (xx=LEFT;xx<RIGHT;xx++,pixel++) {                               \
    DITHER_RGB_TO_8BIT( VB->ColorPtr->data[pv][0],                               \
             VB->ColorPtr->data[pv][1], VB->ColorPtr->data[pv][2], xx, yy);               \
             *pixel = pixelDithered;                                            \
    }                                                                   \
    }
#ifdef __MINGW32__
	#include "tritemp.h"
#else

	#ifdef WIN32
//		#include "..\tritemp.h"
	#else
		#include "tritemp.h"
	#endif
#endif
}

#endif
/************** END DEAD TRIANGLE CODE ***********************/

static triangle_func choose_triangle_function( GLcontext *ctx )
{
#if 0
    WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
    int depth = wmesa->cColorBits;

    if (ctx->Polygon.SmoothFlag)     return NULL;
    if (ctx->Texture._ReallyEnabled)  return NULL;
    if (!wmesa->db_flag) return NULL;
    /*if (wmesa->xm_buffer->buffer==XIMAGE)*/ {
    if (   ctx->Light.ShadeModel==GL_SMOOTH
        && ctx->_RasterMask==DEPTH_BIT
        && ctx->Depth.Func==GL_LESS
        && ctx->Depth.Mask==GL_TRUE
        && ctx->Polygon.StippleFlag==GL_FALSE) {
        switch (wmesa->pixelformat) {
        case PF_8A8B8G8R:
            return smooth_8A8B8G8R_z_triangle;
        case PF_8R8G8B:
            return smooth_8R8G8B_z_triangle;
        case PF_5R6G5B:
            return smooth_5R6G5B_z_triangle;
        case PF_DITHER8:
            return  smooth_DITHER8_z_triangle;
        case PF_INDEX8:
            return smooth_ci_z_triangle;
        default:
            return NULL;
        }
    }
    if (   ctx->Light.ShadeModel==GL_FLAT
        && ctx->_RasterMask==DEPTH_BIT
        && ctx->Depth.Func==GL_LESS
        && ctx->Depth.Mask==GL_TRUE
        && ctx->Polygon.StippleFlag==GL_FALSE) {
        switch (wmesa->pixelformat) {
        case PF_8A8B8G8R:
            return flat_8A8B8G8R_z_triangle;
        case PF_8R8G8B:
            return flat_8R8G8B_z_triangle;
        case PF_5R6G5B:
            return flat_5R6G5B_z_triangle;
        case PF_DITHER8:
            return flat_DITHER8_z_triangle;
        case PF_INDEX8:
            return flat_ci_z_triangle;
        default:
            return NULL;
        }
    }
    if (   ctx->_RasterMask==0   /* no depth test */
        && ctx->Light.ShadeModel==GL_SMOOTH
        && ctx->Polygon.StippleFlag==GL_FALSE) {
        switch (wmesa->pixelformat) {
        case PF_8A8B8G8R:
            return smooth_8A8B8G8R_triangle;
        case PF_8R8G8B:
            return smooth_8R8G8B_triangle;
        case PF_5R6G5B:
            return smooth_5R6G5B_triangle;
        case PF_DITHER8:
            return smooth_DITHER8_triangle;
        case PF_INDEX8:
            return smooth_ci_triangle;
        default:
            return NULL;
        }
    }

    if (   ctx->_RasterMask==0   /* no depth test */
        && ctx->Light.ShadeModel==GL_FLAT
        && ctx->Polygon.StippleFlag==GL_FALSE) {
        switch (wmesa->pixelformat) {
        case PF_8A8B8G8R:
            return flat_8A8B8G8R_triangle;
        case PF_8R8G8B:
            return flat_8R8G8B_triangle;
        case PF_5R6G5B:
            return flat_5R6G5B_triangle;
        case PF_DITHER8:
            return flat_DITHER8_triangle;
        case PF_INDEX8:
            return flat_ci_triangle;
        default:
            return NULL;
        }
    }

    return NULL;
    }
#endif
}

/*
 * Define a new viewport and reallocate auxillary buffers if the size of
 * the window (color buffer) has changed.
 */
void WMesaViewport( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height )
{
  assert(0);  /* I don't think that this is being used. */
#if 0
  /* Save viewport */
  ctx->Viewport.X = x;
  ctx->Viewport.Width = width;
  ctx->Viewport.Y = y;
  ctx->Viewport.Height = height;
  
  /* compute scale and bias values */
/* Pre-Keith 3.1 changes
   ctx->Viewport.Map.m[Sx] = (GLfloat) width / 2.0F;
   ctx->Viewport.Map.m[Tx] = ctx->Viewport.Sx + x;
   ctx->Viewport.Map.m[Sy] = (GLfloat) height / 2.0F;
   ctx->Viewport.Map.m[Ty] = ctx->Viewport.Sy + y;
*/
  ctx->Viewport.WindowMap.m[MAT_SX] = (GLfloat) width / 2.0F;
  ctx->Viewport.WindowMap.m[MAT_TX] = ctx->Viewport.WindowMap.m[MAT_SX] + x;
  ctx->Viewport.WindowMap.m[MAT_SY] = (GLfloat) height / 2.0F;
  ctx->Viewport.WindowMap.m[MAT_TY] = ctx->Viewport.WindowMap.m[MAT_SY] + y;
#endif
}
