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

#ifndef _GBM_DRI_INTERNAL_H_
#define _GBM_DRI_INTERNAL_H_

#include "gbmint.h"

#include "common.h"
#include "common_drm.h"

#include <GL/gl.h> /* dri_interface needs GL types */
#include "GL/internal/dri_interface.h"

struct gbm_dri_device {
   struct gbm_drm_device base;

   void *driver;

   __DRIscreen *screen;

   __DRIcoreExtension   *core;
   __DRIdri2Extension   *dri2;
   __DRIimageExtension  *image;

   const __DRIconfig   **driver_configs;
   const __DRIextension *extensions[3];

   __DRIimage *(*lookup_image)(__DRIscreen *screen, void *image, void *data);
   void *lookup_user_data;
};

struct gbm_dri_bo {
   struct gbm_drm_bo base;

   __DRIimage *image;
};

static inline struct gbm_dri_device *
gbm_dri_device(struct gbm_device *gbm)
{
   return (struct gbm_dri_device *) gbm;
}

static inline struct gbm_dri_bo *
gbm_dri_bo(struct gbm_bo *bo)
{
   return (struct gbm_dri_bo *) bo;
}

char *
dri_fd_get_driver_name(int fd);

#endif
