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
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   _mesa_delete_texture_object(ctx, texObj);
}


static void
intel_free_texture_image_buffer(struct gl_context * ctx,
				struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
   }

   if (texImage->Data) {
      _mesa_free_texmemory(texImage->Data);
      texImage->Data = NULL;
   }

   if (intelImage->depth_rb) {
      _mesa_reference_renderbuffer(&intelImage->depth_rb, NULL);
   }

   if (intelImage->stencil_rb) {
      _mesa_reference_renderbuffer(&intelImage->stencil_rb, NULL);
   }
}

/**
 * Map texture memory/buffer into user space.
 * Note: the region of interest parameters are ignored here.
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
   _mesa_get_format_block_size(mt->format, &bw, &bh);
   assert(y % bh == 0);
   y /= bh;

   if (likely(mt)) {
      void *base = intel_region_map(intel, mt->region);
      unsigned int image_x, image_y;

      intel_miptree_get_image_offset(mt, tex_image->Level, tex_image->Face,
				     slice, &image_x, &image_y);
      x += image_x;
      y += image_y;

      *stride = mt->region->pitch * mt->cpp;
      *map = base + y * *stride + x * mt->cpp;
   } else {
      /* texture data is in malloc'd memory */
      GLuint width = tex_image->Width;
      GLuint height = ALIGN(tex_image->Height, bh) / bh;
      GLuint texelSize = _mesa_get_format_bytes(tex_image->TexFormat);

      assert(map);

      *stride = _mesa_format_row_stride(tex_image->TexFormat, width);
      *map = tex_image->Data + (slice * height + y) * *stride + x * texelSize;

      return;
   }
}

static void
intel_unmap_texture_image(struct gl_context *ctx,
			  struct gl_texture_image *tex_image, GLuint slice)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);

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
      /* sw path: need to map texture images */
      struct intel_context *intel = intel_context(ctx);
      struct intel_texture_object *intelObj = intel_texture_object(texObj);
      struct gl_texture_image *first_image = texObj->Image[0][texObj->BaseLevel];

      fallback_debug("%s - fallback to swrast\n", __FUNCTION__);

      intel_tex_map_level_images(intel, intelObj, texObj->BaseLevel);
      _mesa_generate_mipmap(ctx, target, texObj);
      intel_tex_unmap_level_images(intel, intelObj, texObj->BaseLevel);

      if (!_mesa_is_format_compressed(first_image->TexFormat)) {
         GLuint nr_faces = (texObj->Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
         GLuint face, i;
         for (face = 0; face < nr_faces; face++) {
            for (i = texObj->BaseLevel + 1; i < texObj->MaxLevel; i++) {
               struct intel_texture_image *intelImage =
                  intel_texture_image(texObj->Image[face][i]);
               if (!intelImage)
                  break;
               /* Unreference the miptree to signal that the new Data is a
                * bare pointer from mesa.
                */
               intel_miptree_release(intel, &intelImage->mt);
            }
         }
      }
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
   functions->FreeTextureImageBuffer = intel_free_texture_image_buffer;
   functions->MapTextureImage = intel_map_texture_image;
   functions->UnmapTextureImage = intel_unmap_texture_image;
}
