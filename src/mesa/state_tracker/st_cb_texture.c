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

#include "main/imports.h"
#include "main/convolve.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/texcompress.h"
#include "main/texformat.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"

#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"
#include "state_tracker/st_cb_texture.h"
#include "state_tracker/st_format.h"
#include "state_tracker/st_mipmap_tree.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"


#define DBG if (0) printf


struct st_texture_object
{
   struct gl_texture_object base;       /* The "parent" object */

   /* The mipmap tree must include at least these levels once
    * validated:
    */
   GLuint firstLevel;
   GLuint lastLevel;

   /* Offset for firstLevel image:
    */
   GLuint textureOffset;

   /* On validation any active images held in main memory or in other
    * regions will be copied to this region and the old storage freed.
    */
   struct pipe_mipmap_tree *mt;

   GLboolean imageOverride;
   GLint depthOverride;
   GLuint pitchOverride;
};



struct st_texture_image
{
   struct gl_texture_image base;

   /* These aren't stored in gl_texture_image 
    */
   GLuint level;
   GLuint face;

   /* If stImage->mt != NULL, image data is stored here.
    * Else if stImage->base.Data != NULL, image is stored there.
    * Else there is no image data.
    */
   struct pipe_mipmap_tree *mt;
};




static INLINE struct st_texture_object *
st_texture_object(struct gl_texture_object *obj)
{
   return (struct st_texture_object *) obj;
}

static INLINE struct st_texture_image *
st_texture_image(struct gl_texture_image *img)
{
   return (struct st_texture_image *) img;
}


struct pipe_mipmap_tree *
st_get_texobj_mipmap_tree(struct gl_texture_object *texObj)
{
   struct st_texture_object *stObj = st_texture_object(texObj);
   return stObj->mt;
}


static unsigned
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


static int
compressed_num_bytes(GLuint mesaFormat)
{
   int bytes = 0;
   switch(mesaFormat) {
     
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
     bytes = 2;
     break;
     
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
     bytes = 4;
   default:
     break;
   }
   
   return bytes;
}




static GLboolean
st_IsTextureResident(GLcontext * ctx, struct gl_texture_object *texObj)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);

   return
      stObj->mt &&
      stObj->mt->region &&
      intel_is_region_resident(intel, stObj->mt->region);
#endif
   return 1;
}



static struct gl_texture_image *
st_NewTextureImage(GLcontext * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(st_texture_image);
}


static struct gl_texture_object *
st_NewTextureObject(GLcontext * ctx, GLuint name, GLenum target)
{
   struct st_texture_object *obj = CALLOC_STRUCT(st_texture_object);

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static void 
st_DeleteTextureObject(GLcontext *ctx,
			 struct gl_texture_object *texObj)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_texture_object *stObj = st_texture_object(texObj);

   if (stObj->mt)
      st_miptree_release(pipe, &stObj->mt);

   _mesa_delete_texture_object(ctx, texObj);
}


static void
st_FreeTextureImageData(GLcontext * ctx, struct gl_texture_image *texImage)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_texture_image *stImage = st_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (stImage->mt) {
      st_miptree_release(pipe, &stImage->mt);
   }

   if (texImage->Data) {
      free(texImage->Data);
      texImage->Data = NULL;
   }
}




/* ================================================================
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 * XXX Put this in src/mesa/main/imports.h ???
 */
#if defined(i386) || defined(__i386__)
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


/* The system memcpy (at least on ubuntu 5.10) has problems copying
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
   if ((((unsigned) src) & 63) || (((unsigned) dest) & 63)) {
      return __memcpy(dest, src, n);
   }
   else
      return memcpy(dest, src, n);
}


/* Functions to store texture images.  Where possible, mipmap_tree's
 * will be created or further instantiated with image data, otherwise
 * images will be stored in malloc'd memory.  A validation step is
 * required to pull those images into a mipmap tree, or otherwise
 * decide a fallback is required.
 */


static int
logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}


/* Otherwise, store it in memory if (Border != 0) or (any dimension ==
 * 1).
 *    
 * Otherwise, if max_level >= level >= min_level, create tree with
 * space for textures from min_level down to max_level.
 *
 * Otherwise, create tree with space for textures from (level
 * 0)..(1x1).  Consider pruning this tree at a validation if the
 * saving is worth it.
 */
