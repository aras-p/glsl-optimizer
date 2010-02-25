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
#include "main/simple_list.h"
#include "shader/shader_api.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_state.h"
#include "intel_span.h"
#include "tnl/t_pipeline.h"


/***************************************
 * Mesa's Driver Functions
 ***************************************/

static void brwUseProgram(GLcontext *ctx, GLuint program)
{
   _mesa_use_program(ctx, program);
}

static void brwInitProgFuncs( struct dd_function_table *functions )
{
   functions->UseProgram = brwUseProgram;
}
static void brwInitDriverFunctions( struct dd_function_table *functions )
{
   intelInitDriverFunctions( functions );

   brwInitFragProgFuncs( functions );
   brwInitProgFuncs( functions );
   brw_init_queryobj_functions(functions);

   functions->Viewport = intel_viewport;
}

GLboolean brwCreateContext( const __GLcontextModes *mesaVis,
			    __DRIcontext *driContextPriv,
			    void *sharedContextPrivate)
{
   struct dd_function_table functions;
   struct brw_context *brw = (struct brw_context *) CALLOC_STRUCT(brw_context);
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;

   if (!brw) {
      printf("%s: failed to alloc context\n", __FUNCTION__);
      return GL_FALSE;
   }

   brwInitVtbl( brw );
   brwInitDriverFunctions( &functions );

   if (!intelInitContext( intel, mesaVis, driContextPriv,
			  sharedContextPrivate, &functions )) {
      printf("%s: failed to init intel context\n", __FUNCTION__);
      FREE(brw);
      return GL_FALSE;
   }

   /* Initialize swrast, tnl driver tables: */
   intelInitSpanFuncs(ctx);

   TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

   ctx->Const.MaxDrawBuffers = BRW_MAX_DRAW_BUFFERS;
   ctx->Const.MaxTextureImageUnits = BRW_MAX_TEX_UNIT;
   ctx->Const.MaxTextureCoordUnits = 8; /* Mesa limit */
   ctx->Const.MaxTextureUnits = MIN2(ctx->Const.MaxTextureCoordUnits,
                                     ctx->Const.MaxTextureImageUnits);
   ctx->Const.MaxVertexTextureImageUnits = 0; /* no vertex shader textures */
   ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.MaxVertexTextureImageUnits +
      ctx->Const.MaxTextureImageUnits;

   /* Mesa limits textures to 4kx4k; it would be nice to fix that someday
    */
   ctx->Const.MaxTextureLevels = 13;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;
   ctx->Const.MaxTextureRectSize = (1<<12);
   
   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* if conformance mode is set, swrast can handle any size AA point */
   ctx->Const.MaxPointSizeAA = 255.0;

   /* We want the GLSL compiler to emit code that uses condition codes */
   ctx->Shader.EmitCondCodes = GL_TRUE;
   ctx->Shader.EmitNVTempInitialization = GL_TRUE;

   ctx->Const.VertexProgram.MaxNativeInstructions = (16 * 1024);
   ctx->Const.VertexProgram.MaxAluInstructions = 0;
   ctx->Const.VertexProgram.MaxTexInstructions = 0;
   ctx->Const.VertexProgram.MaxTexIndirections = 0;
   ctx->Const.VertexProgram.MaxNativeAluInstructions = 0;
   ctx->Const.VertexProgram.MaxNativeTexInstructions = 0;
   ctx->Const.VertexProgram.MaxNativeTexIndirections = 0;
   ctx->Const.VertexProgram.MaxNativeAttribs = 16;
   ctx->Const.VertexProgram.MaxNativeTemps = 256;
   ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;
   ctx->Const.VertexProgram.MaxNativeParameters = 1024;
   ctx->Const.VertexProgram.MaxEnvParams =
      MIN2(ctx->Const.VertexProgram.MaxNativeParameters,
	   ctx->Const.VertexProgram.MaxEnvParams);

   ctx->Const.FragmentProgram.MaxNativeInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeAluInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeTexInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeTexIndirections = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeAttribs = 12;
   ctx->Const.FragmentProgram.MaxNativeTemps = 256;
   ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;
   ctx->Const.FragmentProgram.MaxNativeParameters = 1024;
   ctx->Const.FragmentProgram.MaxEnvParams =
      MIN2(ctx->Const.FragmentProgram.MaxNativeParameters,
	   ctx->Const.FragmentProgram.MaxEnvParams);

   if (intel->is_ironlake || intel->is_g4x || intel->gen >= 6) {
      brw->CMD_VF_STATISTICS = CMD_VF_STATISTICS_GM45;
      brw->CMD_PIPELINE_SELECT = CMD_PIPELINE_SELECT_GM45;
      brw->has_surface_tile_offset = GL_TRUE;
      brw->has_compr4 = GL_TRUE;
      brw->has_aa_line_parameters = GL_TRUE;
  } else {
      brw->CMD_VF_STATISTICS = CMD_VF_STATISTICS_965;
      brw->CMD_PIPELINE_SELECT = CMD_PIPELINE_SELECT_965;
   }

   /* WM maximum threads is number of EUs times number of threads per EU. */
   if (intel->is_ironlake) {
      brw->urb.size = 1024;
      brw->vs_max_threads = 72;
      brw->wm_max_threads = 12 * 6;
   } else if (intel->is_g4x) {
      brw->urb.size = 384;
      brw->vs_max_threads = 32;
      brw->wm_max_threads = 10 * 5;
   } else if (intel->gen < 6) {
      brw->urb.size = 256;
      brw->vs_max_threads = 16;
      brw->wm_max_threads = 8 * 4;
      brw->has_negative_rhw_bug = GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD) {
      brw->vs_max_threads = 1;
      brw->wm_max_threads = 1;
   }

   brw_init_state( brw );

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   brw->emit_state_always = 0;

   ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;
   ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

   make_empty_list(&brw->query.active_head);

   brw_draw_init( brw );

   return GL_TRUE;
}

