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
#include "intel_batchbuffer.h"

/* This is used to initialize brw->state.atoms[].  We could use this
 * list directly except for a single atom, brw_constant_buffer, which
 * has a .dirty value which changes according to the parameters of the
 * current fragment and vertex programs, and so cannot be a static
 * value.
 */
const struct brw_tracked_state *atoms[] =
{
   &brw_check_fallback,

   &brw_wm_input_sizes,
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

   &brw_binding_table_pointers,
   &brw_blend_constant_color,

   &brw_depthbuffer,

   &brw_polygon_stipple,
   &brw_polygon_stipple_offset,

   &brw_line_stipple,
   &brw_aa_line_parameters,
   /* Ordering of the commands below is documented as fixed.  
    */
#if 0
   &brw_pipelined_state_pointers,
   &brw_urb_fence,
   &brw_constant_buffer_state,
#else
   &brw_psp_urb_cbs,
#endif

   &brw_drawing_rect,
   &brw_indices,
   &brw_vertices,

   NULL,			/* brw_constant_buffer */
};


void brw_init_state( struct brw_context *brw )
{
   GLuint i;

   brw_init_cache(brw);

   brw->state.atoms = _mesa_malloc(sizeof(atoms));
   brw->state.nr_atoms = sizeof(atoms)/sizeof(*atoms);
   _mesa_memcpy(brw->state.atoms, atoms, sizeof(atoms));

   /* Patch in a pointer to the dynamic state atom:
    */
   for (i = 0; i < brw->state.nr_atoms; i++)
      if (brw->state.atoms[i] == NULL)
	 brw->state.atoms[i] = &brw->curbe.tracked_state;

   _mesa_memcpy(&brw->curbe.tracked_state, 
		&brw_constant_buffer,
		sizeof(brw_constant_buffer));
}


void brw_destroy_state( struct brw_context *brw )
{
   if (brw->state.atoms) {
      _mesa_free(brw->state.atoms);
      brw->state.atoms = NULL;
   }

   brw_destroy_cache(brw);
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
      dri_bo_unreference(brw->state.validated_bos[i]);
      brw->state.validated_bos[i] = NULL;
   }
   brw->state.validated_bo_count = 0;
}

struct dirty_bit_map {
   uint32_t bit;
   char *name;
   uint32_t count;
};

#define DEFINE_BIT(name) {name, #name, 0}

static struct dirty_bit_map mesa_bits[] = {
   DEFINE_BIT(_NEW_MODELVIEW),
   DEFINE_BIT(_NEW_PROJECTION),
   DEFINE_BIT(_NEW_TEXTURE_MATRIX),
   DEFINE_BIT(_NEW_COLOR_MATRIX),
   DEFINE_BIT(_NEW_ACCUM),
   DEFINE_BIT(_NEW_COLOR),
   DEFINE_BIT(_NEW_DEPTH),
   DEFINE_BIT(_NEW_EVAL),
   DEFINE_BIT(_NEW_FOG),
   DEFINE_BIT(_NEW_HINT),
   DEFINE_BIT(_NEW_LIGHT),
   DEFINE_BIT(_NEW_LINE),
   DEFINE_BIT(_NEW_PIXEL),
   DEFINE_BIT(_NEW_POINT),
   DEFINE_BIT(_NEW_POLYGON),
   DEFINE_BIT(_NEW_POLYGONSTIPPLE),
   DEFINE_BIT(_NEW_SCISSOR),
   DEFINE_BIT(_NEW_STENCIL),
   DEFINE_BIT(_NEW_TEXTURE),
   DEFINE_BIT(_NEW_TRANSFORM),
   DEFINE_BIT(_NEW_VIEWPORT),
   DEFINE_BIT(_NEW_PACKUNPACK),
   DEFINE_BIT(_NEW_ARRAY),
   DEFINE_BIT(_NEW_RENDERMODE),
   DEFINE_BIT(_NEW_BUFFERS),
   DEFINE_BIT(_NEW_MULTISAMPLE),
   DEFINE_BIT(_NEW_TRACK_MATRIX),
   DEFINE_BIT(_NEW_PROGRAM),
   {0, 0, 0}
};

static struct dirty_bit_map brw_bits[] = {
   DEFINE_BIT(BRW_NEW_URB_FENCE),
   DEFINE_BIT(BRW_NEW_FRAGMENT_PROGRAM),
   DEFINE_BIT(BRW_NEW_VERTEX_PROGRAM),
   DEFINE_BIT(BRW_NEW_INPUT_DIMENSIONS),
   DEFINE_BIT(BRW_NEW_CURBE_OFFSETS),
   DEFINE_BIT(BRW_NEW_REDUCED_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_CONTEXT),
   DEFINE_BIT(BRW_NEW_WM_INPUT_DIMENSIONS),
   DEFINE_BIT(BRW_NEW_INPUT_VARYING),
   DEFINE_BIT(BRW_NEW_PSP),
   DEFINE_BIT(BRW_NEW_FENCE),
   DEFINE_BIT(BRW_NEW_INDICES),
   DEFINE_BIT(BRW_NEW_VERTICES),
   DEFINE_BIT(BRW_NEW_BATCH),
   DEFINE_BIT(BRW_NEW_DEPTH_BUFFER),
   {0, 0, 0}
};

