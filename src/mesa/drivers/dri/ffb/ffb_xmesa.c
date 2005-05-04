/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_xmesa.c,v 1.4 2002/02/22 21:32:59 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000, 2001 David S. Miller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "ffb_xmesa.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "imports.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "array_cache/acache.h"
#include "drivers/common/driverfuncs.h"

#include "ffb_context.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_stencil.h"
#include "ffb_clear.h"
#include "ffb_vb.h"
#include "ffb_tris.h"
#include "ffb_lines.h"
#include "ffb_points.h"
#include "ffb_state.h"
#include "ffb_tex.h"
#include "ffb_lock.h"
#include "ffb_vtxfmt.h"
#include "ffb_bitmap.h"

#include "drm_sarea.h"

static GLboolean
ffbInitDriver(__DRIscreenPrivate *sPriv)
{
	ffbScreenPrivate *ffbScreen;
	FFBDRIPtr gDRIPriv = (FFBDRIPtr) sPriv->pDevPriv;

	if (getenv("LIBGL_FORCE_XSERVER"))
		return GL_FALSE;

	/* Allocate the private area. */
	ffbScreen = (ffbScreenPrivate *) MALLOC(sizeof(ffbScreenPrivate));
	if (!ffbScreen)
		return GL_FALSE;

	/* Map FBC registers. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hFbcRegs,
		   gDRIPriv->sFbcRegs,
		   &gDRIPriv->mFbcRegs)) {
	        FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->regs = (ffb_fbcPtr) gDRIPriv->mFbcRegs;

	/* Map ramdac registers. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hDacRegs,
		   gDRIPriv->sDacRegs,
		   &gDRIPriv->mDacRegs)) {
		drmUnmap(gDRIPriv->mFbcRegs, gDRIPriv->sFbcRegs);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->dac = (ffb_dacPtr) gDRIPriv->mDacRegs;

	/* Map "Smart" framebuffer views. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb8r,
		   gDRIPriv->sSfb8r,
		   &gDRIPriv->mSfb8r)) {
		drmUnmap(gDRIPriv->mFbcRegs, gDRIPriv->sFbcRegs);
		drmUnmap(gDRIPriv->mDacRegs, gDRIPriv->sDacRegs);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb8r = (volatile char *) gDRIPriv->mSfb8r;

	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb32,
		   gDRIPriv->sSfb32,
		   &gDRIPriv->mSfb32)) {
		drmUnmap(gDRIPriv->mFbcRegs, gDRIPriv->sFbcRegs);
		drmUnmap(gDRIPriv->mDacRegs, gDRIPriv->sDacRegs);
		drmUnmap(gDRIPriv->mSfb8r, gDRIPriv->sSfb8r);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb32 = (volatile char *) gDRIPriv->mSfb32;

	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb64,
		   gDRIPriv->sSfb64,
		   &gDRIPriv->mSfb64)) {
		drmUnmap(gDRIPriv->mFbcRegs, gDRIPriv->sFbcRegs);
		drmUnmap(gDRIPriv->mDacRegs, gDRIPriv->sDacRegs);
		drmUnmap(gDRIPriv->mSfb8r, gDRIPriv->sSfb8r);
		drmUnmap(gDRIPriv->mSfb32, gDRIPriv->sSfb32);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb64 = (volatile char *) gDRIPriv->mSfb64;

	ffbScreen->fifo_cache = 0;
	ffbScreen->rp_active = 0;

	ffbScreen->sPriv = sPriv;
	sPriv->private = (void *) ffbScreen;

	ffbDDLinefuncInit();
	ffbDDPointfuncInit();

	return GL_TRUE;
}


static void
ffbDestroyScreen(__DRIscreenPrivate *sPriv)
{
	ffbScreenPrivate *ffbScreen = sPriv->private;
	FFBDRIPtr gDRIPriv = (FFBDRIPtr) sPriv->pDevPriv;

	drmUnmap(gDRIPriv->mFbcRegs, gDRIPriv->sFbcRegs);
	drmUnmap(gDRIPriv->mDacRegs, gDRIPriv->sDacRegs);
	drmUnmap(gDRIPriv->mSfb8r, gDRIPriv->sSfb8r);
	drmUnmap(gDRIPriv->mSfb32, gDRIPriv->sSfb32);
	drmUnmap(gDRIPriv->mSfb64, gDRIPriv->sSfb64);

	FREE(ffbScreen);
}

static const struct tnl_pipeline_stage *ffb_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,
				/* REMOVE: fog coord stage */
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
				/* REMOVE: point attenuation stage */
   &_tnl_render_stage,		
   0,
};

