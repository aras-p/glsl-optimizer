/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "main/glheader.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/hash.h"
#include "main/fbobject.h"
#include "main/version.h"
#include "swrast/s_renderbuffer.h"
#include "glsl/ralloc.h"

#include "utils.h"
#include "xmlpool.h"

static const __DRIconfigOptionsExtension brw_config_options = {
   .base = { __DRI_CONFIG_OPTIONS, 1 },
   .xml =
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

      DRI_CONF_OPT_BEGIN_B(hiz, "true")
	 DRI_CONF_DESC(en, "Enable Hierarchical Z on gen6+")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN_B(disable_derivative_optimization, "false")
	 DRI_CONF_DESC(en, "Derivatives with finer granularity by default")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END

   DRI_CONF_SECTION_QUALITY
      DRI_CONF_FORCE_S3TC_ENABLE("false")

      DRI_CONF_OPT_BEGIN(clamp_max_samples, int, -1)
              DRI_CONF_DESC(en, "Clamp the value of GL_MAX_SAMPLES to the "
                            "given integer. If negative, then do not clamp.")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END

   DRI_CONF_SECTION_DEBUG
      DRI_CONF_NO_RAST("false")
      DRI_CONF_ALWAYS_FLUSH_BATCH("false")
      DRI_CONF_ALWAYS_FLUSH_CACHE("false")
      DRI_CONF_DISABLE_THROTTLING("false")
      DRI_CONF_FORCE_GLSL_EXTENSIONS_WARN("false")
      DRI_CONF_DISABLE_GLSL_LINE_CONTINUATIONS("false")
      DRI_CONF_DISABLE_BLEND_FUNC_EXTENDED("false")

      DRI_CONF_OPT_BEGIN_B(shader_precompile, "true")
	 DRI_CONF_DESC(en, "Perform code generation at shader link time.")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END
DRI_CONF_END
};

#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_bufmgr.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"
#include "intel_tex.h"
#include "intel_image.h"

#include "brw_context.h"

#include "i915_drm.h"

/**
 * For debugging purposes, this returns a time in seconds.
 */
double
get_time(void)
{
   struct timespec tp;

   clock_gettime(CLOCK_MONOTONIC, &tp);

   return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}

void
aub_dump_bmp(struct gl_context *ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   for (int i = 0; i < fb->_NumColorDrawBuffers; i++) {
      struct intel_renderbuffer *irb =
	 intel_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (irb && irb->mt) {
	 enum aub_dump_bmp_format format;

	 switch (irb->Base.Base.Format) {
	 case MESA_FORMAT_B8G8R8A8_UNORM:
	 case MESA_FORMAT_B8G8R8X8_UNORM:
	    format = AUB_DUMP_BMP_FORMAT_ARGB_8888;
	    break;
	 default:
	    continue;
	 }

         drm_intel_gem_bo_aub_dump_bmp(irb->mt->bo,
				       irb->draw_x,
				       irb->draw_y,
				       irb->Base.Base.Width,
				       irb->Base.Base.Height,
				       format,
				       irb->mt->pitch,
				       0);
      }
   }
}

static const __DRItexBufferExtension intelTexBufferExtension = {
   .base = { __DRI_TEX_BUFFER, 3 },

   .setTexBuffer        = intelSetTexBuffer,
   .setTexBuffer2       = intelSetTexBuffer2,
   .releaseTexBuffer    = NULL,
};

static void
intel_dri2_flush_with_flags(__DRIcontext *cPriv,
                            __DRIdrawable *dPriv,
                            unsigned flags,
                            enum __DRI2throttleReason reason)
{
   struct brw_context *brw = cPriv->driverPrivate;

   if (!brw)
      return;

   struct gl_context *ctx = &brw->ctx;

   FLUSH_VERTICES(ctx, 0);

   if (flags & __DRI2_FLUSH_DRAWABLE)
      intel_resolve_for_dri2_flush(brw, dPriv);

   if (reason == __DRI2_THROTTLE_SWAPBUFFER ||
       reason == __DRI2_THROTTLE_FLUSHFRONT) {
      brw->need_throttle = true;
   }

   intel_batchbuffer_flush(brw);

   if (INTEL_DEBUG & DEBUG_AUB) {
      aub_dump_bmp(ctx);
   }
}

/**
 * Provides compatibility with loaders that only support the older (version
 * 1-3) flush interface.
 *
 * That includes libGL up to Mesa 9.0, and the X Server at least up to 1.13.
 */
static void
intel_dri2_flush(__DRIdrawable *drawable)
{
   intel_dri2_flush_with_flags(drawable->driContextPriv, drawable,
                               __DRI2_FLUSH_DRAWABLE,
                               __DRI2_THROTTLE_SWAPBUFFER);
}

static const struct __DRI2flushExtensionRec intelFlushExtension = {
    .base = { __DRI2_FLUSH, 4 },

    .flush              = intel_dri2_flush,
    .invalidate         = dri2InvalidateDrawable,
    .flush_with_flags   = intel_dri2_flush_with_flags,
};

