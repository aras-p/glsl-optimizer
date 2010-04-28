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
#if FEATURE_convolve
#include "main/convolve.h"
#endif
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mipmap.h"
#include "main/texcompress.h"
#include "main/texfetch.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"

#include "state_tracker/st_debug.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"
#include "state_tracker/st_cb_texture.h"
#include "state_tracker/st_format.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_texture.h"
#include "state_tracker/st_gen_mipmap.h"
#include "state_tracker/st_inlines.h"
#include "state_tracker/st_atom.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_tile.h"
#include "util/u_blit.h"
#include "util/u_format.h"
#include "util/u_surface.h"
#include "util/u_math.h"


#define DBG if (0) printf


static enum pipe_texture_target
gl_target_to_pipe(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
      return PIPE_TEXTURE_1D;

   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE_NV:
      return PIPE_TEXTURE_2D;

   case GL_TEXTURE_3D:
      return PIPE_TEXTURE_3D;

   case GL_TEXTURE_CUBE_MAP_ARB:
      return PIPE_TEXTURE_CUBE;

   default:
      assert(0);
      return 0;
   }
}


/** called via ctx->Driver.NewTextureImage() */
static struct gl_texture_image *
st_NewTextureImage(GLcontext * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) ST_CALLOC_STRUCT(st_texture_image);
}


