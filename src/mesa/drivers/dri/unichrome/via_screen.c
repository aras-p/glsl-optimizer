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

#include <stdio.h>

#include "dri_util.h"
#include "utils.h"
#include "main/glheader.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/simple_list.h"
#include "vblank.h"

#include "via_state.h"
#include "via_tex.h"
#include "via_span.h"
#include "via_screen.h"
#include "via_dri.h"

#include "GL/internal/dri_interface.h"
#include "drirenderbuffer.h"

#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_EXCESS_MIPMAP(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 3;

static drmBufMapPtr via_create_empty_buffers(void)
{
    drmBufMapPtr retval;

    retval = (drmBufMapPtr)MALLOC(sizeof(drmBufMap));
    if (retval == NULL) return NULL;
    memset(retval, 0, sizeof(drmBufMap));

    retval->list = (drmBufPtr)MALLOC(sizeof(drmBuf) * VIA_DMA_BUF_NR);
    if (retval->list == NULL) {
       FREE(retval);
       return NULL;
    }
    memset(retval->list, 0, sizeof(drmBuf) * VIA_DMA_BUF_NR);
    return retval;
}

static void via_free_empty_buffers( drmBufMapPtr bufs )
{
   if (bufs && bufs->list)
      FREE(bufs->list);

   if (bufs)
      FREE(bufs);
}


