
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/bufferobj.h"
#include "main/convolve.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/texcompress.h"
#include "main/texstore.h"
#include "main/texgetimage.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/teximage.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

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
guess_and_alloc_mipmap_tree(struct intel_context *intel,
                            struct intel_texture_object *intelObj,
                            struct intel_texture_image *intelImage,
			    GLboolean expect_accelerated_upload)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = intelImage->base.Width;
   GLuint height = intelImage->base.Height;
   GLuint depth = intelImage->base.Depth;
   GLuint l2width, l2height, l2depth;
   GLuint i, comp_byte = 0;
   GLuint texelBytes;

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Border ||
       ((intelImage->base._BaseFormat == GL_DEPTH_COMPONENT) && 
        ((intelObj->base.WrapS == GL_CLAMP_TO_BORDER) ||
         (intelObj->base.WrapT == GL_CLAMP_TO_BORDER))))
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
       intelImage->level == firstLevel &&
       (intel->gen < 4 || firstLevel == 0)) {
      lastLevel = firstLevel;
   }
   else {
      l2width = logbase2(width);
      l2height = logbase2(height);
      l2depth = logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width, l2height), l2depth);
   }

   assert(!intelObj->mt);
   if (_mesa_is_format_compressed(intelImage->base.TexFormat))
      comp_byte = intel_compressed_num_bytes(intelImage->base.TexFormat);

   texelBytes = _mesa_get_format_bytes(intelImage->base.TexFormat);

   intelObj->mt = intel_miptree_create(intel,
                                       intelObj->base.Target,
                                       intelImage->base._BaseFormat,
                                       intelImage->base.InternalFormat,
                                       firstLevel,
                                       lastLevel,
                                       width,
                                       height,
                                       depth,
                                       texelBytes,
                                       comp_byte,
				       expect_accelerated_upload);

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
                 gl_format mesa_format)
{
   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_RGBA8:
      return (format == GL_BGRA &&
              (type == GL_UNSIGNED_BYTE ||
               type == GL_UNSIGNED_INT_8_8_8_8_REV) &&
              mesa_format == MESA_FORMAT_ARGB8888);
   case 3:
   case GL_RGB:
      return (format == GL_RGB &&
              type == GL_UNSIGNED_SHORT_5_6_5 &&
              mesa_format == MESA_FORMAT_RGB565);
   case 1:
   case GL_LUMINANCE:
      return (format == GL_LUMINANCE &&
	      type == GL_UNSIGNED_BYTE &&
	      mesa_format == MESA_FORMAT_L8);
   case GL_YCBCR_MESA:
      return (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE);
   default:
      return GL_FALSE;
   }
}


/* XXX: Do this for TexSubImage also:
 */
