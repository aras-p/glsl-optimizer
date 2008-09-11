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

/* Authors:  Zack Rusin <zack@tungstengraphics.com>
 *           Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_winsys.h"
#include "util/u_memory.h"
#include "pipe/p_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"


#define DUP( TYPE, VAL )                        \
do {                                            \
   struct TYPE *x = malloc(sizeof(*x));         \
   memcpy(x, VAL, sizeof(*x) );                 \
   return x;                                    \
} while (0)

/************************************************************************
 * Blend 
 */
static void *
brw_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *blend)
{   
   DUP( pipe_blend_state, blend );
}

static void brw_bind_blend_state(struct pipe_context *pipe,
                                 void *blend)
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.Blend = (struct pipe_blend_state*)blend;
   brw->state.dirty.brw |= BRW_NEW_BLEND;
}


static void brw_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   free(blend);
}

static void brw_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.BlendColor = *blend_color;

   brw->state.dirty.brw |= BRW_NEW_BLEND;
}

/************************************************************************
 * Sampler 
 */

static void *
brw_create_sampler_state(struct pipe_context *pipe,
                          const struct pipe_sampler_state *sampler)
{
   DUP( pipe_sampler_state, sampler );
}

static void brw_bind_sampler_states(struct pipe_context *pipe,
                                    unsigned num, void **sampler)
{
   struct brw_context *brw = brw_context(pipe);

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == brw->num_samplers &&
       !memcmp(brw->attribs.Samplers, sampler, num * sizeof(void *)))
      return;

   memcpy(brw->attribs.Samplers, sampler, num * sizeof(void *));
   memset(&brw->attribs.Samplers[num], 0, (PIPE_MAX_SAMPLERS - num) *
          sizeof(void *));

   brw->num_samplers = num;

   brw->state.dirty.brw |= BRW_NEW_SAMPLER;
}

static void brw_delete_sampler_state(struct pipe_context *pipe,
                                      void *sampler)
{
   free(sampler);
}


/************************************************************************
 * Depth stencil 
 */

static void *
brw_create_depth_stencil_state(struct pipe_context *pipe,
                           const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   DUP( pipe_depth_stencil_alpha_state, depth_stencil );
}

static void brw_bind_depth_stencil_state(struct pipe_context *pipe,
                                         void *depth_stencil)
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.DepthStencil = (const struct pipe_depth_stencil_alpha_state *)depth_stencil;

   brw->state.dirty.brw |= BRW_NEW_DEPTH_STENCIL;
}

static void brw_delete_depth_stencil_state(struct pipe_context *pipe,
                                           void *depth_stencil)
{
   free(depth_stencil);
}

/************************************************************************
 * Scissor
 */
static void brw_set_scissor_state( struct pipe_context *pipe,
                                 const struct pipe_scissor_state *scissor )
{
   struct brw_context *brw = brw_context(pipe);

   memcpy( &brw->attribs.Scissor, scissor, sizeof(*scissor) );
   brw->state.dirty.brw |= BRW_NEW_SCISSOR;
}


/************************************************************************
 * Stipple
 */

static void brw_set_polygon_stipple( struct pipe_context *pipe,
                                   const struct pipe_poly_stipple *stipple )
{
}


/************************************************************************
 * Fragment shader
 */

static void * brw_create_fs_state(struct pipe_context *pipe,
                                   const struct pipe_shader_state *shader)
{
   struct brw_fragment_program *brw_fp = CALLOC_STRUCT(brw_fragment_program);

   brw_fp->program.tokens = tgsi_dup_tokens(shader->tokens);
   brw_fp->id = brw_context(pipe)->program_id++;

   tgsi_scan_shader(shader->tokens, &brw_fp->info);

#if 0
   brw_shader_info(shader->tokens,
		   &brw_fp->info2);
#endif

   tgsi_dump(shader->tokens, 0);


   return (void *)brw_fp;
}

static void brw_bind_fs_state(struct pipe_context *pipe, void *shader)
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.FragmentProgram = (struct brw_fragment_program *)shader;
   brw->state.dirty.brw |= BRW_NEW_FS;
}

