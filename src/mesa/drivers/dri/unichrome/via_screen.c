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

#include "utils.h"
#include "dri_util.h"
#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"

#include "via_state.h"
#include "via_tex.h"
#include "via_span.h"
#include "via_tris.h"
#include "via_ioctl.h"
#include "via_screen.h"
#include "via_fb.h"

#include "via_dri.h"
extern viaContextPtr current_mesa;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /* USE_NEW_INTERFACE */


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

static GLboolean
viaInitDriver(__DRIscreenPrivate *sPriv)
{
    viaScreenPrivate *viaScreen;
    VIADRIPtr gDRIPriv = (VIADRIPtr)sPriv->pDevPriv;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif


    /* Allocate the private area */
    viaScreen = (viaScreenPrivate *) CALLOC(sizeof(viaScreenPrivate));
    if (!viaScreen) {
        __driUtilMessage("viaInitDriver: alloc viaScreenPrivate struct failed");
        return GL_FALSE;
    }

    viaScreen->driScrnPriv = sPriv;
    sPriv->private = (void *)viaScreen;

    viaScreen->deviceID = gDRIPriv->deviceID;
    viaScreen->width = gDRIPriv->width;
    viaScreen->height = gDRIPriv->height;
    viaScreen->mem = gDRIPriv->mem;
    viaScreen->bitsPerPixel = gDRIPriv->bytesPerPixel << 3;
    viaScreen->bytesPerPixel = gDRIPriv->bytesPerPixel;
    viaScreen->fbOffset = 0;
    viaScreen->fbSize = gDRIPriv->fbSize;
#ifdef USE_XINERAMA
    viaScreen->drixinerama = gDRIPriv->drixinerama;
#endif
#ifdef DEBUG    
    if (VIA_DEBUG) {
	fprintf(stderr, "deviceID = %08x\n", viaScreen->deviceID);
	fprintf(stderr, "width = %08x\n", viaScreen->width);	
	fprintf(stderr, "height = %08x\n", viaScreen->height);	
	fprintf(stderr, "cpp = %08x\n", viaScreen->cpp);	
	fprintf(stderr, "fbOffset = %08x\n", viaScreen->fbOffset);	
    }
#endif
    /* DBG */
    /*
    if (gDRIPriv->bitsPerPixel == 15)
        viaScreen->fbFormat = DV_PF_555;
    else
        viaScreen->fbFormat = DV_PF_565;
    */	

    viaScreen->bufs = via_create_empty_buffers();
    if (viaScreen->bufs == NULL) {
        __driUtilMessage("viaInitDriver: via_create_empty_buffers() failed");
        FREE(viaScreen);
        return GL_FALSE;
    }

    if (drmMap(sPriv->fd,
               gDRIPriv->regs.handle,
               gDRIPriv->regs.size,
               (drmAddress *)&viaScreen->reg) != 0) {
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
	    FREE(viaScreen);
	    drmUnmap(viaScreen->reg, gDRIPriv->agp.size);
	    sPriv->private = NULL;
	    __driUtilMessage("viaInitDriver: drmMap agp failed");
	    return GL_FALSE;
	}	    
	viaScreen->agpBase = (GLuint *)gDRIPriv->agp.handle;
    } else
	viaScreen->agpLinearStart = 0;

    viaScreen->sareaPrivOffset = gDRIPriv->sarea_priv_offset;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
#endif
    return GL_TRUE;
}

static void
viaDestroyScreen(__DRIscreenPrivate *sPriv)
{
    viaScreenPrivate *viaScreen = (viaScreenPrivate *)sPriv->private;
    VIADRIPtr gDRIPriv = (VIADRIPtr)sPriv->pDevPriv;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif
    drmUnmap(viaScreen->reg, gDRIPriv->regs.size);
    if (gDRIPriv->agp.size)
        drmUnmap(viaScreen->agpLinearStart, gDRIPriv->agp.size);
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);	
#endif    
    FREE(viaScreen);
    sPriv->private = NULL;
}

