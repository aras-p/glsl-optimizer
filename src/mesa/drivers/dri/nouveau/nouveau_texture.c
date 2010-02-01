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
#include "nouveau_util.h"

#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texformat.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/mipmap.h"
#include "main/texfetch.h"

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

	ret = nouveau_bo_map(s->bo, NOUVEAU_BO_RDWR);
	assert(!ret);

	ti->Data = s->bo->map;
}

static void
nouveau_teximage_unmap(GLcontext *ctx, struct gl_texture_image *ti)
{
	struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;

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
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA8:
	case GL_RGB:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return MESA_FORMAT_ARGB8888;
	case GL_RGB5_A1:
		return MESA_FORMAT_ARGB1555;
	case GL_RGBA2:
	case GL_RGBA4:
		return MESA_FORMAT_ARGB4444;

	case 3:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
		return MESA_FORMAT_RGB565;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_ALPHA8:
		return MESA_FORMAT_A8;

	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_LUMINANCE8:
		return MESA_FORMAT_L8;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_LUMINANCE8_ALPHA8:
		return MESA_FORMAT_ARGB8888;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_INTENSITY8:
		return MESA_FORMAT_ARGB8888;

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
	unsigned bo_flags = NOUVEAU_BO_GART | NOUVEAU_BO_RDWR | NOUVEAU_BO_MAP;
	int ret;

	/* Allocate a new bo for the image. */
	nouveau_surface_alloc(ctx, s, LINEAR, bo_flags, ti->TexFormat,
			      width, height);
	ti->RowStride = s->pitch / s->cpp;

	pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
					     format, type, pixels, packing,
					     "glTexImage");
	if (!pixels)
		return;

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

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);
	texture_dirty(t);
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

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	texture_dirty(t);
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

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	texture_dirty(t);
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

	context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
	texture_dirty(t);
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

static void
relayout_miptree(GLcontext *ctx, struct gl_texture_object *t)
{
	struct nouveau_surface *ss = to_nouveau_texture(t)->surfaces;
	unsigned last_level, offset = 0;
	unsigned size;
	int i, ret;

	if (t->MinFilter == GL_NEAREST ||
	    t->MinFilter == GL_LINEAR)
		last_level = t->BaseLevel;
	else
		last_level = t->_MaxLevel;

	/* Deallocate the old storage. */
	for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
		nouveau_bo_ref(NULL, &ss[i].bo);

	/* Relayout the mipmap tree. */
	for (i = t->BaseLevel; i <= last_level; i++) {
		struct nouveau_surface *s =
			&to_nouveau_teximage(t->Image[0][i])->surface;

		size = s->width * s->height * s->cpp;

		/* Images larger than 16B have to be aligned. */
		if (size > 16)
			offset = align(offset, 64);

		ss[i] = (struct nouveau_surface) {
			.offset = offset,
			.layout = SWIZZLED,
			.format = s->format,
			.width = s->width,
			.height = s->height,
			.cpp = s->cpp,
			.pitch = s->width * s->cpp,
		};

		offset += size;
	}

	/* Get new storage. */
	size = align(offset, 64);

	ret = nouveau_bo_new(context_dev(ctx),
			     NOUVEAU_BO_GART | NOUVEAU_BO_VRAM,
			     0, size, &ss[last_level].bo);
	assert(!ret);

	for (i = t->BaseLevel; i < last_level; i++)
		nouveau_bo_ref(ss[last_level].bo, &ss[i].bo);
}

void
nouveau_texture_validate(GLcontext *ctx, struct gl_texture_object *t)
{
	struct nouveau_texture *nt = to_nouveau_texture(t);
	int i;

	if (!nt->dirty)
		return;

	nt->dirty = GL_FALSE;

	relayout_miptree(ctx, t);

	/* Copy the teximages to the actual swizzled miptree. */
	for (i = t->BaseLevel; i < MAX_TEXTURE_LEVELS; i++) {
		struct gl_texture_image *ti = t->Image[0][i];
		struct nouveau_surface *s = &to_nouveau_teximage(ti)->surface;

		if (!nt->surfaces[i].bo)
			break;

		context_drv(ctx)->surface_copy(ctx, &nt->surfaces[i], s,
					       0, 0, 0, 0,
					       s->width, s->height);
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
