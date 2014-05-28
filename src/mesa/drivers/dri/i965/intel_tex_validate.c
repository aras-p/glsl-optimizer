/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "main/mtypes.h"
#include "main/macros.h"
#include "main/samplerobj.h"
#include "main/texobj.h"

#include "brw_context.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * Sets our driver-specific variant of tObj->_MaxLevel for later surface state
 * upload.
 *
 * If we're only ensuring that there is storage for the first miplevel of a
 * texture, then in texture setup we're going to have to make sure we don't
 * allow sampling beyond level 0.
 */
static void
intel_update_max_level(struct intel_texture_object *intelObj,
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
 * At rendering-from-a-texture time, make sure that the texture object has a
 * miptree that can hold the entire texture based on
 * BaseLevel/MaxLevel/filtering, and copy in any texture images that are
 * stored in other miptrees.
 */
GLuint
intel_finalize_mipmap_tree(struct brw_context *brw, GLuint unit)
{
   struct gl_context *ctx = &brw->ctx;
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   GLuint face, i;
   GLuint nr_faces = 0;
   struct intel_texture_image *firstImage;
   int width, height, depth;

   /* TBOs require no validation -- they always just point to their BO. */
   if (tObj->Target == GL_TEXTURE_BUFFER)
      return true;

   /* We know that this is true by now, and if it wasn't, we might have
    * mismatched level sizes and the copies would fail.
    */
   assert(intelObj->base._BaseComplete);

   intel_update_max_level(intelObj, sampler);

   /* What levels does this validated texture image require? */
   int validate_first_level = tObj->BaseLevel;
   int validate_last_level = intelObj->_MaxLevel;

   /* Skip the loop over images in the common case of no images having
    * changed.  But if the GL_BASE_LEVEL or GL_MAX_LEVEL change to something we
    * haven't looked at, then we do need to look at those new images.
    */
   if (!intelObj->needs_validate &&
       validate_first_level >= intelObj->validated_first_level &&
       validate_last_level <= intelObj->validated_last_level) {
      return true;
   }

   /* Immutable textures should not get this far -- they should have been
    * created in a validated state, and nothing can invalidate them.
    */
   assert(!tObj->Immutable);

   firstImage = intel_texture_image(tObj->Image[0][tObj->BaseLevel]);

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    */
   if (intelObj->mt &&
       (!intel_miptree_match_image(intelObj->mt, &firstImage->base.Base) ||
	validate_first_level < intelObj->mt->first_level ||
	validate_last_level > intelObj->mt->last_level)) {
      intel_miptree_release(&intelObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intel_miptree_get_dimensions_for_image(&firstImage->base.Base,
					     &width, &height, &depth);

      perf_debug("Creating new %s %dx%dx%d %d-level miptree to handle "
                 "finalized texture miptree.\n",
                 _mesa_get_format_name(firstImage->base.Base.TexFormat),
                 width, height, depth, validate_last_level + 1);

      intelObj->mt = intel_miptree_create(brw,
                                          intelObj->base.Target,
					  firstImage->base.Base.TexFormat,
                                          0, /* first_level */
                                          validate_last_level,
                                          width,
                                          height,
                                          depth,
					  true,
                                          0 /* num_samples */,
                                          INTEL_MIPTREE_TILING_ANY,
                                          false);
      if (!intelObj->mt)
         return false;
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = _mesa_num_tex_faces(intelObj->base.Target);
   for (face = 0; face < nr_faces; face++) {
      for (i = validate_first_level; i <= validate_last_level; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);
	 /* skip too small size mipmap */
 	 if (intelImage == NULL)
		 break;

         if (intelObj->mt != intelImage->mt) {
            intel_miptree_copy_teximage(brw, intelImage, intelObj->mt,
                                        false /* invalidate */);
         }

         /* After we're done, we'd better agree that our layout is
          * appropriate, or we'll end up hitting this function again on the
          * next draw
          */
         assert(intel_miptree_match_image(intelObj->mt, &intelImage->base.Base));
      }
   }

   intelObj->validated_first_level = validate_first_level;
   intelObj->validated_last_level = validate_last_level;
   intelObj->_Format = intelObj->mt->format;
   intelObj->needs_validate = false;

   return true;
}
