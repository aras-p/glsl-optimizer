/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "intel_winsys.h"

#include "ilo_builder_mi.h"
#include "ilo_shader.h"
#include "ilo_cp.h"

static const struct ilo_cp_owner ilo_cp_default_owner;

static void
ilo_cp_release_owner(struct ilo_cp *cp)
{
   if (cp->owner != &ilo_cp_default_owner) {
      const struct ilo_cp_owner *owner = cp->owner;

      cp->owner = &ilo_cp_default_owner;

      assert(ilo_cp_space(cp) >= owner->reserve);
      owner->release(cp, owner->data);
   }
}

/**
 * Set the parser owner.  If this is a new owner or a new ring, the old owner
 * is released and the new owner's own() is called.  The parser may implicitly
 * submit if there is a ring change.
 *
 * own() is called before \p owner owns the parser.  It must make sure there
 * is more space than \p owner->reserve when it returns.  Calling
 * ilo_cp_submit() is allowed.
 *
 * release() will be called after \p owner loses the parser.  That may happen
 * just before the parser submits and ilo_cp_submit() is not allowed.
 */
void
ilo_cp_set_owner(struct ilo_cp *cp, enum intel_ring_type ring,
                 const struct ilo_cp_owner *owner)
{
   if (!owner)
      owner = &ilo_cp_default_owner;

   if (cp->ring != ring) {
      ilo_cp_submit(cp, "ring change");
      cp->ring = ring;
   }

   if (cp->owner != owner) {
      ilo_cp_release_owner(cp);

      owner->own(cp, owner->data);

      assert(ilo_cp_space(cp) >= owner->reserve);
      cp->owner = owner;
   }
}

static struct intel_bo *
ilo_cp_end_batch(struct ilo_cp *cp, unsigned *used)
{
   struct intel_bo *bo;

   ilo_cp_release_owner(cp);

   if (!ilo_builder_batch_used(&cp->builder)) {
      ilo_builder_batch_discard(&cp->builder);
      return NULL;
   }

   /* see ilo_cp_space() */
   assert(ilo_builder_batch_space(&cp->builder) >= 2);
   gen6_mi_batch_buffer_end(&cp->builder);

   bo = ilo_builder_end(&cp->builder, used);

   /* we have to assume that kernel uploads also failed */
   if (!bo)
      ilo_shader_cache_invalidate(cp->shader_cache);

   return bo;
}

/**
 * Flush the command parser and execute the commands.  When the parser buffer
 * is empty, the callback is not invoked.
 */
void
ilo_cp_submit_internal(struct ilo_cp *cp)
{
   const bool do_exec = !(ilo_debug & ILO_DEBUG_NOHW);
   struct intel_bo *bo;
   unsigned used;
   int err;

   bo = ilo_cp_end_batch(cp, &used);
   if (!bo)
      return;

   if (likely(do_exec)) {
      err = intel_winsys_submit_bo(cp->winsys, cp->ring,
            bo, used, cp->render_ctx, cp->one_off_flags);
   }
   else {
      err = 0;
   }

   cp->one_off_flags = 0;

   if (!err) {
      if (cp->last_submitted_bo)
         intel_bo_unreference(cp->last_submitted_bo);
      cp->last_submitted_bo = bo;
      intel_bo_reference(cp->last_submitted_bo);

      if (ilo_debug & ILO_DEBUG_BATCH)
         ilo_builder_decode(&cp->builder);

      if (cp->submit_callback)
         cp->submit_callback(cp, cp->submit_callback_data);
   }

   ilo_builder_begin(&cp->builder);
}

/**
 * Destroy the command parser.
 */
void
ilo_cp_destroy(struct ilo_cp *cp)
{
   ilo_builder_reset(&cp->builder);

   intel_winsys_destroy_context(cp->winsys, cp->render_ctx);
   FREE(cp);
}

/**
 * Create a command parser.
 */
struct ilo_cp *
ilo_cp_create(const struct ilo_dev_info *dev,
              struct intel_winsys *winsys,
              struct ilo_shader_cache *shc)
{
   struct ilo_cp *cp;

   cp = CALLOC_STRUCT(ilo_cp);
   if (!cp)
      return NULL;

   cp->winsys = winsys;
   cp->shader_cache = shc;
   cp->render_ctx = intel_winsys_create_context(winsys);
   if (!cp->render_ctx) {
      FREE(cp);
      return NULL;
   }

   cp->ring = INTEL_RING_RENDER;
   cp->owner = &ilo_cp_default_owner;

   ilo_builder_init(&cp->builder, dev, winsys);

   if (!ilo_builder_begin(&cp->builder)) {
      ilo_cp_destroy(cp);
      return NULL;
   }

   return cp;
}
