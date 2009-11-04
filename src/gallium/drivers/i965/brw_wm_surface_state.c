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
                   
#include "pipe/p_format.h"

#include "brw_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_screen.h"




static void
brw_update_texture_surface( struct brw_context *brw,
			    struct brw_texture *tex,
			    GLuint surf )
{
   brw->wm.surf_bo[surf] = brw_search_cache(&brw->surface_cache,
                                            BRW_SS_SURFACE,
                                            &tex->ss, sizeof tex->ss,
                                            &tex->bo, 1,
                                            NULL);

   if (brw->wm.surf_bo[surf] == NULL) {
      brw->wm.surf_bo[surf] = brw_upload_cache(&brw->surface_cache, BRW_SS_SURFACE,
					       &tex->ss, sizeof tex->ss,
					       &tex->bo, 1,
					       &tex->ss, sizeof tex->ss,
					       NULL, NULL);
      
      /* Emit relocation to surface contents */
      brw->sws->bo_emit_reloc(brw->wm.surf_bo[surf],
			      BRW_USAGE_SAMPLER,
			      0,
			      offsetof(struct brw_surface_state, ss1),
			      tex->bo);
   }
}








/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
brw_update_renderbuffer_surface(struct brw_context *brw,
				struct brw_surface *surface,
				unsigned int unit)
{
   struct brw_surf_ss0 blend_ss0 = brw->curr.blend->ss0;
   struct brw_surface_state ss;

   /* Surfaces are potentially shared between contexts, so can't
    * scribble the in-place ss0 value in the surface.
    */
   memcpy(&ss, &surface->ss, sizeof ss);

   ss.ss0.color_blend        = blend_ss0.color_blend;
   ss.ss0.writedisable_blue  = blend_ss0.writedisable_blue;
   ss.ss0.writedisable_green = blend_ss0.writedisable_green;
   ss.ss0.writedisable_red   = blend_ss0.writedisable_red;
   ss.ss0.writedisable_alpha = blend_ss0.writedisable_alpha;

   brw->sws->bo_unreference(brw->wm.surf_bo[unit]);
   brw->wm.surf_bo[unit] = brw_search_cache(&brw->surface_cache,
					    BRW_SS_SURFACE,
					    &ss, sizeof(ss),
					    &surface->bo, 1,
					    NULL);

   if (brw->wm.surf_bo[unit] == NULL) {

      brw->wm.surf_bo[unit] = brw_upload_cache(&brw->surface_cache,
                                               BRW_SS_SURFACE,
                                               &ss, sizeof ss,
					       &surface->bo, 1,
					       &ss, sizeof ss,
					       NULL, NULL);

      /* XXX: we will only be rendering to this surface:
       */
      brw->sws->bo_emit_reloc(brw->wm.surf_bo[unit],
			      BRW_USAGE_RENDER_TARGET,
			      ss.ss1.base_addr - surface->bo->offset[0], /* XXX */
			      offsetof(struct brw_surface_state, ss1),
			      surface->bo);
   }
}


/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static struct brw_winsys_buffer *
brw_wm_get_binding_table(struct brw_context *brw)
{
   struct brw_winsys_buffer *bind_bo;

   assert(brw->wm.nr_surfaces <= BRW_WM_MAX_SURF);

   /* Note there is no key for this search beyond the values in the
    * relocation array:
    */
   bind_bo = brw_search_cache(&brw->surface_cache, BRW_SS_SURF_BIND,
			      NULL, 0,
			      brw->wm.surf_bo, brw->wm.nr_surfaces,
			      NULL);

   if (bind_bo == NULL) {
      uint32_t data[BRW_WM_MAX_SURF];
      GLuint data_size = brw->wm.nr_surfaces * sizeof data[0];
      int i;

      for (i = 0; i < brw->wm.nr_surfaces; i++)
	 data[i] = brw->wm.surf_bo[i]->offset[0];

      bind_bo = brw_upload_cache( &brw->surface_cache, BRW_SS_SURF_BIND,
				  NULL, 0,
				  brw->wm.surf_bo, brw->wm.nr_surfaces,
				  data, data_size,
				  NULL, NULL);

      /* Emit binding table relocations to surface state */
      for (i = 0; i < brw->wm.nr_surfaces; i++) {
	 brw->sws->bo_emit_reloc(bind_bo,
				 BRW_USAGE_STATE,
				 0,
				 i * sizeof(GLuint),
				 brw->wm.surf_bo[i]);
      }
   }

   return bind_bo;
}

static int prepare_wm_surfaces(struct brw_context *brw )
{
   GLuint i;
   int nr_surfaces = 0;

   /* Unreference old buffers
    */
   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      brw->sws->bo_unreference(brw->wm.surf_bo[i]);
      brw->wm.surf_bo[i] = NULL;
   }


   /* PIPE_NEW_COLOR_BUFFERS | PIPE_NEW_BLEND
    *
    * Update surfaces for drawing buffers.  Mixes in colormask and
    * blend state.
    *
    * XXX: no color buffer case
    */
   for (i = 0; i < brw->curr.fb.nr_cbufs; i++) {
      brw_update_renderbuffer_surface(brw, 
				      brw_surface(brw->curr.fb.cbufs[i]), 
				      nr_surfaces++);
   }

   /* PIPE_NEW_TEXTURE 
    */
   for (i = 0; i < brw->curr.num_textures; i++) {
      brw_update_texture_surface(brw, 
				 brw_texture(brw->curr.texture[i]),
				 nr_surfaces++);
   }

   /* PIPE_NEW_FRAGMENT_CONSTANTS
    */
#if 0
   if (brw->curr.fragment_constants) {
      brw_update_fragment_constant_surface(brw, 
					   brw->curr.fragment_constants, 
					   nr_surfaces++);
   }
#endif

   brw->sws->bo_unreference(brw->wm.bind_bo);
   brw->wm.bind_bo = brw_wm_get_binding_table(brw);

   if (brw->wm.nr_surfaces != nr_surfaces) {
      brw->wm.nr_surfaces = nr_surfaces;
      brw->state.dirty.brw |= BRW_NEW_NR_WM_SURFACES;
   }

   return 0;
}

const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .mesa = (PIPE_NEW_COLOR_BUFFERS |
               PIPE_NEW_BOUND_TEXTURES |
               PIPE_NEW_FRAGMENT_CONSTANTS |
	       PIPE_NEW_BLEND),
      .brw = (BRW_NEW_CONTEXT |
	      BRW_NEW_WM_SURFACES),
      .cache = 0
   },
   .prepare = prepare_wm_surfaces,
};