/** called via ctx->Driver.NewTextureObject() */
static struct gl_texture_object *
st_NewTextureObject(GLcontext * ctx, GLuint name, GLenum target)
{
   struct st_texture_object *obj = ST_CALLOC_STRUCT(st_texture_object);

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

/** called via ctx->Driver.DeleteTextureImage() */
static void 
st_DeleteTextureObject(GLcontext *ctx,
                       struct gl_texture_object *texObj)
{
   struct st_texture_object *stObj = st_texture_object(texObj);
   if (stObj->pt)
      pipe_texture_reference(&stObj->pt, NULL);

   _mesa_delete_texture_object(ctx, texObj);
}


/** called via ctx->Driver.FreeTexImageData() */
static void
st_FreeTextureImageData(GLcontext * ctx, struct gl_texture_image *texImage)
{
   struct st_texture_image *stImage = st_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (stImage->pt) {
      pipe_texture_reference(&stImage->pt, NULL);
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
 * Return default texture usage bitmask for the given texture format.
 */
static GLuint
default_usage(enum pipe_format fmt)
{
   GLuint usage = PIPE_TEXTURE_USAGE_SAMPLER;
   if (util_format_is_depth_or_stencil(fmt))
      usage |= PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
   else
      usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
   return usage;
}


/**
 * Allocate a pipe_texture object for the given st_texture_object using
 * the given st_texture_image to guess the mipmap size/levels.
 *
 * [comments...]
 * Otherwise, store it in memory if (Border != 0) or (any dimension ==
 * 1).
 *    
 * Otherwise, if max_level >= level >= min_level, create texture with
 * space for images from min_level down to max_level.
 *
 * Otherwise, create texture with space for images from (level 0)..(1x1).
 * Consider pruning this texture at a validation if the saving is worth it.
 */
static void
guess_and_alloc_texture(struct st_context *st,
			struct st_texture_object *stObj,
			const struct st_texture_image *stImage)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = stImage->base.Width2;  /* size w/out border */
   GLuint height = stImage->base.Height2;
   GLuint depth = stImage->base.Depth2;
   GLuint i, usage;
   enum pipe_format fmt;

   DBG("%s\n", __FUNCTION__);

   assert(!stObj->pt);

   if (stObj->pt &&
       (GLint) stImage->level > stObj->base.BaseLevel &&
       (stImage->base.Width == 1 ||
        (stObj->base.Target != GL_TEXTURE_1D &&
         stImage->base.Height == 1) ||
        (stObj->base.Target == GL_TEXTURE_3D &&
         stImage->base.Depth == 1)))
      return;

   /* If this image disrespects BaseLevel, allocate from level zero.
    * Usually BaseLevel == 0, so it's unlikely to happen.
    */
   if ((GLint) stImage->level < stObj->base.BaseLevel)
      firstLevel = 0;
   else
      firstLevel = stObj->base.BaseLevel;


   /* Figure out image dimensions at start level. 
    */
   for (i = stImage->level; i > firstLevel; i--) {
      if (width != 1)
         width <<= 1;
      if (height != 1)
         height <<= 1;
      if (depth != 1)
         depth <<= 1;
   }

   if (width == 0 || height == 0 || depth == 0) {
      /* no texture needed */
      return;
   }

   /* Guess a reasonable value for lastLevel.  This is probably going
    * to be wrong fairly often and might mean that we have to look at
    * resizable buffers, or require that buffers implement lazy
    * pagetable arrangements.
    */
   if ((stObj->base.MinFilter == GL_NEAREST ||
        stObj->base.MinFilter == GL_LINEAR ||
        stImage->base._BaseFormat == GL_DEPTH_COMPONENT ||
        stImage->base._BaseFormat == GL_DEPTH_STENCIL_EXT) &&
       !stObj->base.GenerateMipmap &&
       stImage->level == firstLevel) {
      /* only alloc space for a single mipmap level */
      lastLevel = firstLevel;
   }
   else {
      /* alloc space for a full mipmap */
      GLuint l2width = util_logbase2(width);
      GLuint l2height = util_logbase2(height);
      GLuint l2depth = util_logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width, l2height), l2depth);
   }

   fmt = st_mesa_format_to_pipe_format(stImage->base.TexFormat);

   usage = default_usage(fmt);

   stObj->pt = st_texture_create(st,
                                 gl_target_to_pipe(stObj->base.Target),
                                 fmt,
                                 lastLevel,
                                 width,
                                 height,
                                 depth,
                                 usage);

   DBG("%s - success\n", __FUNCTION__);
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
 * Try to do texture compression via rendering.  If the Gallium driver
 * can render into a compressed surface this will allow us to do texture
 * compression.
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static GLboolean
compress_with_blit(GLcontext * ctx,
                   GLenum target, GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLint width, GLint height, GLint depth,
                   GLenum format, GLenum type, const void *pixels,
                   const struct gl_pixelstore_attrib *unpack,
                   struct gl_texture_image *texImage)
{
   const GLuint dstImageOffsets[1] = {0};
   struct st_texture_image *stImage = st_texture_image(texImage);
   struct pipe_screen *screen = ctx->st->pipe->screen;
   gl_format mesa_format;
   struct pipe_texture templ;
   struct pipe_texture *src_tex;
   struct pipe_surface *dst_surface;
   struct pipe_transfer *tex_xfer;
   void *map;

   if (!stImage->pt) {
      /* XXX: Can this happen? Should we assert? */
      return GL_FALSE;
   }

   /* get destination surface (in the compressed texture) */
   dst_surface = screen->get_tex_surface(screen, stImage->pt,
                                         stImage->face, stImage->level, 0,
                                         PIPE_BUFFER_USAGE_GPU_WRITE);
   if (!dst_surface) {
      /* can't render into this format (or other problem) */
      return GL_FALSE;
   }

   /* Choose format for the temporary RGBA texture image.
    */
   mesa_format = st_ChooseTextureFormat(ctx, GL_RGBA, format, type);
   assert(mesa_format);
   if (!mesa_format)
      return GL_FALSE;

   /* Create the temporary source texture
    */
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = st_mesa_format_to_pipe_format(mesa_format);
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.last_level = 0;
   templ.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;
   src_tex = screen->texture_create(screen, &templ);

   if (!src_tex)
      return GL_FALSE;

   /* Put user's tex data into the temporary texture
    */
   tex_xfer = st_cond_flush_get_tex_transfer(st_context(ctx), src_tex,
					     0, 0, 0, /* face, level are zero */
					     PIPE_TRANSFER_WRITE,
					     0, 0, width, height); /* x, y, w, h */
   map = screen->transfer_map(screen, tex_xfer);

   _mesa_texstore(ctx, 2, GL_RGBA, mesa_format,
                  map,              /* dest ptr */
                  0, 0, 0,          /* dest x/y/z offset */
                  tex_xfer->stride, /* dest row stride (bytes) */
                  dstImageOffsets,  /* image offsets (for 3D only) */
                  width, height, 1, /* size */
                  format, type,     /* source format/type */
                  pixels,           /* source data */
                  unpack);          /* source data packing */

   screen->transfer_unmap(screen, tex_xfer);
   screen->tex_transfer_destroy(tex_xfer);

   /* copy / compress image */
   util_blit_pixels_tex(ctx->st->blit,
                        src_tex,          /* pipe_texture (src) */
                        0, 0,             /* src x0, y0 */
                        width, height,    /* src x1, y1 */
                        dst_surface,      /* pipe_surface (dst) */
                        xoffset, yoffset, /* dst x0, y0 */
                        xoffset + width,  /* dst x1 */
                        yoffset + height, /* dst y1 */
                        0.0,              /* z */
                        PIPE_TEX_MIPFILTER_NEAREST);

   pipe_surface_reference(&dst_surface, NULL);
   pipe_texture_reference(&src_tex, NULL);

   return GL_TRUE;
}


/**
 * Do glTexImage1/2/3D().
 */
static void
st_TexImage(GLcontext * ctx,
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
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct st_texture_image *stImage = st_texture_image(texImage);
   GLint postConvWidth, postConvHeight;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride = 0;
   struct gl_pixelstore_attrib unpackNB;
   enum pipe_transfer_usage transfer_usage = 0;

   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

   /* switch to "normal" */
   if (stObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj);
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

   postConvWidth = width;
   postConvHeight = height;

   stImage->face = _mesa_tex_target_to_face(target);
   stImage->level = level;

#if FEATURE_convolve
   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }
#endif

   _mesa_set_fetch_functions(texImage, dims);

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      /* must be a compressed format */
      texelBytes = 0;
   }
   else {
      texelBytes = _mesa_get_format_bytes(texImage->TexFormat);
      
      /* Minimum pitch of 32 bytes */
      if (postConvWidth * texelBytes < 32) {
	 postConvWidth = 32 / texelBytes;
	 texImage->RowStride = postConvWidth;
      }
      
      /* we'll set RowStride elsewhere when the texture is a "mapped" state */
      /*assert(texImage->RowStride == postConvWidth);*/
   }

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (stImage->pt) {
      pipe_texture_reference(&stImage->pt, NULL);
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
      if (stObj->teximage_realloc ||
          level > (GLint) stObj->pt->last_level ||
          !st_texture_match_image(stObj->pt, &stImage->base,
                                  stImage->face, stImage->level)) {
         DBG("release it\n");
         pipe_texture_reference(&stObj->pt, NULL);
         assert(!stObj->pt);
         stObj->teximage_realloc = FALSE;
      }
   }

   if (width == 0 || height == 0 || depth == 0) {
      /* stop after freeing old image */
      return;
   }

   if (!stObj->pt) {
      guess_and_alloc_texture(ctx->st, stObj, stImage);
      if (!stObj->pt) {
         /* Probably out of memory.
          * Try flushing any pending rendering, then retry.
          */
         st_finish(ctx->st);
         guess_and_alloc_texture(ctx->st, stObj, stImage);
         if (!stObj->pt) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
            return;
         }
      }
   }

   assert(!stImage->pt);

   if (stObj->pt &&
       st_texture_match_image(stObj->pt, &stImage->base,
                                 stImage->face, stImage->level)) {

      pipe_texture_reference(&stImage->pt, stObj->pt);
      assert(stImage->pt);
   }

   if (!stImage->pt)
      DBG("XXX: Image did not fit into texture - storing in local memory!\n");

   /* st_CopyTexImage calls this function with pixels == NULL, with
    * the expectation that the texture will be set up but nothing
    * more will be done.  This is where those calls return:
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

   /* Note: we can't check for pixels==NULL until after we've allocated
    * memory for the texture.
    */

   /* See if we can do texture compression with a blit/render.
    */
   if (!compressed_src &&
       !ctx->Mesa_DXTn &&
       _mesa_is_format_compressed(texImage->TexFormat) &&
       screen->is_format_supported(screen,
                                   stImage->pt->format,
                                   stImage->pt->target,
                                   PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
      if (!pixels)
         goto done;

      if (compress_with_blit(ctx, target, level, 0, 0, 0, width, height, depth,
                             format, type, pixels, unpack, texImage)) {
         goto done;
      }
   }

   if (stImage->pt) {
      if (format == GL_DEPTH_COMPONENT &&
          util_format_is_depth_and_stencil(stImage->pt->format))
         transfer_usage = PIPE_TRANSFER_READ_WRITE;
      else
         transfer_usage = PIPE_TRANSFER_WRITE;

      texImage->Data = st_texture_image_map(ctx->st, stImage, 0,
                                            transfer_usage, 0, 0,
                                            stImage->base.Width,
                                            stImage->base.Height);
      if(stImage->transfer)
         dstRowStride = stImage->transfer->stride;
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      if (_mesa_is_format_compressed(texImage->TexFormat)) {
         sizeInBytes = _mesa_format_image_size(texImage->TexFormat,
                                               texImage->Width,
                                               texImage->Height,
                                               texImage->Depth);
         dstRowStride = _mesa_format_row_stride(texImage->TexFormat, width);
         assert(dims != 3);
      }
      else {
         dstRowStride = postConvWidth * texelBytes;
         sizeInBytes = depth * dstRowStride * postConvHeight;
      }

      texImage->Data = _mesa_align_malloc(sizeInBytes, 16);
   }

   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
      return;
   }

   if (!pixels)
      goto done;

   DBG("Upload image %dx%dx%d row_len %x pitch %x\n",
       width, height, depth, width * texelBytes, dstRowStride);

   /* Copy data.  Would like to know when it's ok for us to eg. use
    * the blitter to copy.  Or, use the hardware to do the format
    * conversion and copy:
    */
   if (compressed_src) {
      const GLuint srcImageStride = _mesa_format_row_stride(texImage->TexFormat, width);
      if(dstRowStride == srcImageStride)
         memcpy(texImage->Data, pixels, imageSize);
      else
      {
         char *dst = texImage->Data;
         const char *src = pixels;
         GLuint i, bw, bh, lines;
         _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);
         lines = (height + bh - 1) / bh;

         for(i = 0; i < lines; ++i)
         {
            memcpy(dst, src, srcImageStride);
            dst += dstRowStride;
            src += srcImageStride;
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
	    st_texture_image_unmap(ctx->st, stImage);
            /* map next slice of 3D texture */
	    texImage->Data = st_texture_image_map(ctx->st, stImage, i + 1,
                                                  transfer_usage, 0, 0,
                                                  stImage->base.Width,
                                                  stImage->base.Height);
	    src += srcImageStride;
	 }
      }
   }

