/*
	WMesa_stereo.c
*/
// Stereo display feature added by Li Wei
// Updated 1996/10/06	11:16:15 CST
// Paralell render feature added by Li Wei
// liwei@aiar.xjtu.edu.cn
// http://sun.aiar.xjtu.edu.cn

#define WMESA_STEREO_C

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wmesadef.h>

#include <GL\wmesa.h>
#include "context.h"
#include "dd.h"
#include "xform.h"
#include "vb.h"
#include "matrix.h"
#include "depth.h"

#ifdef PROFILE
	#include "profile.h"
#endif

#include <wing.h>

// Code added by Li Wei to enable stereo display  and Paralell render


/*#include "mesa_extend.h"*/

#if !defined(NO_STEREO)

	#include "gl\glu.h"
	#include "stereo.h"

	PBYTE Buffer_Stereo;

	void WMesaCreateStereoBuffer(void);

	void WMesaInterleave( GLenum aView);

	void WMesaDestroyStereoBuffer(void);

	void WMesaShowStereo(GLuint list);
#endif
#if !defined(NO_PARALLEL)
	#include "parallel.h"
#endif

/* end of added code*/

/* Bit's used for dest: */
#define FRONT_PIXMAP	1
#define BACK_PIXMAP	2
#define BACK_XIMAGE	4

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

#define DD_GETDC ((Current->db_flag) ? Current->dib.hDC : Current->hDC )
#define DD_RELEASEDC

//#define BEGINGDICALL	if(Current->rgb_flag)wmFlushBits(Current);
#define BEGINGDICALL
//#define ENDGDICALL		if(Current->rgb_flag)wmGetBits(Current);
#define ENDGDICALL

#define FLIP(Y)  (Current->height-(Y)-1)

#define STARTPROFILE
#define ENDPROFILE(PARA)

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

BOOL wmFlush(PWMC pwc);

/*
 * Useful macros:
   Modified from file osmesa.c
 */

#define PIXELADDR(X,Y)  ((GLbyte *)Current->pbPixels + (Current->height-Y)* Current->ScanWidth + (X)*nBypp)


/* Finish all pending operations and synchronize. */
static void finish(GLcontext* ctx)
{
   /* no op */
}


