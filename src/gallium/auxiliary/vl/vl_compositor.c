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
#include "util/u_draw.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_keymap.h>
#include <util/u_draw.h>
#include <util/u_sampler.h>
#include <tgsi/tgsi_ureg.h>
#include "vl_csc.h"
#include "vl_types.h"

typedef float csc_matrix[16];

static void *
create_vert_shader(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src vpos, vtex;
   struct ureg_dst o_vpos, o_vtex;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return false;

   vpos = ureg_DECL_vs_input(shader, 0);
   vtex = ureg_DECL_vs_input(shader, 1);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, 0);
   o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, 1);

   /*
    * o_vpos = vpos
    * o_vtex = vtex
    */
   ureg_MOV(shader, o_vpos, vpos);
   ureg_MOV(shader, o_vtex, vtex);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, c->pipe);
}

static void *
create_frag_shader_video_buffer(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src tc;
   struct ureg_src csc[3];
   struct ureg_src sampler[3];
   struct ureg_dst texel;
   struct ureg_dst fragment;
   unsigned i;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return false;

   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 1, TGSI_INTERPOLATE_LINEAR);
   for (i = 0; i < 3; ++i) {
      csc[i] = ureg_DECL_constant(shader, i);
      sampler[i] = ureg_DECL_sampler(shader, i);
   }
   texel = ureg_DECL_temporary(shader);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * texel.xyz = tex(tc, sampler[i])
    * fragment = csc * texel
    */
   for (i = 0; i < 3; ++i)
      ureg_TEX(shader, ureg_writemask(texel, TGSI_WRITEMASK_X << i), TGSI_TEXTURE_2D, tc, sampler[i]);

   ureg_MOV(shader, ureg_writemask(texel, TGSI_WRITEMASK_W), ureg_imm1f(shader, 1.0f));

   for (i = 0; i < 3; ++i)
      ureg_DP4(shader, ureg_writemask(fragment, TGSI_WRITEMASK_X << i), csc[i], ureg_src(texel));

   ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W), ureg_imm1f(shader, 1.0f));

   ureg_release_temporary(shader, texel);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, c->pipe);
}

static void *
create_frag_shader_palette(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src tc;
   struct ureg_src sampler;
   struct ureg_src palette;
   struct ureg_dst texel;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return false;

   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 1, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   palette = ureg_DECL_sampler(shader, 1);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);
   texel = ureg_DECL_temporary(shader);

   /*
    * texel = tex(tc, sampler)
    * fragment.xyz = tex(texel, palette)
    * fragment.a = texel.a
    */
   ureg_TEX(shader, texel, TGSI_TEXTURE_2D, tc, sampler);
   ureg_TEX(shader, fragment, TGSI_TEXTURE_1D, ureg_src(texel), palette);
   ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W), ureg_src(texel));

   ureg_release_temporary(shader, texel);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, c->pipe);
}

static void *
create_frag_shader_rgba(struct vl_compositor *c)
{
   struct ureg_program *shader;
   struct ureg_src tc;
   struct ureg_src sampler;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return false;

   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, 1, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * fragment = tex(tc, sampler)
    */
   ureg_TEX(shader, fragment, TGSI_TEXTURE_2D, tc, sampler);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, c->pipe);
}

static bool
init_shaders(struct vl_compositor *c)
{
   assert(c);

   c->vs = create_vert_shader(c);
   if (!c->vs) {
      debug_printf("Unable to create vertex shader.\n");
      return false;
   }

   c->fs_video_buffer = create_frag_shader_video_buffer(c);
   if (!c->fs_video_buffer) {
      debug_printf("Unable to create YCbCr-to-RGB fragment shader.\n");
      return false;
   }

   c->fs_palette = create_frag_shader_palette(c);
   if (!c->fs_palette) {
      debug_printf("Unable to create Palette-to-RGB fragment shader.\n");
      return false;
   }

   c->fs_rgba = create_frag_shader_rgba(c);
   if (!c->fs_rgba) {
      debug_printf("Unable to create RGB-to-RGB fragment shader.\n");
      return false;
   }

   return true;
}

static void cleanup_shaders(struct vl_compositor *c)
{
   assert(c);

   c->pipe->delete_vs_state(c->pipe, c->vs);
   c->pipe->delete_fs_state(c->pipe, c->fs_video_buffer);
   c->pipe->delete_fs_state(c->pipe, c->fs_palette);
   c->pipe->delete_fs_state(c->pipe, c->fs_rgba);
}

