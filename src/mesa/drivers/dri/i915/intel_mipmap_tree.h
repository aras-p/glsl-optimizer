/**************************************************************************
 * 
 * Copyright 2006 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTEL_MIPMAP_TREE_H
#define INTEL_MIPMAP_TREE_H

#include <assert.h>

#include "intel_screen.h"
#include "intel_regions.h"
#include "GL/internal/dri_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A layer on top of the intel_regions code which adds:
 *
 * - Code to size and layout a region to hold a set of mipmaps.
 * - Query to determine if a new image fits in an existing tree.
 * - More refcounting 
 *     - maybe able to remove refcounting from intel_region?
 * - ?
 *
 * The fixed mipmap layout of intel hardware where one offset
 * specifies the position of all images in a mipmap hierachy
 * complicates the implementation of GL texture image commands,
 * compared to hardware where each image is specified with an
 * independent offset.
 *
 * In an ideal world, each texture object would be associated with a
 * single bufmgr buffer or 2d intel_region, and all the images within
 * the texture object would slot into the tree as they arrive.  The
 * reality can be a little messier, as images can arrive from the user
 * with sizes that don't fit in the existing tree, or in an order
 * where the tree layout cannot be guessed immediately.  
 * 
 * This structure encodes an idealized mipmap tree.  The GL image
 * commands build these where possible, otherwise store the images in
 * temporary system buffers.
 */

struct intel_texture_image;

struct intel_miptree_map {
   /** Bitfield of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_INVALIDATE_BIT */
   GLbitfield mode;
   /** Region of interest for the map. */
   int x, y, w, h;
   /** Possibly malloced temporary buffer for the mapping. */
   void *buffer;
   /** Possible pointer to a temporary linear miptree for the mapping. */
   struct intel_mipmap_tree *mt;
   /** Pointer to the start of (map_x, map_y) returned by the mapping. */
   void *ptr;
   /** Stride of the mapping. */
   int stride;
};

/**
 * Describes the location of each texture image within a texture region.
 */
struct intel_mipmap_level
{
   /** Offset to this miptree level, used in computing x_offset. */
   GLuint level_x;
   /** Offset to this miptree level, used in computing y_offset. */
   GLuint level_y;
   GLuint width;
   GLuint height;

   /**
    * \brief Number of 2D slices in this miplevel.
    *
    * The exact semantics of depth varies according to the texture target:
    *    - For GL_TEXTURE_CUBE_MAP, depth is 6.
    *    - For GL_TEXTURE_2D_ARRAY, depth is the number of array slices. It is
    *      identical for all miplevels in the texture.
    *    - For GL_TEXTURE_3D, it is the texture's depth at this miplevel. Its
    *      value, like width and height, varies with miplevel.
    *    - For other texture types, depth is 1.
    */
   GLuint depth;

   /**
    * \brief List of 2D images in this mipmap level.
    *
    * This may be a list of cube faces, array slices in 2D array texture, or
    * layers in a 3D texture. The list's length is \c depth.
    */
   struct intel_mipmap_slice {
      /**
       * \name Offset to slice
       * \{
       *
       * Hardware formats are so diverse that that there is no unified way to
       * compute the slice offsets, so we store them in this table.
       *
       * The (x, y) offset to slice \c s at level \c l relative the miptrees
       * base address is
       * \code
       *     x = mt->level[l].slice[s].x_offset
       *     y = mt->level[l].slice[s].y_offset
       */
      GLuint x_offset;
      GLuint y_offset;
      /** \} */

      /**
       * Mapping information. Persistent for the duration of
       * intel_miptree_map/unmap on this slice.
       */
      struct intel_miptree_map *map;
   } *slice;
};


struct intel_mipmap_tree
{
   /* Effectively the key:
    */
   GLenum target;

   /**
    * This is just the same as the gl_texture_image->TexFormat or
    * gl_renderbuffer->Format.
    */
   mesa_format format;

   /**
    * The X offset of each image in the miptree must be aligned to this. See
    * the "Alignment Unit Size" section of the BSpec.
    */
   unsigned int align_w;
   unsigned int align_h; /**< \see align_w */

   GLuint first_level;
   GLuint last_level;

   /**
    * Level zero image dimensions.  These dimensions correspond to the
    * physical layout of data in memory.  Accordingly, they account for the
    * extra width, height, and or depth that must be allocated in order to
    * accommodate multisample formats, and they account for the extra factor
    * of 6 in depth that must be allocated in order to accommodate cubemap
    * textures.
    */
   GLuint physical_width0, physical_height0, physical_depth0;

   GLuint cpp;
   bool compressed;

   /**
    * Level zero image dimensions.  These dimensions correspond to the
    * logical width, height, and depth of the region as seen by client code.
    * Accordingly, they do not account for the extra width, height, and/or
    * depth that must be allocated in order to accommodate multisample
    * formats, nor do they account for the extra factor of 6 in depth that
    * must be allocated in order to accommodate cubemap textures.
    */
   uint32_t logical_width0, logical_height0, logical_depth0;

   /**
    * For 1D array, 2D array, cube, and 2D multisampled surfaces on Gen7: true
    * if the surface only contains LOD 0, and hence no space is for LOD's
    * other than 0 in between array slices.
    *
    * Corresponds to the surface_array_spacing bit in gen7_surface_state.
    */
   bool array_spacing_lod0;

