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
#include "st_public.h"
#include "st_texture.h"
#include "st_cb_fbo.h"
#include "st_inlines.h"
#include "main/enums.h"
#include "main/texfetch.h"
#include "main/teximage.h"
#include "main/texobj.h"

#undef Elements  /* fix re-defined macro warning */

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_rect.h"
#include "util/u_math.h"


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
                  GLuint usage )
{
   struct pipe_texture pt, *newtex;
   struct pipe_screen *screen = st->pipe->screen;

   assert(target <= PIPE_TEXTURE_CUBE);

   DBG("%s target %s format %s last_level %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_lookup_enum_by_nr(format), last_level);

   assert(format);
   assert(screen->is_format_supported(screen, format, target, 
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));

   memset(&pt, 0, sizeof(pt));
   pt.target = target;
   pt.format = format;
   pt.last_level = last_level;
   pt.width0 = width0;
   pt.height0 = height0;
   pt.depth0 = depth0;
   pt.tex_usage = usage;

   newtex = screen->texture_create(screen, &pt);

   assert(!newtex || pipe_is_referenced(&newtex->reference));

   return newtex;
}


/**
 * Check if a texture image can be pulled into a unified mipmap texture.
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

   /* Check if this image's format matches the established texture's format.
    */
   if (st_mesa_format_to_pipe_format(image->TexFormat) != pt->format)
      return GL_FALSE;

   /* Test if this image's size matches what's expected in the
    * established texture.
    */
   if (image->Width != u_minify(pt->width0, level) ||
       image->Height != u_minify(pt->height0, level) ||
       image->Depth != u_minify(pt->depth0, level))
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
		     GLuint zoffset, enum pipe_transfer_usage usage,
                     GLuint x, GLuint y, GLuint w, GLuint h)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt = stImage->pt;

   DBG("%s \n", __FUNCTION__);

   stImage->transfer = st_no_flush_get_tex_transfer(st, pt, stImage->face,
						    stImage->level, zoffset,
						    usage, x, y, w, h);

   if (stImage->transfer)
      return screen->transfer_map(screen, stImage->transfer);
   else
      return NULL;
}


void
st_texture_image_unmap(struct st_context *st,
                       struct st_texture_image *stImage)
{
   struct pipe_screen *screen = st->pipe->screen;

   DBG("%s\n", __FUNCTION__);

   screen->transfer_unmap(screen, stImage->transfer);

   screen->tex_transfer_destroy(stImage->transfer);
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
		struct pipe_transfer *dst,
		unsigned dstx, unsigned dsty,
		const void *src, unsigned src_stride,
		unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   struct pipe_screen *screen = pipe->screen;
   void *map = screen->transfer_map(screen, dst);

   assert(dst->texture);
   util_copy_rect(map,
                  dst->texture->format,
                  dst->stride,
                  dstx, dsty, 
                  width, height, 
                  src, src_stride, 
                  srcx, srcy);

   screen->transfer_unmap(screen, dst);
}


/* Upload data for a particular image.
 */
