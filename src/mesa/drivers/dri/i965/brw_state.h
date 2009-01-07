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
    

#ifndef BRW_STATE_H
#define BRW_STATE_H

#include "brw_context.h"

static inline void
brw_add_validated_bo(struct brw_context *brw, dri_bo *bo)
{
   assert(brw->state.validated_bo_count < ARRAY_SIZE(brw->state.validated_bos));

   if (bo != NULL) {
      dri_bo_reference(bo);
      brw->state.validated_bos[brw->state.validated_bo_count++] = bo;
   }
};

const struct brw_tracked_state brw_blend_constant_color;
const struct brw_tracked_state brw_cc_unit;
const struct brw_tracked_state brw_cc_vp;
const struct brw_tracked_state brw_check_fallback;
const struct brw_tracked_state brw_clip_prog;
const struct brw_tracked_state brw_clip_unit;
const struct brw_tracked_state brw_constant_buffer_state;
const struct brw_tracked_state brw_constant_buffer;
const struct brw_tracked_state brw_curbe_offsets;
const struct brw_tracked_state brw_invarient_state;
const struct brw_tracked_state brw_gs_prog;
const struct brw_tracked_state brw_gs_unit;
const struct brw_tracked_state brw_line_stipple;
const struct brw_tracked_state brw_aa_line_parameters;
const struct brw_tracked_state brw_pipelined_state_pointers;
const struct brw_tracked_state brw_binding_table_pointers;
const struct brw_tracked_state brw_depthbuffer;
const struct brw_tracked_state brw_polygon_stipple_offset;
const struct brw_tracked_state brw_polygon_stipple;
const struct brw_tracked_state brw_program_parameters;
const struct brw_tracked_state brw_recalculate_urb_fence;
const struct brw_tracked_state brw_sf_prog;
const struct brw_tracked_state brw_sf_unit;
const struct brw_tracked_state brw_sf_vp;
const struct brw_tracked_state brw_state_base_address;
const struct brw_tracked_state brw_urb_fence;
const struct brw_tracked_state brw_vertex_state;
const struct brw_tracked_state brw_vs_prog;
const struct brw_tracked_state brw_vs_unit;
const struct brw_tracked_state brw_wm_input_sizes;
const struct brw_tracked_state brw_wm_prog;
const struct brw_tracked_state brw_wm_samplers;
const struct brw_tracked_state brw_wm_surfaces;
const struct brw_tracked_state brw_wm_unit;

const struct brw_tracked_state brw_psp_urb_cbs;

const struct brw_tracked_state brw_pipe_control;

const struct brw_tracked_state brw_clear_surface_cache;
const struct brw_tracked_state brw_clear_batch_cache;

const struct brw_tracked_state brw_drawing_rect;
const struct brw_tracked_state brw_indices;
const struct brw_tracked_state brw_vertices;

/***********************************************************************
 * brw_state.c
 */
void brw_validate_state(struct brw_context *brw);
void brw_upload_state(struct brw_context *brw);
void brw_init_state(struct brw_context *brw);
void brw_destroy_state(struct brw_context *brw);

/***********************************************************************
 * brw_state_cache.c
 */
dri_bo *brw_cache_data(struct brw_cache *cache,
		       enum brw_cache_id cache_id,
		       const void *data,
		       dri_bo **reloc_bufs,
		       GLuint nr_reloc_bufs);

dri_bo *brw_cache_data_sz(struct brw_cache *cache,
			  enum brw_cache_id cache_id,
			  const void *data,
			  GLuint data_size,
			  dri_bo **reloc_bufs,
			  GLuint nr_reloc_bufs);

dri_bo *brw_upload_cache( struct brw_cache *cache,
			  enum brw_cache_id cache_id,
			  const void *key,
			  GLuint key_sz,
			  dri_bo **reloc_bufs,
			  GLuint nr_reloc_bufs,
			  const void *data,
			  GLuint data_sz,
			  const void *aux,
			  void *aux_return );

dri_bo *brw_search_cache( struct brw_cache *cache,
			  enum brw_cache_id cache_id,
			  const void *key,
			  GLuint key_size,
			  dri_bo **reloc_bufs,
			  GLuint nr_reloc_bufs,
			  void *aux_return);
void brw_state_cache_check_size( struct brw_context *brw );

void brw_init_cache( struct brw_context *brw );
void brw_destroy_cache( struct brw_context *brw );

/***********************************************************************
 * brw_state_batch.c
 */
#define BRW_BATCH_STRUCT(brw, s) intel_batchbuffer_data( brw->intel.batch, (s), sizeof(*(s)), IGNORE_CLIPRECTS)
#define BRW_CACHED_BATCH_STRUCT(brw, s) brw_cached_batch_struct( brw, (s), sizeof(*(s)) )

GLboolean brw_cached_batch_struct( struct brw_context *brw,
				   const void *data,
				   GLuint sz );
void brw_destroy_batch_cache( struct brw_context *brw );
void brw_clear_batch_cache_flush( struct brw_context *brw );

#endif
