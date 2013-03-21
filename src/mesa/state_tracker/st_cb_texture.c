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

#include "main/mfeatures.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mipmap.h"
#include "main/pack.h"
#include "main/pbo.h"
#include "main/pixeltransfer.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"

#include "state_tracker/st_debug.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"
#include "state_tracker/st_cb_flush.h"
#include "state_tracker/st_cb_texture.h"
#include "state_tracker/st_cb_bufferobjects.h"
#include "state_tracker/st_format.h"
#include "state_tracker/st_texture.h"
#include "state_tracker/st_gen_mipmap.h"
#include "state_tracker/st_atom.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_tile.h"
#include "util/u_format.h"
#include "util/u_surface.h"
#include "util/u_sampler.h"
#include "util/u_math.h"
#include "util/u_box.h"

#define DBG if (0) printf


enum pipe_texture_target
gl_target_to_pipe(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      return PIPE_TEXTURE_1D;
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
      return PIPE_TEXTURE_2D;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      return PIPE_TEXTURE_RECT;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      return PIPE_TEXTURE_3D;
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return PIPE_TEXTURE_CUBE;
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      return PIPE_TEXTURE_1D_ARRAY;
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      return PIPE_TEXTURE_2D_ARRAY;
   case GL_TEXTURE_BUFFER:
      return PIPE_BUFFER;
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      return PIPE_TEXTURE_CUBE_ARRAY;
   default:
      assert(0);
      return 0;
   }
}


/** called via ctx->Driver.NewTextureImage() */
static struct gl_texture_image *
st_NewTextureImage(struct gl_context * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) ST_CALLOC_STRUCT(st_texture_image);
}


/** called via ctx->Driver.DeleteTextureImage() */
static void
st_DeleteTextureImage(struct gl_context * ctx, struct gl_texture_image *img)
{
   /* nothing special (yet) for st_texture_image */
   _mesa_delete_texture_image(ctx, img);
}


/** called via ctx->Driver.NewTextureObject() */
static struct gl_texture_object *
st_NewTextureObject(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct st_texture_object *obj = ST_CALLOC_STRUCT(st_texture_object);

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

/** called via ctx->Driver.DeleteTextureObject() */
static void 
st_DeleteTextureObject(struct gl_context *ctx,
                       struct gl_texture_object *texObj)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   if (stObj->pt)
      pipe_resource_reference(&stObj->pt, NULL);
   if (stObj->sampler_view) {
      pipe_sampler_view_release(st->pipe, &stObj->sampler_view);
   }
   _mesa_delete_texture_object(ctx, texObj);
}


/** called via ctx->Driver.FreeTextureImageBuffer() */
static void
st_FreeTextureImageBuffer(struct gl_context *ctx,
                          struct gl_texture_image *texImage)
{
   struct st_texture_image *stImage = st_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (stImage->pt) {
      pipe_resource_reference(&stImage->pt, NULL);
   }

   if (stImage->TexData) {
      _mesa_align_free(stImage->TexData);
      stImage->TexData = NULL;
   }
}


/** called via ctx->Driver.MapTextureImage() */
static void
st_MapTextureImage(struct gl_context *ctx,
                   struct gl_texture_image *texImage,
                   GLuint slice, GLuint x, GLuint y, GLuint w, GLuint h,
                   GLbitfield mode,
                   GLubyte **mapOut, GLint *rowStrideOut)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   unsigned pipeMode;
   GLubyte *map;

   pipeMode = 0x0;
   if (mode & GL_MAP_READ_BIT)
      pipeMode |= PIPE_TRANSFER_READ;
   if (mode & GL_MAP_WRITE_BIT)
      pipeMode |= PIPE_TRANSFER_WRITE;
   if (mode & GL_MAP_INVALIDATE_RANGE_BIT)
      pipeMode |= PIPE_TRANSFER_DISCARD_RANGE;

   map = st_texture_image_map(st, stImage, pipeMode, x, y, slice, w, h, 1);
   if (map) {
      *mapOut = map;
      *rowStrideOut = stImage->transfer->stride;
   }
   else {
      *mapOut = NULL;
      *rowStrideOut = 0;
   }
}


/** called via ctx->Driver.UnmapTextureImage() */
static void
st_UnmapTextureImage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
                     GLuint slice)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage  = st_texture_image(texImage);
   st_texture_image_unmap(st, stImage);
}


/**
 * Return default texture resource binding bitmask for the given format.
 */
static GLuint
default_bindings(struct st_context *st, enum pipe_format format)
{
   struct pipe_screen *screen = st->pipe->screen;
   const unsigned target = PIPE_TEXTURE_2D;
   unsigned bindings;

   if (util_format_is_depth_or_stencil(format))
      bindings = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_DEPTH_STENCIL;
   else
      bindings = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;

   if (screen->is_format_supported(screen, format, target, 0, bindings))
      return bindings;
   else {
      /* Try non-sRGB. */
      format = util_format_linear(format);

      if (screen->is_format_supported(screen, format, target, 0, bindings))
         return bindings;
      else
         return PIPE_BIND_SAMPLER_VIEW;
   }
}


/**
 * Given the size of a mipmap image, try to compute the size of the level=0
 * mipmap image.
 *
 * Note that this isn't always accurate for odd-sized, non-POW textures.
 * For example, if level=1 and width=40 then the level=0 width may be 80 or 81.
 *
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static GLboolean
guess_base_level_size(GLenum target,
                      GLuint width, GLuint height, GLuint depth, GLuint level,
                      GLuint *width0, GLuint *height0, GLuint *depth0)
{ 
   assert(width >= 1);
   assert(height >= 1);
   assert(depth >= 1);

   if (level > 0) {
      /* Guess the size of the base level.
       * Depending on the image's size, we can't always make a guess here.
       */
      switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_1D_ARRAY:
         width <<= level;
         break;

      case GL_TEXTURE_2D:
      case GL_TEXTURE_2D_ARRAY:
         /* We can't make a good guess here, because the base level dimensions
          * can be non-square.
          */
         if (width == 1 || height == 1) {
            return GL_FALSE;
         }
         width <<= level;
         height <<= level;
         break;

      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_ARRAY:
         width <<= level;
         height <<= level;
         break;

      case GL_TEXTURE_3D:
         /* We can't make a good guess here, because the base level dimensions
          * can be non-cube.
          */
         if (width == 1 || height == 1 || depth == 1) {
            return GL_FALSE;
         }
         width <<= level;
         height <<= level;
         depth <<= level;
         break;

      case GL_TEXTURE_RECTANGLE:
         break;

      default:
         assert(0);
      }
   }

   *width0 = width;
   *height0 = height;
   *depth0 = depth;

   return GL_TRUE;
}


