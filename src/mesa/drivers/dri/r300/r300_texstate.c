/* $XFree86: xc/lib/GL/mesa/src/drv/r300/r300_texstate.c,v 1.3 2003/02/15 22:18:47 dawes Exp $ */
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

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "macros.h"
#include "texformat.h"
#include "enums.h"

#include "r300_context.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "radeon_ioctl.h"
#include "r300_tex.h"
#include "r300_reg.h"


#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_RGBA_DXT5			\
			   || ((f) >= MESA_FORMAT_RGBA_FLOAT32 &&	\
			       (f) <= MESA_FORMAT_INTENSITY_FLOAT16))	\
			  && tx_table_le[f].flag )

#define _ASSIGN(entry, format)				\
	[ MESA_FORMAT_ ## entry ] = { format, 0, 1}

static const struct {
	GLuint format, filter, flag;
} tx_table_be[] = {
	/*
	 * Note that the _REV formats are the same as the non-REV formats.
	 * This is because the REV and non-REV formats are identical as a
	 * byte string, but differ when accessed as 16-bit or 32-bit words
	 * depending on the endianness of the host.  Since the textures are
	 * transferred to the R300 as a byte string (i.e. without any
	 * byte-swapping), the R300 sees the REV and non-REV formats
	 * identically.  -- paulus
	 */
	    _ASSIGN(RGBA8888, R300_EASY_TX_FORMAT(Z, Y, X, W, W8Z8Y8X8)),
	    _ASSIGN(RGBA8888_REV, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888, R300_EASY_TX_FORMAT(W, Z, Y, X, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
	    _ASSIGN(RGB888, 0xffffffff),
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
	    _ASSIGN(YCBCR, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE ),
	    _ASSIGN(YCBCR_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE),
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
	    };

static const struct {
	GLuint format, filter, flag;
} tx_table_le[] = {
	    _ASSIGN(RGBA8888, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	    _ASSIGN(RGBA8888_REV, R300_EASY_TX_FORMAT(Z, Y, X, W, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888_REV, R300_EASY_TX_FORMAT(W, Z, Y, X, W8Z8Y8X8)),
	    _ASSIGN(RGB888, 0xffffffff),
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
	    _ASSIGN(YCBCR, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE ),
	    _ASSIGN(YCBCR_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE),
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
	    };

#undef _ASSIGN


/**
 * This function computes the number of bytes of storage needed for
 * the given texture object (all mipmap levels, all cube faces).
 * The \c image[face][level].x/y/width/height parameters for upload/blitting
 * are computed here.  \c filter, \c format, etc. will be set here
 * too.
 *
 * \param rmesa Context pointer
 * \param tObj GL texture object whose images are to be posted to
 *                 hardware state.
 */
static void r300SetTexImages(r300ContextPtr rmesa,
			     struct gl_texture_object *tObj)
{
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;
	const struct gl_texture_image *baseImage =
	    tObj->Image[0][tObj->BaseLevel];
	GLint curOffset, blitWidth;
	GLint i, texelBytes;
	GLint numLevels;
	GLint log2Width, log2Height, log2Depth;

	/* Set the hardware texture format
	 */
	if (VALID_FORMAT(baseImage->TexFormat->MesaFormat)) {
		if (_mesa_little_endian()) {
			t->format =
			    tx_table_le[baseImage->TexFormat->MesaFormat].format;
			t->filter |=
			    tx_table_le[baseImage->TexFormat->MesaFormat].filter;
		} else {
			t->format =
			    tx_table_be[baseImage->TexFormat->MesaFormat].format;
			t->filter |=
			    tx_table_be[baseImage->TexFormat->MesaFormat].filter;
		}
	} else {
		_mesa_problem(NULL, "unexpected texture format in %s",
			      __FUNCTION__);
		return;
	}

	texelBytes = baseImage->TexFormat->TexelBytes;

	/* Compute which mipmap levels we really want to send to the hardware.
	 */
	driCalculateTextureFirstLastLevel((driTextureObject *) t);
	log2Width = tObj->Image[0][t->base.firstLevel]->WidthLog2;
	log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;
	log2Depth = tObj->Image[0][t->base.firstLevel]->DepthLog2;

	numLevels = t->base.lastLevel - t->base.firstLevel + 1;

	assert(numLevels <= RADEON_MAX_TEXTURE_LEVELS);

	/* Calculate mipmap offsets and dimensions for blitting (uploading)
	 * The idea is that we lay out the mipmap levels within a block of
	 * memory organized as a rectangle of width BLIT_WIDTH_BYTES.
	 */
	curOffset = 0;
	blitWidth = R300_BLIT_WIDTH_BYTES;
	t->tile_bits = 0;

	/* figure out if this texture is suitable for tiling. */
#if 0 /* Disabled for now */
	if (texelBytes) {
		if (rmesa->texmicrotile  && (tObj->Target != GL_TEXTURE_RECTANGLE_NV) &&
		   /* texrect might be able to use micro tiling too in theory? */
		   (baseImage->Height > 1)) {
			
			/* allow 32 (bytes) x 1 mip (which will use two times the space
			   the non-tiled version would use) max if base texture is large enough */
			if ((numLevels == 1) ||
				(((baseImage->Width * texelBytes / baseImage->Height) <= 32) &&
				(baseImage->Width * texelBytes > 64)) ||
				((baseImage->Width * texelBytes / baseImage->Height) <= 16)) {
				t->tile_bits |= R300_TXO_MICRO_TILE;
			}
		}
		
		if (tObj->Target != GL_TEXTURE_RECTANGLE_NV) {
			/* we can set macro tiling even for small textures, they will be untiled anyway */
			t->tile_bits |= R300_TXO_MACRO_TILE;
		}
	}
#endif

	for (i = 0; i < numLevels; i++) {
	  const struct gl_texture_image *texImage;
	  GLuint size;
	  
	  texImage = tObj->Image[0][i + t->base.firstLevel];
	  if (!texImage)
	    break;
	  
	  /* find image size in bytes */
	  if (texImage->IsCompressed) {
	    if ((t->format & R300_TX_FORMAT_DXT1) == R300_TX_FORMAT_DXT1) {
	      // fprintf(stderr,"DXT 1 %d %08X\n", texImage->Width, t->format);
	      if ((texImage->Width + 3) < 8) /* width one block */
		size = texImage->CompressedSize * 4;
	      else if ((texImage->Width + 3) < 16)
		size = texImage->CompressedSize * 2;
	      else size = texImage->CompressedSize;
	    }
	    else /* DXT3/5, 16 bytes per block */
	    {
	      WARN_ONCE("DXT 3/5 suffers from multitexturing problems!\n");
	      // fprintf(stderr,"DXT 3/5 %d\n", texImage->Width);
	      if ((texImage->Width + 3) < 8)
		size = texImage->CompressedSize * 2;
	      else size = texImage->CompressedSize;
	    }
	    
	  } else if (tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
	    size = ((texImage->Width * texelBytes + 63) & ~63) * texImage->Height;
	    blitWidth = 64 / texelBytes;
	  } else if (t->tile_bits & R300_TXO_MICRO_TILE) {
		/* tile pattern is 16 bytes x2. mipmaps stay 32 byte aligned,
		   though the actual offset may be different (if texture is less than
		   32 bytes width) to the untiled case */
		int w = (texImage->Width * texelBytes * 2 + 31) & ~31;
		size = (w * ((texImage->Height + 1) / 2)) * texImage->Depth;
		blitWidth = MAX2(texImage->Width, 64 / texelBytes);
	  } else {
	    int w = (texImage->Width * texelBytes + 31) & ~31;
	    size = w * texImage->Height * texImage->Depth;
	    blitWidth = MAX2(texImage->Width, 64 / texelBytes);
	  }
	  assert(size > 0);
	  
	  if(0)
	    fprintf(stderr, "w=%d h=%d d=%d tb=%d intFormat=%d\n", texImage->Width, texImage->Height,
		    texImage->Depth, texImage->TexFormat->TexelBytes,
		    texImage->InternalFormat);
	  
	  /* Align to 32-byte offset.  It is faster to do this unconditionally
	   * (no branch penalty).
	   */
	  
	  curOffset = (curOffset + 0x1f) & ~0x1f;
	  
	  if (texelBytes) {
	    t->image[0][i].x = curOffset; /* fix x and y coords up later together with offset */
	    t->image[0][i].y = 0;
	    t->image[0][i].width = MIN2(size / texelBytes, blitWidth);
	    t->image[0][i].height = (size / texelBytes) / t->image[0][i].width;
	  } else {
	    t->image[0][i].x = curOffset % R300_BLIT_WIDTH_BYTES;
	    t->image[0][i].y = curOffset / R300_BLIT_WIDTH_BYTES;
	    t->image[0][i].width = MIN2(size, R300_BLIT_WIDTH_BYTES);
	    t->image[0][i].height = size / t->image[0][i].width;
	  }
#if 0
	  /* for debugging only and only  applicable to non-rectangle targets */
	  assert(size % t->image[0][i].width == 0);
	  assert(t->image[0][i].x == 0
		 || (size < R300_BLIT_WIDTH_BYTES
		     && t->image[0][i].height == 1));
#endif
	  
	  if (0)
	    fprintf(stderr,
		    "level %d: %dx%d x=%d y=%d w=%d h=%d size=%d at %d\n",
		    i, texImage->Width, texImage->Height,
		    t->image[0][i].x, t->image[0][i].y,
		    t->image[0][i].width, t->image[0][i].height,
		    size, curOffset);
	  
	  curOffset += size;
	  
	}
	
	/* Align the total size of texture memory block.
	 */
	t->base.totalSize =
	    (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

	/* Setup remaining cube face blits, if needed */
	if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
		GLuint face;
		for (face = 1; face < 6; face++) {
			for (i = 0; i < numLevels; i++) {
				t->image[face][i].x = t->image[0][i].x;
				t->image[face][i].y = t->image[0][i].y;
				t->image[face][i].width = t->image[0][i].width;
				t->image[face][i].height =
				    t->image[0][i].height;
			}
		}
		t->base.totalSize *= 6;	/* total texmem needed */
	}

	/* Hardware state:
	 */
#if 0
	t->format &= ~(R200_TXFORMAT_WIDTH_MASK |
			    R200_TXFORMAT_HEIGHT_MASK |
			    R200_TXFORMAT_CUBIC_MAP_ENABLE |
			    R200_TXFORMAT_F5_WIDTH_MASK |
			    R200_TXFORMAT_F5_HEIGHT_MASK);
	t->format |= ((log2Width << R200_TXFORMAT_WIDTH_SHIFT) |
			   (log2Height << R200_TXFORMAT_HEIGHT_SHIFT));
#endif
#if 0
	t->format_x &= ~(R200_DEPTH_LOG2_MASK | R200_TEXCOORD_MASK);
	if (tObj->Target == GL_TEXTURE_3D) {
		t->format_x |= (log2Depth << R200_DEPTH_LOG2_SHIFT);
		t->format_x |= R200_TEXCOORD_VOLUME;
	} else if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
		ASSERT(log2Width == log2Height);
		t->format |= R300_TX_FORMAT_CUBIC_MAP;
		
		t->format_x |= R200_TEXCOORD_CUBIC_ENV;
		t->pp_cubic_faces = ((log2Width << R200_FACE_WIDTH_1_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_1_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_2_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_2_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_3_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_3_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_4_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_4_SHIFT));
	}
#endif
	if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
		ASSERT(log2Width == log2Height);
		t->format |= R300_TX_FORMAT_CUBIC_MAP;
	}
	
	t->size = (((tObj->Image[0][t->base.firstLevel]->Width - 1) << R300_TX_WIDTHMASK_SHIFT)
			|((tObj->Image[0][t->base.firstLevel]->Height - 1) << R300_TX_HEIGHTMASK_SHIFT))
			|((numLevels - 1) << R300_TX_MAX_MIP_LEVEL_SHIFT);

	/* Only need to round to nearest 32 for textures, but the blitter
	 * requires 64-byte aligned pitches, and we may/may not need the
	 * blitter.   NPOT only!
	 */
	if (baseImage->IsCompressed) {
		t->pitch =
		    (tObj->Image[0][t->base.firstLevel]->Width + 63) & ~(63);
	}
	else if (tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
		unsigned int align = blitWidth - 1;
		t->pitch = ((tObj->Image[0][t->base.firstLevel]->Width *
		      texelBytes) + 63) & ~(63);
		t->size |= R300_TX_SIZE_TXPITCH_EN;
		t->pitch_reg = (((tObj->Image[0][t->base.firstLevel]->Width) + align) & ~align) - 1;
	}
	else {
		t->pitch =
		    ((tObj->Image[0][t->base.firstLevel]->Width *
		      texelBytes) + 63) & ~(63);
	}

	t->dirty_state = TEX_ALL;

	/* FYI: r300UploadTexImages( rmesa, t ) used to be called here */
}


/* ================================================================
 * Texture unit state management
 */

static GLboolean enable_tex_2d(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	ASSERT(tObj->Target == GL_TEXTURE_2D || tObj->Target == GL_TEXTURE_1D);

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock)
			return GL_FALSE;
	}

	return GL_TRUE;
}

#if ENABLE_HW_3D_TEXTURE
static GLboolean enable_tex_3d(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	/* Need to load the 3d images associated with this unit.
	 */
#if 0
	if (t->format & R200_TXFORMAT_NON_POWER2) {
		t->format &= ~R200_TXFORMAT_NON_POWER2;
		t->base.dirty_images[0] = ~0;
	}
#endif
	ASSERT(tObj->Target == GL_TEXTURE_3D);

	/* R100 & R200 do not support mipmaps for 3D textures.
	 */
	if ((tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR)) {
		return GL_FALSE;
	}

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock)
			return GL_FALSE;
	}

	return GL_TRUE;
}
#endif

