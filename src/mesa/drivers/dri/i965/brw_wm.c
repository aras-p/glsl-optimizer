/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
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
  *   Keith Whitwell <keithw@vmware.com>
  */

#include "brw_context.h"
#include "brw_wm.h"
#include "brw_state.h"
#include "main/enums.h"
#include "main/formats.h"
#include "main/fbobject.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"
#include "program/program.h"
#include "intel_mipmap_tree.h"

#include "util/ralloc.h"

/**
 * Return a bitfield where bit n is set if barycentric interpolation mode n
 * (see enum brw_wm_barycentric_interp_mode) is needed by the fragment shader.
 */
static unsigned
brw_compute_barycentric_interp_modes(struct brw_context *brw,
                                     bool shade_model_flat,
                                     bool persample_shading,
                                     const struct gl_fragment_program *fprog)
{
   unsigned barycentric_interp_modes = 0;
   int attr;

   /* Loop through all fragment shader inputs to figure out what interpolation
    * modes are in use, and set the appropriate bits in
    * barycentric_interp_modes.
    */
   for (attr = 0; attr < VARYING_SLOT_MAX; ++attr) {
      enum glsl_interp_qualifier interp_qualifier =
         fprog->InterpQualifier[attr];
      bool is_centroid = (fprog->IsCentroid & BITFIELD64_BIT(attr)) &&
         !persample_shading;
      bool is_sample = (fprog->IsSample & BITFIELD64_BIT(attr)) ||
         persample_shading;
      bool is_gl_Color = attr == VARYING_SLOT_COL0 || attr == VARYING_SLOT_COL1;

      /* Ignore unused inputs. */
      if (!(fprog->Base.InputsRead & BITFIELD64_BIT(attr)))
         continue;

      /* Ignore WPOS and FACE, because they don't require interpolation. */
      if (attr == VARYING_SLOT_POS || attr == VARYING_SLOT_FACE)
         continue;

      /* Determine the set (or sets) of barycentric coordinates needed to
       * interpolate this variable.  Note that when
       * brw->needs_unlit_centroid_workaround is set, centroid interpolation
       * uses PIXEL interpolation for unlit pixels and CENTROID interpolation
       * for lit pixels, so we need both sets of barycentric coordinates.
       */
      if (interp_qualifier == INTERP_QUALIFIER_NOPERSPECTIVE) {
         if (is_centroid) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC;
         } else if (is_sample) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC;
         }
         if ((!is_centroid && !is_sample) ||
             brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      } else if (interp_qualifier == INTERP_QUALIFIER_SMOOTH ||
                 (!(shade_model_flat && is_gl_Color) &&
                  interp_qualifier == INTERP_QUALIFIER_NONE)) {
         if (is_centroid) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
         } else if (is_sample) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC;
         }
         if ((!is_centroid && !is_sample) ||
             brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      }
   }

   return barycentric_interp_modes;
}