/**
 * Try to allocate a pipe_resource object for the given st_texture_object.
 *
 * We use the given st_texture_image as a clue to determine the size of the
 * mipmap image at level=0.
 *
 * \return GL_TRUE for success, GL_FALSE if out of memory.
 */
static GLboolean
guess_and_alloc_texture(struct st_context *st,
			struct st_texture_object *stObj,
			const struct st_texture_image *stImage)
{
   GLuint lastLevel, width, height, depth;
   GLuint bindings;
   GLuint ptWidth, ptHeight, ptDepth, ptLayers;
   enum pipe_format fmt;

   DBG("%s\n", __FUNCTION__);

   assert(!stObj->pt);

   if (!guess_base_level_size(stObj->base.Target,
                              stImage->base.Width2,
                              stImage->base.Height2,
                              stImage->base.Depth2,
                              stImage->base.Level,
                              &width, &height, &depth)) {
      /* we can't determine the image size at level=0 */
      stObj->width0 = stObj->height0 = stObj->depth0 = 0;
      /* this is not an out of memory error */
      return GL_TRUE;
   }

   /* At this point, (width x height x depth) is the expected size of
    * the level=0 mipmap image.
    */

   /* Guess a reasonable value for lastLevel.  With OpenGL we have no
    * idea how many mipmap levels will be in a texture until we start
    * to render with it.  Make an educated guess here but be prepared
    * to re-allocating a texture buffer with space for more (or fewer)
    * mipmap levels later.
    */
   if ((stObj->base.Sampler.MinFilter == GL_NEAREST ||
        stObj->base.Sampler.MinFilter == GL_LINEAR ||
        (stObj->base.BaseLevel == 0 &&
         stObj->base.MaxLevel == 0) ||
        stImage->base._BaseFormat == GL_DEPTH_COMPONENT ||
        stImage->base._BaseFormat == GL_DEPTH_STENCIL_EXT) &&
       !stObj->base.GenerateMipmap &&
       stImage->base.Level == 0) {
      /* only alloc space for a single mipmap level */
      lastLevel = 0;
   }
   else {
      /* alloc space for a full mipmap */
      lastLevel = _mesa_get_tex_max_num_levels(stObj->base.Target,
                                               width, height, depth) - 1;
   }

   /* Save the level=0 dimensions */
   stObj->width0 = width;
   stObj->height0 = height;
   stObj->depth0 = depth;

   fmt = st_mesa_format_to_pipe_format(stImage->base.TexFormat);

   bindings = default_bindings(st, fmt);

   st_gl_texture_dims_to_pipe_dims(stObj->base.Target,
                                   width, height, depth,
                                   &ptWidth, &ptHeight, &ptDepth, &ptLayers);

   stObj->pt = st_texture_create(st,
                                 gl_target_to_pipe(stObj->base.Target),
                                 fmt,
                                 lastLevel,
                                 ptWidth,
                                 ptHeight,
                                 ptDepth,
                                 ptLayers,
                                 bindings);

   stObj->lastLevel = lastLevel;

   DBG("%s returning %d\n", __FUNCTION__, (stObj->pt != NULL));

   return stObj->pt != NULL;
}


/**
 * Called via ctx->Driver.AllocTextureImageBuffer().
 * If the texture object/buffer already has space for the indicated image,
 * we're done.  Otherwise, allocate memory for the new texture image.
 */
static GLboolean
st_AllocTextureImageBuffer(struct gl_context *ctx,
                           struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct st_texture_object *stObj = st_texture_object(texImage->TexObject);
   const GLuint level = texImage->Level;
   GLuint width = texImage->Width;
   GLuint height = texImage->Height;
   GLuint depth = texImage->Depth;

   DBG("%s\n", __FUNCTION__);

   assert(!stImage->TexData);
   assert(!stImage->pt); /* xxx this might be wrong */

   /* Look if the parent texture object has space for this image */
   if (stObj->pt &&
       level <= stObj->pt->last_level &&
       st_texture_match_image(stObj->pt, texImage)) {
      /* this image will fit in the existing texture object's memory */
      pipe_resource_reference(&stImage->pt, stObj->pt);
      return GL_TRUE;
   }

   /* The parent texture object does not have space for this image */

   pipe_resource_reference(&stObj->pt, NULL);
   pipe_sampler_view_release(st->pipe, &stObj->sampler_view);

   if (!guess_and_alloc_texture(st, stObj, stImage)) {
      /* Probably out of memory.
       * Try flushing any pending rendering, then retry.
       */
      st_finish(st);
      if (!guess_and_alloc_texture(st, stObj, stImage)) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
         return GL_FALSE;
      }
   }

   if (stObj->pt &&
       st_texture_match_image(stObj->pt, texImage)) {
      /* The image will live in the object's mipmap memory */
      pipe_resource_reference(&stImage->pt, stObj->pt);
      assert(stImage->pt);
      return GL_TRUE;
   }
   else {
      /* Create a new, temporary texture/resource/buffer to hold this
       * one texture image.  Note that when we later access this image
       * (either for mapping or copying) we'll want to always specify
       * mipmap level=0, even if the image represents some other mipmap
       * level.
       */
      enum pipe_format format =
         st_mesa_format_to_pipe_format(texImage->TexFormat);
      GLuint bindings = default_bindings(st, format);
      GLuint ptWidth, ptHeight, ptDepth, ptLayers;

      st_gl_texture_dims_to_pipe_dims(stObj->base.Target,
                                      width, height, depth,
                                      &ptWidth, &ptHeight, &ptDepth, &ptLayers);

      stImage->pt = st_texture_create(st,
                                      gl_target_to_pipe(stObj->base.Target),
                                      format,
                                      0, /* lastLevel */
                                      ptWidth,
                                      ptHeight,
                                      ptDepth,
                                      ptLayers,
                                      bindings);
      return stImage->pt != NULL;
   }
}


/**
 * Preparation prior to glTexImage.  Basically check the 'surface_based'
 * field and switch to a "normal" tex image if necessary.
 */
static void
prep_teximage(struct gl_context *ctx, struct gl_texture_image *texImage,
              GLenum format, GLenum type)
{
   struct gl_texture_object *texObj = texImage->TexObject;
   struct st_texture_object *stObj = st_texture_object(texObj);

   /* switch to "normal" */
   if (stObj->surface_based) {
      const GLenum target = texObj->Target;
      const GLuint level = texImage->Level;
      gl_format texFormat;

      _mesa_clear_texture_object(ctx, texObj);
      pipe_resource_reference(&stObj->pt, NULL);

      /* oops, need to init this image again */
      texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                              texImage->InternalFormat, format,
                                              type);

      _mesa_init_teximage_fields(ctx, texImage,
                                 texImage->Width, texImage->Height,
                                 texImage->Depth, texImage->Border,
                                 texImage->InternalFormat, texFormat);

      stObj->surface_based = GL_FALSE;
   }
}


