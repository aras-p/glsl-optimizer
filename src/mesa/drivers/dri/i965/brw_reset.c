/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "brw_context.h"

/**
 * Query information about GPU resets observed by this context
 *
 * Called via \c dd_function_table::GetGraphicsResetStatus.
 */
GLenum
brw_get_graphics_reset_status(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   int err;
   uint32_t reset_count;
   uint32_t active;
   uint32_t pending;

   /* If hardware contexts are not being used (or
    * DRM_IOCTL_I915_GET_RESET_STATS is not supported), this function should
    * not be accessible.
    */
   assert(brw->hw_ctx != NULL);

   /* A reset status other than NO_ERROR was returned last time. I915 returns
    * nonzero active/pending only if reset has been encountered and completed.
    * Return NO_ERROR from now on.
    */
   if (brw->reset_count != 0)
      return GL_NO_ERROR;

   err = drm_intel_get_reset_stats(brw->hw_ctx, &reset_count, &active,
                                   &pending);
   if (err)
      return GL_NO_ERROR;

   /* A reset was observed while a batch from this context was executing.
    * Assume that this context was at fault.
    */
   if (active != 0) {
      brw->reset_count = reset_count;
      return GL_GUILTY_CONTEXT_RESET_ARB;
   }

   /* A reset was observed while a batch from this context was in progress,
    * but the batch was not executing.  In this case, assume that the context
    * was not at fault.
    */
   if (pending != 0) {
      brw->reset_count = reset_count;
      return GL_INNOCENT_CONTEXT_RESET_ARB;
   }

   return GL_NO_ERROR;
}
