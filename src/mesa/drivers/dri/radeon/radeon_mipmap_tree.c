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

#include "radeon_mipmap_tree.h"

#include <errno.h>
#include <unistd.h>

#include "main/simple_list.h"
#include "main/texcompress.h"
#include "main/texformat.h"

static GLuint radeon_compressed_texture_size(GLcontext *ctx,
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
	  //		WARN_ONCE("DXT 3/5 suffers from multitexturing problems!\n");
		if (width + 3 < 8)
			size = size * 2;
	}

	return size;
}


static int radeon_compressed_num_bytes(GLuint mesaFormat)
{
   int bytes = 0;
   switch(mesaFormat) {
     
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
     bytes = 2;
     break;
     
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
     bytes = 4;
   default:
     break;
   }
   
   return bytes;
}

/**
 * Compute sizes and fill in offset and blit information for the given
 * image (determined by \p face and \p level).
 *
 * \param curOffset points to the offset at which the image is to be stored
 * and is updated by this function according to the size of the image.
 */
static void compute_tex_image_offset(radeonContextPtr rmesa, radeon_mipmap_tree *mt,
	GLuint face, GLuint level, GLuint* curOffset)
{
	radeon_mipmap_level *lvl = &mt->levels[level];
	uint32_t row_align;

	/* Find image size in bytes */
	if (mt->compressed) {
		/* TODO: Is this correct? Need test cases for compressed textures! */
		row_align = rmesa->texture_compressed_row_align - 1;
		lvl->rowstride = (lvl->width * mt->bpp + row_align) & ~row_align;
		lvl->size = radeon_compressed_texture_size(mt->radeon->glCtx,
							   lvl->width, lvl->height, lvl->depth, mt->compressed);
	} else if (mt->target == GL_TEXTURE_RECTANGLE_NV) {
		row_align = rmesa->texture_rect_row_align - 1;
		lvl->rowstride = (lvl->width * mt->bpp + row_align) & ~row_align;
		lvl->size = lvl->rowstride * lvl->height;
	} else if (mt->tilebits & RADEON_TXO_MICRO_TILE) {
		/* tile pattern is 16 bytes x2. mipmaps stay 32 byte aligned,
		 * though the actual offset may be different (if texture is less than
		 * 32 bytes width) to the untiled case */
		lvl->rowstride = (lvl->width * mt->bpp * 2 + 31) & ~31;
		lvl->size = lvl->rowstride * ((lvl->height + 1) / 2) * lvl->depth;
	} else {
		row_align = rmesa->texture_row_align - 1;
		lvl->rowstride = (lvl->width * mt->bpp + row_align) & ~row_align;
		lvl->size = lvl->rowstride * lvl->height * lvl->depth;
	}
	assert(lvl->size > 0);

	/* All images are aligned to a 32-byte offset */
	*curOffset = (*curOffset + 0x1f) & ~0x1f;
	lvl->faces[face].offset = *curOffset;
	*curOffset += lvl->size;

	if (RADEON_DEBUG & RADEON_TEXTURE)
	  fprintf(stderr,
		  "level %d, face %d: rs:%d %dx%d at %d\n",
		  level, face, lvl->rowstride, lvl->width, lvl->height, lvl->faces[face].offset);
}

static GLuint minify(GLuint size, GLuint levels)
{
	size = size >> levels;
	if (size < 1)
		size = 1;
	return size;
}