static void
guess_and_alloc_mipmap_tree(struct pipe_context *pipe,
                            struct st_texture_object *stObj,
                            struct st_texture_image *stImage)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = stImage->base.Width;
   GLuint height = stImage->base.Height;
   GLuint depth = stImage->base.Depth;
   GLuint l2width, l2height, l2depth;
   GLuint i, comp_byte = 0;

   DBG("%s\n", __FUNCTION__);

   if (stImage->base.Border)
      return;

   if (stImage->level > stObj->base.BaseLevel &&
       (stImage->base.Width == 1 ||
        (stObj->base.Target != GL_TEXTURE_1D &&
         stImage->base.Height == 1) ||
        (stObj->base.Target == GL_TEXTURE_3D &&
         stImage->base.Depth == 1)))
      return;

   /* If this image disrespects BaseLevel, allocate from level zero.
    * Usually BaseLevel == 0, so it's unlikely to happen.
    */
   if (stImage->level < stObj->base.BaseLevel)
      firstLevel = 0;
   else
      firstLevel = stObj->base.BaseLevel;


   /* Figure out image dimensions at start level. 
    */
   for (i = stImage->level; i > firstLevel; i--) {
      width <<= 1;
      if (height != 1)
         height <<= 1;
      if (depth != 1)
         depth <<= 1;
   }

   /* Guess a reasonable value for lastLevel.  This is probably going
    * to be wrong fairly often and might mean that we have to look at
    * resizable buffers, or require that buffers implement lazy
    * pagetable arrangements.
    */
   if ((stObj->base.MinFilter == GL_NEAREST ||
        stObj->base.MinFilter == GL_LINEAR) &&
       stImage->level == firstLevel) {
      lastLevel = firstLevel;
   }
   else {
      l2width = logbase2(width);
      l2height = logbase2(height);
      l2depth = logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width, l2height), l2depth);
   }

   assert(!stObj->mt);
   if (stImage->base.IsCompressed)
      comp_byte = compressed_num_bytes(stImage->base.TexFormat->MesaFormat);
   stObj->mt = st_miptree_create(pipe,
                                 gl_target_to_pipe(stObj->base.Target),
                                 stImage->base.InternalFormat,
                                 firstLevel,
                                 lastLevel,
                                 width,
                                 height,
                                 depth,
                                 stImage->base.TexFormat->TexelBytes,
                                 comp_byte);

   stObj->mt->format
      = st_mesa_format_to_pipe_format(stImage->base.TexFormat->MesaFormat);

   DBG("%s - success\n", __FUNCTION__);
}




static GLuint
target_to_face(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return ((GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X);
   default:
      return 0;
   }
}



/* There are actually quite a few combinations this will work for,
 * more than what I've listed here.
 */
static GLboolean
check_pbo_format(GLint internalFormat,
                 GLenum format, GLenum type,
                 const struct gl_texture_format *mesa_format)
{
   switch (internalFormat) {
   case 4:
   case GL_RGBA:
      return (format == GL_BGRA &&
              (type == GL_UNSIGNED_BYTE ||
               type == GL_UNSIGNED_INT_8_8_8_8_REV) &&
              mesa_format == &_mesa_texformat_argb8888);
   case 3:
   case GL_RGB:
      return (format == GL_RGB &&
              type == GL_UNSIGNED_SHORT_5_6_5 &&
              mesa_format == &_mesa_texformat_rgb565);
   case GL_YCBCR_MESA:
      return (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE);
   default:
      return GL_FALSE;
   }
}


/* XXX: Do this for TexSubImage also:
 */
static GLboolean
try_pbo_upload(GLcontext *ctx,
               struct st_texture_image *stImage,
               const struct gl_pixelstore_attrib *unpack,
               GLint internalFormat,
               GLint width, GLint height,
               GLenum format, GLenum type, const void *pixels)
{
   return GL_FALSE;  /* XXX fix flushing/locking/blitting below */
#if 000
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_offset, dst_stride;

   if (!pbo ||
       ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      _mesa_printf("%s: failure 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   src_offset = (GLuint) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   dst_offset = st_miptree_image_offset(stImage->mt,
                                           stImage->face,
                                           stImage->level);

   dst_stride = stImage->mt->pitch;

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);
   {
      struct _DriBufferObject *src_buffer =
         intel_bufferobj_buffer(intel, pbo, INTEL_READ);

      /* Temporary hack: cast to _DriBufferObject:
       */
      struct _DriBufferObject *dst_buffer =
         (struct _DriBufferObject *)stImage->mt->region->buffer;


      intelEmitCopyBlit(intel,
                        stImage->mt->cpp,
                        src_stride, src_buffer, src_offset,
                        dst_stride, dst_buffer, dst_offset,
                        0, 0, 0, 0, width, height,
			GL_COPY);

      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);

   return GL_TRUE;
#endif
}