bool
brw_wm_prog_data_compare(const void *in_a, const void *in_b)
{
   const struct brw_wm_prog_data *a = in_a;
   const struct brw_wm_prog_data *b = in_b;

   /* Compare the base structure. */
   if (!brw_stage_prog_data_compare(&a->base, &b->base))
      return false;

   /* Compare the rest of the structure. */
   const unsigned offset = sizeof(struct brw_stage_prog_data);
   if (memcmp(((char *) a) + offset, ((char *) b) + offset,
              sizeof(struct brw_wm_prog_data) - offset))
      return false;

   return true;
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
   struct gl_context *ctx = &brw->ctx;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_wm_prog_data prog_data;
   const GLuint *program;
   struct gl_shader *fs = NULL;
   GLuint program_size;

   if (prog)
      fs = prog->_LinkedShaders[MESA_SHADER_FRAGMENT];

   memset(&prog_data, 0, sizeof(prog_data));
   prog_data.uses_kill = fp->program.UsesKill;

   /* Allocate the references to the uniforms that will end up in the
    * prog_data associated with the compiled program, and which will be freed
    * by the state cache.
    */
   int param_count;
   if (fs) {
      param_count = fs->num_uniform_components;
   } else {
      param_count = fp->program.Base.Parameters->NumParameters * 4;
   }
   /* The backend also sometimes adds params for texture size. */
   param_count += 2 * ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits;
   prog_data.base.param =
      rzalloc_array(NULL, const gl_constant_value *, param_count);
   prog_data.base.pull_param =
      rzalloc_array(NULL, const gl_constant_value *, param_count);
   prog_data.base.nr_params = param_count;

   prog_data.barycentric_interp_modes =
      brw_compute_barycentric_interp_modes(brw, key->flat_shade,
                                           key->persample_shading,
                                           &fp->program);

   program = brw_wm_fs_emit(brw, mem_ctx, key, &prog_data,
                            &fp->program, prog, &program_size);
   if (program == NULL) {
      ralloc_free(mem_ctx);
      return false;
   }

   if (prog_data.base.total_scratch) {
      brw_get_scratch_bo(brw, &brw->wm.base.scratch_bo,
			 prog_data.base.total_scratch * brw->max_wm_threads);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      fprintf(stderr, "\n");

   brw_upload_cache(&brw->cache, BRW_WM_PROG,
		    key, sizeof(struct brw_wm_prog_key),
		    program, program_size,
		    &prog_data, sizeof(prog_data),
		    &brw->wm.base.prog_offset, &brw->wm.prog_data);

   ralloc_free(mem_ctx);

   return true;
}

static bool
key_debug(struct brw_context *brw, const char *name, int a, int b)
{
   if (a != b) {
      perf_debug("  %s %d->%d\n", name, a, b);
      return true;
   } else {
      return false;
   }
}

bool
brw_debug_recompile_sampler_key(struct brw_context *brw,
                                const struct brw_sampler_prog_key_data *old_key,
                                const struct brw_sampler_prog_key_data *key)
{
   bool found = false;

   for (unsigned int i = 0; i < MAX_SAMPLERS; i++) {
      found |= key_debug(brw, "EXT_texture_swizzle or DEPTH_TEXTURE_MODE",
                         old_key->swizzles[i], key->swizzles[i]);
   }
   found |= key_debug(brw, "GL_CLAMP enabled on any texture unit's 1st coordinate",
                      old_key->gl_clamp_mask[0], key->gl_clamp_mask[0]);
   found |= key_debug(brw, "GL_CLAMP enabled on any texture unit's 2nd coordinate",
                      old_key->gl_clamp_mask[1], key->gl_clamp_mask[1]);
   found |= key_debug(brw, "GL_CLAMP enabled on any texture unit's 3rd coordinate",
                      old_key->gl_clamp_mask[2], key->gl_clamp_mask[2]);
   found |= key_debug(brw, "gather channel quirk on any texture unit",
                      old_key->gather_channel_quirk_mask, key->gather_channel_quirk_mask);

   return found;
}

void
brw_wm_debug_recompile(struct brw_context *brw,
                       struct gl_shader_program *prog,
                       const struct brw_wm_prog_key *key)
{
   struct brw_cache_item *c = NULL;
   const struct brw_wm_prog_key *old_key = NULL;
   bool found = false;

   perf_debug("Recompiling fragment shader for program %d\n", prog->Name);

   for (unsigned int i = 0; i < brw->cache.size; i++) {
      for (c = brw->cache.items[i]; c; c = c->next) {
         if (c->cache_id == BRW_WM_PROG) {
            old_key = c->key;

            if (old_key->program_string_id == key->program_string_id)
               break;
         }
      }
      if (c)
         break;
   }

   if (!c) {
      perf_debug("  Didn't find previous compile in the shader cache for debug\n");
      return;
   }

   found |= key_debug(brw, "alphatest, computed depth, depth test, or "
                      "depth write",
                      old_key->iz_lookup, key->iz_lookup);
   found |= key_debug(brw, "depth statistics",
                      old_key->stats_wm, key->stats_wm);
   found |= key_debug(brw, "flat shading",
                      old_key->flat_shade, key->flat_shade);
   found |= key_debug(brw, "per-sample shading",
                      old_key->persample_shading, key->persample_shading);
   found |= key_debug(brw, "per-sample shading and 2x MSAA",
                      old_key->persample_2x, key->persample_2x);
   found |= key_debug(brw, "number of color buffers",
                      old_key->nr_color_regions, key->nr_color_regions);
   found |= key_debug(brw, "MRT alpha test or alpha-to-coverage",
                      old_key->replicate_alpha, key->replicate_alpha);
   found |= key_debug(brw, "rendering to FBO",
                      old_key->render_to_fbo, key->render_to_fbo);
   found |= key_debug(brw, "fragment color clamping",
                      old_key->clamp_fragment_color, key->clamp_fragment_color);
   found |= key_debug(brw, "line smoothing",
                      old_key->line_aa, key->line_aa);
   found |= key_debug(brw, "renderbuffer height",
                      old_key->drawable_height, key->drawable_height);
   found |= key_debug(brw, "input slots valid",
                      old_key->input_slots_valid, key->input_slots_valid);
   found |= key_debug(brw, "mrt alpha test function",
                      old_key->alpha_test_func, key->alpha_test_func);
   found |= key_debug(brw, "mrt alpha test reference value",
                      old_key->alpha_test_ref, key->alpha_test_ref);

   found |= brw_debug_recompile_sampler_key(brw, &old_key->tex, &key->tex);

   if (!found) {
      perf_debug("  Something else\n");
   }
}

static uint8_t
gen6_gather_workaround(GLenum internalformat)
{
   switch (internalformat) {
      case GL_R8I: return WA_SIGN | WA_8BIT;
      case GL_R8UI: return WA_8BIT;
      case GL_R16I: return WA_SIGN | WA_16BIT;
      case GL_R16UI: return WA_16BIT;
      /* note that even though GL_R32I and GL_R32UI have format overrides
       * in the surface state, there is no shader w/a required */
      default: return 0;
   }
}

void
brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				   const struct gl_program *prog,
                                   unsigned sampler_count,
				   struct brw_sampler_prog_key_data *key)
{
   struct brw_context *brw = brw_context(ctx);

