/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_fbo.h"
#include "nouveau_context.h"
#include "nouveau_texture.h"

#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/fbobject.h"

static GLboolean
set_renderbuffer_format(struct gl_renderbuffer *rb, GLenum internalFormat)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	rb->InternalFormat  = internalFormat;

	switch (internalFormat) {
	case GL_RGB:
	case GL_RGB8:
		rb->_BaseFormat  = GL_RGB;
		rb->Format = MESA_FORMAT_B8G8R8X8_UNORM;
		s->cpp = 4;
		break;
	case GL_RGBA:
	case GL_RGBA8:
		rb->_BaseFormat  = GL_RGBA;
		rb->Format = MESA_FORMAT_B8G8R8A8_UNORM;
		s->cpp = 4;
		break;
	case GL_RGB5:
		rb->_BaseFormat  = GL_RGB;
		rb->Format = MESA_FORMAT_B5G6R5_UNORM;
		s->cpp = 2;
		break;
	case GL_DEPTH_COMPONENT16:
		rb->_BaseFormat  = GL_DEPTH_COMPONENT;
		rb->Format = MESA_FORMAT_Z_UNORM16;
		s->cpp = 2;
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT24:
	case GL_STENCIL_INDEX8_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		rb->_BaseFormat  = GL_DEPTH_STENCIL;
		rb->Format = MESA_FORMAT_S8_UINT_Z24_UNORM;
		s->cpp = 4;
		break;
	default:
		return GL_FALSE;
	}

	s->format = rb->Format;

	return GL_TRUE;
}

static GLboolean
nouveau_renderbuffer_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
			     GLenum internalFormat,
			     GLuint width, GLuint height)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	if (!set_renderbuffer_format(rb, internalFormat))
		return GL_FALSE;

	rb->Width = width;
	rb->Height = height;

	nouveau_surface_alloc(ctx, s, TILED, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP,
			      rb->Format, width, height);

	context_dirty(ctx, FRAMEBUFFER);
	return GL_TRUE;
}

static void
nouveau_renderbuffer_del(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	nouveau_surface_ref(NULL, s);
	_mesa_delete_renderbuffer(ctx, rb);
}

static struct gl_renderbuffer *
nouveau_renderbuffer_new(struct gl_context *ctx, GLuint name)
{
	struct gl_renderbuffer *rb;

	rb = (struct gl_renderbuffer *)
		CALLOC_STRUCT(nouveau_renderbuffer);
	if (!rb)
		return NULL;

	_mesa_init_renderbuffer(rb, name);

	rb->AllocStorage = nouveau_renderbuffer_storage;
	rb->Delete = nouveau_renderbuffer_del;

	return rb;
}

static void
nouveau_renderbuffer_map(struct gl_context *ctx,
			 struct gl_renderbuffer *rb,
			 GLuint x, GLuint y, GLuint w, GLuint h,
			 GLbitfield mode,
			 GLubyte **out_map,
			 GLint *out_stride)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;
	GLubyte *map;
	int stride;
	int flags = 0;

	if (mode & GL_MAP_READ_BIT)
		flags |= NOUVEAU_BO_RD;
	if (mode & GL_MAP_WRITE_BIT)
		flags |= NOUVEAU_BO_WR;

	nouveau_bo_map(s->bo, flags, context_client(ctx));

	map = s->bo->map;
	stride = s->pitch;

	if (rb->Name == 0) {
		map += stride * (rb->Height - 1);
		stride = -stride;
	}

	map += x * s->cpp;
	map += (int)y * stride;

	*out_map = map;
	*out_stride = stride;
}

static void
nouveau_renderbuffer_unmap(struct gl_context *ctx,
			   struct gl_renderbuffer *rb)
{
}

static GLboolean
nouveau_renderbuffer_dri_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
				 GLenum internalFormat,
				 GLuint width, GLuint height)
{
	if (!set_renderbuffer_format(rb, internalFormat))
		return GL_FALSE;

	rb->Width = width;
	rb->Height = height;

	return GL_TRUE;
}

struct gl_renderbuffer *
nouveau_renderbuffer_dri_new(GLenum format, __DRIdrawable *drawable)
{
	struct gl_renderbuffer *rb;

	rb = nouveau_renderbuffer_new(NULL, 0);
	if (!rb)
		return NULL;

	rb->AllocStorage = nouveau_renderbuffer_dri_storage;

	if (!set_renderbuffer_format(rb, format)) {
		nouveau_renderbuffer_del(NULL, rb);
		return NULL;
	}

	return rb;
}

