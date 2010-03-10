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

#include "main/glheader.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/hash.h"
#include "main/fbobject.h"

#include "utils.h"
#include "xmlpool.h"

#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_bufmgr.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_screen.h"
#include "intel_tex.h"
#include "intel_regions.h"

#include "i915_drm.h"

#define DRI_CONF_TEXTURE_TILING(def) \

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN
   DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_ALWAYS_SYNC)
      /* Options correspond to DRI_CONF_BO_REUSE_DISABLED,
       * DRI_CONF_BO_REUSE_ALL
       */
      DRI_CONF_OPT_BEGIN_V(bo_reuse, enum, 1, "0:1")
	 DRI_CONF_DESC_BEGIN(en, "Buffer object reuse")
	    DRI_CONF_ENUM(0, "Disable buffer object reuse")
	    DRI_CONF_ENUM(1, "Enable reuse of all sizes of buffer objects")
	 DRI_CONF_DESC_END
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(texture_tiling, bool, true)
	 DRI_CONF_DESC(en, "Enable texture tiling")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(early_z, bool, false)
	 DRI_CONF_DESC(en, "Enable early Z in classic mode (unstable, 945-only).")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(fragment_shader, bool, false)
	 DRI_CONF_DESC(en, "Enable limited ARB_fragment_shader support on 915/945.")
      DRI_CONF_OPT_END

   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_QUALITY
      DRI_CONF_FORCE_S3TC_ENABLE(false)
      DRI_CONF_ALLOW_LARGE_TEXTURES(2)
   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_DEBUG
     DRI_CONF_NO_RAST(false)
     DRI_CONF_ALWAYS_FLUSH_BATCH(false)
     DRI_CONF_ALWAYS_FLUSH_CACHE(false)

      DRI_CONF_OPT_BEGIN(stub_occlusion_query, bool, false)
	 DRI_CONF_DESC(en, "Enable stub ARB_occlusion_query support on 915/945.")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END
DRI_CONF_END;

const GLuint __driNConfigOptions = 11;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

static const __DRItexBufferExtension intelTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   intelSetTexBuffer,
   intelSetTexBuffer2,
};

static void
intelDRI2Flush(__DRIdrawable *drawable)
{
   struct intel_context *intel = drawable->driContextPriv->driverPrivate;

   if (intel->gen < 4)
      INTEL_FIREVERTICES(intel);

   if (intel->batch->map != intel->batch->ptr)
      intel_batchbuffer_flush(intel->batch);
}

static void
intelDRI2Invalidate(__DRIdrawable *drawable)
{
   struct intel_context *intel = drawable->driContextPriv->driverPrivate;

   intel->using_dri2_swapbuffers = GL_TRUE;
   dri2InvalidateDrawable(drawable);
}

static const struct __DRI2flushExtensionRec intelFlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    intelDRI2Flush,
    intelDRI2Invalidate,
};

static __DRIimage *
intel_create_image_from_name(__DRIcontext *context,
			     int width, int height, int format,
			     int name, int pitch, void *loaderPrivate)
{
    __DRIimage *image;
    struct intel_context *intel = context->driverPrivate;
    int cpp;

    image = CALLOC(sizeof *image);
    if (image == NULL)
	return NULL;

    switch (format) {
    case __DRI_IMAGE_FORMAT_RGB565:
       image->format = MESA_FORMAT_RGB565;
       image->internal_format = GL_RGB;
       image->data_type = GL_UNSIGNED_BYTE;
       break;
    case __DRI_IMAGE_FORMAT_XRGB8888:
       image->format = MESA_FORMAT_XRGB8888;
       image->internal_format = GL_RGB;
       image->data_type = GL_UNSIGNED_BYTE;
       break;
    case __DRI_IMAGE_FORMAT_ARGB8888:
       image->format = MESA_FORMAT_ARGB8888;
       image->internal_format = GL_RGBA;
       image->data_type = GL_UNSIGNED_BYTE;
       break;
    default:
       free(image);
       return NULL;
    }

    image->data = loaderPrivate;
    cpp = _mesa_get_format_bytes(image->format);

    image->region = intel_region_alloc_for_handle(intel, cpp, width, height,
						  pitch, name, "image");
    if (image->region == NULL) {
       FREE(image);
       return NULL;
    }

    return image;	
}

static __DRIimage *
intel_create_image_from_renderbuffer(__DRIcontext *context,
				     int renderbuffer, void *loaderPrivate)
{
   __DRIimage *image;
   struct intel_context *intel = context->driverPrivate;
   struct gl_renderbuffer *rb;
   struct intel_renderbuffer *irb;

   rb = _mesa_lookup_renderbuffer(&intel->ctx, renderbuffer);
   if (!rb) {
      _mesa_error(&intel->ctx,
		  GL_INVALID_OPERATION, "glRenderbufferExternalMESA");
      return NULL;
   }

   irb = intel_renderbuffer(rb);
   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   image->internal_format = rb->InternalFormat;
   image->format = rb->Format;
   image->data_type = rb->DataType;
   image->data = loaderPrivate;
   intel_region_reference(&image->region, irb->region);

   return image;
}

