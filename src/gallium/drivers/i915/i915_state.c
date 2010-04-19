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


#include "draw/draw_context.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"

#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state_inlines.h"
#include "i915_fpc.h"

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
   /*         
   case PIPE_TEX_WRAP_MIRRORED_REPEAT:
      return TEXCOORDMODE_MIRROR;
    */
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
   case PIPE_TEX_MIPFILTER_LINEAR:
      return MIPFILTER_LINEAR;
   default:
      assert(0);
      return MIPFILTER_NONE;
   }
}


/* None of this state is actually used for anything yet.
 */
static void *
i915_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *blend)
{
   struct i915_blend_state *cso_data = CALLOC_STRUCT( i915_blend_state );

   {
      unsigned eqRGB  = blend->rt[0].rgb_func;
      unsigned srcRGB = blend->rt[0].rgb_src_factor;
      unsigned dstRGB = blend->rt[0].rgb_dst_factor;

      unsigned eqA    = blend->rt[0].alpha_func;
      unsigned srcA   = blend->rt[0].alpha_src_factor;
      unsigned dstA   = blend->rt[0].alpha_dst_factor;

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

   if ((blend->rt[0].colormask & PIPE_MASK_R) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_RED;

   if ((blend->rt[0].colormask & PIPE_MASK_G) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_GREEN;

   if ((blend->rt[0].colormask & PIPE_MASK_B) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_BLUE;

   if ((blend->rt[0].colormask & PIPE_MASK_A) == 0)
      cso_data->LIS5 |= S5_WRITEDISABLE_ALPHA;

   if (blend->rt[0].blend_enable) {
      unsigned funcRGB = blend->rt[0].rgb_func;
      unsigned srcRGB  = blend->rt[0].rgb_src_factor;
      unsigned dstRGB  = blend->rt[0].rgb_dst_factor;

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
   draw_flush(i915->draw);

   i915->blend = (struct i915_blend_state*)blend;

   i915->dirty |= I915_NEW_BLEND;
}


static void i915_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   FREE(blend);
}

static void i915_set_blend_color( struct pipe_context *pipe,
                                  const struct pipe_blend_color *blend_color )
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

   i915->blend_color = *blend_color;

   i915->dirty |= I915_NEW_BLEND;
}

static void i915_set_stencil_ref( struct pipe_context *pipe,
                                  const struct pipe_stencil_ref *stencil_ref )
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

   i915->stencil_ref = *stencil_ref;

   i915->dirty |= I915_NEW_DEPTH_STENCIL;
}

static void *
i915_create_sampler_state(struct pipe_context *pipe,
                          const struct pipe_sampler_state *sampler)
{
   struct i915_sampler_state *cso = CALLOC_STRUCT( i915_sampler_state );
   const unsigned ws = sampler->wrap_s;
   const unsigned wt = sampler->wrap_t;
   const unsigned wr = sampler->wrap_r;
   unsigned minFilt, magFilt;
   unsigned mipFilt;

   cso->templ = sampler;

   mipFilt = translate_mip_filter(sampler->min_mip_filter);
   minFilt = translate_img_filter( sampler->min_img_filter );
   magFilt = translate_img_filter( sampler->mag_img_filter );
   
   if (sampler->max_anisotropy > 1)
      minFilt = magFilt = FILTER_ANISOTROPIC;

   if (sampler->max_anisotropy > 2) {
      cso->state[0] |= SS2_MAX_ANISO_4;
   }

   {
      int b = (int) (sampler->lod_bias * 16.0);
      b = CLAMP(b, -256, 255);
      cso->state[0] |= ((b << SS2_LOD_BIAS_SHIFT) & SS2_LOD_BIAS_MASK);
   }

   /* Shadow:
    */
   if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) 
   {
      cso->state[0] |= (SS2_SHADOW_ENABLE |
                        i915_translate_compare_func(sampler->compare_func));

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

   if (sampler->normalized_coords)
      cso->state[1] |= SS3_NORMALIZED_COORDS;

   {
      int minlod = (int) (16.0 * sampler->min_lod);
      int maxlod = (int) (16.0 * sampler->max_lod);
      minlod = CLAMP(minlod, 0, 16 * 11);
      maxlod = CLAMP(maxlod, 0, 16 * 11);

      if (minlod > maxlod)
	 maxlod = minlod;

      cso->minlod = minlod;
      cso->maxlod = maxlod;
   }

   {
      ubyte r = float_to_ubyte(sampler->border_color[0]);
      ubyte g = float_to_ubyte(sampler->border_color[1]);
      ubyte b = float_to_ubyte(sampler->border_color[2]);
      ubyte a = float_to_ubyte(sampler->border_color[3]);
      cso->state[2] = I915PACKCOLOR8888(r, g, b, a);
   }
   return cso;
}

static void i915_bind_sampler_states(struct pipe_context *pipe,
                                     unsigned num, void **sampler)
{
   struct i915_context *i915 = i915_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == i915->num_samplers &&
       !memcmp(i915->sampler, sampler, num * sizeof(void *)))
      return;

   draw_flush(i915->draw);

   for (i = 0; i < num; ++i)
      i915->sampler[i] = sampler[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      i915->sampler[i] = NULL;

   i915->num_samplers = num;

   i915->dirty |= I915_NEW_SAMPLER;
}

static void i915_delete_sampler_state(struct pipe_context *pipe,
                                      void *sampler)
{
   FREE(sampler);
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */

static void *
i915_create_depth_stencil_state(struct pipe_context *pipe,
				const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   struct i915_depth_stencil_state *cso = CALLOC_STRUCT( i915_depth_stencil_state );

   {
      int testmask = depth_stencil->stencil[0].valuemask & 0xff;
      int writemask = depth_stencil->stencil[0].writemask & 0xff;

      cso->stencil_modes4 |= (_3DSTATE_MODES_4_CMD |
                              ENABLE_STENCIL_TEST_MASK |
                              STENCIL_TEST_MASK(testmask) |
                              ENABLE_STENCIL_WRITE_MASK |
                              STENCIL_WRITE_MASK(writemask));
   }

   if (depth_stencil->stencil[0].enabled) {
      int test = i915_translate_compare_func(depth_stencil->stencil[0].func);
      int fop  = i915_translate_stencil_op(depth_stencil->stencil[0].fail_op);
      int dfop = i915_translate_stencil_op(depth_stencil->stencil[0].zfail_op);
      int dpop = i915_translate_stencil_op(depth_stencil->stencil[0].zpass_op);

      cso->stencil_LIS5 |= (S5_STENCIL_TEST_ENABLE |
                            S5_STENCIL_WRITE_ENABLE |
                            (test << S5_STENCIL_TEST_FUNC_SHIFT) |
                            (fop  << S5_STENCIL_FAIL_SHIFT) |
                            (dfop << S5_STENCIL_PASS_Z_FAIL_SHIFT) |
                            (dpop << S5_STENCIL_PASS_Z_PASS_SHIFT));
   }

   if (depth_stencil->stencil[1].enabled) {
      int test  = i915_translate_compare_func(depth_stencil->stencil[1].func);
      int fop   = i915_translate_stencil_op(depth_stencil->stencil[1].fail_op);
      int dfop  = i915_translate_stencil_op(depth_stencil->stencil[1].zfail_op);
      int dpop  = i915_translate_stencil_op(depth_stencil->stencil[1].zpass_op);
      int tmask = depth_stencil->stencil[1].valuemask & 0xff;
      int wmask = depth_stencil->stencil[1].writemask & 0xff;

      cso->bfo[0] = (_3DSTATE_BACKFACE_STENCIL_OPS |
                     BFO_ENABLE_STENCIL_FUNCS |
                     BFO_ENABLE_STENCIL_TWO_SIDE |
                     BFO_ENABLE_STENCIL_REF |
                     BFO_STENCIL_TWO_SIDE |
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

   if (depth_stencil->alpha.enabled) {
      int test = i915_translate_compare_func(depth_stencil->alpha.func);
      ubyte refByte = float_to_ubyte(depth_stencil->alpha.ref_value);

      cso->depth_LIS6 |= (S6_ALPHA_TEST_ENABLE |
			  (test << S6_ALPHA_TEST_FUNC_SHIFT) |
			  (((unsigned) refByte) << S6_ALPHA_REF_SHIFT));
   }

   return cso;
}

static void i915_bind_depth_stencil_state(struct pipe_context *pipe,
                                          void *depth_stencil)
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

   i915->depth_stencil = (const struct i915_depth_stencil_state *)depth_stencil;

   i915->dirty |= I915_NEW_DEPTH_STENCIL;
}

static void i915_delete_depth_stencil_state(struct pipe_context *pipe,
                                            void *depth_stencil)
{
   FREE(depth_stencil);
}


static void i915_set_scissor_state( struct pipe_context *pipe,
                                 const struct pipe_scissor_state *scissor )
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

   memcpy( &i915->scissor, scissor, sizeof(*scissor) );
   i915->dirty |= I915_NEW_SCISSOR;
}


static void i915_set_polygon_stipple( struct pipe_context *pipe,
                                   const struct pipe_poly_stipple *stipple )
{
}



static void *
i915_create_fs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct i915_context *i915 = i915_context(pipe);
   struct i915_fragment_shader *ifs = CALLOC_STRUCT(i915_fragment_shader);
   if (!ifs)
      return NULL;

   ifs->state.tokens = tgsi_dup_tokens(templ->tokens);

   tgsi_scan_shader(templ->tokens, &ifs->info);

   /* The shader's compiled to i915 instructions here */
   i915_translate_fragment_program(i915, ifs);

   return ifs;
}

static void
i915_bind_fs_state(struct pipe_context *pipe, void *shader)
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

   i915->fs = (struct i915_fragment_shader*) shader;

   i915->dirty |= I915_NEW_FS;
}

static
void i915_delete_fs_state(struct pipe_context *pipe, void *shader)
{
   struct i915_fragment_shader *ifs = (struct i915_fragment_shader *) shader;

   if (ifs->program)
      FREE(ifs->program);
   ifs->program_len = 0;

   FREE((struct tgsi_token *)ifs->state.tokens);

   FREE(ifs);
}


static void *
i915_create_vs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   return draw_create_vertex_shader(i915->draw, templ);
}