/* Create and initialize the Mesa and driver specific context data */
static GLboolean
ffbCreateContext(const __GLcontextModes *mesaVis,
                 __DRIcontextPrivate *driContextPriv,
                 void *sharedContextPrivate)
{
	ffbContextPtr fmesa;
	GLcontext *ctx, *shareCtx;
	__DRIscreenPrivate *sPriv;
	ffbScreenPrivate *ffbScreen;
	char *debug;
	struct dd_function_table functions;

        /* Allocate ffb context */
	fmesa = (ffbContextPtr) CALLOC(sizeof(ffbContextRec));
	if (!fmesa)
		return GL_FALSE;

	_mesa_init_driver_functions(&functions);

        /* Allocate Mesa context */
        if (sharedContextPrivate)
           shareCtx = ((ffbContextPtr) sharedContextPrivate)->glCtx;
        else 
           shareCtx = NULL;
        fmesa->glCtx = _mesa_create_context(mesaVis, shareCtx,
                                            &functions, fmesa);
        if (!fmesa->glCtx) {
           FREE(fmesa);
           return GL_FALSE;
        }
        driContextPriv->driverPrivate = fmesa;
        ctx = fmesa->glCtx;

	sPriv = driContextPriv->driScreenPriv;
	ffbScreen = (ffbScreenPrivate *) sPriv->private;

	/* Dri stuff. */
	fmesa->hHWContext = driContextPriv->hHWContext;
	fmesa->driFd = sPriv->fd;
	fmesa->driHwLock = &sPriv->pSAREA->lock;

	fmesa->ffbScreen = ffbScreen;
	fmesa->driScreen = sPriv;
	fmesa->ffb_sarea = FFB_DRISHARE(sPriv->pSAREA);

	/* Register and framebuffer hw pointers. */
	fmesa->regs = ffbScreen->regs;
	fmesa->sfb32 = ffbScreen->sfb32;

	ffbDDInitContextHwState(ctx);

	/* Default clear and depth colors. */
	{
		GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
		GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
		GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);

		fmesa->clear_pixel = ((r << 0) |
				      (g << 8) |
				      (b << 16));
	}
	fmesa->clear_depth = Z_FROM_MESA(ctx->Depth.Clear * 4294967295.0f);
	fmesa->clear_stencil = ctx->Stencil.Clear & 0xf;

	/* No wide points. */
	ctx->Const.MinPointSize = 1.0;
	ctx->Const.MinPointSizeAA = 1.0;
	ctx->Const.MaxPointSize = 1.0;
	ctx->Const.MaxPointSizeAA = 1.0;

	/* Disable wide lines as we can't antialias them correctly in
	 * hardware.
	 */
	ctx->Const.MinLineWidth = 1.0;
	ctx->Const.MinLineWidthAA = 1.0;
	ctx->Const.MaxLineWidth = 1.0;
	ctx->Const.MaxLineWidthAA = 1.0;
	ctx->Const.LineWidthGranularity = 1.0;

	/* Instead of having GCC emit these constants a zillion times
	 * everywhere in the driver, put them here.
	 */
	fmesa->ffb_2_30_fixed_scale           = __FFB_2_30_FIXED_SCALE;
	fmesa->ffb_one_over_2_30_fixed_scale  = (1.0 / __FFB_2_30_FIXED_SCALE);
	fmesa->ffb_16_16_fixed_scale          = __FFB_16_16_FIXED_SCALE;
	fmesa->ffb_one_over_16_16_fixed_scale = (1.0 / __FFB_16_16_FIXED_SCALE);
	fmesa->ffb_ubyte_color_scale          = 255.0f;
	fmesa->ffb_zero			      = 0.0f;

	fmesa->debugFallbacks = GL_FALSE;
	debug = getenv("LIBGL_DEBUG");
	if (debug && strstr(debug, "fallbacks"))
		fmesa->debugFallbacks = GL_TRUE;

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext( ctx );
	_ac_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );

	/* All of this need only be done once for a new context. */
	/* XXX these should be moved right after the
	 *  _mesa_init_driver_functions() call above.
	 */
	ffbDDExtensionsInit(ctx);
	ffbDDInitDriverFuncs(ctx);
	ffbDDInitStateFuncs(ctx);
	ffbDDInitSpanFuncs(ctx);
	ffbDDInitDepthFuncs(ctx);
	ffbDDInitStencilFuncs(ctx);
	ffbDDInitRenderFuncs(ctx);
	/*ffbDDInitTexFuncs(ctx); not needed */
	ffbDDInitBitmapFuncs(ctx);
	ffbInitVB(ctx);

