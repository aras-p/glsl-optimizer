/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "main/mtypes.h"
#include "program/prog_parameter.h"

#include "brw_context.h"
#include "brw_state.h"


/* Creates a new GS constant buffer reflecting the current GS program's
 * constants, if needed by the GS program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_gs_pull_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->gs.base;

   /* BRW_NEW_GEOMETRY_PROGRAM */
   struct brw_geometry_program *gp =
      (struct brw_geometry_program *) brw->geometry_program;

   if (!gp)
      return;

   /* CACHE_NEW_GS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->gs.prog_data->base;

   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_vec4_pull_constants(brw, BRW_NEW_GS_CONSTBUF, &gp->program.Base,
                                  stage_state, prog_data);
}

const struct brw_tracked_state brw_gs_pull_constants = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_BATCH | BRW_NEW_GEOMETRY_PROGRAM),
      .cache = CACHE_NEW_GS_PROG,
   },
   .emit = brw_upload_gs_pull_constants,
};

static void
brw_upload_gs_ubo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_stage_state *stage_state = &brw->gs.base;

   /* _NEW_PROGRAM */
   struct gl_shader_program *prog = ctx->Shader.CurrentGeometryProgram;

   if (!prog)
      return;

   brw_upload_ubo_surfaces(brw, prog->_LinkedShaders[MESA_SHADER_GEOMETRY],
			   &stage_state->surf_offset[SURF_INDEX_VEC4_UBO(0)]);
}

const struct brw_tracked_state brw_gs_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH | BRW_NEW_UNIFORM_BUFFER,
      .cache = 0,
   },
   .emit = brw_upload_gs_ubo_surfaces,
};


/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_gs_upload_binding_table(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->gs.base;

   /* If there's no GS, skip changing anything. */
   if (!brw->gs.prog_data)
      return;

   /* CACHE_NEW_GS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->gs.prog_data->base;

   /* BRW_NEW_SURFACES and BRW_NEW_GS_CONSTBUF */
   brw_upload_binding_table(brw, BRW_NEW_GS_BINDING_TABLE, stage_state,
                            prog_data->binding_table_size,
                            SURF_INDEX_VEC4_SHADER_TIME);
}

const struct brw_tracked_state brw_gs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_GS_CONSTBUF |
	      BRW_NEW_SURFACES),
      .cache = CACHE_NEW_GS_PROG
   },
   .emit = brw_gs_upload_binding_table,
};
