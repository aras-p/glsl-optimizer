/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tex_layout.h"
#ifndef I915
#include "brw_state.h"
#endif
#include "main/enums.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE


static GLenum
target_to_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return GL_TEXTURE_CUBE_MAP_ARB;
   default:
      return target;
   }
}


static struct intel_mipmap_tree *
intel_miptree_create_internal(struct intel_context *intel,
			      GLenum target,
			      GLenum internal_format,
			      GLuint first_level,
			      GLuint last_level,
			      GLuint width0,
			      GLuint height0,
			      GLuint depth0, GLuint cpp, GLuint compress_byte,
			      uint32_t tiling)
{
   GLboolean ok;
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);

   DBG("%s target %s format %s level %d..%d <-- %p\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_lookup_enum_by_nr(internal_format), 
       first_level, last_level, mt);

   mt->target = target_to_target(target);
   mt->internal_format = internal_format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->depth0 = depth0;
   mt->cpp = compress_byte ? compress_byte : cpp;
   mt->compressed = compress_byte ? 1 : 0;
   mt->refcount = 1; 
   mt->pitch = 0;

#ifdef I915
   if (intel->is_945)
      ok = i945_miptree_layout(intel, mt, tiling);
   else
      ok = i915_miptree_layout(intel, mt, tiling);
#else
   ok = brw_miptree_layout(intel, mt, tiling);
#endif

   if (!ok) {
      free(mt);
      DBG("%s not okay - returning NULL\n", __FUNCTION__);
      return NULL;
   }

   return mt;
}


struct intel_mipmap_tree *
intel_miptree_create(struct intel_context *intel,
		     GLenum target,
		     GLenum base_format,
		     GLenum internal_format,
		     GLuint first_level,
		     GLuint last_level,
		     GLuint width0,
		     GLuint height0,
		     GLuint depth0, GLuint cpp, GLuint compress_byte,
		     GLboolean expect_accelerated_upload)
{
   struct intel_mipmap_tree *mt;
   uint32_t tiling;

   if (intel->use_texture_tiling && compress_byte == 0) {
      if (intel->gen >= 4 &&
	  (base_format == GL_DEPTH_COMPONENT ||
	   base_format == GL_DEPTH_STENCIL_EXT))
	 tiling = I915_TILING_Y;
      else
	 tiling = I915_TILING_X;
   } else
      tiling = I915_TILING_NONE;

   mt = intel_miptree_create_internal(intel, target, internal_format,
				      first_level, last_level, width0,
				      height0, depth0, cpp, compress_byte,
				      tiling);
   /*
    * pitch == 0 || height == 0  indicates the null texture
    */
   if (!mt || !mt->pitch || !mt->total_height) {
      free(mt);
      return NULL;
   }

   mt->region = intel_region_alloc(intel,
				   tiling,
				   mt->cpp,
				   mt->pitch,
				   mt->total_height,
				   mt->pitch,
				   expect_accelerated_upload);

   if (!mt->region) {
       free(mt);
       return NULL;
   }

   return mt;
}


struct intel_mipmap_tree *
intel_miptree_create_for_region(struct intel_context *intel,
				GLenum target,
				GLenum internal_format,
				GLuint first_level,
				GLuint last_level,
				struct intel_region *region,
				GLuint depth0,
				GLuint compress_byte)
{
   struct intel_mipmap_tree *mt;

   mt = intel_miptree_create_internal(intel, target, internal_format,
				      first_level, last_level,
				      region->width, region->height, 1,
				      region->cpp, compress_byte,
				      I915_TILING_NONE);
   if (!mt)
      return mt;
#if 0
   if (mt->pitch != region->pitch) {
      fprintf(stderr,
	      "region pitch (%d) doesn't match mipmap tree pitch (%d)\n",
	      region->pitch, mt->pitch);
      free(mt);
      return NULL;
   }
#else
   /* The mipmap tree pitch is aligned to 64 bytes to make sure render
    * to texture works, but we don't need that for texturing from a
    * pixmap.  Just override it here. */
   mt->pitch = region->pitch;
#endif

   intel_region_reference(&mt->region, region);

   return mt;
}


/**
 * intel_miptree_pitch_align:
 *
 * @intel: intel context pointer
 *
 * @mt: the miptree to compute pitch alignment for
 *
 * @pitch: the natural pitch value
 *
 * Given @pitch, compute a larger value which accounts for
 * any necessary alignment required by the device
 */
