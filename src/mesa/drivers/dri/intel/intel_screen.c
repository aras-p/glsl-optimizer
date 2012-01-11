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

#include <errno.h>
#include "main/glheader.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/hash.h"
#include "main/fbobject.h"
#include "main/mfeatures.h"
#include "main/version.h"
#include "swrast/s_renderbuffer.h"

#include "utils.h"
#include "xmlpool.h"

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

      DRI_CONF_OPT_BEGIN(fragment_shader, bool, true)
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

      DRI_CONF_OPT_BEGIN(shader_precompile, bool, false)
	 DRI_CONF_DESC(en, "Perform code generation at shader link time.")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END
DRI_CONF_END;

const GLuint __driNConfigOptions = 12;

#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_bufmgr.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"
#include "intel_tex.h"
#include "intel_regions.h"

#include "i915_drm.h"

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
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);

   if (intel != NULL) {
      if (intel->gen < 4)
	 INTEL_FIREVERTICES(intel);

      intel->need_throttle = true;

      if (intel->batch.used)
	 intel_batchbuffer_flush(intel);
   }
}

static const struct __DRI2flushExtensionRec intelFlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    intelDRI2Flush,
    dri2InvalidateDrawable,
};

static __DRIimage *
intel_create_image_from_name(__DRIscreen *screen,
			     int width, int height, int format,
			     int name, int pitch, void *loaderPrivate)
{
    struct intel_screen *intelScreen = screen->driverPrivate;
    __DRIimage *image;
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
    case __DRI_IMAGE_FORMAT_ABGR8888:
       image->format = MESA_FORMAT_RGBA8888_REV;
       image->internal_format = GL_RGBA;
       image->data_type = GL_UNSIGNED_BYTE;
       break;
    default:
       free(image);
       return NULL;
    }

    image->data = loaderPrivate;
    cpp = _mesa_get_format_bytes(image->format);

    image->region = intel_region_alloc_for_handle(intelScreen,
						  cpp, width, height,
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
   intel_region_reference(&image->region, irb->mt->region);

   return image;
}

static void
intel_destroy_image(__DRIimage *image)
{
    intel_region_release(&image->region);
    FREE(image);
}

static __DRIimage *
intel_create_image(__DRIscreen *screen,
		   int width, int height, int format,
		   unsigned int use,
		   void *loaderPrivate)
{
   __DRIimage *image;
   struct intel_screen *intelScreen = screen->driverPrivate;
   uint32_t tiling;
   int cpp;

   tiling = I915_TILING_X;
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
	 return NULL;
      tiling = I915_TILING_NONE;
   }

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
    case __DRI_IMAGE_FORMAT_ABGR8888:
       image->format = MESA_FORMAT_RGBA8888_REV;
       image->internal_format = GL_RGBA;
       image->data_type = GL_UNSIGNED_BYTE;
       break;
   default:
      free(image);
      return NULL;
   }

   image->data = loaderPrivate;
   cpp = _mesa_get_format_bytes(image->format);

   image->region =
      intel_region_alloc(intelScreen, tiling,
			 cpp, width, height, true);
   if (image->region == NULL) {
      FREE(image);
      return NULL;
   }
   
   return image;
}

static GLboolean
intel_query_image(__DRIimage *image, int attrib, int *value)
{
   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = image->region->pitch * image->region->cpp;
      return true;
   case __DRI_IMAGE_ATTRIB_HANDLE:
      *value = image->region->bo->handle;
      return true;
   case __DRI_IMAGE_ATTRIB_NAME:
      return intel_region_flink(image->region, (uint32_t *) value);
   default:
      return false;
   }
}

static __DRIimage *
intel_dup_image(__DRIimage *orig_image, void *loaderPrivate)
{
   __DRIimage *image;

   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   intel_region_reference(&image->region, orig_image->region);
   if (image->region == NULL) {
      FREE(image);
      return NULL;
   }

   image->internal_format = orig_image->internal_format;
   image->format          = orig_image->format;
   image->data_type       = orig_image->data_type;
   image->data            = loaderPrivate;
   
   return image;
}