   for (int s = 0; s < sampler_count; s++) {
      key->swizzles[s] = SWIZZLE_NOOP;

      if (!(prog->SamplersUsed & (1 << s)))
	 continue;

      int unit_id = prog->SamplerUnits[s];
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[unit_id];

      if (unit->_Current && unit->_Current->Target != GL_TEXTURE_BUFFER) {
	 const struct gl_texture_object *t = unit->_Current;
	 const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
	 struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit_id);

         const bool alpha_depth = t->DepthMode == GL_ALPHA &&
            (img->_BaseFormat == GL_DEPTH_COMPONENT ||
             img->_BaseFormat == GL_DEPTH_STENCIL);

         /* Haswell handles texture swizzling as surface format overrides
          * (except for GL_ALPHA); all other platforms need MOVs in the shader.
          */
         if (alpha_depth || (brw->gen < 8 && !brw->is_haswell))
            key->swizzles[s] = brw_get_texture_swizzle(ctx, t);

	 if (brw->gen < 8 &&
             sampler->MinFilter != GL_NEAREST &&
	     sampler->MagFilter != GL_NEAREST) {
	    if (sampler->WrapS == GL_CLAMP)
	       key->gl_clamp_mask[0] |= 1 << s;
	    if (sampler->WrapT == GL_CLAMP)
	       key->gl_clamp_mask[1] |= 1 << s;
	    if (sampler->WrapR == GL_CLAMP)
	       key->gl_clamp_mask[2] |= 1 << s;
	 }

         /* gather4's channel select for green from RG32F is broken;
          * requires a shader w/a on IVB; fixable with just SCS on HSW. */
         if (brw->gen == 7 && !brw->is_haswell && prog->UsesGather) {
            if (img->InternalFormat == GL_RG32F)
               key->gather_channel_quirk_mask |= 1 << s;
         }

         /* Gen6's gather4 is broken for UINT/SINT; we treat them as
          * UNORM/FLOAT instead and fix it in the shader.
          */
         if (brw->gen == 6 && prog->UsesGather) {
            key->gen6_gather_wa[s] = gen6_gather_workaround(img->InternalFormat);
         }

         /* If this is a multisample sampler, and uses the CMS MSAA layout,
          * then we need to emit slightly different code to first sample the
          * MCS surface.
          */
         struct intel_texture_object *intel_tex =
            intel_texture_object((struct gl_texture_object *)t);

         if (brw->gen >= 7 &&
             intel_tex->mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
            key->compressed_multisample_layout_mask |= 1 << s;
         }
      }
   }
}