static struct intel_image_format intel_image_formats[] = {
   { __DRI_IMAGE_FOURCC_ARGB8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } },

   { __DRI_IMAGE_FOURCC_SARGB8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_SARGB8, 4 } } },

   { __DRI_IMAGE_FOURCC_XRGB8888, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XRGB8888, 4 }, } },

   { __DRI_IMAGE_FOURCC_RGB565, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_RGB565, 2 } } },

   { __DRI_IMAGE_FOURCC_YUV410, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV411, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV420, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV422, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV444, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_NV12, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   { __DRI_IMAGE_FOURCC_NV16, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   /* For YUYV buffers, we set up two overlapping DRI images and treat
    * them as planar buffers in the compositors.  Plane 0 is GR88 and
    * samples YU or YV pairs and places Y into the R component, while
    * plane 1 is ARGB and samples YUYV clusters and places pairs and
    * places U into the G component and V into A.  This lets the
    * texture sampler interpolate the Y components correctly when
    * sampling from plane 0, and interpolate U and V correctly when
    * sampling from plane 1. */
   { __DRI_IMAGE_FOURCC_YUYV, __DRI_IMAGE_COMPONENTS_Y_XUXV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88, 2 },
       { 0, 1, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } }
};

static void
intel_image_warn_if_unaligned(__DRIimage *image, const char *func)
{
   uint32_t tiling, swizzle;
   drm_intel_bo_get_tiling(image->bo, &tiling, &swizzle);

   if (tiling != I915_TILING_NONE && (image->offset & 0xfff)) {
      _mesa_warning(NULL, "%s: offset 0x%08x not on tile boundary",
                    func, image->offset);
   }
}

static struct intel_image_format *
intel_image_format_lookup(int fourcc)
{
   struct intel_image_format *f = NULL;

   for (unsigned i = 0; i < ARRAY_SIZE(intel_image_formats); i++) {
      if (intel_image_formats[i].fourcc == fourcc) {
	 f = &intel_image_formats[i];
	 break;
      }
   }

   return f;
}

static __DRIimage *
intel_allocate_image(int dri_format, void *loaderPrivate)
{
    __DRIimage *image;

    image = calloc(1, sizeof *image);
    if (image == NULL)
	return NULL;

    image->dri_format = dri_format;
    image->offset = 0;

    image->format = driImageFormatToGLFormat(dri_format);
    if (dri_format != __DRI_IMAGE_FORMAT_NONE &&
        image->format == MESA_FORMAT_NONE) {
       free(image);
       return NULL;
    }

    image->internal_format = _mesa_get_format_base_format(image->format);
    image->data = loaderPrivate;

    return image;
}

/**
 * Sets up a DRIImage structure to point to a slice out of a miptree.
 */
static void
intel_setup_image_from_mipmap_tree(struct brw_context *brw, __DRIimage *image,
                                   struct intel_mipmap_tree *mt, GLuint level,
                                   GLuint zoffset)
{
   intel_miptree_make_shareable(brw, mt);

   intel_miptree_check_level_layer(mt, level, zoffset);

   image->width = minify(mt->physical_width0, level - mt->first_level);
   image->height = minify(mt->physical_height0, level - mt->first_level);
   image->pitch = mt->pitch;

   image->offset = intel_miptree_get_tile_offsets(mt, level, zoffset,
                                                  &image->tile_x,
                                                  &image->tile_y);

   drm_intel_bo_unreference(image->bo);
   image->bo = mt->bo;
   drm_intel_bo_reference(mt->bo);
}

static __DRIimage *
intel_create_image_from_name(__DRIscreen *screen,
			     int width, int height, int format,
			     int name, int pitch, void *loaderPrivate)
{
    struct intel_screen *intelScreen = screen->driverPrivate;
    __DRIimage *image;
    int cpp;

    image = intel_allocate_image(format, loaderPrivate);
    if (image == NULL)
       return NULL;

    if (image->format == MESA_FORMAT_NONE)
       cpp = 1;
    else
       cpp = _mesa_get_format_bytes(image->format);

    image->width = width;
    image->height = height;
    image->pitch = pitch * cpp;
    image->bo = drm_intel_bo_gem_create_from_name(intelScreen->bufmgr, "image",
                                                  name);
    if (!image->bo) {
       free(image);
       return NULL;
    }

    return image;	
}

