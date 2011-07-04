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


static struct gl_texture_object *
intelNewTextureObject(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

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
intelFreeTextureImageData(struct gl_context * ctx, struct gl_texture_image *texImage)
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
         /* Update the level information in our private data in the new images,
          * since it didn't get set as part of a normal TexImage path.
          */
         for (face = 0; face < nr_faces; face++) {
            for (i = texObj->BaseLevel + 1; i < texObj->MaxLevel; i++) {
               struct intel_texture_image *intelImage =
                  intel_texture_image(texObj->Image[face][i]);
               if (!intelImage)
                  break;
               intelImage->level = i;
               intelImage->face = face;
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
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->FreeTexImageData = intelFreeTextureImageData;
}
