/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/draw/draw_context.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state.h"
#include "i915_state_inlines.h"


/* The i915 (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static unsigned
translate_wrap_mode(unsigned wrap)
{
   switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
      return TEXCOORDMODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP:
      return TEXCOORDMODE_CLAMP_EDGE;   /* not quite correct */
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TEXCOORDMODE_CLAMP_EDGE;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return TEXCOORDMODE_CLAMP_BORDER;
//   case PIPE_TEX_WRAP_MIRRORED_REPEAT:
//      return TEXCOORDMODE_MIRROR;
   default:
      return TEXCOORDMODE_WRAP;
   }
}

static unsigned translate_img_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
      return FILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:
      return FILTER_LINEAR;
   default:
      assert(0);
      return FILTER_NEAREST;
   }
}

static unsigned translate_mip_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      return MIPFILTER_NONE;
   case PIPE_TEX_MIPFILTER_NEAREST:
      return MIPFILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:
      return MIPFILTER_LINEAR;
   default:
      assert(0);
      return MIPFILTER_NONE;
   }
}

static unsigned translate_compare_func(unsigned func)
{
   switch (func) {
   case PIPE_FUNC_NEVER:
   case PIPE_FUNC_LESS:
   case PIPE_FUNC_EQUAL:
   case PIPE_FUNC_LEQUAL:
   case PIPE_FUNC_GREATER:
   case PIPE_FUNC_NOTEQUAL:
   case PIPE_FUNC_GEQUAL:
   case PIPE_FUNC_ALWAYS:
      return 0;
   default:
      assert(0);
      return 0;
   }
}


/* None of this state is actually used for anything yet.
 */
