/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_state.h"
#include "mach64_span.h"
#include "mach64_tex.h"

#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS							\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);			\
   mach64ScreenRec *mach64Screen = mmesa->mach64Screen;			\
   __DRIscreenPrivate *driScreen = mmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;			\
   GLuint pitch = mmesa->drawPitch * mach64Screen->cpp;			\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(driScreen->pFB +				\
			mmesa->drawOffset +				\
			(dPriv->x * mach64Screen->cpp) +		\
			(dPriv->y * pitch));				\
   char *read_buf = (char *)(driScreen->pFB +				\
			     mmesa->readOffset +			\
			     (dPriv->x * mach64Screen->cpp) +		\
			     (dPriv->y * pitch));			\
   GLushort p;								\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS						\
   mach64ScreenRec *mach64Screen = mmesa->mach64Screen;			\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;			\
   __DRIscreenPrivate *driScreen = mmesa->driScreen;			\
   GLuint pitch = mach64Screen->depthPitch * 2;				\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(driScreen->pFB +				\
			mach64Screen->depthOffset +			\
			dPriv->x * 2 +					\
			dPriv->y * pitch)

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define CLIPPIXEL( _x, _y )						\
   ((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))


#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
   if (( _y < miny) || (_y >= maxy)) {					\
      _n1 = 0, _x1 = x;							\
   } else {								\
      _n1 = _n;								\
      _x1 = _x;								\
      if (_x1 < minx) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx;	\
      if (_x1 + _n1 >= maxx) n1 -= (_x1 + n1 - maxx);			\
   }

#define Y_FLIP( _y )	(height - _y - 1)


#define HW_LOCK()							\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);			\
   LOCK_HARDWARE( mmesa );						\
   FINISH_DMA_LOCKED( mmesa );						\

#define HW_CLIPLOOP()							\
   do {									\
      int _nc = mmesa->numClipRects;					\
									\
      while ( _nc-- ) {							\
	 int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;		\
	 int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY;		\
	 int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;		\
	 int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;

#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)

#define HW_UNLOCK()							\
   UNLOCK_HARDWARE( mmesa )						\



/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = MACH64PACKCOLOR565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);		\
	rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;			\
	rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;			\
	rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;			\
	rgba[3] = 0xff;							\
    } while (0)

#define TAG(x) mach64##x##_RGB565
#include "spantmp.h"



/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = MACH64PACKCOLOR8888( color[0], color[1], color[2], color[3] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) |			\
					 (g <<  8) |			\
					 (r << 16) |			\
					 (a << 24) )

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch);			\
   rgba[0] = (p >> 16) & 0xff;						\
   rgba[1] = (p >>  8) & 0xff;						\
   rgba[2] = (p >>  0) & 0xff;						\
   rgba[3] = 0xff; /*(p >> 24) & 0xff;*/				\
} while (0)

#define TAG(x) mach64##x##_ARGB8888
#include "spantmp.h"



/* ================================================================
 * Depth buffer
 */

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + _x*2 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + _x*2 + _y*pitch);

#define TAG(x) mach64##x##_16
#include "depthtmp.h"


/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void mach64DDSetBuffer( GLcontext *ctx,
			       GLframebuffer *colorBuffer,
			       GLuint bufferBit )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   switch ( bufferBit ) {
   case BUFFER_BIT_FRONT_LEFT:
      if (MACH64_DEBUG & DEBUG_VERBOSE_MSG)
	 fprintf(stderr,"%s: BUFFER_BIT_FRONT_LEFT\n", __FUNCTION__);
      mmesa->drawOffset = mmesa->readOffset = mmesa->mach64Screen->frontOffset;
      mmesa->drawPitch  = mmesa->readPitch  = mmesa->mach64Screen->frontPitch;
      break;
   case BUFFER_BIT_BACK_LEFT:
      if (MACH64_DEBUG & DEBUG_VERBOSE_MSG)
	 fprintf(stderr,"%s: BUFFER_BIT_BACK_LEFT\n", __FUNCTION__);
      mmesa->drawOffset = mmesa->readOffset = mmesa->mach64Screen->backOffset;
      mmesa->drawPitch  = mmesa->readPitch  = mmesa->mach64Screen->backPitch;
      break;
   default:
      break;
   }
}


