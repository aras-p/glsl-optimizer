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




static enum pipe_error
brw_update_texture_surface( struct brw_context *brw,
			    struct brw_texture *tex,
                            struct brw_winsys_buffer **bo_out)
{
   enum pipe_error ret;

   if (brw_search_cache(&brw->surface_cache,
                        BRW_SS_SURFACE,
                        &tex->ss, sizeof tex->ss,
                        &tex->bo, 1,
                        NULL,
                        bo_out))
      return PIPE_OK;

   ret = brw_upload_cache(&brw->surface_cache, BRW_SS_SURFACE,
                          &tex->ss, sizeof tex->ss,
                          &tex->bo, 1,
                          &tex->ss, sizeof tex->ss,
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;
      
   /* Emit relocation to surface contents */
   ret = brw->sws->bo_emit_reloc(*bo_out,
                                 BRW_USAGE_SAMPLER,
                                 0,
                                 offsetof(struct brw_surface_state, ss1),
                                 tex->bo);
   if (ret)
      return ret;

   return PIPE_OK;
}








/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static enum pipe_error
brw_update_render_surface(struct brw_context *brw,
                          struct brw_surface *surface,
                          struct brw_winsys_buffer **bo_out)
{
   struct brw_surf_ss0 blend_ss0 = brw->curr.blend->ss0;
   struct brw_surface_state ss;
   enum pipe_error ret;

   /* Surfaces are potentially shared between contexts, so can't
    * scribble the in-place ss0 value in the surface.
    */
   memcpy(&ss, &surface->ss, sizeof ss);

   ss.ss0.color_blend        = blend_ss0.color_blend;
   ss.ss0.writedisable_blue  = blend_ss0.writedisable_blue;
   ss.ss0.writedisable_green = blend_ss0.writedisable_green;
   ss.ss0.writedisable_red   = blend_ss0.writedisable_red;
   ss.ss0.writedisable_alpha = blend_ss0.writedisable_alpha;

   if (brw_search_cache(&brw->surface_cache,
                        BRW_SS_SURFACE,
                        &ss, sizeof(ss),
                        &surface->bo, 1,
                        NULL,
                        bo_out))
      return PIPE_OK;
       
   ret = brw_upload_cache(&brw->surface_cache,
                          BRW_SS_SURFACE,
                          &ss, sizeof ss,
                          &surface->bo, 1,
                          &ss, sizeof ss,
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;

      /* XXX: we will only be rendering to this surface:
       */
   ret = brw->sws->bo_emit_reloc(*bo_out,
                                 BRW_USAGE_RENDER_TARGET,
                                 ss.ss1.base_addr - surface->bo->offset[0], /* XXX */
                                 offsetof(struct brw_surface_state, ss1),
                                 surface->bo);
   if (ret)
      return ret;

   return PIPE_OK;
}


/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static enum pipe_error
brw_wm_get_binding_table(struct brw_context *brw,
                         struct brw_winsys_buffer **bo_out )
{
   enum pipe_error ret;
   uint32_t data[BRW_WM_MAX_SURF];
   GLuint data_size = brw->wm.nr_surfaces * sizeof data[0];
   int i;

   assert(brw->wm.nr_surfaces <= BRW_WM_MAX_SURF);
   assert(brw->wm.nr_surfaces > 0);

   /* Note there is no key for this search beyond the values in the
    * relocation array:
    */
   if (brw_search_cache(&brw->surface_cache, BRW_SS_SURF_BIND,
                        NULL, 0,
                        brw->wm.surf_bo,
                        brw->wm.nr_surfaces,
                        NULL,
                        bo_out))
      return PIPE_OK;

   for (i = 0; i < brw->wm.nr_surfaces; i++)
      data[i] = brw->wm.surf_bo[i]->offset[0];

   ret = brw_upload_cache( &brw->surface_cache, BRW_SS_SURF_BIND,
                           NULL, 0,
                           brw->wm.surf_bo, brw->wm.nr_surfaces,
                           data, data_size,
                           NULL, NULL,
                           bo_out);
   if (ret)
      return ret;

   /* Emit binding table relocations to surface state */
   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      ret = brw->sws->bo_emit_reloc(*bo_out,
                                    BRW_USAGE_STATE,
                                    0,
                                    i * sizeof(GLuint),
                                    brw->wm.surf_bo[i]);
      if (ret)
         return ret;
   }

   return PIPE_OK;
}

static enum pipe_error prepare_wm_surfaces(struct brw_context *brw )
{
   enum pipe_error ret;
   int nr_surfaces = 0;
   GLuint i;

   /* PIPE_NEW_COLOR_BUFFERS | PIPE_NEW_BLEND
    *
    * Update surfaces for drawing buffers.  Mixes in colormask and
    * blend state.
    *
    * XXX: no color buffer case
    */
   for (i = 0; i < brw->curr.fb.nr_cbufs; i++) {
      ret = brw_update_render_surface(brw, 
                                      brw_surface(brw->curr.fb.cbufs[i]), 
                                      &brw->wm.surf_bo[nr_surfaces++]);
      if (ret)
         return ret;
   }

   /* PIPE_NEW_TEXTURE 
    */
   for (i = 0; i < brw->curr.num_textures; i++) {
      ret = brw_update_texture_surface(brw, 
                                       brw_texture(brw->curr.texture[i]),
                                       &brw->wm.surf_bo[nr_surfaces++]);
      if (ret)
         return ret;
   }

   /* PIPE_NEW_FRAGMENT_CONSTANTS
    */
#if 0
   if (brw->curr.fragment_constants) {
      ret = brw_update_fragment_constant_surface(brw, 
                                                 brw->curr.fragment_constants, 
                                                 &brw->wm.surf_bo[nr_surfaces++]);
      if (ret)
         return ret;
   }
#endif

   if (brw->wm.nr_surfaces != nr_surfaces) {

      /* Unreference any left-over old buffers
       */
      for (i = nr_surfaces; i < brw->wm.nr_surfaces; i++)
         bo_reference(&brw->wm.surf_bo[i], NULL);

      brw->wm.nr_surfaces = nr_surfaces;
      brw->state.dirty.brw |= BRW_NEW_NR_WM_SURFACES;
   }

   ret = brw_wm_get_binding_table(brw, &brw->wm.bind_bo);
   if (ret)
      return ret;

   return PIPE_OK;
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