static bool
init_pipe_state(struct vl_compositor *c)
{
   struct pipe_sampler_state sampler;
   struct pipe_blend_state blend;

   assert(c);

   c->fb_state.nr_cbufs = 1;
   c->fb_state.zsbuf = NULL;

   c->viewport.scale[2] = 1;
   c->viewport.scale[3] = 1;
   c->viewport.translate[0] = 0;
   c->viewport.translate[1] = 0;
   c->viewport.translate[2] = 0;
   c->viewport.translate[3] = 0;

   memset(&sampler, 0, sizeof(sampler));
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

   memset(&blend, 0, sizeof blend);
   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   c->blend = c->pipe->create_blend_state(c->pipe, &blend);

   return true;
}

static void cleanup_pipe_state(struct vl_compositor *c)
{
   assert(c);

   c->pipe->delete_sampler_state(c->pipe, c->sampler);
   c->pipe->delete_blend_state(c->pipe, c->blend);
}

static bool
init_buffers(struct vl_compositor *c)
{
   struct pipe_vertex_element vertex_elems[2];

   assert(c);

   /*
    * Create our vertex buffer and vertex buffer elements
    */
   c->vertex_buf.stride = sizeof(struct vertex4f);
   c->vertex_buf.buffer_offset = 0;
   c->vertex_buf.buffer = pipe_buffer_create
   (
      c->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STREAM,
      sizeof(struct vertex4f) * (VL_COMPOSITOR_MAX_LAYERS + 1) * 4
   );

   vertex_elems[0].src_offset = 0;
   vertex_elems[0].instance_divisor = 0;
   vertex_elems[0].vertex_buffer_index = 0;
   vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
   vertex_elems[1].src_offset = sizeof(struct vertex2f);
   vertex_elems[1].instance_divisor = 0;
   vertex_elems[1].vertex_buffer_index = 0;
   vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;
   c->vertex_elems_state = c->pipe->create_vertex_elements_state(c->pipe, 2, vertex_elems);

   /*
    * Create our fragment shader's constant buffer
    * Const buffer contains the color conversion matrix and bias vectors
    */
   /* XXX: Create with IMMUTABLE/STATIC... although it does change every once in a long while... */
   c->csc_matrix = pipe_buffer_create
   (
      c->pipe->screen,
      PIPE_BIND_CONSTANT_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(csc_matrix)
   );

   return true;
}

static void
cleanup_buffers(struct vl_compositor *c)
{
   assert(c);

   c->pipe->delete_vertex_elements_state(c->pipe, c->vertex_elems_state);
   pipe_resource_reference(&c->vertex_buf.buffer, NULL);
   pipe_resource_reference(&c->csc_matrix, NULL);
}

static inline struct pipe_video_rect
default_rect(struct vl_compositor_layer *layer)
{
   struct pipe_resource *res = layer->sampler_views[0]->texture;
   struct pipe_video_rect rect = { 0, 0, res->width0, res->height0 };
   return rect;
}

static void
gen_rect_verts(struct vertex4f *vb,
               struct pipe_video_rect *src_rect,
               struct vertex2f *src_inv_size,
               struct pipe_video_rect *dst_rect,
               struct vertex2f *dst_inv_size)
{
   assert(vb);
   assert(src_rect && src_inv_size);
   assert(dst_rect && dst_inv_size);

   vb[0].x = dst_rect->x * dst_inv_size->x;
   vb[0].y = dst_rect->y * dst_inv_size->y;
   vb[0].z = src_rect->x * src_inv_size->x;
   vb[0].w = src_rect->y * src_inv_size->y;

   vb[1].x = (dst_rect->x + dst_rect->w) * dst_inv_size->x;
   vb[1].y = dst_rect->y * dst_inv_size->y;
   vb[1].z = (src_rect->x + src_rect->w) * src_inv_size->x;
   vb[1].w = src_rect->y * src_inv_size->y;

   vb[2].x = (dst_rect->x + dst_rect->w) * dst_inv_size->x;
   vb[2].y = (dst_rect->y + dst_rect->h) * dst_inv_size->y;
   vb[2].z = (src_rect->x + src_rect->w) * src_inv_size->x;
   vb[2].w = (src_rect->y + src_rect->h) * src_inv_size->y;