static __DRIimage *
intel_create_image_from_renderbuffer(__DRIcontext *context,
				     int renderbuffer, void *loaderPrivate)
{
   __DRIimage *image;
   struct brw_context *brw = context->driverPrivate;
   struct gl_context *ctx = &brw->ctx;
   struct gl_renderbuffer *rb;
   struct intel_renderbuffer *irb;

   rb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glRenderbufferExternalMESA");
      return NULL;
   }

   irb = intel_renderbuffer(rb);
   intel_miptree_make_shareable(brw, irb->mt);
   image = calloc(1, sizeof *image);
   if (image == NULL)
      return NULL;

   image->internal_format = rb->InternalFormat;
   image->format = rb->Format;
   image->offset = 0;
   image->data = loaderPrivate;
   drm_intel_bo_unreference(image->bo);
   image->bo = irb->mt->bo;
   drm_intel_bo_reference(irb->mt->bo);
   image->width = rb->Width;
   image->height = rb->Height;
   image->pitch = irb->mt->pitch;
   image->dri_format = driGLFormatToImageFormat(image->format);
   image->has_depthstencil = irb->mt->stencil_mt? true : false;

   rb->NeedsFinishRenderTexture = true;
   return image;
}

static __DRIimage *
intel_create_image_from_texture(__DRIcontext *context, int target,
                                unsigned texture, int zoffset,
                                int level,
                                unsigned *error,
                                void *loaderPrivate)
{
   __DRIimage *image;
   struct brw_context *brw = context->driverPrivate;
   struct gl_texture_object *obj;
   struct intel_texture_object *iobj;
   GLuint face = 0;

   obj = _mesa_lookup_texture(&brw->ctx, texture);
   if (!obj || obj->Target != target) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      return NULL;
   }

   if (target == GL_TEXTURE_CUBE_MAP)
      face = zoffset;

   _mesa_test_texobj_completeness(&brw->ctx, obj);
   iobj = intel_texture_object(obj);
   if (!obj->_BaseComplete || (level > 0 && !obj->_MipmapComplete)) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      return NULL;
   }

   if (level < obj->BaseLevel || level > obj->_MaxLevel) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }

   if (target == GL_TEXTURE_3D && obj->Image[face][level]->Depth < zoffset) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }
   image = calloc(1, sizeof *image);
   if (image == NULL) {
      *error = __DRI_IMAGE_ERROR_BAD_ALLOC;
      return NULL;
   }

   image->internal_format = obj->Image[face][level]->InternalFormat;
   image->format = obj->Image[face][level]->TexFormat;
   image->data = loaderPrivate;
   intel_setup_image_from_mipmap_tree(brw, image, iobj->mt, level, zoffset);
   image->dri_format = driGLFormatToImageFormat(image->format);
   image->has_depthstencil = iobj->mt->stencil_mt? true : false;
   if (image->dri_format == MESA_FORMAT_NONE) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      free(image);
      return NULL;
   }

   *error = __DRI_IMAGE_ERROR_SUCCESS;
   return image;
}

static void
intel_destroy_image(__DRIimage *image)
{
   drm_intel_bo_unreference(image->bo);
   free(image);
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
   unsigned long pitch;

   tiling = I915_TILING_X;
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
	 return NULL;
      tiling = I915_TILING_NONE;
   }

   if (use & __DRI_IMAGE_USE_LINEAR)
      tiling = I915_TILING_NONE;

   image = intel_allocate_image(format, loaderPrivate);
   if (image == NULL)
      return NULL;

   
   cpp = _mesa_get_format_bytes(image->format);
   image->bo = drm_intel_bo_alloc_tiled(intelScreen->bufmgr, "image",
                                        width, height, cpp, &tiling,
                                        &pitch, 0);
   if (image->bo == NULL) {
      free(image);
      return NULL;
   }
   image->width = width;
   image->height = height;
   image->pitch = pitch;

   return image;
}

static GLboolean
intel_query_image(__DRIimage *image, int attrib, int *value)
{
   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = image->pitch;
      return true;
   case __DRI_IMAGE_ATTRIB_HANDLE:
      *value = image->bo->handle;
      return true;
   case __DRI_IMAGE_ATTRIB_NAME:
      return !drm_intel_bo_flink(image->bo, (uint32_t *) value);
   case __DRI_IMAGE_ATTRIB_FORMAT:
      *value = image->dri_format;
      return true;
   case __DRI_IMAGE_ATTRIB_WIDTH:
      *value = image->width;
      return true;
   case __DRI_IMAGE_ATTRIB_HEIGHT:
      *value = image->height;
      return true;
   case __DRI_IMAGE_ATTRIB_COMPONENTS:
      if (image->planar_format == NULL)
         return false;
      *value = image->planar_format->components;
      return true;
   case __DRI_IMAGE_ATTRIB_FD:
      if (drm_intel_bo_gem_export_to_prime(image->bo, value) == 0)
         return true;
      return false;
  default:
      return false;
   }
}

static __DRIimage *
intel_dup_image(__DRIimage *orig_image, void *loaderPrivate)
{
   __DRIimage *image;

   image = calloc(1, sizeof *image);
   if (image == NULL)
      return NULL;

   drm_intel_bo_reference(orig_image->bo);
   image->bo              = orig_image->bo;
   image->internal_format = orig_image->internal_format;
   image->planar_format   = orig_image->planar_format;
   image->dri_format      = orig_image->dri_format;
   image->format          = orig_image->format;
   image->offset          = orig_image->offset;
   image->width           = orig_image->width;
   image->height          = orig_image->height;
   image->pitch           = orig_image->pitch;
   image->tile_x          = orig_image->tile_x;
   image->tile_y          = orig_image->tile_y;
   image->has_depthstencil = orig_image->has_depthstencil;
   image->data            = loaderPrivate;

   memcpy(image->strides, orig_image->strides, sizeof(image->strides));
   memcpy(image->offsets, orig_image->offsets, sizeof(image->offsets));

   return image;
}

