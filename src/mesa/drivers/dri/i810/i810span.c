#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810span.h"
#include "i810ioctl.h"
#include "swrast/swrast.h"


#define DBG 0

#define LOCAL_VARS					\
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   i810ScreenPrivate *i810Screen = imesa->i810Screen;	\
   GLuint pitch = i810Screen->backPitch;		\
   GLuint height = dPriv->h;				\
   GLushort p;						\
   char *buf = (char *)(imesa->drawMap +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch);		\
   char *read_buf = (char *)(imesa->readMap +		\
			     dPriv->x * 2 +		\
			     dPriv->y * pitch);		\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS				\
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   i810ScreenPrivate *i810Screen = imesa->i810Screen;	\
   GLuint pitch = i810Screen->backPitch;		\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(i810Screen->depth.map +	\
			dPriv->x * 2 +			\
			dPriv->y * pitch)

#define INIT_MONO_PIXEL(p, color) \
   p = PACK_COLOR_565( color[0], color[1], color[2] )

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

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK()				\
   i810ContextPtr imesa = I810_CONTEXT(ctx);	\
   I810_FIREVERTICES(imesa);			\
   i810DmaFinish(imesa);			\
   LOCK_HARDWARE_QUIESCENT(imesa);

#define HW_CLIPLOOP()						\
  do {								\
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;		\
    int _nc = dPriv->numClipRects;				\
    while (_nc--) {						\
       int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
       int miny = dPriv->pClipRects[_nc].y1 - dPriv->y; 	\
       int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
       int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;


#define HW_ENDCLIPLOOP()			\
    }						\
  } while (0)

#define HW_UNLOCK()				\
    UNLOCK_HARDWARE(imesa);




/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);		\
   rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;				\
   rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;				\
   rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;				\
   rgba[3] = 255;							\
} while(0)

#define TAG(x) i810##x##_565
#include "spantmp.h"



/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = d;

#define READ_DEPTH( d, _x, _y )	\
   d = *(GLushort *)(buf + _x*2 + _y*pitch);

#define TAG(x) i810##x##_16
#include "depthtmp.h"


/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void i810SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                          GLuint bufferBit )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   (void) buffer;

   switch(bufferBit) {
    case FRONT_LEFT_BIT:
      if ( imesa->sarea->pf_current_page == 1)
        imesa->readMap = imesa->i810Screen->back.map;
      else
        imesa->readMap = (char*)imesa->driScreen->pFB;
      break;
    case BACK_LEFT_BIT:
      if ( imesa->sarea->pf_current_page == 1)
        imesa->readMap =  (char*)imesa->driScreen->pFB;
      else
        imesa->readMap = imesa->i810Screen->back.map;
      break;
    default:
      	ASSERT(0);
	break;
   }
   imesa->drawMap = imesa->readMap;
}


void i810InitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = i810SetBuffer;

   swdd->WriteRGBASpan = i810WriteRGBASpan_565;
   swdd->WriteRGBSpan = i810WriteRGBSpan_565;
   swdd->WriteMonoRGBASpan = i810WriteMonoRGBASpan_565;
   swdd->WriteRGBAPixels = i810WriteRGBAPixels_565;
   swdd->WriteMonoRGBAPixels = i810WriteMonoRGBAPixels_565;
   swdd->ReadRGBASpan = i810ReadRGBASpan_565;
   swdd->ReadRGBAPixels = i810ReadRGBAPixels_565;

   swdd->ReadDepthSpan = i810ReadDepthSpan_16;
   swdd->WriteDepthSpan = i810WriteDepthSpan_16;
   swdd->ReadDepthPixels = i810ReadDepthPixels_16;
   swdd->WriteDepthPixels = i810WriteDepthPixels_16;
}
