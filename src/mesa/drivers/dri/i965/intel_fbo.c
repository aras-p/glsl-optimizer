/**************************************************************************
 *
 * Copyright 2006 VMware, Inc.
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


#include "main/enums.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/teximage.h"
#include "main/image.h"
#include "main/set.h"
#include "main/condrender.h"
#include "util/hash_table.h"

#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_image.h"
#include "intel_screen.h"
#include "intel_tex.h"
#include "brw_context.h"

#define FILE_DEBUG_FLAG DEBUG_FBO

/**
 * Create a new framebuffer object.
 */
static struct gl_framebuffer *
intel_new_framebuffer(struct gl_context * ctx, GLuint name)
{
   /* Only drawable state in intel_framebuffer at this time, just use Mesa's
    * class
    */
   return _mesa_new_framebuffer(ctx, name);
}


/** Called by gl_renderbuffer::Delete() */
static void
intel_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   ASSERT(irb);

   intel_miptree_release(&irb->mt);
   intel_miptree_release(&irb->singlesample_mt);

   _mesa_delete_renderbuffer(ctx, rb);
}

/**
 * \brief Downsample a winsys renderbuffer from mt to singlesample_mt.
 *
 * If the miptree needs no downsample, then skip.
 */
void
intel_renderbuffer_downsample(struct brw_context *brw,
                              struct intel_renderbuffer *irb)
{
   if (!irb->need_downsample)
      return;
   intel_miptree_updownsample(brw, irb->mt, irb->singlesample_mt);
   irb->need_downsample = false;
}

/**
 * \brief Upsample a winsys renderbuffer from singlesample_mt to mt.
 *
 * The upsample is done unconditionally.
 */
void
intel_renderbuffer_upsample(struct brw_context *brw,
                            struct intel_renderbuffer *irb)
{
   assert(!irb->need_downsample);

   intel_miptree_updownsample(brw, irb->singlesample_mt, irb->mt);
}

/**
 * \see dd_function_table::MapRenderbuffer
 */
static void
intel_map_renderbuffer(struct gl_context *ctx,
		       struct gl_renderbuffer *rb,
		       GLuint x, GLuint y, GLuint w, GLuint h,
		       GLbitfield mode,
		       GLubyte **out_map,
		       GLint *out_stride)
{
   struct brw_context *brw = brw_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_mipmap_tree *mt;
   void *map;
   int stride;

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer), not an irb */
      GLint bpp = _mesa_get_format_bytes(rb->Format);
      GLint rowStride = srb->RowStride;
      *out_map = (GLubyte *) srb->Buffer + y * rowStride + x * bpp;
      *out_stride = rowStride;
      return;
   }

   intel_prepare_render(brw);

   /* The MapRenderbuffer API should always return a single-sampled mapping.
    * The case we are asked to map multisampled RBs is in glReadPixels() (or
    * swrast paths like glCopyTexImage()) from a window-system MSAA buffer,
    * and GL expects an automatic resolve to happen.
    *
    * If it's a color miptree, there is a ->singlesample_mt which wraps the
    * actual window system renderbuffer (which we may resolve to at any time),
    * while the miptree itself is our driver-private allocation.  If it's a
    * depth or stencil miptree, we have a private MSAA buffer and no shared
    * singlesample buffer, and since we don't expect anybody to ever actually
    * resolve it, we just make a temporary singlesample buffer now when we
    * have to.
    */
   if (rb->NumSamples > 1) {
      if (!irb->singlesample_mt) {
         irb->singlesample_mt =
            intel_miptree_create_for_renderbuffer(brw, irb->mt->format,
                                                  rb->Width, rb->Height,
                                                  0 /*num_samples*/);
         if (!irb->singlesample_mt)
            goto fail;
         irb->singlesample_mt_is_tmp = true;
         irb->need_downsample = true;
      }

      intel_renderbuffer_downsample(brw, irb);
      mt = irb->singlesample_mt;

      irb->need_map_upsample = mode & GL_MAP_WRITE_BIT;
   } else {
      mt = irb->mt;
   }

   /* For a window-system renderbuffer, we need to flip the mapping we receive
    * upside-down.  So we need to ask for a rectangle on flipped vertically, and
    * we then return a pointer to the bottom of it with a negative stride.
    */
   if (rb->Name == 0) {
      y = rb->Height - y - h;
   }

   intel_miptree_map(brw, mt, irb->mt_level, irb->mt_layer,
		     x, y, w, h, mode, &map, &stride);

   if (rb->Name == 0) {
      map += (h - 1) * stride;
      stride = -stride;
   }

   DBG("%s: rb %d (%s) mt mapped: (%d, %d) (%dx%d) -> %p/%d\n",
       __FUNCTION__, rb->Name, _mesa_get_format_name(rb->Format),
       x, y, w, h, map, stride);

   *out_map = map;
   *out_stride = stride;
   return;