   vb[3].x = dst_rect->x * dst_inv_size->x;
   vb[3].y = (dst_rect->y + dst_rect->h) * dst_inv_size->y;
   vb[3].z = src_rect->x * src_inv_size->x;
   vb[3].w = (src_rect->y + src_rect->h) * src_inv_size->y;
}

static void
gen_vertex_data(struct vl_compositor *c, struct pipe_video_rect *dst_rect, struct vertex2f *dst_inv_size)
{
   struct vertex4f *vb;
   struct pipe_transfer *buf_transfer;
   unsigned i;

   assert(c);
   assert(dst_rect);

   vb = pipe_buffer_map(c->pipe, c->vertex_buf.buffer,
                        PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD | PIPE_TRANSFER_DONTBLOCK,
                        &buf_transfer);

   if (!vb)
      return;

   for (i = 0; i < VL_COMPOSITOR_MAX_LAYERS; i++) {
      if (c->used_layers & (1 << i)) {
         struct pipe_sampler_view *sv = c->layers[i].sampler_views[0];
         struct vertex2f src_inv_size = {1.0f / sv->texture->width0, 1.0f / sv->texture->height0};

         if (&c->layers[i].fs == c->fs_video_buffer)
            gen_rect_verts(vb, &c->layers[i].src_rect, &src_inv_size, dst_rect, dst_inv_size);
         else
            gen_rect_verts(vb, &c->layers[i].src_rect, &src_inv_size, &c->layers[i].dst_rect, &src_inv_size);

         vb += 4;
      }
   }

   pipe_buffer_unmap(c->pipe, buf_transfer);
}

static void
draw_layers(struct vl_compositor *c)
{
   unsigned vb_index, i;

   assert(c);

   for (i = 0, vb_index = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i) {
      if (c->used_layers & (1 << i)) {
         struct pipe_sampler_view **samplers = &c->layers[i].sampler_views[0];
         unsigned num_sampler_views = !samplers[1] ? 1 : !samplers[2] ? 2 : 3;

         c->pipe->bind_fs_state(c->pipe, c->layers[i].fs);
         c->pipe->set_fragment_sampler_views(c->pipe, num_sampler_views, samplers);
         util_draw_arrays(c->pipe, PIPE_PRIM_QUADS, vb_index * 4, 4);
         vb_index++;
      }
   }
}

static void
vl_compositor_clear_layers(struct pipe_video_compositor *compositor)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   unsigned i, j;

   assert(compositor);

   c->used_layers = 0;
   for ( i = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i) {
      c->layers[i].fs = NULL;
      for ( j = 0; j < 3; j++)
         pipe_sampler_view_reference(&c->layers[i].sampler_views[j], NULL);
   }
}

static void
vl_compositor_destroy(struct pipe_video_compositor *compositor)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   assert(compositor);

   vl_compositor_clear_layers(compositor);

   cleanup_buffers(c);
   cleanup_shaders(c);
   cleanup_pipe_state(c);

   FREE(compositor);
}

static void
vl_compositor_set_csc_matrix(struct pipe_video_compositor *compositor, const float matrix[16])
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   struct pipe_transfer *buf_transfer;

   assert(compositor);

   memcpy
   (
      pipe_buffer_map(c->pipe, c->csc_matrix,
                      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
                      &buf_transfer),
		matrix,
		sizeof(csc_matrix)
   );

   pipe_buffer_unmap(c->pipe, buf_transfer);
}

static void
vl_compositor_set_buffer_layer(struct pipe_video_compositor *compositor,
                               unsigned layer,
                               struct pipe_video_buffer *buffer,
                               struct pipe_video_rect *src_rect,
                               struct pipe_video_rect *dst_rect)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   struct pipe_sampler_view **sampler_views;
   unsigned i;

   assert(compositor && buffer);

   assert(layer < VL_COMPOSITOR_MAX_LAYERS);

   c->used_layers |= 1 << layer;
   c->layers[layer].fs = c->fs_video_buffer;

   sampler_views = buffer->get_sampler_views(buffer);
   for (i = 0; i < 3; ++i)
      pipe_sampler_view_reference(&c->layers[layer].sampler_views[i], sampler_views[i]);

   c->layers[layer].src_rect = src_rect ? *src_rect : default_rect(&c->layers[layer]);
   c->layers[layer].dst_rect = dst_rect ? *dst_rect : default_rect(&c->layers[layer]);
}