static GLboolean
intel_validate_usage(__DRIimage *image, unsigned int use)
{
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (image->width != 64 || image->height != 64)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

static __DRIimage *
intel_create_image_from_names(__DRIscreen *screen,
                              int width, int height, int fourcc,
                              int *names, int num_names,
                              int *strides, int *offsets,
                              void *loaderPrivate)
{
    struct intel_image_format *f = NULL;
    __DRIimage *image;
    int i, index;

    if (screen == NULL || names == NULL || num_names != 1)
        return NULL;

    f = intel_image_format_lookup(fourcc);
    if (f == NULL)
        return NULL;

    image = intel_create_image_from_name(screen, width, height,
                                         __DRI_IMAGE_FORMAT_NONE,
                                         names[0], strides[0],
                                         loaderPrivate);

   if (image == NULL)
      return NULL;

    image->planar_format = f;
    for (i = 0; i < f->nplanes; i++) {
        index = f->planes[i].buffer_index;
        image->offsets[index] = offsets[index];
        image->strides[index] = strides[index];
    }

    return image;
}

static __DRIimage *
intel_create_image_from_fds(__DRIscreen *screen,
                            int width, int height, int fourcc,
                            int *fds, int num_fds, int *strides, int *offsets,
                            void *loaderPrivate)
{
   struct intel_screen *intelScreen = screen->driverPrivate;
   struct intel_image_format *f;
   __DRIimage *image;
   int i, index;

   if (fds == NULL || num_fds != 1)
      return NULL;

   f = intel_image_format_lookup(fourcc);
   if (f == NULL)
      return NULL;

   if (f->nplanes == 1)
      image = intel_allocate_image(f->planes[0].dri_format, loaderPrivate);
   else
      image = intel_allocate_image(__DRI_IMAGE_FORMAT_NONE, loaderPrivate);

   if (image == NULL)
      return NULL;

   image->bo = drm_intel_bo_gem_create_from_prime(intelScreen->bufmgr,
                                                  fds[0],
                                                  height * strides[0]);
   if (image->bo == NULL) {
      free(image);
      return NULL;
   }
   image->width = width;
   image->height = height;
   image->pitch = strides[0];

   image->planar_format = f;
   for (i = 0; i < f->nplanes; i++) {
      index = f->planes[i].buffer_index;
      image->offsets[index] = offsets[index];
      image->strides[index] = strides[index];
   }

   if (f->nplanes == 1) {
      image->offset = image->offsets[0];
      intel_image_warn_if_unaligned(image, __FUNCTION__);
   }

   return image;
}

static __DRIimage *
intel_create_image_from_dma_bufs(__DRIscreen *screen,
                                 int width, int height, int fourcc,
                                 int *fds, int num_fds,
                                 int *strides, int *offsets,
                                 enum __DRIYUVColorSpace yuv_color_space,
                                 enum __DRISampleRange sample_range,
                                 enum __DRIChromaSiting horizontal_siting,
                                 enum __DRIChromaSiting vertical_siting,
                                 unsigned *error,
                                 void *loaderPrivate)
{
   __DRIimage *image;
   struct intel_image_format *f = intel_image_format_lookup(fourcc);

   /* For now only packed formats that have native sampling are supported. */
   if (!f || f->nplanes != 1) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }

   image = intel_create_image_from_fds(screen, width, height, fourcc, fds,
                                       num_fds, strides, offsets,
                                       loaderPrivate);

   /*
    * Invalid parameters and any inconsistencies between are assumed to be
    * checked by the caller. Therefore besides unsupported formats one can fail
    * only in allocation.
    */
   if (!image) {
      *error = __DRI_IMAGE_ERROR_BAD_ALLOC;
      return NULL;
   }

   image->dma_buf_imported = true;
   image->yuv_color_space = yuv_color_space;
   image->sample_range = sample_range;
   image->horizontal_siting = horizontal_siting;
   image->vertical_siting = vertical_siting;

   *error = __DRI_IMAGE_ERROR_SUCCESS;
   return image;
}