static struct __DRIimageExtensionRec intelImageExtension = {
    { __DRI_IMAGE, __DRI_IMAGE_VERSION },
    intel_create_image_from_name,
    intel_create_image_from_renderbuffer,
    intel_destroy_image,
    intel_create_image,
    intel_query_image,
    intel_dup_image
};

static const __DRIextension *intelScreenExtensions[] = {
    &intelTexBufferExtension.base,
    &intelFlushExtension.base,
    &intelImageExtension.base,
    &dri2ConfigQueryExtension.base,
    NULL
};

static bool
intel_get_param(__DRIscreen *psp, int param, int *value)
{
   int ret;
   struct drm_i915_getparam gp;

   gp.param = param;
   gp.value = value;

   ret = drmCommandWriteRead(psp->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (ret) {
      if (ret != -EINVAL)
	 _mesa_warning(NULL, "drm_i915_getparam: %d", ret);
      return false;
   }

   return true;
}

static bool
intel_get_boolean(__DRIscreen *psp, int param)
{
   int value = 0;
   return intel_get_param(psp, param, &value) && value;
}

static void
nop_callback(GLuint key, void *data, void *userData)
{
}

static void
intelDestroyScreen(__DRIscreen * sPriv)
{
   struct intel_screen *intelScreen = sPriv->driverPrivate;

   dri_bufmgr_destroy(intelScreen->bufmgr);
   driDestroyOptionInfo(&intelScreen->optionCache);

   /* Some regions may still have references to them at this point, so
    * flush the hash table to prevent _mesa_DeleteHashTable() from
    * complaining about the hash not being empty; */
   _mesa_HashDeleteAll(intelScreen->named_regions, nop_callback, NULL);
   _mesa_DeleteHashTable(intelScreen->named_regions);

   FREE(intelScreen);
   sPriv->driverPrivate = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
intelCreateBuffer(__DRIscreen * driScrnPriv,
                  __DRIdrawable * driDrawPriv,
                  const struct gl_config * mesaVis, GLboolean isPixmap)
{
   struct intel_renderbuffer *rb;
   struct intel_screen *screen = (struct intel_screen*) driScrnPriv->driverPrivate;

   if (isPixmap) {
      return false;          /* not implemented */
   }
   else {
      gl_format rgbFormat;

      struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);

      if (!fb)
	 return false;

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

      /*
       * Assert here that the gl_config has an expected depth/stencil bit
       * combination: one of d24/s8, d16/s0, d0/s0. (See intelInitScreen2(),
       * which constructs the advertised configs.)
       */
      if (mesaVis->depthBits == 24) {
	 assert(mesaVis->stencilBits == 8);

	 if (screen->hw_has_separate_stencil
	     && screen->dri2_has_hiz != INTEL_DRI2_HAS_HIZ_FALSE) {
	    /*
	     * Request a separate stencil buffer even if we do not yet know if
	     * the screen supports it. (See comments for
	     * enum intel_dri2_has_hiz).
	     */
	    rb = intel_create_renderbuffer(MESA_FORMAT_X8_Z24);
	    _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base);
	    rb = intel_create_renderbuffer(MESA_FORMAT_S8);
	    _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base);
	 } else {
	    /*
	     * Use combined depth/stencil. Note that the renderbuffer is
	     * attached to two attachment points.
	     */
	    rb = intel_create_renderbuffer(MESA_FORMAT_S8_Z24);
	    _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base);
	    _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base);
	 }
      }
      else if (mesaVis->depthBits == 16) {
	 assert(mesaVis->stencilBits == 0);
         /* just 16-bit depth buffer, no hw stencil */
         struct intel_renderbuffer *depthRb
	    = intel_create_renderbuffer(MESA_FORMAT_Z16);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else {
	 assert(mesaVis->depthBits == 0);
	 assert(mesaVis->stencilBits == 0);
      }

      /* now add any/all software-based renderbuffers we may need */
      _swrast_add_soft_renderbuffers(fb,
                                     false, /* never sw color */
                                     false, /* never sw depth */
                                     false, /* never sw stencil */
                                     mesaVis->accumRedBits > 0,
                                     false, /* never sw alpha */
                                     false  /* never sw aux */ );
      driDrawPriv->driverPrivate = fb;

      return true;
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
extern bool
i830CreateContext(const struct gl_config *mesaVis,
		  __DRIcontext *driContextPriv,
		  void *sharedContextPrivate);

extern bool
i915CreateContext(int api,
		  const struct gl_config *mesaVis,
		  __DRIcontext *driContextPriv,
		  void *sharedContextPrivate);
extern bool
brwCreateContext(int api,
	         const struct gl_config *mesaVis,
	         __DRIcontext *driContextPriv,
		 void *sharedContextPrivate);

static GLboolean
intelCreateContext(gl_api api,
		   const struct gl_config * mesaVis,
                   __DRIcontext * driContextPriv,
		   unsigned major_version,
		   unsigned minor_version,
		   uint32_t flags,
		   unsigned *error,
                   void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->driverPrivate;
   bool success = false;

#ifdef I915
   if (IS_9XX(intelScreen->deviceID)) {
      if (!IS_965(intelScreen->deviceID)) {
	 success = i915CreateContext(api, mesaVis, driContextPriv,
				     sharedContextPrivate);
      }
   } else {
      intelScreen->no_vbo = true;
      success = i830CreateContext(mesaVis, driContextPriv,
				  sharedContextPrivate);
   }
#else
   if (IS_965(intelScreen->deviceID))
      success = brwCreateContext(api, mesaVis,
			      driContextPriv,
			      sharedContextPrivate);
#endif

   if (success) {
      struct gl_context *ctx =
	 (struct gl_context *) driContextPriv->driverPrivate;

      _mesa_compute_version(ctx);
      if (ctx->VersionMajor > major_version
	  || (ctx->VersionMajor == major_version
	      && ctx->VersionMinor >= minor_version)) {
	 *error = __DRI_CTX_ERROR_BAD_VERSION;
	 return true;
      }

      intelDestroyContext(driContextPriv);
   } else {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      fprintf(stderr, "Unrecognized deviceID 0x%x\n", intelScreen->deviceID);
   }

   return false;
}

static bool
intel_init_bufmgr(struct intel_screen *intelScreen)
{
   __DRIscreen *spriv = intelScreen->driScrnPriv;
   int num_fences = 0;

   intelScreen->no_hw = (getenv("INTEL_NO_HW") != NULL ||
			 getenv("INTEL_DEVID_OVERRIDE") != NULL);

   intelScreen->bufmgr = intel_bufmgr_gem_init(spriv->fd, BATCH_SZ);
   if (intelScreen->bufmgr == NULL) {
      fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
	      __func__, __LINE__);
      return false;
   }

   if (!intel_get_param(spriv, I915_PARAM_NUM_FENCES_AVAIL, &num_fences) ||
       num_fences == 0) {
      fprintf(stderr, "[%s: %u] Kernel 2.6.29 required.\n", __func__, __LINE__);
      return false;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(intelScreen->bufmgr);

   intelScreen->named_regions = _mesa_NewHashTable();

   intelScreen->relaxed_relocations = 0;
   intelScreen->relaxed_relocations |=
      intel_get_boolean(spriv, I915_PARAM_HAS_RELAXED_DELTA) << 0;

   return true;
}

/**
 * Override intel_screen.hw_has_hiz with environment variable INTEL_HIZ.
 *
 * Valid values for INTEL_HIZ are "0" and "1". If an invalid valid value is
 * encountered, a warning is emitted and INTEL_HIZ is ignored.
 */
static void
intel_override_hiz(struct intel_screen *intel)
{
   const char *s = getenv("INTEL_HIZ");
   if (!s) {
      return;
   } else if (!strncmp("0", s, 2)) {
      intel->hw_has_hiz = false;
   } else if (!strncmp("1", s, 2)) {
      intel->hw_has_hiz = true;
   } else {
      fprintf(stderr,
	      "warning: env variable INTEL_HIZ=\"%s\" has invalid value "
	      "and is ignored", s);
   }
}

/**
 * Override intel_screen.hw_has_separate_stencil with environment variable
 * INTEL_SEPARATE_STENCIL.
 *
 * Valid values for INTEL_SEPARATE_STENCIL are "0" and "1". If an invalid
 * valid value is encountered, a warning is emitted and INTEL_SEPARATE_STENCIL
 * is ignored.
 */
static void
intel_override_separate_stencil(struct intel_screen *screen)
{
   const char *s = getenv("INTEL_SEPARATE_STENCIL");
   if (!s) {
      return;
   } else if (!strncmp("0", s, 2)) {
      screen->hw_has_separate_stencil = false;
   } else if (!strncmp("1", s, 2)) {
      screen->hw_has_separate_stencil = true;
   } else {
      fprintf(stderr,
	      "warning: env variable INTEL_SEPARATE_STENCIL=\"%s\" has "
	      "invalid value and is ignored", s);
   }
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the struct gl_config supported by this driver
 */
static const
__DRIconfig **intelInitScreen2(__DRIscreen *psp)
{
   struct intel_screen *intelScreen;
   GLenum fb_format[3];
   GLenum fb_type[3];
   unsigned int api_mask;
   char *devid_override;

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
      return false;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = psp;
   psp->driverPrivate = (void *) intelScreen;

   /* Determine chipset ID */
   if (!intel_get_param(psp, I915_PARAM_CHIPSET_ID,
			&intelScreen->deviceID))
      return false;

   /* Allow an override of the device ID for the purpose of making the
    * driver produce dumps for debugging of new chipset enablement.
    * This implies INTEL_NO_HW, to avoid programming your actual GPU
    * incorrectly.
    */
   devid_override = getenv("INTEL_DEVID_OVERRIDE");
   if (devid_override) {
      intelScreen->deviceID = strtod(devid_override, NULL);
   }

   intelScreen->kernel_has_gen7_sol_reset =
      intel_get_boolean(intelScreen->driScrnPriv,
			I915_PARAM_HAS_GEN7_SOL_RESET);

   if (IS_GEN7(intelScreen->deviceID)) {
      intelScreen->gen = 7;
   } else if (IS_GEN6(intelScreen->deviceID)) {
      intelScreen->gen = 6;
   } else if (IS_GEN5(intelScreen->deviceID)) {
      intelScreen->gen = 5;
   } else if (IS_965(intelScreen->deviceID)) {
      intelScreen->gen = 4;
   } else if (IS_9XX(intelScreen->deviceID)) {
      intelScreen->gen = 3;
   } else {
      intelScreen->gen = 2;
   }

   intelScreen->hw_has_separate_stencil = intelScreen->gen >= 6;
   intelScreen->hw_must_use_separate_stencil = intelScreen->gen >= 7;
   intelScreen->hw_has_hiz = intelScreen->gen >= 6;
   intelScreen->dri2_has_hiz = INTEL_DRI2_HAS_HIZ_UNKNOWN;

   intel_override_hiz(intelScreen);
   intel_override_separate_stencil(intelScreen);

   api_mask = (1 << __DRI_API_OPENGL);
#if FEATURE_ES1
   api_mask |= (1 << __DRI_API_GLES);
#endif
#if FEATURE_ES2
   api_mask |= (1 << __DRI_API_GLES2);
#endif

   if (IS_9XX(intelScreen->deviceID) || IS_965(intelScreen->deviceID))
      psp->api_mask = api_mask;

   if (!intel_init_bufmgr(intelScreen))
       return false;

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
				     false);
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
				     true);
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

struct intel_buffer {
   __DRIbuffer base;
   struct intel_region *region;
};

/**
 * \brief Get tiling format for a DRI buffer.
 *
 * \param attachment is the buffer's attachmet point, such as
 *        __DRI_BUFFER_DEPTH.
 * \param out_tiling is the returned tiling format for buffer.
 * \return false if attachment is unrecognized or is incompatible with screen.
 */
static bool
intel_get_dri_buffer_tiling(struct intel_screen *screen,
                            uint32_t attachment,
                            uint32_t *out_tiling)
{
   if (screen->gen < 4) {
      *out_tiling = I915_TILING_X;
      return true;
   }

   switch (attachment) {
   case __DRI_BUFFER_DEPTH:
   case __DRI_BUFFER_DEPTH_STENCIL:
   case __DRI_BUFFER_HIZ:
      *out_tiling = I915_TILING_Y;
      return true;
   case __DRI_BUFFER_ACCUM:
   case __DRI_BUFFER_FRONT_LEFT:
   case __DRI_BUFFER_FRONT_RIGHT:
   case __DRI_BUFFER_BACK_LEFT:
   case __DRI_BUFFER_BACK_RIGHT:
   case __DRI_BUFFER_FAKE_FRONT_LEFT:
   case __DRI_BUFFER_FAKE_FRONT_RIGHT:
      *out_tiling = I915_TILING_X;
      return true;
   case __DRI_BUFFER_STENCIL:
      /* The stencil buffer is W tiled. However, we request from the kernel
       * a non-tiled buffer because the GTT is incapable of W fencing.
       */
      *out_tiling = I915_TILING_NONE;
      return true;
   default:
      if(unlikely(INTEL_DEBUG & DEBUG_DRI)) {
	 fprintf(stderr, "error: %s: unrecognized DRI buffer attachment 0x%x\n",
	         __FUNCTION__, attachment);
      }
       return false;
   }
}

static __DRIbuffer *
intelAllocateBuffer(__DRIscreen *screen,
		    unsigned attachment, unsigned format,
		    int width, int height)
{
   struct intel_buffer *intelBuffer;
   struct intel_screen *intelScreen = screen->driverPrivate;

   uint32_t tiling;
   uint32_t region_width;
   uint32_t region_height;
   uint32_t region_cpp;

   bool ok = true;

   ok = intel_get_dri_buffer_tiling(intelScreen, attachment, &tiling);
   if (!ok)
      return NULL;

   intelBuffer = CALLOC(sizeof *intelBuffer);
   if (intelBuffer == NULL)
      return NULL;

   if (attachment == __DRI_BUFFER_STENCIL) {
      /* The stencil buffer has quirky pitch requirements.  From Vol 2a,
       * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
       *    The pitch must be set to 2x the value computed based on width, as
       *    the stencil buffer is stored with two rows interleaved.
       * To accomplish this, we resort to the nasty hack of doubling the
       * region's cpp and halving its height.
       */
      region_width = ALIGN(width, 64);
      region_height = ALIGN(ALIGN(height, 2) / 2, 64);
      region_cpp = format / 4;
   } else {
      region_width = width;
      region_height = height;
      region_cpp = format / 8;
   }

   intelBuffer->region = intel_region_alloc(intelScreen,
                                            tiling,
                                            region_cpp,
                                            region_width,
                                            region_height,
                                            true);
   
   if (intelBuffer->region == NULL) {
	   FREE(intelBuffer);
	   return NULL;
   }
   
   intel_region_flink(intelBuffer->region, &intelBuffer->base.name);

   intelBuffer->base.attachment = attachment;
   intelBuffer->base.cpp = intelBuffer->region->cpp;
   intelBuffer->base.pitch =
         intelBuffer->region->pitch * intelBuffer->region->cpp;

   return &intelBuffer->base;
}

static void
intelReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
   struct intel_buffer *intelBuffer = (struct intel_buffer *) buffer;

   intel_region_release(&intelBuffer->region);
   free(intelBuffer);
}


const struct __DriverAPIRec driDriverAPI = {
   .InitScreen		 = intelInitScreen2,
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = intelCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .AllocateBuffer       = intelAllocateBuffer,
   .ReleaseBuffer        = intelReleaseBuffer
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driDRI2Extension.base,
    NULL
};
