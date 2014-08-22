/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
 *           2013 Red Hat, Inc.
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
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <xf86drm.h>

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "state_tracker/sw_winsys.h"
#include "state_tracker/drm_driver.h"

#if 0
#define DEBUG(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
#else
#define DEBUG(msg, ...)
#endif

struct sw_winsys;

struct sw_winsys *kms_dri_create_winsys(int fd);

struct kms_sw_displaytarget
{
   enum pipe_format format;
   unsigned width;
   unsigned height;
   unsigned stride;
   unsigned size;

   uint32_t handle;
   void *mapped;

   int ref_count;
   struct list_head link;
};

struct kms_sw_winsys
{
   struct sw_winsys base;

   int fd;
   struct list_head bo_list;
};

static INLINE struct kms_sw_displaytarget *
kms_sw_displaytarget( struct sw_displaytarget *dt )
{
   return (struct kms_sw_displaytarget *)dt;
}

static INLINE struct kms_sw_winsys *
kms_sw_winsys( struct sw_winsys *ws )
{
   return (struct kms_sw_winsys *)ws;
}


static boolean
kms_sw_is_displaytarget_format_supported( struct sw_winsys *ws,
                                          unsigned tex_usage,
                                          enum pipe_format format )
{
   /* TODO: check visuals or other sensible thing here */
   return TRUE;
}

static struct sw_displaytarget *
kms_sw_displaytarget_create(struct sw_winsys *ws,
                            unsigned tex_usage,
                            enum pipe_format format,
                            unsigned width, unsigned height,
                            unsigned alignment,
                            unsigned *stride)
{
   struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt;
   struct drm_mode_create_dumb create_req;
   struct drm_mode_destroy_dumb destroy_req;
   int ret;

   kms_sw_dt = CALLOC_STRUCT(kms_sw_displaytarget);
   if (!kms_sw_dt)
      goto no_dt;

   kms_sw_dt->ref_count = 1;

   kms_sw_dt->format = format;
   kms_sw_dt->width = width;
   kms_sw_dt->height = height;

   create_req.bpp = 32;
   create_req.width = width;
   create_req.height = height;
   create_req.handle = 0;
   ret = drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
   if (ret)
      goto free_bo;

   kms_sw_dt->stride = create_req.pitch;
   kms_sw_dt->size = create_req.size;
   kms_sw_dt->handle = create_req.handle;

   list_add(&kms_sw_dt->link, &kms_sw->bo_list);

   DEBUG("KMS-DEBUG: created buffer %u (size %u)\n", kms_sw_dt->handle, kms_sw_dt->size);

   *stride = kms_sw_dt->stride;
   return (struct sw_displaytarget *)kms_sw_dt;

 free_bo:
   memset(&destroy_req, 0, sizeof destroy_req);
   destroy_req.handle = create_req.handle;
   drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
   FREE(kms_sw_dt);
 no_dt:
   return NULL;
}

static void
kms_sw_displaytarget_destroy(struct sw_winsys *ws,
                             struct sw_displaytarget *dt)
{
   struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt = kms_sw_displaytarget(dt);
   struct drm_mode_destroy_dumb destroy_req;

   kms_sw_dt->ref_count --;
   if (kms_sw_dt->ref_count > 0)
      return;

   memset(&destroy_req, 0, sizeof destroy_req);
   destroy_req.handle = kms_sw_dt->handle;
   drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);

   list_del(&kms_sw_dt->link);

   DEBUG("KMS-DEBUG: destroyed buffer %u\n", kms_sw_dt->handle);

   FREE(kms_sw_dt);
}

static void *
kms_sw_displaytarget_map(struct sw_winsys *ws,
                         struct sw_displaytarget *dt,
                         unsigned flags)
{
   struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt = kms_sw_displaytarget(dt);
   struct drm_mode_map_dumb map_req;
   int prot, ret;

   memset(&map_req, 0, sizeof map_req);
   map_req.handle = kms_sw_dt->handle;
   ret = drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req);
   if (ret)
      return NULL;

   prot = (flags == PIPE_TRANSFER_READ) ? PROT_READ : (PROT_READ | PROT_WRITE);
   kms_sw_dt->mapped = mmap(0, kms_sw_dt->size, prot, MAP_SHARED,
                            kms_sw->fd, map_req.offset);

   if (kms_sw_dt->mapped == MAP_FAILED)
      return NULL;

   DEBUG("KMS-DEBUG: mapped buffer %u (size %u) at %p\n",
         kms_sw_dt->handle, kms_sw_dt->size, kms_sw_dt->mapped);

   return kms_sw_dt->mapped;
}

