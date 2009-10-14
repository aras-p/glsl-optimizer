/**************************************************************************
 * 
 * Copyright 2008 Red Hat Inc.
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
#include "main/texformat.h"
#include "main/texrender.h"
#include "drivers/common/meta.h"

#include "radeon_common.h"
#include "radeon_mipmap_tree.h"

#define FILE_DEBUG_FLAG RADEON_TEXTURE
#define DBG(...) do {                                           \
        if (RADEON_DEBUG & FILE_DEBUG_FLAG)                      \
                _mesa_printf(__VA_ARGS__);                      \
} while(0)

static struct gl_framebuffer *
radeon_new_framebuffer(GLcontext *ctx, GLuint name)
{
  return _mesa_new_framebuffer(ctx, name);
}

static void
radeon_delete_renderbuffer(struct gl_renderbuffer *rb)
{
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);

  ASSERT(rrb);

  if (rrb && rrb->bo) {
    radeon_bo_unref(rrb->bo);
  }
  _mesa_free(rrb);
}

static void *
radeon_get_pointer(GLcontext *ctx, struct gl_renderbuffer *rb,
		   GLint x, GLint y)
{
  return NULL;
}

/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
radeon_alloc_renderbuffer_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
  struct radeon_context *radeon = RADEON_CONTEXT(ctx);
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
  GLboolean software_buffer = GL_FALSE;
  int cpp;

   ASSERT(rb->Name != 0);
  switch (internalFormat) {
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      rb->_ActualFormat = GL_RGB5;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->RedBits = 5;
      rb->GreenBits = 6;
      rb->BlueBits = 5;
      cpp = 2;
      break;
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->_ActualFormat = GL_RGB8;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->RedBits = 8;
      rb->GreenBits = 8;
      rb->BlueBits = 8;
      rb->AlphaBits = 0;
      cpp = 4;
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      rb->_ActualFormat = GL_RGBA8;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->RedBits = 8;
      rb->GreenBits = 8;
      rb->BlueBits = 8;
      rb->AlphaBits = 8;
      cpp = 4;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* alloc a depth+stencil buffer */
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->StencilBits = 8;
      cpp = 4;
      break;
   case GL_DEPTH_COMPONENT16:
      rb->_ActualFormat = GL_DEPTH_COMPONENT16;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->DepthBits = 16;
      cpp = 2;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->_ActualFormat = GL_DEPTH_COMPONENT24;
      rb->DataType = GL_UNSIGNED_INT;
      rb->DepthBits = 24;
      cpp = 4;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->DepthBits = 24;
      rb->StencilBits = 8;
      cpp = 4;
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format in intel_alloc_renderbuffer_storage");
      return GL_FALSE;
   }

  if (ctx->Driver.Flush)
	  ctx->Driver.Flush(ctx); /* +r6/r7 */

  if (rrb->bo)
    radeon_bo_unref(rrb->bo);
  
    
   if (software_buffer) {
      return _mesa_soft_renderbuffer_storage(ctx, rb, internalFormat,
                                             width, height);
   }
   else {
     uint32_t size;
     uint32_t pitch = ((cpp * width + 63) & ~63) / cpp;

     fprintf(stderr,"Allocating %d x %d radeon RBO (pitch %d)\n", width,
	  height, pitch);

     size = pitch * height * cpp;
     rrb->pitch = pitch * cpp;
     rrb->cpp = cpp;
     rrb->bo = radeon_bo_open(radeon->radeonScreen->bom,
			      0,
			      size,
			      0,
			      RADEON_GEM_DOMAIN_VRAM,
			      0);
     rb->Width = width;
     rb->Height = height;
       return GL_TRUE;
   }    
   
}


/**
 * Called for each hardware renderbuffer when a _window_ is resized.
 * Just update fields.
 * Not used for user-created renderbuffers!
 */
static GLboolean
radeon_alloc_window_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->_ActualFormat = internalFormat;

   return GL_TRUE;
}