static GLboolean
viaInitDriver(__DRIscreen *sPriv)
{
    viaScreenPrivate *viaScreen;
    VIADRIPtr gDRIPriv = (VIADRIPtr)sPriv->pDevPriv;
    int i;

    if (sPriv->devPrivSize != sizeof(VIADRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(VIADRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
    }

    /* Allocate the private area */
    viaScreen = (viaScreenPrivate *) CALLOC(sizeof(viaScreenPrivate));
    if (!viaScreen) {
        __driUtilMessage("viaInitDriver: alloc viaScreenPrivate struct failed");
        return GL_FALSE;
    }

    /* parse information in __driConfigOptions */
    driParseOptionInfo (&viaScreen->optionCache,
			__driConfigOptions, __driNConfigOptions);


    viaScreen->driScrnPriv = sPriv;
    sPriv->private = (void *)viaScreen;

    viaScreen->deviceID = gDRIPriv->deviceID;
    viaScreen->width = gDRIPriv->width;
    viaScreen->height = gDRIPriv->height;
    viaScreen->mem = gDRIPriv->mem;
    viaScreen->bitsPerPixel = gDRIPriv->bytesPerPixel * 8;
    viaScreen->bytesPerPixel = gDRIPriv->bytesPerPixel;
    viaScreen->fbOffset = 0;
    viaScreen->fbSize = gDRIPriv->fbSize;
    viaScreen->irqEnabled = gDRIPriv->irqEnabled;

    if (VIA_DEBUG & DEBUG_DRI) {
	fprintf(stderr, "deviceID = %08x\n", viaScreen->deviceID);
	fprintf(stderr, "width = %08x\n", viaScreen->width);	
	fprintf(stderr, "height = %08x\n", viaScreen->height);	
	fprintf(stderr, "cpp = %08x\n", viaScreen->cpp);	
	fprintf(stderr, "fbOffset = %08x\n", viaScreen->fbOffset);	
    }

    viaScreen->bufs = via_create_empty_buffers();
    if (viaScreen->bufs == NULL) {
        __driUtilMessage("viaInitDriver: via_create_empty_buffers() failed");
        FREE(viaScreen);
        return GL_FALSE;
    }

    if (drmMap(sPriv->fd,
               gDRIPriv->regs.handle,
               gDRIPriv->regs.size,
               &viaScreen->reg) != 0) {
        FREE(viaScreen);
        sPriv->private = NULL;
        __driUtilMessage("viaInitDriver: drmMap regs failed");
        return GL_FALSE;
    }

    if (gDRIPriv->agp.size) {
        if (drmMap(sPriv->fd,
                   gDRIPriv->agp.handle,
                   gDRIPriv->agp.size,
	           (drmAddress *)&viaScreen->agpLinearStart) != 0) {
	    drmUnmap(viaScreen->reg, gDRIPriv->regs.size);
	    FREE(viaScreen);
	    sPriv->private = NULL;
	    __driUtilMessage("viaInitDriver: drmMap agp failed");
	    return GL_FALSE;
	}

	viaScreen->agpBase = drmAgpBase(sPriv->fd);
    } else
	viaScreen->agpLinearStart = 0;

    viaScreen->sareaPrivOffset = gDRIPriv->sarea_priv_offset;

    i = 0;
    viaScreen->extensions[i++] = &driFrameTrackingExtension.base;
    viaScreen->extensions[i++] = &driReadDrawableExtension;
    if ( viaScreen->irqEnabled ) {
	viaScreen->extensions[i++] = &driSwapControlExtension.base;
	viaScreen->extensions[i++] = &driMediaStreamCounterExtension.base;
    }

    viaScreen->extensions[i++] = NULL;
    sPriv->extensions = viaScreen->extensions;

    return GL_TRUE;
}

static void
viaDestroyScreen(__DRIscreen *sPriv)
{
    viaScreenPrivate *viaScreen = (viaScreenPrivate *)sPriv->private;
    VIADRIPtr gDRIPriv = (VIADRIPtr)sPriv->pDevPriv;

    drmUnmap(viaScreen->reg, gDRIPriv->regs.size);
    if (gDRIPriv->agp.size)
        drmUnmap(viaScreen->agpLinearStart, gDRIPriv->agp.size);

    via_free_empty_buffers(viaScreen->bufs);

    driDestroyOptionInfo(&viaScreen->optionCache);

    FREE(viaScreen);
    sPriv->private = NULL;
}


static GLboolean
viaCreateBuffer(__DRIscreen *driScrnPriv,
                __DRIdrawable *driDrawPriv,
                const __GLcontextModes *mesaVis,
                GLboolean isPixmap)
{
#if 0
    viaScreenPrivate *screen = (viaScreenPrivate *) driScrnPriv->private;
#endif

    GLboolean swStencil = (mesaVis->stencilBits > 0 && 
			   mesaVis->depthBits != 24);
    GLboolean swAccum = mesaVis->accumRedBits > 0;

    if (isPixmap) {
       /* KW: This needs work, disabled for now:
	*/
#if 0
	driDrawPriv->driverPrivate = (void *)
            _mesa_create_framebuffer(mesaVis,
                                     GL_FALSE,	/* software depth buffer? */
                                     swStencil,
                                     mesaVis->accumRedBits > 0,
                                     GL_FALSE 	/* s/w alpha planes */);

        return (driDrawPriv->driverPrivate != NULL);
#endif
	return GL_FALSE;
    }
    else {
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      /* The front color, back color and depth renderbuffers are
       * set up later in calculate_buffer_parameters().
       * Only create/connect software-based buffers here.
       */

#if 000
      /* This code _should_ be put to use.  We have to move the
       * viaRenderbuffer members out of the via_context structure.
       * Those members should just be the renderbuffers hanging off the
       * gl_framebuffer object.
       */
      /* XXX check/fix the offset/pitch parameters! */
      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(MESA_FORMAT_ARGB8888, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(MESA_FORMAT_ARGB8888, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(MESA_FORMAT_Z16, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(MESA_FORMAT_Z24_S8, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 32) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(MESA_FORMAT_Z32, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(MESA_FORMAT_S8, NULL,
                                 screen->bytesPerPixel,
                                 0, screen->width, driDrawPriv);
         viaSetSpanFunctions(stencilRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }
#endif

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   swStencil,
                                   swAccum,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
viaDestroyBuffer(__DRIdrawable *driDrawPriv)
{
   _mesa_reference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)), NULL);
}

static const __DRIconfig **
viaFillInModes( __DRIscreen *psp,
		unsigned pixel_bits, GLboolean have_back_buffer )
{
    __DRIconfig **configs;
    const unsigned back_buffer_factor = (have_back_buffer) ? 2 : 1;
    GLenum fb_format;
    GLenum fb_type;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    /* The 32-bit depth-buffer mode isn't supported yet, so don't actually
     * enable it.
     */
    static const uint8_t depth_bits_array[4]   = { 0, 16, 24, 32 };
    static const uint8_t stencil_bits_array[4] = { 0,  0,  8,  0 };
    uint8_t msaa_samples_array[1] = { 0 };
    const unsigned depth_buffer_factor = 3;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGRA;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    configs = driCreateConfigs(fb_format, fb_type,
			       depth_bits_array, stencil_bits_array,
			       depth_buffer_factor, back_buffer_modes,
			       back_buffer_factor,
                               msaa_samples_array, 1, GL_TRUE);
    if (configs == NULL) {
	fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
		__LINE__);
	return NULL;
    }

    return (const __DRIconfig **) configs;
}


/**
 * This is the driver specific part of the createNewScreen entry point.
 * 
 * \todo maybe fold this into intelInitDriver
 *
 * \return the __GLcontextModes supported by this driver
 */
static const __DRIconfig **
viaInitScreen(__DRIscreen *psp)
{
   static const __DRIversion ddx_expected = { VIA_DRIDDX_VERSION_MAJOR,
                                              VIA_DRIDDX_VERSION_MINOR,
                                              VIA_DRIDDX_VERSION_PATCH };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 2, 3, 0 };
   static const char *driver_name = "Unichrome";
   VIADRIPtr dri_priv = (VIADRIPtr) psp->pDevPriv;

   if ( ! driCheckDriDdxDrmVersions2( driver_name,
				      &psp->dri_version, & dri_expected,
				      &psp->ddx_version, & ddx_expected,
				      &psp->drm_version, & drm_expected) )
      return NULL;

   if (!viaInitDriver(psp))
       return NULL;

   return viaFillInModes( psp, dri_priv->bytesPerPixel * 8, GL_TRUE );

}


/**
 * Get information about previous buffer swaps.
 */
static int
getSwapInfo( __DRIdrawable *dPriv, __DRIswapInfo * sInfo )
{
   struct via_context *vmesa;

   if ( (dPriv == NULL) || (dPriv->driContextPriv == NULL)
	|| (dPriv->driContextPriv->driverPrivate == NULL)
	|| (sInfo == NULL) ) {
      return -1;
   }

   vmesa = (struct via_context *) dPriv->driContextPriv->driverPrivate;
   sInfo->swap_count = vmesa->swap_count;
   sInfo->swap_ust = vmesa->swap_ust;
   sInfo->swap_missed_count = vmesa->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
       ? driCalculateSwapUsage( dPriv, 0, vmesa->swap_missed_ust )
       : 0.0;

   return 0;
}

const struct __DriverAPIRec driDriverAPI = {
   .InitScreen      = viaInitScreen,
   .DestroyScreen   = viaDestroyScreen,
   .CreateContext   = viaCreateContext,
   .DestroyContext  = viaDestroyContext,
   .CreateBuffer    = viaCreateBuffer,
   .DestroyBuffer   = viaDestroyBuffer,
   .SwapBuffers     = viaSwapBuffers,
   .MakeCurrent     = viaMakeCurrent,
   .UnbindContext   = viaUnbindContext,
   .GetSwapInfo     = getSwapInfo,
   .GetDrawableMSC  = driDrawableGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driLegacyExtension.base,
    NULL
};