#if 0
	ffbInitTnlModule(ctx);
#endif

	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, ffb_pipeline);

	return GL_TRUE;
}

static void
ffbDestroyContext(__DRIcontextPrivate *driContextPriv)
{
	ffbContextPtr fmesa = (ffbContextPtr) driContextPriv->driverPrivate;

	if (fmesa) {
		ffbFreeVB(fmesa->glCtx);

		_swsetup_DestroyContext( fmesa->glCtx );
		_tnl_DestroyContext( fmesa->glCtx );
		_ac_DestroyContext( fmesa->glCtx );
		_swrast_DestroyContext( fmesa->glCtx );

                /* free the Mesa context */
                fmesa->glCtx->DriverCtx = NULL;
                _mesa_destroy_context(fmesa->glCtx);

		FREE(fmesa);
	}
}

/* Create and initialize the Mesa and driver specific pixmap buffer data */
static GLboolean
ffbCreateBuffer(__DRIscreenPrivate *driScrnPriv,
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
                                  mesaVis->alphaBits > 0);
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
ffbDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


#define USE_FAST_SWAP

static void
ffbSwapBuffers( __DRIdrawablePrivate *dPriv )
{
	ffbContextPtr fmesa = (ffbContextPtr) dPriv->driContextPriv->driverPrivate;
	unsigned int fbc, wid, wid_reg_val, dac_db_bit;
	unsigned int shadow_dac_addr, active_dac_addr;
	ffb_fbcPtr ffb;
	ffb_dacPtr dac;

	if (fmesa == NULL ||
	    fmesa->glCtx->Visual.doubleBufferMode == 0)
		return;

	/* Flush pending rendering commands */
	_mesa_notifySwapBuffers(fmesa->glCtx);

	ffb = fmesa->regs;
	dac = fmesa->ffbScreen->dac;

	fbc = fmesa->fbc;
	wid = fmesa->wid;

	/* Swap the buffer we render into and read pixels from. */
	fmesa->back_buffer ^= 1;

	/* If we are writing into both buffers, don't mess with
	 * the WB setting.
	 */
	if ((fbc & FFB_FBC_WB_AB) != FFB_FBC_WB_AB) {
		if ((fbc & FFB_FBC_WB_A) != 0)
			fbc = (fbc & ~FFB_FBC_WB_A) | FFB_FBC_WB_B;
		else
			fbc = (fbc & ~FFB_FBC_WB_B) | FFB_FBC_WB_A;
	}

	/* But either way, we must flip the read buffer setting. */
	if ((fbc & FFB_FBC_RB_A) != 0)
		fbc = (fbc & ~FFB_FBC_RB_A) | FFB_FBC_RB_B;
	else
		fbc = (fbc & ~FFB_FBC_RB_B) | FFB_FBC_RB_A;

	LOCK_HARDWARE(fmesa);

	if (fmesa->fbc != fbc) {
		FFBFifo(fmesa, 1);
		ffb->fbc = fmesa->fbc = fbc;
		fmesa->ffbScreen->rp_active = 1;
	}

	/* And swap the buffer displayed in the WID. */
	if (fmesa->ffb_sarea->flags & FFB_DRI_PAC1) {
		shadow_dac_addr = FFBDAC_PAC1_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC1_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC1_WLUT_DB;
	} else {
		shadow_dac_addr = FFBDAC_PAC2_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC2_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC2_WLUT_DB;
	}

	FFBWait(fmesa, ffb);

	wid_reg_val = DACCFG_READ(dac, active_dac_addr);
	if (fmesa->back_buffer == 0)
		wid_reg_val |=  dac_db_bit;
	else
		wid_reg_val &= ~dac_db_bit;
#ifdef USE_FAST_SWAP
	DACCFG_WRITE(dac, active_dac_addr, wid_reg_val);
#else
	DACCFG_WRITE(dac, shadow_dac_addr, wid_reg_val);

	/* Schedule the window transfer. */
	DACCFG_WRITE(dac, FFBDAC_CFG_WTCTRL, 
		     (FFBDAC_CFG_WTCTRL_TCMD | FFBDAC_CFG_WTCTRL_TE));

	{
		int limit = 1000000;
		while (limit--) {
			unsigned int wtctrl = DACCFG_READ(dac, FFBDAC_CFG_WTCTRL);

			if ((wtctrl & FFBDAC_CFG_WTCTRL_DS) == 0)
				break;
		}
	}
#endif

	UNLOCK_HARDWARE(fmesa);
}

