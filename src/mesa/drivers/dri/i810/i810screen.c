/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810screen.c,v 1.2 2002/10/30 12:51:33 alanh Exp $ */

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810state.h"
#include "i810tex.h"
#include "i810span.h"
#include "i810tris.h"
#include "i810ioctl.h"



/*  static int i810_malloc_proxy_buf(drmBufMapPtr buffers) */
/*  { */
/*     char *buffer; */
/*     drmBufPtr buf; */
/*     int i; */

/*     buffer = CALLOC(I810_DMA_BUF_SZ); */
/*     if(buffer == NULL) return -1; */
/*     for(i = 0; i < I810_DMA_BUF_NR; i++) { */
/*        buf = &(buffers->list[i]); */
/*        buf->address = (drmAddress)buffer; */
/*     } */
/*     return 0; */
/*  } */

static drmBufMapPtr i810_create_empty_buffers(void)
{
   drmBufMapPtr retval;

   retval = (drmBufMapPtr)ALIGN_MALLOC(sizeof(drmBufMap), 32);
   if(retval == NULL) return NULL;
   memset(retval, 0, sizeof(drmBufMap));
   retval->list = (drmBufPtr)ALIGN_MALLOC(sizeof(drmBuf) * I810_DMA_BUF_NR, 32);
   if(retval->list == NULL) {
      FREE(retval);
      return NULL;
   }
   memset(retval->list, 0, sizeof(drmBuf) * I810_DMA_BUF_NR);
   return retval;
}


static GLboolean
i810InitDriver(__DRIscreenPrivate *sPriv)
{
   i810ScreenPrivate *i810Screen;
   I810DRIPtr         gDRIPriv = (I810DRIPtr)sPriv->pDevPriv;

   /* Check the DRI externsion version */
   if ( sPriv->driMajor != 4 || sPriv->driMinor < 0 ) {
      __driUtilMessage( "i810 DRI driver expected DRI version 4.0.x "
                        "but got version %d.%d.%d",
                        sPriv->driMajor, sPriv->driMinor, sPriv->driPatch );
      return GL_FALSE;
   }

   /* Check that the DDX driver version is compatible */
   if (sPriv->ddxMajor != 1 ||
       sPriv->ddxMinor < 0) {
      __driUtilMessage("i810 DRI driver expected DDX driver version 1.0.x but got version %d.%d.%d", sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch);
      return GL_FALSE;
   }

   /* Check that the DRM driver version is compatible */
   if (sPriv->drmMajor != 1 ||
       sPriv->drmMinor < 2) {
      __driUtilMessage("i810 DRI driver expected DRM driver version 1.2.x but got version %d.%d.%d", sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch);
      return GL_FALSE;
   }

   /* Allocate the private area */
   i810Screen = (i810ScreenPrivate *)CALLOC(sizeof(i810ScreenPrivate));
   if (!i810Screen) {
      __driUtilMessage("i810InitDriver: alloc i810ScreenPrivate struct failed");
      return GL_FALSE;
   }

   i810Screen->driScrnPriv = sPriv;
   sPriv->private = (void *)i810Screen;

   i810Screen->deviceID=gDRIPriv->deviceID;
   i810Screen->width=gDRIPriv->width;
   i810Screen->height=gDRIPriv->height;
   i810Screen->mem=gDRIPriv->mem;
   i810Screen->cpp=gDRIPriv->cpp;
   i810Screen->fbStride=gDRIPriv->fbStride;
   i810Screen->fbOffset=gDRIPriv->fbOffset;

   if (gDRIPriv->bitsPerPixel == 15)
      i810Screen->fbFormat = DV_PF_555;
   else
      i810Screen->fbFormat = DV_PF_565;

   i810Screen->backOffset=gDRIPriv->backOffset;
   i810Screen->depthOffset=gDRIPriv->depthOffset;
   i810Screen->backPitch = gDRIPriv->auxPitch;
   i810Screen->backPitchBits = gDRIPriv->auxPitchBits;
   i810Screen->textureOffset=gDRIPriv->textureOffset;
   i810Screen->textureSize=gDRIPriv->textureSize;
   i810Screen->logTextureGranularity = gDRIPriv->logTextureGranularity;

   i810Screen->bufs = i810_create_empty_buffers();
   if (i810Screen->bufs == NULL) {
      __driUtilMessage("i810InitDriver: i810_create_empty_buffers() failed");
      FREE(i810Screen);
      return GL_FALSE;
   }

   i810Screen->back.handle = gDRIPriv->backbuffer;
   i810Screen->back.size = gDRIPriv->backbufferSize;

   if (drmMap(sPriv->fd,
	      i810Screen->back.handle,
	      i810Screen->back.size,
	      (drmAddress *)&i810Screen->back.map) != 0) {
      FREE(i810Screen);
      sPriv->private = NULL;
      __driUtilMessage("i810InitDriver: drmMap failed");
      return GL_FALSE;
   }

   i810Screen->depth.handle = gDRIPriv->depthbuffer;
   i810Screen->depth.size = gDRIPriv->depthbufferSize;

   if (drmMap(sPriv->fd,
	      i810Screen->depth.handle,
	      i810Screen->depth.size,
	      (drmAddress *)&i810Screen->depth.map) != 0) {
      FREE(i810Screen);
      drmUnmap(i810Screen->back.map, i810Screen->back.size);
      sPriv->private = NULL;
      __driUtilMessage("i810InitDriver: drmMap (2) failed");
      return GL_FALSE;
   }

   i810Screen->tex.handle = gDRIPriv->textures;
   i810Screen->tex.size = gDRIPriv->textureSize;

   if (drmMap(sPriv->fd,
	      i810Screen->tex.handle,
	      i810Screen->tex.size,
	      (drmAddress *)&i810Screen->tex.map) != 0) {
      FREE(i810Screen);
      drmUnmap(i810Screen->back.map, i810Screen->back.size);
      drmUnmap(i810Screen->depth.map, i810Screen->depth.size);
      sPriv->private = NULL;
      __driUtilMessage("i810InitDriver: drmMap (3) failed");
      return GL_FALSE;
   }

   i810Screen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;

   return GL_TRUE;
}