static GLboolean
try_pbo_zcopy(GLcontext *ctx,
              struct st_texture_image *stImage,
              const struct gl_pixelstore_attrib *unpack,
              GLint internalFormat,
              GLint width, GLint height,
              GLenum format, GLenum type, const void *pixels)
{
   return GL_FALSE;
}






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
              struct gl_texture_image *texImage, GLsizei imageSize, int compressed)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct st_texture_image *stImage = st_texture_image(texImage);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride;


   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

#if 0
   intelFlush(ctx);
#endif

   stImage->face = target_to_face(target);
   stImage->level = level;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   texImage->TexFormat = st_ChooseTextureFormat(ctx, internalFormat,
                                                  format, type);

   _mesa_set_fetch_functions(texImage, dims);

   if (texImage->TexFormat->TexelBytes == 0) {
      /* must be a compressed format */
      texelBytes = 0;
      texImage->IsCompressed = GL_TRUE;
      texImage->CompressedSize =
	 ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
					   texImage->Height, texImage->Depth,
					   texImage->TexFormat->MesaFormat);
   } else {
      texelBytes = texImage->TexFormat->TexelBytes;
      
      /* Minimum pitch of 32 bytes */
      if (postConvWidth * texelBytes < 32) {
	 postConvWidth = 32 / texelBytes;
	 texImage->RowStride = postConvWidth;
      }
      
      assert(texImage->RowStride == postConvWidth);
   }

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (stImage->mt) {
      st_miptree_release(pipe, &stImage->mt);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      _mesa_align_free(texImage->Data);
   }

   /* If this is the only texture image in the tree, could call
    * bmBufferData with NULL data to free the old block and avoid
    * waiting on any outstanding fences.
    */
   if (stObj->mt &&
       stObj->mt->first_level == level &&
       stObj->mt->last_level == level &&
       stObj->mt->target != PIPE_TEXTURE_CUBE &&
       !st_miptree_match_image(stObj->mt, &stImage->base,
                                  stImage->face, stImage->level)) {

      DBG("release it\n");
      st_miptree_release(pipe, &stObj->mt);
      assert(!stObj->mt);
   }

   if (!stObj->mt) {
      guess_and_alloc_mipmap_tree(pipe, stObj, stImage);
      if (!stObj->mt) {
	 DBG("guess_and_alloc_mipmap_tree: failed\n");
      }
   }

   assert(!stImage->mt);

   if (stObj->mt &&
       st_miptree_match_image(stObj->mt, &stImage->base,
                                 stImage->face, stImage->level)) {

      st_miptree_reference(&stImage->mt, stObj->mt);
      assert(stImage->mt);
   }

   if (!stImage->mt)
      DBG("XXX: Image did not fit into tree - storing in local memory!\n");

#if 0 /* XXX FIX when st_buffer_objects are in place */
   /* PBO fastpaths:
    */
   if (dims <= 2 &&
       stImage->mt &&
       intel_buffer_object(unpack->BufferObj) &&
       check_pbo_format(internalFormat, format,
                        type, stImage->base.TexFormat)) {

      DBG("trying pbo upload\n");

      /* Attempt to texture directly from PBO data (zero copy upload).
       *
       * Currently disable as it can lead to worse as well as better
       * performance (in particular when pipe_region_cow() is
       * required).
       */
      if (stObj->mt == stImage->mt &&
          stObj->mt->first_level == level &&
          stObj->mt->last_level == level) {

         if (try_pbo_zcopy(intel, stImage, unpack,
                           internalFormat,
                           width, height, format, type, pixels)) {

            DBG("pbo zcopy upload succeeded\n");
            return;
         }
      }


      /* Otherwise, attempt to use the blitter for PBO image uploads.
       */
      if (try_pbo_upload(intel, stImage, unpack,
                         internalFormat,
                         width, height, format, type, pixels)) {
         DBG("pbo upload succeeded\n");
         return;
      }

      DBG("pbo upload failed\n");
   }
