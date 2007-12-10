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

#ifndef ST_TEXTURE_H
#define ST_TEXTURE_H


#include "main/mtypes.h"

struct pipe_context;
struct pipe_texture;


extern struct pipe_texture *
st_texture_create(struct st_context *st,
                  unsigned target,
		  unsigned format,
                  GLenum internal_format,
                  GLuint first_level,
                  GLuint last_level,
                  GLuint width0,
                  GLuint height0,
                  GLuint depth0,
                  GLuint compress_byte);


/* Check if an image fits an existing texture
 */
extern GLboolean
st_texture_match_image(struct pipe_texture *pt,
                       struct gl_texture_image *image,
                       GLuint face, GLuint level);

/* Return a pointer to an image within a texture.  Return image stride as
 * well.
 */
extern GLubyte *
st_texture_image_map(struct st_context *st,
                     struct st_texture_image *stImage,
		     GLuint zoffset);

extern void
st_texture_image_unmap(struct st_texture_image *stImage);


/* Return pointers to each 2d slice within an image.  Indexed by depth
 * value.
 */
extern const GLuint *
st_texture_depth_offsets(struct pipe_texture *pt, GLuint level);


/* Return the linear offset of an image relative to the start of its region:
 */
extern GLuint
st_texture_image_offset(const struct pipe_texture *pt,
                        GLuint face, GLuint level);

extern GLuint
st_texture_texel_offset(const struct pipe_texture * pt,
                        GLuint face, GLuint level,
                        GLuint col, GLuint row, GLuint img);


/* Upload an image into a texture
 */
extern void
st_texture_image_data(struct pipe_context *pipe,
                      struct pipe_texture *dst,
                      GLuint face, GLuint level, void *src,
                      GLuint src_row_pitch, GLuint src_image_pitch);


/* Copy an image between two textures
 */
extern void
st_texture_image_copy(struct pipe_context *pipe,
                      struct pipe_texture *dst,
                      GLuint face, GLuint level,
                      struct pipe_texture *src);


#endif
