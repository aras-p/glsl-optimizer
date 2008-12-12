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
#include "intel_chipset.h"
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
			      GLuint depth0, GLuint cpp, GLuint compress_byte)
{
   GLboolean ok;
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);

   DBG("%s target %s format %s level %d..%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_lookup_enum_by_nr(internal_format), first_level, last_level);

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
   if (IS_945(intel->intelScreen->deviceID))
      ok = i945_miptree_layout(intel, mt);
   else
      ok = i915_miptree_layout(intel, mt);
#else
   ok = brw_miptree_layout(intel, mt);
#endif

   if (!ok) {
      free(mt);
      return NULL;
   }

   return mt;
}

struct intel_mipmap_tree *
intel_miptree_create(struct intel_context *intel,
		     GLenum target,
		     GLenum internal_format,
		     GLuint first_level,
		     GLuint last_level,
		     GLuint width0,
		     GLuint height0,
		     GLuint depth0, GLuint cpp, GLuint compress_byte)
{
   struct intel_mipmap_tree *mt;

   mt = intel_miptree_create_internal(intel, target, internal_format,
				      first_level, last_level, width0,
				      height0, depth0, cpp, compress_byte);
   /*
    * pitch == 0 || height == 0  indicates the null texture
    */
   if (!mt || !mt->pitch || !mt->total_height)
      return NULL;

   mt->region = intel_region_alloc(intel,
				   mt->cpp,
				   mt->pitch,
				   mt->total_height,
				   mt->pitch);

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
				      region->cpp, compress_byte);
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
			       int pitch)
{
#ifdef I915
   GLcontext *ctx = &intel->ctx;
#endif

   if (!mt->compressed) {
      int pitch_align;

      if (intel->ttm) {
	 /* XXX: Align pitch to multiple of 64 bytes for now to allow
	  * render-to-texture to work in all cases. This should probably be
	  * replaced at some point by some scheme to only do this when really
	  * necessary.
	  */
	 pitch_align = 64;
      } else {
	 pitch_align = 4;
      }

      pitch = ALIGN(pitch * mt->cpp, pitch_align);

#ifdef I915
      /* XXX: At least the i915 seems very upset when the pitch is a multiple
       * of 1024 and sometimes 512 bytes - performance can drop by several
       * times. Go to the next multiple of the required alignment for now.
       */
      if (!(pitch & 511) && 
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

      intel_region_release(&((*mt)->region));

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
         if ((*mt)->level[i].image_offset)
            free((*mt)->level[i].image_offset);

      free(*mt);
   }
   *mt = NULL;
}




/* Can the image be pulled into a unified mipmap tree.  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
GLboolean
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image,
                          GLuint face, GLuint level)
{
   /* Images with borders are never pulled into mipmap trees. 
    */
   if (image->Border ||
       ((image->_BaseFormat == GL_DEPTH_COMPONENT) &&
        ((image->TexObject->WrapS == GL_CLAMP_TO_BORDER) ||
         (image->TexObject->WrapT == GL_CLAMP_TO_BORDER)))) 
      return GL_FALSE;

   if (image->InternalFormat != mt->internal_format ||
       image->IsCompressed != mt->compressed)
      return GL_FALSE;

   if (!image->IsCompressed &&
       !mt->compressed &&
       image->TexFormat->TexelBytes != mt->cpp)
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
   mt->level[level].level_offset = (x + y * mt->pitch) * mt->cpp;
   mt->level[level].nr_images = nr_images;

   DBG("%s level %d size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
       level, w, h, d, x, y, mt->level[level].level_offset);

   /* Not sure when this would happen, but anyway: 
    */
   if (mt->level[level].image_offset) {
      free(mt->level[level].image_offset);
      mt->level[level].image_offset = NULL;
   }

   assert(nr_images);

   mt->level[level].image_offset = malloc(nr_images * sizeof(GLuint));
   mt->level[level].image_offset[0] = 0;
}