static void *
i915_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *blend)
{
   struct i915_blend_state *cso_data = calloc(1, sizeof(struct i915_blend_state));

   {
      unsigned eqRGB  = blend->rgb_func;
      unsigned srcRGB = blend->rgb_src_factor;
      unsigned dstRGB = blend->rgb_dst_factor;

      unsigned eqA    = blend->alpha_func;
      unsigned srcA   = blend->alpha_src_factor;
      unsigned dstA   = blend->alpha_dst_factor;

      /* Special handling for MIN/MAX filter modes handled at
       * state_tracker level.
       */

      if (srcA != srcRGB ||
	  dstA != dstRGB ||
	  eqA != eqRGB) {

	 cso_data->iab = (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD |
                          IAB_MODIFY_ENABLE |
                          IAB_ENABLE |
                          IAB_MODIFY_FUNC |
                          IAB_MODIFY_SRC_FACTOR |
                          IAB_MODIFY_DST_FACTOR |
                          SRC_ABLND_FACT(i915_translate_blend_factor(srcA)) |
                          DST_ABLND_FACT(i915_translate_blend_factor(dstA)) |
                          (i915_translate_blend_func(eqA) << IAB_FUNC_SHIFT));
      }
      else {
	 cso_data->iab = (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD |
                          IAB_MODIFY_ENABLE |
                          0);
      }
   }

   cso_data->modes4 |= (_3DSTATE_MODES_4_CMD |
                        ENABLE_LOGIC_OP_FUNC |
                        LOGIC_OP_FUNC(i915_translate_logic_op(blend->logicop_func)));

   if (blend->logicop_enable)
      cso_data->LIS5 |= S5_LOGICOP_ENABLE;

   if (blend->dither)
      cso_data->LIS5 |= S5_COLOR_DITHER_ENABLE;

   if ((blend->colormask & PIPE_MASK_R) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_RED;

   if ((blend->colormask & PIPE_MASK_G) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_GREEN;

   if ((blend->colormask & PIPE_MASK_B) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_BLUE;

   if ((blend->colormask & PIPE_MASK_A) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_ALPHA;

   if (blend->blend_enable) {
      unsigned funcRGB = blend->rgb_func;
      unsigned srcRGB  = blend->rgb_src_factor;
      unsigned dstRGB  = blend->rgb_dst_factor;

      cso_data->LIS6 |= (S6_CBUF_BLEND_ENABLE |
                         SRC_BLND_FACT(i915_translate_blend_factor(srcRGB)) |
                         DST_BLND_FACT(i915_translate_blend_factor(dstRGB)) |
                         (i915_translate_blend_func(funcRGB) << S6_CBUF_BLEND_FUNC_SHIFT));
   }

   return cso_data;
}

static void i915_bind_blend_state(struct pipe_context *pipe,
                                  void *blend)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->blend = (struct i915_blend_state*)blend;

   i915->dirty |= I915_NEW_BLEND;
}


static void i915_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   free(blend);
}

static void i915_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->blend_color = *blend_color;

   i915->dirty |= I915_NEW_BLEND;
}

static void *
i915_create_sampler_state(struct pipe_context *pipe,
                          const struct pipe_sampler_state *sampler)
{
   struct i915_sampler_state *cso = calloc(1, sizeof(struct i915_sampler_state));
   cso->templ = sampler;

   const unsigned ws = sampler->wrap_s;
   const unsigned wt = sampler->wrap_t;
   const unsigned wr = sampler->wrap_r;
   unsigned minFilt, magFilt;
   unsigned mipFilt;

   mipFilt = translate_mip_filter(sampler->min_mip_filter);
   if (sampler->max_anisotropy > 1.0) {
      minFilt = FILTER_ANISOTROPIC;
      magFilt = FILTER_ANISOTROPIC;
   }
   else {
      minFilt = translate_img_filter( sampler->min_img_filter );
      magFilt = translate_img_filter( sampler->mag_img_filter );
   }

   {
      int b = sampler->lod_bias * 16.0;
      b = CLAMP(b, -256, 255);
      cso->state[0] |= ((b << SS2_LOD_BIAS_SHIFT) & SS2_LOD_BIAS_MASK);
   }

   /* Shadow:
    */
   if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) 
   {
      cso->state[0] |= (SS2_SHADOW_ENABLE |
                        translate_compare_func(sampler->compare_func));

      minFilt = FILTER_4X4_FLAT;
      magFilt = FILTER_4X4_FLAT;
   }

   cso->state[0] |= ((minFilt << SS2_MIN_FILTER_SHIFT) |
                     (mipFilt << SS2_MIP_FILTER_SHIFT) |
                     (magFilt << SS2_MAG_FILTER_SHIFT));

   cso->state[1] |=
      ((translate_wrap_mode(ws) << SS3_TCX_ADDR_MODE_SHIFT) |
       (translate_wrap_mode(wt) << SS3_TCY_ADDR_MODE_SHIFT) |
       (translate_wrap_mode(wr) << SS3_TCZ_ADDR_MODE_SHIFT));

   {
      ubyte r = float_to_ubyte(sampler->border_color[0]);
      ubyte g = float_to_ubyte(sampler->border_color[1]);
      ubyte b = float_to_ubyte(sampler->border_color[2]);
      ubyte a = float_to_ubyte(sampler->border_color[3]);
      cso->state[2] = I915PACKCOLOR8888(r, g, b, a);
   }
   return cso;
}

static void i915_bind_sampler_state(struct pipe_context *pipe,
                                    unsigned unit, void *sampler)
{
   struct i915_context *i915 = i915_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   i915->sampler[unit] = (const struct i915_sampler_state*)sampler;

   i915->dirty |= I915_NEW_SAMPLER;
}