static void ffb_init_wid(ffbContextPtr fmesa, unsigned int wid)
{
	ffb_dacPtr dac = fmesa->ffbScreen->dac;
	unsigned int wid_reg_val, dac_db_bit, active_dac_addr;
	unsigned int shadow_dac_addr;

	if (fmesa->ffb_sarea->flags & FFB_DRI_PAC1) {
		shadow_dac_addr = FFBDAC_PAC1_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC1_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC1_WLUT_DB;
	} else {
		shadow_dac_addr = FFBDAC_PAC2_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC2_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC2_WLUT_DB;
	}

	wid_reg_val = DACCFG_READ(dac, active_dac_addr);
	wid_reg_val &= ~dac_db_bit;
#ifdef USE_FAST_SWAP
	DACCFG_WRITE(dac, active_dac_addr, wid_reg_val);
#else
	DACCFG_WRITE(dac, shadow_dac_addr, wid_reg_val);

	/* Schedule the window transfer. */
	DACCFG_WRITE(dac, FFBDAC_CFG_WTCTRL, 
		     (FFBDAC_CFG_WTCTRL_TCMD | FFBDAC_CFG_WTCTRL_TE));

	{
		int limit = 1000000;
		while (limit--) {
			unsigned int wtctrl = DACCFG_READ(dac, FFBDAC_CFG_WTCTRL);

			if ((wtctrl & FFBDAC_CFG_WTCTRL_DS) == 0)
				break;
		}
	}
#endif
}

/* Force the context `c' to be the current context and associate with it
   buffer `b' */
static GLboolean
ffbMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
	if (driContextPriv) {
		ffbContextPtr fmesa = (ffbContextPtr) driContextPriv->driverPrivate;
		int first_time;

		fmesa->driDrawable = driDrawPriv;

		_mesa_make_current2(fmesa->glCtx, 
			    (GLframebuffer *) driDrawPriv->driverPrivate, 
			    (GLframebuffer *) driReadPriv->driverPrivate);

		first_time = 0;
		if (fmesa->wid == ~0) {
			first_time = 1;
			if (getenv("LIBGL_SOFTWARE_RENDERING"))
				FALLBACK( fmesa->glCtx, FFB_BADATTR_SWONLY, GL_TRUE );
		}

		LOCK_HARDWARE(fmesa);
		if (first_time) {
			fmesa->wid = fmesa->ffb_sarea->wid_table[driDrawPriv->index];
			ffb_init_wid(fmesa, fmesa->wid);
		}

		fmesa->state_dirty |= FFB_STATE_ALL;
		fmesa->state_fifo_ents = fmesa->state_all_fifo_ents;
		ffbSyncHardware(fmesa);
		UNLOCK_HARDWARE(fmesa);

		if (first_time) {
			/* Also, at the first switch to a new context,
			 * we need to clear all the hw buffers.
			 */
			ffbDDClear(fmesa->glCtx,
				   (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT |
				    BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL),
				   1, 0, 0, 0, 0);
		}
	} else {
		_mesa_make_current(NULL, NULL);
	}

	return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer */
static GLboolean
ffbUnbindContext(__DRIcontextPrivate *driContextPriv)
{
	return GL_TRUE;
}

void ffbXMesaUpdateState(ffbContextPtr fmesa)
{
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	__DRIscreenPrivate *sPriv = fmesa->driScreen;
	int stamp = dPriv->lastStamp;

	DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);

	if (dPriv->lastStamp != stamp) {
		GLcontext *ctx = fmesa->glCtx;

		ffbCalcViewport(ctx);
		if (ctx->Polygon.StippleFlag)
			ffbXformAreaPattern(fmesa,
					    (const GLubyte *)ctx->PolygonStipple);
	}
}


static struct __DriverAPIRec ffbAPI = {
   ffbInitDriver,
   ffbDestroyScreen,
   ffbCreateContext,
   ffbDestroyContext,
   ffbCreateBuffer,
   ffbDestroyBuffer,
   ffbSwapBuffers,
   ffbMakeCurrent,
   ffbUnbindContext
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
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &ffbAPI);
   return (void *) psp;
}
