#include "main/mtypes.h"
#include "main/macros.h"
#include "main/samplerobj.h"
#include "main/texobj.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"
#include "intel_tex.h"
#include "intel_tex_layout.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * When validating, we only care about the texture images that could
 * be seen, so for non-mipmapped modes we want to ignore everything
 * but BaseLevel.
 */
static void
intel_update_max_level(struct intel_texture_object *intelObj,
		       struct gl_sampler_object *sampler)
{
   struct gl_texture_object *tObj = &intelObj->base;
   int maxlevel;

   if (sampler->MinFilter == GL_NEAREST ||
       sampler->MinFilter == GL_LINEAR) {
      maxlevel = tObj->BaseLevel;
   } else {
      maxlevel = tObj->_MaxLevel;
   }

   if (intelObj->_MaxLevel != maxlevel) {
      intelObj->_MaxLevel = maxlevel;
      intelObj->needs_validate = true;
   }
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
   int width, height, depth;

   /* TBOs require no validation -- they always just point to their BO. */
   if (tObj->Target == GL_TEXTURE_BUFFER)
      return true;

   /* We know/require this is true by now: 
    */
   assert(intelObj->base._BaseComplete);

   /* What levels must the tree include at a minimum?
    */
   intel_update_max_level(intelObj, sampler);
   if (intelObj->mt && intelObj->mt->first_level != tObj->BaseLevel)
      intelObj->needs_validate = true;

   if (!intelObj->needs_validate)
      return true;

   firstImage = intel_texture_image(tObj->Image[0][tObj->BaseLevel]);

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    *
    * For pre-gen4, we have to match first_level == tObj->BaseLevel,
    * because we don't have the control that gen4 does to make min/mag
    * determination happen at a nonzero (hardware) baselevel.  Because
    * of that, we just always relayout on baselevel change.
    */
   if (intelObj->mt &&
       (!intel_miptree_match_image(intelObj->mt, &firstImage->base.Base) ||
	intelObj->mt->first_level != tObj->BaseLevel ||
	intelObj->mt->last_level < intelObj->_MaxLevel)) {
      intel_miptree_release(&intelObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intel_miptree_get_dimensions_for_image(&firstImage->base.Base,
					     &width, &height, &depth);

      perf_debug("Creating new %s %dx%dx%d %d..%d miptree to handle finalized "
                 "texture miptree.\n",
                 _mesa_get_format_name(firstImage->base.Base.TexFormat),
                 width, height, depth, tObj->BaseLevel, intelObj->_MaxLevel);

      intelObj->mt = intel_miptree_create(intel,
                                          intelObj->base.Target,
					  firstImage->base.Base.TexFormat,
                                          tObj->BaseLevel,
                                          intelObj->_MaxLevel,
                                          width,
                                          height,
                                          depth,
					  true,
                                          0 /* num_samples */,
                                          false /* force_y_tiling */);
      if (!intelObj->mt)
         return false;
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = _mesa_num_tex_faces(intelObj->base.Target);
   for (face = 0; face < nr_faces; face++) {
      for (i = tObj->BaseLevel; i <= intelObj->_MaxLevel; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);
	 /* skip too small size mipmap */
 	 if (intelImage == NULL)
		 break;

         if (intelObj->mt != intelImage->mt) {
            intel_miptree_copy_teximage(intel, intelImage, intelObj->mt,
                                        false /* invalidate */);
         }

         /* After we're done, we'd better agree that our layout is
          * appropriate, or we'll end up hitting this function again on the
          * next draw
          */
         assert(intel_miptree_match_image(intelObj->mt, &intelImage->base.Base));
      }
   }

   intelObj->needs_validate = false;

   return true;
}

/**
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 */
static void
intel_tex_map_image_for_swrast(struct intel_context *intel,
			       struct intel_texture_image *intel_image,
			       GLbitfield mode)
{
   int level;
   int face;
   struct intel_mipmap_tree *mt;
   unsigned int x, y;

   if (!intel_image || !intel_image->mt)
      return;

   level = intel_image->base.Base.Level;
   face = intel_image->base.Base.Face;
   mt = intel_image->mt;

   for (int i = 0; i < mt->level[level].depth; i++)
      intel_miptree_slice_resolve_depth(intel, mt, level, i);

   if (mt->target == GL_TEXTURE_3D ||
       mt->target == GL_TEXTURE_2D_ARRAY ||
       mt->target == GL_TEXTURE_1D_ARRAY) {
      int i;

      /* ImageOffsets[] is only used for swrast's fetch_texel_3d, so we can't
       * share code with the normal path.
       */
      for (i = 0; i < mt->level[level].depth; i++) {
	 intel_miptree_get_image_offset(mt, level, i, &x, &y);
	 intel_image->base.ImageOffsets[i] = x + y * (mt->region->pitch /
                                                      mt->region->cpp);
      }

      DBG("%s \n", __FUNCTION__);

      intel_image->base.Map = intel_miptree_map_raw(intel, mt);
   } else {
      assert(intel_image->base.Base.Depth == 1);
      intel_miptree_get_image_offset(mt, level, face, &x, &y);

      DBG("%s: (%d,%d) -> (%d, %d)/%d\n",
	  __FUNCTION__, face, level, x, y, mt->region->pitch);

      intel_image->base.Map = intel_miptree_map_raw(intel, mt) +
	 x * mt->cpp + y * mt->region->pitch;
   }

   assert(mt->region->pitch % mt->region->cpp == 0);
   intel_image->base.RowStride = mt->region->pitch / mt->region->cpp;
}

static void
intel_tex_unmap_image_for_swrast(struct intel_context *intel,
				 struct intel_texture_image *intel_image)
{
   if (intel_image && intel_image->mt) {
      intel_miptree_unmap_raw(intel, intel_image->mt);
      intel_image->base.Map = NULL;
   }
}

/**
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 */
void
intel_tex_map_images(struct intel_context *intel,
		     struct intel_texture_object *intelObj,
		     GLbitfield mode)
{
   GLuint nr_faces = _mesa_num_tex_faces(intelObj->base.Target);
   int i, face;

   DBG("%s\n", __FUNCTION__);

   for (i = intelObj->base.BaseLevel; i <= intelObj->_MaxLevel; i++) {
      for (face = 0; face < nr_faces; face++) {
	 struct intel_texture_image *intel_image =
	    intel_texture_image(intelObj->base.Image[face][i]);

	 intel_tex_map_image_for_swrast(intel, intel_image, mode);
      }
   }
}

void
intel_tex_unmap_images(struct intel_context *intel,
		       struct intel_texture_object *intelObj)
{
   GLuint nr_faces = _mesa_num_tex_faces(intelObj->base.Target);
   int i, face;

   for (i = intelObj->base.BaseLevel; i <= intelObj->_MaxLevel; i++) {
      for (face = 0; face < nr_faces; face++) {
	 struct intel_texture_image *intel_image =
	    intel_texture_image(intelObj->base.Image[face][i]);

	 intel_tex_unmap_image_for_swrast(intel, intel_image);
      }
   }
}
