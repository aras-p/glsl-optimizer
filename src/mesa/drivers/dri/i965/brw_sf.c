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
  

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_sf.h"
#include "brw_state.h"

static void compile_sf_prog( struct brw_context *brw,
			     struct brw_sf_prog_key *key )
{
   struct brw_sf_compile c;
   const GLuint *program;
   GLuint program_size;
   GLuint i, idx;

   memset(&c, 0, sizeof(c));

   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func);

   c.key = *key;
   c.nr_attrs = brw_count_bits(c.key.attrs);
   c.nr_attr_regs = (c.nr_attrs+1)/2;
   c.nr_setup_attrs = brw_count_bits(c.key.attrs & DO_SETUP_BITS);
   c.nr_setup_regs = (c.nr_setup_attrs+1)/2;

   c.prog_data.urb_read_length = c.nr_attr_regs;
   c.prog_data.urb_entry_size = c.nr_setup_regs * 2;

   /* Construct map from attribute number to position in the vertex.
    */
   for (i = idx = 0; i < VERT_RESULT_MAX; i++) 
      if (c.key.attrs & (1<<i)) {
	 c.attr_to_idx[i] = idx;
	 c.idx_to_attr[idx] = i;
	 if (i >= VERT_RESULT_TEX0 && i <= VERT_RESULT_TEX7) {
		 c.point_attrs[i].CoordReplace = 
			brw->attribs.Point->CoordReplace[i - VERT_RESULT_TEX0];
	 } else
		 c.point_attrs[i].CoordReplace = GL_FALSE;
	 idx++;
      }
   
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
      return;
   }
	 

   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   /* Upload
    */
   dri_bo_unreference(brw->sf.prog_bo);
   brw->sf.prog_bo = brw_upload_cache( &brw->cache, BRW_SF_PROG,
				       &c.key, sizeof(c.key),
				       NULL, 0,
				       program, program_size,
				       &c.prog_data,
				       &brw->sf.prog_data );
}

/* Calculate interpolants for triangle and line rasterization.
 */
static void upload_sf_prog(struct brw_context *brw)
{
   struct brw_sf_prog_key key;

   memset(&key, 0, sizeof(key));

   /* Populate the key, noting state dependencies:
    */
   /* CACHE_NEW_VS_PROG */
   key.attrs = brw->vs.prog_data->outputs_written; 

   /* BRW_NEW_REDUCED_PRIMITIVE */
   switch (brw->intel.reduced_primitive) {
   case GL_TRIANGLES: 
      /* NOTE: We just use the edgeflag attribute as an indicator that
       * unfilled triangles are active.  We don't actually do the
       * edgeflag testing here, it is already done in the clip
       * program.
       */
      if (key.attrs & (1<<VERT_RESULT_EDGE))
	 key.primitive = SF_UNFILLED_TRIS;
      else
	 key.primitive = SF_TRIANGLES;
      break;
   case GL_LINES: 
      key.primitive = SF_LINES; 
      break;
   case GL_POINTS: 
      key.primitive = SF_POINTS; 
      break;
   }

   key.do_point_sprite = brw->attribs.Point->PointSprite;
   key.SpriteOrigin = brw->attribs.Point->SpriteOrigin;
   /* _NEW_LIGHT */
   key.do_flat_shading = (brw->attribs.Light->ShadeModel == GL_FLAT);
   key.do_twoside_color = (brw->attribs.Light->Enabled && brw->attribs.Light->Model.TwoSide);

   /* _NEW_POLYGON */
   if (key.do_twoside_color)
      key.frontface_ccw = (brw->attribs.Polygon->FrontFace == GL_CCW);

   dri_bo_unreference(brw->sf.prog_bo);
   brw->sf.prog_bo = brw_search_cache(&brw->cache, BRW_SF_PROG,
				      &key, sizeof(key),
				      NULL, 0,
				      &brw->sf.prog_data);
   if (brw->sf.prog_bo == NULL)
      compile_sf_prog( brw, &key );
}


const struct brw_tracked_state brw_sf_prog = {
   .dirty = {
      .mesa  = (_NEW_LIGHT|_NEW_POLYGON|_NEW_POINT),
      .brw   = (BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = upload_sf_prog
};

