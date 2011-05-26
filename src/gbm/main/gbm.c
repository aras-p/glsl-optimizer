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

#define _BSD_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gbm.h"
#include "gbmint.h"
#include "common.h"
#include "backend.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

struct gbm_device *devices[16];

static int device_num = 0;

GBM_EXPORT int
gbm_device_get_fd(struct gbm_device *gbm)
{
   return gbm->fd;
}

/* FIXME: maybe superfluous, use udev subclass from the fd? */
GBM_EXPORT const char *
gbm_device_get_backend_name(struct gbm_device *gbm)
{
   return gbm->name;
}

int
gbm_device_is_format_supported(struct gbm_device *gbm,
                               enum gbm_bo_format format,
                               uint32_t usage)
{
   return gbm->is_format_supported(gbm, format, usage);
}

GBM_EXPORT void
gbm_device_destroy(struct gbm_device *gbm)
{
   gbm->refcount--;
   if (gbm->refcount == 0)
      gbm->destroy(gbm);
}

GBM_EXPORT struct gbm_device *
_gbm_mesa_get_device(int fd)
{
   struct gbm_device *gbm = NULL;
   struct stat buf;
   dev_t dev;
   int i;

   if (fd < 0 || fstat(fd, &buf) < 0 || !S_ISCHR(buf.st_mode)) {
      fprintf(stderr, "_gbm_mesa_get_device: invalid fd: %d\n", fd);
      return NULL;
   }

   for (i = 0; i < device_num; ++i) {
      dev = devices[i]->stat.st_rdev;
      if (major(dev) == major(buf.st_rdev) &&
          minor(dev) == minor(buf.st_rdev)) {
         gbm = devices[i];
         gbm->refcount++;
         break;
      }
   }

   return gbm;
}

GBM_EXPORT struct gbm_device *
gbm_create_device(int fd)
{
   struct gbm_device *gbm = NULL;
   struct stat buf;

   if (fd < 0 || fstat(fd, &buf) < 0 || !S_ISCHR(buf.st_mode)) {
      fprintf(stderr, "gbm_create_device: invalid fd: %d\n", fd);
      return NULL;
   }

   if (device_num == 0)
      memset(devices, 0, sizeof devices);

   gbm = _gbm_create_device(fd);
   if (gbm == NULL)
      return NULL;

   gbm->dummy = gbm_create_device;
   gbm->stat = buf;
   gbm->refcount = 1;

   if (device_num < ARRAY_SIZE(devices)-1)
      devices[device_num++] = gbm;

   return gbm;
}

GBM_EXPORT unsigned int
gbm_bo_get_width(struct gbm_bo *bo)
{
   return bo->width;
}

GBM_EXPORT unsigned int
gbm_bo_get_height(struct gbm_bo *bo)
{
   return bo->height;
}

GBM_EXPORT uint32_t
gbm_bo_get_pitch(struct gbm_bo *bo)
{
   return bo->pitch;
}

GBM_EXPORT union gbm_bo_handle
gbm_bo_get_handle(struct gbm_bo *bo)
{
   return bo->handle;
}

GBM_EXPORT void
gbm_bo_destroy(struct gbm_bo *bo)
{
   bo->gbm->bo_destroy(bo);
}

GBM_EXPORT struct gbm_bo *
gbm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              enum gbm_bo_format format, uint32_t usage)
{
   if (width == 0 || height == 0)
      return NULL;

   if (usage & GBM_BO_USE_CURSOR_64X64 &&
       (width != 64 || height != 64))
      return NULL;

   return gbm->bo_create(gbm, width, height, format, usage);
}

GBM_EXPORT struct gbm_bo *
gbm_bo_create_from_egl_image(struct gbm_device *gbm,
                             void *egl_dpy, void *egl_image,
                             uint32_t width, uint32_t height,
                             uint32_t usage)
{
   if (width == 0 || height == 0)
      return NULL;

   return gbm->bo_create_from_egl_image(gbm, egl_dpy, egl_image,
                                        width, height, usage);
}