static void
radeon_resize_buffers(GLcontext *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
     struct radeon_framebuffer *radeon_fb = (struct radeon_framebuffer*)fb;
   int i;

   _mesa_resize_framebuffer(ctx, fb, width, height);

   fb->Initialized = GL_TRUE; /* XXX remove someday */

   if (fb->Name != 0) {
      return;
   }

   /* Make sure all window system renderbuffers are up to date */
   for (i = 0; i < 2; i++) {
      struct gl_renderbuffer *rb = &radeon_fb->color_rb[i]->base;

      /* only resize if size is changing */
      if (rb && (rb->Width != width || rb->Height != height)) {
	 rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height);
      }
   }
}


/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
radeon_nop_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
			 GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "radeon_op_alloc_storage should never be called.");
   return GL_FALSE;
}

struct radeon_renderbuffer *
radeon_create_renderbuffer(GLenum format, __DRIdrawablePrivate *driDrawPriv)
{
    struct radeon_renderbuffer *rrb;

    rrb = CALLOC_STRUCT(radeon_renderbuffer);
    if (!rrb)
	return NULL;

    _mesa_init_renderbuffer(&rrb->base, 0);
    rrb->base.ClassID = RADEON_RB_CLASS;

    /* XXX format junk */
    switch (format) {
	case GL_RGB5:
	    rrb->base._ActualFormat = GL_RGB5;
	    rrb->base._BaseFormat = GL_RGBA;
	    rrb->base.RedBits = 5;
	    rrb->base.GreenBits = 6;
	    rrb->base.BlueBits = 5;
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
	    break;
	case GL_RGB8:
	    rrb->base._ActualFormat = GL_RGB8;
	    rrb->base._BaseFormat = GL_RGB;
	    rrb->base.RedBits = 8;
	    rrb->base.GreenBits = 8;
	    rrb->base.BlueBits = 8;
	    rrb->base.AlphaBits = 0;
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
	    break;
	case GL_RGBA8:
	    rrb->base._ActualFormat = GL_RGBA8;
	    rrb->base._BaseFormat = GL_RGBA;
	    rrb->base.RedBits = 8;
	    rrb->base.GreenBits = 8;
	    rrb->base.BlueBits = 8;
	    rrb->base.AlphaBits = 8;
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
	    break;
	case GL_STENCIL_INDEX8_EXT:
	    rrb->base._ActualFormat = GL_STENCIL_INDEX8_EXT;
	    rrb->base._BaseFormat = GL_STENCIL_INDEX;
	    rrb->base.StencilBits = 8;
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
	    break;
	case GL_DEPTH_COMPONENT16:
	    rrb->base._ActualFormat = GL_DEPTH_COMPONENT16;
	    rrb->base._BaseFormat = GL_DEPTH_COMPONENT;
	    rrb->base.DepthBits = 16;
	    rrb->base.DataType = GL_UNSIGNED_SHORT;
	    break;
	case GL_DEPTH_COMPONENT24:
	    rrb->base._ActualFormat = GL_DEPTH_COMPONENT24;
	    rrb->base._BaseFormat = GL_DEPTH_COMPONENT;
	    rrb->base.DepthBits = 24;
	    rrb->base.DataType = GL_UNSIGNED_INT;
	    break;
	case GL_DEPTH24_STENCIL8_EXT:
	    rrb->base._ActualFormat = GL_DEPTH24_STENCIL8_EXT;
	    rrb->base._BaseFormat = GL_DEPTH_STENCIL_EXT;
	    rrb->base.DepthBits = 24;
	    rrb->base.StencilBits = 8;
	    rrb->base.DataType = GL_UNSIGNED_INT_24_8_EXT;
	    break;
	default:
	    fprintf(stderr, "%s: Unknown format 0x%04x\n", __FUNCTION__, format);
	    _mesa_delete_renderbuffer(&rrb->base);
	    return NULL;
    }

    rrb->dPriv = driDrawPriv;
    rrb->base.InternalFormat = format;

    rrb->base.Delete = radeon_delete_renderbuffer;
    rrb->base.AllocStorage = radeon_alloc_window_storage;
    rrb->base.GetPointer = radeon_get_pointer;

    rrb->bo = NULL;
    return rrb;
}