/**
 * Return a writemask for the gallium blit. The parameters can be base
 * formats or "format" from glDrawPixels/glTexImage/glGetTexImage.
 */
unsigned
st_get_blit_mask(GLenum srcFormat, GLenum dstFormat)
{
   switch (dstFormat) {
   case GL_DEPTH_STENCIL:
      switch (srcFormat) {
      case GL_DEPTH_STENCIL:
         return PIPE_MASK_ZS;
      case GL_DEPTH_COMPONENT:
         return PIPE_MASK_Z;
      case GL_STENCIL_INDEX:
         return PIPE_MASK_S;
      default:
         assert(0);
         return 0;
      }

   case GL_DEPTH_COMPONENT:
      switch (srcFormat) {
      case GL_DEPTH_STENCIL:
      case GL_DEPTH_COMPONENT:
         return PIPE_MASK_Z;
      default:
         assert(0);
         return 0;
      }

   case GL_STENCIL_INDEX:
      switch (srcFormat) {
      case GL_STENCIL_INDEX:
         return PIPE_MASK_S;
      default:
         assert(0);
         return 0;
      }

   default:
      return PIPE_MASK_RGBA;
   }
}


static void
st_TexSubImage(struct gl_context *ctx, GLuint dims,
               struct gl_texture_image *texImage,
               GLint xoffset, GLint yoffset, GLint zoffset,
               GLint width, GLint height, GLint depth,
               GLenum format, GLenum type, const void *pixels,
               const struct gl_pixelstore_attrib *unpack)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct st_texture_object *stObj = st_texture_object(texImage->TexObject);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *dst = stImage->pt;
   struct pipe_resource *src = NULL;
   struct pipe_resource src_templ;
   struct pipe_transfer *transfer;
   struct pipe_blit_info blit;
   enum pipe_format src_format, dst_format;
   gl_format mesa_src_format;
   GLenum gl_target = texImage->TexObject->Target;
   unsigned bind;
   GLubyte *map;

   if (!st->prefer_blit_based_texture_transfer) {
      goto fallback;
   }

   if (!dst) {
      goto fallback;
   }

   /* XXX Fallback for depth-stencil formats due to an incomplete stencil
    * blit implementation in some drivers. */
   if (format == GL_DEPTH_STENCIL) {
      goto fallback;
   }

   /* If the base internal format and the texture format don't match,
    * we can't use blit-based TexSubImage. */
   if (texImage->_BaseFormat !=
       _mesa_get_format_base_format(texImage->TexFormat)) {
      goto fallback;
   }

   /* See if the texture format already matches the format and type,
    * in which case the memcpy-based fast path will likely be used and
    * we don't have to blit. */
   if (_mesa_format_matches_format_and_type(texImage->TexFormat, format,
                                            type, unpack->SwapBytes)) {
      goto fallback;
   }

   if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
      bind = PIPE_BIND_DEPTH_STENCIL;
   else
      bind = PIPE_BIND_RENDER_TARGET;

   /* See if the destination format is supported.
    * For luminance and intensity, only the red channel is stored there. */
   dst_format = util_format_linear(dst->format);
   dst_format = util_format_luminance_to_red(dst_format);
   dst_format = util_format_intensity_to_red(dst_format);

   if (!dst_format ||
       !screen->is_format_supported(screen, dst_format, dst->target,
                                    dst->nr_samples, bind)) {
      goto fallback;
   }

   /* Choose the source format. */
   src_format = st_choose_matching_format(screen, PIPE_BIND_SAMPLER_VIEW,
                                          format, type, unpack->SwapBytes);
   if (!src_format) {
      goto fallback;
   }

   mesa_src_format = st_pipe_format_to_mesa_format(src_format);

   /* There is no reason to do this if we cannot use memcpy for the temporary
    * source texture at least. This also takes transfer ops into account,
    * etc. */
   if (!_mesa_texstore_can_use_memcpy(ctx,
                             _mesa_get_format_base_format(mesa_src_format),
                             mesa_src_format, format, type, unpack)) {
      goto fallback;
   }

   /* TexSubImage only sets a single cubemap face. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
      gl_target = GL_TEXTURE_2D;
   }

   /* Initialize the source texture description. */
   memset(&src_templ, 0, sizeof(src_templ));
   src_templ.target = gl_target_to_pipe(gl_target);
   src_templ.format = src_format;
   src_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   src_templ.usage = PIPE_USAGE_STAGING;

   st_gl_texture_dims_to_pipe_dims(gl_target, width, height, depth,
                                   &src_templ.width0, &src_templ.height0,
                                   &src_templ.depth0, &src_templ.array_size);

   /* Check for NPOT texture support. */
   if (!screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES) &&
       (!util_is_power_of_two(src_templ.width0) ||
        !util_is_power_of_two(src_templ.height0) ||
        !util_is_power_of_two(src_templ.depth0))) {
      goto fallback;
   }

   /* Create the source texture. */
   src = screen->resource_create(screen, &src_templ);
   if (!src) {
      goto fallback;
   }

   /* Map source pixels. */
   pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
                                        format, type, pixels, unpack,
                                        "glTexSubImage");
   if (!pixels) {
      /* This is a GL error. */
      pipe_resource_reference(&src, NULL);
      return;
   }

   /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
      height = 1;
   }

   map = pipe_transfer_map_3d(pipe, src, 0, PIPE_TRANSFER_WRITE, 0, 0, 0,
                              width, height, depth, &transfer);
   if (!map) {
      _mesa_unmap_teximage_pbo(ctx, unpack);
      pipe_resource_reference(&src, NULL);
      goto fallback;
   }

   /* Upload pixels (just memcpy). */
   {
      const uint bytesPerRow = width * util_format_get_blocksize(src_format);
      GLuint row, slice;

      for (slice = 0; slice < depth; slice++) {
         if (gl_target == GL_TEXTURE_1D_ARRAY) {
            /* 1D array textures.
             * We need to convert gallium coords to GL coords.
             */
            GLvoid *src = _mesa_image_address3d(unpack, pixels,
                                                width, depth, format,
                                                type, 0, slice, 0);
            memcpy(map, src, bytesPerRow);
         }
         else {
            ubyte *slice_map = map;

            for (row = 0; row < height; row++) {
               GLvoid *src = _mesa_image_address3d(unpack, pixels,
                                                   width, height, format,
                                                   type, slice, row, 0);
               memcpy(slice_map, src, bytesPerRow);
               slice_map += transfer->stride;
            }
         }
         map += transfer->layer_stride;
      }
   }

   pipe_transfer_unmap(pipe, transfer);
   _mesa_unmap_teximage_pbo(ctx, unpack);

   /* Blit. */
   blit.src.resource = src;
   blit.src.level = 0;
   blit.src.format = src_format;
   blit.dst.resource = dst;
   blit.dst.level = stObj->pt != stImage->pt ? 0 : texImage->Level;
   blit.dst.format = dst_format;
   blit.src.box.x = blit.src.box.y = blit.src.box.z = 0;
   blit.dst.box.x = xoffset;
   blit.dst.box.y = yoffset;
   blit.dst.box.z = zoffset + texImage->Face;
   blit.src.box.width = blit.dst.box.width = width;
   blit.src.box.height = blit.dst.box.height = height;
   blit.src.box.depth = blit.dst.box.depth = depth;
   blit.mask = st_get_blit_mask(format, texImage->_BaseFormat);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
   blit.scissor_enable = FALSE;

   st->pipe->blit(st->pipe, &blit);

   pipe_resource_reference(&src, NULL);
   return;

