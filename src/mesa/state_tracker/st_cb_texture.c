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
#include "main/texfetch.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"

#include "state_tracker/st_debug.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"
#include "state_tracker/st_cb_flush.h"
#include "state_tracker/st_cb_texture.h"
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
      return PIPE_TEXTURE_1D;
   case GL_TEXTURE_2D:
      return PIPE_TEXTURE_2D;
   case GL_TEXTURE_RECTANGLE_NV:
      return PIPE_TEXTURE_RECT;
   case GL_TEXTURE_3D:
      return PIPE_TEXTURE_3D;
   case GL_TEXTURE_CUBE_MAP_ARB:
      return PIPE_TEXTURE_CUBE;
   case GL_TEXTURE_1D_ARRAY_EXT:
      return PIPE_TEXTURE_1D_ARRAY;
   case GL_TEXTURE_2D_ARRAY_EXT:
      return PIPE_TEXTURE_2D_ARRAY;
   case GL_TEXTURE_BUFFER:
      return PIPE_BUFFER;
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
      if (stObj->sampler_view->context != st->pipe) {
         /* Take "ownership" of this texture sampler view by setting
          * its context pointer to this context.  This avoids potential
          * crashes when the texture object is shared among contexts
          * and the original/owner context has already been destroyed.
          */
         stObj->sampler_view->context = st->pipe;
      }
      pipe_sampler_view_reference(&stObj->sampler_view, NULL);
   }
   _mesa_delete_texture_object(ctx, texObj);
}


/** called via ctx->Driver.FreeTexImageData() */
static void
st_FreeTextureImageData(struct gl_context * ctx, struct gl_texture_image *texImage)
{
   struct st_texture_image *stImage = st_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (stImage->pt) {
      pipe_resource_reference(&stImage->pt, NULL);
   }

   if (texImage->Data) {
      _mesa_align_free(texImage->Data);
      texImage->Data = NULL;
   }
}


/**
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 * XXX Put this in src/mesa/main/imports.h ???
 */
#if defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86)
static INLINE void *
__memcpy(void *to, const void *from, size_t n)
{
   int d0, d1, d2;
   __asm__ __volatile__("rep ; movsl\n\t"
                        "testb $2,%b4\n\t"
                        "je 1f\n\t"
                        "movsw\n"
                        "1:\ttestb $1,%b4\n\t"
                        "je 2f\n\t"
                        "movsb\n" "2:":"=&c"(d0), "=&D"(d1), "=&S"(d2)
                        :"0"(n / 4), "q"(n), "1"((long) to), "2"((long) from)
                        :"memory");
   return (to);
}
#else
#define __memcpy(a,b,c) memcpy(a,b,c)
#endif


/**
 * The system memcpy (at least on ubuntu 5.10) has problems copying
 * to agp (writecombined) memory from a source which isn't 64-byte
 * aligned - there is a 4x performance falloff.
 *
 * The x86 __memcpy is immune to this but is slightly slower
 * (10%-ish) than the system memcpy.
 *
 * The sse_memcpy seems to have a slight cliff at 64/32 bytes, but
 * isn't much faster than x86_memcpy for agp copies.
 * 
 * TODO: switch dynamically.
 */
