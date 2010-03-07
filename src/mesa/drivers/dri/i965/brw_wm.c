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
#include "brw_wm.h"
#include "brw_state.h"


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
      return _mesa_num_inst_src_regs(opcode);
   }
}


GLuint brw_wm_is_scalar_result( GLuint opcode )
{
   switch (opcode) {
   case OPCODE_COS:
   case OPCODE_EX2:
   case OPCODE_LG2:
   case OPCODE_POW:
   case OPCODE_RCP:
   case OPCODE_RSQ:
   case OPCODE_SIN:
   case OPCODE_DP3:
   case OPCODE_DP4:
   case OPCODE_DPH:
   case OPCODE_DST:
      return 1;
      
   default:
      return 0;
   }
}


/**
 * Do GPU code generation for non-GLSL shader.  non-GLSL shaders have
 * no flow control instructions so we can more readily do SSA-style
 * optimizations.
 */
static void
brw_wm_non_glsl_emit(struct brw_context *brw, struct brw_wm_compile *c)
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
static void do_wm_prog( struct brw_context *brw,
			struct brw_fragment_program *fp, 
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
         return;
      }
      c->instruction = calloc(1, BRW_WM_MAX_INSN * sizeof(*c->instruction));
      c->prog_instructions = calloc(1, BRW_WM_MAX_INSN *
					  sizeof(*c->prog_instructions));
      c->vreg = calloc(1, BRW_WM_MAX_VREG * sizeof(*c->vreg));
      c->refs = calloc(1, BRW_WM_MAX_REF * sizeof(*c->refs));
   } else {
      void *instruction = c->instruction;
      void *prog_instructions = c->prog_instructions;
      void *vreg = c->vreg;
      void *refs = c->refs;
      memset(c, 0, sizeof(*brw->wm.compile_data));
      c->instruction = instruction;
      c->prog_instructions = prog_instructions;
      c->vreg = vreg;
      c->refs = refs;
   }
   memcpy(&c->key, key, sizeof(*key));

   c->fp = fp;
   c->env_param = brw->intel.ctx.FragmentProgram.Parameters;

   brw_init_compile(brw, &c->func);

   /* temporary sanity check assertion */
   ASSERT(fp->isGLSL == brw_wm_is_glsl(&c->fp->program));

   /*
    * Shader which use GLSL features such as flow control are handled
    * differently from "simple" shaders.
    */
   if (fp->isGLSL) {
      c->dispatch_width = 8;
      brw_wm_glsl_emit(brw, c);
   }
   else {
      c->dispatch_width = 16;
      brw_wm_non_glsl_emit(brw, c);
   }

   if (INTEL_DEBUG & DEBUG_WM)
      fprintf(stderr, "\n");

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   dri_bo_unreference(brw->wm.prog_bo);
   brw->wm.prog_bo = brw_upload_cache_with_auxdata(&brw->cache, BRW_WM_PROG,
						   &c->key, sizeof(c->key),
						   NULL, 0,
						   program, program_size,
						   &c->prog_data,
						   sizeof(c->prog_data),
						   &brw->wm.prog_data);
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   GLcontext *ctx = &brw->intel.ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp = 
      (struct brw_fragment_program *)brw->fragment_program;
   GLboolean uses_depth = (fp->program.Base.InputsRead & (1 << FRAG_ATTRIB_WPOS)) != 0;
   GLuint lookup = 0;
   GLuint line_aa;
   GLuint i;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   /* _NEW_COLOR */
   if (fp->program.UsesKill ||
       ctx->Color.AlphaEnabled)
      lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
      lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   /* _NEW_DEPTH */
   if (ctx->Depth.Test)
      lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

   if (ctx->Depth.Test &&  
       ctx->Depth.Mask) /* ?? */
      lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   /* _NEW_STENCIL */
   if (ctx->Stencil._Enabled) {
      lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

      if (ctx->Stencil.WriteMask[0] ||
	  ctx->Stencil.WriteMask[ctx->Stencil._BackFace])
	 lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;
   }

   line_aa = AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (ctx->Line.SmoothFlag) {
      if (brw->intel.reduced_primitive == GL_LINES) {
	 line_aa = AA_ALWAYS;
      }
      else if (brw->intel.reduced_primitive == GL_TRIANGLES) {
	 if (ctx->Polygon.FrontMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if (ctx->Polygon.BackMode == GL_LINE ||
		(ctx->Polygon.CullFlag &&
		 ctx->Polygon.CullFaceMode == GL_BACK))
	       line_aa = AA_ALWAYS;
	 }
	 else if (ctx->Polygon.BackMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if ((ctx->Polygon.CullFlag &&
		 ctx->Polygon.CullFaceMode == GL_FRONT))
	       line_aa = AA_ALWAYS;
	 }
      }
   }
	 
   brw_wm_lookup_iz(line_aa,
		    lookup,
		    uses_depth,
		    key);


   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   key->proj_attrib_mask = brw->wm.input_size_masks[4-1];

   /* _NEW_LIGHT */
   key->flat_shade = (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_HINT */
   key->linear_color = (ctx->Hint.PerspectiveCorrection == GL_FASTEST);

   /* _NEW_TEXTURE */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[i];

      if (unit->_ReallyEnabled) {
         const struct gl_texture_object *t = unit->_Current;
         const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
	 if (img->InternalFormat == GL_YCBCR_MESA) {
	    key->yuvtex_mask |= 1 << i;
	    if (img->TexFormat == MESA_FORMAT_YCBCR)
		key->yuvtex_swap_mask |= 1 << i;
	 }

         key->tex_swizzles[i] = t->_Swizzle;
      }
      else {
         key->tex_swizzles[i] = SWIZZLE_NOOP;
      }
   }

   /* Shadow */
   key->shadowtex_mask = fp->program.Base.ShadowSamplers;

   /* _NEW_BUFFERS */
   /*
    * Include the draw buffer origin and height so that we can calculate
    * fragment position values relative to the bottom left of the drawable,
    * from the incoming screen origin relative position we get as part of our
    * payload.
    *
    * This is only needed for the WM_WPOSXY opcode when the fragment program
    * uses the gl_FragCoord input.
    *
    * We could avoid recompiling by including this as a constant referenced by
    * our program, but if we were to do that it would also be nice to handle
    * getting that constant updated at batchbuffer submit time (when we
    * hold the lock and know where the buffer really is) rather than at emit
    * time when we don't hold the lock and are just guessing.  We could also
    * just avoid using this as key data if the program doesn't use
    * fragment.position.
    *
    * For DRI2 the origin_x/y will always be (0,0) but we still need the
    * drawable height in order to invert the Y axis.
    */
   if (fp->program.Base.InputsRead & FRAG_BIT_WPOS) {
      key->drawable_height = ctx->DrawBuffer->Height;
   }

   key->nr_color_regions = brw->state.nr_color_regions;

   /* CACHE_NEW_VS_PROG */
   key->vp_outputs_written = brw->vs.prog_data->outputs_written;

   /* The unique fragment program ID */
   key->program_string_id = fp->id;
}


static void brw_prepare_wm_prog(struct brw_context *brw)
{
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->fragment_program;
     
   brw_wm_populate_key(brw, &key);

   /* Make an early check for the key.
    */
   dri_bo_unreference(brw->wm.prog_bo);
   brw->wm.prog_bo = brw_search_cache(&brw->cache, BRW_WM_PROG,
				      &key, sizeof(key),
				      NULL, 0,
				      &brw->wm.prog_data);
   if (brw->wm.prog_bo == NULL)
      do_wm_prog(brw, fp, &key);
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
		_NEW_DEPTH |
                _NEW_HINT |
		_NEW_STENCIL |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_LIGHT |
		_NEW_BUFFERS |
		_NEW_TEXTURE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .prepare = brw_prepare_wm_prog
};