static GLboolean enable_tex_cube(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;
	GLuint face;

	/* Need to load the 2d images associated with this unit.
	 */
#if 0
	if (t->format & R200_TXFORMAT_NON_POWER2) {
		t->format &= ~R200_TXFORMAT_NON_POWER2;
		for (face = 0; face < 6; face++)
			t->base.dirty_images[face] = ~0;
	}
#endif
	ASSERT(tObj->Target == GL_TEXTURE_CUBE_MAP);

	if (t->base.dirty_images[0] || t->base.dirty_images[1] ||
	    t->base.dirty_images[2] || t->base.dirty_images[3] ||
	    t->base.dirty_images[4] || t->base.dirty_images[5]) {
		/* flush */
		R300_FIREVERTICES(rmesa);
		/* layout memory space, once for all faces */
		r300SetTexImages(rmesa, tObj);
	}

	/* upload (per face) */
	for (face = 0; face < 6; face++) {
		if (t->base.dirty_images[face]) {
			r300UploadTexImages(rmesa,
					    (r300TexObjPtr) tObj->DriverData,
					    face);
		}
	}

	if (!t->base.memBlock) {
		/* texmem alloc failed, use s/w fallback */
		return GL_FALSE;
	}

	return GL_TRUE;
}

static GLboolean enable_tex_rect(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	ASSERT(tObj->Target == GL_TEXTURE_RECTANGLE_NV);

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock && !rmesa->prefer_gart_client_texturing)
			return GL_FALSE;
	}

	return GL_TRUE;
}

