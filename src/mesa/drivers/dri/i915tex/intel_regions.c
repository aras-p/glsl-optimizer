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
#include "intel_buffer_objects.h"
#include "dri_bufmgr.h"
#include "intel_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_REGION

void
intel_region_idle(intelScreenPrivate *intelScreen, struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   /* XXX: Using this function is likely bogus -- it ought to only have been
    * used before a map, anyway, but leave this cheap implementation of it
    * for now.
    */
   if (region && region->buffer) {
      /* Mapping it for read will ensure that any acceleration to the region
       * would have landed already.
       */
      dri_bo_map(region->buffer, GL_TRUE);
      dri_bo_unmap(region->buffer);
   }
}

/* XXX: Thread safety?
 */
GLubyte *
intel_region_map(intelScreenPrivate *intelScreen, struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!region->map_refcount++) {
      if (region->pbo)
         intel_region_cow(intelScreen, region);

      dri_bo_map(region->buffer, GL_TRUE);
      region->map = region->buffer->virtual;
   }

   return region->map;
}

void
intel_region_unmap(intelScreenPrivate *intelScreen, struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!--region->map_refcount) {
      dri_bo_unmap(region->buffer);
      region->map = NULL;
   }
}

struct intel_region *
intel_region_alloc(intelScreenPrivate *intelScreen,
                   GLuint cpp, GLuint pitch, GLuint height)
{
   struct intel_region *region = calloc(sizeof(*region), 1);

   DBG("%s\n", __FUNCTION__);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;     /* needed? */
   region->refcount = 1;

   region->buffer = dri_bo_alloc(intelScreen->bufmgr, "region",
				 pitch * cpp * height, 64, 0, 0);
   return region;
}

void
intel_region_reference(struct intel_region **dst, struct intel_region *src)
{
   assert(*dst == NULL);
   if (src) {
      src->refcount++;
      *dst = src;
   }
}

void
intel_region_release(struct intel_region **region)
{
   if (!*region)
      return;

   DBG("%s %d\n", __FUNCTION__, (*region)->refcount - 1);

   ASSERT((*region)->refcount > 0);
   (*region)->refcount--;

   if ((*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);

      if ((*region)->pbo)
	 (*region)->pbo->region = NULL;
      (*region)->pbo = NULL;
      dri_bo_unreference((*region)->buffer);
      free(*region);
   }
   *region = NULL;
}


struct intel_region *
intel_region_create_static(intelScreenPrivate *intelScreen,
                           GLuint mem_type,
                           GLuint offset,
                           void *virtual,
                           GLuint cpp, GLuint pitch, GLuint height)
{
   struct intel_region *region = calloc(sizeof(*region), 1);
   DBG("%s\n", __FUNCTION__);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;     /* needed? */
   region->refcount = 1;

   /* XXX: questionable flags */
   region->buffer = dri_bo_alloc_static(intelScreen->bufmgr, "static region",
					offset, pitch * cpp * height, virtual,
					DRM_BO_FLAG_MEM_TT |
					DRM_BO_FLAG_NO_MOVE |
					DRM_BO_FLAG_READ |
					DRM_BO_FLAG_WRITE, 0);

   return region;
}



void
intel_region_update_static(intelScreenPrivate *intelScreen,
			   struct intel_region *region,
                           GLuint mem_type,
                           GLuint offset,
                           void *virtual,
                           GLuint cpp, GLuint pitch, GLuint height)
{
   DBG("%s\n", __FUNCTION__);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;     /* needed? */

   /*
    * We use a "shared" buffer type to indicate buffers created and
    * shared by others.
    */

   dri_bo_unreference(region->buffer);
   /* XXX: questionable flags */
   region->buffer = dri_bo_alloc_static(intelScreen->bufmgr, "static region",
					offset, pitch * cpp * height, virtual,
					DRM_BO_FLAG_MEM_TT |
					DRM_BO_FLAG_NO_MOVE |
					DRM_BO_FLAG_READ |
					DRM_BO_FLAG_WRITE, 0);
}



/*
 * XXX Move this into core Mesa?
 */