static void i915_delete_sampler_state(struct pipe_context *pipe,
                                      void *sampler)
{
   free(sampler);
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */

static void *
i915_create_depth_stencil_state(struct pipe_context *pipe,
                           const struct pipe_depth_stencil_state *depth_stencil)
{
   struct i915_depth_stencil_state *cso = calloc(1, sizeof(struct i915_depth_stencil_state));

   {
      int testmask = depth_stencil->stencil.value_mask[0] & 0xff;
      int writemask = depth_stencil->stencil.write_mask[0] & 0xff;

      cso->stencil_modes4 |= (_3DSTATE_MODES_4_CMD |
                              ENABLE_STENCIL_TEST_MASK |
                              STENCIL_TEST_MASK(testmask) |
                              ENABLE_STENCIL_WRITE_MASK |
                              STENCIL_WRITE_MASK(writemask));
   }

   if (depth_stencil->stencil.front_enabled) {
      int test = i915_translate_compare_func(depth_stencil->stencil.front_func);
      int fop  = i915_translate_stencil_op(depth_stencil->stencil.front_fail_op);
      int dfop = i915_translate_stencil_op(depth_stencil->stencil.front_zfail_op);
      int dpop = i915_translate_stencil_op(depth_stencil->stencil.front_zpass_op);
      int ref  = depth_stencil->stencil.ref_value[0] & 0xff;

      cso->stencil_LIS5 |= (S5_STENCIL_TEST_ENABLE |
                            S5_STENCIL_WRITE_ENABLE |
                            (ref  << S5_STENCIL_REF_SHIFT) |
                            (test << S5_STENCIL_TEST_FUNC_SHIFT) |
                            (fop  << S5_STENCIL_FAIL_SHIFT) |
                            (dfop << S5_STENCIL_PASS_Z_FAIL_SHIFT) |
                            (dpop << S5_STENCIL_PASS_Z_PASS_SHIFT));
   }

   if (depth_stencil->stencil.back_enabled) {
      int test  = i915_translate_compare_func(depth_stencil->stencil.back_func);
      int fop   = i915_translate_stencil_op(depth_stencil->stencil.back_fail_op);
      int dfop  = i915_translate_stencil_op(depth_stencil->stencil.back_zfail_op);
      int dpop  = i915_translate_stencil_op(depth_stencil->stencil.back_zpass_op);
      int ref   = depth_stencil->stencil.ref_value[1] & 0xff;
      int tmask = depth_stencil->stencil.value_mask[1] & 0xff;
      int wmask = depth_stencil->stencil.write_mask[1] & 0xff;

      cso->bfo[0] = (_3DSTATE_BACKFACE_STENCIL_OPS |
                     BFO_ENABLE_STENCIL_FUNCS |
                     BFO_ENABLE_STENCIL_TWO_SIDE |
                     BFO_ENABLE_STENCIL_REF |
                     BFO_STENCIL_TWO_SIDE |
                     (ref  << BFO_STENCIL_REF_SHIFT) |
                     (test << BFO_STENCIL_TEST_SHIFT) |
                     (fop  << BFO_STENCIL_FAIL_SHIFT) |
                     (dfop << BFO_STENCIL_PASS_Z_FAIL_SHIFT) |
                     (dpop << BFO_STENCIL_PASS_Z_PASS_SHIFT));

      cso->bfo[1] = (_3DSTATE_BACKFACE_STENCIL_MASKS |
                     BFM_ENABLE_STENCIL_TEST_MASK |
                     BFM_ENABLE_STENCIL_WRITE_MASK |
                     (tmask << BFM_STENCIL_TEST_MASK_SHIFT) |
                     (wmask << BFM_STENCIL_WRITE_MASK_SHIFT));
   }
   else {
      /* This actually disables two-side stencil: The bit set is a
       * modify-enable bit to indicate we are changing the two-side
       * setting.  Then there is a symbolic zero to show that we are
       * setting the flag to zero/off.
       */
      cso->bfo[0] = (_3DSTATE_BACKFACE_STENCIL_OPS |
                     BFO_ENABLE_STENCIL_TWO_SIDE |
                     0);
      cso->bfo[1] = 0;
   }

   if (depth_stencil->depth.enabled) {
      int func = i915_translate_compare_func(depth_stencil->depth.func);

      cso->depth_LIS6 |= (S6_DEPTH_TEST_ENABLE |
                          (func << S6_DEPTH_TEST_FUNC_SHIFT));

      if (depth_stencil->depth.writemask)
	 cso->depth_LIS6 |= S6_DEPTH_WRITE_ENABLE;
   }

   return cso;
}

static void i915_bind_depth_stencil_state(struct pipe_context *pipe,
                                          void *depth_stencil)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->depth_stencil = (const struct i915_depth_stencil_state *)depth_stencil;

   i915->dirty |= I915_NEW_DEPTH_STENCIL;
}

