
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

#include "drivers/common/meta.h"

#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_image.h"

#include "brw_context.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Work back from the specified level of the image to the baselevel and create a
 * miptree of that size.
 */
struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct brw_context *brw,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  bool expect_accelerated_upload)
{
   GLuint lastLevel;
   int width, height, depth;
   GLuint i;

   intel_miptree_get_dimensions_for_image(&intelImage->base.Base,
                                          &width, &height, &depth);

   DBG("%s\n", __FUNCTION__);

   /* Figure out image dimensions at start level. */
   for (i = intelImage->base.Base.Level; i > 0; i--) {
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
       intelImage->base.Base.Level == 0 &&
       !intelObj->base.GenerateMipmap) {
      lastLevel = 0;
   } else {
      lastLevel = _mesa_get_tex_max_num_levels(intelObj->base.Target,
                                               width, height, depth) - 1;
   }

   return intel_miptree_create(brw,
			       intelObj->base.Target,
			       intelImage->base.Base.TexFormat,
			       0,
			       lastLevel,
			       width,
			       height,
			       depth,
			       expect_accelerated_upload,
                               intelImage->base.Base.NumSamples,
                               INTEL_MIPTREE_TILING_ANY,
                               false);
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
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset;
   drm_intel_bo *src_buffer;

   if (!_mesa_is_bufferobj(unpack->BufferObj))
      return false;

   DBG("trying pbo upload\n");

   if (ctx->_ImageTransferState || unpack->SkipPixels || unpack->SkipRows) {
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

   int src_stride =
      _mesa_image_row_stride(unpack, image->Width, format, type);

   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset = (GLuint) (unsigned long) pixels;
   src_buffer = intel_bufferobj_buffer(brw, pbo,
                                       src_offset, src_stride * image->Height);

   struct intel_mipmap_tree *pbo_mt =
      intel_miptree_create_for_bo(brw,
                                  src_buffer,
                                  intelImage->mt->format,
                                  src_offset,
                                  image->Width, image->Height,
                                  src_stride);
   if (!pbo_mt)
      return false;

   if (!intel_miptree_blit(brw,
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
   bool ok;

   DBG("%s mesa_format %s target %s format %s type %s level %d %dx%dx%d\n",
       __FUNCTION__, _mesa_get_format_name(texImage->TexFormat),
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       _mesa_lookup_enum_by_nr(format), _mesa_lookup_enum_by_nr(type),
       texImage->Level, texImage->Width, texImage->Height, texImage->Depth);

   ok = intel_texsubimage_tiled_memcpy(ctx, dims, texImage,
                                       0, 0, 0, /*x,y,z offsets*/
                                       texImage->Width,
                                       texImage->Height,
                                       texImage->Depth,
                                       format, type, pixels, unpack,
                                       true /*for_glTexImage*/);
   if (ok)
      return;

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
 * Binds a BO to a texture image, as if it was uploaded by glTexImage2D().
 *
 * Used for GLX_EXT_texture_from_pixmap and EGL image extensions,
 */
static void
intel_set_texture_image_bo(struct gl_context *ctx,
                           struct gl_texture_image *image,
                           drm_intel_bo *bo,
                           GLenum target,
                           GLenum internalFormat,
                           mesa_format format,
                           uint32_t offset,
                           GLuint width, GLuint height,
                           GLuint pitch,
                           GLuint tile_x, GLuint tile_y)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);
   uint32_t draw_x, draw_y;

   _mesa_init_teximage_fields(&brw->ctx, image,
			      width, height, 1,
			      0, internalFormat, format);

   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   intel_image->mt = intel_miptree_create_for_bo(brw, bo, image->TexFormat,
                                                 0, width, height, pitch);
   if (intel_image->mt == NULL)
       return;
   intel_image->mt->target = target;
   intel_image->mt->total_width = width;
   intel_image->mt->total_height = height;
   intel_image->mt->level[0].slice[0].x_offset = tile_x;
   intel_image->mt->level[0].slice[0].y_offset = tile_y;

   intel_miptree_get_tile_offsets(intel_image->mt, 0, 0, &draw_x, &draw_y);

   /* From "OES_EGL_image" error reporting. We report GL_INVALID_OPERATION
    * for EGL images from non-tile aligned sufaces in gen4 hw and earlier which has
    * trouble resolving back to destination image due to alignment issues.
    */
   if (!brw->has_surface_tile_offset &&
       (draw_x != 0 || draw_y != 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
      intel_miptree_release(&intel_image->mt);
      return;
   }

   intel_texobj->needs_validate = true;

   intel_image->mt->offset = offset;
   assert(pitch % intel_image->mt->cpp == 0);
   intel_image->base.RowStride = pitch / intel_image->mt->cpp;

   /* Immediately validate the image to the object. */
   intel_miptree_reference(&intel_texobj->mt, intel_image->mt);
}

void
intelSetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
		   GLint texture_format,
		   __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = dPriv->driverPrivate;
   struct brw_context *brw = pDRICtx->driverPrivate;
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   int level = 0, internalFormat = 0;
   mesa_format texFormat = MESA_FORMAT_NONE;

   texObj = _mesa_get_current_tex_object(ctx, target);

   if (!texObj)
      return;

   if (dPriv->lastStamp != dPriv->dri2.stamp ||
       !pDRICtx->driScreenPriv->dri2.useInvalidate)
      intel_update_renderbuffers(pDRICtx, dPriv);

   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   /* If the miptree isn't set, then intel_update_renderbuffers was unable
    * to get the BO for the drawable from the window system.
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

   _mesa_lock_texture(&brw->ctx, texObj);
   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   intel_miptree_make_shareable(brw, rb->mt);
   intel_set_texture_image_bo(ctx, texImage, rb->mt->bo, target,
                              internalFormat, texFormat, 0,
                              rb->Base.Base.Width,
                              rb->Base.Base.Height,
                              rb->mt->pitch,
                              0, 0);
   _mesa_unlock_texture(&brw->ctx, texObj);
}

static GLboolean
intel_bind_renderbuffer_tex_image(struct gl_context *ctx,
                                  struct gl_renderbuffer *rb,
                                  struct gl_texture_image *image)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);

   /* We can only handle RB allocated with AllocRenderbufferStorage, or
    * window-system renderbuffers.
    */
   assert(!rb->TexImage);

   if (!irb->mt)
      return false;

   _mesa_lock_texture(ctx, texobj);
   _mesa_init_teximage_fields(ctx, image,
			      rb->Width, rb->Height, 1,
			      0, rb->InternalFormat, rb->Format);
   image->NumSamples = rb->NumSamples;

   intel_miptree_reference(&intel_image->mt, irb->mt);

   /* Immediately validate the image to the object. */
   intel_miptree_reference(&intel_texobj->mt, intel_image->mt);

   intel_texobj->needs_validate = true;
   _mesa_unlock_texture(ctx, texobj);

   return true;
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
   struct brw_context *brw = brw_context(ctx);
   __DRIscreen *screen;
   __DRIimage *image;

   screen = brw->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   /**
    * Images originating via EGL_EXT_image_dma_buf_import can be used only
    * with GL_OES_EGL_image_external only.
    */
   if (image->dma_buf_imported && target != GL_TEXTURE_EXTERNAL_OES) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetTexture2DOES(dma buffers can be used with "
               "GL_OES_EGL_image_external only");
      return;
   }

   if (target == GL_TEXTURE_EXTERNAL_OES && !image->dma_buf_imported) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetTexture2DOES(external target is enabled only "
               "for images created with EGL_EXT_image_dma_buf_import");
      return;
   }

   /* Disallow depth/stencil textures: we don't have a way to pass the
    * separate stencil miptree of a GL_DEPTH_STENCIL texture through.
    */
   if (image->has_depthstencil) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
      return;
   }

   intel_set_texture_image_bo(ctx, texImage, image->bo,
                              target, image->internal_format,
                              image->format, image->offset,
                              image->width,  image->height,
                              image->pitch,
                              image->tile_x, image->tile_y);
}