fallback:
   _mesa_store_texsubimage(ctx, dims, texImage, xoffset, yoffset, zoffset,
                           width, height, depth, format, type, pixels,
                           unpack);
}

static void
st_TexImage(struct gl_context * ctx, GLuint dims,
            struct gl_texture_image *texImage,
            GLenum format, GLenum type, const void *pixels,
            const struct gl_pixelstore_attrib *unpack)
{
   assert(dims == 1 || dims == 2 || dims == 3);

   prep_teximage(ctx, texImage, format, type);

   if (texImage->Width == 0 || texImage->Height == 0 || texImage->Depth == 0)
      return;

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
      return;
   }

   st_TexSubImage(ctx, dims, texImage, 0, 0, 0,
                  texImage->Width, texImage->Height, texImage->Depth,
                  format, type, pixels, unpack);
}


static void
st_CompressedTexImage(struct gl_context *ctx, GLuint dims,
                      struct gl_texture_image *texImage,
                      GLsizei imageSize, const GLvoid *data)
{
   prep_teximage(ctx, texImage, GL_NONE, GL_NONE);
   _mesa_store_compressed_teximage(ctx, dims, texImage, imageSize, data);
}




/**
 * Called via ctx->Driver.GetTexImage()
 *
 * This uses a blit to copy the texture to a texture format which matches
 * the format and type combo and then a fast read-back is done using memcpy.
 * We can do arbitrary X/Y/Z/W/0/1 swizzling here as long as there is
 * a format which matches the swizzling.
 *
 * If such a format isn't available, it falls back to _mesa_get_teximage.
 *
 * NOTE: Drivers usually do a blit to convert between tiled and linear
 *       texture layouts during texture uploads/downloads, so the blit
 *       we do here should be free in such cases.
 */
