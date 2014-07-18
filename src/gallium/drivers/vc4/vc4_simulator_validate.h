/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef VC4_SIMULATOR_VALIDATE_H
#define VC4_SIMULATOR_VALIDATE_H

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define DRM_INFO(...) fprintf(stderr, __VA_ARGS__)
#define DRM_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define kmalloc(size, arg) malloc(size)
#define kfree(ptr) free(ptr)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define roundup(x, y) align(x, y)

static inline int
copy_from_user(void *dst, void *src, size_t size)
{
        memcpy(dst, src, size);
        return 0;
}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct drm_device {
        struct vc4_context *vc4;
        uint32_t simulator_mem_next;
};

struct drm_gem_cma_object {
        struct vc4_bo *bo;

        struct {
                uint32_t size;
        } base;
        uint32_t paddr;
        void *vaddr;
};

struct exec_info {
	/* This is the array of BOs that were looked up at the start of exec.
	 * Command validation will use indices into this array.
	 */
	struct drm_gem_cma_object **bo;
	uint32_t bo_count;

	/* Current indices into @bo loaded by the non-hardware packet
	 * that passes in indices.  This can be used even without
	 * checking that we've seen one of those packets, because
	 * @bo_count is always >= 1, and this struct is initialized to
	 * 0.
	 */
	uint32_t bo_index[2];
	uint32_t max_width, max_height;

	/**
	 * This is the BO where we store the validated command lists
	 * and shader records.
	 */
	struct drm_gem_cma_object *exec_bo;

	/**
	 * This tracks the per-shader-record state (packet 64) that
	 * determines the length of the shader record and the offset
	 * it's expected to be found at.  It gets read in from the
	 * command lists.
	 */
	struct vc4_shader_state {
		uint8_t packet;
		uint32_t addr;
	} *shader_state;

	/** How many shader states the user declared they were using. */
	uint32_t shader_state_size;
	/** How many shader state records the validator has seen. */
	uint32_t shader_state_count;

	/**
	 * Computed addresses pointing into exec_bo where we start the
	 * bin thread (ct0) and render thread (ct1).
	 */
	uint32_t ct0ca, ct0ea;
	uint32_t ct1ca, ct1ea;
	uint32_t shader_paddr;
};

int vc4_validate_cl(struct drm_device *dev,
                    void *validated,
                    void *unvalidated,
                    uint32_t len,
                    bool is_bin,
                    struct exec_info *exec);

int vc4_validate_shader_recs(struct drm_device *dev,
                             void *validated,
                             void *unvalidated,
                             uint32_t len,
                             struct exec_info *exec);

#endif /* VC4_SIMULATOR_VALIDATE_H */
