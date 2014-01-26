
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/pbo.h"
#include "main/renderbuffer.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstore.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Work back from the specified level of the image to the baselevel and create a
 * miptree of that size.
 */
struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct intel_context *intel,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  bool expect_accelerated_upload)
{
   GLuint firstLevel;
   GLuint lastLevel;
   int width, height, depth;
   GLuint i;

   intel_miptree_get_dimensions_for_image(&intelImage->base.Base,
                                          &width, &height, &depth);

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Base.Level > intelObj->base.BaseLevel &&
       (width == 1 ||
        (intelObj->base.Target != GL_TEXTURE_1D && height == 1) ||
        (intelObj->base.Target == GL_TEXTURE_3D && depth == 1))) {
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
	  intelImage->base.Base.Level == firstLevel) {
	 lastLevel = firstLevel;
      } else {
	 lastLevel = (firstLevel +
                      _mesa_get_tex_max_num_levels(intelObj->base.Target,
                                                   width, height, depth) - 1);
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
			       expect_accelerated_upload,
                               INTEL_MIPTREE_TILING_ANY);
}

/* XXX: Do this for TexSubImage also:
 */
static bool
try_pbo_upload(struct gl_context *ctx,
               struct gl_texture_image *image,
               const struct gl_pixelstore_attrib *unpack,
	       GLenum format, GLenum type, const void *pixels)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset;
   drm_intel_bo *src_buffer;

   if (!_mesa_is_bufferobj(unpack->BufferObj))
      return false;

   DBG("trying pbo upload\n");

   if (intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: image transfer\n", __FUNCTION__);
      return false;
   }

   ctx->Driver.AllocTextureImageBuffer(ctx, image);

   if (!intelImage->mt) {
      DBG("%s: no miptree\n", __FUNCTION__);
      return false;
   }

   if (!_mesa_format_matches_format_and_type(intelImage->mt->format,
                                             format, type, false)) {
      DBG("%s: format mismatch (upload to %s with format 0x%x, type 0x%x)\n",
	  __FUNCTION__, _mesa_get_format_name(intelImage->mt->format),
	  format, type);
      return false;
   }

   if (image->TexObject->Target == GL_TEXTURE_1D_ARRAY ||
       image->TexObject->Target == GL_TEXTURE_2D_ARRAY) {
      DBG("%s: no support for array textures\n", __FUNCTION__);
      return false;
   }

   src_buffer = intel_bufferobj_source(intel, pbo, 64, &src_offset);
   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset += (GLuint) (unsigned long) pixels;

   int src_stride =
      _mesa_image_row_stride(unpack, image->Width, format, type);

   struct intel_mipmap_tree *pbo_mt =
      intel_miptree_create_for_bo(intel,
                                  src_buffer,
                                  intelImage->mt->format,
                                  src_offset,
                                  image->Width, image->Height,
                                  src_stride, I915_TILING_NONE);
   if (!pbo_mt)
      return false;

   if (!intel_miptree_blit(intel,
                           pbo_mt, 0, 0,
                           0, 0, false,
                           intelImage->mt, image->Level, image->Face,
                           0, 0, false,
                           image->Width, image->Height, GL_COPY)) {
      DBG("%s: blit failed\n", __FUNCTION__);
      intel_miptree_release(&pbo_mt);
      return false;
   }

   intel_miptree_release(&pbo_mt);

   DBG("%s: success\n", __FUNCTION__);
   return true;
}

