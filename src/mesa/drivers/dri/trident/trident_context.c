/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */
#include "trident_dri.h"
#include "trident_context.h"
#include "trident_lock.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "context.h"
#include "simple_list.h"
#include "mem.h"
#include "matrix.h"
#include "extensions.h"
#if defined(USE_X86_ASM)
#include "X86/common_x86_asm.h"
#endif
#include "simple_list.h"
#include "mem.h"
#include "mm.h"

static const struct gl_pipeline_stage *trident_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
   &_tnl_render_stage,		
   0,
};


GLboolean tridentCreateContext( Display *dpy, const __GLcontextModes *glVisual,
			     __DRIcontextPrivate *driContextPriv,
                     	     void *sharedContextPrivate)
{
   GLcontext *ctx, *shareCtx;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   tridentContextPtr tmesa;
   tridentScreenPtr tridentscrn;
#if 0
   drm_trident_sarea_t *saPriv=(drm_trident_sarea_t *)(((char*)sPriv->pSAREA)+
						 sizeof(XF86DRISAREARec));
#endif

   tmesa = (tridentContextPtr) CALLOC( sizeof(*tmesa) );
   if ( !tmesa ) return GL_FALSE;

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((tridentContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   tmesa->glCtx = _mesa_create_context(glVisual, shareCtx, tmesa, GL_TRUE);
   if (!tmesa->glCtx) {
      FREE(tmesa);
      return GL_FALSE;
   }

   tmesa->display = dpy;

   tmesa->driContext = driContextPriv;
   tmesa->driScreen = sPriv;
   tmesa->driDrawable = NULL; /* Set by XMesaMakeCurrent */

   tmesa->hHWContext = driContextPriv->hHWContext;
   tmesa->driHwLock = &sPriv->pSAREA->lock;
   tmesa->driFd = sPriv->fd;
#if 0
   tmesa->sarea = saPriv;
#endif

   tridentscrn = tmesa->tridentScreen = (tridentScreenPtr)(sPriv->private);

   ctx = tmesa->glCtx;

   ctx->Const.MaxTextureLevels = 13;  /* 4K by 4K?  Is that right? */
   ctx->Const.MaxTextureUnits = 1; /* Permedia 3 */

   ctx->Const.MinLineWidth = 0.0;
   ctx->Const.MaxLineWidth = 255.0;

   ctx->Const.MinLineWidthAA = 0.0;
   ctx->Const.MaxLineWidthAA = 65536.0;

   ctx->Const.MinPointSize = 0.0;
   ctx->Const.MaxPointSize = 255.0;

   ctx->Const.MinPointSizeAA = 0.5; /* 4x4 quality mode */
   ctx->Const.MaxPointSizeAA = 16.0; 
   ctx->Const.PointSizeGranularity = 0.25;

#if 0
   tmesa->texHeap = mmInit( 0, tmesa->tridentScreen->textureSize );

   make_empty_list(&tmesa->TexObjList);
   make_empty_list(&tmesa->SwappedOut);

   tmesa->CurrentTexObj[0] = 0;
   tmesa->CurrentTexObj[1] = 0; /* Permedia 3, second texture */

   tmesa->RenderIndex = ~0;
#endif

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, trident_pipeline );

   /* Configure swrast to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   tridentInitVB( ctx );
   tridentDDInitExtensions( ctx );
   tridentDDInitDriverFuncs( ctx );
   tridentDDInitStateFuncs( ctx );
#if 0
   tridentDDInitSpanFuncs( ctx );
   tridentDDInitTextureFuncs( ctx );
#endif
   tridentDDInitTriFuncs( ctx );
   tridentDDInitState( tmesa );

    driContextPriv->driverPrivate = (void *)tmesa;

   UNLOCK_HARDWARE(tmesa);

    return GL_TRUE;
}

static void 
tridentDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    tridentContextPtr tmesa = (tridentContextPtr)driContextPriv->driverPrivate;

    if (tmesa) {
      _swsetup_DestroyContext( tmesa->glCtx );
      _tnl_DestroyContext( tmesa->glCtx );
      _ac_DestroyContext( tmesa->glCtx );
      _swrast_DestroyContext( tmesa->glCtx );

      /* free the Mesa context */
      tmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(tmesa->glCtx);

      Xfree(tmesa);
      driContextPriv->driverPrivate = NULL;
    }
}


static GLboolean
tridentCreateBuffer( Display *dpy,
                   __DRIscreenPrivate *driScrnPriv,
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
                                  mesaVis->alphaBits > 0
                                  );
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
tridentDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}

static void
tridentSwapBuffers(Display *dpy, void *drawablePrivate)
{
   __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
   (void) dpy;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      tridentContextPtr tmesa;
      GLcontext *ctx;
      tmesa = (tridentContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = tmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_swapbuffers( ctx );  /* flush pending rendering comands */
         tridentCopyBuffer( dPriv );
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "tridentSwapBuffers: drawable has no context!\n");
   }
}

