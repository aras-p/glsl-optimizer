/*
 *	File name	:	wmesa.c
 *  Version		:	2.3
 *
 *  Display driver for Mesa 2.3  under
 *	Windows95 and WindowsNT
 *
 *	Copyright (C) 1996-  Li Wei
 *  Address		:		Institute of Artificial Intelligence
 *				:			& Robotics
 *				:		Xi'an Jiaotong University
 *  Email		:		liwei@aiar.xjtu.edu.cn
 *  Web page	:		http://sun.aiar.xjtu.edu.cn
 *
 *  This file and its associations are partially borrowed from the
 *  Windows NT driver for Mesa 1.8 , written by Mark Leaming
 *  (mark@rsinc.com).
 */


/*
 * $Log: wmesaOld.c,v $
 * Revision 1.3  2001/03/03 20:33:29  brianp
 * lots of gl_*() to _mesa_*() namespace clean-up
 *
 * Revision 1.2  2000/11/05 18:41:00  keithw
 * - Changes for new software rasterizer modules
 * - Remove support for choosing software fallbacks from core code
 * - Remove partial fallback code from vbrender.c -- drivers are now
 *   expected to be able to find a triangle/quad function for every state,
 *   even if they have to use _swsetup_Triangle or _swsetup_Quad.
 * - Marked derived variables in the GLcontext struct with a leading
 *   underscore '_'.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.2  1999/01/03 03:08:57  brianp
 * Ted Jump's changes
 *
 * Revision 1.0  1997/06/14 17:51:00 CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * New display driver for Mesa 2.x using Microsoft Direct Draw
 * Initial vision
 */


#define WMESA_STEREO_C

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/wmesa.h>
#include "wmesadef.h"
#include "context.h"
#include "dd.h"
#include "xform.h"
#include "vb.h"
#include "matrix.h"
#include "depth.h"

#ifdef PROFILE
//	#include "profile.h"
#endif

#ifdef DITHER
	#include <wing.h>
#endif

#ifdef __CYGWIN32__
#include "macros.h"
#include <string.h>
#define CopyMemory memcpy
#endif
#include "mesa_extend.h"
#include "colors.h"

#if !defined(NO_STEREO)

	#include "gl\glu.h"
	#include "stereo.h"

#endif
#if !defined(NO_PARALLEL)
//	#include "parallel.h"
#endif

struct DISPLAY_OPTIONS displayOptions;

GLenum stereoCompile = GL_FALSE ;
GLenum stereoShowing  = GL_FALSE ;
GLenum stereoBuffer = GL_FALSE;
#if !defined(NO_STEREO)
GLint displayList = MAXIMUM_DISPLAY_LIST ;
#endif
GLint stereo_flag = 0 ;

/* end of added code*/

static PWMC Current = NULL;
WMesaContext WC = NULL;

#ifdef NDEBUG
  #define assert(ignore)	((void) 0)
#else
  void Mesa_Assert(void *Cond,void *File,unsigned Line)
  {
    char Msg[512];
    sprintf(Msg,"%s %s %d",Cond,File,Line);
    MessageBox(NULL,Msg,"Assertion failed.",MB_OK);
    exit(1);
  }
  #define assert(e)	if (!e) Mesa_Assert(#e,__FILE__,__LINE__);
#endif

//#define DD_GETDC (Current->hDC )
#define DD_GETDC ((Current->db_flag) ? Current->dib.hDC : Current->hDC )
//#define DD_GETDC ((Current->db_flag) ? Current->hDCPrimary : Current->hDCBack )
#define DD_RELEASEDC

//#define BEGINGDICALL	if(Current->rgb_flag)wmFlushBits(Current);
#define BEGINGDICALL
//#define ENDGDICALL		if(Current->rgb_flag)wmGetBits(Current);
#define ENDGDICALL

//#define FLIP(Y)  (Current->dither_flag? Y : Current->height-(Y)-1)
//#define FLIP(Y)  (Current->height-(Y)-1)
//#define FLIP(Y) Y
#define FLIP(Y)  (Current->db_flag? Y: Current->height-(Y)-1)
#define STARTPROFILE
#define ENDPROFILE(PARA)

#define DITHER_RGB_TO_8BIT_SETUP			\
		GLubyte pixelDithered;

#define DITHER_RGB_TO_8BIT(red, green, blue, pixel, scanline)				\
{																			\
	char unsigned redtemp, greentemp, bluetemp, paletteindex;				\
    redtemp = aDividedBy51[red]												\
          + (aModulo51[red] > aHalftone8x8[(pixel%8)*8						\
          + scanline%8]);													\
    greentemp = aDividedBy51[(char unsigned)green]							\
          + (aModulo51[green] > aHalftone8x8[								\
          (pixel%8)*8 + scanline%8]);										\
    bluetemp = aDividedBy51[(char unsigned)blue]							\
          + (aModulo51[blue] > aHalftone8x8[								\
          (pixel%8)*8 +scanline%8]);										\
    paletteindex = redtemp + aTimes6[greentemp] + aTimes36[bluetemp];		\
	pixelDithered = aWinGHalftoneTranslation[paletteindex];					\
}


#ifdef DDRAW
	static BOOL DDInit( WMesaContext wc, HWND hwnd);
	static void DDFree( WMesaContext wc);
	static HRESULT DDRestoreAll( WMesaContext wc );
	static void DDDeleteOffScreen(WMesaContext wc);
	static BOOL DDCreateOffScreen(WMesaContext wc);
#endif

static void FlushToFile(PWMC pwc, PSTR	szFile);

BOOL wmCreateBackingStore(PWMC pwc, long lxSize, long lySize);
BOOL wmDeleteBackingStore(PWMC pwc);
void wmCreatePalette( PWMC pwdc );
BOOL wmSetDibColors(PWMC pwc);
void wmSetPixel(PWMC pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b);

void wmCreateDIBSection(
	HDC	 hDC,
    PWMC pwc,	// handle of device context
    CONST BITMAPINFO *pbmi,	// address of structure containing bitmap size, format, and color data
    UINT iUsage	// color data type indicator: RGB values or palette indices
    );


void WMesaViewport( GLcontext *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height );

static triangle_func choose_triangle_function( GLcontext *ctx );

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