static void brw_delete_fs_state(struct pipe_context *pipe, void *shader)
{
   struct brw_fragment_program *brw_fp = (struct brw_fragment_program *) shader;

   FREE((void *) brw_fp->program.tokens);
   FREE(brw_fp);
}


/************************************************************************
 * Vertex shader and other TNL state 
 */

static void *brw_create_vs_state(struct pipe_context *pipe,
                                 const struct pipe_shader_state *shader)
{
   struct brw_vertex_program *brw_vp = CALLOC_STRUCT(brw_vertex_program);

   brw_vp->program.tokens = tgsi_dup_tokens(shader->tokens);
   brw_vp->id = brw_context(pipe)->program_id++;

   tgsi_scan_shader(shader->tokens, &brw_vp->info);

#if 0
   brw_shader_info(shader->tokens,
		   &brw_vp->info2);
#endif
   tgsi_dump(shader->tokens, 0);

   return (void *)brw_vp;
}

static void brw_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.VertexProgram = (struct brw_vertex_program *)vs;
   brw->state.dirty.brw |= BRW_NEW_VS;

   debug_printf("YYYYYYYYYYYYY BINDING VERTEX SHADER\n");
}

static void brw_delete_vs_state(struct pipe_context *pipe, void *shader)
{
   struct brw_vertex_program *brw_vp = (struct brw_vertex_program *) shader;

   FREE((void *) brw_vp->program.tokens);
   FREE(brw_vp);
}


static void brw_set_clip_state( struct pipe_context *pipe,
                                const struct pipe_clip_state *clip )
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.Clip = *clip;
}


static void brw_set_viewport_state( struct pipe_context *pipe,
				     const struct pipe_viewport_state *viewport )
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.Viewport = *viewport; /* struct copy */
   brw->state.dirty.brw |= BRW_NEW_VIEWPORT;

   /* pass the viewport info to the draw module */
   //draw_set_viewport_state(brw->draw, viewport);
}


static void brw_set_vertex_buffers(struct pipe_context *pipe,
				   unsigned count,
				   const struct pipe_vertex_buffer *buffers)
{
   struct brw_context *brw = brw_context(pipe);
   memcpy(brw->vb.vbo_array, buffers, count * sizeof(buffers[0]));
}

static void brw_set_vertex_elements(struct pipe_context *pipe,
                                    unsigned count,
                                    const struct pipe_vertex_element *elements)
{
   /* flush ? */
   struct brw_context *brw = brw_context(pipe);
   uint i;

   assert(count <= PIPE_MAX_ATTRIBS);

   for (i = 0; i < count; i++) {
      struct brw_vertex_element_state el;
      memset(&el, 0, sizeof(el));

      el.ve0.src_offset = elements[i].src_offset;
      el.ve0.src_format = brw_translate_surface_format(elements[i].src_format);
      el.ve0.valid = 1;
      el.ve0.vertex_buffer_index = elements[i].vertex_buffer_index;

      el.ve1.dst_offset   = i * 4;

      el.ve1.vfcomponent3 = BRW_VFCOMPONENT_STORE_SRC;
      el.ve1.vfcomponent2 = BRW_VFCOMPONENT_STORE_SRC;
      el.ve1.vfcomponent1 = BRW_VFCOMPONENT_STORE_SRC;
      el.ve1.vfcomponent0 = BRW_VFCOMPONENT_STORE_SRC;

      switch (elements[i].nr_components) {
      case 1: el.ve1.vfcomponent1 = BRW_VFCOMPONENT_STORE_0;
      case 2: el.ve1.vfcomponent2 = BRW_VFCOMPONENT_STORE_0;
      case 3: el.ve1.vfcomponent3 = BRW_VFCOMPONENT_STORE_1_FLT;
         break;
      }

      brw->vb.inputs[i] = el;
   }
}



/************************************************************************
 * Constant buffers
 */

static void brw_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     const struct pipe_constant_buffer *buf)
{
   struct brw_context *brw = brw_context(pipe);

   assert(buf == 0 || index == 0);

   brw->attribs.Constants[shader] = buf;
   brw->state.dirty.brw |= BRW_NEW_CONSTANTS;
}


/************************************************************************
 * Texture surfaces
 */


