/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2014 LunarG, Inc.
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
enum intel_ring_type {
   INTEL_RING_RENDER    = 1,
   INTEL_RING_BSD       = 2,
   INTEL_RING_BLT       = 3,
   INTEL_RING_VEBOX     = 4,
};

/* this is compatible with i915_drm.h's definitions */
enum intel_exec_flag {
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

struct winsys_handle;
struct intel_winsys;
struct intel_context;
struct intel_bo;

struct intel_winsys_info {
   int devid;

   int max_batch_size;
   bool has_llc;
   bool has_address_swizzling;
   bool has_logical_context;
   bool has_ppgtt;

   /* valid registers for intel_winsys_read_reg() */
   bool has_timestamp;

   /* valid flags for intel_winsys_submit_bo() */
   bool has_gen7_sol_reset;
};

struct intel_winsys *
intel_winsys_create_for_fd(int fd);

void
intel_winsys_destroy(struct intel_winsys *winsys);

const struct intel_winsys_info *
intel_winsys_get_info(const struct intel_winsys *winsys);

/**
 * Create a logical context for use with the render ring.
 */
struct intel_context *
intel_winsys_create_context(struct intel_winsys *winsys);

/**
 * Destroy a logical context.
 */
void
intel_winsys_destroy_context(struct intel_winsys *winsys,
                             struct intel_context *ctx);

/**
 * Read a register.  Only registers that are considered safe, such as
 *
 *   TIMESTAMP (0x2358)
 *
 * can be read.
 */
int
intel_winsys_read_reg(struct intel_winsys *winsys,
                      uint32_t reg, uint64_t *val);

/**
 * Allocate a linear buffer object.
 *
 * \param name             Informative description of the bo.
 * \param size             Size of the bo.
 * \param initial_domain   Initial (write) domain.
 */
struct intel_bo *
intel_winsys_alloc_buffer(struct intel_winsys *winsys,
                          const char *name,
                          unsigned long size,
                          uint32_t initial_domain);

/**
 * Allocate a 2-dimentional buffer object.
 *
 * \param name             Informative description of the bo.
 * \param width            Width of the bo.
 * \param height           Height of the bo.
 * \param cpp              Bytes per texel.
 * \param tiling           Tiling mode.
 * \param initial_domain   Initial (write) domain.
 * \param pitch            Pitch of the bo.
 */
struct intel_bo *
intel_winsys_alloc_texture(struct intel_winsys *winsys,
                           const char *name,
                           int width, int height, int cpp,
                           enum intel_tiling_mode tiling,
                           uint32_t initial_domain,
                           unsigned long *pitch);

/**
 * Create a bo from a winsys handle.
 */
struct intel_bo *
intel_winsys_import_handle(struct intel_winsys *winsys,
                           const char *name,
                           const struct winsys_handle *handle,
                           int width, int height, int cpp,
                           enum intel_tiling_mode *tiling,
                           unsigned long *pitch);

/**
 * Export \p bo as a winsys handle for inter-process sharing.
 */
int
intel_winsys_export_handle(struct intel_winsys *winsys,
                           struct intel_bo *bo,
                           enum intel_tiling_mode tiling,
                           unsigned long pitch,
                           struct winsys_handle *handle);

/**
 * Return true when buffer objects directly specified in \p bo_array, and
 * those indirectly referenced by them, can fit in the aperture space.
 */
bool
intel_winsys_can_submit_bo(struct intel_winsys *winsys,
                           struct intel_bo **bo_array,
                           int count);

/**
 * Submit \p bo for execution.
 *
 * \p bo and all bos referenced by \p bo will be considered busy until all
 * commands are parsed and executed.  \p ctx is ignored when the bo is not
 * submitted to the render ring.
 */
int
intel_winsys_submit_bo(struct intel_winsys *winsys,
                       enum intel_ring_type ring,
                       struct intel_bo *bo, int used,
                       struct intel_context *ctx,
                       unsigned long flags);

/**
 * Decode the commands contained in \p bo.  For debugging.
 *
 * \param bo      Batch buffer to decode.
 * \param used    Size of the commands in bytes.
 */
void
intel_winsys_decode_bo(struct intel_winsys *winsys,
                       struct intel_bo *bo, int used);

/**
 * Increase the reference count of \p bo.
 */
void
intel_bo_reference(struct intel_bo *bo);

/**
 * Decrease the reference count of \p bo.  When the reference count reaches
 * zero, \p bo is destroyed.
 */
void
intel_bo_unreference(struct intel_bo *bo);

/**
 * Map \p bo for CPU access.  Recursive mapping is allowed.
 *
 * map() maps the backing store into CPU address space, cached.  It will block
 * if the bo is busy.  This variant allows fastest random reads and writes,
 * but the caller needs to handle tiling or swizzling manually if the bo is
 * tiled or swizzled.  If write is enabled and there is no shared last-level
 * cache (LLC), the CPU cache will be flushed, which is expensive.
 *
 * map_gtt() maps the bo for MMIO access, uncached but write-combined.  It
 * will block if the bo is busy.  This variant promises a reasonable speed for
 * sequential writes, but reads would be very slow.  Callers always have a
 * linear view of the bo.
 *
 * map_unsynchronized() is similar to map_gtt(), except that it does not
 * block.
 */
void *
intel_bo_map(struct intel_bo *bo, bool write_enable);

void *
intel_bo_map_gtt(struct intel_bo *bo);

void *
intel_bo_map_unsynchronized(struct intel_bo *bo);

/**
 * Unmap \p bo.
 */
void
intel_bo_unmap(struct intel_bo *bo);

/**
 * Write data to \p bo.
 */
int
intel_bo_pwrite(struct intel_bo *bo, unsigned long offset,
                unsigned long size, const void *data);

/**
 * Read data from the bo.
 */
int
intel_bo_pread(struct intel_bo *bo, unsigned long offset,
               unsigned long size, void *data);

/**
 * Add \p target_bo to the relocation list.
 *
 * When \p bo is submitted for execution, and if \p target_bo has moved,
 * the kernel will patch \p bo at \p offset to \p target_bo->offset plus
 * \p target_offset.
 *
 * \p presumed_offset should be written to \p bo at \p offset.
 */
int
intel_bo_add_reloc(struct intel_bo *bo, uint32_t offset,
                   struct intel_bo *target_bo, uint32_t target_offset,
                   uint32_t read_domains, uint32_t write_domain,
                   uint64_t *presumed_offset);

/**
 * Return the current number of relocations.
 */
int
intel_bo_get_reloc_count(struct intel_bo *bo);

/**
 * Truncate all relocations except the first \p start ones.
 *
 * Combined with \p intel_bo_get_reloc_count(), they can be used to undo the
 * \p intel_bo_add_reloc() calls that were just made.
 */
void
intel_bo_truncate_relocs(struct intel_bo *bo, int start);

/**
 * Return true if \p target_bo is on the relocation list of \p bo, or on
 * the relocation list of some bo that is referenced by \p bo.
 */
bool
intel_bo_has_reloc(struct intel_bo *bo, struct intel_bo *target_bo);

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
