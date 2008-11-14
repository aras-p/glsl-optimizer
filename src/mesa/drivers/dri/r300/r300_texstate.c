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

**************************************************************************/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 *
 * \todo Enable R300 texture tiling code?
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/texformat.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/enums.h"

#include "r300_context.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "radeon_ioctl.h"
#include "r300_mipmap_tree.h"
#include "r300_tex.h"
#include "r300_reg.h"
#include "radeon_buffer.h"

#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_RGBA_DXT5			\
			   || ((f) >= MESA_FORMAT_RGBA_FLOAT32 &&	\
			       (f) <= MESA_FORMAT_INTENSITY_FLOAT16))	\
			  && tx_table[f].flag )

#define _ASSIGN(entry, format)				\
	[ MESA_FORMAT_ ## entry ] = { format, 0, 1}

/*
 * Note that the _REV formats are the same as the non-REV formats.  This is
 * because the REV and non-REV formats are identical as a byte string, but
 * differ when accessed as 16-bit or 32-bit words depending on the endianness of
 * the host.  Since the textures are transferred to the R300 as a byte string
 * (i.e. without any byte-swapping), the R300 sees the REV and non-REV formats
 * identically.  -- paulus
 */

static const struct tx_table {
	GLuint format, filter, flag;
} tx_table[] = {
	/* *INDENT-OFF* */
#ifdef MESA_LITTLE_ENDIAN
	_ASSIGN(RGBA8888, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	_ASSIGN(RGBA8888_REV, R300_EASY_TX_FORMAT(Z, Y, X, W, W8Z8Y8X8)),
	_ASSIGN(ARGB8888, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
	_ASSIGN(ARGB8888_REV, R300_EASY_TX_FORMAT(W, Z, Y, X, W8Z8Y8X8)),
#else
	_ASSIGN(RGBA8888, R300_EASY_TX_FORMAT(Z, Y, X, W, W8Z8Y8X8)),
	_ASSIGN(RGBA8888_REV, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	_ASSIGN(ARGB8888, R300_EASY_TX_FORMAT(W, Z, Y, X, W8Z8Y8X8)),
	_ASSIGN(ARGB8888_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
#endif
	_ASSIGN(RGB888, R300_EASY_TX_FORMAT(X, Y, Z, ONE, W8Z8Y8X8)),
	_ASSIGN(RGB565, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5)),
	_ASSIGN(RGB565_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5)),
	_ASSIGN(ARGB4444, R300_EASY_TX_FORMAT(X, Y, Z, W, W4Z4Y4X4)),
	_ASSIGN(ARGB4444_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W4Z4Y4X4)),
	_ASSIGN(ARGB1555, R300_EASY_TX_FORMAT(X, Y, Z, W, W1Z5Y5X5)),
	_ASSIGN(ARGB1555_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W1Z5Y5X5)),
	_ASSIGN(AL88, R300_EASY_TX_FORMAT(X, X, X, Y, Y8X8)),
	_ASSIGN(AL88_REV, R300_EASY_TX_FORMAT(X, X, X, Y, Y8X8)),
	_ASSIGN(RGB332, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z3Y3X2)),
	_ASSIGN(A8, R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X8)),
	_ASSIGN(L8, R300_EASY_TX_FORMAT(X, X, X, ONE, X8)),
	_ASSIGN(I8, R300_EASY_TX_FORMAT(X, X, X, X, X8)),
	_ASSIGN(CI8, R300_EASY_TX_FORMAT(X, X, X, X, X8)),
	_ASSIGN(YCBCR, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8) | R300_TX_FORMAT_YUV_MODE),
	_ASSIGN(YCBCR_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8) | R300_TX_FORMAT_YUV_MODE),
	_ASSIGN(RGB_DXT1, R300_EASY_TX_FORMAT(X, Y, Z, ONE, DXT1)),
	_ASSIGN(RGBA_DXT1, R300_EASY_TX_FORMAT(X, Y, Z, W, DXT1)),
	_ASSIGN(RGBA_DXT3, R300_EASY_TX_FORMAT(X, Y, Z, W, DXT3)),
	_ASSIGN(RGBA_DXT5, R300_EASY_TX_FORMAT(Y, Z, W, X, DXT5)),
	_ASSIGN(RGBA_FLOAT32, R300_EASY_TX_FORMAT(Z, Y, X, W, FL_R32G32B32A32)),
	_ASSIGN(RGBA_FLOAT16, R300_EASY_TX_FORMAT(Z, Y, X, W, FL_R16G16B16A16)),
	_ASSIGN(RGB_FLOAT32, 0xffffffff),
	_ASSIGN(RGB_FLOAT16, 0xffffffff),
	_ASSIGN(ALPHA_FLOAT32, R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, FL_I32)),
	_ASSIGN(ALPHA_FLOAT16, R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, FL_I16)),
	_ASSIGN(LUMINANCE_FLOAT32, R300_EASY_TX_FORMAT(X, X, X, ONE, FL_I32)),
	_ASSIGN(LUMINANCE_FLOAT16, R300_EASY_TX_FORMAT(X, X, X, ONE, FL_I16)),
	_ASSIGN(LUMINANCE_ALPHA_FLOAT32, R300_EASY_TX_FORMAT(X, X, X, Y, FL_I32A32)),
	_ASSIGN(LUMINANCE_ALPHA_FLOAT16, R300_EASY_TX_FORMAT(X, X, X, Y, FL_I16A16)),
	_ASSIGN(INTENSITY_FLOAT32, R300_EASY_TX_FORMAT(X, X, X, X, FL_I32)),
	_ASSIGN(INTENSITY_FLOAT16, R300_EASY_TX_FORMAT(X, X, X, X, FL_I16)),
	_ASSIGN(Z16, R300_EASY_TX_FORMAT(X, X, X, X, X16)),
	_ASSIGN(Z24_S8, R300_EASY_TX_FORMAT(X, X, X, X, X24_Y8)),
	_ASSIGN(Z32, R300_EASY_TX_FORMAT(X, X, X, X, X32)),
	/* *INDENT-ON* */
};