static void i915_bind_vs_state(struct pipe_context *pipe, void *shader)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_bind_vertex_shader(i915->draw, (struct draw_vertex_shader *) shader);

   i915->dirty |= I915_NEW_VS;
}

static void i915_delete_vs_state(struct pipe_context *pipe, void *shader)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_delete_vertex_shader(i915->draw, (struct draw_vertex_shader *) shader);
}

static void i915_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     struct pipe_buffer *buf)
{
   struct i915_context *i915 = i915_context(pipe);
   struct pipe_screen *screen = pipe->screen;
   draw_flush(i915->draw);

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
   if (buf) {
      void *mapped;
      if (buf->size &&
          (mapped = pipe_buffer_map(screen, buf,
                                    PIPE_BUFFER_USAGE_CPU_READ))) {
         memcpy(i915->current.constants[shader], mapped, buf->size);
         pipe_buffer_unmap(screen, buf);
         i915->current.num_user_constants[shader]
            = buf->size / (4 * sizeof(float));
      }
      else {
         i915->current.num_user_constants[shader] = 0;
      }
   }

   i915->dirty |= I915_NEW_CONSTANTS;
}


static void i915_set_sampler_textures(struct pipe_context *pipe,
                                      unsigned num,
                                      struct pipe_texture **texture)
{
   struct i915_context *i915 = i915_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == i915->num_textures &&
       !memcmp(i915->texture, texture, num * sizeof(struct pipe_texture *)))
      return;

   /* Fixes wrong texture in texobj with VBUF */
   draw_flush(i915->draw);

   for (i = 0; i < num; i++)
      pipe_texture_reference((struct pipe_texture **) &i915->texture[i],
                             texture[i]);

   for (i = num; i < i915->num_textures; i++)
      pipe_texture_reference((struct pipe_texture **) &i915->texture[i],
                             NULL);

   i915->num_textures = num;

   i915->dirty |= I915_NEW_TEXTURE;
}