static __DRIimage *
intel_from_planar(__DRIimage *parent, int plane, void *loaderPrivate)
{
    int width, height, offset, stride, dri_format, index;
    struct intel_image_format *f;
    __DRIimage *image;

    if (parent == NULL || parent->planar_format == NULL)
        return NULL;

    f = parent->planar_format;

    if (plane >= f->nplanes)
        return NULL;

    width = parent->width >> f->planes[plane].width_shift;
    height = parent->height >> f->planes[plane].height_shift;
    dri_format = f->planes[plane].dri_format;
    index = f->planes[plane].buffer_index;
    offset = parent->offsets[index];
    stride = parent->strides[index];

    image = intel_allocate_image(dri_format, loaderPrivate);
    if (image == NULL)
       return NULL;

    if (offset + height * stride > parent->bo->size) {
       _mesa_warning(NULL, "intel_create_sub_image: subimage out of bounds");
       free(image);
       return NULL;
    }

    image->bo = parent->bo;
    drm_intel_bo_reference(parent->bo);

    image->width = width;
    image->height = height;
    image->pitch = stride;
    image->offset = offset;

    intel_image_warn_if_unaligned(image, __FUNCTION__);

    return image;
}

static const __DRIimageExtension intelImageExtension = {
    .base = { __DRI_IMAGE, 8 },

    .createImageFromName                = intel_create_image_from_name,
    .createImageFromRenderbuffer        = intel_create_image_from_renderbuffer,
    .destroyImage                       = intel_destroy_image,
    .createImage                        = intel_create_image,
    .queryImage                         = intel_query_image,
    .dupImage                           = intel_dup_image,
    .validateUsage                      = intel_validate_usage,
    .createImageFromNames               = intel_create_image_from_names,
    .fromPlanar                         = intel_from_planar,
    .createImageFromTexture             = intel_create_image_from_texture,
    .createImageFromFds                 = intel_create_image_from_fds,
    .createImageFromDmaBufs             = intel_create_image_from_dma_bufs
};

static int
brw_query_renderer_integer(__DRIscreen *psp, int param, unsigned int *value)
{
   const struct intel_screen *const intelScreen =
      (struct intel_screen *) psp->driverPrivate;

   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = 0x8086;
      return 0;
   case __DRI2_RENDERER_DEVICE_ID:
      value[0] = intelScreen->deviceID;
      return 0;
   case __DRI2_RENDERER_ACCELERATED:
      value[0] = 1;
      return 0;
   case __DRI2_RENDERER_VIDEO_MEMORY: {
      /* Once a batch uses more than 75% of the maximum mappable size, we
       * assume that there's some fragmentation, and we start doing extra
       * flushing, etc.  That's the big cliff apps will care about.
       */
      size_t aper_size;
      size_t mappable_size;

      drm_intel_get_aperture_sizes(psp->fd, &mappable_size, &aper_size);

      const unsigned gpu_mappable_megabytes =
         (aper_size / (1024 * 1024)) * 3 / 4;

      const long system_memory_pages = sysconf(_SC_PHYS_PAGES);
      const long system_page_size = sysconf(_SC_PAGE_SIZE);

      if (system_memory_pages <= 0 || system_page_size <= 0)
         return -1;

      const uint64_t system_memory_bytes = (uint64_t) system_memory_pages
         * (uint64_t) system_page_size;

      const unsigned system_memory_megabytes =
         (unsigned) (system_memory_bytes / (1024 * 1024));

      value[0] = MIN2(system_memory_megabytes, gpu_mappable_megabytes);
      return 0;
   }
   case __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE:
      value[0] = 1;
      return 0;
   case __DRI2_RENDERER_PREFERRED_PROFILE:
      value[0] = (psp->max_gl_core_version != 0)
         ? (1U << __DRI_API_OPENGL_CORE) : (1U << __DRI_API_OPENGL);
      return 0;
   default:
      return driQueryRendererIntegerCommon(psp, param, value);
   }

   return -1;
}

static int
brw_query_renderer_string(__DRIscreen *psp, int param, const char **value)
{
   const struct intel_screen *intelScreen =
      (struct intel_screen *) psp->driverPrivate;

   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = brw_vendor_string;
      return 0;
   case __DRI2_RENDERER_DEVICE_ID:
      value[0] = brw_get_renderer_string(intelScreen->deviceID);
      return 0;
   default:
      break;
   }

   return -1;
}

static const __DRI2rendererQueryExtension intelRendererQueryExtension = {
   .base = { __DRI2_RENDERER_QUERY, 1 },

   .queryInteger = brw_query_renderer_integer,
   .queryString = brw_query_renderer_string
};

static const __DRIrobustnessExtension dri2Robustness = {
   .base = { __DRI2_ROBUSTNESS, 1 }
};

static const __DRIextension *intelScreenExtensions[] = {
    &intelTexBufferExtension.base,
    &intelFlushExtension.base,
    &intelImageExtension.base,
    &intelRendererQueryExtension.base,
    &dri2ConfigQueryExtension.base,
    NULL
};

static const __DRIextension *intelRobustScreenExtensions[] = {
    &intelTexBufferExtension.base,
    &intelFlushExtension.base,
    &intelImageExtension.base,
    &intelRendererQueryExtension.base,
    &dri2ConfigQueryExtension.base,
    &dri2Robustness.base,
    NULL
};

