#include "main/mtypes.h"
#include "main/macros.h"
#include "main/samplerobj.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * When validating, we only care about the texture images that could
 * be seen, so for non-mipmapped modes we want to ignore everything
 * but BaseLevel.
 */
static void
intel_update_max_level(struct intel_context *intel,
		       struct intel_texture_object *intelObj,
		       struct gl_sampler_object *sampler)
{
   struct gl_texture_object *tObj = &intelObj->base;

   if (sampler->MinFilter == GL_NEAREST ||
       sampler->MinFilter == GL_LINEAR) {
      intelObj->_MaxLevel = tObj->BaseLevel;
   } else {
      intelObj->_MaxLevel = tObj->_MaxLevel;
   }
}

/**
 * Copies the image's contents at its level into the object's miptree,
 * and updates the image to point at the object's miptree.
 */
static void
copy_image_data_to_tree(struct intel_context *intel,
                        struct intel_texture_object *intelObj,
                        struct intel_texture_image *intelImage)
{
   if (intelImage->mt) {
      /* Copy potentially with the blitter:
       */
      intel_miptree_image_copy(intel,
                               intelObj->mt,
                               intelImage->face,
                               intelImage->level, intelImage->mt);

      intel_miptree_release(intel, &intelImage->mt);
   }
   else {
      assert(intelImage->base.Data != NULL);

      /* More straightforward upload.  
       */
      intel_miptree_image_data(intel,
                               intelObj->mt,
                               intelImage->face,
                               intelImage->level,
                               intelImage->base.Data,
                               intelImage->base.RowStride,
                               intelImage->base.RowStride *
                               intelImage->base.Height);
      _mesa_align_free(intelImage->base.Data);
      intelImage->base.Data = NULL;
   }

   intel_miptree_reference(&intelImage->mt, intelObj->mt);
}


/*  
 */
GLuint
intel_finalize_mipmap_tree(struct intel_context *intel, GLuint unit)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_texture_object *tObj = intel->ctx.Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   GLuint face, i;
   GLuint nr_faces = 0;
   struct intel_texture_image *firstImage;

   /* We know/require this is true by now: 
    */
   assert(intelObj->base._Complete);

   /* What levels must the tree include at a minimum?
    */
   intel_update_max_level(intel, intelObj, sampler);
   firstImage = intel_texture_image(tObj->Image[0][tObj->BaseLevel]);

   /* Fallback case:
    */
   if (firstImage->base.Border) {
      if (intelObj->mt) {
         intel_miptree_release(intel, &intelObj->mt);
      }
      return GL_FALSE;
   }

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    *
    * For pre-gen4, we have to match first_level == tObj->BaseLevel,
    * because we don't have the control that gen4 does to make min/mag
    * determination happen at a nonzero (hardware) baselevel.  Because
    * of that, we just always relayout on baselevel change.
    */
   if (intelObj->mt &&
       (intelObj->mt->target != intelObj->base.Target ||
	intelObj->mt->format != firstImage->base.TexFormat ||
	intelObj->mt->first_level != tObj->BaseLevel ||
	intelObj->mt->last_level < intelObj->_MaxLevel ||
	intelObj->mt->width0 != firstImage->base.Width ||
	intelObj->mt->height0 != firstImage->base.Height ||
	intelObj->mt->depth0 != firstImage->base.Depth)) {
      intel_miptree_release(intel, &intelObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intelObj->mt = intel_miptree_create(intel,
                                          intelObj->base.Target,
					  firstImage->base.TexFormat,
                                          tObj->BaseLevel,
                                          intelObj->_MaxLevel,
                                          firstImage->base.Width,
                                          firstImage->base.Height,
                                          firstImage->base.Depth,
					  GL_TRUE);
      if (!intelObj->mt)
         return GL_FALSE;
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   for (face = 0; face < nr_faces; face++) {
      for (i = tObj->BaseLevel; i <= intelObj->_MaxLevel; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);
	 /* skip too small size mipmap */
 	 if (intelImage == NULL)
		 break;
	 /* Need to import images in main memory or held in other trees.
	  * If it's a render target, then its data isn't needed to be in
	  * the object tree (otherwise we'd be FBO incomplete), and we need
	  * to keep track of the image's MT as needing to be pulled in still,
	  * or we'll lose the rendering that's done to it.
          */
         if (intelObj->mt != intelImage->mt &&
	     !intelImage->used_as_render_target) {
            copy_image_data_to_tree(intel, intelObj, intelImage);
         }
      }
   }

   return GL_TRUE;
}

void
intel_tex_map_level_images(struct intel_context *intel,
			   struct intel_texture_object *intelObj,
			   int level)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face;

   for (face = 0; face < nr_faces; face++) {
      struct intel_texture_image *intelImage =
	 intel_texture_image(intelObj->base.Image[face][level]);

      if (intelImage && intelImage->mt) {
	 intelImage->base.Data =
	    intel_miptree_image_map(intel,
				    intelImage->mt,
				    intelImage->face,
				    intelImage->level,
				    &intelImage->base.RowStride,
				    intelImage->base.ImageOffsets);
	 /* convert stride to texels, not bytes */
	 intelImage->base.RowStride /= intelImage->mt->cpp;
	 /* intelImage->base.ImageStride /= intelImage->mt->cpp; */
      }
   }
}

void
intel_tex_unmap_level_images(struct intel_context *intel,
			     struct intel_texture_object *intelObj,
			     int level)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face;

   for (face = 0; face < nr_faces; face++) {
      struct intel_texture_image *intelImage =
	 intel_texture_image(intelObj->base.Image[face][level]);

      if (intelImage && intelImage->mt) {
	 intel_miptree_image_unmap(intel, intelImage->mt);
	 intelImage->base.Data = NULL;
      }
   }
}

void
intel_tex_map_images(struct intel_context *intel,
                     struct intel_texture_object *intelObj)
{
   int i;

   DBG("%s\n", __FUNCTION__);

   for (i = intelObj->base.BaseLevel; i <= intelObj->_MaxLevel; i++)
      intel_tex_map_level_images(intel, intelObj, i);
}

void
intel_tex_unmap_images(struct intel_context *intel,
                       struct intel_texture_object *intelObj)
{
   int i;

   for (i = intelObj->base.BaseLevel; i <= intelObj->_MaxLevel; i++)
      intel_tex_unmap_level_images(intel, intelObj, i);
}
