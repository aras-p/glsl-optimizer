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

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_resolve_map.h"
#include "intel_span.h"
#include "intel_tex_layout.h"
#include "intel_tex.h"
#include "intel_blit.h"

#include "main/enums.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/teximage.h"

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
			      gl_format format,
			      GLuint first_level,
			      GLuint last_level,
			      GLuint width0,
			      GLuint height0,
			      GLuint depth0)
{
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);
   int compress_byte = 0;

   DBG("%s target %s format %s level %d..%d <-- %p\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_get_format_name(format),
       first_level, last_level, mt);

   if (_mesa_is_format_compressed(format))
      compress_byte = intel_compressed_num_bytes(format);

   mt->target = target_to_target(target);
   mt->format = format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->cpp = compress_byte ? compress_byte : _mesa_get_format_bytes(mt->format);
   mt->compressed = compress_byte ? 1 : 0;
   mt->refcount = 1; 

   intel_get_texture_alignment_unit(intel, format,
				    &mt->align_w, &mt->align_h);

   if (target == GL_TEXTURE_CUBE_MAP) {
      assert(depth0 == 1);
      mt->depth0 = 6;
   } else {
      mt->depth0 = depth0;
   }

   if (format == MESA_FORMAT_S8) {
      /* The stencil buffer has quirky pitch requirements.  From Vol 2a,
       * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
       *    The pitch must be set to 2x the value computed based on width, as
       *    the stencil buffer is stored with two rows interleaved.
       */
      assert(intel->has_separate_stencil);
      mt->cpp = 2;
   }

#ifdef I915
   (void) intel;
   if (intel->is_945)
      i945_miptree_layout(mt);
   else
      i915_miptree_layout(mt);
#else
   brw_miptree_layout(intel, mt);
#endif

   if (_mesa_is_depthstencil_format(_mesa_get_format_base_format(format)) &&
       (intel->must_use_separate_stencil ||
	(intel->has_separate_stencil &&
	 intel->vtbl.is_hiz_depth_format(intel, format)))) {
      mt->stencil_mt = intel_miptree_create(intel,
                                            mt->target,
                                            MESA_FORMAT_S8,
                                            mt->first_level,
                                            mt->last_level,
                                            mt->width0,
                                            mt->height0,
                                            mt->depth0,
                                            true);
      if (!mt->stencil_mt) {
	 intel_miptree_release(&mt);
	 return NULL;
      }
   }

   return mt;
}


struct intel_mipmap_tree *
intel_miptree_create(struct intel_context *intel,
		     GLenum target,
		     gl_format format,
		     GLuint first_level,
		     GLuint last_level,
		     GLuint width0,
		     GLuint height0,
		     GLuint depth0,
		     bool expect_accelerated_upload)
{
   struct intel_mipmap_tree *mt;
   uint32_t tiling = I915_TILING_NONE;
   GLenum base_format = _mesa_get_format_base_format(format);

   if (intel->use_texture_tiling && !_mesa_is_format_compressed(format)) {
      if (intel->gen >= 4 &&
	  (base_format == GL_DEPTH_COMPONENT ||
	   base_format == GL_DEPTH_STENCIL_EXT))
	 tiling = I915_TILING_Y;
      else if (format == MESA_FORMAT_S8)
	 tiling = I915_TILING_NONE;
      else if (width0 >= 64)
	 tiling = I915_TILING_X;
   }

   mt = intel_miptree_create_internal(intel, target, format,
				      first_level, last_level, width0,
				      height0, depth0);
   /*
    * pitch == 0 || height == 0  indicates the null texture
    */
   if (!mt || !mt->total_width || !mt->total_height) {
      free(mt);
      return NULL;
   }

   mt->region = intel_region_alloc(intel->intelScreen,
				   tiling,
				   mt->cpp,
				   mt->total_width,
				   mt->total_height,
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
				gl_format format,
				struct intel_region *region)
{
   struct intel_mipmap_tree *mt;

   mt = intel_miptree_create_internal(intel, target, format,
				      0, 0,
				      region->width, region->height, 1);
   if (!mt)
      return mt;

   intel_region_reference(&mt->region, region);

   return mt;
}

struct intel_mipmap_tree*
intel_miptree_create_for_renderbuffer(struct intel_context *intel,
                                      gl_format format,
                                      uint32_t tiling,
                                      uint32_t cpp,
                                      uint32_t width,
                                      uint32_t height)
{
   struct intel_region *region;
   struct intel_mipmap_tree *mt;

   region = intel_region_alloc(intel->intelScreen,
                               tiling, cpp, width, height, true);
   if (!region)
      return NULL;

   mt = intel_miptree_create_for_region(intel, GL_TEXTURE_2D, format, region);
   intel_region_release(&region);
   return mt;
}

void
intel_miptree_reference(struct intel_mipmap_tree **dst,
                        struct intel_mipmap_tree *src)
{
   if (*dst == src)
      return;

   intel_miptree_release(dst);

   if (src) {
      src->refcount++;
      DBG("%s %p refcount now %d\n", __FUNCTION__, src, src->refcount);
   }

   *dst = src;
}


void
intel_miptree_release(struct intel_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n", __FUNCTION__, *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __FUNCTION__, *mt);

      intel_region_release(&((*mt)->region));
      intel_miptree_release(&(*mt)->stencil_mt);
      intel_miptree_release(&(*mt)->hiz_mt);
      intel_resolve_map_clear(&(*mt)->hiz_map);

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 free((*mt)->level[i].slice);
      }

      free(*mt);
   }
   *mt = NULL;
}

