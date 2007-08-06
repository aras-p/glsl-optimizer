/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef INTEL_MIPMAP_TREE_H
#define INTEL_MIPMAP_TREE_H

#include "intel_context.h"
#include "main/glheader.h"

struct pipe_region;


struct pipe_mipmap_tree *intel_miptree_create(struct intel_context *intel,
                                               GLenum target,
                                               GLenum internal_format,
                                               GLuint first_level,
                                               GLuint last_level,
                                               GLuint width0,
                                               GLuint height0,
                                               GLuint depth0,
                                               GLuint cpp,
                                               GLuint compress_byte);

void intel_miptree_reference(struct pipe_mipmap_tree **dst,
                             struct pipe_mipmap_tree *src);

void intel_miptree_release(struct intel_context *intel,
                           struct pipe_mipmap_tree **mt);

/* Check if an image fits an existing mipmap tree layout
 */
GLboolean intel_miptree_match_image(struct pipe_mipmap_tree *mt,
                                    struct gl_texture_image *image,
                                    GLuint face, GLuint level);

/* Return a pointer to an image within a tree.  Return image stride as
 * well.
 */
GLubyte *intel_miptree_image_map(struct intel_context *intel,
                                 struct pipe_mipmap_tree *mt,
                                 GLuint face,
                                 GLuint level,
                                 GLuint * row_stride, GLuint * image_stride);

void intel_miptree_image_unmap(struct intel_context *intel,
                               struct pipe_mipmap_tree *mt);


/* Return the linear offset of an image relative to the start of the
 * tree:
 */
GLuint intel_miptree_image_offset(struct pipe_mipmap_tree *mt,
                                  GLuint face, GLuint level);

/* Return pointers to each 2d slice within an image.  Indexed by depth
 * value.
 */
const GLuint *intel_miptree_depth_offsets(struct pipe_mipmap_tree *mt,
                                          GLuint level);


void intel_miptree_set_level_info(struct pipe_mipmap_tree *mt,
                                  GLuint level,
                                  GLuint nr_images,
                                  GLuint x, GLuint y,
                                  GLuint w, GLuint h, GLuint d);

void intel_miptree_set_image_offset(struct pipe_mipmap_tree *mt,
                                    GLuint level,
                                    GLuint img, GLuint x, GLuint y);


/* Upload an image into a tree
 */
void intel_miptree_image_data(struct intel_context *intel,
                              struct pipe_mipmap_tree *dst,
                              GLuint face,
                              GLuint level,
                              void *src,
                              GLuint src_row_pitch, GLuint src_image_pitch);

/* Copy an image between two trees
 */
void intel_miptree_image_copy(struct intel_context *intel,
                              struct pipe_mipmap_tree *dst,
                              GLuint face, GLuint level,
                              struct pipe_mipmap_tree *src);

/* i915_mipmap_tree.c:
 */
GLboolean i915_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);
GLboolean i945_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);



#endif