static void
st_GetTexImage(struct gl_context * ctx,
               GLenum format, GLenum type, GLvoid * pixels,
               struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   GLuint width = texImage->Width;
   GLuint height = texImage->Height;
   GLuint depth = texImage->Depth;
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct pipe_resource *src = st_texture_object(texImage->TexObject)->pt;
   struct pipe_resource *dst = NULL;
   struct pipe_resource dst_templ;
   enum pipe_format dst_format, src_format;
   gl_format mesa_format;
   GLenum gl_target = texImage->TexObject->Target;
   enum pipe_texture_target pipe_target;
   struct pipe_blit_info blit;
   unsigned bind = PIPE_BIND_TRANSFER_READ;
   struct pipe_transfer *tex_xfer;
   ubyte *map = NULL;
   boolean done = FALSE;

   if (!st->prefer_blit_based_texture_transfer) {
      goto fallback;
   }

   if (!stImage->pt || !src) {
      goto fallback;
   }

   /* XXX Fallback to _mesa_get_teximage for depth-stencil formats
    * due to an incomplete stencil blit implementation in some drivers. */
   if (format == GL_DEPTH_STENCIL) {
      goto fallback;
   }

   /* If the base internal format and the texture format don't match, we have
    * to fall back to _mesa_get_teximage. */
   if (texImage->_BaseFormat !=
       _mesa_get_format_base_format(texImage->TexFormat)) {
      goto fallback;
   }

   /* See if the texture format already matches the format and type,
    * in which case the memcpy-based fast path will be used. */
   if (_mesa_format_matches_format_and_type(texImage->TexFormat, format,
                                            type, ctx->Pack.SwapBytes)) {
      goto fallback;
   }

   /* Convert the source format to what is expected by GetTexImage
    * and see if it's supported.
    *
    * This only applies to glGetTexImage:
    * - Luminance must be returned as (L,0,0,1).
    * - Luminance alpha must be returned as (L,0,0,A).
    * - Intensity must be returned as (I,0,0,1)
    */
   src_format = util_format_linear(src->format);
   src_format = util_format_luminance_to_red(src_format);
   src_format = util_format_intensity_to_red(src_format);

   if (!src_format ||
       !screen->is_format_supported(screen, src_format, src->target,
                                    src->nr_samples,
                                    PIPE_BIND_SAMPLER_VIEW)) {
      goto fallback;
   }

   if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
      bind |= PIPE_BIND_DEPTH_STENCIL;
   else
      bind |= PIPE_BIND_RENDER_TARGET;

   /* GetTexImage only returns a single face for cubemaps. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
      gl_target = GL_TEXTURE_2D;
   }
   pipe_target = gl_target_to_pipe(gl_target);

   /* Choose the destination format by finding the best match
    * for the format+type combo. */
   dst_format = st_choose_matching_format(screen, bind, format, type,
					  ctx->Pack.SwapBytes);

   if (dst_format == PIPE_FORMAT_NONE) {
      GLenum dst_glformat;

      /* Fall back to _mesa_get_teximage except for compressed formats,
       * where decompression with a blit is always preferred. */
      if (!util_format_is_compressed(src->format)) {
         goto fallback;
      }

      /* Set the appropriate format for the decompressed texture.
       * Luminance and sRGB formats shouldn't appear here.*/
      switch (src_format) {
      case PIPE_FORMAT_DXT1_RGB:
      case PIPE_FORMAT_DXT1_RGBA:
      case PIPE_FORMAT_DXT3_RGBA:
      case PIPE_FORMAT_DXT5_RGBA:
      case PIPE_FORMAT_RGTC1_UNORM:
      case PIPE_FORMAT_RGTC2_UNORM:
      case PIPE_FORMAT_ETC1_RGB8:
         dst_glformat = GL_RGBA8;
         break;
      case PIPE_FORMAT_RGTC1_SNORM:
      case PIPE_FORMAT_RGTC2_SNORM:
         if (!ctx->Extensions.EXT_texture_snorm)
            goto fallback;
         dst_glformat = GL_RGBA8_SNORM;
         break;
      /* TODO: for BPTC_*FLOAT, set RGBA32F and check for ARB_texture_float */
      default:
         assert(0);
         goto fallback;
      }

      dst_format = st_choose_format(st, dst_glformat, format, type,
                                    pipe_target, 0, bind, FALSE);

      if (dst_format == PIPE_FORMAT_NONE) {
         /* unable to get an rgba format!?! */
         goto fallback;
      }
   }

   /* create the destination texture */
   memset(&dst_templ, 0, sizeof(dst_templ));
   dst_templ.target = pipe_target;
   dst_templ.format = dst_format;
   dst_templ.bind = bind;
   dst_templ.usage = PIPE_USAGE_STAGING;

   st_gl_texture_dims_to_pipe_dims(gl_target, width, height, depth,
                                   &dst_templ.width0, &dst_templ.height0,
                                   &dst_templ.depth0, &dst_templ.array_size);

   dst = screen->resource_create(screen, &dst_templ);
   if (!dst) {
      goto fallback;
   }

   /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
      height = 1;
   }

   blit.src.resource = src;
   blit.src.level = texImage->Level;
   blit.src.format = src_format;
   blit.dst.resource = dst;
   blit.dst.level = 0;
   blit.dst.format = dst->format;
   blit.src.box.x = blit.dst.box.x = 0;
   blit.src.box.y = blit.dst.box.y = 0;
   blit.src.box.z = texImage->Face;
   blit.dst.box.z = 0;
   blit.src.box.width = blit.dst.box.width = width;
   blit.src.box.height = blit.dst.box.height = height;
   blit.src.box.depth = blit.dst.box.depth = depth;
   blit.mask = st_get_blit_mask(texImage->_BaseFormat, format);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
   blit.scissor_enable = FALSE;

   /* blit/render/decompress */
   st->pipe->blit(st->pipe, &blit);

   pixels = _mesa_map_pbo_dest(ctx, &ctx->Pack, pixels);

   map = pipe_transfer_map_3d(pipe, dst, 0, PIPE_TRANSFER_READ,
                              0, 0, 0, width, height, depth, &tex_xfer);
   if (!map) {
      goto end;
   }

   mesa_format = st_pipe_format_to_mesa_format(dst_format);

   /* copy/pack data into user buffer */
   if (_mesa_format_matches_format_and_type(mesa_format, format, type,
                                            ctx->Pack.SwapBytes)) {
      /* memcpy */
      const uint bytesPerRow = width * util_format_get_blocksize(dst_format);
      GLuint row, slice;

      for (slice = 0; slice < depth; slice++) {
         if (gl_target == GL_TEXTURE_1D_ARRAY) {
            /* 1D array textures.
             * We need to convert gallium coords to GL coords.
             */
            GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                 width, depth, format,
                                                 type, 0, slice, 0);
            memcpy(dest, map, bytesPerRow);
         }
         else {
            ubyte *slice_map = map;

            for (row = 0; row < height; row++) {
               GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                    width, height, format,
                                                    type, slice, row, 0);
               memcpy(dest, slice_map, bytesPerRow);
               slice_map += tex_xfer->stride;
            }
         }
         map += tex_xfer->layer_stride;
      }
   }
   else {
      /* format translation via floats */
      GLuint row, slice;
      GLfloat *rgba;

      assert(util_format_is_compressed(src->format));

      rgba = malloc(width * 4 * sizeof(GLfloat));
      if (!rgba) {
         goto end;
      }

      if (ST_DEBUG & DEBUG_FALLBACK)
         debug_printf("%s: fallback format translation\n", __FUNCTION__);

      for (slice = 0; slice < depth; slice++) {
         if (gl_target == GL_TEXTURE_1D_ARRAY) {
            /* 1D array textures.
             * We need to convert gallium coords to GL coords.
             */
            GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                 width, depth, format,
                                                 type, 0, slice, 0);

            /* get float[4] rgba row from surface */
            pipe_get_tile_rgba_format(tex_xfer, map, 0, 0, width, 1,
                                      dst_format, rgba);

            _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                       type, dest, &ctx->Pack, 0);
         }
         else {
            for (row = 0; row < height; row++) {
               GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                    width, height, format,
                                                    type, slice, row, 0);

               /* get float[4] rgba row from surface */
               pipe_get_tile_rgba_format(tex_xfer, map, 0, row, width, 1,
                                         dst_format, rgba);

               _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                          type, dest, &ctx->Pack, 0);
            }
         }
         map += tex_xfer->layer_stride;
      }

      free(rgba);
   }
   done = TRUE;

end:
   if (map)
      pipe_transfer_unmap(pipe, tex_xfer);

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
   pipe_resource_reference(&dst, NULL);

fallback:
   if (!done) {
      _mesa_get_teximage(ctx, format, type, pixels, texImage);
   }
}


/**
 * Do a CopyTexSubImage operation using a read transfer from the source,
 * a write transfer to the destination and get_tile()/put_tile() to access
 * the pixels/texels.
 *
 * Note: srcY=0=TOP of renderbuffer
 */
