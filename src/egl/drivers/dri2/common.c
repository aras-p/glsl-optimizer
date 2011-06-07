#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "egl_dri2.h"

#ifdef HAVE_LIBUDEV

#define DRIVER_MAP_DRI2_ONLY
#include "pci_ids/pci_id_driver_map.h"

#include <libudev.h>

static struct udev_device *
dri2_udev_device_new_from_fd(struct udev *udev, int fd)
{
   struct udev_device *device;
   struct stat buf;

   if (fstat(fd, &buf) < 0) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: failed to stat fd %d", fd);
      return NULL;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      _eglLog(_EGL_WARNING,
              "EGL-DRI2: could not create udev device for fd %d", fd);
      return NULL;
   }

   return device;
}

char *
dri2_get_device_name_for_fd(int fd)
{
   struct udev *udev;
   struct udev_device *device;
   const char *const_device_name;
   char *device_name = NULL;

   udev = udev_new();
   device = dri2_udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   const_device_name = udev_device_get_devnode(device);
   if (!const_device_name)
      goto out;
   device_name = strdup(const_device_name);

out:
   udev_device_unref(device);
   udev_unref(udev);

   return device_name;
}

char *
dri2_get_driver_for_fd(int fd)
{
   struct udev *udev;
   struct udev_device *device, *parent;
   const char *pci_id;
   char *driver = NULL;
   int vendor_id, chip_id, i, j;

   udev = udev_new();
   device = dri2_udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      _eglLog(_EGL_WARNING, "DRI2: could not get parent device");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL ||
       sscanf(pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: malformed or no PCI ID");
      goto out;
   }

   for (i = 0; driver_map[i].driver; i++) {
      if (vendor_id != driver_map[i].vendor_id)
         continue;
      if (driver_map[i].num_chips_ids == -1) {
         driver = strdup(driver_map[i].driver);
         _eglLog(_EGL_DEBUG, "pci id for %d: %04x:%04x, driver %s",
                 fd, vendor_id, chip_id, driver);
         goto out;
      }

      for (j = 0; j < driver_map[i].num_chips_ids; j++)
         if (driver_map[i].chip_ids[j] == chip_id) {
            driver = strdup(driver_map[i].driver);
            _eglLog(_EGL_DEBUG, "pci id for %d: %04x:%04x, driver %s",
                    fd, vendor_id, chip_id, driver);
            goto out;
         }
   }

out:
   udev_device_unref(device);
   udev_unref(udev);

   return driver;
}

#endif /* HAVE_LIBUDEV */
