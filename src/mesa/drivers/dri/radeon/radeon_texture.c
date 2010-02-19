/*
 * Copyright (C) 2009 Maciej Cencora.
 * Copyright (C) 2008 Nicolai Haehnle.
 * Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
 *
 * The Weather Channel (TM) funded Tungsten Graphics to develop the
 * initial release of the Radeon 8500 driver under the XFree86 license.
 * This notice must be preserved.
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

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/convolve.h"
#include "main/enums.h"
#include "main/mipmap.h"
#include "main/texcompress.h"
#include "main/texstore.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texgetimage.h"

#include "xmlpool.h"		/* for symbolic values of enum-type options */

#include "radeon_common.h"

#include "radeon_mipmap_tree.h"


void copy_rows(void* dst, GLuint dststride, const void* src, GLuint srcstride,
	GLuint numrows, GLuint rowsize)
{
	assert(rowsize <= dststride);
	assert(rowsize <= srcstride);

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s dst %p, stride %u, src %p, stride %u, "
		"numrows %u, rowsize %u.\n",
		__func__, dst, dststride,
		src, srcstride,
		numrows, rowsize);

	if (rowsize == srcstride && rowsize == dststride) {
		memcpy(dst, src, numrows*rowsize);
	} else {
		GLuint i;
		for(i = 0; i < numrows; ++i) {
			memcpy(dst, src, rowsize);
			dst += dststride;
			src += srcstride;
		}
	}
}

/* textures */
/**
 * Allocate an empty texture image object.
 */
struct gl_texture_image *radeonNewTextureImage(GLcontext *ctx)
{
	return CALLOC(sizeof(radeon_texture_image));
}

/**
 * Free memory associated with this texture image.
 */
void radeonFreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage)
{
	radeon_texture_image* image = get_radeon_texture_image(timage);

	if (image->mt) {
		radeon_miptree_unreference(&image->mt);
		assert(!image->base.Data);
	} else {
		_mesa_free_texture_image_data(ctx, timage);
	}
	if (image->bo) {
		radeon_bo_unref(image->bo);
		image->bo = NULL;
	}
	if (timage->Data) {
		_mesa_free_texmemory(timage->Data);
		timage->Data = NULL;
	}
}

/* Set Data pointer and additional data for mapped texture image */
static void teximage_set_map_data(radeon_texture_image *image)
{
	radeon_mipmap_level *lvl;

	if (!image->mt) {
		radeon_warning("%s(%p) Trying to set map data without miptree.\n",
				__func__, image);

		return;
	}

	lvl = &image->mt->levels[image->mtlevel];

	image->base.Data = image->mt->bo->ptr + lvl->faces[image->mtface].offset;
	image->base.RowStride = lvl->rowstride / _mesa_get_format_bytes(image->base.TexFormat);
}


/**
 * Map a single texture image for glTexImage and friends.
 */
void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable)
{
	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(img %p), write_enable %s.\n",
			__func__, image,
			write_enable ? "true": "false");
	if (image->mt) {
		assert(!image->base.Data);

		radeon_bo_map(image->mt->bo, write_enable);
		teximage_set_map_data(image);
	}
}


void radeon_teximage_unmap(radeon_texture_image *image)
{
	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(img %p)\n",
			__func__, image);
	if (image->mt) {
		assert(image->base.Data);

		image->base.Data = 0;
		radeon_bo_unmap(image->mt->bo);
	}
}

static void map_override(GLcontext *ctx, radeonTexObj *t)
{
	radeon_texture_image *img = get_radeon_texture_image(t->base.Image[0][0]);

	radeon_bo_map(t->bo, GL_FALSE);

	img->base.Data = t->bo->ptr;
}

static void unmap_override(GLcontext *ctx, radeonTexObj *t)
{
	radeon_texture_image *img = get_radeon_texture_image(t->base.Image[0][0]);

	radeon_bo_unmap(t->bo);

	img->base.Data = NULL;
}