static void i915_delete_depth_stencil_state(struct pipe_context *pipe,
                                            void *depth_stencil)
{
   free(depth_stencil);
}


static void *
i915_create_alpha_test_state(struct pipe_context *pipe,
                             const struct pipe_alpha_test_state *alpha_test)
{
   struct i915_alpha_test_state *cso = calloc(1, sizeof(struct i915_alpha_test_state));

   if (alpha_test->enabled) {
      int test = i915_translate_compare_func(alpha_test->func);
      ubyte refByte = float_to_ubyte(alpha_test->ref);

      cso->LIS6 |= (S6_ALPHA_TEST_ENABLE |
                    (test << S6_ALPHA_TEST_FUNC_SHIFT) |
                    (((unsigned) refByte) << S6_ALPHA_REF_SHIFT));
   }
   return cso;
}

static void i915_bind_alpha_test_state(struct pipe_context *pipe,
                                       void *alpha)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->alpha_test = (const struct i915_alpha_test_state*)alpha;

   i915->dirty |= I915_NEW_ALPHA_TEST;
}

static void i915_delete_alpha_test_state(struct pipe_context *pipe,
                                         void *alpha)
{
   free(alpha);
}

static void i915_set_scissor_state( struct pipe_context *pipe,
                                 const struct pipe_scissor_state *scissor )
{
   struct i915_context *i915 = i915_context(pipe);

   memcpy( &i915->scissor, scissor, sizeof(*scissor) );
   i915->dirty |= I915_NEW_SCISSOR;
}


static void i915_set_polygon_stipple( struct pipe_context *pipe,
                                   const struct pipe_poly_stipple *stipple )
{
}

static void * i915_create_fs_state(struct pipe_context *pipe,
                                   const struct pipe_shader_state *templ)
{
   return 0;
}

static void i915_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->fs = (struct pipe_shader_state *)fs;

   i915->dirty |= I915_NEW_FS;
}

static void i915_delete_fs_state(struct pipe_context *pipe, void *shader)
{
   /*do nothing*/
}

static void *
i915_create_vs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   return draw_create_vertex_shader(i915->draw, templ);
}

static void i915_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_bind_vertex_shader(i915->draw, vs);
}

static void i915_delete_vs_state(struct pipe_context *pipe, void *shader)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_delete_vertex_shader(i915->draw, shader);
}

static void i915_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     const struct pipe_constant_buffer *buf)
{
   struct i915_context *i915 = i915_context(pipe);
   struct pipe_winsys *ws = pipe->winsys;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   /* Make a copy of shader constants.
    * During fragment program translation we may add additional
    * constants to the array.
    *
    * We want to consider the situation where some user constants
    * (ex: a material color) may change frequently but the shader program
    * stays the same.  In that case we should only be updating the first
    * N constants, leaving any extras from shader translation alone.
    */
   {
      void *mapped;
      if (buf->size &&
          (mapped = ws->buffer_map(ws, buf->buffer, PIPE_BUFFER_FLAG_READ))) {
         memcpy(i915->current.constants[shader], mapped, buf->size);
         ws->buffer_unmap(ws, buf->buffer);
         i915->current.num_user_constants[shader]
            = buf->size / (4 * sizeof(float));
      }
      else {
         i915->current.num_user_constants[shader] = 0;
      }
   }

   i915->dirty |= I915_NEW_CONSTANTS;
}


static void i915_set_texture_state(struct pipe_context *pipe,
				   unsigned unit,
				   struct pipe_mipmap_tree *texture)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->texture[unit] = texture;  /* ptr, not struct */

   i915->dirty |= I915_NEW_TEXTURE;
}



