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

#ifndef ILO_CP_H
#define ILO_CP_H

#include "intel_winsys.h"

#include "ilo_builder.h"
#include "ilo_common.h"

struct ilo_cp;
struct ilo_shader_cache;

typedef void (*ilo_cp_callback)(struct ilo_cp *cp, void *data);

/**
 * Parser owners are notified when they gain or lose the ownership of the
 * parser.  This gives owners a chance to emit prolog or epilog.
 */
struct ilo_cp_owner {
   ilo_cp_callback own;
   ilo_cp_callback release;
   void *data;

   /*
    * Space reserved for release().  This can be modified at any time, as long
    * as it is never increased by more than ilo_cp_space().
    */
   int reserve;
};

/**
 * Command parser.
 */
struct ilo_cp {
   struct intel_winsys *winsys;
   struct ilo_shader_cache *shader_cache;
   struct intel_context *render_ctx;

   ilo_cp_callback submit_callback;
   void *submit_callback_data;

   enum intel_ring_type ring;
   const struct ilo_cp_owner *owner;

   unsigned one_off_flags;

   struct ilo_builder builder;
   struct intel_bo *last_submitted_bo;
};

struct ilo_cp *
ilo_cp_create(const struct ilo_dev_info *dev,
              struct intel_winsys *winsys,
              struct ilo_shader_cache *shc);

void
ilo_cp_destroy(struct ilo_cp *cp);

void
ilo_cp_submit_internal(struct ilo_cp *cp);

static inline void
ilo_cp_submit(struct ilo_cp *cp, const char *reason)
{
   if (ilo_debug & ILO_DEBUG_SUBMIT) {
      ilo_printf("submit batch buffer to %s ring because of %s: ",
            (cp->ring == INTEL_RING_RENDER) ? "render" : "unknown", reason);
      ilo_builder_batch_print_stats(&cp->builder);
   }

   ilo_cp_submit_internal(cp);
}

void
ilo_cp_set_owner(struct ilo_cp *cp, enum intel_ring_type ring,
                 const struct ilo_cp_owner *owner);

/**
 * Return the remaining space (in dwords) in the parser buffer.
 */
static inline int
ilo_cp_space(struct ilo_cp *cp)
{
   const int space = ilo_builder_batch_space(&cp->builder);
   const int mi_batch_buffer_end_space = 2;

   assert(space >= cp->owner->reserve + mi_batch_buffer_end_space);

   return space - cp->owner->reserve - mi_batch_buffer_end_space;
}

/**
 * Set one-off flags.  They will be cleared after submission.
 */
static inline void
ilo_cp_set_one_off_flags(struct ilo_cp *cp, unsigned flags)
{
   cp->one_off_flags |= flags;
}

/**
 * Set submit callback.  The callback is invoked after the bo has been
 * successfully submitted, and before the bo is reallocated.
 */
static inline void
ilo_cp_set_submit_callback(struct ilo_cp *cp, ilo_cp_callback callback,
                          void *data)
{
   cp->submit_callback = callback;
   cp->submit_callback_data = data;
}

#endif /* ILO_CP_H */
