/* $Id: fxddspan.c,v 1.26 2003/10/09 15:12:21 dborca Exp $ */

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
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */


/* fxdd.c - 3Dfx VooDoo Mesa span and pixel functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "fxglidew.h"
#include "swrast/swrast.h"


#define writeRegionClipped(fxm,dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)



/* KW: Rearranged the args in the call to grLfbWriteRegion().
 */
#define LFB_WRITE_SPAN_MESA(dst_buffer,		\
			    dst_x,		\
			    dst_y,		\
			    src_width,		\
			    src_stride,		\
			    src_data)		\
  writeRegionClipped(fxMesa, dst_buffer,	\
		   dst_x,			\
		   dst_y,			\
		   GR_LFB_SRC_FMT_8888,		\
		   src_width,			\
		   1,				\
		   src_stride,			\
		   src_data)			\


/************************************************************************/
/*****                    Span functions                            *****/
/************************************************************************/
#define TDFXPACKCOLOR1555( r, g, b, a )					   \
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	   \
    ((a) ? 0x8000 : 0))
#define TDFXPACKCOLOR565( r, g, b )					   \
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))
#define TDFXPACKCOLOR8888( r, g, b, a )					   \
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
/************************************************************************/


#define DBG 0


#define LOCAL_VARS							\
    GLuint pitch = info.strideInBytes;					\
    GLuint height = fxMesa->height;					\
    char *buf = (char *)((char *)info.lfbPtr + 0 /* x, y offset */);	\
    GLuint p;								\
    (void) buf; (void) p;

#define CLIPPIXEL( _x, _y )	( _x >= minx && _x < maxx &&		\
				  _y >= miny && _y < maxy )

#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
    if ( _y < miny || _y >= maxy ) {					\
	_n1 = 0, _x1 = x;						\
    } else {								\
	_n1 = _n;							\
	_x1 = _x;							\
	if ( _x1 < minx ) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx;\
	if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + n1 - maxx);		\
    }

#define Y_FLIP(_y)		(height - _y - 1)

#define HW_WRITE_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_WRITE_ONLY,					\
                   fxMesa->currentFB, LFB_MODE,				\
		   GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_WRITE_UNLOCK()						\
	grLfbUnlock( GR_LFB_WRITE_ONLY, fxMesa->currentFB );		\
    }

#define HW_READ_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_READ_ONLY, fxMesa->currentFB,		\
                    LFB_MODE, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_READ_UNLOCK()						\
	grLfbUnlock( GR_LFB_READ_ONLY, fxMesa->currentFB );		\
    }

