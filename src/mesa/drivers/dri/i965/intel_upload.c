/*
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "intel_regions.h"

#include "brw_context.h"

#define INTEL_UPLOAD_SIZE (64*1024)

void
intel_upload_finish(struct brw_context *brw)
{
   if (!brw->upload.bo)
      return;

   if (brw->upload.buffer_len) {
      drm_intel_bo_subdata(brw->upload.bo,
                           brw->upload.buffer_offset,
                           brw->upload.buffer_len,
                           brw->upload.buffer);
      brw->upload.buffer_len = 0;
   }

   drm_intel_bo_unreference(brw->upload.bo);
   brw->upload.bo = NULL;
}

static void
wrap_buffers(struct brw_context *brw, GLuint size)
{
   intel_upload_finish(brw);

   if (size < INTEL_UPLOAD_SIZE)
      size = INTEL_UPLOAD_SIZE;

   brw->upload.bo = drm_intel_bo_alloc(brw->bufmgr, "upload", size, 0);
   brw->upload.offset = 0;
}

void
intel_upload_data(struct brw_context *brw,
                  const void *ptr, GLuint size, GLuint align,
                  drm_intel_bo **return_bo,
                  GLuint *return_offset)
{
   GLuint base, delta;

   base = (brw->upload.offset + align - 1) / align * align;
   if (brw->upload.bo == NULL || base + size > brw->upload.bo->size) {
      wrap_buffers(brw, size);
      base = 0;
   }

   drm_intel_bo_reference(brw->upload.bo);
   *return_bo = brw->upload.bo;
   *return_offset = base;

   delta = base - brw->upload.offset;
   if (brw->upload.buffer_len &&
       brw->upload.buffer_len + delta + size > sizeof(brw->upload.buffer)) {
      drm_intel_bo_subdata(brw->upload.bo,
                           brw->upload.buffer_offset,
                           brw->upload.buffer_len,
                           brw->upload.buffer);
      brw->upload.buffer_len = 0;
   }

   if (size < sizeof(brw->upload.buffer)) {
      if (brw->upload.buffer_len == 0)
         brw->upload.buffer_offset = base;
      else
         brw->upload.buffer_len += delta;

      memcpy(brw->upload.buffer + brw->upload.buffer_len, ptr, size);
      brw->upload.buffer_len += size;
   } else {
      drm_intel_bo_subdata(brw->upload.bo, base, size, ptr);
   }

   brw->upload.offset = base + size;
}

void *
intel_upload_map(struct brw_context *brw, GLuint size, GLuint align)
{
   GLuint base, delta;
   char *ptr;

   base = (brw->upload.offset + align - 1) / align * align;
   if (brw->upload.bo == NULL || base + size > brw->upload.bo->size) {
      wrap_buffers(brw, size);
      base = 0;
   }

   delta = base - brw->upload.offset;
   if (brw->upload.buffer_len &&
       brw->upload.buffer_len + delta + size > sizeof(brw->upload.buffer)) {
      drm_intel_bo_subdata(brw->upload.bo,
                           brw->upload.buffer_offset,
                           brw->upload.buffer_len,
                           brw->upload.buffer);
      brw->upload.buffer_len = 0;
   }

   if (size <= sizeof(brw->upload.buffer)) {
      if (brw->upload.buffer_len == 0)
         brw->upload.buffer_offset = base;
      else
         brw->upload.buffer_len += delta;

      ptr = brw->upload.buffer + brw->upload.buffer_len;
      brw->upload.buffer_len += size;
   } else {
      ptr = malloc(size);
   }

   return ptr;
}

void
intel_upload_unmap(struct brw_context *brw,
                   const void *ptr, GLuint size, GLuint align,
                   drm_intel_bo **return_bo,
                   GLuint *return_offset)
{
   GLuint base;

   base = (brw->upload.offset + align - 1) / align * align;
   if (size > sizeof(brw->upload.buffer)) {
      drm_intel_bo_subdata(brw->upload.bo, base, size, ptr);
      free((void*)ptr);
   }

   drm_intel_bo_reference(brw->upload.bo);
   *return_bo = brw->upload.bo;
   *return_offset = base;

   brw->upload.offset = base + size;
}