static void brw_set_sampler_textures(struct pipe_context *pipe,
                                     unsigned num,
                                     struct pipe_texture **texture)
{
   struct brw_context *brw = brw_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == brw->num_textures &&
       !memcmp(brw->attribs.Texture, texture, num *
               sizeof(struct pipe_texture *)))
      return;

   for (i = 0; i < num; i++)
      pipe_texture_reference((struct pipe_texture **) &brw->attribs.Texture[i],
                             texture[i]);

   for (i = num; i < brw->num_textures; i++)
      pipe_texture_reference((struct pipe_texture **) &brw->attribs.Texture[i],
                             NULL);

   brw->num_textures = num;

   brw->state.dirty.brw |= BRW_NEW_TEXTURE;
}


/************************************************************************
 * Render targets, etc
 */

static void brw_set_framebuffer_state(struct pipe_context *pipe,
				       const struct pipe_framebuffer_state *fb)
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.FrameBuffer = *fb; /* struct copy */

   brw->state.dirty.brw |= BRW_NEW_FRAMEBUFFER;
}



/************************************************************************
 * Rasterizer state
 */

static void *
brw_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *rasterizer)
{
   DUP(pipe_rasterizer_state, rasterizer);
}

static void brw_bind_rasterizer_state( struct pipe_context *pipe,
                                        void *setup )
{
   struct brw_context *brw = brw_context(pipe);

   brw->attribs.Raster = (struct pipe_rasterizer_state *)setup;

   /* Also pass-through to draw module:
    */
   //draw_set_rasterizer_state(brw->draw, setup);

   brw->state.dirty.brw |= BRW_NEW_RASTERIZER;
}

static void brw_delete_rasterizer_state(struct pipe_context *pipe,
                                         void *setup)
{
   free(setup);
}



void
brw_init_state_functions( struct brw_context *brw )
{
   brw->pipe.create_blend_state = brw_create_blend_state;
   brw->pipe.bind_blend_state = brw_bind_blend_state;
   brw->pipe.delete_blend_state = brw_delete_blend_state;

   brw->pipe.create_sampler_state = brw_create_sampler_state;
   brw->pipe.bind_sampler_states = brw_bind_sampler_states;
   brw->pipe.delete_sampler_state = brw_delete_sampler_state;

   brw->pipe.create_depth_stencil_alpha_state = brw_create_depth_stencil_state;
   brw->pipe.bind_depth_stencil_alpha_state = brw_bind_depth_stencil_state;
   brw->pipe.delete_depth_stencil_alpha_state = brw_delete_depth_stencil_state;

   brw->pipe.create_rasterizer_state = brw_create_rasterizer_state;
   brw->pipe.bind_rasterizer_state = brw_bind_rasterizer_state;
   brw->pipe.delete_rasterizer_state = brw_delete_rasterizer_state;
   brw->pipe.create_fs_state = brw_create_fs_state;
   brw->pipe.bind_fs_state = brw_bind_fs_state;
   brw->pipe.delete_fs_state = brw_delete_fs_state;
   brw->pipe.create_vs_state = brw_create_vs_state;
   brw->pipe.bind_vs_state = brw_bind_vs_state;
   brw->pipe.delete_vs_state = brw_delete_vs_state;

   brw->pipe.set_blend_color = brw_set_blend_color;
   brw->pipe.set_clip_state = brw_set_clip_state;
   brw->pipe.set_constant_buffer = brw_set_constant_buffer;
   brw->pipe.set_framebuffer_state = brw_set_framebuffer_state;

//   brw->pipe.set_feedback_state = brw_set_feedback_state;
//   brw->pipe.set_feedback_buffer = brw_set_feedback_buffer;

   brw->pipe.set_polygon_stipple = brw_set_polygon_stipple;
   brw->pipe.set_scissor_state = brw_set_scissor_state;
   brw->pipe.set_sampler_textures = brw_set_sampler_textures;
   brw->pipe.set_viewport_state = brw_set_viewport_state;
   brw->pipe.set_vertex_buffers = brw_set_vertex_buffers;
   brw->pipe.set_vertex_elements = brw_set_vertex_elements;
}
