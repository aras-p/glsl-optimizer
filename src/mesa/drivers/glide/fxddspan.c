/* $Id: fxddspan.c,v 1.19 2001/09/23 16:50:01 brianp Exp $ */

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
 */


/* fxdd.c - 3Dfx VooDoo Mesa span and pixel functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "swrast/swrast.h"

#ifdef _MSC_VER
#ifdef _WIN32
#pragma warning( disable : 4090 4022 )
/* 4101 : "different 'const' qualifier"
 * 4022 : "pointer mistmatch for actual parameter 'n'
 */
#endif
#endif


#if !defined(FXMESA_USE_ARGB)



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


#else /* !defined(FXMESA_USE_RGBA) */

#define writeRegionClipped(fxm,dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)


#define MESACOLOR_TO_ARGB(c) (				\
             ( ((unsigned int)(c[ACOMP]))<<24 ) |	\
             ( ((unsigned int)(c[RCOMP]))<<16 ) |	\
             ( ((unsigned int)(c[GCOMP]))<<8 )  |	\
             (  (unsigned int)(c[BCOMP])) )

inline void
LFB_WRITE_SPAN_MESA(GrBuffer_t dst_buffer,
		    FxU32 dst_x,
		    FxU32 dst_y,
		    FxU32 src_width, FxI32 src_stride, void *src_data)
{
   /* Covert to ARGB */
   GLubyte(*rgba)[4] = src_data;
   GLuint argb[MAX_WIDTH];
   int i;

   for (i = 0; i < src_width; i++) {
      argb[i] = MESACOLOR_TO_ARGB(rgba[i]);
   }
   writeRegionClipped( /*fxMesa, */ NULL, dst_buffer,
		      dst_x,
		      dst_y,
		      GR_LFB_SRC_FMT_8888,
		      src_width, 1, src_stride, (void *) argb);
}

#endif /* !defined(FXMESA_USE_RGBA) */


/************************************************************************/
/*****                    Span functions                            *****/
/************************************************************************/


static void
fxDDWriteRGBASpan(const GLcontext * ctx,
		  GLuint n, GLint x, GLint y,
		  const GLubyte rgba[][4], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint i;
   GLint bottom = fxMesa->height - 1;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteRGBASpan(...)\n");
   }

   if (mask) {
      int span = 0;

      for (i = 0; i < n; i++) {
	 if (mask[i]) {
	    ++span;
	 }
	 else {
	    if (span > 0) {
	       LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x + i - span,
				   bottom - y,
				   /* GR_LFB_SRC_FMT_8888, */ span, /*1, */ 0,
				   (void *) rgba[i - span]);
	       span = 0;
	    }
	 }
      }

      if (span > 0)
	 LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x + n - span, bottom - y,
			     /* GR_LFB_SRC_FMT_8888, */ span, /*1, */ 0,
			     (void *) rgba[n - span]);
   }
   else
      LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x, bottom - y,	/* GR_LFB_SRC_FMT_8888, */
			  n, /* 1, */ 0, (void *) rgba);
}


static void
fxDDWriteRGBSpan(const GLcontext * ctx,
		 GLuint n, GLint x, GLint y,
		 const GLubyte rgb[][3], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint i;
   GLint bottom = fxMesa->height - 1;
   GLubyte rgba[MAX_WIDTH][4];

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteRGBSpan()\n");
   }

   if (mask) {
      int span = 0;

      for (i = 0; i < n; i++) {
	 if (mask[i]) {
	    rgba[span][RCOMP] = rgb[i][0];
	    rgba[span][GCOMP] = rgb[i][1];
	    rgba[span][BCOMP] = rgb[i][2];
	    rgba[span][ACOMP] = 255;
	    ++span;
	 }
	 else {
	    if (span > 0) {
	       LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x + i - span,
				   bottom - y,
				   /*GR_LFB_SRC_FMT_8888, */ span, /* 1, */ 0,
				   (void *) rgba);
	       span = 0;
	    }
	 }
      }

      if (span > 0)
	 LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x + n - span, bottom - y,
			     /*GR_LFB_SRC_FMT_8888, */ span, /* 1, */ 0,
			     (void *) rgba);
   }
   else {
      for (i = 0; i < n; i++) {
	 rgba[i][RCOMP] = rgb[i][0];
	 rgba[i][GCOMP] = rgb[i][1];
	 rgba[i][BCOMP] = rgb[i][2];
	 rgba[i][ACOMP] = 255;
      }

      LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x, bottom - y,	/* GR_LFB_SRC_FMT_8888, */
			  n, /* 1, */ 0, (void *) rgba);
   }
}


