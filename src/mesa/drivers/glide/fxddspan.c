/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


/* fxdd.c - 3Dfx VooDoo Mesa span and pixel functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"

#ifdef _MSC_VER
#ifdef _WIN32
#pragma warning( disable : 4090 4022 )
/* 4101 : "different 'const' qualifier"
 * 4022 : "pointer mistmatch for actual parameter 'n'
 */
#endif
#endif


#if !defined(FXMESA_USE_ARGB) 


#if defined(FX_GLIDE3) && defined(XF86DRI)

static FxBool writeRegionClipped(fxMesaContext fxMesa, GrBuffer_t dst_buffer,
			  FxU32 dst_x, FxU32 dst_y, GrLfbSrcFmt_t src_format,
			  FxU32 src_width, FxU32 src_height, FxI32 src_stride,
			  void *src_data)
{
  int i, x, w, srcElt;
  void *data;

  if (src_width==1 && src_height==1) { /* Easy case writing a point */
    for (i=0; i<fxMesa->numClipRects; i++) {
      if ((dst_x>=fxMesa->pClipRects[i].x1) && 
	  (dst_x<fxMesa->pClipRects[i].x2) &&
	  (dst_y>=fxMesa->pClipRects[i].y1) && 
	  (dst_y<fxMesa->pClipRects[i].y2)) {
	FX_grLfbWriteRegion(dst_buffer, dst_x, dst_y, src_format,
			    1, 1, src_stride, src_data);
	return GL_TRUE;
      }
    }
  } else if (src_height==1) { /* Writing a span */
    if (src_format==GR_LFB_SRC_FMT_8888) srcElt=4;
    else if (src_format==GR_LFB_SRC_FMT_ZA16) srcElt=2;
    else {
      fprintf(stderr, "Unknown src_format passed to writeRegionClipped\n");
      return GL_FALSE;
    }
    for (i=0; i<fxMesa->numClipRects; i++) {
      if (dst_y>=fxMesa->pClipRects[i].y1 && dst_y<fxMesa->pClipRects[i].y2) {
	if (dst_x<fxMesa->pClipRects[i].x1) {
	  x=fxMesa->pClipRects[i].x1;
	  data=((char*)src_data)+srcElt*(x - dst_x);
	  w=src_width-(x-dst_x);
	} else {
	  x=dst_x;
	  data=src_data;
	  w=src_width;
	}
	if (x+w>fxMesa->pClipRects[i].x2) {
	  w=fxMesa->pClipRects[i].x2-x;
	}
	FX_grLfbWriteRegion(dst_buffer, x, dst_y, src_format, w, 1,
			    src_stride, data);
      }
    }
  } else { /* Punt on the case of arbitrary rectangles */
    return GL_FALSE;
  }
  return GL_TRUE;
}

#else

#define writeRegionClipped(fxm,dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)

#endif


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
  
inline void LFB_WRITE_SPAN_MESA(GrBuffer_t dst_buffer, 
			 FxU32 dst_x, 
			 FxU32 dst_y, 
			 FxU32 src_width,
			 FxI32 src_stride, 
			 void *src_data )
{
   /* Covert to ARGB */
   GLubyte (*rgba)[4] = src_data;
   GLuint argb[MAX_WIDTH];
   int i;
   
   for (i = 0; i < src_width; i++)
   {
      argb[i] = MESACOLOR_TO_ARGB(rgba[i]);
   }
   writeRegionClipped( /*fxMesa,*/ NULL, dst_buffer,
		       dst_x,
		       dst_y,
		       GR_LFB_SRC_FMT_8888,
		       src_width,
		       1,
		       src_stride,
		       (void*)argb);
}
 
#endif /* !defined(FXMESA_USE_RGBA) */


/************************************************************************/
/*****                    Span functions                            *****/
/************************************************************************/


static void fxDDWriteRGBASpan(const GLcontext *ctx, 
                              GLuint n, GLint x, GLint y,
                              const GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1; 

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDWriteRGBASpan(...)\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        ++span; 
      } else {
        if (span > 0) {
          LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+i-span, bottom-y,
                           /* GR_LFB_SRC_FMT_8888,*/ span, /*1,*/ 0, (void *) rgba[i-span] );
          span = 0;
        }
      }
    }

    if (span > 0)
      LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+n-span, bottom-y,
                        /* GR_LFB_SRC_FMT_8888, */ span, /*1,*/ 0, (void *) rgba[n-span] );
  } else
    LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x, bottom-y,/* GR_LFB_SRC_FMT_8888,*/
                      n,/* 1,*/ 0, (void *) rgba );
}