#undef _ASSIGN

void r300SetDepthTexMode(struct gl_texture_object *tObj)
{
	static const GLuint formats[3][3] = {
		{
			R300_EASY_TX_FORMAT(X, X, X, ONE, X16),
			R300_EASY_TX_FORMAT(X, X, X, X, X16),
			R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X16),
		},
		{
			R300_EASY_TX_FORMAT(X, X, X, ONE, X24_Y8),
			R300_EASY_TX_FORMAT(X, X, X, X, X24_Y8),
			R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X24_Y8),
		},
		{
			R300_EASY_TX_FORMAT(X, X, X, ONE, X32),
			R300_EASY_TX_FORMAT(X, X, X, X, X32),
			R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X32),
		},
	};
	const GLuint *format;
	r300TexObjPtr t;

	if (!tObj)
		return;

	t = r300_tex_obj(tObj);

	switch (tObj->Image[0][tObj->BaseLevel]->TexFormat->MesaFormat) {
	case MESA_FORMAT_Z16:
		format = formats[0];
		break;
	case MESA_FORMAT_Z24_S8:
		format = formats[1];
		break;
	case MESA_FORMAT_Z32:
		format = formats[2];
		break;
	default:
		/* Error...which should have already been caught by higher
		 * levels of Mesa.
		 */
		ASSERT(0);
		return;
	}

	switch (tObj->DepthMode) {
	case GL_LUMINANCE:
		t->format = format[0];
		break;
	case GL_INTENSITY:
		t->format = format[1];
		break;
	case GL_ALPHA:
		t->format = format[2];
		break;
	default:
		/* Error...which should have already been caught by higher
		 * levels of Mesa.
		 */
		ASSERT(0);
		return;
	}
}


/**
 * Compute the cached hardware register values for the given texture object.
 *
 * \param rmesa Context pointer
 * \param t the r300 texture object
 */
