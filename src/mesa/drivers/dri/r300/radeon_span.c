/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "swrast/swrast.h"
#include "colormac.h"

#include "r200_context.h"
#include "radeon_ioctl.h"
#include "r300_ioctl.h"
#include "radeon_span.h"

#define DBG 0

#define LOCAL_VARS							\
   radeonContextPtr radeon = RADEON_CONTEXT(ctx);			\
   radeonScreenPtr radeonScreen = radeon->radeonScreen;			\
   __DRIscreenPrivate *sPriv = radeon->dri.screen;			\
   __DRIdrawablePrivate *dPriv = radeon->dri.drawable;			\
   GLuint pitch = radeonScreen->frontPitch * radeonScreen->cpp;		\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			radeon->state.color.drawOffset +		\
			(dPriv->x * radeonScreen->cpp) +		\
			(dPriv->y * pitch));				\
   char *read_buf = (char *)(sPriv->pFB +				\
			     radeon->state.pixel.readOffset +		\
			     (dPriv->x * radeonScreen->cpp) +		\
			     (dPriv->y * pitch));			\
   GLuint p;								\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS						\
   radeonContextPtr radeon = RADEON_CONTEXT(ctx);			\
   radeonScreenPtr radeonScreen = radeon->radeonScreen;			\
   __DRIscreenPrivate *sPriv = radeon->dri.screen;			\
   __DRIdrawablePrivate *dPriv = radeon->dri.drawable;			\
   GLuint height = dPriv->h;						\
   GLuint xo = dPriv->x;						\
   GLuint yo = dPriv->y;						\
   char *buf = (char *)(sPriv->pFB + radeonScreen->depthOffset);	\
   GLuint pitch = radeonScreen->depthPitch;				\
   (void) buf; (void) pitch

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define CLIPPIXEL( _x, _y )						\
   ((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))

#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
   if ( _y < miny || _y >= maxy ) {					\
      _n1 = 0, _x1 = x;							\
   } else {								\
      _n1 = _n;								\
      _x1 = _x;								\
      if ( _x1 < minx ) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx; \
      if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + n1 - maxx);		        \
   }

#define Y_FLIP( _y )		(height - _y - 1)

#define HW_LOCK()

#define HW_CLIPLOOP()							\
   do {									\
      __DRIdrawablePrivate *dPriv = radeon->dri.drawable;		\
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

#define HW_UNLOCK()

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
   do {									\
      GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);		\
      rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;				\
      rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;				\
      rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;				\
      rgba[3] = 0xff;							\
   } while (0)

#define TAG(x) radeon##x##_RGB565
#include "spantmp.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )			\
do {								\
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) |		\
					 (g <<  8) |		\
					 (r << 16) |		\
					 (a << 24) );		\
} while (0)

#define WRITE_PIXEL( _x, _y, p ) 			\
do {							\
   *(GLuint *)(buf + _x*4 + _y*pitch) = p;		\
} while (0)

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   volatile GLuint *ptr = (volatile GLuint *)(read_buf + _x*4 + _y*pitch); \
   GLuint p = *ptr;					\
   rgba[0] = (p >> 16) & 0xff;					\
   rgba[1] = (p >>  8) & 0xff;					\
   rgba[2] = (p >>  0) & 0xff;					\
   rgba[3] = (p >> 24) & 0xff;					\
} while (0)

#define TAG(x) radeon##x##_ARGB8888
#include "spantmp.h"

/* ================================================================
 * Depth buffer
 */

/* The Radeon family has depth tiling on all the time, so we have to convert
 * the x,y coordinates into the memory bus address (mba) in the same
 * manner as the engine.  In each case, the linear block address (ba)
 * is calculated, and then wired with x and y to produce the final
 * memory address.
 */

#define BIT(x,b) ((x & (1<<b))>>b)
static GLuint radeon_mba_z32(radeonContextPtr radeon, GLint x, GLint y)
{
	GLuint pitch = radeon->radeonScreen->depthPitch;
	GLuint b =
	    ((y & 0x3FF) >> 4) * ((pitch & 0xFFF) >> 5) + ((x & 0x3FF) >> 5);
	GLuint a =
	    (BIT(x, 0) << 2) | (BIT(y, 0) << 3) | (BIT(x, 1) << 4) | (BIT(y, 1)
								      << 5) |
	    (BIT(x, 3) << 6) | (BIT(x, 4) << 7) | (BIT(x, 2) << 8) | (BIT(y, 2)
								      << 9) |
	    (BIT(y, 3) << 10) |
	    (((pitch & 0x20) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y, 4)))) << 11) |
	    ((b >> 1) << 12);
	return a;
}

