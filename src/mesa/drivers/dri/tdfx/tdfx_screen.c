/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
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
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_screen.c,v 1.3 2002/02/22 21:45:03 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#include "tdfx_dri.h"
#include "tdfx_context.h"
#include "tdfx_lock.h"
#include "tdfx_vb.h"
#include "tdfx_tris.h"


#ifdef DEBUG_LOCKING
char *prevLockFile = 0;
int prevLockLine = 0;
#endif

#ifndef TDFX_DEBUG
int TDFX_DEBUG = (0
/*  		  | DEBUG_ALWAYS_SYNC */
/*		  | DEBUG_VERBOSE_API */
/*		  | DEBUG_VERBOSE_MSG */
/*		  | DEBUG_VERBOSE_LRU */
/*  		  | DEBUG_VERBOSE_DRI */
/*  		  | DEBUG_VERBOSE_IOCTL */
/*   		  | DEBUG_VERBOSE_2D */
   );
#endif



static GLboolean
tdfxCreateScreen( __DRIscreenPrivate *sPriv )
{
   tdfxScreenPrivate *fxScreen;
   TDFXDRIPtr fxDRIPriv = (TDFXDRIPtr) sPriv->pDevPriv;

   /* Allocate the private area */
   fxScreen = (tdfxScreenPrivate *) CALLOC( sizeof(tdfxScreenPrivate) );
   if ( !fxScreen )
      return GL_FALSE;

   fxScreen->driScrnPriv = sPriv;
   sPriv->private = (void *) fxScreen;

   fxScreen->regs.handle	= fxDRIPriv->regs;
   fxScreen->regs.size		= fxDRIPriv->regsSize;
   fxScreen->deviceID		= fxDRIPriv->deviceID;
   fxScreen->width		= fxDRIPriv->width;
   fxScreen->height		= fxDRIPriv->height;
   fxScreen->mem		= fxDRIPriv->mem;
   fxScreen->cpp		= fxDRIPriv->cpp;
   fxScreen->stride		= fxDRIPriv->stride;
   fxScreen->fifoOffset		= fxDRIPriv->fifoOffset;
   fxScreen->fifoSize		= fxDRIPriv->fifoSize;
   fxScreen->fbOffset		= fxDRIPriv->fbOffset;
   fxScreen->backOffset		= fxDRIPriv->backOffset;
   fxScreen->depthOffset	= fxDRIPriv->depthOffset;
   fxScreen->textureOffset	= fxDRIPriv->textureOffset;
   fxScreen->textureSize	= fxDRIPriv->textureSize;
   fxScreen->sarea_priv_offset	= fxDRIPriv->sarea_priv_offset;

   if ( drmMap( sPriv->fd, fxScreen->regs.handle,
		fxScreen->regs.size, &fxScreen->regs.map ) ) {
      return GL_FALSE;
   }

   return GL_TRUE;
}


static void
tdfxDestroyScreen( __DRIscreenPrivate *sPriv )
{
   tdfxScreenPrivate *fxScreen = (tdfxScreenPrivate *) sPriv->private;

   if ( fxScreen ) {
      drmUnmap( fxScreen->regs.map, fxScreen->regs.size );

      FREE( fxScreen );
      sPriv->private = NULL;
   }
}


static GLboolean
tdfxInitDriver( __DRIscreenPrivate *sPriv )
{
   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)sPriv );
   }

   /* Check the DRI externsion version */
   if ( sPriv->driMajor != 4 || sPriv->driMinor < 0 ) {
       __driUtilMessage( "tdfx DRI driver expected DRI version 4.0.x "
                         "but got version %d.%d.%d",
                         sPriv->driMajor, sPriv->driMinor, sPriv->driPatch );
       return GL_FALSE;
   }

   /* Check that the DDX driver version is compatible */
   if ( sPriv->ddxMajor != 1 ||
	sPriv->ddxMinor < 0 ) {
      __driUtilMessage(
	       "3dfx DRI driver expected DDX driver version 1.0.x "
	       "but got version %d.%d.%d",
	       sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch );
      return GL_FALSE;
   }

   /* Check that the DRM driver version is compatible */
   if ( sPriv->drmMajor != 1 ||
	sPriv->drmMinor < 0 ) {
      __driUtilMessage(
	       "3dfx DRI driver expected DRM driver version 1.0.x "
	       "but got version %d.%d.%d",
	       sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch );
      return GL_FALSE;
   }

   if ( !tdfxCreateScreen( sPriv ) ) {
      tdfxDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}


static GLboolean
tdfxCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                  __DRIdrawablePrivate *driDrawPriv,
                  const __GLcontextModes *mesaVis,
                  GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      driDrawPriv->driverPrivate = (void *) 
         _mesa_create_framebuffer( mesaVis,
                                   GL_FALSE, /* software depth buffer? */
                                   mesaVis->stencilBits > 0,
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE /* software alpha channel? */ );
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
tdfxDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


