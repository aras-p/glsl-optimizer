
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/pbo.h"
#include "main/renderbuffer.h"
#include "main/texcompress.h"
#include "main/texstore.h"
#include "main/texgetimage.h"
#include "main/texobj.h"
#include "main/teximage.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_span.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Functions to store texture images.  Where possible, mipmap_tree's
 * will be created or further instantiated with image data, otherwise
 * images will be stored in malloc'd memory.  A validation step is
 * required to pull those images into a mipmap tree, or otherwise
 * decide a fallback is required.
 */



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
static struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct intel_context *intel,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  GLboolean expect_accelerated_upload)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = intelImage->base.Base.Width;
   GLuint height = intelImage->base.Base.Height;
   GLuint depth = intelImage->base.Base.Depth;
   GLuint i;

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Base.Border)
      return NULL;

   if (intelImage->base.Base.Level > intelObj->base.BaseLevel &&
       (intelImage->base.Base.Width == 1 ||
        (intelObj->base.Target != GL_TEXTURE_1D &&
         intelImage->base.Base.Height == 1) ||
        (intelObj->base.Target == GL_TEXTURE_3D &&
         intelImage->base.Base.Depth == 1))) {
      /* For this combination, we're at some lower mipmap level and
       * some important dimension is 1.  We can't extrapolate up to a
       * likely base level width/height/depth for a full mipmap stack
       * from this info, so just allocate this one level.
       */
      firstLevel = intelImage->base.Base.Level;
      lastLevel = intelImage->base.Base.Level;
   } else {
      /* If this image disrespects BaseLevel, allocate from level zero.
       * Usually BaseLevel == 0, so it's unlikely to happen.
       */
      if (intelImage->base.Base.Level < intelObj->base.BaseLevel)
	 firstLevel = 0;
      else
	 firstLevel = intelObj->base.BaseLevel;

      /* Figure out image dimensions at start level. */
      for (i = intelImage->base.Base.Level; i > firstLevel; i--) {
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
      if ((intelObj->base.Sampler.MinFilter == GL_NEAREST ||
	   intelObj->base.Sampler.MinFilter == GL_LINEAR) &&
	  intelImage->base.Base.Level == firstLevel &&
	  (intel->gen < 4 || firstLevel == 0)) {
	 lastLevel = firstLevel;
      } else {
	 lastLevel = firstLevel + _mesa_logbase2(MAX2(MAX2(width, height), depth));
      }
   }

   return intel_miptree_create(intel,
			       intelObj->base.Target,
			       intelImage->base.Base.TexFormat,
			       firstLevel,
			       lastLevel,
			       width,
			       height,
			       depth,
			       expect_accelerated_upload);
}

/* There are actually quite a few combinations this will work for,
 * more than what I've listed here.
 */
static bool
check_pbo_format(GLenum format, GLenum type,
                 gl_format mesa_format)
{
   switch (mesa_format) {
   case MESA_FORMAT_ARGB8888:
      return (format == GL_BGRA && (type == GL_UNSIGNED_BYTE ||
				    type == GL_UNSIGNED_INT_8_8_8_8_REV));
   case MESA_FORMAT_RGB565:
      return (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5);
   case MESA_FORMAT_L8:
      return (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE);
   case MESA_FORMAT_YCBCR:
      return (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE);
   default:
      return false;
   }
}


/* XXX: Do this for TexSubImage also:
 */
