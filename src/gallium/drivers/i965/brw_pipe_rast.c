
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


static void
calculate_line_stipple_rast()
{
   GLfloat tmp;
   GLint tmpi;

   memset(&bls, 0, sizeof(bls));
   bls.header.opcode = CMD_LINE_STIPPLE_PATTERN;
   bls.header.length = sizeof(bls)/4 - 2;
   bls.bits0.pattern = brw->curr.rast.line_stipple_pattern;
   bls.bits1.repeat_count = brw->curr.rast.line_stipple_factor + 1;

   tmp = 1.0 / (GLfloat) bls.bits1.repeat_count;
   tmpi = tmp * (1<<13);

   bls.bits1.inverse_repeat_count = tmpi;

}



static void
calculate_wm_lookup()
{
   if (rast->fill_cw == PIPE_POLYGON_MODE_LINE &&
       rast->fill_ccw == PIPE_POLYGON_MODE_LINE) {
      line_aa = AA_ALWAYS;
   }
   else if (rast->fill_cw == PIPE_POLYGON_MODE_LINE ||
	    rast->fill_ccw == PIPE_POLYGON_MODE_LINE) {
      line_aa = AA_SOMETIMES;
   }
   else {
      line_aa = AA_NEVER;
   }
}