static void fxDDWriteRGBSpan(const GLcontext *ctx, 
                             GLuint n, GLint x, GLint y,
                             const GLubyte rgb[][3], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;
  GLubyte rgba[MAX_WIDTH][4];

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDWriteRGBSpan()\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        rgba[span][RCOMP] = rgb[i][0];
        rgba[span][GCOMP] = rgb[i][1];
        rgba[span][BCOMP] = rgb[i][2];
        rgba[span][ACOMP] = 255;
        ++span;
      } else {
        if (span > 0) {
          LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+i-span, bottom-y,
                            /*GR_LFB_SRC_FMT_8888,*/ span,/* 1,*/ 0, (void *) rgba );
          span = 0;
        }
      }
    }

    if (span > 0)
      LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+n-span, bottom-y,
                        /*GR_LFB_SRC_FMT_8888,*/ span,/* 1,*/ 0, (void *) rgba );
  } else {
    for (i=0;i<n;i++) {
      rgba[i][RCOMP]=rgb[i][0];
      rgba[i][GCOMP]=rgb[i][1];
      rgba[i][BCOMP]=rgb[i][2];
      rgba[i][ACOMP]=255;
    }

    LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x, bottom-y,/* GR_LFB_SRC_FMT_8888,*/
                      n,/* 1,*/ 0, (void *) rgba );
  }
}


static void fxDDWriteMonoRGBASpan(const GLcontext *ctx, 
                                  GLuint n, GLint x, GLint y,
                                  const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;
  GLuint data[MAX_WIDTH];

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDWriteMonoRGBASpan(...)\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        data[span] = (GLuint) fxMesa->color;
        ++span;
      } else {
        if (span > 0) {
          writeRegionClipped(fxMesa,  fxMesa->currentFB, x+i-span, bottom-y,
                            GR_LFB_SRC_FMT_8888, span, 1, 0,
                            (void *) data );
          span = 0;
        }
      }
    }

    if (span > 0)
      writeRegionClipped(fxMesa,  fxMesa->currentFB, x+n-span, bottom-y,
                        GR_LFB_SRC_FMT_8888, span, 1, 0,
                        (void *) data );
  } else {
    for (i=0;i<n;i++) {
      data[i]=(GLuint) fxMesa->color;
    }

    writeRegionClipped(fxMesa,  fxMesa->currentFB, x, bottom-y, GR_LFB_SRC_FMT_8888,
                      n, 1, 0, (void *) data );
  }
}


