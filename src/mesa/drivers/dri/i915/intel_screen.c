/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "utils.h"
#include "xmlpool.h"


#include "intel_screen.h"

#include "intel_tex.h"
#include "intel_span.h"
#include "intel_tris.h"
#include "intel_ioctl.h"



#include "i830_dri.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
       DRI_CONF_FORCE_S3TC_ENABLE(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
const GLuint __driNConfigOptions = 1;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE*/


static void intelPrintDRIInfo(intelScreenPrivate *intelScreen,
			     __DRIscreenPrivate *sPriv,
			    I830DRIPtr gDRIPriv)
{
   fprintf(stderr, "Front size : 0x%x\n", sPriv->fbSize);
   fprintf(stderr, "Front offset : 0x%x\n", intelScreen->frontOffset);
   fprintf(stderr, "Back size : 0x%x\n", intelScreen->back.size);
   fprintf(stderr, "Back offset : 0x%x\n", intelScreen->backOffset);
   fprintf(stderr, "Depth size : 0x%x\n", intelScreen->depth.size);
   fprintf(stderr, "Depth offset : 0x%x\n", intelScreen->depthOffset);
   fprintf(stderr, "Texture size : 0x%x\n", intelScreen->textureSize);
   fprintf(stderr, "Texture offset : 0x%x\n", intelScreen->textureOffset);
   fprintf(stderr, "Memory : 0x%x\n", gDRIPriv->mem);
}

static GLboolean intelInitDriver(__DRIscreenPrivate *sPriv)
{
   intelScreenPrivate *intelScreen;
   I830DRIPtr         gDRIPriv = (I830DRIPtr)sPriv->pDevPriv;


   /* Allocate the private area */
   intelScreen = (intelScreenPrivate *)MALLOC(sizeof(intelScreenPrivate));
   if (!intelScreen) {
      fprintf(stderr,"\nERROR!  Allocating private area failed\n");
      return GL_FALSE;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo (&intelScreen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = sPriv;
   sPriv->private = (void *)intelScreen;

   intelScreen->deviceID = gDRIPriv->deviceID;
   intelScreen->width = gDRIPriv->width;
   intelScreen->height = gDRIPriv->height;
   intelScreen->mem = gDRIPriv->mem;
   intelScreen->cpp = gDRIPriv->cpp;
   intelScreen->frontPitch = gDRIPriv->fbStride;
   intelScreen->frontOffset = gDRIPriv->fbOffset;
			 
   switch (gDRIPriv->bitsPerPixel) {
   case 15: intelScreen->fbFormat = DV_PF_555; break;
   case 16: intelScreen->fbFormat = DV_PF_565; break;
   case 32: intelScreen->fbFormat = DV_PF_8888; break;
   }
			 
   intelScreen->backOffset = gDRIPriv->backOffset;
   intelScreen->backPitch = gDRIPriv->backPitch;
   intelScreen->depthOffset = gDRIPriv->depthOffset;
   intelScreen->depthPitch = gDRIPriv->depthPitch;
   intelScreen->textureOffset = gDRIPriv->textureOffset;
   intelScreen->textureSize = gDRIPriv->textureSize;
   intelScreen->logTextureGranularity = gDRIPriv->logTextureGranularity;
   intelScreen->back.handle = gDRIPriv->backbuffer;
   intelScreen->back.size = gDRIPriv->backbufferSize;
			 
   if (drmMap(sPriv->fd,
	      intelScreen->back.handle,
	      intelScreen->back.size,
	      (drmAddress *)&intelScreen->back.map) != 0) {
      fprintf(stderr, "\nERROR: line %d, Function %s, File %s\n",
	      __LINE__, __FUNCTION__, __FILE__);
      FREE(intelScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   intelScreen->depth.handle = gDRIPriv->depthbuffer;
   intelScreen->depth.size = gDRIPriv->depthbufferSize;

   if (drmMap(sPriv->fd, 
	      intelScreen->depth.handle,
	      intelScreen->depth.size,
	      (drmAddress *)&intelScreen->depth.map) != 0) {
      fprintf(stderr, "\nERROR: line %d, Function %s, File %s\n", 
	      __LINE__, __FUNCTION__, __FILE__);
      FREE(intelScreen);
      drmUnmap(intelScreen->back.map, intelScreen->back.size);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   intelScreen->tex.handle = gDRIPriv->textures;
   intelScreen->tex.size = gDRIPriv->textureSize;

   if (drmMap(sPriv->fd,
	      intelScreen->tex.handle,
	      intelScreen->tex.size,
	      (drmAddress *)&intelScreen->tex.map) != 0) {
      fprintf(stderr, "\nERROR: line %d, Function %s, File %s\n",
	      __LINE__, __FUNCTION__, __FILE__);
      FREE(intelScreen);
      drmUnmap(intelScreen->back.map, intelScreen->back.size);
      drmUnmap(intelScreen->depth.map, intelScreen->depth.size);
      sPriv->private = NULL;
      return GL_FALSE;
   }
			 
   intelScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;
   
   if (0) intelPrintDRIInfo(intelScreen, sPriv, gDRIPriv);

   intelScreen->drmMinor = sPriv->drmMinor;

   {
      int ret;
      drmI830GetParam gp;

      gp.param = I830_PARAM_IRQ_ACTIVE;
      gp.value = &intelScreen->irq_active;

      ret = drmCommandWriteRead( sPriv->fd, DRM_I830_GETPARAM,
				 &gp, sizeof(gp));
      if (ret) {
	 fprintf(stderr, "drmI830GetParam: %d\n", ret);
	 return GL_FALSE;
      }
   }

   {
      int ret;
      drmI830GetParam gp;

      gp.param = I830_PARAM_ALLOW_BATCHBUFFER;
      gp.value = &intelScreen->allow_batchbuffer;

      ret = drmCommandWriteRead( sPriv->fd, DRM_I830_GETPARAM,
				 &gp, sizeof(gp));
      if (ret) {
	 fprintf(stderr, "drmI830GetParam: (%d) %d\n", gp.param, ret);
	 return GL_FALSE;
      }
   }

   if ( driCompareGLXAPIVersion( 20030813 ) >= 0 ) {
      PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
          (PFNGLXSCRENABLEEXTENSIONPROC) glXGetProcAddress( (const GLubyte *) "__glXScrEnableExtension" );
      void * const psc = sPriv->psc->screenConfigs;

      if (glx_enable_extension != NULL) {
	 (*glx_enable_extension)( psc, "GLX_SGI_make_current_read" );

	 if ( driCompareGLXAPIVersion( 20030915 ) >= 0 ) {
	    (*glx_enable_extension)( psc, "GLX_SGIX_fbconfig" );
	    (*glx_enable_extension)( psc, "GLX_OML_swap_method" );
	 }

	 if ( driCompareGLXAPIVersion( 20030818 ) >= 0 ) {
	    sPriv->psc->allocateMemory = (void *) intelAllocateMemoryMESA;
	    sPriv->psc->freeMemory     = (void *) intelFreeMemoryMESA;
	    sPriv->psc->memoryOffset   = (void *) intelGetMemoryOffsetMESA;

	    (*glx_enable_extension)( psc, "GLX_MESA_allocate_memory" );
	 }
      }
   }

   return GL_TRUE;
}
		
		
static void intelDestroyScreen(__DRIscreenPrivate *sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *)sPriv->private;

   /* Need to unmap all the bufs and maps here:
    */
   drmUnmap(intelScreen->back.map, intelScreen->back.size);
   drmUnmap(intelScreen->depth.map, intelScreen->depth.size);
   drmUnmap(intelScreen->tex.map, intelScreen->tex.size);
   FREE(intelScreen);
   sPriv->private = NULL;
}

static GLboolean intelCreateBuffer( __DRIscreenPrivate *driScrnPriv,
				    __DRIdrawablePrivate *driDrawPriv,
				    const __GLcontextModes *mesaVis,
				    GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   } else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 && 
			     mesaVis->depthBits != 24);

      driDrawPriv->driverPrivate = (void *) 
	 _mesa_create_framebuffer(mesaVis,
				  GL_FALSE,  /* software depth buffer? */
				  swStencil,
				  mesaVis->accumRedBits > 0,
				  GL_FALSE /* s/w alpha planes */);
      
      return (driDrawPriv->driverPrivate != NULL);
   }
}

static void intelDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}


/* There are probably better ways to do this, such as an
 * init-designated function to register chipids and createcontext
 * functions.
 */
extern GLboolean i830CreateContext( const __GLcontextModes *mesaVis,
				    __DRIcontextPrivate *driContextPriv,
				    void *sharedContextPrivate);

extern GLboolean i915CreateContext( const __GLcontextModes *mesaVis,
				   __DRIcontextPrivate *driContextPriv,
				   void *sharedContextPrivate);




static GLboolean intelCreateContext( const __GLcontextModes *mesaVis,
				   __DRIcontextPrivate *driContextPriv,
				   void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *)sPriv->private;

   switch (intelScreen->deviceID) {
   case PCI_CHIP_845_G:
   case PCI_CHIP_I830_M:
   case PCI_CHIP_I855_GM:
   case PCI_CHIP_I865_G:
      return i830CreateContext( mesaVis, driContextPriv, 
				sharedContextPrivate );

   case PCI_CHIP_I915_G:
      return i915CreateContext( mesaVis, driContextPriv, 
			       sharedContextPrivate );
 
   default:
      fprintf(stderr, "Unrecognized deviceID %x\n", intelScreen->deviceID);
      return GL_FALSE;
   }
}


