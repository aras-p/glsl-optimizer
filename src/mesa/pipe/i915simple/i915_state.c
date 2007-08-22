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
#include "i915_state.h"

/* None of this state is actually used for anything yet.
 */
static void i915_set_blend_state( struct pipe_context *pipe,
			     const struct pipe_blend_state *blend )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->blend = *blend;

   i915->dirty |= I915_NEW_BLEND;
}


static void i915_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->blend_color = *blend_color;

   i915->dirty |= I915_NEW_BLEND;
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */
static void i915_set_depth_test_state(struct pipe_context *pipe,
                              const struct pipe_depth_state *depth)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->depth_test = *depth;

   i915->dirty |= I915_NEW_DEPTH_TEST;
}

static void i915_set_alpha_test_state(struct pipe_context *pipe,
                              const struct pipe_alpha_test_state *alpha)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->alpha_test = *alpha;

   i915->dirty |= I915_NEW_ALPHA_TEST;
}

static void i915_set_stencil_state(struct pipe_context *pipe,
                           const struct pipe_stencil_state *stencil)
{
   struct i915_context *i915 = i915_context(pipe);

   i915->stencil = *stencil;

   i915->dirty |= I915_NEW_STENCIL;
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



static void i915_set_fs_state( struct pipe_context *pipe,
                               const struct pipe_shader_state *fs )
{
   struct i915_context *i915 = i915_context(pipe);

   memcpy(&i915->fs, fs, sizeof(*fs));

   i915->dirty |= I915_NEW_FS;
}


static void i915_set_vs_state( struct pipe_context *pipe,
                               const struct pipe_shader_state *vs )
{
   struct i915_context *i915 = i915_context(pipe);

   /* just pass-through to draw module */
   draw_set_vertex_shader(i915->draw, vs);
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
         fprintf(stderr, "i915 problem: map of constant buffer failed\n");
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


static void i915_set_sampler_state(struct pipe_context *pipe,
                           unsigned unit,
                           const struct pipe_sampler_state *sampler)
{
   struct i915_context *i915 = i915_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   i915->sampler[unit] = *sampler;

   i915->dirty |= I915_NEW_SAMPLER;
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

static void i915_set_setup_state( struct pipe_context *pipe,
				      const struct pipe_setup_state *setup )
{
   struct i915_context *i915 = i915_context(pipe);

   i915->setup = *setup;

   /* pass-through to draw module */
   draw_set_setup_state(i915->draw, setup);

   i915->dirty |= I915_NEW_SETUP;
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
   i915->pipe.set_alpha_test_state = i915_set_alpha_test_state;
   i915->pipe.set_blend_color = i915_set_blend_color;
   i915->pipe.set_blend_state = i915_set_blend_state;
   i915->pipe.set_clip_state = i915_set_clip_state;
   i915->pipe.set_clear_color_state = i915_set_clear_color_state;
   i915->pipe.set_constant_buffer = i915_set_constant_buffer;
   i915->pipe.set_depth_state = i915_set_depth_test_state;
   i915->pipe.set_framebuffer_state = i915_set_framebuffer_state;
   i915->pipe.set_fs_state = i915_set_fs_state;
   i915->pipe.set_vs_state = i915_set_vs_state;
   i915->pipe.set_polygon_stipple = i915_set_polygon_stipple;
   i915->pipe.set_sampler_state = i915_set_sampler_state;
   i915->pipe.set_scissor_state = i915_set_scissor_state;
   i915->pipe.set_setup_state = i915_set_setup_state;
   i915->pipe.set_stencil_state = i915_set_stencil_state;
   i915->pipe.set_texture_state = i915_set_texture_state;
   i915->pipe.set_viewport_state = i915_set_viewport_state;
   i915->pipe.set_vertex_buffer = i915_set_vertex_buffer;
   i915->pipe.set_vertex_element = i915_set_vertex_element;
}
