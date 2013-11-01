/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
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
  *   Keith Whitwell <keithw@vmware.com>
  */

#include "main/mtypes.h"
#include "program/prog_parameter.h"

#include "brw_context.h"
#include "brw_state.h"
#include "intel_buffer_objects.h"

void
brw_upload_vec4_pull_constants(struct brw_context *brw,
                               GLbitfield brw_new_constbuf,
                               const struct gl_program *prog,
                               struct brw_stage_state *stage_state,
                               const struct brw_vec4_prog_data *prog_data)
{
   int i;
   uint32_t surf_index = prog_data->base.binding_table.pull_constants_start;

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   _mesa_load_state_parameters(&brw->ctx, prog->Parameters);

   if (!prog_data->base.nr_pull_params) {
      if (stage_state->surf_offset[surf_index]) {
	 stage_state->surf_offset[surf_index] = 0;
	 brw->state.dirty.brw |= brw_new_constbuf;
      }
      return;
   }

   /* _NEW_PROGRAM_CONSTANTS */
   uint32_t size = prog_data->base.nr_pull_params * 4;
   drm_intel_bo *const_bo = NULL;
   uint32_t const_offset;
   float *constants = intel_upload_space(brw, size, 64,
                                         &const_bo, &const_offset);

   for (i = 0; i < prog_data->base.nr_pull_params; i++) {
      constants[i] = *prog_data->base.pull_param[i];
   }

   if (0) {
      for (i = 0; i < ALIGN(prog_data->base.nr_pull_params, 4) / 4; i++) {
	 float *row = &constants[i * 4];
	 fprintf(stderr, "const surface %3d: %4.3f %4.3f %4.3f %4.3f\n",
                 i, row[0], row[1], row[2], row[3]);
      }
   }

   brw_create_constant_surface(brw, const_bo, const_offset, size,
                               &stage_state->surf_offset[surf_index],
                               false);
   drm_intel_bo_unreference(const_bo);

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
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_shader_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];

   if (!prog)
      return;

   /* CACHE_NEW_VS_PROG */
   brw_upload_ubo_surfaces(brw, prog->_LinkedShaders[MESA_SHADER_VERTEX],
			   &brw->vs.base, &brw->vs.prog_data->base.base);
}

const struct brw_tracked_state brw_vs_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH | BRW_NEW_UNIFORM_BUFFER,
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_upload_vs_ubo_surfaces,
};

static void
brw_upload_vs_abo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_shader_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];

   if (prog) {
      /* CACHE_NEW_VS_PROG */
      brw_upload_abo_surfaces(brw, prog, &brw->vs.base,
                              &brw->vs.prog_data->base.base);
   }
}

const struct brw_tracked_state brw_vs_abo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH | BRW_NEW_ATOMIC_BUFFER,
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_upload_vs_abo_surfaces,
};