static bool
blit_texture_to_pbo(struct gl_context *ctx,
                    GLenum format, GLenum type,
                    GLvoid * pixels, struct gl_texture_image *texImage)
{
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   struct brw_context *brw = brw_context(ctx);
   const struct gl_pixelstore_attrib *pack = &ctx->Pack;
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   drm_intel_bo *dst_buffer;
   GLenum target = texImage->TexObject->Target;

   /* Check if we can use GPU blit to copy from the hardware texture
    * format to the user's format/type.
    * Note that GL's pixel transfer ops don't apply to glGetTexImage()
    */

   if (!_mesa_format_matches_format_and_type(intelImage->mt->format, format,
                                             type, false))
   {
      perf_debug("%s: unsupported format, fallback to CPU mapping for PBO\n",
                 __FUNCTION__);

      return false;
   }

   if (ctx->_ImageTransferState) {
      perf_debug("%s: bad transfer state, fallback to CPU mapping for PBO\n",
                 __FUNCTION__);
      return false;
   }

   if (pack->SwapBytes || pack->LsbFirst) {
      perf_debug("%s: unsupported pack swap params\n",
                 __FUNCTION__);
      return false;
   }

   if (target == GL_TEXTURE_1D_ARRAY ||
       target == GL_TEXTURE_2D_ARRAY ||
       target == GL_TEXTURE_CUBE_MAP_ARRAY ||
       target == GL_TEXTURE_3D) {
      perf_debug("%s: no support for multiple slices, fallback to CPU mapping "
                 "for PBO\n", __FUNCTION__);
      return false;
   }

   int dst_stride = _mesa_image_row_stride(pack, texImage->Width, format, type);
   bool dst_flip = false;
   /* Mesa flips the dst_stride for ctx->Pack.Invert, our mt must have a
    * normal dst_stride.
    */
   struct gl_pixelstore_attrib uninverted_pack = *pack;
   if (ctx->Pack.Invert) {
      dst_stride = -dst_stride;
      dst_flip = true;
      uninverted_pack.Invert = false;
   }
   dst_offset = (GLintptr) pixels;
   dst_offset += _mesa_image_offset(2, &uninverted_pack, texImage->Width,
                                    texImage->Height, format, type, 0, 0, 0);
   dst_buffer = intel_bufferobj_buffer(brw, dst, dst_offset,
                                       texImage->Height * dst_stride);

   struct intel_mipmap_tree *pbo_mt =
      intel_miptree_create_for_bo(brw,
                                  dst_buffer,
                                  intelImage->mt->format,
                                  dst_offset,
                                  texImage->Width, texImage->Height,
                                  dst_stride);

   if (!pbo_mt)
      return false;

   if (!intel_miptree_blit(brw,
                           intelImage->mt, texImage->Level, texImage->Face,
                           0, 0, false,
                           pbo_mt, 0, 0,
                           0, 0, dst_flip,
                           texImage->Width, texImage->Height, GL_COPY))
      return false;

   intel_miptree_release(&pbo_mt);

   return true;
}

static void
intel_get_tex_image(struct gl_context *ctx,
                    GLenum format, GLenum type, GLvoid *pixels,
                    struct gl_texture_image *texImage) {
   DBG("%s\n", __FUNCTION__);

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* Using PBOs, so try the BLT based path. */
      if (blit_texture_to_pbo(ctx, format, type, pixels, texImage))
         return;

   }

   _mesa_meta_GetTexImage(ctx, format, type, pixels, texImage);

   DBG("%s - DONE\n", __FUNCTION__);
}

void
intelInitTextureImageFuncs(struct dd_function_table *functions)
{
   functions->TexImage = intelTexImage;
   functions->EGLImageTargetTexture2D = intel_image_target_texture_2d;
   functions->BindRenderbufferTexImage = intel_bind_renderbuffer_tex_image;
   functions->GetTexImage = intel_get_tex_image;
}
