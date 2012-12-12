/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_format_s3tc.h"
#include "intel_chipset.h"
#include "intel_winsys.h"

#include "ilo_context.h"
#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_public.h"
#include "ilo_screen.h"

int ilo_debug;

static const struct debug_named_value ilo_debug_flags[] = {
   { "3d",        ILO_DEBUG_3D,       "Dump 3D commands and states" },
   { "vs",        ILO_DEBUG_VS,       "Dump vertex shaders" },
   { "gs",        ILO_DEBUG_GS,       "Dump geometry shaders" },
   { "fs",        ILO_DEBUG_FS,       "Dump fragment shaders" },
   { "cs",        ILO_DEBUG_CS,       "Dump compute shaders" },
   { "nohw",      ILO_DEBUG_NOHW,     "Do not send commands to HW" },
   { "nocache",   ILO_DEBUG_NOCACHE,  "Always invalidate HW caches" },
   DEBUG_NAMED_VALUE_END
};

static void
ilo_screen_destroy(struct pipe_screen *screen)
{
   struct ilo_screen *is = ilo_screen(screen);

   /* as it seems, winsys is owned by the screen */
   is->winsys->destroy(is->winsys);

   FREE(is);
}

struct pipe_screen *
ilo_screen_create(struct intel_winsys *ws)
{
   struct ilo_screen *is;
   const struct intel_winsys_info *info;

   ilo_debug = debug_get_flags_option("ILO_DEBUG", ilo_debug_flags, 0);

   is = CALLOC_STRUCT(ilo_screen);
   if (!is)
      return NULL;

   is->winsys = ws;

   info = is->winsys->get_info(is->winsys);

   is->devid = info->devid;
   if (IS_GEN7(info->devid)) {
      is->gen = ILO_GEN(7);
   }
   else if (IS_GEN6(info->devid)) {
      is->gen = ILO_GEN(6);
   }
   else {
      ilo_err("unknown GPU generation\n");
      FREE(is);
      return NULL;
   }

   is->has_llc = info->has_llc;

   util_format_s3tc_init();

   is->base.destroy = ilo_screen_destroy;
   is->base.get_name = NULL;
   is->base.get_vendor = NULL;
   is->base.get_param = NULL;
   is->base.get_paramf = NULL;
   is->base.get_shader_param = NULL;
   is->base.get_video_param = NULL;
   is->base.get_compute_param = NULL;

   is->base.get_timestamp = NULL;

   is->base.flush_frontbuffer = NULL;

   is->base.fence_reference = NULL;
   is->base.fence_signalled = NULL;
   is->base.fence_finish = NULL;

   is->base.get_driver_query_info = NULL;

   ilo_init_format_functions(is);
   ilo_init_context_functions(is);
   ilo_init_resource_functions(is);

   return &is->base;
}