fail:
   *out_map = NULL;
   *out_stride = 0;
}

/**
 * \see dd_function_table::UnmapRenderbuffer
 */
static void
intel_unmap_renderbuffer(struct gl_context *ctx,
			 struct gl_renderbuffer *rb)
{
   struct brw_context *brw = brw_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_mipmap_tree *mt;

   DBG("%s: rb %d (%s)\n", __FUNCTION__,
       rb->Name, _mesa_get_format_name(rb->Format));

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer) */
      /* nothing to do */
      return;
   }

   if (rb->NumSamples > 1) {
      mt = irb->singlesample_mt;
   } else {
      mt = irb->mt;
   }

   intel_miptree_unmap(brw, mt, irb->mt_level, irb->mt_layer);

   if (irb->need_map_upsample) {
      intel_renderbuffer_upsample(brw, irb);
      irb->need_map_upsample = false;
   }

   if (irb->singlesample_mt_is_tmp)
      intel_miptree_release(&irb->singlesample_mt);
}


/**
 * Round up the requested multisample count to the next supported sample size.
 */
unsigned
intel_quantize_num_samples(struct intel_screen *intel, unsigned num_samples)
{
   const int *msaa_modes = intel_supported_msaa_modes(intel);
   int quantized_samples = 0;

   for (int i = 0; msaa_modes[i] != -1; ++i) {
      if (msaa_modes[i] >= num_samples)
         quantized_samples = msaa_modes[i];
      else
         break;
   }

   return quantized_samples;
}

static mesa_format
intel_renderbuffer_format(struct gl_context * ctx, GLenum internalFormat)
{
   struct brw_context *brw = brw_context(ctx);

   switch (internalFormat) {
   default:
      /* Use the same format-choice logic as for textures.
       * Renderbuffers aren't any different from textures for us,
       * except they're less useful because you can't texture with
       * them.
       */
      return ctx->Driver.ChooseTextureFormat(ctx, GL_TEXTURE_2D,
                                             internalFormat,
                                             GL_NONE, GL_NONE);
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* These aren't actual texture formats, so force them here. */
      if (brw->has_separate_stencil) {
	 return MESA_FORMAT_S_UINT8;
      } else {
	 assert(!brw->must_use_separate_stencil);
	 return MESA_FORMAT_Z24_UNORM_S8_UINT;
      }
   }
}

static GLboolean
intel_alloc_private_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                         GLenum internalFormat,
                                         GLuint width, GLuint height)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_screen *screen = brw->intelScreen;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   assert(rb->Format != MESA_FORMAT_NONE);

   rb->NumSamples = intel_quantize_num_samples(screen, rb->NumSamples);
   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

   intel_miptree_release(&irb->mt);

   DBG("%s: %s: %s (%dx%d)\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat),
       _mesa_get_format_name(rb->Format), width, height);

   if (width == 0 || height == 0)
      return true;

   irb->mt = intel_miptree_create_for_renderbuffer(brw, rb->Format,
						   width, height,
                                                   rb->NumSamples);
   if (!irb->mt)
      return false;

   irb->layer_count = 1;

   return true;
}

/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
intel_alloc_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   rb->Format = intel_renderbuffer_format(ctx, internalFormat);
   return intel_alloc_private_renderbuffer_storage(ctx, rb, internalFormat, width, height);
}