/**
 * Map a validated texture for reading during software rendering.
 */
void radeonMapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	int face, level;

	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(%p, tex %p)\n",
			__func__, ctx, texObj);

	if (!radeon_validate_texture_miptree(ctx, texObj)) {
		radeon_error("%s(%p, tex %p) Failed to validate miptree for "
			"sw fallback.\n",
			__func__, ctx, texObj);
		return;
	}

	if (t->image_override && t->bo) {
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(%p, tex %p) Work around for missing miptree in r100.\n",
			__func__, ctx, texObj);

		map_override(ctx, t);
	}

	/* for r100 3D sw fallbacks don't have mt */
	if (!t->mt) {
		radeon_warning("%s(%p, tex %p) No miptree in texture.\n",
			__func__, ctx, texObj);
		return;
	}

	radeon_bo_map(t->mt->bo, GL_FALSE);
	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->minLod; level <= t->maxLod; ++level)
			teximage_set_map_data(get_radeon_texture_image(texObj->Image[face][level]));
	}
}

void radeonUnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	int face, level;

	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(%p, tex %p)\n",
			__func__, ctx, texObj);

	if (t->image_override && t->bo)
		unmap_override(ctx, t);
	/* for r100 3D sw fallbacks don't have mt */
	if (!t->mt)
	  return;

	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->minLod; level <= t->maxLod; ++level)
			texObj->Image[face][level]->Data = 0;
	}
	radeon_bo_unmap(t->mt->bo);
}

/**
 * Wraps Mesa's implementation to ensure that the base level image is mapped.
 *
 * This relies on internal details of _mesa_generate_mipmap, in particular
 * the fact that the memory for recreated texture images is always freed.
 */
static void radeon_generate_mipmap(GLcontext *ctx, GLenum target,
				   struct gl_texture_object *texObj)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	GLuint nr_faces = (t->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
	int i, face;

	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s(%p, tex %p) Target type %s.\n",
			__func__, ctx, texObj,
			_mesa_lookup_enum_by_nr(target));

	_mesa_generate_mipmap(ctx, target, texObj);

	for (face = 0; face < nr_faces; face++) {
		for (i = texObj->BaseLevel + 1; i < texObj->MaxLevel; i++) {
			radeon_texture_image *image;

			image = get_radeon_texture_image(texObj->Image[face][i]);

			if (image == NULL)
				break;

			image->mtlevel = i;
			image->mtface = face;

			radeon_miptree_unreference(&image->mt);
		}
	}
	
}

void radeonGenerateMipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	struct radeon_bo *bo;
	GLuint face = _mesa_tex_target_to_face(target);
	radeon_texture_image *baseimage = get_radeon_texture_image(texObj->Image[face][texObj->BaseLevel]);
	bo = !baseimage->mt ? baseimage->bo : baseimage->mt->bo;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, target %s, tex %p)\n",
		__func__, ctx, _mesa_lookup_enum_by_nr(target),
		texObj);

	if (bo && radeon_bo_is_referenced_by_cs(bo, rmesa->cmdbuf.cs)) {
		radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s(%p, tex %p) Trying to generate mipmap for texture "
			"in processing by GPU.\n",
			__func__, ctx, texObj);
		radeon_firevertices(rmesa);
	}

	radeon_teximage_map(baseimage, GL_FALSE);
	radeon_generate_mipmap(ctx, target, texObj);
	radeon_teximage_unmap(baseimage);
}


