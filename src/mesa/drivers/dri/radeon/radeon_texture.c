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
#include "main/enums.h"
#include "main/mfeatures.h"
#include "main/mipmap.h"
#include "main/pbo.h"
#include "main/texcompress.h"
#include "main/texstore.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "drivers/common/meta.h"

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
struct gl_texture_image *radeonNewTextureImage(struct gl_context *ctx)
{
	return CALLOC(sizeof(radeon_texture_image));
}


/**
 * Delete a texture image object.
 */
static void
radeonDeleteTextureImage(struct gl_context *ctx, struct gl_texture_image *img)
{
	/* nothing special (yet) for radeon_texture_image */
	_mesa_delete_texture_image(ctx, img);
}


/**
 * Free memory associated with this texture image.
 */
void radeonFreeTextureImageBuffer(struct gl_context *ctx, struct gl_texture_image *timage)
{
	radeon_texture_image* image = get_radeon_texture_image(timage);

	if (image->mt) {
		radeon_miptree_unreference(&image->mt);
		assert(!image->base.Buffer);
	} else {
		_swrast_free_texture_image_buffer(ctx, timage);
	}
	if (image->bo) {
		radeon_bo_unref(image->bo);
		image->bo = NULL;
	}
	if (image->base.Buffer) {
		_mesa_align_free(image->base.Buffer);
		image->base.Buffer = NULL;
	}

	if (image->base.ImageOffsets) {
		free(image->base.ImageOffsets);
		image->base.ImageOffsets = NULL;
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

	lvl = &image->mt->levels[image->base.Base.Level];

	image->base.Data = image->mt->bo->ptr + lvl->faces[image->base.Base.Face].offset;
	image->base.RowStride = lvl->rowstride / _mesa_get_format_bytes(image->base.Base.TexFormat);
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

static void map_override(struct gl_context *ctx, radeonTexObj *t)
{
	radeon_texture_image *img = get_radeon_texture_image(t->base.Image[0][0]);

	radeon_bo_map(t->bo, GL_FALSE);

	img->base.Data = t->bo->ptr;
}

static void unmap_override(struct gl_context *ctx, radeonTexObj *t)
{
	radeon_texture_image *img = get_radeon_texture_image(t->base.Image[0][0]);

	radeon_bo_unmap(t->bo);

	img->base.Data = NULL;
}

/**
 * Map a validated texture for reading during software rendering.
 */
void radeonMapTexture(struct gl_context *ctx, struct gl_texture_object *texObj)
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

void radeonUnmapTexture(struct gl_context *ctx, struct gl_texture_object *texObj)
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
		for(level = t->minLod; level <= t->maxLod; ++level) {
			radeon_texture_image *image =
				get_radeon_texture_image(texObj->Image[face][level]);
			image->base.Data = NULL;
		}
	}
	radeon_bo_unmap(t->mt->bo);
}


/**
 * Map texture memory/buffer into user space.
 * Note: the region of interest parameters are ignored here.
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride in bytes
 */