//
// We cache all gl draw routines until a flush is made
//
static void flush(GLcontext* ctx)
{
	STARTPROFILE
	if(Current->rgb_flag && !(Current->dib.fFlushed)&&!(Current->db_flag)){
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
static void clear_color( GLcontext* ctx, const GLchan color[4] )
{
  STARTPROFILE
  Current->clearpixel = RGB(color[0], color[1], color[2]);
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
	LPDWORD	lpdw = (LPDWORD)Current->pbPixels;
	LPWORD	lpw = (LPWORD)Current->pbPixels;
	LPBYTE	lpb = Current->pbPixels;

    STARTPROFILE

	if (all){
		x=y=0;
		width=Current->width;
		height=Current->height;
	}
	if (Current->rgb_flag==GL_TRUE){
		if(Current->db_flag==GL_TRUE){
			UINT	nBypp = Current->cColorBits / 8;
			int		i = 0;
			int		iSize;

			if(nBypp == 2){
				iSize = (Current->width * Current->height) / nBypp;

				wColor = BGR16(GetRValue(Current->clearpixel),
							   GetGValue(Current->clearpixel),
							   GetBValue(Current->clearpixel));
				dwColor = MAKELONG(wColor, wColor);
			}
			else if(nBypp == 4){
				iSize = (Current->width * Current->height);

				dwColor = BGR32(GetRValue(Current->clearpixel),
							   GetGValue(Current->clearpixel),
							   GetBValue(Current->clearpixel));
			}
			//
			// This is the 24bit case
			//
			else {

				iSize = (Current->width * Current->height) / nBypp;

				dwColor = BGR24(GetRValue(Current->clearpixel),
							   GetGValue(Current->clearpixel),
							   GetBValue(Current->clearpixel));


				while(i < iSize){
					*lpdw = dwColor;
					lpb += nBypp;
					lpdw = (LPDWORD)lpb;
					i++;
				}

	//			ENDPROFILE(clear)

				return;
			}

			while(i < iSize){
				*lpdw = dwColor;
				lpdw++;
				i++;
			}
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
	}
	else {
		int i;
		char *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
		for (i=0; i<height; i++){
			memset(Mem,Current->clearpixel,width);
			Mem+=width;
		}
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
			if (Current->rgb_flag==GL_TRUE){
				wmDeleteBackingStore(Current);
				wmCreateBackingStore(Current, Current->width, Current->height);
			}
			else{
				Current->ScanWidth=Current->width;
			    if ((Current->ScanWidth%sizeof(long))!=0)
				Current->ScanWidth+=(sizeof(long)-(Current->ScanWidth%sizeof(long)));

				Current->IndexFormat->bmiHeader.biWidth=Current->width;

				if (Current->IndexFormat->bmiHeader.biHeight<0)
					Current->IndexFormat->bmiHeader.biHeight=-(Current->height);
				else
					Current->IndexFormat->bmiHeader.biHeight=Current->height;

				Current->Compat_BM=WinGCreateBitmap(Current->dib.hDC,Current->IndexFormat,&((void *) Current->ScreenMem));

				DeleteObject(SelectObject(Current->dib.hDC,Current->Compat_BM));
			}
//Code added by Li Wei to enable stereo display
// Recreate stereo buffer when stereo_flag is TRUE while parallelFlag is FALSE
#if !defined(NO_STEREO)
			if(stereo_flag
#if !defined(NO_PARALLEL)
				&&!parallelFlag
#endif
				) {
			if(stereoBuffer == GL_TRUE)
				WMesaDestroyStereoBuffer();
			WMesaCreateStereoBuffer();
			}
#endif
//	Resize OsmesaBuffer if in Parallel mode
#if !defined(NO_PARALLEL)
			if(parallelFlag)
			PRSizeRenderBuffer(Current->width, Current->height,Current->ScanWidth,
			Current->rgb_flag == GL_TRUE ? Current->pbPixels: Current->ScreenMem);
#endif
//end modification

		}
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
         if (Current->gl_ctx->VB->ClipMask[i]==0) {
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
         if (Current->gl_ctx->VB->ClipMask[i]==0) {
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
       && !ctx->Texture._ReallyEnabled  && ctx->Visual->RGBAflag) {
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
       && !ctx->Texture._ReallyEnabled && Current->rgb_flag) {
   ENDPROFILE(choose_line_function)
      return fast_flat_rgb_line;
   }
   else {
   ENDPROFILE(choose_line_function)
      return NULL;
   }
}

/**********************************************************************/
/*****                 Optimized triangle rendering               *****/
/**********************************************************************/


/*
 * Smooth-shaded, z-less triangle, RGBA color.
 */
static void smooth_color_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                     GLuint v2, GLuint pv )
{
UINT	nBypp = Current->cColorBits / 8;
GLbyte* img;
GLushort* img16;
GLuint *img24 ,*img32;
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INNER_LOOP( LEFT, RIGHT, Y )							\
{																\
   GLint i, len = RIGHT-LEFT;									\
   img = PIXELADDR(LEFT,Y);   									\
   for (i=0;i<len;i++,img+=nBypp) {								\
      GLdepth z = FixedToDepth(ffz);							\
      if (z < zRow[i]) {										\
		 img16 = img24 = img32 = img;							\
		 if(nBypp == 2)											\
			*img16 = BGR16(	FixedToInt(ffr), FixedToInt(ffg),	\
							FixedToInt(ffb));					\
		 if(nBypp == 3)											\
			*img24 = BGR24(	FixedToInt(ffr), FixedToInt(ffg),	\
							FixedToInt(ffb));					\
   		 if(nBypp == 4)											\
			*img32 = BGR32(	FixedToInt(ffr), FixedToInt(ffg),	\
							FixedToInt(ffb));					\
         zRow[i] = z;											\
      }															\
      ffr += fdrdx;  ffg += fdgdx;  ffb += fdbdx;  ffa += fdadx;\
      ffz += fdzdx;												\
   }															\
}

	#include "tritemp.h"
 }




/*
 * Flat-shaded, z-less triangle, RGBA color.
 */
static void flat_color_z_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                   GLuint v2, GLuint pv )
{
GLbyte* img;
GLushort* img16;
GLuint *img24, *img32;
UINT	nBypp = Current->cColorBits / 8;
GLubyte r, g, b ;
GLushort pixel16 = BGR16(r,g,b);
GLuint   pixel24 = BGR24(r,g,b);
GLuint   pixel32 = BGR32(r,g,b);

#define INTERP_Z 1
#define SETUP_CODE			\
   r = VB->Color[pv][0];	\
   g = VB->Color[pv][1];	\
   b = VB->Color[pv][2];

#define INNER_LOOP( LEFT, RIGHT, Y )							\
{																\
   GLint i, len = RIGHT-LEFT;									\
   img = PIXELADDR(LEFT,Y);										\
   for (i=0;i<len;i++,img+=nBypp) {								\
      GLdepth z = FixedToDepth(ffz);							\
      if (z < zRow[i]) {										\
         img16 = img24 = img32 = img;							\
		 if(nBypp == 2)											\
			*img16 = pixel16;									\
		 if(nBypp == 3)											\
			*img24 = pixel24;									\
   		 if(nBypp == 4)											\
			*img32 = pixel32;									\
         zRow[i] = z;											\
      }															\
      ffz += fdzdx;												\
   }															\
}

#include "tritemp.h"
}



/*
 * Return pointer to an accelerated triangle function if possible.
 */
static triangle_func choose_triangle_function( GLcontext *ctx )
{
   if (ctx->Polygon.SmoothFlag)     return NULL;
   if (ctx->Polygon.StippleFlag)    return NULL;
   if (ctx->Texture._ReallyEnabled)  return NULL;

   if (ctx->_RasterMask==DEPTH_BIT
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_TRUE
       && ctx->Visual->RGBAflag) {
	if (ctx->Light.ShadeModel==GL_SMOOTH) {
         return smooth_color_z_triangle;
      }
      else {
         return flat_color_z_triangle;
      }
   }
   return NULL;
}


/* Draw a convex polygon using color Current->gl_ctx->VB->Color[pv] */
static void fast_flat_rgb_polygon( GLcontext* ctx, GLuint n, GLuint vlist[], GLuint pv )
{
   STARTPROFILE
   POINT *Pts=(POINT *) malloc(n*sizeof(POINT));
   HDC DC=DD_GETDC;
   HPEN Pen;
   HBRUSH Brush;
   HPEN Old_Pen;
   HBRUSH Old_Brush;
   GLint pixel;
   GLuint i;

   if (Current->gl_ctx->VB->MonoColor) {
      pixel = Current->pixel;  /* use current color */
   }
   else {
      pixel = RGB(Current->gl_ctx->VB->Color[pv][0]*255.0, Current->gl_ctx->VB->Color[pv][1]*255.0, Current->gl_ctx->VB->Color[pv][2]*255.0);
   }

   Pen=CreatePen(PS_SOLID,1,pixel);
   Brush=CreateSolidBrush(pixel);
   Old_Pen=SelectObject(DC,Pen);
   Old_Brush=SelectObject(DC,Brush);

   for (i=0; i<n; i++) {
      int j = vlist[i];
      Pts[i].x =       (int) Current->gl_ctx->VB->Win[j][0];
      Pts[i].y = FLIP( (int) Current->gl_ctx->VB->Win[j][1] );
   }

   BEGINGDICALL

   Polygon(DC,Pts,n);
   SelectObject(DC,Old_Pen);
   SelectObject(DC,Old_Brush);
   DeleteObject(Pen);
   DeleteObject(Brush);
   DD_RELEASEDC;
   free(Pts);

   ENDGDICALL

  ENDPROFILE(fast_flat_rgb_polygon)
}



/* Return pointer to accerated polygon function */
static polygon_func choose_polygon_function( GLcontext* ctx )
{
   STARTPROFILE
   if (!ctx->Polygon.SmoothFlag && !ctx->Polygon.StippleFlag
       && ctx->Light.ShadeModel==GL_FLAT && ctx->_RasterMask==0
       && !ctx->Texture._ReallyEnabled && Current->rgb_flag==GL_TRUE) {
      ENDPROFILE(choose_polygon_function)
      return fast_flat_rgb_polygon;
   }
   else {
      ENDPROFILE(choose_polygon_function)
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
   char *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
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
	  char *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
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
		char *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
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
         char *Mem=Current->ScreenMem+y[i]*Current->ScanWidth+x[i];
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
         char *Mem=Current->ScreenMem+y[i]*Current->ScanWidth+x[i];
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
  char *Mem=Current->ScreenMem+y*Current->ScanWidth+x;
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
        indx[i]=*(Current->ScreenMem+y[i]*Current->ScanWidth+x[i]);
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
   ctx->Driver.Finish = finish;
   ctx->Driver.Flush = flush;

   ctx->Driver.ClearIndex = clear_index;
   ctx->Driver.ClearColor = clear_color;
   ctx->Driver.Clear = clear;

   ctx->Driver.Index = set_index;
   ctx->Driver.Color = set_color;

   ctx->Driver.SetBuffer = set_buffer;
   ctx->Driver.GetBufferSize = buffer_size;

   ctx->Driver.PointsFunc = choose_points_function(ctx);
   ctx->Driver.LineFunc = choose_line_function(ctx);
   ctx->Driver.TriangleFunc = choose_triangle_function( ctx );
   //   ctx->Driver.TriangleFunc = choose_polygon_function(ctx);

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

//
// MesaGL32 is the DLL version of MesaGL for Win32
//

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


WMesaContext /*APIENTRY*/ WMesaCreateContext( HWND hWnd, HPALETTE Pal,
											 /*HDC hDC,*/ GLboolean rgb_flag,
											  GLboolean db_flag )
{
  BITMAPINFO *Rec;
  //HDC DC;
  RECT CR;
  WMesaContext c;

  c = (struct wmesa_context * ) calloc(1,sizeof(struct wmesa_context));
  if (!c)
    return NULL;

  c->Window=hWnd;
  c->hDC = GetDC(hWnd);

  if (rgb_flag==GL_FALSE)
  {
    c->rgb_flag = GL_FALSE;
    c->pixel = 1;
    db_flag=GL_TRUE; // WinG requires double buffering
    //c->gl_ctx->BufferDepth = windepth;
  }
  else
  {
    c->rgb_flag = GL_TRUE;
    c->pixel = 0;
  }
  GetClientRect(c->Window,&CR);
  c->width=CR.right;
  c->height=CR.bottom;
  if (db_flag)
  {
    c->db_flag = 1;
//	c->hDC GetDC(c->Window);
	/* Double buffered */
    if (c->rgb_flag==GL_TRUE)
    {
      //DC = c->hDC = hDC;

//		DC = c->hDC = GetDC(c->Window);
		wmCreateBackingStore(c, c->width, c->height);
//		ReleaseDC(c->Window,DC);
    }
    else
    {
      c->dib.hDC=WinGCreateDC();
      Rec=(BITMAPINFO *) malloc(sizeof(BITMAPINFO)+(PAL_SIZE-1)*sizeof(RGBQUAD));
      c->hPal=Pal;
      GetPalette(Pal,Rec->bmiColors);
      WinGRecommendDIBFormat(Rec);
      Rec->bmiHeader.biWidth=c->width;
      Rec->bmiHeader.biHeight*=c->height;
      Rec->bmiHeader.biClrUsed=PAL_SIZE;
      if (Rec->bmiHeader.biPlanes!=1 || Rec->bmiHeader.biBitCount!=8)
      {
        MessageBox(NULL,"Error.","This code presumes a 256 color, single plane, WinG Device.\n",MB_OK);
        exit(1);
      }
      c->Compat_BM=WinGCreateBitmap(c->dib.hDC,Rec,&((void *) c->ScreenMem));
      c->Old_Compat_BM=SelectObject(c->dib.hDC,c->Compat_BM);
      WinGSetDIBColorTable(c->dib.hDC,0,PAL_SIZE,Rec->bmiColors);
      c->IndexFormat=Rec;
      c->ScanWidth=c->width;
	  c->cColorBits = 8;
      if ((c->ScanWidth%sizeof(long))!=0)
        c->ScanWidth+=(sizeof(long)-(c->ScanWidth%sizeof(long)));
    }
  }
  else
  {
    /* Single Buffered */
	c->db_flag = 0;

//	wmCreateBackingStore(c, c->width, c->height);
  }



  c->gl_visual = _mesa_create_visual(rgb_flag,
                                     db_flag,	/* db_flag */
                                     GL_TRUE,   /* stereo */
                                     8, 8, 8, 8,/* rgba bits */
                                     0,         /* index bits */
                                     16,	/* depth_bits */
                                     8,		/* stencil_bits */
                                     16,16,16,16,/* accum_bits */
                                     1 );

	if (!c->gl_visual) {
         return NULL;
      }

  /* allocate a new Mesa context */
  c->gl_ctx = _mesa_create_context( c->gl_visual, NULL,c);

  if (!c->gl_ctx) {
         _mesa_destroy_visual( c->gl_visual );
         free(c);
         return NULL;
      }

      c->gl_buffer = _mesa_create_framebuffer( c->gl_visual );
      if (!c->gl_buffer) {
         _mesa_destroy_visual( c->gl_visual );
         _mesa_destroy_context( c->gl_ctx );
         free(c);
         return NULL;
      }
//  setup_DD_pointers(c->gl_ctx);

  return c;
}



void /*APIENTRY*/ WMesaDestroyContext( void )
{
	WMesaContext c = Current;
	ReleaseDC(c->Window,c->hDC);
	WC = c;

    _mesa_destroy_visual( c->gl_visual );
    _mesa_destroy_framebuffer( c->gl_buffer );
    _mesa_destroy_context( c->gl_ctx );

	if (c->db_flag){
		wmDeleteBackingStore(c);

//Code added by Li Wei to enable parallel render
#if !defined(NO_STEREO)
		if(stereoBuffer==GL_TRUE){
			WMesaDestroyStereoBuffer();
			stereoBuffer=GL_FALSE;
		}
#endif
// End modification
	}
	free( (void *) c );
//Code added by Li Wei to enable parallel render
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
	_mesa_make_current(c->gl_ctx, c->gl_buffer);
	Current = c;
	setup_DD_pointers(c->gl_ctx);
	if (Current->gl_ctx->Viewport.Width==0) {
	  /* initialize viewport to window size */
	  _mesa_set_viewport( Current->gl_ctx,
		           0, 0, Current->width, Current->height );
	}
}



void /*APIENTRY*/ WMesaSwapBuffers( void )
{
  HDC DC = Current->hDC;
  if (Current->db_flag)
  {
    if (Current->rgb_flag)
		wmFlush(Current);
    else
      WinGBitBlt(DC,0,0,Current->width,Current->height,Current->dib.hDC,0,0);
  }
}



void /*APIENTRY*/ WMesaPaletteChange(HPALETTE Pal)
{
  if (Current && Current->rgb_flag==GL_FALSE)
  {
    Current->hPal=Pal;
    GetPalette(Pal,Current->IndexFormat->bmiColors);
    WinGSetDIBColorTable(Current->dib.hDC,0,PAL_SIZE,Current->IndexFormat->bmiColors);
  }
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
    pbmi->bmiHeader.biBitCount = GetDeviceCaps(pwc->hDC, BITSPIXEL);
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

	iUsage = (pbmi->bmiHeader.biBitCount <= 8) ? DIB_PAL_COLORS : DIB_RGB_COLORS;

	pwc->cColorBits = pbmi->bmiHeader.biBitCount;
	pwc->ScanWidth = lxSize;

	wmCreateDIBSection(hdc, pwc, pbmi, iUsage);

	if ((iUsage == DIB_PAL_COLORS) && !(pwc->hGLPalette)) {
		wmCreatePalette( pwc );
		wmSetDibColors( pwc );
	}

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

BOOL GLWINAPI wmSetPixelFormat( PWMC pwdc, HDC hDC, DWORD dwFlags )
{
	return(TRUE);
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
	    bRet = SetDIBColorTable(pwc->hDC, 0, nColors, pColTab );

	if(!bRet)
		dwErr = GetLastError();

    free( pColTab );
    free( pPal );

	return(bRet);
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

		pwc->dib.fFlushed = FALSE;

		lpb += pwc->ScanWidth * iScanLine;
		// Now move to the desired pixel
		lpb += iPixel * nBypp;

		lpdw = (LPDWORD)lpb;
		lpw = (LPWORD)lpb;

		if(nBypp == 2)
			*lpw = BGR16(r,g,b);
		else if (nBypp == 3){
			*lpdw = BGR24(r,g,b);
		}
		else
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

	pwc->ScanWidth = dwScanWidth;

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

	pwc->pbPixels = ((LPBYTE)pwc->dib.base) + sizeof(BITMAPINFO);

	pwc->dib.hDC = CreateCompatibleDC(hDC);

	CopyMemory(pwc->dib.base, pbmi, sizeof(BITMAPINFO));

	hic = CreateIC("display", NULL, NULL, NULL);

/*	pwc->hbmDIB = CreateDIBitmap(hic,
						 &(pwc->bmi.bmiHeader),
						 CBM_INIT,
						 pwc->pbPixels,
						 &(pwc->bmi),
						 DIB_RGB_COLORS);
*/
  pwc->hbmDIB = CreateDIBSection(hic,
						&(pwc->bmi.bmiHeader),
						DIB_RGB_COLORS,
						&(pwc->pbPixels),
						pwc->dib.hFileMap,
						0);
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


//	wmFlushBits(pwc);

	bRet = BitBlt(pwc->hDC, 0, 0, pwc->width, pwc->height,
		   pwc->dib.hDC, 0, 0, SRCCOPY);

	if(!bRet)
		dwErr = GetLastError();

	pwc->dib.fFlushed = TRUE;

	return(TRUE);

}


// The following code is added by Li Wei to enable stereo display

#if !defined(NO_STEREO)

void WMesaCreateStereoBuffer()
{
	/* Must use double buffer and not in parallelMode */
	if (! Current->db_flag
#if !defined(NO_PARALLEL)
		|| parallelFlag
#endif
		)
		return;

	Buffer_Stereo = malloc( Current->ScanWidth * Current->height);
	ZeroMemory(Buffer_Stereo,Current->ScanWidth * Current->height);
	stereoBuffer = GL_TRUE ;
}

void WMesaDestroyStereoBuffer()
{
	/* Must use double buffer and not in parallelMode */
	if (! Current->db_flag
#if !defined(NO_PARALLEL)
		|| parallelFlag
#endif
		)
		return;
	if(stereoBuffer){
		free(Buffer_Stereo);
		stereoBuffer = GL_FALSE ;
	}
}

void WMesaInterleave(GLenum aView)
{
	int offset;
	unsigned line;
	LPBYTE dest;
	LPBYTE src;
	if(aView == FIRST)
		offset = 0;
	else offset = 1;

	dest = Buffer_Stereo + offset * Current->ScanWidth;
	if(Current->rgb_flag )
		src = Current->pbPixels + Current->ScanWidth*(Current->height/2);
	else
		src = Current->ScreenMem;

	for(line = 0; line<Current->height/2; line ++){
		CopyMemory(dest, src, Current->ScanWidth);
		dest += 2*Current->ScanWidth;
		src += Current->ScanWidth;
	}
	if(aView == SECOND)
		if(Current->rgb_flag)
			CopyMemory(Current->pbPixels, Buffer_Stereo, Current->ScanWidth*Current->height);
		else
			CopyMemory(Current->ScreenMem, Buffer_Stereo, Current->ScanWidth*Current->height);
}

void WMesaShowStereo(GLuint list)
{

	GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	GLfloat cm[16];
	// Must use double Buffer
	if( ! Current-> db_flag )
		return;

	glViewport(0,0,Current->width,Current->height/2);

	glGetFloatv(GL_MODELVIEW_MATRIX,cm);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(viewDistance/2,0.0,0.0 ,
			 viewDistance/2,0.0,-1.0,
			 0.0,1.0,0.0 );
	glMultMatrixf( cm );
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glCallList( list );
	glPopMatrix();
	glFlush();
	WMesaInterleave( FIRST );

	glGetFloatv(GL_MODELVIEW_MATRIX,cm);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(-viewDistance/2,0.0,0.0 ,
			 -viewDistance/2,0.0,-1.0,
			 0.0,1.0,0.0 );
	glMultMatrixf(cm);
	glMatrixMode(GL_MODELVIEW);
	glCallList(list);
	glFlush();
	WMesaInterleave( SECOND );
	glViewport(0,0,Current->width,Current->height);
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
			WMesaCreateStereoBuffer();
			}
	}
	else {
		stereo_flag = 0;
	if(stereoBuffer==GL_TRUE)
#if !defined(NO_PARALLEL)
		if(!parallelFlag)
#endif
			if(stereoBuffer==GL_TRUE){
				WMesaDestroyStereoBuffer();
			}
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
