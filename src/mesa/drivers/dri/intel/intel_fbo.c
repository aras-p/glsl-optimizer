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
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/teximage.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tex.h"
#include "intel_span.h"
#ifndef I915
#include "brw_context.h"
#endif

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
intel_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   ASSERT(irb);

   intel_region_release(&irb->region);
   intel_region_release(&irb->hiz_region);

   _mesa_reference_renderbuffer(&irb->wrapped_depth, NULL);
   _mesa_reference_renderbuffer(&irb->wrapped_stencil, NULL);

   free(irb);
}


/**
 * Return a pointer to a specific pixel in a renderbuffer.
 */
static void *
intel_get_pointer(struct gl_context * ctx, struct gl_renderbuffer *rb,
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
GLboolean
intel_alloc_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   int cpp, tiling;

   ASSERT(rb->Name != 0);

   switch (internalFormat) {
   default:
      /* Use the same format-choice logic as for textures.
       * Renderbuffers aren't any different from textures for us,
       * except they're less useful because you can't texture with
       * them.
       */
      rb->Format = intel->ctx.Driver.ChooseTextureFormat(ctx, internalFormat,
							 GL_NONE, GL_NONE);
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* These aren't actual texture formats, so force them here. */
      if (intel->has_separate_stencil) {
	 rb->Format = MESA_FORMAT_S8;
      } else {
	 assert(!intel->must_use_separate_stencil);
	 rb->Format = MESA_FORMAT_S8_Z24;
      }
      break;
   }

   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);
   rb->DataType = intel_mesa_format_to_rb_datatype(rb->Format);
   cpp = _mesa_get_format_bytes(rb->Format);

   intel_flush(ctx);

   /* free old region */
   if (irb->region) {
      intel_region_release(&irb->region);
   }
   if (irb->hiz_region) {
      intel_region_release(&irb->hiz_region);
   }

   /* allocate new memory region/renderbuffer */

   /* alloc hardware renderbuffer */
   DBG("Allocating %d x %d Intel RBO\n", width, height);

   tiling = I915_TILING_NONE;
   if (intel->use_texture_tiling) {
      GLenum base_format = _mesa_get_format_base_format(rb->Format);

      if (intel->gen >= 4 && (base_format == GL_DEPTH_COMPONENT ||
			      base_format == GL_STENCIL_INDEX ||
			      base_format == GL_DEPTH_STENCIL))
	 tiling = I915_TILING_Y;
      else
	 tiling = I915_TILING_X;
   }

   if (irb->Base.Format == MESA_FORMAT_S8) {
      /*
       * The stencil buffer is W tiled. However, we request from the kernel a
       * non-tiled buffer because the GTT is incapable of W fencing.
       *
       * The stencil buffer has quirky pitch requirements.  From Vol 2a,
       * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
       *    The pitch must be set to 2x the value computed based on width, as
       *    the stencil buffer is stored with two rows interleaved.
       * To accomplish this, we resort to the nasty hack of doubling the drm
       * region's cpp and halving its height.
       *
       * If we neglect to double the pitch, then render corruption occurs.
       */
      irb->region = intel_region_alloc(intel->intelScreen,
				       I915_TILING_NONE,
				       cpp * 2,
				       ALIGN(width, 64),
				       ALIGN((height + 1) / 2, 64),
				       GL_TRUE);
      if (!irb->region)
	return false;

   } else if (irb->Base.Format == MESA_FORMAT_S8_Z24
	      && intel->must_use_separate_stencil) {

      bool ok = true;
      struct gl_renderbuffer *depth_rb;
      struct gl_renderbuffer *stencil_rb;

      depth_rb = intel_create_wrapped_renderbuffer(ctx, width, height,
						   MESA_FORMAT_X8_Z24);
      stencil_rb = intel_create_wrapped_renderbuffer(ctx, width, height,
						     MESA_FORMAT_S8);
      ok = depth_rb && stencil_rb;
      ok = ok && intel_alloc_renderbuffer_storage(ctx, depth_rb,
						  depth_rb->InternalFormat,
						  width, height);
      ok = ok && intel_alloc_renderbuffer_storage(ctx, stencil_rb,
						  stencil_rb->InternalFormat,
						  width, height);

      if (!ok) {
	 if (depth_rb) {
	    intel_delete_renderbuffer(depth_rb);
	 }
	 if (stencil_rb) {
	    intel_delete_renderbuffer(stencil_rb);
	 }
	 return false;
      }

      depth_rb->Wrapped = rb;
      stencil_rb->Wrapped = rb;
      _mesa_reference_renderbuffer(&irb->wrapped_depth, depth_rb);
      _mesa_reference_renderbuffer(&irb->wrapped_stencil, stencil_rb);

   } else {
      irb->region = intel_region_alloc(intel->intelScreen, tiling, cpp,
				       width, height, GL_TRUE);
      if (!irb->region)
	 return false;

      if (intel->vtbl.is_hiz_depth_format(intel, rb->Format)) {
	 irb->hiz_region = intel_region_alloc(intel->intelScreen,
					      I915_TILING_Y,
					      irb->region->cpp,
					      irb->region->width,
					      irb->region->height,
					      GL_TRUE);
	 if (!irb->hiz_region) {
	    intel_region_release(&irb->region);
	    return false;
	 }
      }
   }

   return GL_TRUE;
}