static struct gl_renderbuffer *
radeon_new_renderbuffer(GLcontext * ctx, GLuint name)
{
  struct radeon_renderbuffer *rrb;

  rrb = CALLOC_STRUCT(radeon_renderbuffer);
  if (!rrb)
    return NULL;

  _mesa_init_renderbuffer(&rrb->base, name);
  rrb->base.ClassID = RADEON_RB_CLASS;

  rrb->base.Delete = radeon_delete_renderbuffer;
  rrb->base.AllocStorage = radeon_alloc_renderbuffer_storage;
  rrb->base.GetPointer = radeon_get_pointer;

  return &rrb->base;
}

static void
radeon_bind_framebuffer(GLcontext * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      radeon_draw_buffer(ctx, fb);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}

static void
radeon_framebuffer_renderbuffer(GLcontext * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{

	if (ctx->Driver.Flush)
		ctx->Driver.Flush(ctx); /* +r6/r7 */

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   radeon_draw_buffer(ctx, fb);
}


static GLboolean
radeon_update_wrapper(GLcontext *ctx, struct radeon_renderbuffer *rrb, 
		     struct gl_texture_image *texImage)
{
	int retry = 0;
restart:
	if (texImage->TexFormat == &_mesa_texformat_argb8888) {
		rrb->cpp = 4;
		rrb->base._ActualFormat = GL_RGBA8;
		rrb->base._BaseFormat = GL_RGBA;
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to RGBA8 texture OK\n");
	}
	else if (texImage->TexFormat == &_mesa_texformat_rgb565) {
		rrb->cpp = 2;
		rrb->base._ActualFormat = GL_RGB5;
		rrb->base._BaseFormat = GL_RGB;
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to RGB5 texture OK\n");
	}
	else if (texImage->TexFormat == &_mesa_texformat_argb1555) {
		rrb->cpp = 2;
		rrb->base._ActualFormat = GL_RGB5_A1;
		rrb->base._BaseFormat = GL_RGBA;
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to ARGB1555 texture OK\n");
	}
	else if (texImage->TexFormat == &_mesa_texformat_argb4444) {
		rrb->cpp = 2;
		rrb->base._ActualFormat = GL_RGBA4;
		rrb->base._BaseFormat = GL_RGBA;
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to ARGB1555 texture OK\n");
	}
	else if (texImage->TexFormat == &_mesa_texformat_z16) {
		rrb->cpp = 2;
		rrb->base._ActualFormat = GL_DEPTH_COMPONENT16;
		rrb->base._BaseFormat = GL_DEPTH_COMPONENT;
		rrb->base.DataType = GL_UNSIGNED_SHORT;
		DBG("Render to DEPTH16 texture OK\n");
	}
	else if (texImage->TexFormat == &_mesa_texformat_s8_z24) {
		rrb->cpp = 4;
		rrb->base._ActualFormat = GL_DEPTH24_STENCIL8_EXT;
		rrb->base._BaseFormat = GL_DEPTH_STENCIL_EXT;
		rrb->base.DataType = GL_UNSIGNED_INT_24_8_EXT;
		DBG("Render to DEPTH_STENCIL texture OK\n");
	}
	else {
		/* try redoing the FBO */
		if (retry == 1) {
			DBG("Render to texture BAD FORMAT %d\n",
			    texImage->TexFormat->MesaFormat);
			return GL_FALSE;
		}
		texImage->TexFormat = radeonChooseTextureFormat(ctx, texImage->InternalFormat, 0,
								texImage->TexFormat->DataType,
								1);

		retry++;
		goto restart;
	}
	
	rrb->base.InternalFormat = rrb->base._ActualFormat;
	rrb->base.Width = texImage->Width;
	rrb->base.Height = texImage->Height;
	rrb->base.RedBits = texImage->TexFormat->RedBits;
	rrb->base.GreenBits = texImage->TexFormat->GreenBits;
	rrb->base.BlueBits = texImage->TexFormat->BlueBits;
	rrb->base.AlphaBits = texImage->TexFormat->AlphaBits;
	rrb->base.DepthBits = texImage->TexFormat->DepthBits;
	rrb->base.StencilBits = texImage->TexFormat->StencilBits;
	
	rrb->base.Delete = radeon_delete_renderbuffer;
	rrb->base.AllocStorage = radeon_nop_alloc_storage;
	
	return GL_TRUE;
}


static struct radeon_renderbuffer *
radeon_wrap_texture(GLcontext * ctx, struct gl_texture_image *texImage)
{
  const GLuint name = ~0;   /* not significant, but distinct for debugging */
  struct radeon_renderbuffer *rrb;

   /* make an radeon_renderbuffer to wrap the texture image */
   rrb = CALLOC_STRUCT(radeon_renderbuffer);
   if (!rrb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture");
      return NULL;
   }

   _mesa_init_renderbuffer(&rrb->base, name);
   rrb->base.ClassID = RADEON_RB_CLASS;

   if (!radeon_update_wrapper(ctx, rrb, texImage)) {
      _mesa_free(rrb);
      return NULL;
   }

   return rrb;
  
}
static void
radeon_render_texture(GLcontext * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *newImage
      = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(att->Renderbuffer);
   radeon_texture_image *radeon_image;
   GLuint imageOffset;

   (void) fb;

   ASSERT(newImage);

   if (newImage->Border != 0) {
      /* Fallback on drawing to a texture with a border, which won't have a
       * miptree.
       */
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      _mesa_render_texture(ctx, fb, att);
      return;
   }
   else if (!rrb) {
      rrb = radeon_wrap_texture(ctx, newImage);
      if (rrb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &rrb->base);
      }
      else {
         /* fallback to software rendering */
         _mesa_render_texture(ctx, fb, att);
         return;
      }
   }

   if (!radeon_update_wrapper(ctx, rrb, newImage)) {
       _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
       _mesa_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render texture tid %x tex=%u w=%d h=%d refcount=%d\n",
       _glthread_GetID(),
       att->Texture->Name, newImage->Width, newImage->Height,
       rrb->base.RefCount);

   /* point the renderbufer's region to the texture image region */
   radeon_image = (radeon_texture_image *)newImage;
   if (rrb->bo != radeon_image->mt->bo) {
      if (rrb->bo)
  	radeon_bo_unref(rrb->bo);
      rrb->bo = radeon_image->mt->bo;
      radeon_bo_ref(rrb->bo);
   }

   /* compute offset of the particular 2D image within the texture region */
   imageOffset = radeon_miptree_image_offset(radeon_image->mt,
                                            att->CubeMapFace,
                                            att->TextureLevel);

   if (att->Texture->Target == GL_TEXTURE_3D) {
      GLuint offsets[6];
      radeon_miptree_depth_offsets(radeon_image->mt, att->TextureLevel,
				   offsets);
      imageOffset += offsets[att->Zoffset];
   }

   /* store that offset in the region, along with the correct pitch for
    * the image we are rendering to */
   rrb->draw_offset = imageOffset;
   rrb->pitch = radeon_image->mt->levels[att->TextureLevel].rowstride;

   /* update drawing region, etc */
   radeon_draw_buffer(ctx, fb);
}

