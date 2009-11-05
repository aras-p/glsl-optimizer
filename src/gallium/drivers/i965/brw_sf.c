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
  
#include "pipe/p_state.h"

#include "brw_batchbuffer.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_pipe_rast.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_sf.h"
#include "brw_state.h"

static enum pipe_error compile_sf_prog( struct brw_context *brw,
                                        struct brw_sf_prog_key *key,
                                        struct brw_winsys_buffer **bo_out )
{
   enum pipe_error ret;
   struct brw_sf_compile c;
   const GLuint *program;
   GLuint program_size;

   memset(&c, 0, sizeof(c));

   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func);

   c.key = *key;
   c.nr_attrs = c.key.nr_attrs;
   c.nr_attr_regs = (c.nr_attrs+1)/2;
   c.nr_setup_attrs = c.key.nr_attrs;
   c.nr_setup_regs = (c.nr_setup_attrs+1)/2;

   c.prog_data.urb_read_length = c.nr_attr_regs;
   c.prog_data.urb_entry_size = c.nr_setup_regs * 2;

   
   /* Which primitive?  Or all three? 
    */
   switch (key->primitive) {
   case SF_TRIANGLES:
      c.nr_verts = 3;
      brw_emit_tri_setup( &c, GL_TRUE );
      break;
   case SF_LINES:
      c.nr_verts = 2;
      brw_emit_line_setup( &c, GL_TRUE );
      break;
   case SF_POINTS:
      c.nr_verts = 1;
      if (key->do_point_sprite)
	  brw_emit_point_sprite_setup( &c, GL_TRUE );
      else
	  brw_emit_point_setup( &c, GL_TRUE );
      break;
   case SF_UNFILLED_TRIS:
      c.nr_verts = 3;
      brw_emit_anyprim_setup( &c );
      break;
   default:
      assert(0);
      return PIPE_ERROR_BAD_INPUT;
   }

   /* get the program
    */
   ret = brw_get_program(&c.func, &program, &program_size);
   if (ret)
      return ret;

   /* Upload
    */
   ret = brw_upload_cache( &brw->cache, BRW_SF_PROG,
                           &c.key, sizeof(c.key),
                           NULL, 0,
                           program, program_size,
                           &c.prog_data,
                           &brw->sf.prog_data,
                           bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}

/* Calculate interpolants for triangle and line rasterization.
 */
static enum pipe_error upload_sf_prog(struct brw_context *brw)
{
   enum pipe_error ret;
   struct brw_sf_prog_key key;

   memset(&key, 0, sizeof(key));

   /* Populate the key, noting state dependencies:
    */
   /* CACHE_NEW_VS_PROG */
   key.nr_attrs = brw->curr.vertex_shader->info.file_max[TGSI_FILE_OUTPUT] + 1;


   /* XXX: this is probably where the mapping between vertex shader
    * outputs and fragment shader inputs should be handled.  Assume
    * for now 1:1 correspondance.
    *
    * XXX: scan frag shader inputs to work out linear vs. perspective
    * interpolation below.
    *
    * XXX: as long as we're hard-wiring, is eg. position required to
    * be linear?
    */
   key.linear_attrs = 0;
   key.persp_attrs = (1 << key.nr_attrs) - 1;

   /* BRW_NEW_REDUCED_PRIMITIVE */
   switch (brw->reduced_primitive) {
   case PIPE_PRIM_TRIANGLES: 
      /* PIPE_NEW_RAST
       */
      if (brw->curr.rast->templ.fill_cw != PIPE_POLYGON_MODE_FILL ||
	  brw->curr.rast->templ.fill_ccw != PIPE_POLYGON_MODE_FILL)
	 key.primitive = SF_UNFILLED_TRIS;
      else
	 key.primitive = SF_TRIANGLES;
      break;
   case PIPE_PRIM_LINES: 
      key.primitive = SF_LINES; 
      break;
   case PIPE_PRIM_POINTS: 
      key.primitive = SF_POINTS; 
      break;
   }

   key.do_point_sprite = brw->curr.rast->templ.point_sprite;
   key.sprite_origin_lower_left = 0; /* XXX: ctx->Point.SpriteOrigin - fix rast state */
   key.do_flat_shading = brw->curr.rast->templ.flatshade;
   key.do_twoside_color = brw->curr.rast->templ.light_twoside;

   if (key.do_twoside_color) {
      key.frontface_ccw = (brw->curr.rast->templ.front_winding == 
			   PIPE_WINDING_CCW);
   }

   if (brw_search_cache(&brw->cache, BRW_SF_PROG,
                        &key, sizeof(key),
                        NULL, 0,
                        &brw->sf.prog_data,
                        &brw->sf.prog_bo))
      return PIPE_OK;

   ret = compile_sf_prog( brw, &key, &brw->sf.prog_bo );
   if (ret)
      return ret;

   return PIPE_OK;
}


const struct brw_tracked_state brw_sf_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_RAST | PIPE_NEW_VERTEX_SHADER),
      .brw   = (BRW_NEW_REDUCED_PRIMITIVE),
      .cache = 0
   },
   .prepare = upload_sf_prog
};

