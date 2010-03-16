/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/texrender.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"


#define FILE_DEBUG_FLAG DEBUG_FBO


/**
 * Create a new framebuffer object.
 */
static struct gl_framebuffer *
intel_new_framebuffer(GLcontext * ctx, GLuint name)
{
   /* Only drawable state in intel_framebuffer at this time, just use Mesa's
    * class
    */
   return _mesa_new_framebuffer(ctx, name);
}


/** Called by gl_renderbuffer::Delete() */
static void
intel_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   ASSERT(irb);

   if (intel && irb->region) {
      intel_region_release(&irb->region);
   }

   free(irb);
}


/**
 * Return a pointer to a specific pixel in a renderbuffer.
 */
static void *
intel_get_pointer(GLcontext * ctx, struct gl_renderbuffer *rb,
                  GLint x, GLint y)
{
   /* By returning NULL we force all software rendering to go through
    * the span routines.
    */
   return NULL;
}


/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
intel_alloc_renderbuffer_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   int cpp;

   ASSERT(rb->Name != 0);

   switch (internalFormat) {
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      rb->Format = MESA_FORMAT_RGB565;
      rb->DataType = GL_UNSIGNED_BYTE;
      break;
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->Format = MESA_FORMAT_XRGB8888;
      rb->DataType = GL_UNSIGNED_BYTE;
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      rb->Format = MESA_FORMAT_ARGB8888;
      rb->DataType = GL_UNSIGNED_BYTE;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* alloc a depth+stencil buffer */
      rb->Format = MESA_FORMAT_S8_Z24;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      break;
   case GL_DEPTH_COMPONENT16:
      rb->Format = MESA_FORMAT_Z16;
      rb->DataType = GL_UNSIGNED_SHORT;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->Format = MESA_FORMAT_S8_Z24;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->Format = MESA_FORMAT_S8_Z24;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format in intel_alloc_renderbuffer_storage");
      return GL_FALSE;
   }

   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);
   cpp = _mesa_get_format_bytes(rb->Format);

   intelFlush(ctx);

   /* free old region */
   if (irb->region) {
      intel_region_release(&irb->region);
   }

   /* allocate new memory region/renderbuffer */

   /* alloc hardware renderbuffer */
   DBG("Allocating %d x %d Intel RBO\n", width, height);

   irb->region = intel_region_alloc(intel, I915_TILING_NONE, cpp,
				    width, height, GL_TRUE);
   if (!irb->region)
      return GL_FALSE;       /* out of memory? */

   ASSERT(irb->region->buffer);

   rb->Width = width;
   rb->Height = height;

   return GL_TRUE;
}


#if FEATURE_OES_EGL_image
static void
intel_image_target_renderbuffer_storage(GLcontext *ctx,
					struct gl_renderbuffer *rb,
					void *image_handle)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(intel->driContext, image_handle,
					      intel->driContext->loaderPrivate);
   if (image == NULL)
      return;

   irb = intel_renderbuffer(rb);
   if (irb->region)
      intel_region_release(&irb->region);
   intel_region_reference(&irb->region, image->region);

   rb->InternalFormat = image->internal_format;
   rb->Width = image->region->width;
   rb->Height = image->region->height;
   rb->Format = image->format;
   rb->DataType = image->data_type;
   rb->_BaseFormat = _mesa_base_fbo_format(&intel->ctx,
					   image->internal_format);
}
#endif

/**
 * Called for each hardware renderbuffer when a _window_ is resized.
 * Just update fields.
 * Not used for user-created renderbuffers!
 */
static GLboolean
intel_alloc_window_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return GL_TRUE;
}


static void
intel_resize_buffers(GLcontext *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
   int i;

   _mesa_resize_framebuffer(ctx, fb, width, height);

   fb->Initialized = GL_TRUE; /* XXX remove someday */

   if (fb->Name != 0) {
      return;
   }


   /* Make sure all window system renderbuffers are up to date */
   for (i = BUFFER_FRONT_LEFT; i <= BUFFER_BACK_RIGHT; i++) {
      struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;

      /* only resize if size is changing */
      if (rb && (rb->Width != width || rb->Height != height)) {
	 rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height);
      }
   }
}


/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
intel_nop_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                        GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "intel_op_alloc_storage should never be called.");
   return GL_FALSE;
}


void
intel_renderbuffer_set_region(struct intel_renderbuffer *rb,
			      struct intel_region *region)
{
   struct intel_region *old;

   old = rb->region;
   rb->region = NULL;
   intel_region_reference(&rb->region, region);
   intel_region_release(&old);
}


/**
 * Create a new intel_renderbuffer which corresponds to an on-screen window,
 * not a user-created renderbuffer.
 */