static GLboolean update_tex_common(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	/* Fallback if there's a texture border */
	if (tObj->Image[0][tObj->BaseLevel]->Border > 0)
		return GL_FALSE;

	/* Update state if this is a different texture object to last
	 * time.
	 */
	if (rmesa->state.texture.unit[unit].texobj != t) {
		if (rmesa->state.texture.unit[unit].texobj != NULL) {
			/* The old texture is no longer bound to this texture unit.
			 * Mark it as such.
			 */

			rmesa->state.texture.unit[unit].texobj->base.bound &=
			    ~(1UL << unit);
		}

		rmesa->state.texture.unit[unit].texobj = t;
		t->base.bound |= (1UL << unit);
		t->dirty_state |= 1 << unit;
		driUpdateTextureLRU((driTextureObject *) t);	/* XXX: should be locked! */
	}

	return !t->border_fallback;
}

static GLboolean r300UpdateTextureUnit(GLcontext * ctx, int unit)
{
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

	if (texUnit->_ReallyEnabled & (TEXTURE_RECT_BIT)) {
		return (enable_tex_rect(ctx, unit) &&
			update_tex_common(ctx, unit));
	} else if (texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT)) {
		return (enable_tex_2d(ctx, unit) &&
			update_tex_common(ctx, unit));
	}
#if ENABLE_HW_3D_TEXTURE
	else if (texUnit->_ReallyEnabled & (TEXTURE_3D_BIT)) {
		return (enable_tex_3d(ctx, unit) &&
			update_tex_common(ctx, unit));
	}
#endif
	else if (texUnit->_ReallyEnabled & (TEXTURE_CUBE_BIT)) {
		return (enable_tex_cube(ctx, unit) &&
			update_tex_common(ctx, unit));
	} else if (texUnit->_ReallyEnabled) {
		return GL_FALSE;
	} else {
		return GL_TRUE;
 	}
}

void r300UpdateTextureState(GLcontext * ctx)
{
	GLboolean ok;

	ok = (r300UpdateTextureUnit(ctx, 0) &&
	      r300UpdateTextureUnit(ctx, 1) &&
	      r300UpdateTextureUnit(ctx, 2) &&
	      r300UpdateTextureUnit(ctx, 3) &&
	      r300UpdateTextureUnit(ctx, 4) &&
	      r300UpdateTextureUnit(ctx, 5) &&
	      r300UpdateTextureUnit(ctx, 6) &&
	      r300UpdateTextureUnit(ctx, 7)
	      );
}
