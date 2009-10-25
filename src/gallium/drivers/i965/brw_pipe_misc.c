
static void brw_set_polygon_stipple( struct pipe_context *pipe,
				     const unsigned *stipple )
{
   struct brw_polygon_stipple *bps = &brw->curr.bps;
   GLuint i;

   memset(bps, 0, sizeof *bps);
   bps->header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps->header.length = sizeof *bps/4-2;

   for (i = 0; i < 32; i++)
      bps->stipple[i] = brw->curr.poly_stipple[i]; /* don't invert */
}