static void
fxDDWriteMonoRGBASpan(const GLcontext * ctx,
		      GLuint n, GLint x, GLint y,
		      const GLchan color[4], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint i;
   GLint bottom = fxMesa->height - 1;
   GLuint data[MAX_WIDTH];
   GrColor_t gColor = FXCOLOR4(color);

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteMonoRGBASpan(...)\n");
   }

   if (mask) {
      int span = 0;

      for (i = 0; i < n; i++) {
	 if (mask[i]) {
	    data[span] = (GLuint) gColor;
	    ++span;
	 }
	 else {
	    if (span > 0) {
	       writeRegionClipped(fxMesa, fxMesa->currentFB, x + i - span,
				  bottom - y, GR_LFB_SRC_FMT_8888, span, 1, 0,
				  (void *) data);
	       span = 0;
	    }
	 }
      }

      if (span > 0)
	 writeRegionClipped(fxMesa, fxMesa->currentFB, x + n - span,
			    bottom - y, GR_LFB_SRC_FMT_8888, span, 1, 0,
			    (void *) data);
   }
   else {
      for (i = 0; i < n; i++) {
	 data[i] = (GLuint) gColor;
      }

      writeRegionClipped(fxMesa, fxMesa->currentFB, x, bottom - y,
			 GR_LFB_SRC_FMT_8888, n, 1, 0, (void *) data);
   }
}


#if 0
static void
fxDDReadRGBASpan(const GLcontext * ctx,
		 GLuint n, GLint x, GLint y, GLubyte rgba[][4])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLushort data[MAX_WIDTH];
   GLuint i;
   GLint bottom = fxMesa->height - 1;

   printf("read span %d, %d, %d\n", x, y, n);
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDReadRGBASpan(...)\n");
   }

   assert(n < MAX_WIDTH);

   FX_grLfbReadRegion(fxMesa->currentFB, x, bottom - y, n, 1, 0, data);

   for (i = 0; i < n; i++) {
      GLushort pixel = data[i];
      rgba[i][RCOMP] = FX_PixelToR[pixel];
      rgba[i][GCOMP] = FX_PixelToG[pixel];
      rgba[i][BCOMP] = FX_PixelToB[pixel];
      rgba[i][ACOMP] = 255;
   }
}
#endif


/*
 * Read a span of 16-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void
read_R5G6B5_span(const GLcontext * ctx,
		 GLuint n, GLint x, GLint y, GLubyte rgba[][4])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GrLfbInfo_t info;
   BEGIN_BOARD_LOCK();
   if (grLfbLock(GR_LFB_READ_ONLY,
		 fxMesa->currentFB,
		 GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
      const GLint winX = 0;
      const GLint winY = fxMesa->height - 1;
      const GLint srcStride = info.strideInBytes / 2;	/* stride in GLushorts */
      const GLushort *data16 = (const GLushort *) info.lfbPtr
	 + (winY - y) * srcStride + (winX + x);
      const GLuint *data32 = (const GLuint *) data16;
      GLuint i, j;
      GLuint extraPixel = (n & 1);
      n -= extraPixel;
      for (i = j = 0; i < n; i += 2, j++) {
	 GLuint pixel = data32[j];
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
      }
      if (extraPixel) {
	 GLushort pixel = data16[n];
	 rgba[n][RCOMP] = FX_PixelToR[pixel];
	 rgba[n][GCOMP] = FX_PixelToG[pixel];
	 rgba[n][BCOMP] = FX_PixelToB[pixel];
	 rgba[n][ACOMP] = 255;
      }

      grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
   }
   END_BOARD_LOCK();
}


/************************************************************************/
/*****                    Pixel functions                           *****/
/************************************************************************/

static void
fxDDWriteRGBAPixels(const GLcontext * ctx,
		    GLuint n, const GLint x[], const GLint y[],
		    CONST GLubyte rgba[][4], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint i;
   GLint bottom = fxMesa->height - 1;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteRGBAPixels(...)\n");
   }

   for (i = 0; i < n; i++)
      if (mask[i])
	 LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x[i], bottom - y[i],
			     1, 1, (void *) rgba[i]);
}

static void
fxDDWriteMonoRGBAPixels(const GLcontext * ctx,
			GLuint n, const GLint x[], const GLint y[],
			const GLchan color[4], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint i;
   GLint bottom = fxMesa->height - 1;
   GrColor_t gColor = FXCOLOR4(color);

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteMonoRGBAPixels(...)\n");
   }

   for (i = 0; i < n; i++)
      if (mask[i])
	 writeRegionClipped(fxMesa, fxMesa->currentFB, x[i], bottom - y[i],
			    GR_LFB_SRC_FMT_8888, 1, 1, 0, (void *) &gColor);
}