static bool
try_pbo_upload(struct intel_context *intel,
               struct intel_texture_image *intelImage,
               const struct gl_pixelstore_attrib *unpack,
	       GLenum format, GLenum type,
               GLint width, GLint height, const void *pixels)
{
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_x, dst_y, dst_stride;
   drm_intel_bo *dst_buffer, *src_buffer;

   if (!_mesa_is_bufferobj(unpack->BufferObj))
      return false;

   DBG("trying pbo upload\n");

   if (!intelImage->mt) {
      DBG("%s: no miptree\n", __FUNCTION__);
      return false;
   }

   if (intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: image transfer\n", __FUNCTION__);
      return false;
   }

   if (!check_pbo_format(format, type, intelImage->base.Base.TexFormat)) {
      DBG("%s: format mismatch (upload to %s with format 0x%x, type 0x%x)\n",
	  __FUNCTION__, _mesa_get_format_name(intelImage->base.Base.TexFormat),
	  format, type);
      return false;
   }

   dst_buffer = intel_region_buffer(intel, intelImage->mt->region, INTEL_WRITE_FULL);
   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset = (GLuint) (unsigned long) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   intel_miptree_get_image_offset(intelImage->mt, intelImage->base.Base.Level,
				  intelImage->base.Base.Face, 0,
				  &dst_x, &dst_y);

   dst_stride = intelImage->mt->region->pitch;

   {
      GLuint offset;
      drm_intel_bo *src_buffer =
	      intel_bufferobj_source(intel, pbo, 64, &offset);

      if (!intelEmitCopyBlit(intel,
			     intelImage->mt->cpp,
			     src_stride, src_buffer,
			     src_offset + offset, GL_FALSE,
			     dst_stride, dst_buffer, 0,
			     intelImage->mt->region->tiling,
			     0, 0, dst_x, dst_y, width, height,
			     GL_COPY)) {
	 DBG("%s: blit failed\n", __FUNCTION__);
	 return false;
      }
   }

   DBG("%s: success\n", __FUNCTION__);
   return true;
}

/**
 * \param scatter Scatter if true. Gather if false.
 *
 * \see intel_tex_image_x8z24_scatter
 * \see intel_tex_image_x8z24_gather
 */
static void
intel_tex_image_s8z24_scattergather(struct intel_context *intel,
				    struct intel_texture_image *intel_image,
				    bool scatter)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_renderbuffer *depth_rb = intel_image->depth_rb;
   struct gl_renderbuffer *stencil_rb = intel_image->stencil_rb;

   int w = intel_image->base.Base.Width;
   int h = intel_image->base.Base.Height;

   uint32_t depth_row[w];
   uint8_t stencil_row[w];

   intel_renderbuffer_map(intel, depth_rb);
   intel_renderbuffer_map(intel, stencil_rb);

   if (scatter) {
      for (int y = 0; y < h; ++y) {
	 depth_rb->GetRow(ctx, depth_rb, w, 0, y, depth_row);
	 for (int x = 0; x < w; ++x) {
	    stencil_row[x] = depth_row[x] >> 24;
	 }
	 stencil_rb->PutRow(ctx, stencil_rb, w, 0, y, stencil_row, NULL);
      }
   } else { /* gather */
      for (int y = 0; y < h; ++y) {
	 depth_rb->GetRow(ctx, depth_rb, w, 0, y, depth_row);
	 stencil_rb->GetRow(ctx, stencil_rb, w, 0, y, stencil_row);
	 for (int x = 0; x < w; ++x) {
	    uint32_t s8_x24 = stencil_row[x] << 24;
	    uint32_t x8_z24 = depth_row[x] & 0x00ffffff;
	    depth_row[x] = s8_x24 | x8_z24;
	 }
	 depth_rb->PutRow(ctx, depth_rb, w, 0, y, depth_row, NULL);
      }
   }

   intel_renderbuffer_unmap(intel, depth_rb);
   intel_renderbuffer_unmap(intel, stencil_rb);
}

/**
 * Copy the x8 bits from intel_image->depth_rb to intel_image->stencil_rb.
 */
void
intel_tex_image_s8z24_scatter(struct intel_context *intel,
			      struct intel_texture_image *intel_image)
{
   intel_tex_image_s8z24_scattergather(intel, intel_image, true);
}

/**
 * Copy the data in intel_image->stencil_rb to the x8 bits in
 * intel_image->depth_rb.
 */
void
intel_tex_image_s8z24_gather(struct intel_context *intel,
			     struct intel_texture_image *intel_image)
{
   intel_tex_image_s8z24_scattergather(intel, intel_image, false);
}

static bool
intel_tex_image_s8z24_create_renderbuffers(struct intel_context *intel,
					   struct intel_texture_image *image)
{
   struct gl_context *ctx = &intel->ctx;