static void setup_hardware_state(r300ContextPtr rmesa, r300TexObj *t)
{
	const struct gl_texture_image *firstImage =
	    t->base.Image[0][t->mt->firstLevel];

	if (!t->image_override
	    && VALID_FORMAT(firstImage->TexFormat->MesaFormat)) {
		if (firstImage->TexFormat->BaseFormat == GL_DEPTH_COMPONENT) {
			r300SetDepthTexMode(&t->base);
		} else {
			t->format = tx_table[firstImage->TexFormat->MesaFormat].format;
		}

		t->filter |= tx_table[firstImage->TexFormat->MesaFormat].filter;
	} else if (!t->image_override) {
		_mesa_problem(NULL, "unexpected texture format in %s",
			      __FUNCTION__);
		return;
	}

	t->tile_bits = 0;

	if (t->base.Target == GL_TEXTURE_CUBE_MAP)
		t->format |= R300_TX_FORMAT_CUBIC_MAP;
	if (t->base.Target == GL_TEXTURE_3D)
		t->format |= R300_TX_FORMAT_3D;

	t->size = (((firstImage->Width - 1) << R300_TX_WIDTHMASK_SHIFT)
		| ((firstImage->Height - 1) << R300_TX_HEIGHTMASK_SHIFT))
		| ((t->mt->lastLevel - t->mt->firstLevel) << R300_TX_MAX_MIP_LEVEL_SHIFT);

	if (t->base.Target == GL_TEXTURE_RECTANGLE_NV) {
		unsigned int align = (64 / t->mt->bpp) - 1;
		t->size |= R300_TX_SIZE_TXPITCH_EN;
		if (!t->image_override)
			t->pitch_reg = ((firstImage->Width + align) & ~align) - 1;
	}

	if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
	    if (firstImage->Width > 2048)
		t->pitch_reg |= R500_TXWIDTH_BIT11;
	    if (firstImage->Height > 2048)
		t->pitch_reg |= R500_TXHEIGHT_BIT11;
	}
}