static void
radeon_map_texture_image(struct gl_context *ctx,
			 struct gl_texture_image *texImage,
			 GLuint slice,
			 GLuint x, GLuint y, GLuint w, GLuint h,
			 GLbitfield mode,
			 GLubyte **map,
			 GLint *stride)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeon_texture_image *image = get_radeon_texture_image(texImage);
	radeon_mipmap_tree *mt = image->mt;
	GLuint texel_size = _mesa_get_format_bytes(texImage->TexFormat);
	GLuint width = texImage->Width;
	GLuint height = texImage->Height;
	struct radeon_bo *bo = !image->mt ? image->bo : image->mt->bo;
	unsigned int bw, bh;
	GLboolean write = (mode & GL_MAP_WRITE_BIT) != 0;

	_mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);
	assert(y % bh == 0);
	y /= bh;
	height /= bh;

	if (bo && radeon_bo_is_referenced_by_cs(bo, rmesa->cmdbuf.cs)) {
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			     "%s for texture that is "
			     "queued for GPU processing.\n",
			     __func__);
		radeon_firevertices(rmesa);
	}

	if (image->bo) {
		/* TFP case */
		radeon_bo_map(image->bo, write);
		*stride = get_texture_image_row_stride(rmesa, texImage->TexFormat, width, 0, texImage->TexObject->Target);
		*map = bo->ptr;
	} else if (likely(mt)) {
		radeon_bo_map(mt->bo, write);
		radeon_mipmap_level *lvl = &image->mt->levels[texImage->Level];
		void *base = mt->bo->ptr + lvl->faces[image->base.Base.Face].offset;

		*stride = lvl->rowstride;
		*map = base + (slice * height) * *stride;
	} else {
		/* texture data is in malloc'd memory */

		assert(map);

		*stride = _mesa_format_row_stride(texImage->TexFormat, width);
		*map = image->base.Buffer + (slice * height) * *stride;
	}

	*map += y * *stride + x * texel_size;
}

static void
radeon_unmap_texture_image(struct gl_context *ctx,
			   struct gl_texture_image *texImage, GLuint slice)
{
	radeon_texture_image *image = get_radeon_texture_image(texImage);

	if (image->bo)
		radeon_bo_unmap(image->bo);
	else if (image->mt)
		radeon_bo_unmap(image->mt->bo);
}

/* try to find a format which will only need a memcopy */
static gl_format radeonChoose8888TexFormat(radeonContextPtr rmesa,
					   GLenum srcFormat,
					   GLenum srcType, GLboolean fbo)
{
#if defined(RADEON_R100)
	/* r100 can only do this */
	return _radeon_texformat_argb8888;
#elif defined(RADEON_R200)
	const GLuint ui = 1;
	const GLubyte littleEndian = *((const GLubyte *)&ui);

	if (fbo)
		return _radeon_texformat_argb8888;

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
	} else
		return _radeon_texformat_argb8888;
#endif
}

gl_format radeonChooseTextureFormat_mesa(struct gl_context * ctx,
					 GLint internalFormat,
					 GLenum format,
					 GLenum type)
{
	return radeonChooseTextureFormat(ctx, internalFormat, format,
					 type, 0);
}

gl_format radeonChooseTextureFormat(struct gl_context * ctx,
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
			return do32bpt ? _radeon_texformat_argb8888 :
			    _radeon_texformat_argb1555;
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _radeon_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _radeon_texformat_argb1555;
		default:
			return do32bpt ? radeonChoose8888TexFormat(rmesa, format, type, fbo) :
			    _radeon_texformat_argb4444;
		}

	case 3:
	case GL_RGB:
	case GL_COMPRESSED_RGB:
		switch (type) {
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _radeon_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _radeon_texformat_argb1555;
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return _radeon_texformat_rgb565;
		default:
			return do32bpt ? _radeon_texformat_argb8888 :
			    _radeon_texformat_rgb565;
		}

	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
		return !force16bpt ?
			radeonChoose8888TexFormat(rmesa, format, type, fbo) :
			_radeon_texformat_argb4444;

	case GL_RGBA4:
	case GL_RGBA2:
		return _radeon_texformat_argb4444;

	case GL_RGB5_A1:
		return _radeon_texformat_argb1555;

	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return !force16bpt ? _radeon_texformat_argb8888 :
		    _radeon_texformat_rgb565;

	case GL_RGB5:
	case GL_RGB4:
	case GL_R3_G3_B2:
		return _radeon_texformat_rgb565;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_COMPRESSED_ALPHA:
#if defined(RADEON_R200)
		/* r200: can't use a8 format since interpreting hw I8 as a8 would result
		   in wrong rgb values (same as alpha value instead of 0). */
		return _radeon_texformat_al88;
#else
		return MESA_FORMAT_A8;
