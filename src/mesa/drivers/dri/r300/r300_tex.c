/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/mipmap.h"
#include "main/simple_list.h"
#include "main/texformat.h"
#include "main/texstore.h"
#include "main/teximage.h"
#include "main/texobj.h"

#include "texmem.h"

#include "r300_context.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "r300_mipmap_tree.h"
#include "r300_tex.h"

#include "xmlpool.h"


static unsigned int translate_wrap_mode(GLenum wrapmode)
{
	switch(wrapmode) {
	case GL_REPEAT: return R300_TX_REPEAT;
	case GL_CLAMP: return R300_TX_CLAMP;
	case GL_CLAMP_TO_EDGE: return R300_TX_CLAMP_TO_EDGE;
	case GL_CLAMP_TO_BORDER: return R300_TX_CLAMP_TO_BORDER;
	case GL_MIRRORED_REPEAT: return R300_TX_REPEAT | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_EXT: return R300_TX_CLAMP | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT: return R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT: return R300_TX_CLAMP_TO_BORDER | R300_TX_MIRRORED;
	default:
		_mesa_problem(NULL, "bad wrap mode in %s", __FUNCTION__);
		return 0;
	}
}


/**
 * Update the cached hardware registers based on the current texture wrap modes.
 *
 * \param t Texture object whose wrap modes are to be set
 */
static void r300UpdateTexWrap(r300TexObjPtr t)
{
	struct gl_texture_object *tObj = &t->base;

	t->filter &=
	    ~(R300_TX_WRAP_S_MASK | R300_TX_WRAP_T_MASK | R300_TX_WRAP_R_MASK);

	t->filter |= translate_wrap_mode(tObj->WrapS) << R300_TX_WRAP_S_SHIFT;

	if (tObj->Target != GL_TEXTURE_1D) {
		t->filter |= translate_wrap_mode(tObj->WrapT) << R300_TX_WRAP_T_SHIFT;

		if (tObj->Target == GL_TEXTURE_3D)
			t->filter |= translate_wrap_mode(tObj->WrapR) << R300_TX_WRAP_R_SHIFT;
	}
}

static GLuint aniso_filter(GLfloat anisotropy)
{
	if (anisotropy >= 16.0) {
		return R300_TX_MAX_ANISO_16_TO_1;
	} else if (anisotropy >= 8.0) {
		return R300_TX_MAX_ANISO_8_TO_1;
	} else if (anisotropy >= 4.0) {
		return R300_TX_MAX_ANISO_4_TO_1;
	} else if (anisotropy >= 2.0) {
		return R300_TX_MAX_ANISO_2_TO_1;
	} else {
		return R300_TX_MAX_ANISO_1_TO_1;
	}
}

/**
 * Set the texture magnification and minification modes.
 *
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 * \param anisotropy Maximum anisotropy level
 */
static void r300SetTexFilter(r300TexObjPtr t, GLenum minf, GLenum magf, GLfloat anisotropy)
{
	/* Force revalidation to account for switches from/to mipmapping. */
	t->validated = GL_FALSE;

	t->filter &= ~(R300_TX_MIN_FILTER_MASK | R300_TX_MIN_FILTER_MIP_MASK | R300_TX_MAG_FILTER_MASK | R300_TX_MAX_ANISO_MASK);
	t->filter_1 &= ~R300_EDGE_ANISO_EDGE_ONLY;

	/* Note that EXT_texture_filter_anisotropic is extremely vague about
	 * how anisotropic filtering interacts with the "normal" filter modes.
	 * When anisotropic filtering is enabled, we override min and mag
	 * filter settings completely. This includes driconf's settings.
	 */
	if (anisotropy >= 2.0 && (minf != GL_NEAREST) && (magf != GL_NEAREST)) {
		t->filter |= R300_TX_MAG_FILTER_ANISO
			| R300_TX_MIN_FILTER_ANISO
			| R300_TX_MIN_FILTER_MIP_LINEAR
			| aniso_filter(anisotropy);
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "Using maximum anisotropy of %f\n", anisotropy);
		return;
	}

	switch (minf) {
	case GL_NEAREST:
		t->filter |= R300_TX_MIN_FILTER_NEAREST;
		break;
	case GL_LINEAR:
		t->filter |= R300_TX_MIN_FILTER_LINEAR;
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		t->filter |= R300_TX_MIN_FILTER_NEAREST|R300_TX_MIN_FILTER_MIP_NEAREST;
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		t->filter |= R300_TX_MIN_FILTER_NEAREST|R300_TX_MIN_FILTER_MIP_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		t->filter |= R300_TX_MIN_FILTER_LINEAR|R300_TX_MIN_FILTER_MIP_NEAREST;
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		t->filter |= R300_TX_MIN_FILTER_LINEAR|R300_TX_MIN_FILTER_MIP_LINEAR;
		break;
	}

	/* Note we don't have 3D mipmaps so only use the mag filter setting
	 * to set the 3D texture filter mode.
	 */
	switch (magf) {
	case GL_NEAREST:
		t->filter |= R300_TX_MAG_FILTER_NEAREST;
		break;
	case GL_LINEAR:
		t->filter |= R300_TX_MAG_FILTER_LINEAR;
		break;
	}
}