//
// This function sets the color table of a DIB section
// to match that of the destination DC
//
BOOL /*WINAPI*/ wmSetDibColors(PWMC pwc)
{
    RGBQUAD			*pColTab, *pRGB;
    PALETTEENTRY	*pPal, *pPE;
    int				i, nColors;
	BOOL			bRet=TRUE;
	DWORD			dwErr=0;

    /* Build a color table in the DIB that maps to the
       selected palette in the DC.
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

	return(bRet);
}


//
// Free up the dib section that was created
//
BOOL wmDeleteBackingStore(PWMC pwc)
{
	SelectObject(pwc->dib.hDC, pwc->hOldBitmap);
	DeleteDC(pwc->dib.hDC);
	DeleteObject(pwc->hbmDIB);
	UnmapViewOfFile(pwc->dib.base);
	CloseHandle(pwc->dib.hFileMap);
	return TRUE;
}


//
// This function creates the DIB section that is used for combined
// GL and GDI calls
//
BOOL /*WINAPI*/ wmCreateBackingStore(PWMC pwc, long lxSize, long lySize)
{
    HDC hdc = pwc->hDC;
    LPBITMAPINFO pbmi = &(pwc->bmi);
	int		iUsage;

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
	return(TRUE);

}


//
// This function copies one scan line in a DIB section to another
//
BOOL GLWINAPI wmSetDIBits(PWMC pwc, UINT uiScanWidth, UINT uiNumScans, UINT nBypp, UINT uiNewWidth, LPBYTE pBits)
{
	UINT uiScans = 0;
	LPBYTE	pDest = pwc->pbPixels;
	DWORD	dwNextScan = uiScanWidth;
	DWORD	dwNewScan = uiNewWidth;
	DWORD	dwScanWidth = (uiScanWidth * nBypp);

	//
	// We need to round up to the nearest DWORD
	// and multiply by the number of bytes per
	// pixel
	//
	dwNextScan = (((dwNextScan * nBypp)+ 3) & ~3);
	dwNewScan = (((dwNewScan * nBypp)+ 3) & ~3);

	for(uiScans = 0; uiScans < uiNumScans; uiScans++){
		CopyMemory(pDest, pBits, dwScanWidth);
		pBits += dwNextScan;
		pDest += dwNewScan;
	}

	return(TRUE);

}


BOOL wmFlush(PWMC pwc);

/*
 * Useful macros:
   Modified from file osmesa.c
 */


#define PIXELADDR(X,Y)  ((GLubyte *)Current->pbPixels + (Current->height-Y-1)* Current->ScanWidth + (X)*nBypp)
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


//
// We cache all gl draw routines until a flush is made
//
static void flush(GLcontext* ctx)
{
	STARTPROFILE
	if((Current->rgb_flag /*&& !(Current->dib.fFlushed)*/&&!(Current->db_flag))
		||(!Current->rgb_flag))
	{
		wmFlush(Current);
	}
	ENDPROFILE(flush)

}



/*
 * Set the color index used to clear the color buffer.
 */
static void clear_index(GLcontext* ctx, GLuint index)
{
  STARTPROFILE
  Current->clearpixel = index;
  ENDPROFILE(clear_index)
}



/*
 * Set the color used to clear the color buffer.
 */
static void clear_color( GLcontext* ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
  STARTPROFILE
  Current->clearpixel=RGB(r, g, b );
  ENDPROFILE(clear_color)
}



/*
 * Clear the specified region of the color buffer using the clear color
 * or index as specified by one of the two functions above.
 */
static void clear(GLcontext* ctx,
				  GLboolean all,GLint x, GLint y, GLint width, GLint height )
{
	DWORD	dwColor;
	WORD	wColor;
	BYTE	bColor;
	LPDWORD	lpdw = (LPDWORD)Current->pbPixels;
	LPWORD	lpw = (LPWORD)Current->pbPixels;
	LPBYTE	lpb = Current->pbPixels;
	int		lines;

    STARTPROFILE

	if (all){
		x=y=0;
		width=Current->width;
		height=Current->height;
	}
	if(Current->db_flag==GL_TRUE){
		UINT	nBypp = Current->cColorBits / 8;
		int		i = 0;
		int		iSize;

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

		//
		// This is the 24bit case
		//
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
		if(stereo_flag) lines = height /2;
		else lines = height;
		do{
			lpb += Current->ScanWidth;
			memcpy(lpb, Current->pbPixels, iSize*4);
			i++;
		}
		while(i<lines-1);
	}
	else{ // For single buffer
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



	ENDPROFILE(clear)
}



/* Set the current color index. */
static void set_index(GLcontext* ctx, GLuint index)
{
  STARTPROFILE
  Current->pixel=index;
  ENDPROFILE(set_index)
}



/* Set the current RGBA color. */
static void set_color( GLcontext* ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
  STARTPROFILE
  Current->pixel = RGB( r, g, b );
  ENDPROFILE(set_color)
}



/* Set the index mode bitplane mask. */
static GLboolean index_mask(GLcontext* ctx, GLuint mask)
{
   /* can't implement */
   return GL_FALSE;
}



/* Set the RGBA drawing mask. */
static GLboolean color_mask( GLcontext* ctx,
							 GLboolean rmask, GLboolean gmask,
							 GLboolean bmask, GLboolean amask)
{
   /* can't implement */
   return GL_FALSE;
}



/*
 * Set the pixel logic operation.  Return GL_TRUE if the device driver
 * can perform the operation, otherwise return GL_FALSE.  If GL_FALSE
 * is returned, the logic op will be done in software by Mesa.
 */
GLboolean logicop( GLcontext* ctx, GLenum op )
{
   /* can't implement */
   return GL_FALSE;
}


static void dither( GLcontext* ctx, GLboolean enable )
{
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



static GLboolean set_buffer( GLcontext* ctx, GLenum mode )
{
   STARTPROFILE
   /* TODO: this could be better */
   if (mode==GL_FRONT || mode==GL_BACK) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
   ENDPROFILE(set_buffer)
}



/* Return characteristics of the output buffer. */
static void buffer_size( GLcontext* ctx, GLuint *width, GLuint *height /*, GLuint *depth */)
{

	int New_Size;
	RECT CR;

	STARTPROFILE
	GetClientRect(Current->Window,&CR);

	*width=CR.right;
	*height=CR.bottom;
//	*depth = Current->depth;

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

//	Resize OsmesaBuffer if in Parallel mode
#if !defined(NO_PARALLEL)
			if(parallelFlag)
			PRSizeRenderBuffer(Current->width, Current->height,Current->ScanWidth,
			Current->rgb_flag == GL_TRUE ? Current->pbPixels: Current->ScreenMem);
#endif
		}
   ENDPROFILE(buffer_size)
}



/**********************************************************************/
/*****           Accelerated point, line, polygon rendering       *****/
/**********************************************************************/


static void fast_rgb_points( GLcontext* ctx, GLuint first, GLuint last )
{
   GLuint i;
 //  HDC DC=DD_GETDC;
	PWMC	pwc = Current;

	STARTPROFILE

	if (Current->gl_ctx->VB->MonoColor) {
      /* all drawn with current color */
      for (i=first;i<=last;i++) {
         if (!Current->gl_ctx->VB->ClipMask[i]) {
            int x, y;
            x =       (GLint) Current->gl_ctx->VB->Win[i][0];
            y = FLIP( (GLint) Current->gl_ctx->VB->Win[i][1] );
			wmSetPixel(pwc, y,x,GetRValue(Current->pixel),
					    GetGValue(Current->pixel), GetBValue(Current->pixel));
         }
      }
   }
   else {
      /* draw points of different colors */
      for (i=first;i<=last;i++) {
         if (!Current->gl_ctx->VB->ClipMask[i]) {
            int x, y;
            unsigned long pixel=RGB(Current->gl_ctx->VB->Color[i][0]*255.0,
                                    Current->gl_ctx->VB->Color[i][1]*255.0,
                                    Current->gl_ctx->VB->Color[i][2]*255.0);
            x =       (GLint) Current->gl_ctx->VB->Win[i][0];
            y = FLIP( (GLint) Current->gl_ctx->VB->Win[i][1] );
			wmSetPixel(pwc, y,x,Current->gl_ctx->VB->Color[i][0]*255.0,
                                    Current->gl_ctx->VB->Color[i][1]*255.0,
                                    Current->gl_ctx->VB->Color[i][2]*255.0);
         }
      }
   }
//   DD_RELEASEDC;
   ENDPROFILE(fast_rgb_points)
}



/* Return pointer to accerated points function */
extern points_func choose_points_function( GLcontext* ctx )
{
   STARTPROFILE
   if (ctx->Point.Size==1.0 && !ctx->Point.SmoothFlag && ctx->_RasterMask==0
       && !ctx->Texture.Enabled  && ctx->Visual->RGBAflag) {
   ENDPROFILE(choose_points_function)
      return fast_rgb_points;
   }
   else {
   ENDPROFILE(choose_points_function)
      return NULL;
   }
}



/* Draw a line using the color specified by Current->gl_ctx->VB->Color[pv] */
static void fast_flat_rgb_line( GLcontext* ctx, GLuint v0, GLuint v1, GLuint pv )
{
	STARTPROFILE
	int x0, y0, x1, y1;
	unsigned long pixel;
	HDC DC=DD_GETDC;
	HPEN Pen;
	HPEN Old_Pen;

	if (Current->gl_ctx->VB->MonoColor) {
	  pixel = Current->pixel;  /* use current color */
	}
	else {
	  pixel = RGB(Current->gl_ctx->VB->Color[pv][0]*255.0, Current->gl_ctx->VB->Color[pv][1]*255.0, Current->gl_ctx->VB->Color[pv][2]*255.0);
	}

	x0 =       (int) Current->gl_ctx->VB->Win[v0][0];
	y0 = FLIP( (int) Current->gl_ctx->VB->Win[v0][1] );
	x1 =       (int) Current->gl_ctx->VB->Win[v1][0];
	y1 = FLIP( (int) Current->gl_ctx->VB->Win[v1][1] );


	BEGINGDICALL

	Pen=CreatePen(PS_SOLID,1,pixel);
	Old_Pen=SelectObject(DC,Pen);
	MoveToEx(DC,x0,y0,NULL);
	LineTo(DC,x1,y1);
	SelectObject(DC,Old_Pen);
	DeleteObject(Pen);
	DD_RELEASEDC;

	ENDGDICALL

	ENDPROFILE(fast_flat_rgb_line)
}



/* Return pointer to accerated line function */
static line_func choose_line_function( GLcontext* ctx )
{
	STARTPROFILE
   if (ctx->Line.Width==1.0 && !ctx->Line.SmoothFlag && !ctx->Line.StippleFlag
       && ctx->Light.ShadeModel==GL_FLAT && ctx->_RasterMask==0
       && !ctx->Texture.Enabled && Current->rgb_flag) {
   ENDPROFILE(choose_line_function)
      return fast_flat_rgb_line;
   }
   else {
   ENDPROFILE(choose_line_function)
      return NULL;
   }
}


/**********************************************************************/
/*****                 Span-based pixel drawing                   *****/
/**********************************************************************/


/* Write a horizontal span of color-index pixels with a boolean mask. */
static void write_index_span( GLcontext* ctx,
							  GLuint n, GLint x, GLint y,
							  const GLuint index[],
                              const GLubyte mask[] )
{
	  STARTPROFILE
	  GLuint i;
	  PBYTE Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
	  assert(Current->rgb_flag==GL_FALSE);
	  for (i=0; i<n; i++)
		if (mask[i])
		  Mem[i]=index[i];
	   ENDPROFILE(write_index_span)
}



/*
 * Write a horizontal span of pixels with a boolean mask.  The current
 * color index is used for all pixels.
 */
static void write_monoindex_span(GLcontext* ctx,
								 GLuint n,GLint x,GLint y,
								 const GLubyte mask[])
{
	  STARTPROFILE
	  GLuint i;
	  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
	  assert(Current->rgb_flag==GL_FALSE);
	  for (i=0; i<n; i++)
		if (mask[i])
		  Mem[i]=Current->pixel;
	  ENDPROFILE(write_monoindex_span)
}

/*
	To improve the performance of this routine, frob the data into an actual scanline
	and call bitblt on the complete scan line instead of SetPixel.
*/

/* Write a horizontal span of color pixels with a boolean mask. */
static void write_color_span( GLcontext* ctx,
			  GLuint n, GLint x, GLint y,
			  const GLubyte
			  red[], const GLubyte green[],
			  const GLubyte blue[], const GLubyte alpha[],
			  const GLubyte mask[] )
{
	STARTPROFILE

	PWMC	pwc = Current;

	if (pwc->rgb_flag==GL_TRUE)
	{
		GLuint i;
		HDC DC=DD_GETDC;
		y=FLIP(y);

		if (mask) {
			for (i=0; i<n; i++)
				if (mask[i])
					wmSetPixel(pwc, y, x + i,red[i], green[i], blue[i]);
		}

		else {
			for (i=0; i<n; i++)
				wmSetPixel(pwc, y, x + i, red[i], green[i], blue[i]);
		}

		DD_RELEASEDC;

	}

  else
  {
		GLuint i;
		BYTE *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
		y=FLIP(y);
		if (mask) {
		   for (i=0; i<n; i++)
			 if (mask[i])
			   Mem[i]=GetNearestPaletteIndex(Current->hPal,RGB(red[i],green[i],blue[i]));
		}
		else {
		   for (i=0; i<n; i++)
			 Mem[i]=GetNearestPaletteIndex(Current->hPal,RGB(red[i],green[i],blue[i]));
			}
	}
   ENDPROFILE(write_color_span)

}

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_monocolor_span( GLcontext* ctx,
								  GLuint n, GLint x, GLint y,
								  const GLubyte mask[])
{
  STARTPROFILE
  GLuint i;
  HDC DC=DD_GETDC;
  PWMC	pwc = Current;

  assert(Current->rgb_flag==GL_TRUE);
  y=FLIP(y);

  if(Current->rgb_flag==GL_TRUE){
	  for (i=0; i<n; i++)
		if (mask[i])
// Trying
		wmSetPixel(pwc,y,x+i,GetRValue(Current->pixel), GetGValue(Current->pixel), GetBValue(Current->pixel));
  }
  else {
	  for (i=0; i<n; i++)
		if (mask[i])
			SetPixel(DC, y, x+i, Current->pixel);
  }

	DD_RELEASEDC;

	ENDPROFILE(write_monocolor_span)
}



/**********************************************************************/
/*****                   Array-based pixel drawing                *****/
/**********************************************************************/


/* Write an array of pixels with a boolean mask. */
static void write_index_pixels( GLcontext* ctx,
							    GLuint n, const GLint x[], const GLint y[],
								const GLuint index[], const GLubyte mask[] )
{
   STARTPROFILE
   GLuint i;
   assert(Current->rgb_flag==GL_FALSE);
   for (i=0; i<n; i++) {
      if (mask[i]) {
         BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
		   *Mem = index[i];
      }
   }
   ENDPROFILE(write_index_pixels)
}



/*
 * Write an array of pixels with a boolean mask.  The current color
 * index is used for all pixels.
 */
static void write_monoindex_pixels( GLcontext* ctx,
								    GLuint n,
									const GLint x[], const GLint y[],
                                    const GLubyte mask[] )
{
   STARTPROFILE
   GLuint i;
   assert(Current->rgb_flag==GL_FALSE);
   for (i=0; i<n; i++) {
      if (mask[i]) {
         BYTE *Mem=Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i];
			*Mem = Current->pixel;
      }
   }
   ENDPROFILE(write_monoindex_pixels)
}



/* Write an array of pixels with a boolean mask. */
static void write_color_pixels( GLcontext* ctx,
							    GLuint n, const GLint x[], const GLint y[],
								const GLubyte r[], const GLubyte g[],
                                const GLubyte b[], const GLubyte a[],
                                const GLubyte mask[] )
{
	STARTPROFILE
	GLuint i;
	PWMC	pwc = Current;
	HDC DC=DD_GETDC;
	assert(Current->rgb_flag==GL_TRUE);
	for (i=0; i<n; i++)
		if (mask[i])
			wmSetPixel(pwc, FLIP(y[i]),x[i],r[i],g[i],b[i]);
	DD_RELEASEDC;
	ENDPROFILE(write_color_pixels)
}



/*
 * Write an array of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_monocolor_pixels( GLcontext* ctx,
								    GLuint n,
									const GLint x[], const GLint y[],
                                    const GLubyte mask[] )
{
	STARTPROFILE
	GLuint i;
	PWMC	pwc = Current;
	HDC DC=DD_GETDC;
	assert(Current->rgb_flag==GL_TRUE);
	for (i=0; i<n; i++)
		if (mask[i])
			wmSetPixel(pwc, FLIP(y[i]),x[i],GetRValue(Current->pixel),
					    GetGValue(Current->pixel), GetBValue(Current->pixel));
	DD_RELEASEDC;
	ENDPROFILE(write_monocolor_pixels)
}



/**********************************************************************/
/*****            Read spans/arrays of pixels                     *****/
/**********************************************************************/


/* Read a horizontal span of color-index pixels. */
static void read_index_span( GLcontext* ctx, GLuint n, GLint x, GLint y, GLuint index[])
{
  STARTPROFILE
  GLuint i;
  BYTE *Mem=Current->ScreenMem+FLIP(y)*Current->ScanWidth+x;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++)
    index[i]=Mem[i];
  ENDPROFILE(read_index_span)

}