static bool
intel_get_param(__DRIscreen *psp, int param, int *value)
{
   int ret;
   struct drm_i915_getparam gp;

   memset(&gp, 0, sizeof(gp));
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
intelDestroyScreen(__DRIscreen * sPriv)
{
   struct intel_screen *intelScreen = sPriv->driverPrivate;

   dri_bufmgr_destroy(intelScreen->bufmgr);
   driDestroyOptionInfo(&intelScreen->optionCache);

   ralloc_free(intelScreen);
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
   mesa_format rgbFormat;
   unsigned num_samples = intel_quantize_num_samples(screen, mesaVis->samples);
   struct gl_framebuffer *fb;

   if (isPixmap)
      return false;

   fb = CALLOC_STRUCT(gl_framebuffer);
   if (!fb)
      return false;

   _mesa_initialize_window_framebuffer(fb, mesaVis);

   if (screen->winsys_msaa_samples_override != -1) {
      num_samples = screen->winsys_msaa_samples_override;
      fb->Visual.samples = num_samples;
   }

   if (mesaVis->redBits == 5)
      rgbFormat = MESA_FORMAT_B5G6R5_UNORM;
   else if (mesaVis->sRGBCapable)
      rgbFormat = MESA_FORMAT_B8G8R8A8_SRGB;
   else if (mesaVis->alphaBits == 0)
      rgbFormat = MESA_FORMAT_B8G8R8X8_UNORM;
   else {
      rgbFormat = MESA_FORMAT_B8G8R8A8_SRGB;
      fb->Visual.sRGBCapable = true;
   }

   /* setup the hardware-based renderbuffers */
   rb = intel_create_renderbuffer(rgbFormat, num_samples);
   _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &rb->Base.Base);

   if (mesaVis->doubleBufferMode) {
      rb = intel_create_renderbuffer(rgbFormat, num_samples);
      _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &rb->Base.Base);
   }

   /*
    * Assert here that the gl_config has an expected depth/stencil bit
    * combination: one of d24/s8, d16/s0, d0/s0. (See intelInitScreen2(),
    * which constructs the advertised configs.)
    */
   if (mesaVis->depthBits == 24) {
      assert(mesaVis->stencilBits == 8);

      if (screen->devinfo->has_hiz_and_separate_stencil) {
         rb = intel_create_private_renderbuffer(MESA_FORMAT_Z24_UNORM_X8_UINT,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
         rb = intel_create_private_renderbuffer(MESA_FORMAT_S_UINT8,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base.Base);
      } else {
         /*
          * Use combined depth/stencil. Note that the renderbuffer is
          * attached to two attachment points.
          */
         rb = intel_create_private_renderbuffer(MESA_FORMAT_Z24_UNORM_S8_UINT,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base.Base);
      }
   }
   else if (mesaVis->depthBits == 16) {
      assert(mesaVis->stencilBits == 0);
      rb = intel_create_private_renderbuffer(MESA_FORMAT_Z_UNORM16,
                                             num_samples);
      _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
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

static void
intelDestroyBuffer(__DRIdrawable * driDrawPriv)
{
    struct gl_framebuffer *fb = driDrawPriv->driverPrivate;

    _mesa_reference_framebuffer(&fb, NULL);
}

static bool
intel_init_bufmgr(struct intel_screen *intelScreen)
{
   __DRIscreen *spriv = intelScreen->driScrnPriv;

   intelScreen->no_hw = getenv("INTEL_NO_HW") != NULL;

   intelScreen->bufmgr = intel_bufmgr_gem_init(spriv->fd, BATCH_SZ);
   if (intelScreen->bufmgr == NULL) {
      fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
	      __func__, __LINE__);
      return false;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(intelScreen->bufmgr);

   if (!intel_get_boolean(spriv, I915_PARAM_HAS_RELAXED_DELTA)) {
      fprintf(stderr, "[%s: %u] Kernel 2.6.39 required.\n", __func__, __LINE__);
      return false;
   }

   return true;
}

static bool
intel_detect_swizzling(struct intel_screen *screen)
{
   drm_intel_bo *buffer;
   unsigned long flags = 0;
   unsigned long aligned_pitch;
   uint32_t tiling = I915_TILING_X;
   uint32_t swizzle_mode = 0;

   buffer = drm_intel_bo_alloc_tiled(screen->bufmgr, "swizzle test",
				     64, 64, 4,
				     &tiling, &aligned_pitch, flags);
   if (buffer == NULL)
      return false;

   drm_intel_bo_get_tiling(buffer, &tiling, &swizzle_mode);
   drm_intel_bo_unreference(buffer);

   if (swizzle_mode == I915_BIT_6_SWIZZLE_NONE)
      return false;
   else
      return true;
}

/**
 * Return array of MSAA modes supported by the hardware. The array is
 * zero-terminated and sorted in decreasing order.
 */
const int*
intel_supported_msaa_modes(const struct intel_screen  *screen)
{
   static const int gen8_modes[] = {8, 4, 2, 0, -1};
   static const int gen7_modes[] = {8, 4, 0, -1};
   static const int gen6_modes[] = {4, 0, -1};
   static const int gen4_modes[] = {0, -1};

   if (screen->devinfo->gen >= 8) {
      return gen8_modes;
   } else if (screen->devinfo->gen >= 7) {
      return gen7_modes;
   } else if (screen->devinfo->gen == 6) {
      return gen6_modes;
   } else {
      return gen4_modes;
   }
}

static __DRIconfig**
intel_screen_make_configs(__DRIscreen *dri_screen)
{
   static const mesa_format formats[] = {
      MESA_FORMAT_B5G6R5_UNORM,
      MESA_FORMAT_B8G8R8A8_UNORM
   };

   /* GLX_SWAP_COPY_OML is not supported due to page flipping. */
   static const GLenum back_buffer_modes[] = {
       GLX_SWAP_UNDEFINED_OML, GLX_NONE,
   };

   static const uint8_t singlesample_samples[1] = {0};
   static const uint8_t multisample_samples[2]  = {4, 8};

   struct intel_screen *screen = dri_screen->driverPrivate;
   const struct brw_device_info *devinfo = screen->devinfo;
   uint8_t depth_bits[4], stencil_bits[4];
   __DRIconfig **configs = NULL;

   /* Generate singlesample configs without accumulation buffer. */
   for (int i = 0; i < ARRAY_SIZE(formats); i++) {
      __DRIconfig **new_configs;
      int num_depth_stencil_bits = 2;

      /* Starting with DRI2 protocol version 1.1 we can request a depth/stencil
       * buffer that has a different number of bits per pixel than the color
       * buffer, gen >= 6 supports this.
       */
      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM) {
         depth_bits[1] = 16;
         stencil_bits[1] = 0;
         if (devinfo->gen >= 6) {
             depth_bits[2] = 24;
             stencil_bits[2] = 8;
             num_depth_stencil_bits = 3;
         }
      } else {
         depth_bits[1] = 24;
         stencil_bits[1] = 8;
      }

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes, 2,
                                     singlesample_samples, 1,
                                     false);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate the minimum possible set of configs that include an
    * accumulation buffer.
    */
   for (int i = 0; i < ARRAY_SIZE(formats); i++) {
      __DRIconfig **new_configs;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM) {
         depth_bits[0] = 16;
         stencil_bits[0] = 0;
      } else {
         depth_bits[0] = 24;
         stencil_bits[0] = 8;
      }

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits, stencil_bits, 1,
                                     back_buffer_modes, 1,
                                     singlesample_samples, 1,
                                     true);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate multisample configs.
    *
    * This loop breaks early, and hence is a no-op, on gen < 6.
    *
    * Multisample configs must follow the singlesample configs in order to
    * work around an X server bug present in 1.12. The X server chooses to
    * associate the first listed RGBA888-Z24S8 config, regardless of its
    * sample count, with the 32-bit depth visual used for compositing.
    *
    * Only doublebuffer configs with GLX_SWAP_UNDEFINED_OML behavior are
    * supported.  Singlebuffer configs are not supported because no one wants
    * them.
    */
   for (int i = 0; i < ARRAY_SIZE(formats); i++) {
      if (devinfo->gen < 6)
         break;

      __DRIconfig **new_configs;
      const int num_depth_stencil_bits = 2;
      int num_msaa_modes = 0;

      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM) {
         depth_bits[1] = 16;
         stencil_bits[1] = 0;
      } else {
         depth_bits[1] = 24;
         stencil_bits[1] = 8;
      }

      if (devinfo->gen >= 7)
         num_msaa_modes = 2;
      else if (devinfo->gen == 6)
         num_msaa_modes = 1;

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes, 1,
                                     multisample_samples,
                                     num_msaa_modes,
                                     false);
      configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return configs;
}