static void
intel_destroy_image(__DRIimage *image)
{
    intel_region_release(&image->region);
    FREE(image);
}

static struct __DRIimageExtensionRec intelImageExtension = {
    { __DRI_IMAGE, __DRI_IMAGE_VERSION },
    intel_create_image_from_name,
    intel_create_image_from_renderbuffer,
    intel_destroy_image,
};

static const __DRIextension *intelScreenExtensions[] = {
    &driReadDrawableExtension,
    &intelTexBufferExtension.base,
    &intelFlushExtension.base,
    &intelImageExtension.base,
    NULL
};

static GLboolean
intel_get_param(__DRIscreen *psp, int param, int *value)
{
   int ret;
   struct drm_i915_getparam gp;

   gp.param = param;
   gp.value = value;

   ret = drmCommandWriteRead(psp->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (ret) {
      _mesa_warning(NULL, "drm_i915_getparam: %d", ret);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static void
nop_callback(GLuint key, void *data, void *userData)
{
}

static void
intelDestroyScreen(__DRIscreen * sPriv)
{
   struct intel_screen *intelScreen = sPriv->private;

   dri_bufmgr_destroy(intelScreen->bufmgr);
   driDestroyOptionInfo(&intelScreen->optionCache);

   /* Some regions may still have references to them at this point, so
    * flush the hash table to prevent _mesa_DeleteHashTable() from
    * complaining about the hash not being empty; */
   _mesa_HashDeleteAll(intelScreen->named_regions, nop_callback, NULL);
   _mesa_DeleteHashTable(intelScreen->named_regions);

   FREE(intelScreen);
   sPriv->private = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
intelCreateBuffer(__DRIscreen * driScrnPriv,
                  __DRIdrawable * driDrawPriv,
                  const __GLcontextModes * mesaVis, GLboolean isPixmap)
{
   struct intel_renderbuffer *rb;

   if (isPixmap) {
      return GL_FALSE;          /* not implemented */
   }
   else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 &&
                             mesaVis->depthBits != 24);
      gl_format rgbFormat;

      struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);

      if (!fb)
	 return GL_FALSE;

      _mesa_initialize_window_framebuffer(fb, mesaVis);

      if (mesaVis->redBits == 5)
	 rgbFormat = MESA_FORMAT_RGB565;
      else if (mesaVis->alphaBits == 0)
	 rgbFormat = MESA_FORMAT_XRGB8888;
      else
	 rgbFormat = MESA_FORMAT_ARGB8888;

      /* setup the hardware-based renderbuffers */
      rb = intel_create_renderbuffer(rgbFormat);
      _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &rb->Base);

      if (mesaVis->doubleBufferMode) {
	 rb = intel_create_renderbuffer(rgbFormat);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &rb->Base);
      }

      if (mesaVis->depthBits == 24) {
	 if (mesaVis->stencilBits == 8) {
	    /* combined depth/stencil buffer */
	    struct intel_renderbuffer *depthStencilRb
	       = intel_create_renderbuffer(MESA_FORMAT_S8_Z24);
	    /* note: bind RB to two attachment points */
	    _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthStencilRb->Base);
	    _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &depthStencilRb->Base);
	 } else {
	    struct intel_renderbuffer *depthRb
	       = intel_create_renderbuffer(MESA_FORMAT_X8_Z24);
	    _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
	 }
      }
      else if (mesaVis->depthBits == 16) {
         /* just 16-bit depth buffer, no hw stencil */
         struct intel_renderbuffer *depthRb
	    = intel_create_renderbuffer(MESA_FORMAT_Z16);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      /* now add any/all software-based renderbuffers we may need */
      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* never sw color */
                                   GL_FALSE, /* never sw depth */
                                   swStencil, mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* never sw alpha */
                                   GL_FALSE  /* never sw aux */ );
      driDrawPriv->driverPrivate = fb;

      return GL_TRUE;
   }
}

static void
intelDestroyBuffer(__DRIdrawable * driDrawPriv)
{
    struct gl_framebuffer *fb = driDrawPriv->driverPrivate;
  
    _mesa_reference_framebuffer(&fb, NULL);
}

/* There are probably better ways to do this, such as an
 * init-designated function to register chipids and createcontext
 * functions.
 */
extern GLboolean i830CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontext * driContextPriv,
                                   void *sharedContextPrivate);

extern GLboolean i915CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontext * driContextPriv,
                                   void *sharedContextPrivate);
extern GLboolean brwCreateContext(const __GLcontextModes * mesaVis,
				  __DRIcontext * driContextPriv,
				  void *sharedContextPrivate);