static void *
do_memcpy(void *dest, const void *src, size_t n)
{
   if ((((unsigned long) src) & 63) || (((unsigned long) dest) & 63)) {
      return __memcpy(dest, src, n);
   }
   else
      return memcpy(dest, src, n);
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


/** Return number of image dimensions (1, 2 or 3) for a texture target. */
static GLuint
get_texture_dims(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_BUFFER:
      return 1;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_2D_ARRAY_EXT:
      return 2;
   case GL_TEXTURE_3D:
      return 3;
   default:
      assert(0 && "invalid texture target in get_texture_dims()");
      return 1;
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
   const GLuint dims = get_texture_dims(target);

   assert(width >= 1);
   assert(height >= 1);
   assert(depth >= 1);

   if (level > 0) {
      /* Depending on the image's size, we can't always make a guess here */
      if ((dims >= 1 && width == 1) ||
          (dims >= 2 && height == 1) ||
          (dims >= 3 && depth == 1)) {
         /* we can't determine the image size at level=0 */
         return GL_FALSE;
      }

      /* grow the image size until we hit level = 0 */
      while (level > 0) {
         if (width > 1)
            width <<= 1;
         if (height > 1)
            height <<= 1;
         if (depth > 1)
            depth <<= 1;
         level--;
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
                              stImage->level,
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
        stImage->base._BaseFormat == GL_DEPTH_COMPONENT ||
        stImage->base._BaseFormat == GL_DEPTH_STENCIL_EXT) &&
       !stObj->base.GenerateMipmap &&
       stImage->level == 0) {
      /* only alloc space for a single mipmap level */
      lastLevel = 0;
   }
   else {
      /* alloc space for a full mipmap */
      GLuint l2width = util_logbase2(width);
      GLuint l2height = util_logbase2(height);
      GLuint l2depth = util_logbase2(depth);
      lastLevel = MAX2(MAX2(l2width, l2height), l2depth);
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

   DBG("%s returning %d\n", __FUNCTION__, (stObj->pt != NULL));

   return stObj->pt != NULL;
}


/**
 * Adjust pixel unpack params and image dimensions to strip off the
 * texture border.
 * Gallium doesn't support texture borders.  They've seldem been used
 * and seldom been implemented correctly anyway.
 * \param unpackNew  returns the new pixel unpack parameters
 */
static void
strip_texture_border(GLint border,
                     GLint *width, GLint *height, GLint *depth,
                     const struct gl_pixelstore_attrib *unpack,
                     struct gl_pixelstore_attrib *unpackNew)
{
   assert(border > 0);  /* sanity check */

   *unpackNew = *unpack;

   if (unpackNew->RowLength == 0)
      unpackNew->RowLength = *width;

   if (depth && unpackNew->ImageHeight == 0)
      unpackNew->ImageHeight = *height;

   unpackNew->SkipPixels += border;
   if (height)
      unpackNew->SkipRows += border;
   if (depth)
      unpackNew->SkipImages += border;

   assert(*width >= 3);
   *width = *width - 2 * border;
   if (height && *height >= 3)
      *height = *height - 2 * border;
   if (depth && *depth >= 3)
      *depth = *depth - 2 * border;
}


/**
 * Do glTexImage1/2/3D().
 */
static void
st_TexImage(struct gl_context * ctx,
            GLint dims,
            GLenum target, GLint level,
            GLint internalFormat,
            GLint width, GLint height, GLint depth,
            GLint border,
            GLenum format, GLenum type, const void *pixels,
            const struct gl_pixelstore_attrib *unpack,
            struct gl_texture_object *texObj,
            struct gl_texture_image *texImage,
            GLsizei imageSize, GLboolean compressed_src)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct st_texture_image *stImage = st_texture_image(texImage);
   GLuint dstRowStride = 0;
   struct gl_pixelstore_attrib unpackNB;
   enum pipe_transfer_usage transfer_usage = 0;

   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

   /* switch to "normal" */
   if (stObj->surface_based) {
      gl_format texFormat;

      _mesa_clear_texture_object(ctx, texObj);
      pipe_resource_reference(&stObj->pt, NULL);

      /* oops, need to init this image again */
      texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                              internalFormat, format, type);

      _mesa_init_teximage_fields(ctx, target, texImage,
                                 width, height, depth, border,
                                 internalFormat, texFormat);

      stObj->surface_based = GL_FALSE;
   }

   /* gallium does not support texture borders, strip it off */
   if (border) {
      strip_texture_border(border, &width, &height, &depth, unpack, &unpackNB);
      unpack = &unpackNB;
      texImage->Width = width;
      texImage->Height = height;
      texImage->Depth = depth;
      texImage->Border = 0;
      border = 0;
   }
   else {
      assert(texImage->Width == width);
      assert(texImage->Height == height);
      assert(texImage->Depth == depth);
   }

   stImage->face = _mesa_tex_target_to_face(target);
   stImage->level = level;

   _mesa_set_fetch_functions(texImage, dims);

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (stImage->pt) {
      pipe_resource_reference(&stImage->pt, NULL);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      _mesa_align_free(texImage->Data);
   }

   /*
    * See if the new image is somehow incompatible with the existing
    * mipmap.  If so, free the old mipmap.
    */
   if (stObj->pt) {
      if (level > (GLint) stObj->pt->last_level ||
          !st_texture_match_image(stObj->pt, &stImage->base,
                                  stImage->face, stImage->level)) {
         DBG("release it\n");
         pipe_resource_reference(&stObj->pt, NULL);
         assert(!stObj->pt);
         pipe_sampler_view_reference(&stObj->sampler_view, NULL);
      }
   }

   if (width == 0 || height == 0 || depth == 0) {
      /* stop after freeing old image */
      return;
   }

   if (!stObj->pt) {
      if (!guess_and_alloc_texture(st, stObj, stImage)) {
         /* Probably out of memory.
          * Try flushing any pending rendering, then retry.
          */
         st_finish(st);
         if (!guess_and_alloc_texture(st, stObj, stImage)) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
            return;
         }
      }
   }

   assert(!stImage->pt);

   /* Check if this texture image can live inside the texture object's buffer.
    * If so, store the image there.  Otherwise the image will temporarily live
    * in its own buffer.
    */
   if (stObj->pt &&
       st_texture_match_image(stObj->pt, &stImage->base,
                              stImage->face, stImage->level)) {

      pipe_resource_reference(&stImage->pt, stObj->pt);
      assert(stImage->pt);
   }

   if (!stImage->pt)
      DBG("XXX: Image did not fit into texture - storing in local memory!\n");

   /* Pixel data may come from regular user memory or a PBO.  For the later,
    * do bounds checking and map the PBO to read pixels data from it.
    *
    * XXX we should try to use a GPU-accelerated path to copy the image data
    * from the PBO to the texture.
    */
   if (compressed_src) {
      pixels = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, pixels,
						      unpack,
						      "glCompressedTexImage");
   }
   else {
      pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, 1,
					   format, type,
					   pixels, unpack, "glTexImage");
   }

   /* for a 1D array upload the image as a series of layer with height = 1 */
   if (target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
      height = 1;
   }

   /*
    * Prepare to store the texture data.  Either map the gallium texture buffer
    * memory or malloc space for it.
    */
   if (stImage->pt) {
      if (!pixels) {
         /* We've allocated texture resource, but have no pixel data - all done. */
         goto done;
      }

      /* Store the image in the gallium transfer object */
      if (format == GL_DEPTH_COMPONENT &&
          util_format_is_depth_and_stencil(stImage->pt->format))
         transfer_usage = PIPE_TRANSFER_READ_WRITE;
      else
         transfer_usage = PIPE_TRANSFER_WRITE;

      texImage->Data = st_texture_image_map(st, stImage, 0,
                                            transfer_usage, 0, 0, width, height);
      if(stImage->transfer)
         dstRowStride = stImage->transfer->stride;
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      GLuint imageSize = _mesa_format_image_size(texImage->TexFormat,
                                                 width, height, depth);
      dstRowStride = _mesa_format_row_stride(texImage->TexFormat, width);

      texImage->Data = _mesa_align_malloc(imageSize, 16);
   }

   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
      return;
   }

   if (!pixels) {
      /* We've allocated texture memory, but have no pixel data - all done. */
      goto done;
   }

   DBG("Upload image %dx%dx%d row_len %x pitch %x\n",
       width, height, depth, width, dstRowStride);

   /* Copy user texture image into the texture buffer.
    */
   if (compressed_src) {
      const GLuint srcRowStride =
         _mesa_format_row_stride(texImage->TexFormat, width);
      if (dstRowStride == srcRowStride) {
         memcpy(texImage->Data, pixels, imageSize);
      }
      else {
         char *dst = texImage->Data;
         const char *src = pixels;
         GLuint i, bw, bh, lines;
         _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);
         lines = (height + bh - 1) / bh;

         for (i = 0; i < lines; ++i) {
            memcpy(dst, src, srcRowStride);
            dst += dstRowStride;
            src += srcRowStride;
         }
      }
   }
   else {
      const GLuint srcImageStride =
         _mesa_image_image_stride(unpack, width, height, format, type);
      GLint i;
      const GLubyte *src = (const GLubyte *) pixels;

      for (i = 0; i < depth; i++) {
	 if (!_mesa_texstore(ctx, dims, 
                             texImage->_BaseFormat, 
                             texImage->TexFormat, 
                             texImage->Data,
                             0, 0, 0, /* dstX/Y/Zoffset */
                             dstRowStride,
                             texImage->ImageOffsets,
                             width, height, 1,
                             format, type, src, unpack)) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
	 }

	 if (stImage->pt && i + 1 < depth) {
            /* unmap this slice */
	    st_texture_image_unmap(st, stImage);
            /* map next slice of 3D texture */
	    texImage->Data = st_texture_image_map(st, stImage, i + 1,
                                                  transfer_usage, 0, 0,
                                                  width, height);
	    src += srcImageStride;
	 }
      }
   }

