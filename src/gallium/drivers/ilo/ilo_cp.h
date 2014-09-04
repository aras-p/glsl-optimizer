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
   struct intel_context *render_ctx;

   ilo_cp_callback flush_callback;
   void *flush_callback_data;

   const struct ilo_cp_owner *owner;
   int owner_reserve;

   enum intel_ring_type ring;
   bool no_implicit_flush;
   unsigned one_off_flags;

   struct ilo_builder builder;
   struct intel_bo *last_submitted_bo;

   unsigned pos;
   uint32_t *ptr;
   int cmd_cur, cmd_end;
};

struct ilo_cp *
ilo_cp_create(const struct ilo_dev_info *dev, struct intel_winsys *winsys);

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
   if (cp->no_implicit_flush) {
      assert(!"unexpected command parser flush");
      ilo_builder_batch_discard(&cp->builder);
   }

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
 * Assert that no function should flush implicitly.
 */
static inline void
ilo_cp_assert_no_implicit_flush(struct ilo_cp *cp, bool enable)
{
   cp->no_implicit_flush = enable;
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
      const bool no_implicit_flush = cp->no_implicit_flush;

      /* reclaim the reserved space */
      cp->owner_reserve = 0;

      /* invoke the release callback */
      cp->no_implicit_flush = true;
      cp->owner->release_callback(cp, cp->owner->release_data);
      cp->no_implicit_flush = no_implicit_flush;

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

/**
 * Begin writing a command.
 */
static inline void
ilo_cp_begin(struct ilo_cp *cp, int cmd_size)
{
   if (ilo_cp_space(cp) < cmd_size) {
      ilo_cp_implicit_flush(cp);
      assert(ilo_cp_space(cp) >= cmd_size);
   }

   cp->pos = ilo_builder_batch_pointer(&cp->builder, cmd_size, &cp->ptr);

   cp->cmd_cur = 0;
   cp->cmd_end = cmd_size;
}

/**
 * Begin writing data to a space stolen from the top of the parser buffer.
 *
 * \param item builder item type
 * \param data_size in dwords
 * \param align in dwords
 * \param bo_offset in bytes to the stolen space
 */
static inline void
ilo_cp_steal(struct ilo_cp *cp, enum ilo_builder_item_type item,
             int data_size, int align, uint32_t *bo_offset)
{
   if (!align)
      align = 1;

   /* flush if there is not enough space after stealing */
   if (ilo_cp_space(cp) < data_size + align - 1) {
      ilo_cp_implicit_flush(cp);
      assert(ilo_cp_space(cp) >= data_size + align - 1);
   }

   cp->pos = ilo_builder_state_pointer(&cp->builder,
         item, align << 2, data_size, &cp->ptr) >> 2;

   cp->cmd_cur = 0;
   cp->cmd_end = data_size;

   /* offset in cp->bo */
   if (bo_offset)
      *bo_offset = cp->pos << 2;
}

/**
 * Write a dword to the parser buffer.  This function must be enclosed by
 * ilo_cp_begin()/ilo_cp_steal() and ilo_cp_end().
 */
static inline void
ilo_cp_write(struct ilo_cp *cp, uint32_t val)
{
   assert(cp->cmd_cur < cp->cmd_end);
   cp->ptr[cp->cmd_cur++] = val;
}

/**
 * Write multiple dwords to the parser buffer.
 */
static inline void
ilo_cp_write_multi(struct ilo_cp *cp, const void *vals, int num_vals)
{
   assert(cp->cmd_cur + num_vals <= cp->cmd_end);
   memcpy(cp->ptr + cp->cmd_cur, vals, num_vals * 4);
   cp->cmd_cur += num_vals;
}

/**
 * Write a bo to the parser buffer.  In addition to writing the offset of the
 * bo to the buffer, it also emits a relocation.
 */
static inline void
ilo_cp_write_bo(struct ilo_cp *cp, uint32_t val,
                struct intel_bo *bo, uint32_t flags)
{
   if (bo) {
      ilo_builder_batch_reloc(&cp->builder, cp->pos + cp->cmd_cur,
            bo, val, flags);
      cp->cmd_cur++;
   }
   else {
      ilo_cp_write(cp, 0);
   }
}

/**
 * End a command.  Every ilo_cp_begin() or ilo_cp_steal() must have a
 * matching ilo_cp_end().
 */
static inline void
ilo_cp_end(struct ilo_cp *cp)
{
   assert(cp->cmd_cur == cp->cmd_end);
}

/**
 * A variant of ilo_cp_steal() where the data are written via the returned
 * pointer.
 *
 * \return ptr pointer where the data are written to.  It is valid until any
 *             change is made to the parser.
 */
static inline void *
ilo_cp_steal_ptr(struct ilo_cp *cp, enum ilo_builder_item_type item,
                 int data_size, int align, uint32_t *bo_offset)
{
   void *ptr;

   ilo_cp_steal(cp, item, data_size, align, bo_offset);

   ptr = &cp->ptr[cp->cmd_cur];
   cp->cmd_cur = cp->cmd_end;

   ilo_cp_end(cp);

   return ptr;
}

#endif /* ILO_CP_H */
