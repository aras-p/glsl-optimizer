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

struct ilo_cp_owner {
   ilo_cp_callback release_callback;
   void *release_data;
};

/**
 * Command parser.
 */
struct ilo_cp {
   struct intel_winsys *winsys;
   struct ilo_shader_cache *shader_cache;
   struct intel_context *render_ctx;

   ilo_cp_callback flush_callback;
   void *flush_callback_data;

   const struct ilo_cp_owner *owner;
   int owner_reserve;

   enum intel_ring_type ring;
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
ilo_cp_flush_internal(struct ilo_cp *cp);

static inline void
ilo_cp_flush(struct ilo_cp *cp, const char *reason)
{
   if (ilo_debug & ILO_DEBUG_FLUSH) {
      ilo_printf("cp flushed for %s because of %s: ",
            (cp->ring == INTEL_RING_RENDER) ? "render" : "other", reason);
      ilo_builder_batch_print_stats(&cp->builder);
   }

   ilo_cp_flush_internal(cp);
}

/**
 * Return true if the parser buffer is empty.
 */
static inline bool
ilo_cp_empty(struct ilo_cp *cp)
{
   return !ilo_builder_batch_used(&cp->builder);
}

/**
 * Return the remaining space (in dwords) in the parser buffer.
 */
static inline int
ilo_cp_space(struct ilo_cp *cp)
{
   const int space = ilo_builder_batch_space(&cp->builder);
   const int mi_batch_buffer_end_space = 2;

   assert(space >= cp->owner_reserve + mi_batch_buffer_end_space);

   return space - cp->owner_reserve - mi_batch_buffer_end_space;
}

/**
 * Internal function called by functions that flush implicitly.
 */
static inline void
ilo_cp_implicit_flush(struct ilo_cp *cp)
{
   ilo_cp_flush(cp, "out of space (implicit)");
}

/**
 * Set the ring buffer.
 */
static inline void
ilo_cp_set_ring(struct ilo_cp *cp, enum intel_ring_type ring)
{
   if (cp->ring != ring) {
      ilo_cp_implicit_flush(cp);
      cp->ring = ring;
   }
}

/**
 * Set one-off flags.  They will be cleared after flushing.
 */
static inline void
ilo_cp_set_one_off_flags(struct ilo_cp *cp, unsigned flags)
{
   cp->one_off_flags |= flags;
}

/**
 * Set flush callback.  The callback is invoked after the bo has been
 * successfully executed, and before the bo is reallocated.
 */
static inline void
ilo_cp_set_flush_callback(struct ilo_cp *cp, ilo_cp_callback callback,
                          void *data)
{
   cp->flush_callback = callback;
   cp->flush_callback_data = data;
}

/**
 * Set the parser owner.  If this is a new owner, the previous owner is
 * notified and the space it reserved is reclaimed.
 *
 * \return true if this is a new owner
 */
static inline bool
ilo_cp_set_owner(struct ilo_cp *cp, const struct ilo_cp_owner *owner,
                 int reserve)
{
   const bool new_owner = (cp->owner != owner);

   /* release current owner */
   if (new_owner && cp->owner) {
      /* reclaim the reserved space */
      cp->owner_reserve = 0;

      /* invoke the release callback */
      cp->owner->release_callback(cp, cp->owner->release_data);

      cp->owner = NULL;
   }

   if (cp->owner_reserve != reserve) {
      const int extra = reserve - cp->owner_reserve;

      if (ilo_cp_space(cp) < extra) {
         ilo_cp_implicit_flush(cp);

         assert(ilo_cp_space(cp) >= reserve);
         cp->owner_reserve = reserve;
      }
      else {
         cp->owner_reserve += extra;
      }
   }

   /* set owner last because of the possible flush above */
   cp->owner = owner;

   return new_owner;
}

#endif /* ILO_CP_H */
