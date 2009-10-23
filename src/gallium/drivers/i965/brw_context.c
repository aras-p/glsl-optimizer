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


#include "main/imports.h"
#include "main/api_noop.h"
#include "main/macros.h"
#include "main/vtxfmt.h"
#include "main/simple_list.h"
#include "shader/shader_api.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_state.h"
#include "brw_vs.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_batchbuffer.h"
#include "intel_pixel.h"
#include "intel_span.h"
#include "tnl/t_pipeline.h"

#include "utils.h"



GLboolean brwCreateContext( const __GLcontextModes *mesaVis,
			    __DRIcontextPrivate *driContextPriv,
			    void *sharedContextPrivate)
{
   struct brw_context *brw = (struct brw_context *) CALLOC_STRUCT(brw_context);

   if (!brw) {
      debug_printf("%s: failed to alloc context\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* We want the GLSL compiler to emit code that uses condition codes */
   ctx->Shader.EmitCondCodes = GL_TRUE;
   ctx->Shader.EmitNVTempInitialization = GL_TRUE;


   brw_init_query( brw );
   brw_init_state( brw );
   brw_draw_init( brw );

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   brw->emit_state_always = 0;

   make_empty_list(&brw->query.active_head);


   return GL_TRUE;
}

/**
 * called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   int i;

   brw_destroy_state(brw);
   brw_draw_destroy( brw );

   _mesa_free(brw->wm.compile_data);

   for (i = 0; i < brw->state.nr_color_regions; i++)
      intel_region_release(&brw->state.color_regions[i]);
   brw->state.nr_color_regions = 0;
   intel_region_release(&brw->state.depth_region);

   dri_bo_unreference(brw->curbe.curbe_bo);
   dri_bo_unreference(brw->vs.prog_bo);
   dri_bo_unreference(brw->vs.state_bo);
   dri_bo_unreference(brw->vs.bind_bo);
   dri_bo_unreference(brw->gs.prog_bo);
   dri_bo_unreference(brw->gs.state_bo);
   dri_bo_unreference(brw->clip.prog_bo);
   dri_bo_unreference(brw->clip.state_bo);
   dri_bo_unreference(brw->clip.vp_bo);
   dri_bo_unreference(brw->sf.prog_bo);
   dri_bo_unreference(brw->sf.state_bo);
   dri_bo_unreference(brw->sf.vp_bo);
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++)
      dri_bo_unreference(brw->wm.sdc_bo[i]);
   dri_bo_unreference(brw->wm.bind_bo);
   for (i = 0; i < BRW_WM_MAX_SURF; i++)
      dri_bo_unreference(brw->wm.surf_bo[i]);
   dri_bo_unreference(brw->wm.sampler_bo);
   dri_bo_unreference(brw->wm.prog_bo);
   dri_bo_unreference(brw->wm.state_bo);
   dri_bo_unreference(brw->cc.prog_bo);
   dri_bo_unreference(brw->cc.state_bo);
   dri_bo_unreference(brw->cc.vp_bo);
}
