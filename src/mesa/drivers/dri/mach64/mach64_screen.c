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
#include "mach64_tris.h"
#include "mach64_vb.h"

#include "context.h"
#include "imports.h"

#include "utils.h"
#include "vblank.h"

#ifndef _SOLO
#include "glxextensions.h"
#endif

/* Mach64 configuration
 */
#include "xmlpool.h"

const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
#if ENABLE_PERF_BOXES
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_PERFORMANCE_BOXES(false)
    DRI_CONF_SECTION_END
#endif
DRI_CONF_END;
#if ENABLE_PERF_BOXES
static const GLuint __driNConfigOptions = 2;
#else
static const GLuint __driNConfigOptions = 1;
#endif


/* Create the device specific screen private data struct.
 */
static mach64ScreenRec *
mach64CreateScreen( __DRIscreenPrivate *sPriv )
{
   mach64ScreenPtr mach64Screen;
   ATIDRIPtr serverInfo = (ATIDRIPtr)sPriv->pDevPriv;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_DRI ) 
      fprintf( stderr, "%s\n", __FUNCTION__ );

   if ( ! driCheckDriDdxDrmVersions( sPriv, "Mach64", 4, 0, 6, 4, 1, 0 ) )
      return NULL;

   /* Allocate the private area */
   mach64Screen = (mach64ScreenPtr) CALLOC( sizeof(*mach64Screen) );
   if ( !mach64Screen ) return NULL;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&mach64Screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   mach64Screen->IsPCI = serverInfo->IsPCI;

   {
      drmMach64GetParam gp;
      int ret;

      gp.param = MACH64_PARAM_IRQ_NR;
      gp.value = &mach64Screen->irq;

      ret = drmCommandWriteRead( sPriv->fd, DRM_MACH64_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "DRM_MACH64_GETPARAM (MACH64_PARAM_IRQ_NR): %d\n", ret);
         FREE( mach64Screen );
         return NULL;
      }
   }

   mach64Screen->mmio.handle = serverInfo->regs;
   mach64Screen->mmio.size   = serverInfo->regsSize;
   if ( drmMap( sPriv->fd,
		mach64Screen->mmio.handle,
		mach64Screen->mmio.size,
		(drmAddressPtr)&mach64Screen->mmio.map ) != 0 ) {
      FREE( mach64Screen );
      return NULL;
   }

   mach64Screen->buffers = drmMapBufs( sPriv->fd );
   if ( !mach64Screen->buffers ) {
      drmUnmap( (drmAddress)mach64Screen->mmio.map,
		mach64Screen->mmio.size );
      FREE( mach64Screen );
      return NULL;
   }

   if ( !mach64Screen->IsPCI ) {
      mach64Screen->agpTextures.handle = serverInfo->agp;
      mach64Screen->agpTextures.size   = serverInfo->agpSize;
      if ( drmMap( sPriv->fd,
		   mach64Screen->agpTextures.handle,
		   mach64Screen->agpTextures.size,
		   (drmAddressPtr)&mach64Screen->agpTextures.map ) ) {
	 drmUnmapBufs( mach64Screen->buffers );
	 drmUnmap( (drmAddress)mach64Screen->mmio.map, mach64Screen->mmio.size );
	 FREE( mach64Screen );
	 return NULL;
      }
   }

   mach64Screen->AGPMode	= serverInfo->AGPMode;

   mach64Screen->chipset	= serverInfo->chipset;
   mach64Screen->width		= serverInfo->width;
   mach64Screen->height		= serverInfo->height;
   mach64Screen->mem		= serverInfo->mem;
   mach64Screen->cpp		= serverInfo->cpp;

   mach64Screen->frontOffset	= serverInfo->frontOffset;
   mach64Screen->frontPitch	= serverInfo->frontPitch;
   mach64Screen->backOffset	= serverInfo->backOffset;
   mach64Screen->backPitch	= serverInfo->backPitch;
   mach64Screen->depthOffset	= serverInfo->depthOffset;
   mach64Screen->depthPitch	= serverInfo->depthPitch;

   mach64Screen->texOffset[MACH64_CARD_HEAP] = serverInfo->textureOffset;
   mach64Screen->texSize[MACH64_CARD_HEAP] = serverInfo->textureSize;
   mach64Screen->logTexGranularity[MACH64_CARD_HEAP] =
      serverInfo->logTextureGranularity;

   if ( mach64Screen->IsPCI ) {
      mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS - 1;
      mach64Screen->firstTexHeap = MACH64_CARD_HEAP;
      mach64Screen->texOffset[MACH64_AGP_HEAP] = 0;
      mach64Screen->texSize[MACH64_AGP_HEAP] = 0;
      mach64Screen->logTexGranularity[MACH64_AGP_HEAP] = 0;
   } else {
      if (mach64Screen->texSize[MACH64_CARD_HEAP] > 0) {
	 mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS;
	 mach64Screen->firstTexHeap = MACH64_CARD_HEAP;
      } else {
	 mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS - 1;
	 mach64Screen->firstTexHeap = MACH64_AGP_HEAP;
      }
      mach64Screen->texOffset[MACH64_AGP_HEAP] = serverInfo->agpTextureOffset;
      mach64Screen->texSize[MACH64_AGP_HEAP] = serverInfo->agpSize;
      mach64Screen->logTexGranularity[MACH64_AGP_HEAP] = serverInfo->logAgpTextureGranularity;
   }

   mach64Screen->driScreen = sPriv;