static GLboolean
try_pbo_upload(struct intel_context *intel,
               struct intel_texture_image *intelImage,
               const struct gl_pixelstore_attrib *unpack,
               GLint internalFormat,
               GLint width, GLint height,
               GLenum format, GLenum type, const void *pixels)
{
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_x, dst_y, dst_stride;
   dri_bo *dst_buffer = intel_region_buffer(intel,
					    intelImage->mt->region,
					    INTEL_WRITE_FULL);

   if (!_mesa_is_bufferobj(unpack->BufferObj) ||
       intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: failure 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset = (GLuint) (unsigned long) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   intel_miptree_get_image_offset(intelImage->mt, intelImage->level,
				  intelImage->face, 0,
				  &dst_x, &dst_y);

   dst_stride = intelImage->mt->pitch;

   if (drm_intel_bo_references(intel->batch->buf, dst_buffer))
      intelFlush(&intel->ctx);
   intel_prepare_render(intel);
   {
      dri_bo *src_buffer = intel_bufferobj_buffer(intel, pbo, INTEL_READ);

      if (!intelEmitCopyBlit(intel,
			     intelImage->mt->cpp,
			     src_stride, src_buffer, src_offset, GL_FALSE,
			     dst_stride, dst_buffer, 0,
			     intelImage->mt->region->tiling,
			     0, 0, dst_x, dst_y, width, height,
			     GL_COPY)) {
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}


static GLboolean
try_pbo_zcopy(struct intel_context *intel,
              struct intel_texture_image *intelImage,
              const struct gl_pixelstore_attrib *unpack,
              GLint internalFormat,
              GLint width, GLint height,
              GLenum format, GLenum type, const void *pixels)
{
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_x, dst_y, dst_stride;

   if (!_mesa_is_bufferobj(unpack->BufferObj) ||
       intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: failure 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset = (GLuint) (unsigned long) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   intel_miptree_get_image_offset(intelImage->mt, intelImage->level,
				  intelImage->face, 0,
				  &dst_x, &dst_y);

   dst_stride = intelImage->mt->pitch;

   if (src_stride != dst_stride || dst_x != 0 || dst_y != 0 ||
       src_offset != 0) {
      DBG("%s: failure 2\n", __FUNCTION__);
      return GL_FALSE;
   }

   intel_region_attach_pbo(intel, intelImage->mt->region, pbo);

   return GL_TRUE;
}


static void
intelTexImage(GLcontext * ctx,
              GLint dims,
              GLenum target, GLint level,
              GLint internalFormat,
              GLint width, GLint height, GLint depth,
              GLint border,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack,
              struct gl_texture_object *texObj,
              struct gl_texture_image *texImage, GLsizei imageSize,
              GLboolean compressed)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride = 0, srcRowStride = texImage->RowStride;

   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

   intelImage->face = target_to_face(target);
   intelImage->level = level;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      texelBytes = 0;
   }
   else {
      texelBytes = _mesa_get_format_bytes(texImage->TexFormat);
      
      /* Minimum pitch of 32 bytes */
      if (postConvWidth * texelBytes < 32) {
	 postConvWidth = 32 / texelBytes;
	 texImage->RowStride = postConvWidth;
      }

      if (!intelImage->mt) {      
	  assert(texImage->RowStride == postConvWidth);
      }
   }

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      _mesa_free_texmemory(texImage->Data);
      texImage->Data = NULL;
   }

   /* If this is the only texture image in the tree, could call
    * bmBufferData with NULL data to free the old block and avoid
    * waiting on any outstanding fences.
    */
   if (intelObj->mt &&
       intelObj->mt->first_level == level &&
       intelObj->mt->last_level == level &&
       intelObj->mt->target != GL_TEXTURE_CUBE_MAP_ARB &&
       !intel_miptree_match_image(intelObj->mt, &intelImage->base)) {

      DBG("release it\n");
      intel_miptree_release(intel, &intelObj->mt);
      assert(!intelObj->mt);
   }

   if (!intelObj->mt) {
      guess_and_alloc_mipmap_tree(intel, intelObj, intelImage, pixels == NULL);
      if (!intelObj->mt) {
	 DBG("guess_and_alloc_mipmap_tree: failed\n");
      }
   }

   assert(!intelImage->mt);

   if (intelObj->mt &&
       intel_miptree_match_image(intelObj->mt, &intelImage->base)) {

      intel_miptree_reference(&intelImage->mt, intelObj->mt);
      assert(intelImage->mt);
   } else if (intelImage->base.Border == 0) {
      int comp_byte = 0;
      GLuint texelBytes = _mesa_get_format_bytes(intelImage->base.TexFormat);
      GLenum baseFormat = _mesa_get_format_base_format(intelImage->base.TexFormat);
      if (_mesa_is_format_compressed(intelImage->base.TexFormat)) {
	 comp_byte =
	    intel_compressed_num_bytes(intelImage->base.TexFormat);
      }

      /* Didn't fit in the object miptree, but it's suitable for inclusion in
       * a miptree, so create one just for our level and store it in the image.
       * It'll get moved into the object miptree at validate time.
       */
      intelImage->mt = intel_miptree_create(intel, target,
					    baseFormat,
					    internalFormat,
					    level, level,
					    width, height, depth,
					    texelBytes,
					    comp_byte, pixels == NULL);

   }

   /* PBO fastpaths:
    */
   if (dims <= 2 &&
       intelImage->mt &&
       _mesa_is_bufferobj(unpack->BufferObj) &&
       check_pbo_format(internalFormat, format,
                        type, intelImage->base.TexFormat)) {

      DBG("trying pbo upload\n");

      /* Attempt to texture directly from PBO data (zero copy upload).
       *
       * Currently disable as it can lead to worse as well as better
       * performance (in particular when intel_region_cow() is
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

   intel_prepare_render(intel);

   if (intelImage->mt) {
      if (pixels != NULL) {
	 /* Flush any queued rendering with the texture before mapping. */
	 if (drm_intel_bo_references(intel->batch->buf,
				     intelImage->mt->region->buffer)) {
	    intelFlush(ctx);
	 }
         texImage->Data = intel_miptree_image_map(intel,
                                                  intelImage->mt,
                                                  intelImage->face,
                                                  intelImage->level,
                                                  &dstRowStride,
                                                  intelImage->base.ImageOffsets);
      }

      texImage->RowStride = dstRowStride / intelImage->mt->cpp;
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      if (_mesa_is_format_compressed(texImage->TexFormat)) {
         sizeInBytes = _mesa_format_image_size(texImage->TexFormat,
                                               texImage->Width,
                                               texImage->Height,
                                               texImage->Depth);
         dstRowStride =
            _mesa_format_row_stride(texImage->TexFormat, width);
         assert(dims != 3);
      }
      else {
         dstRowStride = postConvWidth * texelBytes;
         sizeInBytes = depth * dstRowStride * postConvHeight;
      }

      texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   }

   DBG("Upload image %dx%dx%d row_len %d "
       "pitch %d pixels %d compressed %d\n",
       width, height, depth, width * texelBytes, dstRowStride,
       pixels ? 1 : 0, compressed);

   /* Copy data.  Would like to know when it's ok for us to eg. use
    * the blitter to copy.  Or, use the hardware to do the format
    * conversion and copy:
    */
   if (pixels) {
       if (compressed) {
	   if (intelImage->mt) {
	       struct intel_region *dst = intelImage->mt->region;
	       _mesa_copy_rect(texImage->Data, dst->cpp, dst->pitch,
			       0, 0,
			       intelImage->mt->level[level].width,
			       (intelImage->mt->level[level].height+3)/4,
			       pixels,
			       srcRowStride,
			       0, 0);
	   }
           else {
	       memcpy(texImage->Data, pixels, imageSize);
           }
       }
       else if (!_mesa_texstore(ctx, dims, 
                                texImage->_BaseFormat, 
                                texImage->TexFormat, 
                                texImage->Data, 0, 0, 0, /* dstX/Y/Zoffset */
                                dstRowStride,
                                texImage->ImageOffsets,
                                width, height, depth,
                                format, type, pixels, unpack)) {
          _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
       }
   }

   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (intelImage->mt) {
      if (pixels != NULL)
         intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }
}