/* try to find a format which will only need a memcopy */
static gl_format radeonChoose8888TexFormat(radeonContextPtr rmesa,
					   GLenum srcFormat,
					   GLenum srcType, GLboolean fbo)
{
	const GLuint ui = 1;
	const GLubyte littleEndian = *((const GLubyte *)&ui);

	/* r100 can only do this */
	if (IS_R100_CLASS(rmesa->radeonScreen) || fbo)
	  return _dri_texformat_argb8888;

	if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
	    (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian)) {
		return MESA_FORMAT_RGBA8888;
	} else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
		   (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) {
		return MESA_FORMAT_RGBA8888_REV;
	} else if (IS_R200_CLASS(rmesa->radeonScreen)) {
		return _dri_texformat_argb8888;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8)) {
		return MESA_FORMAT_ARGB8888_REV;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
		return MESA_FORMAT_ARGB8888;
	} else
		return _dri_texformat_argb8888;
}

gl_format radeonChooseTextureFormat_mesa(GLcontext * ctx,
					 GLint internalFormat,
					 GLenum format,
					 GLenum type)
{
	return radeonChooseTextureFormat(ctx, internalFormat, format,
					 type, 0);
}

gl_format radeonChooseTextureFormat(GLcontext * ctx,
				    GLint internalFormat,
				    GLenum format,
				    GLenum type, GLboolean fbo)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	const GLboolean do32bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32);
	const GLboolean force16bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16);
	(void)format;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s InternalFormat=%s(%d) type=%s format=%s\n",
		__func__,
		_mesa_lookup_enum_by_nr(internalFormat), internalFormat,
		_mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));
	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s do32bpt=%d force16bpt=%d\n",
			__func__, do32bpt, force16bpt);

	switch (internalFormat) {
	case 4:
	case GL_RGBA:
	case GL_COMPRESSED_RGBA:
		switch (type) {
		case GL_UNSIGNED_INT_10_10_10_2:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		default:
			return do32bpt ? radeonChoose8888TexFormat(rmesa, format, type, fbo) :
			    _dri_texformat_argb4444;
		}

	case 3:
	case GL_RGB:
	case GL_COMPRESSED_RGB:
		switch (type) {
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return _dri_texformat_rgb565;
		default:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_rgb565;
		}

	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
		return !force16bpt ?
			radeonChoose8888TexFormat(rmesa, format, type, fbo) :
			_dri_texformat_argb4444;

	case GL_RGBA4:
	case GL_RGBA2:
		return _dri_texformat_argb4444;

	case GL_RGB5_A1:
		return _dri_texformat_argb1555;

	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return !force16bpt ? _dri_texformat_argb8888 :
		    _dri_texformat_rgb565;

	case GL_RGB5:
	case GL_RGB4:
	case GL_R3_G3_B2:
		return _dri_texformat_rgb565;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_COMPRESSED_ALPHA:
		/* r200: can't use a8 format since interpreting hw I8 as a8 would result
		   in wrong rgb values (same as alpha value instead of 0). */
		if (IS_R200_CLASS(rmesa->radeonScreen))
			return _dri_texformat_al88;
		else
			return _dri_texformat_a8;
	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_COMPRESSED_LUMINANCE:
		return _dri_texformat_l8;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_COMPRESSED_LUMINANCE_ALPHA:
		return _dri_texformat_al88;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_COMPRESSED_INTENSITY:
		return _dri_texformat_i8;

	case GL_YCBCR_MESA:
		if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
		    type == GL_UNSIGNED_BYTE)
			return MESA_FORMAT_YCBCR;
		else
			return MESA_FORMAT_YCBCR_REV;

	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return MESA_FORMAT_RGB_DXT1;

	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return MESA_FORMAT_RGBA_DXT1;

	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return MESA_FORMAT_RGBA_DXT3;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return MESA_FORMAT_RGBA_DXT5;

	case GL_ALPHA16F_ARB:
		return MESA_FORMAT_ALPHA_FLOAT16;
	case GL_ALPHA32F_ARB:
		return MESA_FORMAT_ALPHA_FLOAT32;
	case GL_LUMINANCE16F_ARB:
		return MESA_FORMAT_LUMINANCE_FLOAT16;
	case GL_LUMINANCE32F_ARB:
		return MESA_FORMAT_LUMINANCE_FLOAT32;
	case GL_LUMINANCE_ALPHA16F_ARB:
		return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16;
	case GL_LUMINANCE_ALPHA32F_ARB:
		return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32;
	case GL_INTENSITY16F_ARB:
		return MESA_FORMAT_INTENSITY_FLOAT16;
	case GL_INTENSITY32F_ARB:
		return MESA_FORMAT_INTENSITY_FLOAT32;
	case GL_RGB16F_ARB:
		return MESA_FORMAT_RGBA_FLOAT16;
	case GL_RGB32F_ARB:
		return MESA_FORMAT_RGBA_FLOAT32;
	case GL_RGBA16F_ARB:
		return MESA_FORMAT_RGBA_FLOAT16;
	case GL_RGBA32F_ARB:
		return MESA_FORMAT_RGBA_FLOAT32;

