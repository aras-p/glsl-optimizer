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


struct st_texture_image
{
   struct gl_texture_image base;

   /* These aren't stored in gl_texture_image 
    */
   GLuint level;
   GLuint face;

   /* If stImage->pt != NULL, image data is stored here.
    * Else if stImage->base.Data != NULL, image is stored there.
    * Else there is no image data.
    */
   struct pipe_texture *pt;

   struct pipe_transfer *transfer;
};



struct st_texture_object
{
   struct gl_texture_object base;       /* The "parent" object */

   /* The texture must include at levels [0..lastLevel] once validated:
    */
   GLuint lastLevel;

   /* On validation any active images held in main memory or in other
    * textures will be copied to this texture and the old storage freed.
    */
   struct pipe_texture *pt;

   GLboolean teximage_realloc;

   /* True if there is/was a surface bound to this texture object.  It helps
    * track whether the texture object is surface based or not.
    */
   GLboolean surface_based;
};


static INLINE struct st_texture_image *
st_texture_image(struct gl_texture_image *img)
{
   return (struct st_texture_image *) img;
}

static INLINE struct st_texture_object *
st_texture_object(struct gl_texture_object *obj)
{
   return (struct st_texture_object *) obj;
}


static INLINE struct pipe_texture *
st_get_texobj_texture(struct gl_texture_object *texObj)
{
   struct st_texture_object *stObj = st_texture_object(texObj);
   return stObj ? stObj->pt : NULL;
}


static INLINE struct pipe_texture *
st_get_stobj_texture(struct st_texture_object *stObj)
{
   return stObj ? stObj->pt : NULL;
}


extern struct pipe_texture *
st_texture_create(struct st_context *st,
                  enum pipe_texture_target target,
		  enum pipe_format format,
                  GLuint last_level,
                  GLuint width0,
                  GLuint height0,
                  GLuint depth0,
                  GLuint tex_usage );


/* Check if an image fits into an existing texture object.
 */
extern GLboolean
st_texture_match_image(const struct pipe_texture *pt,
                       const struct gl_texture_image *image,
                       GLuint face, GLuint level);

/* Return a pointer to an image within a texture.  Return image stride as
 * well.
 */
extern GLubyte *
st_texture_image_map(struct st_context *st,
                     struct st_texture_image *stImage,
		     GLuint zoffset,
                     enum pipe_transfer_usage usage,
                     unsigned x, unsigned y,
                     unsigned w, unsigned h);

extern void
st_texture_image_unmap(struct st_context *st,
                       struct st_texture_image *stImage);


/* Return pointers to each 2d slice within an image.  Indexed by depth
 * value.
 */
extern const GLuint *
st_texture_depth_offsets(struct pipe_texture *pt, GLuint level);


/* Return the linear offset of an image relative to the start of its region.
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
st_texture_image_data(struct st_context *st,
                      struct pipe_texture *dst,
                      GLuint face, GLuint level, void *src,
                      GLuint src_row_pitch, GLuint src_image_pitch);


/* Copy an image between two textures
 */
extern void
st_texture_image_copy(struct pipe_context *pipe,
                      struct pipe_texture *dst, GLuint dstLevel,
                      struct pipe_texture *src,
                      GLuint face);

extern void
st_teximage_flush_before_map(struct st_context *st,
			     struct pipe_texture *pt,
			     unsigned int face,
			     unsigned int level,
			     enum pipe_transfer_usage usage);
#endif
