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
#include "util/u_blit.h"
#include "util/u_format.h"
#include "util/u_surface.h"
#include "util/u_sampler.h"
#include "util/u_math.h"
#include "util/u_box.h"

#define DBG if (0) printf


static enum pipe_texture_target
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


static void
st_TexImage(struct gl_context * ctx, GLuint dims,
            struct gl_texture_image *texImage,
            GLenum format, GLenum type, const void *pixels,
            const struct gl_pixelstore_attrib *unpack)
{
   prep_teximage(ctx, texImage, format, type);
   _mesa_store_teximage(ctx, dims, texImage, format, type, pixels, unpack);
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
 * glGetTexImage() helper: decompress a compressed texture by rendering
 * a textured quad.  Store the results in the user's buffer.
 */
static void
decompress_with_blit(struct gl_context * ctx,
                     GLenum format, GLenum type, GLvoid *pixels,
                     struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   const GLuint depth = texImage->Depth;
   struct pipe_resource *src = st_texture_object(texImage->TexObject)->pt;
   struct pipe_resource *dst;
   struct pipe_resource dst_templ;
   enum pipe_format pipe_format;
   gl_format mesa_format;
   GLenum gl_target = texImage->TexObject->Target;
   enum pipe_texture_target pipe_target;
   struct pipe_blit_info blit;
   unsigned bind = (PIPE_BIND_RENDER_TARGET | PIPE_BIND_TRANSFER_READ);
   struct pipe_transfer *tex_xfer;
   ubyte *map;

   /* GetTexImage only returns a single face for cubemaps. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
      gl_target = GL_TEXTURE_2D;
   }

   pipe_target = gl_target_to_pipe(gl_target);

   /* Find the best match for the format+type combo. */
   pipe_format = st_choose_format(pipe->screen, GL_RGBA8, format, type,
                                  pipe_target, 0, bind);
   if (pipe_format == PIPE_FORMAT_NONE) {
      /* unable to get an rgba format!?! */
      _mesa_problem(ctx, "%s: cannot find a supported format", __func__);
      return;
   }

   /* create the destination texture */
   memset(&dst_templ, 0, sizeof(dst_templ));
   dst_templ.target = pipe_target;
   dst_templ.format = pipe_format;
   dst_templ.bind = bind;
   dst_templ.usage = PIPE_USAGE_STAGING;

   st_gl_texture_dims_to_pipe_dims(gl_target, width, height, depth,
                                   &dst_templ.width0, &dst_templ.height0,
                                   &dst_templ.depth0, &dst_templ.array_size);

   dst = screen->resource_create(screen, &dst_templ);
   if (!dst) {
      _mesa_problem(ctx, "%s: cannot create a temporary texture", __func__);
      return;
   }

   blit.src.resource = src;
   blit.src.level = texImage->Level;
   blit.src.format = util_format_linear(src->format);
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
   blit.mask = PIPE_MASK_RGBA;
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

   mesa_format = st_pipe_format_to_mesa_format(pipe_format);

   /* copy/pack data into user buffer */
   if (_mesa_format_matches_format_and_type(mesa_format, format, type,
                                            ctx->Pack.SwapBytes)) {
      /* memcpy */
      const uint bytesPerRow = width * util_format_get_blocksize(pipe_format);
      GLuint row, slice;

      for (slice = 0; slice < depth; slice++) {
         ubyte *slice_map = map;

         for (row = 0; row < height; row++) {
            GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                 width, height, format,
                                                 type, slice, row, 0);
            memcpy(dest, slice_map, bytesPerRow);
            slice_map += tex_xfer->stride;
         }
         map += tex_xfer->layer_stride;
      }
   }
   else {
      /* format translation via floats */
      GLuint row, slice;
      enum pipe_format pformat = util_format_linear(dst->format);
      GLfloat *rgba;

      rgba = malloc(width * 4 * sizeof(GLfloat));
      if (!rgba) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage()");
         goto end;
      }

      for (slice = 0; slice < depth; slice++) {
         for (row = 0; row < height; row++) {
            const GLbitfield transferOps = 0x0; /* bypassed for glGetTexImage() */
            GLvoid *dest = _mesa_image_address3d(&ctx->Pack, pixels,
                                                 width, height, format,
                                                 type, slice, row, 0);

            if (ST_DEBUG & DEBUG_FALLBACK)
               debug_printf("%s: fallback format translation\n", __FUNCTION__);

            /* get float[4] rgba row from surface */
            pipe_get_tile_rgba_format(tex_xfer, map, 0, row, width, 1,
                                      pformat, rgba);

            _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                       type, dest, &ctx->Pack, transferOps);
         }
         map += tex_xfer->layer_stride;
      }

      free(rgba);
   }

end:
   if (map)
      pipe_transfer_unmap(pipe, tex_xfer);

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
   pipe_resource_reference(&dst, NULL);
}



/**
 * Called via ctx->Driver.GetTexImage()
 */
