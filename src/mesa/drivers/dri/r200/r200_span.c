/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_span.c,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
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
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_span.h"
#include "r200_tex.h"

#define DBG 0

#define LOCAL_VARS							\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   r200ScreenPtr r200Screen = rmesa->r200Screen;			\
   __DRIscreenPrivate *sPriv = rmesa->dri.screen;			\
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;			\
   GLuint pitch = r200Screen->frontPitch * r200Screen->cpp;		\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			rmesa->state.color.drawOffset +			\
			(dPriv->x * r200Screen->cpp) +		\
			(dPriv->y * pitch));				\
   char *read_buf = (char *)(sPriv->pFB +				\
			     rmesa->state.pixel.readOffset +		\
			     (dPriv->x * r200Screen->cpp) +		\
			     (dPriv->y * pitch));			\
   GLuint p;								\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   r200ScreenPtr r200Screen = rmesa->r200Screen;			\
   __DRIscreenPrivate *sPriv = rmesa->dri.screen;			\
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;			\
   GLuint height = dPriv->h;						\
   GLuint xo = dPriv->x;						\
   GLuint yo = dPriv->y;						\
   char *buf = (char *)(sPriv->pFB + r200Screen->depthOffset);	\
   (void) buf

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
      __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;		\
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

#define TAG(x) r200##x##_RGB565
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

#define TAG(x) r200##x##_ARGB8888
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
static GLuint r200_mba_z32( r200ContextPtr rmesa,
				       GLint x, GLint y )
{
   GLuint pitch = rmesa->r200Screen->frontPitch;
   GLuint b = ((y & 0x3FF) >> 4) * ((pitch & 0xFFF) >> 5) + ((x & 0x3FF) >> 5);
   GLuint a = 
      (BIT(x,0) << 2) |
      (BIT(y,0) << 3) |
      (BIT(x,1) << 4) |
      (BIT(y,1) << 5) |
      (BIT(x,3) << 6) |
      (BIT(x,4) << 7) |
      (BIT(x,2) << 8) |
      (BIT(y,2) << 9) |
      (BIT(y,3) << 10) |
      (((pitch & 0x20) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y,4)))) << 11) |
      ((b >> 1) << 12);
   return a;
}

static GLuint r200_mba_z16( r200ContextPtr rmesa, GLint x, GLint y )
{
   GLuint pitch = rmesa->r200Screen->frontPitch;
   GLuint b = ((y & 0x3FF) >> 4) * ((pitch & 0xFFF) >> 6) + ((x & 0x3FF) >> 6);
   GLuint a = 
      (BIT(x,0) << 1) |
      (BIT(y,0) << 2) |
      (BIT(x,1) << 3) |
      (BIT(y,1) << 4) |
      (BIT(x,2) << 5) |
      (BIT(x,4) << 6) |
      (BIT(x,5) << 7) |
      (BIT(x,3) << 8) |
      (BIT(y,2) << 9) |
      (BIT(y,3) << 10) |
      (((pitch & 0x40) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y,4)))) << 11) |
      ((b >> 1) << 12);
   return a;
}


/* 16-bit depth buffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + r200_mba_z16( rmesa, _x + xo, _y + yo )) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + r200_mba_z16( rmesa, _x + xo, _y + yo ));

#define TAG(x) r200##x##_16
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = r200_mba_z32( rmesa, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLuint *)(buf + r200_mba_z32( rmesa, _x + xo,		\
					 _y + yo )) & 0x00ffffff;

#define TAG(x) r200##x##_24_8
#include "depthtmp.h"


/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = r200_mba_z32( rmesa, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = r200_mba_z32( rmesa, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   d = tmp >> 24;							\
} while (0)

#define TAG(x) r200##x##_24_8
#include "stenciltmp.h"


/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void r200SetBuffer( GLcontext *ctx,
                           GLframebuffer *colorBuffer,
                           GLuint bufferBit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   switch ( bufferBit ) {
   case DD_FRONT_LEFT_BIT:
      if ( rmesa->doPageFlip && rmesa->sarea->pfCurrentPage == 1 ) {
        rmesa->state.pixel.readOffset = rmesa->r200Screen->backOffset;
        rmesa->state.pixel.readPitch  = rmesa->r200Screen->backPitch;
        rmesa->state.color.drawOffset = rmesa->r200Screen->backOffset;
        rmesa->state.color.drawPitch  = rmesa->r200Screen->backPitch;
      } else {
      	rmesa->state.pixel.readOffset = rmesa->r200Screen->frontOffset;
      	rmesa->state.pixel.readPitch  = rmesa->r200Screen->frontPitch;
      	rmesa->state.color.drawOffset = rmesa->r200Screen->frontOffset;
      	rmesa->state.color.drawPitch  = rmesa->r200Screen->frontPitch;
      }
      break;
   case DD_BACK_LEFT_BIT:
      if ( rmesa->doPageFlip && rmesa->sarea->pfCurrentPage == 1 ) {
      	rmesa->state.pixel.readOffset = rmesa->r200Screen->frontOffset;
      	rmesa->state.pixel.readPitch  = rmesa->r200Screen->frontPitch;
      	rmesa->state.color.drawOffset = rmesa->r200Screen->frontOffset;
      	rmesa->state.color.drawPitch  = rmesa->r200Screen->frontPitch;
      } else {
        rmesa->state.pixel.readOffset = rmesa->r200Screen->backOffset;
        rmesa->state.pixel.readPitch  = rmesa->r200Screen->backPitch;
        rmesa->state.color.drawOffset = rmesa->r200Screen->backOffset;
        rmesa->state.color.drawPitch  = rmesa->r200Screen->backPitch;
      }
      break;
   default:
      _mesa_problem(ctx, "Bad bufferBit in %s", __FUNCTION__);
      break;
   }
}

/* Move locking out to get reasonable span performance (10x better
 * than doing this in HW_LOCK above).  WaitForIdle() is the main
 * culprit.
 */

