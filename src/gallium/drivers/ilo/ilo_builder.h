/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#ifndef ILO_BUILDER_H
#define ILO_BUILDER_H

#include "intel_winsys.h"
#include "ilo_common.h"

enum ilo_builder_writer_type {
   ILO_BUILDER_WRITER_BATCH,
   ILO_BUILDER_WRITER_INSTRUCTION,

   ILO_BUILDER_WRITER_COUNT,
};

enum ilo_builder_item_type {
   /* for state buffer */
   ILO_BUILDER_ITEM_BLOB,
   ILO_BUILDER_ITEM_CLIP_VIEWPORT,
   ILO_BUILDER_ITEM_SF_VIEWPORT,
   ILO_BUILDER_ITEM_SCISSOR_RECT,
   ILO_BUILDER_ITEM_CC_VIEWPORT,
   ILO_BUILDER_ITEM_COLOR_CALC,
   ILO_BUILDER_ITEM_DEPTH_STENCIL,
   ILO_BUILDER_ITEM_BLEND,
   ILO_BUILDER_ITEM_SAMPLER,

   /* for surface buffer */
   ILO_BUILDER_ITEM_SURFACE,
   ILO_BUILDER_ITEM_BINDING_TABLE,

   /* for instruction buffer */
   ILO_BUILDER_ITEM_KERNEL,

   ILO_BUILDER_ITEM_COUNT,
};

struct ilo_builder_item {
   enum ilo_builder_item_type type;
   unsigned offset;
   unsigned size;
};

struct ilo_builder_writer {
   /* internal flags */
   unsigned flags;

   unsigned size;
   struct intel_bo *bo;
   void *ptr;

   /* data written to the bottom */
   unsigned used;
   /* data written to the top */
   unsigned stolen;

   /* for decoding */
   struct ilo_builder_item *items;
   unsigned item_alloc;
   unsigned item_used;
};

/**
 * A snapshot of the writer state.
 */
struct ilo_builder_snapshot {
   unsigned reloc_count;

   unsigned used;
   unsigned stolen;
   unsigned item_used;
};

struct ilo_builder {
   const struct ilo_dev_info *dev;
   struct intel_winsys *winsys;

   struct ilo_builder_writer writers[ILO_BUILDER_WRITER_COUNT];
   bool unrecoverable_error;

   /* for writers that have their data appended */
   unsigned begin_used[ILO_BUILDER_WRITER_COUNT];

   /* for STATE_BASE_ADDRESS */
   unsigned sba_instruction_pos;
};

void
ilo_builder_init(struct ilo_builder *builder,
                 const struct ilo_dev_info *dev,
                 struct intel_winsys *winsys);

void
ilo_builder_reset(struct ilo_builder *builder);

void
ilo_builder_decode(struct ilo_builder *builder);

bool
ilo_builder_begin(struct ilo_builder *builder);

struct intel_bo *
ilo_builder_end(struct ilo_builder *builder, unsigned *used);

bool
ilo_builder_validate(struct ilo_builder *builder,
                     unsigned bo_count, struct intel_bo **bos);

/**
 * Return true if the builder has a relocation entry for \p bo.
 */
static inline bool
ilo_builder_has_reloc(const struct ilo_builder *builder,
                      struct intel_bo *bo)
{
   int i;

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++) {
      const struct ilo_builder_writer *writer = &builder->writers[i];
      if (intel_bo_has_reloc(writer->bo, bo))
         return true;
   }

   return false;
}

void
ilo_builder_writer_discard(struct ilo_builder *builder,
                           enum ilo_builder_writer_type which);

bool
ilo_builder_writer_grow(struct ilo_builder *builder,
                        enum ilo_builder_writer_type which,
                        unsigned new_size, bool preserve);

bool
ilo_builder_writer_record(struct ilo_builder *builder,
                          enum ilo_builder_writer_type which,
                          enum ilo_builder_item_type type,
                          unsigned offset, unsigned size);

static inline void
ilo_builder_writer_checked_record(struct ilo_builder *builder,
                                  enum ilo_builder_writer_type which,
                                  enum ilo_builder_item_type item,
                                  unsigned offset, unsigned size)
{
   if (unlikely(ilo_debug & ILO_DEBUG_BATCH)) {
      if (!ilo_builder_writer_record(builder, which, item, offset, size)) {
         builder->unrecoverable_error = true;
         builder->writers[which].item_used = 0;
      }
   }
}

/**
 * Return an offset to a region that is aligned to \p alignment and has at
 * least \p size bytes.  The region is reserved from the bottom.
 */