static void
st_GetTexImage(struct gl_context * ctx,
               GLenum format, GLenum type, GLvoid * pixels,
               struct gl_texture_image *texImage)
{
   struct st_texture_image *stImage = st_texture_image(texImage);

   if (stImage->pt && util_format_is_s3tc(stImage->pt->format)) {
      /* Need to decompress the texture.
       * We'll do this by rendering a textured quad (which is hopefully
       * faster than using the fallback code in texcompress.c).
       * Note that we only expect RGBA formats (no Z/depth formats).
       */
      decompress_with_blit(ctx, format, type, pixels, texImage);
   }
   else {
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
 * If the format of the src renderbuffer and the format of the dest
 * texture are compatible (in terms of blitting), return a TGSI writemask
 * to be used during the blit.
 * If the src/dest are incompatible, return 0.
 */
static unsigned
compatible_src_dst_formats(struct gl_context *ctx,
                           const struct gl_renderbuffer *src,
                           const struct gl_texture_image *dst)
{
   /* Get logical base formats for the src and dest.
    * That is, use the user-requested formats and not the actual, device-
    * chosen formats.
    * For example, the user may have requested an A8 texture but the
    * driver may actually be using an RGBA texture format.  When we
    * copy/blit to that texture, we only want to copy the Alpha channel
    * and not the RGB channels.
    *
    * Similarly, when the src FBO was created an RGB format may have been
    * requested but the driver actually chose an RGBA format.  In that case,
    * we don't want to copy the undefined Alpha channel to the dest texture
    * (it should be 1.0).
    */
   const GLenum srcFormat = _mesa_base_fbo_format(ctx, src->InternalFormat);
   const GLenum dstFormat = _mesa_base_tex_format(ctx, dst->InternalFormat);

   /**
    * XXX when we have red-only and red/green renderbuffers we'll need
    * to add more cases here (or implement a general-purpose routine that
    * queries the existance of the R,G,B,A channels in the src and dest).
    */
   if (srcFormat == dstFormat) {
      /* This is the same as matching_base_formats, which should
       * always pass, as it did previously.
       */
      return TGSI_WRITEMASK_XYZW;
   }
   else if (srcFormat == GL_RGB && dstFormat == GL_RGBA) {
      /* Make sure that A in the dest is 1.  The actual src format
       * may be RGBA and have undefined A values.
       */
      return TGSI_WRITEMASK_XYZ;
   }
   else if (srcFormat == GL_RGBA && dstFormat == GL_RGB) {
      /* Make sure that A in the dest is 1.  The actual dst format
       * may be RGBA and will need A=1 to provide proper alpha values
       * when sampled later.
       */
      return TGSI_WRITEMASK_XYZ;
   }
   else {
      if (ST_DEBUG & DEBUG_FALLBACK)
         debug_printf("%s failed for src %s, dst %s\n",
                      __FUNCTION__, 
                      _mesa_lookup_enum_by_nr(srcFormat),
                      _mesa_lookup_enum_by_nr(dstFormat));

      /* Otherwise fail.
       */
      return 0;
   }
}


/**
 * Do pipe->blit. Return FALSE if the blitting is unsupported
 * for the given formats.
 */
static GLboolean
st_pipe_blit(struct pipe_context *pipe, struct pipe_blit_info *blit)
{
   struct pipe_screen *screen = pipe->screen;
   unsigned dst_usage;

   if (util_format_is_depth_or_stencil(blit->dst.format)) {
      dst_usage = PIPE_BIND_DEPTH_STENCIL;
   }
   else {
      dst_usage = PIPE_BIND_RENDER_TARGET;
   }

   /* try resource_copy_region in case the format is not supported
    * for rendering */
   if (util_try_blit_via_copy_region(pipe, blit)) {
      return GL_TRUE; /* done */
   }

   /* check the format support */
   if (!screen->is_format_supported(screen, blit->src.format,
                                    PIPE_TEXTURE_2D, 0,
                                    PIPE_BIND_SAMPLER_VIEW) ||
       !screen->is_format_supported(screen, blit->dst.format,
                                    PIPE_TEXTURE_2D, 0,
                                    dst_usage)) {
      return GL_FALSE;
   }

   pipe->blit(pipe, blit);
   return GL_TRUE;
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
   const GLenum texBaseFormat = texImage->_BaseFormat;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   enum pipe_format dest_format, src_format;
   GLuint color_writemask;
   struct pipe_surface *dest_surface = NULL;
   GLboolean do_flip = (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP);
   struct pipe_surface surf_tmpl;
   unsigned dst_usage;
   unsigned blit_mask;
   GLint srcY0, srcY1, yStep;

   /* make sure finalize_textures has been called? 
    */
   if (0) st_validate_state(st);

   if (!strb || !strb->surface || !stImage->pt) {
      debug_printf("%s: null strb or stImage\n", __FUNCTION__);
      return;
   }

   assert(strb);
   assert(strb->surface);
   assert(stImage->pt);

   src_format = strb->surface->format;
   dest_format = stImage->pt->format;

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

   if (ctx->_ImageTransferState) {
      goto fallback;
   }

   /* Compressed and subsampled textures aren't supported for blitting. */
   if (!util_format_is_plain(dest_format)) {
      goto fallback;
   }

   /* Set the blit writemask. */
   switch (texBaseFormat) {
   case GL_DEPTH_STENCIL:
      switch (strb->Base._BaseFormat) {
      case GL_DEPTH_STENCIL:
         blit_mask = PIPE_MASK_ZS;
         break;
      case GL_DEPTH_COMPONENT:
         blit_mask = PIPE_MASK_Z;
         break;
      case GL_STENCIL_INDEX:
         blit_mask = PIPE_MASK_S;
         break;
      default:
         assert(0);
         return;
      }
      dst_usage = PIPE_BIND_DEPTH_STENCIL;
      break;

   case GL_DEPTH_COMPONENT:
      blit_mask = PIPE_MASK_Z;
      dst_usage = PIPE_BIND_DEPTH_STENCIL;
      break;

   default:
      /* Colorbuffers.
       *
       * Determine if the src framebuffer and dest texture have the same
       * base format.  We need this to detect a case such as the framebuffer
       * being GL_RGBA but the texture being GL_RGB.  If the actual hardware
       * texture format stores RGBA we need to set A=1 (overriding the
       * framebuffer's alpha values).
       *
       * XXX util_blit_pixels doesn't support MSAA resolve, so always use
       *     pipe->blit
       */
      if (texBaseFormat == strb->Base._BaseFormat ||
          strb->texture->nr_samples > 1) {
         blit_mask = PIPE_MASK_RGBA;
      }
      else {
         blit_mask = 0;
      }
      dst_usage = PIPE_BIND_RENDER_TARGET;
   }

   /* Blit the texture.
    * This supports flipping, format conversions, and downsampling.
    */
   if (blit_mask) {
      /* If stImage->pt is an independent image (not a pointer into a full
       * mipmap) stImage->pt.last_level will be zero and we need to use that
       * as the dest level.
       */
      unsigned dstLevel = MIN2(stImage->base.Level, stImage->pt->last_level);
      struct pipe_blit_info blit;

      memset(&blit, 0, sizeof(blit));
      blit.src.resource = strb->texture;
      blit.src.format = src_format;
      blit.src.level = strb->surface->u.tex.level;
      blit.src.box.x = srcX;
      blit.src.box.y = srcY0;
      blit.src.box.z = strb->surface->u.tex.first_layer;
      blit.src.box.width = width;
      blit.src.box.height = srcY1 - srcY0;
      blit.src.box.depth = 1;
      blit.dst.resource = stImage->pt;
      blit.dst.format = dest_format;
      blit.dst.level = dstLevel;
      blit.dst.box.x = destX;
      blit.dst.box.y = destY;
      blit.dst.box.z = stImage->base.Face + destZ;
      blit.dst.box.width = width;
      blit.dst.box.height = height;
      blit.dst.box.depth = 1;
      blit.mask = blit_mask;
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

            if (!st_pipe_blit(pipe, &blit)) {
               goto fallback;
            }
         }
      }
      else {
         /* All the other texture targets. */
         if (!st_pipe_blit(pipe, &blit)) {
            goto fallback;
         }
      }
      return;
   }

