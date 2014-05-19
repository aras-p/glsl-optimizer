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

#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_FBO

static struct gl_renderbuffer *
intel_new_renderbuffer(struct gl_context * ctx, GLuint name);

struct intel_region*
intel_get_rb_region(struct gl_framebuffer *fb, GLuint attIndex)
{
   struct intel_renderbuffer *irb = intel_get_renderbuffer(fb, attIndex);
   if (irb && irb->mt)
      return irb->mt->region;
   else
      return NULL;
}

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

   _mesa_delete_renderbuffer(ctx, rb);
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
   struct intel_context *intel = intel_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
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

   intel_prepare_render(intel);

   /* For a window-system renderbuffer, we need to flip the mapping we receive
    * upside-down.  So we need to ask for a rectangle on flipped vertically, and
    * we then return a pointer to the bottom of it with a negative stride.
    */
   if (rb->Name == 0) {
      y = rb->Height - y - h;
   }

   intel_miptree_map(intel, irb->mt, irb->mt_level, irb->mt_layer,
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
}

/**
 * \see dd_function_table::UnmapRenderbuffer
 */
static void
intel_unmap_renderbuffer(struct gl_context *ctx,
			 struct gl_renderbuffer *rb)
{
   struct intel_context *intel = intel_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   DBG("%s: rb %d (%s)\n", __FUNCTION__,
       rb->Name, _mesa_get_format_name(rb->Format));

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer) */
      /* nothing to do */
      return;
   }

   intel_miptree_unmap(intel, irb->mt, irb->mt_level, irb->mt_layer);
}

static mesa_format
intel_renderbuffer_format(struct gl_context * ctx, GLenum internalFormat)
{
   struct intel_context *intel = intel_context(ctx);

   switch (internalFormat) {
   default:
      /* Use the same format-choice logic as for textures.
       * Renderbuffers aren't any different from textures for us,
       * except they're less useful because you can't texture with
       * them.
       */
      return intel->ctx.Driver.ChooseTextureFormat(ctx, GL_TEXTURE_2D,
                                                   internalFormat,
                                                   GL_NONE, GL_NONE);
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* These aren't actual texture formats, so force them here. */
      return MESA_FORMAT_Z24_UNORM_S8_UINT;
   }
}

