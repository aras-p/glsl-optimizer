/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009, 2012 Intel Corporation.
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

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/condrender.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_blit.h"
#include "intel_clear.h"
#include "intel_fbo.h"
#include "intel_regions.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

static const char *buffer_names[] = {
   [BUFFER_FRONT_LEFT] = "front",
   [BUFFER_BACK_LEFT] = "back",
   [BUFFER_FRONT_RIGHT] = "front right",
   [BUFFER_BACK_RIGHT] = "back right",
   [BUFFER_DEPTH] = "depth",
   [BUFFER_STENCIL] = "stencil",
   [BUFFER_ACCUM] = "accum",
   [BUFFER_AUX0] = "aux0",
   [BUFFER_COLOR0] = "color0",
   [BUFFER_COLOR1] = "color1",
   [BUFFER_COLOR2] = "color2",
   [BUFFER_COLOR3] = "color3",
   [BUFFER_COLOR4] = "color4",
   [BUFFER_COLOR5] = "color5",
   [BUFFER_COLOR6] = "color6",
   [BUFFER_COLOR7] = "color7",
};

static void
debug_mask(const char *name, GLbitfield mask)
{
   GLuint i;

   if (unlikely(INTEL_DEBUG & DEBUG_BLIT)) {
      DBG("%s clear:", name);
      for (i = 0; i < BUFFER_COUNT; i++) {
	 if (mask & (1 << i))
	    DBG(" %s", buffer_names[i]);
      }
      DBG("\n");
   }
}

/**
 * Called by ctx->Driver.Clear.
 */
static void
brw_clear(struct gl_context *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT)) {
      intel->front_buffer_dirty = true;
   }

   GLbitfield tri_mask = mask & (BUFFER_BITS_COLOR |
				 BUFFER_BIT_STENCIL |
				 BUFFER_BIT_DEPTH);

   if (tri_mask) {
      debug_mask("tri", tri_mask);
      mask &= ~tri_mask;
      _mesa_meta_glsl_Clear(&intel->ctx, tri_mask);
   }

   /* Any strange buffers get passed off to swrast */
   if (mask) {
      debug_mask("swrast", mask);
      _swrast_Clear(ctx, mask);
   }
}


void
intelInitClearFuncs(struct dd_function_table *functions)
{
   functions->Clear = brw_clear;
}
