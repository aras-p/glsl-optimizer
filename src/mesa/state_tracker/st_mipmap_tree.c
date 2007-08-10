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

#include "st_mipmap_tree.h"
#include "enums.h"

#include "pipe/p_state.h"
#include "pipe/p_context.h"


#define DBG if(0) printf

static GLenum
target_to_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return GL_TEXTURE_CUBE_MAP_ARB;
   default:
      return target;
   }
}

struct pipe_mipmap_tree *
st_miptree_create(struct pipe_context *pipe,
                     GLenum target,
                     GLenum internal_format,
                     GLuint first_level,
                     GLuint last_level,
                     GLuint width0,
                     GLuint height0,
                     GLuint depth0, GLuint cpp, GLuint compress_byte)
{
   GLboolean ok;
   struct pipe_mipmap_tree *mt = calloc(sizeof(*mt), 1);
   GLbitfield flags = 0x0;

   DBG("%s target %s format %s level %d..%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_lookup_enum_by_nr(internal_format), first_level, last_level);

   mt->target = target_to_target(target);
   mt->internal_format = internal_format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->depth0 = depth0;
   mt->cpp = compress_byte ? compress_byte : cpp;
   mt->compressed = compress_byte ? 1 : 0;
   mt->refcount = 1; 

   ok = pipe->mipmap_tree_layout(pipe, mt);
   if (ok) {
      /* note: it's OK to pass 'pitch' as 'width' here: */
      mt->region = pipe->region_alloc(pipe, mt->cpp, mt->pitch,
                                      mt->total_height, flags);
   }

   if (!mt->region) {
      free(mt);
      return NULL;
   }

   return mt;
}


void
st_miptree_reference(struct pipe_mipmap_tree **dst,
                        struct pipe_mipmap_tree *src)
{
   src->refcount++;
   *dst = src;
   DBG("%s %p refcount now %d\n", __FUNCTION__, (void *) src, src->refcount);
}

void
st_miptree_release(struct pipe_context *pipe,
                   struct pipe_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __FUNCTION__, (void *) *mt);

      pipe->region_release(pipe, &((*mt)->region));

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
         if ((*mt)->level[i].image_offset)
            free((*mt)->level[i].image_offset);

      free(*mt);
   }
   *mt = NULL;
}




/* Can the image be pulled into a unified mipmap tree.  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
GLboolean
st_miptree_match_image(struct pipe_mipmap_tree *mt,
                          struct gl_texture_image *image,
                          GLuint face, GLuint level)
{
   /* Images with borders are never pulled into mipmap trees. 
    */
   if (image->Border) 
      return GL_FALSE;

   if (image->InternalFormat != mt->internal_format ||
       image->IsCompressed != mt->compressed)
      return GL_FALSE;

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (image->Width != mt->level[level].width ||
       image->Height != mt->level[level].height ||
       image->Depth != mt->level[level].depth)
      return GL_FALSE;

   return GL_TRUE;
}


/* Although we use the image_offset[] array to store relative offsets
 * to cube faces, Mesa doesn't know anything about this and expects
 * each cube face to be treated as a separate image.
 *
 * These functions present that view to mesa:
 */
const GLuint *
st_miptree_depth_offsets(struct pipe_mipmap_tree *mt, GLuint level)
{
   static const GLuint zero = 0;

   if (mt->target != GL_TEXTURE_3D || mt->level[level].nr_images == 1)
      return &zero;
   else
      return mt->level[level].image_offset;
}


GLuint
st_miptree_image_offset(const struct pipe_mipmap_tree * mt,
                        GLuint face, GLuint level)
{
   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB)
      return (mt->level[level].level_offset +
              mt->level[level].image_offset[face] * mt->cpp);
   else
      return mt->level[level].level_offset;
}


GLuint
st_miptree_texel_offset(const struct pipe_mipmap_tree * mt,
                        GLuint face, GLuint level,
                        GLuint col, GLuint row, GLuint img)
{
   GLuint imgOffset = st_miptree_image_offset(mt, face, level);

   return imgOffset + row * (mt->pitch + col) * mt->cpp;
}



/**
 * Map a teximage in a mipmap tree.
 * \param row_stride  returns row stride in bytes
 * \param image_stride  returns image stride in bytes (for 3D textures).
 * \return address of mapping
 */
GLubyte *
st_miptree_image_map(struct pipe_context *pipe,
                        struct pipe_mipmap_tree * mt,
                        GLuint face,
                        GLuint level,
                        GLuint * row_stride, GLuint * image_offsets)
{
   GLubyte *ptr;
   DBG("%s \n", __FUNCTION__);

   if (row_stride)
      *row_stride = mt->pitch * mt->cpp;

   if (image_offsets)
      memcpy(image_offsets, mt->level[level].image_offset,
             mt->level[level].depth * sizeof(GLuint));

   ptr = pipe->region_map(pipe, mt->region);

   return ptr + st_miptree_image_offset(mt, face, level);
}

void
st_miptree_image_unmap(struct pipe_context *pipe,
                          struct pipe_mipmap_tree *mt)
{
   DBG("%s\n", __FUNCTION__);
   pipe->region_unmap(pipe, mt->region);
}



/* Upload data for a particular image.
 */
void
st_miptree_image_data(struct pipe_context *pipe,
                      struct pipe_mipmap_tree *dst,
                      GLuint face,
                      GLuint level,
                      void *src,
                      GLuint src_row_pitch, GLuint src_image_pitch)
{
   GLuint depth = dst->level[level].depth;
   GLuint dst_offset = st_miptree_image_offset(dst, face, level);
   const GLuint *dst_depth_offset = st_miptree_depth_offsets(dst, level);
   GLuint i;
   GLuint height = 0;
   const GLubyte *srcUB = src;

   DBG("%s\n", __FUNCTION__);
   for (i = 0; i < depth; i++) {
      height = dst->level[level].height;
      if(dst->compressed)
	 height /= 4;
      pipe->region_data(pipe, dst->region,
                        dst_offset + dst_depth_offset[i], /* dst_offset */
                        0, 0,                             /* dstx, dsty */
                        srcUB,
                        src_row_pitch,
                        0, 0,                             /* source x, y */
                        dst->level[level].width, height); /* width, height */

      srcUB += src_image_pitch * dst->cpp;
   }
}

/* Copy mipmap image between trees
 */
void
st_miptree_image_copy(struct pipe_context *pipe,
                         struct pipe_mipmap_tree *dst,
                         GLuint face, GLuint level,
                         struct pipe_mipmap_tree *src)
{
   GLuint width = src->level[level].width;
   GLuint height = src->level[level].height;
   GLuint depth = src->level[level].depth;
   GLuint dst_offset = st_miptree_image_offset(dst, face, level);
   GLuint src_offset = st_miptree_image_offset(src, face, level);
   const GLuint *dst_depth_offset = st_miptree_depth_offsets(dst, level);
   const GLuint *src_depth_offset = st_miptree_depth_offsets(src, level);
   GLuint i;

   if (dst->compressed)
      height /= 4;
   for (i = 0; i < depth; i++) {
      pipe->region_copy(pipe,
                        dst->region, dst_offset + dst_depth_offset[i],
                        0,
                        0,
                        src->region, src_offset + src_depth_offset[i],
                        0, 0, width, height);
   }

}