#if FEATURE_OES_EGL_image
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
   case MESA_FORMAT_RGBA8888_REV:
      _mesa_error(&intel->ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(unsupported image format");
      return;
      break;
   default:
      break;
   }

   irb = intel_renderbuffer(rb);
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
intel_alloc_window_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return GL_TRUE;
}


static void
intel_resize_buffers(struct gl_context *ctx, struct gl_framebuffer *fb,
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
intel_nop_alloc_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                        GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "intel_op_alloc_storage should never be called.");
   return GL_FALSE;
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
   irb->Base._BaseFormat = _mesa_get_format_base_format(format);
   irb->Base.Format = format;
   irb->Base.InternalFormat = irb->Base._BaseFormat;
   irb->Base.DataType = intel_mesa_format_to_rb_datatype(format);

   /* intel-specific methods */
   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_alloc_window_storage;
   irb->Base.GetPointer = intel_get_pointer;

   return irb;
}


struct gl_renderbuffer*
intel_create_wrapped_renderbuffer(struct gl_context * ctx,
				  int width, int height,
				  gl_format format)
{
   /*
    * The name here is irrelevant, as long as its nonzero, because the
    * renderbuffer never gets entered into Mesa's renderbuffer hash table.
    */
   GLuint name = ~0;

   struct intel_renderbuffer *irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   struct gl_renderbuffer *rb = &irb->Base;
   _mesa_init_renderbuffer(rb, name);
   rb->ClassID = INTEL_RB_CLASS;
   rb->_BaseFormat = _mesa_get_format_base_format(format);
   rb->Format = format;
   rb->InternalFormat = rb->_BaseFormat;
   rb->DataType = intel_mesa_format_to_rb_datatype(format);
   rb->Width = width;
   rb->Height = height;

   return rb;
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

   intel_flush(ctx);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   intel_draw_buffer(ctx);
}

static bool
intel_update_tex_wrapper_regions(struct intel_context *intel,
				 struct intel_renderbuffer *irb,
				 struct intel_texture_image *intel_image);

static GLboolean
intel_update_wrapper(struct gl_context *ctx, struct intel_renderbuffer *irb, 
		     struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(texImage);

   if (!intel_span_supports_format(texImage->TexFormat)) {
      DBG("Render to texture BAD FORMAT %s\n",
	  _mesa_get_format_name(texImage->TexFormat));
      return GL_FALSE;
   } else {
      DBG("Render to texture %s\n", _mesa_get_format_name(texImage->TexFormat));
   }

   irb->Base.Format = texImage->TexFormat;
   irb->Base.DataType = intel_mesa_format_to_rb_datatype(texImage->TexFormat);
   irb->Base.InternalFormat = texImage->InternalFormat;
   irb->Base._BaseFormat = _mesa_base_tex_format(ctx, irb->Base.InternalFormat);
   irb->Base.Width = texImage->Width;
   irb->Base.Height = texImage->Height;

   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_nop_alloc_storage;

   if (intel_image->stencil_rb) {
      /*  The tex image has packed depth/stencil format, but is using separate
       * stencil. */

      bool ok;
      struct intel_renderbuffer *depth_irb =
	 intel_renderbuffer(intel_image->depth_rb);

      /* Update the hiz region if necessary. */
      ok =  intel_update_tex_wrapper_regions(intel, depth_irb, intel_image);
      if (!ok) {
	 return false;
      }

      /* The tex image shares its embedded depth and stencil renderbuffers with
       * the renderbuffer wrapper. */
      if (irb->wrapped_depth != intel_image->depth_rb) {
	 _mesa_reference_renderbuffer(&irb->wrapped_depth,
				      intel_image->depth_rb);
      }
      if (irb->wrapped_stencil != intel_image->stencil_rb) {
	 _mesa_reference_renderbuffer(&irb->wrapped_stencil,
				      intel_image->stencil_rb);
      }

      return true;

   } else {
      return intel_update_tex_wrapper_regions(intel, irb, intel_image);
   }
}

