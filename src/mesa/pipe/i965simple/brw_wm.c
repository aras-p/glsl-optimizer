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
#include "brw_state.h"

unsigned brw_wm_nr_args( unsigned opcode )
{
   switch (opcode) {

   case WM_PIXELXY:
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_FRC:
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_LG2:
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_SIN:
   case TGSI_OPCODE_SCS:
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXD:
   case TGSI_OPCODE_KIL:
   case TGSI_OPCODE_LIT:
   case WM_CINTERP:
   case WM_WPOSXY:
      return 1;

   case TGSI_OPCODE_POW:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_XPD:
   case TGSI_OPCODE_DP3:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_DPH:
   case TGSI_OPCODE_DST:
   case WM_LINTERP:
   case WM_DELTAXY:
   case WM_PIXELW:
      return 2;

   case WM_FB_WRITE:
   case WM_PINTERP:
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_CMP:
   case TGSI_OPCODE_LRP:
      return 3;

   default:
      return 0;
   }
}


unsigned brw_wm_is_scalar_result( unsigned opcode )
{
   switch (opcode) {
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_LG2:
   case TGSI_OPCODE_POW:
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_SIN:
   case TGSI_OPCODE_DP3:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_DPH:
   case TGSI_OPCODE_DST:
      return 1;

   default:
      return 0;
   }
}


static void do_wm_prog( struct brw_context *brw,
			struct brw_fragment_program *fp,
			struct brw_wm_prog_key *key)
{
   struct brw_wm_compile *c;
   const unsigned *program;
   unsigned program_size;

   c = brw->wm.compile_data;
   if (c == NULL) {
     brw->wm.compile_data = calloc(1, sizeof(*brw->wm.compile_data));
     c = brw->wm.compile_data;
   } else {
     memset(c, 0, sizeof(*brw->wm.compile_data));
   }
   memcpy(&c->key, key, sizeof(*key));

   c->fp = fp;
   fprintf(stderr, "XXXXXXXX FP\n");
   
#if 0
   c->env_param = brw->intel.ctx.FragmentProgram.Parameters;

   if (brw_wm_is_glsl(&c->fp->program)) {
       brw_wm_glsl_emit(c);
   } else
   {
       /* Augment fragment program.  Add instructions for pre- and
	* post-fragment-program tasks such as interpolation and fogging.
	*/
       brw_wm_pass_fp(c);

       /* Translate to intermediate representation.  Build register usage
	* chains.
	*/
       brw_wm_pass0(c);

       /* Dead code removal.
	*/
       brw_wm_pass1(c);

       /* Register allocation.
	*/
       c->grf_limit = BRW_WM_MAX_GRF/2;

       /* This is where we start emitting gen4 code:
	*/
       brw_init_compile(&c->func);

       brw_wm_pass2(c);

       c->prog_data.total_grf = c->max_wm_grf;
       if (c->last_scratch) {
	   c->prog_data.total_scratch =
	       c->last_scratch + 0x40;
       } else {
	   c->prog_data.total_scratch = 0;
       }

       /* Emit GEN4 code.
	*/
       brw_wm_emit(c);
   }
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
#endif
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *)brw->attribs.FragmentProgram;
   unsigned lookup = 0;
   unsigned line_aa;
   unsigned i;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   /* _NEW_COLOR */
   if (fp->UsesKill ||
       brw->attribs.AlphaTest->enabled)
      lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fp->ComputesDepth)
      lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   /* _NEW_DEPTH */
   if (brw->attribs.DepthStencil->depth.enabled)
      lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

   if (brw->attribs.DepthStencil->depth.enabled &&
       brw->attribs.DepthStencil->depth.writemask) /* ?? */
      lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   /* _NEW_STENCIL */
   if (brw->attribs.DepthStencil->stencil.front_enabled) {
      lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

      if (brw->attribs.DepthStencil->stencil.write_mask[0] ||
	  (brw->attribs.DepthStencil->stencil.back_enabled &&
           brw->attribs.DepthStencil->stencil.write_mask[1]))
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
   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   key->projtex_mask = brw->wm.input_size_masks[4-1] >> (FRAG_ATTRIB_TEX0 - FRAG_ATTRIB_WPOS);
#endif

   /* _NEW_LIGHT */
   key->flat_shade = (brw->attribs.Raster->flatshade);

   /* _NEW_TEXTURE */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct pipe_sampler_state *unit = brw->attribs.Samplers[i];

      if (unit) {

	 if (unit->compare &&
             unit->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
	    key->shadowtex_mask |= 1<<i;
	 }
#if 0
	 if (t->Image[0][t->BaseLevel]->InternalFormat == GL_YCBCR_MESA)
	    key->yuvtex_mask |= 1<<i;
#endif
      }
   }


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
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = 0
   },
   .update = brw_upload_wm_prog
};

