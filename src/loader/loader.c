/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
 *
 * This code is derived from the following files.
 *
 * * src/glx/dri3_common.c
 * Copyright © 2013 Keith Packard
 *
 * * src/egl/drivers/dri2/common.c
 * * src/gbm/backends/dri/driver_name.c
 * Copyright © 2011 Intel Corporation
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 *
 * * src/gallium/targets/egl-static/egl.c
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 *
 * * src/gallium/state_trackers/egl/drm/native_drm.c
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
 *
 * * src/egl/drivers/dri2/platform_android.c
 *
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * Based on platform_x11, which has
 *
 * Copyright © 2011 Intel Corporation
 *
 * * src/gallium/auxiliary/pipe-loader/pipe_loader_drm.c
 * Copyright 2011 Intel Corporation
 * Copyright 2012 Francisco Jerez
 * All Rights Reserved.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_LIBUDEV
#include <assert.h>
#include <dlfcn.h>
#endif
#include "loader.h"

#ifndef __NOT_HAVE_DRM_H
#include <xf86drm.h>
#endif

#define __IS_LOADER
#include "pci_id_driver_map.h"

static void default_logger(int level, const char *fmt, ...)
{
   if (level <= _LOADER_WARNING) {
      va_list args;
      va_start(args, fmt);
      vfprintf(stderr, fmt, args);
      va_end(args);
   }
}

static void (*log_)(int level, const char *fmt, ...) = default_logger;

#ifdef HAVE_LIBUDEV
#include <libudev.h>

static void *udev_handle = NULL;

static void *
udev_dlopen_handle(void)
{
   if (!udev_handle) {
      udev_handle = dlopen("libudev.so.1", RTLD_LOCAL | RTLD_LAZY);

      if (!udev_handle) {
         /* libudev.so.1 changed the return types of the two unref functions
          * from voids to pointers.  We don't use those return values, and the
          * only ABI I've heard that cares about this kind of change (calling
          * a function with a void * return that actually only returns void)
          * might be ia64.
          */
         udev_handle = dlopen("libudev.so.0", RTLD_LOCAL | RTLD_LAZY);

         if (!udev_handle) {
            log_(_LOADER_FATAL, "Couldn't dlopen libudev.so.1 or libudev.so.0, "
                 "driver detection may be broken.\n");
         }
      }
   }

   return udev_handle;
}

static void *
asserted_dlsym(void *dlopen_handle, const char *name)
{
   void *result = dlsym(dlopen_handle, name);
   assert(result);
   return result;
}

#define UDEV_SYMBOL(ret, name, args) \
   ret (*name) args = asserted_dlsym(udev_dlopen_handle(), #name);


static inline struct udev_device *
udev_device_new_from_fd(struct udev *udev, int fd)
{
   struct udev_device *device;
   struct stat buf;
   UDEV_SYMBOL(struct udev_device *, udev_device_new_from_devnum,
               (struct udev *udev, char type, dev_t devnum));

   if (fstat(fd, &buf) < 0) {
      log_(_LOADER_WARNING, "MESA-LOADER: failed to stat fd %d\n", fd);
      return NULL;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      log_(_LOADER_WARNING,
              "MESA-LOADER: could not create udev device for fd %d\n", fd);
      return NULL;
   }

   return device;
}

int
loader_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
{
   struct udev *udev = NULL;
   struct udev_device *device = NULL, *parent;
   const char *pci_id;
   UDEV_SYMBOL(struct udev *, udev_new, (void));
   UDEV_SYMBOL(struct udev_device *, udev_device_get_parent,
               (struct udev_device *));
   UDEV_SYMBOL(const char *, udev_device_get_property_value,
               (struct udev_device *, const char *));
   UDEV_SYMBOL(struct udev_device *, udev_device_unref,
               (struct udev_device *));
   UDEV_SYMBOL(struct udev *, udev_unref, (struct udev *));

   *chip_id = -1;

   udev = udev_new();
   device = udev_device_new_from_fd(udev, fd);
   if (!device)
      goto out;

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      log_(_LOADER_WARNING, "MESA-LOADER: could not get parent device\n");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL ||
       sscanf(pci_id, "%x:%x", vendor_id, chip_id) != 2) {
      log_(_LOADER_WARNING, "MESA-LOADER: malformed or no PCI ID\n");
      *chip_id = -1;
      goto out;
   }

out:
   if (device)
      udev_device_unref(device);
   if (udev)
      udev_unref(udev);

   return (*chip_id >= 0);
}

#elif !defined(__NOT_HAVE_DRM_H)