   /* Derived from the above:
    */
   GLuint total_width;
   GLuint total_height;

   /* Includes image offset tables:
    */
   struct intel_mipmap_level level[MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct intel_region *region;

   /* Offset into region bo where miptree starts:
    */
   uint32_t offset;

   /* These are also refcounted:
    */
   GLuint refcount;
};

enum intel_miptree_tiling_mode {
   INTEL_MIPTREE_TILING_ANY,
   INTEL_MIPTREE_TILING_Y,
   INTEL_MIPTREE_TILING_NONE,
};

struct intel_mipmap_tree *intel_miptree_create(struct intel_context *intel,
                                               GLenum target,
					       mesa_format format,
                                               GLuint first_level,
                                               GLuint last_level,
                                               GLuint width0,
                                               GLuint height0,
                                               GLuint depth0,
					       bool expect_accelerated_upload,
                                               enum intel_miptree_tiling_mode);

struct intel_mipmap_tree *
intel_miptree_create_layout(struct intel_context *intel,
                            GLenum target,
                            mesa_format format,
                            GLuint first_level,
                            GLuint last_level,
                            GLuint width0,
                            GLuint height0,
                            GLuint depth0,
                            bool for_bo);

struct intel_mipmap_tree *
intel_miptree_create_for_bo(struct intel_context *intel,
                            drm_intel_bo *bo,
                            mesa_format format,
                            uint32_t offset,
                            uint32_t width,
                            uint32_t height,
                            int pitch,
                            uint32_t tiling);

struct intel_mipmap_tree*
intel_miptree_create_for_dri2_buffer(struct intel_context *intel,
                                     unsigned dri_attachment,
                                     mesa_format format,
                                     struct intel_region *region);

struct intel_mipmap_tree*
intel_miptree_create_for_image_buffer(struct intel_context *intel,
                                      enum __DRIimageBufferMask buffer_type,
                                      mesa_format format,
                                      uint32_t num_samples,
                                      struct intel_region *region);

/**
 * Create a miptree appropriate as the storage for a non-texture renderbuffer.
 * The miptree has the following properties:
 *     - The target is GL_TEXTURE_2D.
 *     - There are no levels other than the base level 0.
 *     - Depth is 1.
 */
struct intel_mipmap_tree*
intel_miptree_create_for_renderbuffer(struct intel_context *intel,
                                      mesa_format format,
                                      uint32_t width,
                                      uint32_t height);

/** \brief Assert that the level and layer are valid for the miptree. */
static inline void
intel_miptree_check_level_layer(struct intel_mipmap_tree *mt,
                                uint32_t level,
                                uint32_t layer)
{
   assert(level >= mt->first_level);
   assert(level <= mt->last_level);
   assert(layer < mt->level[level].depth);
}

int intel_miptree_pitch_align (struct intel_context *intel,
			       struct intel_mipmap_tree *mt,
			       uint32_t tiling,
			       int pitch);

void intel_miptree_reference(struct intel_mipmap_tree **dst,
                             struct intel_mipmap_tree *src);

void intel_miptree_release(struct intel_mipmap_tree **mt);

/* Check if an image fits an existing mipmap tree layout
 */
bool intel_miptree_match_image(struct intel_mipmap_tree *mt,
                                    struct gl_texture_image *image);

void
intel_miptree_get_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint slice,
			       GLuint *x, GLuint *y);

void
intel_miptree_get_dimensions_for_image(struct gl_texture_image *image,
                                       int *width, int *height, int *depth);

uint32_t
intel_miptree_get_tile_offsets(struct intel_mipmap_tree *mt,
                               GLuint level, GLuint slice,
                               uint32_t *tile_x,
                               uint32_t *tile_y);

void intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
                                  GLuint level,
                                  GLuint x, GLuint y,
                                  GLuint w, GLuint h, GLuint d);

void intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
                                    GLuint level,
                                    GLuint img, GLuint x, GLuint y);

void
intel_miptree_copy_teximage(struct intel_context *intel,
                            struct intel_texture_image *intelImage,
                            struct intel_mipmap_tree *dst_mt, bool invalidate);

/**\}*/

/* i915_mipmap_tree.c:
 */
void i915_miptree_layout(struct intel_mipmap_tree *mt);
void i945_miptree_layout(struct intel_mipmap_tree *mt);
void brw_miptree_layout(struct intel_context *intel,
			struct intel_mipmap_tree *mt);

void *intel_miptree_map_raw(struct intel_context *intel,
                            struct intel_mipmap_tree *mt);

void intel_miptree_unmap_raw(struct intel_context *intel,
                             struct intel_mipmap_tree *mt);

void
intel_miptree_map(struct intel_context *intel,
		  struct intel_mipmap_tree *mt,
		  unsigned int level,
		  unsigned int slice,
		  unsigned int x,
		  unsigned int y,
		  unsigned int w,
		  unsigned int h,
		  GLbitfield mode,
		  void **out_ptr,
		  int *out_stride);

void
intel_miptree_unmap(struct intel_context *intel,
		    struct intel_mipmap_tree *mt,
		    unsigned int level,
		    unsigned int slice);


#ifdef __cplusplus
}
#endif

#endif
