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
#include "nouveau_context.h"
#include "nouveau_texture.h"
#include "nouveau_fbo.h"
#include "nouveau_util.h"

#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texformat.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/mipmap.h"
#include "main/texfetch.h"
#include "main/teximage.h"

static struct gl_texture_object *
nouveau_texture_new(GLcontext *ctx, GLuint name, GLenum target)
{
	struct nouveau_texture *nt = CALLOC_STRUCT(nouveau_texture);

	_mesa_initialize_texture_object(&nt->base, name, target);

	return &nt->base;
}

static void
nouveau_texture_free(GLcontext *ctx, struct gl_texture_object *t)
{
	struct nouveau_texture *nt = to_nouveau_texture(t);
	int i;

	for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
		nouveau_surface_ref(NULL, &nt->surfaces[i]);

	_mesa_delete_texture_object(ctx, t);
}

static struct gl_texture_image *
nouveau_teximage_new(GLcontext *ctx)
{
	struct nouveau_teximage *nti = CALLOC_STRUCT(nouveau_teximage);

	return &nti->base;
}

static void
nouveau_teximage_free(GLcontext *ctx, struct gl_texture_image *ti)
{
	struct nouveau_teximage *nti = to_nouveau_teximage(ti);

	nouveau_surface_ref(NULL, &nti->surface);
}

static void
nouveau_teximage_map(GLcontext *ctx, struct gl_texture_image *ti)
{
	struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;
	int ret;

	if (s->bo) {
		ret = nouveau_bo_map(s->bo, NOUVEAU_BO_RDWR);
		assert(!ret);

		ti->Data = s->bo->map;
	}
}

static void
nouveau_teximage_unmap(GLcontext *ctx, struct gl_texture_image *ti)
{
	struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;

	if (s->bo)
		nouveau_bo_unmap(s->bo);
	ti->Data = NULL;
}

static gl_format
nouveau_choose_tex_format(GLcontext *ctx, GLint internalFormat,
			  GLenum srcFormat, GLenum srcType)
{
	switch (internalFormat) {
	case 4:
	case GL_RGBA:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGB10_A2:
		return MESA_FORMAT_ARGB8888;
	case GL_RGB5_A1:
		return MESA_FORMAT_ARGB1555;

	case GL_RGB:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return MESA_FORMAT_XRGB8888;
	case 3:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
		return MESA_FORMAT_RGB565;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_LUMINANCE8_ALPHA8:
		return MESA_FORMAT_ARGB8888;

	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_LUMINANCE8:
		return MESA_FORMAT_L8;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_ALPHA8:
		return MESA_FORMAT_A8;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_INTENSITY8:
		return MESA_FORMAT_I8;

	case GL_COLOR_INDEX:
	case GL_COLOR_INDEX1_EXT:
	case GL_COLOR_INDEX2_EXT:
	case GL_COLOR_INDEX4_EXT:
	case GL_COLOR_INDEX12_EXT:
	case GL_COLOR_INDEX16_EXT:
	case GL_COLOR_INDEX8_EXT:
		return MESA_FORMAT_CI8;

	default:
		assert(0);
	}
}

static GLboolean
teximage_fits(struct gl_texture_object *t, int level,
	      struct gl_texture_image *ti)
{
	struct nouveau_surface *s = &to_nouveau_texture(t)->surfaces[level];

	return t->Target == GL_TEXTURE_RECTANGLE ||
		(s->bo && s->width == ti->Width &&
		 s->height == ti->Height &&
		 s->format == ti->TexFormat);
}

static GLboolean
validate_teximage(GLcontext *ctx, struct gl_texture_object *t,
		  int level, int x, int y, int z,
		  int width, int height, int depth)
{
	struct gl_texture_image *ti = t->Image[0][level];

	if (ti && teximage_fits(t, level, ti)) {
		struct nouveau_surface *ss = to_nouveau_texture(t)->surfaces;
		struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;

		if (t->Target == GL_TEXTURE_RECTANGLE)
			nouveau_surface_ref(s, &ss[level]);
		else
			context_drv(ctx)->surface_copy(ctx, &ss[level], s,
						       x, y, x, y,
						       width, height);

		return GL_TRUE;
	}

	return GL_FALSE;
}

