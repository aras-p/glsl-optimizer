/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mga_xmesa.c,v 1.19 2003/03/26 20:43:49 tsi Exp $ */
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include <stdio.h>

#include "mga_common.h"
#include "mga_xmesa.h"
#include "context.h"
#include "matrix.h"
/*#include "mmath.h"*/
#include "simple_list.h"
/*#include "mem.h"*/

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"

#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgaspan.h"
#include "mgaioctl.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mgabuffers.h"
#include "mgapixel.h"

#include "mga_xmesa.h"

#include "mga_dri.h"


#ifndef MGA_DEBUG
int MGA_DEBUG = (0
/*		 | DEBUG_ALWAYS_SYNC */
/*		 | DEBUG_VERBOSE_MSG */
/*		 | DEBUG_VERBOSE_LRU */
/*		 | DEBUG_VERBOSE_DRI */
/*		 | DEBUG_VERBOSE_IOCTL */
/*		 | DEBUG_VERBOSE_2D */
/*		 | DEBUG_VERBOSE_FALLBACK */
   );
#endif


static GLboolean
mgaInitDriver(__DRIscreenPrivate *sPriv)
{
   mgaScreenPrivate *mgaScreen;
   MGADRIPtr         serverInfo = (MGADRIPtr)sPriv->pDevPriv;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaInitDriver\n");

   /* Check that the DRM driver version is compatible */
   if (sPriv->drmMajor != 3 ||
       sPriv->drmMinor < 0) {
      __driUtilMessage("MGA DRI driver expected DRM driver version 3.0.x but got version %d.%d.%d", sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch);
      return GL_FALSE;
   }


   /* Allocate the private area */
   mgaScreen = (mgaScreenPrivate *)MALLOC(sizeof(mgaScreenPrivate));
   if (!mgaScreen) {
      __driUtilMessage("Couldn't malloc screen struct");
      return GL_FALSE;
   }

   mgaScreen->sPriv = sPriv;
   sPriv->private = (void *)mgaScreen;

   if (sPriv->drmMinor >= 1) {
      int ret;
      drmMGAGetParam gp;

      gp.param = MGA_PARAM_IRQ_NR;
      gp.value = (int *) &mgaScreen->irq;

      ret = drmCommandWriteRead( sPriv->fd, DRM_MGA_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
	    fprintf(stderr, "drmMgaGetParam (MGA_PARAM_IRQ_NR): %d\n", ret);
	    FREE(mgaScreen);
	    sPriv->private = NULL;
	    return GL_FALSE;
      }
   }
   
   if (serverInfo->chipset != MGA_CARD_TYPE_G200 &&
       serverInfo->chipset != MGA_CARD_TYPE_G400) {
      FREE(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Unrecognized chipset");
      return GL_FALSE;
   }


   mgaScreen->chipset = serverInfo->chipset;
   mgaScreen->width = serverInfo->width;
   mgaScreen->height = serverInfo->height;
   mgaScreen->mem = serverInfo->mem;
   mgaScreen->cpp = serverInfo->cpp;

   mgaScreen->agpMode = serverInfo->agpMode;

   mgaScreen->frontPitch = serverInfo->frontPitch;
   mgaScreen->frontOffset = serverInfo->frontOffset;
   mgaScreen->backOffset = serverInfo->backOffset;
   mgaScreen->backPitch  =  serverInfo->backPitch;
   mgaScreen->depthOffset = serverInfo->depthOffset;
   mgaScreen->depthPitch  =  serverInfo->depthPitch;

   mgaScreen->mmio.handle = serverInfo->registers.handle;
   mgaScreen->mmio.size = serverInfo->registers.size;
   if ( drmMap( sPriv->fd,
		mgaScreen->mmio.handle, mgaScreen->mmio.size,
		&mgaScreen->mmio.map ) < 0 ) {
      FREE( mgaScreen );
      sPriv->private = NULL;
      __driUtilMessage( "Couldn't map MMIO registers" );
      return GL_FALSE;
   }

   mgaScreen->primary.handle = serverInfo->primary.handle;
   mgaScreen->primary.size = serverInfo->primary.size;
   mgaScreen->buffers.handle = serverInfo->buffers.handle;
   mgaScreen->buffers.size = serverInfo->buffers.size;

#if 0
   mgaScreen->agp.handle = serverInfo->agp;
   mgaScreen->agp.size = serverInfo->agpSize;

   if (drmMap(sPriv->fd,
	      mgaScreen->agp.handle,
	      mgaScreen->agp.size,
	      (drmAddress *)&mgaScreen->agp.map) != 0)
   {
      Xfree(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Couldn't map agp region");
      return GL_FALSE;
   }
#endif

   mgaScreen->textureOffset[MGA_CARD_HEAP] = serverInfo->textureOffset;
   mgaScreen->textureOffset[MGA_AGP_HEAP] = (serverInfo->agpTextureOffset |
					     PDEA_pagpxfer_enable | 1);

   mgaScreen->textureSize[MGA_CARD_HEAP] = serverInfo->textureSize;
   mgaScreen->textureSize[MGA_AGP_HEAP] = serverInfo->agpTextureSize;

   mgaScreen->logTextureGranularity[MGA_CARD_HEAP] =
      serverInfo->logTextureGranularity;
   mgaScreen->logTextureGranularity[MGA_AGP_HEAP] =
      serverInfo->logAgpTextureGranularity;

   mgaScreen->texVirtual[MGA_CARD_HEAP] = (char *)(mgaScreen->sPriv->pFB +
					   serverInfo->textureOffset);
   if (drmMap(sPriv->fd,
              serverInfo->agpTextureOffset,
              serverInfo->agpTextureSize,
              (drmAddress *)&mgaScreen->texVirtual[MGA_AGP_HEAP]) != 0)
   {
      free(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Couldn't map agptexture region");
      return GL_FALSE;
   }

#if 0
   mgaScreen->texVirtual[MGA_AGP_HEAP] = (mgaScreen->agp.map +
					  serverInfo->agpTextureOffset);
#endif

   mgaScreen->mAccess = serverInfo->mAccess;

   /* For calculating setupdma addresses.
    */
   mgaScreen->dmaOffset = serverInfo->buffers.handle;

   mgaScreen->bufs = drmMapBufs(sPriv->fd);
   if (!mgaScreen->bufs) {
      /*drmUnmap(mgaScreen->agp_tex.map, mgaScreen->agp_tex.size);*/
      FREE(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Couldn't map dma buffers");
      return GL_FALSE;
   }
   mgaScreen->sarea_priv_offset = serverInfo->sarea_priv_offset;

   return GL_TRUE;
}


static void
mgaDestroyScreen(__DRIscreenPrivate *sPriv)
{
   mgaScreenPrivate *mgaScreen = (mgaScreenPrivate *) sPriv->private;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaDestroyScreen\n");

   /*drmUnmap(mgaScreen->agp_tex.map, mgaScreen->agp_tex.size);*/
   free(mgaScreen);
   sPriv->private = NULL;
}


extern const struct gl_pipeline_stage _mga_render_stage;

static const struct gl_pipeline_stage *mga_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,	
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
				/* REMOVE: point attenuation stage */
#if 0
   &_mga_render_stage,		/* ADD: unclipped rastersetup-to-dma */
                                /* Need new ioctl for wacceptseq */
#endif
   &_tnl_render_stage,		
   0,
};


static GLboolean
mgaCreateContext( const __GLcontextModes *mesaVis,
                  __DRIcontextPrivate *driContextPriv,
                  void *sharedContextPrivate )
{
   int i;
   GLcontext *ctx, *shareCtx;
   mgaContextPtr mmesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   mgaScreenPrivate *mgaScreen = (mgaScreenPrivate *)sPriv->private;
   MGASAREAPrivPtr saPriv=(MGASAREAPrivPtr)(((char*)sPriv->pSAREA)+
					      mgaScreen->sarea_priv_offset);

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaCreateContext\n");

   /* allocate mga context */
   mmesa = (mgaContextPtr) CALLOC(sizeof(mgaContext));
   if (!mmesa) {
      return GL_FALSE;
   }

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((mgaContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;
   mmesa->glCtx = _mesa_create_context(mesaVis, shareCtx, mmesa, GL_TRUE);
   if (!mmesa->glCtx) {
      FREE(mmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = mmesa;

   /* Init mga state */
   mmesa->hHWContext = driContextPriv->hHWContext;
   mmesa->driFd = sPriv->fd;
   mmesa->driHwLock = &sPriv->pSAREA->lock;

   mmesa->mgaScreen = mgaScreen;
   mmesa->driScreen = sPriv;
   mmesa->sarea = (void *)saPriv;
   mmesa->glBuffer = NULL;

   make_empty_list(&mmesa->SwappedOut);

   mmesa->lastTexHeap = mgaScreen->texVirtual[MGA_AGP_HEAP] ? 2 : 1;

   for (i = 0 ; i < mmesa->lastTexHeap ; i++) {
      mmesa->texHeap[i] = mmInit( 0, mgaScreen->textureSize[i]);
      make_empty_list(&mmesa->TexObjList[i]);
   }

   /* Set the maximum texture size small enough that we can guarentee
    * that both texture units can bind a maximal texture and have them
    * on the card at once.
    */
   ctx = mmesa->glCtx;
   { 
      int nr = 2;

      if (mgaScreen->chipset == MGA_CARD_TYPE_G200)
	 nr = 1;

      if (mgaScreen->textureSize[0] < nr*1024*1024) {
	 ctx->Const.MaxTextureLevels = 9;
      } else if (mgaScreen->textureSize[0] < nr*4*1024*1024) {
	 ctx->Const.MaxTextureLevels = 10;
      } else {
	 ctx->Const.MaxTextureLevels = 11;
      }

      ctx->Const.MaxTextureUnits = nr;
   }

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 1.0;

   mmesa->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;

   switch (mesaVis->depthBits) {
   case 16: 
      mmesa->depth_scale = 1.0/(GLdouble)0xffff; 
      mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffff;
      break;
   case 24:
      mmesa->depth_scale = 1.0/(GLdouble)0xffffff;
      if (mmesa->hw_stencil) {
	 mmesa->depth_clear_mask = 0xffffff00;
	 mmesa->stencil_clear_mask = 0x000000ff;
      } else
	 mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffffff00;
      break;
   case 32:
      mmesa->depth_scale = 1.0/(GLdouble)0xffffffff;
      mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffffffff;
      break;
   };

   mmesa->haveHwStipple = GL_FALSE;
   mmesa->RenderIndex = -1;		/* impossible value */
   mmesa->new_state = ~0;
   mmesa->dirty = ~0;
   mmesa->vertex_format = 0;   
   mmesa->CurrentTexObj[0] = 0;
   mmesa->CurrentTexObj[1] = 0;
   mmesa->tmu_source[0] = 0;
   mmesa->tmu_source[1] = 1;

   mmesa->texAge[0] = 0;
   mmesa->texAge[1] = 0;
   
   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, mga_pipeline );

   /* Configure swrast to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   mmesa->primary_offset = mmesa->mgaScreen->primary.handle;

   ctx->DriverCtx = (void *) mmesa;
   mmesa->glCtx = ctx;

   mgaDDExtensionsInit( ctx );

   mgaDDInitStateFuncs( ctx );
   mgaDDInitTextureFuncs( ctx );
   mgaDDInitSpanFuncs( ctx );
   mgaDDInitDriverFuncs( ctx );
   mgaDDInitIoctlFuncs( ctx );
   mgaDDInitPixelFuncs( ctx );
   mgaDDInitTriFuncs( ctx );

   mgaInitVB( ctx );
   mgaInitState( mmesa );

   driContextPriv->driverPrivate = (void *) mmesa;

   return GL_TRUE;
}

static void
mgaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaDestroyContext\n");

   assert(mmesa); /* should never be null */
   if (mmesa) {
      _swsetup_DestroyContext( mmesa->glCtx );
      _tnl_DestroyContext( mmesa->glCtx );
      _ac_DestroyContext( mmesa->glCtx );
      _swrast_DestroyContext( mmesa->glCtx );

      mgaFreeVB( mmesa->glCtx );

      /* free the Mesa context */
      mmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(mmesa->glCtx);
      /* free the mga context */
      FREE(mmesa);
   }
}


static GLboolean
mgaCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 const __GLcontextModes *mesaVis,
                 GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 && 
			     mesaVis->depthBits != 24);

      driDrawPriv->driverPrivate = (void *) 
         _mesa_create_framebuffer(mesaVis,
                                  GL_FALSE,  /* software depth buffer? */
                                  swStencil,
                                  mesaVis->accumRedBits > 0,
                                  mesaVis->alphaBits > 0 );

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
mgaDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


static GLboolean
mgaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;
   if (mmesa)
      mmesa->dirty = ~0;

   return GL_TRUE;
}

static GLboolean
mgaOpenFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}

static GLboolean
mgaCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}


/* This looks buggy to me - the 'b' variable isn't used anywhere...
 * Hmm - It seems that the drawable is already hooked in to
 * driDrawablePriv.
 *
 * But why are we doing context initialization here???
 */
static GLboolean
mgaMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
   fprintf(stderr, "%s\n", __FUNCTION__);

   if (driContextPriv) {
      mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;

      if (mmesa->driDrawable != driDrawPriv) {
	 mmesa->driDrawable = driDrawPriv;
	 mmesa->dirty = ~0; 
	 mmesa->dirty_cliprects = (MGA_FRONT|MGA_BACK); 
      }

      _mesa_make_current2(mmesa->glCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate);

      if (!mmesa->glCtx->Viewport.Width)
	 _mesa_set_viewport(mmesa->glCtx, 0, 0,
                            driDrawPriv->w, driDrawPriv->h);

   }
   else {
      _mesa_make_current(NULL, NULL);
   }

   return GL_TRUE;
}

void mgaGetLock( mgaContextPtr mmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   MGASAREAPrivPtr sarea = mmesa->sarea;
   int me = mmesa->hHWContext;
   int i;

   fprintf(stderr, "%s\n", __FUNCTION__);

   drmGetLock(mmesa->driFd, mmesa->hHWContext, flags);
   
   fprintf(stderr, 
	   "mmesa->lastStamp %d dpriv->lastStamp %d *(dpriv->pStamp) %d\n",
	   mmesa->lastStamp, 
	   dPriv->lastStamp,
	   *(dPriv->pStamp));
   
   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

   if ( mmesa->lastStamp == 0 ||
	mmesa->lastStamp != dPriv->lastStamp ) {
      mmesa->lastStamp = dPriv->lastStamp;
      mmesa->SetupNewInputs |= VERT_BIT_CLIP;
      mmesa->dirty_cliprects = (MGA_FRONT|MGA_BACK);
      mgaUpdateRects( mmesa, (MGA_FRONT|MGA_BACK) );
   }

   mmesa->dirty |= MGA_UPLOAD_CONTEXT | MGA_UPLOAD_CLIPRECTS;

   mmesa->sarea->dirty |= MGA_UPLOAD_CONTEXT;

   if (sarea->ctxOwner != me) {
      mmesa->dirty |= (MGA_UPLOAD_CONTEXT | MGA_UPLOAD_TEX0 |
		       MGA_UPLOAD_TEX1 | MGA_UPLOAD_PIPE);
      sarea->ctxOwner=me;
   }

   for (i = 0 ; i < mmesa->lastTexHeap ; i++)
      if (sarea->texAge[i] != mmesa->texAge[i])
	 mgaAgeTextures( mmesa, i );

   sarea->last_quiescent = -1;	/* just kill it for now */
}



static const struct __DriverAPIRec mgaAPI = {
   mgaInitDriver,
   mgaDestroyScreen,
   mgaCreateContext,
   mgaDestroyContext,
   mgaCreateBuffer,
   mgaDestroyBuffer,
   mgaSwapBuffers,
   mgaMakeCurrent,
   mgaUnbindContext,
   mgaOpenFullScreen,
   mgaCloseFullScreen
};



/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(struct DRIDriverRec *driver,
                        struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(driver, driverContext, &mgaAPI);
   return (void *) psp;
}
