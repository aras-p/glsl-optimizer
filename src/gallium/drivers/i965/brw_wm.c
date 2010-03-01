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
#include "tgsi/tgsi_info.h"

#include "brw_context.h"
#include "brw_screen.h"
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
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXD:
      /* sampler arg is held as a field in the instruction, not in an
       * actual register:
       */
      return tgsi_get_opcode_info(opcode)->num_src - 1;

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
static enum pipe_error do_wm_prog( struct brw_context *brw,
                                   struct brw_fragment_shader *fp, 
                                   struct brw_wm_prog_key *key,
                                   struct brw_winsys_buffer **bo_out)
{
   enum pipe_error ret;
   struct brw_wm_compile *c;
   const GLuint *program;
   GLuint program_size;

   if (brw->wm.compile_data == NULL) {
      brw->wm.compile_data = MALLOC(sizeof(*brw->wm.compile_data));
      if (!brw->wm.compile_data) 
         return PIPE_ERROR_OUT_OF_MEMORY;
   }

   c = brw->wm.compile_data;
   memset(c, 0, sizeof *c);

   c->key = *key;
   c->fp = fp;
   c->env_param = NULL; /*brw->intel.ctx.FragmentProgram.Parameters;*/

   brw_init_compile(brw, &c->func);

   /*
    * Shader which use GLSL features such as flow control are handled
    * differently from "simple" shaders.
    */
   if (fp->has_flow_control) {
      c->dispatch_width = 8;
      /* XXX: GLSL support
       */
      exit(1);
      /* brw_wm_branching_shader_emit(brw, c); */
   }
   else {
      c->dispatch_width = 16;
      brw_wm_linear_shader_emit(brw, c);
   }

   if (BRW_DEBUG & DEBUG_WM)
      debug_printf("\n");

   /* get the program
    */
   ret = brw_get_program(&c->func, &program, &program_size);
   if (ret)
      return ret;

   ret = brw_upload_cache( &brw->cache, BRW_WM_PROG,
                           &c->key, sizeof(c->key),
                           NULL, 0,
                           program, program_size,
                           &c->prog_data,
                           &brw->wm.prog_data,
                           bo_out );
   if (ret)
      return ret;

   return PIPE_OK;
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
      line_aa = (brw->curr.rast->templ.line_smooth ? 
                 AA_ALWAYS : AA_NEVER);
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
      const struct brw_texture *tex = brw_texture(brw->curr.texture[i]);
	 
      if (tex->base.format == PIPE_FORMAT_UYVY)
	 key->yuvtex_mask |= 1 << i;

      if (tex->base.format == PIPE_FORMAT_YUYV)
	 key->yuvtex_swap_mask |= 1 << i;

      /* XXX: shadow texture
       */
      /* key->shadowtex_mask |= 1<<i; */
   }

   /* CACHE_NEW_VS_PROG */
   key->vp_nr_outputs = brw->vs.prog_data->nr_outputs;

   key->nr_cbufs = brw->curr.fb.nr_cbufs;

   key->nr_inputs = brw->curr.fragment_shader->info.num_inputs;

   /* The unique fragment program ID */
   key->program_string_id = brw->curr.fragment_shader->id;
}


static enum pipe_error brw_prepare_wm_prog(struct brw_context *brw)
{
   struct brw_wm_prog_key key;
   struct brw_fragment_shader *fs = brw->curr.fragment_shader;
   enum pipe_error ret;
     
   brw_wm_populate_key(brw, &key);

   /* Make an early check for the key.
    */
   if (brw_search_cache(&brw->cache, BRW_WM_PROG,
                        &key, sizeof(key),
                        NULL, 0,
                        &brw->wm.prog_data,
                        &brw->wm.prog_bo))
      return PIPE_OK;

   ret = do_wm_prog(brw, fs, &key, &brw->wm.prog_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_FRAGMENT_SHADER |
		PIPE_NEW_DEPTH_STENCIL_ALPHA |
		PIPE_NEW_RAST |
		PIPE_NEW_NR_CBUFS |
		PIPE_NEW_BOUND_TEXTURES),
      .brw   = (BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .prepare = brw_prepare_wm_prog
};