   bool ok = true;
   int width = image->base.Base.Width;
   int height = image->base.Base.Height;
   struct gl_renderbuffer *drb;
   struct gl_renderbuffer *srb;
   struct intel_renderbuffer *idrb;
   struct intel_renderbuffer *isrb;

   assert(intel->has_separate_stencil);
   assert(image->base.Base.TexFormat == MESA_FORMAT_S8_Z24);
   assert(image->mt != NULL);

   drb = intel_create_wrapped_renderbuffer(ctx, width, height,
					   MESA_FORMAT_X8_Z24);
   srb = intel_create_wrapped_renderbuffer(ctx, width, height,
					   MESA_FORMAT_S8);

   if (!drb || !srb) {
      if (drb) {
	 drb->Delete(drb);
      }
      if (srb) {
	 srb->Delete(srb);
      }
      return false;
   }

   idrb = intel_renderbuffer(drb);
   isrb = intel_renderbuffer(srb);

   intel_region_reference(&idrb->region, image->mt->region);
   ok = intel_alloc_renderbuffer_storage(ctx, srb, GL_STENCIL_INDEX8,
					 width, height);

   if (!ok) {
      drb->Delete(drb);
      srb->Delete(srb);
      return false;
   }

   intel_renderbuffer_set_draw_offset(idrb, image, 0);
   intel_renderbuffer_set_draw_offset(isrb, image, 0);

   _mesa_reference_renderbuffer(&image->depth_rb, drb);
   _mesa_reference_renderbuffer(&image->stencil_rb, srb);

   return true;
}