static void
fallback_copy_texsubimage(struct gl_context *ctx,
                          struct st_renderbuffer *strb,
                          struct st_texture_image *stImage,
                          GLenum baseFormat,
                          GLint destX, GLint destY, GLint destZ,
                          GLint srcX, GLint srcY,
                          GLsizei width, GLsizei height)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer *src_trans;
   GLubyte *texDest;
   enum pipe_transfer_usage transfer_usage;
   void *map;
   unsigned dst_width = width;
   unsigned dst_height = height;
   unsigned dst_depth = 1;

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      srcY = strb->Base.Height - srcY - height;
   }

   if (stImage->pt->target == PIPE_TEXTURE_1D_ARRAY) {
      /* Move y/height to z/depth for 1D array textures.  */
      destZ = destY;
      destY = 0;
      dst_depth = dst_height;
      dst_height = 1;
   }

   map = pipe_transfer_map(pipe,
                           strb->texture,
                           strb->rtt_level,
                           strb->rtt_face + strb->rtt_slice,
                           PIPE_TRANSFER_READ,
                           srcX, srcY,
                           width, height, &src_trans);

   if ((baseFormat == GL_DEPTH_COMPONENT ||
        baseFormat == GL_DEPTH_STENCIL) &&
       util_format_is_depth_and_stencil(stImage->pt->format))
      transfer_usage = PIPE_TRANSFER_READ_WRITE;
   else
      transfer_usage = PIPE_TRANSFER_WRITE;

   texDest = st_texture_image_map(st, stImage, transfer_usage,
                                  destX, destY, destZ,
                                  dst_width, dst_height, dst_depth);

   if (baseFormat == GL_DEPTH_COMPONENT ||
       baseFormat == GL_DEPTH_STENCIL) {
      const GLboolean scaleOrBias = (ctx->Pixel.DepthScale != 1.0F ||
                                     ctx->Pixel.DepthBias != 0.0F);
      GLint row, yStep;
      uint *data;

      /* determine bottom-to-top vs. top-to-bottom order for src buffer */
      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         srcY = height - 1;
         yStep = -1;
      }
      else {
         srcY = 0;
         yStep = 1;
      }

      data = malloc(width * sizeof(uint));

      if (data) {
         /* To avoid a large temp memory allocation, do copy row by row */
         for (row = 0; row < height; row++, srcY += yStep) {
            pipe_get_tile_z(src_trans, map, 0, srcY, width, 1, data);
            if (scaleOrBias) {
               _mesa_scale_and_bias_depth_uint(ctx, width, data);
            }

            if (stImage->pt->target == PIPE_TEXTURE_1D_ARRAY) {
               pipe_put_tile_z(stImage->transfer,
                               texDest + row*stImage->transfer->layer_stride,
                               0, 0, width, 1, data);
            }
            else {
               pipe_put_tile_z(stImage->transfer, texDest, 0, row, width, 1,
                               data);
            }
         }
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage()");
      }

      free(data);
   }
   else {
      /* RGBA format */
      GLfloat *tempSrc =
         malloc(width * height * 4 * sizeof(GLfloat));

      if (tempSrc && texDest) {
         const GLint dims = 2;
         GLint dstRowStride;
         struct gl_texture_image *texImage = &stImage->base;
         struct gl_pixelstore_attrib unpack = ctx->DefaultPacking;

         if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
            unpack.Invert = GL_TRUE;
         }

         if (stImage->pt->target == PIPE_TEXTURE_1D_ARRAY) {
            dstRowStride = stImage->transfer->layer_stride;
         }
         else {
            dstRowStride = stImage->transfer->stride;
         }

         /* get float/RGBA image from framebuffer */
         /* XXX this usually involves a lot of int/float conversion.
          * try to avoid that someday.
          */
         pipe_get_tile_rgba_format(src_trans, map, 0, 0, width, height,
                                   util_format_linear(strb->texture->format),
                                   tempSrc);

         /* Store into texture memory.
          * Note that this does some special things such as pixel transfer
          * ops and format conversion.  In particular, if the dest tex format
          * is actually RGBA but the user created the texture as GL_RGB we
          * need to fill-in/override the alpha channel with 1.0.
          */
         _mesa_texstore(ctx, dims,
                        texImage->_BaseFormat, 
                        texImage->TexFormat, 
                        dstRowStride,
                        &texDest,
                        width, height, 1,
                        GL_RGBA, GL_FLOAT, tempSrc, /* src */
                        &unpack);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
      }

      free(tempSrc);
   }

   st_texture_image_unmap(st, stImage);
   pipe->transfer_unmap(pipe, src_trans);
}


/**
 * Do a CopyTex[Sub]Image1/2/3D() using a hardware (blit) path if possible.
 * Note that the region to copy has already been clipped so we know we
 * won't read from outside the source renderbuffer's bounds.
 *
 * Note: srcY=0=Bottom of renderbuffer (GL convention)
 */
static void
st_CopyTexSubImage(struct gl_context *ctx, GLuint dims,
                   struct gl_texture_image *texImage,
                   GLint destX, GLint destY, GLint destZ,
                   struct gl_renderbuffer *rb,
                   GLint srcX, GLint srcY, GLsizei width, GLsizei height)
{
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct st_texture_object *stObj = st_texture_object(texImage->TexObject);
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_blit_info blit;
   enum pipe_format dst_format;
   GLboolean do_flip = (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP);
   unsigned bind;
   GLint srcY0, srcY1, yStep;

   if (!strb || !strb->surface || !stImage->pt) {
      debug_printf("%s: null strb or stImage\n", __FUNCTION__);
      return;
   }

   if (_mesa_texstore_needs_transfer_ops(ctx, texImage->_BaseFormat,
                                         texImage->TexFormat)) {
      goto fallback;
   }

   /* The base internal format must match the mesa format, so make sure
    * e.g. an RGB internal format is really allocated as RGB and not as RGBA.
    */
   if (texImage->_BaseFormat !=
       _mesa_get_format_base_format(texImage->TexFormat) ||
       rb->_BaseFormat != _mesa_get_format_base_format(rb->Format)) {
      goto fallback;
   }

   /* Choose the destination format to match the TexImage behavior. */
   dst_format = util_format_linear(stImage->pt->format);
   dst_format = util_format_luminance_to_red(dst_format);
   dst_format = util_format_intensity_to_red(dst_format);

   /* See if the destination format is supported. */
   if (texImage->_BaseFormat == GL_DEPTH_STENCIL ||
       texImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      bind = PIPE_BIND_DEPTH_STENCIL;
   }
   else {
      bind = PIPE_BIND_RENDER_TARGET;
   }

   if (!dst_format ||
       !screen->is_format_supported(screen, dst_format, stImage->pt->target,
                                    stImage->pt->nr_samples, bind)) {
      goto fallback;
   }

   /* Y flipping for the main framebuffer. */
   if (do_flip) {
      srcY1 = strb->Base.Height - srcY - height;
      srcY0 = srcY1 + height;
      yStep = -1;
   }
   else {
      srcY0 = srcY;
      srcY1 = srcY0 + height;
      yStep = 1;
   }

   /* Blit the texture.
    * This supports flipping, format conversions, and downsampling.
    */
   memset(&blit, 0, sizeof(blit));
   blit.src.resource = strb->texture;
   blit.src.format = util_format_linear(strb->surface->format);
   blit.src.level = strb->surface->u.tex.level;
   blit.src.box.x = srcX;
   blit.src.box.y = srcY0;
   blit.src.box.z = strb->surface->u.tex.first_layer;
   blit.src.box.width = width;
   blit.src.box.height = srcY1 - srcY0;
   blit.src.box.depth = 1;
   blit.dst.resource = stImage->pt;
   blit.dst.format = dst_format;
   blit.dst.level = stObj->pt != stImage->pt ? 0 : texImage->Level;
   blit.dst.box.x = destX;
   blit.dst.box.y = destY;
   blit.dst.box.z = stImage->base.Face + destZ;
   blit.dst.box.width = width;
   blit.dst.box.height = height;
   blit.dst.box.depth = 1;
   blit.mask = st_get_blit_mask(rb->_BaseFormat, texImage->_BaseFormat);
   blit.filter = PIPE_TEX_FILTER_NEAREST;

   /* 1D array textures need special treatment.
    * Blit rows from the source to layers in the destination. */
   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY) {
      int y, layer;

      for (y = srcY0, layer = 0; layer < height; y += yStep, layer++) {
         blit.src.box.y = y;
         blit.src.box.height = 1;
         blit.dst.box.y = 0;
         blit.dst.box.height = 1;
         blit.dst.box.z = destY + layer;

         pipe->blit(pipe, &blit);
      }
   }
   else {
      /* All the other texture targets. */
      pipe->blit(pipe, &blit);
   }
   return;