static const struct __DriverAPIRec intelAPI = {
   .InitDriver      = intelInitDriver,
   .DestroyScreen   = intelDestroyScreen,
   .CreateContext   = intelCreateContext,
   .DestroyContext  = intelDestroyContext,
   .CreateBuffer    = intelCreateBuffer,
   .DestroyBuffer   = intelDestroyBuffer,
   .SwapBuffers     = intelSwapBuffers,
   .MakeCurrent     = intelMakeCurrent,
   .UnbindContext   = intelUnbindContext,
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
#if !defined(DRI_NEW_INTERFACE_ONLY)
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
			int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &intelAPI);
   return (void *) psp;
}
#endif /* !defined(DRI_NEW_INTERFACE_ONLY) */
	     

#ifdef USE_NEW_INTERFACE
static __GLcontextModes *
intelFillInModes( unsigned pixel_bits, unsigned depth_bits,
		 unsigned stencil_bits, GLboolean have_back_buffer )
{
   __GLcontextModes * modes;
   __GLcontextModes * m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

   /* GLX_SWAP_COPY_OML is only supported because the MGA driver doesn't
    * support pageflipping at all.
    */
   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   u_int8_t depth_bits_array[2];
   u_int8_t stencil_bits_array[2];


   depth_bits_array[0] = 0;
   depth_bits_array[1] = depth_bits;

   /* Just like with the accumulation buffer, always provide some modes
    * with a stencil buffer.  It will be a sw fallback, but some apps won't
    * care about that.
    */
   stencil_bits_array[0] = 0;
   stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

   depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
   back_buffer_factor  = (have_back_buffer) ? 3 : 1;

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGRA;
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

   /* There's no direct color modes on intel? */

   /* Mark the visual as slow if there are "fake" stencil bits.
    */
   for ( m = modes ; m != NULL ; m = m->next ) {
      if ( (m->stencilBits != 0) && (m->stencilBits != stencil_bits) ) {
	 m->visualRating = GLX_SLOW_CONFIG;
      }
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
PUBLIC
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
   static const __DRIversion ddx_expected = { 1, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 1, 0 };

   if ( ! driCheckDriDdxDrmVersions2( "i915",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &intelAPI);
   if ( psp != NULL ) {
      create_context_modes = (PFNGLXCREATECONTEXTMODES)
	  glXGetProcAddress( (const GLubyte *) "__glXCreateContextModes" );
      if ( create_context_modes != NULL ) {
	 I830DRIPtr dri_priv = (I830DRIPtr) psp->pDevPriv;
	 *driver_modes = intelFillInModes( dri_priv->cpp * 8,
					   (dri_priv->cpp == 2) ? 16 : 24,
					   (dri_priv->cpp == 2) ? 0  : 8,
					   (dri_priv->backOffset != dri_priv->depthOffset) );
      }
   }

   return (void *) psp;
}
#endif /* USE_NEW_INTERFACE */
