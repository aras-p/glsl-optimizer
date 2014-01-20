#include "swrast/swrast.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/mipmap.h"
#include "drivers/common/meta.h"
#include "brw_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"
#include "intel_fbo.h"

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

   if (obj == NULL)
      return NULL;

   _mesa_initialize_texture_object(ctx, &obj->base, name, target);

   obj->needs_validate = true;

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
				 struct gl_texture_image *image)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);

   assert(image->Border == 0);

   /* Quantize sample count */
   if (image->NumSamples) {
      image->NumSamples = intel_quantize_num_samples(brw->intelScreen, image->NumSamples);
      if (!image->NumSamples)
         return false;
   }

   /* Because the driver uses AllocTextureImageBuffer() internally, it may end
    * up mismatched with FreeTextureImageBuffer(), but that is safe to call
    * multiple times.
    */
   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   if (!_swrast_init_texture_image(image))
      return false;

   if (intel_texobj->mt &&
       intel_miptree_match_image(intel_texobj->mt, image)) {
      intel_miptree_reference(&intel_image->mt, intel_texobj->mt);
      DBG("%s: alloc obj %p level %d %dx%dx%d using object's miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_texobj->mt);
   } else {
      intel_image->mt = intel_miptree_create_for_teximage(brw, intel_texobj,
                                                          intel_image,
                                                          false);

      /* Even if the object currently has a mipmap tree associated
       * with it, this one is a more likely candidate to represent the
       * whole object since our level didn't fit what was there
       * before, and any lower levels would fit into our miptree.
       */
      intel_miptree_reference(&intel_texobj->mt, intel_image->mt);

      DBG("%s: alloc obj %p level %d %dx%dx%d using new miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_image->mt);
   }

   intel_texobj->needs_validate = true;

   return true;
}

/**
 * ctx->Driver.AllocTextureStorage() handler.
 *
 * Compare this to _mesa_alloc_texture_storage, which would call into
 * intel_alloc_texture_image_buffer() above.
 */
static GLboolean
intel_alloc_texture_storage(struct gl_context *ctx,
                            struct gl_texture_object *texobj,
                            GLsizei levels, GLsizei width,
                            GLsizei height, GLsizei depth)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);
   struct gl_texture_image *first_image = texobj->Image[0][0];
   int num_samples = intel_quantize_num_samples(brw->intelScreen,
                                                first_image->NumSamples);
   const int numFaces = _mesa_num_tex_faces(texobj->Target);
   int face;
   int level;

   /* If the object's current miptree doesn't match what we need, make a new
    * one.
    */
   if (!intel_texobj->mt ||
       !intel_miptree_match_image(intel_texobj->mt, first_image) ||
       intel_texobj->mt->last_level != levels - 1) {
      intel_miptree_release(&intel_texobj->mt);
      intel_texobj->mt = intel_miptree_create(brw, texobj->Target,
                                              first_image->TexFormat,
                                              0, levels - 1,
                                              width, height, depth,
                                              false, /* expect_accelerated */
                                              num_samples,
                                              INTEL_MIPTREE_TILING_ANY);

   }

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < levels; level++) {
         struct gl_texture_image *image = texobj->Image[face][level];
         struct intel_texture_image *intel_image = intel_texture_image(image);

         image->NumSamples = num_samples;

         _swrast_free_texture_image_buffer(ctx, image);
         if (!_swrast_init_texture_image(image))
            return false;

         intel_miptree_reference(&intel_image->mt, intel_texobj->mt);
      }
   }

   /* The miptree is in a validated state, so no need to check later. */
   intel_texobj->needs_validate = false;
   intel_texobj->validated_first_level = 0;
   intel_texobj->validated_last_level = levels - 1;
   intel_texobj->_Format = intel_texobj->mt->format;

   return true;
}


static void
intel_free_texture_image_buffer(struct gl_context * ctx,
				struct gl_texture_image *texImage)
{
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   intel_miptree_release(&intelImage->mt);

   _swrast_free_texture_image_buffer(ctx, texImage);
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
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);
   struct intel_mipmap_tree *mt = intel_image->mt;

   /* Our texture data is always stored in a miptree. */
   assert(mt);

   /* Check that our caller wasn't confused about how to map a 1D texture. */
   assert(tex_image->TexObject->Target != GL_TEXTURE_1D_ARRAY ||
	  h == 1);

   /* intel_miptree_map operates on a unified "slice" number that references the
    * cube face, since it's all just slices to the miptree code.
    */
   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   intel_miptree_map(brw, mt,
                     tex_image->Level + tex_image->TexObject->MinLevel,
                     slice + tex_image->TexObject->MinLayer,
                     x, y, w, h, mode,
                     (void **)map, stride);
}

static void
intel_unmap_texture_image(struct gl_context *ctx,
			  struct gl_texture_image *tex_image, GLuint slice)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);
   struct intel_mipmap_tree *mt = intel_image->mt;

   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   intel_miptree_unmap(brw, mt,
         tex_image->Level + tex_image->TexObject->MinLevel,
         slice + tex_image->TexObject->MinLayer);
}

static GLboolean
intel_texture_view(struct gl_context *ctx,
                   struct gl_texture_object *texObj,
                   struct gl_texture_object *origTexObj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_object *intel_tex = intel_texture_object(texObj);
   struct intel_texture_object *intel_orig_tex = intel_texture_object(origTexObj);

   assert(intel_orig_tex->mt);
   intel_miptree_reference(&intel_tex->mt, intel_orig_tex->mt);

   /* Since we can only make views of immutable-format textures,
    * we can assume that everything is in origTexObj's miptree.
    *
    * Mesa core has already made us a copy of all the teximage objects,
    * except it hasn't copied our mt pointers, etc.
    */
   const int numFaces = _mesa_num_tex_faces(texObj->Target);
   const int numLevels = texObj->NumLevels;

   int face;
   int level;

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < numLevels; level++) {
         struct gl_texture_image *image = texObj->Image[face][level];
         struct intel_texture_image *intel_image = intel_texture_image(image);

         intel_miptree_reference(&intel_image->mt, intel_orig_tex->mt);
      }
   }

   /* The miptree is in a validated state, so no need to check later. */
   intel_tex->needs_validate = false;
   intel_tex->validated_first_level = 0;
   intel_tex->validated_last_level = numLevels - 1;

   /* Set the validated texture format, with the same adjustments that
    * would have been applied to determine the underlying texture's
    * mt->format.
    */
   intel_tex->_Format = intel_depth_format_for_depthstencil_format(
         intel_lower_compressed_format(brw, texObj->Image[0][0]->TexFormat));

   return GL_TRUE;
}

void
intelInitTextureFuncs(struct dd_function_table *functions)
{
   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTextureImage = intelDeleteTextureImage;
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->AllocTextureImageBuffer = intel_alloc_texture_image_buffer;
   functions->FreeTextureImageBuffer = intel_free_texture_image_buffer;
   functions->AllocTextureStorage = intel_alloc_texture_storage;
   functions->MapTextureImage = intel_map_texture_image;
   functions->UnmapTextureImage = intel_unmap_texture_image;
   functions->TextureView = intel_texture_view;
}