static inline unsigned
ilo_builder_writer_reserve_bottom(struct ilo_builder *builder,
                                  enum ilo_builder_writer_type which,
                                  unsigned alignment, unsigned size)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   unsigned offset;

   assert(alignment && util_is_power_of_two(alignment));
   offset = align(writer->used, alignment);

   if (unlikely(offset + size > writer->size - writer->stolen)) {
      if (!ilo_builder_writer_grow(builder, which,
            offset + size + writer->stolen, true)) {
         builder->unrecoverable_error = true;
         ilo_builder_writer_discard(builder, which);
         offset = 0;
      }

      assert(offset + size <= writer->size - writer->stolen);
   }

   return offset;
}

/**
 * Similar to ilo_builder_writer_reserve_bottom(), but reserve from the top.
 */
static inline unsigned
ilo_builder_writer_reserve_top(struct ilo_builder *builder,
                               enum ilo_builder_writer_type which,
                               unsigned alignment, unsigned size)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   unsigned offset;

   assert(alignment && util_is_power_of_two(alignment));
   offset = (writer->size - writer->stolen - size) & ~(alignment - 1);

   if (unlikely(offset < writer->used ||
            size > writer->size - writer->stolen)) {
      if (!ilo_builder_writer_grow(builder, which,
            align(writer->used, alignment) + size + writer->stolen, true)) {
         builder->unrecoverable_error = true;
         ilo_builder_writer_discard(builder, which);
      }

      offset = (writer->size - writer->stolen - size) & ~(alignment - 1);
      assert(offset + size <= writer->size - writer->stolen);
   }

   return offset;
}

/**
 * Add a relocation entry to the writer.
 */
static inline void
ilo_builder_writer_reloc(struct ilo_builder *builder,
                         enum ilo_builder_writer_type which,
                         unsigned offset, struct intel_bo *bo,
                         unsigned bo_offset, unsigned reloc_flags)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   uint64_t presumed_offset;
   int err;

   assert(offset + sizeof(uint32_t) <= writer->used ||
          (offset >= writer->size - writer->stolen &&
           offset + sizeof(uint32_t) <= writer->size));

   err = intel_bo_add_reloc(writer->bo, offset, bo, bo_offset,
         reloc_flags, &presumed_offset);
   if (unlikely(err))
      builder->unrecoverable_error = true;

   /* 32-bit addressing */
   assert(presumed_offset == (uint64_t) ((uint32_t) presumed_offset));
   *((uint32_t *) ((char *) writer->ptr + offset)) = presumed_offset;
}

/**
 * Reserve a region from the state buffer.  Both the offset, in bytes, and the
 * pointer to the reserved region are returned.  The pointer is only valid
 * until the next reserve call.
 *
 * Note that \p alignment is in bytes and \p len is in DWords.
 */
static inline uint32_t
ilo_builder_state_pointer(struct ilo_builder *builder,
                          enum ilo_builder_item_type item,
                          unsigned alignment, unsigned len,
                          uint32_t **dw)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   const unsigned size = len << 2;
   const unsigned offset = ilo_builder_writer_reserve_top(builder,
         which, alignment, size);
   struct ilo_builder_writer *writer = &builder->writers[which];

   /* all states are at least aligned to 32-bytes */
   assert(alignment % 32 == 0);

   *dw = (uint32_t *) ((char *) writer->ptr + offset);

   writer->stolen = writer->size - offset;

   ilo_builder_writer_checked_record(builder, which, item, offset, size);

   return offset;
}

/**
 * Write a dynamic state to the state buffer.
 */
static inline uint32_t
ilo_builder_state_write(struct ilo_builder *builder,
                        enum ilo_builder_item_type item,
                        unsigned alignment, unsigned len,
                        const uint32_t *dw)
{
   uint32_t offset, *dst;

   offset = ilo_builder_state_pointer(builder, item, alignment, len, &dst);
   memcpy(dst, dw, len << 2);

   return offset;
}

/**
 * Write a surface state to the surface buffer.  The offset, in bytes, of the
 * state is returned.
 *
 * Note that \p alignment is in bytes and \p len is in DWords.
 */
static inline uint32_t
ilo_builder_surface_write(struct ilo_builder *builder,
                          enum ilo_builder_item_type item,
                          unsigned alignment, unsigned len,
                          const uint32_t *dw)
{
   assert(item == ILO_BUILDER_ITEM_SURFACE ||
          item == ILO_BUILDER_ITEM_BINDING_TABLE);

   return ilo_builder_state_write(builder, item, alignment, len, dw);
}