/* Read an array of color index pixels. */
static void read_index_pixels( GLcontext* ctx,
							   GLuint n, const GLint x[], const GLint y[],
							   GLuint indx[], const GLubyte mask[] )
{
   STARTPROFILE
   GLuint i;
  assert(Current->rgb_flag==GL_FALSE);
  for (i=0; i<n; i++) {
     if (mask[i]) {
        indx[i]=*(Current->ScreenMem+FLIP(y[i])*Current->ScanWidth+x[i]);
     }
  }
   ENDPROFILE(read_index_pixels)
}



/* Read a horizontal span of color pixels. */
static void read_color_span( GLcontext* ctx,
							 GLuint n, GLint x, GLint y,
							 GLubyte red[], GLubyte green[],
                             GLubyte blue[], GLubyte alpha[] )
{
   STARTPROFILE
  UINT i;
  COLORREF Color;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  y=FLIP(y);
  for (i=0; i<n; i++)
  {
    Color=GetPixel(DC,x+i,y);
    red[i]=GetRValue(Color);
    green[i]=GetGValue(Color);
    blue[i]=GetBValue(Color);
    alpha[i]=255;
  }
  DD_RELEASEDC;
  memset(alpha,0,n*sizeof(GLint));
   ENDPROFILE(read_color_span)
}