#if 0
static void fxDDReadRGBASpan(const GLcontext *ctx, 
                             GLuint n, GLint x, GLint y, GLubyte rgba[][4])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLushort data[MAX_WIDTH];
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  printf("read span %d, %d, %d\n", x,y,n);
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDReadRGBASpan(...)\n");
  }

  assert(n < MAX_WIDTH);

  x+=fxMesa->x_offset;
  FX_grLfbReadRegion( fxMesa->currentFB, x, bottom-y, n, 1, 0, data);

  for (i=0;i<n;i++) {
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
static void read_R5G6B5_span(const GLcontext *ctx, 
                             GLuint n, GLint x, GLint y, GLubyte rgba[][4])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GrLfbInfo_t info;
  BEGIN_BOARD_LOCK();
  if (grLfbLock(GR_LFB_READ_ONLY,
                fxMesa->currentFB,
                GR_LFBWRITEMODE_ANY,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winX = fxMesa->x_offset;
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
#ifdef XF86DRI
    const GLint srcStride = (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
                          ? (fxMesa->screen_width) : (info.strideInBytes / 2);
#else
    const GLint srcStride = info.strideInBytes / 2; /* stride in GLushorts */
#endif
    const GLushort *data16 = (const GLushort *) info.lfbPtr
                           + (winY - y) * srcStride
                           + (winX + x);
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
      rgba[i+1][RCOMP] = FX_PixelToR[pixel1];
      rgba[i+1][GCOMP] = FX_PixelToG[pixel1];
      rgba[i+1][BCOMP] = FX_PixelToB[pixel1];
      rgba[i+1][ACOMP] = 255;
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

static void fxDDWriteRGBAPixels(const GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                CONST GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDWriteRGBAPixels(...)\n");
  }

  for(i=0;i<n;i++)
    if(mask[i])
       LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x[i]+fxMesa->x_offset, bottom-y[i],
                       1, 1, (void *)rgba[i]);
}

static void fxDDWriteMonoRGBAPixels(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDWriteMonoRGBAPixels(...)\n");
  }

  for(i=0;i<n;i++)
    if(mask[i])
      writeRegionClipped(fxMesa, fxMesa->currentFB,x[i]+fxMesa->x_offset,bottom-y[i],
                       GR_LFB_SRC_FMT_8888,1,1,0,(void *) &fxMesa->color);
}


static void read_R5G6B5_pixels(const GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
  GrLfbInfo_t info;
  BEGIN_BOARD_LOCK();
  if (grLfbLock(GR_LFB_READ_ONLY,
                fxMesa->currentFB,
                GR_LFBWRITEMODE_ANY,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
#ifdef XF86DRI
    const GLint srcStride = (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
                          ? (fxMesa->screen_width) : (info.strideInBytes / 2);
#else
    const GLint srcStride = info.strideInBytes / 2; /* stride in GLushorts */
#endif
    const GLint winX = fxMesa->x_offset;
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    GLuint i;
    for(i=0;i<n;i++) {
      if(mask[i]) {
        const GLushort *data16 = (const GLushort *) info.lfbPtr
                               + (winY - y[i]) * srcStride
                               + (winX + x[i]);
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

void fxDDWriteDepthSpan(GLcontext *ctx,
                        GLuint n, GLint x, GLint y, const GLdepth depth[],
                        const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
     fprintf(stderr, "fxmesa: fxDDWriteDepthSpan(...)\n");
  }

  x += fxMesa->x_offset;

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


void fxDDReadDepthSpan(GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLdepth depth[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLushort depth16[MAX_WIDTH];
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
     fprintf(stderr, "fxmesa: fxDDReadDepthSpan(...)\n");
  }

  x += fxMesa->x_offset;
  FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth16);
  for (i = 0; i < n; i++) {
    depth[i] = depth16[i];
  }
}



void fxDDWriteDepthPixels(GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          const GLdepth depth[], const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDWriteDepthPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    if (mask[i]) {
      int xpos = x[i] + fxMesa->x_offset;
      int ypos = bottom - y[i];
      GLushort d = depth[i];
      writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, xpos, ypos,
                         GR_LFB_SRC_FMT_ZA16, 1, 1, 0, (void *) &d);
    }
  }
}


void fxDDReadDepthPixels(GLcontext *ctx, GLuint n,
                         const GLint x[], const GLint y[], GLdepth depth[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDReadDepthPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    int xpos = x[i] + fxMesa->x_offset;
    int ypos = bottom - y[i];
    GLushort d;
    FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, xpos, ypos, 1, 1, 0, &d);
    depth[i] = d;
  }
}




/************************************************************************/


void fxSetupDDSpanPointers(GLcontext *ctx)
{
  ctx->Driver.WriteRGBASpan       =fxDDWriteRGBASpan;
  ctx->Driver.WriteRGBSpan        =fxDDWriteRGBSpan;
  ctx->Driver.WriteMonoRGBASpan   =fxDDWriteMonoRGBASpan;
  ctx->Driver.WriteRGBAPixels     =fxDDWriteRGBAPixels;
  ctx->Driver.WriteMonoRGBAPixels =fxDDWriteMonoRGBAPixels;

  ctx->Driver.WriteCI8Span        =NULL;
  ctx->Driver.WriteCI32Span       =NULL;
  ctx->Driver.WriteMonoCISpan     =NULL;
  ctx->Driver.WriteCI32Pixels     =NULL;
  ctx->Driver.WriteMonoCIPixels   =NULL;

  /*  ctx->Driver.ReadRGBASpan        =fxDDReadRGBASpan;*/
  ctx->Driver.ReadRGBASpan = read_R5G6B5_span;
  ctx->Driver.ReadRGBAPixels      = read_R5G6B5_pixels;

  ctx->Driver.ReadCI32Span        =NULL;
  ctx->Driver.ReadCI32Pixels      =NULL;
}


#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_span(void)
{
  return 0;
}

#endif  /* FX */
