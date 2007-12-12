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

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 *   - some logic for moving the buffers to the best memory pools for
 *     given operations.
 *
 * Most of this is to make it easier to implement the fixed-layout
 * mipmap tree required by intel hardware in the face of GL's
 * programming interface where each image can be specifed in random
 * order and it isn't clear what layout the tree should have until the
 * last moment.
 */

#include "intel_context.h"
#include "intel_regions.h"
#include "intel_blit.h"
#include "dri_bufmgr.h"
#include "intel_bufmgr_ttm.h"
#include "imports.h"

#define FILE_DEBUG_FLAG DEBUG_REGION

/* XXX: Thread safety?
 */
GLubyte *intel_region_map(struct intel_context *intel, struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!region->map_refcount++) {
      dri_bo_map(region->buffer, GL_TRUE);
      region->map = region->buffer->virtual;
   }

   return region->map;
}

void intel_region_unmap(struct intel_context *intel, 
			struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!--region->map_refcount) {
      dri_bo_unmap(region->buffer);
      region->map = NULL;
   }
}

struct intel_region *intel_region_alloc( struct intel_context *intel, 
					 GLuint cpp,
					 GLuint pitch, 
					 GLuint height )
{
   struct intel_region *region = calloc(sizeof(*region), 1);

   DBG("%s %dx%dx%d == 0x%x bytes\n", __FUNCTION__,
       cpp, pitch, height, cpp*pitch*height);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height; 	/* needed? */
   region->refcount = 1;

   region->buffer = dri_bo_alloc(intel->bufmgr, "region",
				 pitch * cpp * height, 64, DRM_BO_FLAG_MEM_TT);

   return region;
}

void intel_region_reference( struct intel_region **dst,
			     struct intel_region *src)
{
   src->refcount++;
   assert(*dst == NULL);
   *dst = src;
}

void intel_region_release( struct intel_context *intel,
			   struct intel_region **region )
{
   if (!*region)
      return;

   DBG("%s %d\n", __FUNCTION__, (*region)->refcount-1);
   
   if (--(*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);
      dri_bo_unreference((*region)->buffer);
      free(*region);
   }
   *region = NULL;
}

void _mesa_copy_rect( GLubyte *dst,
		      GLuint cpp,
		      GLuint dst_pitch,
		      GLuint dst_x, 
		      GLuint dst_y,
		      GLuint width,
		      GLuint height,
		      const GLubyte *src,
		      GLuint src_pitch,
		      GLuint src_x,
		      GLuint src_y )
{
   GLuint i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && 
       width == src_pitch)
      do_memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
	 do_memcpy(dst, src, width);
	 dst += dst_pitch;
	 src += src_pitch;
      }
   }
}


/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
GLboolean intel_region_data(struct intel_context *intel, 
			    struct intel_region *dst,
			    GLuint dst_offset,
			    GLuint dstx, GLuint dsty,
			    const void *src, GLuint src_pitch,
			    GLuint srcx, GLuint srcy,
			    GLuint width, GLuint height)
{
   DBG("%s\n", __FUNCTION__);

   assert (dst_offset + dstx + width +
	   (dsty + height - 1) * dst->pitch * dst->cpp <=
	   dst->pitch * dst->cpp * dst->height);

   _mesa_copy_rect(intel_region_map(intel, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);
   intel_region_unmap(intel, dst);

   return GL_TRUE;
}
			  
/* Copy rectangular sub-regions. Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void intel_region_copy( struct intel_context *intel,
			struct intel_region *dst,
			GLuint dst_offset,
			GLuint dstx, GLuint dsty,
			struct intel_region *src,
			GLuint src_offset,
			GLuint srcx, GLuint srcy,
			GLuint width, GLuint height )
{
   DBG("%s\n", __FUNCTION__);

   assert(src->cpp == dst->cpp);

   intelEmitCopyBlit(intel,
		     dst->cpp,
		     src->pitch, src->buffer, src_offset, src->tiled,
		     dst->pitch, dst->buffer, dst_offset, dst->tiled,
		     srcx, srcy,
		     dstx, dsty,
		     width, height,
		     GL_COPY );
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void intel_region_fill( struct intel_context *intel,
			struct intel_region *dst,
			GLuint dst_offset,
			GLuint dstx, GLuint dsty,
			GLuint width, GLuint height,
			GLuint color )
{
   DBG("%s\n", __FUNCTION__);
   
   intelEmitFillBlit(intel,
		     dst->cpp,
		     dst->pitch, dst->buffer, dst_offset, dst->tiled,
		     dstx, dsty,
		     width, height,
		     color );
}

static struct intel_region *
intel_recreate_static(struct intel_context *intel,
		      const char *name,
		      struct intel_region *region,
		      intelRegion *region_desc,
		      GLuint mem_type)
{
   intelScreenPrivate *intelScreen = intel->intelScreen;

   if (region == NULL) {
      region = calloc(sizeof(*region), 1);
      region->refcount = 1;
   }

   region->cpp = intelScreen->cpp;
   region->pitch = region_desc->pitch / intelScreen->cpp;
   region->height = intelScreen->height;     /* needed? */
   region->tiled = region_desc->tiled;

   if (intel->ttm) {
      assert(region_desc->bo_handle != -1);
      region->buffer = intel_ttm_bo_create_from_handle(intel->bufmgr,
						       name,
						       region_desc->bo_handle);
   } else {
      region->buffer = dri_bo_alloc_static(intel->bufmgr,
					   name,
					   region_desc->offset,
					   region_desc->pitch *
					   intelScreen->height,
					   region_desc->map,
					   DRM_BO_FLAG_MEM_TT);
   }

   assert(region->buffer != NULL);

   return region;
}

/**
 * Create intel_region structs to describe the static front, back, and depth
 * buffers created by the xserver.
 *
 * Although FBO's mean we now no longer use these as render targets in
 * all circumstances, they won't go away until the back and depth
 * buffers become private, and the front buffer will remain even then.
 *
 * Note that these don't allocate video memory, just describe
 * allocations alread made by the X server.
 */
void
intel_recreate_static_regions(struct intel_context *intel)
{
   intelScreenPrivate *intelScreen = intel->intelScreen;

   intel->front_region =
      intel_recreate_static(intel, "front",
			    intel->front_region,
			    &intelScreen->front,
			    DRM_BO_FLAG_MEM_TT);

   intel->back_region =
      intel_recreate_static(intel, "back",
			    intel->back_region,
			    &intelScreen->back,
			    DRM_BO_FLAG_MEM_TT);

   /* Still assumes front.cpp == depth.cpp.  We can kill this when we move to
    * private buffers.
    */
   intel->depth_region =
      intel_recreate_static(intel, "depth",
			    intel->depth_region,
			    &intelScreen->depth,
			    DRM_BO_FLAG_MEM_TT);
}