/**
 * FIXME: The handling of the hiz region is broken for mipmapped depth textures
 * FIXME: because intel_finalize_mipmap_tree is unaware of it.
 */
static bool
intel_update_tex_wrapper_regions(struct intel_context *intel,
				 struct intel_renderbuffer *irb,
				 struct intel_texture_image *intel_image)
{
   struct gl_renderbuffer *rb = &irb->Base;

   /* Point the renderbuffer's region to the texture's region. */
   if (irb->region != intel_image->mt->region) {
      intel_region_reference(&irb->region, intel_image->mt->region);
   }

   /* Allocate the texture's hiz region if necessary. */
   if (intel->vtbl.is_hiz_depth_format(intel, rb->Format)
       && !intel_image->mt->hiz_region) {
      intel_image->mt->hiz_region =
         intel_region_alloc(intel->intelScreen,
                            I915_TILING_Y,
                            _mesa_get_format_bytes(rb->Format),
                            rb->Width,
                            rb->Height,
                            GL_TRUE);
      if (!intel_image->mt->hiz_region)
         return GL_FALSE;
   }

   /* Point the renderbuffer's hiz region to the texture's hiz region. */
   if (irb->hiz_region != intel_image->mt->hiz_region) {
      intel_region_reference(&irb->hiz_region, intel_image->mt->hiz_region);
   }

   return GL_TRUE;
}


/**
 * When glFramebufferTexture[123]D is called this function sets up the
 * gl_renderbuffer wrapper around the texture image.
 * This will have the region info needed for hardware rendering.
 */
static struct intel_renderbuffer *
intel_wrap_texture(struct gl_context * ctx, struct gl_texture_image *texImage)
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

void
intel_renderbuffer_set_draw_offset(struct intel_renderbuffer *irb,
				   struct intel_texture_image *intel_image,
				   int zoffset)
{
   unsigned int dst_x, dst_y;

   /* compute offset of the particular 2D image within the texture region */
   intel_miptree_get_image_offset(intel_image->mt,
				  intel_image->base.Base.Level,
				  intel_image->base.Base.Face,
				  zoffset,
				  &dst_x, &dst_y);

   irb->draw_x = dst_x;
   irb->draw_y = dst_y;
}

/**
 * Rendering to tiled buffers requires that the base address of the
 * buffer be aligned to a page boundary.  We generally render to
 * textures by pointing the surface at the mipmap image level, which
 * may not be aligned to a tile boundary.
 *
 * This function returns an appropriately-aligned base offset
 * according to the tiling restrictions, plus any required x/y offset
 * from there.
 */
uint32_t
intel_renderbuffer_tile_offsets(struct intel_renderbuffer *irb,
				uint32_t *tile_x,
				uint32_t *tile_y)
{
   int cpp = irb->region->cpp;
   uint32_t pitch = irb->region->pitch * cpp;

   if (irb->region->tiling == I915_TILING_NONE) {
      *tile_x = 0;
      *tile_y = 0;
      return irb->draw_x * cpp + irb->draw_y * pitch;
   } else if (irb->region->tiling == I915_TILING_X) {
      *tile_x = irb->draw_x % (512 / cpp);
      *tile_y = irb->draw_y % 8;
      return ((irb->draw_y / 8) * (8 * pitch) +
	      (irb->draw_x - *tile_x) / (512 / cpp) * 4096);
   } else {
      assert(irb->region->tiling == I915_TILING_Y);
      *tile_x = irb->draw_x % (128 / cpp);
      *tile_y = irb->draw_y % 32;
      return ((irb->draw_y / 32) * (32 * pitch) +
	      (irb->draw_x - *tile_x) / (128 / cpp) * 4096);
   }
}

