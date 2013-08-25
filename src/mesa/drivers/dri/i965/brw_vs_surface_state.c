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

#include "main/mtypes.h"
#include "program/prog_parameter.h"

#include "brw_context.h"
#include "brw_state.h"


void
brw_upload_vec4_pull_constants(struct brw_context *brw,
                               GLbitfield brw_new_constbuf,
                               const struct gl_program *prog,
                               struct brw_stage_state *stage_state,
                               const struct brw_vec4_prog_data *prog_data)
{
   int i;

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   _mesa_load_state_parameters(&brw->ctx, prog->Parameters);

   if (!prog_data->nr_pull_params) {
      if (stage_state->const_bo) {
	 drm_intel_bo_unreference(stage_state->const_bo);
	 stage_state->const_bo = NULL;
	 stage_state->surf_offset[SURF_INDEX_VEC4_CONST_BUFFER] = 0;
	 brw->state.dirty.brw |= brw_new_constbuf;
      }
      return;
   }

   /* _NEW_PROGRAM_CONSTANTS */
   drm_intel_bo_unreference(stage_state->const_bo);
   uint32_t size = prog_data->nr_pull_params * 4;
   stage_state->const_bo = drm_intel_bo_alloc(brw->bufmgr, "vec4_const_buffer",
                                           size, 64);

   drm_intel_gem_bo_map_gtt(stage_state->const_bo);

   for (i = 0; i < prog_data->nr_pull_params; i++) {
      memcpy(stage_state->const_bo->virtual + i * 4,
	     prog_data->pull_param[i],
	     4);
   }

   if (0) {
      for (i = 0; i < ALIGN(prog_data->nr_pull_params, 4) / 4; i++) {
	 float *row = (float *)stage_state->const_bo->virtual + i * 4;
	 printf("const surface %3d: %4.3f %4.3f %4.3f %4.3f\n",
		i, row[0], row[1], row[2], row[3]);
      }
   }

   drm_intel_gem_bo_unmap_gtt(stage_state->const_bo);

   const int surf = SURF_INDEX_VEC4_CONST_BUFFER;
   brw->vtbl.create_constant_surface(brw, stage_state->const_bo, 0, size,
                                     &stage_state->surf_offset[surf], false);

   brw->state.dirty.brw |= brw_new_constbuf;
}


/* Creates a new VS constant buffer reflecting the current VS program's
 * constants, if needed by the VS program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_vs_pull_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   /* BRW_NEW_VERTEX_PROGRAM */
   struct brw_vertex_program *vp =
      (struct brw_vertex_program *) brw->vertex_program;

   /* CACHE_NEW_VS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->vs.prog_data->base;

   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_vec4_pull_constants(brw, BRW_NEW_VS_CONSTBUF, &vp->program.Base,
                                  stage_state, prog_data);
}

const struct brw_tracked_state brw_vs_pull_constants = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_BATCH | BRW_NEW_VERTEX_PROGRAM),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_upload_vs_pull_constants,
};

static void
brw_upload_vs_ubo_surfaces(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_shader_program *prog = ctx->Shader.CurrentVertexProgram;

   if (!prog)
      return;

   brw_upload_ubo_surfaces(brw, prog->_LinkedShaders[MESA_SHADER_VERTEX],
			   &stage_state->surf_offset[SURF_INDEX_VEC4_UBO(0)]);
}

const struct brw_tracked_state brw_vs_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH | BRW_NEW_UNIFORM_BUFFER,
      .cache = 0,
   },
   .emit = brw_upload_vs_ubo_surfaces,
};


void
brw_vec4_upload_binding_table(struct brw_context *brw,
                              GLbitfield brw_new_binding_table,
                              struct brw_stage_state *stage_state,
                              const struct brw_vec4_prog_data *prog_data)
{
   uint32_t *bind;
   int i;

   if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
      gen7_create_shader_time_surface(brw, &stage_state->surf_offset[SURF_INDEX_VEC4_SHADER_TIME]);
   }

   /* Skip making a binding table if we don't use textures or pull
    * constants.
    */
   const unsigned entries = prog_data->binding_table_size;
   if (entries == 0) {
      if (stage_state->bind_bo_offset != 0) {
	 brw->state.dirty.brw |= brw_new_binding_table;
	 stage_state->bind_bo_offset = 0;
      }
      return;
   }

   /* Might want to calculate nr_surfaces first, to avoid taking up so much
    * space for the binding table.
    */
   bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
			  sizeof(uint32_t) * entries,
			  32, &stage_state->bind_bo_offset);

   /* BRW_NEW_SURFACES and BRW_NEW_*_CONSTBUF */
   for (i = 0; i < entries; i++) {
      bind[i] = stage_state->surf_offset[i];
   }

   brw->state.dirty.brw |= brw_new_binding_table;
}


/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_vs_upload_binding_table(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;
   /* CACHE_NEW_VS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->vs.prog_data->base;

   /* BRW_NEW_SURFACES and BRW_NEW_VS_CONSTBUF */
   brw_vec4_upload_binding_table(brw, BRW_NEW_VS_BINDING_TABLE, stage_state,
                                 prog_data);
}

const struct brw_tracked_state brw_vs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_VS_CONSTBUF |
	      BRW_NEW_SURFACES),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = brw_vs_upload_binding_table,
};
