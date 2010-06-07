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

#include "main/mtypes.h"
#include "main/texstore.h"
#include "shader/prog_parameter.h"

#include "brw_context.h"
#include "brw_state.h"

/* Creates a new VS constant buffer reflecting the current VS program's
 * constants, if needed by the VS program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
prepare_vs_constants(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   struct brw_vertex_program *vp =
      (struct brw_vertex_program *) brw->vertex_program;
   const struct gl_program_parameter_list *params = vp->program.Base.Parameters;
   const int size = params->NumParameters * 4 * sizeof(GLfloat);
   int i;

   if (vp->program.IsNVProgram)
      _mesa_load_tracked_matrices(ctx);

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   _mesa_load_state_parameters(&brw->intel.ctx, vp->program.Base.Parameters);

   /* BRW_NEW_VERTEX_PROGRAM */
   if (!vp->use_const_buffer) {
      if (brw->vs.const_bo) {
	 drm_intel_bo_unreference(brw->vs.const_bo);
	 brw->vs.const_bo = NULL;
	 brw->state.dirty.brw |= BRW_NEW_VS_CONSTBUF;
      }
      return;
   }

   /* _NEW_PROGRAM_CONSTANTS */
   drm_intel_bo_unreference(brw->vs.const_bo);
   brw->vs.const_bo = drm_intel_bo_alloc(intel->bufmgr, "vp_const_buffer",
					 size, 64);

   drm_intel_gem_bo_map_gtt(brw->vs.const_bo);
   for (i = 0; i < params->NumParameters; i++) {
      memcpy(brw->vs.const_bo->virtual + i * 4 * sizeof(float),
	     params->ParameterValues[i],
	     4 * sizeof(float));
   }
   drm_intel_gem_bo_unmap_gtt(brw->vs.const_bo);
   brw->state.dirty.brw |= BRW_NEW_VS_CONSTBUF;
}

const struct brw_tracked_state brw_vs_constants = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_VERTEX_PROGRAM),
      .cache = 0
   },
   .prepare = prepare_vs_constants,
};

/**
 * Update the surface state for a VS constant buffer.
 *
 * Sets brw->vs.surf_bo[surf] and brw->vp->const_buffer.
 */
static void
brw_update_vs_constant_surface( GLcontext *ctx,
                                GLuint surf)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_surface_key key;
   struct brw_vertex_program *vp =
      (struct brw_vertex_program *) brw->vertex_program;
   const struct gl_program_parameter_list *params = vp->program.Base.Parameters;

   assert(surf == 0);

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (brw->vs.const_bo == NULL) {
      drm_intel_bo_unreference(brw->vs.surf_bo[surf]);
      brw->vs.surf_bo[surf] = NULL;
      return;
   }

   memset(&key, 0, sizeof(key));

   key.format = MESA_FORMAT_RGBA_FLOAT32;
   key.internal_format = GL_RGBA;
   key.bo = brw->vs.const_bo;
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

   drm_intel_bo_unreference(brw->vs.surf_bo[surf]);
   brw->vs.surf_bo[surf] = brw_search_cache(&brw->surface_cache,
                                            BRW_SS_SURFACE,
                                            &key, sizeof(key),
                                            &key.bo, 1,
                                            NULL);
   if (brw->vs.surf_bo[surf] == NULL) {
      brw->vs.surf_bo[surf] = brw_create_constant_surface(brw, &key);
   }
}


static void
prepare_vs_surfaces(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
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

   for (i = 0; i < BRW_VS_MAX_SURF; i++) {
      brw_add_validated_bo(brw, brw->vs.surf_bo[i]);
   }
}

/**
 * Vertex shader surfaces (constant buffer).
 *
 * This consumes the state updates for the constant buffer needing
 * to be updated, and produces BRW_NEW_NR_VS_SURFACES for the VS unit and
 * CACHE_NEW_SURF_BIND for the binding table upload.
 */
static void upload_vs_surfaces(struct brw_context *brw)
{
   uint32_t *bind;
   int i;

   /* BRW_NEW_NR_VS_SURFACES */
   if (brw->vs.nr_surfaces == 0) {
      if (brw->vs.bind_bo) {
	 drm_intel_bo_unreference(brw->vs.bind_bo);
	 brw->vs.bind_bo = NULL;
	 brw->state.dirty.brw |= BRW_NEW_BINDING_TABLE;
      }
      return;
   }

   /* Might want to calculate nr_surfaces first, to avoid taking up so much
    * space for the binding table. (once we have vs samplers)
    */
   bind = brw_state_batch(brw, sizeof(uint32_t) * BRW_VS_MAX_SURF,
			  32, &brw->vs.bind_bo, &brw->vs.bind_bo_offset);

   for (i = 0; i < BRW_VS_MAX_SURF; i++) {
      /* BRW_NEW_VS_CONSTBUF */
      if (brw->vs.surf_bo[i]) {
	 drm_intel_bo_emit_reloc(brw->vs.bind_bo,
				 brw->vs.bind_bo_offset + i * sizeof(uint32_t),
				 brw->vs.surf_bo[i], 0,
				 I915_GEM_DOMAIN_INSTRUCTION, 0);
	 bind[i] = brw->vs.surf_bo[i]->offset;
      } else {
	 bind[i] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_BINDING_TABLE;
}

const struct brw_tracked_state brw_vs_surfaces = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_VS_CONSTBUF |
	      BRW_NEW_NR_VS_SURFACES |
	      BRW_NEW_BATCH),
      .cache = 0
   },
   .prepare = prepare_vs_surfaces,
   .emit = upload_vs_surfaces,
};
