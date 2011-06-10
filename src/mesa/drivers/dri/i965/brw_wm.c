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
#include "main/formats.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "../glsl/ralloc.h"

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
   case OPCODE_DP2:
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
   c->prog_data.reg_blocks = brw_register_blocks(c->max_wm_grf);

   /* Emit GEN4 code.
    */
   brw_wm_emit(c);
}

void
brw_wm_payload_setup(struct brw_context *brw,
		     struct brw_wm_compile *c)
{
   struct intel_context *intel = &brw->intel;
   bool uses_depth = (c->fp->program.Base.InputsRead &
		      (1 << FRAG_ATTRIB_WPOS)) != 0;

   if (intel->gen >= 6) {
      /* R0-1: masks, pixel X/Y coordinates. */
      c->nr_payload_regs = 2;
      /* R2: only for 32-pixel dispatch.*/
      /* R3-4: perspective pixel location barycentric */
      c->nr_payload_regs += 2;
      /* R5-6: perspective pixel location bary for dispatch width != 8 */
      if (c->dispatch_width == 16) {
	 c->nr_payload_regs += 2;
      }
      /* R7-10: perspective centroid barycentric */
      /* R11-14: perspective sample barycentric */
      /* R15-18: linear pixel location barycentric */
      /* R19-22: linear centroid barycentric */
      /* R23-26: linear sample barycentric */

      /* R27: interpolated depth if uses source depth */
      if (uses_depth) {
	 c->source_depth_reg = c->nr_payload_regs;
	 c->nr_payload_regs++;
	 if (c->dispatch_width == 16) {
	    /* R28: interpolated depth if not 8-wide. */
	    c->nr_payload_regs++;
	 }
      }
      /* R29: interpolated W set if GEN6_WM_USES_SOURCE_W.
       */
      if (uses_depth) {
	 c->source_w_reg = c->nr_payload_regs;
	 c->nr_payload_regs++;
	 if (c->dispatch_width == 16) {
	    /* R30: interpolated W if not 8-wide. */
	    c->nr_payload_regs++;
	 }
      }
      /* R31: MSAA position offsets. */
      /* R32-: bary for 32-pixel. */
      /* R58-59: interp W for 32-pixel. */

      if (c->fp->program.Base.OutputsWritten &
	  BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
	 c->source_depth_to_render_target = GL_TRUE;
	 c->computes_depth = GL_TRUE;
      }
   } else {
      brw_wm_lookup_iz(intel, c);
   }
}

/**
 * All Mesa program -> GPU code generation goes through this function.
 * Depending on the instructions used (i.e. flow control instructions)
 * we'll use one of two code generators.
 */
bool do_wm_prog(struct brw_context *brw,
		struct gl_shader_program *prog,
		struct brw_fragment_program *fp,
		struct brw_wm_prog_key *key)
{
   struct intel_context *intel = &brw->intel;
   struct brw_wm_compile *c;
   const GLuint *program;
   GLuint program_size;

