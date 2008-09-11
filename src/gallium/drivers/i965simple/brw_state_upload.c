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


#include "brw_context.h"
#include "brw_state.h"

#include "util/u_memory.h"

/* This is used to initialize brw->state.atoms[].  We could use this
 * list directly except for a single atom, brw_constant_buffer, which
 * has a .dirty value which changes according to the parameters of the
 * current fragment and vertex programs, and so cannot be a static
 * value.
 */
const struct brw_tracked_state *atoms[] =
{
   &brw_vs_prog,
   &brw_gs_prog,
   &brw_clip_prog,
   &brw_sf_prog,
   &brw_wm_prog,

   /* Once all the programs are done, we know how large urb entry
    * sizes need to be and can decide if we need to change the urb
    * layout.
    */
   &brw_curbe_offsets,
   &brw_recalculate_urb_fence,


   &brw_cc_vp,
   &brw_cc_unit,

   &brw_wm_surfaces,		/* must do before samplers */
   &brw_wm_samplers,

   &brw_wm_unit,
   &brw_sf_vp,
   &brw_sf_unit,
   &brw_vs_unit,		/* always required, enabled or not */
   &brw_clip_unit,
   &brw_gs_unit,

   /* Command packets:
    */
   &brw_invarient_state,
   &brw_state_base_address,
   &brw_pipe_control,

   &brw_binding_table_pointers,
   &brw_blend_constant_color,

   &brw_drawing_rect,
   &brw_depthbuffer,

   &brw_polygon_stipple,
   &brw_line_stipple,

   &brw_psp_urb_cbs,

   &brw_constant_buffer
};


void brw_init_state( struct brw_context *brw )
{
   brw_init_pools(brw);
   brw_init_caches(brw);

   brw->state.dirty.brw = ~0;
   brw->emit_state_always = 0;
}


void brw_destroy_state( struct brw_context *brw )
{
   brw_destroy_caches(brw);
   brw_destroy_batch_cache(brw);
   brw_destroy_pools(brw);
}

/***********************************************************************
 */

static boolean check_state( const struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   return ((a->brw & b->brw) ||
	   (a->cache & b->cache));
}

static void accumulate_state( struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   a->brw |= b->brw;
   a->cache |= b->cache;
}


static void xor_states( struct brw_state_flags *result,
			     const struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   result->brw = a->brw ^ b->brw;
   result->cache = a->cache ^ b->cache;
}


/***********************************************************************
 * Emit all state:
 */
void brw_validate_state( struct brw_context *brw )
{
   struct brw_state_flags *state = &brw->state.dirty;
   unsigned i;

   if (brw->emit_state_always) 
      state->brw |= ~0;

   if (state->cache == 0 &&
       state->brw == 0)
      return;

   if (brw->state.dirty.brw & BRW_NEW_SCENE)
      brw_clear_batch_cache_flush(brw);

   if (BRW_DEBUG) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct brw_state_flags examined, prev;
      memset(&examined, 0, sizeof(examined));
      prev = *state;

      for (i = 0; i < Elements(atoms); i++) {
	 const struct brw_tracked_state *atom = atoms[i];
	 struct brw_state_flags generated;

	 assert(atom->dirty.brw ||
		atom->dirty.cache);
	 assert(atom->update);

	 if (check_state(state, &atom->dirty)) {
	    atom->update( brw );
	 }

	 accumulate_state(&examined, &atom->dirty);

	 /* generated = (prev ^ state)
	  * if (examined & generated)
	  *     fail;
	  */
	 xor_states(&generated, &prev, state);
	 assert(!check_state(&examined, &generated));
	 prev = *state;
      }
   }
   else {
      for (i = 0; i < Elements(atoms); i++) {
	 const struct brw_tracked_state *atom = atoms[i];

	 assert(atom->dirty.brw ||
		atom->dirty.cache);
	 assert(atom->update);

	 if (check_state(state, &atom->dirty))
	    atom->update( brw );
      }
   }

   memset(state, 0, sizeof(*state));
}
