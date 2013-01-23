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
#include "main/fbobject.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "glsl/ralloc.h"

/**
 * Return a bitfield where bit n is set if barycentric interpolation mode n
 * (see enum brw_wm_barycentric_interp_mode) is needed by the fragment shader.
 */
static unsigned
brw_compute_barycentric_interp_modes(struct brw_context *brw,
                                     bool shade_model_flat,
                                     const struct gl_fragment_program *fprog)
{
   unsigned barycentric_interp_modes = 0;
   int attr;

   /* Loop through all fragment shader inputs to figure out what interpolation
    * modes are in use, and set the appropriate bits in
    * barycentric_interp_modes.
    */
   for (attr = 0; attr < FRAG_ATTRIB_MAX; ++attr) {
      enum glsl_interp_qualifier interp_qualifier =
         fprog->InterpQualifier[attr];
      bool is_centroid = fprog->IsCentroid & BITFIELD64_BIT(attr);
      bool is_gl_Color = attr == FRAG_ATTRIB_COL0 || attr == FRAG_ATTRIB_COL1;

      /* Ignore unused inputs. */
      if (!(fprog->Base.InputsRead & BITFIELD64_BIT(attr)))
         continue;

      /* Ignore WPOS and FACE, because they don't require interpolation. */
      if (attr == FRAG_ATTRIB_WPOS || attr == FRAG_ATTRIB_FACE)
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
         }
         if (!is_centroid || brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      } else if (interp_qualifier == INTERP_QUALIFIER_SMOOTH ||
                 (!(shade_model_flat && is_gl_Color) &&
                  interp_qualifier == INTERP_QUALIFIER_NONE)) {
         if (is_centroid) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
         }
         if (!is_centroid || brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      }
   }

   return barycentric_interp_modes;
}

bool
brw_wm_prog_data_compare(const void *in_a, const void *in_b,
                         int aux_size, const void *in_key)
{
   const struct brw_wm_prog_data *a = in_a;
   const struct brw_wm_prog_data *b = in_b;

   /* Compare all the struct up to the pointers. */
   if (memcmp(a, b, offsetof(struct brw_wm_prog_data, param)))
      return false;

   if (memcmp(a->param, b->param, a->nr_params * sizeof(void *)))
      return false;

   if (memcmp(a->pull_param, b->pull_param, a->nr_pull_params * sizeof(void *)))
      return false;

   return true;
}

void
brw_wm_prog_data_free(const void *in_prog_data)
{
   const struct brw_wm_prog_data *prog_data = in_prog_data;

   ralloc_free((void *)prog_data->param);
   ralloc_free((void *)prog_data->pull_param);
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
   struct gl_shader *fs = NULL;
   GLuint program_size;

   if (prog)
      fs = prog->_LinkedShaders[MESA_SHADER_FRAGMENT];

   c = rzalloc(NULL, struct brw_wm_compile);

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
   param_count += 2 * BRW_MAX_TEX_UNIT;
   c->prog_data.param = rzalloc_array(NULL, const float *, param_count);
   c->prog_data.pull_param = rzalloc_array(NULL, const float *, param_count);

   memcpy(&c->key, key, sizeof(*key));

   c->prog_data.barycentric_interp_modes =
      brw_compute_barycentric_interp_modes(brw, c->key.flat_shade,
                                           &fp->program);

   program = brw_wm_fs_emit(brw, c, &fp->program, prog, &program_size);
   if (program == NULL)
      return false;

   /* Scratch space is used for register spilling */
   if (c->last_scratch) {
      perf_debug("Fragment shader triggered register spilling.  "
                 "Try reducing the number of live scalar values to "
                 "improve performance.\n");

      c->prog_data.total_scratch = brw_get_scratch_size(c->last_scratch);

      brw_get_scratch_bo(intel, &brw->wm.scratch_bo,
			 c->prog_data.total_scratch * brw->max_wm_threads);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      fprintf(stderr, "\n");

   brw_upload_cache(&brw->cache, BRW_WM_PROG,
		    &c->key, sizeof(c->key),
		    program, program_size,
		    &c->prog_data, sizeof(c->prog_data),
		    &brw->wm.prog_offset, &brw->wm.prog_data);

   ralloc_free(c);

   return true;
}

static bool
key_debug(const char *name, int a, int b)
{
   if (a != b) {
      perf_debug("  %s %d->%d\n", name, a, b);
      return true;
   } else {
      return false;
   }
}

bool
brw_debug_recompile_sampler_key(const struct brw_sampler_prog_key_data *old_key,
                                const struct brw_sampler_prog_key_data *key)
{
   bool found = false;

   for (unsigned int i = 0; i < MAX_SAMPLERS; i++) {
      found |= key_debug("EXT_texture_swizzle or DEPTH_TEXTURE_MODE",
                         old_key->swizzles[i], key->swizzles[i]);
   }
   found |= key_debug("GL_CLAMP enabled on any texture unit's 1st coordinate",
                      old_key->gl_clamp_mask[0], key->gl_clamp_mask[0]);
   found |= key_debug("GL_CLAMP enabled on any texture unit's 2nd coordinate",
                      old_key->gl_clamp_mask[1], key->gl_clamp_mask[1]);
   found |= key_debug("GL_CLAMP enabled on any texture unit's 3rd coordinate",
                      old_key->gl_clamp_mask[2], key->gl_clamp_mask[2]);
   found |= key_debug("GL_MESA_ycbcr texturing\n",
                      old_key->yuvtex_mask, key->yuvtex_mask);
   found |= key_debug("GL_MESA_ycbcr UV swapping\n",
                      old_key->yuvtex_swap_mask, key->yuvtex_swap_mask);

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
      perf_debug("  Didn't find previous compile in the shader cache for "
                 "debug\n");
      return;
   }

   found |= key_debug("alphatest, computed depth, depth test, or depth write",
                      old_key->iz_lookup, key->iz_lookup);
   found |= key_debug("depth statistics", old_key->stats_wm, key->stats_wm);
   found |= key_debug("flat shading", old_key->flat_shade, key->flat_shade);
   found |= key_debug("number of color buffers", old_key->nr_color_regions, key->nr_color_regions);
   found |= key_debug("sample alpha to coverage", old_key->sample_alpha_to_coverage, key->sample_alpha_to_coverage);
   found |= key_debug("rendering to FBO", old_key->render_to_fbo, key->render_to_fbo);
   found |= key_debug("fragment color clamping", old_key->clamp_fragment_color, key->clamp_fragment_color);
   found |= key_debug("line smoothing", old_key->line_aa, key->line_aa);
   found |= key_debug("proj_attrib_mask", old_key->proj_attrib_mask, key->proj_attrib_mask);
   found |= key_debug("renderbuffer height", old_key->drawable_height, key->drawable_height);
   found |= key_debug("vertex shader outputs", old_key->vp_outputs_written, key->vp_outputs_written);

   found |= brw_debug_recompile_sampler_key(&old_key->tex, &key->tex);

   if (!found) {
      perf_debug("  Something else\n");
   }
}

