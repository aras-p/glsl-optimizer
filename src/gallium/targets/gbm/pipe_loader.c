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
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdio.h>
#include "util/u_string.h"
#include "util/u_memory.h"

#include <libudev.h>

#include "gbm_gallium_drmint.h"
#include "pipe_loader.h"
#define DRIVER_MAP_GALLIUM_ONLY
#include "pci_ids/pci_id_driver_map.h"

static struct pipe_module pipe_modules[16];

static INLINE char *
loader_strdup(const char *str)
{
   return mem_dup(str, strlen(str) + 1);
}

char *
drm_fd_get_screen_name(int fd)
{
   struct udev *udev;
   struct udev_device *device, *parent;
   const char *pci_id;
   char *driver = NULL;
   int vendor_id, chip_id, i, j;

   udev = udev_new();
   device = _gbm_udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      fprintf(stderr, "gbm: could not get parent device");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL ||
       sscanf(pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
      fprintf(stderr, "gbm: malformed or no PCI ID");
      goto out;
   }

   for (i = 0; driver_map[i].driver; i++) {
      if (vendor_id != driver_map[i].vendor_id)
         continue;
      if (driver_map[i].num_chips_ids == -1) {
         driver = loader_strdup(driver_map[i].driver);
         _gbm_log("pci id for %d: %04x:%04x, driver %s",
                  fd, vendor_id, chip_id, driver);
         goto out;
      }

      for (j = 0; j < driver_map[i].num_chips_ids; j++)
         if (driver_map[i].chip_ids[j] == chip_id) {
            driver = loader_strdup(driver_map[i].driver);
            _gbm_log("pci id for %d: %04x:%04x, driver %s",
                     fd, vendor_id, chip_id, driver);
            goto out;
         }
   }

out:
   udev_device_unref(device);
   udev_unref(udev);

   return driver;
}

static void
find_pipe_module(struct pipe_module *pmod, const char *name)
{
   char *search_paths, *end, *next, *p;
   char path[PATH_MAX];
   int ret;
   
   search_paths = NULL;
   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use GBM_BACKENDS_PATH */
      search_paths = getenv("GBM_BACKENDS_PATH");
   }
   if (search_paths == NULL)
      search_paths = GBM_BACKEND_SEARCH_DIR;

   end = search_paths + strlen(search_paths);
   for (p = search_paths; p < end && pmod->lib == NULL; p = next + 1) {
      int len;
      next = strchr(p, ':');
      if (next == NULL)
         next = end;

      len = next - p;

      if (len) {
         ret = util_snprintf(path, sizeof(path),
                             "%.*s/" PIPE_PREFIX "%s" UTIL_DL_EXT, len, p, pmod->name);
      }
      else {
         ret = util_snprintf(path, sizeof(path),
                             PIPE_PREFIX "%s" UTIL_DL_EXT, pmod->name);
      }
      if (ret > 0 && ret < sizeof(path)) {
         pmod->lib = util_dl_open(path);
         debug_printf("loaded %s\n", path);
      }

   }
}

static boolean
load_pipe_module(struct pipe_module *pmod, const char *name)
{
   pmod->name = loader_strdup(name);
   if (!pmod->name)
      return FALSE;

   find_pipe_module(pmod, name);

   if (pmod->lib) {
      pmod->drmdd = (const struct drm_driver_descriptor *)
         util_dl_get_proc_address(pmod->lib, "driver_descriptor");

      /* sanity check on the name */
      if (pmod->drmdd && strcmp(pmod->drmdd->name, pmod->name) != 0)
         pmod->drmdd = NULL;

      if (!pmod->drmdd) {
         util_dl_close(pmod->lib);
         pmod->lib = NULL;
      }
   }

   return (pmod->drmdd != NULL);
}

struct pipe_module *
get_pipe_module(const char *name)
{
   struct pipe_module *pmod = NULL;
   int i;

   if (!name)
      return NULL;

   for (i = 0; i < Elements(pipe_modules); i++) {
      if (!pipe_modules[i].initialized ||
          strcmp(pipe_modules[i].name, name) == 0) {
         pmod = &pipe_modules[i];
         break;
      }
   }
   if (!pmod)
      return NULL;

   if (!pmod->initialized) {
      load_pipe_module(pmod, name);
      pmod->initialized = TRUE;
   }

   return pmod;
}
