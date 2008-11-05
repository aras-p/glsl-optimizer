/*
 * Copyright (C) 2008 Nicolai Haehnle.
 *
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

#include "r300_mipmap_tree.h"

#include <errno.h>
#include <unistd.h>

#include "main/simple_list.h"
#include "main/texcompress.h"
#include "main/texformat.h"

#include "radeon_buffer.h"

static GLuint r300_compressed_texture_size(GLcontext *ctx,
		GLsizei width, GLsizei height, GLsizei depth,
		GLuint mesaFormat)
{
	GLuint size = _mesa_compressed_texture_size(ctx, width, height, depth, mesaFormat);

	if (mesaFormat == MESA_FORMAT_RGB_DXT1 ||
	    mesaFormat == MESA_FORMAT_RGBA_DXT1) {
		if (width + 3 < 8)	/* width one block */
			size = size * 4;
		else if (width + 3 < 16)
			size = size * 2;
	} else {
		/* DXT3/5, 16 bytes per block */
		WARN_ONCE("DXT 3/5 suffers from multitexturing problems!\n");
		if (width + 3 < 8)
			size = size * 2;
	}

	return size;
}

/**
 * Compute sizes and fill in offset and blit information for the given
 * image (determined by \p face and \p level).
 *
 * \param curOffset points to the offset at which the image is to be stored
 * and is updated by this function according to the size of the image.
 */
static void compute_tex_image_offset(r300_mipmap_tree *mt,
	GLuint face, GLuint level, GLuint* curOffset)
{
	r300_mipmap_level *lvl = &mt->levels[level];

	/* Find image size in bytes */
	if (mt->compressed) {
		/* TODO: Is this correct? Need test cases for compressed textures! */
		GLuint align;

		if (mt->target == GL_TEXTURE_RECTANGLE_NV)
			align = 64 / mt->bpp;
		else
			align = 32 / mt->bpp;
		lvl->rowstride = (lvl->width + align - 1) & ~(align - 1);
		lvl->size = r300_compressed_texture_size(mt->r300->radeon.glCtx,
			lvl->width, lvl->height, lvl->depth, mt->compressed);
	} else if (mt->target == GL_TEXTURE_RECTANGLE_NV) {
		lvl->rowstride = (lvl->width * mt->bpp + 63) & ~63;
		lvl->size = lvl->rowstride * lvl->height;
	} else if (mt->tilebits & R300_TXO_MICRO_TILE) {
		/* tile pattern is 16 bytes x2. mipmaps stay 32 byte aligned,
		 * though the actual offset may be different (if texture is less than
		 * 32 bytes width) to the untiled case */
		lvl->rowstride = (lvl->width * mt->bpp * 2 + 31) & ~31;
		lvl->size = lvl->rowstride * ((lvl->height + 1) / 2) * lvl->depth;
	} else {
		lvl->rowstride = (lvl->width * mt->bpp + 31) & ~31;
		lvl->size = lvl->rowstride * lvl->height * lvl->depth;
	}
	assert(lvl->size > 0);

	/* All images are aligned to a 32-byte offset */
	*curOffset = (*curOffset + 0x1f) & ~0x1f;
	lvl->faces[face].offset = *curOffset;
	*curOffset += lvl->size;
}

static GLuint minify(GLuint size, GLuint levels)
{
	size = size >> levels;
	if (size < 1)
		size = 1;
	return size;
}

