/**************************************************************************
 *
 * Copyright 2005 VMware, Inc.
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

#ifndef INTEL_BUFFEROBJ_H
#define INTEL_BUFFEROBJ_H

#include "main/mtypes.h"

struct brw_context;
struct gl_buffer_object;


/**
 * Intel vertex/pixel buffer object, derived from Mesa's gl_buffer_object.
 */
struct intel_buffer_object
{
   struct gl_buffer_object Base;
   drm_intel_bo *buffer;     /* the low-level buffer manager's buffer handle */

   drm_intel_bo *range_map_bo[MAP_COUNT];

   /**
    * Alignment offset from the range_map_bo temporary mapping to the returned
    * obj->Pointer (caused by GL_ARB_map_buffer_alignment).
    */
   unsigned map_extra[MAP_COUNT];

   /** @{
    * Tracking for what range of the BO may currently be in use by the GPU.
    *
    * Users often want to either glBufferSubData() or glMapBufferRange() a
    * buffer object where some subset of it is busy on the GPU, without either
    * stalling or doing an extra blit (since our blits are extra expensive,
    * given that we have to reupload most of the 3D state when switching
    * rings).  We wish they'd just use glMapBufferRange() with the
    * UNSYNC|INVALIDATE_RANGE flag or the INVALIDATE_BUFFER flag, but lots
    * don't.
    *
    * To work around apps, we track what range of the BO we might have used on
    * the GPU as vertex data, tranform feedback output, buffer textures, etc.,
    * and just do glBufferSubData() with an unsynchronized map when they're
    * outside of that range.
    *
    * If gpu_active_start > gpu_active_end, then the GPU is not currently
    * accessing the BO (and we can map it without synchronization).
    */
   uint32_t gpu_active_start;
   uint32_t gpu_active_end;

   /**
    * If we've avoided stalls/blits using the active tracking, flag the buffer
    * for (occasional) stalling in the future to avoid getting stuck in a
    * cycle of blitting on buffer wraparound.
    */
   bool prefer_stall_to_blit;
   /** @} */
};


/* Get the bm buffer associated with a GL bufferobject:
 */
drm_intel_bo *intel_bufferobj_buffer(struct brw_context *brw,
                                     struct intel_buffer_object *obj,
                                     uint32_t offset,
                                     uint32_t size);

void intel_upload_data(struct brw_context *brw,
                       const void *data,
                       uint32_t size,
                       uint32_t alignment,
                       drm_intel_bo **out_bo,
                       uint32_t *out_offset);

void *intel_upload_space(struct brw_context *brw,
                         uint32_t size,
                         uint32_t alignment,
                         drm_intel_bo **out_bo,
                         uint32_t *out_offset);

void intel_upload_finish(struct brw_context *brw);

/* Hook the bufferobject implementation into mesa:
 */
void intelInitBufferObjectFuncs(struct dd_function_table *functions);

static inline struct intel_buffer_object *
intel_buffer_object(struct gl_buffer_object *obj)
{
   return (struct intel_buffer_object *) obj;
}

#endif