static GLboolean
viaCreateBuffer(__DRIscreenPrivate *driScrnPriv,
                __DRIdrawablePrivate *driDrawPriv,
                const __GLcontextModes *mesaVis,
                GLboolean isPixmap)
{
    viaContextPtr vmesa = current_mesa;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif    
    /*=* John Sheng [2003.7.2] for visual config & patch viewperf *=*/
    if (mesaVis->depthBits == 32 && vmesa->depthBits == 16) {
	vmesa->depthBits = mesaVis->depthBits;
	vmesa->depth.size *= 2;
	vmesa->depth.pitch *= 2;
	vmesa->depth.bpp *= 2;
	if (vmesa->depth.map)
	    via_free_depth_buffer(vmesa);
	if (!via_alloc_depth_buffer(vmesa)) {
	    via_free_depth_buffer(vmesa);
	    return GL_FALSE;
	}
	
	((__GLcontextModes*)mesaVis)->depthBits = 16; /* XXX : sure you want to change read-only data? */
    }
    
    if (isPixmap) {
	driDrawPriv->driverPrivate = (void *)
            _mesa_create_framebuffer(mesaVis,
                                     GL_FALSE,	/* software depth buffer? */
                                     mesaVis->stencilBits > 0,
                                     mesaVis->accumRedBits > 0,
                                     GL_FALSE 	/* s/w alpha planes */);

	if (vmesa) vmesa->drawType = GLX_PBUFFER_BIT;
#ifdef DEBUG    
        if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);				     
#endif	
        return (driDrawPriv->driverPrivate != NULL);
    }
    else {
        driDrawPriv->driverPrivate = (void *)
            _mesa_create_framebuffer(mesaVis,
                                     GL_FALSE,	/* software depth buffer? */
                                     mesaVis->stencilBits > 0,
                                     mesaVis->accumRedBits > 0,
                                     GL_FALSE 	/* s/w alpha planes */);
	
	if (vmesa) vmesa->drawType = GLX_WINDOW_BIT;
#ifdef DEBUG    
        if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);				     
#endif	
        return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
viaDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif
    _mesa_destroy_framebuffer((GLframebuffer *)(driDrawPriv->driverPrivate));
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
}


#if 0
/* Initialize the fullscreen mode.
 */
GLboolean
XMesaOpenFullScreen(__DRIcontextPrivate *driContextPriv)
{
    viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;
    vmesa->doPageFlip = 1;
    vmesa->currentPage = 0;
    return GL_TRUE;
}

/* Shut down the fullscreen mode.
 */
GLboolean
XMesaCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;

    if (vmesa->currentPage == 1) {
        viaPageFlip(vmesa);
        vmesa->currentPage = 0;
    }

    vmesa->doPageFlip = GL_FALSE;
    vmesa->Setup[VIA_DESTREG_DI0] = vmesa->driScreen->front_offset;
    return GL_TRUE;
}
#endif


static struct __DriverAPIRec viaAPI = {
    viaInitDriver,
    viaDestroyScreen,
    viaCreateContext,
    viaDestroyContext,
    viaCreateBuffer,
    viaDestroyBuffer,
    viaSwapBuffers,
    viaMakeCurrent,
    viaUnbindContext
};


/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
#if !defined(DRI_NEW_INTERFACE_ONLY)
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
    __DRIscreenPrivate *psp;
    psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &viaAPI);
    return (void *)psp;
}
#endif /* !defined(DRI_NEW_INTERFACE_ONLY) */


#ifdef USE_NEW_INTERFACE
static __GLcontextModes *
viaFillInModes( unsigned pixel_bits, GLboolean have_back_buffer )
{
    __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
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
    static const u_int8_t depth_bits_array[4]   = { 0, 16, 24, 32 };
    static const u_int8_t stencil_bits_array[4] = { 0,  0,  8,  0 };
    const unsigned depth_buffer_factor = 3;


    num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGR;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    modes = (*create_context_modes)( num_modes, sizeof( __GLcontextModes ) );
    m = modes;
    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_TRUE_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_DIRECT_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    return modes;
}
#endif /* USE_NEW_INTERFACE */


/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 * 
 * \return A pointer to a \c __DRIscreenPrivate on success, or \c NULL on 
 *         failure.
 */
#ifdef USE_NEW_INTERFACE
void * __driCreateNewScreen( __DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
			     const __GLcontextModes * modes,
			     const __DRIversion * ddx_version,
			     const __DRIversion * dri_version,
			     const __DRIversion * drm_version,
			     const __DRIframebuffer * frame_buffer,
			     drmAddress pSAREA, int fd, 
			     int internal_api_version,
			     __GLcontextModes ** driver_modes )
			     
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 4, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 2, 3, 0 };

   if ( ! driCheckDriDdxDrmVersions2( "Unichrome",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }
      
   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &viaAPI);
   if ( psp != NULL ) {
      create_context_modes = (PFNGLXCREATECONTEXTMODES)
	  glXGetProcAddress( (const GLubyte *) "__glXCreateContextModes" );
      if ( create_context_modes != NULL ) {
	 VIADRIPtr dri_priv = (VIADRIPtr) psp->pDevPriv;
	 *driver_modes = viaFillInModes( dri_priv->bytesPerPixel * 8,
					 GL_TRUE );
      }
   }

   return (void *) psp;
}
#endif /* USE_NEW_INTERFACE */