static void r300SetTexBorderColor(r300TexObjPtr t, GLubyte c[4])
{
	t->pp_border_color = PACK_COLOR_8888(c[3], c[0], c[1], c[2]);
}

/* try to find a format which will only need a memcopy */
static const struct gl_texture_format *r300Choose8888TexFormat(GLenum srcFormat,
							       GLenum srcType)
{
	const GLuint ui = 1;
	const GLubyte littleEndian = *((const GLubyte *)&ui);

	if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
	    (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian)) {
		return &_mesa_texformat_rgba8888;
	} else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
		   (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) {
		return &_mesa_texformat_rgba8888_rev;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8)) {
		return &_mesa_texformat_argb8888_rev;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
		return &_mesa_texformat_argb8888;
	} else
		return _dri_texformat_argb8888;
}

static const struct gl_texture_format *r300ChooseTextureFormat(GLcontext * ctx,
							       GLint
							       internalFormat,
							       GLenum format,
							       GLenum type)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const GLboolean do32bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32);
	const GLboolean force16bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16);
	(void)format;

#if 0
	fprintf(stderr, "InternalFormat=%s(%d) type=%s format=%s\n",
		_mesa_lookup_enum_by_nr(internalFormat), internalFormat,
		_mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));
	fprintf(stderr, "do32bpt=%d force16bpt=%d\n", do32bpt, force16bpt);
#endif

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
			return do32bpt ? r300Choose8888TexFormat(format, type) :
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
		    r300Choose8888TexFormat(format,
					    type) : _dri_texformat_argb4444;

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
			return &_mesa_texformat_ycbcr;
		else
			return &_mesa_texformat_ycbcr_rev;

	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgb_dxt1;

	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgba_dxt1;

	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return &_mesa_texformat_rgba_dxt3;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return &_mesa_texformat_rgba_dxt5;

	case GL_ALPHA16F_ARB:
		return &_mesa_texformat_alpha_float16;
	case GL_ALPHA32F_ARB:
		return &_mesa_texformat_alpha_float32;
	case GL_LUMINANCE16F_ARB:
		return &_mesa_texformat_luminance_float16;
	case GL_LUMINANCE32F_ARB:
		return &_mesa_texformat_luminance_float32;
	case GL_LUMINANCE_ALPHA16F_ARB:
		return &_mesa_texformat_luminance_alpha_float16;
	case GL_LUMINANCE_ALPHA32F_ARB:
		return &_mesa_texformat_luminance_alpha_float32;
	case GL_INTENSITY16F_ARB:
		return &_mesa_texformat_intensity_float16;
	case GL_INTENSITY32F_ARB:
		return &_mesa_texformat_intensity_float32;
	case GL_RGB16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGB32F_ARB:
		return &_mesa_texformat_rgba_float32;
	case GL_RGBA16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGBA32F_ARB:
		return &_mesa_texformat_rgba_float32;

	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
#if 0
		switch (type) {
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT:
			return &_mesa_texformat_z16;
		case GL_UNSIGNED_INT:
			return &_mesa_texformat_z32;
		case GL_UNSIGNED_INT_24_8_EXT:
		default:
			return &_mesa_texformat_z24_s8;
		}
#else
		return &_mesa_texformat_z16;
#endif

	default:
		_mesa_problem(ctx,
			      "unexpected internalFormat 0x%x in r300ChooseTextureFormat",
			      (int)internalFormat);
		return NULL;
	}

	return NULL;		/* never get here */
}


/**
 * Allocate an empty texture image object.
 */
static struct gl_texture_image *r300NewTextureImage(GLcontext *ctx)
{
	return CALLOC(sizeof(r300_texture_image));
}

