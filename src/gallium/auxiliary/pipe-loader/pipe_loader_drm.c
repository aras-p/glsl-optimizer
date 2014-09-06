/**************************************************************************
 *
 * Copyright 2011 Intel Corporation
 * Copyright 2012 Francisco Jerez
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 *
 **************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <xf86drm.h>
#include <unistd.h>

#ifdef HAVE_PIPE_LOADER_XCB

#include <xcb/dri2.h>

#endif

#include "loader.h"
#include "state_tracker/drm_driver.h"
#include "pipe_loader_priv.h"

#include "util/u_memory.h"
#include "util/u_dl.h"
#include "util/u_debug.h"

#define DRM_RENDER_NODE_DEV_NAME_FORMAT "%s/renderD%d"
#define DRM_RENDER_NODE_MAX_NODES 63
#define DRM_RENDER_NODE_MIN_MINOR 128
#define DRM_RENDER_NODE_MAX_MINOR (DRM_RENDER_NODE_MIN_MINOR + DRM_RENDER_NODE_MAX_NODES)

struct pipe_loader_drm_device {
   struct pipe_loader_device base;
   struct util_dl_library *lib;
   int fd;
};

#define pipe_loader_drm_device(dev) ((struct pipe_loader_drm_device *)dev)

static struct pipe_loader_ops pipe_loader_drm_ops;

#ifdef HAVE_PIPE_LOADER_XCB

static xcb_screen_t *
get_xcb_screen(xcb_screen_iterator_t iter, int screen)
{
    for (; iter.rem; --screen, xcb_screen_next(&iter))
        if (screen == 0)
            return iter.data;

    return NULL;
}

#endif

static void
pipe_loader_drm_x_auth(int fd)
{
#ifdef HAVE_PIPE_LOADER_XCB
   /* Try authenticate with the X server to give us access to devices that X
    * is running on. */
   xcb_connection_t *xcb_conn;
   const xcb_setup_t *xcb_setup;
   xcb_screen_iterator_t s;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_connect_reply_t *connect;
   drm_magic_t magic;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_dri2_authenticate_reply_t *authenticate;
   int screen;

   xcb_conn = xcb_connect(NULL, &screen);

   if(!xcb_conn)
      return;

   xcb_setup = xcb_get_setup(xcb_conn);

  if (!xcb_setup)
    goto disconnect;

   s = xcb_setup_roots_iterator(xcb_setup);
   connect_cookie = xcb_dri2_connect_unchecked(xcb_conn,
                                               get_xcb_screen(s, screen)->root,
                                               XCB_DRI2_DRIVER_TYPE_DRI);
   connect = xcb_dri2_connect_reply(xcb_conn, connect_cookie, NULL);

   if (!connect || connect->driver_name_length
                   + connect->device_name_length == 0) {

      goto disconnect;
   }

   if (drmGetMagic(fd, &magic))
      goto disconnect;

   authenticate_cookie = xcb_dri2_authenticate_unchecked(xcb_conn,
                                                         s.data->root,
                                                         magic);
   authenticate = xcb_dri2_authenticate_reply(xcb_conn,
                                              authenticate_cookie,
                                              NULL);
   FREE(authenticate);

disconnect:
   xcb_disconnect(xcb_conn);

#endif
}

bool
pipe_loader_drm_probe_fd(struct pipe_loader_device **dev, int fd,
                         boolean auth_x)
{
   struct pipe_loader_drm_device *ddev = CALLOC_STRUCT(pipe_loader_drm_device);
   int vendor_id, chip_id;

   if (!ddev)
      return false;

   if (loader_get_pci_id_for_fd(fd, &vendor_id, &chip_id)) {
      ddev->base.type = PIPE_LOADER_DEVICE_PCI;
      ddev->base.u.pci.vendor_id = vendor_id;
      ddev->base.u.pci.chip_id = chip_id;
   } else {
      ddev->base.type = PIPE_LOADER_DEVICE_PLATFORM;
   }
   ddev->base.ops = &pipe_loader_drm_ops;
   ddev->fd = fd;

   if (auth_x)
      pipe_loader_drm_x_auth(fd);

   ddev->base.driver_name = loader_get_driver_for_fd(fd, _LOADER_GALLIUM);
   if (!ddev->base.driver_name)
      goto fail;

   *dev = &ddev->base;
   return true;

  fail:
   FREE(ddev);
   return false;
}