static void
intel_image_target_renderbuffer_storage(struct gl_context *ctx,
					struct gl_renderbuffer *rb,
					void *image_handle)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_renderbuffer *irb;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = brw->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   if (image->planar_format && image->planar_format->nplanes > 1) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(planar buffers are not "
               "supported as render targets.");
      return;
   }

   /* Buffers originating from outside are for read-only. */
   if (image->dma_buf_imported) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(dma buffers are read-only)");
      return;
   }

   /* __DRIimage is opaque to the core so it has to be checked here */
   switch (image->format) {
   case MESA_FORMAT_R8G8B8A8_UNORM:
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(unsupported image format");
      return;
      break;
   default:
      break;
   }

   irb = intel_renderbuffer(rb);
   intel_miptree_release(&irb->mt);
   irb->mt = intel_miptree_create_for_bo(brw,
                                         image->bo,
                                         image->format,
                                         image->offset,
                                         image->width,
                                         image->height,
                                         image->pitch);
   if (!irb->mt)
      return;

   rb->InternalFormat = image->internal_format;
   rb->Width = image->width;
   rb->Height = image->height;
   rb->Format = image->format;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, image->internal_format);
   rb->NeedsFinishRenderTexture = true;
   irb->layer_count = 1;
}

/**
 * Called by _mesa_resize_framebuffer() for each hardware renderbuffer when a
 * window system framebuffer is resized.
 *
 * Any actual buffer reallocations for hardware renderbuffers (which would
 * have triggered _mesa_resize_framebuffer()) were done by
 * intel_process_dri2_buffer().
 */
static GLboolean
intel_alloc_window_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return true;
}

/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
intel_nop_alloc_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                        GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "intel_nop_alloc_storage should never be called.");
   return false;
}

/**
 * Create a new intel_renderbuffer which corresponds to an on-screen window,
 * not a user-created renderbuffer.
 *
 * \param num_samples must be quantized.
 */
struct intel_renderbuffer *
intel_create_renderbuffer(mesa_format format, unsigned num_samples)
{
   struct intel_renderbuffer *irb;
   struct gl_renderbuffer *rb;

   GET_CURRENT_CONTEXT(ctx);

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   rb = &irb->Base.Base;
   irb->layer_count = 1;

   _mesa_init_renderbuffer(rb, 0);
   rb->ClassID = INTEL_RB_CLASS;
   rb->_BaseFormat = _mesa_get_format_base_format(format);
   rb->Format = format;
   rb->InternalFormat = rb->_BaseFormat;
   rb->NumSamples = num_samples;

   /* intel-specific methods */
   rb->Delete = intel_delete_renderbuffer;
   rb->AllocStorage = intel_alloc_window_storage;

   return irb;
}

/**
 * Private window-system buffers (as opposed to ones shared with the display
 * server created with intel_create_renderbuffer()) are most similar in their
 * handling to user-created renderbuffers, but they have a resize handler that
 * may be called at intel_update_renderbuffers() time.
 *
 * \param num_samples must be quantized.
 */
struct intel_renderbuffer *
intel_create_private_renderbuffer(mesa_format format, unsigned num_samples)
{
   struct intel_renderbuffer *irb;

   irb = intel_create_renderbuffer(format, num_samples);
   irb->Base.Base.AllocStorage = intel_alloc_private_renderbuffer_storage;

   return irb;
}

/**
 * Create a new renderbuffer object.
 * Typically called via glBindRenderbufferEXT().
 */
static struct gl_renderbuffer *
intel_new_renderbuffer(struct gl_context * ctx, GLuint name)
{
   struct intel_renderbuffer *irb;
   struct gl_renderbuffer *rb;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   rb = &irb->Base.Base;

   _mesa_init_renderbuffer(rb, name);
   rb->ClassID = INTEL_RB_CLASS;

   /* intel-specific methods */
   rb->Delete = intel_delete_renderbuffer;
   rb->AllocStorage = intel_alloc_renderbuffer_storage;
   /* span routines set in alloc_storage function */

   return rb;
}

static bool
intel_renderbuffer_update_wrapper(struct brw_context *brw,
                                  struct intel_renderbuffer *irb,
                                  struct gl_texture_image *image,
                                  uint32_t layer,
                                  bool layered)
{
   struct gl_renderbuffer *rb = &irb->Base.Base;
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   int level = image->Level;

   rb->AllocStorage = intel_nop_alloc_storage;

   /* adjust for texture view parameters */
   layer += image->TexObject->MinLayer;
   level += image->TexObject->MinLevel;

   intel_miptree_check_level_layer(mt, level, layer);
   irb->mt_level = level;

   int layer_multiplier;
   switch (mt->msaa_layout) {
      case INTEL_MSAA_LAYOUT_UMS:
      case INTEL_MSAA_LAYOUT_CMS:
         layer_multiplier = mt->num_samples;
         break;

      default:
         layer_multiplier = 1;
   }

   irb->mt_layer = layer_multiplier * layer;

   if (layered) {
      irb->layer_count = image->TexObject->NumLayers ?: mt->level[level].depth / layer_multiplier;
   } else {
      irb->layer_count = 1;
   }

   intel_miptree_reference(&irb->mt, mt);

   intel_renderbuffer_set_draw_offset(irb);

   if (mt->hiz_mt == NULL && brw_is_hiz_depth_format(brw, rb->Format)) {
      intel_miptree_alloc_hiz(brw, mt);
      if (!mt->hiz_mt)
	 return false;
   }

   return true;
}

