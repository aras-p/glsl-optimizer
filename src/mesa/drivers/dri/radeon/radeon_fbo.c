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
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/texrender.h"
#include "drivers/common/meta.h"

#include "radeon_common.h"
#include "radeon_mipmap_tree.h"

#define FILE_DEBUG_FLAG RADEON_TEXTURE
#define DBG(...) do {                                           \
        if (RADEON_DEBUG & FILE_DEBUG_FLAG)                      \
                printf(__VA_ARGS__);                      \
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

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(rb %p, rrb %p) \n",
		__func__, rb, rrb);

  ASSERT(rrb);

  if (rrb && rrb->bo) {
    radeon_bo_unref(rrb->bo);
  }
  free(rrb);
}

static void *
radeon_get_pointer(GLcontext *ctx, struct gl_renderbuffer *rb,
		   GLint x, GLint y)
{
  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rb %p) \n",
		__func__, ctx, rb);

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

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rb %p) \n",
		__func__, ctx, rb);

   ASSERT(rb->Name != 0);
  switch (internalFormat) {
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      rb->Format = _dri_texformat_rgb565;
      rb->DataType = GL_UNSIGNED_BYTE;
      cpp = 2;
      break;
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->Format = _dri_texformat_argb8888;
      rb->DataType = GL_UNSIGNED_BYTE;
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
      rb->Format = _dri_texformat_argb8888;
      rb->DataType = GL_UNSIGNED_BYTE;
      cpp = 4;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* alloc a depth+stencil buffer */
      rb->Format = MESA_FORMAT_S8_Z24;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      cpp = 4;
      break;
   case GL_DEPTH_COMPONENT16:
      rb->Format = MESA_FORMAT_Z16;
      rb->DataType = GL_UNSIGNED_SHORT;
      cpp = 2;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->Format = MESA_FORMAT_X8_Z24;
      rb->DataType = GL_UNSIGNED_INT;
      cpp = 4;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->Format = MESA_FORMAT_S8_Z24;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      cpp = 4;
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format in radeon_alloc_renderbuffer_storage");
      return GL_FALSE;
   }

  rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

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

     if (RADEON_DEBUG & RADEON_MEMORY)
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
   rb->InternalFormat = internalFormat;
  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rb %p) \n",
		__func__, ctx, rb);


   return GL_TRUE;
}


static void
radeon_resize_buffers(GLcontext *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
     struct radeon_framebuffer *radeon_fb = (struct radeon_framebuffer*)fb;
   int i;

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p) \n",
		__func__, ctx, fb);

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


/**
 * Create a renderbuffer for a window's color, depth and/or stencil buffer.
 * Not used for user-created renderbuffers.
 */
struct radeon_renderbuffer *
radeon_create_renderbuffer(gl_format format, __DRIdrawable *driDrawPriv)
{
    struct radeon_renderbuffer *rrb;

    rrb = CALLOC_STRUCT(radeon_renderbuffer);

    radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s( rrb %p ) \n",
		__func__, rrb);

    if (!rrb)
	return NULL;

    _mesa_init_renderbuffer(&rrb->base, 0);
    rrb->base.ClassID = RADEON_RB_CLASS;

    rrb->base.Format = format;

    switch (format) {
        case MESA_FORMAT_RGB565:
	    assert(_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGB;
	    break;
        case MESA_FORMAT_RGB565_REV:
	    assert(!_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGB;
	    break;
        case MESA_FORMAT_XRGB8888:
	    assert(_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGB;
	    break;
        case MESA_FORMAT_XRGB8888_REV:
	    assert(!_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGB;
	    break;
	case MESA_FORMAT_ARGB8888:
	    assert(_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGBA;
	    break;
	case MESA_FORMAT_ARGB8888_REV:
	    assert(!_mesa_little_endian());
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_RGBA;
	    break;
	case MESA_FORMAT_S8:
	    rrb->base.DataType = GL_UNSIGNED_BYTE;
            rrb->base._BaseFormat = GL_STENCIL_INDEX;
	    break;
	case MESA_FORMAT_Z16:
	    rrb->base.DataType = GL_UNSIGNED_SHORT;
            rrb->base._BaseFormat = GL_DEPTH_COMPONENT;
	    break;
	case MESA_FORMAT_X8_Z24:
	    rrb->base.DataType = GL_UNSIGNED_INT;
            rrb->base._BaseFormat = GL_DEPTH_COMPONENT;
	    break;
	case MESA_FORMAT_S8_Z24:
	    rrb->base.DataType = GL_UNSIGNED_INT_24_8_EXT;
            rrb->base._BaseFormat = GL_DEPTH_STENCIL;
	    break;
	default:
	    fprintf(stderr, "%s: Unknown format %s\n",
                    __FUNCTION__, _mesa_get_format_name(format));
	    _mesa_delete_renderbuffer(&rrb->base);
	    return NULL;
    }

    rrb->dPriv = driDrawPriv;
    rrb->base.InternalFormat = _mesa_get_format_base_format(format);

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

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p) \n",
		__func__, ctx, rrb);

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
  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, target %s) \n",
		__func__, ctx, fb,
		_mesa_lookup_enum_by_nr(target));

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

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, rb %p) \n",
		__func__, ctx, fb, rb);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   radeon_draw_buffer(ctx, fb);
}