static void
i810DestroyScreen(__DRIscreenPrivate *sPriv)
{
   i810ScreenPrivate *i810Screen = (i810ScreenPrivate *)sPriv->private;

   /* Need to unmap all the bufs and maps here:
    */
   drmUnmap(i810Screen->back.map, i810Screen->back.size);
   drmUnmap(i810Screen->depth.map, i810Screen->depth.size);
   drmUnmap(i810Screen->tex.map, i810Screen->tex.size);

   FREE(i810Screen);
   sPriv->private = NULL;
}


static GLboolean
i810CreateBuffer( __DRIscreenPrivate *driScrnPriv,
                  __DRIdrawablePrivate *driDrawPriv,
                  const __GLcontextModes *mesaVis,
                  GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      driDrawPriv->driverPrivate = (void *)
         _mesa_create_framebuffer(mesaVis,
                                  GL_FALSE,  /* software depth buffer? */
                                  mesaVis->stencilBits > 0,
                                  mesaVis->accumRedBits > 0,
                                  GL_FALSE /* s/w alpha planes */);
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
i810DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


static GLboolean
i810OpenCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}

static const struct __DriverAPIRec i810API = {
   .InitDriver      = i810InitDriver,
   .DestroyScreen   = i810DestroyScreen,
   .CreateContext   = i810CreateContext,
   .DestroyContext  = i810DestroyContext,
   .CreateBuffer    = i810CreateBuffer,
   .DestroyBuffer   = i810DestroyBuffer,
   .SwapBuffers     = i810SwapBuffers,
   .MakeCurrent     = i810MakeCurrent,
   .UnbindContext   = i810UnbindContext,
   .OpenFullScreen  = i810OpenCloseFullScreen,
   .CloseFullScreen = i810OpenCloseFullScreen,
   .GetSwapInfo     = NULL,
   .GetMSC          = NULL,
   .WaitForMSC      = NULL,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};


/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
#ifndef _SOLO
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &i810API);
   return (void *) psp;
}
#else
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &i810API);
   return (void *) psp;
}
#endif
