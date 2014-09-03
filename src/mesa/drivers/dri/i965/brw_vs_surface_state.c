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

/**
 * Creates a temporary BO containing the pull constant data for the shader
 * stage, and the SURFACE_STATE struct that points at it.
 *
 * Pull constants are GLSL uniforms (and other constant data) beyond what we
 * could fit as push constants, or that have variable-index array access
 * (which is easiest to support using pull constants, and avoids filling
 * register space with mostly-unused data).
 *
 * Compare this path to brw_curbe.c for gen4/5 push constants, and
 * gen6_vs_state.c for gen6+ push constants.
 */
void
brw_upload_pull_constants(struct brw_context *brw,
                          GLbitfield brw_new_constbuf,
                          const struct gl_program *prog,
                          struct brw_stage_state *stage_state,
                          const struct brw_stage_prog_data *prog_data,
                          bool dword_pitch)
{
   int i;
   uint32_t surf_index = prog_data->binding_table.pull_constants_start;

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   _mesa_load_state_parameters(&brw->ctx, prog->Parameters);

   if (!prog_data->nr_pull_params) {
      if (stage_state->surf_offset[surf_index]) {
	 stage_state->surf_offset[surf_index] = 0;
	 brw->state.dirty.brw |= brw_new_constbuf;
      }
      return;
   }

   /* CACHE_NEW_*_PROG | _NEW_PROGRAM_CONSTANTS */
   uint32_t size = prog_data->nr_pull_params * 4;
   drm_intel_bo *const_bo = NULL;
   uint32_t const_offset;
   gl_constant_value *constants = intel_upload_space(brw, size, 64,
                                                     &const_bo, &const_offset);

   STATIC_ASSERT(sizeof(gl_constant_value) == sizeof(float));

   for (i = 0; i < prog_data->nr_pull_params; i++) {
      constants[i] = *prog_data->pull_param[i];
   }

   if (0) {
      for (i = 0; i < ALIGN(prog_data->nr_pull_params, 4) / 4; i++) {
	 const gl_constant_value *row = &constants[i * 4];
	 fprintf(stderr, "const surface %3d: %4.3f %4.3f %4.3f %4.3f\n",
                 i, row[0].f, row[1].f, row[2].f, row[3].f);
      }
   }

   brw_create_constant_surface(brw, const_bo, const_offset, size,
                               &stage_state->surf_offset[surf_index],
                               dword_pitch);
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
   const struct brw_stage_prog_data *prog_data = &brw->vs.prog_data->base.base;

   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_pull_constants(brw, BRW_NEW_VS_CONSTBUF, &vp->program.Base,
                             stage_state, prog_data, false);
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