void
brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				   const struct gl_program *prog,
				   struct brw_sampler_prog_key_data *key)
{
   struct intel_context *intel = intel_context(ctx);

   for (int s = 0; s < MAX_SAMPLERS; s++) {
      key->swizzles[s] = SWIZZLE_NOOP;

      if (!(prog->SamplersUsed & (1 << s)))
	 continue;

      int unit_id = prog->SamplerUnits[s];
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[unit_id];

      if (unit->_ReallyEnabled && unit->_Current->Target != GL_TEXTURE_BUFFER) {
	 const struct gl_texture_object *t = unit->_Current;
	 const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
	 struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit_id);

         const bool alpha_depth = t->DepthMode == GL_ALPHA &&
            (img->_BaseFormat == GL_DEPTH_COMPONENT ||
             img->_BaseFormat == GL_DEPTH_STENCIL);

         /* Haswell handles texture swizzling as surface format overrides
          * (except for GL_ALPHA); all other platforms need MOVs in the shader.
          */
         if (!intel->is_haswell || alpha_depth)
            key->swizzles[s] = brw_get_texture_swizzle(ctx, t);

	 if (img->InternalFormat == GL_YCBCR_MESA) {
	    key->yuvtex_mask |= 1 << s;
	    if (img->TexFormat == MESA_FORMAT_YCBCR)
		key->yuvtex_swap_mask |= 1 << s;
	 }

	 if (sampler->MinFilter != GL_NEAREST &&
	     sampler->MagFilter != GL_NEAREST) {
	    if (sampler->WrapS == GL_CLAMP)
	       key->gl_clamp_mask[0] |= 1 << s;
	    if (sampler->WrapT == GL_CLAMP)
	       key->gl_clamp_mask[1] |= 1 << s;
	    if (sampler->WrapR == GL_CLAMP)
	       key->gl_clamp_mask[2] |= 1 << s;
	 }
      }
   }
}

static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp = 
      (struct brw_fragment_program *)brw->fragment_program;
   const struct gl_program *prog = (struct gl_program *) brw->fragment_program;
   GLuint lookup = 0;
   GLuint line_aa;
   bool program_uses_dfdy = fp->program.UsesDFdy;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   if (intel->gen < 6) {
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

      /* _NEW_STENCIL */
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

   key->line_aa = line_aa;

   if (intel->gen < 6)
      key->stats_wm = brw->intel.stats_wm;

   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   /* Only set this for fixed function.  The optimization it enables isn't
    * useful for programs using shaders.
    */
   if (ctx->Shader.CurrentFragmentProgram)
      key->proj_attrib_mask = 0xffffffff;
   else
      key->proj_attrib_mask = brw->wm.input_size_masks[4-1];

   /* _NEW_LIGHT */
   key->flat_shade = (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_FRAG_CLAMP | _NEW_BUFFERS */
   key->clamp_fragment_color = ctx->Color._ClampFragmentColor;

   /* _NEW_TEXTURE */
   brw_populate_sampler_prog_key_data(ctx, prog, &key->tex);

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

   if ((fp->program.Base.InputsRead & FRAG_BIT_WPOS) || program_uses_dfdy) {
      key->render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   }

   /* _NEW_BUFFERS */
   key->nr_color_regions = ctx->DrawBuffer->_NumColorDrawBuffers;
  /* _NEW_MULTISAMPLE */
   key->sample_alpha_to_coverage = ctx->Multisample.SampleAlphaToCoverage;

   /* CACHE_NEW_VS_PROG */
   if (intel->gen < 6)
      key->vp_outputs_written = brw->vs.prog_data->outputs_written;

   /* The unique fragment program ID */
   key->program_string_id = fp->id;
}


static void
brw_upload_wm_prog(struct brw_context *brw)
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
      bool success = do_wm_prog(brw, ctx->Shader._CurrentFragmentProgram, fp,
				&key);
      (void) success;
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
		_NEW_TEXTURE |
		_NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_upload_wm_prog
};