static GLboolean
intelCreateContext(const __GLcontextModes * mesaVis,
                   __DRIcontext * driContextPriv,
                   void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->private;

#ifdef I915
   if (IS_9XX(intelScreen->deviceID)) {
      if (!IS_965(intelScreen->deviceID)) {
	 return i915CreateContext(mesaVis, driContextPriv,
				  sharedContextPrivate);
      }
   } else {
      intelScreen->no_vbo = GL_TRUE;
      return i830CreateContext(mesaVis, driContextPriv, sharedContextPrivate);
   }
#else
   if (IS_965(intelScreen->deviceID))
      return brwCreateContext(mesaVis, driContextPriv, sharedContextPrivate);
#endif
   fprintf(stderr, "Unrecognized deviceID %x\n", intelScreen->deviceID);
   return GL_FALSE;
}

static GLboolean
intel_init_bufmgr(struct intel_screen *intelScreen)
{
   __DRIscreen *spriv = intelScreen->driScrnPriv;
   int num_fences = 0;

   intelScreen->no_hw = getenv("INTEL_NO_HW") != NULL;

   intelScreen->bufmgr = intel_bufmgr_gem_init(spriv->fd, BATCH_SZ);
   /* Otherwise, use the classic buffer manager. */
   if (intelScreen->bufmgr == NULL) {
      fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
	      __func__, __LINE__);
      return GL_FALSE;
   }

   if (!intel_get_param(spriv, I915_PARAM_NUM_FENCES_AVAIL, &num_fences) ||
       num_fences == 0) {
      fprintf(stderr, "[%s: %u] Kernel 2.6.29 required.\n", __func__, __LINE__);
      return GL_FALSE;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(intelScreen->bufmgr);

   intelScreen->named_regions = _mesa_NewHashTable();

   return GL_TRUE;
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the __GLcontextModes supported by this driver
 */
static const
__DRIconfig **intelInitScreen2(__DRIscreen *psp)
{
   struct intel_screen *intelScreen;
   GLenum fb_format[3];
   GLenum fb_type[3];

   static const GLenum back_buffer_modes[] = {
       GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };
   uint8_t depth_bits[4], stencil_bits[4], msaa_samples_array[1];
   int color;
   __DRIconfig **configs = NULL;

   /* Allocate the private area */
   intelScreen = CALLOC(sizeof *intelScreen);
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return GL_FALSE;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = psp;
   psp->private = (void *) intelScreen;

   /* Determine chipset ID */
   if (!intel_get_param(psp, I915_PARAM_CHIPSET_ID,
			&intelScreen->deviceID))
      return GL_FALSE;

   if (!intel_init_bufmgr(intelScreen))
       return GL_FALSE;

   psp->extensions = intelScreenExtensions;

   msaa_samples_array[0] = 0;

   fb_format[0] = GL_RGB;
   fb_type[0] = GL_UNSIGNED_SHORT_5_6_5;

   fb_format[1] = GL_BGR;
   fb_type[1] = GL_UNSIGNED_INT_8_8_8_8_REV;

   fb_format[2] = GL_BGRA;
   fb_type[2] = GL_UNSIGNED_INT_8_8_8_8_REV;

   depth_bits[0] = 0;
   stencil_bits[0] = 0;

   /* Generate a rich set of useful configs that do not include an
    * accumulation buffer.
    */
   for (color = 0; color < ARRAY_SIZE(fb_format); color++) {
      __DRIconfig **new_configs;
      int depth_factor;

      /* Starting with DRI2 protocol version 1.1 we can request a depth/stencil
       * buffer that has a diffferent number of bits per pixel than the color
       * buffer.  This isn't yet supported here.
       */
      if (fb_type[color] == GL_UNSIGNED_SHORT_5_6_5) {
	 depth_bits[1] = 16;
	 stencil_bits[1] = 0;
      } else {
	 depth_bits[1] = 24;
	 stencil_bits[1] = 8;
      }

      depth_factor = 2;

      new_configs = driCreateConfigs(fb_format[color], fb_type[color],
				     depth_bits,
				     stencil_bits,
				     depth_factor,
				     back_buffer_modes,
				     ARRAY_SIZE(back_buffer_modes),
				     msaa_samples_array,
				     ARRAY_SIZE(msaa_samples_array),
				     GL_FALSE);
      if (configs == NULL)
	 configs = new_configs;
      else
	 configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate the minimum possible set of configs that include an
    * accumulation buffer.
    */
   for (color = 0; color < ARRAY_SIZE(fb_format); color++) {
      __DRIconfig **new_configs;

      if (fb_type[color] == GL_UNSIGNED_SHORT_5_6_5) {
	 depth_bits[0] = 16;
	 stencil_bits[0] = 0;
      } else {
	 depth_bits[0] = 24;
	 stencil_bits[0] = 8;
      }

      new_configs = driCreateConfigs(fb_format[color], fb_type[color],
				     depth_bits, stencil_bits, 1,
				     back_buffer_modes + 1, 1,
				     msaa_samples_array, 1,
				     GL_TRUE);
      if (configs == NULL)
	 configs = new_configs;
      else
	 configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

const struct __DriverAPIRec driDriverAPI = {
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = intelCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .InitScreen2		 = intelInitScreen2,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driDRI2Extension.base,
    NULL
};
