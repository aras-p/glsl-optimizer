/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgaspan.c,v 1.11 2002/10/30 12:51:36 alanh Exp $ */

#include "mtypes.h"
#include "mgadd.h"
#include "mgacontext.h"
#include "mgaspan.h"
#include "mgaioctl.h"
#include "swrast/swrast.h"

#define DBG 0


#define LOCAL_VARS					\
   __DRIdrawablePrivate *dPriv = mmesa->mesa_drawable;	\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;	\
   GLuint pitch = mgaScreen->frontPitch;		\
   GLuint height = dPriv->h;				\
   char *read_buf = (char *)(sPriv->pFB +		\
			mmesa->readOffset +		\
			dPriv->x * mgaScreen->cpp +	\
			dPriv->y * pitch);		\
   char *buf = (char *)(sPriv->pFB +			\
			mmesa->drawOffset +		\
			dPriv->x * mgaScreen->cpp +	\
			dPriv->y * pitch);		\
   GLuint p;						\
   (void) read_buf; (void) buf; (void) p
   


#define LOCAL_DEPTH_VARS						\
   __DRIdrawablePrivate *dPriv = mmesa->mesa_drawable;			\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;			\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;			\
   GLuint pitch = mgaScreen->frontPitch;				\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			mgaScreen->depthOffset +			\
			dPriv->x * mgaScreen->cpp +			\
			dPriv->y * pitch)

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


#define HW_LOCK()				\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);	\
   FLUSH_BATCH(mmesa);				\
   LOCK_HARDWARE_QUIESCENT(mmesa);


#define HW_CLIPLOOP()						\
  do {								\
    int _nc = mmesa->numClipRects;				\
    while (_nc--) {						\
       int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;	\
       int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY;	\
       int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;	\
       int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;

#define HW_ENDCLIPLOOP()			\
    }						\
  } while (0)

#define HW_UNLOCK()				\
    UNLOCK_HARDWARE(mmesa);







/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define Y_FLIP(_y) (height - _y - 1)

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_565( color[0], color[1], color[2] )


#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (((p >> 11) & 0x1f) * 255) / 31;			\
   rgba[1] = (((p >>  5) & 0x3f) * 255) / 63;			\
   rgba[2] = (((p >>  0) & 0x1f) * 255) / 31;			\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) mga##x##_565
#include "spantmp.h"





/* 32 bit, 8888 argb color spanline and pixel functions
 */

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )


#define WRITE_RGBA(_x, _y, r, g, b, a)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = ((r << 16) |	\
					  (g << 8)  |	\
					  (b << 0)  |	\
					  (a << 24) )

#define WRITE_PIXEL(_x, _y, p)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA(rgba, _x, _y)					\
    do {							\
	GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch);	\
	rgba[0] = (p >> 16) & 0xff;				\
	rgba[1] = (p >> 8)  & 0xff;				\
	rgba[2] = (p >> 0)  & 0xff;				\
	rgba[3] = 0xff;						\
    } while (0)

#define TAG(x) mga##x##_8888
#include "spantmp.h"




/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + _x*2 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + _x*2 + _y*pitch);

#define TAG(x) mga##x##_16
#include "depthtmp.h"




/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + _x*4 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch);

#define TAG(x) mga##x##_32
#include "depthtmp.h"



/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xff;						\
   tmp |= (d) << 8;					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )	{				\
   d = (*(GLuint *)(buf + _x*4 + _y*pitch) & ~0xff) >> 8;	\
}

#define TAG(x) mga##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= d & 0xff;					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xff;

#define TAG(x) mga##x##_24_8
#include "stenciltmp.h"



/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void mgaDDSetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                           GLuint bufferBit)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   unsigned int   offset;

   assert((bufferBit == DD_FRONT_LEFT_BIT) || (bufferBit == DD_BACK_LEFT_BIT));

   offset = (bufferBit == DD_FRONT_LEFT_BIT)
       ? mmesa->mgaScreen->frontOffset
       : mmesa->mgaScreen->backOffset;

   mmesa->drawOffset = offset;
   mmesa->readOffset = offset;

   assert( (buffer == mmesa->driDrawable->driverPrivate)
	   || (buffer == mmesa->driReadable->driverPrivate) );

   mmesa->mesa_drawable = (buffer == mmesa->driDrawable->driverPrivate)
       ? mmesa->driDrawable : mmesa->driReadable;
}

void mgaDDInitSpanFuncs( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = mgaDDSetBuffer;

   switch (mmesa->mgaScreen->cpp) {
   case 2:
      swdd->WriteRGBASpan = mgaWriteRGBASpan_565;
      swdd->WriteRGBSpan = mgaWriteRGBSpan_565;
      swdd->WriteMonoRGBASpan = mgaWriteMonoRGBASpan_565;
      swdd->WriteRGBAPixels = mgaWriteRGBAPixels_565;
      swdd->WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_565;
      swdd->ReadRGBASpan = mgaReadRGBASpan_565;
      swdd->ReadRGBAPixels = mgaReadRGBAPixels_565;

      swdd->ReadDepthSpan = mgaReadDepthSpan_16;
      swdd->WriteDepthSpan = mgaWriteDepthSpan_16;
      swdd->ReadDepthPixels = mgaReadDepthPixels_16;
      swdd->WriteDepthPixels = mgaWriteDepthPixels_16;
      break;

   case 4:
      swdd->WriteRGBASpan = mgaWriteRGBASpan_8888;
      swdd->WriteRGBSpan = mgaWriteRGBSpan_8888;
      swdd->WriteMonoRGBASpan = mgaWriteMonoRGBASpan_8888;
      swdd->WriteRGBAPixels = mgaWriteRGBAPixels_8888;
      swdd->WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_8888;
      swdd->ReadRGBASpan = mgaReadRGBASpan_8888;
      swdd->ReadRGBAPixels = mgaReadRGBAPixels_8888;
      
      if (!mmesa->hw_stencil) {
	 swdd->ReadDepthSpan = mgaReadDepthSpan_32;
	 swdd->WriteDepthSpan = mgaWriteDepthSpan_32;
	 swdd->ReadDepthPixels = mgaReadDepthPixels_32;
	 swdd->WriteDepthPixels = mgaWriteDepthPixels_32;
      } else {
	 swdd->ReadDepthSpan = mgaReadDepthSpan_24_8;
	 swdd->WriteDepthSpan = mgaWriteDepthSpan_24_8;
	 swdd->ReadDepthPixels = mgaReadDepthPixels_24_8;
	 swdd->WriteDepthPixels = mgaWriteDepthPixels_24_8;

	 swdd->ReadStencilSpan = mgaReadStencilSpan_24_8;
	 swdd->WriteStencilSpan = mgaWriteStencilSpan_24_8;
	 swdd->ReadStencilPixels = mgaReadStencilPixels_24_8;
	 swdd->WriteStencilPixels = mgaWriteStencilPixels_24_8;
      }
      break;
   }
}