static void i915_set_framebuffer_state(struct pipe_context *pipe,
				       const struct pipe_framebuffer_state *fb)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->framebuffer = *fb; /* struct copy */

   i915->dirty |= I915_NEW_FRAMEBUFFER;
}


static void
i915_set_feedback_state(struct pipe_context *pipe,
                        const struct pipe_feedback_state *feedback)
{
   struct i915_context *i915 = i915_context(pipe);
   i915->feedback = *feedback;
   draw_set_feedback_state(i915->draw, feedback);
}


static void
i915_set_feedback_buffer(struct pipe_context *pipe,
                             unsigned index,
                             const struct pipe_feedback_buffer *feedback)
{
   struct i915_context *i915 = i915_context(pipe);

   assert(index < PIPE_MAX_FEEDBACK_ATTRIBS);

   /* Need to be careful with referencing */
   pipe->winsys->buffer_reference(pipe->winsys,
                                  &i915->feedback_buffer[index].buffer,
                                  feedback->buffer);
   i915->feedback_buffer[index].size = feedback->size;
   i915->feedback_buffer[index].start_offset = feedback->start_offset;
}



static void i915_set_clear_color_state(struct pipe_context *pipe,
                               const struct pipe_clear_color_state *clear)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->clear_color = *clear; /* struct copy */
}



static void i915_set_clip_state( struct pipe_context *pipe,
			     const struct pipe_clip_state *clip )
{
   struct i915_context *i915 = i915_context(pipe);

   draw_set_clip_state(i915->draw, clip);

   i915->dirty |= I915_NEW_CLIP;
}



/* Called when driver state tracker notices changes to the viewport
 * matrix:
 */
static void i915_set_viewport_state( struct pipe_context *pipe,
				     const struct pipe_viewport_state *viewport )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->viewport = *viewport; /* struct copy */

   /* pass the viewport info to the draw module */
   draw_set_viewport_state(i915->draw, &i915->viewport);

   /* Using tnl/ and vf/ modules is temporary while getting started.
    * Full pipe will have vertex shader, vertex fetch of its own.
    */
   i915->dirty |= I915_NEW_VIEWPORT;

}


static void *
i915_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *rasterizer)
{
   struct i915_rasterizer_state *cso = calloc(1, sizeof(struct i915_rasterizer_state));

   cso->templ = rasterizer;
   cso->color_interp = rasterizer->flatshade ? INTERP_CONSTANT : INTERP_LINEAR;
   cso->light_twoside = rasterizer->light_twoside;
   cso->ds[0].u = _3DSTATE_DEPTH_OFFSET_SCALE;
   cso->ds[1].f = rasterizer->offset_scale;
   if (rasterizer->poly_stipple_enable) {
      cso->st |= ST1_ENABLE;
   }

   if (rasterizer->scissor)
      cso->sc[0] = _3DSTATE_SCISSOR_ENABLE_CMD | ENABLE_SCISSOR_RECT;
   else
      cso->sc[0] = _3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT;

   switch (rasterizer->cull_mode) {
   case PIPE_WINDING_NONE:
      cso->LIS4 |= S4_CULLMODE_NONE;
      break;
   case PIPE_WINDING_CW:
      cso->LIS4 |= S4_CULLMODE_CW;
      break;
   case PIPE_WINDING_CCW:
      cso->LIS4 |= S4_CULLMODE_CCW;
      break;
   case PIPE_WINDING_BOTH:
      cso->LIS4 |= S4_CULLMODE_BOTH;
      break;
   }

   {
      int line_width = CLAMP((int)(rasterizer->line_width * 2), 1, 0xf);

      cso->LIS4 |= line_width << S4_LINE_WIDTH_SHIFT;

      if (rasterizer->line_smooth)
	 cso->LIS4 |= S4_LINE_ANTIALIAS_ENABLE;
   }

   {
      int point_size = CLAMP((int) rasterizer->point_size, 1, 0xff);

      cso->LIS4 |= point_size << S4_POINT_WIDTH_SHIFT;
   }

   if (rasterizer->flatshade) {
      cso->LIS4 |= (S4_FLATSHADE_ALPHA |
                    S4_FLATSHADE_COLOR |
                    S4_FLATSHADE_SPECULAR);
   }

   cso->LIS7 = rasterizer->offset_units; /* probably incorrect */


   return cso;
}