int intel_miptree_pitch_align (struct intel_context *intel,
			       struct intel_mipmap_tree *mt,
			       uint32_t tiling,
			       int pitch)
{
#ifdef I915
   GLcontext *ctx = &intel->ctx;
#endif

   if (!mt->compressed) {
      int pitch_align;

      /* XXX: Align pitch to multiple of 64 bytes for now to allow
       * render-to-texture to work in all cases. This should probably be
       * replaced at some point by some scheme to only do this when really
       * necessary.
       */
      pitch_align = 64;

      if (tiling == I915_TILING_X)
	 pitch_align = 512;
      else if (tiling == I915_TILING_Y)
	 pitch_align = 128;

      pitch = ALIGN(pitch * mt->cpp, pitch_align);

#ifdef I915
      /* Do a little adjustment to linear allocations so that we avoid
       * hitting the same channel of memory for 2 different pages when
       * reading a 2x2 subspan or doing bilinear filtering.
       */
      if (tiling == I915_TILING_NONE && !(pitch & 511) &&
	 (pitch + pitch_align) < (1 << ctx->Const.MaxTextureLevels))
	 pitch += pitch_align;
#endif

      pitch /= mt->cpp;
   }
   return pitch;
}


void
intel_miptree_reference(struct intel_mipmap_tree **dst,
                        struct intel_mipmap_tree *src)
{
   src->refcount++;
   *dst = src;
   DBG("%s %p refcount now %d\n", __FUNCTION__, src, src->refcount);
}


void
intel_miptree_release(struct intel_context *intel,
                      struct intel_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n", __FUNCTION__, *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __FUNCTION__, *mt);

#ifndef I915
      /* Free up cached binding tables holding a reference on our buffer, to
       * avoid excessive memory consumption.
       *
       * This isn't as aggressive as we could be, as we'd like to do
       * it from any time we free the last ref on a region.  But intel_region.c
       * is context-agnostic.  Perhaps our constant state cache should be, as
       * well.
       */
      brw_state_cache_bo_delete(&brw_context(&intel->ctx)->surface_cache,
				(*mt)->region->buffer);
#endif

      intel_region_release(&((*mt)->region));

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 free((*mt)->level[i].x_offset);
	 free((*mt)->level[i].y_offset);
      }

      free(*mt);
   }
   *mt = NULL;
}


/**
 * Can the image be pulled into a unified mipmap tree?  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
GLboolean
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image)
{
   GLboolean isCompressed = _mesa_is_format_compressed(image->TexFormat);
   struct intel_texture_image *intelImage = intel_texture_image(image);
   GLuint level = intelImage->level;

   /* Images with borders are never pulled into mipmap trees. */
   if (image->Border)
      return GL_FALSE;

   if (image->InternalFormat != mt->internal_format ||
       isCompressed != mt->compressed)
      return GL_FALSE;

   if (!isCompressed &&
       !mt->compressed &&
       _mesa_get_format_bytes(image->TexFormat) != mt->cpp)
      return GL_FALSE;

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (image->Width != mt->level[level].width ||
       image->Height != mt->level[level].height ||
       image->Depth != mt->level[level].depth)
      return GL_FALSE;

   return GL_TRUE;
}


void
intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
			     GLuint level,
			     GLuint nr_images,
			     GLuint x, GLuint y,
			     GLuint w, GLuint h, GLuint d)
{
   mt->level[level].width = w;
   mt->level[level].height = h;
   mt->level[level].depth = d;
   mt->level[level].level_x = x;
   mt->level[level].level_y = y;
   mt->level[level].nr_images = nr_images;

   DBG("%s level %d size: %d,%d,%d offset %d,%d\n", __FUNCTION__,
       level, w, h, d, x, y);

   assert(nr_images);
   assert(!mt->level[level].x_offset);

   mt->level[level].x_offset = malloc(nr_images * sizeof(GLuint));
   mt->level[level].x_offset[0] = mt->level[level].level_x;
   mt->level[level].y_offset = malloc(nr_images * sizeof(GLuint));
   mt->level[level].y_offset[0] = mt->level[level].level_y;
}


void
intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint img,
			       GLuint x, GLuint y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].nr_images);

   mt->level[level].x_offset[img] = mt->level[level].level_x + x;
   mt->level[level].y_offset[img] = mt->level[level].level_y + y;

   DBG("%s level %d img %d pos %d,%d\n",
       __FUNCTION__, level, img,
       mt->level[level].x_offset[img], mt->level[level].y_offset[img]);
}