#define HW_WRITE_CLIPLOOP()						\
    do {								\
	int _nc = 1; /* numcliprects */					\
	while (_nc--) {							\
	    const int minx = fxMesa->clipMinX;				\
	    const int miny = fxMesa->clipMinY;				\
	    const int maxx = fxMesa->clipMaxX;				\
	    const int maxy = fxMesa->clipMaxY;

#define HW_READ_CLIPLOOP()						\
    do {								\
	int _nc = 1; /* numcliprects */					\
	while (_nc--) {							\
	    const int minx = fxMesa->clipMinX;				\
	    const int miny = fxMesa->clipMinY;				\
	    const int maxx = fxMesa->clipMaxX;				\
	    const int maxy = fxMesa->clipMaxY;

#define HW_ENDCLIPLOOP()						\
	}								\
    } while (0)


/* 16 bit, ARGB1555 color spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_1555

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 2

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR1555( color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR1555( r, g, b, a )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
	rgba[0] = FX_rgb_scale_5[(p >> 10) & 0x1F];			\
	rgba[1] = FX_rgb_scale_5[(p >> 5)  & 0x1F];			\
	rgba[2] = FX_rgb_scale_5[ p        & 0x1F];			\
	rgba[3] = (p & 0x8000) ? 255 : 0;				\
    } while (0)

#define TAG(x) tdfx##x##_ARGB1555
#include "../dri/common/spantmp.h"


/* 16 bit, RGB565 color spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_565

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 2

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR565( color[RCOMP], color[GCOMP], color[BCOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR565( r, g, b )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
	rgba[0] = FX_rgb_scale_5[(p >> 11) & 0x1F];			\
	rgba[1] = FX_rgb_scale_6[(p >> 5)  & 0x3F];			\
	rgba[2] = FX_rgb_scale_5[ p        & 0x1F];			\
	rgba[3] = 0xff;							\
    } while (0)

#define TAG(x) tdfx##x##_RGB565
#include "../dri/common/spantmp.h"


/* 32 bit, ARGB8888 color spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_8888

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 4

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR8888( color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR8888( r, g, b, a )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLuint p = *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
        rgba[0] = (p >> 16) & 0xff;					\
        rgba[1] = (p >>  8) & 0xff;					\
        rgba[2] = (p >>  0) & 0xff;					\
        rgba[3] = (p >> 24) & 0xff;					\
    } while (0)

#define TAG(x) tdfx##x##_ARGB8888
#include "../dri/common/spantmp.h"


/************************************************************************/
/*****                    Span functions (optimized)                *****/
/************************************************************************/

/*
 * Read a span of 15-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_ARGB1555 (const GLcontext * ctx,
                                     GLuint n,
                                     GLint x, GLint y,
                                     GLubyte rgba[][4])
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 GrLfbInfo_t info;
 info.size = sizeof(GrLfbInfo_t);
 if (grLfbLock(GR_LFB_READ_ONLY, fxMesa->currentFB,
               GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
    const GLint winX = 0;
    const GLint winY = fxMesa->height - 1;
    const GLushort *data16 = (const GLushort *)((const GLubyte *)info.lfbPtr +
	                                        (winY - y) * info.strideInBytes +
                                                (winX + x) * 2);
    const GLuint *data32 = (const GLuint *) data16;
    GLuint i, j;
    GLuint extraPixel = (n & 1);
    n -= extraPixel;

    for (i = j = 0; i < n; i += 2, j++) {
	GLuint pixel = data32[j];
	rgba[i][0] = FX_rgb_scale_5[(pixel >> 10) & 0x1F];
	rgba[i][1] = FX_rgb_scale_5[(pixel >> 5)  & 0x1F];
	rgba[i][2] = FX_rgb_scale_5[ pixel        & 0x1F];
	rgba[i][3] = (pixel & 0x8000) ? 255 : 0;
	rgba[i+1][0] = FX_rgb_scale_5[(pixel >> 26) & 0x1F];
	rgba[i+1][1] = FX_rgb_scale_5[(pixel >> 21) & 0x1F];
	rgba[i+1][2] = FX_rgb_scale_5[(pixel >> 16) & 0x1F];
	rgba[i+1][3] = (pixel & 0x80000000) ? 255 : 0;
    }
    if (extraPixel) {
       GLushort pixel = data16[n];
       rgba[n][0] = FX_rgb_scale_5[(pixel >> 10) & 0x1F];
       rgba[n][1] = FX_rgb_scale_5[(pixel >> 5)  & 0x1F];
       rgba[n][2] = FX_rgb_scale_5[ pixel        & 0x1F];
       rgba[n][3] = (pixel & 0x8000) ? 255 : 0;
    }

    grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
 }
}

/*
 * Read a span of 16-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_RGB565 (const GLcontext * ctx,
                                   GLuint n,
                                   GLint x, GLint y,
                                   GLubyte rgba[][4])
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 GrLfbInfo_t info;
 info.size = sizeof(GrLfbInfo_t);
 if (grLfbLock(GR_LFB_READ_ONLY, fxMesa->currentFB,
               GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
    const GLint winX = 0;
    const GLint winY = fxMesa->height - 1;
    const GLushort *data16 = (const GLushort *)((const GLubyte *)info.lfbPtr +
	                                        (winY - y) * info.strideInBytes +
                                                (winX + x) * 2);
    const GLuint *data32 = (const GLuint *) data16;
    GLuint i, j;
    GLuint extraPixel = (n & 1);
    n -= extraPixel;

    for (i = j = 0; i < n; i += 2, j++) {
        GLuint pixel = data32[j];
#if 0
        GLuint pixel0 = pixel & 0xffff;
        GLuint pixel1 = pixel >> 16;
        rgba[i][RCOMP] = FX_PixelToR[pixel0];
        rgba[i][GCOMP] = FX_PixelToG[pixel0];
        rgba[i][BCOMP] = FX_PixelToB[pixel0];
        rgba[i][ACOMP] = 255;
        rgba[i + 1][RCOMP] = FX_PixelToR[pixel1];
        rgba[i + 1][GCOMP] = FX_PixelToG[pixel1];
        rgba[i + 1][BCOMP] = FX_PixelToB[pixel1];
        rgba[i + 1][ACOMP] = 255;
#else
	rgba[i][0] = FX_rgb_scale_5[(pixel >> 11) & 0x1F];
	rgba[i][1] = FX_rgb_scale_6[(pixel >> 5)  & 0x3F];
	rgba[i][2] = FX_rgb_scale_5[ pixel        & 0x1F];
	rgba[i][3] = 255;
	rgba[i+1][0] = FX_rgb_scale_5[(pixel >> 27) & 0x1F];
	rgba[i+1][1] = FX_rgb_scale_6[(pixel >> 21) & 0x3F];
	rgba[i+1][2] = FX_rgb_scale_5[(pixel >> 16) & 0x1F];
	rgba[i+1][3] = 255;
#endif
    }
    if (extraPixel) {
       GLushort pixel = data16[n];
#if 0
       rgba[n][RCOMP] = FX_PixelToR[pixel];
       rgba[n][GCOMP] = FX_PixelToG[pixel];
       rgba[n][BCOMP] = FX_PixelToB[pixel];
       rgba[n][ACOMP] = 255;
#else
       rgba[n][0] = FX_rgb_scale_5[(pixel >> 11) & 0x1F];
       rgba[n][1] = FX_rgb_scale_6[(pixel >> 5)  & 0x3F];
       rgba[n][2] = FX_rgb_scale_5[ pixel        & 0x1F];
       rgba[n][3] = 255;
#endif
    }

    grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
 }
}

/*
 * Read a span of 32-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_ARGB8888 (const GLcontext * ctx,
                                     GLuint n,
                                     GLint x, GLint y,
                                     GLubyte rgba[][4])
{
 /* Hack alert: WRONG! */
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 grLfbReadRegion(fxMesa->currentFB, x, fxMesa->height - 1 - y, n, 1, n * 4, rgba);
}


/************************************************************************/
/*****                    Depth functions                           *****/
/************************************************************************/

void
fxDDWriteDepthSpan(GLcontext * ctx,
		   GLuint n, GLint x, GLint y, const GLdepth depth[],
		   const GLubyte mask[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }


   if (mask) {
      GLint i;
      for (i = 0; i < n; i++) {
	 if (mask[i]) {
	    GLshort d = depth[i];
	    writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x + i, bottom - y,
			       GR_LFB_SRC_FMT_ZA16, 1, 1, 0, (void *) &d);
	 }
      }
   }
   else {
      GLushort depth16[MAX_WIDTH];
      GLint i;
      for (i = 0; i < n; i++) {
	 depth16[i] = depth[i];
      }
      writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x, bottom - y,
			 GR_LFB_SRC_FMT_ZA16, n, 1, 0, (void *) depth16);
   }
}