static void i915_bind_rasterizer_state( struct pipe_context *pipe,
                                        void *setup )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->rasterizer = (struct i915_rasterizer_state *)setup;

   /* pass-through to draw module */
   draw_set_rasterizer_state(i915->draw, i915->rasterizer->templ);

   i915->dirty |= I915_NEW_RASTERIZER;
}

static void i915_delete_rasterizer_state(struct pipe_context *pipe,
                                         void *setup)
{
   free(setup);
}

static void i915_set_vertex_buffer( struct pipe_context *pipe,
                                    unsigned index,
                                    const struct pipe_vertex_buffer *buffer )
{
   struct i915_context *i915 = i915_context(pipe);
   i915->vertex_buffer[index] = *buffer;
   /* pass-through to draw module */
   draw_set_vertex_buffer(i915->draw, index, buffer);
}

static void i915_set_vertex_element( struct pipe_context *pipe,
                                     unsigned index,
                                     const struct pipe_vertex_element *element)
{
   struct i915_context *i915 = i915_context(pipe);
   /* pass-through to draw module */
   draw_set_vertex_element(i915->draw, index, element);
}



void
i915_init_state_functions( struct i915_context *i915 )
{
   i915->pipe.create_alpha_test_state = i915_create_alpha_test_state;
   i915->pipe.bind_alpha_test_state   = i915_bind_alpha_test_state;
   i915->pipe.delete_alpha_test_state = i915_delete_alpha_test_state;

   i915->pipe.create_blend_state = i915_create_blend_state;
   i915->pipe.bind_blend_state = i915_bind_blend_state;
   i915->pipe.delete_blend_state = i915_delete_blend_state;

   i915->pipe.create_sampler_state = i915_create_sampler_state;
   i915->pipe.bind_sampler_state = i915_bind_sampler_state;
   i915->pipe.delete_sampler_state = i915_delete_sampler_state;

   i915->pipe.create_depth_stencil_state = i915_create_depth_stencil_state;
   i915->pipe.bind_depth_stencil_state = i915_bind_depth_stencil_state;
   i915->pipe.delete_depth_stencil_state = i915_delete_depth_stencil_state;

   i915->pipe.create_rasterizer_state = i915_create_rasterizer_state;
   i915->pipe.bind_rasterizer_state = i915_bind_rasterizer_state;
   i915->pipe.delete_rasterizer_state = i915_delete_rasterizer_state;
   i915->pipe.create_fs_state = i915_create_fs_state;
   i915->pipe.bind_fs_state = i915_bind_fs_state;
   i915->pipe.delete_fs_state = i915_delete_fs_state;
   i915->pipe.create_vs_state = i915_create_vs_state;
   i915->pipe.bind_vs_state = i915_bind_vs_state;
   i915->pipe.delete_vs_state = i915_delete_vs_state;

   i915->pipe.set_blend_color = i915_set_blend_color;
   i915->pipe.set_clip_state = i915_set_clip_state;
   i915->pipe.set_clear_color_state = i915_set_clear_color_state;
   i915->pipe.set_constant_buffer = i915_set_constant_buffer;
   i915->pipe.set_framebuffer_state = i915_set_framebuffer_state;
   i915->pipe.set_feedback_state = i915_set_feedback_state;
   i915->pipe.set_feedback_buffer = i915_set_feedback_buffer;

   i915->pipe.set_polygon_stipple = i915_set_polygon_stipple;
   i915->pipe.set_scissor_state = i915_set_scissor_state;
   i915->pipe.set_texture_state = i915_set_texture_state;
   i915->pipe.set_viewport_state = i915_set_viewport_state;
   i915->pipe.set_vertex_buffer = i915_set_vertex_buffer;
   i915->pipe.set_vertex_element = i915_set_vertex_element;
}