static int
get_last_level(struct gl_texture_object *t)
{
	struct gl_texture_image *base = t->Image[0][t->BaseLevel];

	if (t->MinFilter == GL_NEAREST ||
	    t->MinFilter == GL_LINEAR || !base)
		return t->BaseLevel;
	else
		return MIN2(t->BaseLevel + base->MaxLog2, t->MaxLevel);
}

static void
relayout_texture(GLcontext *ctx, struct gl_texture_object *t)
{
	struct gl_texture_image *base = t->Image[0][t->BaseLevel];

	if (base && t->Target != GL_TEXTURE_RECTANGLE) {
		struct nouveau_surface *ss = to_nouveau_texture(t)->surfaces;
		struct nouveau_surface *s = &to_nouveau_teximage(base)->surface;
		int i, ret, last = get_last_level(t);
		unsigned size, offset = 0,
			width = s->width,
			height = s->height;

		/* Deallocate the old storage. */
		for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
			nouveau_bo_ref(NULL, &ss[i].bo);

		/* Relayout the mipmap tree. */
		for (i = t->BaseLevel; i <= last; i++) {
			size = width * height * s->cpp;

			/* Images larger than 16B have to be aligned. */
			if (size > 16)
				offset = align(offset, 64);

			ss[i] = (struct nouveau_surface) {
				.offset = offset,
				.layout = SWIZZLED,
				.format = s->format,
				.width = width,
				.height = height,
				.cpp = s->cpp,
				.pitch = width * s->cpp,
			};

			offset += size;
			width = MAX2(1, width / 2);
			height = MAX2(1, height / 2);
		}

		/* Get new storage. */
		size = align(offset, 64);

		ret = nouveau_bo_new(context_dev(ctx), NOUVEAU_BO_MAP |
				     NOUVEAU_BO_GART | NOUVEAU_BO_VRAM,
				     0, size, &ss[last].bo);
		assert(!ret);

		for (i = t->BaseLevel; i < last; i++)
			nouveau_bo_ref(ss[last].bo, &ss[i].bo);
	}
}

GLboolean
nouveau_texture_validate(GLcontext *ctx, struct gl_texture_object *t)
{
	struct nouveau_texture *nt = to_nouveau_texture(t);
	int i, last = get_last_level(t);

	if (!nt->surfaces[last].bo)
		return GL_FALSE;

	if (nt->dirty) {
		nt->dirty = GL_FALSE;

		/* Copy the teximages to the actual miptree. */
		for (i = t->BaseLevel; i <= last; i++) {
			struct nouveau_surface *s = &nt->surfaces[i];

			validate_teximage(ctx, t, i, 0, 0, 0,
					  s->width, s->height, 1);
		}
	}

	return GL_TRUE;
}

void
nouveau_texture_reallocate(GLcontext *ctx, struct gl_texture_object *t)
{
	texture_dirty(t);
	relayout_texture(ctx, t);
	nouveau_texture_validate(ctx, t);
}

static unsigned
get_teximage_placement(struct gl_texture_image *ti)
{
	if (ti->TexFormat == MESA_FORMAT_A8 ||
	    ti->TexFormat == MESA_FORMAT_L8 ||
	    ti->TexFormat == MESA_FORMAT_I8)
		/* 1 cpp formats will have to be swizzled by the CPU,
		 * so leave them in system RAM for now. */
		return NOUVEAU_BO_MAP;
	else
		return NOUVEAU_BO_GART | NOUVEAU_BO_MAP;
}

static void
nouveau_teximage(GLcontext *ctx, GLint dims, GLenum target, GLint level,
		 GLint internalFormat,
		 GLint width, GLint height, GLint depth, GLint border,
		 GLenum format, GLenum type, const GLvoid *pixels,
		 const struct gl_pixelstore_attrib *packing,
		 struct gl_texture_object *t,
		 struct gl_texture_image *ti)
{
	struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;
	int ret;

	/* Allocate a new bo for the image. */
	nouveau_surface_alloc(ctx, s, LINEAR, get_teximage_placement(ti),
			      ti->TexFormat, width, height);
	ti->RowStride = s->pitch / s->cpp;

	pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
					     format, type, pixels, packing,
					     "glTexImage");
	if (pixels) {
		/* Store the pixel data. */
		nouveau_teximage_map(ctx, ti);

		ret = _mesa_texstore(ctx, dims, ti->_BaseFormat,
				     ti->TexFormat, ti->Data,
				     0, 0, 0, s->pitch,
				     ti->ImageOffsets,
				     width, height, depth,
				     format, type, pixels, packing);
		assert(ret);

		nouveau_teximage_unmap(ctx, ti);
		_mesa_unmap_teximage_pbo(ctx, packing);

		if (!validate_teximage(ctx, t, level, 0, 0, 0,
				       width, height, depth))
			/* It doesn't fit, mark it as dirty. */
			texture_dirty(t);
	}

	if (level == t->BaseLevel) {
		if (!teximage_fits(t, level, ti))
			relayout_texture(ctx, t);
		nouveau_texture_validate(ctx, t);
	}

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);
}

static void
nouveau_teximage_1d(GLcontext *ctx, GLenum target, GLint level,
		    GLint internalFormat,
		    GLint width, GLint border,
		    GLenum format, GLenum type, const GLvoid *pixels,
		    const struct gl_pixelstore_attrib *packing,
		    struct gl_texture_object *t,
		    struct gl_texture_image *ti)
{
	nouveau_teximage(ctx, 1, target, level, internalFormat,
			 width, 1, 1, border, format, type, pixels,
			 packing, t, ti);
}

static void
nouveau_teximage_2d(GLcontext *ctx, GLenum target, GLint level,
		    GLint internalFormat,
		    GLint width, GLint height, GLint border,
		    GLenum format, GLenum type, const GLvoid *pixels,
		    const struct gl_pixelstore_attrib *packing,
		    struct gl_texture_object *t,
		    struct gl_texture_image *ti)
{
	nouveau_teximage(ctx, 2, target, level, internalFormat,
			 width, height, 1, border, format, type, pixels,
			 packing, t, ti);
}

static void
nouveau_teximage_3d(GLcontext *ctx, GLenum target, GLint level,
		    GLint internalFormat,
		    GLint width, GLint height, GLint depth, GLint border,
		    GLenum format, GLenum type, const GLvoid *pixels,
		    const struct gl_pixelstore_attrib *packing,
		    struct gl_texture_object *t,
		    struct gl_texture_image *ti)
{
	nouveau_teximage(ctx, 3, target, level, internalFormat,
			 width, height, depth, border, format, type, pixels,
			 packing, t, ti);
}

static void
nouveau_texsubimage_3d(GLcontext *ctx, GLenum target, GLint level,
		       GLint xoffset, GLint yoffset, GLint zoffset,
		       GLint width, GLint height, GLint depth,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       struct gl_texture_object *t,
		       struct gl_texture_image *ti)
{
	nouveau_teximage_map(ctx, ti);
	_mesa_store_texsubimage3d(ctx, target, level, xoffset, yoffset, zoffset,
				  width, height, depth, format, type, pixels,
				  packing, t, ti);
	nouveau_teximage_unmap(ctx, ti);

	if (!to_nouveau_texture(t)->dirty)
		validate_teximage(ctx, t, level, xoffset, yoffset, zoffset,
				  width, height, depth);
}

static void
nouveau_texsubimage_2d(GLcontext *ctx, GLenum target, GLint level,
		       GLint xoffset, GLint yoffset,
		       GLint width, GLint height,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       struct gl_texture_object *t,
		       struct gl_texture_image *ti)
{
	nouveau_teximage_map(ctx, ti);
	_mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset,
				  width, height, format, type, pixels,
				  packing, t, ti);
	nouveau_teximage_unmap(ctx, ti);

	if (!to_nouveau_texture(t)->dirty)
		validate_teximage(ctx, t, level, xoffset, yoffset, 0,
				  width, height, 1);
}

static void
nouveau_texsubimage_1d(GLcontext *ctx, GLenum target, GLint level,
		       GLint xoffset, GLint width,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       struct gl_texture_object *t,
		       struct gl_texture_image *ti)
{
	nouveau_teximage_map(ctx, ti);
	_mesa_store_texsubimage1d(ctx, target, level, xoffset,
				  width, format, type, pixels,
				  packing, t, ti);
	nouveau_teximage_unmap(ctx, ti);