#ifndef I915
static bool
need_tile_offset_workaround(struct brw_context *brw,
			    struct intel_renderbuffer *irb)
{
   uint32_t tile_x, tile_y;

   if (brw->has_surface_tile_offset)
      return false;

   intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y);

   return tile_x != 0 || tile_y != 0;
}
#endif

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
   struct gl_texture_image *image = _mesa_get_attachment_teximage(att);
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);
   struct intel_texture_image *intel_image = intel_texture_image(image);

   (void) fb;

   if (!intel_image->mt) {
      /* Fallback on drawing to a texture that doesn't have a miptree
       * (has a border, width/height 0, etc.)
       */
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      _swrast_render_texture(ctx, fb, att);
      return;
   }
   else if (!irb) {
      irb = intel_wrap_texture(ctx, image);
      if (irb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &irb->Base);
      }
      else {
         /* fallback to software rendering */
         _swrast_render_texture(ctx, fb, att);
         return;
      }
   }

   if (!intel_update_wrapper(ctx, irb, image)) {
       _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render texture tid %lx tex=%u w=%d h=%d refcount=%d\n",
       _glthread_GetID(),
       att->Texture->Name, image->Width, image->Height,
       irb->Base.RefCount);

   intel_renderbuffer_set_draw_offset(irb, intel_image, att->Zoffset);
   intel_image->used_as_render_target = GL_TRUE;

#ifndef I915
   if (need_tile_offset_workaround(brw_context(ctx), irb)) {
      /* Original gen4 hardware couldn't draw to a non-tile-aligned
       * destination in a miptree unless you actually setup your
       * renderbuffer as a miptree and used the fragile
       * lod/array_index/etc. controls to select the image.  So,
       * instead, we just make a new single-level miptree and render
       * into that.
       */
      struct intel_context *intel = intel_context(ctx);
      struct intel_mipmap_tree *old_mt = intel_image->mt;
      struct intel_mipmap_tree *new_mt;

      new_mt = intel_miptree_create(intel, image->TexObject->Target,
				    intel_image->base.Base.TexFormat,
				    intel_image->base.Base.Level,
				    intel_image->base.Base.Level,
				    intel_image->base.Base.Width,
				    intel_image->base.Base.Height,
				    intel_image->base.Base.Depth,
				    GL_TRUE);

      intel_miptree_image_copy(intel,
                               new_mt,
			       intel_image->base.Base.Face,
			       intel_image->base.Base.Level,
			       old_mt);

      intel_miptree_release(intel, &intel_image->mt);
      intel_image->mt = new_mt;
      intel_renderbuffer_set_draw_offset(irb, intel_image, att->Zoffset);

      intel_region_reference(&irb->region, intel_image->mt->region);
   }
#endif
   /* update drawing region, etc */
   intel_draw_buffer(ctx);
}


/**
 * Called by Mesa when rendering to a texture is done.
 */
static void
intel_finish_render_texture(struct gl_context * ctx,
                            struct gl_renderbuffer_attachment *att)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_texture_object *tex_obj = att->Texture;
   struct gl_texture_image *image =
      tex_obj->Image[att->CubeMapFace][att->TextureLevel];
   struct intel_texture_image *intel_image = intel_texture_image(image);

   DBG("Finish render texture tid %lx tex=%u\n",
       _glthread_GetID(), att->Texture->Name);

   /* Flag that this image may now be validated into the object's miptree. */
   if (intel_image)
      intel_image->used_as_render_target = GL_FALSE;

   /* Since we've (probably) rendered to the texture and will (likely) use
    * it in the texture domain later on in this batchbuffer, flush the
    * batch.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer like GEM does in the kernel.
    */
   intel_batchbuffer_emit_mi_flush(intel);
}

/**
 * Do additional "completeness" testing of a framebuffer object.
 */
