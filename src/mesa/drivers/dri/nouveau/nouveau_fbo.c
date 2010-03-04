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
		rb->Format = MESA_FORMAT_XRGB8888;
		rb->DataType = GL_UNSIGNED_BYTE;
		s->cpp = 4;
		break;
	case GL_RGBA:
	case GL_RGBA8:
		rb->_BaseFormat  = GL_RGBA;
		rb->Format = MESA_FORMAT_ARGB8888;
		rb->DataType = GL_UNSIGNED_BYTE;
		s->cpp = 4;
		break;
	case GL_RGB5:
		rb->_BaseFormat  = GL_RGB;
		rb->Format = MESA_FORMAT_RGB565;
		rb->DataType = GL_UNSIGNED_BYTE;
		s->cpp = 2;
		break;
	case GL_DEPTH_COMPONENT16:
		rb->_BaseFormat  = GL_DEPTH_COMPONENT;
		rb->Format = MESA_FORMAT_Z16;
		rb->DataType = GL_UNSIGNED_SHORT;
		s->cpp = 2;
		break;
	case GL_DEPTH_COMPONENT24:
	case GL_STENCIL_INDEX8_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		rb->_BaseFormat  = GL_DEPTH_STENCIL;
		rb->Format = MESA_FORMAT_Z24_S8;
		rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
		s->cpp = 4;
		break;
	default:
		return GL_FALSE;
	}

	s->format = rb->Format;

	return GL_TRUE;
}

static GLboolean
nouveau_renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
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
nouveau_renderbuffer_del(struct gl_renderbuffer *rb)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	nouveau_surface_ref(NULL, s);
	FREE(rb);
}

static struct gl_renderbuffer *
nouveau_renderbuffer_new(GLcontext *ctx, GLuint name)
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

static GLboolean
nouveau_renderbuffer_dri_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
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
		nouveau_renderbuffer_del(rb);
		return NULL;
	}

	return rb;
}

static struct gl_framebuffer *
nouveau_framebuffer_new(GLcontext *ctx, GLuint name)
{
	struct nouveau_framebuffer *nfb;

	nfb = CALLOC_STRUCT(nouveau_framebuffer);
	if (!nfb)
		return NULL;

	_mesa_initialize_user_framebuffer(&nfb->base, name);

	return &nfb->base;
}

struct gl_framebuffer *
nouveau_framebuffer_dri_new(const GLvisual *visual)
{
	struct nouveau_framebuffer *nfb;

	nfb = CALLOC_STRUCT(nouveau_framebuffer);
	if (!nfb)
		return NULL;

	_mesa_initialize_window_framebuffer(&nfb->base, visual);

	return &nfb->base;
}

static void
nouveau_bind_framebuffer(GLcontext *ctx, GLenum target,
			 struct gl_framebuffer *dfb,
			 struct gl_framebuffer *rfb)
{
	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_framebuffer_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
				 GLenum attachment, struct gl_renderbuffer *rb)
{
	_mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);

	context_dirty(ctx, FRAMEBUFFER);
}

static GLenum
get_tex_format(struct gl_texture_image *ti)
{
	switch (ti->TexFormat) {
	case MESA_FORMAT_ARGB8888:
		return GL_RGBA8;
	case MESA_FORMAT_XRGB8888:
		return GL_RGB8;
	case MESA_FORMAT_RGB565:
		return GL_RGB5;
	default:
		assert(0);
	}
}

static void
nouveau_render_texture(GLcontext *ctx, struct gl_framebuffer *fb,
		       struct gl_renderbuffer_attachment *att)
{
	struct gl_renderbuffer *rb = att->Renderbuffer;
	struct gl_texture_image *ti =
		att->Texture->Image[att->CubeMapFace][att->TextureLevel];
	int ret;

	/* Allocate a renderbuffer object for the texture if we
	 * haven't already done so. */
	if (!rb) {
		rb = nouveau_renderbuffer_new(ctx, 0);
		assert(rb);

		rb->AllocStorage = NULL;
		_mesa_reference_renderbuffer(&att->Renderbuffer, rb);
	}

	/* Update the renderbuffer fields from the texture. */
	ret = set_renderbuffer_format(rb, get_tex_format(ti));
	assert(ret);

	rb->Width = ti->Width;
	rb->Height = ti->Height;
	nouveau_surface_ref(&to_nouveau_teximage(ti)->surface,
			    &to_nouveau_renderbuffer(rb)->surface);

	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_finish_render_texture(GLcontext *ctx,
			      struct gl_renderbuffer_attachment *att)
{
	struct nouveau_renderbuffer *nrb
		= to_nouveau_renderbuffer(att->Renderbuffer);

	texture_dirty(att->Texture);
	nouveau_surface_ref(NULL, &nrb->surface);
}

void
nouveau_fbo_functions_init(struct dd_function_table *functions)
{
	functions->NewFramebuffer = nouveau_framebuffer_new;
	functions->NewRenderbuffer = nouveau_renderbuffer_new;
	functions->BindFramebuffer = nouveau_bind_framebuffer;
	functions->FramebufferRenderbuffer = nouveau_framebuffer_renderbuffer;
	functions->RenderTexture = nouveau_render_texture;
	functions->FinishRenderTexture = nouveau_finish_render_texture;
}
