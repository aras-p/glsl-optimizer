/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include "vl_compositor.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_ureg.h>
#include <util/u_memory.h>
#include "vl_csc.h"

struct vertex2f
{
   float x, y;
};

struct vertex4f
{
   float x, y, z, w;
};

struct vertex_shader_consts
{
   struct vertex4f dst_scale;
   struct vertex4f dst_trans;
   struct vertex4f src_scale;
   struct vertex4f src_trans;
};

struct fragment_shader_consts
{
   float matrix[16];
};

/*
 * Represents 2 triangles in a strip in normalized coords.
 * Used to render the surface onto the frame buffer.
 */
static const struct vertex2f surface_verts[4] =
{
   {0.0f, 0.0f},
   {0.0f, 1.0f},
   {1.0f, 0.0f},
   {1.0f, 1.0f}
};

/*
 * Represents texcoords for the above. We can use the position values directly.
 * TODO: Duplicate these in the shader, no need to create a buffer.
 */
static const struct vertex2f *surface_texcoords = surface_verts;

static bool
create_vert_shader(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src vpos, vtex;
   struct ureg_src vpos_scale, vpos_trans, vtex_scale, vtex_trans;
   struct ureg_dst o_vpos, o_vtex;
   
   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return false;

   vpos = ureg_DECL_vs_input(shader, 0);
   vtex = ureg_DECL_vs_input(shader, 1);
   vpos_scale = ureg_DECL_constant(shader, 0);
   vpos_trans = ureg_DECL_constant(shader, 1);
   vtex_scale = ureg_DECL_constant(shader, 2);
   vtex_trans = ureg_DECL_constant(shader, 3);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, 0);
   o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, 1);

   /*
    * o_vpos = vpos * vpos_scale + vpos_trans
    * o_vtex = vtex * vtex_scale + vtex_trans
    */
   ureg_MAD(shader, o_vpos, vpos, vpos_scale, vpos_trans);
   ureg_MAD(shader, o_vtex, vtex, vtex_scale, vtex_trans);

   ureg_END(shader);

   c->vertex_shader = ureg_create_shader_and_destroy(shader, c->pipe);
   if (!c->vertex_shader)
      return false;

   return true;
}

static bool
create_frag_shader(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src tc;
   struct ureg_src csc[4];
   struct ureg_src sampler;
   struct ureg_dst texel;
   struct ureg_dst fragment;
   unsigned i;
   
   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return false;

   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 1, TGSI_INTERPOLATE_LINEAR);
   for (i = 0; i < 4; ++i)
      csc[i] = ureg_DECL_constant(shader, i);
   sampler = ureg_DECL_sampler(shader, 0);
   texel = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel = tex(tc, sampler)
    * fragment = csc * texel
    */
   ureg_TEX(shader, texel, TGSI_TEXTURE_2D, tc, sampler);
   for (i = 0; i < 4; ++i)
      ureg_DP4(shader, ureg_writemask(fragment, TGSI_WRITEMASK_X << i), csc[i], ureg_src(texel));

   ureg_release_temporary(shader, texel);
   ureg_END(shader);

   c->fragment_shader = ureg_create_shader_and_destroy(shader, c->pipe);
   if (!c->fragment_shader)
      return false;

   return true;
}

static bool
init_pipe_state(struct vl_compositor *c)
{
   struct pipe_sampler_state sampler;

   assert(c);

   c->fb_state.nr_cbufs = 1;
   c->fb_state.zsbuf = NULL;

   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   /*sampler.prefilter = ;*/
   /*sampler.lod_bias = ;*/
   /*sampler.min_lod = ;*/
   /*sampler.max_lod = ;*/
   /*sampler.border_color[i] = ;*/
   /*sampler.max_anisotropy = ;*/
   c->sampler = c->pipe->create_sampler_state(c->pipe, &sampler);
	
   return true;
}

static void cleanup_pipe_state(struct vl_compositor *c)
{
   assert(c);
	
   c->pipe->delete_sampler_state(c->pipe, c->sampler);
}

static bool
init_shaders(struct vl_compositor *c)
{
   assert(c);

   create_vert_shader(c);
   create_frag_shader(c);

   return true;
}

static void cleanup_shaders(struct vl_compositor *c)
{
   assert(c);
	
   c->pipe->delete_vs_state(c->pipe, c->vertex_shader);
   c->pipe->delete_fs_state(c->pipe, c->fragment_shader);
}