#else
   (void) try_pbo_upload;
   (void) check_pbo_format;
   (void) try_pbo_zcopy;
#endif


   /* intelCopyTexImage calls this function with pixels == NULL, with
    * the expectation that the mipmap tree will be set up but nothing
    * more will be done.  This is where those calls return:
    */
   if (compressed) {
      pixels = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, pixels,
						      unpack,
						      "glCompressedTexImage");
   } else {
      pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, 1,
					   format, type,
					   pixels, unpack, "glTexImage");
   }
   if (!pixels)
      return;

   if (stImage->mt) {
      texImage->Data = st_miptree_image_map(pipe,
                                               stImage->mt,
                                               stImage->face,
                                               stImage->level,
                                               &dstRowStride,
                                               stImage->base.ImageOffsets);
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      if (texImage->IsCompressed) {
         sizeInBytes = texImage->CompressedSize;
         dstRowStride =
            _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, width);
         assert(dims != 3);
      }
      else {
         dstRowStride = postConvWidth * texelBytes;
         sizeInBytes = depth * dstRowStride * postConvHeight;
      }

      texImage->Data = malloc(sizeInBytes);
   }

   DBG("Upload image %dx%dx%d row_len %x pitch %x\n",
       width, height, depth, width * texelBytes, dstRowStride);

   /* Copy data.  Would like to know when it's ok for us to eg. use
    * the blitter to copy.  Or, use the hardware to do the format
    * conversion and copy:
    */
   if (compressed) {
     memcpy(texImage->Data, pixels, imageSize);
   }
   else if (!texImage->TexFormat->StoreImage(ctx, dims, 
                                             texImage->_BaseFormat, 
                                             texImage->TexFormat, 
                                             texImage->Data,
                                             0, 0, 0, /* dstX/Y/Zoffset */
                                             dstRowStride,
                                             texImage->ImageOffsets,
                                             width, height, depth,
                                             format, type, pixels, unpack)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
   }

   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (stImage->mt) {
      st_miptree_image_unmap(pipe, stImage->mt);
      texImage->Data = NULL;
   }

#if 0
   /* GL_SGIS_generate_mipmap -- this can be accelerated now.
    */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      intel_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif
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
   st_TexImage(ctx, 3, target, level,
                 internalFormat, width, height, depth, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
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
   st_TexImage(ctx, 2, target, level,
                 internalFormat, width, height, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
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
   st_TexImage(ctx, 1, target, level,
                 internalFormat, width, 1, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
}


static void
st_CompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
				GLint internalFormat,
				GLint width, GLint height, GLint border,
				GLsizei imageSize, const GLvoid *data,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage )
{
   st_TexImage(ctx, 2, target, level,
		 internalFormat, width, height, 1, border,
		 0, 0, data, &ctx->Unpack, texObj, texImage, imageSize, 1);
}


/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
st_get_tex_image(GLcontext * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage, int compressed)
{
   /*
   struct intel_context *intel = intel_context(ctx);
   */
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_texture_image *stImage = st_texture_image(texImage);

   /* Map */
   if (stImage->mt) {
      /* Image is stored in hardware format in a buffer managed by the
       * kernel.  Need to explicitly map and unmap it.
       */
      stImage->base.Data =
         st_miptree_image_map(pipe,
                                 stImage->mt,
                                 stImage->face,
                                 stImage->level,
                                 &stImage->base.RowStride,
                                 stImage->base.ImageOffsets);
      stImage->base.RowStride /= stImage->mt->cpp;
   }
   else {
      /* Otherwise, the image should actually be stored in
       * stImage->base.Data.  This is pretty confusing for
       * everybody, I'd much prefer to separate the two functions of
       * texImage->Data - storage for texture images in main memory
       * and access (ie mappings) of images.  In other words, we'd
       * create a new texImage->Map field and leave Data simply for
       * storage.
       */
      assert(stImage->base.Data);
   }


   if (compressed) {
      _mesa_get_compressed_teximage(ctx, target, level, pixels,
				    texObj, texImage);
   } else {
      _mesa_get_teximage(ctx, target, level, format, type, pixels,
			 texObj, texImage);
   }
     

   /* Unmap */
   if (stImage->mt) {
      st_miptree_image_unmap(pipe, stImage->mt);
      stImage->base.Data = NULL;
   }
}


static void
st_GetTexImage(GLcontext * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, format, type, pixels,
                    texObj, texImage, 0);
}


