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

#include "ilo_builder.h"
#include "ilo_builder_render.h" /* for ilo_builder_batch_patch_sba() */

enum ilo_builder_writer_flags {
   /*
    * When this bit is set, ilo_builder_begin() will not realllocate.  New
    * data will be appended instead.
    */
   WRITER_FLAG_APPEND    = 1 << 0,

   /*
    * When this bit is set, the writer grows when full.  When not, callers
    * must make sure the writer never needs to grow.
    */
   WRITER_FLAG_GROW      = 1 << 1,

   /*
    * The writer will be mapped directly.
    */
   WRITER_FLAG_MAP       = 1 << 2,
};

/**
 * Set the initial size and flags of a writer.
 */
static void
ilo_builder_writer_init(struct ilo_builder *builder,
                        enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];

   switch (which) {
   case ILO_BUILDER_WRITER_BATCH:
      writer->size = sizeof(uint32_t) * 8192;
      break;
   case ILO_BUILDER_WRITER_INSTRUCTION:
      /*
       * The EUs pretch some instructions.  But since the kernel invalidates
       * the instruction cache between batch buffers, we can set
       * WRITER_FLAG_APPEND without worrying the EUs would see invalid
       * instructions prefetched.
       */
      writer->flags = WRITER_FLAG_APPEND | WRITER_FLAG_GROW;
      writer->size = 8192;
      break;
   default:
      assert(!"unknown builder writer");
      return;
      break;
   }

   if (builder->dev->has_llc)
      writer->flags |= WRITER_FLAG_MAP;
}

/**
 * Free all resources used by a writer.  Note that the initial size is not
 * reset.
 */
static void
ilo_builder_writer_reset(struct ilo_builder *builder,
                         enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];

   if (writer->ptr) {
      if (writer->flags & WRITER_FLAG_MAP)
         intel_bo_unmap(writer->bo);
      else
         FREE(writer->ptr);

      writer->ptr = NULL;
   }

   if (writer->bo) {
      intel_bo_unreference(writer->bo);
      writer->bo = NULL;
   }

   writer->used = 0;
   writer->stolen = 0;

   if (writer->items) {
      FREE(writer->items);
      writer->item_alloc = 0;
      writer->item_used = 0;
   }
}

/**
 * Discard everything written so far.
 */
void
ilo_builder_writer_discard(struct ilo_builder *builder,
                           enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];

   intel_bo_truncate_relocs(writer->bo, 0);
   writer->used = 0;
   writer->stolen = 0;
   writer->item_used = 0;
}

static struct intel_bo *
alloc_writer_bo(struct intel_winsys *winsys,
                enum ilo_builder_writer_type which,
                unsigned size)
{
   static const char *writer_names[ILO_BUILDER_WRITER_COUNT] = {
      [ILO_BUILDER_WRITER_BATCH] = "batch",
      [ILO_BUILDER_WRITER_INSTRUCTION] = "instruction",
   };

   return intel_winsys_alloc_buffer(winsys, writer_names[which], size, true);
}

static void *
map_writer_bo(struct intel_bo *bo, unsigned flags)
{
   assert(flags & WRITER_FLAG_MAP);

   if (flags & WRITER_FLAG_APPEND)
      return intel_bo_map_gtt_async(bo);
   else
      return intel_bo_map(bo, true);
}

/**
 * Allocate and map the buffer for writing.
 */
static bool
ilo_builder_writer_alloc_and_map(struct ilo_builder *builder,
                                 enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];

   /* allocate a new bo when not appending */
   if (!(writer->flags & WRITER_FLAG_APPEND) || !writer->bo) {
      struct intel_bo *bo;

      bo = alloc_writer_bo(builder->winsys, which, writer->size);
      if (bo) {
         if (writer->bo)
            intel_bo_unreference(writer->bo);
         writer->bo = bo;
      } else if (writer->bo) {
         /* reuse the old bo */
         ilo_builder_writer_discard(builder, which);
      } else {
         return false;
      }

      writer->used = 0;
      writer->stolen = 0;
      writer->item_used = 0;
   }

   /* map the bo or allocate the staging system memory */
   if (writer->flags & WRITER_FLAG_MAP)
      writer->ptr = map_writer_bo(writer->bo, writer->flags);
   else if (!writer->ptr)
      writer->ptr = MALLOC(writer->size);

   return (writer->ptr != NULL);
}

