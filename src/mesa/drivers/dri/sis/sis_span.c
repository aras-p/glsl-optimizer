/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_span.c,v 1.5 2001/03/21 16:14:26 dawes Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_span.h"
#include "sis_lock.h"
#include "sis_tris.h"

#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS							\
   sisContextPtr smesa = SIS_CONTEXT(ctx);				\
   char *buf = (char *)(smesa->FbBase + smesa->drawOffset);		\
   char *read_buf = (char *)(smesa->FbBase + smesa->readOffset);	\
   GLuint p;								\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS						\
   sisContextPtr smesa = SIS_CONTEXT(ctx);				\
   char *buf = smesa->depthbuffer;					\

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define CLIPPIXEL(_x,_y) (_x >= minx && _x < maxx && \
			  _y >= miny && _y < maxy)

#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
   if ( _y < miny || _y >= maxy ) {					\
      _n1 = 0, _x1 = x;							\
   } else {								\
      _n1 = _n;								\
      _x1 = _x;								\
      if ( _x1 < minx ) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx; \
      if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + n1 - maxx);		        \
   }

#define HW_LOCK() do {} while(0);

#define HW_CLIPLOOP()							\
   do {									\
      __DRIdrawablePrivate *dPriv = smesa->driDrawable;			\
      int _nc = dPriv->numClipRects;					\
									\
      while ( _nc-- ) {							\
	 int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
	 int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;		\
	 int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
	 int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;

#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)

#define HW_UNLOCK() do {} while(0);

/* RGB565 */
#define INIT_MONO_PIXEL(p, color) \
  p = SISPACKCOLOR565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*smesa->drawPitch) =			\
					     (((r & 0xf8) << 8) |	\
					     ((g & 0xfc) << 3) |	\
					     (b >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*smesa->drawPitch) = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*smesa->readPitch);	\
   rgba[0] = (p & 0xf800) >> 8;				\
   rgba[1] = (p & 0x07e0) >> 3;			        \
   rgba[2] = (p & 0x001f) << 3;			        \
   rgba[3] = 0xff;					\
} while(0)

#define TAG(x) sis##x##_565
#include "spantmp.h"


/* ARGB8888 */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = SISPACKCOLOR8888( color[0], color[1], color[2], color[3] )

#define WRITE_RGBA( _x, _y, r, g, b, a )			\
   *(GLuint *)(buf + _x*4 + _y*smesa->drawPitch) =		\
					   (((a) << 24) |	\
					   ((r) << 16) |	\
					   ((g) << 8) |		\
					   ((b)))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLuint *)(buf + _x*4 + _y*smesa->drawPitch)  = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLuint p = *(GLuint *)(read_buf + _x*4 + _y*smesa->readPitch);	\
   rgba[0] = (p >> 16) & 0xff;				\
   rgba[1] = (p >> 8) & 0xff;				\
   rgba[2] = (p >> 0) & 0xff;				\
   rgba[3] = 0xff;					\
} while(0)

#define TAG(x) sis##x##_8888
#include "spantmp.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + (_x)*2 + (_y)*smesa->depthPitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*smesa->depthPitch);

#define TAG(x) sis##x##_16
#include "depthtmp.h"


/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch);

#define TAG(x) sis##x##_32
#include "depthtmp.h"