/**
 * Add a relocation entry for a DWord of a surface state.
 */
static inline void
ilo_builder_surface_reloc(struct ilo_builder *builder,
                          uint32_t offset, unsigned dw_index,
                          struct intel_bo *bo, unsigned bo_offset,
                          unsigned reloc_flags)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;

   ilo_builder_writer_reloc(builder, which, offset + (dw_index << 2),
         bo, bo_offset, reloc_flags);
}

/**
 * Write a kernel to the instruction buffer.  The offset, in bytes, of the
 * kernel is returned.
 */
static inline uint32_t
ilo_builder_instruction_write(struct ilo_builder *builder,
                              unsigned size, const void *kernel)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_INSTRUCTION;
   /*
    * From the Sandy Bridge PRM, volume 4 part 2, page 112:
    *
    *     "Due to prefetch of the instruction stream, the EUs may attempt to
    *      access up to 8 instructions (128 bytes) beyond the end of the
    *      kernel program - possibly into the next memory page.  Although
    *      these instructions will not be executed, software must account for
    *      the prefetch in order to avoid invalid page access faults."
    */
   const unsigned reserved_size = size + 128;
   /* kernels are aligned to 64 bytes */
   const unsigned alignment = 64;
   const unsigned offset = ilo_builder_writer_reserve_bottom(builder,
         which, alignment, reserved_size);
   struct ilo_builder_writer *writer = &builder->writers[which];

   memcpy((char *) writer->ptr + offset, kernel, size);

   writer->used = offset + size;

   ilo_builder_writer_checked_record(builder, which,
         ILO_BUILDER_ITEM_KERNEL, offset, size);

   return offset;
}

/**
 * Reserve a region from the batch buffer.  Both the offset, in DWords, and
 * the pointer to the reserved region are returned.  The pointer is only valid
 * until the next reserve call.
 *
 * Note that \p len is in DWords.
 */
static inline unsigned
ilo_builder_batch_pointer(struct ilo_builder *builder,
                          unsigned len, uint32_t **dw)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   /*
    * We know the batch bo is always aligned.  Using 1 here should allow the
    * compiler to optimize away aligning.
    */
   const unsigned alignment = 1;
   const unsigned size = len << 2;
   const unsigned offset = ilo_builder_writer_reserve_bottom(builder,
         which, alignment, size);
   struct ilo_builder_writer *writer = &builder->writers[which];

   assert(offset % 4 == 0);
   *dw = (uint32_t *) ((char *) writer->ptr + offset);

   writer->used = offset + size;

   return offset >> 2;
}

/**
 * Write a command to the batch buffer.
 */
static inline unsigned
ilo_builder_batch_write(struct ilo_builder *builder,
                        unsigned len, const uint32_t *dw)
{
   unsigned pos;
   uint32_t *dst;

   pos = ilo_builder_batch_pointer(builder, len, &dst);
   memcpy(dst, dw, len << 2);

   return pos;
}

/**
 * Add a relocation entry for a DWord of a command.
 */
static inline void
ilo_builder_batch_reloc(struct ilo_builder *builder, unsigned pos,
                        struct intel_bo *bo, unsigned bo_offset,
                        unsigned reloc_flags)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;

   ilo_builder_writer_reloc(builder, which, pos << 2,
         bo, bo_offset, reloc_flags);
}

static inline unsigned
ilo_builder_batch_used(const struct ilo_builder *builder)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   const struct ilo_builder_writer *writer = &builder->writers[which];

   return writer->used >> 2;
}

static inline unsigned
ilo_builder_batch_space(const struct ilo_builder *builder)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   const struct ilo_builder_writer *writer = &builder->writers[which];

   return (writer->size - writer->stolen - writer->used) >> 2;
}

static inline void
ilo_builder_batch_discard(struct ilo_builder *builder)
{
   ilo_builder_writer_discard(builder, ILO_BUILDER_WRITER_BATCH);
}

static inline void
ilo_builder_batch_print_stats(const struct ilo_builder *builder)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   const struct ilo_builder_writer *writer = &builder->writers[which];

   ilo_printf("%d+%d bytes (%d%% full)\n",
         writer->used, writer->stolen,
         (writer->used + writer->stolen) * 100 / writer->size);
}

void
ilo_builder_batch_snapshot(const struct ilo_builder *builder,
                           struct ilo_builder_snapshot *snapshot);

void
ilo_builder_batch_restore(struct ilo_builder *builder,
                          const struct ilo_builder_snapshot *snapshot);

#endif /* ILO_BUILDER_H */
