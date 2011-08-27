/*
 * Copyright © 2011 Intel Corporation
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#ifndef _GBM_H_
#define _GBM_H_

#ifdef __cplusplus
extern "C" {
#endif


#define __GBM__ 1

#include <stdint.h>

struct gbm_device;
struct gbm_bo;

union gbm_bo_handle {
   void *ptr;
   int32_t s32;
   uint32_t u32;
   int64_t s64;
   uint64_t u64;
};

enum gbm_bo_format {
   GBM_BO_FORMAT_XRGB8888,
   GBM_BO_FORMAT_ARGB8888,
};

enum gbm_bo_flags {
   GBM_BO_USE_SCANOUT      = (1 << 0),
   GBM_BO_USE_CURSOR_64X64 = (1 << 1),
   GBM_BO_USE_RENDERING    = (1 << 2),
};

int
gbm_device_get_fd(struct gbm_device *gbm);

const char *
gbm_device_get_backend_name(struct gbm_device *gbm);

int
gbm_device_is_format_supported(struct gbm_device *gbm,
                               enum gbm_bo_format format,
                               uint32_t usage);

void
gbm_device_destroy(struct gbm_device *gbm);

struct gbm_device *
gbm_create_device(int fd);

struct gbm_bo *
gbm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              enum gbm_bo_format format, uint32_t flags);

struct gbm_bo *
gbm_bo_create_from_egl_image(struct gbm_device *gbm,
                             void *egl_dpy, void *egl_img,
                             uint32_t width, uint32_t height,
                             uint32_t usage);

uint32_t
gbm_bo_get_width(struct gbm_bo *bo);

uint32_t
gbm_bo_get_height(struct gbm_bo *bo);

uint32_t
gbm_bo_get_pitch(struct gbm_bo *bo);

union gbm_bo_handle
gbm_bo_get_handle(struct gbm_bo *bo);

void
gbm_bo_destroy(struct gbm_bo *bo);

#ifdef __cplusplus
}
#endif

#endif
