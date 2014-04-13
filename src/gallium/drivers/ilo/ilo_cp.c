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

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_cp.h"

#define MI_NOOP             GEN_MI_CMD(MI_NOOP)
#define MI_BATCH_BUFFER_END GEN_MI_CMD(MI_BATCH_BUFFER_END)

/* the size of the private space */
static const int ilo_cp_private = 2;

/**
 * Dump the contents of the parser bo.  This can only be called in the flush
 * callback.
 */
void
ilo_cp_dump(struct ilo_cp *cp)
{
   ilo_printf("dumping %d bytes\n", cp->used * 4);
   if (cp->used)
      intel_winsys_decode_bo(cp->winsys, cp->bo, cp->used * 4);
}

/**
 * Save the command parser state for rewind.
 *
 * Note that this cannot rewind a flush, and the caller must make sure
 * that does not happend.
 */
void
ilo_cp_setjmp(struct ilo_cp *cp, struct ilo_cp_jmp_buf *jmp)
{
   jmp->id = pointer_to_intptr(cp->bo);

   jmp->size = cp->size;
   jmp->used = cp->used;
   jmp->stolen = cp->stolen;
   /* save reloc count to rewind ilo_cp_write_bo() */
   jmp->reloc_count = intel_bo_get_reloc_count(cp->bo);
}

/**
 * Rewind to the saved state.
 */
void
ilo_cp_longjmp(struct ilo_cp *cp, const struct ilo_cp_jmp_buf *jmp)
{
   if (jmp->id != pointer_to_intptr(cp->bo)) {
      assert(!"invalid use of CP longjmp");
      return;
   }

   cp->size = jmp->size;
   cp->used = jmp->used;
   cp->stolen = jmp->stolen;
   intel_bo_truncate_relocs(cp->bo, jmp->reloc_count);
}

/**
 * Clear the parser buffer.
 */
static void
ilo_cp_clear_buffer(struct ilo_cp *cp)
{
   cp->cmd_cur = 0;
   cp->cmd_end = 0;

   cp->used = 0;
   cp->stolen = 0;

   /*
    * Recalculate cp->size.  This is needed not only because cp->stolen is
    * reset above, but also that ilo_cp_private are added to cp->size in
    * ilo_cp_end_buffer().
    */
   cp->size = cp->bo_size - ilo_cp_private;
}

/**
 * Add MI_BATCH_BUFFER_END to the private space of the parser buffer.
 */
static void
ilo_cp_end_buffer(struct ilo_cp *cp)
{
   /* make the private space available */
   cp->size += ilo_cp_private;

   assert(cp->used + 2 <= cp->size);

   cp->ptr[cp->used++] = MI_BATCH_BUFFER_END;

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 107:
    *
    *     "The batch buffer must be QWord aligned and a multiple of QWords in
    *      length."
    */
   if (cp->used & 1)
      cp->ptr[cp->used++] = MI_NOOP;
}

/**
 * Upload the parser buffer to the bo.
 */
static int
ilo_cp_upload_buffer(struct ilo_cp *cp)
{
   int err;

   if (!cp->sys) {
      intel_bo_unmap(cp->bo);
      return 0;
   }

   err = intel_bo_pwrite(cp->bo, 0, cp->used * 4, cp->ptr);
   if (likely(!err && cp->stolen)) {
      const int offset = cp->bo_size - cp->stolen;

      err = intel_bo_pwrite(cp->bo, offset * 4,
            cp->stolen * 4, &cp->ptr[offset]);
   }

   return err;
}

/**
 * Reallocate the parser bo.
 */
static void
ilo_cp_realloc_bo(struct ilo_cp *cp)
{
   struct intel_bo *bo;

   /*
    * allocate the new bo before unreferencing the old one so that they
    * won't point at the same address, which is needed for jmpbuf
    */
   bo = intel_winsys_alloc_buffer(cp->winsys,
         "batch buffer", cp->bo_size * 4, INTEL_DOMAIN_CPU);
   if (unlikely(!bo)) {
      /* reuse the old one */
      bo = cp->bo;
      intel_bo_reference(bo);
   }

   if (cp->bo)
      intel_bo_unreference(cp->bo);
   cp->bo = bo;

   if (!cp->sys)
      cp->ptr = intel_bo_map(cp->bo, true);
}

/**
 * Execute the parser bo.
 */
static int
ilo_cp_exec_bo(struct ilo_cp *cp)
{
   const bool do_exec = !(ilo_debug & ILO_DEBUG_NOHW);
   int err;

   if (likely(do_exec)) {
      err = intel_winsys_submit_bo(cp->winsys, cp->ring,
            cp->bo, cp->used * 4, cp->render_ctx, cp->one_off_flags);
   }
   else {
      err = 0;
   }

   cp->one_off_flags = 0;

   return err;
}

/**
 * Flush the command parser and execute the commands.  When the parser buffer
 * is empty, the callback is not invoked.
 */
void
ilo_cp_flush_internal(struct ilo_cp *cp)
{
   int err;

   ilo_cp_set_owner(cp, NULL, 0);

   /* sanity check */
   assert(cp->bo_size == cp->size + cp->stolen + ilo_cp_private);

   if (!cp->used) {
      /* return the space stolen and etc. */
      ilo_cp_clear_buffer(cp);

      return;
   }

   ilo_cp_end_buffer(cp);

   /* upload and execute */
   err = ilo_cp_upload_buffer(cp);
   if (likely(!err))
      err = ilo_cp_exec_bo(cp);

   if (likely(!err && cp->flush_callback))
      cp->flush_callback(cp, cp->flush_callback_data);

   ilo_cp_clear_buffer(cp);
   ilo_cp_realloc_bo(cp);
}

/**
 * Destroy the command parser.
 */
void
ilo_cp_destroy(struct ilo_cp *cp)
{
   if (cp->bo) {
      if (!cp->sys)
         intel_bo_unmap(cp->bo);

      intel_bo_unreference(cp->bo);
   }

   intel_winsys_destroy_context(cp->winsys, cp->render_ctx);

   FREE(cp->sys);
   FREE(cp);
}

/**
 * Create a command parser.
 */
struct ilo_cp *
ilo_cp_create(struct intel_winsys *winsys, int size, bool direct_map)
{
   struct ilo_cp *cp;

   cp = CALLOC_STRUCT(ilo_cp);
   if (!cp)
      return NULL;

   cp->winsys = winsys;
   cp->render_ctx = intel_winsys_create_context(winsys);
   if (!cp->render_ctx) {
      FREE(cp);
      return NULL;
   }

   cp->ring = INTEL_RING_RENDER;
   cp->no_implicit_flush = false;

   cp->bo_size = size;

   if (!direct_map) {
      cp->sys = MALLOC(cp->bo_size * 4);
      if (!cp->sys) {
         FREE(cp);
         return NULL;
      }

      cp->ptr = cp->sys;
   }

   ilo_cp_realloc_bo(cp);
   if (!cp->bo) {
      FREE(cp->sys);
      FREE(cp);
      return NULL;
   }

   ilo_cp_clear_buffer(cp);

   return cp;
}
