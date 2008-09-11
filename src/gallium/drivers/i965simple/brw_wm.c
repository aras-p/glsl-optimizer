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


#include "brw_context.h"
#include "brw_util.h"
#include "brw_wm.h"
#include "brw_eu.h"
#include "brw_state.h"
#include "util/u_memory.h"



static void do_wm_prog( struct brw_context *brw,
			struct brw_fragment_program *fp,
			struct brw_wm_prog_key *key)
{
   struct brw_wm_compile *c = CALLOC_STRUCT(brw_wm_compile);
   const unsigned *program;
   unsigned program_size;

   c->key = *key;
   c->fp = fp;
   
   c->delta_xy[0] = brw_null_reg();
   c->delta_xy[1] = brw_null_reg();
   c->pixel_xy[0] = brw_null_reg();
   c->pixel_xy[1] = brw_null_reg();
   c->pixel_w = brw_null_reg();


   debug_printf("XXXXXXXX FP\n");
   
   brw_wm_glsl_emit(c);

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   /*
    */
   brw->wm.prog_gs_offset = brw_upload_cache( &brw->cache[BRW_WM_PROG],
					      &c->key,
					      sizeof(c->key),
					      program,
					      program_size,
					      &c->prog_data,
					      &brw->wm.prog_data );

   FREE(c);
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *)brw->attribs.FragmentProgram;
   unsigned lookup = 0;
   unsigned line_aa;
   
   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   /* BRW_NEW_DEPTH_STENCIL */
   if (fp->info.uses_kill ||
       brw->attribs.DepthStencil->alpha.enabled)
      lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fp->info.writes_z)
      lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   if (brw->attribs.DepthStencil->depth.enabled)
      lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

   if (brw->attribs.DepthStencil->depth.enabled &&
       brw->attribs.DepthStencil->depth.writemask) /* ?? */
      lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   if (brw->attribs.DepthStencil->stencil[0].enabled) {
      lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

      if (brw->attribs.DepthStencil->stencil[0].write_mask ||
	  brw->attribs.DepthStencil->stencil[1].write_mask)
	 lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;
   }

   /* XXX: when should this be disabled?
    */
   if (1)
      lookup |= IZ_EARLY_DEPTH_TEST_BIT;


   line_aa = AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (brw->attribs.Raster->line_smooth) {
      if (brw->reduced_primitive == PIPE_PRIM_LINES) {
	 line_aa = AA_ALWAYS;
      }
      else if (brw->reduced_primitive == PIPE_PRIM_TRIANGLES) {
	 if (brw->attribs.Raster->fill_ccw == PIPE_POLYGON_MODE_LINE) {
	    line_aa = AA_SOMETIMES;

	    if (brw->attribs.Raster->fill_cw == PIPE_POLYGON_MODE_LINE ||
		(brw->attribs.Raster->cull_mode == PIPE_WINDING_CW))
	       line_aa = AA_ALWAYS;
	 }
	 else if (brw->attribs.Raster->fill_cw == PIPE_POLYGON_MODE_LINE) {
	    line_aa = AA_SOMETIMES;

	    if (brw->attribs.Raster->cull_mode == PIPE_WINDING_CCW)
	       line_aa = AA_ALWAYS;
	 }
      }
   }

   brw_wm_lookup_iz(line_aa,
		    lookup,
		    key);


#if 0
   /* BRW_NEW_SAMPLER 
    *
    * Not doing any of this at the moment:
    */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct pipe_sampler_state *unit = brw->attribs.Samplers[i];

      if (unit) {

	 if (unit->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
	    key->shadowtex_mask |= 1<<i;
	 }
	 if (t->Image[0][t->BaseLevel]->InternalFormat == GL_YCBCR_MESA)
	    key->yuvtex_mask |= 1<<i;
      }
   }
#endif


   /* Extra info:
    */
   key->program_string_id = fp->id;

}


static void brw_upload_wm_prog( struct brw_context *brw )
{
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->attribs.FragmentProgram;

   brw_wm_populate_key(brw, &key);

   /* Make an early check for the key.
    */
   if (brw_search_cache(&brw->cache[BRW_WM_PROG],
			&key, sizeof(key),
			&brw->wm.prog_data,
			&brw->wm.prog_gs_offset))
      return;

   do_wm_prog(brw, fp, &key);
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .brw   = (BRW_NEW_FS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = 0
   },
   .update = brw_upload_wm_prog
};

