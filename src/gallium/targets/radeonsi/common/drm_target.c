/**************************************************************************
 *
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeon/drm/radeon_winsys.h"
#include "radeonsi/si_public.h"

static struct pipe_screen *create_screen(int fd)
{
   struct radeon_winsys *radeon;

   radeon = radeon_drm_winsys_create(fd, radeonsi_screen_create);
   return radeon ? debug_screen_wrap(radeon->screen) : NULL;
}

static const struct drm_conf_ret throttle_ret = {
   .type = DRM_CONF_INT,
   .val.val_int = 2,
};

/* Technically this is only true for kernels >= 3.12, which
 * support lseek on dma-buf fds.
 *
 * We could check for this in create_screen and return the correct
 * value, but for now just return true in all cases.
 * 
 * createImageFromFds fails gracefully on kernel < 3.12, so this
 * shouldn't be a huge problem.
 */
static const struct drm_conf_ret share_fd_ret = {
   .type = DRM_CONF_BOOL,
   .val.val_int = true,
};

static const struct drm_conf_ret *drm_configuration(enum drm_conf conf)
{
   switch (conf) {
   case DRM_CONF_THROTTLE:
      return &throttle_ret;
   case DRM_CONF_SHARE_FD:
      return &share_fd_ret;
   default:
      break;
   }
   return NULL;
}

DRM_DRIVER_DESCRIPTOR("radeonsi", "radeon", create_screen, drm_configuration)