void
intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint img,
			       GLuint x, GLuint y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].nr_images);

   mt->level[level].image_offset[img] = (x + y * mt->pitch) * mt->cpp;

   DBG("%s level %d img %d pos %d,%d image_offset %x\n",
       __FUNCTION__, level, img, x, y, mt->level[level].image_offset[img]);
}


/* Although we use the image_offset[] array to store relative offsets
 * to cube faces, Mesa doesn't know anything about this and expects
 * each cube face to be treated as a separate image.
 *
 * These functions present that view to mesa:
 */
const GLuint *
intel_miptree_depth_offsets(struct intel_mipmap_tree *mt, GLuint level)
{
   static const GLuint zero = 0;

   if (mt->target != GL_TEXTURE_3D || mt->level[level].nr_images == 1)
      return &zero;
   else
      return mt->level[level].image_offset;
}


GLuint
intel_miptree_image_offset(struct intel_mipmap_tree *mt,
			   GLuint face, GLuint level)
{
   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB)
      return (mt->level[level].level_offset +
	      mt->level[level].image_offset[face]);
   else
      return mt->level[level].level_offset;
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
   DBG("%s \n", __FUNCTION__);

   if (row_stride)
      *row_stride = mt->pitch * mt->cpp;

   if (mt->target == GL_TEXTURE_3D) {
      int i;

      for (i = 0; i < mt->level[level].depth; i++)
	 image_offsets[i] = mt->level[level].image_offset[i] / mt->cpp;
   } else {
      assert(mt->level[level].depth == 1);
      assert(mt->target == GL_TEXTURE_CUBE_MAP ||
	     mt->level[level].image_offset[0] == 0);
      image_offsets[0] = 0;
   }

   return (intel_region_map(intel, mt->region) +
           intel_miptree_image_offset(mt, face, level));
}

void
intel_miptree_image_unmap(struct intel_context *intel,
                          struct intel_mipmap_tree *mt)
{
   DBG("%s\n", __FUNCTION__);
   intel_region_unmap(intel, mt->region);
}



/* Upload data for a particular image.
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
   GLuint depth = dst->level[level].depth;
   GLuint dst_offset = intel_miptree_image_offset(dst, face, level);
   const GLuint *dst_depth_offset = intel_miptree_depth_offsets(dst, level);
   GLuint i;
   GLuint height = 0;

   DBG("%s: %d/%d\n", __FUNCTION__, face, level);
   for (i = 0; i < depth; i++) {
      height = dst->level[level].height;
      if(dst->compressed)
	 height = (height + 3) / 4;
      intel_region_data(intel,
			dst->region,
			dst_offset + dst_depth_offset[i], /* dst_offset */
			0, 0,                             /* dstx, dsty */
			src,
			src_row_pitch,
			0, 0,                             /* source x, y */
			dst->level[level].width, height); /* width, height */

      src += src_image_pitch * dst->cpp;
   }
}

extern GLuint intel_compressed_alignment(GLenum);
/* Copy mipmap image between trees
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
   GLuint dst_offset = intel_miptree_image_offset(dst, face, level);
   GLuint src_offset = intel_miptree_image_offset(src, face, level);
   const GLuint *dst_depth_offset = intel_miptree_depth_offsets(dst, level);
   const GLuint *src_depth_offset = intel_miptree_depth_offsets(src, level);
   GLuint i;

   if (dst->compressed) {
       GLuint alignment = intel_compressed_alignment(dst->internal_format);
       height = (height + 3) / 4;
       width = ((width + alignment - 1) & ~(alignment - 1));
   }

   for (i = 0; i < depth; i++) {
      intel_region_copy(intel,
                        dst->region, dst_offset + dst_depth_offset[i],
                        0,
                        0,
                        src->region, src_offset + src_depth_offset[i],
                        0, 0, width, height);
   }

}