static GLboolean
intel_alloc_private_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                         GLenum internalFormat,
                                         GLuint width, GLuint height)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   assert(rb->Format != MESA_FORMAT_NONE);

   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

   intel_miptree_release(&irb->mt);

   DBG("%s: %s: %s (%dx%d)\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat),
       _mesa_get_format_name(rb->Format), width, height);

   if (width == 0 || height == 0)
      return true;

   irb->mt = intel_miptree_create_for_renderbuffer(intel, rb->Format,
						   width, height);
   if (!irb->mt)
      return false;

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
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   /* __DRIimage is opaque to the core so it has to be checked here */
   switch (image->format) {
   case MESA_FORMAT_R8G8B8A8_UNORM:
      _mesa_error(&intel->ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(unsupported image format");
      return;
      break;
   default:
      break;
   }

   irb = intel_renderbuffer(rb);
   intel_miptree_release(&irb->mt);
   irb->mt = intel_miptree_create_for_bo(intel,
                                         image->region->bo,
                                         image->format,
                                         image->offset,
                                         image->region->width,
                                         image->region->height,
                                         image->region->pitch,
                                         image->region->tiling);
   if (!irb->mt)
      return;

   rb->InternalFormat = image->internal_format;
   rb->Width = image->region->width;
   rb->Height = image->region->height;
   rb->Format = image->format;
   rb->_BaseFormat = _mesa_base_fbo_format(&intel->ctx,
					   image->internal_format);
   rb->NeedsFinishRenderTexture = true;
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
   _mesa_problem(ctx, "intel_op_alloc_storage should never be called.");
   return false;
}

/**
 * Create a new intel_renderbuffer which corresponds to an on-screen window,
 * not a user-created renderbuffer.
 */
struct intel_renderbuffer *
intel_create_renderbuffer(mesa_format format)
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

   _mesa_init_renderbuffer(rb, 0);
   rb->ClassID = INTEL_RB_CLASS;
   rb->_BaseFormat = _mesa_get_format_base_format(format);
   rb->Format = format;
   rb->InternalFormat = rb->_BaseFormat;

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
 */
struct intel_renderbuffer *
intel_create_private_renderbuffer(mesa_format format)
{
   struct intel_renderbuffer *irb;

   irb = intel_create_renderbuffer(format);
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
   /*struct intel_context *intel = intel_context(ctx); */
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


/**
 * Called via glBindFramebufferEXT().
 */
static void
intel_bind_framebuffer(struct gl_context * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      intel_draw_buffer(ctx);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}


/**
 * Called via glFramebufferRenderbufferEXT().
 */
static void
intel_framebuffer_renderbuffer(struct gl_context * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   DBG("Intel FramebufferRenderbuffer %u %u\n", fb->Name, rb ? rb->Name : 0);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   intel_draw_buffer(ctx);
}

static bool
intel_renderbuffer_update_wrapper(struct intel_context *intel,
                                  struct intel_renderbuffer *irb,
				  struct gl_texture_image *image,
                                  uint32_t layer)
{
   struct gl_renderbuffer *rb = &irb->Base.Base;
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   int level = image->Level;

   rb->AllocStorage = intel_nop_alloc_storage;

   intel_miptree_check_level_layer(mt, level, layer);
   irb->mt_level = level;
   irb->mt_layer = layer;

   intel_miptree_reference(&irb->mt, mt);

   intel_renderbuffer_set_draw_offset(irb);

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
   struct intel_context *intel = intel_context(ctx);
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

   if (!intel_renderbuffer_update_wrapper(intel, irb, image, layer)) {
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render %s texture tex=%u w=%d h=%d d=%d refcount=%d\n",
       _mesa_get_format_name(image->TexFormat),
       att->Texture->Name, image->Width, image->Height, image->Depth,
       rb->RefCount);

   /* update drawing region, etc */
   intel_draw_buffer(ctx);
}


/**
 * Called by Mesa when rendering to a texture is done.
 */
static void
intel_finish_render_texture(struct gl_context * ctx, struct gl_renderbuffer *rb)
{
   struct intel_context *intel = intel_context(ctx);

   DBG("Finish render %s texture\n", _mesa_get_format_name(rb->Format));

   /* Since we've (probably) rendered to the texture and will (likely) use
    * it in the texture domain later on in this batchbuffer, flush the
    * batch.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer like GEM does in the kernel.
    */
   intel_batchbuffer_emit_mi_flush(intel);
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
   struct intel_context *intel = intel_context(ctx);
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
   if (stencilRb)
      stencil_mt = stencilRb->mt;

   if (depth_mt && stencil_mt) {
      /* Make sure that the depth and stencil buffers are actually the same
       * slice of the same miptree, since we only support packed
       * depth/stencil.
       */
      if (depth_mt == stencil_mt) {
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
         fbo_incomplete(fb, "FBO incomplete: separate stencil unsupported\n");
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

      if (!intel->vtbl.render_target_supported(intel, rb)) {
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
   struct intel_context *intel = intel_context(ctx);

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
            !ctx->Scissor.EnableFlags)) {
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

         mesa_format src_format = _mesa_get_srgb_format_linear(src_rb->Format);
         mesa_format dst_format = _mesa_get_srgb_format_linear(dst_rb->Format);
         if (src_format != dst_format) {
            perf_debug("glBlitFramebuffer(): unsupported blit from %s to %s.  "
                       "Falling back to software rendering.\n",
                       _mesa_get_format_name(src_format),
                       _mesa_get_format_name(dst_format));
            return mask;
         }

         if (!intel_miptree_blit(intel,
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
   /* Try using the BLT engine. */
   mask = intel_blit_framebuffer_with_blitter(ctx,
                                              srcX0, srcY0, srcX1, srcY1,
                                              dstX0, dstY0, dstX1, dstY1,
                                              mask, filter);
   if (mask == 0x0)
      return;


   _mesa_meta_and_swrast_BlitFramebuffer(ctx,
                                         srcX0, srcY0, srcX1, srcY1,
                                         dstX0, dstY0, dstX1, dstY1,
                                         mask, filter);
}

/**
 * Do one-time context initializations related to GL_EXT_framebuffer_object.
 * Hook in device driver functions.
 */
void
intel_fbo_init(struct intel_context *intel)
{
   intel->ctx.Driver.NewFramebuffer = intel_new_framebuffer;
   intel->ctx.Driver.NewRenderbuffer = intel_new_renderbuffer;
   intel->ctx.Driver.MapRenderbuffer = intel_map_renderbuffer;
   intel->ctx.Driver.UnmapRenderbuffer = intel_unmap_renderbuffer;
   intel->ctx.Driver.BindFramebuffer = intel_bind_framebuffer;
   intel->ctx.Driver.FramebufferRenderbuffer = intel_framebuffer_renderbuffer;
   intel->ctx.Driver.RenderTexture = intel_render_texture;
   intel->ctx.Driver.FinishRenderTexture = intel_finish_render_texture;
   intel->ctx.Driver.ValidateFramebuffer = intel_validate_framebuffer;
   intel->ctx.Driver.BlitFramebuffer = intel_blit_framebuffer;
   intel->ctx.Driver.EGLImageTargetRenderbufferStorage =
      intel_image_target_renderbuffer_storage;
}