static void calculate_miptree_layout_r100(radeonContextPtr rmesa, radeon_mipmap_tree *mt)
{
	GLuint curOffset;
	GLuint numLevels;
	GLuint i;
	GLuint face;

	numLevels = mt->lastLevel - mt->firstLevel + 1;
	assert(numLevels <= rmesa->glCtx->Const.MaxTextureLevels);

	curOffset = 0;
	for(face = 0; face < mt->faces; face++) {

		for(i = 0; i < numLevels; i++) {
			mt->levels[i].width = minify(mt->width0, i);
			mt->levels[i].height = minify(mt->height0, i);
			mt->levels[i].depth = minify(mt->depth0, i);
			compute_tex_image_offset(rmesa, mt, face, i, &curOffset);
		}
	}

	/* Note the required size in memory */
	mt->totalsize = (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;
}

static void calculate_miptree_layout_r300(radeonContextPtr rmesa, radeon_mipmap_tree *mt)
{
	GLuint curOffset;
	GLuint numLevels;
	GLuint i;

	numLevels = mt->lastLevel - mt->firstLevel + 1;
	assert(numLevels <= rmesa->glCtx->Const.MaxTextureLevels);

	curOffset = 0;
	for(i = 0; i < numLevels; i++) {
		GLuint face;

		mt->levels[i].width = minify(mt->width0, i);
		mt->levels[i].height = minify(mt->height0, i);
		mt->levels[i].depth = minify(mt->depth0, i);

		for(face = 0; face < mt->faces; face++)
			compute_tex_image_offset(rmesa, mt, face, i, &curOffset);
	}

	/* Note the required size in memory */
	mt->totalsize = (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;
}

/**
 * Create a new mipmap tree, calculate its layout and allocate memory.
 */
radeon_mipmap_tree* radeon_miptree_create(radeonContextPtr rmesa, radeonTexObj *t,
		GLenum target, GLenum internal_format, GLuint firstLevel, GLuint lastLevel,
		GLuint width0, GLuint height0, GLuint depth0,
		GLuint bpp, GLuint tilebits, GLuint compressed)
{
	radeon_mipmap_tree *mt = CALLOC_STRUCT(_radeon_mipmap_tree);

	mt->radeon = rmesa;
	mt->internal_format = internal_format;
	mt->refcount = 1;
	mt->t = t;
	mt->target = target;
	mt->faces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
	mt->firstLevel = firstLevel;
	mt->lastLevel = lastLevel;
	mt->width0 = width0;
	mt->height0 = height0;
	mt->depth0 = depth0;
	mt->bpp = compressed ? radeon_compressed_num_bytes(compressed) : bpp;
	mt->tilebits = tilebits;
	mt->compressed = compressed;

	if (rmesa->radeonScreen->chip_family >= CHIP_FAMILY_R300)
		calculate_miptree_layout_r300(rmesa, mt);
	else
		calculate_miptree_layout_r100(rmesa, mt);

	mt->bo = radeon_bo_open(rmesa->radeonScreen->bom,
                            0, mt->totalsize, 1024,
                            RADEON_GEM_DOMAIN_VRAM,
                            0);

	return mt;
}

void radeon_miptree_reference(radeon_mipmap_tree *mt)
{
	mt->refcount++;
	assert(mt->refcount > 0);
}

void radeon_miptree_unreference(radeon_mipmap_tree *mt)
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


/**
 * Calculate first and last mip levels for the given texture object,
 * where the dimensions are taken from the given texture image at
 * the given level.
 *
 * Note: level is the OpenGL level number, which is not necessarily the same
 * as the first level that is actually present.
 *
 * The base level image of the given texture face must be non-null,
 * or this will fail.
 */
static void calculate_first_last_level(struct gl_texture_object *tObj,
				       GLuint *pfirstLevel, GLuint *plastLevel,
				       GLuint face, GLuint level)
{
	const struct gl_texture_image * const baseImage =
		tObj->Image[face][level];

	assert(baseImage);
	
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
			firstLevel = MIN2(firstLevel, level + baseImage->MaxLog2);
			lastLevel = tObj->BaseLevel + (GLint)(tObj->MaxLod + 0.5);
			lastLevel = MAX2(lastLevel, tObj->BaseLevel);
			lastLevel = MIN2(lastLevel, level + baseImage->MaxLog2);
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
GLboolean radeon_miptree_matches_image(radeon_mipmap_tree *mt,
		struct gl_texture_image *texImage, GLuint face, GLuint level)
{
	radeon_mipmap_level *lvl;

	if (face >= mt->faces || level < mt->firstLevel || level > mt->lastLevel)
		return GL_FALSE;

	if (texImage->InternalFormat != mt->internal_format ||
	    texImage->IsCompressed != mt->compressed)
		return GL_FALSE;

	if (!texImage->IsCompressed &&
	    !mt->compressed &&
	    texImage->TexFormat->TexelBytes != mt->bpp)
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
GLboolean radeon_miptree_matches_texture(radeon_mipmap_tree *mt, struct gl_texture_object *texObj)
{
	struct gl_texture_image *firstImage;
	GLuint compressed;
	GLuint numfaces = 1;
	GLuint firstLevel, lastLevel;

	calculate_first_last_level(texObj, &firstLevel, &lastLevel, 0, texObj->BaseLevel);
	if (texObj->Target == GL_TEXTURE_CUBE_MAP)
		numfaces = 6;

	firstImage = texObj->Image[0][firstLevel];
	compressed = firstImage->IsCompressed ? firstImage->TexFormat->MesaFormat : 0;

	return (mt->firstLevel == firstLevel &&
	        mt->lastLevel == lastLevel &&
	        mt->width0 == firstImage->Width &&
	        mt->height0 == firstImage->Height &&
	        mt->depth0 == firstImage->Depth &&
	        mt->compressed == compressed &&
	        (!mt->compressed ? (mt->bpp == firstImage->TexFormat->TexelBytes) : 1));
}


/**
 * Try to allocate a mipmap tree for the given texture that will fit the
 * given image in the given position.
 */
void radeon_try_alloc_miptree(radeonContextPtr rmesa, radeonTexObj *t,
		radeon_texture_image *image, GLuint face, GLuint level)
{
	GLuint compressed = image->base.IsCompressed ? image->base.TexFormat->MesaFormat : 0;
	GLuint numfaces = 1;
	GLuint firstLevel, lastLevel;

	assert(!t->mt);

	calculate_first_last_level(&t->base, &firstLevel, &lastLevel, face, level);
	if (t->base.Target == GL_TEXTURE_CUBE_MAP)
		numfaces = 6;

	if (level != firstLevel || face >= numfaces)
		return;

	t->mt = radeon_miptree_create(rmesa, t, t->base.Target,
		image->base.InternalFormat,
		firstLevel, lastLevel,
		image->base.Width, image->base.Height, image->base.Depth,
		image->base.TexFormat->TexelBytes, t->tile_bits, compressed);
}

/* Although we use the image_offset[] array to store relative offsets
 * to cube faces, Mesa doesn't know anything about this and expects
 * each cube face to be treated as a separate image.
 *
 * These functions present that view to mesa:
 */
void
radeon_miptree_depth_offsets(radeon_mipmap_tree *mt, GLuint level, GLuint *offsets)
{
     if (mt->target != GL_TEXTURE_3D || mt->faces == 1)
        offsets[0] = 0;
     else {
	int i;
	for (i = 0; i < 6; i++)
		offsets[i] = mt->levels[level].faces[i].offset;
     }
}

GLuint
radeon_miptree_image_offset(radeon_mipmap_tree *mt,
			    GLuint face, GLuint level)
{
   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB)
      return (mt->levels[level].faces[face].offset);
   else
      return mt->levels[level].faces[0].offset;
}