#ifdef RADEON_R300
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
		return MESA_FORMAT_Z16;
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_STENCIL_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		if (rmesa->radeonScreen->chip_family >= CHIP_FAMILY_RV515)
			return MESA_FORMAT_S8_Z24;
		else
			return MESA_FORMAT_Z16;
#else
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_STENCIL_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		return MESA_FORMAT_S8_Z24;
#endif

	/* EXT_texture_sRGB */
	case GL_SRGB:
	case GL_SRGB8:
	case GL_SRGB_ALPHA:
	case GL_SRGB8_ALPHA8:
	case GL_COMPRESSED_SRGB:
	case GL_COMPRESSED_SRGB_ALPHA:
		return MESA_FORMAT_SRGBA8;

	case GL_SLUMINANCE:
	case GL_SLUMINANCE8:
	case GL_COMPRESSED_SLUMINANCE:
		return MESA_FORMAT_SL8;

	case GL_SLUMINANCE_ALPHA:
	case GL_SLUMINANCE8_ALPHA8:
	case GL_COMPRESSED_SLUMINANCE_ALPHA:
		return MESA_FORMAT_SLA8;

	default:
		_mesa_problem(ctx,
			      "unexpected internalFormat 0x%x in %s",
			      (int)internalFormat, __func__);
		return MESA_FORMAT_NONE;
	}

	return MESA_FORMAT_NONE;		/* never get here */
}

/** Check if given image is valid within current texture object.
 */
static int image_matches_texture_obj(struct gl_texture_object *texObj,
	struct gl_texture_image *texImage,
	unsigned level)
{
	const struct gl_texture_image *baseImage = texObj->Image[0][texObj->BaseLevel];

	if (!baseImage)
		return 0;

	if (level < texObj->BaseLevel || level > texObj->MaxLevel)
		return 0;

	const unsigned levelDiff = level - texObj->BaseLevel;
	const unsigned refWidth = MAX2(baseImage->Width >> levelDiff, 1);
	const unsigned refHeight = MAX2(baseImage->Height >> levelDiff, 1);
	const unsigned refDepth = MAX2(baseImage->Depth >> levelDiff, 1);

	return (texImage->Width == refWidth &&
			texImage->Height == refHeight &&
			texImage->Depth == refDepth);
}

static void teximage_assign_miptree(radeonContextPtr rmesa,
	struct gl_texture_object *texObj,
	struct gl_texture_image *texImage,
	unsigned face,
	unsigned level)
{
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	/* Since miptree holds only images for levels <BaseLevel..MaxLevel>
	 * don't allocate the miptree if the teximage won't fit.
	 */
	if (!image_matches_texture_obj(texObj, texImage, level))
		return;

	/* Try using current miptree, or create new if there isn't any */
	if (!t->mt || !radeon_miptree_matches_image(t->mt, texImage, face, level)) {
		radeon_miptree_unreference(&t->mt);
		radeon_try_alloc_miptree(rmesa, t);
		radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
				"%s: texObj %p, texImage %p, face %d, level %d, "
				"texObj miptree doesn't match, allocated new miptree %p\n",
				__FUNCTION__, texObj, texImage, face, level, t->mt);
	}

	/* Miptree alocation may have failed,
	 * when there was no image for baselevel specified */
	if (t->mt) {
		image->mtface = face;
		image->mtlevel = level;
		radeon_miptree_reference(t->mt, &image->mt);
	} else
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
				"%s Failed to allocate miptree.\n", __func__);
}

