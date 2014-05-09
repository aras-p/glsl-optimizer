/**************************************************************************
 *
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
 **************************************************************************/

#include "pipe_loader_priv.h"

#include "util/u_memory.h"
#include "util/u_dl.h"
#include "sw/dri/dri_sw_winsys.h"
#include "sw/null/null_sw_winsys.h"
#ifdef HAVE_PIPE_LOADER_XLIB
/* Explicitly wrap the header to ease build without X11 headers */
#include "sw/xlib/xlib_sw_winsys.h"
#endif
#include "target-helpers/inline_sw_helper.h"
#include "state_tracker/drisw_api.h"

struct pipe_loader_sw_device {
   struct pipe_loader_device base;
   struct util_dl_library *lib;
   struct sw_winsys *ws;
};

#define pipe_loader_sw_device(dev) ((struct pipe_loader_sw_device *)dev)

static struct pipe_loader_ops pipe_loader_sw_ops;

static struct sw_winsys *(*backends[])() = {
   null_sw_create
};

#ifdef HAVE_PIPE_LOADER_XLIB
bool
pipe_loader_sw_probe_xlib(struct pipe_loader_device **devs, Display *display)
{
   struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);

   if (!sdev)
      return false;

   sdev->base.type = PIPE_LOADER_DEVICE_SOFTWARE;
   sdev->base.driver_name = "swrast";
   sdev->base.ops = &pipe_loader_sw_ops;
   sdev->ws = xlib_create_sw_winsys(display);
   if (!sdev->ws) {
      FREE(sdev);
      return false;
   }
   *devs = &sdev->base;

   return true;
}
#endif

#ifdef HAVE_PIPE_LOADER_DRI
bool
pipe_loader_sw_probe_dri(struct pipe_loader_device **devs, struct drisw_loader_funcs *drisw_lf)
{
   struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);

   if (!sdev)
      return false;

   sdev->base.type = PIPE_LOADER_DEVICE_SOFTWARE;
   sdev->base.driver_name = "swrast";
   sdev->base.ops = &pipe_loader_sw_ops;
   sdev->ws = dri_create_sw_winsys(drisw_lf);
   if (!sdev->ws) {
      FREE(sdev);
      return false;
   }
   *devs = &sdev->base;

   return true;
}
#endif

bool
pipe_loader_sw_probe_null(struct pipe_loader_device **devs)
{
   struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);

   if (!sdev)
      return false;

   sdev->base.type = PIPE_LOADER_DEVICE_SOFTWARE;
   sdev->base.driver_name = "swrast";
   sdev->base.ops = &pipe_loader_sw_ops;
   sdev->ws = null_sw_create();
   if (!sdev->ws) {
      FREE(sdev);
      return false;
   }
   *devs = &sdev->base;

   return true;
}

int
pipe_loader_sw_probe(struct pipe_loader_device **devs, int ndev)
{
   int i;

   for (i = 0; i < Elements(backends); i++) {
      if (i < ndev) {
         struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
	 /* TODO: handle CALLOC_STRUCT failure */

         sdev->base.type = PIPE_LOADER_DEVICE_SOFTWARE;
         sdev->base.driver_name = "swrast";
         sdev->base.ops = &pipe_loader_sw_ops;
         sdev->ws = backends[i]();
         devs[i] = &sdev->base;
      }
   }

   return i;
}

static void
pipe_loader_sw_release(struct pipe_loader_device **dev)
{
   struct pipe_loader_sw_device *sdev = pipe_loader_sw_device(*dev);

   if (sdev->lib)
      util_dl_close(sdev->lib);

   FREE(sdev);
   *dev = NULL;
}

static struct pipe_screen *
pipe_loader_sw_create_screen(struct pipe_loader_device *dev,
                             const char *library_paths)
{
   struct pipe_loader_sw_device *sdev = pipe_loader_sw_device(dev);
   struct pipe_screen *(*init)(struct sw_winsys *);

   if (!sdev->lib)
      sdev->lib = pipe_loader_find_module(dev, library_paths);
   if (!sdev->lib)
      return NULL;

   init = (void *)util_dl_get_proc_address(sdev->lib, "swrast_create_screen");
   if (!init){
      util_dl_close(sdev->lib);
      sdev->lib = NULL;
      return NULL;
   }

   return init(sdev->ws);
}

static struct pipe_loader_ops pipe_loader_sw_ops = {
   .create_screen = pipe_loader_sw_create_screen,
   .release = pipe_loader_sw_release
};