static void i915_set_framebuffer_state(struct pipe_context *pipe,
				       const struct pipe_framebuffer_state *fb)
{
   struct i915_context *i915 = i915_context(pipe);
   int i;

   draw_flush(i915->draw);

   i915->framebuffer.width = fb->width;
   i915->framebuffer.height = fb->height;
   i915->framebuffer.nr_cbufs = fb->nr_cbufs;
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&i915->framebuffer.cbufs[i], fb->cbufs[i]);
   }
   pipe_surface_reference(&i915->framebuffer.zsbuf, fb->zsbuf);

   i915->dirty |= I915_NEW_FRAMEBUFFER;
}



static void i915_set_clip_state( struct pipe_context *pipe,
			     const struct pipe_clip_state *clip )
{
   struct i915_context *i915 = i915_context(pipe);
   draw_flush(i915->draw);

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

   i915->dirty |= I915_NEW_VIEWPORT;
}


static void *
i915_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *rasterizer)
{
   struct i915_rasterizer_state *cso = CALLOC_STRUCT( i915_rasterizer_state );

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

   cso->LIS7 = fui( rasterizer->offset_units );


   return cso;
}

static void i915_bind_rasterizer_state( struct pipe_context *pipe,
                                        void *raster )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->rasterizer = (struct i915_rasterizer_state *)raster;

   /* pass-through to draw module */
   draw_set_rasterizer_state(i915->draw,
                           (i915->rasterizer ? i915->rasterizer->templ : NULL),
                           raster);

   i915->dirty |= I915_NEW_RASTERIZER;
}