static GLuint radeon_mba_z16(radeonContextPtr radeon, GLint x, GLint y)
{
	GLuint pitch = radeon->radeonScreen->depthPitch;
	GLuint b =
	    ((y & 0x3FF) >> 4) * ((pitch & 0xFFF) >> 6) + ((x & 0x3FF) >> 6);
	GLuint a =
	    (BIT(x, 0) << 1) | (BIT(y, 0) << 2) | (BIT(x, 1) << 3) | (BIT(y, 1)
								      << 4) |
	    (BIT(x, 2) << 5) | (BIT(x, 4) << 6) | (BIT(x, 5) << 7) | (BIT(x, 3)
								      << 8) |
	    (BIT(y, 2) << 9) | (BIT(y, 3) << 10) |
	    (((pitch & 0x40) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y, 4)))) << 11) |
	    ((b >> 1) << 12);
	return a;
}


/* 16-bit depth buffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + radeon_mba_z16( radeon, _x + xo, _y + yo )) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + radeon_mba_z16( radeon, _x + xo, _y + yo ));

#define TAG(x) radeon##x##_16_TILE
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( radeon, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLuint *)(buf + radeon_mba_z32( radeon, _x + xo,		\
					 _y + yo )) & 0x00ffffff;

#define TAG(x) radeon##x##_24_8_TILE
#include "depthtmp.h"

/* 16-bit depth buffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + (_x + xo + (_y + yo)*pitch)*2 ) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + (_x + xo + (_y + yo)*pitch)*2 );

#define TAG(x) radeon##x##_16_LINEAR
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = (_x + xo + (_y + yo)*pitch)*4;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x000000ff;							\
   tmp |= ((d << 8) & 0xffffff00);					\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_DEPTH( d, _x, _y )						\
   d = (*(GLuint *)(buf + (_x + xo + (_y + yo)*pitch)*4) & 0xffffff00) >> 8;

#define TAG(x) radeon##x##_24_8_LINEAR
#include "depthtmp.h"

/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( radeon, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = radeon_mba_z32( radeon, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   d = tmp >> 24;							\
} while (0)

#define TAG(x) radeon##x##_24_8_TILE
#include "stenciltmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = (_x + xo)*4 + (_y + yo)*pitch;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = (_x + xo)*4 + (_y + yo)*pitch;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   d = tmp >> 24;							\
} while (0)

#define TAG(x) radeon##x##_24_8_LINEAR
#include "stenciltmp.h"

/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void radeonSetBuffer(GLcontext * ctx,
			  GLframebuffer * colorBuffer, GLuint bufferBit)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	int buffer;

	switch (bufferBit) {
	case DD_FRONT_LEFT_BIT:
		buffer = 0;
		break;

	case DD_BACK_LEFT_BIT:
		buffer = 1;
		break;

	default:
		_mesa_problem(ctx, "Bad bufferBit in %s", __FUNCTION__);
		return;
	}

	if (radeon->doPageFlip && radeon->sarea->pfCurrentPage == 1)
		buffer ^= 1;

#if 0
	fprintf(stderr, "%s: using %s buffer\n", __FUNCTION__,
		buffer ? "back" : "front");
#endif

	if (buffer) {
		radeon->state.pixel.readOffset =
			radeon->radeonScreen->backOffset;
		radeon->state.pixel.readPitch =
			radeon->radeonScreen->backPitch;
		radeon->state.color.drawOffset =
			radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch =
			radeon->radeonScreen->backPitch;
	} else {
		radeon->state.pixel.readOffset =
			radeon->radeonScreen->frontOffset;
		radeon->state.pixel.readPitch =
			radeon->radeonScreen->frontPitch;
		radeon->state.color.drawOffset =
			radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch =
			radeon->radeonScreen->frontPitch;
	}
}

/* Move locking out to get reasonable span performance (10x better
 * than doing this in HW_LOCK above).  WaitForIdle() is the main
 * culprit.
 */

static void radeonSpanRenderStart(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	if (IS_FAMILY_R200(radeon))
		R200_FIREVERTICES((r200ContextPtr)radeon);
	else
		r300Flush(ctx);

	LOCK_HARDWARE(radeon);
	radeonWaitForIdleLocked(radeon);

	/* Read & rewrite the first pixel in the frame buffer.  This should
	 * be a noop, right?  In fact without this conform fails as reading
	 * from the framebuffer sometimes produces old results -- the
	 * on-card read cache gets mixed up and doesn't notice that the
	 * framebuffer has been updated.
	 *
	 * In the worst case this is buggy too as p might get the wrong
	 * value first time, so really need a hidden pixel somewhere for this.
	 */
	{
		int p;
		volatile int *read_buf =
		    (volatile int *)(radeon->dri.screen->pFB +
				     radeon->state.pixel.readOffset);
		p = *read_buf;
		*read_buf = p;
	}
}