done:
   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (stImage->pt && texImage->Data) {
      st_texture_image_unmap(st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_TexImage3D(struct gl_context * ctx,
              GLenum target, GLint level,
              GLint internalFormat,
              GLint width, GLint height, GLint depth,
              GLint border,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack,
              struct gl_texture_object *texObj,
              struct gl_texture_image *texImage)
{
   st_TexImage(ctx, 3, target, level, internalFormat, width, height, depth,
               border, format, type, pixels, unpack, texObj, texImage,
               0, GL_FALSE);
}


static void
st_TexImage2D(struct gl_context * ctx,
              GLenum target, GLint level,
              GLint internalFormat,
              GLint width, GLint height, GLint border,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack,
              struct gl_texture_object *texObj,
              struct gl_texture_image *texImage)
{
   st_TexImage(ctx, 2, target, level, internalFormat, width, height, 1, border,
               format, type, pixels, unpack, texObj, texImage, 0, GL_FALSE);
}


static void
st_TexImage1D(struct gl_context * ctx,
              GLenum target, GLint level,
              GLint internalFormat,
              GLint width, GLint border,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack,
              struct gl_texture_object *texObj,
              struct gl_texture_image *texImage)
{
   st_TexImage(ctx, 1, target, level, internalFormat, width, 1, 1, border,
               format, type, pixels, unpack, texObj, texImage, 0, GL_FALSE);
}


static void
st_CompressedTexImage2D(struct gl_context *ctx, GLenum target, GLint level,
                        GLint internalFormat,
                        GLint width, GLint height, GLint border,
                        GLsizei imageSize, const GLvoid *data,
                        struct gl_texture_object *texObj,
                        struct gl_texture_image *texImage)
{
   st_TexImage(ctx, 2, target, level, internalFormat, width, height, 1, border,
               0, 0, data, &ctx->Unpack, texObj, texImage, imageSize, GL_TRUE);
}



/**
 * glGetTexImage() helper: decompress a compressed texture by rendering
 * a textured quad.  Store the results in the user's buffer.
 */
static void
decompress_with_blit(struct gl_context * ctx, GLenum target, GLint level,
                     GLenum format, GLenum type, GLvoid *pixels,
                     struct gl_texture_object *texObj,
                     struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct pipe_sampler_view *src_view =
      st_get_texture_sampler_view(stObj, pipe);
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   struct pipe_surface *dst_surface;
   struct pipe_resource *dst_texture;
   struct pipe_transfer *tex_xfer;
   unsigned bind = (PIPE_BIND_RENDER_TARGET | /* util_blit may choose to render */
		    PIPE_BIND_TRANSFER_READ);

   /* create temp / dest surface */
   if (!util_create_rgba_surface(pipe, width, height, bind,
                                 &dst_texture, &dst_surface)) {
      _mesa_problem(ctx, "util_create_rgba_surface() failed "
                    "in decompress_with_blit()");
      return;
   }

   /* Disable conditional rendering. */
   if (st->render_condition) {
      pipe->render_condition(pipe, NULL, 0);
   }

   /* blit/render/decompress */
   util_blit_pixels_tex(st->blit,
                        src_view,      /* pipe_resource (src) */
                        0, 0,             /* src x0, y0 */
                        width, height,    /* src x1, y1 */
                        dst_surface,      /* pipe_surface (dst) */
                        0, 0,             /* dst x0, y0 */
                        width, height,    /* dst x1, y1 */
                        0.0,              /* z */
                        PIPE_TEX_MIPFILTER_NEAREST);

   /* Restore conditional rendering state. */
   if (st->render_condition) {
      pipe->render_condition(pipe, st->render_condition,
                             st->condition_mode);
   }

   /* map the dst_surface so we can read from it */
   tex_xfer = pipe_get_transfer(pipe,
                                dst_texture, 0, 0,
                                PIPE_TRANSFER_READ,
                                0, 0, width, height);

   pixels = _mesa_map_pbo_dest(ctx, &ctx->Pack, pixels);

   /* copy/pack data into user buffer */
   if (st_equal_formats(stImage->pt->format, format, type)) {
      /* memcpy */
      const uint bytesPerRow = width * util_format_get_blocksize(stImage->pt->format);
      ubyte *map = pipe_transfer_map(pipe, tex_xfer);
      GLuint row;
      for (row = 0; row < height; row++) {
         GLvoid *dest = _mesa_image_address2d(&ctx->Pack, pixels, width,
                                              height, format, type, row, 0);
         memcpy(dest, map, bytesPerRow);
         map += tex_xfer->stride;
      }
      pipe_transfer_unmap(pipe, tex_xfer);
   }
   else {
      /* format translation via floats */
      GLuint row;
      enum pipe_format pformat = util_format_linear(dst_texture->format);
      for (row = 0; row < height; row++) {
         const GLbitfield transferOps = 0x0; /* bypassed for glGetTexImage() */
         GLfloat rgba[4 * MAX_WIDTH];
         GLvoid *dest = _mesa_image_address2d(&ctx->Pack, pixels, width,
                                              height, format, type, row, 0);

         if (ST_DEBUG & DEBUG_FALLBACK)
            debug_printf("%s: fallback format translation\n", __FUNCTION__);

         /* get float[4] rgba row from surface */
         pipe_get_tile_rgba_format(pipe, tex_xfer, 0, row, width, 1,
                                   pformat, rgba);

         _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                    type, dest, &ctx->Pack, transferOps);
      }
   }

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);

   pipe->transfer_destroy(pipe, tex_xfer);

   /* destroy the temp / dest surface */
   util_destroy_rgba_surface(dst_texture, dst_surface);
}