static struct kms_sw_displaytarget *
kms_sw_displaytarget_add_from_prime(struct kms_sw_winsys *kms_sw, int fd)
{
   uint32_t handle = -1;
   struct kms_sw_displaytarget * kms_sw_dt;
   int ret;

   ret = drmPrimeFDToHandle(kms_sw->fd, fd, &handle);

   if (ret)
      return NULL;

   kms_sw_dt = CALLOC_STRUCT(kms_sw_displaytarget);
   if (!kms_sw_dt)
      return NULL;

   kms_sw_dt->ref_count = 1;
   kms_sw_dt->handle = handle;
   kms_sw_dt->size = lseek(fd, 0, SEEK_END);

   if (kms_sw_dt->size == (off_t)-1) {
      FREE(kms_sw_dt);
      return NULL;
   }

   lseek(fd, 0, SEEK_SET);

   list_add(&kms_sw_dt->link, &kms_sw->bo_list);

   return kms_sw_dt;
}

static void
kms_sw_displaytarget_unmap(struct sw_winsys *ws,
                           struct sw_displaytarget *dt)
{
   struct kms_sw_displaytarget *kms_sw_dt = kms_sw_displaytarget(dt);

   DEBUG("KMS-DEBUG: unmapped buffer %u (was %p)\n", kms_sw_dt->handle, kms_sw_dt->mapped);

   munmap(kms_sw_dt->mapped, kms_sw_dt->size);
   kms_sw_dt->mapped = NULL;
}

static struct sw_displaytarget *
kms_sw_displaytarget_from_handle(struct sw_winsys *ws,
                                 const struct pipe_resource *templ,
                                 struct winsys_handle *whandle,
                                 unsigned *stride)
{
   struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt;

   assert(whandle->type == DRM_API_HANDLE_TYPE_KMS ||
          whandle->type == DRM_API_HANDLE_TYPE_FD);

   switch(whandle->type) {
   case DRM_API_HANDLE_TYPE_FD:
      kms_sw_dt = kms_sw_displaytarget_add_from_prime(kms_sw, whandle->handle);
      if (kms_sw_dt) {
         kms_sw_dt->ref_count++;
         kms_sw_dt->width = templ->width0;
         kms_sw_dt->height = templ->height0;
         kms_sw_dt->stride = whandle->stride;
         *stride = kms_sw_dt->stride;
      }
      return (struct sw_displaytarget *)kms_sw_dt;
   case DRM_API_HANDLE_TYPE_KMS:
      LIST_FOR_EACH_ENTRY(kms_sw_dt, &kms_sw->bo_list, link) {
         if (kms_sw_dt->handle == whandle->handle) {
            kms_sw_dt->ref_count++;

            DEBUG("KMS-DEBUG: imported buffer %u (size %u)\n", kms_sw_dt->handle, kms_sw_dt->size);

            *stride = kms_sw_dt->stride;
            return (struct sw_displaytarget *)kms_sw_dt;
         }
      }
      /* fallthrough */
   default:
      break;
   }

   assert(0);
   return NULL;
}

static boolean
kms_sw_displaytarget_get_handle(struct sw_winsys *winsys,
                                struct sw_displaytarget *dt,
                                struct winsys_handle *whandle)
{
   struct kms_sw_winsys *kms_sw = kms_sw_winsys(winsys);
   struct kms_sw_displaytarget *kms_sw_dt = kms_sw_displaytarget(dt);

   switch(whandle->type) {
   case DRM_API_HANDLE_TYPE_KMS:
      whandle->handle = kms_sw_dt->handle;
      whandle->stride = kms_sw_dt->stride;
      return TRUE;
   case DRM_API_HANDLE_TYPE_FD:
      if (!drmPrimeHandleToFD(kms_sw->fd, kms_sw_dt->handle,
                             DRM_CLOEXEC, &whandle->handle)) {
         whandle->stride = kms_sw_dt->stride;
         return TRUE;
      }
      /* fallthrough */
   default:
      whandle->handle = 0;
      whandle->stride = 0;
      return FALSE;
   }
}

static void
kms_sw_displaytarget_display(struct sw_winsys *ws,
                             struct sw_displaytarget *dt,
                             void *context_private,
                             struct pipe_box *box)
{
   /* This function should not be called, instead the dri2 loader should
      handle swap buffers internally.
   */
   assert(0);
}


static void
kms_destroy_sw_winsys(struct sw_winsys *winsys)
{
   FREE(winsys);
}

struct sw_winsys *
kms_dri_create_winsys(int fd)
{
   struct kms_sw_winsys *ws;

   ws = CALLOC_STRUCT(kms_sw_winsys);
   if (!ws)
      return NULL;

   ws->fd = fd;
   list_inithead(&ws->bo_list);

   ws->base.destroy = kms_destroy_sw_winsys;

   ws->base.is_displaytarget_format_supported = kms_sw_is_displaytarget_format_supported;

   /* screen texture functions */
   ws->base.displaytarget_create = kms_sw_displaytarget_create;
   ws->base.displaytarget_destroy = kms_sw_displaytarget_destroy;
   ws->base.displaytarget_from_handle = kms_sw_displaytarget_from_handle;
   ws->base.displaytarget_get_handle = kms_sw_displaytarget_get_handle;

   /* texture functions */
   ws->base.displaytarget_map = kms_sw_displaytarget_map;
   ws->base.displaytarget_unmap = kms_sw_displaytarget_unmap;

   ws->base.displaytarget_display = kms_sw_displaytarget_display;

   return &ws->base;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