/* for i915 */
#include <i915_drm.h>
/* for radeon */
#include <radeon_drm.h>

int
loader_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
{
   drmVersionPtr version;

   *chip_id = -1;

   version = drmGetVersion(fd);
   if (!version) {
      log_(_LOADER_WARNING, "MESA-LOADER: invalid drm fd\n");
      return 0;
   }
   if (!version->name) {
      log_(_LOADER_WARNING, "MESA-LOADER: unable to determine the driver name\n");
      drmFreeVersion(version);
      return 0;
   }

   if (strcmp(version->name, "i915") == 0) {
      struct drm_i915_getparam gp;
      int ret;

      *vendor_id = 0x8086;

      memset(&gp, 0, sizeof(gp));
      gp.param = I915_PARAM_CHIPSET_ID;
      gp.value = chip_id;
      ret = drmCommandWriteRead(fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
      if (ret) {
         log_(_LOADER_WARNING, "MESA-LOADER: failed to get param for i915\n");
	 *chip_id = -1;
      }
   }
   else if (strcmp(version->name, "radeon") == 0) {
      struct drm_radeon_info info;
      int ret;

      *vendor_id = 0x1002;

      memset(&info, 0, sizeof(info));
      info.request = RADEON_INFO_DEVICE_ID;
      info.value = (unsigned long) chip_id;
      ret = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
      if (ret) {
         log_(_LOADER_WARNING, "MESA-LOADER: failed to get info for radeon\n");
	 *chip_id = -1;
      }
   }
   else if (strcmp(version->name, "nouveau") == 0) {
      *vendor_id = 0x10de;
      /* not used */
      *chip_id = 0;
   }
   else if (strcmp(version->name, "vmwgfx") == 0) {
      *vendor_id = 0x15ad;
      /* assume SVGA II */
      *chip_id = 0x0405;
   }

   drmFreeVersion(version);

   return (*chip_id >= 0);
}

#else

int
loader_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
{
   return 0;
}

#endif


char *
loader_get_device_name_for_fd(int fd)
{
   char *device_name = NULL;
#ifdef HAVE_LIBUDEV
   struct udev *udev;
   struct udev_device *device;
   const char *const_device_name;
   UDEV_SYMBOL(struct udev *, udev_new, (void));
   UDEV_SYMBOL(const char *, udev_device_get_devnode,
               (struct udev_device *));
   UDEV_SYMBOL(struct udev_device *, udev_device_unref,
               (struct udev_device *));
   UDEV_SYMBOL(struct udev *, udev_unref, (struct udev *));

   udev = udev_new();
   device = udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   const_device_name = udev_device_get_devnode(device);
   if (!const_device_name)
      goto out;
   device_name = strdup(const_device_name);

out:
   udev_device_unref(device);
   udev_unref(udev);
#endif
   return device_name;
}

char *
loader_get_driver_for_fd(int fd, unsigned driver_types)
{
   int vendor_id, chip_id, i, j;
   char *driver = NULL;

   if (!driver_types)
      driver_types = _LOADER_GALLIUM | _LOADER_DRI;

   if (!loader_get_pci_id_for_fd(fd, &vendor_id, &chip_id)) {

#ifndef __NOT_HAVE_DRM_H
      /* fallback to drmGetVersion(): */
      drmVersionPtr version = drmGetVersion(fd);

      if (!version) {
         log_(_LOADER_WARNING, "failed to get driver name for fd %d\n", fd);
         return NULL;
      }

      driver = strndup(version->name, version->name_len);
      log_(_LOADER_INFO, "using driver %s for %d\n", driver, fd);

      drmFreeVersion(version);
#endif

      return driver;
   }

   for (i = 0; driver_map[i].driver; i++) {
      if (vendor_id != driver_map[i].vendor_id)
         continue;

      if (!(driver_types & driver_map[i].driver_types))
         continue;

      if (driver_map[i].predicate && !driver_map[i].predicate(fd))
         continue;

      if (driver_map[i].num_chips_ids == -1) {
         driver = strdup(driver_map[i].driver);
         goto out;
      }

      for (j = 0; j < driver_map[i].num_chips_ids; j++)
         if (driver_map[i].chip_ids[j] == chip_id) {
            driver = strdup(driver_map[i].driver);
            goto out;
         }
   }

out:
   log_(driver ? _LOADER_DEBUG : _LOADER_WARNING,
         "pci id for fd %d: %04x:%04x, driver %s\n",
         fd, vendor_id, chip_id, driver);
   return driver;
}

void
loader_set_logger(void (*logger)(int level, const char *fmt, ...))
{
   log_ = logger;
}
