#include "swrast/swrast.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/mipmap.h"
#include "drivers/common/meta.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static struct gl_texture_image *
intelNewTextureImage(struct gl_context * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(intel_texture_image);
}

static void
intelDeleteTextureImage(struct gl_context * ctx, struct gl_texture_image *img)
{
   /* nothing special (yet) for intel_texture_image */
   _mesa_delete_texture_image(ctx, img);
}


static struct gl_texture_object *
intelNewTextureObject(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   (void) ctx;

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static void 
intelDeleteTextureObject(struct gl_context *ctx,
			 struct gl_texture_object *texObj)
{
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   intel_miptree_release(&intelObj->mt);
   _mesa_delete_texture_object(ctx, texObj);
}

static GLboolean
intel_alloc_texture_image_buffer(struct gl_context *ctx,
				 struct gl_texture_image *image,
				 gl_format format, GLsizei width,
				 GLsizei height, GLsizei depth)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);

   /* Because the driver uses AllocTextureImageBuffer() internally, it may end
    * up mismatched with FreeTextureImageBuffer(), but that is safe to call
    * multiple times.
    */
   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   if (intel_texobj->mt &&
       intel_miptree_match_image(intel_texobj->mt, image)) {
      intel_miptree_reference(&intel_image->mt, intel_texobj->mt);
      DBG("%s: alloc obj %p level %d %dx%dx%d using object's miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          width, height, depth, intel_texobj->mt);
      return true;
   } else if (image->Border == 0) {
      intel_image->mt = intel_miptree_create_for_teximage(intel, intel_texobj,
                                                          intel_image,
                                                          false);

      /* Even if the object currently has a mipmap tree associated
       * with it, this one is a more likely candidate to represent the
       * whole object since our level didn't fit what was there
       * before, and any lower levels would fit into our miptree.
       */
      intel_miptree_reference(&intel_texobj->mt, intel_image->mt);

      if (intel->must_use_separate_stencil
          && image->TexFormat == MESA_FORMAT_S8_Z24) {
         intel_tex_image_s8z24_create_renderbuffers(intel, intel_image);
      }

      DBG("%s: alloc obj %p level %d %dx%dx%d using new miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          width, height, depth, intel_image->mt);
      return true;
   }

   DBG("%s: alloc obj %p level %d %dx%dx%d using swrast\n",
       __FUNCTION__, texobj, image->Level, width, height, depth);

   return _swrast_alloc_texture_image_buffer(ctx, image, format,
					     width, height, depth);
}

static void
intel_free_texture_image_buffer(struct gl_context * ctx,
				struct gl_texture_image *texImage)
{
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   intel_miptree_release(&intelImage->mt);

   if (texImage->Data) {
      _mesa_free_texmemory(texImage->Data);
      texImage->Data = NULL;
   }

   _mesa_reference_renderbuffer(&intelImage->depth_rb, NULL);
   _mesa_reference_renderbuffer(&intelImage->stencil_rb, NULL);
}

/**
 * Map texture memory/buffer into user space.
 * Note: the region of interest parameters are ignored here.
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride in bytes
 */
static void
intel_map_texture_image(struct gl_context *ctx,
			struct gl_texture_image *tex_image,
			GLuint slice,
			GLuint x, GLuint y, GLuint w, GLuint h,
			GLbitfield mode,
			GLubyte **map,
			GLint *stride)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   unsigned int bw, bh;

   /* Check that our caller wasn't confused about how to map a 1D texture. */
   assert(tex_image->TexObject->Target != GL_TEXTURE_1D_ARRAY ||
	  h == 1);

   if (intel_image->stencil_rb) {
      /*
       * The texture has packed depth/stencil format, but uses separate
       * stencil. The texture's embedded stencil buffer contains the real
       * stencil data, so copy that into the miptree.
       */
      intel_tex_image_s8z24_gather(intel, intel_image);
   }

   /* For compressed formats, the stride is the number of bytes per
    * row of blocks.  intel_miptree_get_image_offset() already does
    * the divide.
    */
   _mesa_get_format_block_size(tex_image->TexFormat, &bw, &bh);
   assert(y % bh == 0);
   y /= bh;

   if (likely(mt)) {
      void *base = intel_region_map(intel, mt->region, mode);
      unsigned int image_x, image_y;

      intel_miptree_get_image_offset(mt, tex_image->Level, tex_image->Face,
				     slice, &image_x, &image_y);
      x += image_x;
      y += image_y;

      *stride = mt->region->pitch * mt->cpp;
      *map = base + y * *stride + x * mt->cpp;

      DBG("%s: %d,%d %dx%d from mt %p %d,%d = %p/%d\n", __FUNCTION__,
	  x - image_x, y - image_y, w, h,
	  mt, x, y, *map, *stride);
   } else {
      /* texture data is in malloc'd memory */
      GLuint width = tex_image->Width;
      GLuint height = ALIGN(tex_image->Height, bh) / bh;
      GLuint texelSize = _mesa_get_format_bytes(tex_image->TexFormat);

      assert(map);

      *stride = _mesa_format_row_stride(tex_image->TexFormat, width);
      *map = tex_image->Data + (slice * height + y) * *stride + x * texelSize;

      DBG("%s: %d,%d %dx%d from data %p = %p/%d\n", __FUNCTION__,
	  x, y, w, h,
	  tex_image->Data, *map, *stride);
   }
}

static void
intel_unmap_texture_image(struct gl_context *ctx,
			  struct gl_texture_image *tex_image, GLuint slice)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);

   if (intel_image->mt)
      intel_region_unmap(intel, intel_image->mt->region);

   if (intel_image->stencil_rb) {
      /*
       * The texture has packed depth/stencil format, but uses separate
       * stencil. The texture's embedded stencil buffer contains the real
       * stencil data, so copy that into the miptree.
       */
      intel_tex_image_s8z24_scatter(intel, intel_image);
   }
}

/**
 * Called via ctx->Driver.GenerateMipmap()
 * This is basically a wrapper for _mesa_meta_GenerateMipmap() which checks
 * if we'll be using software mipmap generation.  In that case, we need to
 * map/unmap the base level texture image.
 */
static void
intelGenerateMipmap(struct gl_context *ctx, GLenum target,
                    struct gl_texture_object *texObj)
{
   if (_mesa_meta_check_generate_mipmap_fallback(ctx, target, texObj)) {
      fallback_debug("%s - fallback to swrast\n", __FUNCTION__);

      _mesa_generate_mipmap(ctx, target, texObj);
   }
   else {
      _mesa_meta_GenerateMipmap(ctx, target, texObj);
   }
}


void
intelInitTextureFuncs(struct dd_function_table *functions)
{
   functions->GenerateMipmap = intelGenerateMipmap;

   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTextureImage = intelDeleteTextureImage;
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->AllocTextureImageBuffer = intel_alloc_texture_image_buffer;
   functions->FreeTextureImageBuffer = intel_free_texture_image_buffer;
   functions->MapTextureImage = intel_map_texture_image;
   functions->UnmapTextureImage = intel_unmap_texture_image;
}