static void
intelTexImage3D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint height, GLint depth,
                GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 3, target, level,
                 internalFormat, width, height, depth, border,
                 format, type, pixels, unpack, texObj, texImage, 0, GL_FALSE);
}


static void
intelTexImage2D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint height, GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 2, target, level,
                 internalFormat, width, height, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, GL_FALSE);
}


static void
intelTexImage1D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 1, target, level,
                 internalFormat, width, 1, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, GL_FALSE);
}


static void
intelCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                           GLint internalFormat,
                           GLint width, GLint height, GLint border,
                           GLsizei imageSize, const GLvoid *data,
                           struct gl_texture_object *texObj,
                           struct gl_texture_image *texImage )
{
   intelTexImage(ctx, 2, target, level,
		 internalFormat, width, height, 1, border,
		 0, 0, data, &ctx->Unpack, texObj, texImage, imageSize, GL_TRUE);
}


/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
intel_get_tex_image(GLcontext * ctx, GLenum target, GLint level,
		    GLenum format, GLenum type, GLvoid * pixels,
		    struct gl_texture_object *texObj,
		    struct gl_texture_image *texImage, GLboolean compressed)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   /* If we're reading from a texture that has been rendered to, need to
    * make sure rendering is complete.
    * We could probably predicate this on texObj->_RenderToTexture
    */
   intelFlush(ctx);

   /* Map */
   if (intelImage->mt) {
      /* Image is stored in hardware format in a buffer managed by the
       * kernel.  Need to explicitly map and unmap it.
       */
      intelImage->base.Data =
         intel_miptree_image_map(intel,
                                 intelImage->mt,
                                 intelImage->face,
                                 intelImage->level,
                                 &intelImage->base.RowStride,
                                 intelImage->base.ImageOffsets);
      intelImage->base.RowStride /= intelImage->mt->cpp;
   }
   else {
      /* Otherwise, the image should actually be stored in
       * intelImage->base.Data.  This is pretty confusing for
       * everybody, I'd much prefer to separate the two functions of
       * texImage->Data - storage for texture images in main memory
       * and access (ie mappings) of images.  In other words, we'd
       * create a new texImage->Map field and leave Data simply for
       * storage.
       */
      assert(intelImage->base.Data);
   }


   if (compressed) {
      _mesa_get_compressed_teximage(ctx, target, level, pixels,
				    texObj, texImage);
   }
   else {
      _mesa_get_teximage(ctx, target, level, format, type, pixels,
                         texObj, texImage);
   }
     

   /* Unmap */
   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      intelImage->base.Data = NULL;
   }
}