void
intel_miptree_get_dimensions_for_image(struct gl_texture_image *image,
                                       int *width, int *height, int *depth)
{
   switch (image->TexObject->Target) {
   case GL_TEXTURE_1D_ARRAY:
      *width = image->Width;
      *height = 1;
      *depth = image->Height;
      break;
   default:
      *width = image->Width;
      *height = image->Height;
      *depth = image->Depth;
      break;
   }
}

/**
 * Can the image be pulled into a unified mipmap tree?  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
bool
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   GLuint level = intelImage->base.Base.Level;
   int width, height, depth;

   if (image->TexFormat != mt->format)
      return false;

   intel_miptree_get_dimensions_for_image(image, &width, &height, &depth);

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (width != mt->level[level].width ||
       height != mt->level[level].height ||
       depth != mt->level[level].depth)
      return false;

   return true;
}


void
intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
			     GLuint level,
			     GLuint x, GLuint y,
			     GLuint w, GLuint h, GLuint d)
{
   mt->level[level].width = w;
   mt->level[level].height = h;
   mt->level[level].depth = d;
   mt->level[level].level_x = x;
   mt->level[level].level_y = y;

   DBG("%s level %d size: %d,%d,%d offset %d,%d\n", __FUNCTION__,
       level, w, h, d, x, y);

   assert(mt->level[level].slice == NULL);

   mt->level[level].slice = malloc(d * sizeof(*mt->level[0].slice));
   mt->level[level].slice[0].x_offset = mt->level[level].level_x;
   mt->level[level].slice[0].y_offset = mt->level[level].level_y;
}


void
intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint img,
			       GLuint x, GLuint y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].depth);

   mt->level[level].slice[img].x_offset = mt->level[level].level_x + x;
   mt->level[level].slice[img].y_offset = mt->level[level].level_y + y;

   DBG("%s level %d img %d pos %d,%d\n",
       __FUNCTION__, level, img,
       mt->level[level].slice[img].x_offset,
       mt->level[level].slice[img].y_offset);
}


/**
 * For cube map textures, either the \c face parameter can be used, of course,
 * or the cube face can be interpreted as a depth layer and the \c layer
 * parameter used.
 */
void
intel_miptree_get_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint face, GLuint layer,
			       GLuint *x, GLuint *y)
{
   int slice;

   if (face > 0) {
      assert(mt->target == GL_TEXTURE_CUBE_MAP);
      assert(face < 6);
      assert(layer == 0);
      slice = face;
   } else {
      /* This branch may be taken even if the texture target is a cube map. In
       * that case, the caller chose to interpret each cube face as a layer.
       */
      assert(face == 0);
      slice = layer;
   }

   *x = mt->level[level].slice[slice].x_offset;
   *y = mt->level[level].slice[slice].y_offset;
}

static void
intel_miptree_copy_slice(struct intel_context *intel,
			 struct intel_mipmap_tree *dst_mt,
			 struct intel_mipmap_tree *src_mt,
			 int level,
			 int face,
			 int depth)

