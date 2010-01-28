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

#include <sys/ioctl.h>
#include <errno.h>

#include "intel_context.h"
#include "intel_regions.h"
#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "intel_bufmgr.h"
#include "intel_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_REGION

/* This should be set to the maximum backtrace size desired.
 * Set it to 0 to disable backtrace debugging.
 */
#define DEBUG_BACKTRACE_SIZE 0

#if DEBUG_BACKTRACE_SIZE == 0
/* Use the standard debug output */
#define _DBG(...) DBG(__VA_ARGS__)
#else
/* Use backtracing debug output */
#define _DBG(...) {debug_backtrace(); DBG(__VA_ARGS__);}

/* Backtracing debug support */
#include <execinfo.h>

static void
debug_backtrace(void)
{
   void *trace[DEBUG_BACKTRACE_SIZE];
   char **strings = NULL;
   int traceSize;
   register int i;

   traceSize = backtrace(trace, DEBUG_BACKTRACE_SIZE);
   strings = backtrace_symbols(trace, traceSize);
   if (strings == NULL) {
      DBG("no backtrace:");
      return;
   }

   /* Spit out all the strings with a colon separator.  Ignore
    * the first, since we don't really care about the call
    * to debug_backtrace() itself.  Skip until the final "/" in
    * the trace to avoid really long lines.
    */
   for (i = 1; i < traceSize; i++) {
      char *p = strings[i], *slash = strings[i];
      while (*p) {
         if (*p++ == '/') {
            slash = p;
         }
      }

      DBG("%s:", slash);
   }

   /* Free up the memory, and we're done */
   free(strings);
}

#endif



/* XXX: Thread safety?
 */
GLubyte *
intel_region_map(struct intel_context *intel, struct intel_region *region)
{
   intelFlush(&intel->ctx);

   _DBG("%s %p\n", __FUNCTION__, region);
   if (!region->map_refcount++) {
      if (region->pbo)
         intel_region_cow(intel, region);

      if (region->tiling != I915_TILING_NONE &&
	  intel->intelScreen->kernel_exec_fencing)
	 drm_intel_gem_bo_map_gtt(region->buffer);
      else
	 dri_bo_map(region->buffer, GL_TRUE);
      region->map = region->buffer->virtual;
   }

   return region->map;
}

void
intel_region_unmap(struct intel_context *intel, struct intel_region *region)
{
   _DBG("%s %p\n", __FUNCTION__, region);
   if (!--region->map_refcount) {
      if (region->tiling != I915_TILING_NONE &&
	  intel->intelScreen->kernel_exec_fencing)
	 drm_intel_gem_bo_unmap_gtt(region->buffer);
      else
	 dri_bo_unmap(region->buffer);
      region->map = NULL;
   }
}

static struct intel_region *
intel_region_alloc_internal(struct intel_context *intel,
			    GLuint cpp,
			    GLuint width, GLuint height, GLuint pitch,
			    dri_bo *buffer)
{
   struct intel_region *region;

   if (buffer == NULL) {
      _DBG("%s <-- NULL\n", __FUNCTION__);
      return NULL;
   }

   region = calloc(sizeof(*region), 1);
   region->cpp = cpp;
   region->width = width;
   region->height = height;
   region->pitch = pitch;
   region->refcount = 1;
   region->buffer = buffer;

   /* Default to no tiling */
   region->tiling = I915_TILING_NONE;
   region->bit_6_swizzle = I915_BIT_6_SWIZZLE_NONE;

   _DBG("%s <-- %p\n", __FUNCTION__, region);
   return region;
}

struct intel_region *
intel_region_alloc(struct intel_context *intel,
		   uint32_t tiling,
                   GLuint cpp, GLuint width, GLuint height, GLuint pitch,
		   GLboolean expect_accelerated_upload)
{
   dri_bo *buffer;
   struct intel_region *region;

   /* If we're tiled, our allocations are in 8 or 32-row blocks, so
    * failure to align our height means that we won't allocate enough pages.
    *
    * If we're untiled, we still have to align to 2 rows high because the
    * data port accesses 2x2 blocks even if the bottom row isn't to be
    * rendered, so failure to align means we could walk off the end of the
    * GTT and fault.
    */
   if (tiling == I915_TILING_X)
      height = ALIGN(height, 8);
   else if (tiling == I915_TILING_Y)
      height = ALIGN(height, 32);
   else
      height = ALIGN(height, 2);

   /* If we're untiled, we have to align to 2 rows high because the
    * data port accesses 2x2 blocks even if the bottom row isn't to be
    * rendered, so failure to align means we could walk off the end of the
    * GTT and fault.
    */
   height = ALIGN(height, 2);

   if (expect_accelerated_upload) {
      buffer = drm_intel_bo_alloc_for_render(intel->bufmgr, "region",
					     pitch * cpp * height, 64);
   } else {
      buffer = drm_intel_bo_alloc(intel->bufmgr, "region",
				  pitch * cpp * height, 64);
   }

   region = intel_region_alloc_internal(intel, cpp, width, height,
					pitch, buffer);

   if (tiling != I915_TILING_NONE) {
      assert(((pitch * cpp) & 127) == 0);
      drm_intel_bo_set_tiling(buffer, &tiling, pitch * cpp);
      drm_intel_bo_get_tiling(buffer, &region->tiling, &region->bit_6_swizzle);
   }

   return region;
}

