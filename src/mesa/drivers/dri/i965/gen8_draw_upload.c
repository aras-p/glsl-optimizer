/*
 * Copyright © 2012 Intel Corporation
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

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

static void
gen8_emit_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   brw_prepare_vertices(brw);

   if (brw->vs.prog_data->uses_vertexid) {
      unsigned vue = brw->vb.nr_enabled;

      WARN_ONCE(brw->vs.prog_data->inputs_read & VERT_BIT_EDGEFLAG,
                "Using VID/IID with edgeflags, need to reorder the "
                "vertex attributes");
      WARN_ONCE(vue >= 33,
                "Trying to insert VID/IID past 33rd vertex element, "
                "need to reorder the vertex attrbutes.");

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_VF_SGVS << 16 | (2 - 2));
      OUT_BATCH(GEN8_SGVS_ENABLE_VERTEX_ID |
                (0 << GEN8_SGVS_VERTEX_ID_COMPONENT_SHIFT) |   /* .x channel */
                (vue << GEN8_SGVS_VERTEX_ID_ELEMENT_OFFSET_SHIFT) |
                GEN8_SGVS_ENABLE_INSTANCE_ID |
                (1 << GEN8_SGVS_INSTANCE_ID_COMPONENT_SHIFT) | /* .y channel */
                (vue << GEN8_SGVS_INSTANCE_ID_ELEMENT_OFFSET_SHIFT));
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_VF_SGVS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (brw->vb.nr_enabled == 0) {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (3 - 2));
      OUT_BATCH((0 << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
                (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_1_FLT << BRW_VE1_COMPONENT_3_SHIFT));
      ADVANCE_BATCH();
      return;
   }

   /* Now emit 3DSTATE_VERTEX_BUFFERS and 3DSTATE_VERTEX_ELEMENTS packets. */
   if (brw->vb.nr_buffers) {
      assert(brw->vb.nr_buffers <= 33);

      BEGIN_BATCH(1 + 4*brw->vb.nr_buffers);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (4*brw->vb.nr_buffers - 1));
      for (unsigned i = 0; i < brw->vb.nr_buffers; i++) {
         struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
         uint32_t dw0 = 0;

         dw0 |= i << GEN6_VB0_INDEX_SHIFT;
         dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;
         dw0 |= buffer->stride << BRW_VB0_PITCH_SHIFT;
         dw0 |= BDW_MOCS_WB << 16;

         OUT_BATCH(dw0);
         OUT_RELOC64(buffer->bo, I915_GEM_DOMAIN_VERTEX, 0, buffer->offset);
         OUT_BATCH(buffer->bo->size);
      }
      ADVANCE_BATCH();
   }

   unsigned nr_elements = brw->vb.nr_enabled;

   /* The hardware allows one more VERTEX_ELEMENTS than VERTEX_BUFFERS,
    * presumably for VertexID/InstanceID.
    */
   assert(nr_elements <= 34);

   struct brw_vertex_element *gen6_edgeflag_input = NULL;

   BEGIN_BATCH(1 + nr_elements * 2);
   OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (2 * nr_elements - 1));
   for (unsigned i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glarray);
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      /* The gen4 driver expects edgeflag to come in as a float, and passes
       * that float on to the tests in the clipper.  Mesa's current vertex
       * attribute value for EdgeFlag is stored as a float, which works out.
       * glEdgeFlagPointer, on the other hand, gives us an unnormalized
       * integer ubyte.  Just rewrite that to convert to a float.
       */
      if (input == &brw->vb.inputs[VERT_ATTRIB_EDGEFLAG]) {
         /* Gen6+ passes edgeflag as sideband along with the vertex, instead
          * of in the VUE.  We have to upload it sideband as the last vertex
          * element according to the B-Spec.
          */
         gen6_edgeflag_input = input;
         continue;
      }

      switch (input->glarray->Size) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0;
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0;
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0;
      case 3: comp3 = input->glarray->Integer ? BRW_VE1_COMPONENT_STORE_1_INT
                                              : BRW_VE1_COMPONENT_STORE_1_FLT;
         break;
      }

      OUT_BATCH((input->buffer << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                (format << BRW_VE0_FORMAT_SHIFT) |
                (input->offset << BRW_VE0_SRC_OFFSET_SHIFT));

      OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
                (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
                (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
                (comp3 << BRW_VE1_COMPONENT_3_SHIFT));
   }

   if (gen6_edgeflag_input) {
      uint32_t format =
         brw_get_vertex_surface_type(brw, gen6_edgeflag_input->glarray);

      OUT_BATCH((gen6_edgeflag_input->buffer << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                GEN6_VE0_EDGE_FLAG_ENABLE |
                (format << BRW_VE0_FORMAT_SHIFT) |
                (gen6_edgeflag_input->offset << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
   }
   ADVANCE_BATCH();

   for (unsigned i = 0; i < brw->vb.nr_enabled; i++) {
      const struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct brw_vertex_buffer *buffer = &brw->vb.buffers[input->buffer];

      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_VF_INSTANCING << 16 | (3 - 2));
      OUT_BATCH(i | (buffer->step_rate ? GEN8_VF_INSTANCING_ENABLE : 0));
      OUT_BATCH(buffer->step_rate);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state gen8_vertices = {
   .dirty = {
      .mesa = _NEW_POLYGON,
      .brw = BRW_NEW_BATCH | BRW_NEW_VERTICES,
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = gen8_emit_vertices,
};

static void
gen8_emit_index_buffer(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;

   if (index_buffer == NULL)
      return;

   BEGIN_BATCH(5);
   OUT_BATCH(CMD_INDEX_BUFFER << 16 | (5 - 2));
   OUT_BATCH(brw_get_index_type(index_buffer->type) << 8 | BDW_MOCS_WB);
   OUT_RELOC64(brw->ib.bo, I915_GEM_DOMAIN_VERTEX, 0, 0);
   OUT_BATCH(brw->ib.bo->size);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_INDEX_BUFFER,
      .cache = 0,
   },
   .emit = gen8_emit_index_buffer,
};

static void
gen8_emit_vf_topology(struct brw_context *brw)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VF_TOPOLOGY << 16 | (2 - 2));
   OUT_BATCH(brw->primitive);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_vf_topology = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_PRIMITIVE,
      .cache = 0,
   },
   .emit = gen8_emit_vf_topology,
};
