/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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

#ifndef _INTEL_INIT_H_
#define _INTEL_INIT_H_

#include <stdbool.h>
#include <sys/time.h>
#include "dri_util.h"
#include "intel_bufmgr.h"
#include "brw_device_info.h"
#include "i915_drm.h"
#include "xmlconfig.h"

struct intel_screen
{
   int deviceID;
   const struct brw_device_info *devinfo;

   __DRIscreen *driScrnPriv;

   bool no_hw;

   bool hw_must_use_separate_stencil;

   bool hw_has_swizzling;

   /**
    * Does the kernel support context reset notifications?
    */
   bool has_context_reset_notification;

   dri_bufmgr *bufmgr;

   /**
    * A unique ID for shader programs.
    */
   unsigned program_id;

   int winsys_msaa_samples_override;

   struct {
      struct ra_regs *regs;

      /**
       * Array of the ra classes for the unaligned contiguous register
       * block sizes used.
       */
      int *classes;

      /**
       * Mapping for register-allocated objects in *regs to the first
       * GRF for that object.
       */
      uint8_t *ra_reg_to_grf;
   } vec4_reg_set;

   struct {
      struct ra_regs *regs;

      /**
       * Array of the ra classes for the unaligned contiguous register
       * block sizes used, indexed by register size.
       */
      int classes[16];

      /**
       * Mapping from classes to ra_reg ranges.  Each of the per-size
       * classes corresponds to a range of ra_reg nodes.  This array stores
       * those ranges in the form of first ra_reg in each class and the
       * total number of ra_reg elements in the last array element.  This
       * way the range of the i'th class is given by:
       * [ class_to_ra_reg_range[i], class_to_ra_reg_range[i+1] )
       */
      int class_to_ra_reg_range[17];

      /**
       * Mapping for register-allocated objects in *regs to the first
       * GRF for that object.
       */
      uint8_t *ra_reg_to_grf;

      /**
       * ra class for the aligned pairs we use for PLN, which doesn't
       * appear in *classes.
       */
      int aligned_pairs_class;
   } wm_reg_sets[2];

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;
};

extern void intelDestroyContext(__DRIcontext * driContextPriv);

extern GLboolean intelUnbindContext(__DRIcontext * driContextPriv);

PUBLIC const __DRIextension **__driDriverGetExtensions_i965(void);

extern GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv);

double get_time(void);
void aub_dump_bmp(struct gl_context *ctx);

const int*
intel_supported_msaa_modes(const struct intel_screen  *screen);

#endif
