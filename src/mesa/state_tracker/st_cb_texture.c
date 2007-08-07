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
#include "state_tracker/st_cb_texture.h"
#include "state_tracker/st_mipmap_tree.h"

#include "pipe/p_context.h"


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


static int
intel_compressed_num_bytes(GLuint mesaFormat)
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


/* It works out that this function is fine for all the supported
 * hardware.  However, there is still a need to map the formats onto
 * hardware descriptors.
 */
/* Note that the i915 can actually support many more formats than
 * these if we take the step of simply swizzling the colors
 * immediately after sampling...
 */
static const struct gl_texture_format *
st_ChooseTextureFormat(GLcontext * ctx, GLint internalFormat,
                         GLenum format, GLenum type)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   const GLboolean do32bpt = (intel->intelScreen->front.cpp == 4);
#else
   const GLboolean do32bpt = 1;
#endif

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if (format == GL_BGRA) {
         if (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) {
            return &_mesa_texformat_argb8888;
         }
         else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
            return &_mesa_texformat_argb4444;
         }
         else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
            return &_mesa_texformat_argb1555;
         }
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) {
         return &_mesa_texformat_rgb565;
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_argb8888;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return &_mesa_texformat_rgba_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return &_mesa_texformat_z16;

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      return &_mesa_texformat_z24_s8;

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n",
              _mesa_lookup_enum_by_nr(internalFormat), __FUNCTION__);
      return NULL;
   }

   return NULL;                 /* never get here */
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
                            struct st_texture_object *intelObj,
                            struct st_texture_image *intelImage)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = intelImage->base.Width;
   GLuint height = intelImage->base.Height;
   GLuint depth = intelImage->base.Depth;
   GLuint l2width, l2height, l2depth;
   GLuint i, comp_byte = 0;

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Border)
      return;

   if (intelImage->level > intelObj->base.BaseLevel &&
       (intelImage->base.Width == 1 ||
        (intelObj->base.Target != GL_TEXTURE_1D &&
         intelImage->base.Height == 1) ||
        (intelObj->base.Target == GL_TEXTURE_3D &&
         intelImage->base.Depth == 1)))
      return;

   /* If this image disrespects BaseLevel, allocate from level zero.
    * Usually BaseLevel == 0, so it's unlikely to happen.
    */
   if (intelImage->level < intelObj->base.BaseLevel)
      firstLevel = 0;
   else
      firstLevel = intelObj->base.BaseLevel;


   /* Figure out image dimensions at start level. 
    */
   for (i = intelImage->level; i > firstLevel; i--) {
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
   if ((intelObj->base.MinFilter == GL_NEAREST ||
        intelObj->base.MinFilter == GL_LINEAR) &&
       intelImage->level == firstLevel) {
      lastLevel = firstLevel;
   }
   else {
      l2width = logbase2(width);
      l2height = logbase2(height);
      l2depth = logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width, l2height), l2depth);
   }

   assert(!intelObj->mt);
   if (intelImage->base.IsCompressed)
      comp_byte = intel_compressed_num_bytes(intelImage->base.TexFormat->MesaFormat);
   intelObj->mt = st_miptree_create(pipe,
                                       intelObj->base.Target,
                                       intelImage->base.InternalFormat,
                                       firstLevel,
                                       lastLevel,
                                       width,
                                       height,
                                       depth,
                                       intelImage->base.TexFormat->TexelBytes,
                                       comp_byte);

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
               struct st_texture_image *intelImage,
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

   dst_offset = st_miptree_image_offset(intelImage->mt,
                                           intelImage->face,
                                           intelImage->level);

   dst_stride = intelImage->mt->pitch;

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);
   {
      struct _DriBufferObject *src_buffer =
         intel_bufferobj_buffer(intel, pbo, INTEL_READ);

      /* Temporary hack: cast to _DriBufferObject:
       */
      struct _DriBufferObject *dst_buffer =
         (struct _DriBufferObject *)intelImage->mt->region->buffer;


      intelEmitCopyBlit(intel,
                        intelImage->mt->cpp,
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
              struct st_texture_image *intelImage,
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
   struct st_texture_object *intelObj = st_texture_object(texObj);
   struct st_texture_image *intelImage = st_texture_image(texImage);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride;


   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

#if 0
   intelFlush(ctx);
#endif

   intelImage->face = target_to_face(target);
   intelImage->level = level;

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
   if (intelImage->mt) {
      st_miptree_release(pipe, &intelImage->mt);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      _mesa_align_free(texImage->Data);
   }

   /* If this is the only texture image in the tree, could call
    * bmBufferData with NULL data to free the old block and avoid
    * waiting on any outstanding fences.
    */
   if (intelObj->mt &&
       intelObj->mt->first_level == level &&
       intelObj->mt->last_level == level &&
       intelObj->mt->target != GL_TEXTURE_CUBE_MAP_ARB &&
       !st_miptree_match_image(intelObj->mt, &intelImage->base,
                                  intelImage->face, intelImage->level)) {

      DBG("release it\n");
      st_miptree_release(pipe, &intelObj->mt);
      assert(!intelObj->mt);
   }

   if (!intelObj->mt) {
      guess_and_alloc_mipmap_tree(pipe, intelObj, intelImage);
      if (!intelObj->mt) {
	 DBG("guess_and_alloc_mipmap_tree: failed\n");
      }
   }

   assert(!intelImage->mt);

   if (intelObj->mt &&
       st_miptree_match_image(intelObj->mt, &intelImage->base,
                                 intelImage->face, intelImage->level)) {

      st_miptree_reference(&intelImage->mt, intelObj->mt);
      assert(intelImage->mt);
   }

   if (!intelImage->mt)
      DBG("XXX: Image did not fit into tree - storing in local memory!\n");

#if 0 /* XXX FIX when st_buffer_objects are in place */
   /* PBO fastpaths:
    */
   if (dims <= 2 &&
       intelImage->mt &&
       intel_buffer_object(unpack->BufferObj) &&
       check_pbo_format(internalFormat, format,
                        type, intelImage->base.TexFormat)) {

      DBG("trying pbo upload\n");

      /* Attempt to texture directly from PBO data (zero copy upload).
       *
       * Currently disable as it can lead to worse as well as better
       * performance (in particular when pipe_region_cow() is
       * required).
       */
      if (intelObj->mt == intelImage->mt &&
          intelObj->mt->first_level == level &&
          intelObj->mt->last_level == level) {

         if (try_pbo_zcopy(intel, intelImage, unpack,
                           internalFormat,
                           width, height, format, type, pixels)) {

            DBG("pbo zcopy upload succeeded\n");
            return;
         }
      }


      /* Otherwise, attempt to use the blitter for PBO image uploads.
       */
      if (try_pbo_upload(intel, intelImage, unpack,
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


   if (intelImage->mt)
      pipe->region_idle(pipe, intelImage->mt->region);

#if 0
   LOCK_HARDWARE(intel);
#endif

   if (intelImage->mt) {
      texImage->Data = st_miptree_image_map(pipe,
                                               intelImage->mt,
                                               intelImage->face,
                                               intelImage->level,
                                               &dstRowStride,
                                               intelImage->base.ImageOffsets);
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

   if (intelImage->mt) {
      st_miptree_image_unmap(pipe, intelImage->mt);
      texImage->Data = NULL;
   }

#if 0
   UNLOCK_HARDWARE(intel);
#endif

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

   if (stImage->mt)
      pipe->region_idle(pipe, stImage->mt->region);

#if 0
   LOCK_HARDWARE(intel);
#endif

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

#if 0
   UNLOCK_HARDWARE(intel);
#endif
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
   st_TexSubimage(ctx, 3,
                    target, level,
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
   st_TexSubimage(ctx, 2,
                    target, level,
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
   st_TexSubimage(ctx, 1,
                    target, level,
                    xoffset, 0, 0,
                    width, 1, 1,
                    format, type, pixels, packing, texObj, texImage);
}



/**
 * Get the pipe_region which is the source for any glCopyTex[Sub]Image call.
 *
 * Do the best we can using the blitter.  A future project is to use
 * the texture engine and fragment programs for these copies.
 */
static const struct pipe_region *
get_teximage_source(GLcontext *ctx, GLenum internalFormat)
{
#if 00
   struct intel_renderbuffer *irb;

   DBG("%s %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat));

   switch (internalFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16_ARB:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 2)
         return irb->region;
      return NULL;
   case GL_DEPTH24_STENCIL8_EXT:
   case GL_DEPTH_STENCIL_EXT:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 4)
         return irb->region;
      return NULL;
   case GL_RGBA:
   case GL_RGBA8:
      return intel_readbuf_region(intel);
   case GL_RGB:
      if (intel->intelScreen->front.cpp == 2)
         return intel_readbuf_region(intel);
      return NULL;
   default:
      return NULL;
   }
#else
   return NULL;
#endif
}


static GLboolean
do_copy_texsubimage(GLcontext *ctx,
                    struct st_texture_image *stImage,
                    GLenum internalFormat,
                    GLint dstx, GLint dsty,
                    GLint x, GLint y, GLsizei width, GLsizei height)
{
   const struct pipe_region *src =
      get_teximage_source(ctx, internalFormat);

   if (!stImage->mt || !src) {
      DBG("%s fail %p %p\n", __FUNCTION__, (void *) stImage->mt, (void *) src);
      return GL_FALSE;
   }

#if 00 /* XXX FIX flush/locking */
   intelFlush(ctx);
   /* XXX still need the lock ? */
   LOCK_HARDWARE(intel);
#endif

   {
      GLuint image_offset = st_miptree_image_offset(stImage->mt,
                                                       stImage->face,
                                                       stImage->level);
      const GLint orig_x = x;
      const GLint orig_y = y;
      const struct gl_framebuffer *fb = ctx->DrawBuffer;

      if (_mesa_clip_to_region(fb->_Xmin, fb->_Ymin, fb->_Xmax, fb->_Ymax,
                               &x, &y, &width, &height)) {
         /* Update dst for clipped src.  Need to also clip the source rect.
          */
         dstx += x - orig_x;
         dsty += y - orig_y;

         if (!(ctx->ReadBuffer->Name == 0)) {
	    /* XXX this looks bogus ? */
	    /* FBO: invert Y */
	    y = ctx->ReadBuffer->Height - y - 1;
         }

         /* A bit of fiddling to get the blitter to work with -ve
          * pitches.  But we get a nice inverted blit this way, so it's
          * worth it:
          */
#if 0
         intelEmitCopyBlit(intel,
                           stImage->mt->cpp,
                           -src->pitch,
                           src->buffer,
                           src->height * src->pitch * src->cpp,
                           stImage->mt->pitch,
                           stImage->mt->region->buffer,
                           image_offset,
                           x, y + height, dstx, dsty, width, height,
			   GL_COPY); /* ? */
         intel_batchbuffer_flush(intel->batch);
#else
         /* XXX use pipe->region_copy() ??? */
         (void) image_offset;
#endif
      }
   }

#if 0
   UNLOCK_HARDWARE(intel);
#endif

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

   return GL_TRUE;
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

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                          width, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   if (!do_copy_texsubimage(ctx,
                            st_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, 1))
      goto fail;

   return;

 fail:
#if 0
   _swrast_copy_teximage1d(ctx, target, level, internalFormat, x, y,
                           width, border);
#endif
   ;
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

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                          width, height, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);


   if (!do_copy_texsubimage(ctx,
                            st_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
#if 0
   _swrast_copy_teximage2d(ctx, target, level, internalFormat, x, y,
                           width, height, border);
#endif
   assert(0);
}


static void
st_CopyTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint x, GLint y, GLsizei width)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;

   /* XXX need to check <border> as in above function? */

   /* Need to check texture is compatible with source format. 
    */

   if (!do_copy_texsubimage(ctx,
                            st_texture_image(texImage),
                            internalFormat, xoffset, 0, x, y, width, 1)) {
#if 0
      _swrast_copy_texsubimage1d(ctx, target, level, xoffset, x, y, width);
#endif
      assert(0);
   }
}


static void
st_CopyTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;


   /* Need to check texture is compatible with source format. 
    */

   if (!do_copy_texsubimage(ctx,
                            st_texture_image(texImage),
                            internalFormat,
                            xoffset, yoffset, x, y, width, height)) {
#if 0
      _swrast_copy_texsubimage2d(ctx, target, level,
                                 xoffset, yoffset, x, y, width, height);
#endif
      assert(0);
   }
}




/**
 * Compute which mipmap levels that really need to be sent to the hardware.
 * This depends on the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
 */
static void
intel_calculate_first_last_level(struct st_texture_object *intelObj)
{
   struct gl_texture_object *tObj = &intelObj->base;
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
   intelObj->firstLevel = firstLevel;
   intelObj->lastLevel = lastLevel;
}


static void
copy_image_data_to_tree(struct pipe_context *pipe,
                        struct st_texture_object *intelObj,
                        struct st_texture_image *stImage)
{
   if (stImage->mt) {
      /* Copy potentially with the blitter:
       */
      st_miptree_image_copy(pipe,
                               intelObj->mt,
                               stImage->face,
                               stImage->level, stImage->mt);

      st_miptree_release(pipe, &stImage->mt);
   }
   else {
      assert(stImage->base.Data != NULL);

      /* More straightforward upload.  
       */
      st_miptree_image_data(pipe,
                               intelObj->mt,
                               stImage->face,
                               stImage->level,
                               stImage->base.Data,
                               stImage->base.RowStride,
                               stImage->base.RowStride *
                               stImage->base.Height);
      _mesa_align_free(stImage->base.Data);
      stImage->base.Data = NULL;
   }

   st_miptree_reference(&stImage->mt, intelObj->mt);
}


/*  
 */
GLuint
st_finalize_mipmap_tree(GLcontext *ctx,
                        struct pipe_context *pipe, GLuint unit,
                        GLboolean *needFlush)
{
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct st_texture_object *intelObj = st_texture_object(tObj);
   int comp_byte = 0;
   int cpp;

   GLuint face, i;
   GLuint nr_faces = 0;
   struct st_texture_image *firstImage;

   *needFlush = GL_FALSE;

   /* We know/require this is true by now: 
    */
   assert(intelObj->base._Complete);

   /* What levels must the tree include at a minimum?
    */
   intel_calculate_first_last_level(intelObj);
   firstImage =
      st_texture_image(intelObj->base.Image[0][intelObj->firstLevel]);

   /* Fallback case:
    */
   if (firstImage->base.Border) {
      if (intelObj->mt) {
         st_miptree_release(pipe, &intelObj->mt);
      }
      return GL_FALSE;
   }


   /* If both firstImage and intelObj have a tree which can contain
    * all active images, favour firstImage.  Note that because of the
    * completeness requirement, we know that the image dimensions
    * will match.
    */
   if (firstImage->mt &&
       firstImage->mt != intelObj->mt &&
       firstImage->mt->first_level <= intelObj->firstLevel &&
       firstImage->mt->last_level >= intelObj->lastLevel) {

      if (intelObj->mt)
         st_miptree_release(pipe, &intelObj->mt);

      st_miptree_reference(&intelObj->mt, firstImage->mt);
   }

   if (firstImage->base.IsCompressed) {
      comp_byte = intel_compressed_num_bytes(firstImage->base.TexFormat->MesaFormat);
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
   if (intelObj->mt &&
       (intelObj->mt->target != intelObj->base.Target ||
	intelObj->mt->internal_format != firstImage->base.InternalFormat ||
	intelObj->mt->first_level != intelObj->firstLevel ||
	intelObj->mt->last_level != intelObj->lastLevel ||
	intelObj->mt->width0 != firstImage->base.Width ||
	intelObj->mt->height0 != firstImage->base.Height ||
	intelObj->mt->depth0 != firstImage->base.Depth ||
	intelObj->mt->cpp != cpp ||
	intelObj->mt->compressed != firstImage->base.IsCompressed)) {
      st_miptree_release(pipe, &intelObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intelObj->mt = st_miptree_create(pipe,
                                          intelObj->base.Target,
                                          firstImage->base.InternalFormat,
                                          intelObj->firstLevel,
                                          intelObj->lastLevel,
                                          firstImage->base.Width,
                                          firstImage->base.Height,
                                          firstImage->base.Depth,
                                          cpp,
                                          comp_byte);
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(intelObj->base.Image[face][i]);

         /* Need to import images in main memory or held in other trees.
          */
         if (intelObj->mt != stImage->mt) {
            copy_image_data_to_tree(pipe, intelObj, stImage);
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
                  struct st_texture_object *intelObj)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   DBG("%s\n", __FUNCTION__);

   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(intelObj->base.Image[face][i]);

         if (stImage->mt) {
            stImage->base.Data =
               st_miptree_image_map(pipe,
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
                    struct st_texture_object *intelObj)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct st_texture_image *stImage =
            st_texture_image(intelObj->base.Image[face][i]);

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
