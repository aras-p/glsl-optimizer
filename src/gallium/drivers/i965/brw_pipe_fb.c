
/**
 * called from intelDrawBuffer()
 */
static void brw_set_draw_region( struct pipe_context *pipe, 
                                 struct intel_region *color_regions[],
                                 struct intel_region *depth_region,
                                 GLuint num_color_regions)
{
   struct brw_context *brw = brw_context(pipe);
   GLuint i;

   /* release old color/depth regions */
   if (brw->state.depth_region != depth_region)
      brw->state.dirty.brw |= BRW_NEW_DEPTH_BUFFER;
   for (i = 0; i < brw->state.nr_color_regions; i++)
       intel_region_release(&brw->state.color_regions[i]);
   intel_region_release(&brw->state.depth_region);

   /* reference new color/depth regions */
   for (i = 0; i < num_color_regions; i++)
       intel_region_reference(&brw->state.color_regions[i], color_regions[i]);
   intel_region_reference(&brw->state.depth_region, depth_region);
   brw->state.nr_color_regions = num_color_regions;
}