void
fxDDWriteDepth32Span(GLcontext * ctx,
		   GLuint n, GLint x, GLint y, const GLdepth depth[],
		   const GLubyte mask[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }


   if (mask) {
      for (i = 0; i < n; i++) {
	 if (mask[i]) {
            GLuint d = depth[i] << 8;
	    writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x + i, bottom - y,
			       GR_LFBWRITEMODE_Z32, 1, 1, 0, (void *) &d);
	 }
      }
   }
   else {
      GLuint depth32[MAX_WIDTH];
      for (i = 0; i < n; i++) {
          depth32[i] = depth[i] << 8;
      }
      writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x, bottom - y,
			 GR_LFBWRITEMODE_Z32, n, 1, 0, (void *) depth32);
   }
}


void
fxDDReadDepthSpan(GLcontext * ctx,
		  GLuint n, GLint x, GLint y, GLdepth depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLushort depth16[MAX_WIDTH];
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth16);
   for (i = 0; i < n; i++) {
      depth[i] = depth16[i];
   }
}


void
fxDDReadDepth32Span(GLcontext * ctx,
		  GLuint n, GLint x, GLint y, GLdepth depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth);
}



void
fxDDWriteDepthPixels(GLcontext * ctx,
		     GLuint n, const GLint x[], const GLint y[],
		     const GLdepth depth[], const GLubyte mask[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   for (i = 0; i < n; i++) {
      if (mask[i]) {
	 int xpos = x[i];
	 int ypos = bottom - y[i];
	 GLushort d = depth[i];
	 writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, xpos, ypos,
			    GR_LFB_SRC_FMT_ZA16, 1, 1, 0, (void *) &d);
      }
   }
}


void
fxDDWriteDepth32Pixels(GLcontext * ctx,
		     GLuint n, const GLint x[], const GLint y[],
		     const GLdepth depth[], const GLubyte mask[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   for (i = 0; i < n; i++) {
      if (mask[i]) {
	 int xpos = x[i];
	 int ypos = bottom - y[i];
         GLuint d = depth[i] << 8;
	 writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, xpos, ypos,
			    GR_LFBWRITEMODE_Z32, 1, 1, 0, (void *) &d);
      }
   }
}