/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
st_get_tex_image(struct gl_context * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage, GLboolean compressed_dst)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   const GLuint dstImageStride =
      _mesa_image_image_stride(&ctx->Pack, texImage->Width, texImage->Height,
                               format, type);
   GLuint depth, i;
   GLubyte *dest;

   if (stImage->pt &&
       util_format_is_s3tc(stImage->pt->format) &&
       !compressed_dst) {
      /* Need to decompress the texture.
       * We'll do this by rendering a textured quad.
       * Note that we only expect RGBA formats (no Z/depth formats).
       */
      decompress_with_blit(ctx, target, level, format, type, pixels,
                           texObj, texImage);
      return;
   }

   /* Map */
   if (stImage->pt) {
      /* Image is stored in hardware format in a buffer managed by the
       * kernel.  Need to explicitly map and unmap it.
       */
      texImage->Data = st_texture_image_map(st, stImage, 0,
                                            PIPE_TRANSFER_READ, 0, 0,
                                            stImage->base.Width,
                                            stImage->base.Height);
      /* compute stride in texels from stride in bytes */
      texImage->RowStride = stImage->transfer->stride
         * util_format_get_blockwidth(stImage->pt->format)
         / util_format_get_blocksize(stImage->pt->format);
   }
   else {
      /* Otherwise, the image should actually be stored in
       * texImage->Data.  This is pretty confusing for
       * everybody, I'd much prefer to separate the two functions of
       * texImage->Data - storage for texture images in main memory
       * and access (ie mappings) of images.  In other words, we'd
       * create a new texImage->Map field and leave Data simply for
       * storage.
       */
      assert(texImage->Data);
   }

   depth = texImage->Depth;
   texImage->Depth = 1;

   dest = (GLubyte *) pixels;

   _mesa_set_fetch_functions(texImage, get_texture_dims(target));

   for (i = 0; i < depth; i++) {
      if (compressed_dst) {
	 _mesa_get_compressed_teximage(ctx, target, level, dest,
				       texObj, texImage);
      }
      else {
	 _mesa_get_teximage(ctx, target, level, format, type, dest,
			    texObj, texImage);
      }

      if (stImage->pt && i + 1 < depth) {
         /* unmap this slice */
	 st_texture_image_unmap(st, stImage);
         /* map next slice of 3D texture */
	 texImage->Data = st_texture_image_map(st, stImage, i + 1,
                                               PIPE_TRANSFER_READ, 0, 0,
                                               stImage->base.Width,
                                               stImage->base.Height);
	 dest += dstImageStride;
      }
   }

   texImage->Depth = depth;

   /* Unmap */
   if (stImage->pt) {
      st_texture_image_unmap(st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_GetTexImage(struct gl_context * ctx, GLenum target, GLint level,
               GLenum format, GLenum type, GLvoid * pixels,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, format, type, pixels, texObj, texImage,
                    GL_FALSE);
}


static void
st_GetCompressedTexImage(struct gl_context *ctx, GLenum target, GLint level,
                         GLvoid *pixels,
                         struct gl_texture_object *texObj,
                         struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, 0, 0, pixels, texObj, texImage,
                    GL_TRUE);
}