static GLuint * allocate_image_offsets(GLcontext *ctx,
	unsigned alignedWidth,
	unsigned height,
	unsigned depth)
{
	int i;
	GLuint *offsets;

	offsets = malloc(depth * sizeof(GLuint)) ;
	if (!offsets) {
		_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTex[Sub]Image");
		return NULL;
	}

	for (i = 0; i < depth; ++i) {
		offsets[i] = alignedWidth * height * i;
	}

	return offsets;
}

/**
 * Update a subregion of the given texture image.
 */
static void radeon_store_teximage(GLcontext* ctx, int dims,
		GLint xoffset, GLint yoffset, GLint zoffset,
		GLsizei width, GLsizei height, GLsizei depth,
		GLsizei imageSize,
		GLenum format, GLenum type,
		const GLvoid * pixels,
		const struct gl_pixelstore_attrib *packing,
		struct gl_texture_object *texObj,
		struct gl_texture_image *texImage,
		int compressed)
{
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	GLuint dstRowStride;
	GLuint *dstImageOffsets;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s(%p, tex %p, image %p) compressed %d\n",
			__func__, ctx, texObj, texImage, compressed);

	if (image->mt) {
		dstRowStride = image->mt->levels[image->mtlevel].rowstride;
	} else if (t->bo) {
		/* TFP case */
		/* TODO */
		assert(0);
	} else {
		dstRowStride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
	}

	assert(dstRowStride);

	if (dims == 3) {
		unsigned alignedWidth = dstRowStride/_mesa_get_format_bytes(texImage->TexFormat);
		dstImageOffsets = allocate_image_offsets(ctx, alignedWidth, texImage->Height, texImage->Depth);
		if (!dstImageOffsets) {
			radeon_warning("%s Failed to allocate dstImaeOffset.\n", __func__);
			return;
		}
	} else {
		dstImageOffsets = texImage->ImageOffsets;
	}

	radeon_teximage_map(image, GL_TRUE);

	if (compressed) {
		uint32_t srcRowStride, bytesPerRow, rows, block_width, block_height;
		GLubyte *img_start;

		_mesa_get_format_block_size(texImage->TexFormat, &block_width, &block_height);

		if (!image->mt) {
			dstRowStride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
			img_start = _mesa_compressed_image_address(xoffset, yoffset, 0,
									texImage->TexFormat,
									texImage->Width, texImage->Data);
		}
		else {
			uint32_t offset;
			offset = dstRowStride / _mesa_get_format_bytes(texImage->TexFormat) * yoffset / block_height + xoffset / block_width;
			offset *= _mesa_get_format_bytes(texImage->TexFormat);
			img_start = texImage->Data + offset;
		}
		srcRowStride = _mesa_format_row_stride(texImage->TexFormat, width);
		bytesPerRow = srcRowStride;
		rows = (height + block_height - 1) / block_height;

		copy_rows(img_start, dstRowStride, pixels, srcRowStride, rows, bytesPerRow);
	}
	else {
		if (!_mesa_texstore(ctx, dims, texImage->_BaseFormat,
					texImage->TexFormat, texImage->Data,
					xoffset, yoffset, zoffset,
					dstRowStride,
					dstImageOffsets,
					width, height, depth,
					format, type, pixels, packing)) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
		}
	}

	if (dims == 3) {
		free(dstImageOffsets);
	}

	radeon_teximage_unmap(image);
}

