/* $XFree86$ */
/**************************************************************************

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
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "dri_util.h"

#include "context.h"
#include "utils.h"
#include "imports.h"

#include "sis_context.h"
#include "sis_dri.h"
#include "sis_lock.h"

/* Create the device specific screen private data struct.
 */
static sisScreenPtr
sisCreateScreen( __DRIscreenPrivate *sPriv )
{
   sisScreenPtr sisScreen;
   SISDRIPtr sisDRIPriv = (SISDRIPtr)sPriv->pDevPriv;

   if ( !driCheckDriDdxDrmVersions( sPriv, "SiS", 4, 0, 0, 1, 1, 0 ) )
      return NULL;

   /* Allocate the private area */
   sisScreen = (sisScreenPtr)CALLOC( sizeof(*sisScreen) );
   if ( sisScreen == NULL )
      return NULL;

   sisScreen->screenX = sisDRIPriv->width;
   sisScreen->screenY = sisDRIPriv->height;
   sisScreen->cpp = sisDRIPriv->bytesPerPixel;
   sisScreen->irqEnabled = sisDRIPriv->bytesPerPixel;
   sisScreen->deviceID = sisDRIPriv->deviceID;
   sisScreen->AGPCmdBufOffset = sisDRIPriv->AGPCmdBufOffset;
   sisScreen->AGPCmdBufSize = sisDRIPriv->AGPCmdBufSize;
   sisScreen->sarea_priv_offset = sizeof(XF86DRISAREARec);

   sisScreen->mmio.handle = sisDRIPriv->regs.handle;
   sisScreen->mmio.size   = sisDRIPriv->regs.size;
   if ( drmMap( sPriv->fd, sisScreen->mmio.handle, sisScreen->mmio.size,
	       &sisScreen->mmio.map ) )
   {
      FREE( sisScreen );
      return NULL;
   }

   if (sisDRIPriv->agp.size) {
      sisScreen->agp.handle = sisDRIPriv->agp.handle;
      sisScreen->agp.size   = sisDRIPriv->agp.size;
      if ( drmMap( sPriv->fd, sisScreen->agp.handle, sisScreen->agp.size,
                   &sisScreen->agp.map ) )
      {
         sisScreen->agp.size = 0;
      }
   }

   sisScreen->driScreen = sPriv;

   return sisScreen;
}

/* Destroy the device specific screen private data struct.
 */
static void
sisDestroyScreen( __DRIscreenPrivate *sPriv )
{
   sisScreenPtr sisScreen = (sisScreenPtr)sPriv->private;

   if ( sisScreen == NULL )
      return;

   if (sisScreen->agp.size != 0)
      drmUnmap( sisScreen->agp.map, sisScreen->agp.size );
   drmUnmap( sisScreen->mmio.map, sisScreen->mmio.size );

   FREE( sisScreen );
   sPriv->private = NULL;
}

/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
sisCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 const __GLcontextModes *mesaVis,
                 GLboolean isPixmap )
{
   if (isPixmap)
      return GL_FALSE; /* not implemented */

   driDrawPriv->driverPrivate = (void *)_mesa_create_framebuffer(
				 mesaVis,
				 GL_FALSE,  /* software depth buffer? */
				 mesaVis->stencilBits > 0,
				 mesaVis->accumRedBits > 0,
				 mesaVis->alphaBits > 0 ); /* XXX */
   return (driDrawPriv->driverPrivate != NULL);
}


static void
sisDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}

__inline__ static void
sis_bitblt_copy_cmd (sisContextPtr smesa, ENGPACKET * pkt)
{
   GLint *lpdwDest, *lpdwSrc;
   int i;

   lpdwSrc = (GLint *) pkt;
   lpdwDest = (GLint *) (GET_IOBase (smesa) + REG_SRC_ADDR);

   mWait3DCmdQueue (10);

   for (i = 0; i < 7; i++)
      *lpdwDest++ = *lpdwSrc++;

   MMIO(REG_CMD0, *(GLint *)&pkt->stdwCmd);
   MMIO(REG_QueueLen, -1);
}

static void sisCopyBuffer( __DRIdrawablePrivate *dPriv )
{
   sisContextPtr smesa = (sisContextPtr)dPriv->driContextPriv->driverPrivate;
   int i;
   ENGPACKET stEngPacket;
  
   while ((*smesa->FrameCountPtr) - MMIO_READ(0x8a2c) > SIS_MAX_FRAME_LENGTH)
      usleep(1);

   LOCK_HARDWARE();

   stEngPacket.dwSrcBaseAddr = smesa->backOffset;
   stEngPacket.dwSrcPitch = smesa->backPitch |
      ((smesa->bytesPerPixel == 2) ? 0x80000000 : 0xc0000000);
   stEngPacket.dwDestBaseAddr = 0;
   stEngPacket.wDestPitch = smesa->frontPitch;
   /* TODO: set maximum value? */
   stEngPacket.wDestHeight = smesa->virtualY;

   stEngPacket.stdwCmd.cRop = 0xcc;

   if (smesa->blockWrite)
      stEngPacket.stdwCmd.cCmd0 = CMD0_PAT_FG_COLOR;
   else
      stEngPacket.stdwCmd.cCmd0 = 0;
   stEngPacket.stdwCmd.cCmd1 = CMD1_DIR_X_INC | CMD1_DIR_Y_INC;

   for (i = 0; i < dPriv->numClipRects; i++) {
      XF86DRIClipRectPtr box = &dPriv->pClipRects[i];
      stEngPacket.stdwSrcPos.wY = box->y1 - dPriv->y;
      stEngPacket.stdwSrcPos.wX = box->x1 - dPriv->x;
      stEngPacket.stdwDestPos.wY = box->y1;
      stEngPacket.stdwDestPos.wX = box->x1;

      stEngPacket.stdwDim.wWidth = (GLshort) box->x2 - box->x1;
      stEngPacket.stdwDim.wHeight = (GLshort) box->y2 - box->y1;
      sis_bitblt_copy_cmd( smesa, &stEngPacket );
   }

   *(GLint *)(smesa->IOBase+0x8a2c) = *smesa->FrameCountPtr;
   (*smesa->FrameCountPtr)++;  

   UNLOCK_HARDWARE ();
}


/* Copy the back color buffer to the front color buffer */
static void
sisSwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
         sisContextPtr smesa = (sisContextPtr) dPriv->driContextPriv->driverPrivate;
         GLcontext *ctx = smesa->glCtx;

      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         sisCopyBuffer( dPriv );
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
sisInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) sisCreateScreen( sPriv );

   if ( !sPriv->private ) {
      sisDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}

/* Fullscreen mode change stub
 */
static GLboolean
sisOpenCloseFullScreen( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

static struct __DriverAPIRec sisAPI = {
   .InitDriver      = sisInitDriver,
   .DestroyScreen   = sisDestroyScreen,
   .CreateContext   = sisCreateContext,
   .DestroyContext  = sisDestroyContext,
   .CreateBuffer    = sisCreateBuffer,
   .DestroyBuffer   = sisDestroyBuffer,
   .SwapBuffers     = sisSwapBuffers,
   .MakeCurrent     = sisMakeCurrent,
   .UnbindContext   = sisUnbindContext,
   .OpenFullScreen  = sisOpenCloseFullScreen,
   .CloseFullScreen = sisOpenCloseFullScreen,
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
   psp = __driUtilCreateScreen( dpy, scrn, psc, numConfigs, config, &sisAPI );
   return (void *)psp;
}
#else
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &sisAPI);
   return (void *) psp;
}
#endif