/* Read an array of color pixels. */
static void read_color_pixels( GLcontext* ctx,
							   GLuint n, const GLint x[], const GLint y[],
							   GLubyte red[], GLubyte green[],
                               GLubyte blue[], GLubyte alpha[],
                               const GLubyte mask[] )
{
   STARTPROFILE
  GLuint i;
  COLORREF Color;
  HDC DC=DD_GETDC;
  assert(Current->rgb_flag==GL_TRUE);
  for (i=0; i<n; i++) {
     if (mask[i]) {
        Color=GetPixel(DC,x[i],FLIP(y[i]));
        red[i]=GetRValue(Color);
        green[i]=GetGValue(Color);
        blue[i]=GetBValue(Color);
        alpha[i]=255;
     }
  }
  DD_RELEASEDC;
  memset(alpha,0,n*sizeof(GLint));
   ENDPROFILE(read_color_pixels)
}



/**********************************************************************/
/**********************************************************************/



void setup_DD_pointers( GLcontext* ctx )
{
   ctx->Driver.UpdateState = setup_DD_pointers;
   ctx->Driver.GetBufferSize = buffer_size;
   ctx->Driver.Finish = finish;
   ctx->Driver.Flush = flush;

   ctx->Driver.ClearIndex = clear_index;
   ctx->Driver.ClearColor = clear_color;
   ctx->Driver.Clear = clear;

   ctx->Driver.Index = set_index;
   ctx->Driver.Color = set_color;
   ctx->Driver.IndexMask = index_mask;
   ctx->Driver.ColorMask = color_mask;

   ctx->Driver.LogicOp = logicop;
   ctx->Driver.Dither = dither;

   ctx->Driver.SetBuffer = set_buffer;
   ctx->Driver.GetBufferSize = buffer_size;

   ctx->Driver.PointsFunc = choose_points_function(ctx);
   ctx->Driver.LineFunc = choose_line_function(ctx);
   ctx->Driver.TriangleFunc = choose_triangle_function( ctx );

   /* Pixel/span writing functions: */
   ctx->Driver.WriteColorSpan       = write_color_span;
   ctx->Driver.WriteMonocolorSpan   = write_monocolor_span;
   ctx->Driver.WriteColorPixels     = write_color_pixels;
   ctx->Driver.WriteMonocolorPixels = write_monocolor_pixels;
   ctx->Driver.WriteIndexSpan       = write_index_span;
   ctx->Driver.WriteMonoindexSpan   = write_monoindex_span;
   ctx->Driver.WriteIndexPixels     = write_index_pixels;
   ctx->Driver.WriteMonoindexPixels = write_monoindex_pixels;

   /* Pixel/span reading functions: */
   ctx->Driver.ReadIndexSpan = read_index_span;
   ctx->Driver.ReadColorSpan = read_color_span;
   ctx->Driver.ReadIndexPixels = read_index_pixels;
   ctx->Driver.ReadColorPixels = read_color_pixels;
}


