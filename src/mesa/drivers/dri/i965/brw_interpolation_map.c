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

#include "brw_state.h"

static char const *get_qual_name(int mode)
{
   switch (mode) {
      case INTERP_QUALIFIER_NONE:          return "none";
      case INTERP_QUALIFIER_FLAT:          return "flat";
      case INTERP_QUALIFIER_SMOOTH:        return "smooth";
      case INTERP_QUALIFIER_NOPERSPECTIVE: return "nopersp";
      default:                             return "???";
   }
}


/* Set up interpolation modes for every element in the VUE */
static void
brw_setup_vue_interpolation(struct brw_context *brw)
{
   const struct gl_fragment_program *fprog = brw->fragment_program;
   struct brw_vue_map *vue_map = &brw->vue_map_geom_out;

   memset(&brw->interpolation_mode, INTERP_QUALIFIER_NONE, sizeof(brw->interpolation_mode));

   brw->state.dirty.brw |= BRW_NEW_INTERPOLATION_MAP;

   if (!fprog)
      return;

   for (int i = 0; i < vue_map->num_slots; i++) {
      int varying = vue_map->slot_to_varying[i];
      if (varying == -1)
         continue;

      /* HPOS always wants noperspective. setting it up here allows
       * us to not need special handling in the SF program. */
      if (varying == VARYING_SLOT_POS) {
         brw->interpolation_mode.mode[i] = INTERP_QUALIFIER_NOPERSPECTIVE;
         continue;
      }

      int frag_attrib = varying;
      if (varying == VARYING_SLOT_BFC0 || varying == VARYING_SLOT_BFC1)
         frag_attrib = varying - VARYING_SLOT_BFC0 + VARYING_SLOT_COL0;

      if (!(fprog->Base.InputsRead & BITFIELD64_BIT(frag_attrib)))
         continue;

      enum glsl_interp_qualifier mode = fprog->InterpQualifier[frag_attrib];

      /* If the mode is not specified, the default varies: Color values
       * follow GL_SHADE_MODEL; everything else is smooth.
       */
      if (mode == INTERP_QUALIFIER_NONE) {
         if (frag_attrib == VARYING_SLOT_COL0 || frag_attrib == VARYING_SLOT_COL1)
            mode = brw->ctx.Light.ShadeModel == GL_FLAT
               ? INTERP_QUALIFIER_FLAT : INTERP_QUALIFIER_SMOOTH;
         else
            mode = INTERP_QUALIFIER_SMOOTH;
      }

      brw->interpolation_mode.mode[i] = mode;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_VUE)) {
      fprintf(stderr, "VUE map:\n");
      for (int i = 0; i < vue_map->num_slots; i++) {
         int varying = vue_map->slot_to_varying[i];
         if (varying == -1) {
            fprintf(stderr, "%d: --\n", i);
            continue;
         }

         fprintf(stderr, "%d: %d %s ofs %d\n",
                 i, varying,
                 get_qual_name(brw->interpolation_mode.mode[i]),
                 brw_vue_slot_to_offset(i));
      }
   }
}


const struct brw_tracked_state brw_interpolation_map = {
   .dirty = {
      .mesa  = _NEW_LIGHT,
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_VUE_MAP_GEOM_OUT)
   },
   .emit = brw_setup_vue_interpolation
};
