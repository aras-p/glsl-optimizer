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
#include <util/u_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include <util/u_memory.h>
#include "vl_csc.h"
#include "vl_shader_build.h"

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

static void
create_vert_shader(struct vl_compositor *c)
{
   const unsigned max_tokens = 50;

   struct pipe_shader_state vs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(c);

   tokens = (struct tgsi_token*)MALLOC(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header*)&tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor*)&tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

   ti = 2;

   /*
    * decl i0             ; Vertex pos
    * decl i1             ; Vertex texcoords
    */
   for (i = 0; i < 2; i++) {
      decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * decl c0             ; Scaling vector to scale vertex pos rect to destination size
    * decl c1             ; Translation vector to move vertex pos rect into position
    * decl c2             ; Scaling vector to scale texcoord rect to source size
    * decl c3             ; Translation vector to move texcoord rect into position
    */
   decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 3);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * decl o0             ; Vertex pos
    * decl o1             ; Vertex texcoords
    */
   for (i = 0; i < 2; i++) {
      decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /* decl t0, t1 */
   decl = vl_decl_temps(0, 1);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * mad o0, i0, c0, c1  ; Scale and translate unit output rect to destination size and pos
    * mad o1, i1, c2, c3  ; Scale and translate unit texcoord rect to source size and pos
    */
   for (i = 0; i < 2; ++i) {
      inst = vl_inst4(TGSI_OPCODE_MAD, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i, TGSI_FILE_CONSTANT, i * 2, TGSI_FILE_CONSTANT, i * 2 + 1);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   vs.tokens = tokens;
   c->vertex_shader = c->pipe->create_vs_state(c->pipe, &vs);
   FREE(tokens);
}

static void
create_frag_shader(struct vl_compositor *c)
{
   const unsigned max_tokens = 50;

   struct pipe_shader_state fs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(c);

   tokens = (struct tgsi_token*)MALLOC(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header*)&tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor*)&tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

   ti = 2;

   /* decl i0             ; Texcoords for s0 */
   decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, 1, 0, 0, TGSI_INTERPOLATE_LINEAR);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * decl c0-c3          ; CSC matrix c0-c3
    */
   decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 3);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl o0             ; Fragment color */
   decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl t0 */
   decl = vl_decl_temps(0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl s0             ; Sampler for tex containing picture to display */
   decl = vl_decl_samplers(0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* tex2d t0, i0, s0    ; Read src pixel */
   inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, 0);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /*
    * dp4 o0.x, t0, c0    ; Multiply pixel by the color conversion matrix
    * dp4 o0.y, t0, c1
    * dp4 o0.z, t0, c2
    * dp4 o0.w, t0, c3
    */
   for (i = 0; i < 4; ++i) {
      inst = vl_inst3(TGSI_OPCODE_DP4, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i);
      inst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X << i;
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
   assert(ti <= max_tokens);

   fs.tokens = tokens;
   c->fragment_shader = c->pipe->create_fs_state(c->pipe, &fs);
   FREE(tokens);
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
   c->vertex_elems[0].instance_divisor = 0;
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
   c->vertex_elems[1].instance_divisor = 0;
   c->vertex_elems[1].vertex_buffer_index = 1;
   c->vertex_elems[1].nr_components = 2;
   c->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /*
    * Create our vertex shader's constant buffer
    * Const buffer contains scaling and translation vectors
    */
   c->vs_const_buf = pipe_buffer_create
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
   c->fs_const_buf = pipe_buffer_create
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

   pipe_buffer_reference(&c->vs_const_buf, NULL);
   pipe_buffer_reference(&c->fs_const_buf, NULL);
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

   compositor->fb_state.width = dst_surface->width0;
   compositor->fb_state.height = dst_surface->height0;
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

   compositor->scissor.maxx = compositor->fb_state.width;
   compositor->scissor.maxy = compositor->fb_state.height;

   compositor->pipe->set_framebuffer_state(compositor->pipe, &compositor->fb_state);
   compositor->pipe->set_viewport_state(compositor->pipe, &compositor->viewport);
   compositor->pipe->set_scissor_state(compositor->pipe, &compositor->scissor);
   compositor->pipe->bind_fragment_sampler_states(compositor->pipe, 1, &compositor->sampler);
   compositor->pipe->set_fragment_sampler_textures(compositor->pipe, 1, &src_surface);
   compositor->pipe->bind_vs_state(compositor->pipe, compositor->vertex_shader);
   compositor->pipe->bind_fs_state(compositor->pipe, compositor->fragment_shader);
   compositor->pipe->set_vertex_buffers(compositor->pipe, 2, compositor->vertex_bufs);
   compositor->pipe->set_vertex_elements(compositor->pipe, 2, compositor->vertex_elems);
   compositor->pipe->set_constant_buffer(compositor->pipe, PIPE_SHADER_VERTEX, 0, compositor->vs_const_buf);
   compositor->pipe->set_constant_buffer(compositor->pipe, PIPE_SHADER_FRAGMENT, 0, compositor->fs_const_buf);

   vs_consts = pipe_buffer_map
   (
      compositor->pipe->screen,
      compositor->vs_const_buf,
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

   vs_consts->src_scale.x = src_area->w / (float)src_surface->width0;
   vs_consts->src_scale.y = src_area->h / (float)src_surface->height0;
   vs_consts->src_scale.z = 1;
   vs_consts->src_scale.w = 1;
   vs_consts->src_trans.x = src_area->x / (float)src_surface->width0;
   vs_consts->src_trans.y = src_area->y / (float)src_surface->height0;
   vs_consts->src_trans.z = 0;
   vs_consts->src_trans.w = 0;

   pipe_buffer_unmap(compositor->pipe->screen, compositor->vs_const_buf);

   compositor->pipe->draw_arrays(compositor->pipe, PIPE_PRIM_TRIANGLE_STRIP, 0, 4);
   compositor->pipe->flush(compositor->pipe, PIPE_FLUSH_RENDER_CACHE, fence);

   pipe_surface_reference(&compositor->fb_state.cbufs[0], NULL);
}

void vl_compositor_set_csc_matrix(struct vl_compositor *compositor, const float *mat)
{
   assert(compositor);

   memcpy
   (
      pipe_buffer_map(compositor->pipe->screen, compositor->fs_const_buf, PIPE_BUFFER_USAGE_CPU_WRITE),
      mat,
      sizeof(struct fragment_shader_consts)
   );

   pipe_buffer_unmap(compositor->pipe->screen, compositor->fs_const_buf);
}
