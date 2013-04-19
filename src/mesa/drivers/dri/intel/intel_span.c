/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Intel Corporation
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
 * Authors:
 *     Chad Versace <chad@chad-versace.us>
 *
 **************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"

#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"
#include "intel_span.h"
#include "intel_regions.h"
#include "intel_tex.h"

#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"

/**
 * Prepare for software rendering.  Map current read/draw framebuffers'
 * renderbuffers and all currently bound texture objects.
 */
void
intelSpanRenderStart(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_prepare_render(intel);
   intel_flush(ctx);

   _swrast_map_textures(ctx);
   _swrast_map_renderbuffers(ctx);
}

/**
 * Called when done software rendering.  Unmap the buffers we mapped in
 * the above function.
 */
void
intelSpanRenderFinish(struct gl_context * ctx)
{
   _swrast_flush(ctx);
   _swrast_unmap_textures(ctx);
   _swrast_unmap_renderbuffers(ctx);
}


void
intelInitSpanFuncs(struct gl_context * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   if (swdd) {
      swdd->SpanRenderStart = intelSpanRenderStart;
      swdd->SpanRenderFinish = intelSpanRenderFinish;
   }
}
