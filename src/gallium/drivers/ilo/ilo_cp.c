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

#include "intel_reg.h" /* for MI_xxx */
#include "intel_winsys.h"

#include "ilo_cp.h"

/* the size of the private space */
static const int ilo_cp_private = 2;

/**
 * Dump the contents of the parser bo.  This must be called in a post-flush
 * hook.
 */
void
ilo_cp_dump(struct ilo_cp *cp)
{
   ilo_printf("dumping %d bytes\n", cp->used * 4);
   if (cp->used)
      cp->winsys->decode_commands(cp->winsys, cp->bo, cp->used * 4);
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
   jmp->reloc_count = cp->bo->get_reloc_count(cp->bo);
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
   cp->bo->clear_relocs(cp->bo, jmp->reloc_count);
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
    * reset above, but also that we added cp->reserve_for_pre_flush and
    * ilo_cp_private to cp->size in ilo_cp_flush().
    */
   cp->size = cp->bo_size - (cp->reserve_for_pre_flush + ilo_cp_private);
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

   err = cp->bo->pwrite(cp->bo, 0, cp->used * 4, cp->ptr);
   if (likely(!err && cp->stolen)) {
      const int offset = cp->bo_size - cp->stolen;

      err = cp->bo->pwrite(cp->bo, offset * 4,
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
   bo = cp->winsys->alloc_buffer(cp->winsys,
         "batch buffer", cp->bo_size * 4, 0);
   if (unlikely(!bo))
      return;

   if (cp->bo)
      cp->bo->unreference(cp->bo);
   cp->bo = bo;

   if (!cp->sys) {
      cp->bo->map(cp->bo, true);
      cp->ptr = cp->bo->get_virtual(cp->bo);
   }
}

/**
 * Execute the parser bo.
 */
static int
ilo_cp_exec_bo(struct ilo_cp *cp)
{
   const bool do_exec = !(ilo_debug & ILO_DEBUG_NOHW);
   unsigned long flags;
   int err;

   switch (cp->ring) {
   case ILO_CP_RING_RENDER:
      flags = INTEL_EXEC_RENDER;
      break;
   case ILO_CP_RING_BLT:
      flags = INTEL_EXEC_BLT;
      break;
   default:
      flags = 0;
      break;
   }

   if (likely(do_exec))
      err = cp->bo->exec(cp->bo, cp->used * 4, cp->hw_ctx, flags);
   else
      err = 0;

   return err;
}

static void
ilo_cp_call_hook(struct ilo_cp *cp, enum ilo_cp_hook hook)
{
   const bool no_implicit_flush = cp->no_implicit_flush;

   if (!cp->hooks[hook].func)
      return;

   /* no implicit flush in hooks */
   cp->no_implicit_flush = true;
   cp->hooks[hook].func(cp, cp->hooks[hook].data);

   cp->no_implicit_flush = no_implicit_flush;
}

/**
 * Flush the command parser and execute the commands.  When the parser buffer
 * is empty, the hooks are not invoked.
 */
void
ilo_cp_flush(struct ilo_cp *cp)
{
   int err;

   /* sanity check */
   assert(cp->bo_size == cp->size +
         cp->reserve_for_pre_flush + ilo_cp_private + cp->stolen);

   if (!cp->used) {
      ilo_cp_clear_buffer(cp);
      return;
   }

   /* make the reserved space available temporarily */
   cp->size += cp->reserve_for_pre_flush;
   ilo_cp_call_hook(cp, ILO_CP_HOOK_PRE_FLUSH);

   ilo_cp_end_buffer(cp);

   if (cp->sys) {
      err = ilo_cp_upload_buffer(cp);
      if (likely(!err))
         err = ilo_cp_exec_bo(cp);
   }
   else {
      cp->bo->unmap(cp->bo);
      err = ilo_cp_exec_bo(cp);
   }

   if (likely(!err)) {
      ilo_cp_call_hook(cp, ILO_CP_HOOK_POST_FLUSH);
      ilo_cp_clear_buffer(cp);
   }
   else {
      /* reset first so that post-flush hook knows nothing was executed */
      ilo_cp_clear_buffer(cp);
      ilo_cp_call_hook(cp, ILO_CP_HOOK_POST_FLUSH);
   }

   ilo_cp_realloc_bo(cp);
   ilo_cp_call_hook(cp, ILO_CP_HOOK_NEW_BATCH);
}

/**
 * Destroy the command parser.
 */
void
ilo_cp_destroy(struct ilo_cp *cp)
{
   if (cp->bo)
      cp->bo->unreference(cp->bo);
   if (cp->hw_ctx)
      cp->winsys->destroy_context(cp->winsys, cp->hw_ctx);

   FREE(cp->sys);
   FREE(cp);
}

/**
 * Create a command parser.
 */
struct ilo_cp *
ilo_cp_create(struct intel_winsys *winsys, bool direct_map)
{
   struct ilo_cp *cp;

   cp = CALLOC_STRUCT(ilo_cp);
   if (!cp)
      return NULL;

   cp->winsys = winsys;
   cp->hw_ctx = winsys->create_context(winsys);

   cp->ring = ILO_CP_RING_RENDER;
   cp->no_implicit_flush = false;
   cp->reserve_for_pre_flush = 0;

   memset(cp->hooks, 0, sizeof(cp->hooks));

   cp->bo_size = 8192;

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
