/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_screen.c,v 1.9 2003/03/26 20:43:49 tsi Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
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
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#include "r128_dri.h"

#include "r128_context.h"
#include "r128_ioctl.h"
#include "r128_tris.h"
#include "r128_vb.h"

#include "context.h"
#include "imports.h"

#include "utils.h"
#include "vblank.h"

#ifndef _SOLO
#include "glxextensions.h"
#endif

/* R128 configuration
 */
#include "xmlpool.h"

const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
    DRI_CONF_SECTION_END
#if ENABLE_PERF_BOXES
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_PERFORMANCE_BOXES(false)
    DRI_CONF_SECTION_END
#endif
DRI_CONF_END;
#if ENABLE_PERF_BOXES
static const GLuint __driNConfigOptions = 3;
#else
static const GLuint __driNConfigOptions = 2;
#endif

#if 1
/* Including xf86PciInfo.h introduces a bunch of errors...
 */
#define PCI_CHIP_RAGE128LE	0x4C45
#define PCI_CHIP_RAGE128LF	0x4C46
#define PCI_CHIP_RAGE128PF	0x5046
#define PCI_CHIP_RAGE128PR	0x5052
#define PCI_CHIP_RAGE128RE	0x5245
#define PCI_CHIP_RAGE128RF	0x5246
#define PCI_CHIP_RAGE128RK	0x524B
#define PCI_CHIP_RAGE128RL	0x524C
#endif


/* Create the device specific screen private data struct.
 */
static r128ScreenPtr
r128CreateScreen( __DRIscreenPrivate *sPriv )
{
   r128ScreenPtr r128Screen;
   R128DRIPtr r128DRIPriv = (R128DRIPtr)sPriv->pDevPriv;

   if ( ! driCheckDriDdxDrmVersions( sPriv, "Rage128", 4, 0, 4, 0, 2, 2 ) )
      return NULL;

   /* Allocate the private area */
   r128Screen = (r128ScreenPtr) CALLOC( sizeof(*r128Screen) );
   if ( !r128Screen ) return NULL;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&r128Screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   /* This is first since which regions we map depends on whether or
    * not we are using a PCI card.
    */
   r128Screen->IsPCI = r128DRIPriv->IsPCI;
   r128Screen->sarea_priv_offset = r128DRIPriv->sarea_priv_offset;
   
   if (sPriv->drmMinor >= 3) {
      drm_r128_getparam_t gp;
      int ret;

      gp.param = R128_PARAM_IRQ_NR;
      gp.value = &r128Screen->irq;

      ret = drmCommandWriteRead( sPriv->fd, DRM_R128_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "drmR128GetParam (R128_PARAM_IRQ_NR): %d\n", ret);
         FREE( r128Screen );
         return NULL;
      }
   }

   r128Screen->mmio.handle = r128DRIPriv->registerHandle;
   r128Screen->mmio.size   = r128DRIPriv->registerSize;
   if ( drmMap( sPriv->fd,
		r128Screen->mmio.handle,
		r128Screen->mmio.size,
		(drmAddressPtr)&r128Screen->mmio.map ) ) {
      FREE( r128Screen );
      return NULL;
   }

   r128Screen->buffers = drmMapBufs( sPriv->fd );
   if ( !r128Screen->buffers ) {
      drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );
      FREE( r128Screen );
      return NULL;
   }

   if ( !r128Screen->IsPCI ) {
      r128Screen->agpTextures.handle = r128DRIPriv->agpTexHandle;
      r128Screen->agpTextures.size   = r128DRIPriv->agpTexMapSize;
      if ( drmMap( sPriv->fd,
		   r128Screen->agpTextures.handle,
		   r128Screen->agpTextures.size,
		   (drmAddressPtr)&r128Screen->agpTextures.map ) ) {
	 drmUnmapBufs( r128Screen->buffers );
	 drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );
	 FREE( r128Screen );
	 return NULL;
      }
   }

   switch ( r128DRIPriv->deviceID ) {
   case PCI_CHIP_RAGE128RE:
   case PCI_CHIP_RAGE128RF:
   case PCI_CHIP_RAGE128RK:
   case PCI_CHIP_RAGE128RL:
      r128Screen->chipset = R128_CARD_TYPE_R128;
      break;
   case PCI_CHIP_RAGE128PF:
      r128Screen->chipset = R128_CARD_TYPE_R128_PRO;
      break;
   case PCI_CHIP_RAGE128LE:
   case PCI_CHIP_RAGE128LF:
      r128Screen->chipset = R128_CARD_TYPE_R128_MOBILITY;
      break;
   default:
      r128Screen->chipset = R128_CARD_TYPE_R128;
      break;
   }

   r128Screen->cpp = r128DRIPriv->bpp / 8;
   r128Screen->AGPMode = r128DRIPriv->AGPMode;

   r128Screen->frontOffset	= r128DRIPriv->frontOffset;
   r128Screen->frontPitch	= r128DRIPriv->frontPitch;
   r128Screen->backOffset	= r128DRIPriv->backOffset;
   r128Screen->backPitch	= r128DRIPriv->backPitch;
   r128Screen->depthOffset	= r128DRIPriv->depthOffset;
   r128Screen->depthPitch	= r128DRIPriv->depthPitch;
   r128Screen->spanOffset	= r128DRIPriv->spanOffset;

   r128Screen->texOffset[R128_LOCAL_TEX_HEAP] = r128DRIPriv->textureOffset;
   r128Screen->texSize[R128_LOCAL_TEX_HEAP] = r128DRIPriv->textureSize;
   r128Screen->logTexGranularity[R128_LOCAL_TEX_HEAP] = r128DRIPriv->log2TexGran;

   if ( r128Screen->IsPCI ) {
      r128Screen->numTexHeaps = R128_NR_TEX_HEAPS - 1;
      r128Screen->texOffset[R128_AGP_TEX_HEAP] = 0;
      r128Screen->texSize[R128_AGP_TEX_HEAP] = 0;
      r128Screen->logTexGranularity[R128_AGP_TEX_HEAP] = 0;
   } else {
      r128Screen->numTexHeaps = R128_NR_TEX_HEAPS;
      r128Screen->texOffset[R128_AGP_TEX_HEAP] =
	 r128DRIPriv->agpTexOffset + R128_AGP_TEX_OFFSET;
      r128Screen->texSize[R128_AGP_TEX_HEAP] = r128DRIPriv->agpTexMapSize;
      r128Screen->logTexGranularity[R128_AGP_TEX_HEAP] =
	 r128DRIPriv->log2AGPTexGran;
   }

   r128Screen->driScreen = sPriv;