/* 8/24 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {				\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch); \
   tmp &= 0xff000000;						\
   tmp |= (d & 0x00ffffff);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch) = tmp;	\
}

#define READ_DEPTH( d, _x, _y )	{			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch) & 0x00ffffff; \
}

#define TAG(x) sis##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {				\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch); \
   tmp &= 0x00ffffff;						\
   tmp |= (d << 24);						\
   *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch) = tmp;	\
}

#define READ_STENCIL( d, _x, _y )			\
   d = (*(GLuint *)(buf + (_x)*4 + (_y)*smesa->depthPitch) & 0xff000000) >> 24;

#define TAG(x) sis##x##_24_8
#include "stenciltmp.h"

/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void sisDDSetBuffer( GLcontext *ctx,
                            GLframebuffer *colorBuffer,
                            GLuint bufferBit )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   switch ( bufferBit ) {
   case BUFFER_BIT_FRONT_LEFT:
      smesa->drawOffset = smesa->readOffset = smesa->frontOffset;
      smesa->drawPitch  = smesa->readPitch  = smesa->frontPitch;
      break;
   case BUFFER_BIT_BACK_LEFT:
      smesa->drawOffset = smesa->readOffset = smesa->backOffset;
      smesa->drawPitch  = smesa->readPitch  = smesa->backPitch;
      break;
   default:
      break;
   }
}

void sisSpanRenderStart( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   SIS_FIREVERTICES(smesa);
   LOCK_HARDWARE();
   WaitEngIdle( smesa );
}

void sisSpanRenderFinish( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   _swrast_flush( ctx );
   UNLOCK_HARDWARE();
}

void
sisDDInitSpanFuncs( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = sisDDSetBuffer;

   switch (smesa->zFormat)
   {
   case SiS_ZFORMAT_Z16:
#if 0
      swdd->ReadDepthSpan = sisReadDepthSpan_16;
      swdd->ReadDepthPixels = sisReadDepthPixels_16;
      swdd->WriteDepthSpan = sisWriteDepthSpan_16;
      swdd->WriteDepthPixels = sisWriteDepthPixels_16;

      swdd->ReadStencilSpan = NULL;
      swdd->ReadStencilPixels = NULL;
      swdd->WriteStencilSpan = NULL;
      swdd->WriteStencilPixels = NULL;
#endif
      break;
   case SiS_ZFORMAT_Z32:
#if 0
      swdd->ReadDepthSpan = sisReadDepthSpan_32;
      swdd->ReadDepthPixels = sisReadDepthPixels_32;
      swdd->WriteDepthSpan = sisWriteDepthSpan_32;
      swdd->WriteDepthPixels = sisWriteDepthPixels_32;

      swdd->ReadStencilSpan = NULL;
      swdd->ReadStencilPixels = NULL;
      swdd->WriteStencilSpan = NULL;
      swdd->WriteStencilPixels = NULL;
#endif
      break;
   case SiS_ZFORMAT_S8Z24:
#if 0
      swdd->ReadDepthSpan = sisReadDepthSpan_24_8;
      swdd->ReadDepthPixels = sisReadDepthPixels_24_8;
      swdd->WriteDepthSpan = sisWriteDepthSpan_24_8;
      swdd->WriteDepthPixels = sisWriteDepthPixels_24_8;

      swdd->ReadStencilSpan = sisReadStencilSpan_24_8;
      swdd->ReadStencilPixels = sisReadStencilPixels_24_8;
      swdd->WriteStencilSpan = sisWriteStencilSpan_24_8;
      swdd->WriteStencilPixels = sisWriteStencilPixels_24_8;
#endif
      break;
   }

#if 0
   switch ( smesa->bytesPerPixel )
   {
   case 2:
      swdd->WriteRGBASpan = sisWriteRGBASpan_565;
      swdd->WriteRGBSpan = sisWriteRGBSpan_565;
      swdd->WriteMonoRGBASpan = sisWriteMonoRGBASpan_565;
      swdd->WriteRGBAPixels = sisWriteRGBAPixels_565;
      swdd->WriteMonoRGBAPixels = sisWriteMonoRGBAPixels_565;
      swdd->ReadRGBASpan = sisReadRGBASpan_565;
      swdd->ReadRGBAPixels = sisReadRGBAPixels_565;
      break;
   case 4:
      swdd->WriteRGBASpan = sisWriteRGBASpan_8888;
      swdd->WriteRGBSpan = sisWriteRGBSpan_8888;
      swdd->WriteMonoRGBASpan = sisWriteMonoRGBASpan_8888;
      swdd->WriteRGBAPixels = sisWriteRGBAPixels_8888;
      swdd->WriteMonoRGBAPixels = sisWriteMonoRGBAPixels_8888;
      swdd->ReadRGBASpan = sisReadRGBASpan_8888;
      swdd->ReadRGBAPixels = sisReadRGBAPixels_8888;
      break;
    default:
      sis_fatal_error("Bad bytesPerPixel.\n");
      break;
   }

   swdd->WriteCI8Span      = NULL;
   swdd->WriteCI32Span     = NULL;
   swdd->WriteMonoCISpan   = NULL;
   swdd->WriteCI32Pixels   = NULL;
   swdd->WriteMonoCIPixels = NULL;
   swdd->ReadCI32Span      = NULL;
   swdd->ReadCI32Pixels    = NULL;
#endif

   swdd->SpanRenderStart   = sisSpanRenderStart;
   swdd->SpanRenderFinish  = sisSpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
sisSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow        = sisReadRGBASpan_565;
         drb->Base.GetValues     = sisReadRGBAPixels_565;
         drb->Base.PutRow        = sisWriteRGBASpan_565;
         drb->Base.PutRowRGB     = sisWriteRGBSpan_565;
         drb->Base.PutMonoRow    = sisWriteMonoRGBASpan_565;
         drb->Base.PutValues     = sisWriteRGBAPixels_565;
         drb->Base.PutMonoValues = sisWriteMonoRGBAPixels_565;
      }
      else {
         drb->Base.GetRow        = sisReadRGBASpan_8888;
         drb->Base.GetValues     = sisReadRGBAPixels_8888;
         drb->Base.PutRow        = sisWriteRGBASpan_8888;
         drb->Base.PutRowRGB     = sisWriteRGBSpan_8888;
         drb->Base.PutMonoRow    = sisWriteMonoRGBASpan_8888;
         drb->Base.PutValues     = sisWriteRGBAPixels_8888;
         drb->Base.PutMonoValues = sisWriteMonoRGBAPixels_8888;
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = sisReadDepthSpan_16;
      drb->Base.GetValues     = sisReadDepthPixels_16;
      drb->Base.PutRow        = sisWriteDepthSpan_16;
      drb->Base.PutMonoRow    = sisWriteMonoDepthSpan_16;
      drb->Base.PutValues     = sisWriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = sisReadDepthSpan_24_8;
      drb->Base.GetValues     = sisReadDepthPixels_24_8;
      drb->Base.PutRow        = sisWriteDepthSpan_24_8;
      drb->Base.PutMonoRow    = sisWriteMonoDepthSpan_24_8;
      drb->Base.PutValues     = sisWriteDepthPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      drb->Base.GetRow        = sisReadDepthSpan_32;
      drb->Base.GetValues     = sisReadDepthPixels_32;
      drb->Base.PutRow        = sisWriteDepthSpan_32;
      drb->Base.PutMonoRow    = sisWriteMonoDepthSpan_32;
      drb->Base.PutValues     = sisWriteDepthPixels_32;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = sisReadStencilSpan_24_8;
      drb->Base.GetValues     = sisReadStencilPixels_24_8;
      drb->Base.PutRow        = sisWriteStencilSpan_24_8;
      drb->Base.PutMonoRow    = sisWriteMonoStencilSpan_24_8;
      drb->Base.PutValues     = sisWriteStencilPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
}