static void
st_TexSubimage(struct gl_context *ctx, GLint dims, GLenum target, GLint level,
               GLint xoffset, GLint yoffset, GLint zoffset,
               GLint width, GLint height, GLint depth,
               GLenum format, GLenum type, const void *pixels,
               const struct gl_pixelstore_attrib *packing,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   GLuint dstRowStride;
   const GLuint srcImageStride =
      _mesa_image_image_stride(packing, width, height, format, type);
   GLint i;
   const GLubyte *src;
   /* init to silence warning only: */
   enum pipe_transfer_usage transfer_usage = PIPE_TRANSFER_WRITE;

   DBG("%s target %s level %d offset %d,%d %dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       level, xoffset, yoffset, width, height);

   pixels =
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth, format,
                                  type, pixels, packing, "glTexSubImage2D");
   if (!pixels)
      return;

   /* for a 1D array upload the image as a series of layer with height = 1 */
   if (target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
      height = 1;
   }

   /* Map buffer if necessary.  Need to lock to prevent other contexts
    * from uploading the buffer under us.
    */
   if (stImage->pt) {
      if (format == GL_DEPTH_COMPONENT &&
          util_format_is_depth_and_stencil(stImage->pt->format))
         transfer_usage = PIPE_TRANSFER_READ_WRITE;
      else
         transfer_usage = PIPE_TRANSFER_WRITE;

      texImage->Data = st_texture_image_map(st, stImage, zoffset, 
                                            transfer_usage,
                                            xoffset, yoffset,
                                            width, height);
   }

   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
      goto done;
   }

   src = (const GLubyte *) pixels;
   dstRowStride = stImage->transfer->stride;

   for (i = 0; i < depth; i++) {
      if (!_mesa_texstore(ctx, dims, texImage->_BaseFormat,
                          texImage->TexFormat,
                          texImage->Data,
                          0, 0, 0,
                          dstRowStride,
                          texImage->ImageOffsets,
                          width, height, 1,
                          format, type, src, packing)) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
      }

      if (stImage->pt && i + 1 < depth) {
         /* unmap this slice */
	 st_texture_image_unmap(st, stImage);
         /* map next slice of 3D texture */
	 texImage->Data = st_texture_image_map(st, stImage,
                                               zoffset + i + 1,
                                               transfer_usage,
                                               xoffset, yoffset,
                                               width, height);
	 src += srcImageStride;
      }
   }

done:
   _mesa_unmap_teximage_pbo(ctx, packing);

   if (stImage->pt && texImage->Data) {
      st_texture_image_unmap(st, stImage);
      texImage->Data = NULL;
   }
}



static void
st_TexSubImage3D(struct gl_context *ctx, GLenum target, GLint level,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLenum format, GLenum type, const GLvoid *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 3, target, level, xoffset, yoffset, zoffset,
                  width, height, depth, format, type,
                  pixels, packing, texObj, texImage);
}


static void
st_TexSubImage2D(struct gl_context *ctx, GLenum target, GLint level,
                 GLint xoffset, GLint yoffset,
                 GLsizei width, GLsizei height,
                 GLenum format, GLenum type, const GLvoid * pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 2, target, level, xoffset, yoffset, 0,
                  width, height, 1, format, type,
                  pixels, packing, texObj, texImage);
}


static void
st_TexSubImage1D(struct gl_context *ctx, GLenum target, GLint level,
                 GLint xoffset, GLsizei width, GLenum format, GLenum type,
                 const GLvoid * pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 1, target, level, xoffset, 0, 0, width, 1, 1,
                  format, type, pixels, packing, texObj, texImage);
}


static void
st_CompressedTexSubImage1D(struct gl_context *ctx, GLenum target, GLint level,
                           GLint xoffset, GLsizei width,
                           GLenum format,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage)
{
   assert(0);
}