static void
read_R5G6B5_pixels(const GLcontext * ctx,
		   GLuint n, const GLint x[], const GLint y[],
		   GLubyte rgba[][4], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GrLfbInfo_t info;
   BEGIN_BOARD_LOCK();
   if (grLfbLock(GR_LFB_READ_ONLY,
		 fxMesa->currentFB,
		 GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
      const GLint srcStride = info.strideInBytes / 2;	/* stride in GLushorts */
      const GLint winX = 0;
      const GLint winY = fxMesa->height - 1;
      GLuint i;
      for (i = 0; i < n; i++) {
	 if (mask[i]) {
	    const GLushort *data16 = (const GLushort *) info.lfbPtr
	       + (winY - y[i]) * srcStride + (winX + x[i]);
	    const GLushort pixel = *data16;
	    rgba[i][RCOMP] = FX_PixelToR[pixel];
	    rgba[i][GCOMP] = FX_PixelToG[pixel];
	    rgba[i][BCOMP] = FX_PixelToB[pixel];
	    rgba[i][ACOMP] = 255;
	 }
      }
      grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
   }
   END_BOARD_LOCK();
}



/************************************************************************/
/*****                    Depth functions                           *****/
/************************************************************************/

void
fxDDWriteDepthSpan(GLcontext * ctx,
		   GLuint n, GLint x, GLint y, const GLdepth depth[],
		   const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLint bottom = fxMesa->height - 1;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteDepthSpan(...)\n");
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
fxDDReadDepthSpan(GLcontext * ctx,
		  GLuint n, GLint x, GLint y, GLdepth depth[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLint bottom = fxMesa->height - 1;
   GLushort depth16[MAX_WIDTH];
   GLuint i;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDReadDepthSpan(...)\n");
   }

   FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth16);
   for (i = 0; i < n; i++) {
      depth[i] = depth16[i];
   }
}



void
fxDDWriteDepthPixels(GLcontext * ctx,
		     GLuint n, const GLint x[], const GLint y[],
		     const GLdepth depth[], const GLubyte mask[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDWriteDepthPixels(...)\n");
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
fxDDReadDepthPixels(GLcontext * ctx, GLuint n,
		    const GLint x[], const GLint y[], GLdepth depth[])
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDReadDepthPixels(...)\n");
   }

   for (i = 0; i < n; i++) {
      int xpos = x[i];
      int ypos = bottom - y[i];
      GLushort d;
      FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, xpos, ypos, 1, 1, 0, &d);
      depth[i] = d;
   }
}



/* Set the buffer used for reading */
/* XXX support for separate read/draw buffers hasn't been tested */
static void
fxDDSetReadBuffer(GLcontext * ctx, GLframebuffer * buffer, GLenum mode)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   (void) buffer;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDSetBuffer(%x)\n", (int) mode);
   }

   if (mode == GL_FRONT_LEFT) {
      fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
      FX_grRenderBuffer(fxMesa->currentFB);
   }
   else if (mode == GL_BACK_LEFT) {
      fxMesa->currentFB = GR_BUFFER_BACKBUFFER;
      FX_grRenderBuffer(fxMesa->currentFB);
   }
}


/************************************************************************/



void
fxSetupDDSpanPointers(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );

   swdd->SetReadBuffer = fxDDSetReadBuffer;

   swdd->WriteRGBASpan = fxDDWriteRGBASpan;
   swdd->WriteRGBSpan = fxDDWriteRGBSpan;
   swdd->WriteMonoRGBASpan = fxDDWriteMonoRGBASpan;
   swdd->WriteRGBAPixels = fxDDWriteRGBAPixels;
   swdd->WriteMonoRGBAPixels = fxDDWriteMonoRGBAPixels;

   swdd->WriteDepthSpan = fxDDWriteDepthSpan;
   swdd->WriteDepthPixels = fxDDWriteDepthPixels;
   swdd->ReadDepthSpan = fxDDReadDepthSpan;
   swdd->ReadDepthPixels = fxDDReadDepthPixels;

   /*  swdd->ReadRGBASpan        =fxDDReadRGBASpan; */
   swdd->ReadRGBASpan = read_R5G6B5_span;
   swdd->ReadRGBAPixels = read_R5G6B5_pixels;
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