static void
_mesa_copy_rect(GLubyte * dst,
                GLuint cpp,
                GLuint dst_pitch,
                GLuint dst_x,
                GLuint dst_y,
                GLuint width,
                GLuint height,
                const GLubyte * src,
                GLuint src_pitch, GLuint src_x, GLuint src_y)
{
   GLuint i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && width == src_pitch)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
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
void
intel_region_data(intelScreenPrivate *intelScreen,
                  struct intel_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  const void *src, GLuint src_pitch,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;

   if (dst->pbo) {
      if (dstx == 0 &&
          dsty == 0 && width == dst->pitch && height == dst->height)
         intel_region_release_pbo(intelScreen, dst);
      else
         intel_region_cow(intelScreen, dst);
   }


   LOCK_HARDWARE(intel);

   _mesa_copy_rect(intel_region_map(intelScreen, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   intel_region_unmap(intelScreen, dst);

   UNLOCK_HARDWARE(intel);

}

/* Copy rectangular sub-regions. Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void
intel_region_copy(intelScreenPrivate *intelScreen,
                  struct intel_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  struct intel_region *src,
                  GLuint src_offset,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;

   if (dst->pbo) {
      if (dstx == 0 &&
          dsty == 0 && width == dst->pitch && height == dst->height)
         intel_region_release_pbo(intelScreen, dst);
      else
         intel_region_cow(intelScreen, dst);
   }

   assert(src->cpp == dst->cpp);

   intelEmitCopyBlit(intel,
                     dst->cpp,
                     src->pitch, src->buffer, src_offset,
                     dst->pitch, dst->buffer, dst_offset,
                     srcx, srcy, dstx, dsty, width, height,
		     GL_COPY);
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void
intel_region_fill(intelScreenPrivate *intelScreen,
                  struct intel_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  GLuint width, GLuint height, GLuint color)
{
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;   

   if (dst->pbo) {
      if (dstx == 0 &&
          dsty == 0 && width == dst->pitch && height == dst->height)
         intel_region_release_pbo(intelScreen, dst);
      else
         intel_region_cow(intelScreen, dst);
   }

   intelEmitFillBlit(intel,
                     dst->cpp,
                     dst->pitch, dst->buffer, dst_offset,
                     dstx, dsty, width, height, color);
}

/* Attach to a pbo, discarding our data.  Effectively zero-copy upload
 * the pbo's data.
 */
void
intel_region_attach_pbo(intelScreenPrivate *intelScreen,
                        struct intel_region *region,
                        struct intel_buffer_object *pbo)
{
   if (region->pbo == pbo)
      return;

   /* If there is already a pbo attached, break the cow tie now.
    * Don't call intel_region_release_pbo() as that would
    * unnecessarily allocate a new buffer we would have to immediately
    * discard.
    */
   if (region->pbo) {
      region->pbo->region = NULL;
      region->pbo = NULL;
   }

   if (region->buffer) {
      dri_bo_unreference(region->buffer);
      region->buffer = NULL;
   }

   region->pbo = pbo;
   region->pbo->region = region;
   dri_bo_reference(pbo->buffer);
   region->buffer = pbo->buffer;
}


/* Break the COW tie to the pbo and allocate a new buffer.
 * The pbo gets to keep the data.
 */
void
intel_region_release_pbo(intelScreenPrivate *intelScreen,
                         struct intel_region *region)
{
   assert(region->buffer == region->pbo->buffer);
   region->pbo->region = NULL;
   region->pbo = NULL;
   dri_bo_unreference(region->buffer);
   region->buffer = NULL;

   region->buffer = dri_bo_alloc(intelScreen->bufmgr, "region",
				 region->pitch * region->cpp * region->height,
				 64, 0, 0);
}

/* Break the COW tie to the pbo.  Both the pbo and the region end up
 * with a copy of the data.
 */
void
intel_region_cow(intelScreenPrivate *intelScreen, struct intel_region *region)
{
   struct intel_context *intel = intelScreenContext(intelScreen);
   struct intel_buffer_object *pbo = region->pbo;

   if (intel == NULL)
      return;

   intel_region_release_pbo(intelScreen, region);

   assert(region->cpp * region->pitch * region->height == pbo->Base.Size);

   DBG("%s (%d bytes)\n", __FUNCTION__, pbo->Base.Size);

   /* Now blit from the texture buffer to the new buffer: 
    */

   intel_batchbuffer_flush(intel->batch);

   if (!intel->locked) {
      LOCK_HARDWARE(intel);
      intelEmitCopyBlit(intel,
			region->cpp,
			region->pitch,
			region->buffer, 0,
			region->pitch,
			pbo->buffer, 0,
			0, 0, 0, 0, 
			region->pitch, region->height,
			GL_COPY);
      
      intel_batchbuffer_flush(intel->batch);
      UNLOCK_HARDWARE(intel);
   }
   else {
      intelEmitCopyBlit(intel,
			region->cpp,
			region->pitch,
			region->buffer, 0,
			region->pitch,
			pbo->buffer, 0,
			0, 0, 0, 0, 
			region->pitch, region->height,
			GL_COPY);
      
      intel_batchbuffer_flush(intel->batch);
   }
}

dri_bo *
intel_region_buffer(intelScreenPrivate *intelScreen,
                    struct intel_region *region, GLuint flag)
{
   if (region->pbo) {
      if (flag == INTEL_WRITE_PART)
         intel_region_cow(intelScreen, region);
      else if (flag == INTEL_WRITE_FULL)
         intel_region_release_pbo(intelScreen, region);
   }

   return region->buffer;
}