{
   gl_format format = src_mt->format;
   uint32_t width = src_mt->level[level].width;
   uint32_t height = src_mt->level[level].height;

   assert(depth < src_mt->level[level].depth);

   if (dst_mt->compressed) {
      height = ALIGN(height, dst_mt->align_h) / dst_mt->align_h;
      width = ALIGN(width, dst_mt->align_w);
   }

   uint32_t dst_x, dst_y, src_x, src_y;
   intel_miptree_get_image_offset(dst_mt, level, face, depth,
				  &dst_x, &dst_y);
   intel_miptree_get_image_offset(src_mt, level, face, depth,
				  &src_x, &src_y);

   DBG("validate blit mt %p %d,%d/%d -> mt %p %d,%d/%d (%dx%d)\n",
       src_mt, src_x, src_y, src_mt->region->pitch * src_mt->region->cpp,
       dst_mt, dst_x, dst_y, dst_mt->region->pitch * dst_mt->region->cpp,
       width, height);

   if (!intelEmitCopyBlit(intel,
			  dst_mt->region->cpp,
			  src_mt->region->pitch, src_mt->region->bo,
			  0, src_mt->region->tiling,
			  dst_mt->region->pitch, dst_mt->region->bo,
			  0, dst_mt->region->tiling,
			  src_x, src_y,
			  dst_x, dst_y,
			  width, height,
			  GL_COPY)) {

      fallback_debug("miptree validate blit for %s failed\n",
		     _mesa_get_format_name(format));
      void *dst = intel_region_map(intel, dst_mt->region, GL_MAP_WRITE_BIT);
      void *src = intel_region_map(intel, src_mt->region, GL_MAP_READ_BIT);

      _mesa_copy_rect(dst,
		      dst_mt->cpp,
		      dst_mt->region->pitch,
		      dst_x, dst_y,
		      width, height,
		      src, src_mt->region->pitch,
		      src_x, src_y);

      intel_region_unmap(intel, dst_mt->region);
      intel_region_unmap(intel, src_mt->region);
   }

   if (src_mt->stencil_mt) {
      intel_miptree_copy_slice(intel,
                               dst_mt->stencil_mt, src_mt->stencil_mt,
                               level, face, depth);
   }
}

/**
 * Copies the image's current data to the given miptree, and associates that
 * miptree with the image.
 */
void
intel_miptree_copy_teximage(struct intel_context *intel,
			    struct intel_texture_image *intelImage,
			    struct intel_mipmap_tree *dst_mt)
{
   struct intel_mipmap_tree *src_mt = intelImage->mt;
   int level = intelImage->base.Base.Level;
   int face = intelImage->base.Base.Face;
   GLuint depth = intelImage->base.Base.Depth;

   for (int slice = 0; slice < depth; slice++) {
      intel_miptree_copy_slice(intel, dst_mt, src_mt, level, face, slice);
   }

   intel_miptree_reference(&intelImage->mt, dst_mt);
}

/**
 * \param scatter Scatter if true. Gather if false.
 *
 * \see intel_miptree_s8z24_scatter()
 * \see intel_miptree_s8z24_gather()
 */
static void
intel_miptree_s8z24_scattergather(struct intel_context *intel,
                                  struct intel_mipmap_tree *mt,
                                  uint32_t level,
                                  uint32_t layer,
                                  bool scatter)
{
   /* Check function inputs. */
   assert(level >= mt->first_level);
   assert(level <= mt->last_level);
   assert(layer < mt->level[level].depth);

   /* Label everything and its bit layout, just to make the code easier to
    * read.
    */
   struct intel_mipmap_tree  *s8_mt    = mt->stencil_mt;
   struct intel_mipmap_level *s8_level = &s8_mt->level[level];
   struct intel_mipmap_slice *s8_slice = &s8_mt->level[level].slice[layer];

   struct intel_mipmap_tree  *s8z24_mt    = mt;
   struct intel_mipmap_level *s8z24_level = &s8z24_mt->level[level];
   struct intel_mipmap_slice *s8z24_slice = &s8z24_mt->level[level].slice[layer];

   /* Check that both miptree levels have the same dimensions. */
   assert(s8_level->width  == s8z24_level->width);
   assert(s8_level->height == s8z24_level->height);
   assert(s8_level->depth  == s8z24_level->depth);

   /* Map the buffers. */
   if (drm_intel_bo_references(intel->batch.bo, s8_mt->region->bo) ||
       drm_intel_bo_references(intel->batch.bo, s8z24_mt->region->bo)) {
      intel_batchbuffer_flush(intel);
   }
   drm_intel_gem_bo_map_gtt(s8_mt->region->bo);
   drm_intel_gem_bo_map_gtt(s8z24_mt->region->bo);

   /* Define the invariant values outside the for loop, because I don't trust
    * GCC to do it for us.
    */
   uint8_t *s8_map = s8_mt->region->bo->virtual
		   + s8_slice->x_offset
		   + s8_slice->y_offset;

   uint8_t *s8z24_map = s8z24_mt->region->bo->virtual
		      + s8z24_slice->x_offset
		      + s8z24_slice->y_offset;

   ptrdiff_t s8z24_stride = s8z24_mt->region->pitch * s8z24_mt->region->cpp;

   uint32_t w = s8_level->width;
   uint32_t h = s8_level->height;

   for (uint32_t y = 0; y < h; ++y) {
      for (uint32_t x = 0; x < w; ++x) {
	 ptrdiff_t s8_offset = intel_offset_S8(s8_mt->region->pitch, x, y);
	 ptrdiff_t s8z24_offset = y * s8z24_stride
				+ x * 4
				+ 3;
	 if (scatter) {
	    s8_map[s8_offset] = s8z24_map[s8z24_offset];
	 } else {
	    s8z24_map[s8z24_offset] = s8_map[s8_offset];
	 }
      }
   }

   drm_intel_gem_bo_unmap_gtt(s8_mt->region->bo);
   drm_intel_gem_bo_unmap_gtt(s8z24_mt->region->bo);
}

