/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"
#include "via_context.h"
#include "via_span.h"
#include "via_ioctl.h"
#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_DEPTH_VARS                                \
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;   \
    viaScreenPrivate *viaScreen = vmesa->viaScreen;     \
    GLuint pitch = viaScreen->backPitch;                \
    GLuint height = dPriv->h;                           \
    char *buf = (char *)(vmesa->depth.map)

#define CLIPPIXEL(_x,_y) (_x >= minx && _x < maxx &&    \
                          _y >= miny && _y < maxy)


#define CLIPSPAN(_x, _y, _n, _x1, _n1, _i)                                  \
    if (_y < miny || _y >= maxy) {                                          \
        _n1 = 0, _x1 = x;                                                   \
    }                                                                       \
    else {                                                                  \
        _n1 = _n;                                                           \
        _x1 = _x;                                                           \
        if (_x1 < minx) _i += (minx -_x1), n1 -= (minx -_x1), _x1 = minx;   \
        if (_x1 + _n1 >= maxx) n1 -= (_x1 + n1 - maxx);                     \
   }

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK() 
#define HW_CLIPLOOP()								\
    do {									\
        __DRIdrawablePrivate *dPriv = vmesa->driDrawable;			\
        int _nc = dPriv->numClipRects;						\
        while (_nc--) {								\
		int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
        	int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;		\
        	int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
        	int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;        


#define HW_ENDCLIPLOOP()                                            \
        }                                                           \
    } while (0)

#define HW_UNLOCK()

/* 16 bit, 565 rgb color spanline and pixel functions
 */
#undef LOCAL_VARS
#define LOCAL_VARS                                                   	\
    viaContextPtr vmesa = VIA_CONTEXT(ctx);             \
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;                	\
    GLuint pitch = vmesa->drawPitch;                                 	\
    GLuint height = dPriv->h;                                        	\
    GLushort p;                                                      	\
    char *buf, *read_buf;                                            	\
    p = 0;							     	\
    if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {	\
	buf = (char *)(vmesa->drawMap);                              	\
	read_buf = (char *)(vmesa->readMap);                         	\
    }                                                                	\
    else {                                                           	\
	buf = (char *)(vmesa->drawMap +                              	\
                         dPriv->x * 2 +                              	\
                         dPriv->y * pitch);                          	\
	read_buf = (char *)(vmesa->readMap +                         	\
                              dPriv->x * 2 +                         	\
                              dPriv->y * pitch);                     	\
    }

#define INIT_MONO_PIXEL(p, color)                       \
    p = PACK_COLOR_565(color[0], color[1], color[2])
    