static void
st_GetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
			   GLvoid *pixels,
			   const struct gl_texture_object *texObj,
			   const struct gl_texture_image *texImage)
{
   st_get_tex_image(ctx, target, level, 0, 0, pixels,
                    (struct gl_texture_object *) texObj,
                    (struct gl_texture_image *) texImage, 1);
}



static void
st_TexSubimage(GLcontext * ctx,
                 GLint dims,
                 GLenum target, GLint level,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLint width, GLint height, GLint depth,
                 GLenum format, GLenum type, const void *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_texture_image *stImage = st_texture_image(texImage);
   GLuint dstRowStride;

   DBG("%s target %s level %d offset %d,%d %dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       level, xoffset, yoffset, width, height);

#if 0
   intelFlush(ctx);
#endif

   pixels =
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth, format,
                                  type, pixels, packing, "glTexSubImage2D");
   if (!pixels)
      return;

   /* Map buffer if necessary.  Need to lock to prevent other contexts
    * from uploading the buffer under us.
    */
   if (stImage->mt) 
      texImage->Data = st_miptree_image_map(pipe,
                                               stImage->mt,
                                               stImage->face,
                                               stImage->level,
                                               &dstRowStride,
                                               texImage->ImageOffsets);

   assert(dstRowStride);

   if (!texImage->TexFormat->StoreImage(ctx, dims, texImage->_BaseFormat,
                                        texImage->TexFormat,
                                        texImage->Data,
                                        xoffset, yoffset, zoffset,
                                        dstRowStride,
                                        texImage->ImageOffsets,
                                        width, height, depth,
                                        format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

#if 0
   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif

   _mesa_unmap_teximage_pbo(ctx, packing);

   if (stImage->mt) {
      st_miptree_image_unmap(pipe, stImage->mt);
      texImage->Data = NULL;
   }
}



static void
st_TexSubImage3D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 3, target, level,
                  xoffset, yoffset, zoffset,
                  width, height, depth,
                  format, type, pixels, packing, texObj, texImage);
}



static void
st_TexSubImage2D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 2, target, level,
                  xoffset, yoffset, 0,
                  width, height, 1,
                  format, type, pixels, packing, texObj, texImage);
}


static void
st_TexSubImage1D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset,
                   GLsizei width,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   st_TexSubimage(ctx, 1, target, level,
                  xoffset, 0, 0,
                  width, 1, 1,
                  format, type, pixels, packing, texObj, texImage);
}



/**
 * Return 0 for GL_TEXTURE_CUBE_MAP_POSITIVE_X,
 *        1 for GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
 *        etc.
 * XXX duplicated from main/teximage.c
 */
static uint
texture_face(GLenum target)
{
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)
      return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   else
      return 0;
}



/**
 * Do a CopyTexSubImage operation by mapping the source region and
 * dest region and using get_tile()/put_tile() to access the pixels/texels.
 *
 * Note: srcY=0=TOP of renderbuffer
 */
static void
fallback_copy_texsubimage(GLcontext *ctx,
                          GLenum target,
                          GLint level,
                          struct st_renderbuffer *strb,
                          struct st_texture_image *stImage,
                          GLenum baseFormat,
                          GLint destX, GLint destY, GLint destZ,
                          GLint srcX, GLint srcY,
                          GLsizei width, GLsizei height)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const uint face = texture_face(target);
   struct pipe_mipmap_tree *mt = stImage->mt;
   struct pipe_surface *src_surf, *dest_surf;
   GLfloat *data;
   GLint row, yStep;

   /* determine bottom-to-top vs. top-to-bottom order */
   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      destY = height - 1 - destY;
      yStep = -1;
   }
   else {
      yStep = 1;
   }

   src_surf = strb->surface;

   dest_surf = pipe->get_tex_surface(pipe, mt,
                                    face, level, destZ);

   (void) pipe->region_map(pipe, dest_surf->region);
   (void) pipe->region_map(pipe, src_surf->region);

   /* buffer for one row */
   data = (GLfloat *) malloc(width * 4 * sizeof(GLfloat));

   /* do copy row by row */
   for (row = 0; row < height; row++) {
      src_surf->get_tile(src_surf, srcX, srcY + row, width, 1, data);

      /* XXX we're ignoring convolution for now */
      if (ctx->_ImageTransferState) {
         _mesa_apply_rgba_transfer_ops(ctx,
                            ctx->_ImageTransferState & ~IMAGE_CONVOLUTION_BIT,
                            width, (GLfloat (*)[4])data);
      }

      dest_surf->put_tile(dest_surf, destX, destY, width, 1, data);
      destY += yStep;
   }


   (void) pipe->region_unmap(pipe, dest_surf->region);
   (void) pipe->region_unmap(pipe, src_surf->region);

   free(data);
}