static void copy_rows(void* dst, GLuint dststride, const void* src, GLuint srcstride,
	GLuint numrows, GLuint rowsize)
{
	assert(rowsize <= dststride);
	assert(rowsize <= srcstride);

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


/**
 * Ensure that the given image is stored in the given miptree from now on.
 */
static void migrate_image_to_miptree(r300_mipmap_tree *mt, r300_texture_image *image, int face, int level)
{
	r300_mipmap_level *dstlvl = &mt->levels[level - mt->firstLevel];
	unsigned char *dest;

	assert(image->mt != mt);
	assert(dstlvl->width == image->base.Width);
	assert(dstlvl->height == image->base.Height);
	assert(dstlvl->depth == image->base.Depth);

	radeon_bo_map(mt->bo, GL_TRUE);
	dest = mt->bo->ptr + dstlvl->faces[face].offset;

	if (image->mt) {
		/* Format etc. should match, so we really just need a memcpy().
		 * In fact, that memcpy() could be done by the hardware in many
		 * cases, provided that we have a proper memory manager.
		 */
		r300_mipmap_level *srclvl = &image->mt->levels[image->mtlevel];

		assert(srclvl->size == dstlvl->size);
		assert(srclvl->rowstride == dstlvl->rowstride);

		radeon_bo_map(image->mt->bo, GL_FALSE);
		memcpy(dest,
			image->mt->bo->ptr + srclvl->faces[face].offset,
			dstlvl->size);
		radeon_bo_unmap(image->mt->bo);

		r300_miptree_unreference(image->mt);
	} else {
		uint srcrowstride = image->base.Width * image->base.TexFormat->TexelBytes;

		if (mt->tilebits)
			WARN_ONCE("%s: tiling not supported yet", __FUNCTION__);

		copy_rows(dest, dstlvl->rowstride, image->base.Data, srcrowstride,
			image->base.Height * image->base.Depth, srcrowstride);

		_mesa_free_texmemory(image->base.Data);
		image->base.Data = 0;
	}

	radeon_bo_unmap(mt->bo);

	image->mt = mt;
	image->mtface = face;
	image->mtlevel = level;
	r300_miptree_reference(image->mt);
}


/**
 * Ensure the given texture is ready for rendering.
 *
 * Mostly this means populating the texture object's mipmap tree.
 */
static GLboolean r300_validate_texture(GLcontext * ctx, struct gl_texture_object *texObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300TexObj *t = r300_tex_obj(texObj);
	r300_texture_image *baseimage = get_r300_texture_image(texObj->Image[0][texObj->BaseLevel]);
	int face, level;

	if (t->validated || t->image_override)
		return GL_TRUE;

	if (RADEON_DEBUG & DEBUG_TEXTURE)
		fprintf(stderr, "%s: Validating texture %p now\n", __FUNCTION__, texObj);

	if (baseimage->base.Border > 0)
		return GL_FALSE;

	/* Ensure a matching miptree exists.
	 *
	 * Differing mipmap trees can result when the app uses TexImage to
	 * change texture dimensions.
	 *
	 * Prefer to use base image's miptree if it
	 * exists, since that most likely contains more valid data (remember
	 * that the base level is usually significantly larger than the rest
	 * of the miptree, so cubemaps are the only possible exception).
	 */
	if (baseimage->mt &&
	    baseimage->mt != t->mt &&
	    r300_miptree_matches_texture(baseimage->mt, &t->base)) {
		r300_miptree_unreference(t->mt);
		t->mt = baseimage->mt;
		r300_miptree_reference(t->mt);
	} else if (t->mt && !r300_miptree_matches_texture(t->mt, &t->base)) {
		r300_miptree_unreference(t->mt);
		t->mt = 0;
	}

	if (!t->mt) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, " Allocate new miptree\n");
		r300_try_alloc_miptree(rmesa, t, &baseimage->base, 0, texObj->BaseLevel);
		if (!t->mt) {
			_mesa_problem(ctx, "r300_validate_texture failed to alloc miptree");
			return GL_FALSE;
		}
	}

	/* Ensure all images are stored in the single main miptree */
	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level) {
			r300_texture_image *image = get_r300_texture_image(texObj->Image[face][level]);
			if (RADEON_DEBUG & DEBUG_TEXTURE)
				fprintf(stderr, " face %i, level %i... ", face, level);
			if (t->mt == image->mt) {
				if (RADEON_DEBUG & DEBUG_TEXTURE)
					fprintf(stderr, "OK\n");
				continue;
			}

			if (RADEON_DEBUG & DEBUG_TEXTURE)
				fprintf(stderr, "migrating\n");
			migrate_image_to_miptree(t->mt, image, face, level);
		}
	}

	/* Configure the hardware registers (more precisely, the cached version
	 * of the hardware registers). */
	setup_hardware_state(rmesa, t);

	t->validated = GL_TRUE;
	return GL_TRUE;
}


/**
 * Ensure all enabled and complete textures are uploaded.
 */
void r300ValidateTextures(GLcontext * ctx)
{
	int i;

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; ++i) {
		if (!ctx->Texture.Unit[i]._ReallyEnabled)
			continue;

		if (!r300_validate_texture(ctx, ctx->Texture.Unit[i]._Current)) {
			_mesa_warning(ctx,
				      "failed to validate texture for unit %d.\n",
				      i);
		}
	}
}

void r300SetTexOffset(__DRIcontext * pDRICtx, GLint texname,
		      unsigned long long offset, GLint depth, GLuint pitch)
{
	r300ContextPtr rmesa = pDRICtx->driverPrivate;
	struct gl_texture_object *tObj =
	    _mesa_lookup_texture(rmesa->radeon.glCtx, texname);
	r300TexObjPtr t = r300_tex_obj(tObj);
	uint32_t pitch_val;

	if (!tObj)
		return;

	t->image_override = GL_TRUE;

	if (!offset)
		return;
    t->bo = NULL;
	t->override_offset = offset;
	t->pitch_reg &= (1 << 13) -1;
	pitch_val = pitch;

	switch (depth) {
	case 32:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8);
		t->filter |= tx_table[2].filter;
		pitch_val /= 4;
		break;
	case 24:
	default:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, ONE, W8Z8Y8X8);
		t->filter |= tx_table[4].filter;
		pitch_val /= 4;
		break;
	case 16:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5);
		t->filter |= tx_table[5].filter;
		pitch_val /= 2;
		break;
	}
	pitch_val--;

	t->pitch_reg |= pitch_val;
}