fallback:
   /* software fallback */
   fallback_copy_texsubimage(ctx,
                             strb, stImage, texImage->_BaseFormat,
                             destX, destY, destZ,
                             srcX, srcY, width, height);
}


/**
 * Copy image data from stImage into the texture object 'stObj' at level
 * 'dstLevel'.
 */
static void
copy_image_data_to_texture(struct st_context *st,
			   struct st_texture_object *stObj,
                           GLuint dstLevel,
			   struct st_texture_image *stImage)
{
   /* debug checks */
   {
      const struct gl_texture_image *dstImage =
         stObj->base.Image[stImage->base.Face][dstLevel];
      assert(dstImage);
      assert(dstImage->Width == stImage->base.Width);
      assert(dstImage->Height == stImage->base.Height);
      assert(dstImage->Depth == stImage->base.Depth);
   }

   if (stImage->pt) {
      /* Copy potentially with the blitter:
       */
      GLuint src_level;
      if (stImage->pt->last_level == 0)
         src_level = 0;
      else
         src_level = stImage->base.Level;

      assert(src_level <= stImage->pt->last_level);
      assert(u_minify(stImage->pt->width0, src_level) == stImage->base.Width);
      assert(stImage->pt->target == PIPE_TEXTURE_1D_ARRAY ||
             u_minify(stImage->pt->height0, src_level) == stImage->base.Height);
      assert(stImage->pt->target == PIPE_TEXTURE_2D_ARRAY ||
             stImage->pt->target == PIPE_TEXTURE_CUBE_ARRAY ||
             u_minify(stImage->pt->depth0, src_level) == stImage->base.Depth);

      st_texture_image_copy(st->pipe,
                            stObj->pt, dstLevel,  /* dest texture, level */
                            stImage->pt, src_level, /* src texture, level */
                            stImage->base.Face);

      pipe_resource_reference(&stImage->pt, NULL);
   }
   else if (stImage->TexData) {
      /* Copy from malloc'd memory */
      /* XXX this should be re-examined/tested with a compressed format */
      GLuint blockSize = util_format_get_blocksize(stObj->pt->format);
      GLuint srcRowStride = stImage->base.Width * blockSize;
      GLuint srcSliceStride = stImage->base.Height * srcRowStride;
      st_texture_image_data(st,
                            stObj->pt,
                            stImage->base.Face,
                            dstLevel,
                            stImage->TexData,
                            srcRowStride,
                            srcSliceStride);
      _mesa_align_free(stImage->TexData);
      stImage->TexData = NULL;
   }

   pipe_resource_reference(&stImage->pt, stObj->pt);
}


/**
 * Called during state validation.  When this function is finished,
 * the texture object should be ready for rendering.
 * \return GL_TRUE for success, GL_FALSE for failure (out of mem)
 */
GLboolean
st_finalize_texture(struct gl_context *ctx,
		    struct pipe_context *pipe,
		    struct gl_texture_object *tObj)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(tObj);
   const GLuint nr_faces = (stObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face;
   struct st_texture_image *firstImage;
   enum pipe_format firstImageFormat;
   GLuint ptWidth, ptHeight, ptDepth, ptLayers;

   if (_mesa_is_texture_complete(tObj, &tObj->Sampler)) {
      /* The texture is complete and we know exactly how many mipmap levels
       * are present/needed.  This is conditional because we may be called
       * from the st_generate_mipmap() function when the texture object is
       * incomplete.  In that case, we'll have set stObj->lastLevel before
       * we get here.
       */
      if (stObj->base.Sampler.MinFilter == GL_LINEAR ||
          stObj->base.Sampler.MinFilter == GL_NEAREST)
         stObj->lastLevel = stObj->base.BaseLevel;
      else
         stObj->lastLevel = stObj->base._MaxLevel;
   }

   if (tObj->Target == GL_TEXTURE_BUFFER) {
      struct st_buffer_object *st_obj = st_buffer_object(tObj->BufferObject);

      if (st_obj->buffer != stObj->pt) {
         pipe_resource_reference(&stObj->pt, st_obj->buffer);
         pipe_sampler_view_release(st->pipe, &stObj->sampler_view);
         stObj->width0 = stObj->pt->width0 / _mesa_get_format_bytes(tObj->_BufferObjectFormat);
         stObj->height0 = 1;
         stObj->depth0 = 1;
      }
      return GL_TRUE;

   }

   firstImage = st_texture_image(stObj->base.Image[0][stObj->base.BaseLevel]);
   assert(firstImage);

   /* If both firstImage and stObj point to a texture which can contain
    * all active images, favour firstImage.  Note that because of the
    * completeness requirement, we know that the image dimensions
    * will match.
    */
   if (firstImage->pt &&
       firstImage->pt != stObj->pt &&
       (!stObj->pt || firstImage->pt->last_level >= stObj->pt->last_level)) {
      pipe_resource_reference(&stObj->pt, firstImage->pt);
      pipe_sampler_view_release(st->pipe, &stObj->sampler_view);
   }

   /* Find gallium format for the Mesa texture */
   firstImageFormat = st_mesa_format_to_pipe_format(firstImage->base.TexFormat);

   /* Find size of level=0 Gallium mipmap image, plus number of texture layers */
   {
      GLuint width, height, depth;
      if (!guess_base_level_size(stObj->base.Target,
                                 firstImage->base.Width2,
                                 firstImage->base.Height2,
                                 firstImage->base.Depth2,
                                 firstImage->base.Level,
                                 &width, &height, &depth)) {
         width = stObj->width0;
         height = stObj->height0;
         depth = stObj->depth0;
      }
      /* convert GL dims to Gallium dims */
      st_gl_texture_dims_to_pipe_dims(stObj->base.Target, width, height, depth,
                                      &ptWidth, &ptHeight, &ptDepth, &ptLayers);
   }

   /* If we already have a gallium texture, check that it matches the texture
    * object's format, target, size, num_levels, etc.
    */
   if (stObj->pt) {
      if (stObj->pt->target != gl_target_to_pipe(stObj->base.Target) ||
          !st_sampler_compat_formats(stObj->pt->format, firstImageFormat) ||
          stObj->pt->last_level < stObj->lastLevel ||
          stObj->pt->width0 != ptWidth ||
          stObj->pt->height0 != ptHeight ||
          stObj->pt->depth0 != ptDepth ||
          stObj->pt->array_size != ptLayers)
      {
         /* The gallium texture does not match the Mesa texture so delete the
          * gallium texture now.  We'll make a new one below.
          */
         pipe_resource_reference(&stObj->pt, NULL);
         pipe_sampler_view_release(st->pipe, &stObj->sampler_view);
         st->dirty.st |= ST_NEW_FRAMEBUFFER;
      }
   }

   /* May need to create a new gallium texture:
    */
   if (!stObj->pt) {
      GLuint bindings = default_bindings(st, firstImageFormat);

      stObj->pt = st_texture_create(st,
                                    gl_target_to_pipe(stObj->base.Target),
                                    firstImageFormat,
                                    stObj->lastLevel,
                                    ptWidth,
                                    ptHeight,
                                    ptDepth,
                                    ptLayers,
                                    bindings);

      if (!stObj->pt) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
         return GL_FALSE;
      }
   }

   /* Pull in any images not in the object's texture:
    */
   for (face = 0; face < nr_faces; face++) {
      GLuint level;
      for (level = stObj->base.BaseLevel; level <= stObj->lastLevel; level++) {
         struct st_texture_image *stImage =
            st_texture_image(stObj->base.Image[face][level]);

         /* Need to import images in main memory or held in other textures.
          */
         if (stImage && stObj->pt != stImage->pt) {
            if (level == 0 ||
                (stImage->base.Width == u_minify(stObj->width0, level) &&
                 stImage->base.Height == u_minify(stObj->height0, level) &&
                 stImage->base.Depth == u_minify(stObj->depth0, level))) {
               /* src image fits expected dest mipmap level size */
               copy_image_data_to_texture(st, stObj, level, stImage);
            }
         }
      }
   }

   return GL_TRUE;
}