static void
st_CompressedTexSubImage2D(struct gl_context *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint yoffset,
                           GLsizei width, GLint height,
                           GLenum format,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_image *stImage = st_texture_image(texImage);
   int srcBlockStride;
   int dstBlockStride;
   int y;
   enum pipe_format pformat;

   if (stImage->pt) {
      pformat = stImage->pt->format;

      texImage->Data = st_texture_image_map(st, stImage, 0, 
                                            PIPE_TRANSFER_WRITE,
                                            xoffset, yoffset,
                                            width, height);
      
      srcBlockStride = util_format_get_stride(pformat, width);
      dstBlockStride = stImage->transfer->stride;
   } else {
      assert(stImage->pt);
      /* TODO find good values for block and strides */
      /* TODO also adjust texImage->data for yoffset/xoffset */
      return;
   }

   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage");
      return;
   }

   assert(xoffset % util_format_get_blockwidth(pformat) == 0);
   assert(yoffset % util_format_get_blockheight(pformat) == 0);

   for (y = 0; y < height; y += util_format_get_blockheight(pformat)) {
      /* don't need to adjust for xoffset and yoffset as st_texture_image_map does that */
      const char *src = (const char*)data + srcBlockStride * util_format_get_nblocksy(pformat, y);
      char *dst = (char*)texImage->Data + dstBlockStride * util_format_get_nblocksy(pformat, y);
      memcpy(dst, src, util_format_get_stride(pformat, width));
   }

   if (stImage->pt) {
      st_texture_image_unmap(st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_CompressedTexSubImage3D(struct gl_context *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint yoffset, GLint zoffset,
                           GLsizei width, GLint height, GLint depth,
                           GLenum format,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage)
{
   assert(0);
}



/**
 * Do a CopyTexSubImage operation using a read transfer from the source,
 * a write transfer to the destination and get_tile()/put_tile() to access
 * the pixels/texels.
 *
 * Note: srcY=0=TOP of renderbuffer
 */
static void
fallback_copy_texsubimage(struct gl_context *ctx, GLenum target, GLint level,
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
   GLvoid *texDest;
   enum pipe_transfer_usage transfer_usage;

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   assert(width <= MAX_WIDTH);

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      srcY = strb->Base.Height - srcY - height;
   }

   src_trans = pipe_get_transfer(pipe,
                                 strb->texture,
                                 strb->rtt_level,
                                 strb->rtt_face + strb->rtt_slice,
                                 PIPE_TRANSFER_READ,
                                 srcX, srcY,
                                 width, height);

   if ((baseFormat == GL_DEPTH_COMPONENT ||
        baseFormat == GL_DEPTH_STENCIL) &&
       util_format_is_depth_and_stencil(stImage->pt->format))
      transfer_usage = PIPE_TRANSFER_READ_WRITE;
   else
      transfer_usage = PIPE_TRANSFER_WRITE;

   /* XXX this used to ignore destZ param */
   texDest = st_texture_image_map(st, stImage, destZ, transfer_usage,
                                  destX, destY, width, height);

   if (baseFormat == GL_DEPTH_COMPONENT ||
       baseFormat == GL_DEPTH_STENCIL) {
      const GLboolean scaleOrBias = (ctx->Pixel.DepthScale != 1.0F ||
                                     ctx->Pixel.DepthBias != 0.0F);
      GLint row, yStep;

      /* determine bottom-to-top vs. top-to-bottom order for src buffer */
      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         srcY = height - 1;
         yStep = -1;
      }
      else {
         srcY = 0;
         yStep = 1;
      }

      /* To avoid a large temp memory allocation, do copy row by row */
      for (row = 0; row < height; row++, srcY += yStep) {
         uint data[MAX_WIDTH];
         pipe_get_tile_z(pipe, src_trans, 0, srcY, width, 1, data);
         if (scaleOrBias) {
            _mesa_scale_and_bias_depth_uint(ctx, width, data);
         }
         pipe_put_tile_z(pipe, stImage->transfer, 0, row, width, 1, data);
      }
   }
   else {
      /* RGBA format */
      GLfloat *tempSrc =
         (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

      if (tempSrc && texDest) {
         const GLint dims = 2;
         const GLint dstRowStride = stImage->transfer->stride;
         struct gl_texture_image *texImage = &stImage->base;
         struct gl_pixelstore_attrib unpack = ctx->DefaultPacking;

         if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
            unpack.Invert = GL_TRUE;
         }

         /* get float/RGBA image from framebuffer */
         /* XXX this usually involves a lot of int/float conversion.
          * try to avoid that someday.
          */
         pipe_get_tile_rgba_format(pipe, src_trans, 0, 0, width, height,
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
                        texDest,
                        0, 0, 0,
                        dstRowStride,
                        texImage->ImageOffsets,
                        width, height, 1,
                        GL_RGBA, GL_FLOAT, tempSrc, /* src */
                        &unpack);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");
      }

      if (tempSrc)
         free(tempSrc);
   }

   st_texture_image_unmap(st, stImage);
   pipe->transfer_destroy(pipe, src_trans);
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
 * Do a CopyTex[Sub]Image1/2/3D() using a hardware (blit) path if possible.
 * Note that the region to copy has already been clipped so we know we
 * won't read from outside the source renderbuffer's bounds.
 *
 * Note: srcY=0=Bottom of renderbuffer (GL convention)
 */
static void
st_copy_texsubimage(struct gl_context *ctx,
                    GLenum target, GLint level,
                    GLint destX, GLint destY, GLint destZ,
                    GLint srcX, GLint srcY,
                    GLsizei width, GLsizei height)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   struct st_texture_image *stImage = st_texture_image(texImage);
   const GLenum texBaseFormat = texImage->_BaseFormat;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb;
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   enum pipe_format dest_format, src_format;
   GLboolean use_fallback = GL_TRUE;
   GLboolean matching_base_formats;
   GLuint format_writemask, sample_count;
   struct pipe_surface *dest_surface = NULL;
   GLboolean do_flip = (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP);

   /* make sure finalize_textures has been called? 
    */
   if (0) st_validate_state(st);

   /* determine if copying depth or color data */
   if (texBaseFormat == GL_DEPTH_COMPONENT ||
       texBaseFormat == GL_DEPTH_STENCIL) {
      strb = st_renderbuffer(fb->_DepthBuffer);
      if (strb->Base.Wrapped) {
         strb = st_renderbuffer(strb->Base.Wrapped);
      }
   }
   else {
      /* texBaseFormat == GL_RGB, GL_RGBA, GL_ALPHA, etc */
      strb = st_renderbuffer(fb->_ColorReadBuffer);
   }

   if (!strb || !strb->surface || !stImage->pt) {
      debug_printf("%s: null strb or stImage\n", __FUNCTION__);
      return;
   }

   sample_count = strb->surface->texture->nr_samples;
   /* I believe this would be legal, presumably would need to do a resolve
      for color, and for depth/stencil spec says to just use one of the
      depth/stencil samples per pixel? Need some transfer clarifications. */
   assert(sample_count < 2);

   if (srcX < 0) {
      width -= -srcX;
      destX += -srcX;
      srcX = 0;
   }

   if (srcY < 0) {
      height -= -srcY;
      destY += -srcY;
      srcY = 0;
   }

   if (destX < 0) {
      width -= -destX;
      srcX += -destX;
      destX = 0;
   }

   if (destY < 0) {
      height -= -destY;
      srcY += -destY;
      destY = 0;
   }

   if (width < 0 || height < 0)
      return;


   assert(strb);
   assert(strb->surface);
   assert(stImage->pt);

   src_format = strb->surface->format;
   dest_format = stImage->pt->format;

   /*
    * Determine if the src framebuffer and dest texture have the same
    * base format.  We need this to detect a case such as the framebuffer
    * being GL_RGBA but the texture being GL_RGB.  If the actual hardware
    * texture format stores RGBA we need to set A=1 (overriding the
    * framebuffer's alpha values).  We can't do that with the blit or
    * textured-quad paths.
    */
   matching_base_formats =
      (_mesa_get_format_base_format(strb->Base.Format) ==
       _mesa_get_format_base_format(texImage->TexFormat));
   format_writemask = compatible_src_dst_formats(ctx, &strb->Base, texImage);

   if (ctx->_ImageTransferState == 0x0) {

      if (matching_base_formats &&
          src_format == dest_format &&
          !do_flip)
      {
         /* use surface_copy() / blit */
         struct pipe_box src_box;
         u_box_2d_zslice(srcX, srcY, strb->surface->u.tex.first_layer,
                         width, height, &src_box);

         /* for resource_copy_region(), y=0=top, always */
         pipe->resource_copy_region(pipe,
                                    /* dest */
                                    stImage->pt,
                                    stImage->level,
                                    destX, destY, destZ + stImage->face,
                                    /* src */
                                    strb->texture,
                                    strb->surface->u.tex.level,
                                    &src_box);
         use_fallback = GL_FALSE;
      }
      else if (format_writemask &&
               texBaseFormat != GL_DEPTH_COMPONENT &&
               texBaseFormat != GL_DEPTH_STENCIL &&
               screen->is_format_supported(screen, src_format,
                                           PIPE_TEXTURE_2D, sample_count,
                                           PIPE_BIND_SAMPLER_VIEW) &&
               screen->is_format_supported(screen, dest_format,
                                           PIPE_TEXTURE_2D, 0,
                                           PIPE_BIND_RENDER_TARGET)) {
         /* draw textured quad to do the copy */
         GLint srcY0, srcY1;
         struct pipe_surface surf_tmpl;
         memset(&surf_tmpl, 0, sizeof(surf_tmpl));
         surf_tmpl.format = util_format_linear(stImage->pt->format);
         surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
         surf_tmpl.u.tex.level = stImage->level;
         surf_tmpl.u.tex.first_layer = stImage->face + destZ;
         surf_tmpl.u.tex.last_layer = stImage->face + destZ;

         dest_surface = pipe->create_surface(pipe, stImage->pt,
                                             &surf_tmpl);

         if (do_flip) {
            srcY1 = strb->Base.Height - srcY - height;
            srcY0 = srcY1 + height;
         }
         else {
            srcY0 = srcY;
            srcY1 = srcY0 + height;
         }

         /* Disable conditional rendering. */
         if (st->render_condition) {
            pipe->render_condition(pipe, NULL, 0);
         }

         util_blit_pixels_writemask(st->blit,
                                    strb->texture,
                                    strb->surface->u.tex.level,
                                    srcX, srcY0,
                                    srcX + width, srcY1,
                                    strb->surface->u.tex.first_layer,
                                    dest_surface,
                                    destX, destY,
                                    destX + width, destY + height,
                                    0.0, PIPE_TEX_MIPFILTER_NEAREST,
                                    format_writemask);

         /* Restore conditional rendering state. */
         if (st->render_condition) {
            pipe->render_condition(pipe, st->render_condition,
                                   st->condition_mode);
         }

         use_fallback = GL_FALSE;
      }

      if (dest_surface)
         pipe_surface_reference(&dest_surface, NULL);
   }

   if (use_fallback) {
      /* software fallback */
      fallback_copy_texsubimage(ctx, target, level,
                                strb, stImage, texBaseFormat,
                                destX, destY, destZ,
                                srcX, srcY, width, height);
   }
}



