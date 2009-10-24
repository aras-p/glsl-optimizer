   /* _NEW_BUFFERS */
   if (IS_965(brw->brw_screen->pci_id) &&
       !IS_G4X(brw->brw_screen->pci_id)) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
	 struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
	 struct intel_renderbuffer *irb = intel_renderbuffer(rb);

	 /* The original gen4 hardware couldn't set up WM surfaces pointing
	  * at an offset within a tile, which can happen when rendering to
	  * anything but the base level of a texture or the +X face/0 depth.
	  * This was fixed with the 4 Series hardware.
	  *
	  * For these original chips, you would have to make the depth and
	  * color destination surfaces include information on the texture
	  * type, LOD, face, and various limits to use them as a destination.
	  * I would have done this, but there's also a nasty requirement that
	  * the depth and the color surfaces all be of the same LOD, which
	  * may be a worse requirement than this alignment.  (Also, we may
	  * want to just demote the texture to untiled, instead).
	  */
	 if (irb->region && 
	     irb->region->tiling != I915_TILING_NONE &&
	     (irb->region->draw_offset & 4095)) {
	    DBG("FALLBACK: non-tile-aligned destination for tiled FBO\n");
	    return GL_TRUE;
	 }
      }
