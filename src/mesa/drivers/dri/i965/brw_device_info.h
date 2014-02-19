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
  *
  */

#pragma once
#include <stdbool.h>

struct brw_device_info
{
   int gen; /**< Generation number: 4, 5, 6, 7, ... */
   int gt;

   bool is_g4x;
   bool is_ivybridge;
   bool is_baytrail;
   bool is_haswell;
   bool is_cherryview;

   bool has_hiz_and_separate_stencil;
   bool must_use_separate_stencil;

   bool has_llc;

   bool has_pln;
   bool has_compr4;
   bool has_surface_tile_offset;

   /**
    * Quirks:
    *  @{
    */
   bool has_negative_rhw_bug;

   /**
    * Some versions of Gen hardware don't do centroid interpolation correctly
    * on unlit pixels, causing incorrect values for derivatives near triangle
    * edges.  Enabling this flag causes the fragment shader to use
    * non-centroid interpolation for unlit pixels, at the expense of two extra
    * fragment shader instructions.
    */
   bool needs_unlit_centroid_workaround;
   /** @} */

   /**
    * GPU Limits:
    *  @{
    */
   unsigned max_vs_threads;
   unsigned max_gs_threads;
   unsigned max_wm_threads;

   struct {
      unsigned size;
      unsigned min_vs_entries;
      unsigned max_vs_entries;
      unsigned max_gs_entries;
   } urb;
   /** @} */
};

const struct brw_device_info *brw_get_device_info(int devid);
