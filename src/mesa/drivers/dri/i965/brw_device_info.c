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
};

static const struct brw_device_info brw_device_info_g4x = {
   .gen = 4,
   .is_g4x = true,
};

static const struct brw_device_info brw_device_info_ilk = {
   .gen = 5,
};

static const struct brw_device_info brw_device_info_snb_gt1 = {
   .gen = 6,
   .gt = 2,
   .has_hiz_and_separate_stencil = true,
   .has_llc = true,
};

static const struct brw_device_info brw_device_info_snb_gt2 = {
   .gen = 6,
   .gt = 2,
   .has_hiz_and_separate_stencil = true,
   .has_llc = true,
};

#define GEN7_FEATURES                               \
   .gen = 7,                                        \
   .has_hiz_and_separate_stencil = true,            \
   .must_use_separate_stencil = true,               \
   .has_llc = true

static const struct brw_device_info brw_device_info_ivb_gt1 = {
   GEN7_FEATURES, .is_ivybridge = true, .gt = 1,
};

static const struct brw_device_info brw_device_info_ivb_gt2 = {
   GEN7_FEATURES, .is_ivybridge = true, .gt = 2,
};

static const struct brw_device_info brw_device_info_byt = {
   GEN7_FEATURES, .is_baytrail = true, .gt = 1,
   .has_llc = false,
};

static const struct brw_device_info brw_device_info_hsw_gt1 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 1,
};

static const struct brw_device_info brw_device_info_hsw_gt2 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 2,
};

static const struct brw_device_info brw_device_info_hsw_gt3 = {
   GEN7_FEATURES, .is_haswell = true, .gt = 3,
};

const struct brw_device_info *
brw_get_device_info(int devid)
{
   switch (devid) {
#undef CHIPSET
#define CHIPSET(id, family, name) case id: return &brw_device_info_##family;
#include "pci_ids/i965_pci_ids.h"
   default:
      fprintf(stderr, "Unknown Intel device.");
      abort();
   }
}