static bool
init_buffers(struct vl_compositor *c)
{
   struct fragment_shader_consts fsc;

   assert(c);
	
   /*
    * Create our vertex buffer and vertex buffer element
    * VB contains 4 vertices that render a quad covering the entire window
    * to display a rendered surface
    * Quad is rendered as a tri strip
    */
   c->vertex_bufs[0].stride = sizeof(struct vertex2f);
   c->vertex_bufs[0].max_index = 3;
   c->vertex_bufs[0].buffer_offset = 0;
   c->vertex_bufs[0].buffer = pipe_buffer_create
   (
      c->pipe->screen,
      1,
      PIPE_BUFFER_USAGE_VERTEX,
      sizeof(struct vertex2f) * 4
   );

   memcpy
   (
      pipe_buffer_map(c->pipe->screen, c->vertex_bufs[0].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
      surface_verts,
      sizeof(struct vertex2f) * 4
   );

   pipe_buffer_unmap(c->pipe->screen, c->vertex_bufs[0].buffer);

   c->vertex_elems[0].src_offset = 0;
   c->vertex_elems[0].vertex_buffer_index = 0;
   c->vertex_elems[0].nr_components = 2;
   c->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /*
    * Create our texcoord buffer and texcoord buffer element
    * Texcoord buffer contains the TCs for mapping the rendered surface to the 4 vertices
    */
   c->vertex_bufs[1].stride = sizeof(struct vertex2f);
   c->vertex_bufs[1].max_index = 3;
   c->vertex_bufs[1].buffer_offset = 0;
   c->vertex_bufs[1].buffer = pipe_buffer_create
   (
      c->pipe->screen,
      1,
      PIPE_BUFFER_USAGE_VERTEX,
      sizeof(struct vertex2f) * 4
   );

   memcpy
   (
      pipe_buffer_map(c->pipe->screen, c->vertex_bufs[1].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
      surface_texcoords,
      sizeof(struct vertex2f) * 4
   );

   pipe_buffer_unmap(c->pipe->screen, c->vertex_bufs[1].buffer);

   c->vertex_elems[1].src_offset = 0;
   c->vertex_elems[1].vertex_buffer_index = 1;
   c->vertex_elems[1].nr_components = 2;
   c->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /*
    * Create our vertex shader's constant buffer
    * Const buffer contains scaling and translation vectors
    */
   c->vs_const_buf.buffer = pipe_buffer_create
   (
      c->pipe->screen,
      1,
      PIPE_BUFFER_USAGE_CONSTANT | PIPE_BUFFER_USAGE_DISCARD,
      sizeof(struct vertex_shader_consts)
   );

   /*
    * Create our fragment shader's constant buffer
    * Const buffer contains the color conversion matrix and bias vectors
    */
   c->fs_const_buf.buffer = pipe_buffer_create
   (
      c->pipe->screen,
      1,
      PIPE_BUFFER_USAGE_CONSTANT,
      sizeof(struct fragment_shader_consts)
   );

   vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_IDENTITY, NULL, true, fsc.matrix);

   vl_compositor_set_csc_matrix(c, fsc.matrix);

   return true;
}

static void
cleanup_buffers(struct vl_compositor *c)
{
   unsigned i;

   assert(c);
	
   for (i = 0; i < 2; ++i)
      pipe_buffer_reference(&c->vertex_bufs[i].buffer, NULL);

   pipe_buffer_reference(&c->vs_const_buf.buffer, NULL);
   pipe_buffer_reference(&c->fs_const_buf.buffer, NULL);
}

bool vl_compositor_init(struct vl_compositor *compositor, struct pipe_context *pipe)
{
   assert(compositor);

   memset(compositor, 0, sizeof(struct vl_compositor));

   compositor->pipe = pipe;

   if (!init_pipe_state(compositor))
      return false;
   if (!init_shaders(compositor)) {
      cleanup_pipe_state(compositor);
      return false;
   }
   if (!init_buffers(compositor)) {
      cleanup_shaders(compositor);
      cleanup_pipe_state(compositor);
      return false;
   }

   return true;
}

void vl_compositor_cleanup(struct vl_compositor *compositor)
{
   assert(compositor);
	
   cleanup_buffers(compositor);
   cleanup_shaders(compositor);
   cleanup_pipe_state(compositor);
}

void vl_compositor_render(struct vl_compositor          *compositor,
                          /*struct pipe_texture         *backround,
                          struct pipe_video_rect        *backround_area,*/
                          struct pipe_texture           *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_texture           *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_texture           *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_texture           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          /*unsigned                      num_layers,
                          struct pipe_texture           *layers,
                          struct pipe_video_rect        *layer_src_areas,
                          struct pipe_video_rect        *layer_dst_areas*/
                          struct pipe_fence_handle      **fence)
{
   struct vertex_shader_consts *vs_consts;

   assert(compositor);
   assert(src_surface);
   assert(src_area);
   assert(dst_surface);
   assert(dst_area);
   assert(picture_type == PIPE_MPEG12_PICTURE_TYPE_FRAME);

   compositor->fb_state.width = dst_surface->width[0];
   compositor->fb_state.height = dst_surface->height[0];
   compositor->fb_state.cbufs[0] = compositor->pipe->screen->get_tex_surface
   (
      compositor->pipe->screen,
      dst_surface,
      0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
   );

   compositor->viewport.scale[0] = compositor->fb_state.width;
   compositor->viewport.scale[1] = compositor->fb_state.height;
   compositor->viewport.scale[2] = 1;
   compositor->viewport.scale[3] = 1;
   compositor->viewport.translate[0] = 0;
   compositor->viewport.translate[1] = 0;
   compositor->viewport.translate[2] = 0;
   compositor->viewport.translate[3] = 0;

   compositor->pipe->set_framebuffer_state(compositor->pipe, &compositor->fb_state);
   compositor->pipe->set_viewport_state(compositor->pipe, &compositor->viewport);
   compositor->pipe->bind_sampler_states(compositor->pipe, 1, &compositor->sampler);
   compositor->pipe->set_sampler_textures(compositor->pipe, 1, &src_surface);
   compositor->pipe->bind_vs_state(compositor->pipe, compositor->vertex_shader);
   compositor->pipe->bind_fs_state(compositor->pipe, compositor->fragment_shader);
   compositor->pipe->set_vertex_buffers(compositor->pipe, 2, compositor->vertex_bufs);
   compositor->pipe->set_vertex_elements(compositor->pipe, 2, compositor->vertex_elems);
   compositor->pipe->set_constant_buffer(compositor->pipe, PIPE_SHADER_VERTEX, 0, &compositor->vs_const_buf);
   compositor->pipe->set_constant_buffer(compositor->pipe, PIPE_SHADER_FRAGMENT, 0, &compositor->fs_const_buf);

   vs_consts = pipe_buffer_map
   (
      compositor->pipe->screen,
      compositor->vs_const_buf.buffer,
      PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
   );

   vs_consts->dst_scale.x = dst_area->w / (float)compositor->fb_state.cbufs[0]->width;
   vs_consts->dst_scale.y = dst_area->h / (float)compositor->fb_state.cbufs[0]->height;
   vs_consts->dst_scale.z = 1;
   vs_consts->dst_scale.w = 1;
   vs_consts->dst_trans.x = dst_area->x / (float)compositor->fb_state.cbufs[0]->width;
   vs_consts->dst_trans.y = dst_area->y / (float)compositor->fb_state.cbufs[0]->height;
   vs_consts->dst_trans.z = 0;
   vs_consts->dst_trans.w = 0;

   vs_consts->src_scale.x = src_area->w / (float)src_surface->width[0];
   vs_consts->src_scale.y = src_area->h / (float)src_surface->height[0];
   vs_consts->src_scale.z = 1;
   vs_consts->src_scale.w = 1;
   vs_consts->src_trans.x = src_area->x / (float)src_surface->width[0];
   vs_consts->src_trans.y = src_area->y / (float)src_surface->height[0];
   vs_consts->src_trans.z = 0;
   vs_consts->src_trans.w = 0;

   pipe_buffer_unmap(compositor->pipe->screen, compositor->vs_const_buf.buffer);

   compositor->pipe->draw_arrays(compositor->pipe, PIPE_PRIM_TRIANGLE_STRIP, 0, 4);
   compositor->pipe->flush(compositor->pipe, PIPE_FLUSH_RENDER_CACHE, fence);

   pipe_surface_reference(&compositor->fb_state.cbufs[0], NULL);
}

void vl_compositor_set_csc_matrix(struct vl_compositor *compositor, const float *mat)
{
   assert(compositor);

   memcpy
   (
      pipe_buffer_map(compositor->pipe->screen, compositor->fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
      mat,
      sizeof(struct fragment_shader_consts)
   );

   pipe_buffer_unmap(compositor->pipe->screen, compositor->fs_const_buf.buffer);
}
