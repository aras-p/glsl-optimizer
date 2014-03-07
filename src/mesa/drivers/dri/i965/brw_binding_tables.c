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

/**
 * \file brw_binding_tables.c
 *
 * State atoms which upload the "binding table" for each shader stage.
 *
 * Binding tables map a numeric "surface index" to the SURFACE_STATE structure
 * for a currently bound surface.  This allows SEND messages (such as sampler
 * or data port messages) to refer to a particular surface by number, rather
 * than by pointer.
 *
 * The binding table is stored as a (sparse) array of SURFACE_STATE entries;
 * surface indexes are simply indexes into the array.  The ordering of the
 * entries is entirely left up to software; see the SURF_INDEX_* macros in
 * brw_context.h to see our current layout.
 */

#include "main/mtypes.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"

/**
 * Upload a shader stage's binding table as indirect state.
 *
 * This copies brw_stage_state::surf_offset[] into the indirect state section
 * of the batchbuffer (allocated by brw_state_batch()).
 */
static void
brw_upload_binding_table(struct brw_context *brw,
                         uint32_t packet_name,
                         GLbitfield brw_new_binding_table,
                         struct brw_stage_state *stage_state)
{
   /* CACHE_NEW_*_PROG */
   struct brw_stage_prog_data *prog_data = stage_state->prog_data;

   if (prog_data->binding_table.size_bytes == 0) {
      /* There are no surfaces; skip making the binding table altogether. */
      if (stage_state->bind_bo_offset == 0)
         return;

      stage_state->bind_bo_offset = 0;
   } else {
      /* Upload a new binding table. */
      if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
         brw->vtbl.create_raw_surface(
            brw, brw->shader_time.bo, 0, brw->shader_time.bo->size,
            &stage_state->surf_offset[prog_data->binding_table.shader_time_start], true);
      }

      uint32_t *bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
                                       prog_data->binding_table.size_bytes, 32,
                                       &stage_state->bind_bo_offset);

      /* BRW_NEW_SURFACES and BRW_NEW_*_CONSTBUF */
      memcpy(bind, stage_state->surf_offset,
             prog_data->binding_table.size_bytes);
   }

   brw->state.dirty.brw |= brw_new_binding_table;

   if (brw->gen >= 7) {
      BEGIN_BATCH(2);
      OUT_BATCH(packet_name << 16 | (2 - 2));
      OUT_BATCH(stage_state->bind_bo_offset);
      ADVANCE_BATCH();
   }
}

/**
 * State atoms which upload the binding table for a particular shader stage.
 *  @{
 */

/** Upload the VS binding table. */
static void
brw_vs_upload_binding_table(struct brw_context *brw)
{
   brw_upload_binding_table(brw,
                            _3DSTATE_BINDING_TABLE_POINTERS_VS,
                            BRW_NEW_VS_BINDING_TABLE, &brw->vs.base);
}

const struct brw_tracked_state brw_vs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_VS_CONSTBUF |
             BRW_NEW_SURFACES,
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = brw_vs_upload_binding_table,
};


/** Upload the PS binding table. */
static void
brw_upload_wm_binding_table(struct brw_context *brw)
{
   brw_upload_binding_table(brw,
                            _3DSTATE_BINDING_TABLE_POINTERS_PS,
                            BRW_NEW_PS_BINDING_TABLE, &brw->wm.base);
}

const struct brw_tracked_state brw_wm_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_SURFACES,
      .cache = CACHE_NEW_WM_PROG
   },
   .emit = brw_upload_wm_binding_table,
};

/** Upload the GS binding table (if GS is active). */
static void
brw_gs_upload_binding_table(struct brw_context *brw)
{
   /* If there's no GS, skip changing anything. */
   if (brw->geometry_program == NULL)
      return;

   brw_upload_binding_table(brw,
                            _3DSTATE_BINDING_TABLE_POINTERS_GS,
                            BRW_NEW_GS_BINDING_TABLE, &brw->gs.base);
}

const struct brw_tracked_state brw_gs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_GS_CONSTBUF |
             BRW_NEW_SURFACES,
      .cache = CACHE_NEW_GS_PROG
   },
   .emit = brw_gs_upload_binding_table,
};

/** @} */

/**
 * State atoms which emit 3DSTATE packets to update the binding table pointers.
 *  @{
 */

/**
 * (Gen4-5) Upload the binding table pointers for all shader stages.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void
gen4_upload_binding_table_pointers(struct brw_context *brw)
{
   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 | (6 - 2));
   OUT_BATCH(brw->vs.base.bind_bo_offset);
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_BATCH(brw->wm.base.bind_bo_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_STATE_BASE_ADDRESS |
              BRW_NEW_VS_BINDING_TABLE |
              BRW_NEW_GS_BINDING_TABLE |
              BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = gen4_upload_binding_table_pointers,
};

/**
 * (Sandybridge Only) Upload the binding table pointers for all shader stages.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void
gen6_upload_binding_table_pointers(struct brw_context *brw)
{
   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 |
             GEN6_BINDING_TABLE_MODIFY_VS |
             GEN6_BINDING_TABLE_MODIFY_GS |
             GEN6_BINDING_TABLE_MODIFY_PS |
             (4 - 2));
   OUT_BATCH(brw->vs.base.bind_bo_offset); /* vs */
   OUT_BATCH(brw->ff_gs.bind_bo_offset); /* gs */
   OUT_BATCH(brw->wm.base.bind_bo_offset); /* wm/ps */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_STATE_BASE_ADDRESS |
              BRW_NEW_VS_BINDING_TABLE |
              BRW_NEW_GS_BINDING_TABLE |
              BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = gen6_upload_binding_table_pointers,
};

/* Gen7+ code lives in gen7_{vs,gs,wm}_state.c. */

/** @} */