/**
 * All glTexImage calls go through this function.
 */
static void radeon_teximage(
	GLcontext *ctx, int dims,
	GLenum target, GLint level,
	GLint internalFormat,
	GLint width, GLint height, GLint depth,
	GLsizei imageSize,
	GLenum format, GLenum type, const GLvoid * pixels,
	const struct gl_pixelstore_attrib *packing,
	struct gl_texture_object *texObj,
	struct gl_texture_image *texImage,
	int compressed)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeonTexObj* t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);
	GLint postConvWidth = width;
	GLint postConvHeight = height;
	GLuint face = _mesa_tex_target_to_face(target);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s %dd: texObj %p, texImage %p, face %d, level %d\n",
			__func__, dims, texObj, texImage, face, level);
	{
		struct radeon_bo *bo;
		bo = !image->mt ? image->bo : image->mt->bo;
		if (bo && radeon_bo_is_referenced_by_cs(bo, rmesa->cmdbuf.cs)) {
			radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
				"%s Calling teximage for texture that is "
				"queued for GPU processing.\n",
				__func__);
			radeon_firevertices(rmesa);
		}
	}


	t->validated = GL_FALSE;

	if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
	       _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
						  &postConvHeight);
	}

	if (!_mesa_is_format_compressed(texImage->TexFormat)) {
		GLuint texelBytes = _mesa_get_format_bytes(texImage->TexFormat);
		/* Minimum pitch of 32 bytes */
		if (postConvWidth * texelBytes < 32) {
			postConvWidth = 32 / texelBytes;
			texImage->RowStride = postConvWidth;
		}
		if (!image->mt) {
			assert(texImage->RowStride == postConvWidth);
		}
	}

	/* Mesa core only clears texImage->Data but not image->mt */
	radeonFreeTexImageData(ctx, texImage);

	if (!t->bo) {
		teximage_assign_miptree(rmesa, texObj, texImage, face, level);
		if (!image->mt) {
			int size = _mesa_format_image_size(texImage->TexFormat,
								texImage->Width,
								texImage->Height,
								texImage->Depth);
			texImage->Data = _mesa_alloc_texmemory(size);
			radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
					"%s %dd: texObj %p, texImage %p, "
					" no miptree assigned, using local memory %p\n",
					__func__, dims, texObj, texImage, texImage->Data);
		}
	}

	/* Upload texture image; note that the spec allows pixels to be NULL */
	if (compressed) {
		pixels = _mesa_validate_pbo_compressed_teximage(
			ctx, imageSize, pixels, packing, "glCompressedTexImage");
	} else {
		pixels = _mesa_validate_pbo_teximage(
			ctx, dims, width, height, depth,
			format, type, pixels, packing, "glTexImage");
	}

	if (pixels) {
		radeon_store_teximage(ctx, dims,
			0, 0, 0,
			width, height, depth,
			imageSize, format, type,
			pixels, packing,
			texObj, texImage,
			compressed);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);
}

void radeonTexImage1D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 1, target, level, internalFormat, width, 1, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

void radeonTexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)