/**********************************************************************/
/*****                  WMesa API Functions                       *****/
/**********************************************************************/



#define PAL_SIZE 256
static void GetPalette(HPALETTE Pal,RGBQUAD *aRGB)
{
   STARTPROFILE
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
  	  ENDPROFILE(GetPalette)
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
//    c->pixel = 1;
    c->db_flag = db_flag =GL_TRUE; // WinG requires double buffering
	printf("Single buffer is not supported in color index mode, setting to double buffer.\n");
  }
  else
  {
    c->rgb_flag = GL_TRUE;
//    c->pixel = 0;
  }
  GetClientRect(c->Window,&CR);
  c->width=CR.right;
  c->height=CR.bottom;
  if (db_flag)
  {
    c->db_flag = 1;
	/* Double buffered */
#ifndef DDRAW
//	if (c->rgb_flag==GL_TRUE && c->dither_flag != GL_TRUE )
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


  c->gl_visual = gl_create_visual(rgb_flag,
								  GL_FALSE,	/* software alpha */
                                  db_flag,	/* db_flag */
                                  16,		/* depth_bits */
                                  8,		/* stencil_bits */
                                  8,		/* accum_bits */
                                  8,
                                  255.0, 255.0, 255.0, 255.0,
								  8,8,8,8 );

	if (!c->gl_visual) {
         return NULL;
      }

  /* allocate a new Mesa context */
  c->gl_ctx = gl_create_context( c->gl_visual, NULL,c);

  if (!c->gl_ctx) {
         gl_destroy_visual( c->gl_visual );
         free(c);
         return NULL;
      }

      c->gl_buffer = gl_create_framebuffer( c->gl_visual );
      if (!c->gl_buffer) {
         gl_destroy_visual( c->gl_visual );
         gl_destroy_context( c->gl_ctx );
         free(c);
         return NULL;
      }
//  setup_DD_pointers(c->gl_ctx);

  return c;
}

void WMesaDestroyContext( void )
{
	WMesaContext c = Current;
	ReleaseDC(c->Window,c->hDC);
	WC = c;
	if(c->hPalHalfTone != NULL)
		DeleteObject(c->hPalHalfTone);
    gl_destroy_visual( c->gl_visual );
    gl_destroy_framebuffer( c->gl_buffer );
	gl_destroy_context( c->gl_ctx );

	if (c->db_flag)
#ifdef DDRAW
		DDFree(c);
#else
		wmDeleteBackingStore(c);
#endif
	free( (void *) c );
//Following code is added to enable parallel render
// Parallel render only work in double buffer mode
#if !defined(NO_PARALLEL)
	if(parallelMachine)
		PRDestroyRenderBuffer();
#endif
// End modification
}



void /*APIENTRY*/ WMesaMakeCurrent( WMesaContext c )
{
	if(!c){
		Current = c;
		return;
	}

	//
	// A little optimization
	// If it already is current,
	// don't set it again
	//
	if(Current == c)
		return;

	//gl_set_context( c->gl_ctx );
	gl_make_current(c->gl_ctx, c->gl_buffer);
	Current = c;
	setup_DD_pointers(c->gl_ctx);
	if (Current->gl_ctx->Viewport.Width==0) {
	  /* initialize viewport to window size */
	  _mesa_set_viewport( Current->gl_ctx,
		           0, 0, Current->width, Current->height );
	}
	if ((c->cColorBits <= 8 ) && (c->rgb_flag == GL_TRUE)){
			WMesaPaletteChange(c->hPalHalfTone);
	}
}



