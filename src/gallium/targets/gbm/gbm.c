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

#include "util/u_inlines.h"

#include "gbm_gallium_drmint.h"
#include "pipe_loader.h"

static struct pipe_screen *
create_drm_screen(const char *name, int fd)
{
   struct pipe_module *pmod = get_pipe_module(name);
 
   return (pmod && pmod->drmdd && pmod->drmdd->create_screen) ?
      pmod->drmdd->create_screen(fd) : NULL;
}

int
gallium_screen_create(struct gbm_gallium_drm_device *gdrm)
{
   gdrm->base.driver_name = drm_fd_get_screen_name(gdrm->base.base.fd);
   if (gdrm->base.driver_name == NULL)
      return -1;

   gdrm->screen = create_drm_screen(gdrm->base.driver_name, gdrm->base.base.fd);
   if (gdrm->screen == NULL) {
      debug_printf("failed to load driver: %s\n", gdrm->base.driver_name);
      return -1;
   };

   return 0;
}

GBM_EXPORT struct gbm_backend gbm_backend = {
   .backend_name = "gallium_drm",
   .create_device = gbm_gallium_drm_device_create,
};