/**
 * Free memory associated with this texture image.
 */
static void r300FreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage)
{
	r300_texture_image* image = get_r300_texture_image(timage);

	if (image->mt) {
		r300_miptree_unreference(image->mt);
		image->mt = 0;
		assert(!image->base.Data);
	} else {
		_mesa_free_texture_image_data(ctx, timage);
	}
    if (image->bo) {
        radeon_bo_unref(image->bo);
        image->bo = NULL;
    }
}


/* Set Data pointer and additional data for mapped texture image */
static void teximage_set_map_data(r300_texture_image *image)
{
	r300_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
	image->base.Data = image->mt->bo->ptr + lvl->faces[image->mtface].offset;
	image->base.RowStride = lvl->rowstride / image->mt->bpp;
}


/**
 * Map a single texture image for glTexImage and friends.
 */
static void r300_teximage_map(r300_texture_image *image, GLboolean write_enable)
{
	if (image->mt) {
		assert(!image->base.Data);

		radeon_bo_map(image->mt->bo, write_enable);
		teximage_set_map_data(image);
	}
}


static void r300_teximage_unmap(r300_texture_image *image)
{
	if (image->mt) {
		assert(image->base.Data);

		image->base.Data = 0;
		radeon_bo_unmap(image->mt->bo);
	}
}

/**
 * Map a validated texture for reading during software rendering.
 */
static void r300MapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	r300TexObj* t = r300_tex_obj(texObj);
	int face, level;

	assert(texObj->_Complete);
	assert(t->mt);

	radeon_bo_map(t->mt->bo, GL_FALSE);
	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level)
			teximage_set_map_data(get_r300_texture_image(texObj->Image[face][level]));
	}
}

static void r300UnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	r300TexObj* t = r300_tex_obj(texObj);
	int face, level;

	assert(texObj->_Complete);
	assert(t->mt);

	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level)
			texObj->Image[face][level]->Data = 0;
	}
	radeon_bo_unmap(t->mt->bo);
}

/**
 * All glTexImage calls go through this function.
 */
static void r300_teximage(
	GLcontext *ctx, int dims,
	GLint face, GLint level,
	GLint internalFormat,
	GLint width, GLint height, GLint depth,
	GLsizei imageSize,
	GLenum format, GLenum type, const GLvoid * pixels,
	const struct gl_pixelstore_attrib *packing,
	struct gl_texture_object *texObj,
	struct gl_texture_image *texImage,
	int compressed)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300TexObj* t = r300_tex_obj(texObj);
	r300_texture_image* image = get_r300_texture_image(texImage);

	R300_FIREVERTICES(rmesa);

	t->validated = GL_FALSE;

	/* Choose and fill in the texture format for this image */
	texImage->TexFormat = r300ChooseTextureFormat(ctx, internalFormat, format, type);
	_mesa_set_fetch_functions(texImage, dims);

	if (texImage->TexFormat->TexelBytes == 0) {
		texImage->IsCompressed = GL_TRUE;
		texImage->CompressedSize =
			ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
					   texImage->Height, texImage->Depth,
					   texImage->TexFormat->MesaFormat);
	} else {
		texImage->IsCompressed = GL_FALSE;
		texImage->CompressedSize = 0;
	}

	/* Allocate memory for image */
	r300FreeTexImageData(ctx, texImage); /* Mesa core only clears texImage->Data but not image->mt */

	if (!t->mt)
		r300_try_alloc_miptree(rmesa, t, texImage, face, level);
	if (t->mt && r300_miptree_matches_image(t->mt, texImage, face, level)) {
		image->mt = t->mt;
		image->mtlevel = level - t->mt->firstLevel;
		image->mtface = face;
		r300_miptree_reference(t->mt);
	} else {
		int size;
		if (texImage->IsCompressed) {
			size = texImage->CompressedSize;
		} else {
			size = texImage->Width * texImage->Height * texImage->Depth * texImage->TexFormat->TexelBytes;
		}
		texImage->Data = _mesa_alloc_texmemory(size);
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
		r300_teximage_map(image, GL_TRUE);

		if (compressed) {
			memcpy(texImage->Data, pixels, imageSize);
		} else {
			GLuint dstRowStride;
			if (image->mt) {
				r300_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
				dstRowStride = lvl->rowstride;
			} else {
				dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
			}
			if (!texImage->TexFormat->StoreImage(ctx, dims,
						texImage->_BaseFormat,
						texImage->TexFormat,
						texImage->Data, 0, 0, 0, /* dstX/Y/Zoffset */
						dstRowStride,
						texImage->ImageOffsets,
						width, height, depth,
						format, type, pixels, packing))
				_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
		}

		r300_teximage_unmap(image);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);

	/* SGIS_generate_mipmap */
	if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
		ctx->Driver.GenerateMipmap(ctx, texObj->Target, texObj);
	}
}