/**
 * Unmap the buffer for submission.
 */
static bool
ilo_builder_writer_unmap(struct ilo_builder *builder,
                         enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   unsigned offset;
   int err = 0;

   if (writer->flags & WRITER_FLAG_MAP) {
      intel_bo_unmap(writer->bo);
      writer->ptr = NULL;
      return true;
   }

   offset = builder->begin_used[which];
   if (writer->used > offset) {
      err = intel_bo_pwrite(writer->bo, offset, writer->used - offset,
            (char *) writer->ptr + offset);
   }

   if (writer->stolen && !err) {
      const unsigned offset = writer->size - writer->stolen;
      err = intel_bo_pwrite(writer->bo, offset, writer->stolen,
            (const char *) writer->ptr + offset);
   }

   /* keep writer->ptr */

   return !err;
}

/**
 * Grow a mapped writer to at least \p new_size.
 */
bool
ilo_builder_writer_grow(struct ilo_builder *builder,
                        enum ilo_builder_writer_type which,
                        unsigned new_size, bool preserve)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   struct intel_bo *new_bo;
   void *new_ptr;

   if (!(writer->flags & WRITER_FLAG_GROW))
      return false;

   /* stolen data may already be referenced and cannot be moved */
   if (writer->stolen)
      return false;

   if (new_size < writer->size << 1)
      new_size = writer->size << 1;
   /* STATE_BASE_ADDRESS requires page-aligned buffers */
   new_size = align(new_size, 4096);

   new_bo = alloc_writer_bo(builder->winsys, which, new_size);
   if (!new_bo)
      return false;

   /* map and copy the data over */
   if (writer->flags & WRITER_FLAG_MAP) {
      new_ptr = map_writer_bo(new_bo, writer->flags);

      /*
       * When WRITER_FLAG_APPEND and WRITER_FLAG_GROW are both set, we may end
       * up copying between two GTT-mapped BOs.  That is slow.  The issue
       * could be solved by adding intel_bo_map_async(), or callers may choose
       * to manually grow the writer without preserving the data.
       */
      if (new_ptr && preserve)
         memcpy(new_ptr, writer->ptr, writer->used);
   } else if (preserve) {
      new_ptr = REALLOC(writer->ptr, writer->size, new_size);
   } else {
      new_ptr = MALLOC(new_size);
   }

   if (!new_ptr) {
      intel_bo_unreference(new_bo);
      return false;
   }

   if (writer->flags & WRITER_FLAG_MAP)
      intel_bo_unmap(writer->bo);
   else if (!preserve)
      FREE(writer->ptr);

   intel_bo_unreference(writer->bo);

   writer->size = new_size;
   writer->bo = new_bo;
   writer->ptr = new_ptr;

   return true;
}

/**
 * Record an item for later decoding.
 */
bool
ilo_builder_writer_record(struct ilo_builder *builder,
                          enum ilo_builder_writer_type which,
                          enum ilo_builder_item_type type,
                          unsigned offset, unsigned size)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   struct ilo_builder_item *item;

   if (writer->item_used == writer->item_alloc) {
      const unsigned new_alloc = (writer->item_alloc) ?
         writer->item_alloc << 1 : 256;
      struct ilo_builder_item *items;

      items = REALLOC(writer->items,
            sizeof(writer->items[0]) * writer->item_alloc,
            sizeof(writer->items[0]) * new_alloc);
      if (!items)
         return false;

      writer->items = items;
      writer->item_alloc = new_alloc;
   }

   item = &writer->items[writer->item_used++];
   item->type = type;
   item->offset = offset;
   item->size = size;

   return true;
}

/**
 * Initialize the builder.
 */
void
ilo_builder_init(struct ilo_builder *builder,
                 const struct ilo_dev_info *dev,
                 struct intel_winsys *winsys)
{
   int i;

