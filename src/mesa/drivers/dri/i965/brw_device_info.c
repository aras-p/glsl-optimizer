/*
 * Copyright © 2013 Intel Corporation
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "brw_device_info.h"

static const struct brw_device_info brw_device_info_i965 = {
   .gen = 4,
   .has_negative_rhw_bug = true,
   .needs_unlit_centroid_workaround = true,
   .max_vs_threads = 16,
   .max_gs_threads = 2,
   .max_wm_threads = 8 * 4,
   .urb = {
      .size = 256,
   },
};

static const struct brw_device_info brw_device_info_g4x = {
   .gen = 4,
   .has_pln = true,
   .has_compr4 = true,
   .has_surface_tile_offset = true,
   .needs_unlit_centroid_workaround = true,
   .is_g4x = true,
   .max_vs_threads = 32,
   .max_gs_threads = 2,
   .max_wm_threads = 10 * 5,
   .urb = {
      .size = 384,
   },
};

static const struct brw_device_info brw_device_info_ilk = {
   .gen = 5,
   .has_pln = true,
   .has_compr4 = true,
   .has_surface_tile_offset = true,
   .needs_unlit_centroid_workaround = true,
   .max_vs_threads = 72,
   .max_gs_threads = 32,
   .max_wm_threads = 12 * 6,
   .urb = {
      .size = 1024,
   },
};

static const struct brw_device_info brw_device_info_snb_gt1 = {
   .gen = 6,
   .gt = 1,
   .has_hiz_and_separate_stencil = true,
   .has_llc = true,
   .has_pln = true,
   .has_surface_tile_offset = true,
   .needs_unlit_centroid_workaround = true,
   .max_vs_threads = 24,
   .max_gs_threads = 21, /* conservative; 24 if rendering disabled. */
   .max_wm_threads = 40,
   .urb = {
      .size = 32,
      .min_vs_entries = 24,
      .max_vs_entries = 256,
      .max_gs_entries = 256,
   },
};

static const struct brw_device_info brw_device_info_snb_gt2 = {
   .gen = 6,
   .gt = 2,
   .has_hiz_and_separate_stencil = true,
   .has_llc = true,
   .has_pln = true,
   .has_surface_tile_offset = true,
   .needs_unlit_centroid_workaround = true,
   .max_vs_threads = 60,
   .max_gs_threads = 60,
   .max_wm_threads = 80,
   .urb = {
      .size = 64,
      .min_vs_entries = 24,
      .max_vs_entries = 256,
      .max_gs_entries = 256,
   },
};

#define GEN7_FEATURES                               \
   .gen = 7,                                        \
   .has_hiz_and_separate_stencil = true,            \
   .must_use_separate_stencil = true,               \
   .has_llc = true,                                 \
   .has_pln = true,                                 \
   .has_surface_tile_offset = true,                 \
   .needs_unlit_centroid_workaround = true

static const struct brw_device_info brw_device_info_ivb_gt1 = {
   GEN7_FEATURES, .is_ivybridge = true, .gt = 1,
   .max_vs_threads = 36,
   .max_gs_threads = 36,
   .max_wm_threads = 48,
   .urb = {
      .size = 128,
      .min_vs_entries = 32,
      .max_vs_entries = 512,
      .max_gs_entries = 192,
   },
};

static const struct brw_device_info brw_device_info_ivb_gt2 = {
   GEN7_FEATURES, .is_ivybridge = true, .gt = 2,
   .max_vs_threads = 128,
   .max_gs_threads = 128,
   .max_wm_threads = 172,
   .urb = {
      .size = 256,
      .min_vs_entries = 32,
      .max_vs_entries = 704,
      .max_gs_entries = 320,
   },
};

static const struct brw_device_info brw_device_info_byt = {
   GEN7_FEATURES, .is_baytrail = true, .gt = 1,
   .has_llc = false,
   .max_vs_threads = 36,
   .max_gs_threads = 36,
   .max_wm_threads = 48,
   .urb = {
      .size = 128,
      .min_vs_entries = 32,
      .max_vs_entries = 512,
      .max_gs_entries = 192,
   },
};

static const struct brw_device_info brw_device_info_hsw_gt1 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 1,
   .max_vs_threads = 70,
   .max_gs_threads = 70,
   .max_wm_threads = 102,
   .urb = {
      .size = 128,
      .min_vs_entries = 32,
      .max_vs_entries = 640,
      .max_gs_entries = 256,
   },
};

static const struct brw_device_info brw_device_info_hsw_gt2 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 2,
   .max_vs_threads = 280,
   .max_gs_threads = 256,
   .max_wm_threads = 204,
   .urb = {
      .size = 256,
      .min_vs_entries = 64,
      .max_vs_entries = 1664,
      .max_gs_entries = 640,
   },
};

static const struct brw_device_info brw_device_info_hsw_gt3 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 3,
   .max_vs_threads = 280,
   .max_gs_threads = 256,
   .max_wm_threads = 408,
   .urb = {
      .size = 512,
      .min_vs_entries = 64,
      .max_vs_entries = 1664,
      .max_gs_entries = 640,
   },
};

#define GEN8_FEATURES                               \
   .gen = 8,                                        \
   .has_hiz_and_separate_stencil = true,            \
   .must_use_separate_stencil = true,               \
   .has_llc = true,                                 \
   .has_pln = true,                                 \
   .max_vs_threads = 504,                           \
   .max_gs_threads = 504,                           \
   .max_wm_threads = 384                            \

static const struct brw_device_info brw_device_info_bdw_gt1 = {
   GEN8_FEATURES, .gt = 1,
   .urb = {
      .size = 192,
      .min_vs_entries = 64,
      .max_vs_entries = 2560,
      .max_gs_entries = 960,
   }
};

static const struct brw_device_info brw_device_info_bdw_gt2 = {
   GEN8_FEATURES, .gt = 2,
   .urb = {
      .size = 384,
      .min_vs_entries = 64,
      .max_vs_entries = 2560,
      .max_gs_entries = 960,
   }
};

static const struct brw_device_info brw_device_info_bdw_gt3 = {
   GEN8_FEATURES, .gt = 3,
   .urb = {
      .size = 384,
      .min_vs_entries = 64,
      .max_vs_entries = 2560,
      .max_gs_entries = 960,
   }
};

/* Thread counts and URB limits are placeholders, and may not be accurate.
 * These were copied from Haswell GT1, above.
 */
static const struct brw_device_info brw_device_info_chv = {
   GEN8_FEATURES, .is_cherryview = 1, .gt = 1,
   .has_llc = false,
   .max_vs_threads = 70,
   .max_gs_threads = 70,
   .max_wm_threads = 102,
   .urb = {
      .size = 128,
      .min_vs_entries = 64,
      .max_vs_entries = 640,
      .max_gs_entries = 256,
   }
};

const struct brw_device_info *
brw_get_device_info(int devid)
{
   switch (devid) {
#undef CHIPSET
#define CHIPSET(id, family, name) case id: return &brw_device_info_##family;
#include "pci_ids/i965_pci_ids.h"
   default:
      fprintf(stderr, "i965_dri.so does not support the 0x%x PCI ID.\n", devid);
      return NULL;
   }
}