static struct gl_framebuffer *
nouveau_framebuffer_new(struct gl_context *ctx, GLuint name)
{
	struct nouveau_framebuffer *nfb;

	nfb = CALLOC_STRUCT(nouveau_framebuffer);
	if (!nfb)
		return NULL;

	_mesa_initialize_user_framebuffer(&nfb->base, name);

	return &nfb->base;
}

struct gl_framebuffer *
nouveau_framebuffer_dri_new(const struct gl_config *visual)
{
	struct nouveau_framebuffer *nfb;

	nfb = CALLOC_STRUCT(nouveau_framebuffer);
	if (!nfb)
		return NULL;

	_mesa_initialize_window_framebuffer(&nfb->base, visual);
	nfb->need_front = !visual->doubleBufferMode;

	return &nfb->base;
}

static void
nouveau_bind_framebuffer(struct gl_context *ctx, GLenum target,
			 struct gl_framebuffer *dfb,
			 struct gl_framebuffer *rfb)
{
	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_framebuffer_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
				 GLenum attachment, struct gl_renderbuffer *rb)
{
	_mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);

	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_render_texture(struct gl_context *ctx, struct gl_framebuffer *fb,
		       struct gl_renderbuffer_attachment *att)
{
	struct gl_renderbuffer *rb = att->Renderbuffer;
	struct gl_texture_image *ti = rb->TexImage;

	/* Update the renderbuffer fields from the texture. */
	nouveau_surface_ref(&to_nouveau_teximage(ti)->surface,
			    &to_nouveau_renderbuffer(rb)->surface);

	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_finish_render_texture(struct gl_context *ctx,
			      struct gl_renderbuffer *rb)
{
	if (rb && rb->TexImage)
		texture_dirty(rb->TexImage->TexObject);
}

static int
validate_format_bpp(mesa_format format)
{
	switch (format) {
	case MESA_FORMAT_B8G8R8X8_UNORM:
	case MESA_FORMAT_B8G8R8A8_UNORM:
	case MESA_FORMAT_S8_UINT_Z24_UNORM:
		return 32;
	case MESA_FORMAT_B5G6R5_UNORM:
	case MESA_FORMAT_Z_UNORM16:
		return 16;
	default:
		return 0;
	}
}

static void
nouveau_check_framebuffer_complete(struct gl_context *ctx,
				   struct gl_framebuffer *fb)
{
	struct gl_renderbuffer_attachment *color =
		&fb->Attachment[BUFFER_COLOR0];
	struct gl_renderbuffer_attachment *depth =
		&fb->Attachment[BUFFER_DEPTH];
	int color_bpp = 0, zeta_bpp;

	if (color->Type == GL_TEXTURE) {
		color_bpp = validate_format_bpp(
				color->Renderbuffer->TexImage->TexFormat);
		if (!color_bpp)
			goto err;
	}

	if (depth->Type == GL_TEXTURE) {
		zeta_bpp = validate_format_bpp(
				depth->Renderbuffer->TexImage->TexFormat);
		if (!zeta_bpp)
			goto err;
		/* NV04/NV05 requires same bpp-ness for color/zeta */
		if (context_chipset(ctx) < 0x10 &&
		    color_bpp && color_bpp != zeta_bpp)
			goto err;
	}

	return;
err:
	fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	return;
}

void
nouveau_fbo_functions_init(struct dd_function_table *functions)
{
	functions->NewFramebuffer = nouveau_framebuffer_new;
	functions->NewRenderbuffer = nouveau_renderbuffer_new;
	functions->MapRenderbuffer = nouveau_renderbuffer_map;
	functions->UnmapRenderbuffer = nouveau_renderbuffer_unmap;
	functions->BindFramebuffer = nouveau_bind_framebuffer;
	functions->FramebufferRenderbuffer = nouveau_framebuffer_renderbuffer;
	functions->RenderTexture = nouveau_render_texture;
	functions->FinishRenderTexture = nouveau_finish_render_texture;
	functions->ValidateFramebuffer = nouveau_check_framebuffer_complete;
}
