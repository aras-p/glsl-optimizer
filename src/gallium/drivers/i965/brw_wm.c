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
#include "pipe/p_error.h"

#include "tgsi/tgsi_info.h"

#include "brw_context.h"
#include "brw_screen.h"
#include "brw_util.h"
#include "brw_wm.h"
#include "brw_state.h"
#include "brw_debug.h"
#include "brw_pipe_rast.h"


/** Return number of src args for given instruction */
GLuint brw_wm_nr_args( GLuint opcode )
{
   switch (opcode) {
   case WM_FRONTFACING:
   case WM_PIXELXY:
      return 0;
   case WM_CINTERP:
   case WM_WPOSXY:
   case WM_DELTAXY:
      return 1;
   case WM_LINTERP:
   case WM_PIXELW:
      return 2;
   case WM_FB_WRITE:
   case WM_PINTERP:
      return 3;
   default:
      assert(opcode < MAX_OPCODE);
      return tgsi_get_opcode_info(opcode)->num_src;
   }
}


GLuint brw_wm_is_scalar_result( GLuint opcode )
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


/**
 * Do GPU code generation for shaders without flow control.  Shaders
 * without flow control instructions can more readily be analysed for
 * SSA-style optimizations.
 */
static void
brw_wm_linear_shader_emit(struct brw_context *brw, struct brw_wm_compile *c)
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
    * Divide by two because we operate on 16 pixels at a time and require
    * two GRF entries for each logical shader register.
    */
   c->grf_limit = BRW_WM_MAX_GRF / 2;

   brw_wm_pass2(c);

   /* how many general-purpose registers are used */
   c->prog_data.total_grf = c->max_wm_grf;

   /* Scratch space is used for register spilling */
   if (c->last_scratch) {
      c->prog_data.total_scratch = c->last_scratch + 0x40;
   }
   else {
      c->prog_data.total_scratch = 0;
   }

   /* Emit GEN4 code.
    */
   brw_wm_emit(c);
}


/**
 * All Mesa program -> GPU code generation goes through this function.
 * Depending on the instructions used (i.e. flow control instructions)
 * we'll use one of two code generators.
 */
static int do_wm_prog( struct brw_context *brw,
			struct brw_fragment_shader *fp, 
			struct brw_wm_prog_key *key)
{
   struct brw_wm_compile *c;
   const GLuint *program;
   GLuint program_size;

   c = brw->wm.compile_data;
   if (c == NULL) {
      brw->wm.compile_data = calloc(1, sizeof(*brw->wm.compile_data));
      c = brw->wm.compile_data;
      if (c == NULL) {
         /* Ouch - big out of memory problem.  Can't continue
          * without triggering a segfault, no way to signal,
          * so just return.
          */
         return PIPE_ERROR_OUT_OF_MEMORY;
      }
   } else {
      memset(c, 0, sizeof(*brw->wm.compile_data));
   }
   memcpy(&c->key, key, sizeof(*key));

   c->fp = fp;
   c->env_param = NULL; /*brw->intel.ctx.FragmentProgram.Parameters;*/

   brw_init_compile(brw, &c->func);

   /* temporary sanity check assertion */
   assert(fp->has_flow_control == brw_wm_has_flow_control(c->fp));

   /*
    * Shader which use GLSL features such as flow control are handled
    * differently from "simple" shaders.
    */
   if (fp->has_flow_control) {
      c->dispatch_width = 8;
      /* XXX: GLSL support
       */
      exit(1);
      //brw_wm_branching_shader_emit(brw, c);
   }
   else {
      c->dispatch_width = 16;
      brw_wm_linear_shader_emit(brw, c);
   }

   if (BRW_DEBUG & DEBUG_WM)
      debug_printf("\n");

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   brw->sws->bo_unreference(brw->wm.prog_bo);
   brw->wm.prog_bo = brw_upload_cache( &brw->cache, BRW_WM_PROG,
				       &c->key, sizeof(c->key),
				       NULL, 0,
				       program, program_size,
				       &c->prog_data,
				       &brw->wm.prog_data );

   return 0;
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   unsigned lookup, line_aa;
   unsigned i;

   memset(key, 0, sizeof(*key));

   /* PIPE_NEW_FRAGMENT_SHADER
    * PIPE_NEW_DEPTH_STENCIL_ALPHA
    */
   lookup = (brw->curr.zstencil->iz_lookup |
	     brw->curr.fragment_shader->iz_lookup);


   /* PIPE_NEW_RAST
    * BRW_NEW_REDUCED_PRIMITIVE 
    */
   switch (brw->reduced_primitive) {
   case PIPE_PRIM_POINTS:
      line_aa = AA_NEVER;
      break;
   case PIPE_PRIM_LINES:
      line_aa = AA_ALWAYS;
      break;
   default:
      line_aa = brw->curr.rast->unfilled_aa_line;
      break;
   }
	 
   brw_wm_lookup_iz(line_aa,
		    lookup,
		    brw->curr.fragment_shader->uses_depth,
		    key);

   /* PIPE_NEW_RAST */
   key->flat_shade = brw->curr.rast->templ.flatshade;


   /* PIPE_NEW_BOUND_TEXTURES */
   for (i = 0; i < brw->curr.num_textures; i++) {
      const struct brw_texture *tex = brw->curr.texture[i];
	 
      if (tex->base.format == PIPE_FORMAT_YCBCR)
	 key->yuvtex_mask |= 1 << i;

      if (tex->base.format == PIPE_FORMAT_YCBCR_REV)
	 key->yuvtex_swap_mask |= 1 << i;

      /* XXX: shadow texture
       */
      /* key->shadowtex_mask |= 1<<i; */
   }

   /* CACHE_NEW_VS_PROG */
   key->vp_nr_outputs = brw->vs.prog_data->nr_outputs;

   /* The unique fragment program ID */
   key->program_string_id = brw->curr.fragment_shader->id;
}


static int brw_prepare_wm_prog(struct brw_context *brw)
{
   struct brw_wm_prog_key key;
   struct brw_fragment_shader *fs = brw->curr.fragment_shader;
     
   brw_wm_populate_key(brw, &key);

   /* Make an early check for the key.
    */
   brw->sws->bo_unreference(brw->wm.prog_bo);
   brw->wm.prog_bo = brw_search_cache(&brw->cache, BRW_WM_PROG,
				      &key, sizeof(key),
				      NULL, 0,
				      &brw->wm.prog_data);
   if (brw->wm.prog_bo == NULL)
      return do_wm_prog(brw, fs, &key);

   return 0;
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_FRAGMENT_SHADER |
		PIPE_NEW_DEPTH_STENCIL_ALPHA |
		PIPE_NEW_RAST |
		PIPE_NEW_BOUND_TEXTURES),
      .brw   = (BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .prepare = brw_prepare_wm_prog
};

