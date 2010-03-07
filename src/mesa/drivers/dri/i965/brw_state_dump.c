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

#include "main/mtypes.h"

#include "brw_context.h"
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
state_out(const char *name, void *data, uint32_t hw_offset, int index,
	  char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%8s: 0x%08x: 0x%08x: ",
	    name, hw_offset + index * 4, ((uint32_t *)data)[index]);
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

/** Generic, undecoded state buffer debug printout */
static void
state_struct_out(const char *name, dri_bo *buffer, unsigned int state_size)
{
   int i;

   if (buffer == NULL)
      return;

   dri_bo_map(buffer, GL_FALSE);
   for (i = 0; i < state_size / 4; i++) {
      state_out(name, buffer->virtual, buffer->offset, i,
		"dword %d\n", i);
   }
   dri_bo_unmap(buffer);
}

static const char *
get_965_surfacetype(unsigned int surfacetype)
{
    switch (surfacetype) {
    case 0: return "1D";
    case 1: return "2D";
    case 2: return "3D";
    case 3: return "CUBE";
    case 4: return "BUFFER";
    case 7: return "NULL";
    default: return "unknown";
    }
}

static const char *
get_965_surface_format(unsigned int surface_format)
{
    switch (surface_format) {
    case 0x000: return "r32g32b32a32_float";
    case 0x0c1: return "b8g8r8a8_unorm";
    case 0x100: return "b5g6r5_unorm";
    case 0x102: return "b5g5r5a1_unorm";
    case 0x104: return "b4g4r4a4_unorm";
    default: return "unknown";
    }
}

static void dump_wm_surface_state(struct brw_context *brw)
{
   int i;

   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      dri_bo *surf_bo = brw->wm.surf_bo[i];
      unsigned int surfoff;
      struct brw_surface_state *surf;
      char name[20];

      if (surf_bo == NULL) {
	 fprintf(stderr, "  WM SS%d: NULL\n", i);
	 continue;
      }
      dri_bo_map(surf_bo, GL_FALSE);
      surfoff = surf_bo->offset;
      surf = (struct brw_surface_state *)(surf_bo->virtual);

      sprintf(name, "WM SS%d", i);
      state_out(name, surf, surfoff, 0, "%s %s\n",
		get_965_surfacetype(surf->ss0.surface_type),
		get_965_surface_format(surf->ss0.surface_format));
      state_out(name, surf, surfoff, 1, "offset\n");
      state_out(name, surf, surfoff, 2, "%dx%d size, %d mips\n",
		surf->ss2.width + 1, surf->ss2.height + 1, surf->ss2.mip_count);
      state_out(name, surf, surfoff, 3, "pitch %d, %stiled\n",
		surf->ss3.pitch + 1, surf->ss3.tiled_surface ? "" : "not ");
      state_out(name, surf, surfoff, 4, "mip base %d\n",
		surf->ss4.min_lod);
      state_out(name, surf, surfoff, 5, "x,y offset: %d,%d\n",
		surf->ss5.x_offset, surf->ss5.y_offset);

      dri_bo_unmap(surf_bo);
   }
}

static void dump_sf_viewport_state(struct brw_context *brw)
{
   const char *name = "SF VP";
   struct brw_sf_viewport *vp;
   uint32_t vp_off;

   if (brw->sf.vp_bo == NULL)
      return;

   dri_bo_map(brw->sf.vp_bo, GL_FALSE);

   vp = brw->sf.vp_bo->virtual;
   vp_off = brw->sf.vp_bo->offset;

   state_out(name, vp, vp_off, 0, "m00 = %f\n", vp->viewport.m00);
   state_out(name, vp, vp_off, 1, "m11 = %f\n", vp->viewport.m11);
   state_out(name, vp, vp_off, 2, "m22 = %f\n", vp->viewport.m22);
   state_out(name, vp, vp_off, 3, "m30 = %f\n", vp->viewport.m30);
   state_out(name, vp, vp_off, 4, "m31 = %f\n", vp->viewport.m31);
   state_out(name, vp, vp_off, 5, "m32 = %f\n", vp->viewport.m32);

   state_out(name, vp, vp_off, 6, "top left = %d,%d\n",
	     vp->scissor.xmin, vp->scissor.ymin);
   state_out(name, vp, vp_off, 7, "bottom right = %d,%d\n",
	     vp->scissor.xmax, vp->scissor.ymax);

   dri_bo_unmap(brw->sf.vp_bo);
}

static void brw_debug_prog(const char *name, dri_bo *prog)
{
   unsigned int i;
   uint32_t *data;

   if (prog == NULL)
      return;

   dri_bo_map(prog, GL_FALSE);

   data = prog->virtual;

   for (i = 0; i < prog->size / 4 / 4; i++) {
      fprintf(stderr, "%8s: 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	      name, (unsigned int)prog->offset + i * 4 * 4,
	      data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
      /* Stop at the end of the program.  It'd be nice to keep track of the actual
       * intended program size instead of guessing like this.
       */
      if (data[i * 4 + 0] == 0 &&
	  data[i * 4 + 1] == 0 &&
	  data[i * 4 + 2] == 0 &&
	  data[i * 4 + 3] == 0)
	 break;
   }

   dri_bo_unmap(prog);
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

   state_struct_out("WM bind", brw->wm.bind_bo, 4 * brw->wm.nr_surfaces);
   dump_wm_surface_state(brw);

   state_struct_out("VS", brw->vs.state_bo, sizeof(struct brw_vs_unit_state));
   brw_debug_prog("VS prog", brw->vs.prog_bo);

   state_struct_out("GS", brw->gs.state_bo, sizeof(struct brw_gs_unit_state));
   brw_debug_prog("GS prog", brw->gs.prog_bo);

   state_struct_out("SF", brw->sf.state_bo, sizeof(struct brw_sf_unit_state));
   dump_sf_viewport_state(brw);
   brw_debug_prog("SF prog", brw->sf.prog_bo);

   state_struct_out("WM", brw->wm.state_bo, sizeof(struct brw_wm_unit_state));
   brw_debug_prog("WM prog", brw->wm.prog_bo);
}
