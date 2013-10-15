/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
**********************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"
#include "main/framebuffer.h"

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_program.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_wm.h"

#include "gen6_blorp.h"
#include "gen7_blorp.h"

#include "glsl/ralloc.h"

void brwInitVtbl( struct brw_context *brw )
{
   assert(brw->gen >= 4);
   if (brw->gen >= 7) {
      gen7_init_vtable_surface_functions(brw);
      gen7_init_vtable_sampler_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen7_emit_depth_stencil_hiz;
   } else if (brw->gen >= 4) {
      gen4_init_vtable_surface_functions(brw);
      gen4_init_vtable_sampler_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = brw_emit_depth_stencil_hiz;
   }
}