/**
 * Do a CopyTex[Sub]Image using an optimized hardware (blit) path.
 * Note that the region to copy has already been clip tested.
 *
 * Note: srcY=0=Bottom of renderbuffer
 *
 * \return GL_TRUE if success, GL_FALSE if failure (use a fallback)
 */
static void
do_copy_texsubimage(GLcontext *ctx,
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
   GLenum baseFormat = texImage->InternalFormat;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb;
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_region *src_region, *dest_region;
   uint dest_offset, src_offset;
   uint dest_format, src_format;

   (void) texImage;

   /* determine if copying depth or color data */
   if (baseFormat == GL_DEPTH_COMPONENT) {
      strb = st_renderbuffer(fb->_DepthBuffer);
   }
   else if (baseFormat == GL_DEPTH_STENCIL_EXT) {
      strb = st_renderbuffer(fb->_StencilBuffer);
   }
   else {
      /* baseFormat == GL_RGB, GL_RGBA, GL_ALPHA, etc */
      strb = st_renderbuffer(fb->_ColorReadBuffer);
   }

   assert(strb);
   assert(strb->surface);
   assert(stImage->mt);

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      srcY = strb->Base.Height - srcY - height;
   }

   src_format = strb->surface->format;
   dest_format = stImage->mt->format;

   src_region = strb->surface->region;
   dest_region = stImage->mt->region;

   if (src_format == dest_format &&
       ctx->_ImageTransferState == 0x0 &&
       src_region &&
       dest_region &&
       src_region->cpp == dest_region->cpp) {
      /* do blit-style copy */
      src_offset = 0;
      dest_offset = st_miptree_image_offset(stImage->mt,
                                            stImage->face,
                                            stImage->level);

      /* XXX may need to invert image depending on window
       * vs. user-created FBO
       */

#if 00 /* XXX FIX flush/locking */
      intelFlush(ctx);
      /* XXX still need the lock ? */
      LOCK_HARDWARE(intel);
#endif

#if 0
      /* A bit of fiddling to get the blitter to work with -ve
       * pitches.  But we get a nice inverted blit this way, so it's
       * worth it:
       */
      intelEmitCopyBlit(intel,
                        stImage->mt->cpp,
                        -src->pitch,
                        src->buffer,
                        src->height * src->pitch * src->cpp,
                        stImage->mt->pitch,
                        stImage->mt->region->buffer,
                        dest_offset,
                        x, y + height, dstx, dsty, width, height,
                        GL_COPY); /* ? */
      intel_batchbuffer_flush(intel->batch);
#else

      pipe->region_copy(pipe,
                        /* dest */
                        dest_region,
                        dest_offset,
                        destX, destY,
                        /* src */
                        src_region,
                        src_offset,
                        srcX, srcY,
                        /* size */
                        width, height);
#endif

#if 0
      UNLOCK_HARDWARE(intel);
#endif
   }
   else {
      fallback_copy_texsubimage(ctx, target, level,
                                strb, stImage, baseFormat,
                                destX, destY, destZ,
                                srcX, srcY, width, height);
   }


#if 0
   /* GL_SGIS_generate_mipmap -- this can be accelerated now.
    * XXX Add a ctx->Driver.GenerateMipmaps() function?
    */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      intel_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif

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

#if 0
   if (border)
      goto fail;
#endif

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                          width, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   do_copy_texsubimage(ctx, target, level,
                       0, 0, 0,
                       x, y, width, 1);
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

#if 0
   if (border)
      goto fail;
#endif

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                          width, height, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);


   do_copy_texsubimage(ctx, target, level,
                       0, 0, 0,
                       x, y, width, height);
}


static void
st_CopyTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint x, GLint y, GLsizei width)
{
   const GLint yoffset = 0, zoffset = 0;
   const GLsizei height = 1;
   do_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,
                       x, y, width, height);
}


static void
st_CopyTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   const GLint zoffset = 0;
   do_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,
                       x, y, width, height);
}