static void
vl_compositor_set_palette_layer(struct pipe_video_compositor *compositor,
                                unsigned layer,
                                struct pipe_sampler_view *indexes,
                                struct pipe_sampler_view *palette,
                                struct pipe_video_rect *src_rect,
                                struct pipe_video_rect *dst_rect)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   assert(compositor && indexes && palette);

   assert(layer < VL_COMPOSITOR_MAX_LAYERS);

   c->used_layers |= 1 << layer;
   c->layers[layer].fs = c->fs_palette;
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[0], indexes);
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[1], palette);
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[2], NULL);
   c->layers[layer].src_rect = src_rect ? *src_rect : default_rect(&c->layers[layer]);
   c->layers[layer].dst_rect = dst_rect ? *dst_rect : default_rect(&c->layers[layer]);
}

static void
vl_compositor_set_rgba_layer(struct pipe_video_compositor *compositor,
                             unsigned layer,
                             struct pipe_sampler_view *rgba,
                             struct pipe_video_rect *src_rect,
                             struct pipe_video_rect *dst_rect)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   assert(compositor && rgba);

   assert(layer < VL_COMPOSITOR_MAX_LAYERS);

   c->used_layers |= 1 << layer;
   c->layers[layer].fs = c->fs_rgba;
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[0], rgba);
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[1], NULL);
   pipe_sampler_view_reference(&c->layers[layer].sampler_views[2], NULL);
   c->layers[layer].src_rect = src_rect ? *src_rect : default_rect(&c->layers[layer]);
   c->layers[layer].dst_rect = dst_rect ? *dst_rect : default_rect(&c->layers[layer]);
}

static void
vl_compositor_render(struct pipe_video_compositor *compositor,
                     enum pipe_mpeg12_picture_type picture_type,
                     struct pipe_surface           *dst_surface,
                     struct pipe_video_rect        *dst_area,
                     struct pipe_fence_handle      **fence)
{
   struct vl_compositor *c = (struct vl_compositor *)compositor;
   struct vertex2f dst_inv_size;
   void *samplers[3];

   assert(compositor);
   assert(dst_surface);
   assert(dst_area);

   c->fb_state.width = dst_surface->width;
   c->fb_state.height = dst_surface->height;
   c->fb_state.cbufs[0] = dst_surface;

   c->viewport.scale[0] = dst_surface->width;
   c->viewport.scale[1] = dst_surface->height;

   dst_inv_size.x = 1.0f / dst_surface->width;
   dst_inv_size.y = 1.0f / dst_surface->height;

   samplers[0] = samplers[1] = samplers[2] = c->sampler;

   gen_vertex_data(c, dst_area, &dst_inv_size);

   c->pipe->set_framebuffer_state(c->pipe, &c->fb_state);
   c->pipe->set_viewport_state(c->pipe, &c->viewport);
   c->pipe->bind_fragment_sampler_states(c->pipe, 3, &samplers[0]);
   c->pipe->bind_vs_state(c->pipe, c->vs);
   c->pipe->set_vertex_buffers(c->pipe, 1, &c->vertex_buf);
   c->pipe->bind_vertex_elements_state(c->pipe, c->vertex_elems_state);
   c->pipe->set_constant_buffer(c->pipe, PIPE_SHADER_FRAGMENT, 0, c->csc_matrix);
   c->pipe->bind_blend_state(c->pipe, c->blend);

   draw_layers(c);

   c->pipe->flush(c->pipe, fence);
}

struct pipe_video_compositor *
vl_compositor_init(struct pipe_video_context *vpipe, struct pipe_context *pipe)
{
   csc_matrix csc_matrix;
   struct vl_compositor *compositor;

   compositor = CALLOC_STRUCT(vl_compositor);

   compositor->base.context = vpipe;
   compositor->base.destroy = vl_compositor_destroy;
   compositor->base.set_csc_matrix = vl_compositor_set_csc_matrix;
   compositor->base.clear_layers = vl_compositor_clear_layers;
   compositor->base.set_buffer_layer = vl_compositor_set_buffer_layer;
   compositor->base.set_palette_layer = vl_compositor_set_palette_layer;
   compositor->base.set_rgba_layer = vl_compositor_set_rgba_layer;
   compositor->base.render_picture = vl_compositor_render;

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

   vl_compositor_clear_layers(&compositor->base);

   vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_IDENTITY, NULL, true, csc_matrix);
   vl_compositor_set_csc_matrix(&compositor->base, csc_matrix);

   return &compositor->base;
}