void
st_texture_image_data(struct st_context *st,
                      struct pipe_texture *dst,
                      GLuint face,
                      GLuint level,
                      void *src,
                      GLuint src_row_stride, GLuint src_image_stride)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   GLuint depth = u_minify(dst->depth0, level);
   GLuint i;
   const GLubyte *srcUB = src;
   struct pipe_transfer *dst_transfer;

   DBG("%s\n", __FUNCTION__);

   for (i = 0; i < depth; i++) {
      dst_transfer = st_no_flush_get_tex_transfer(st, dst, face, level, i,
						  PIPE_TRANSFER_WRITE, 0, 0,
						  u_minify(dst->width0, level),
                                                  u_minify(dst->height0, level));

      st_surface_data(pipe, dst_transfer,
		      0, 0,                             /* dstx, dsty */
		      srcUB,
		      src_row_stride,
		      0, 0,                             /* source x, y */
		      u_minify(dst->width0, level),
                      u_minify(dst->height0, level));      /* width, height */

      screen->tex_transfer_destroy(dst_transfer);

      srcUB += src_image_stride;
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
   GLuint width = u_minify(dst->width0, dstLevel); 
   GLuint height = u_minify(dst->height0, dstLevel); 
   GLuint depth = u_minify(dst->depth0, dstLevel); 
   struct pipe_surface *src_surface;
   struct pipe_surface *dst_surface;
   GLuint i;

   for (i = 0; i < depth; i++) {
      GLuint srcLevel;

      /* find src texture level of needed size */
      for (srcLevel = 0; srcLevel <= src->last_level; srcLevel++) {
         if (u_minify(src->width0, srcLevel) == width &&
             u_minify(src->height0, srcLevel) == height) {
            break;
         }
      }
      assert(u_minify(src->width0, srcLevel) == width);
      assert(u_minify(src->height0, srcLevel) == height);

#if 0
      {
         src_surface = screen->get_tex_surface(screen, src, face, srcLevel, i,
                                               PIPE_BUFFER_USAGE_CPU_READ);
         ubyte *map = screen->surface_map(screen, src_surface, PIPE_BUFFER_USAGE_CPU_READ);
         map += src_surface->width * src_surface->height * 4 / 2;
         printf("%s center pixel: %d %d %d %d (pt %p[%d] -> %p[%d])\n",
                __FUNCTION__,
                map[0], map[1], map[2], map[3],
                src, srcLevel, dst, dstLevel);

         screen->surface_unmap(screen, src_surface);
         pipe_surface_reference(&src_surface, NULL);
      }
#endif

      dst_surface = screen->get_tex_surface(screen, dst, face, dstLevel, i,
                                            PIPE_BUFFER_USAGE_GPU_WRITE);

      src_surface = screen->get_tex_surface(screen, src, face, srcLevel, i,
                                            PIPE_BUFFER_USAGE_GPU_READ);

      if (pipe->surface_copy) {
         pipe->surface_copy(pipe,
			    dst_surface,
			    0, 0, /* destX, Y */
			    src_surface,
			    0, 0, /* srcX, Y */
			    width, height);
      } else {
         util_surface_copy(pipe, FALSE,
			   dst_surface,
			   0, 0, /* destX, Y */
			   src_surface,
			   0, 0, /* srcX, Y */
			   width, height);
      }

      pipe_surface_reference(&src_surface, NULL);
      pipe_surface_reference(&dst_surface, NULL);
   }
}


/**
 * Bind a pipe surface to a texture object.  After the call,
 * the texture object is marked dirty and will be (re-)validated.
 *
 * If this is the first surface bound, the texture object is said to
 * switch from normal to surface based.  It will be cleared first in
 * this case.
 *
 * \param ps      pipe surface to be unbound
 * \param target  texture target
 * \param level   image level
 * \param format  internal format of the texture
 */
int
st_bind_texture_surface(struct pipe_surface *ps, int target, int level,
                        enum pipe_format format)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   struct st_texture_object *stObj;
   struct st_texture_image *stImage;
   GLenum internalFormat;

   switch (target) {
   case ST_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   case ST_TEXTURE_RECT:
      target = GL_TEXTURE_RECTANGLE_ARB;
      break;
   default:
      return 0;
   }

   /* map pipe format to base format for now */
   if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 3) > 0)
      internalFormat = GL_RGBA;
   else
      internalFormat = GL_RGB;

   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   _mesa_lock_texture(ctx, texObj);

   stObj = st_texture_object(texObj);
   /* switch to surface based */
   if (!stObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj);
      stObj->surface_based = GL_TRUE;
   }

   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   stImage = st_texture_image(texImage);

   _mesa_init_teximage_fields(ctx, target, texImage,
                              ps->width, ps->height, 1, 0, internalFormat);
   texImage->TexFormat = st_ChooseTextureFormat(ctx, internalFormat,
                                                GL_RGBA, GL_UNSIGNED_BYTE);
   _mesa_set_fetch_functions(texImage, 2);
   pipe_texture_reference(&stImage->pt, ps->texture);

   _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
   _mesa_unlock_texture(ctx, texObj);
   
   return 1;
}


