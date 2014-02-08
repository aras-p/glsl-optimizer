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

static void teximage_assign_miptree(radeonContextPtr rmesa,
				    struct gl_texture_object *texObj,
				    struct gl_texture_image *texImage);

static radeon_mipmap_tree *radeon_miptree_create_for_teximage(radeonContextPtr rmesa,
							      struct gl_texture_object *texObj,
							      struct gl_texture_image *texImage);

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
	return calloc(1, sizeof(radeon_texture_image));
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

static GLboolean
radeonAllocTextureImageBuffer(struct gl_context *ctx,
			      struct gl_texture_image *timage)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	struct gl_texture_object *texobj = timage->TexObject;

	ctx->Driver.FreeTextureImageBuffer(ctx, timage);

	if (!_swrast_init_texture_image(timage))
		return GL_FALSE;

	teximage_assign_miptree(rmesa, texobj, timage);
				
	return GL_TRUE;
}


/**
 * Free memory associated with this texture image.
 */
void radeonFreeTextureImageBuffer(struct gl_context *ctx, struct gl_texture_image *timage)
{
	radeon_texture_image* image = get_radeon_texture_image(timage);

	if (image->mt) {
		radeon_miptree_unreference(&image->mt);
	}
	if (image->bo) {
		radeon_bo_unref(image->bo);
		image->bo = NULL;
	}

        _swrast_free_texture_image_buffer(ctx, timage);
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
	texel_size /= bw;

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
		void *base;
		radeon_mipmap_level *lvl = &image->mt->levels[texImage->Level];
		       
		radeon_bo_map(mt->bo, write);
		base = mt->bo->ptr + lvl->faces[image->base.Base.Face].offset;

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
static mesa_format radeonChoose8888TexFormat(radeonContextPtr rmesa,
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
		return MESA_FORMAT_A8B8G8R8_UNORM;
	} else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
		   (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) {
		return MESA_FORMAT_R8G8B8A8_UNORM;
	} else
		return _radeon_texformat_argb8888;
#endif
}

mesa_format radeonChooseTextureFormat_mesa(struct gl_context * ctx,
					 GLenum target,
					 GLint internalFormat,
					 GLenum format,
					 GLenum type)
{
	return radeonChooseTextureFormat(ctx, internalFormat, format,
					 type, 0);
}

mesa_format radeonChooseTextureFormat(struct gl_context * ctx,
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
		return MESA_FORMAT_A_UNORM8;
#endif
	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_COMPRESSED_LUMINANCE:
		return MESA_FORMAT_L_UNORM8;

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
		return MESA_FORMAT_I_UNORM8;

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
		return MESA_FORMAT_A_FLOAT16;
	case GL_ALPHA32F_ARB:
		return MESA_FORMAT_A_FLOAT32;
	case GL_LUMINANCE16F_ARB:
		return MESA_FORMAT_L_FLOAT16;
	case GL_LUMINANCE32F_ARB:
		return MESA_FORMAT_L_FLOAT32;
	case GL_LUMINANCE_ALPHA16F_ARB:
		return MESA_FORMAT_LA_FLOAT16;
	case GL_LUMINANCE_ALPHA32F_ARB:
		return MESA_FORMAT_LA_FLOAT32;
	case GL_INTENSITY16F_ARB:
		return MESA_FORMAT_I_FLOAT16;
	case GL_INTENSITY32F_ARB:
		return MESA_FORMAT_I_FLOAT32;
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
		return MESA_FORMAT_Z24_UNORM_S8_UINT;

	/* EXT_texture_sRGB */
	case GL_SRGB:
	case GL_SRGB8:
	case GL_SRGB_ALPHA:
	case GL_SRGB8_ALPHA8:
	case GL_COMPRESSED_SRGB:
	case GL_COMPRESSED_SRGB_ALPHA:
		return MESA_FORMAT_B8G8R8A8_SRGB;

	case GL_SLUMINANCE:
	case GL_SLUMINANCE8:
	case GL_COMPRESSED_SLUMINANCE:
		return MESA_FORMAT_L_SRGB8;

	case GL_SLUMINANCE_ALPHA:
	case GL_SLUMINANCE8_ALPHA8:
	case GL_COMPRESSED_SLUMINANCE_ALPHA:
      return MESA_FORMAT_L8A8_SRGB;

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
static void teximage_assign_miptree(radeonContextPtr rmesa,
				    struct gl_texture_object *texObj,
				    struct gl_texture_image *texImage)
{
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	/* Try using current miptree, or create new if there isn't any */
	if (!t->mt || !radeon_miptree_matches_image(t->mt, texImage)) {
		radeon_miptree_unreference(&t->mt);
		t->mt = radeon_miptree_create_for_teximage(rmesa,
							   texObj,
							   texImage);

		radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			     "%s: texObj %p, texImage %p, "
				"texObj miptree doesn't match, allocated new miptree %p\n",
				__FUNCTION__, texObj, texImage, t->mt);
	}

	/* Miptree alocation may have failed,
	 * when there was no image for baselevel specified */
	if (t->mt) {
		radeon_miptree_reference(t->mt, &image->mt);
	} else
		radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
				"%s Failed to allocate miptree.\n", __func__);
}