static void
st_CopyTexImage1D(struct gl_context * ctx, GLenum target, GLint level,
                  GLenum internalFormat,
                  GLint x, GLint y, GLsizei width, GLint border)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);

   /* Setup or redefine the texture object, texture and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                          width, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   st_copy_texsubimage(ctx, target, level,
                       0, 0, 0,  /* destX,Y,Z */
                       x, y, width, 1);  /* src X, Y, size */
}


static void
st_CopyTexImage2D(struct gl_context * ctx, GLenum target, GLint level,
                  GLenum internalFormat,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  GLint border)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);

   /* Setup or redefine the texture object, texture and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                          width, height, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   st_copy_texsubimage(ctx, target, level,
                       0, 0, 0,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
st_CopyTexSubImage1D(struct gl_context * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint x, GLint y, GLsizei width)
{
   const GLint yoffset = 0, zoffset = 0;
   const GLsizei height = 1;
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
st_CopyTexSubImage2D(struct gl_context * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   const GLint zoffset = 0;
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
st_CopyTexSubImage3D(struct gl_context * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
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
         stObj->base.Image[stImage->face][dstLevel];
      assert(dstImage);
      assert(dstImage->Width == stImage->base.Width);
      assert(dstImage->Height == stImage->base.Height);
      assert(dstImage->Depth == stImage->base.Depth);
   }

   if (stImage->pt) {
      /* Copy potentially with the blitter:
       */
      st_texture_image_copy(st->pipe,
                            stObj->pt, dstLevel,  /* dest texture, level */
                            stImage->pt, stImage->level, /* src texture, level */
                            stImage->face);

      pipe_resource_reference(&stImage->pt, NULL);
   }
   else if (stImage->base.Data) {
      st_texture_image_data(st,
                            stObj->pt,
                            stImage->face,
                            dstLevel,
                            stImage->base.Data,
                            stImage->base.RowStride * 
                            util_format_get_blocksize(stObj->pt->format),
                            stImage->base.RowStride *
                            stImage->base.Height *
                            util_format_get_blocksize(stObj->pt->format));
      _mesa_align_free(stImage->base.Data);
      stImage->base.Data = NULL;
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

   if (stObj->base._Complete) {
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
      pipe_sampler_view_reference(&stObj->sampler_view, NULL);
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
                                 stObj->base.BaseLevel,
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
         pipe_sampler_view_reference(&stObj->sampler_view, NULL);
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
            if (level == 0 || (stImage->base.Width == u_minify(stObj->width0, level) &&
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
 * Returns pointer to a default/dummy texture.
 * This is typically used when the current shader has tex/sample instructions
 * but the user has not provided a (any) texture(s).
 */
struct gl_texture_object *
st_get_default_texture(struct st_context *st)
{
   if (!st->default_texture) {
      static const GLenum target = GL_TEXTURE_2D;
      GLubyte pixels[16][16][4];
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImg;
      GLuint i, j;

      /* The ARB_fragment_program spec says (0,0,0,1) should be returned
       * when attempting to sample incomplete textures.
       */
      for (i = 0; i < 16; i++) {
         for (j = 0; j < 16; j++) {
            pixels[i][j][0] = 0;
            pixels[i][j][1] = 0;
            pixels[i][j][2] = 0;
            pixels[i][j][3] = 255;
         }
      }

      texObj = st->ctx->Driver.NewTextureObject(st->ctx, 0, target);

      texImg = _mesa_get_tex_image(st->ctx, texObj, target, 0);

      _mesa_init_teximage_fields(st->ctx, target, texImg,
                                 16, 16, 1, 0,  /* w, h, d, border */
                                 GL_RGBA, MESA_FORMAT_RGBA8888);

      st_TexImage(st->ctx, 2, target,
                  0, GL_RGBA,    /* level, intformat */
                  16, 16, 1, 0,  /* w, h, d, border */
                  GL_RGBA, GL_UNSIGNED_BYTE, pixels,
                  &st->ctx->DefaultPacking,
                  texObj, texImg,
                  0, 0);

      texObj->Sampler.MinFilter = GL_NEAREST;
      texObj->Sampler.MagFilter = GL_NEAREST;
      texObj->_Complete = GL_TRUE;

      st->default_texture = texObj;
   }
   return st->default_texture;
}


void
st_init_texture_functions(struct dd_function_table *functions)
{
   functions->ChooseTextureFormat = st_ChooseTextureFormat;
   functions->TexImage1D = st_TexImage1D;
   functions->TexImage2D = st_TexImage2D;
   functions->TexImage3D = st_TexImage3D;
   functions->TexSubImage1D = st_TexSubImage1D;
   functions->TexSubImage2D = st_TexSubImage2D;
   functions->TexSubImage3D = st_TexSubImage3D;
   functions->CompressedTexSubImage1D = st_CompressedTexSubImage1D;
   functions->CompressedTexSubImage2D = st_CompressedTexSubImage2D;
   functions->CompressedTexSubImage3D = st_CompressedTexSubImage3D;
   functions->CopyTexImage1D = st_CopyTexImage1D;
   functions->CopyTexImage2D = st_CopyTexImage2D;
   functions->CopyTexSubImage1D = st_CopyTexSubImage1D;
   functions->CopyTexSubImage2D = st_CopyTexSubImage2D;
   functions->CopyTexSubImage3D = st_CopyTexSubImage3D;
   functions->GenerateMipmap = st_generate_mipmap;

   functions->GetTexImage = st_GetTexImage;

   /* compressed texture functions */
   functions->CompressedTexImage2D = st_CompressedTexImage2D;
   functions->GetCompressedTexImage = st_GetCompressedTexImage;

   functions->NewTextureObject = st_NewTextureObject;
   functions->NewTextureImage = st_NewTextureImage;
   functions->DeleteTexture = st_DeleteTextureObject;
   functions->FreeTexImageData = st_FreeTextureImageData;

   functions->TextureMemCpy = do_memcpy;

   /* XXX Temporary until we can query pipe's texture sizes */
   functions->TestProxyTexImage = _mesa_test_proxy_teximage;
}
