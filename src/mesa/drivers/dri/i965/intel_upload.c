/*
 * Copyright 2003 VMware, Inc.
 * Copyright © 2007 Intel Corporation
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

/**
 * @file intel_upload.c
 *
 * Batched upload via BOs.
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"

#include "brw_context.h"
#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "brw_context.h"

#define INTEL_UPLOAD_SIZE (64*1024)

/**
 * Like ALIGN(), but works with a non-power-of-two alignment.
 */
#define ALIGN_NPOT(value, alignment) \
   (((value) + (alignment) - 1) / (alignment) * (alignment))

void
intel_upload_finish(struct brw_context *brw)
{
   if (!brw->upload.bo)
      return;

   drm_intel_bo_unmap(brw->upload.bo);
   drm_intel_bo_unreference(brw->upload.bo);
   brw->upload.bo = NULL;
   brw->upload.next_offset = 0;
}

/**
 * Interface for getting memory for uploading streamed data to the GPU
 *
 * In most cases, streamed data (for GPU state structures, for example) is
 * uploaded through brw_state_batch(), since that interface allows relocations
 * from the streamed space returned to other BOs.  However, that interface has
 * the restriction that the amount of space allocated has to be "small" (see
 * estimated_max_prim_size in brw_draw.c).
 *
 * This interface, on the other hand, is able to handle arbitrary sized
 * allocation requests, though it will batch small allocations into the same
 * BO for efficiency and reduced memory footprint.
 *
 * \note The returned pointer is valid only until intel_upload_finish(), which
 * will happen at batch flush or the next
 * intel_upload_space()/intel_upload_data().
 *
 * \param out_bo Pointer to a BO, which must point to a valid BO or NULL on
 * entry, and will have a reference to the new BO containing the state on
 * return.
 *
 * \param out_offset Offset within the buffer object that the data will land.
 */
void *
intel_upload_space(struct brw_context *brw,
                   uint32_t size,
                   uint32_t alignment,
                   drm_intel_bo **out_bo,
                   uint32_t *out_offset)
{
   uint32_t offset;

   offset = ALIGN_NPOT(brw->upload.next_offset, alignment);
   if (brw->upload.bo && offset + size > brw->upload.bo->size) {
      intel_upload_finish(brw);
      offset = 0;
   }

   if (!brw->upload.bo) {
      brw->upload.bo = drm_intel_bo_alloc(brw->bufmgr, "streamed data",
                                          MAX2(INTEL_UPLOAD_SIZE, size), 4096);
      if (brw->has_llc)
         drm_intel_bo_map(brw->upload.bo, true);
      else
         drm_intel_gem_bo_map_gtt(brw->upload.bo);
   }

   brw->upload.next_offset = offset + size;

   *out_offset = offset;
   if (*out_bo != brw->upload.bo) {
      drm_intel_bo_unreference(*out_bo);
      *out_bo = brw->upload.bo;
      drm_intel_bo_reference(brw->upload.bo);
   }

   return brw->upload.bo->virtual + offset;
}

/**
 * Handy interface to upload some data to temporary GPU memory quickly.
 *
 * References to this memory should not be retained across batch flushes.
 */
void
intel_upload_data(struct brw_context *brw,
                  const void *data,
                  uint32_t size,
                  uint32_t alignment,
                  drm_intel_bo **out_bo,
                  uint32_t *out_offset)
{
   void *dst = intel_upload_space(brw, size, alignment, out_bo, out_offset);
   memcpy(dst, data, size);
}