   /* try u_blit */
   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY) {
      /* u_blit cannot copy 1D array textures as required by CopyTexSubImage */
      goto fallback;
   }

   color_writemask = compatible_src_dst_formats(ctx, &strb->Base, texImage);

   if (!color_writemask ||
       !screen->is_format_supported(screen, src_format,
                                    PIPE_TEXTURE_2D, 0,
                                    PIPE_BIND_SAMPLER_VIEW) ||
       !screen->is_format_supported(screen, dest_format,
                                    PIPE_TEXTURE_2D, 0,
                                    dst_usage)) {
      goto fallback;
   }

   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = util_format_linear(stImage->pt->format);
   surf_tmpl.u.tex.level = stImage->base.Level;
   surf_tmpl.u.tex.first_layer = stImage->base.Face + destZ;
   surf_tmpl.u.tex.last_layer = stImage->base.Face + destZ;

   dest_surface = pipe->create_surface(pipe, stImage->pt,
                                       &surf_tmpl);
   util_blit_pixels(st->blit,
                    strb->texture,
                    strb->surface->u.tex.level,
                    srcX, srcY0,
                    srcX + width, srcY1,
                    strb->surface->u.tex.first_layer,
                    dest_surface,
                    destX, destY,
                    destX + width, destY + height,
                    0.0, PIPE_TEX_MIPFILTER_NEAREST,
                    color_writemask, 0);
   pipe_surface_reference(&dest_surface, NULL);
   return;

fallback:
   /* software fallback */
   fallback_copy_texsubimage(ctx,
                             strb, stImage, texBaseFormat,
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
   functions->TexSubImage = _mesa_store_texsubimage;
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