void /*APIENTRY*/ WMesaSwapBuffers( void )
{
  HDC DC = Current->hDC;
  if (Current->db_flag)
	wmFlush(Current);
}



void /*APIENTRY*/ WMesaPaletteChange(HPALETTE Pal)
{
  int vRet;
  LPPALETTEENTRY pPal;
  if (Current && (Current->rgb_flag==GL_FALSE || Current->dither_flag == GL_TRUE))
  {
	pPal = (PALETTEENTRY *)malloc( 256 * sizeof(PALETTEENTRY));
	Current->hPal=Pal;
//	GetPaletteEntries( Pal, 0, 256, pPal );
	GetPalette( Pal, pPal );
#ifdef DDRAW
	Current->lpDD->lpVtbl->CreatePalette(Current->lpDD,DDPCAPS_8BIT,
		pPal, &(Current->lpDDPal), NULL);
	if (Current->lpDDPal)
        Current->lpDDSPrimary->lpVtbl->SetPalette(Current->lpDDSPrimary,Current->lpDDPal);
#else
    vRet = SetDIBColorTable(Current->dib.hDC,0,256,pPal);
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

void /*WINAPI*/ wmCreatePalette( PWMC pwdc )
{
    /* Create a compressed and re-expanded 3:3:2 palette */
  	int            i;
	LOGPALETTE     *pPal;
    BYTE           rb, rs, gb, gs, bb, bs;

    pwdc->nColors = 0x100;

	pPal = (PLOGPALETTE)malloc(sizeof(LOGPALETTE) + pwdc->nColors * sizeof(PALETTEENTRY));
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

void /*WINAPI*/ wmSetPixel(PWMC pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b)
{
	if(Current->db_flag){
		LPBYTE	lpb = pwc->pbPixels;
		LPDWORD	lpdw;
		LPWORD	lpw;
		UINT	nBypp = pwc->cColorBits / 8;
		UINT	nOffset = iPixel % nBypp;

		// Move the pixel buffer pointer to the scanline that we
		// want to access

//		pwc->dib.fFlushed = FALSE;

		lpb += pwc->ScanWidth * iScanLine;
		// Now move to the desired pixel
		lpb += iPixel * nBypp;
		lpb = PIXELADDR(iPixel, iScanLine);
		lpdw = (LPDWORD)lpb;
		lpw = (LPWORD)lpb;

		if(nBypp == 1){
			if(pwc->dither_flag)
				*lpb = DITHER_RGB_2_8BIT(r,g,b,iScanLine,iPixel);
			else
				*lpb = BGR8(r,g,b);
		}
		else if(nBypp == 2)
			*lpw = BGR16(r,g,b);
		else if (nBypp == 3){
			*lpdw = BGR24(r,g,b);
		}
		else if (nBypp == 4)
			*lpdw = BGR32(r,g,b);
	}
	else{
		HDC DC = DD_GETDC;
		SetPixel(DC, iPixel, iScanLine, RGB(r,g,b));
		DD_RELEASEDC;
	}
}

void /*WINAPI*/ wmCreateDIBSection(
	HDC	 hDC,
    PWMC pwc,	// handle of device context
    CONST BITMAPINFO *pbmi,	// address of structure containing bitmap size, format, and color data
    UINT iUsage	// color data type indicator: RGB values or palette indices
)
{
	DWORD	dwSize = 0;
	DWORD	dwScanWidth;
	UINT	nBypp = pwc->cColorBits / 8;
	HDC		hic;

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

//	pwc->pbPixels = pwc->addrOffScreen = ((LPBYTE)pwc->dib.base) + sizeof(BITMAPINFO);

//	pwc->dib.hDC = CreateCompatibleDC(hDC);

	CopyMemory(pwc->dib.base, pbmi, sizeof(BITMAPINFO));

	hic = CreateIC("display", NULL, NULL, NULL);
	pwc->dib.hDC = CreateCompatibleDC(hic);


/*	pwc->hbmDIB = CreateDIBitmap(hic,
						 &(pwc->bmi.bmiHeader),
						 CBM_INIT,
						 pwc->pbPixels,
						 &(pwc->bmi),
						 DIB_RGB_COLORS);
*/
  pwc->hbmDIB = CreateDIBSection(hic,
						&(pwc->bmi),
						(iUsage ? DIB_PAL_COLORS : DIB_RGB_COLORS),
						&(pwc->pbPixels),
						pwc->dib.hFileMap,
						0);
  /*
	pwc->hbmDIB = CreateDIBSection(hic,
						&(pwc->bmi),
						DIB_RGB_COLORS,
						&(pwc->pbPixels),
						pwc->dib.hFileMap,
						0);
	*/
	pwc->ScreenMem = pwc->addrOffScreen = pwc->pbPixels;
	pwc->hOldBitmap = SelectObject(pwc->dib.hDC, pwc->hbmDIB);

	DeleteDC(hic);

	return;

}

//
// Blit memory DC to screen DC
//
BOOL /*WINAPI*/ wmFlush(PWMC pwc)
{
	BOOL	bRet = 0;
	DWORD	dwErr = 0;
	HRESULT             ddrval;

    // Now search through the torus frames and mark used colors
	if(pwc->db_flag){
#ifdef DDRAW
		if (pwc->lpDDSOffScreen == NULL)
		if(DDCreateOffScreen(pwc) == GL_FALSE)
			return;

		pwc->lpDDSOffScreen->lpVtbl->Unlock(pwc->lpDDSOffScreen, NULL);

		while( 1 )
		{
			ddrval = pwc->lpDDSPrimary->lpVtbl->Blt( pwc->lpDDSPrimary,
					&(pwc->rectSurface), pwc->lpDDSOffScreen, &(pwc->rectOffScreen), 0, NULL );

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
						NULL, &(pwc->ddsd), 0, NULL) == DDERR_WASSTILLDRAWING)
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


// The following code is added by Li Wei to enable stereo display

#if !defined(NO_STEREO)

void WMesaShowStereo(GLuint list)
{

	GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	GLfloat cm[16];
	GLint matrix_mode;
	// Must use double Buffer
	if( ! Current-> db_flag )
		return;


    glGetIntegerv(GL_MATRIX_MODE,&matrix_mode);

//	glPushMatrix();  //****
	WMesaViewport(Current->gl_ctx,0,Current->height/2,Current->width,Current->height/2);
//	Current->gl_ctx->NewState = 0;

	//	glViewport(0,0,Current->width,Current->height/2);
	if(matrix_mode!=GL_MODELVIEW)
        glMatrixMode(GL_MODELVIEW);

	glGetFloatv(GL_MODELVIEW_MATRIX,cm);
	glLoadIdentity();
	gluLookAt(viewDistance/2,0.0,0.0 ,
			 viewDistance/2,0.0,-1.0,
			 0.0,1.0,0.0 );
//	glTranslatef(viewDistance/2.0,0.,0.);
	glMultMatrixf( cm );

	Current->ScreenMem = Current->pbPixels = Current->addrOffScreen;
	//glPushMatrix();
	glCallList( list );
	//glPopMatrix();

    glGetFloatv(GL_MODELVIEW_MATRIX,cm);
	glLoadIdentity();
	gluLookAt(-viewDistance/2,0.0,0.0 ,
			 -viewDistance/2,0.0,-1.0,
			 0.0,1.0,0.0 );
//	glTranslatef(-viewDistance/2.0,0.,0.);
	glMultMatrixf(cm);

	Current->ScreenMem = Current->pbPixels = Current->addrOffScreen + Current->pitch;
	glCallList(list);
	if(matrix_mode!=GL_MODELVIEW)
		glMatrixMode(matrix_mode);

//	glPopMatrix();
	glFlush();

	WMesaViewport(Current->gl_ctx,0,0,Current->width,Current->height);
//	Current->gl_ctx->NewState = 0;
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

#endif // End if NO_STEREO not defined

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
									  Current->rgb_flag? Current->pbPixels: Current->ScreenMem);
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
// Seems something wrong!!!!
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
#endif //End if NO_PARALLEL not defined

//end modification

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
    wc->ddsd.ddsCaps.dwCaps =	DDSCAPS_PRIMARYSURFACE;

    ddrval = wc->lpDD->lpVtbl->CreateSurface( wc->lpDD,&(wc->ddsd), &(wc->lpDDSPrimary), NULL );
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
	POINT	pt;
	HRESULT		ddrval;
	if(wc->lpDD == NULL)
		return FALSE;
	GetClientRect( wc->hwnd, &(wc->rectOffScreen) );
	wc->ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    wc->ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    wc->ddsd.dwHeight = wc->rectOffScreen.bottom - wc->rectOffScreen.top;
    wc->ddsd.dwWidth = wc->rectOffScreen.right - wc->rectOffScreen.left;

    ddrval = wc->lpDD->lpVtbl->CreateSurface( wc->lpDD, &(wc->ddsd), &(wc->lpDDSOffScreen), NULL );
    if( ddrval != DD_OK )
    {
		return FALSE;
    }

	while (wc->lpDDSOffScreen->lpVtbl->Lock(wc->lpDDSOffScreen,NULL, &(wc->ddsd), 0, NULL) == DDERR_WASSTILLDRAWING)
        ;
//	while ((ddrval = wc->lpDDSOffScreen->lpVtbl->Lock(wc->lpDDSOffScreen,NULL, &(wc->ddsd), DDLOCK_SURFACEMEMORYPTR , NULL)) != DD_OK)
		;
	if(wc->ddsd.lpSurface==NULL)
		return initFail(wc->hwnd, wc);

	wc->ScreenMem = wc->pbPixels = wc->addrOffScreen = (PBYTE)(wc->ddsd.lpSurface);
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
        ddrval = wc->lpDD->lpVtbl->SetCooperativeLevel( wc->lpDD, hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
    }
    else
    {
        ddrval = wc->lpDD->lpVtbl->SetCooperativeLevel( wc->lpDD, hwnd, DDSCL_NORMAL );
    }
    if( ddrval != DD_OK )
    {
        return initFail(hwnd , wc);
    }


/*	ddrval = wc->lpDD->lpVtbl->QueryInterface(wc->lpDD, IID_IDirectDraw2,
			(LPVOID *)((wc->lpDD2)));

*/
	if(ddrval != DD_OK)
        return initFail(hwnd , wc);


   //ddrval = wc->lpDD->lpVtbl->GetDisplayMode( wc->lpDD, &(wc->ddsd));
 //  wc->lpDD2->lpVtbl->GetMonitorFrequency(wc->lpDD, &dwFrequency);
    switch( wc->gMode )
    {
        case 1:  ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 640, 480, displayOptions.bpp); break;
        case 2:  ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 800, 600, displayOptions.bpp); break;
        case 3:  ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1024, 768, displayOptions.bpp); break;
		case 4:  ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1152, 864, displayOptions.bpp); break;
        case 5:  ddrval = wc->lpDD->lpVtbl->SetDisplayMode( wc->lpDD, 1280, 1024, displayOptions.bpp); break;
    }

    if( ddrval != DD_OK )
    {
		printf("Can't modify display mode, current mode used\n");
//        return initFail(hwnd , wc);
    }
