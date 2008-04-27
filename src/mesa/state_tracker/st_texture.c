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

#include "st_context.h"
#include "st_format.h"
#include "st_texture.h"
#include "enums.h"

#undef Elements  /* fix re-defined macro warning */

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"


#define DBG if(0) printf

#if 0
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
#endif


/**
 * Allocate a new pipe_texture object
 * width0, height0, depth0 are the dimensions of the level 0 image
 * (the highest resolution).  last_level indicates how many mipmap levels
 * to allocate storage for.  For non-mipmapped textures, this will be zero.
 */
struct pipe_texture *
st_texture_create(struct st_context *st,
                  enum pipe_texture_target target,
		  enum pipe_format format,
		  GLuint last_level,
		  GLuint width0,
		  GLuint height0,
		  GLuint depth0,
		  GLuint compress_byte)
{
   struct pipe_texture pt, *newtex;
   struct pipe_screen *screen = st->pipe->screen;

   assert(target <= PIPE_TEXTURE_CUBE);

   DBG("%s target %s format %s last_level %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_lookup_enum_by_nr(format), last_level);

   assert(format);
   assert(screen->is_format_supported(screen, format, PIPE_TEXTURE));

   memset(&pt, 0, sizeof(pt));
   pt.target = target;
   pt.format = format;
   pt.last_level = last_level;
   pt.width[0] = width0;
   pt.height[0] = height0;
   pt.depth[0] = depth0;
   pt.compressed = compress_byte ? 1 : 0;
   pt.cpp = pt.compressed ? compress_byte : st_sizeof_format(format);

   newtex = screen->texture_create(screen, &pt);

   assert(!newtex || newtex->refcount == 1);

   return newtex;
}


/**
 * Check if a texture image be pulled into a unified mipmap texture.
 * This mirrors the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
GLboolean
st_texture_match_image(const struct pipe_texture *pt,
                       const struct gl_texture_image *image,
                       GLuint face, GLuint level)
{
   /* Images with borders are never pulled into mipmap textures. 
    */
   if (image->Border) 
      return GL_FALSE;

   if (st_mesa_format_to_pipe_format(image->TexFormat->MesaFormat) != pt->format ||
       image->IsCompressed != pt->compressed)
      return GL_FALSE;

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * texture, changed targets, etc.
    */
   if (image->Width != pt->width[level] ||
       image->Height != pt->height[level] ||
       image->Depth != pt->depth[level])
      return GL_FALSE;

   return GL_TRUE;
}


#if 000
/* Although we use the image_offset[] array to store relative offsets
 * to cube faces, Mesa doesn't know anything about this and expects
 * each cube face to be treated as a separate image.
 *
 * These functions present that view to mesa:
 */
const GLuint *
st_texture_depth_offsets(struct pipe_texture *pt, GLuint level)
{
   static const GLuint zero = 0;

   if (pt->target != PIPE_TEXTURE_3D || pt->level[level].nr_images == 1)
      return &zero;
   else
      return pt->level[level].image_offset;
}


/**
 * Return the offset to the given mipmap texture image within the
 * texture memory buffer, in bytes.
 */
GLuint
st_texture_image_offset(const struct pipe_texture * pt,
                        GLuint face, GLuint level)
{
   if (pt->target == PIPE_TEXTURE_CUBE)
      return (pt->level[level].level_offset +
              pt->level[level].image_offset[face] * pt->cpp);
   else
      return pt->level[level].level_offset;
}
#endif


/**
 * Map a teximage in a mipmap texture.
 * \param row_stride  returns row stride in bytes
 * \param image_stride  returns image stride in bytes (for 3D textures).
 * \return address of mapping
 */
GLubyte *
st_texture_image_map(struct st_context *st, struct st_texture_image *stImage,
		     GLuint zoffset)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct pipe_texture *pt = stImage->pt;
   DBG("%s \n", __FUNCTION__);

   stImage->surface = screen->get_tex_surface(screen, pt, stImage->face,
                                              stImage->level, zoffset);

   return pipe_surface_map(stImage->surface);
}


void
st_texture_image_unmap(struct st_texture_image *stImage)
{
   DBG("%s\n", __FUNCTION__);

   pipe_surface_unmap(stImage->surface);

   pipe_surface_reference(&stImage->surface, NULL);
}



/**
 * Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
static void
st_surface_data(struct pipe_context *pipe,
		struct pipe_surface *dst,
		unsigned dstx, unsigned dsty,
		const void *src, unsigned src_pitch,
		unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   pipe_copy_rect(pipe_surface_map(dst),
                  dst->cpp,
                  dst->pitch,
                  dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe_surface_unmap(dst);
}


/* Upload data for a particular image.
 */
void
st_texture_image_data(struct pipe_context *pipe,
                      struct pipe_texture *dst,
                      GLuint face,
                      GLuint level,
                      void *src,
                      GLuint src_row_pitch, GLuint src_image_pitch)
{
   struct pipe_screen *screen = pipe->screen;
   GLuint depth = dst->depth[level];
   GLuint i;
   GLuint height = 0;
   const GLubyte *srcUB = src;
   struct pipe_surface *dst_surface;

   DBG("%s\n", __FUNCTION__);
   for (i = 0; i < depth; i++) {
      height = dst->height[level];
      if(dst->compressed)
	 height /= 4;

      dst_surface = screen->get_tex_surface(screen, dst, face, level, i);

      st_surface_data(pipe, dst_surface,
		      0, 0,                             /* dstx, dsty */
		      srcUB,
		      src_row_pitch,
		      0, 0,                             /* source x, y */
		      dst->width[level], height);       /* width, height */

      pipe_surface_reference(&dst_surface, NULL);

      srcUB += src_image_pitch * dst->cpp;
   }
}


/* Copy mipmap image between textures
 */
void
st_texture_image_copy(struct pipe_context *pipe,
                      struct pipe_texture *dst, GLuint dstLevel,
                      struct pipe_texture *src,
                      GLuint face)
{
   struct pipe_screen *screen = pipe->screen;
   GLuint width = dst->width[dstLevel];
   GLuint height = dst->height[dstLevel];
   GLuint depth = dst->depth[dstLevel];
   struct pipe_surface *src_surface;
   struct pipe_surface *dst_surface;
   GLuint i;

   /* XXX this is a hack */
   const GLuint copyHeight = dst->compressed ? height / 4 : height;

   for (i = 0; i < depth; i++) {
      GLuint srcLevel;

      /* find src texture level of needed size */
      for (srcLevel = 0; srcLevel <= src->last_level; srcLevel++) {
         if (src->width[srcLevel] == width &&
             src->height[srcLevel] == height) {
            break;
         }
      }
      assert(src->width[srcLevel] == width);
      assert(src->height[srcLevel] == height);

      dst_surface = screen->get_tex_surface(screen, dst, face, dstLevel, i);
      src_surface = screen->get_tex_surface(screen, src, face, srcLevel, i);

      pipe->surface_copy(pipe,
                         FALSE,
			 dst_surface,
			 0, 0, /* destX, Y */
			 src_surface,
			 0, 0, /* srcX, Y */
			 width, copyHeight);

      pipe_surface_reference(&dst_surface, NULL);
      pipe_surface_reference(&src_surface, NULL);
   }
}