void
intel_miptree_s8z24_scatter(struct intel_context *intel,
                            struct intel_mipmap_tree *mt,
                            uint32_t level,
                            uint32_t layer)
{
   intel_miptree_s8z24_scattergather(intel, mt, level, layer, true);
}

void
intel_miptree_s8z24_gather(struct intel_context *intel,
                           struct intel_mipmap_tree *mt,
                           uint32_t level,
                           uint32_t layer)
{
   intel_miptree_s8z24_scattergather(intel, mt, level, layer, false);
}

bool
intel_miptree_alloc_hiz(struct intel_context *intel,
			struct intel_mipmap_tree *mt)
{
   assert(mt->hiz_mt == NULL);
   mt->hiz_mt = intel_miptree_create(intel,
                                     mt->target,
                                     MESA_FORMAT_X8_Z24,
                                     mt->first_level,
                                     mt->last_level,
                                     mt->width0,
                                     mt->height0,
                                     mt->depth0,
                                     true);

   if (!mt->hiz_mt)
      return false;

   /* Mark that all slices need a HiZ resolve. */
   struct intel_resolve_map *head = &mt->hiz_map;
   for (int level = mt->first_level; level <= mt->last_level; ++level) {
      for (int layer = 0; layer < mt->level[level].depth; ++layer) {
	 head->next = malloc(sizeof(*head->next));
	 head->next->prev = head;
	 head->next->next = NULL;
	 head = head->next;

	 head->level = level;
	 head->layer = layer;
	 head->need = INTEL_NEED_HIZ_RESOLVE;
      }
   }

   return true;
}

void
intel_miptree_slice_set_needs_hiz_resolve(struct intel_mipmap_tree *mt,
					  uint32_t level,
					  uint32_t layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   if (!mt->hiz_mt)
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, INTEL_NEED_HIZ_RESOLVE);
}


void
intel_miptree_slice_set_needs_depth_resolve(struct intel_mipmap_tree *mt,
                                            uint32_t level,
                                            uint32_t layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   if (!mt->hiz_mt)
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, INTEL_NEED_DEPTH_RESOLVE);
}

typedef void (*resolve_func_t)(struct intel_context *intel,
			       struct intel_mipmap_tree *mt,
			       uint32_t level,
			       uint32_t layer);

static bool
intel_miptree_slice_resolve(struct intel_context *intel,
			    struct intel_mipmap_tree *mt,
			    uint32_t level,
			    uint32_t layer,
			    enum intel_need_resolve need,
			    resolve_func_t func)
{
   intel_miptree_check_level_layer(mt, level, layer);

   struct intel_resolve_map *item =
	 intel_resolve_map_get(&mt->hiz_map, level, layer);

   if (!item || item->need != need)
      return false;

   func(intel, mt, level, layer);
   intel_resolve_map_remove(item);
   return true;
}

bool
intel_miptree_slice_resolve_hiz(struct intel_context *intel,
				struct intel_mipmap_tree *mt,
				uint32_t level,
				uint32_t layer)
{
   return intel_miptree_slice_resolve(intel, mt, level, layer,
				      INTEL_NEED_HIZ_RESOLVE,
				      intel->vtbl.resolve_hiz_slice);
}

bool
intel_miptree_slice_resolve_depth(struct intel_context *intel,
				  struct intel_mipmap_tree *mt,
				  uint32_t level,
				  uint32_t layer)
{
   return intel_miptree_slice_resolve(intel, mt, level, layer,
				      INTEL_NEED_DEPTH_RESOLVE,
				      intel->vtbl.resolve_depth_slice);
}

static bool
intel_miptree_all_slices_resolve(struct intel_context *intel,
				 struct intel_mipmap_tree *mt,
				 enum intel_need_resolve need,
				 resolve_func_t func)
{
   bool did_resolve = false;
   struct intel_resolve_map *i;

   for (i = mt->hiz_map.next; i; i = i->next) {
      if (i->need != need)
	 continue;
      func(intel, mt, i->level, i->layer);
      intel_resolve_map_remove(i);
      did_resolve = true;
   }

   return did_resolve;
}

bool
intel_miptree_all_slices_resolve_hiz(struct intel_context *intel,
				     struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(intel, mt,
					   INTEL_NEED_HIZ_RESOLVE,
					   intel->vtbl.resolve_hiz_slice);
}

bool
intel_miptree_all_slices_resolve_depth(struct intel_context *intel,
				       struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(intel, mt,
					   INTEL_NEED_DEPTH_RESOLVE,
					   intel->vtbl.resolve_depth_slice);
}
