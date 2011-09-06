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
#include "intel_tex.h"
#include "intel_blit.h"
#include "main/enums.h"
#include "main/formats.h"
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
   mt->depth0 = depth0;
   mt->cpp = compress_byte ? compress_byte : _mesa_get_format_bytes(mt->format);
   mt->compressed = compress_byte ? 1 : 0;
   mt->refcount = 1; 

#ifdef I915
   (void) intel;
   if (intel->is_945)
      i945_miptree_layout(mt);
   else
      i915_miptree_layout(mt);
#else
   brw_miptree_layout(intel, mt);
#endif

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
		     GLboolean expect_accelerated_upload)
{
   struct intel_mipmap_tree *mt;
   uint32_t tiling = I915_TILING_NONE;
   GLenum base_format = _mesa_get_format_base_format(format);

   if (intel->use_texture_tiling && !_mesa_is_format_compressed(format)) {
      if (intel->gen >= 4 &&
	  (base_format == GL_DEPTH_COMPONENT ||
	   base_format == GL_DEPTH_STENCIL_EXT))
	 tiling = I915_TILING_Y;
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
      intel_region_release(&((*mt)->hiz_region));

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 free((*mt)->level[i].x_offset);
	 free((*mt)->level[i].y_offset);
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
GLboolean
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   GLuint level = intelImage->base.Base.Level;
   int width, height, depth;

   /* Images with borders are never pulled into mipmap trees. */
   if (image->Border)
      return GL_FALSE;

   if (image->TexFormat != mt->format)
      return GL_FALSE;

   intel_miptree_get_dimensions_for_image(image, &width, &height, &depth);

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (width != mt->level[level].width ||
       height != mt->level[level].height ||
       depth != mt->level[level].depth)
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
   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP_ARB:
      *x = mt->level[level].x_offset[face];
      *y = mt->level[level].y_offset[face];
      break;
   case GL_TEXTURE_3D:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_1D_ARRAY_EXT:
      assert(depth < mt->level[level].nr_images);
      *x = mt->level[level].x_offset[depth];
      *y = mt->level[level].y_offset[depth];
      break;
   default:
      *x = mt->level[level].x_offset[0];
      *y = mt->level[level].y_offset[0];
      break;
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
   GLuint width = src_mt->level[level].width;
   GLuint height = src_mt->level[level].height;
   GLuint depth = src_mt->level[level].depth;
   int slice;
   void *src, *dst;

   if (dst_mt->compressed) {
      unsigned int align_w, align_h;

      intel_get_texture_alignment_unit(intelImage->base.Base.TexFormat,
				       &align_w, &align_h);
      height = ALIGN(height, align_h) / align_h;
      width = ALIGN(width, align_w);
   }

   for (slice = 0; slice < depth; slice++) {
      unsigned int dst_x, dst_y, src_x, src_y;

      intel_miptree_get_image_offset(dst_mt, level, face, slice,
				     &dst_x, &dst_y);

      if (src_mt) {
	 /* Copy potentially with the blitter:
	  */
	 intel_miptree_get_image_offset(src_mt, level, face, slice,
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
			   _mesa_get_format_name(intelImage->base.Base.TexFormat));
	    dst = intel_region_map(intel, dst_mt->region);
	    src = intel_region_map(intel, src_mt->region);

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
      } else {
	 dst = intel_region_map(intel, dst_mt->region);

	 DBG("validate upload mt %p -> mt %p %d,%d/%d (%dx%d)\n",
	     src,
	     dst_mt, dst_x, dst_y, dst_mt->region->pitch * dst_mt->region->cpp,
	     width, height);

	 src = intelImage->base.Base.Data;
	 src += (intelImage->base.Base.RowStride *
		 intelImage->base.Base.Height *
		 dst_mt->region->cpp *
		 slice);

	 _mesa_copy_rect(dst,
			 dst_mt->region->cpp,
			 dst_mt->region->pitch,
			 dst_x, dst_y,
			 width, height,
			 src,
			 intelImage->base.Base.RowStride,
			 0, 0);

	 intel_region_unmap(intel, dst_mt->region);
      }
   }

   if (!src_mt) {
      _mesa_free_texmemory(intelImage->base.Base.Data);
      intelImage->base.Base.Data = NULL;
   }

   intel_miptree_reference(&intelImage->mt, dst_mt);
}