/**
 * Unbind a pipe surface from a texture object.  After the call,
 * the texture object is marked dirty and will be (re-)validated.
 *
 * \param ps      pipe surface to be unbound
 * \param target  texture target
 * \param level   image level
 */
int
st_unbind_texture_surface(struct pipe_surface *ps, int target, int level)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   struct st_texture_object *stObj;
   struct st_texture_image *stImage;

   switch (target) {
   case ST_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   case ST_TEXTURE_RECT:
      target = GL_TEXTURE_RECTANGLE_ARB;
      break;
   default:
      return 0;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   _mesa_lock_texture(ctx, texObj);

   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   stObj = st_texture_object(texObj);
   stImage = st_texture_image(texImage);

   /* Make sure the pipe surface is still bound.  The texture object is still
    * considered surface based even if this is the last bound surface. */
   if (stImage->pt == ps->texture) {
      pipe_texture_reference(&stImage->pt, NULL);
      _mesa_clear_texture_image(ctx, texImage);

      _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
   }

   _mesa_unlock_texture(ctx, texObj);
   
   return 1;
}


/** Redirect rendering into stfb's surface to a texture image */
int
st_bind_teximage(struct st_framebuffer *stfb, uint surfIndex,
                 int target, int format, int level)
{
   GET_CURRENT_CONTEXT(ctx);
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   const GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   struct st_texture_image *stImage;
   struct st_renderbuffer *strb;
   GLint face = 0, slice = 0;

   assert(surfIndex <= ST_SURFACE_DEPTH);

   strb = st_renderbuffer(stfb->Base.Attachment[surfIndex].Renderbuffer);

   if (strb->texture_save || strb->surface_save) {
      /* Error! */
      return 0;
   }

   if (target == ST_TEXTURE_2D) {
      texObj = texUnit->CurrentTex[TEXTURE_2D_INDEX];
      texImage = _mesa_get_tex_image(ctx, texObj, GL_TEXTURE_2D, level);
      stImage = st_texture_image(texImage);
   }
   else {
      /* unsupported target */
      return 0;
   }

   st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* save the renderbuffer's surface/texture info */
   pipe_texture_reference(&strb->texture_save, strb->texture);
   pipe_surface_reference(&strb->surface_save, strb->surface);

   /* plug in new surface/texture info */
   pipe_texture_reference(&strb->texture, stImage->pt);
   strb->surface = screen->get_tex_surface(screen, strb->texture,
                                           face, level, slice,
                                           (PIPE_BUFFER_USAGE_GPU_READ |
                                            PIPE_BUFFER_USAGE_GPU_WRITE));

   st->dirty.st |= ST_NEW_FRAMEBUFFER;

   return 1;
}


/** Undo surface-to-texture binding */
int
st_release_teximage(struct st_framebuffer *stfb, uint surfIndex,
                    int target, int format, int level)
{
   GET_CURRENT_CONTEXT(ctx);
   struct st_context *st = ctx->st;
   struct st_renderbuffer *strb;

   assert(surfIndex <= ST_SURFACE_DEPTH);

   strb = st_renderbuffer(stfb->Base.Attachment[surfIndex].Renderbuffer);

   if (!strb->texture_save || !strb->surface_save) {
      /* Error! */
      return 0;
   }

   st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* free tex surface, restore original */
   pipe_surface_reference(&strb->surface, strb->surface_save);
   pipe_texture_reference(&strb->texture, strb->texture_save);

   pipe_surface_reference(&strb->surface_save, NULL);
   pipe_texture_reference(&strb->texture_save, NULL);

   st->dirty.st |= ST_NEW_FRAMEBUFFER;

   return 1;
}

void
st_teximage_flush_before_map(struct st_context *st,
			     struct pipe_texture *pt,
			     unsigned int face,
			     unsigned int level,
			     enum pipe_transfer_usage usage)
{
   struct pipe_context *pipe = st->pipe;
   unsigned referenced =
      pipe->is_texture_referenced(pipe, pt, face, level);

   if (referenced && ((referenced & PIPE_REFERENCED_FOR_WRITE) ||
		      (usage & PIPE_TRANSFER_WRITE)))
      st->pipe->flush(st->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
}
