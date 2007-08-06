/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef ST_MIPMAP_TREE_H
#define ST_MIPMAP_TREE_H


#include "main/mtypes.h"

struct pipe_context;
struct pipe_mipmap_tree;
struct pipe_region;


struct pipe_mipmap_tree *st_miptree_create(struct pipe_context *pipe,
                                               GLenum target,
                                               GLenum internal_format,
                                               GLuint first_level,
                                               GLuint last_level,
                                               GLuint width0,
                                               GLuint height0,
                                               GLuint depth0,
                                               GLuint cpp,
                                               GLuint compress_byte);

void st_miptree_reference(struct pipe_mipmap_tree **dst,
                             struct pipe_mipmap_tree *src);

void st_miptree_release(struct pipe_context *pipe,
                           struct pipe_mipmap_tree **mt);

/* Check if an image fits an existing mipmap tree layout
 */
GLboolean st_miptree_match_image(struct pipe_mipmap_tree *mt,
                                    struct gl_texture_image *image,
                                    GLuint face, GLuint level);

/* Return a pointer to an image within a tree.  Return image stride as
 * well.
 */
GLubyte *st_miptree_image_map(struct pipe_context *pipe,
                                 struct pipe_mipmap_tree *mt,
                                 GLuint face,
                                 GLuint level,
                                 GLuint * row_stride, GLuint * image_stride);

void st_miptree_image_unmap(struct pipe_context *pipe,
                               struct pipe_mipmap_tree *mt);


/* Return the linear offset of an image relative to the start of the
 * tree:
 */
GLuint st_miptree_image_offset(struct pipe_mipmap_tree *mt,
                               GLuint face, GLuint level);

/* Return pointers to each 2d slice within an image.  Indexed by depth
 * value.
 */
const GLuint *st_miptree_depth_offsets(struct pipe_mipmap_tree *mt,
                                       GLuint level);


/* Upload an image into a tree
 */
void st_miptree_image_data(struct pipe_context *pipe,
                              struct pipe_mipmap_tree *dst,
                              GLuint face,
                              GLuint level,
                              void *src,
                              GLuint src_row_pitch, GLuint src_image_pitch);

/* Copy an image between two trees
 */
void st_miptree_image_copy(struct pipe_context *pipe,
                              struct pipe_mipmap_tree *dst,
                              GLuint face, GLuint level,
                              struct pipe_mipmap_tree *src);


#endif