/**
 * Called via ctx->Driver.AllocTextureStorage() to allocate texture memory
 * for a whole mipmap stack.
 */
static GLboolean
st_AllocTextureStorage(struct gl_context *ctx,
                       struct gl_texture_object *texObj,
                       GLsizei levels, GLsizei width,
                       GLsizei height, GLsizei depth)
{
   const GLuint numFaces = _mesa_num_tex_faces(texObj->Target);
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   GLuint ptWidth, ptHeight, ptDepth, ptLayers, bindings;
   enum pipe_format fmt;
   GLint level;

   assert(levels > 0);

   /* Save the level=0 dimensions */
   stObj->width0 = width;
   stObj->height0 = height;
   stObj->depth0 = depth;
   stObj->lastLevel = levels - 1;

   fmt = st_mesa_format_to_pipe_format(texObj->Image[0][0]->TexFormat);

   bindings = default_bindings(st, fmt);

   st_gl_texture_dims_to_pipe_dims(texObj->Target,
                                   width, height, depth,
                                   &ptWidth, &ptHeight, &ptDepth, &ptLayers);

   stObj->pt = st_texture_create(st,
                                 gl_target_to_pipe(texObj->Target),
                                 fmt,
                                 levels,
                                 ptWidth,
                                 ptHeight,
                                 ptDepth,
                                 ptLayers,
                                 bindings);
   if (!stObj->pt)
      return GL_FALSE;

   /* Set image resource pointers */
   for (level = 0; level < levels; level++) {
      GLuint face;
      for (face = 0; face < numFaces; face++) {
         struct st_texture_image *stImage =
            st_texture_image(texObj->Image[face][level]);
         pipe_resource_reference(&stImage->pt, stObj->pt);
      }
   }

   return GL_TRUE;
}


static GLboolean
st_TestProxyTexImage(struct gl_context *ctx, GLenum target,
                     GLint level, gl_format format,
                     GLint width, GLint height,
                     GLint depth, GLint border)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;

   if (width == 0 || height == 0 || depth == 0) {
      /* zero-sized images are legal, and always fit! */
      return GL_TRUE;
   }

   if (pipe->screen->can_create_resource) {
      /* Ask the gallium driver if the texture is too large */
      struct gl_texture_object *texObj =
         _mesa_get_current_tex_object(ctx, target);
      struct pipe_resource pt;

      /* Setup the pipe_resource object
       */
      memset(&pt, 0, sizeof(pt));

      pt.target = gl_target_to_pipe(target);
      pt.format = st_mesa_format_to_pipe_format(format);

      st_gl_texture_dims_to_pipe_dims(target,
                                      width, height, depth,
                                      &pt.width0, &pt.height0,
                                      &pt.depth0, &pt.array_size);

      if (level == 0 && (texObj->Sampler.MinFilter == GL_LINEAR ||
                         texObj->Sampler.MinFilter == GL_NEAREST)) {
         /* assume just one mipmap level */
         pt.last_level = 0;
      }
      else {
         /* assume a full set of mipmaps */
         pt.last_level = _mesa_logbase2(MAX3(width, height, depth));
      }

      return pipe->screen->can_create_resource(pipe->screen, &pt);
   }
   else {
      /* Use core Mesa fallback */
      return _mesa_test_proxy_teximage(ctx, target, level, format,
                                       width, height, depth, border);
   }
}


void
st_init_texture_functions(struct dd_function_table *functions)
{
   functions->ChooseTextureFormat = st_ChooseTextureFormat;
   functions->QuerySamplesForFormat = st_QuerySamplesForFormat;
   functions->TexImage = st_TexImage;
   functions->TexSubImage = st_TexSubImage;
   functions->CompressedTexSubImage = _mesa_store_compressed_texsubimage;
   functions->CopyTexSubImage = st_CopyTexSubImage;
   functions->GenerateMipmap = st_generate_mipmap;

   functions->GetTexImage = st_GetTexImage;

   /* compressed texture functions */
   functions->CompressedTexImage = st_CompressedTexImage;
   functions->GetCompressedTexImage = _mesa_get_compressed_teximage;

   functions->NewTextureObject = st_NewTextureObject;
   functions->NewTextureImage = st_NewTextureImage;
   functions->DeleteTextureImage = st_DeleteTextureImage;
   functions->DeleteTexture = st_DeleteTextureObject;
   functions->AllocTextureImageBuffer = st_AllocTextureImageBuffer;
   functions->FreeTextureImageBuffer = st_FreeTextureImageBuffer;
   functions->MapTextureImage = st_MapTextureImage;
   functions->UnmapTextureImage = st_UnmapTextureImage;

   /* XXX Temporary until we can query pipe's texture sizes */
   functions->TestProxyTexImage = st_TestProxyTexImage;

   functions->AllocTextureStorage = st_AllocTextureStorage;
}