static void
set_max_gl_versions(struct intel_screen *screen)
{
   __DRIscreen *psp = screen->driScrnPriv;

   switch (screen->devinfo->gen) {
   case 8:
   case 7:
      psp->max_gl_core_version = 33;
      psp->max_gl_compat_version = 30;
      psp->max_gl_es1_version = 11;
      psp->max_gl_es2_version = 30;
      break;
   case 6:
      psp->max_gl_core_version = 31;
      psp->max_gl_compat_version = 30;
      psp->max_gl_es1_version = 11;
      psp->max_gl_es2_version = 30;
      break;
   case 5:
   case 4:
      psp->max_gl_core_version = 0;
      psp->max_gl_compat_version = 21;
      psp->max_gl_es1_version = 11;
      psp->max_gl_es2_version = 20;
      break;
   default:
      assert(!"unrecognized intel_screen::gen");
      break;
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

   if (psp->image.loader) {
   } else if (psp->dri2.loader->base.version <= 2 ||
       psp->dri2.loader->getBuffersWithFormat == NULL) {
      fprintf(stderr,
	      "\nERROR!  DRI2 loader with getBuffersWithFormat() "
	      "support required\n");
      return false;
   }

   /* Allocate the private area */
   intelScreen = rzalloc(NULL, struct intel_screen);
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return false;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache, brw_config_options.xml);

   intelScreen->driScrnPriv = psp;
   psp->driverPrivate = (void *) intelScreen;

   if (!intel_init_bufmgr(intelScreen))
       return false;

   intelScreen->deviceID = drm_intel_bufmgr_gem_get_devid(intelScreen->bufmgr);
   intelScreen->devinfo = brw_get_device_info(intelScreen->deviceID);
   if (!intelScreen->devinfo)
      return false;

   intelScreen->hw_must_use_separate_stencil = intelScreen->devinfo->gen >= 7;

   intelScreen->hw_has_swizzling = intel_detect_swizzling(intelScreen);

   const char *force_msaa = getenv("INTEL_FORCE_MSAA");
   if (force_msaa) {
      intelScreen->winsys_msaa_samples_override =
         intel_quantize_num_samples(intelScreen, atoi(force_msaa));
      printf("Forcing winsys sample count to %d\n",
             intelScreen->winsys_msaa_samples_override);
   } else {
      intelScreen->winsys_msaa_samples_override = -1;
   }

   set_max_gl_versions(intelScreen);

   /* Notification of GPU resets requires hardware contexts and a kernel new
    * enough to support DRM_IOCTL_I915_GET_RESET_STATS.  If the ioctl is
    * supported, calling it with a context of 0 will either generate EPERM or
    * no error.  If the ioctl is not supported, it always generate EINVAL.
    * Use this to determine whether to advertise the __DRI2_ROBUSTNESS
    * extension to the loader.
    *
    * Don't even try on pre-Gen6, since we don't attempt to use contexts there.
    */
   if (intelScreen->devinfo->gen >= 6) {
      struct drm_i915_reset_stats stats;
      memset(&stats, 0, sizeof(stats));

      const int ret = drmIoctl(psp->fd, DRM_IOCTL_I915_GET_RESET_STATS, &stats);

      intelScreen->has_context_reset_notification =
         (ret != -1 || errno != EINVAL);
   }

   psp->extensions = !intelScreen->has_context_reset_notification
      ? intelScreenExtensions : intelRobustScreenExtensions;

   brw_fs_alloc_reg_sets(intelScreen);
   brw_vec4_alloc_reg_set(intelScreen);

   return (const __DRIconfig**) intel_screen_make_configs(psp);
}

