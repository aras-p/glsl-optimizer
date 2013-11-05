/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * This code is derived from src/egl/drivers/dri2/common.c which
 * carries the following copyright:
 * 
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
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <GL/gl.h>
#include "glapi.h"
#include "glxclient.h"
#include "xf86dri.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "xf86drm.h"
#include "dri_common.h"
#include "dri3_priv.h"

#define DRIVER_MAP_DRI3_ONLY
#include "pci_ids/pci_id_driver_map.h"

#include <libudev.h>

static struct udev_device *
dri3_udev_device_new_from_fd(struct udev *udev, int fd)
{
   struct udev_device *device;
   struct stat buf;

   if (fstat(fd, &buf) < 0) {
      ErrorMessageF("DRI3: failed to stat fd %d", fd);
      return NULL;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      ErrorMessageF("DRI3: could not create udev device for fd %d", fd);
      return NULL;
   }

   return device;
}

char *
dri3_get_driver_for_fd(int fd)
{
   struct udev *udev;
   struct udev_device *device, *parent;
   const char *pci_id;
   char *driver = NULL;
   int vendor_id, chip_id, i, j;

   udev = udev_new();
   device = dri3_udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      ErrorMessageF("DRI3: could not get parent device");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL ||
       sscanf(pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
      ErrorMessageF("DRI3: malformed or no PCI ID");
      goto out;
   }

   for (i = 0; driver_map[i].driver; i++) {
      if (vendor_id != driver_map[i].vendor_id)
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
   udev_device_unref(device);
   udev_unref(udev);

   return driver;
}