void
fxDDReadDepthPixels(GLcontext * ctx, GLuint n,
		    const GLint x[], const GLint y[], GLdepth depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   for (i = 0; i < n; i++) {
      int xpos = x[i];
      int ypos = bottom - y[i];
      GLushort d;
      grLfbReadRegion(GR_BUFFER_AUXBUFFER, xpos, ypos, 1, 1, 0, &d);
      depth[i] = d;
   }
}


void
fxDDReadDepth32Pixels(GLcontext * ctx, GLuint n,
		    const GLint x[], const GLint y[], GLdepth depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(...)\n", __FUNCTION__);
   }

   for (i = 0; i < n; i++) {
      int xpos = x[i];
      int ypos = bottom - y[i];
      grLfbReadRegion(GR_BUFFER_AUXBUFFER, xpos, ypos, 1, 1, 0, &depth[i]);
   }
}



/* Set the buffer used for reading */
/* XXX support for separate read/draw buffers hasn't been tested */
static void
fxDDSetBuffer(GLcontext * ctx, GLframebuffer * buffer, GLuint bufferBit)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   (void) buffer;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(%x)\n", __FUNCTION__, (int)bufferBit);
   }

   if (bufferBit == FRONT_LEFT_BIT) {
      fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
      grRenderBuffer(fxMesa->currentFB);
   }
   else if (bufferBit == BACK_LEFT_BIT) {
      fxMesa->currentFB = GR_BUFFER_BACKBUFFER;
      grRenderBuffer(fxMesa->currentFB);
   }
}


/************************************************************************/



void
fxSetupDDSpanPointers(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   swdd->SetBuffer = fxDDSetBuffer;

   switch (fxMesa->colDepth) {
          case 15:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_ARGB1555;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_ARGB1555;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_ARGB1555;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_ARGB1555;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_ARGB1555;
               swdd->ReadRGBASpan = /*td*/fxReadRGBASpan_ARGB1555;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_ARGB1555;

               swdd->WriteDepthSpan = fxDDWriteDepthSpan;
               swdd->WriteDepthPixels = fxDDWriteDepthPixels;
               swdd->ReadDepthSpan = fxDDReadDepthSpan;
               swdd->ReadDepthPixels = fxDDReadDepthPixels;
               break;
          case 16:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_RGB565;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_RGB565;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_RGB565;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_RGB565;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_RGB565;
               swdd->ReadRGBASpan = /*td*/fxReadRGBASpan_RGB565;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_RGB565;
               
               swdd->WriteDepthSpan = fxDDWriteDepthSpan;
               swdd->WriteDepthPixels = fxDDWriteDepthPixels;
               swdd->ReadDepthSpan = fxDDReadDepthSpan;
               swdd->ReadDepthPixels = fxDDReadDepthPixels;
               break;
          case 32:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_ARGB8888;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_ARGB8888;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_ARGB8888;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_ARGB8888;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_ARGB8888;
               swdd->ReadRGBASpan = tdfxReadRGBASpan_ARGB8888;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_ARGB8888;

               swdd->WriteDepthSpan = fxDDWriteDepth32Span;
               swdd->WriteDepthPixels = fxDDWriteDepth32Pixels;
               swdd->ReadDepthSpan = fxDDReadDepth32Span;
               swdd->ReadDepthPixels = fxDDReadDepth32Pixels;
               break;
   }

#if 0
   if ( fxMesa->haveHwStencil ) {
      swdd->WriteStencilSpan	= write_stencil_span;
      swdd->ReadStencilSpan	= read_stencil_span;
      swdd->WriteStencilPixels	= write_stencil_pixels;
      swdd->ReadStencilPixels	= read_stencil_pixels;
   }

   swdd->WriteDepthSpan		= tdfxDDWriteDepthSpan;
   swdd->WriteDepthPixels	= tdfxDDWriteDepthPixels;
   swdd->ReadDepthSpan		= tdfxDDReadDepthSpan;
   swdd->ReadDepthPixels	= tdfxDDReadDepthPixels;

   swdd->WriteCI8Span		= NULL;
   swdd->WriteCI32Span		= NULL;
   swdd->WriteMonoCISpan	= NULL;
   swdd->WriteCI32Pixels	= NULL;
   swdd->WriteMonoCIPixels	= NULL;
   swdd->ReadCI32Span		= NULL;
   swdd->ReadCI32Pixels		= NULL;

   swdd->SpanRenderStart          = tdfxSpanRenderStart; /* BEGIN_BOARD_LOCK */
   swdd->SpanRenderFinish         = tdfxSpanRenderFinish; /* END_BOARD_LOCK */
#endif
}


#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_span(void);
int
gl_fx_dummy_function_span(void)
{
   return 0;
}

#endif /* FX */