static void
intelTexImage(struct gl_context * ctx,
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
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride = 0, srcRowStride = texImage->RowStride;

   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      texelBytes = 0;
   }
   else {
      texelBytes = _mesa_get_format_bytes(texImage->TexFormat);

      if (!intelImage->mt) {      
	  assert(texImage->RowStride == width);
      }
   }

   assert(!intelImage->mt);

   if (intelObj->mt &&
       intel_miptree_match_image(intelObj->mt, &intelImage->base.Base)) {
      /* Use an existing miptree when possible */
      intel_miptree_reference(&intelImage->mt, intelObj->mt);
      assert(intelImage->mt);
   } else if (intelImage->base.Base.Border == 0) {
      /* Didn't fit in the object miptree, but it's suitable for inclusion in
       * a miptree, so create one just for our level and store it in the image.
       * It'll get moved into the object miptree at validate time.
       */
      intelImage->mt = intel_miptree_create_for_teximage(intel, intelObj,
							 intelImage,
							 pixels == NULL);

      /* Even if the object currently has a mipmap tree associated
       * with it, this one is a more likely candidate to represent the
       * whole object since our level didn't fit what was there
       * before, and any lower levels would fit into our miptree.
       */
      if (intelImage->mt) {
	 intel_miptree_release(intel, &intelObj->mt);
	 intel_miptree_reference(&intelObj->mt, intelImage->mt);
      }
   }

   /* Attempt to use the blitter for PBO image uploads.
    */
   if (dims <= 2 &&
       try_pbo_upload(intel, intelImage, unpack, format, type,
		      width, height, pixels)) {
      return;
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

   if (intelImage->mt) {
      if (pixels != NULL) {
	 /* Flush any queued rendering with the texture before mapping. */
	 if (drm_intel_bo_references(intel->batch.bo,
				     intelImage->mt->region->buffer)) {
	    intel_flush(ctx);
	 }
         texImage->Data = intel_miptree_image_map(intel,
                                                  intelImage->mt,
                                                  intelImage->base.Base.Face,
                                                  intelImage->base.Base.Level,
                                                  &dstRowStride,
                                                  intelImage->base.Base.ImageOffsets);
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
         dstRowStride = width * texelBytes;
         sizeInBytes = depth * dstRowStride * height;
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

   if (intel->must_use_separate_stencil
       && texImage->TexFormat == MESA_FORMAT_S8_Z24) {
      intel_tex_image_s8z24_create_renderbuffers(intel, intelImage);
      intel_tex_image_s8z24_scatter(intel, intelImage);
   }

   if (intelImage->mt) {
      if (pixels != NULL)
         intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }
}


static void
intelTexImage3D(struct gl_context * ctx,
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
intelTexImage2D(struct gl_context * ctx,
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
intelTexImage1D(struct gl_context * ctx,
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
intelCompressedTexImage2D( struct gl_context *ctx, GLenum target, GLint level,
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
intel_get_tex_image(struct gl_context * ctx, GLenum target, GLint level,
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
   intel_flush(ctx);

   /* Map */
   if (intelImage->mt) {
      /* Image is stored in hardware format in a buffer managed by the
       * kernel.  Need to explicitly map and unmap it.
       */
      intelImage->base.Base.Data =
         intel_miptree_image_map(intel,
                                 intelImage->mt,
                                 intelImage->base.Base.Face,
                                 intelImage->base.Base.Level,
                                 &intelImage->base.Base.RowStride,
                                 intelImage->base.Base.ImageOffsets);
      intelImage->base.Base.RowStride /= intelImage->mt->cpp;
   }
   else {
      /* Otherwise, the image should actually be stored in
       * intelImage->base.Base.Data.  This is pretty confusing for
       * everybody, I'd much prefer to separate the two functions of
       * texImage->Data - storage for texture images in main memory
       * and access (ie mappings) of images.  In other words, we'd
       * create a new texImage->Map field and leave Data simply for
       * storage.
       */
      assert(intelImage->base.Base.Data);
   }

   if (intelImage->stencil_rb) {
      /*
       * The texture has packed depth/stencil format, but uses separate
       * stencil. The texture's embedded stencil buffer contains the real
       * stencil data, so copy that into the miptree.
       */
      intel_tex_image_s8z24_gather(intel, intelImage);
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
      intelImage->base.Base.Data = NULL;
   }
}


static void
intelGetTexImage(struct gl_context * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   intel_get_tex_image(ctx, target, level, format, type, pixels,
		       texObj, texImage, GL_FALSE);
}


static void
intelGetCompressedTexImage(struct gl_context *ctx, GLenum target, GLint level,
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
   struct gl_context *ctx = &intel->ctx;
   struct intel_texture_object *intelObj;
   struct intel_texture_image *intelImage;
   struct intel_mipmap_tree *mt;
   struct intel_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   int level = 0, internalFormat;
   gl_format texFormat;

   texObj = _mesa_get_current_tex_object(ctx, target);
   intelObj = intel_texture_object(texObj);

   if (!intelObj)
      return;

   if (dPriv->lastStamp != dPriv->dri2.stamp ||
       !pDRICtx->driScreenPriv->dri2.useInvalidate)
      intel_update_renderbuffers(pDRICtx, dPriv);

   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   /* If the region isn't set, then intel_update_renderbuffers was unable
    * to get the buffers for the drawable.
    */
   if (rb->region == NULL)
      return;

   if (texture_format == __DRI_TEXTURE_FORMAT_RGB) {
      internalFormat = GL_RGB;
      texFormat = MESA_FORMAT_XRGB8888;
   }
   else {
      internalFormat = GL_RGBA;
      texFormat = MESA_FORMAT_ARGB8888;
   }

   mt = intel_miptree_create_for_region(intel, target, texFormat, rb->region);
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
			      0, internalFormat, texFormat);

   texImage->RowStride = rb->region->pitch;
   intel_miptree_reference(&intelImage->mt, intelObj->mt);

   if (!intel_miptree_match_image(intelObj->mt, &intelImage->base.Base)) {
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
intel_image_target_texture_2d(struct gl_context *ctx, GLenum target,
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
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   mt = intel_miptree_create_for_region(intel, target, image->format,
					image->region);
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
			      0, image->internal_format, image->format);

   texImage->RowStride = image->region->pitch;
   intel_miptree_reference(&intelImage->mt, intelObj->mt);

   if (!intel_miptree_match_image(intelObj->mt, &intelImage->base.Base))
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