static void r200SpanRenderStart( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   R200_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );
   r200WaitForIdleLocked( rmesa );

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
      volatile int *read_buf = (volatile int *)(rmesa->dri.screen->pFB + 
						rmesa->state.pixel.readOffset);
      p = *read_buf;
      *read_buf = p;
   }
}

static void r200SpanRenderFinish( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( rmesa );
}

void r200InitSpanFuncs( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = r200SetBuffer;

   switch ( rmesa->r200Screen->cpp ) {
   case 2:
      swdd->WriteRGBASpan	= r200WriteRGBASpan_RGB565;
      swdd->WriteRGBSpan	= r200WriteRGBSpan_RGB565;
      swdd->WriteMonoRGBASpan	= r200WriteMonoRGBASpan_RGB565;
      swdd->WriteRGBAPixels	= r200WriteRGBAPixels_RGB565;
      swdd->WriteMonoRGBAPixels	= r200WriteMonoRGBAPixels_RGB565;
      swdd->ReadRGBASpan	= r200ReadRGBASpan_RGB565;
      swdd->ReadRGBAPixels      = r200ReadRGBAPixels_RGB565;
      break;

   case 4:
      swdd->WriteRGBASpan	= r200WriteRGBASpan_ARGB8888;
      swdd->WriteRGBSpan	= r200WriteRGBSpan_ARGB8888;
      swdd->WriteMonoRGBASpan   = r200WriteMonoRGBASpan_ARGB8888;
      swdd->WriteRGBAPixels     = r200WriteRGBAPixels_ARGB8888;
      swdd->WriteMonoRGBAPixels = r200WriteMonoRGBAPixels_ARGB8888;
      swdd->ReadRGBASpan	= r200ReadRGBASpan_ARGB8888;
      swdd->ReadRGBAPixels      = r200ReadRGBAPixels_ARGB8888;
      break;

   default:
      break;
   }

   switch ( rmesa->glCtx->Visual.depthBits ) {
   case 16:
      swdd->ReadDepthSpan	= r200ReadDepthSpan_16;
      swdd->WriteDepthSpan	= r200WriteDepthSpan_16;
      swdd->ReadDepthPixels	= r200ReadDepthPixels_16;
      swdd->WriteDepthPixels	= r200WriteDepthPixels_16;
      break;

   case 24:
      swdd->ReadDepthSpan	= r200ReadDepthSpan_24_8;
      swdd->WriteDepthSpan	= r200WriteDepthSpan_24_8;
      swdd->ReadDepthPixels	= r200ReadDepthPixels_24_8;
      swdd->WriteDepthPixels	= r200WriteDepthPixels_24_8;

      swdd->ReadStencilSpan	= r200ReadStencilSpan_24_8;
      swdd->WriteStencilSpan	= r200WriteStencilSpan_24_8;
      swdd->ReadStencilPixels	= r200ReadStencilPixels_24_8;
      swdd->WriteStencilPixels	= r200WriteStencilPixels_24_8;
      break;

   default:
      break;
   }

   swdd->SpanRenderStart          = r200SpanRenderStart;
   swdd->SpanRenderFinish         = r200SpanRenderFinish; 
}