struct intel_renderbuffer *
intel_create_renderbuffer(gl_format format)
{
   GET_CURRENT_CONTEXT(ctx);

   struct intel_renderbuffer *irb;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, 0);
   irb->Base.ClassID = INTEL_RB_CLASS;

   switch (format) {
   case MESA_FORMAT_RGB565:
      irb->Base._BaseFormat = GL_RGB;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      break;
   case MESA_FORMAT_XRGB8888:
      irb->Base._BaseFormat = GL_RGB;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      break;
   case MESA_FORMAT_ARGB8888:
      irb->Base._BaseFormat = GL_RGBA;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      break;
   case MESA_FORMAT_Z16:
      irb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      irb->Base.DataType = GL_UNSIGNED_SHORT;
      break;
   case MESA_FORMAT_X8_Z24:
      irb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      irb->Base.DataType = GL_UNSIGNED_INT;
      break;
   case MESA_FORMAT_S8_Z24:
      irb->Base._BaseFormat = GL_DEPTH_STENCIL;
      irb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
      break;
   default:
      _mesa_problem(NULL,
                    "Unexpected intFormat in intel_create_renderbuffer");
      free(irb);
      return NULL;
   }

   irb->Base.Format = format;
   irb->Base.InternalFormat = irb->Base._BaseFormat;

   /* intel-specific methods */
   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_alloc_window_storage;
   irb->Base.GetPointer = intel_get_pointer;

   return irb;
}


/**
 * Create a new renderbuffer object.
 * Typically called via glBindRenderbufferEXT().
 */
static struct gl_renderbuffer *
intel_new_renderbuffer(GLcontext * ctx, GLuint name)
{
   /*struct intel_context *intel = intel_context(ctx); */
   struct intel_renderbuffer *irb;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, name);
   irb->Base.ClassID = INTEL_RB_CLASS;

   /* intel-specific methods */
   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_alloc_renderbuffer_storage;
   irb->Base.GetPointer = intel_get_pointer;
   /* span routines set in alloc_storage function */

   return &irb->Base;
}


/**
 * Called via glBindFramebufferEXT().
 */
static void
intel_bind_framebuffer(GLcontext * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      intel_draw_buffer(ctx, fb);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}


/**
 * Called via glFramebufferRenderbufferEXT().
 */
static void
intel_framebuffer_renderbuffer(GLcontext * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   DBG("Intel FramebufferRenderbuffer %u %u\n", fb->Name, rb ? rb->Name : 0);

   intelFlush(ctx);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   intel_draw_buffer(ctx, fb);
}