void
intel_renderbuffer_set_draw_offset(struct intel_renderbuffer *irb)
{
   unsigned int dst_x, dst_y;

   /* compute offset of the particular 2D image within the texture region */
   intel_miptree_get_image_offset(irb->mt,
				  irb->mt_level,
				  irb->mt_layer,
				  &dst_x, &dst_y);

   irb->draw_x = dst_x;
   irb->draw_y = dst_y;
}

/**
 * Called by glFramebufferTexture[123]DEXT() (and other places) to
 * prepare for rendering into texture memory.  This might be called
 * many times to choose different texture levels, cube faces, etc
 * before intel_finish_render_texture() is ever called.
 */
static void
intel_render_texture(struct gl_context * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_renderbuffer *rb = att->Renderbuffer;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct gl_texture_image *image = rb->TexImage;
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   int layer;

   (void) fb;

   if (att->CubeMapFace > 0) {
      assert(att->Zoffset == 0);
      layer = att->CubeMapFace;
   } else {
      layer = att->Zoffset;
   }

   if (!intel_image->mt) {
      /* Fallback on drawing to a texture that doesn't have a miptree
       * (has a border, width/height 0, etc.)
       */
      _swrast_render_texture(ctx, fb, att);
      return;
   }

   intel_miptree_check_level_layer(mt, att->TextureLevel, layer);

   if (!intel_renderbuffer_update_wrapper(brw, irb, image, layer, att->Layered)) {
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render %s texture tex=%u w=%d h=%d d=%d refcount=%d\n",
       _mesa_get_format_name(image->TexFormat),
       att->Texture->Name, image->Width, image->Height, image->Depth,
       rb->RefCount);
}


#define fbo_incomplete(fb, ...) do {                                          \
      static GLuint msg_id = 0;                                               \
      if (unlikely(ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)) {    \
         _mesa_gl_debug(ctx, &msg_id,                                         \
                        MESA_DEBUG_TYPE_OTHER,                                \
                        MESA_DEBUG_SEVERITY_MEDIUM,                           \
                        __VA_ARGS__);                                         \
      }                                                                       \
      DBG(__VA_ARGS__);                                                       \
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;                               \
   } while (0)

/**
 * Do additional "completeness" testing of a framebuffer object.
 */