//ddrval = wc->lpDD->lpVtbl->GetDisplayMode( wc->lpDD, &(wc->ddsd));
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
	POINT	pt;
	if (Current != NULL){
		GetClientRect( wc->hwnd, &(wc->rectSurface) );
		pt.x = pt.y = 0;
		ClientToScreen( wc->hwnd, &pt );
		OffsetRect(&(wc->rectSurface), pt.x, pt.y);
	}
}

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
#define INTERP_RGB 1
#define PIXEL_ADDRESS(X,Y) PIXELADDR4(X,Y)
#define PIXEL_TYPE GLuint
//#define BYTES_PER_ROW (wmesa->xm_buffer->backimage->bytes_per_line)
#define BYTES_PER_ROW (wmesa->ScanWidth)
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
         pRow[i] = PACK_5R6G5B( FixedToInt(ffr), FixedToInt(ffg),	\
				 FixedToInt(ffb) );			\
         zRow[i] = z;							\
      }									\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
      ffz += fdzdx;							\
   }									\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_8B8G8R( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint i, len = RIGHT-LEFT;						\
   for (i=0;i<len;i++) {						\
      GLdepth z = FixedToDepth(ffz);					\
      if (z < zRow[i]) {						\
	 pRow[i] = p;							\
         zRow[i] = z;							\
      }									\
      ffz += fdzdx;							\
   }									\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_8R8G8B( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 pRow[i] = p;					\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_5R6G5B( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint i, len = RIGHT-LEFT;				\
   for (i=0;i<len;i++) {				\
      GLdepth z = FixedToDepth(ffz);			\
      if (z < zRow[i]) {				\
	 pRow[i] = p;					\
         zRow[i] = z;					\
      }							\
      ffz += fdzdx;					\
   }							\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = PACK_8B8G8R( FixedToInt(ffr), FixedToInt(ffg),		\
				FixedToInt(ffb) );			\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = PACK_8R8G8B( FixedToInt(ffr), FixedToInt(ffg),		\
				FixedToInt(ffb) );			\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = PACK_5R6G5B( FixedToInt(ffr), FixedToInt(ffg),		\
				FixedToInt(ffb) );			\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;			\
   }									\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_8B8G8R( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = p;					\
   }							\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_8R8G8B( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = p;					\
   }							\
}
#include "tritemp.h"
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
#define SETUP_CODE					\
   unsigned long p = PACK_5R6G5B( VB->Color[pv][0],	\
		 VB->Color[pv][1], VB->Color[pv][2] );
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = p;					\
   }							\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )								\
{																	\
   GLint i, len = RIGHT-LEFT;										\
   for (i=0;i<len;i++) {											\
      GLdepth z = FixedToDepth(ffz);								\
      if (z < zRow[i]) {											\
         pRow[i] = FixedToInt(ffi);									\
         zRow[i] = z;												\
      }																\
      ffi += fdidx;													\
      ffz += fdzdx;													\
   }																\
}