void r300SetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
    struct gl_texture_unit *texUnit;
    struct gl_texture_object *texObj;
    struct gl_texture_image *texImage;
	struct radeon_renderbuffer *rb;
	r300_texture_image *rImage;
	radeonContextPtr radeon;
	r300ContextPtr rmesa;
	GLframebuffer *fb;
	r300TexObjPtr t;
	uint32_t pitch_val;

    target = GL_TEXTURE_RECTANGLE_ARB;
	radeon = pDRICtx->driverPrivate;
	rmesa = pDRICtx->driverPrivate;
	fb = dPriv->driverPrivate;
    texUnit = &radeon->glCtx->Texture.Unit[radeon->glCtx->Texture.CurrentUnit];
    texObj = _mesa_select_tex_object(radeon->glCtx, texUnit, target);
    texImage = _mesa_get_tex_image(radeon->glCtx, texObj, target, 0);
	rImage = get_r300_texture_image(texImage);
	t = r300_tex_obj(texObj);
    if (t == NULL) {
        return;
    }

    radeon_update_renderbuffers(pDRICtx, dPriv);
    /* back & depth buffer are useless free them right away */
    rb = (void*)fb->Attachment[BUFFER_DEPTH].Renderbuffer;
    if (rb && rb->bo) {
        radeon_bo_unref(rb->bo);
        rb->bo = NULL;
    }
    rb = (void*)fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
    if (rb && rb->bo) {
        radeon_bo_unref(rb->bo);
        rb->bo = NULL;
    }
    rb = (void*)fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
    if (rb->bo == NULL) {
        /* Failed to BO for the buffer */
        return;
    }

    _mesa_lock_texture(radeon->glCtx, texObj);
    if (t->bo) {
        t->bo = NULL;
    }
    if (t->mt) {
        t->mt = NULL;
    }
    if (rImage->bo) {
        radeon_bo_unref(rImage->bo);
        rImage->bo = NULL;
    }
    if (rImage->mt) {
        r300_miptree_unreference(rImage->mt);
        rImage->mt = NULL;
    }
    _mesa_init_teximage_fields(radeon->glCtx, target, texImage,
                               rb->width, rb->height, rb->cpp, 0, rb->cpp);
	texImage->TexFormat = &_mesa_texformat_rgba8888_rev;
    rImage->bo = rb->bo;

    t->bo = rb->bo;
    t->tile_bits = 0;
	t->image_override = GL_TRUE;
	t->override_offset = 0;
	t->pitch_reg &= (1 << 13) -1;
	pitch_val = rb->pitch;
	switch (rb->cpp) {
	case 4:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8);
		t->filter |= tx_table[2].filter;
		pitch_val /= 4;
		break;
	case 3:
	default:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, ONE, W8Z8Y8X8);
		t->filter |= tx_table[4].filter;
		pitch_val /= 4;
		break;
	case 2:
		t->format = R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5);
		t->filter |= tx_table[5].filter;
		pitch_val /= 2;
		break;
	}
	pitch_val--;
	t->size = ((rb->width - 1) << R300_TX_WIDTHMASK_SHIFT) |
              ((rb->height - 1) << R300_TX_HEIGHTMASK_SHIFT);
    t->size |= R300_TX_SIZE_TXPITCH_EN;
	t->pitch_reg |= pitch_val;
	t->validated = GL_TRUE;
    _mesa_unlock_texture(radeon->glCtx, texObj);
    return;
}