struct intel_region *
intel_region_alloc_for_handle(struct intel_context *intel,
			      GLuint cpp,
			      GLuint width, GLuint height, GLuint pitch,
			      GLuint handle, const char *name)
{
   struct intel_region *region;
   dri_bo *buffer;
   int ret;

   buffer = intel_bo_gem_create_from_name(intel->bufmgr, name, handle);

   region = intel_region_alloc_internal(intel, cpp,
					width, height, pitch, buffer);
   if (region == NULL)
      return region;

   ret = dri_bo_get_tiling(region->buffer, &region->tiling,
			   &region->bit_6_swizzle);
   if (ret != 0) {
      fprintf(stderr, "Couldn't get tiling of buffer %d (%s): %s\n",
	      handle, name, strerror(-ret));
      intel_region_release(&region);
      return NULL;
   }

   return region;
}

void
intel_region_reference(struct intel_region **dst, struct intel_region *src)
{
   if (src)
      _DBG("%s %p %d\n", __FUNCTION__, src, src->refcount);

   assert(*dst == NULL);
   if (src) {
      src->refcount++;
      *dst = src;
   }
}

void
intel_region_release(struct intel_region **region_handle)
{
   struct intel_region *region = *region_handle;

   if (region == NULL) {
      _DBG("%s NULL\n", __FUNCTION__);
      return;
   }

   _DBG("%s %p %d\n", __FUNCTION__, region, region->refcount - 1);

   ASSERT(region->refcount > 0);
   region->refcount--;

   if (region->refcount == 0) {
      assert(region->map_refcount == 0);

      if (region->pbo)
	 region->pbo->region = NULL;
      region->pbo = NULL;
      dri_bo_unreference(region->buffer);

      if (region->classic_map != NULL) {
	 drmUnmap(region->classic_map,
			region->pitch * region->cpp * region->height);
      }

      free(region);
   }
   *region_handle = NULL;
}

/*
 * XXX Move this into core Mesa?
 */
void
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
intel_region_data(struct intel_context *intel,
                  struct intel_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  const void *src, GLuint src_pitch,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   _DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;

   if (dst->pbo) {
      if (dstx == 0 &&
          dsty == 0 && width == dst->pitch && height == dst->height)
         intel_region_release_pbo(intel, dst);
      else
         intel_region_cow(intel, dst);
   }

   _mesa_copy_rect(intel_region_map(intel, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   intel_region_unmap(intel, dst);
}

/* Copy rectangular sub-regions. Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
GLboolean
intel_region_copy(struct intel_context *intel,
                  struct intel_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  struct intel_region *src,
                  GLuint src_offset,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height,
		  GLenum logicop)
{
   _DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return GL_FALSE;

   if (dst->pbo) {
      if (dstx == 0 &&
          dsty == 0 && width == dst->pitch && height == dst->height)
         intel_region_release_pbo(intel, dst);
      else
         intel_region_cow(intel, dst);
   }

   assert(src->cpp == dst->cpp);

   return intelEmitCopyBlit(intel,
			    dst->cpp,
			    src->pitch, src->buffer, src_offset, src->tiling,
			    dst->pitch, dst->buffer, dst_offset, dst->tiling,
			    srcx, srcy, dstx, dsty, width, height,
			    logicop);
}

/* Attach to a pbo, discarding our data.  Effectively zero-copy upload
 * the pbo's data.
 */
void
intel_region_attach_pbo(struct intel_context *intel,
                        struct intel_region *region,
                        struct intel_buffer_object *pbo)
{
   dri_bo *buffer;

   if (region->pbo == pbo)
      return;

   _DBG("%s %p %p\n", __FUNCTION__, region, pbo);

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

   /* make sure pbo has a buffer of its own */
   buffer = intel_bufferobj_buffer(intel, pbo, INTEL_WRITE_FULL);

   region->pbo = pbo;
   region->pbo->region = region;
   dri_bo_reference(buffer);
   region->buffer = buffer;
   region->tiling = I915_TILING_NONE;
}


/* Break the COW tie to the pbo and allocate a new buffer.
 * The pbo gets to keep the data.
 */
void
intel_region_release_pbo(struct intel_context *intel,
                         struct intel_region *region)
{
   _DBG("%s %p\n", __FUNCTION__, region);
   assert(region->buffer == region->pbo->buffer);
   region->pbo->region = NULL;
   region->pbo = NULL;
   dri_bo_unreference(region->buffer);
   region->buffer = NULL;

   region->buffer = dri_bo_alloc(intel->bufmgr, "region",
				 region->pitch * region->cpp * region->height,
				 64);
}

/* Break the COW tie to the pbo.  Both the pbo and the region end up
 * with a copy of the data.
 */
void
intel_region_cow(struct intel_context *intel, struct intel_region *region)
{
   struct intel_buffer_object *pbo = region->pbo;
   GLboolean ok;

   intel_region_release_pbo(intel, region);

   assert(region->cpp * region->pitch * region->height == pbo->Base.Size);

   _DBG("%s %p (%d bytes)\n", __FUNCTION__, region, pbo->Base.Size);

   /* Now blit from the texture buffer to the new buffer: 
    */

   ok = intelEmitCopyBlit(intel,
                          region->cpp,
                          region->pitch, pbo->buffer, 0, region->tiling,
                          region->pitch, region->buffer, 0, region->tiling,
                          0, 0, 0, 0,
                          region->pitch, region->height,
                          GL_COPY);
   assert(ok);
}

dri_bo *
intel_region_buffer(struct intel_context *intel,
                    struct intel_region *region, GLuint flag)
{
   if (region->pbo) {
      if (flag == INTEL_WRITE_PART)
         intel_region_cow(intel, region);
      else if (flag == INTEL_WRITE_FULL)
         intel_region_release_pbo(intel, region);
   }

   return region->buffer;
}