unsigned radeonIsFormatRenderable(mesa_format mesa_format)
{
	if (mesa_format == _radeon_texformat_argb8888 || mesa_format == _radeon_texformat_rgb565 ||
		mesa_format == _radeon_texformat_argb1555 || mesa_format == _radeon_texformat_argb4444)
		return 1;

	switch (mesa_format)
	{
		case MESA_FORMAT_Z_UNORM16:
		case MESA_FORMAT_Z24_UNORM_S8_UINT:
			return 1;
		default:
			return 0;
	}
}

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

	if (!radeon_miptree_matches_image(t->mt, &radeonImage->base.Base))
		fprintf(stderr, "miptree doesn't match image\n");
}

mesa_format _radeon_texformat_rgba8888 = MESA_FORMAT_NONE;
mesa_format _radeon_texformat_argb8888 = MESA_FORMAT_NONE;
mesa_format _radeon_texformat_rgb565 = MESA_FORMAT_NONE;
mesa_format _radeon_texformat_argb4444 = MESA_FORMAT_NONE;
mesa_format _radeon_texformat_argb1555 = MESA_FORMAT_NONE;
mesa_format _radeon_texformat_al88 = MESA_FORMAT_NONE;
/*@}*/


static void
radeonInitTextureFormats(void)
{
   if (_mesa_little_endian()) {
      _radeon_texformat_rgba8888	= MESA_FORMAT_A8B8G8R8_UNORM;
      _radeon_texformat_argb8888	= MESA_FORMAT_B8G8R8A8_UNORM;
      _radeon_texformat_rgb565		= MESA_FORMAT_B5G6R5_UNORM;
      _radeon_texformat_argb4444	= MESA_FORMAT_B4G4R4A4_UNORM;
      _radeon_texformat_argb1555	= MESA_FORMAT_B5G5R5A1_UNORM;
      _radeon_texformat_al88		= MESA_FORMAT_L8A8_UNORM;
   }
   else {
      _radeon_texformat_rgba8888	= MESA_FORMAT_R8G8B8A8_UNORM;
      _radeon_texformat_argb8888	= MESA_FORMAT_A8R8G8B8_UNORM;
      _radeon_texformat_rgb565		= MESA_FORMAT_R5G6B5_UNORM;
      _radeon_texformat_argb4444	= MESA_FORMAT_A4R4G4B4_UNORM;
      _radeon_texformat_argb1555	= MESA_FORMAT_A1R5G5B5_UNORM;
      _radeon_texformat_al88		= MESA_FORMAT_A8L8_UNORM;
   }
}

void
radeon_init_common_texture_funcs(radeonContextPtr radeon,
				 struct dd_function_table *functions)
{
	functions->NewTextureImage = radeonNewTextureImage;
	functions->DeleteTextureImage = radeonDeleteTextureImage;
	functions->AllocTextureImageBuffer = radeonAllocTextureImageBuffer;
	functions->FreeTextureImageBuffer = radeonFreeTextureImageBuffer;
	functions->MapTextureImage = radeon_map_texture_image;
	functions->UnmapTextureImage = radeon_unmap_texture_image;

	functions->ChooseTextureFormat	= radeonChooseTextureFormat_mesa;

	functions->CopyTexSubImage = radeonCopyTexSubImage;

	functions->Bitmap = _mesa_meta_Bitmap;
	functions->EGLImageTargetTexture2D = radeon_image_target_texture_2d;

	radeonInitTextureFormats();
}

static radeon_mipmap_tree *radeon_miptree_create_for_teximage(radeonContextPtr rmesa,
						       struct gl_texture_object *texObj,
						       struct gl_texture_image *texImage)
{
	radeonTexObj *t = radeon_tex_obj(texObj);
	GLuint firstLevel;
	GLuint lastLevel;
	int width, height, depth;
	int i;

	width = texImage->Width;
	height = texImage->Height;
	depth = texImage->Depth;

	if (texImage->Level > texObj->BaseLevel &&
	    (width == 1 ||
	     (texObj->Target != GL_TEXTURE_1D && height == 1) ||
	     (texObj->Target == GL_TEXTURE_3D && depth == 1))) {
		/* For this combination, we're at some lower mipmap level and
		 * some important dimension is 1.  We can't extrapolate up to a
		 * likely base level width/height/depth for a full mipmap stack
		 * from this info, so just allocate this one level.
		 */
		firstLevel = texImage->Level;
		lastLevel = texImage->Level;
	} else {
		if (texImage->Level < texObj->BaseLevel)
			firstLevel = 0;
		else
			firstLevel = texObj->BaseLevel;

		for (i = texImage->Level; i > firstLevel; i--) {
			width <<= 1;
			if (height != 1)
				height <<= 1;
			if (depth != 1)
				depth <<= 1;
		}
		if ((texObj->Sampler.MinFilter == GL_NEAREST ||
		     texObj->Sampler.MinFilter == GL_LINEAR) &&
		    texImage->Level == firstLevel) {
			lastLevel = firstLevel;
		} else {
			lastLevel = firstLevel + _mesa_logbase2(MAX2(MAX2(width, height), depth));
		}
	}

	return  radeon_miptree_create(rmesa, texObj->Target,
				      texImage->TexFormat, firstLevel, lastLevel - firstLevel + 1,
				      width, height, depth, 
				      t->tile_bits);
}				     