   memset(builder, 0, sizeof(*builder));

   builder->dev = dev;
   builder->winsys = winsys;

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++)
      ilo_builder_writer_init(builder, i);
}

/**
 * Reset the builder and free all resources used.  After resetting, the
 * builder behaves as if it is newly initialized, except for potentially
 * larger initial bo sizes.
 */
void
ilo_builder_reset(struct ilo_builder *builder)
{
   int i;

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++)
      ilo_builder_writer_reset(builder, i);
}

/**
 * Allocate and map the BOs.  It may re-allocate or reuse existing BOs if
 * there is any.
 *
 * Most builder functions can only be called after ilo_builder_begin() and
 * before ilo_builder_end().
 */
bool
ilo_builder_begin(struct ilo_builder *builder)
{
   int i;

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++) {
      if (!ilo_builder_writer_alloc_and_map(builder, i)) {
         ilo_builder_reset(builder);
         return false;
      }

      builder->begin_used[i] = builder->writers[i].used;
   }

   builder->unrecoverable_error = false;
   builder->sba_instruction_pos = 0;

   return true;
}

/**
 * Unmap BOs and make sure the written data landed the BOs.  The batch buffer
 * ready for submission is returned.
 */
struct intel_bo *
ilo_builder_end(struct ilo_builder *builder, unsigned *used)
{
   struct ilo_builder_writer *bat;
   int i;

   ilo_builder_batch_patch_sba(builder);

   assert(ilo_builder_validate(builder, 0, NULL));

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++) {
      if (!ilo_builder_writer_unmap(builder, i))
         builder->unrecoverable_error = true;
   }

   if (builder->unrecoverable_error)
      return NULL;

   bat = &builder->writers[ILO_BUILDER_WRITER_BATCH];

   *used = bat->used;

   return bat->bo;
}

/**
 * Return true if the builder is in a valid state, after accounting for the
 * additional BOs specified.  The additional BOs can be listed to avoid
 * snapshotting and restoring when they are known ahead of time.
 *
 * The number of additional BOs should not be more than a few.  Like two, for
 * copying between two BOs.
 *
 * Callers must make sure the builder is in a valid state when
 * ilo_builder_end() is called.
 */
bool
ilo_builder_validate(struct ilo_builder *builder,
                     unsigned bo_count, struct intel_bo **bos)
{
   const unsigned max_bo_count = 2;
   struct intel_bo *bos_to_submit[ILO_BUILDER_WRITER_COUNT + max_bo_count];
   int i;

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++)
      bos_to_submit[i] = builder->writers[i].bo;

   if (bo_count) {
      assert(bo_count <= max_bo_count);
      if (bo_count > max_bo_count)
         return false;

      memcpy(&bos_to_submit[ILO_BUILDER_WRITER_COUNT],
            bos, sizeof(*bos) * bo_count);
      i += bo_count;
   }

   return intel_winsys_can_submit_bo(builder->winsys, bos_to_submit, i);
}

/**
 * Take a snapshot of the writer state.
 */
void
ilo_builder_batch_snapshot(const struct ilo_builder *builder,
                           struct ilo_builder_snapshot *snapshot)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   const struct ilo_builder_writer *writer = &builder->writers[which];

   snapshot->reloc_count = intel_bo_get_reloc_count(writer->bo);
   snapshot->used = writer->used;
   snapshot->stolen = writer->stolen;
   snapshot->item_used = writer->item_used;
}

/**
 * Restore the writer state to when the snapshot was taken, except that it
 * does not (unnecessarily) shrink BOs or the item array.
 */
void
ilo_builder_batch_restore(struct ilo_builder *builder,
                          const struct ilo_builder_snapshot *snapshot)
{
   const enum ilo_builder_writer_type which = ILO_BUILDER_WRITER_BATCH;
   struct ilo_builder_writer *writer = &builder->writers[which];

   intel_bo_truncate_relocs(writer->bo, snapshot->reloc_count);
   writer->used = snapshot->used;
   writer->stolen = snapshot->stolen;
   writer->item_used = snapshot->item_used;
}