static GLuint face_for_target(GLenum target)
{
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	default:
		return 0;
	}
}


static void r300TexImage1D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	r300_teximage(ctx, 1, 0, level, internalFormat, width, 1, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

static void r300TexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	GLuint face = face_for_target(target);

	r300_teximage(ctx, 2, face, level, internalFormat, width, height, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

static void r300CompressedTexImage2D(GLcontext * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)
{
	GLuint face = face_for_target(target);

	r300_teximage(ctx, 2, face, level, internalFormat, width, height, 1,
		imageSize, 0, 0, data, 0, texObj, texImage, 1);
}

static void r300TexImage3D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint depth,
			   GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	r300_teximage(ctx, 3, 0, level, internalFormat, width, height, depth,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

/**
 * Update a subregion of the given texture image.
 */
static void r300_texsubimage(GLcontext* ctx, int dims, int level,
		GLint xoffset, GLint yoffset, GLint zoffset,
		GLsizei width, GLsizei height, GLsizei depth,
		GLenum format, GLenum type,
		const GLvoid * pixels,
		const struct gl_pixelstore_attrib *packing,
		struct gl_texture_object *texObj,
		struct gl_texture_image *texImage,
		int compressed)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300_texture_image* image = get_r300_texture_image(texImage);

	R300_FIREVERTICES(rmesa);

	pixels = _mesa_validate_pbo_teximage(ctx, dims,
		width, height, depth, format, type, pixels, packing, "glTexSubImage1D");

	if (pixels) {
		GLint dstRowStride;
		r300_teximage_map(image, GL_TRUE);

		if (image->mt) {
			r300_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
			dstRowStride = lvl->rowstride;
		} else {
			dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
		}

		if (!texImage->TexFormat->StoreImage(ctx, dims, texImage->_BaseFormat,
				texImage->TexFormat, texImage->Data,
				xoffset, yoffset, zoffset,
				dstRowStride,
				texImage->ImageOffsets,
				width, height, depth,
				format, type, pixels, packing))
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");

		r300_teximage_unmap(image);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);

	/* GL_SGIS_generate_mipmap */
	if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
		ctx->Driver.GenerateMipmap(ctx, texObj->Target, texObj);
	}
}

static void r300TexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset,
			      GLsizei width,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage)
{
	r300_texsubimage(ctx, 1, level, xoffset, 0, 0, width, 1, 1,
		format, type, pixels, packing, texObj, texImage, 0);
}

static void r300TexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint yoffset,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage)
{
	r300_texsubimage(ctx, 2, level, xoffset, yoffset, 0, width, height, 1,
		format, type, pixels, packing, texObj, texImage, 0);
}

static void r300CompressedTexSubImage2D(GLcontext * ctx, GLenum target,
					GLint level, GLint xoffset,
					GLint yoffset, GLsizei width,
					GLsizei height, GLenum format,
					GLsizei imageSize, const GLvoid * data,
					struct gl_texture_object *texObj,
					struct gl_texture_image *texImage)
{
	r300_texsubimage(ctx, 2, level, xoffset, yoffset, 0, width, height, 1,
		format, 0, data, 0, texObj, texImage, 1);
}

static void
r300TexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset, GLint yoffset, GLint zoffset,
		  GLsizei width, GLsizei height, GLsizei depth,
		  GLenum format, GLenum type,
		  const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
	r300_texsubimage(ctx, 3, level, xoffset, yoffset, zoffset, width, height, depth,
		format, type, pixels, packing, texObj, texImage, 0);
}


/**
 * Wraps Mesa's implementation to ensure that the base level image is mapped.
 *
 * This relies on internal details of _mesa_generate_mipmap, in particular
 * the fact that the memory for recreated texture images is always freed.
 */
static void r300_generate_mipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj)
{
	GLuint face = face_for_target(target);
	r300_texture_image *baseimage = get_r300_texture_image(texObj->Image[face][texObj->BaseLevel]);

	r300_teximage_map(baseimage, GL_FALSE);
	_mesa_generate_mipmap(ctx, target, texObj);
	r300_teximage_unmap(baseimage);
}