static void i915_delete_rasterizer_state(struct pipe_context *pipe,
                                         void *raster)
{
   FREE(raster);
}

static void i915_set_vertex_buffers(struct pipe_context *pipe,
                                    unsigned count,
                                    const struct pipe_vertex_buffer *buffers)
{
   struct i915_context *i915 = i915_context(pipe);
   /* Because we change state before the draw_set_vertex_buffers call
    * we need a flush here, just to be sure.
    */
   draw_flush(i915->draw);

   memcpy(i915->vertex_buffer, buffers, count * sizeof(buffers[0]));
   i915->num_vertex_buffers = count;

   /* pass-through to draw module */
   draw_set_vertex_buffers(i915->draw, count, buffers);
}

static void i915_set_vertex_elements(struct pipe_context *pipe,
                                     unsigned count,
                                     const struct pipe_vertex_element *elements)
{
   struct i915_context *i915 = i915_context(pipe);
   /* Because we change state before the draw_set_vertex_buffers call
    * we need a flush here, just to be sure.
    */
   draw_flush(i915->draw);

   i915->num_vertex_elements = count;
   /* pass-through to draw module */
   draw_set_vertex_elements(i915->draw, count, elements);
}


void
i915_init_state_functions( struct i915_context *i915 )
{
   i915->base.create_blend_state = i915_create_blend_state;
   i915->base.bind_blend_state = i915_bind_blend_state;
   i915->base.delete_blend_state = i915_delete_blend_state;

   i915->base.create_sampler_state = i915_create_sampler_state;
   i915->base.bind_fragment_sampler_states = i915_bind_sampler_states;
   i915->base.delete_sampler_state = i915_delete_sampler_state;

   i915->base.create_depth_stencil_alpha_state = i915_create_depth_stencil_state;
   i915->base.bind_depth_stencil_alpha_state = i915_bind_depth_stencil_state;
   i915->base.delete_depth_stencil_alpha_state = i915_delete_depth_stencil_state;

   i915->base.create_rasterizer_state = i915_create_rasterizer_state;
   i915->base.bind_rasterizer_state = i915_bind_rasterizer_state;
   i915->base.delete_rasterizer_state = i915_delete_rasterizer_state;
   i915->base.create_fs_state = i915_create_fs_state;
   i915->base.bind_fs_state = i915_bind_fs_state;
   i915->base.delete_fs_state = i915_delete_fs_state;
   i915->base.create_vs_state = i915_create_vs_state;
   i915->base.bind_vs_state = i915_bind_vs_state;
   i915->base.delete_vs_state = i915_delete_vs_state;

   i915->base.set_blend_color = i915_set_blend_color;
   i915->base.set_stencil_ref = i915_set_stencil_ref;
   i915->base.set_clip_state = i915_set_clip_state;
   i915->base.set_constant_buffer = i915_set_constant_buffer;
   i915->base.set_framebuffer_state = i915_set_framebuffer_state;

   i915->base.set_polygon_stipple = i915_set_polygon_stipple;
   i915->base.set_scissor_state = i915_set_scissor_state;
   i915->base.set_fragment_sampler_textures = i915_set_sampler_textures;
   i915->base.set_viewport_state = i915_set_viewport_state;
   i915->base.set_vertex_buffers = i915_set_vertex_buffers;
   i915->base.set_vertex_elements = i915_set_vertex_elements;
}