static void
radeon_finish_render_texture(GLcontext * ctx,
                            struct gl_renderbuffer_attachment *att)
{

}
static void
radeon_validate_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
}

void radeon_fbo_init(struct radeon_context *radeon)
{
  radeon->glCtx->Driver.NewFramebuffer = radeon_new_framebuffer;
  radeon->glCtx->Driver.NewRenderbuffer = radeon_new_renderbuffer;
  radeon->glCtx->Driver.BindFramebuffer = radeon_bind_framebuffer;
  radeon->glCtx->Driver.FramebufferRenderbuffer = radeon_framebuffer_renderbuffer;
  radeon->glCtx->Driver.RenderTexture = radeon_render_texture;
  radeon->glCtx->Driver.FinishRenderTexture = radeon_finish_render_texture;
  radeon->glCtx->Driver.ResizeBuffers = radeon_resize_buffers;
  radeon->glCtx->Driver.ValidateFramebuffer = radeon_validate_framebuffer;
  radeon->glCtx->Driver.BlitFramebuffer = _mesa_meta_blit_framebuffer;
}

  
void radeon_renderbuffer_set_bo(struct radeon_renderbuffer *rb,
				struct radeon_bo *bo)
{
  struct radeon_bo *old;
  old = rb->bo;
  rb->bo = bo;
  radeon_bo_ref(bo);
  if (old)
    radeon_bo_unref(old);
}