done:
   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (stImage->pt && texImage->Data) {
      st_texture_image_unmap(ctx->st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_TexImage3D(GLcontext * ctx,
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
st_TexImage2D(GLcontext * ctx,
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
st_TexImage1D(GLcontext * ctx,
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
st_CompressedTexImage2D(GLcontext *ctx, GLenum target, GLint level,
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
decompress_with_blit(GLcontext * ctx, GLenum target, GLint level,
                     GLenum format, GLenum type, GLvoid *pixels,
                     struct gl_texture_object *texObj,
                     struct gl_texture_image *texImage)
{
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_texture_image *stImage = st_texture_image(texImage);
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   struct pipe_surface *dst_surface;
   struct pipe_texture *dst_texture;
   struct pipe_transfer *tex_xfer;

   /* create temp / dest surface */
   if (!util_create_rgba_surface(screen, width, height,
                                 &dst_texture, &dst_surface)) {
      _mesa_problem(ctx, "util_create_rgba_surface() failed "
                    "in decompress_with_blit()");
      return;
   }

   /* blit/render/decompress */
   util_blit_pixels_tex(ctx->st->blit,
                        stImage->pt,      /* pipe_texture (src) */
                        0, 0,             /* src x0, y0 */
                        width, height,    /* src x1, y1 */
                        dst_surface,      /* pipe_surface (dst) */
                        0, 0,             /* dst x0, y0 */
                        width, height,    /* dst x1, y1 */
                        0.0,              /* z */
                        PIPE_TEX_MIPFILTER_NEAREST);

   /* map the dst_surface so we can read from it */
   tex_xfer = st_cond_flush_get_tex_transfer(st_context(ctx),
					     dst_texture, 0, 0, 0,
					     PIPE_TRANSFER_READ,
					     0, 0, width, height);

   pixels = _mesa_map_pbo_dest(ctx, &ctx->Pack, pixels);

   /* copy/pack data into user buffer */
   if (st_equal_formats(stImage->pt->format, format, type)) {
      /* memcpy */
      const uint bytesPerRow = width * util_format_get_blocksize(stImage->pt->format);
      ubyte *map = screen->transfer_map(screen, tex_xfer);
      GLuint row;
      for (row = 0; row < height; row++) {
         GLvoid *dest = _mesa_image_address2d(&ctx->Pack, pixels, width,
                                              height, format, type, row, 0);
         memcpy(dest, map, bytesPerRow);
         map += tex_xfer->stride;
      }
      screen->transfer_unmap(screen, tex_xfer);
   }
   else {
      /* format translation via floats */
      GLuint row;
      for (row = 0; row < height; row++) {
         const GLbitfield transferOps = 0x0; /* bypassed for glGetTexImage() */
         GLfloat rgba[4 * MAX_WIDTH];
         GLvoid *dest = _mesa_image_address2d(&ctx->Pack, pixels, width,
                                              height, format, type, row, 0);

         if (ST_DEBUG & DEBUG_FALLBACK)
            debug_printf("%s: fallback format translation\n", __FUNCTION__);

         /* get float[4] rgba row from surface */
         pipe_get_tile_rgba(tex_xfer, 0, row, width, 1, rgba);

         _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                    type, dest, &ctx->Pack, transferOps);
      }
   }

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);

   screen->tex_transfer_destroy(tex_xfer);

   /* destroy the temp / dest surface */
   util_destroy_rgba_surface(dst_texture, dst_surface);
}



/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
st_get_tex_image(GLcontext * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage, GLboolean compressed_dst)
{
   struct st_texture_image *stImage = st_texture_image(texImage);
   const GLuint dstImageStride =
      _mesa_image_image_stride(&ctx->Pack, texImage->Width, texImage->Height,
                               format, type);
   GLuint depth, i;
   GLubyte *dest;

   if (stImage->pt &&
       util_format_is_compressed(stImage->pt->format) &&
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
      unsigned face = _mesa_tex_target_to_face(target);

      st_teximage_flush_before_map(ctx->st, stImage->pt, face, level,
				   PIPE_TRANSFER_READ);

      texImage->Data = st_texture_image_map(ctx->st, stImage, 0,
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
	 st_texture_image_unmap(ctx->st, stImage);
         /* map next slice of 3D texture */
	 texImage->Data = st_texture_image_map(ctx->st, stImage, i + 1,
                                               PIPE_TRANSFER_READ, 0, 0,
                                               stImage->base.Width,
                                               stImage->base.Height);
	 dest += dstImageStride;
      }
   }

   texImage->Depth = depth;

   /* Unmap */
   if (stImage->pt) {
      st_texture_image_unmap(ctx->st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_GetTexImage(GLcontext * ctx, GLenum target, GLint level,
               GLenum format, GLenum type, GLvoid * pixels,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, format, type, pixels, texObj, texImage,
                    GL_FALSE);
}


static void
st_GetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
                         GLvoid *pixels,
                         struct gl_texture_object *texObj,
                         struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, 0, 0, pixels, texObj, texImage,
                    GL_TRUE);
}



static void
st_TexSubimage(GLcontext *ctx, GLint dims, GLenum target, GLint level,
               GLint xoffset, GLint yoffset, GLint zoffset,
               GLint width, GLint height, GLint depth,
               GLenum format, GLenum type, const void *pixels,
               const struct gl_pixelstore_attrib *packing,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
   struct pipe_screen *screen = ctx->st->pipe->screen;
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

   /* See if we can do texture compression with a blit/render.
    */
   if (!ctx->Mesa_DXTn &&
       _mesa_is_format_compressed(texImage->TexFormat) &&
       screen->is_format_supported(screen,
                                   stImage->pt->format,
                                   stImage->pt->target,
                                   PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
      if (compress_with_blit(ctx, target, level,
                             xoffset, yoffset, zoffset,
                             width, height, depth,
                             format, type, pixels, packing, texImage)) {
         goto done;
      }
   }

   /* Map buffer if necessary.  Need to lock to prevent other contexts
    * from uploading the buffer under us.
    */
   if (stImage->pt) {
      unsigned face = _mesa_tex_target_to_face(target);

      if (format == GL_DEPTH_COMPONENT &&
          util_format_is_depth_and_stencil(stImage->pt->format))
         transfer_usage = PIPE_TRANSFER_READ_WRITE;
      else
         transfer_usage = PIPE_TRANSFER_WRITE;

      st_teximage_flush_before_map(ctx->st, stImage->pt, face, level,
				   transfer_usage);
      texImage->Data = st_texture_image_map(ctx->st, stImage, zoffset, 
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
	 st_texture_image_unmap(ctx->st, stImage);
         /* map next slice of 3D texture */
	 texImage->Data = st_texture_image_map(ctx->st, stImage,
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
      st_texture_image_unmap(ctx->st, stImage);
      texImage->Data = NULL;
   }
}



static void
st_TexSubImage3D(GLcontext *ctx, GLenum target, GLint level,
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
st_TexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
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
st_TexSubImage1D(GLcontext *ctx, GLenum target, GLint level,
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
st_CompressedTexSubImage1D(GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLsizei width,
                           GLenum format,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage)
{
   assert(0);
}


static void
st_CompressedTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint yoffset,
                           GLsizei width, GLint height,
                           GLenum format,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage)
{
   struct st_texture_image *stImage = st_texture_image(texImage);
   int srcBlockStride;
   int dstBlockStride;
   int y;
   enum pipe_format pformat= stImage->pt->format;

   if (stImage->pt) {
      unsigned face = _mesa_tex_target_to_face(target);

      st_teximage_flush_before_map(ctx->st, stImage->pt, face, level,
				   PIPE_TRANSFER_WRITE);
      texImage->Data = st_texture_image_map(ctx->st, stImage, 0, 
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
   assert(width % util_format_get_blockwidth(pformat) == 0);
   assert(height % util_format_get_blockheight(pformat) == 0);

   for (y = 0; y < height; y += util_format_get_blockheight(pformat)) {
      /* don't need to adjust for xoffset and yoffset as st_texture_image_map does that */
      const char *src = (const char*)data + srcBlockStride * util_format_get_nblocksy(pformat, y);
      char *dst = (char*)texImage->Data + dstBlockStride * util_format_get_nblocksy(pformat, y);
      memcpy(dst, src, util_format_get_stride(pformat, width));
   }

   if (stImage->pt) {
      st_texture_image_unmap(ctx->st, stImage);
      texImage->Data = NULL;
   }
}


static void
st_CompressedTexSubImage3D(GLcontext *ctx, GLenum target, GLint level,
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
fallback_copy_texsubimage(GLcontext *ctx, GLenum target, GLint level,
                          struct st_renderbuffer *strb,
                          struct st_texture_image *stImage,
                          GLenum baseFormat,
                          GLint destX, GLint destY, GLint destZ,
                          GLint srcX, GLint srcY,
                          GLsizei width, GLsizei height)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *src_trans;
   GLvoid *texDest;
   enum pipe_transfer_usage transfer_usage;
   
   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   assert(width <= MAX_WIDTH);

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      srcY = strb->Base.Height - srcY - height;
   }

   src_trans = st_cond_flush_get_tex_transfer( st_context(ctx),
					       strb->texture,
					       0, 0, 0,
					       PIPE_TRANSFER_READ,
					       srcX, srcY,
					       width, height);

   if ((baseFormat == GL_DEPTH_COMPONENT ||
        baseFormat == GL_DEPTH_STENCIL) &&
       util_format_is_depth_and_stencil(stImage->pt->format))
      transfer_usage = PIPE_TRANSFER_READ_WRITE;
   else
      transfer_usage = PIPE_TRANSFER_WRITE;

   st_teximage_flush_before_map(ctx->st, stImage->pt, 0, 0,
				transfer_usage);

   texDest = st_texture_image_map(ctx->st, stImage, 0, transfer_usage,
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
         pipe_get_tile_z(src_trans, 0, srcY, width, 1, data);
         if (scaleOrBias) {
            _mesa_scale_and_bias_depth_uint(ctx, width, data);
         }
         pipe_put_tile_z(stImage->transfer, 0, row, width, 1, data);
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
         pipe_get_tile_rgba(src_trans, 0, 0, width, height, tempSrc);

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

   st_texture_image_unmap(ctx->st, stImage);
   screen->tex_transfer_destroy(src_trans);
}



/**
 * If the format of the src renderbuffer and the format of the dest
 * texture are compatible (in terms of blitting), return a TGSI writemask
 * to be used during the blit.
 * If the src/dest are incompatible, return 0.
 */
static unsigned
compatible_src_dst_formats(GLcontext *ctx,
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
st_copy_texsubimage(GLcontext *ctx,
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
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   enum pipe_format dest_format, src_format;
   GLboolean use_fallback = GL_TRUE;
   GLboolean matching_base_formats;
   GLuint format_writemask;
   struct pipe_surface *dest_surface = NULL;
   GLboolean do_flip = (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP);

   /* any rendering in progress must flushed before we grab the fb image */
   st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* make sure finalize_textures has been called? 
    */
   if (0) st_validate_state(ctx->st);

   /* determine if copying depth or color data */
   if (texBaseFormat == GL_DEPTH_COMPONENT ||
       texBaseFormat == GL_DEPTH_STENCIL) {
      strb = st_renderbuffer(fb->_DepthBuffer);
   }
   else {
      /* texBaseFormat == GL_RGB, GL_RGBA, GL_ALPHA, etc */
      strb = st_renderbuffer(fb->_ColorReadBuffer);
   }

   if (!strb || !strb->surface || !stImage->pt) {
      debug_printf("%s: null strb or stImage\n", __FUNCTION__);
      return;
   }

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

      if (pipe->surface_copy &&
          matching_base_formats &&
          src_format == dest_format &&
          !do_flip) 
      {
         /* use surface_copy() / blit */

         dest_surface = screen->get_tex_surface(screen, stImage->pt,
                                                stImage->face, stImage->level,
                                                destZ,
                                                PIPE_BUFFER_USAGE_GPU_WRITE);

         /* for surface_copy(), y=0=top, always */
         pipe->surface_copy(pipe,
                            /* dest */
                            dest_surface,
                            destX, destY,
                            /* src */
                            strb->surface,
                            srcX, srcY,
                            /* size */
                            width, height);
         use_fallback = GL_FALSE;
      }
      else if (format_writemask &&
               texBaseFormat != GL_DEPTH_COMPONENT &&
               texBaseFormat != GL_DEPTH_STENCIL &&
               screen->is_format_supported(screen, src_format,
                                           PIPE_TEXTURE_2D, 
                                           PIPE_TEXTURE_USAGE_SAMPLER,
                                           0) &&
               screen->is_format_supported(screen, dest_format,
                                           PIPE_TEXTURE_2D, 
                                           PIPE_TEXTURE_USAGE_RENDER_TARGET,
                                           0)) {
         /* draw textured quad to do the copy */
         GLint srcY0, srcY1;

         dest_surface = screen->get_tex_surface(screen, stImage->pt,
                                                stImage->face, stImage->level,
                                                destZ,
                                                PIPE_BUFFER_USAGE_GPU_WRITE);

         if (do_flip) {
            srcY1 = strb->Base.Height - srcY - height;
            srcY0 = srcY1 + height;
         }
         else {
            srcY0 = srcY;
            srcY1 = srcY0 + height;
         }
         util_blit_pixels_writemask(ctx->st->blit,
                                    strb->surface,
                                    srcX, srcY0,
                                    srcX + width, srcY1,
                                    dest_surface,
                                    destX, destY,
                                    destX + width, destY + height,
                                    0.0, PIPE_TEX_MIPFILTER_NEAREST,
                                    format_writemask);
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
st_CopyTexImage1D(GLcontext * ctx, GLenum target, GLint level,
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
st_CopyTexImage2D(GLcontext * ctx, GLenum target, GLint level,
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
st_CopyTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint x, GLint y, GLsizei width)
{
   const GLint yoffset = 0, zoffset = 0;
   const GLsizei height = 1;
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
st_CopyTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   const GLint zoffset = 0;
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
st_CopyTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   st_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,  /* destX,Y,Z */
                       x, y, width, height);  /* src X, Y, size */
}


static void
copy_image_data_to_texture(struct st_context *st,
			   struct st_texture_object *stObj,
                           GLuint dstLevel,
			   struct st_texture_image *stImage)
{
   if (stImage->pt) {
      /* Copy potentially with the blitter:
       */
      st_texture_image_copy(st->pipe,
                            stObj->pt, dstLevel,  /* dest texture, level */
                            stImage->pt, /* src texture */
                            stImage->face);

      pipe_texture_reference(&stImage->pt, NULL);
   }
   else if (stImage->base.Data) {
      /* More straightforward upload.  
       */
      st_teximage_flush_before_map(st, stObj->pt, stImage->face, dstLevel,
				   PIPE_TRANSFER_WRITE);

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

   pipe_texture_reference(&stImage->pt, stObj->pt);
}


/**
 * Called during state validation.  When this function is finished,
 * the texture object should be ready for rendering.
 * \return GL_TRUE for success, GL_FALSE for failure (out of mem)
 */
GLboolean
st_finalize_texture(GLcontext *ctx,
		    struct pipe_context *pipe,
		    struct gl_texture_object *tObj,
		    GLboolean *needFlush)
{
   struct st_texture_object *stObj = st_texture_object(tObj);
   const GLuint nr_faces = (stObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint blockSize, face;
   struct st_texture_image *firstImage;

   *needFlush = GL_FALSE;

   if (stObj->base._Complete) {
      /* The texture is complete and we know exactly how many mipmap levels
       * are present/needed.  This is conditional because we may be called
       * from the st_generate_mipmap() function when the texture object is
       * incomplete.  In that case, we'll have set stObj->lastLevel before
       * we get here.
       */
      if (stObj->base.MinFilter == GL_LINEAR ||
          stObj->base.MinFilter == GL_NEAREST)
         stObj->lastLevel = stObj->base.BaseLevel;
      else
         stObj->lastLevel = stObj->base._MaxLevel - stObj->base.BaseLevel;
   }

   firstImage = st_texture_image(stObj->base.Image[0][stObj->base.BaseLevel]);

   /* If both firstImage and stObj point to a texture which can contain
    * all active images, favour firstImage.  Note that because of the
    * completeness requirement, we know that the image dimensions
    * will match.
    */
   if (firstImage->pt &&
       firstImage->pt != stObj->pt &&
       firstImage->pt->last_level >= stObj->lastLevel) {
      pipe_texture_reference(&stObj->pt, firstImage->pt);
   }

   /* bytes per pixel block (blocks are usually 1x1) */
   blockSize = _mesa_get_format_bytes(firstImage->base.TexFormat);

   /* If we already have a gallium texture, check that it matches the texture
    * object's format, target, size, num_levels, etc.
    */
   if (stObj->pt) {
      const enum pipe_format fmt =
         st_mesa_format_to_pipe_format(firstImage->base.TexFormat);
      if (stObj->pt->target != gl_target_to_pipe(stObj->base.Target) ||
          stObj->pt->format != fmt ||
          stObj->pt->last_level < stObj->lastLevel ||
          stObj->pt->width0 != firstImage->base.Width2 ||
          stObj->pt->height0 != firstImage->base.Height2 ||
          stObj->pt->depth0 != firstImage->base.Depth2)
      {
         pipe_texture_reference(&stObj->pt, NULL);
         ctx->st->dirty.st |= ST_NEW_FRAMEBUFFER;
      }
   }

   /* May need to create a new gallium texture:
    */
   if (!stObj->pt) {
      const enum pipe_format fmt =
         st_mesa_format_to_pipe_format(firstImage->base.TexFormat);
      GLuint usage = default_usage(fmt);

      stObj->pt = st_texture_create(ctx->st,
                                    gl_target_to_pipe(stObj->base.Target),
                                    fmt,
                                    stObj->lastLevel,
                                    firstImage->base.Width2,
                                    firstImage->base.Height2,
                                    firstImage->base.Depth2,
                                    usage);

      if (!stObj->pt) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
         return GL_FALSE;
      }
   }

   /* Pull in any images not in the object's texture:
    */
   for (face = 0; face < nr_faces; face++) {
      GLuint level;
      for (level = 0; level <= stObj->lastLevel; level++) {
         struct st_texture_image *stImage =
            st_texture_image(stObj->base.Image[face][stObj->base.BaseLevel + level]);

         /* Need to import images in main memory or held in other textures.
          */
         if (stImage && stObj->pt != stImage->pt) {
            copy_image_data_to_texture(ctx->st, stObj, level, stImage);
	    *needFlush = GL_TRUE;
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
                                 GL_RGBA);

      st_TexImage(st->ctx, 2, target,
                  0, GL_RGBA,    /* level, intformat */
                  16, 16, 1, 0,  /* w, h, d, border */
                  GL_RGBA, GL_UNSIGNED_BYTE, pixels,
                  &st->ctx->DefaultPacking,
                  texObj, texImg,
                  0, 0);

      texObj->MinFilter = GL_NEAREST;
      texObj->MagFilter = GL_NEAREST;
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
   functions->UpdateTexturePalette = 0;

   functions->TextureMemCpy = do_memcpy;

   /* XXX Temporary until we can query pipe's texture sizes */
   functions->TestProxyTexImage = _mesa_test_proxy_teximage;
}
