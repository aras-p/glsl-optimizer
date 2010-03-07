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
#include "brw_batchbuffer.h"
#include "brw_debug.h"

const struct brw_tracked_state *atoms[] =
{
/*   &brw_wm_input_sizes, */
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

   &brw_vs_surfaces,		/* must do before unit */
   /*&brw_wm_constant_surface,*/	/* must do before wm surfaces/bind bo */
   &brw_wm_surfaces,		/* must do before samplers and unit */
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

   &brw_binding_table_pointers,
   &brw_blend_constant_color,

   &brw_depthbuffer,
   &brw_polygon_stipple,
   &brw_line_stipple,

   &brw_psp_urb_cbs,

   &brw_drawing_rect,
   &brw_indices,
   &brw_index_buffer,
   &brw_vertices,

   &brw_curbe_buffer
};


void brw_init_state( struct brw_context *brw )
{
   brw_init_caches(brw);
}


void brw_destroy_state( struct brw_context *brw )
{
   brw_destroy_caches(brw);
   brw_destroy_batch_cache(brw);
}

/***********************************************************************
 */

static GLboolean check_state( const struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   return ((a->mesa & b->mesa) ||
	   (a->brw & b->brw) ||
	   (a->cache & b->cache));
}

static void accumulate_state( struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   a->mesa |= b->mesa;
   a->brw |= b->brw;
   a->cache |= b->cache;
}


static void xor_states( struct brw_state_flags *result,
			     const struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   result->mesa = a->mesa ^ b->mesa;
   result->brw = a->brw ^ b->brw;
   result->cache = a->cache ^ b->cache;
}

static void
brw_clear_validated_bos(struct brw_context *brw)
{
   int i;

   /* Clear the last round of validated bos */
   for (i = 0; i < brw->state.validated_bo_count; i++) {
      bo_reference(&brw->state.validated_bos[i], NULL);
   }
   brw->state.validated_bo_count = 0;
}


/***********************************************************************
 * Emit all state:
 */
enum pipe_error brw_validate_state( struct brw_context *brw )
{
   struct brw_state_flags *state = &brw->state.dirty;
   GLuint i;
   int ret;

   brw_clear_validated_bos(brw);
   brw_add_validated_bo(brw, brw->batch->buf);

   if (brw->flags.always_emit_state) {
      state->mesa |= ~0;
      state->brw |= ~0;
      state->cache |= ~0;
   }

   if (state->mesa == 0 &&
       state->cache == 0 &&
       state->brw == 0)
      return 0;

   if (brw->state.dirty.brw & BRW_NEW_CONTEXT)
      brw_clear_batch_cache(brw);

   /* do prepare stage for all atoms */
   for (i = 0; i < Elements(atoms); i++) {
      const struct brw_tracked_state *atom = atoms[i];

      if (check_state(state, &atom->dirty)) {
         if (atom->prepare) {
            ret = atom->prepare(brw);
	    if (ret)
	       return ret;
        }
      }
   }

   /* Make sure that the textures which are referenced by the current
    * brw fragment program are actually present/valid.
    * If this fails, we can experience GPU lock-ups.
    */
   {
      const struct brw_fragment_shader *fp = brw->curr.fragment_shader;
      if (fp) {
         assert(fp->info.file_max[TGSI_FILE_SAMPLER] < (int)brw->curr.num_samplers);
	 /*assert(fp->info.texture_max <= brw->curr.num_textures);*/
      }
   }

   return 0;
}


enum pipe_error brw_upload_state(struct brw_context *brw)
{
   struct brw_state_flags *state = &brw->state.dirty;
   int ret;
   int i;

   brw_clear_validated_bos(brw);

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

	 assert(atom->dirty.mesa ||
		atom->dirty.brw ||
		atom->dirty.cache);

	 if (check_state(state, &atom->dirty)) {
	    if (atom->emit) {
	       ret = atom->emit( brw );
	       if (ret)
		  return ret;
	    }
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

	 if (check_state(state, &atom->dirty)) {
	    if (atom->emit) {
	       ret = atom->emit( brw );
	       if (ret)
		  return ret;
	    }
	 }
      }
   }

   if (BRW_DEBUG & DEBUG_STATE) {
      brw_update_dirty_counts( state->mesa, 
			       state->brw,
			       state->cache );
   }
   
   /* Clear dirty flags:
    */
   memset(state, 0, sizeof(*state));
   return 0;
}