/* TODO: According to EXT_fbo spec internal format of texture image
 * once set during glTexImage call, should be preserved when
 * attaching image to renderbuffer. When HW doesn't support
 * rendering to format of attached image, set framebuffer
 * completeness accordingly in radeon_validate_framebuffer (issue #79).
 */
static GLboolean
radeon_update_wrapper(GLcontext *ctx, struct radeon_renderbuffer *rrb, 
		     struct gl_texture_image *texImage)
{
	int retry = 0;
	gl_format texFormat;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p, texImage %p) \n",
		__func__, ctx, rrb, texImage);

restart:
	if (texImage->TexFormat == _dri_texformat_argb8888) {
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to RGBA8 texture OK\n");
	}
	else if (texImage->TexFormat == _dri_texformat_rgb565) {
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to RGB5 texture OK\n");
	}
	else if (texImage->TexFormat == _dri_texformat_argb1555) {
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to ARGB1555 texture OK\n");
	}
	else if (texImage->TexFormat == _dri_texformat_argb4444) {
		rrb->base.DataType = GL_UNSIGNED_BYTE;
		DBG("Render to ARGB4444 texture OK\n");
	}
	else if (texImage->TexFormat == MESA_FORMAT_Z16) {
		rrb->base.DataType = GL_UNSIGNED_SHORT;
		DBG("Render to DEPTH16 texture OK\n");
	}
	else if (texImage->TexFormat == MESA_FORMAT_S8_Z24) {
		rrb->base.DataType = GL_UNSIGNED_INT_24_8_EXT;
		DBG("Render to DEPTH_STENCIL texture OK\n");
	}
	else {
		/* try redoing the FBO */
		if (retry == 1) {
			DBG("Render to texture BAD FORMAT %d\n",
			    texImage->TexFormat);
			return GL_FALSE;
		}
                /* XXX why is the tex format being set here?
                 * I think this can be removed.
                 */
		texImage->TexFormat = radeonChooseTextureFormat(ctx, texImage->InternalFormat, 0,
								_mesa_get_format_datatype(texImage->TexFormat),
								1);

		retry++;
		goto restart;
	}
	
	texFormat = texImage->TexFormat;

	rrb->base.Format = texFormat;

        rrb->cpp = _mesa_get_format_bytes(texFormat);
	rrb->pitch = texImage->Width * rrb->cpp;
	rrb->base.InternalFormat = texImage->InternalFormat;
        rrb->base._BaseFormat = _mesa_base_fbo_format(ctx, rrb->base.InternalFormat);

	rrb->base.Width = texImage->Width;
	rrb->base.Height = texImage->Height;
	
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

   radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p, texImage %p) \n",
		__func__, ctx, rrb, texImage);

   if (!rrb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture");
      return NULL;
   }

   _mesa_init_renderbuffer(&rrb->base, name);
   rrb->base.ClassID = RADEON_RB_CLASS;

   if (!radeon_update_wrapper(ctx, rrb, texImage)) {
      free(rrb);
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

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, rrb %p, att %p)\n",
		__func__, ctx, fb, rrb, att);

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

   DBG("Begin render texture tid %lx tex=%u w=%d h=%d refcount=%d\n",
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
      imageOffset += radeon_image->mt->levels[att->TextureLevel].rowstride *
                     radeon_image->mt->levels[att->TextureLevel].height *
                     att->Zoffset;
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
  radeon->glCtx->Driver.BlitFramebuffer = _mesa_meta_BlitFramebuffer;
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