static void
intelTexImage(struct gl_context * ctx,
              GLuint dims,
              struct gl_texture_image *texImage,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack)
{
   DBG("%s target %s level %d %dx%dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       texImage->Level, texImage->Width, texImage->Height, texImage->Depth);

   /* Attempt to use the blitter for PBO image uploads.
    */
   if (dims <= 2 &&
       try_pbo_upload(ctx, texImage, unpack, format, type, pixels)) {
      return;
   }

   DBG("%s: upload image %dx%dx%d pixels %p\n",
       __FUNCTION__, texImage->Width, texImage->Height, texImage->Depth,
       pixels);

   _mesa_store_teximage(ctx, dims, texImage,
                        format, type, pixels, unpack);
}


/**
 * Binds a region to a texture image, like it was uploaded by glTexImage2D().
 *
 * Used for GLX_EXT_texture_from_pixmap and EGL image extensions,
 */
static void
intel_set_texture_image_region(struct gl_context *ctx,
			       struct gl_texture_image *image,
			       struct intel_region *region,
			       GLenum target,
			       GLenum internalFormat,
			       mesa_format format,
                               uint32_t offset,
                               GLuint width,
                               GLuint height,
                               GLuint tile_x,
                               GLuint tile_y)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);
   bool has_surface_tile_offset = false;
   uint32_t draw_x, draw_y;

   _mesa_init_teximage_fields(&intel->ctx, image,
			      width, height, 1,
			      0, internalFormat, format);

   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   intel_image->mt = intel_miptree_create_layout(intel, target, image->TexFormat,
                                                 0, 0,
                                                 width, height, 1,
                                                 true);
   if (intel_image->mt == NULL)
       return;
   intel_region_reference(&intel_image->mt->region, region);
   intel_image->mt->total_width = width;
   intel_image->mt->total_height = height;
   intel_image->mt->level[0].slice[0].x_offset = tile_x;
   intel_image->mt->level[0].slice[0].y_offset = tile_y;

   intel_miptree_get_tile_offsets(intel_image->mt, 0, 0, &draw_x, &draw_y);

   /* From "OES_EGL_image" error reporting. We report GL_INVALID_OPERATION
    * for EGL images from non-tile aligned sufaces in gen4 hw and earlier which has
    * trouble resolving back to destination image due to alignment issues.
    */
   if (!has_surface_tile_offset &&
       (draw_x != 0 || draw_y != 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
      intel_miptree_release(&intel_image->mt);
      return;
   }

   intel_texobj->needs_validate = true;

   intel_image->mt->offset = offset;
   assert(region->pitch % region->cpp == 0);
   intel_image->base.RowStride = region->pitch / region->cpp;

   /* Immediately validate the image to the object. */
   intel_miptree_reference(&intel_texobj->mt, intel_image->mt);
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
   struct intel_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   int level = 0, internalFormat = 0;
   mesa_format texFormat = MESA_FORMAT_NONE;

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
   if (!rb || !rb->mt)
      return;

   if (rb->mt->cpp == 4) {
      if (texture_format == __DRI_TEXTURE_FORMAT_RGB) {
         internalFormat = GL_RGB;
         texFormat = MESA_FORMAT_B8G8R8X8_UNORM;
      }
      else {
         internalFormat = GL_RGBA;
         texFormat = MESA_FORMAT_B8G8R8A8_UNORM;
      }
   } else if (rb->mt->cpp == 2) {
      internalFormat = GL_RGB;
      texFormat = MESA_FORMAT_B5G6R5_UNORM;
   }

   _mesa_lock_texture(&intel->ctx, texObj);
   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   intel_set_texture_image_region(ctx, texImage, rb->mt->region, target,
                                  internalFormat, texFormat, 0,
                                  rb->mt->region->width,
                                  rb->mt->region->height,
                                  0, 0);
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

static void
intel_image_target_texture_2d(struct gl_context *ctx, GLenum target,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage,
			      GLeglImageOES image_handle)
{
   struct intel_context *intel = intel_context(ctx);
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   intel_set_texture_image_region(ctx, texImage, image->region,
				  target, image->internal_format,
                                  image->format, image->offset,
                                  image->width,  image->height,
                                  image->tile_x, image->tile_y);
}

void
intelInitTextureImageFuncs(struct dd_function_table *functions)
{
   functions->TexImage = intelTexImage;
   functions->EGLImageTargetTexture2D = intel_image_target_texture_2d;
}