static GLboolean
intel_update_wrapper(GLcontext *ctx, struct intel_renderbuffer *irb, 
		     struct gl_texture_image *texImage)
{
   if (texImage->TexFormat == MESA_FORMAT_ARGB8888) {
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      DBG("Render to RGBA8 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_XRGB8888) {
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      DBG("Render to XGBA8 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_RGB565) {
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      DBG("Render to RGB5 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_ARGB1555) {
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      DBG("Render to ARGB1555 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_ARGB4444) {
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      DBG("Render to ARGB4444 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_Z16) {
      irb->Base.DataType = GL_UNSIGNED_SHORT;
      DBG("Render to DEPTH16 texture OK\n");
   }
   else if (texImage->TexFormat == MESA_FORMAT_S8_Z24) {
      irb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
      DBG("Render to DEPTH_STENCIL texture OK\n");
   }
   else {
      DBG("Render to texture BAD FORMAT %s\n",
	  _mesa_get_format_name(texImage->TexFormat));
      return GL_FALSE;
   }

   irb->Base.Format = texImage->TexFormat;

   irb->Base.InternalFormat = texImage->InternalFormat;
   irb->Base._BaseFormat = _mesa_base_fbo_format(ctx, irb->Base.InternalFormat);
   irb->Base.Width = texImage->Width;
   irb->Base.Height = texImage->Height;

   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_nop_alloc_storage;

   return GL_TRUE;
}


/**
 * When glFramebufferTexture[123]D is called this function sets up the
 * gl_renderbuffer wrapper around the texture image.
 * This will have the region info needed for hardware rendering.
 */
static struct intel_renderbuffer *
intel_wrap_texture(GLcontext * ctx, struct gl_texture_image *texImage)
{
   const GLuint name = ~0;   /* not significant, but distinct for debugging */
   struct intel_renderbuffer *irb;

   /* make an intel_renderbuffer to wrap the texture image */
   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, name);
   irb->Base.ClassID = INTEL_RB_CLASS;

   if (!intel_update_wrapper(ctx, irb, texImage)) {
      free(irb);
      return NULL;
   }

   return irb;
}


/**
 * Called by glFramebufferTexture[123]DEXT() (and other places) to
 * prepare for rendering into texture memory.  This might be called
 * many times to choose different texture levels, cube faces, etc
 * before intel_finish_render_texture() is ever called.
 */
static void
intel_render_texture(GLcontext * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *newImage
      = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);
   struct intel_texture_image *intel_image;
   GLuint dst_x, dst_y;

   (void) fb;

   ASSERT(newImage);

   intel_image = intel_texture_image(newImage);
   if (!intel_image->mt) {
      /* Fallback on drawing to a texture that doesn't have a miptree
       * (has a border, width/height 0, etc.)
       */
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      _mesa_render_texture(ctx, fb, att);
      return;
   }
   else if (!irb) {
      irb = intel_wrap_texture(ctx, newImage);
      if (irb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &irb->Base);
      }
      else {
         /* fallback to software rendering */
         _mesa_render_texture(ctx, fb, att);
         return;
      }
   }

   if (!intel_update_wrapper(ctx, irb, newImage)) {
       _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
       _mesa_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render texture tid %lx tex=%u w=%d h=%d refcount=%d\n",
       _glthread_GetID(),
       att->Texture->Name, newImage->Width, newImage->Height,
       irb->Base.RefCount);

   /* point the renderbufer's region to the texture image region */
   if (irb->region != intel_image->mt->region) {
      if (irb->region)
	 intel_region_release(&irb->region);
      intel_region_reference(&irb->region, intel_image->mt->region);
   }

   /* compute offset of the particular 2D image within the texture region */
   intel_miptree_get_image_offset(intel_image->mt,
				  att->TextureLevel,
				  att->CubeMapFace,
				  att->Zoffset,
				  &dst_x, &dst_y);

   intel_image->mt->region->draw_offset = (dst_y * intel_image->mt->pitch +
					   dst_x) * intel_image->mt->cpp;
   intel_image->mt->region->draw_x = dst_x;
   intel_image->mt->region->draw_y = dst_y;
   intel_image->used_as_render_target = GL_TRUE;

   /* update drawing region, etc */
   intel_draw_buffer(ctx, fb);
}


/**
 * Called by Mesa when rendering to a texture is done.
 */
static void
intel_finish_render_texture(GLcontext * ctx,
                            struct gl_renderbuffer_attachment *att)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_texture_object *tex_obj = att->Texture;
   struct gl_texture_image *image =
      tex_obj->Image[att->CubeMapFace][att->TextureLevel];
   struct intel_texture_image *intel_image = intel_texture_image(image);

   /* Flag that this image may now be validated into the object's miptree. */
   intel_image->used_as_render_target = GL_FALSE;

   /* Since we've (probably) rendered to the texture and will (likely) use
    * it in the texture domain later on in this batchbuffer, flush the
    * batch.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer like GEM does in the kernel.
    */
   intel_batchbuffer_emit_mi_flush(intel->batch);
}

/**
 * Do additional "completeness" testing of a framebuffer object.
 */
static void
intel_validate_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
   const struct intel_renderbuffer *depthRb =
      intel_get_renderbuffer(fb, BUFFER_DEPTH);
   const struct intel_renderbuffer *stencilRb =
      intel_get_renderbuffer(fb, BUFFER_STENCIL);
   int i;

   if (depthRb && stencilRb && stencilRb != depthRb) {
      if (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Type == GL_TEXTURE &&
	  ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Type == GL_TEXTURE &&
	  (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Texture->Name ==
	   ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Texture->Name)) {
	 /* OK */
      } else {
	 /* we only support combined depth/stencil buffers, not separate
	  * stencil buffers.
	  */
	 DBG("Only supports combined depth/stencil (found %s, %s)\n",
	     depthRb ? _mesa_get_format_name(depthRb->Base.Format): "NULL",
	     stencilRb ? _mesa_get_format_name(stencilRb->Base.Format): "NULL");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      }
   }

   for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);

      if (rb == NULL)
	 continue;

      if (irb == NULL) {
	 DBG("software rendering renderbuffer\n");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 continue;
      }

      switch (irb->Base.Format) {
      case MESA_FORMAT_ARGB8888:
      case MESA_FORMAT_XRGB8888:
      case MESA_FORMAT_RGB565:
      case MESA_FORMAT_ARGB1555:
      case MESA_FORMAT_ARGB4444:
	 break;
      default:
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      }
   }
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
   intel->ctx.Driver.BindFramebuffer = intel_bind_framebuffer;
   intel->ctx.Driver.FramebufferRenderbuffer = intel_framebuffer_renderbuffer;
   intel->ctx.Driver.RenderTexture = intel_render_texture;
   intel->ctx.Driver.FinishRenderTexture = intel_finish_render_texture;
   intel->ctx.Driver.ResizeBuffers = intel_resize_buffers;
   intel->ctx.Driver.ValidateFramebuffer = intel_validate_framebuffer;
   intel->ctx.Driver.BlitFramebuffer = _mesa_meta_BlitFramebuffer;

#if FEATURE_OES_EGL_image
   intel->ctx.Driver.EGLImageTargetRenderbufferStorage =
      intel_image_target_renderbuffer_storage;
#endif   
}
