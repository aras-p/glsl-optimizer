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

#ifndef __RADEON_MIPMAP_TREE_H_
#define __RADEON_MIPMAP_TREE_H_

#include "radeon_common.h"

typedef struct _radeon_mipmap_tree radeon_mipmap_tree;
typedef struct _radeon_mipmap_level radeon_mipmap_level;
typedef struct _radeon_mipmap_image radeon_mipmap_image;

struct _radeon_mipmap_image {
	GLuint offset; /** Offset of this image from the start of mipmap tree buffer, in bytes */
};

struct _radeon_mipmap_level {
	GLuint width;
	GLuint height;
	GLuint depth;
	GLuint size; /** Size of each image, in bytes */
	GLuint rowstride; /** in bytes */
	radeon_mipmap_image faces[6];
};

/* store the max possible in the miptree */
#define RADEON_MIPTREE_MAX_TEXTURE_LEVELS 13

/**
 * A mipmap tree contains texture images in the layout that the hardware
 * expects.
 *
 * The meta-data of mipmap trees is immutable, i.e. you cannot change the
 * layout on-the-fly; however, the texture contents (i.e. texels) can be
 * changed.
 */
struct _radeon_mipmap_tree {
	radeonContextPtr radeon;
	radeonTexObj *t;
	struct radeon_bo *bo;
	GLuint refcount;

	GLuint totalsize; /** total size of the miptree, in bytes */

	GLenum target; /** GL_TEXTURE_xxx */
	GLenum internal_format;
	GLuint faces; /** # of faces: 6 for cubemaps, 1 otherwise */
	GLuint firstLevel; /** First mip level stored in this mipmap tree */
	GLuint lastLevel; /** Last mip level stored in this mipmap tree */

	GLuint width0; /** Width of firstLevel image */
	GLuint height0; /** Height of firstLevel image */
	GLuint depth0; /** Depth of firstLevel image */

	GLuint bpp; /** Bytes per texel */
	GLuint tilebits; /** RADEON_TXO_xxx_TILE */
	GLuint compressed; /** MESA_FORMAT_xxx indicating a compressed format, or 0 if uncompressed */

	radeon_mipmap_level levels[RADEON_MIPTREE_MAX_TEXTURE_LEVELS];
};

radeon_mipmap_tree* radeon_miptree_create(radeonContextPtr rmesa, radeonTexObj *t,
		GLenum target, GLenum internal_format, GLuint firstLevel, GLuint lastLevel,
		GLuint width0, GLuint height0, GLuint depth0,
		GLuint bpp, GLuint tilebits, GLuint compressed);
void radeon_miptree_reference(radeon_mipmap_tree *mt);
void radeon_miptree_unreference(radeon_mipmap_tree *mt);

GLboolean radeon_miptree_matches_image(radeon_mipmap_tree *mt,
		struct gl_texture_image *texImage, GLuint face, GLuint level);
GLboolean radeon_miptree_matches_texture(radeon_mipmap_tree *mt, struct gl_texture_object *texObj);
void radeon_try_alloc_miptree(radeonContextPtr rmesa, radeonTexObj *t,
			      radeon_texture_image *texImage, GLuint face, GLuint level);
GLuint radeon_miptree_image_offset(radeon_mipmap_tree *mt,
				   GLuint face, GLuint level);
void radeon_miptree_depth_offsets(radeon_mipmap_tree *mt, GLuint level, GLuint *offsets);
#endif /* __RADEON_MIPMAP_TREE_H_ */