#define WRITE_RGBA(_x, _y, r, g, b, a)                                      \
    *(GLushort *)(buf + _x * 2 + _y * pitch) = ((((int)r & 0xf8) << 8) |    \
                                                (((int)g & 0xfc) << 3) |    \
                                                (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL(_x, _y, p)                      \
    *(GLushort *)(buf + _x * 2 + _y * pitch) = p

#define READ_RGBA(rgba, _x, _y)                                             \
    do {                                                                    \
        GLushort p = *(GLushort *)(read_buf + _x * 2 + _y * pitch);         \
        rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;                           \
        rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;                           \
        rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;                           \
        rgba[3] = 255;                                                      \
    } while (0)

#define TAG(x) via##x##_565
#include "spantmp.h"

/* 32 bit, 8888 argb color spanline and pixel functions
 */
#undef LOCAL_VARS
#undef LOCAL_DEPTH_VARS
#undef INIT_MONO_PIXEL
#undef DBG
#define DBG 0
 
#define LOCAL_VARS                                                   	\
    viaContextPtr vmesa = VIA_CONTEXT(ctx);             \
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;                	\
    GLuint pitch = vmesa->drawPitch;                                 	\
    GLuint height = dPriv->h;                                        	\
    GLuint p;                                                        	\
    char *buf, *read_buf;                                            	\
    p = 0;	                                                        \
    if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {	\
	buf = (char *)(vmesa->drawMap);                              	\
	read_buf = (char *)(vmesa->readMap);                         	\
    }                                                                	\
    else {                                                           	\
	buf = (char *)(vmesa->drawMap +                              	\
                         dPriv->x * 4 +                              	\
                         dPriv->y * pitch);                          	\
	read_buf = (char *)(vmesa->readMap +                         	\
                              dPriv->x * 4 +                         	\
                              dPriv->y * pitch);                     	\
    }

#define GET_SRC_PTR(_x, _y) (read_buf + _x * 4 + _y * pitch)
#define GET_DST_PTR(_x, _y) (     buf + _x * 4 + _y * pitch)
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    via##x##_8888
#define TAG2(x,y) via##x##_8888##y
#include "spantmp2.h"


/* 16 bit depthbuffer functions.
 */
#define LOCAL_DEPTH_VARS                                \
    viaContextPtr vmesa = VIA_CONTEXT(ctx);             \
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;   \
    GLuint pitch = vmesa->depth.pitch;                  \
    GLuint height = dPriv->h;                           \
    char *buf = (char *)(vmesa->depth.map)   

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 


#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLushort *)(buf + _x * 2 + _y * pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(GLushort *)(buf + _x * 2 + _y * pitch);

#define TAG(x) via##x##_16
#include "depthtmp.h"

/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLuint *)(buf + _x * 4 + _y * pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(GLuint *)(buf + _x * 4 + _y * pitch);

#define TAG(x) via##x##_32
#include "depthtmp.h"



/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0x000000ff;					\
   tmp |= ((d)<<8);				\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = (*(GLuint *)(buf + (_x)*4 + (_y)*pitch)) >> 8;


#define TAG(x) via##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= (d);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) & 0xff;

#define TAG(x) via##x##_24_8
#include "stenciltmp.h"



static void viaSetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                      GLuint bufferBit)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (bufferBit == DD_FRONT_LEFT_BIT) {
	vmesa->drawMap = (char *)vmesa->driScreen->pFB;
        vmesa->readMap = (char *)vmesa->driScreen->pFB;
	vmesa->drawPitch = vmesa->front.pitch;
	vmesa->readPitch = vmesa->front.pitch;
    }
    else if (bufferBit == DD_BACK_LEFT_BIT) {
	vmesa->drawMap = vmesa->back.map;
        vmesa->readMap = vmesa->back.map;
	vmesa->drawPitch = vmesa->back.pitch;
	vmesa->readPitch = vmesa->back.pitch;	
    }
    else {
        ASSERT(0);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

/* Move locking out to get reasonable span performance.
 */
static void viaSpanRenderStart( GLcontext *ctx )
{
   viaContextPtr vmesa = VIA_CONTEXT(ctx);     
   LOCK_HARDWARE(vmesa);
   viaFlushPrimsLocked(vmesa);
   WAIT_IDLE(vmesa);
    
}

static void viaSpanRenderFinish( GLcontext *ctx )
{
   viaContextPtr vmesa = VIA_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( vmesa );
}

void viaInitSpanFuncs(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

    swdd->SetBuffer = viaSetBuffer;
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (vmesa->viaScreen->bitsPerPixel == 16) {
	swdd->WriteRGBASpan = viaWriteRGBASpan_565;
	swdd->WriteRGBSpan = viaWriteRGBSpan_565;
	swdd->WriteMonoRGBASpan = viaWriteMonoRGBASpan_565;
	swdd->WriteRGBAPixels = viaWriteRGBAPixels_565;
	swdd->WriteMonoRGBAPixels = viaWriteMonoRGBAPixels_565;
	swdd->ReadRGBASpan = viaReadRGBASpan_565;
	swdd->ReadRGBAPixels = viaReadRGBAPixels_565;
    }
    else if (vmesa->viaScreen->bitsPerPixel == 32) {
	viaInitPointers_8888( swdd );
    }
    else {
       fprintf(stderr, "%s: failed\n", __FUNCTION__);
	assert(0);
    }
	
    if (vmesa->glCtx->Visual.depthBits == 16) {
    	swdd->ReadDepthSpan = viaReadDepthSpan_16;
	swdd->WriteDepthSpan = viaWriteDepthSpan_16;
	swdd->WriteMonoDepthSpan = viaWriteMonoDepthSpan_16;
	swdd->ReadDepthPixels = viaReadDepthPixels_16;
	swdd->WriteDepthPixels = viaWriteDepthPixels_16;
    }	
    else if (vmesa->glCtx->Visual.depthBits == 24) {
       fprintf(stderr, "%s: 24/8 span functions\n", __FUNCTION__);
        swdd->ReadDepthSpan = viaReadDepthSpan_24_8;
	swdd->WriteDepthSpan = viaWriteDepthSpan_24_8;
	swdd->ReadDepthPixels = viaReadDepthPixels_24_8;
	swdd->WriteDepthPixels = viaWriteDepthPixels_24_8;

	swdd->WriteStencilSpan = viaWriteStencilSpan_24_8;
	swdd->ReadStencilSpan = viaReadStencilSpan_24_8;
	swdd->WriteStencilPixels = viaWriteStencilPixels_24_8;
	swdd->ReadStencilPixels = viaReadStencilPixels_24_8;
    }
    else if (vmesa->glCtx->Visual.depthBits == 32) {
    	swdd->ReadDepthSpan = viaReadDepthSpan_32;
	swdd->WriteDepthSpan = viaWriteDepthSpan_32;
	swdd->WriteMonoDepthSpan = viaWriteMonoDepthSpan_32;
	swdd->ReadDepthPixels = viaReadDepthPixels_32;
	swdd->WriteDepthPixels = viaWriteDepthPixels_32;
    }

    swdd->SpanRenderStart = viaSpanRenderStart;
    swdd->SpanRenderFinish = viaSpanRenderFinish; 

    
    swdd->WriteCI8Span = NULL;
    swdd->WriteCI32Span = NULL;
    swdd->WriteMonoCISpan = NULL;
    swdd->WriteCI32Pixels = NULL;
    swdd->WriteMonoCIPixels = NULL;
    swdd->ReadCI32Span = NULL;
    swdd->ReadCI32Pixels = NULL;	
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}
