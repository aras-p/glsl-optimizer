/*
 * Copyright © 2007 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "mtypes.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

/**
 * Prints out a header, the contents, and the message associated with
 * the hardware state data given.
 *
 * \param name Name of the state object
 * \param data Pointer to the base of the state object
 * \param hw_offset Hardware offset of the base of the state data.
 * \param index Index of the DWORD being output.
 */
static void
state_out(char *name, uint32_t *data, uint32_t hw_offset, int index,
	  char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%8s: 0x%08x: 0x%08x: ",
	    name, hw_offset + index * 4, data[index]);
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

/** Generic, undecoded state buffer debug printout */
static void
state_struct_out(char *name, dri_bo *buffer, unsigned int pool_offset,
		 unsigned int state_size)
{
   int i;
   uint32_t *state;

   state = buffer->virtual + pool_offset;
   for (i = 0; i < state_size / 4; i++) {
      state_out(name, state, buffer->offset + pool_offset, i,
		"dword %d\n", i);
   }
}

static void dump_wm_surface_state(struct brw_context *brw, dri_bo *ss_buffer)
{
   int i;

   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      unsigned int surfoff = ss_buffer->offset + brw->wm.bind.surf_ss_offset[i];
      struct brw_surface_state *surf =
	 (struct brw_surface_state *)(ss_buffer->virtual +
				      brw->wm.bind.surf_ss_offset[i]);
      uint32_t *surfvals = (uint32_t *)surf;
      char name[20];

      sprintf(name, "WM SS%d", i);
      state_out(name, surfvals, surfoff, 0, "\n");
      state_out(name, surfvals, surfoff, 1, "offset\n");
      state_out(name, surfvals, surfoff, 2, "%dx%d size, %d mips\n",
		surf->ss2.width + 1, surf->ss2.height + 1, surf->ss2.mip_count);
      state_out(name, surfvals, surfoff, 3, "pitch %d, %stiled\n",
		surf->ss3.pitch + 1, surf->ss3.tiled_surface ? "" : "not ");
      state_out(name, surfvals, surfoff, 4, "mip base %d\n",
		surf->ss4.min_lod);
   }
}

/**
 * Print additional debug information associated with the batchbuffer
 * when DEBUG_BATCH is set.
 *
 * For 965, this means mapping the state buffers that would have been referenced
 * by the batchbuffer and dumping them.
 *
 * The buffer offsets printed rely on the buffer containing the last offset
 * it was validated at.
 */
void brw_debug_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   dri_bo *ss_buffer, *gs_buffer;

   ss_buffer = brw->pool[BRW_SS_POOL].buffer;
   gs_buffer = brw->pool[BRW_GS_POOL].buffer;

   dri_bo_map(ss_buffer, GL_FALSE);
   dri_bo_map(gs_buffer, GL_FALSE);

   state_struct_out("WM bind", ss_buffer, brw->wm.bind_ss_offset,
		    4 * brw->wm.nr_surfaces);
   dump_wm_surface_state(brw, ss_buffer);

   state_struct_out("VS", gs_buffer, brw->vs.state_gs_offset,
		    sizeof(struct brw_vs_unit_state));
   state_struct_out("SF", gs_buffer, brw->sf.state_gs_offset,
		    sizeof(struct brw_sf_unit_state));
   state_struct_out("SF viewport", gs_buffer, brw->sf.state_gs_offset,
		    sizeof(struct brw_sf_unit_state));
   state_struct_out("WM", gs_buffer, brw->wm.state_gs_offset,
		    sizeof(struct brw_wm_unit_state));

   dri_bo_unmap(gs_buffer);
   dri_bo_unmap(ss_buffer);
}