{
	radeon_teximage(ctx, 2, target, level, internalFormat, width, height, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

void radeonCompressedTexImage2D(GLcontext * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 2, target, level, internalFormat, width, height, 1,
		imageSize, 0, 0, data, &ctx->Unpack, texObj, texImage, 1);
}

void radeonTexImage3D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint depth,
		      GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 3, target, level, internalFormat, width, height, depth,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

/**
 * All glTexSubImage calls go through this function.
 */
static void radeon_texsubimage(GLcontext* ctx, int dims, GLenum target, int level,
		GLint xoffset, GLint yoffset, GLint zoffset,
		GLsizei width, GLsizei height, GLsizei depth,
		GLsizei imageSize,
		GLenum format, GLenum type,
		const GLvoid * pixels,
		const struct gl_pixelstore_attrib *packing,
		struct gl_texture_object *texObj,
		struct gl_texture_image *texImage,
		int compressed)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeonTexObj* t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s %dd: texObj %p, texImage %p, face %d, level %d\n",
			__func__, dims, texObj, texImage,
			_mesa_tex_target_to_face(target), level);
	{
		struct radeon_bo *bo;
		bo = !image->mt ? image->bo : image->mt->bo;
		if (bo && radeon_bo_is_referenced_by_cs(bo, rmesa->cmdbuf.cs)) {
			radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
				"%s Calling texsubimage for texture that is "
				"queued for GPU processing.\n",
				__func__);
			radeon_firevertices(rmesa);
		}
	}


	t->validated = GL_FALSE;
	if (compressed) {
		pixels = _mesa_validate_pbo_compressed_teximage(
			ctx, imageSize, pixels, packing, "glCompressedTexSubImage");
	} else {
		pixels = _mesa_validate_pbo_teximage(ctx, dims,
			width, height, depth, format, type, pixels, packing, "glTexSubImage");
	}

	if (pixels) {
		radeon_store_teximage(ctx, dims,
			xoffset, yoffset, zoffset,
			width, height, depth,
			imageSize, format, type,
			pixels, packing,
			texObj, texImage,
			compressed);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);
}

void radeonTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset,
			 GLsizei width,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 1, target, level, xoffset, 0, 0, width, 1, 1, 0,
		format, type, pixels, packing, texObj, texImage, 0);
}

void radeonTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset,
			 GLsizei width, GLsizei height,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 2, target, level, xoffset, yoffset, 0, width, height, 1,
			   0, format, type, pixels, packing, texObj, texImage,
			   0);
}

void radeonCompressedTexSubImage2D(GLcontext * ctx, GLenum target,
				   GLint level, GLint xoffset,
				   GLint yoffset, GLsizei width,
				   GLsizei height, GLenum format,
				   GLsizei imageSize, const GLvoid * data,
				   struct gl_texture_object *texObj,
				   struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 2, target, level, xoffset, yoffset, 0, width, height, 1,
		imageSize, format, 0, data, &ctx->Unpack, texObj, texImage, 1);
}


void radeonTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 3, target, level, xoffset, yoffset, zoffset, width, height, depth, 0,
		format, type, pixels, packing, texObj, texImage, 0);
}

/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
radeon_get_tex_image(GLcontext * ctx, GLenum target, GLint level,
		     GLenum format, GLenum type, GLvoid * pixels,
		     struct gl_texture_object *texObj,
		     struct gl_texture_image *texImage, int compressed)
{
	radeon_texture_image *image = get_radeon_texture_image(texImage);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s(%p, tex %p, image %p) compressed %d.\n",
			__func__, ctx, texObj, image, compressed);

	if (image->mt) {
		/* Map the texture image read-only */
		radeon_teximage_map(image, GL_FALSE);
	} else {
		/* Image hasn't been uploaded to a miptree yet */
		assert(image->base.Data);
	}

	if (compressed) {
		/* FIXME: this can't work for small textures (mips) which
		         use different hw stride */
		_mesa_get_compressed_teximage(ctx, target, level, pixels,
					      texObj, texImage);
	} else {
		_mesa_get_teximage(ctx, target, level, format, type, pixels,
				   texObj, texImage);
	}
     
	if (image->mt) {
		radeon_teximage_unmap(image);
	}
}

void
radeonGetTexImage(GLcontext * ctx, GLenum target, GLint level,
		  GLenum format, GLenum type, GLvoid * pixels,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
	radeon_get_tex_image(ctx, target, level, format, type, pixels,
			     texObj, texImage, 0);
}

void
radeonGetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
			    GLvoid *pixels,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage)
{
	radeon_get_tex_image(ctx, target, level, 0, 0, pixels,
			     texObj, texImage, 1);
}
