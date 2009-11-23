
#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_pipe_rast.h"
#include "brw_wm.h"


static unsigned translate_fill( unsigned fill )
{
   switch (fill) {
   case PIPE_POLYGON_MODE_FILL:
      return CLIP_FILL;
   case PIPE_POLYGON_MODE_LINE:
      return CLIP_LINE;
   case PIPE_POLYGON_MODE_POINT:
      return CLIP_POINT;
   default:
      assert(0);
      return CLIP_FILL;
   }
}


/* Calculates the key for triangle-mode clipping.  Non-triangle
 * clipping keys use much less information and are computed on the
 * fly.
 */
static void
calculate_clip_key_rast( const struct brw_context *brw,
			 const struct pipe_rasterizer_state *templ,
			 const struct brw_rasterizer_state *rast,
			 struct brw_clip_prog_key *key)
{
   memset(key, 0, sizeof *key);

   if (brw->chipset.is_igdng)
       key->clip_mode = BRW_CLIPMODE_KERNEL_CLIP;
   else
       key->clip_mode = BRW_CLIPMODE_NORMAL;

   key->do_flat_shading = templ->flatshade;

   if (templ->cull_mode == PIPE_WINDING_BOTH) {
      key->clip_mode = BRW_CLIPMODE_REJECT_ALL;
      return;
   }

   key->fill_ccw = CLIP_CULL;
   key->fill_cw = CLIP_CULL;

   if (!(templ->cull_mode & PIPE_WINDING_CCW)) {
      key->fill_ccw = translate_fill(templ->fill_ccw);
   }

   if (!(templ->cull_mode & PIPE_WINDING_CW)) {
      key->fill_cw = translate_fill(templ->fill_cw);
   }

   if (key->fill_cw == CLIP_LINE ||
       key->fill_ccw == CLIP_LINE ||
       key->fill_cw == CLIP_POINT ||
       key->fill_ccw == CLIP_POINT) {
      key->do_unfilled = 1;
      key->clip_mode = BRW_CLIPMODE_CLIP_NON_REJECTED;
   }

   key->offset_ccw = templ->offset_ccw;
   key->offset_cw = templ->offset_cw;

   if (templ->light_twoside && key->fill_cw != CLIP_CULL) 
      key->copy_bfc_cw = 1;
   
   if (templ->light_twoside && key->fill_ccw != CLIP_CULL) 
      key->copy_bfc_ccw = 1;
}


static void
calculate_line_stipple_rast( const struct pipe_rasterizer_state *templ,
			     struct brw_line_stipple *bls )
{
   GLfloat tmp = 1.0f / (templ->line_stipple_factor + 1);
   GLint tmpi = tmp * (1<<13);

   bls->header.opcode = CMD_LINE_STIPPLE_PATTERN;
   bls->header.length = sizeof(*bls)/4 - 2;
   bls->bits0.pattern = templ->line_stipple_pattern;
   bls->bits1.repeat_count = templ->line_stipple_factor + 1;
   bls->bits1.inverse_repeat_count = tmpi;
}

static void *brw_create_rasterizer_state( struct pipe_context *pipe,
					  const struct pipe_rasterizer_state *templ )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_rasterizer_state *rast;

   rast = CALLOC_STRUCT(brw_rasterizer_state);
   if (rast == NULL)
      return NULL;

   rast->templ = *templ;

   calculate_clip_key_rast( brw, templ, rast, &rast->clip_key );
   
   if (templ->line_stipple_enable)
      calculate_line_stipple_rast( templ, &rast->bls );

   /* Caclculate lookup value for WM IZ table.
    */
   if (templ->line_smooth) {
      if (templ->fill_cw == PIPE_POLYGON_MODE_LINE &&
	  templ->fill_ccw == PIPE_POLYGON_MODE_LINE) {
	 rast->unfilled_aa_line = AA_ALWAYS;
      }
      else if (templ->fill_cw == PIPE_POLYGON_MODE_LINE ||
	       templ->fill_ccw == PIPE_POLYGON_MODE_LINE) {
	 rast->unfilled_aa_line = AA_SOMETIMES;
      }
      else {
	 rast->unfilled_aa_line = AA_NEVER;
      }
   }
   else {
      rast->unfilled_aa_line = AA_NEVER;
   }

   return (void *)rast;
}


static void brw_bind_rasterizer_state(struct pipe_context *pipe,
				 void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.rast = (const struct brw_rasterizer_state *)cso;
   brw->state.dirty.mesa |= PIPE_NEW_RAST;
}

static void brw_delete_rasterizer_state(struct pipe_context *pipe,
				  void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   assert((const void *)cso != (const void *)brw->curr.rast);
   FREE(cso);
}



void brw_pipe_rast_init( struct brw_context *brw )
{
   brw->base.create_rasterizer_state = brw_create_rasterizer_state;
   brw->base.bind_rasterizer_state = brw_bind_rasterizer_state;
   brw->base.delete_rasterizer_state = brw_delete_rasterizer_state;
}

void brw_pipe_rast_cleanup( struct brw_context *brw )
{
}
