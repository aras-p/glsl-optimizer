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
#include "brw_winsys.h"

/* XXX: disabled true constant buffer functionality
 */


/* Creates a new VS constant buffer reflecting the current VS program's
 * constants, if needed by the VS program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
#if 0
static struct brw_winsys_buffer *
brw_vs_update_constant_buffer(struct brw_context *brw)
{
   /* XXX: true constant buffers
    */
   struct brw_vertex_program *vp =
      (struct brw_vertex_program *) brw->vertex_program;
   const struct gl_program_parameter_list *params = vp->program.Base.Parameters;
   const int size = params->NumParameters * 4 * sizeof(GLfloat);
   drm_intel_bo *const_buffer;

   /* BRW_NEW_VERTEX_PROGRAM */
   if (!vp->use_const_buffer)
      return NULL;

   const_buffer = brw->sws->bo_alloc(brw->sws, 
				     BRW_BUFFER_TYPE_SHADER_CONSTANTS,
				     size, 64);

   /* _NEW_PROGRAM_CONSTANTS */
   brw->sws->bo_subdata(const_buffer, 0, size, params->ParameterValues,
                        NULL, 0);

   return const_buffer;
}
#endif

/**
 * Update the surface state for a VS constant buffer.
 *
 * Sets brw->vs.surf_bo[surf] and brw->vp->const_buffer.
 */
#if 0
static void
brw_update_vs_constant_surface( struct brw_context *brw,
                                GLuint surf)
{
   struct brw_surface_key key;
   struct pipe_buffer *cb = brw->curr.vs_constants;
   enum pipe_error ret;

   assert(surf == 0);

   /* If we're in this state update atom, we need to update VS constants, so
    * free the old buffer and create a new one for the new contents.
    */
   ret = brw_vs_update_constant_buffer(brw, &vp->const_buffer);
   if (ret)
      return ret;

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (vp->const_buffer == NULL) {
      bo_reference(brw->vs.surf_bo[surf], NULL);
      return PIPE_OK;
   }

   memset(&key, 0, sizeof(key));

   key.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   key.bo = vp->const_buffer;
   key.depthmode = GL_NONE;
   key.pitch = params->NumParameters;
   key.width = params->NumParameters;
   key.height = 1;
   key.depth = 1;
   key.cpp = 16;

   /*
   printf("%s:\n", __FUNCTION__);
   printf("  width %d  height %d  depth %d  cpp %d  pitch %d\n",
          key.width, key.height, key.depth, key.cpp, key.pitch);
   */

   if (brw_search_cache(&brw->surface_cache,
                        BRW_SS_SURFACE,
                        &key, sizeof(key),
                        &key.bo, key.bo ? 1 : 0,
                        NULL,
                        &brw->vs.surf_bo[surf]))
      return PIPE_OK;

   ret = brw_create_constant_surface(brw, &key
                                     &brw->vs.surf_bo[surf]);
   if (ret)
      return ret;
   
   return PIPE_OK;
}
#endif


/**
 * Constructs the binding table for the VS surface state.
 */
static enum pipe_error
brw_vs_get_binding_table(struct brw_context *brw,
                         struct brw_winsys_buffer **bo_out)
{
#if 0
   static GLuint data[BRW_VS_MAX_SURF]; /* always zero */
   struct brw_winsys_reloc reloc[BRW_VS_MAX_SURF];
   int i;

   /* Emit binding table relocations to surface state */
   for (i = 0; i < BRW_VS_MAX_SURF; i++) {
      make_reloc(&reloc[i],
                 BRW_USAGE_STATE,
                 0,
                 i * 4,
                 brw->vs.surf_bo[i]);
   }
   
   ret = brw_cache_data( &brw->surface_cache, 
                         BRW_SS_SURF_BIND,
                         NULL, 0,
                         reloc, nr_reloc,
                         data, sizeof data,
                         NULL, NULL,
                         bo_out);
   if (ret)
      return ret;

   FREE(data);
   return PIPE_OK;
#else
   return PIPE_OK;
#endif
}

/**
 * Vertex shader surfaces (constant buffer).
 *
 * This consumes the state updates for the constant buffer needing
 * to be updated, and produces BRW_NEW_NR_VS_SURFACES for the VS unit and
 * CACHE_NEW_SURF_BIND for the binding table upload.
 */
static enum pipe_error prepare_vs_surfaces(struct brw_context *brw )
{
   enum pipe_error ret;

#if 0
   int i;
   int nr_surfaces = 0;

   brw_update_vs_constant_surface(ctx, SURF_INDEX_VERT_CONST_BUFFER);

   for (i = 0; i < BRW_VS_MAX_SURF; i++) {
      if (brw->vs.surf_bo[i] != NULL) {
	 nr_surfaces = i + 1;
      }
   }

   if (brw->vs.nr_surfaces != nr_surfaces) {
      brw->state.dirty.brw |= BRW_NEW_NR_VS_SURFACES;
      brw->vs.nr_surfaces = nr_surfaces;
   }
#endif

   /* Note that we don't end up updating the bind_bo if we don't have a
    * surface to be pointing at.  This should be relatively harmless, as it
    * just slightly increases our working set size.
    */
   if (brw->vs.nr_surfaces != 0) {
      ret = brw_vs_get_binding_table(brw, &brw->vs.bind_bo);
      if (ret)
         return ret;
   }

   return PIPE_OK;
}

const struct brw_tracked_state brw_vs_surfaces = {
   .dirty = {
      .mesa = (PIPE_NEW_VERTEX_CONSTANTS |
	       PIPE_NEW_VERTEX_SHADER),
      .brw = 0,
      .cache = 0
   },
   .prepare = prepare_vs_surfaces,
};