static void
st_CopyTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   do_copy_texsubimage(ctx, target, level,
                       xoffset, yoffset, zoffset,
                       x, y, width, height);
}




/**
 * Compute which mipmap levels that really need to be sent to the hardware.
 * This depends on the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
 */
static void
calculate_first_last_level(struct st_texture_object *stObj)
{
   struct gl_texture_object *tObj = &stObj->base;
   const struct gl_texture_image *const baseImage =
      tObj->Image[0][tObj->BaseLevel];

   /* These must be signed values.  MinLod and MaxLod can be negative numbers,
    * and having firstLevel and lastLevel as signed prevents the need for
    * extra sign checks.
    */
   int firstLevel;
   int lastLevel;

   /* Yes, this looks overly complicated, but it's all needed.
    */
   switch (tObj->Target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
      if (tObj->MinFilter == GL_NEAREST || tObj->MinFilter == GL_LINEAR) {
         /* GL_NEAREST and GL_LINEAR only care about GL_TEXTURE_BASE_LEVEL.
          */
         firstLevel = lastLevel = tObj->BaseLevel;
      }
      else {
         firstLevel = tObj->BaseLevel + (GLint) (tObj->MinLod + 0.5);
         firstLevel = MAX2(firstLevel, tObj->BaseLevel);
         lastLevel = tObj->BaseLevel + (GLint) (tObj->MaxLod + 0.5);
         lastLevel = MAX2(lastLevel, tObj->BaseLevel);
         lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
         lastLevel = MIN2(lastLevel, tObj->MaxLevel);
         lastLevel = MAX2(firstLevel, lastLevel);       /* need at least one level */
      }
      break;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_4D_SGIS:
      firstLevel = lastLevel = 0;
      break;
   default:
      return;
   }

   /* save these values */
   stObj->firstLevel = firstLevel;
   stObj->lastLevel = lastLevel;
}


static void
copy_image_data_to_tree(struct pipe_context *pipe,
                        struct st_texture_object *stObj,
                        struct st_texture_image *stImage)
{
   if (stImage->mt) {
      /* Copy potentially with the blitter:
       */
      st_miptree_image_copy(pipe,
                               stObj->mt,
                               stImage->face,
                               stImage->level, stImage->mt);

      st_miptree_release(pipe, &stImage->mt);
   }
   else {
      assert(stImage->base.Data != NULL);

      /* More straightforward upload.  
       */
      st_miptree_image_data(pipe,
                               stObj->mt,
                               stImage->face,
                               stImage->level,
                               stImage->base.Data,
                               stImage->base.RowStride,
                               stImage->base.RowStride *
                               stImage->base.Height);
      _mesa_align_free(stImage->base.Data);
      stImage->base.Data = NULL;
   }

   st_miptree_reference(&stImage->mt, stObj->mt);
}


/*  
 */