static void
intelGetTexImage(GLcontext * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   intel_get_tex_image(ctx, target, level, format, type, pixels,
		       texObj, texImage, GL_FALSE);
}


static void
intelGetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
			   GLvoid *pixels,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
   intel_get_tex_image(ctx, target, level, 0, 0, pixels,
		       texObj, texImage, GL_TRUE);
}

void
intelSetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
		   GLint texture_format,
		   __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = dPriv->driverPrivate;
   struct intel_context *intel = pDRICtx->driverPrivate;
   GLcontext *ctx = &intel->ctx;
   struct intel_texture_object *intelObj;
   struct intel_texture_image *intelImage;
   struct intel_mipmap_tree *mt;
   struct intel_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   int level = 0, internalFormat;

   texObj = _mesa_get_current_tex_object(ctx, target);
   intelObj = intel_texture_object(texObj);

   if (!intelObj)
      return;

   if (dPriv->lastStamp != dPriv->dri2.stamp)
      intel_update_renderbuffers(pDRICtx, dPriv);

   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   /* If the region isn't set, then intel_update_renderbuffers was unable
    * to get the buffers for the drawable.
    */
   if (rb->region == NULL)
      return;

   if (texture_format == __DRI_TEXTURE_FORMAT_RGB)
      internalFormat = GL_RGB;
   else
      internalFormat = GL_RGBA;

   mt = intel_miptree_create_for_region(intel, target,
					internalFormat,
					0, 0, rb->region, 1, 0);
   if (mt == NULL)
       return;

   _mesa_lock_texture(&intel->ctx, texObj);

   texImage = _mesa_get_tex_image(&intel->ctx, texObj, target, level);
   intelImage = intel_texture_image(texImage);

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
      assert(!texImage->Data);
   }
   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   intelObj->mt = mt;
   _mesa_init_teximage_fields(&intel->ctx, target, texImage,
			      rb->region->width, rb->region->height, 1,
			      0, internalFormat);

   intelImage->face = target_to_face(target);
   intelImage->level = level;
   if (texture_format == __DRI_TEXTURE_FORMAT_RGB)
      texImage->TexFormat = MESA_FORMAT_XRGB8888;
   else
      texImage->TexFormat = MESA_FORMAT_ARGB8888;
   texImage->RowStride = rb->region->pitch;
   intel_miptree_reference(&intelImage->mt, intelObj->mt);

   if (!intel_miptree_match_image(intelObj->mt, &intelImage->base)) {
	   fprintf(stderr, "miptree doesn't match image\n");
   }

   _mesa_unlock_texture(&intel->ctx, texObj);
}

void
intelSetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
   /* The old interface didn't have the format argument, so copy our
    * implementation's behavior at the time.
    */
   intelSetTexBuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

#if FEATURE_OES_EGL_image
static void
intel_image_target_texture_2d(GLcontext *ctx, GLenum target,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage,
			      GLeglImageOES image_handle)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   struct intel_mipmap_tree *mt;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(intel->driContext, image_handle,
					      intel->driContext->loaderPrivate);
   if (image == NULL)
      return;

   mt = intel_miptree_create_for_region(intel, target,
					image->internal_format,
					0, 0, image->region, 1, 0);
   if (mt == NULL)
       return;

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
      assert(!texImage->Data);
   }
   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   intelObj->mt = mt;
   _mesa_init_teximage_fields(&intel->ctx, target, texImage,
			      image->region->width, image->region->height, 1,
			      0, image->internal_format);

   intelImage->face = target_to_face(target);
   intelImage->level = 0;
   texImage->TexFormat = image->format;
   texImage->RowStride = image->region->pitch;
   intel_miptree_reference(&intelImage->mt, intelObj->mt);

   if (!intel_miptree_match_image(intelObj->mt, &intelImage->base))
      fprintf(stderr, "miptree doesn't match image\n");
}
#endif

void
intelInitTextureImageFuncs(struct dd_function_table *functions)
{
   functions->TexImage1D = intelTexImage1D;
   functions->TexImage2D = intelTexImage2D;
   functions->TexImage3D = intelTexImage3D;
   functions->GetTexImage = intelGetTexImage;

   functions->CompressedTexImage2D = intelCompressedTexImage2D;
   functions->GetCompressedTexImage = intelGetCompressedTexImage;

#if FEATURE_OES_EGL_image
   functions->EGLImageTargetTexture2D = intel_image_target_texture_2d;
#endif
}