static void calculate_miptree_layout(r300_mipmap_tree *mt)
{
	GLuint curOffset;
	GLuint numLevels;
	GLuint i;

	numLevels = mt->lastLevel - mt->firstLevel + 1;
	assert(numLevels <= RADEON_MAX_TEXTURE_LEVELS);

	curOffset = 0;
	for(i = 0; i < numLevels; i++) {
		GLuint face;

		mt->levels[i].width = minify(mt->width0, i);
		mt->levels[i].height = minify(mt->height0, i);
		mt->levels[i].depth = minify(mt->depth0, i);

		for(face = 0; face < mt->faces; face++)
			compute_tex_image_offset(mt, face, i, &curOffset);
	}

	/* Note the required size in memory */
	mt->totalsize = (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;
}


/**
 * Create a new mipmap tree, calculate its layout and allocate memory.
 */
r300_mipmap_tree* r300_miptree_create(r300ContextPtr rmesa, r300TexObj *t,
		GLenum target, GLuint firstLevel, GLuint lastLevel,
		GLuint width0, GLuint height0, GLuint depth0,
		GLuint bpp, GLuint tilebits, GLuint compressed)
{
	r300_mipmap_tree *mt = CALLOC_STRUCT(_r300_mipmap_tree);

	mt->r300 = rmesa;
	mt->refcount = 1;
	mt->t = t;
	mt->target = target;
	mt->faces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
	mt->firstLevel = firstLevel;
	mt->lastLevel = lastLevel;
	mt->width0 = width0;
	mt->height0 = height0;
	mt->depth0 = depth0;
	mt->bpp = bpp;
	mt->tilebits = tilebits;
	mt->compressed = compressed;

	calculate_miptree_layout(mt);

	mt->bo = radeon_bo_open(rmesa->radeon.radeonScreen->bom, 0, mt->totalsize, 1024, 0);

	return mt;
}

void r300_miptree_reference(r300_mipmap_tree *mt)
{
	mt->refcount++;
	assert(mt->refcount > 0);
}

void r300_miptree_unreference(r300_mipmap_tree *mt)
{
	if (!mt)
		return;

	assert(mt->refcount > 0);
	mt->refcount--;
	if (!mt->refcount) {
		radeon_bo_unref(mt->bo);
		free(mt);
	}
}


static void calculate_first_last_level(struct gl_texture_object *tObj,
				       GLuint *pfirstLevel, GLuint *plastLevel)
{
	const struct gl_texture_image * const baseImage =
		tObj->Image[0][tObj->BaseLevel];

	/* These must be signed values.  MinLod and MaxLod can be negative numbers,
	* and having firstLevel and lastLevel as signed prevents the need for
	* extra sign checks.
	*/
	int   firstLevel;
	int   lastLevel;

	/* Yes, this looks overly complicated, but it's all needed.
	*/
	switch (tObj->Target) {
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
	case GL_TEXTURE_CUBE_MAP:
		if (tObj->MinFilter == GL_NEAREST || tObj->MinFilter == GL_LINEAR) {
			/* GL_NEAREST and GL_LINEAR only care about GL_TEXTURE_BASE_LEVEL.
			*/
			firstLevel = lastLevel = tObj->BaseLevel;
		} else {
			firstLevel = tObj->BaseLevel + (GLint)(tObj->MinLod + 0.5);
			firstLevel = MAX2(firstLevel, tObj->BaseLevel);
			firstLevel = MIN2(firstLevel, tObj->BaseLevel + baseImage->MaxLog2);
			lastLevel = tObj->BaseLevel + (GLint)(tObj->MaxLod + 0.5);
			lastLevel = MAX2(lastLevel, tObj->BaseLevel);
			lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
			lastLevel = MIN2(lastLevel, tObj->MaxLevel);
			lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */
		}
		break;
	case GL_TEXTURE_RECTANGLE_NV:
	case GL_TEXTURE_4D_SGIS:
		firstLevel = lastLevel = 0;
		break;
	default:
		return;
	}

	/* save these values */
	*pfirstLevel = firstLevel;
	*plastLevel = lastLevel;
}


/**
 * Checks whether the given miptree can hold the given texture image at the
 * given face and level.
 */
GLboolean r300_miptree_matches_image(r300_mipmap_tree *mt,
		struct gl_texture_image *texImage, GLuint face, GLuint level)
{
	r300_mipmap_level *lvl;

	if (face >= mt->faces || level < mt->firstLevel || level > mt->lastLevel)
		return GL_FALSE;

	if (texImage->TexFormat->TexelBytes != mt->bpp)
		return GL_FALSE;

	lvl = &mt->levels[level - mt->firstLevel];
	if (lvl->width != texImage->Width ||
	    lvl->height != texImage->Height ||
	    lvl->depth != texImage->Depth)
		return GL_FALSE;

	return GL_TRUE;
}


/**
 * Checks whether the given miptree has the right format to store the given texture object.
 */
GLboolean r300_miptree_matches_texture(r300_mipmap_tree *mt, struct gl_texture_object *texObj)
{
	struct gl_texture_image *firstImage;
	GLuint compressed;
	GLuint numfaces = 1;
	GLuint firstLevel, lastLevel;

	calculate_first_last_level(texObj, &firstLevel, &lastLevel);
	if (texObj->Target == GL_TEXTURE_CUBE_MAP)
		numfaces = 6;

	firstImage = texObj->Image[0][firstLevel];
	compressed = firstImage->IsCompressed ? firstImage->TexFormat->MesaFormat : 0;

	return (mt->firstLevel == firstLevel &&
	        mt->lastLevel == lastLevel &&
	        mt->width0 == firstImage->Width &&
	        mt->height0 == firstImage->Height &&
	        mt->depth0 == firstImage->Depth &&
	        mt->bpp == firstImage->TexFormat->TexelBytes &&
	        mt->compressed == compressed);
}


/**
 * Try to allocate a mipmap tree for the given texture that will fit the
 * given image in the given position.
 */
void r300_try_alloc_miptree(r300ContextPtr rmesa, r300TexObj *t,
		struct gl_texture_image *texImage, GLuint face, GLuint level)
{
	GLuint compressed = texImage->IsCompressed ? texImage->TexFormat->MesaFormat : 0;
	GLuint numfaces = 1;
	GLuint firstLevel, lastLevel;

	assert(!t->mt);

	calculate_first_last_level(&t->base, &firstLevel, &lastLevel);
	if (t->base.Target == GL_TEXTURE_CUBE_MAP)
		numfaces = 6;

	if (level != firstLevel || face >= numfaces)
		return;

	t->mt = r300_miptree_create(rmesa, t, t->base.Target,
		firstLevel, lastLevel,
		texImage->Width, texImage->Height, texImage->Depth,
		texImage->TexFormat->TexelBytes, t->tile_bits, compressed);
}
