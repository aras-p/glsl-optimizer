/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
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
#include "i915_drm.h"
#include "xmlconfig.h"

/**
 * \brief Does X driver support DRI2BufferHiz and DRI2BufferStencil?
 *
 * (Here, "X driver" referes to the DDX driver, xf86-video-intel).
 *
 * The DRI2 protocol does not allow us to query the X driver's version nor
 * query for a list of buffer formats that the driver supports. So, to
 * determine if the X driver supports DRI2BufferHiz and DRI2BufferStencil we
 * must resort to a handshake.
 *
 * If the hardware lacks support for separate stencil (and consequently, lacks
 * support for hiz also), then the X driver's separate stencil and hiz support
 * is irrelevant and the handshake never occurs.
 *
 * Complications
 * -------------
 * The handshake is complicated by a bug in xf86-video-intel 2.15. Even though
 * that version of the X driver did not supppot requests for DRI2BufferHiz or
 * DRI2BufferStencil, if requested one it still allocated and returned one.
 * The returned buffer, however, was incorrectly X tiled.
 *
 * How the handshake works
 * -----------------------
 * Initially, intel_screen.dri2_has_hiz is set to unknown. The first time the
 * user requests a depth and stencil buffer, intelCreateBuffers() creates a
 * framebuffer with separate depth and stencil attachments (with formats
 * x8_z24 and s8).
 *
 * Eventually, intel_update_renderbuffers() makes a DRI2 request for
 * DRI2BufferStencil and DRI2BufferHiz. If the stencil buffer's tiling is
 * I915_TILING_NONE [1], then we joyfully set intel_screen.dri2_has_hiz to
 * true and continue as if nothing happend.
 *
 * [1] The stencil buffer is actually W tiled. However, we request from the
 *     kernel a non-tiled buffer because the GTT is incapable of W fencing.
 *
 * If the buffers are X tiled, however, the handshake has failed and we must
 * clean up.
 *    1. Angrily set intel_screen.dri2_has_hiz to false.
 *    2. Discard the framebuffer's depth and stencil attachments.
 *    3. Attach a packed depth/stencil buffer to the framebuffer (with format
 *       s8_z24).
 *    4. Make a DRI2 request for the new buffer, using attachment type
 *       DRI2BufferDepthStencil).
 *
 * Future Considerations
 * ---------------------
 * On a sunny day in the far future, when we are certain that no one has an
 * xf86-video-intel installed without hiz and separate stencil support, then
 * this enumerant and the handshake should die.
 */
enum intel_dri2_has_hiz {
   INTEL_DRI2_HAS_HIZ_UNKNOWN,
   INTEL_DRI2_HAS_HIZ_TRUE,
   INTEL_DRI2_HAS_HIZ_FALSE,
};

struct intel_screen
{
   int deviceID;
   int gen;

   int logTextureGranularity;

   __DRIscreen *driScrnPriv;

   bool no_hw;
   GLuint relaxed_relocations;

   /*
    * The hardware hiz and separate stencil fields are needed in intel_screen,
    * rather than solely in intel_context, because glXCreatePbuffer and
    * glXCreatePixmap are not passed a GLXContext.
    */
   bool hw_has_separate_stencil;
   bool hw_must_use_separate_stencil;
   bool hw_has_hiz;
   enum intel_dri2_has_hiz dri2_has_hiz;

   bool kernel_has_gen7_sol_reset;

   bool hw_has_llc;
   bool hw_has_swizzling;

   bool no_vbo;
   dri_bufmgr *bufmgr;
   struct _mesa_HashTable *named_regions;

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;
};

extern bool intelMapScreenRegions(__DRIscreen * sPriv);

extern void intelDestroyContext(__DRIcontext * driContextPriv);

extern GLboolean intelUnbindContext(__DRIcontext * driContextPriv);

extern GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv);

#endif