void
intel_miptree_get_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint face, GLuint depth,
			       GLuint *x, GLuint *y)
{
   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB) {
      *x = mt->level[level].x_offset[face];
      *y = mt->level[level].y_offset[face];
   } else if (mt->target == GL_TEXTURE_3D) {
      *x = mt->level[level].x_offset[depth];
      *y = mt->level[level].y_offset[depth];
   } else {
      *x = mt->level[level].x_offset[0];
      *y = mt->level[level].y_offset[0];
   }
}

/**
 * Map a teximage in a mipmap tree.
 * \param row_stride  returns row stride in bytes
 * \param image_stride  returns image stride in bytes (for 3D textures).
 * \param image_offsets pointer to array of pixel offsets from the returned
 *	  pointer to each depth image
 * \return address of mapping
 */
GLubyte *
intel_miptree_image_map(struct intel_context * intel,
                        struct intel_mipmap_tree * mt,
                        GLuint face,
                        GLuint level,
                        GLuint * row_stride, GLuint * image_offsets)
{
   GLuint x, y;
   DBG("%s \n", __FUNCTION__);

   if (row_stride)
      *row_stride = mt->pitch * mt->cpp;

   if (mt->target == GL_TEXTURE_3D) {
      int i;

      for (i = 0; i < mt->level[level].depth; i++) {

	 intel_miptree_get_image_offset(mt, level, face, i,
					&x, &y);
	 image_offsets[i] = x + y * mt->pitch;
      }

      return intel_region_map(intel, mt->region);
   } else {
      assert(mt->level[level].depth == 1);
      intel_miptree_get_image_offset(mt, level, face, 0,
				     &x, &y);
      image_offsets[0] = 0;

      return intel_region_map(intel, mt->region) +
	 (x + y * mt->pitch) * mt->cpp;
   }
}


void
intel_miptree_image_unmap(struct intel_context *intel,
                          struct intel_mipmap_tree *mt)
{
   DBG("%s\n", __FUNCTION__);
   intel_region_unmap(intel, mt->region);
}


/**
 * Upload data for a particular image.
 */
void
intel_miptree_image_data(struct intel_context *intel,
			 struct intel_mipmap_tree *dst,
			 GLuint face,
			 GLuint level,
			 void *src,
			 GLuint src_row_pitch,
			 GLuint src_image_pitch)
{
   const GLuint depth = dst->level[level].depth;
   GLuint i;

   DBG("%s: %d/%d\n", __FUNCTION__, face, level);
   for (i = 0; i < depth; i++) {
      GLuint dst_x, dst_y, height;

      intel_miptree_get_image_offset(dst, level, face, i, &dst_x, &dst_y);

      height = dst->level[level].height;
      if(dst->compressed)
	 height = (height + 3) / 4;

      intel_region_data(intel,
			dst->region, 0, dst_x, dst_y,
			src,
			src_row_pitch,
			0, 0,                             /* source x, y */
			dst->level[level].width, height); /* width, height */

      src = (char *)src + src_image_pitch * dst->cpp;
   }
}


/**
 * Copy mipmap image between trees
 */
void
intel_miptree_image_copy(struct intel_context *intel,
                         struct intel_mipmap_tree *dst,
                         GLuint face, GLuint level,
                         struct intel_mipmap_tree *src)
{
   GLuint width = src->level[level].width;
   GLuint height = src->level[level].height;
   GLuint depth = src->level[level].depth;
   GLuint src_x, src_y, dst_x, dst_y;
   GLuint i;
   GLboolean success;

   if (dst->compressed) {
       GLuint align_w, align_h;

       intel_get_texture_alignment_unit(dst->internal_format,
                                        &align_w, &align_h);
       height = (height + 3) / 4;
       width = ALIGN(width, align_w);
   }

   for (i = 0; i < depth; i++) {
      intel_miptree_get_image_offset(src, level, face, i, &src_x, &src_y);
      intel_miptree_get_image_offset(dst, level, face, i, &dst_x, &dst_y);
      success = intel_region_copy(intel,
				  dst->region, 0, dst_x, dst_y,
				  src->region, 0, src_x, src_y,
				  width, height, GL_FALSE,
				  GL_COPY);
      if (!success) {
	 GLubyte *src_ptr, *dst_ptr;

	 src_ptr = intel_region_map(intel, src->region);
	 dst_ptr = intel_region_map(intel, dst->region);

	 _mesa_copy_rect(dst_ptr + dst->cpp * (dst_x + dst_y * dst->pitch),
			 dst->cpp,
			 dst->pitch,
			 0, 0, width, height,
			 src_ptr + src->cpp * (src_x + src_y * src->pitch),
			 src->pitch,
			 0, 0);
	 intel_region_unmap(intel, src->region);
	 intel_region_unmap(intel, dst->region);
      }
   }
}
