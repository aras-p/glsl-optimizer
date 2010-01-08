/* XXX: Constant buffers disabled
 */


/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will be
 * read from this buffer with Data Port Read instructions/messages.
 */
enum pipe_error
brw_create_constant_surface( struct brw_context *brw,
                             struct brw_surface_key *key,
                             struct brw_winsys_buffer **bo_out )
{
   const GLint w = key->width - 1;
   struct brw_winsys_buffer *bo;
   struct brw_winsys_reloc reloc[1];
   enum pipe_error ret;

      /* Emit relocation to surface contents */
   make_reloc(&reloc[0],
              BRW_USAGE_SAMPLER,
              0,
              offsetof(struct brw_surface_state, ss1),
              key->bo);

   
   memset(&surf, 0, sizeof(surf));

   surf.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   surf.ss0.surface_type = BRW_SURFACE_BUFFER;
   surf.ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   surf.ss1.base_addr = 0; /* reloc */

   surf.ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf.ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf.ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf.ss3.pitch = (key->pitch * key->cpp) - 1; /* ignored?? */
   brw_set_surface_tiling(&surf, key->tiling); /* tiling now allowed */
 
   ret = brw_upload_cache(&brw->surface_cache, BRW_SS_SURFACE,
                          key, sizeof(*key),
                          reloc, Elements(reloc),
                          &surf, sizeof(surf),
                          NULL, NULL,
                          &bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}



/**
 * Update the surface state for a WM constant buffer.
 * The constant buffer will be (re)allocated here if needed.
 */
static enum pipe_error
brw_update_wm_constant_surface( struct brw_context *brw,
                                GLuint surf)
{
   struct brw_surface_key key;
   struct brw_fragment_shader *fp = brw->curr.fragment_shader;
   struct pipe_buffer *cbuf = brw->curr.fragment_constants;
   int pitch = cbuf->size / (4 * sizeof(float));
   enum pipe_error ret;

   /* If we're in this state update atom, we need to update WM constants, so
    * free the old buffer and create a new one for the new contents.
    */
   ret = brw_wm_update_constant_buffer(brw, &fp->const_buffer);
   if (ret)
      return ret;

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (cbuf == NULL) {
      bo_reference(&brw->wm.surf_bo[surf], NULL);
      return PIPE_OK;
   }

   memset(&key, 0, sizeof(key));

   key.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   key.ss0.surface_type = BRW_SURFACE_BUFFER;
   key.ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   key.bo = brw_buffer(cbuf)->bo;

   key.ss2.width = (pitch-1) & 0x7f;            /* bits 6:0 of size or width */
   key.ss2.height = ((pitch-1) >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   key.ss3.depth = ((pitch-1) >> 20) & 0x7f;    /* bits 26:20 of size or width */
   key.ss3.pitch = (pitch * 4 * sizeof(float)) - 1; /* ignored?? */
   brw_set_surface_tiling(&surf, key->tiling); /* tiling now allowed */


   /*
   printf("%s:\n", __FUNCTION__);
   printf("  width %d  height %d  depth %d  cpp %d  pitch %d\n",
          key.width, key.height, key.depth, key.cpp, key.pitch);
   */

   if (brw_search_cache(&brw->surface_cache,
                        BRW_SS_SURFACE,
                        &key, sizeof(key),
                        &key.bo, 1,
                        NULL,
                        &brw->wm.surf_bo[surf]))
      return PIPE_OK;

   ret = brw_create_constant_surface(brw, &key, &brw->wm.surf_bo[surf]);
   if (ret)
      return ret;

   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
   return PIPE_OK;
}

/**
 * Updates surface / buffer for fragment shader constant buffer, if
 * one is required.
 *
 * This consumes the state updates for the constant buffer, and produces
 * BRW_NEW_WM_SURFACES to get picked up by brw_prepare_wm_surfaces for
 * inclusion in the binding table.
 */
static enum pipe_error prepare_wm_constant_surface(struct brw_context *brw )
{
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   GLuint surf = SURF_INDEX_FRAG_CONST_BUFFER;

   ret = brw_wm_update_constant_buffer(brw,
                                       &fp->const_buffer);
   if (ret)
      return ret;

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (fp->const_buffer == 0) {
      if (brw->wm.surf_bo[surf] != NULL) {
	 bo_reference(&brw->wm.surf_bo[surf], NULL);
	 brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
      }
      return PIPE_OK;
   }

   ret = brw_update_wm_constant_surface(ctx, surf);
   if (ret)
      return ret;

   return PIPE_OK
}

const struct brw_tracked_state brw_wm_constant_surface = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_FRAGMENT_PROGRAM),
      .cache = 0
   },
   .prepare = prepare_wm_constant_surface,
};