static void
intel_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct intel_context *intel = intel_context(ctx);
   const struct intel_renderbuffer *depthRb =
      intel_get_renderbuffer(fb, BUFFER_DEPTH);
   const struct intel_renderbuffer *stencilRb =
      intel_get_renderbuffer(fb, BUFFER_STENCIL);
   int i;

   /*
    * The depth and stencil renderbuffers are the same renderbuffer or wrap
    * the same texture.
    */
   if (depthRb && stencilRb) {
      bool depth_stencil_are_same;
      if (depthRb == stencilRb)
	 depth_stencil_are_same = true;
      else if ((fb->Attachment[BUFFER_DEPTH].Type == GL_TEXTURE) &&
	       (fb->Attachment[BUFFER_STENCIL].Type == GL_TEXTURE) &&
	       (fb->Attachment[BUFFER_DEPTH].Texture->Name ==
		fb->Attachment[BUFFER_STENCIL].Texture->Name))
	 depth_stencil_are_same = true;
      else
	 depth_stencil_are_same = false;

      if (!intel->has_separate_stencil && !depth_stencil_are_same) {
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
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
	 DBG("attachment without renderbuffer\n");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 continue;
      }

      irb = intel_renderbuffer(rb);
      if (irb == NULL) {
	 DBG("software rendering renderbuffer\n");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 continue;
      }

      if (!intel_span_supports_format(irb->Base.Format) ||
	  !intel->vtbl.render_target_supported(irb->Base.Format)) {
	 DBG("Unsupported texture/renderbuffer format attached: %s\n",
	     _mesa_get_format_name(irb->Base.Format));
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
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
intel_blit_framebuffer_copy_tex_sub_image(struct gl_context *ctx,
                                          GLint srcX0, GLint srcY0,
                                          GLint srcX1, GLint srcY1,
                                          GLint dstX0, GLint dstY0,
                                          GLint dstX1, GLint dstY1,
                                          GLbitfield mask, GLenum filter)
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *drawFb = ctx->DrawBuffer;
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer_attachment *drawAtt =
         &drawFb->Attachment[drawFb->_ColorDrawBufferIndexes[0]];

      /* If the source and destination are the same size with no
         mirroring, the rectangles are within the size of the
         texture and there is no scissor then we can use
         glCopyTexSubimage2D to implement the blit. This will end
         up as a fast hardware blit on some drivers */
      if (drawAtt && drawAtt->Texture &&
          srcX0 - srcX1 == dstX0 - dstX1 &&
          srcY0 - srcY1 == dstY0 - dstY1 &&
          srcX1 >= srcX0 &&
          srcY1 >= srcY0 &&
          srcX0 >= 0 && srcX1 <= readFb->Width &&
          srcY0 >= 0 && srcY1 <= readFb->Height &&
          dstX0 >= 0 && dstX1 <= drawFb->Width &&
          dstY0 >= 0 && dstY1 <= drawFb->Height &&
          !ctx->Scissor.Enabled) {
         const struct gl_texture_object *texObj = drawAtt->Texture;
         const GLuint dstLevel = drawAtt->TextureLevel;
         const GLenum target = texObj->Target;

         struct gl_texture_image *texImage =
            _mesa_select_tex_image(ctx, texObj, target, dstLevel);

         if (intel_copy_texsubimage(intel_context(ctx),
                                    intel_texture_image(texImage),
                                    dstX0, dstY0,
                                    srcX0, srcY0,
                                    srcX1 - srcX0, /* width */
                                    srcY1 - srcY0))
            mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   return mask;
}

static void
intel_blit_framebuffer(struct gl_context *ctx,
                       GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
   /* Try faster, glCopyTexSubImage2D approach first which uses the BLT. */
   mask = intel_blit_framebuffer_copy_tex_sub_image(ctx,
                                                    srcX0, srcY0, srcX1, srcY1,
                                                    dstX0, dstY0, dstX1, dstY1,
                                                    mask, filter);
   if (mask == 0x0)
      return;

   _mesa_meta_BlitFramebuffer(ctx,
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
   intel->ctx.Driver.BindFramebuffer = intel_bind_framebuffer;
   intel->ctx.Driver.FramebufferRenderbuffer = intel_framebuffer_renderbuffer;
   intel->ctx.Driver.RenderTexture = intel_render_texture;
   intel->ctx.Driver.FinishRenderTexture = intel_finish_render_texture;
   intel->ctx.Driver.ResizeBuffers = intel_resize_buffers;
   intel->ctx.Driver.ValidateFramebuffer = intel_validate_framebuffer;
   intel->ctx.Driver.BlitFramebuffer = intel_blit_framebuffer;

#if FEATURE_OES_EGL_image
   intel->ctx.Driver.EGLImageTargetRenderbufferStorage =
      intel_image_target_renderbuffer_storage;
#endif   
}