#endif
	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_COMPRESSED_LUMINANCE:
		return MESA_FORMAT_L8;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_COMPRESSED_LUMINANCE_ALPHA:
		return _radeon_texformat_al88;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_COMPRESSED_INTENSITY:
		return MESA_FORMAT_I8;

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

	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_STENCIL_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		return MESA_FORMAT_S8_Z24;

	/* EXT_texture_sRGB */
	case GL_SRGB:
	case GL_SRGB8:
	case GL_SRGB_ALPHA:
	case GL_SRGB8_ALPHA8:
	case GL_COMPRESSED_SRGB:
	case GL_COMPRESSED_SRGB_ALPHA:
		return MESA_FORMAT_SARGB8;

	case GL_SLUMINANCE:
	case GL_SLUMINANCE8:
	case GL_COMPRESSED_SLUMINANCE:
		return MESA_FORMAT_SL8;

	case GL_SLUMINANCE_ALPHA:
	case GL_SLUMINANCE8_ALPHA8:
	case GL_COMPRESSED_SLUMINANCE_ALPHA:
		return MESA_FORMAT_SLA8;

	case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
		return MESA_FORMAT_SRGB_DXT1;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
		return MESA_FORMAT_SRGBA_DXT1;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
		return MESA_FORMAT_SRGBA_DXT3;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
		return MESA_FORMAT_SRGBA_DXT5;

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
		radeon_miptree_reference(t->mt, &image->mt);
	} else
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
				"%s Failed to allocate miptree.\n", __func__);
}


/**
 * Update a subregion of the given texture image.
 */
static void radeon_store_teximage(struct gl_context* ctx, int dims,
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
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);
	GLuint texel_size = _mesa_get_format_bytes(texImage->TexFormat);

	GLuint dstRowStride;
	GLuint alignedWidth;
	GLint i;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s(%p, tex %p, image %p) compressed %d\n",
			__func__, ctx, texObj, texImage, compressed);

	if (image->mt) {
		dstRowStride = image->mt->levels[image->base.Base.Level].rowstride;
	} else if (t->bo) {
		/* TFP case */
		dstRowStride = get_texture_image_row_stride(rmesa, texImage->TexFormat, width, 0, texObj->Target);
	} else {
		dstRowStride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
	}

	assert(dstRowStride);

	/* fill in the ImageOffsets array */
	alignedWidth = dstRowStride / texel_size;
	for (i = 0; i < texImage->Depth; ++i) {
		image->base.ImageOffsets[i] = alignedWidth * texImage->Height * i;
	}
	/* and fill in RowStride (in texels) */
	image->base.RowStride = alignedWidth;

	radeon_teximage_map(image, GL_TRUE);

	if (compressed) {
		uint32_t srcRowStride, bytesPerRow, rows, block_width, block_height;
		GLubyte *img_start;

		_mesa_get_format_block_size(texImage->TexFormat, &block_width, &block_height);

		if (!image->mt) {
			dstRowStride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
			img_start = _mesa_compressed_image_address(xoffset, yoffset, 0,
									texImage->TexFormat,
									texImage->Width, image->base.Data);
		}
		else {
			uint32_t offset;
			offset = dstRowStride / texel_size * yoffset / block_height + xoffset / block_width;
			offset *= texel_size;
			img_start = image->base.Data + offset;
		}
		srcRowStride = _mesa_format_row_stride(texImage->TexFormat, width);
		bytesPerRow = srcRowStride;
		rows = (height + block_height - 1) / block_height;

		copy_rows(img_start, dstRowStride, pixels, srcRowStride, rows, bytesPerRow);
	}
	else {
		GLubyte *slices[512];
		GLuint i;
		assert(depth <= 512);
		for (i = 0; i < depth; i++) {
			slices[i] = (GLubyte *) image->base.Data
				+ image->base.ImageOffsets[i] * texel_size;
		}
		if (!_mesa_texstore(ctx, dims, texImage->_BaseFormat,
					texImage->TexFormat,
					xoffset, yoffset, zoffset,
					dstRowStride,
					slices,
					width, height, depth,
					format, type, pixels, packing)) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
		}
	}

	radeon_teximage_unmap(image);
}