static void
intel_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_renderbuffer *depthRb =
      intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *stencilRb =
      intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *depth_mt = NULL, *stencil_mt = NULL;
   int i;

   DBG("%s() on fb %p (%s)\n", __FUNCTION__,
       fb, (fb == ctx->DrawBuffer ? "drawbuffer" :
	    (fb == ctx->ReadBuffer ? "readbuffer" : "other buffer")));

   if (depthRb)
      depth_mt = depthRb->mt;
   if (stencilRb) {
      stencil_mt = stencilRb->mt;
      if (stencil_mt->stencil_mt)
	 stencil_mt = stencil_mt->stencil_mt;
   }

   if (depth_mt && stencil_mt) {
      if (brw->gen >= 6) {
         /* For gen >= 6, we are using the lod/minimum-array-element fields
          * and supporting layered rendering. This means that we must restrict
          * the depth & stencil attachments to match in various more retrictive
          * ways. (width, height, depth, LOD and layer)
          */
	 if (depth_mt->physical_width0 != stencil_mt->physical_width0 ||
             depth_mt->physical_height0 != stencil_mt->physical_height0 ||
             depth_mt->physical_depth0 != stencil_mt->physical_depth0 ||
             depthRb->mt_level != stencilRb->mt_level ||
	     depthRb->mt_layer != stencilRb->mt_layer) {
	    fbo_incomplete(fb,
                           "FBO incomplete: depth and stencil must match in"
                           "width, height, depth, LOD and layer\n");
	 }
      }
      if (depth_mt == stencil_mt) {
	 /* For true packed depth/stencil (not faked on prefers-separate-stencil
	  * hardware) we need to be sure they're the same level/layer, since
	  * we'll be emitting a single packet describing the packed setup.
	  */
	 if (depthRb->mt_level != stencilRb->mt_level ||
	     depthRb->mt_layer != stencilRb->mt_layer) {
	    fbo_incomplete(fb,
                           "FBO incomplete: depth image level/layer %d/%d != "
                           "stencil image %d/%d\n",
                           depthRb->mt_level,
                           depthRb->mt_layer,
                           stencilRb->mt_level,
                           stencilRb->mt_layer);
	 }
      } else {
	 if (!brw->has_separate_stencil) {
	    fbo_incomplete(fb, "FBO incomplete: separate stencil "
                           "unsupported\n");
	 }
	 if (stencil_mt->format != MESA_FORMAT_S_UINT8) {
	    fbo_incomplete(fb, "FBO incomplete: separate stencil is %s "
                           "instead of S8\n",
                           _mesa_get_format_name(stencil_mt->format));
	 }
	 if (brw->gen < 7 && !intel_renderbuffer_has_hiz(depthRb)) {
	    /* Before Gen7, separate depth and stencil buffers can be used
	     * only if HiZ is enabled. From the Sandybridge PRM, Volume 2,
	     * Part 1, Bit 3DSTATE_DEPTH_BUFFER.SeparateStencilBufferEnable:
	     *     [DevSNB]: This field must be set to the same value (enabled
	     *     or disabled) as Hierarchical Depth Buffer Enable.
	     */
	    fbo_incomplete(fb, "FBO incomplete: separate stencil "
                           "without HiZ\n");
	 }
      }
   }

   for (i = 0; i < Elements(fb->Attachment); i++) {
      struct gl_renderbuffer *rb;
      struct intel_renderbuffer *irb;

      if (fb->Attachment[i].Type == GL_NONE)
	 continue;

      /* A supported attachment will have a Renderbuffer set either
       * from being a Renderbuffer or being a texture that got the
       * intel_wrap_texture() treatment.
       */
      rb = fb->Attachment[i].Renderbuffer;
      if (rb == NULL) {
	 fbo_incomplete(fb, "FBO incomplete: attachment without "
                        "renderbuffer\n");
	 continue;
      }

      if (fb->Attachment[i].Type == GL_TEXTURE) {
	 if (rb->TexImage->Border) {
	    fbo_incomplete(fb, "FBO incomplete: texture with border\n");
	    continue;
	 }
      }

      irb = intel_renderbuffer(rb);
      if (irb == NULL) {
	 fbo_incomplete(fb, "FBO incomplete: software rendering "
                        "renderbuffer\n");
	 continue;
      }

      if (!brw_render_target_supported(brw, rb)) {
	 fbo_incomplete(fb, "FBO incomplete: Unsupported HW "
                        "texture/renderbuffer format attached: %s\n",
                        _mesa_get_format_name(intel_rb_format(irb)));
      }
   }
}