#include "tritemp.h"
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
#define SETUP_CODE													\
   GLuint index = VB->Index[pv];									\
   if (!VB->MonoColor) {											\
      /* set the color index */										\
      (*ctx->Driver.Index)( ctx, index );							\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )								\
{																	\
   GLint i, len = RIGHT-LEFT;										\
   for (i=0;i<len;i++) {											\
      GLdepth z = FixedToDepth(ffz);								\
      if (z < zRow[i]) {											\
		 pRow[i] = index;											\
         zRow[i] = z;												\
      }																\
      ffz += fdzdx;													\
   }																\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )					\
{									\
   GLint xx;								\
   PIXEL_TYPE *pixel = pRow;						\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {				\
      *pixel = FixedToInt(ffi);			\
      ffi += fdidx;			\
   }									\
}
#include "tritemp.h"
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
#define SETUP_CODE													\
   GLuint index = VB->Index[pv];									\
   if (!VB->MonoColor) {											\
      /* set the color index */										\
      (*ctx->Driver.Index)( ctx, index );							\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )			\
{							\
   GLint xx;						\
   PIXEL_TYPE *pixel = pRow;				\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {		\
      *pixel = index;					\
   }							\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )									\
{																		\
   GLint i, xx = LEFT, yy = FLIP(Y), len = RIGHT-LEFT;					\
   for (i=0;i<len;i++,xx++) {											\
      GLdepth z = FixedToDepth(ffz);									\
      if (z < zRow[i]) {												\
		 DITHER_RGB_TO_8BIT( FixedToInt(ffr), FixedToInt(ffg),			\
								FixedToInt(ffb), xx, yy);				\
		 pRow[i] = pixelDithered;										\
         zRow[i] = z;													\
      }																	\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;						\
      ffz += fdzdx;														\
   }																	\
}
#include "tritemp.h"
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

#define INNER_LOOP( LEFT, RIGHT, Y )									\
{																		\
   GLint i, xx = LEFT, yy = FLIP(Y), len = RIGHT-LEFT;					\
   for (i=0;i<len;i++,xx++) {											\
      GLdepth z = FixedToDepth(ffz);									\
      if (z < zRow[i]) {												\
		DITHER_RGB_TO_8BIT( VB->Color[pv][0],							\
			 VB->Color[pv][1], VB->Color[pv][2], xx, yy);				\
		pRow[i] = pixelDithered;										\
         zRow[i] = z;													\
      }																	\
      ffz += fdzdx;														\
   }																	\
}
#include "tritemp.h"
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
#define INNER_LOOP( LEFT, RIGHT, Y )									\
{																		\
   GLint xx, yy = FLIP(Y);												\
   PIXEL_TYPE *pixel = pRow;											\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {								\
	  DITHER_RGB_TO_8BIT( VB->Color[pv][0],	VB->Color[pv][1], VB->Color[pv][2], xx, yy);\
	  *pixel = pixelDithered;											\
	  ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;						\
   }																	\
}
#include "tritemp.h"
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

#define INNER_LOOP( LEFT, RIGHT, Y )									\
{																		\
   GLint xx, yy = FLIP(Y);												\
   PIXEL_TYPE *pixel = pRow;											\
   for (xx=LEFT;xx<RIGHT;xx++,pixel++) {								\
      DITHER_RGB_TO_8BIT( VB->Color[pv][0],								\
			 VB->Color[pv][1], VB->Color[pv][2], xx, yy);				\
	  *pixel = pixelDithered;											\
   }																	\
}
#include "tritemp.h"
}




static triangle_func choose_triangle_function( GLcontext *ctx )
{
   WMesaContext wmesa = (WMesaContext) ctx->DriverCtx;
   int depth = wmesa->cColorBits;

   if (ctx->Polygon.SmoothFlag)     return NULL;
   if (ctx->Texture.Enabled)        return NULL;
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
}

/*
 * Define a new viewport and reallocate auxillary buffers if the size of
 * the window (color buffer) has changed.
 */
void WMesaViewport( GLcontext *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height )
{
   /* Save viewport */
   ctx->Viewport.X = x;
   ctx->Viewport.Width = width;
   ctx->Viewport.Y = y;
   ctx->Viewport.Height = height;

   /* compute scale and bias values */
   ctx->Viewport.Sx = (GLfloat) width / 2.0F;
   ctx->Viewport.Tx = ctx->Viewport.Sx + x;
   ctx->Viewport.Sy = (GLfloat) height / 2.0F;
   ctx->Viewport.Ty = ctx->Viewport.Sy + y;
}