/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void r300TexParameter(GLcontext * ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     GLenum pname, const GLfloat * params)
{
	r300TexObj* t = r300_tex_obj(texObj);

	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(pname));
	}

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		r300SetTexFilter(t, texObj->MinFilter, texObj->MagFilter, texObj->MaxAnisotropy);
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
		r300UpdateTexWrap(t);
		break;

	case GL_TEXTURE_BORDER_COLOR:
		r300SetTexBorderColor(t, texObj->_BorderChan);
		break;

	case GL_TEXTURE_BASE_LEVEL:
	case GL_TEXTURE_MAX_LEVEL:
	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
		/* This isn't the most efficient solution but there doesn't appear to
		 * be a nice alternative.  Since there's no LOD clamping,
		 * we just have to rely on loading the right subset of mipmap levels
		 * to simulate a clamped LOD.
		 */
		if (t->mt) {
			r300_miptree_unreference(t->mt);
			t->mt = 0;
			t->validated = GL_FALSE;
		}
		break;

	case GL_DEPTH_TEXTURE_MODE:
		if (!texObj->Image[0][texObj->BaseLevel])
			return;
		if (texObj->Image[0][texObj->BaseLevel]->TexFormat->BaseFormat
		    == GL_DEPTH_COMPONENT) {
			r300SetDepthTexMode(texObj);
			break;
		} else {
			/* If the texture isn't a depth texture, changing this
			 * state won't cause any changes to the hardware.
			 * Don't force a flush of texture state.
			 */
			return;
		}

	default:
		return;
	}
}

static void r300DeleteTexture(GLcontext * ctx, struct gl_texture_object *texObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300TexObj* t = r300_tex_obj(texObj);

	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %p (target = %s) )\n", __FUNCTION__,
			(void *)texObj,
			_mesa_lookup_enum_by_nr(texObj->Target));
	}

	if (rmesa) {
		int i;
		R300_FIREVERTICES(rmesa);

		for(i = 0; i < R300_MAX_TEXTURE_UNITS; ++i)
			if (rmesa->hw.textures[i] == t)
				rmesa->hw.textures[i] = 0;
	}

	if (t->mt) {
		r300_miptree_unreference(t->mt);
		t->mt = 0;
	}
	_mesa_delete_texture_object(ctx, texObj);
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Fixup MaxAnisotropy according to user preference.
 */
static struct gl_texture_object *r300NewTextureObject(GLcontext * ctx,
						      GLuint name,
						      GLenum target)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300TexObj* t = CALLOC_STRUCT(r300_tex_obj);


	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %p (target = %s) )\n", __FUNCTION__,
			t, _mesa_lookup_enum_by_nr(target));
	}

	_mesa_initialize_texture_object(&t->base, name, target);
	t->base.MaxAnisotropy = rmesa->initialMaxAnisotropy;

	/* Initialize hardware state */
	r300UpdateTexWrap(t);
	r300SetTexFilter(t, t->base.MinFilter, t->base.MagFilter, t->base.MaxAnisotropy);
	r300SetTexBorderColor(t, t->base._BorderChan);

	return &t->base;
}

void r300InitTextureFuncs(struct dd_function_table *functions)
{
	/* Note: we only plug in the functions we implement in the driver
	 * since _mesa_init_driver_functions() was already called.
	 */
	functions->NewTextureImage = r300NewTextureImage;
	functions->FreeTexImageData = r300FreeTexImageData;
	functions->MapTexture = r300MapTexture;
	functions->UnmapTexture = r300UnmapTexture;

	functions->ChooseTextureFormat = r300ChooseTextureFormat;
	functions->TexImage1D = r300TexImage1D;
	functions->TexImage2D = r300TexImage2D;
	functions->TexImage3D = r300TexImage3D;
	functions->TexSubImage1D = r300TexSubImage1D;
	functions->TexSubImage2D = r300TexSubImage2D;
	functions->TexSubImage3D = r300TexSubImage3D;
	functions->NewTextureObject = r300NewTextureObject;
	functions->DeleteTexture = r300DeleteTexture;
	functions->IsTextureResident = driIsTextureResident;

	functions->TexParameter = r300TexParameter;

	functions->CompressedTexImage2D = r300CompressedTexImage2D;
	functions->CompressedTexSubImage2D = r300CompressedTexSubImage2D;

	functions->GenerateMipmap = r300_generate_mipmap;

	driInitTextureFormats();
}
