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

#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state.h"
#include "i915_state_inlines.h"

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
   return 0;
}

static void i915_bind_sampler_state(struct pipe_context *pipe,
                                    unsigned unit, void *sampler)
{
   struct i915_context *i915 = i915_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   i915->sampler[unit] = (const struct pipe_sampler_state *)sampler;

   i915->dirty |= I915_NEW_SAMPLER;
}

static void i915_delete_sampler_state(struct pipe_context *pipe,
                                      void *sampler)
{
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */

static void *
i915_create_depth_stencil_state(struct pipe_context *pipe,
                           const struct pipe_depth_stencil_state *depth_stencil)
{
   return 0;
}

static void i915_bind_depth_stencil_state(struct pipe_context *pipe,
                                          void *depth_stencil)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->depth_stencil = (const struct pipe_depth_stencil_state *)depth_stencil;

   i915->dirty |= I915_NEW_DEPTH_STENCIL;
}

static void i915_delete_depth_stencil_state(struct pipe_context *pipe,
                                            void *depth_stencil)
{
   /* do nothing */
}

static void i915_set_alpha_test_state(struct pipe_context *pipe,
                              const struct pipe_alpha_test_state *alpha)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->alpha_test = *alpha;

   i915->dirty |= I915_NEW_ALPHA_TEST;
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


static void *
i915_create_shader_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   return 0;
}

static void i915_bind_fs_state( struct pipe_context *pipe,
                               void *fs )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->fs = (struct pipe_shader_state *)fs;

   i915->dirty |= I915_NEW_FS;
}


static void i915_bind_vs_state(struct pipe_context *pipe,
                               void *vs)
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_set_vertex_shader(i915->draw, (const struct pipe_shader_state *)vs);
}

static void i915_delete_shader_state(struct pipe_context *pipe,
                                     void *shader)
{
   /*do nothing*/
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
                             const struct pipe_rasterizer_state *setup)
{
   return 0;
}

static void i915_bind_rasterizer_state( struct pipe_context *pipe,
                                        void *setup )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->rasterizer = (struct pipe_rasterizer_state *)setup;

   /* pass-through to draw module */
   draw_set_rasterizer_state(i915->draw, setup);

   i915->dirty |= I915_NEW_RASTERIZER;
}

static void i915_delete_rasterizer_state(struct pipe_context *pipe,
                                         void *setup)
{
   /* do nothing */
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
   i915->pipe.create_fs_state = i915_create_shader_state;
   i915->pipe.bind_fs_state = i915_bind_fs_state;
   i915->pipe.delete_fs_state = i915_delete_shader_state;
   i915->pipe.create_vs_state = i915_create_shader_state;
   i915->pipe.bind_vs_state = i915_bind_vs_state;
   i915->pipe.delete_vs_state = i915_delete_shader_state;

   i915->pipe.set_alpha_test_state = i915_set_alpha_test_state;
   i915->pipe.set_blend_color = i915_set_blend_color;
   i915->pipe.set_clip_state = i915_set_clip_state;
   i915->pipe.set_clear_color_state = i915_set_clear_color_state;
   i915->pipe.set_constant_buffer = i915_set_constant_buffer;
   i915->pipe.set_framebuffer_state = i915_set_framebuffer_state;
   i915->pipe.set_polygon_stipple = i915_set_polygon_stipple;
   i915->pipe.set_scissor_state = i915_set_scissor_state;
   i915->pipe.set_texture_state = i915_set_texture_state;
   i915->pipe.set_viewport_state = i915_set_viewport_state;
   i915->pipe.set_vertex_buffer = i915_set_vertex_buffer;
   i915->pipe.set_vertex_element = i915_set_vertex_element;
}
