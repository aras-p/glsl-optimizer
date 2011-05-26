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

#ifndef INTERNAL_H_
#define INTERNAL_H_

#include "gbm.h"
#include <sys/stat.h>

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define GBM_EXPORT __attribute__ ((visibility("default")))
#else
#define GBM_EXPORT
#endif

struct gbm_device {
   /* Hack to make a gbm_device detectable by its first element. */
   struct gbm_device *(*dummy)(int);

   int fd;
   const char *name;
   unsigned int refcount;
   struct stat stat;

   void (*destroy)(struct gbm_device *gbm);
   int (*is_format_supported)(struct gbm_device *gbm,
                              enum gbm_bo_format format,
                              uint32_t usage);

   struct gbm_bo *(*bo_create)(struct gbm_device *gbm,
                               uint32_t width, uint32_t height,
                               enum gbm_bo_format format,
                               uint32_t usage);
   struct gbm_bo *(*bo_create_from_egl_image)(struct gbm_device *gbm,
                                              void *egl_dpy, void *egl_img,
                                              uint32_t width, uint32_t height,
                                              uint32_t usage);
   void (*bo_destroy)(struct gbm_bo *bo);
};

struct gbm_bo {
   struct gbm_device *gbm;
   uint32_t width;
   uint32_t height;
   uint32_t pitch;
   union gbm_bo_handle  handle;
};

struct gbm_backend {
   const char *backend_name;
   struct gbm_device *(*create_device)(int fd);
};

GBM_EXPORT struct gbm_device *
_gbm_mesa_get_device(int fd);

#endif