static struct dirty_bit_map cache_bits[] = {
   DEFINE_BIT(CACHE_NEW_CC_VP),
   DEFINE_BIT(CACHE_NEW_CC_UNIT),
   DEFINE_BIT(CACHE_NEW_WM_PROG),
   DEFINE_BIT(CACHE_NEW_SAMPLER_DEFAULT_COLOR),
   DEFINE_BIT(CACHE_NEW_SAMPLER),
   DEFINE_BIT(CACHE_NEW_WM_UNIT),
   DEFINE_BIT(CACHE_NEW_SF_PROG),
   DEFINE_BIT(CACHE_NEW_SF_VP),
   DEFINE_BIT(CACHE_NEW_SF_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_PROG),
   DEFINE_BIT(CACHE_NEW_GS_UNIT),
   DEFINE_BIT(CACHE_NEW_GS_PROG),
   DEFINE_BIT(CACHE_NEW_CLIP_VP),
   DEFINE_BIT(CACHE_NEW_CLIP_UNIT),
   DEFINE_BIT(CACHE_NEW_CLIP_PROG),
   DEFINE_BIT(CACHE_NEW_SURFACE),
   DEFINE_BIT(CACHE_NEW_SURF_BIND),
   {0, 0, 0}
};


static void
brw_update_dirty_count(struct dirty_bit_map *bit_map, int32_t bits)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      if (bit_map[i].bit & bits)
	 bit_map[i].count++;
   }
}

static void
brw_print_dirty_count(struct dirty_bit_map *bit_map, int32_t bits)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      fprintf(stderr, "0x%08x: %12d (%s)\n",
	      bit_map[i].bit, bit_map[i].count, bit_map[i].name);
   }
}

/***********************************************************************
 * Emit all state:
 */
void brw_validate_state( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   struct brw_state_flags *state = &brw->state.dirty;
   GLuint i;

   brw_clear_validated_bos(brw);

   state->mesa |= brw->intel.NewGLState;
   brw->intel.NewGLState = 0;

   brw_add_validated_bo(brw, intel->batch->buf);

   if (brw->emit_state_always) {
      state->mesa |= ~0;
      state->brw |= ~0;
   }

   if (brw->fragment_program != brw->attribs.FragmentProgram->_Current) {
      brw->fragment_program = brw->attribs.FragmentProgram->_Current;
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
   }

   if (brw->vertex_program != brw->attribs.VertexProgram->_Current) {
      brw->vertex_program = brw->attribs.VertexProgram->_Current;
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
   }

   if (state->mesa == 0 &&
       state->cache == 0 &&
       state->brw == 0)
      return;

   if (brw->state.dirty.brw & BRW_NEW_CONTEXT)
      brw_clear_batch_cache_flush(brw);

   brw->intel.Fallback = 0;

   /* do prepare stage for all atoms */
   for (i = 0; i < Elements(atoms); i++) {
      const struct brw_tracked_state *atom = brw->state.atoms[i];

      if (brw->intel.Fallback)
         break;

      if (check_state(state, &atom->dirty)) {
         if (atom->prepare) {
            atom->prepare(brw);
        }
      }
   }
}


void brw_upload_state(struct brw_context *brw)
{
   struct brw_state_flags *state = &brw->state.dirty;
   int i;
   static int dirty_count = 0;

   brw_clear_validated_bos(brw);

   if (INTEL_DEBUG) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct brw_state_flags examined, prev;      
      _mesa_memset(&examined, 0, sizeof(examined));
      prev = *state;

      for (i = 0; i < brw->state.nr_atoms; i++) {	 
	 const struct brw_tracked_state *atom = brw->state.atoms[i];
	 struct brw_state_flags generated;

	 assert(atom->dirty.mesa ||
		atom->dirty.brw ||
		atom->dirty.cache);

	 if (brw->intel.Fallback)
	    break;

	 if (check_state(state, &atom->dirty)) {
	    if (atom->emit) {
	       atom->emit( brw );
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
	 const struct brw_tracked_state *atom = brw->state.atoms[i];

	 if (brw->intel.Fallback)
	    break;

	 if (check_state(state, &atom->dirty)) {
	    if (atom->emit) {
	       atom->emit( brw );
	    }
	 }
      }
   }

   if (INTEL_DEBUG & DEBUG_STATE) {
      brw_update_dirty_count(mesa_bits, state->mesa);
      brw_update_dirty_count(brw_bits, state->brw);
      brw_update_dirty_count(cache_bits, state->cache);
      if (dirty_count++ % 1000 == 0) {
	 brw_print_dirty_count(mesa_bits, state->mesa);
	 brw_print_dirty_count(brw_bits, state->brw);
	 brw_print_dirty_count(cache_bits, state->cache);
	 fprintf(stderr, "\n");
      }
   }

   if (!brw->intel.Fallback)
      memset(state, 0, sizeof(*state));
}