#ifndef _SOLO
   if ( driCompareGLXAPIVersion( 20030813 ) >= 0 ) {
      PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
          (PFNGLXSCRENABLEEXTENSIONPROC) glXGetProcAddress( (const GLubyte *) "__glXScrEnableExtension" );
      void * const psc = sPriv->psc->screenConfigs;

      if ( glx_enable_extension != NULL ) {
	 if ( r128Screen->irq != 0 ) {
	    (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
	    (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
	    (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
	 }

	 (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
      }
   }
#endif
   return r128Screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
r128DestroyScreen( __DRIscreenPrivate *sPriv )
{
   r128ScreenPtr r128Screen = (r128ScreenPtr)sPriv->private;

   if ( !r128Screen )
      return;

   if ( !r128Screen->IsPCI ) {
      drmUnmap( (drmAddress)r128Screen->agpTextures.map,
		r128Screen->agpTextures.size );
   }
   drmUnmapBufs( r128Screen->buffers );
   drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );

   /* free all option information */
   driDestroyOptionInfo (&r128Screen->optionCache);

   FREE( r128Screen );
   sPriv->private = NULL;
}


/* Initialize the fullscreen mode.
 */
static GLboolean
r128OpenCloseFullScreen( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
r128CreateBuffer( __DRIscreenPrivate *driScrnPriv,
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
r128DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


/* Copy the back color buffer to the front color buffer */
static void
r128SwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      r128ContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (r128ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         if ( rmesa->doPageFlip ) {
            r128PageFlip( dPriv );
         }
         else {
            r128CopyBuffer( dPriv );
         }
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
r128InitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) r128CreateScreen( sPriv );

   if ( !sPriv->private ) {
      r128DestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}

#ifndef _SOLO
/**
 * This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we register new extension functions with the dispatcher.
 *
 * \todo This interface has been deprecated, so we should probably remove
 *       this function before the next XFree86 release.
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

static struct __DriverAPIRec r128API = {
   .InitDriver      = r128InitDriver,
   .DestroyScreen   = r128DestroyScreen,
   .CreateContext   = r128CreateContext,
   .DestroyContext  = r128DestroyContext,
   .CreateBuffer    = r128CreateBuffer,
   .DestroyBuffer   = r128DestroyBuffer,
   .SwapBuffers     = r128SwapBuffers,
   .MakeCurrent     = r128MakeCurrent,
   .UnbindContext   = r128UnbindContext,
   .OpenFullScreen  = r128OpenCloseFullScreen,
   .CloseFullScreen = r128OpenCloseFullScreen,
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
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &r128API);
   return (void *) psp;
}
#else
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &r128API);
   return (void *) psp;
}
#endif