/**
 * Try to do a glBlitFramebuffer using glCopyTexSubImage2D
 * We can do this when the dst renderbuffer is actually a texture and
 * there is no scaling, mirroring or scissoring.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
intel_blit_framebuffer_with_blitter(struct gl_context *ctx,
                                    GLint srcX0, GLint srcY0,
                                    GLint srcX1, GLint srcY1,
                                    GLint dstX0, GLint dstY0,
                                    GLint dstX1, GLint dstY1,
                                    GLbitfield mask, GLenum filter)
{
   struct brw_context *brw = brw_context(ctx);

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking for the buffers.
    */
   intel_prepare_render(brw);

   if (mask & GL_COLOR_BUFFER_BIT) {
      GLint i;
      const struct gl_framebuffer *drawFb = ctx->DrawBuffer;
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      struct gl_renderbuffer *src_rb = readFb->_ColorReadBuffer;
      struct intel_renderbuffer *src_irb = intel_renderbuffer(src_rb);

      if (!src_irb) {
         perf_debug("glBlitFramebuffer(): missing src renderbuffer.  "
                    "Falling back to software rendering.\n");
         return mask;
      }

      /* If the source and destination are the same size with no mirroring,
       * the rectangles are within the size of the texture and there is no
       * scissor, then we can probably use the blit engine.
       */
      if (!(srcX0 - srcX1 == dstX0 - dstX1 &&
            srcY0 - srcY1 == dstY0 - dstY1 &&
            srcX1 >= srcX0 &&
            srcY1 >= srcY0 &&
            srcX0 >= 0 && srcX1 <= readFb->Width &&
            srcY0 >= 0 && srcY1 <= readFb->Height &&
            dstX0 >= 0 && dstX1 <= drawFb->Width &&
            dstY0 >= 0 && dstY1 <= drawFb->Height &&
            !(ctx->Scissor.EnableFlags))) {
         perf_debug("glBlitFramebuffer(): non-1:1 blit.  "
                    "Falling back to software rendering.\n");
         return mask;
      }

      /* Blit to all active draw buffers.  We don't do any pre-checking,
       * because we assume that copying to MRTs is rare, and failure midway
       * through copying is even more rare.  Even if it was to occur, it's
       * safe to let meta start the copy over from scratch, because
       * glBlitFramebuffer completely overwrites the destination pixels, and
       * results are undefined if any destination pixels have a dependency on
       * source pixels.
       */
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         struct gl_renderbuffer *dst_rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
         struct intel_renderbuffer *dst_irb = intel_renderbuffer(dst_rb);

         if (!dst_irb) {
            perf_debug("glBlitFramebuffer(): missing dst renderbuffer.  "
                       "Falling back to software rendering.\n");
            return mask;
         }

         if (!intel_miptree_blit(brw,
                                 src_irb->mt,
                                 src_irb->mt_level, src_irb->mt_layer,
                                 srcX0, srcY0, src_rb->Name == 0,
                                 dst_irb->mt,
                                 dst_irb->mt_level, dst_irb->mt_layer,
                                 dstX0, dstY0, dst_rb->Name == 0,
                                 dstX1 - dstX0, dstY1 - dstY0, GL_COPY)) {
            perf_debug("glBlitFramebuffer(): unknown blit failure.  "
                       "Falling back to software rendering.\n");
            return mask;
         }
      }

      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   return mask;
}

static void
intel_blit_framebuffer(struct gl_context *ctx,
                       GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
   struct brw_context *brw = brw_context(ctx);

   /* Page 679 of OpenGL 4.4 spec says:
    *    "Added BlitFramebuffer to commands affected by conditional rendering in
    *     section 10.10 (Bug 9562)."
    */
   if (!_mesa_check_conditional_render(ctx))
      return;

   mask = brw_blorp_framebuffer(brw,
                                srcX0, srcY0, srcX1, srcY1,
                                dstX0, dstY0, dstX1, dstY1,
                                mask, filter);
   if (mask == 0x0)
      return;

   if (brw->gen >= 8 && (mask & GL_STENCIL_BUFFER_BIT)) {
      brw_meta_fbo_stencil_blit(brw_context(ctx),
                                srcX0, srcY0, srcX1, srcY1,
                                dstX0, dstY0, dstX1, dstY1);
      mask &= ~GL_STENCIL_BUFFER_BIT;
      if (mask == 0x0)
         return;
   }

   /* Try using the BLT engine. */
   mask = intel_blit_framebuffer_with_blitter(ctx,
                                              srcX0, srcY0, srcX1, srcY1,
                                              dstX0, dstY0, dstX1, dstY1,
                                              mask, filter);
   if (mask == 0x0)
      return;

   mask = _mesa_meta_BlitFramebuffer(ctx,
                                     srcX0, srcY0, srcX1, srcY1,
                                     dstX0, dstY0, dstX1, dstY1,
                                     mask, filter);
   if (mask == 0x0)
      return;

   _swrast_BlitFramebuffer(ctx,
                           srcX0, srcY0, srcX1, srcY1,
                           dstX0, dstY0, dstX1, dstY1,
                           mask, filter);
}

/**
 * Does the renderbuffer have hiz enabled?
 */
bool
intel_renderbuffer_has_hiz(struct intel_renderbuffer *irb)
{
   return intel_miptree_level_has_hiz(irb->mt, irb->mt_level);
}

bool
intel_renderbuffer_resolve_hiz(struct brw_context *brw,
			       struct intel_renderbuffer *irb)
{
   if (irb->mt)
      return intel_miptree_slice_resolve_hiz(brw,
                                             irb->mt,
                                             irb->mt_level,
                                             irb->mt_layer);

