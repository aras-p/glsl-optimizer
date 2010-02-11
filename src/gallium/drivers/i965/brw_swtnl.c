
#include "brw_context.h"
#include "brw_pipe_rast.h"


#if 0

static GLboolean need_swtnl( struct brw_context *brw )
{
   const struct pipe_rasterizer_state *rast = &brw->curr.rast->templ;

   /* If we don't require strict OpenGL conformance, never 
    * use fallbacks.  If we're forcing fallbacks, always
    * use fallfacks.
    */
   if (brw->flags.no_swtnl)
      return FALSE;

   if (brw->flags.force_swtnl)
      return TRUE;

   /* Exceeding hw limits on number of VS inputs?
    */
   if (brw->curr.num_vertex_elements == 0 ||
       brw->curr.num_vertex_elements >= BRW_VEP_MAX) {
      return TRUE;
   }

   /* Position array with zero stride?
    *
    * XXX: position isn't always at zero...
    * XXX: eliminate zero-stride arrays
    */
   {
      int ve0_vb = brw->curr.vertex_element[0].vertex_buffer_index;
      
      if (brw->curr.vertex_buffer[ve0_vb].stride == 0)
	 return TRUE;
   }

   /* XXX: short-circuit
    */
   return FALSE;

   if (brw->reduced_primitive == PIPE_PRIM_TRIANGLES) {
      if (rast->poly_smooth)
	 return TRUE;

   }
   
   if (brw->reduced_primitive == PIPE_PRIM_LINES ||
       (brw->reduced_primitive == PIPE_PRIM_TRIANGLES &&
	(rast->fill_cw == PIPE_POLYGON_MODE_LINE ||
	 rast->fill_ccw == PIPE_POLYGON_MODE_LINE)))
   {
      /* BRW hardware will do AA lines, but they are non-conformant it
       * seems.  TBD whether we keep this fallback:
       */
      if (rast->line_smooth)
	 return TRUE;

      /* XXX: was a fallback in mesa (gs doesn't get enough
       * information to know when to reset stipple counter), but there
       * must be a way around it.
       */
      if (rast->line_stipple_enable &&
	  (brw->reduced_primitive == PIPE_PRIM_TRIANGLES ||
	   brw->primitive == PIPE_PRIM_LINE_LOOP || 
	   brw->primitive == PIPE_PRIM_LINE_STRIP))
	 return TRUE;
   }

   
   if (brw->reduced_primitive == PIPE_PRIM_POINTS ||
       (brw->reduced_primitive == PIPE_PRIM_TRIANGLES &&
	(rast->fill_cw == PIPE_POLYGON_MODE_POINT ||
	 rast->fill_ccw == PIPE_POLYGON_MODE_POINT)))
   {
      if (rast->point_smooth)
	 return TRUE;
   }

   /* BRW hardware doesn't handle CLAMP texturing correctly;
    * brw_wm_sampler_state:translate_wrap_mode() treats CLAMP
    * as CLAMP_TO_EDGE instead.  If we're using CLAMP, and
    * we want strict conformance, force the fallback.
    *
    * XXX: need a workaround for this.
    */
      
   /* Nothing stopping us from the fast path now */
   return FALSE;
}

#endif