	if (!to_nouveau_texture(t)->dirty)
		validate_teximage(ctx, t, level, xoffset, 0, 0,
				  width, 1, 1);
}

static void
nouveau_get_teximage(GLcontext *ctx, GLenum target, GLint level,
		     GLenum format, GLenum type, GLvoid *pixels,
		     struct gl_texture_object *t,
		     struct gl_texture_image *ti)
{
	nouveau_teximage_map(ctx, ti);
	_mesa_get_teximage(ctx, target, level, format, type, pixels,
			   t, ti);
	nouveau_teximage_unmap(ctx, ti);
}

static void
nouveau_bind_texture(GLcontext *ctx, GLenum target,
		     struct gl_texture_object *t)
{
	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);
}

static gl_format
get_texbuffer_format(struct gl_renderbuffer *rb, GLint format)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	if (s->cpp < 4)
		return s->format;
	else if (format == __DRI_TEXTURE_FORMAT_RGBA)
		return MESA_FORMAT_ARGB8888;
	else
		return MESA_FORMAT_XRGB8888;
}

void
nouveau_set_texbuffer(__DRIcontext *dri_ctx,
		      GLint target, GLint format,
		      __DRIdrawable *draw)
{
	struct nouveau_context *nctx = dri_ctx->driverPrivate;
	GLcontext *ctx = &nctx->base;
	struct gl_framebuffer *fb = draw->driverPrivate;
	struct gl_renderbuffer *rb =
		fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
	struct gl_texture_object *t = _mesa_get_current_tex_object(ctx, target);
	struct gl_texture_image *ti;
	struct nouveau_surface *s;

	_mesa_lock_texture(ctx, t);
	ti = _mesa_get_tex_image(ctx, t, target, 0);
	s = &to_nouveau_teximage(ti)->surface;

	/* Update the texture surface with the given drawable. */
	nouveau_update_renderbuffers(dri_ctx, draw);
	nouveau_surface_ref(&to_nouveau_renderbuffer(rb)->surface, s);

	/* Update the image fields. */
	_mesa_init_teximage_fields(ctx, target, ti, s->width, s->height,
				   1, 0, s->cpp);
	ti->RowStride = s->pitch / s->cpp;
	ti->TexFormat = s->format = get_texbuffer_format(rb, format);

	/* Try to validate it. */
	if (!validate_teximage(ctx, t, 0, 0, 0, 0, s->width, s->height, 1))
		nouveau_texture_reallocate(ctx, t);

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);

	_mesa_unlock_texture(ctx, t);
}

static void
nouveau_texture_map(GLcontext *ctx, struct gl_texture_object *t)
{
	int i;

	for (i = t->BaseLevel; i < t->_MaxLevel; i++) {
		if (t->Image[0][i])
			nouveau_teximage_map(ctx, t->Image[0][i]);
	}
}

static void
nouveau_texture_unmap(GLcontext *ctx, struct gl_texture_object *t)
{
	int i;

	for (i = t->BaseLevel; i < t->_MaxLevel; i++) {
		if (t->Image[0][i])
			nouveau_teximage_unmap(ctx, t->Image[0][i]);
	}
}

void
nouveau_texture_functions_init(struct dd_function_table *functions)
{
	functions->NewTextureObject = nouveau_texture_new;
	functions->DeleteTexture = nouveau_texture_free;
	functions->NewTextureImage = nouveau_teximage_new;
	functions->FreeTexImageData = nouveau_teximage_free;
	functions->ChooseTextureFormat = nouveau_choose_tex_format;
	functions->TexImage1D = nouveau_teximage_1d;
	functions->TexImage2D = nouveau_teximage_2d;
	functions->TexImage3D = nouveau_teximage_3d;
	functions->TexSubImage1D = nouveau_texsubimage_1d;
	functions->TexSubImage2D = nouveau_texsubimage_2d;
	functions->TexSubImage3D = nouveau_texsubimage_3d;
	functions->GetTexImage = nouveau_get_teximage;
	functions->BindTexture = nouveau_bind_texture;
	functions->MapTexture = nouveau_texture_map;
	functions->UnmapTexture = nouveau_texture_unmap;
}