static void
tdfxSwapBuffers( __DRIdrawablePrivate *driDrawPriv )

{
   GET_CURRENT_CONTEXT(ctx);
   tdfxContextPtr fxMesa = 0;
   GLframebuffer *mesaBuffer;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)driDrawPriv );
   }

   mesaBuffer = (GLframebuffer *) driDrawPriv->driverPrivate;
   if ( !mesaBuffer->Visual.doubleBufferMode )
      return; /* can't swap a single-buffered window */

   /* If the current context's drawable matches the given drawable
    * we have to do a glFinish (per the GLX spec).
    */
   if ( ctx ) {
      __DRIdrawablePrivate *curDrawPriv;
      fxMesa = TDFX_CONTEXT(ctx);
      curDrawPriv = fxMesa->driContext->driDrawablePriv;

      if ( curDrawPriv == driDrawPriv ) {
	 /* swapping window bound to current context, flush first */
	 _mesa_notifySwapBuffers( ctx );
	 LOCK_HARDWARE( fxMesa );
      }
      else {
         /* find the fxMesa context previously bound to the window */
	 fxMesa = (tdfxContextPtr) driDrawPriv->driContextPriv->driverPrivate;
         if (!fxMesa)
            return;
	 LOCK_HARDWARE( fxMesa );
	 fxMesa->Glide.grSstSelect( fxMesa->Glide.Board );
         printf("SwapBuf SetState 1\n");
	 fxMesa->Glide.grGlideSetState(fxMesa->Glide.State );
      }
   }

#ifdef STATS
   {
      int stalls;
      static int prevStalls = 0;

      stalls = fxMesa->Glide.grFifoGetStalls();

      fprintf( stderr, "%s:\n", __FUNCTION__ );
      if ( stalls != prevStalls ) {
	 fprintf( stderr, "    %d stalls occurred\n",
		  stalls - prevStalls );
	 prevStalls = stalls;
      }
      if ( fxMesa && fxMesa->texSwaps ) {
	 fprintf( stderr, "    %d texture swaps occurred\n",
		  fxMesa->texSwaps );
	 fxMesa->texSwaps = 0;
      }
   }
#endif

   if (fxMesa->scissoredClipRects) {
      /* restore clip rects without scissor box */
      fxMesa->Glide.grDRIPosition( driDrawPriv->x, driDrawPriv->y,
                                   driDrawPriv->w, driDrawPriv->h,
                                   driDrawPriv->numClipRects,
                                   driDrawPriv->pClipRects );
   }

   fxMesa->Glide.grDRIBufferSwap( fxMesa->Glide.SwapInterval );

   if (fxMesa->scissoredClipRects) {
      /* restore clip rects WITH scissor box */
      fxMesa->Glide.grDRIPosition( driDrawPriv->x, driDrawPriv->y,
                                   driDrawPriv->w, driDrawPriv->h,
                                   fxMesa->numClipRects, fxMesa->pClipRects );
   }


#if 0
   {
      FxI32 result;
      do {
         FxI32 result;
         fxMesa->Glide.grGet(GR_PENDING_BUFFERSWAPS, 4, &result);
      } while ( result > fxMesa->maxPendingSwapBuffers );
   }
#endif

   fxMesa->stats.swapBuffer++;

   if (ctx) {
      if (ctx->DriverCtx != fxMesa) {
         fxMesa = TDFX_CONTEXT(ctx);
	 fxMesa->Glide.grSstSelect( fxMesa->Glide.Board );
         printf("SwapBuf SetState 2\n");
	 fxMesa->Glide.grGlideSetState(fxMesa->Glide.State );
      }
      UNLOCK_HARDWARE( fxMesa );
   }
}


static GLboolean
tdfxOpenCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}


static const struct __DriverAPIRec tdfxAPI = {
   .InitDriver      = tdfxInitDriver,
   .DestroyScreen   = tdfxDestroyScreen,
   .CreateContext   = tdfxCreateContext,
   .DestroyContext  = tdfxDestroyContext,
   .CreateBuffer    = tdfxCreateBuffer,
   .DestroyBuffer   = tdfxDestroyBuffer,
   .SwapBuffers     = tdfxSwapBuffers,
   .MakeCurrent     = tdfxMakeCurrent,
   .UnbindContext   = tdfxUnbindContext,
   .OpenFullScreen  = tdfxOpenCloseFullScreen,
   .CloseFullScreen = tdfxOpenCloseFullScreen,
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
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &tdfxAPI);
   return (void *) psp;
}
#else
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &tdfxAPI);
   return (void *) psp;
}
#endif
