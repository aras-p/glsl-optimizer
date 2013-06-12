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

#ifndef INTEL_WINSYS_H
#define INTEL_WINSYS_H

#include "pipe/p_compiler.h"

/* this is compatible with i915_drm.h's definitions */
enum intel_exec_flag {
   /* bits[2:0]: ring type */
   INTEL_EXEC_DEFAULT         = 0 << 0,
   INTEL_EXEC_RENDER          = 1 << 0,
   INTEL_EXEC_BSD             = 2 << 0,
   INTEL_EXEC_BLT             = 3 << 0,

   /* bits[7:6]: constant buffer addressing mode */

   /* bits[8]: reset SO write offset register on GEN7+ */
   INTEL_EXEC_GEN7_SOL_RESET  = 1 << 8,
};

/* this is compatible with i915_drm.h's definitions */
enum intel_domain_flag {
   INTEL_DOMAIN_CPU           = 0x00000001,
   INTEL_DOMAIN_RENDER        = 0x00000002,
   INTEL_DOMAIN_SAMPLER       = 0x00000004,
   INTEL_DOMAIN_COMMAND	      = 0x00000008,
   INTEL_DOMAIN_INSTRUCTION   = 0x00000010,
   INTEL_DOMAIN_VERTEX        = 0x00000020,
   INTEL_DOMAIN_GTT           = 0x00000040,
};

/* this is compatible with i915_drm.h's definitions */
enum intel_tiling_mode {
   INTEL_TILING_NONE = 0,
   INTEL_TILING_X    = 1,
   INTEL_TILING_Y    = 2,
};

/* this is compatible with intel_bufmgr.h's definitions */
enum intel_alloc_flag {
   INTEL_ALLOC_FOR_RENDER     = 1 << 0,
};

struct winsys_handle;
struct intel_winsys;
struct intel_context;
struct intel_bo;

struct intel_winsys_info {
   int devid;
   bool has_llc;
   bool has_gen7_sol_reset;
   bool has_address_swizzling;
};

struct intel_winsys *
intel_winsys_create_for_fd(int fd);

void
intel_winsys_destroy(struct intel_winsys *winsys);

const struct intel_winsys_info *
intel_winsys_get_info(const struct intel_winsys *winsys);

void
intel_winsys_enable_reuse(struct intel_winsys *winsys);

struct intel_context *
intel_winsys_create_context(struct intel_winsys *winsys);

void
intel_winsys_destroy_context(struct intel_winsys *winsys,
                             struct intel_context *ctx);

int
intel_winsys_read_reg(struct intel_winsys *winsys,
                      uint32_t reg, uint64_t *val);

struct intel_bo *
intel_winsys_alloc_buffer(struct intel_winsys *winsys,
                          const char *name,
                          unsigned long size,
                          unsigned long flags);

struct intel_bo *
intel_winsys_alloc_texture(struct intel_winsys *winsys,
                           const char *name,
                           int width, int height, int cpp,
                           enum intel_tiling_mode tiling,
                           unsigned long flags,
                           unsigned long *pitch);

struct intel_bo *
intel_winsys_import_handle(struct intel_winsys *winsys,
                           const char *name,
                           const struct winsys_handle *handle,
                           int width, int height, int cpp,
                           enum intel_tiling_mode *tiling,
                           unsigned long *pitch);

/**
 * Export a handle for inter-process sharing.
 */
int
intel_winsys_export_handle(struct intel_winsys *winsys,
                           struct intel_bo *bo,
                           enum intel_tiling_mode tiling,
                           unsigned long pitch,
                           struct winsys_handle *handle);

int
intel_winsys_check_aperture_space(struct intel_winsys *winsys,
                                  struct intel_bo **bo_array,
                                  int count);

void
intel_winsys_decode_commands(struct intel_winsys *winsys,
                             struct intel_bo *bo, int used);

void
intel_bo_reference(struct intel_bo *bo);

void
intel_bo_unreference(struct intel_bo *bo);

unsigned long
intel_bo_get_size(const struct intel_bo *bo);

unsigned long
intel_bo_get_offset(const struct intel_bo *bo);

void *
intel_bo_get_virtual(const struct intel_bo *bo);

/**
 * Map/unmap \p bo for CPU access.
 *
 * map() maps the backing store into CPU address space, cached.  This
 * variant allows for fast random reads and writes.  But the caller needs
 * handle tiling or swizzling manually if the bo is tiled or swizzled.  If
 * write is enabled and there is no shared last-level cache (LLC), unmap()
 * needs to flush the cache, which is rather expensive.
 *
 * map_gtt() maps the bo for MMIO access, uncached but write-combined.
 * This variant promises a reasonable speed for sequential writes, but
 * reads would be very slow.  Callers always have a linear view of the bo.
 *
 * map_unsynchronized() is similar to map_gtt(), except that it does not
 * wait until the bo is idle.
 */
int
intel_bo_map(struct intel_bo *bo, bool write_enable);

int
intel_bo_map_gtt(struct intel_bo *bo);

int
intel_bo_map_unsynchronized(struct intel_bo *bo);

void
intel_bo_unmap(struct intel_bo *bo);

/**
 * Move data in to or out of the bo.
 */
int
intel_bo_pwrite(struct intel_bo *bo, unsigned long offset,
                unsigned long size, const void *data);
int
intel_bo_pread(struct intel_bo *bo, unsigned long offset,
               unsigned long size, void *data);

/**
 * Add \p target_bo to the relocation list.
 *
 * When \p bo is submitted for execution, and if \p target_bo has moved,
 * the kernel will patch \p bo at \p offset to \p target_bo->offset plus
 * \p target_offset.
 */
int
intel_bo_emit_reloc(struct intel_bo *bo, uint32_t offset,
                    struct intel_bo *target_bo, uint32_t target_offset,
                    uint32_t read_domains, uint32_t write_domain);

/**
 * Return the current number of relocations.
 */
int
intel_bo_get_reloc_count(struct intel_bo *bo);

/**
 * Discard all relocations except the first \p start ones.
 *
 * Combined with \p get_reloc_count(), they can be used to undo
 * the \p emit_reloc() calls that were just made.
 */
void
intel_bo_clear_relocs(struct intel_bo *bo, int start);

/**
 * Return true if \p target_bo is on the relocation list of \p bo, or on
 * the relocation list of some bo that is referenced by \p bo.
 */
bool
intel_bo_references(struct intel_bo *bo, struct intel_bo *target_bo);

/**
 * Submit \p bo for execution.
 *
 * \p bo and all bos referenced by \p bo will be considered busy until all
 * commands are parsed and executed.
 */
int
intel_bo_exec(struct intel_bo *bo, int used,
              struct intel_context *ctx, unsigned long flags);

/**
 * Wait until \bo is idle, or \p timeout nanoseconds have passed.  A
 * negative timeout means to wait indefinitely.
 *
 * \return 0 only when \p bo is idle
 */
int
intel_bo_wait(struct intel_bo *bo, int64_t timeout);

/**
 * Return true if \p bo is busy.
 */
static inline bool
intel_bo_is_busy(struct intel_bo *bo)
{
   return (intel_bo_wait(bo, 0) != 0);
}

#endif /* INTEL_WINSYS_H */