   return false;
}

void
intel_renderbuffer_att_set_needs_depth_resolve(struct gl_renderbuffer_attachment *att)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);
   if (irb->mt) {
      if (att->Layered) {
         intel_miptree_set_all_slices_need_depth_resolve(irb->mt, irb->mt_level);
      } else {
         intel_miptree_slice_set_needs_depth_resolve(irb->mt,
                                                     irb->mt_level,
                                                     irb->mt_layer);
      }
   }
}

bool
intel_renderbuffer_resolve_depth(struct brw_context *brw,
				 struct intel_renderbuffer *irb)
{
   if (irb->mt)
      return intel_miptree_slice_resolve_depth(brw,
                                               irb->mt,
                                               irb->mt_level,
                                               irb->mt_layer);

   return false;
}

void
intel_renderbuffer_move_to_temp(struct brw_context *brw,
                                struct intel_renderbuffer *irb,
                                bool invalidate)
{
   struct gl_renderbuffer *rb =&irb->Base.Base;
   struct intel_texture_image *intel_image = intel_texture_image(rb->TexImage);
   struct intel_mipmap_tree *new_mt;
   int width, height, depth;

   intel_miptree_get_dimensions_for_image(rb->TexImage, &width, &height, &depth);

   new_mt = intel_miptree_create(brw, rb->TexImage->TexObject->Target,
                                 intel_image->base.Base.TexFormat,
                                 intel_image->base.Base.Level,
                                 intel_image->base.Base.Level,
                                 width, height, depth,
                                 true,
                                 irb->mt->num_samples,
                                 INTEL_MIPTREE_TILING_ANY,
                                 false);

   if (brw_is_hiz_depth_format(brw, new_mt->format)) {
      intel_miptree_alloc_hiz(brw, new_mt);
   }

   intel_miptree_copy_teximage(brw, intel_image, new_mt, invalidate);

   intel_miptree_reference(&irb->mt, intel_image->mt);
   intel_renderbuffer_set_draw_offset(irb);
   intel_miptree_release(&new_mt);
}

void
brw_render_cache_set_clear(struct brw_context *brw)
{
   struct set_entry *entry;

   set_foreach(brw->render_cache, entry) {
      _mesa_set_remove(brw->render_cache, entry);
   }
}

void
brw_render_cache_set_add_bo(struct brw_context *brw, drm_intel_bo *bo)
{
   _mesa_set_add(brw->render_cache, _mesa_hash_pointer(bo), bo);
}

/**
 * Emits an appropriate flush for a BO if it has been rendered to within the
 * same batchbuffer as a read that's about to be emitted.
 *
 * The GPU has separate, incoherent caches for the render cache and the
 * sampler cache, along with other caches.  Usually data in the different
 * caches don't interact (e.g. we don't render to our driver-generated
 * immediate constant data), but for render-to-texture in FBOs we definitely
 * do.  When a batchbuffer is flushed, the kernel will ensure that everything
 * necessary is flushed before another use of that BO, but for reuse from
 * different caches within a batchbuffer, it's all our responsibility.
 */
void
brw_render_cache_set_check_flush(struct brw_context *brw, drm_intel_bo *bo)
{
   if (!_mesa_set_search(brw->render_cache, _mesa_hash_pointer(bo), bo))
      return;

   intel_batchbuffer_emit_mi_flush(brw);
}

/**
 * Do one-time context initializations related to GL_EXT_framebuffer_object.
 * Hook in device driver functions.
 */
void
intel_fbo_init(struct brw_context *brw)
{
   struct dd_function_table *dd = &brw->ctx.Driver;
   dd->NewFramebuffer = intel_new_framebuffer;
   dd->NewRenderbuffer = intel_new_renderbuffer;
   dd->MapRenderbuffer = intel_map_renderbuffer;
   dd->UnmapRenderbuffer = intel_unmap_renderbuffer;
   dd->RenderTexture = intel_render_texture;
   dd->ValidateFramebuffer = intel_validate_framebuffer;
   dd->BlitFramebuffer = intel_blit_framebuffer;
   dd->EGLImageTargetRenderbufferStorage =
      intel_image_target_renderbuffer_storage;

   brw->render_cache = _mesa_set_create(brw, _mesa_key_pointer_equal);
}