static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp =
      (struct brw_fragment_program *)brw->fragment_program;
   const struct gl_program *prog = (struct gl_program *) brw->fragment_program;
   GLuint lookup = 0;
   GLuint line_aa;
   bool program_uses_dfdy = fp->program.UsesDFdy;
   bool multisample_fbo = ctx->DrawBuffer->Visual.samples > 1;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   if (brw->gen < 6) {
      /* _NEW_COLOR */
      if (fp->program.UsesKill || ctx->Color.AlphaEnabled)
	 lookup |= IZ_PS_KILL_ALPHATEST_BIT;

      if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
	 lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

      /* _NEW_DEPTH */
      if (ctx->Depth.Test)
	 lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

      if (ctx->Depth.Test && ctx->Depth.Mask) /* ?? */
	 lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

      /* _NEW_STENCIL | _NEW_BUFFERS */
      if (ctx->Stencil._Enabled) {
	 lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

	 if (ctx->Stencil.WriteMask[0] ||
	     ctx->Stencil.WriteMask[ctx->Stencil._BackFace])
	    lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;
      }
      key->iz_lookup = lookup;
   }

   line_aa = AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (ctx->Line.SmoothFlag) {
      if (brw->reduced_primitive == GL_LINES) {
	 line_aa = AA_ALWAYS;
      }
      else if (brw->reduced_primitive == GL_TRIANGLES) {
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

   key->line_aa = line_aa;

   /* _NEW_HINT */
   if (brw->disable_derivative_optimization) {
      key->high_quality_derivatives =
         ctx->Hint.FragmentShaderDerivative != GL_FASTEST;
   } else {
      key->high_quality_derivatives =
         ctx->Hint.FragmentShaderDerivative == GL_NICEST;
   }

   if (brw->gen < 6)
      key->stats_wm = brw->stats_wm;

   /* _NEW_LIGHT */
   key->flat_shade = (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_FRAG_CLAMP | _NEW_BUFFERS */
   key->clamp_fragment_color = ctx->Color._ClampFragmentColor;

   /* _NEW_TEXTURE */
   brw_populate_sampler_prog_key_data(ctx, prog, brw->wm.base.sampler_count,
                                      &key->tex);

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
   if (fp->program.Base.InputsRead & VARYING_BIT_POS) {
      key->drawable_height = ctx->DrawBuffer->Height;
   }

   if ((fp->program.Base.InputsRead & VARYING_BIT_POS) || program_uses_dfdy) {
      key->render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   }

   /* _NEW_BUFFERS */
   key->nr_color_regions = ctx->DrawBuffer->_NumColorDrawBuffers;

   /* _NEW_MULTISAMPLE, _NEW_COLOR, _NEW_BUFFERS */
   key->replicate_alpha = ctx->DrawBuffer->_NumColorDrawBuffers > 1 &&
      (ctx->Multisample.SampleAlphaToCoverage || ctx->Color.AlphaEnabled);

   /* _NEW_BUFFERS _NEW_MULTISAMPLE */
   /* Ignore sample qualifier while computing this flag. */
   key->persample_shading =
      _mesa_get_min_invocations_per_fragment(ctx, &fp->program, true) > 1;
   if (key->persample_shading)
      key->persample_2x = ctx->DrawBuffer->Visual.samples == 2;

   key->compute_pos_offset =
      _mesa_get_min_invocations_per_fragment(ctx, &fp->program, false) > 1 &&
      fp->program.Base.SystemValuesRead & SYSTEM_BIT_SAMPLE_POS;

   key->compute_sample_id =
      multisample_fbo &&
      ctx->Multisample.Enabled &&
      (fp->program.Base.SystemValuesRead & SYSTEM_BIT_SAMPLE_ID);

   /* BRW_NEW_VUE_MAP_GEOM_OUT */
   if (brw->gen < 6 || _mesa_bitcount_64(fp->program.Base.InputsRead &
                                         BRW_FS_VARYING_INPUT_MASK) > 16)
      key->input_slots_valid = brw->vue_map_geom_out.slots_valid;


   /* _NEW_COLOR | _NEW_BUFFERS */
   /* Pre-gen6, the hardware alpha test always used each render
    * target's alpha to do alpha test, as opposed to render target 0's alpha
    * like GL requires.  Fix that by building the alpha test into the
    * shader, and we'll skip enabling the fixed function alpha test.
    */
   if (brw->gen < 6 && ctx->DrawBuffer->_NumColorDrawBuffers > 1 && ctx->Color.AlphaEnabled) {
      key->alpha_test_func = ctx->Color.AlphaFunc;
      key->alpha_test_ref = ctx->Color.AlphaRef;
   }

   /* The unique fragment program ID */
   key->program_string_id = fp->id;
}


static void
brw_upload_wm_prog(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->fragment_program;

   brw_wm_populate_key(brw, &key);

   if (!brw_search_cache(&brw->cache, BRW_WM_PROG,
			 &key, sizeof(key),
			 &brw->wm.base.prog_offset, &brw->wm.prog_data)) {
      bool success = do_wm_prog(brw, ctx->_Shader->_CurrentFragmentProgram, fp,
				&key);
      (void) success;
      assert(success);
   }
   brw->wm.base.prog_data = &brw->wm.prog_data->base;
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
		_NEW_DEPTH |
		_NEW_STENCIL |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_HINT |
		_NEW_LIGHT |
		_NEW_FRAG_CLAMP |
		_NEW_BUFFERS |
		_NEW_TEXTURE |
		_NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_REDUCED_PRIMITIVE |
                BRW_NEW_VUE_MAP_GEOM_OUT |
                BRW_NEW_STATS_WM)
   },
   .emit = brw_upload_wm_prog
};