/**
 * All glTexImage calls go through this function.
 */
static void radeon_teximage(
	struct gl_context *ctx, int dims,
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
	GLuint face = _mesa_tex_target_to_face(target);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s %dd: texObj %p, texImage %p, face %d, level %d\n",
			__func__, dims, texObj, texImage, face, level);

	t->validated = GL_FALSE;

	radeonFreeTextureImageBuffer(ctx, texImage);

	if (!t->bo) {
		teximage_assign_miptree(rmesa, texObj, texImage, face, level);
		if (!image->mt) {
			int size = _mesa_format_image_size(texImage->TexFormat,
								texImage->Width,
								texImage->Height,
								texImage->Depth);
			image->base.Buffer = _mesa_align_malloc(size, 512);

			radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
					"%s %dd: texObj %p, texImage %p, "
					" no miptree assigned, using local memory %p\n",
					__func__, dims, texObj, texImage, image->base.Buffer);
		}
	}

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

	image->base.ImageOffsets =
		(GLuint *) malloc(texImage->Depth * sizeof(GLuint));


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

void radeonTexImage1D(struct gl_context * ctx, GLenum target, GLint level,
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

void radeonTexImage2D(struct gl_context * ctx, GLenum target, GLint level,
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

void radeonCompressedTexImage2D(struct gl_context * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 2, target, level, internalFormat, width, height, 1,
		imageSize, 0, 0, data, &ctx->Unpack, texObj, texImage, 1);
}

void radeonTexImage3D(struct gl_context * ctx, GLenum target, GLint level,
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
static void radeon_texsubimage(struct gl_context* ctx, int dims, GLenum target, int level,
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

void radeonTexSubImage1D(struct gl_context * ctx, GLenum target, GLint level,
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

void radeonTexSubImage2D(struct gl_context * ctx, GLenum target, GLint level,
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

void radeonCompressedTexSubImage2D(struct gl_context * ctx, GLenum target,
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


void radeonTexSubImage3D(struct gl_context * ctx, GLenum target, GLint level,
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

unsigned radeonIsFormatRenderable(gl_format mesa_format)
{
	if (mesa_format == _radeon_texformat_argb8888 || mesa_format == _radeon_texformat_rgb565 ||
		mesa_format == _radeon_texformat_argb1555 || mesa_format == _radeon_texformat_argb4444)
		return 1;

	switch (mesa_format)
	{
		case MESA_FORMAT_Z16:
		case MESA_FORMAT_S8_Z24:
			return 1;
		default:
			return 0;
	}
}

#if FEATURE_OES_EGL_image
void radeon_image_target_texture_2d(struct gl_context *ctx, GLenum target,
				    struct gl_texture_object *texObj,
				    struct gl_texture_image *texImage,
				    GLeglImageOES image_handle)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image *radeonImage = get_radeon_texture_image(texImage);
	__DRIscreen *screen;
	__DRIimage *image;

	screen = radeon->dri.screen;
	image = screen->dri2.image->lookupEGLImage(screen, image_handle,
						   screen->loaderPrivate);
	if (image == NULL)
		return;

	radeonFreeTextureImageBuffer(ctx, texImage);

	texImage->Width = image->width;
	texImage->Height = image->height;
	texImage->Depth = 1;
	texImage->_BaseFormat = GL_RGBA;
	texImage->TexFormat = image->format;
	radeonImage->base.RowStride = image->pitch;
	texImage->InternalFormat = image->internal_format;

	if(t->mt)
	{
		radeon_miptree_unreference(&t->mt);
		t->mt = NULL;
	}

	/* NOTE: The following is *very* ugly and will probably break. But
	   I don't know how to deal with it, without creating a whole new
	   function like radeon_miptree_from_bo() so I'm going with the
	   easy but error-prone way. */

	radeon_try_alloc_miptree(radeon, t);

	radeon_miptree_reference(t->mt, &radeonImage->mt);

	if (t->mt == NULL)
	{
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			     "%s Failed to allocate miptree.\n", __func__);
		return;
	}

	/* Particularly ugly: this is guaranteed to break, if image->bo is
	   not of the required size for a miptree. */
	radeon_bo_unref(t->mt->bo);
	radeon_bo_ref(image->bo);
	t->mt->bo = image->bo;

	if (!radeon_miptree_matches_image(t->mt, &radeonImage->base.Base,
					  radeonImage->base.Base.Face, 0))
		fprintf(stderr, "miptree doesn't match image\n");
}
#endif

gl_format _radeon_texformat_rgba8888 = MESA_FORMAT_NONE;
gl_format _radeon_texformat_argb8888 = MESA_FORMAT_NONE;
gl_format _radeon_texformat_rgb565 = MESA_FORMAT_NONE;
gl_format _radeon_texformat_argb4444 = MESA_FORMAT_NONE;
gl_format _radeon_texformat_argb1555 = MESA_FORMAT_NONE;
gl_format _radeon_texformat_al88 = MESA_FORMAT_NONE;
/*@}*/


static void
radeonInitTextureFormats(void)
{
   if (_mesa_little_endian()) {
      _radeon_texformat_rgba8888	= MESA_FORMAT_RGBA8888;
      _radeon_texformat_argb8888	= MESA_FORMAT_ARGB8888;
      _radeon_texformat_rgb565		= MESA_FORMAT_RGB565;
      _radeon_texformat_argb4444	= MESA_FORMAT_ARGB4444;
      _radeon_texformat_argb1555	= MESA_FORMAT_ARGB1555;
      _radeon_texformat_al88		= MESA_FORMAT_AL88;
   }
   else {
      _radeon_texformat_rgba8888	= MESA_FORMAT_RGBA8888_REV;
      _radeon_texformat_argb8888	= MESA_FORMAT_ARGB8888_REV;
      _radeon_texformat_rgb565		= MESA_FORMAT_RGB565_REV;
      _radeon_texformat_argb4444	= MESA_FORMAT_ARGB4444_REV;
      _radeon_texformat_argb1555	= MESA_FORMAT_ARGB1555_REV;
      _radeon_texformat_al88		= MESA_FORMAT_AL88_REV;
   }
}

void
radeon_init_common_texture_funcs(radeonContextPtr radeon,
				 struct dd_function_table *functions)
{
	functions->NewTextureImage = radeonNewTextureImage;
	functions->DeleteTextureImage = radeonDeleteTextureImage;
	functions->FreeTextureImageBuffer = radeonFreeTextureImageBuffer;
	functions->MapTexture = radeonMapTexture;
	functions->UnmapTexture = radeonUnmapTexture;
	functions->MapTextureImage = radeon_map_texture_image;
	functions->UnmapTextureImage = radeon_unmap_texture_image;

	functions->ChooseTextureFormat	= radeonChooseTextureFormat_mesa;

	functions->TexImage1D = radeonTexImage1D;
	functions->TexImage2D = radeonTexImage2D;
	functions->TexImage3D = radeonTexImage3D;
	functions->TexSubImage1D = radeonTexSubImage1D;
	functions->TexSubImage2D = radeonTexSubImage2D;
	functions->TexSubImage3D = radeonTexSubImage3D;
	functions->CompressedTexImage2D = radeonCompressedTexImage2D;
	functions->CompressedTexSubImage2D = radeonCompressedTexSubImage2D;

	functions->CopyTexSubImage2D = radeonCopyTexSubImage2D;

	functions->Bitmap = _mesa_meta_Bitmap;
#if FEATURE_OES_EGL_image
	functions->EGLImageTargetTexture2D = radeon_image_target_texture_2d;
#endif

	radeonInitTextureFormats();
}