static GLboolean 
tridentMakeCurrent(__DRIcontextPrivate *driContextPriv,
		 __DRIdrawablePrivate *driDrawPriv,
		 __DRIdrawablePrivate *driReadPriv)
{
    if (driContextPriv) {
	GET_CURRENT_CONTEXT(ctx);
	tridentContextPtr oldCtx = ctx ? TRIDENT_CONTEXT(ctx) : NULL;
	tridentContextPtr newCtx = (tridentContextPtr) driContextPriv->driverPrivate;

	if ( newCtx != oldCtx ) {
	    newCtx->dirty = ~0;
	}

	if (newCtx->driDrawable != driDrawPriv) {
	    newCtx->driDrawable = driDrawPriv;
#if 0
	    tridentUpdateWindow ( newCtx->glCtx );
	    tridentUpdateViewportOffset( newCtx->glCtx );
#endif
	}

   newCtx->drawOffset = newCtx->tridentScreen->backOffset;
   newCtx->drawPitch = newCtx->tridentScreen->backPitch;

	_mesa_make_current2( newCtx->glCtx, 
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate );

	if (!newCtx->glCtx->Viewport.Width) {
	    _mesa_set_viewport(newCtx->glCtx, 0, 0, 
					driDrawPriv->w, driDrawPriv->h);
	}
    } else {
	_mesa_make_current( 0, 0 );
    }
    return GL_TRUE;
}


static GLboolean 
tridentUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

static GLboolean
tridentOpenFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}

static GLboolean
tridentCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}


tridentScreenPtr tridentCreateScreen( __DRIscreenPrivate *sPriv )
{
   TRIDENTDRIPtr tDRIPriv = (TRIDENTDRIPtr)sPriv->pDevPriv;
   tridentScreenPtr tridentScreen;
   int i;

#if 0
   /* Check the DRI version */
   {
      int major, minor, patch;
      if ( XF86DRIQueryVersion( sPriv->display, &major, &minor, &patch ) ) {
         if ( major != 3 || minor != 1 || patch < 0 ) {
	    __driUtilMessage( "r128 DRI driver expected DRI version 3.1.x but got version %d.%d.%d", major, minor, patch );
            return GL_FALSE;
         }
      }
   }

   /* Check that the DDX driver version is compatible */
   if ( sPriv->ddxMajor != 4 ||
	sPriv->ddxMinor != 0 ||
	sPriv->ddxPatch < 0 ) {
      __driUtilMessage( "r128 DRI driver expected DDX driver version 4.0.x but got version %d.%d.%d", sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch );
      return GL_FALSE;
   }

   /* Check that the DRM driver version is compatible */
   if ( sPriv->drmMajor != 2 ||
	sPriv->drmMinor != 1 ||
	sPriv->drmPatch < 0 ) {
      __driUtilMessage( "r128 DRI driver expected DRM driver version 2.1.x but got version %d.%d.%d", sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch );
      return GL_FALSE;
   }
#endif

    /* Allocate the private area */
    tridentScreen = (tridentScreenPtr) CALLOC( sizeof(*tridentScreen) );
    if ( !tridentScreen ) return NULL;

   tridentScreen->driScreen = sPriv;

   tridentScreen->frontOffset = tDRIPriv->frontOffset;
   tridentScreen->backOffset = tDRIPriv->backOffset;
   tridentScreen->depthOffset = tDRIPriv->depthOffset;
   tridentScreen->frontPitch = tDRIPriv->frontPitch;
   tridentScreen->backPitch = tDRIPriv->backPitch;
   tridentScreen->depthPitch = tDRIPriv->depthPitch;
   tridentScreen->width = tDRIPriv->width;
   tridentScreen->height = tDRIPriv->height;

printf("%d %d\n",tridentScreen->width,tridentScreen->height);
printf("%d %d\n",tridentScreen->frontPitch,tridentScreen->backPitch);
printf("offset 0x%x 0x%x\n",tridentScreen->backOffset,tridentScreen->depthOffset);

   tridentScreen->mmio.handle = tDRIPriv->regs;
   tridentScreen->mmio.size = 0x20000;
    
   if (drmMap(sPriv->fd,
	 	tridentScreen->mmio.handle, tridentScreen->mmio.size,
		(drmAddressPtr)&tridentScreen->mmio.map)) {
	    FREE(tridentScreen);
	    return GL_FALSE;
    }
printf("MAPPED at 0x%x\n",tridentScreen->mmio.map);

   return tridentScreen;
}

/* Destroy the device specific screen private data struct.
 */
void tridentDestroyScreen( __DRIscreenPrivate *sPriv )
{
    tridentScreenPtr tridentScreen = (tridentScreenPtr)sPriv->private;

    FREE(tridentScreen);
}
static GLboolean 
tridentInitDriver(__DRIscreenPrivate *sPriv)
{
    sPriv->private = (void *) tridentCreateScreen( sPriv );

    if (!sPriv->private) {
	tridentDestroyScreen( sPriv );
	return GL_FALSE;
    }

    return GL_TRUE;
}

static struct __DriverAPIRec tridentAPI = {
   tridentInitDriver,
   tridentDestroyScreen,
   tridentCreateContext,
   tridentDestroyContext,
   tridentCreateBuffer,
   tridentDestroyBuffer,
   tridentSwapBuffers,
   tridentMakeCurrent,
   tridentUnbindContext,
   tridentOpenFullScreen,
   tridentCloseFullScreen
};

/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &tridentAPI);
   return (void *) psp;
}

void __driRegisterExtensions(void)
{
   /* No extensions */
}
