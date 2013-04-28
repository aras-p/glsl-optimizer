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

#include "ilo_common.h"

struct ilo_cp;

enum ilo_cp_ring {
   ILO_CP_RING_RENDER,
   ILO_CP_RING_BLT,

   ILO_CP_RING_COUNT,
};

enum ilo_cp_hook {
   ILO_CP_HOOK_NEW_BATCH,
   ILO_CP_HOOK_PRE_FLUSH,
   ILO_CP_HOOK_POST_FLUSH,

   ILO_CP_HOOK_COUNT,
};

typedef void (*ilo_cp_hook_func)(struct ilo_cp *cp, void *data);

/**
 * Command parser.
 */
struct ilo_cp {
   struct intel_winsys *winsys;
   struct intel_context *hw_ctx;

   enum ilo_cp_ring ring;
   bool no_implicit_flush;
   int reserve_for_pre_flush;
   unsigned one_off_flags;

   struct {
      ilo_cp_hook_func func;
      void *data;
   } hooks[ILO_CP_HOOK_COUNT];

   int bo_size;
   struct intel_bo *bo;
   uint32_t *sys;

   uint32_t *ptr;
   int size, used, stolen;

   int cmd_cur, cmd_end;
};

/**
 * Jump buffer to save command parser state for rewind.
 */
struct ilo_cp_jmp_buf {
   intptr_t id;
   int size, used, stolen;
   int reloc_count;
};

struct ilo_cp *
ilo_cp_create(struct intel_winsys *winsys, bool direct_map);

void
ilo_cp_destroy(struct ilo_cp *cp);

void
ilo_cp_flush(struct ilo_cp *cp);

void
ilo_cp_dump(struct ilo_cp *cp);

void
ilo_cp_setjmp(struct ilo_cp *cp, struct ilo_cp_jmp_buf *jmp);

void
ilo_cp_longjmp(struct ilo_cp *cp, const struct ilo_cp_jmp_buf *jmp);

/**
 * Return true if the parser buffer is empty.
 */
static inline bool
ilo_cp_empty(struct ilo_cp *cp)
{
   return !cp->used;
}

/**
 * Return the remaining space (in dwords) in the parser buffer.
 */
static inline int
ilo_cp_space(struct ilo_cp *cp)
{
   return cp->size - cp->used;
}

/**
 * Internal function called by functions that flush implicitly.
 */
static inline void
ilo_cp_implicit_flush(struct ilo_cp *cp)
{
   if (cp->no_implicit_flush) {
      assert(!"unexpected command parser flush");
      /* discard the commands */
      cp->used = 0;
   }

   ilo_cp_flush(cp);
}

/**
 * Set the ring buffer.
 */
static inline void
ilo_cp_set_ring(struct ilo_cp *cp, enum ilo_cp_ring ring)
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
 * Reserve the given size of space from the parser buffer.  The reserved space
 * will be made available temporarily for the pre-flush hook.
 *
 * \param reserve size in dwords to reserve.  It may be negative.
 */
static inline void
ilo_cp_reserve_for_pre_flush(struct ilo_cp *cp, int reserve)
{
   assert(cp->reserve_for_pre_flush + reserve >= 0);

   if (cp->used > cp->size - reserve) {
      ilo_cp_implicit_flush(cp);
      assert(cp->used <= cp->size - reserve);
   }

   cp->size -= reserve;
   cp->reserve_for_pre_flush += reserve;
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
 * Set a command parser hook.
 */
static inline void
ilo_cp_set_hook(struct ilo_cp *cp, enum ilo_cp_hook hook,
                ilo_cp_hook_func func, void *data)
{
   cp->hooks[hook].func = func;
   cp->hooks[hook].data = data;
}

/**
 * Begin writing a command.
 */
static inline void
ilo_cp_begin(struct ilo_cp *cp, int cmd_size)
{
   if (cp->used + cmd_size > cp->size) {
      ilo_cp_implicit_flush(cp);
      assert(cp->used + cmd_size <= cp->size);
   }

   assert(cp->cmd_cur == cp->cmd_end);
   cp->cmd_cur = cp->used;
   cp->cmd_end = cp->cmd_cur + cmd_size;
   cp->used = cp->cmd_end;
}

/**
 * Begin writing data to a space stolen from the top of the parser buffer.
 *
 * \param desc informative description of the data to be written
 * \param data_size in dwords
 * \param align in dwords
 * \param bo_offset in bytes to the stolen space
 */
static inline void
ilo_cp_steal(struct ilo_cp *cp, const char *desc,
             int data_size, int align, uint32_t *bo_offset)
{
   int pad, steal;

   if (!align)
      align = 1;

   pad = (cp->bo_size - cp->stolen - data_size) % align;
   steal = data_size + pad;

   /* flush if there is not enough space after stealing */
   if (cp->used > cp->size - steal) {
      ilo_cp_implicit_flush(cp);

      pad = (cp->bo_size - cp->stolen - data_size) % align;
      steal = data_size + steal;

      assert(cp->used <= cp->size - steal);
   }

   cp->size -= steal;
   cp->stolen += steal;

   assert(cp->cmd_cur == cp->cmd_end);
   cp->cmd_cur = cp->bo_size - cp->stolen;
   cp->cmd_end = cp->cmd_cur + data_size;

   /* offset in cp->bo */
   if (bo_offset)
      *bo_offset = cp->cmd_cur * 4;
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
ilo_cp_write_bo(struct ilo_cp *cp, uint32_t val, struct intel_bo *bo,
                uint32_t read_domains, uint32_t write_domain)
{
   if (bo) {
      cp->bo->emit_reloc(cp->bo, cp->cmd_cur * 4,
            bo, val, read_domains, write_domain);

      ilo_cp_write(cp, val + bo->get_offset(bo));
   }
   else {
      ilo_cp_write(cp, val);
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
ilo_cp_steal_ptr(struct ilo_cp *cp, const char *desc,
                 int data_size, int align, uint32_t *bo_offset)
{
   void *ptr;

   ilo_cp_steal(cp, desc, data_size, align, bo_offset);

   ptr = &cp->ptr[cp->cmd_cur];
   cp->cmd_cur = cp->cmd_end;

   ilo_cp_end(cp);

   return ptr;
}

#endif /* ILO_CP_H */
