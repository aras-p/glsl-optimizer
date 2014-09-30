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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "vc4_context.h"
#include "vc4_qpu_defines.h"

struct exec_info;

#define DRM_INFO(...) fprintf(stderr, __VA_ARGS__)
#define DRM_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define kmalloc(size, arg) malloc(size)
#define kcalloc(size, count, arg) calloc(size, count)
#define kfree(ptr) free(ptr)
#define krealloc(ptr, size, args) realloc(ptr, size)
#define roundup(x, y) align(x, y)
#define max(x, y) MAX2(x, y)
#define min(x, y) MiN2(x, y)
#define BUG_ON(condition) assert(!(condition))

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


struct drm_gem_cma_object *
drm_gem_cma_create(struct drm_device *dev, size_t size);

int
vc4_cl_validate(struct drm_device *dev, struct exec_info *exec);

#endif /* VC4_SIMULATOR_VALIDATE_H */