   c = brw->wm.compile_data;
   if (c == NULL) {
      brw->wm.compile_data = rzalloc(NULL, struct brw_wm_compile);
      c = brw->wm.compile_data;
      if (c == NULL) {
         /* Ouch - big out of memory problem.  Can't continue
          * without triggering a segfault, no way to signal,
          * so just return.
          */
         return false;
      }
      c->instruction = rzalloc_array(c, struct brw_wm_instruction, BRW_WM_MAX_INSN);
      c->prog_instructions = rzalloc_array(c, struct prog_instruction, BRW_WM_MAX_INSN);
      c->vreg = rzalloc_array(c, struct brw_wm_value, BRW_WM_MAX_VREG);
      c->refs = rzalloc_array(c, struct brw_wm_ref, BRW_WM_MAX_REF);
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

   brw_init_compile(brw, &c->func, c);

   if (prog && prog->FragmentProgram) {
      if (!brw_wm_fs_emit(brw, c, prog))
	 return false;
   } else {
      /* Fallback for fixed function and ARB_fp shaders. */
      c->dispatch_width = 16;
      brw_wm_payload_setup(brw, c);
      brw_wm_non_glsl_emit(brw, c);
      c->prog_data.dispatch_width = 16;
   }

   /* Scratch space is used for register spilling */
   if (c->last_scratch) {
      uint32_t total_scratch;

      /* Per-thread scratch space is power-of-two sized. */
      for (c->prog_data.total_scratch = 1024;
	   c->prog_data.total_scratch <= c->last_scratch;
	   c->prog_data.total_scratch *= 2) {
	 /* empty */
      }
      total_scratch = c->prog_data.total_scratch * brw->wm_max_threads;

      if (brw->wm.scratch_bo && total_scratch > brw->wm.scratch_bo->size) {
	 drm_intel_bo_unreference(brw->wm.scratch_bo);
	 brw->wm.scratch_bo = NULL;
      }
      if (brw->wm.scratch_bo == NULL) {
	 brw->wm.scratch_bo = drm_intel_bo_alloc(intel->bufmgr,
						 "wm scratch",
						 total_scratch,
						 4096);
      }
   }
   else {
      c->prog_data.total_scratch = 0;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      fprintf(stderr, "\n");

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   brw_upload_cache(&brw->cache, BRW_WM_PROG,
		    &c->key, sizeof(c->key),
		    program, program_size,
		    &c->prog_data, sizeof(c->prog_data),
		    &brw->wm.prog_offset, &brw->wm.prog_data);

   return true;
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp = 
      (struct brw_fragment_program *)brw->fragment_program;
   GLuint lookup = 0;
   GLuint line_aa;
   GLuint i;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   /* _NEW_COLOR */
   key->alpha_test = ctx->Color.AlphaEnabled;
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

   key->iz_lookup = lookup;
   key->line_aa = line_aa;
   key->stats_wm = brw->intel.stats_wm;

   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   key->proj_attrib_mask = brw->wm.input_size_masks[4-1];

   /* _NEW_LIGHT */
   key->flat_shade = (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_FRAG_CLAMP | _NEW_BUFFERS */
   key->clamp_fragment_color = ctx->Color._ClampFragmentColor;

   /* _NEW_TEXTURE */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[i];

      if (unit->_ReallyEnabled) {
         const struct gl_texture_object *t = unit->_Current;
         const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
	 struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, i);
	 int swizzles[SWIZZLE_NIL + 1] = {
	    SWIZZLE_X,
	    SWIZZLE_Y,
	    SWIZZLE_Z,
	    SWIZZLE_W,
	    SWIZZLE_ZERO,
	    SWIZZLE_ONE,
	    SWIZZLE_NIL
	 };

	 /* GL_DEPTH_TEXTURE_MODE is normally handled through
	  * brw_wm_surface_state, but it applies to shadow compares as
	  * well and our shadow compares always return the result in
	  * all 4 channels.
	  */
	 if (sampler->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
	    key->compare_funcs[i] = sampler->CompareFunc;

	    if (sampler->DepthMode == GL_ALPHA) {
	       swizzles[0] = SWIZZLE_ZERO;
	       swizzles[1] = SWIZZLE_ZERO;
	       swizzles[2] = SWIZZLE_ZERO;
	    } else if (sampler->DepthMode == GL_LUMINANCE) {
	       swizzles[3] = SWIZZLE_ONE;
	    } else if (sampler->DepthMode == GL_RED) {
	       /* See table 3.23 of the GL 3.0 spec. */
	       swizzles[1] = SWIZZLE_ZERO;
	       swizzles[2] = SWIZZLE_ZERO;
	       swizzles[3] = SWIZZLE_ONE;
	    }
	 }

	 if (img->InternalFormat == GL_YCBCR_MESA) {
	    key->yuvtex_mask |= 1 << i;
	    if (img->TexFormat == MESA_FORMAT_YCBCR)
		key->yuvtex_swap_mask |= 1 << i;
	 }

	 key->tex_swizzles[i] =
	    MAKE_SWIZZLE4(swizzles[GET_SWZ(t->_Swizzle, 0)],
			  swizzles[GET_SWZ(t->_Swizzle, 1)],
			  swizzles[GET_SWZ(t->_Swizzle, 2)],
			  swizzles[GET_SWZ(t->_Swizzle, 3)]);

	 if (sampler->MinFilter != GL_NEAREST &&
	     sampler->MagFilter != GL_NEAREST) {
	    if (sampler->WrapS == GL_CLAMP)
	       key->gl_clamp_mask[0] |= 1 << i;
	    if (sampler->WrapT == GL_CLAMP)
	       key->gl_clamp_mask[1] |= 1 << i;
	    if (sampler->WrapR == GL_CLAMP)
	       key->gl_clamp_mask[2] |= 1 << i;
	 }
      }
      else {
         key->tex_swizzles[i] = SWIZZLE_NOOP;
      }
   }

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
      key->render_to_fbo = ctx->DrawBuffer->Name != 0;
   }

   /* _NEW_BUFFERS */
   key->nr_color_regions = ctx->DrawBuffer->_NumColorDrawBuffers;

   /* CACHE_NEW_VS_PROG */
   key->vp_outputs_written = brw->vs.prog_data->outputs_written;

   /* The unique fragment program ID */
   key->program_string_id = fp->id;
}


static void brw_prepare_wm_prog(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->fragment_program;

   brw_wm_populate_key(brw, &key);

   if (!brw_search_cache(&brw->cache, BRW_WM_PROG,
			 &key, sizeof(key),
			 &brw->wm.prog_offset, &brw->wm.prog_data)) {
      bool success = do_wm_prog(brw, ctx->Shader.CurrentFragmentProgram, fp,
				&key);
      assert(success);
   }
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
		_NEW_DEPTH |
		_NEW_STENCIL |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_LIGHT |
		_NEW_FRAG_CLAMP |
		_NEW_BUFFERS |
		_NEW_TEXTURE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .prepare = brw_prepare_wm_prog
};