static int
open_drm_minor(int minor)
{
   char path[PATH_MAX];
   snprintf(path, sizeof(path), DRM_DEV_NAME, DRM_DIR_NAME, minor);
   return open(path, O_RDWR, 0);
}

static int
open_drm_render_node_minor(int minor)
{
   char path[PATH_MAX];
   snprintf(path, sizeof(path), DRM_RENDER_NODE_DEV_NAME_FORMAT, DRM_DIR_NAME,
            minor);
   return open(path, O_RDWR, 0);
}

int
pipe_loader_drm_probe(struct pipe_loader_device **devs, int ndev)
{
   int i, k, fd, num_render_node_devs;
   int j = 0;

   struct {
      unsigned vendor_id;
      unsigned chip_id;
   } render_node_devs[DRM_RENDER_NODE_MAX_NODES];

   /* Look for render nodes first */
   for (i = DRM_RENDER_NODE_MIN_MINOR, j = 0;
        i <= DRM_RENDER_NODE_MAX_MINOR; i++) {
      fd = open_drm_render_node_minor(i);
      struct pipe_loader_device *dev;
      if (fd < 0)
         continue;

      if (!pipe_loader_drm_probe_fd(&dev, fd, false)) {
         close(fd);
         continue;
      }

      render_node_devs[j].vendor_id = dev->u.pci.vendor_id;
      render_node_devs[j].chip_id = dev->u.pci.chip_id;

      if (j < ndev) {
         devs[j] = dev;
      } else {
         close(fd);
         dev->ops->release(&dev);
      }
      j++;
   }

   num_render_node_devs = j;

   /* Next look for drm devices. */
   for (i = 0; i < DRM_MAX_MINOR; i++) {
      struct pipe_loader_device *dev;
      boolean duplicate = FALSE;
      fd = open_drm_minor(i);
      if (fd < 0)
         continue;

      if (!pipe_loader_drm_probe_fd(&dev, fd, true)) {
         close(fd);
         continue;
      }

      /* Check to make sure we aren't already accessing this device via
       * render nodes.
       */
      for (k = 0; k < num_render_node_devs; k++) {
         if (dev->u.pci.vendor_id == render_node_devs[k].vendor_id &&
             dev->u.pci.chip_id == render_node_devs[k].chip_id) {
            close(fd);
            dev->ops->release(&dev);
            duplicate = TRUE;
            break;
         }
      }

      if (duplicate)
         continue;

      if (j < ndev) {
         devs[j] = dev;
      } else {
         dev->ops->release(&dev);
      }

      j++;
   }

   return j;
}

static void
pipe_loader_drm_release(struct pipe_loader_device **dev)
{
   struct pipe_loader_drm_device *ddev = pipe_loader_drm_device(*dev);

   if (ddev->lib)
      util_dl_close(ddev->lib);

   close(ddev->fd);
   FREE(ddev->base.driver_name);
   FREE(ddev);
   *dev = NULL;
}

static const struct drm_conf_ret *
pipe_loader_drm_configuration(struct pipe_loader_device *dev,
                              enum drm_conf conf)
{
   struct pipe_loader_drm_device *ddev = pipe_loader_drm_device(dev);
   const struct drm_driver_descriptor *dd;

   if (!ddev->lib)
      return NULL;

   dd = (const struct drm_driver_descriptor *)
      util_dl_get_proc_address(ddev->lib, "driver_descriptor");

   /* sanity check on the name */
   if (!dd || strcmp(dd->name, ddev->base.driver_name) != 0)
      return NULL;

   if (!dd->configuration)
      return NULL;

   return dd->configuration(conf);
}

static struct pipe_screen *
pipe_loader_drm_create_screen(struct pipe_loader_device *dev,
                              const char *library_paths)
{
   struct pipe_loader_drm_device *ddev = pipe_loader_drm_device(dev);
   const struct drm_driver_descriptor *dd;

   if (!ddev->lib)
      ddev->lib = pipe_loader_find_module(dev, library_paths);
   if (!ddev->lib)
      return NULL;

   dd = (const struct drm_driver_descriptor *)
      util_dl_get_proc_address(ddev->lib, "driver_descriptor");

   /* sanity check on the name */
   if (!dd || strcmp(dd->name, ddev->base.driver_name) != 0)
      return NULL;

   return dd->create_screen(ddev->fd);
}

static struct pipe_loader_ops pipe_loader_drm_ops = {
   .create_screen = pipe_loader_drm_create_screen,
   .configuration = pipe_loader_drm_configuration,
   .release = pipe_loader_drm_release
};
