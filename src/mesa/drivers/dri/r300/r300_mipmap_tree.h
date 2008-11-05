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

#ifndef __R300_MIPMAP_TREE_H_
#define __R300_MIPMAP_TREE_H_

#include "r300_context.h"

typedef struct _r300_mipmap_tree r300_mipmap_tree;
typedef struct _r300_mipmap_level r300_mipmap_level;
typedef struct _r300_mipmap_image r300_mipmap_image;

struct _r300_mipmap_image {
	GLuint offset; /** Offset of this image from the start of mipmap tree buffer, in bytes */
};

struct _r300_mipmap_level {
	GLuint width;
	GLuint height;
	GLuint depth;
	GLuint size; /** Size of each image, in bytes */
	GLuint rowstride; /** in bytes */
	r300_mipmap_image faces[6];
};


/**
 * A mipmap tree contains texture images in the layout that the hardware
 * expects.
 *
 * The meta-data of mipmap trees is immutable, i.e. you cannot change the
 * layout on-the-fly; however, the texture contents (i.e. texels) can be
 * changed.
 */
struct _r300_mipmap_tree {
	r300ContextPtr r300;
	r300TexObj *t;
	struct radeon_bo *bo;
	GLuint refcount;

	GLuint totalsize; /** total size of the miptree, in bytes */

	GLenum target; /** GL_TEXTURE_xxx */
	GLuint faces; /** # of faces: 6 for cubemaps, 1 otherwise */
	GLuint firstLevel; /** First mip level stored in this mipmap tree */
	GLuint lastLevel; /** Last mip level stored in this mipmap tree */

	GLuint width0; /** Width of firstLevel image */
	GLuint height0; /** Height of firstLevel image */
	GLuint depth0; /** Depth of firstLevel image */

	GLuint bpp; /** Bytes per texel */
	GLuint tilebits; /** R300_TXO_xxx_TILE */
	GLuint compressed; /** MESA_FORMAT_xxx indicating a compressed format, or 0 if uncompressed */

	r300_mipmap_level levels[RADEON_MAX_TEXTURE_LEVELS];
};

r300_mipmap_tree* r300_miptree_create(r300ContextPtr rmesa, r300TexObj *t,
		GLenum target, GLuint firstLevel, GLuint lastLevel,
		GLuint width0, GLuint height0, GLuint depth0,
		GLuint bpp, GLuint tilebits, GLuint compressed);
void r300_miptree_reference(r300_mipmap_tree *mt);
void r300_miptree_unreference(r300_mipmap_tree *mt);

GLboolean r300_miptree_matches_image(r300_mipmap_tree *mt,
		struct gl_texture_image *texImage, GLuint face, GLuint level);
GLboolean r300_miptree_matches_texture(r300_mipmap_tree *mt, struct gl_texture_object *texObj);
void r300_try_alloc_miptree(r300ContextPtr rmesa, r300TexObj *t,
		struct gl_texture_image *texImage, GLuint face, GLuint level);


#endif /* __R300_MIPMAP_TREE_H_ */