#ifndef _SOLO
   if ( driCompareGLXAPIVersion( 20030813 ) >= 0 ) {
      PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
          (PFNGLXSCRENABLEEXTENSIONPROC) glXGetProcAddress( (const GLubyte *) "__glXScrEnableExtension" );
      void * const psc = sPriv->psc->screenConfigs;

      if ( glx_enable_extension != NULL ) {
	 if ( mach64Screen->irq != 0 ) {
	    (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
	    (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
	    (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
	 }

	 (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
      }
   }
#endif
   return mach64Screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
mach64DestroyScreen( __DRIscreenPrivate *driScreen )
{
   mach64ScreenRec *mach64Screen = (mach64ScreenRec *) driScreen->private;

   if ( !mach64Screen )
      return;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_DRI ) 
      fprintf( stderr, "%s\n", __FUNCTION__ );

   if ( !mach64Screen->IsPCI ) {
      drmUnmap( (drmAddress)mach64Screen->agpTextures.map,
		mach64Screen->agpTextures.size );
   }

   drmUnmapBufs( mach64Screen->buffers );
   drmUnmap( (drmAddress)mach64Screen->mmio.map, mach64Screen->mmio.size );

   FREE( mach64Screen );
   driScreen->private = NULL;
}

/* Initialize the fullscreen mode.
 */
static GLboolean
mach64OpenFullScreen( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

/* Shut down the fullscreen mode.
 */
static GLboolean
mach64CloseFullScreen( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
mach64CreateBuffer( __DRIscreenPrivate *driScrnPriv,
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
                                   GL_FALSE,  /* software depth buffer? */
                                   mesaVis->stencilBits > 0,
                                   mesaVis->accumRedBits > 0,
                                   mesaVis->alphaBits > 0 );
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
mach64DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


/* Copy the back color buffer to the front color buffer */
static void
mach64SwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      mach64ContextPtr mmesa;
      GLcontext *ctx;
      mmesa = (mach64ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = mmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         mach64CopyBuffer( dPriv );
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
mach64InitDriver( __DRIscreenPrivate *driScreen )
{
   driScreen->private = (void *) mach64CreateScreen( driScreen );

   if ( !driScreen->private ) {
      mach64DestroyScreen( driScreen );
      return GL_FALSE;
   }

   return GL_TRUE;
}

#ifndef _SOLO
/* This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we register new extension functions with the dispatcher.
 */
void __driRegisterExtensions( void )
{
   PFNGLXENABLEEXTENSIONPROC glx_enable_extension;

   if ( driCompareGLXAPIVersion( 20030317 ) >= 0 ) {
      glx_enable_extension = (PFNGLXENABLEEXTENSIONPROC)
	  glXGetProcAddress( (const GLubyte *) "__glXEnableExtension" );

      if ( glx_enable_extension != NULL ) {
	 glx_enable_extension( "GLX_SGI_swap_control", GL_FALSE );
	 glx_enable_extension( "GLX_SGI_video_sync", GL_FALSE );
	 glx_enable_extension( "GLX_MESA_swap_control", GL_FALSE );
      }
   }
}
#endif

static struct __DriverAPIRec mach64API = {
   .InitDriver      = mach64InitDriver,
   .DestroyScreen   = mach64DestroyScreen,
   .CreateContext   = mach64CreateContext,
   .DestroyContext  = mach64DestroyContext,
   .CreateBuffer    = mach64CreateBuffer,
   .DestroyBuffer   = mach64DestroyBuffer,
   .SwapBuffers     = mach64SwapBuffers,
   .MakeCurrent     = mach64MakeCurrent,
   .UnbindContext   = mach64UnbindContext,
   .OpenFullScreen  = mach64OpenFullScreen,
   .CloseFullScreen = mach64CloseFullScreen,
   .GetSwapInfo     = NULL,
   .GetMSC          = driGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
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
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &mach64API);
   return (void *) psp;
}
#else
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &mach64API);
   return (void *) psp;
}
#endif