static void radeonSpanRenderFinish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	_swrast_flush(ctx);
	UNLOCK_HARDWARE(radeon);
}

void radeonInitSpanFuncs(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);

	swdd->SetBuffer = radeonSetBuffer;

	switch (radeon->radeonScreen->cpp) {
	case 2:
		swdd->WriteRGBASpan = radeonWriteRGBASpan_RGB565;
		swdd->WriteRGBSpan = radeonWriteRGBSpan_RGB565;
		swdd->WriteMonoRGBASpan = radeonWriteMonoRGBASpan_RGB565;
		swdd->WriteRGBAPixels = radeonWriteRGBAPixels_RGB565;
		swdd->WriteMonoRGBAPixels = radeonWriteMonoRGBAPixels_RGB565;
		swdd->ReadRGBASpan = radeonReadRGBASpan_RGB565;
		swdd->ReadRGBAPixels = radeonReadRGBAPixels_RGB565;
		break;

	case 4:
		swdd->WriteRGBASpan = radeonWriteRGBASpan_ARGB8888;
		swdd->WriteRGBSpan = radeonWriteRGBSpan_ARGB8888;
		swdd->WriteMonoRGBASpan = radeonWriteMonoRGBASpan_ARGB8888;
		swdd->WriteRGBAPixels = radeonWriteRGBAPixels_ARGB8888;
		swdd->WriteMonoRGBAPixels = radeonWriteMonoRGBAPixels_ARGB8888;
		swdd->ReadRGBASpan = radeonReadRGBASpan_ARGB8888;
		swdd->ReadRGBAPixels = radeonReadRGBAPixels_ARGB8888;
		break;

	default:
		break;
	}

	if (IS_FAMILY_R300(radeon))
	{
		switch (radeon->glCtx->Visual.depthBits) {
		case 16:
			swdd->ReadDepthSpan = radeonReadDepthSpan_16_LINEAR;
			swdd->WriteDepthSpan = radeonWriteDepthSpan_16_LINEAR;
			swdd->WriteMonoDepthSpan = radeonWriteMonoDepthSpan_16_LINEAR;
			swdd->ReadDepthPixels = radeonReadDepthPixels_16_LINEAR;
			swdd->WriteDepthPixels = radeonWriteDepthPixels_16_LINEAR;
			break;

		case 24:
			swdd->ReadDepthSpan = radeonReadDepthSpan_24_8_LINEAR;
			swdd->WriteDepthSpan = radeonWriteDepthSpan_24_8_LINEAR;
			swdd->WriteMonoDepthSpan = radeonWriteMonoDepthSpan_24_8_LINEAR;
			swdd->ReadDepthPixels = radeonReadDepthPixels_24_8_LINEAR;
			swdd->WriteDepthPixels = radeonWriteDepthPixels_24_8_LINEAR;

			swdd->ReadStencilSpan = radeonReadStencilSpan_24_8_LINEAR;
			swdd->WriteStencilSpan = radeonWriteStencilSpan_24_8_LINEAR;
			swdd->ReadStencilPixels = radeonReadStencilPixels_24_8_LINEAR;
			swdd->WriteStencilPixels = radeonWriteStencilPixels_24_8_LINEAR;
			break;

		default:
			break;
		}
	}
	else
	{
		switch (radeon->glCtx->Visual.depthBits) {
		case 16:
			swdd->ReadDepthSpan = radeonReadDepthSpan_16_TILE;
			swdd->WriteDepthSpan = radeonWriteDepthSpan_16_TILE;
			swdd->WriteMonoDepthSpan = radeonWriteMonoDepthSpan_16_TILE;
			swdd->ReadDepthPixels = radeonReadDepthPixels_16_TILE;
			swdd->WriteDepthPixels = radeonWriteDepthPixels_16_TILE;
			break;

		case 24:
			swdd->ReadDepthSpan = radeonReadDepthSpan_24_8_TILE;
			swdd->WriteDepthSpan = radeonWriteDepthSpan_24_8_TILE;
			swdd->WriteMonoDepthSpan = radeonWriteMonoDepthSpan_24_8_TILE;
			swdd->ReadDepthPixels = radeonReadDepthPixels_24_8_TILE;
			swdd->WriteDepthPixels = radeonWriteDepthPixels_24_8_TILE;

			swdd->ReadStencilSpan = radeonReadStencilSpan_24_8_TILE;
			swdd->WriteStencilSpan = radeonWriteStencilSpan_24_8_TILE;
			swdd->ReadStencilPixels = radeonReadStencilPixels_24_8_TILE;
			swdd->WriteStencilPixels = radeonWriteStencilPixels_24_8_TILE;
			break;

		default:
			break;
		}
	}

	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}