struct intel_buffer {
   __DRIbuffer base;
   drm_intel_bo *bo;
};

static __DRIbuffer *
intelAllocateBuffer(__DRIscreen *screen,
		    unsigned attachment, unsigned format,
		    int width, int height)
{
   struct intel_buffer *intelBuffer;
   struct intel_screen *intelScreen = screen->driverPrivate;

   assert(attachment == __DRI_BUFFER_FRONT_LEFT ||
          attachment == __DRI_BUFFER_BACK_LEFT);

   intelBuffer = calloc(1, sizeof *intelBuffer);
   if (intelBuffer == NULL)
      return NULL;

   /* The front and back buffers are color buffers, which are X tiled. */
   uint32_t tiling = I915_TILING_X;
   unsigned long pitch;
   int cpp = format / 8;
   intelBuffer->bo = drm_intel_bo_alloc_tiled(intelScreen->bufmgr,
                                              "intelAllocateBuffer",
                                              width,
                                              height,
                                              cpp,
                                              &tiling, &pitch,
                                              BO_ALLOC_FOR_RENDER);

   if (intelBuffer->bo == NULL) {
	   free(intelBuffer);
	   return NULL;
   }

   drm_intel_bo_flink(intelBuffer->bo, &intelBuffer->base.name);

   intelBuffer->base.attachment = attachment;
   intelBuffer->base.cpp = cpp;
   intelBuffer->base.pitch = pitch;

   return &intelBuffer->base;
}

static void
intelReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
   struct intel_buffer *intelBuffer = (struct intel_buffer *) buffer;

   drm_intel_bo_unreference(intelBuffer->bo);
   free(intelBuffer);
}

static const struct __DriverAPIRec brw_driver_api = {
   .InitScreen		 = intelInitScreen2,
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = brwCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .AllocateBuffer       = intelAllocateBuffer,
   .ReleaseBuffer        = intelReleaseBuffer
};

static const struct __DRIDriverVtableExtensionRec brw_vtable = {
   .base = { __DRI_DRIVER_VTABLE, 1 },
   .vtable = &brw_driver_api,
};

static const __DRIextension *brw_driver_extensions[] = {
    &driCoreExtension.base,
    &driImageDriverExtension.base,
    &driDRI2Extension.base,
    &brw_vtable.base,
    &brw_config_options.base,
    NULL
};

PUBLIC const __DRIextension **__driDriverGetExtensions_i965(void)
{
   globalDriverAPI = &brw_driver_api;

   return brw_driver_extensions;
}
