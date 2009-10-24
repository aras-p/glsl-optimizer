
static void
calculate_clip_key_rast()
{
   if (BRW_IS_IGDNG(brw))
       key.clip_mode = BRW_CLIPMODE_KERNEL_CLIP;
   else
       key.clip_mode = BRW_CLIPMODE_NORMAL;

   key.do_flat_shading = brw->rast->templ.flatshade;

   if (key.primitive == PIPE_PRIM_TRIANGLES) {
      if (brw->rast->templ.cull_mode = PIPE_WINDING_BOTH)
	 key.clip_mode = BRW_CLIPMODE_REJECT_ALL;
      else {
	 key.fill_ccw = CLIP_CULL;
	 key.fill_cw = CLIP_CULL;

	 if (!(brw->rast->templ.cull_mode & PIPE_WINDING_CCW)) {
	    key.fill_ccw = translate_fill(brw->rast.fill_ccw);
	 }

	 if (!(brw->rast->templ.cull_mode & PIPE_WINDING_CW)) {
	    key.fill_cw = translate_fill(brw->rast.fill_cw);
	 }

	 if (key.fill_cw != CLIP_FILL ||
	     key.fill_ccw != CLIP_FILL) {
	    key.do_unfilled = 1;
	    key.clip_mode = BRW_CLIPMODE_CLIP_NON_REJECTED;
	 }

	 key.offset_ccw = brw->rast.templ.offset_ccw;
	 key.offset_cw = brw->rast.templ.offset_cw;

	 if (brw->rast.templ.light_twoside &&
	     key.fill_cw != CLIP_CULL) 
	    key.copy_bfc_cw = 1;

	 if (brw->rast.templ.light_twoside &&
	     key.fill_ccw != CLIP_CULL) 
	    key.copy_bfc_ccw = 1;
	 }
      }
   }
}
