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

#include "intel_chipset.h"

#include "ilo_3d.h"
#include "ilo_blit.h"
#include "ilo_gpgpu.h"
#include "ilo_query.h"
#include "ilo_resource.h"
#include "ilo_screen.h"
#include "ilo_state.h"
#include "ilo_video.h"
#include "ilo_context.h"

static void
ilo_context_destroy(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);

   FREE(ilo);
}

static struct pipe_context *
ilo_context_create(struct pipe_screen *screen, void *priv)
{
   struct ilo_screen *is = ilo_screen(screen);
   struct ilo_context *ilo;

   ilo = CALLOC_STRUCT(ilo_context);
   if (!ilo)
      return NULL;

   ilo->winsys = is->winsys;
   ilo->devid = is->devid;
   ilo->gen = is->gen;

   if (IS_SNB_GT1(ilo->devid) ||
       IS_IVB_GT1(ilo->devid) ||
       IS_HSW_GT1(ilo->devid) ||
       IS_BAYTRAIL(ilo->devid))
      ilo->gt = 1;
   else if (IS_SNB_GT2(ilo->devid) ||
            IS_IVB_GT2(ilo->devid) ||
            IS_HSW_GT2(ilo->devid))
      ilo->gt = 2;
   else
      ilo->gt = 0;

   /* stolen from classic i965 */
   /* WM maximum threads is number of EUs times number of threads per EU. */
   if (ilo->gen >= ILO_GEN(7)) {
      if (ilo->gt == 1) {
	 ilo->max_wm_threads = 48;
	 ilo->max_vs_threads = 36;
	 ilo->max_gs_threads = 36;
	 ilo->urb.size = 128;
	 ilo->urb.max_vs_entries = 512;
	 ilo->urb.max_gs_entries = 192;
      } else if (ilo->gt == 2) {
	 ilo->max_wm_threads = 172;
	 ilo->max_vs_threads = 128;
	 ilo->max_gs_threads = 128;
	 ilo->urb.size = 256;
	 ilo->urb.max_vs_entries = 704;
	 ilo->urb.max_gs_entries = 320;
      } else {
	 assert(!"Unknown gen7 device.");
      }
   } else if (ilo->gen == ILO_GEN(6)) {
      if (ilo->gt == 2) {
	 ilo->max_wm_threads = 80;
	 ilo->max_vs_threads = 60;
	 ilo->max_gs_threads = 60;
	 ilo->urb.size = 64;            /* volume 5c.5 section 5.1 */
	 ilo->urb.max_vs_entries = 256; /* volume 2a (see 3DSTATE_URB) */
	 ilo->urb.max_gs_entries = 256;
      } else {
	 ilo->max_wm_threads = 40;
	 ilo->max_vs_threads = 24;
	 ilo->max_gs_threads = 21; /* conservative; 24 if rendering disabled */
	 ilo->urb.size = 32;            /* volume 5c.5 section 5.1 */
	 ilo->urb.max_vs_entries = 256; /* volume 2a (see 3DSTATE_URB) */
	 ilo->urb.max_gs_entries = 256;
      }
   }

   ilo->base.screen = screen;
   ilo->base.priv = priv;

   ilo->base.destroy = ilo_context_destroy;
   ilo->base.flush = NULL;

   ilo_init_3d_functions(ilo);
   ilo_init_query_functions(ilo);
   ilo_init_state_functions(ilo);
   ilo_init_blit_functions(ilo);
   ilo_init_transfer_functions(ilo);
   ilo_init_video_functions(ilo);
   ilo_init_gpgpu_functions(ilo);

   return &ilo->base;
}

/**
 * Initialize context-related functions.
 */
void
ilo_init_context_functions(struct ilo_screen *is)
{
   is->base.context_create = ilo_context_create;
}