GLboolean
st_finalize_mipmap_tree(GLcontext *ctx,
                        struct pipe_context *pipe, GLuint unit,
                        GLboolean *needFlush)
{
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct st_texture_object *stObj = st_texture_object(tObj);
   int comp_byte = 0;
   int cpp;

   GLuint face, i;
   GLuint nr_faces = 0;
   struct st_texture_image *firstImage;

   *needFlush = GL_FALSE;

   /* We know/require this is true by now: 
    */
   assert(stObj->base._Complete);

   /* What levels must the tree include at a minimum?
    */
   calculate_first_last_level(stObj);
   firstImage =
      st_texture_image(stObj->base.Image[0][stObj->firstLevel]);

   /* Fallback case:
    */
   if (firstImage->base.Border) {
      if (stObj->mt) {
         st_miptree_release(pipe, &stObj->mt);
      }
      return GL_FALSE;
   }


   /* If both firstImage and stObj have a tree which can contain
    * all active images, favour firstImage.  Note that because of the
    * completeness requirement, we know that the image dimensions
    * will match.
    */
   if (firstImage->mt &&
       firstImage->mt != stObj->mt &&
       firstImage->mt->first_level <= stObj->firstLevel &&
       firstImage->mt->last_level >= stObj->lastLevel) {

      if (stObj->mt)
         st_miptree_release(pipe, &stObj->mt);

      st_miptree_reference(&stObj->mt, firstImage->mt);
   }

   if (firstImage->base.IsCompressed) {
      comp_byte = compressed_num_bytes(firstImage->base.TexFormat->MesaFormat);
      cpp = comp_byte;
   }
   else cpp = firstImage->base.TexFormat->TexelBytes;

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    * 
    * XXX: For some layouts (eg i945?), the test might have to be
    * first_level == firstLevel, as the tree isn't valid except at the
    * original start level.  Hope to get around this by
    * programming minLod, maxLod, baseLevel into the hardware and
    * leaving the tree alone.
    */
   if (stObj->mt &&
       (stObj->mt->target != gl_target_to_pipe(stObj->base.Target) ||
	stObj->mt->internal_format != firstImage->base.InternalFormat ||
	stObj->mt->first_level != stObj->firstLevel ||
	stObj->mt->last_level != stObj->lastLevel ||
	stObj->mt->width0 != firstImage->base.Width ||
	stObj->mt->height0 != firstImage->base.Height ||
	stObj->mt->depth0 != firstImage->base.Depth ||
	stObj->mt->cpp != cpp ||
	stObj->mt->compressed != firstImage->base.IsCompressed)) {
      st_miptree_release(pipe, &stObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!stObj->mt) {
      stObj->mt = st_miptree_create(pipe,
                                    gl_target_to_pipe(stObj->base.Target),
                                    firstImage->base.InternalFormat,
                                    stObj->firstLevel,
                                    stObj->lastLevel,
                                    firstImage->base.Width,
                                    firstImage->base.Height,
                                    firstImage->base.Depth,
                                    cpp,
                                    comp_byte);

      stObj->mt->format
         = st_mesa_format_to_pipe_format(firstImage->base.TexFormat->MesaFormat);
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = (stObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   for (face = 0; face < nr_faces; face++) {
      for (i = stObj->firstLevel; i <= stObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(stObj->base.Image[face][i]);

         /* Need to import images in main memory or held in other trees.
          */
         if (stObj->mt != stImage->mt) {
            copy_image_data_to_tree(pipe, stObj, stImage);
	    *needFlush = GL_TRUE;
         }
      }
   }

   /**
   if (need_flush)
      intel_batchbuffer_flush(intel->batch);
   **/

   return GL_TRUE;
}


#if 0 /* unused? */
void
st_tex_map_images(struct pipe_context *pipe,
                  struct st_texture_object *stObj)
{
   GLuint nr_faces = (stObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   DBG("%s\n", __FUNCTION__);

   for (face = 0; face < nr_faces; face++) {
      for (i = stObj->firstLevel; i <= stObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(stObj->base.Image[face][i]);

         if (stImage->mt) {
            stImage->base.Data
               = st_miptree_image_map(pipe,
                                      stImage->mt,
                                      stImage->face,
                                      stImage->level,
                                      &stImage->base.RowStride,
                                      stImage->base.ImageOffsets);
            /* convert stride to texels, not bytes */
            stImage->base.RowStride /= stImage->mt->cpp;
/*             stImage->base.ImageStride /= stImage->mt->cpp; */
         }
      }
   }
}



void
st_tex_unmap_images(struct pipe_context *pipe,
                    struct st_texture_object *stObj)
{
   GLuint nr_faces = (stObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   for (face = 0; face < nr_faces; face++) {
      for (i = stObj->firstLevel; i <= stObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(stObj->base.Image[face][i]);

         if (stImage->mt) {
            st_miptree_image_unmap(pipe, stImage->mt);
            stImage->base.Data = NULL;
         }
      }
   }
}
#endif




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
   functions->CopyTexImage1D = st_CopyTexImage1D;
   functions->CopyTexImage2D = st_CopyTexImage2D;
   functions->CopyTexSubImage1D = st_CopyTexSubImage1D;
   functions->CopyTexSubImage2D = st_CopyTexSubImage2D;
   functions->CopyTexSubImage3D = st_CopyTexSubImage3D;

   functions->GetTexImage = st_GetTexImage;

   /* compressed texture functions */
   functions->CompressedTexImage2D = st_CompressedTexImage2D;
   functions->GetCompressedTexImage = st_GetCompressedTexImage;

   functions->NewTextureObject = st_NewTextureObject;
   functions->NewTextureImage = st_NewTextureImage;
   functions->DeleteTexture = st_DeleteTextureObject;
   functions->FreeTexImageData = st_FreeTextureImageData;
   functions->UpdateTexturePalette = 0;
   functions->IsTextureResident = st_IsTextureResident;

   functions->TextureMemCpy = do_memcpy;
}