void mach64DDInitSpanFuncs( GLcontext *ctx )
{
#if 0
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
#endif
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = mach64DDSetBuffer;

#if 0
   switch ( mmesa->mach64Screen->cpp ) {
   case 2:
      swdd->WriteRGBASpan	= mach64WriteRGBASpan_RGB565;
      swdd->WriteRGBSpan	= mach64WriteRGBSpan_RGB565;
      swdd->WriteMonoRGBASpan	= mach64WriteMonoRGBASpan_RGB565;
      swdd->WriteRGBAPixels	= mach64WriteRGBAPixels_RGB565;
      swdd->WriteMonoRGBAPixels	= mach64WriteMonoRGBAPixels_RGB565;
      swdd->ReadRGBASpan	= mach64ReadRGBASpan_RGB565;
      swdd->ReadRGBAPixels	= mach64ReadRGBAPixels_RGB565;
      break;

   case 4:
      swdd->WriteRGBASpan	= mach64WriteRGBASpan_ARGB8888;
      swdd->WriteRGBSpan	= mach64WriteRGBSpan_ARGB8888;
      swdd->WriteMonoRGBASpan	= mach64WriteMonoRGBASpan_ARGB8888;
      swdd->WriteRGBAPixels	= mach64WriteRGBAPixels_ARGB8888;
      swdd->WriteMonoRGBAPixels	= mach64WriteMonoRGBAPixels_ARGB8888;
      swdd->ReadRGBASpan	= mach64ReadRGBASpan_ARGB8888;
      swdd->ReadRGBAPixels	= mach64ReadRGBAPixels_ARGB8888;

      break;

   default:
      break;
   }
#endif

   /* Depth buffer is always 16 bit */
#if 0
   swdd->ReadDepthSpan		= mach64ReadDepthSpan_16;
   swdd->WriteDepthSpan		= mach64WriteDepthSpan_16;
   swdd->ReadDepthPixels	= mach64ReadDepthPixels_16;
   swdd->WriteDepthPixels	= mach64WriteDepthPixels_16;
#endif
   /* No hardware stencil buffer */
   swdd->ReadStencilSpan	= NULL;
   swdd->WriteStencilSpan	= NULL;
   swdd->ReadStencilPixels	= NULL;
   swdd->WriteStencilPixels	= NULL;

   swdd->WriteCI8Span		= NULL;
   swdd->WriteCI32Span		= NULL;
   swdd->WriteMonoCISpan	= NULL;
   swdd->WriteCI32Pixels	= NULL;
   swdd->WriteMonoCIPixels	= NULL;
   swdd->ReadCI32Span		= NULL;
   swdd->ReadCI32Pixels		= NULL;
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
mach64SetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow        = mach64ReadRGBASpan_RGB565;
         drb->Base.GetValues     = mach64ReadRGBAPixels_RGB565;
         drb->Base.PutRow        = mach64WriteRGBASpan_RGB565;
         drb->Base.PutRowRGB     = mach64WriteRGBSpan_RGB565;
         drb->Base.PutMonoRow    = mach64WriteMonoRGBASpan_RGB565;
         drb->Base.PutValues     = mach64WriteRGBAPixels_RGB565;
         drb->Base.PutMonoValues = mach64WriteMonoRGBAPixels_RGB565;
      }
      else {
         drb->Base.GetRow        = mach64ReadRGBASpan_ARGB8888;
         drb->Base.GetValues     = mach64ReadRGBAPixels_ARGB8888;
         drb->Base.PutRow        = mach64WriteRGBASpan_ARGB8888;
         drb->Base.PutRowRGB     = mach64WriteRGBSpan_ARGB8888;
         drb->Base.PutMonoRow    = mach64WriteMonoRGBASpan_ARGB8888;
         drb->Base.PutValues     = mach64WriteRGBAPixels_ARGB8888;
         drb->Base.PutMonoValues = mach64WriteMonoRGBAPixels_ARGB8888;
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = mach64ReadDepthSpan_16;
      drb->Base.GetValues     = mach64ReadDepthPixels_16;
      drb->Base.PutRow        = mach64WriteDepthSpan_16;
      drb->Base.PutMonoRow    = mach64WriteMonoDepthSpan_16;
      drb->Base.PutValues     = mach64WriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      /* never */
      drb->Base.GetRow        = NULL;
      drb->Base.GetValues     = NULL;
      drb->Base.PutRow        = NULL;
      drb->Base.PutMonoRow    = NULL;
      drb->Base.PutValues     = NULL;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      /* never */
      drb->Base.GetRow        = NULL;
      drb->Base.GetValues     = NULL;
      drb->Base.PutRow        = NULL;
      drb->Base.PutMonoRow    = NULL;
      drb->Base.PutValues     = NULL;
      drb->Base.PutMonoValues = NULL;
   }
}
