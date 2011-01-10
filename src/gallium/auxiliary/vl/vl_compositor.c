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

static bool
u_video_rects_equal(struct pipe_video_rect *a, struct pipe_video_rect *b)
{
   assert(a && b);

   if (a->x != b->x)
      return false;
   if (a->y != b->y)
      return false;
   if (a->w != b->w)
      return false;
   if (a->h != b->h)
      return false;

   return true;
}

static bool
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

   c->vertex_shader = ureg_create_shader_and_destroy(shader, c->pipe);
   if (!c->vertex_shader)
      return false;

   return true;
}

static bool
create_frag_shader_ycbcr_2_rgb(struct vl_compositor *c)
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

   c->fragment_shader.ycbcr_2_rgb = ureg_create_shader_and_destroy(shader, c->pipe);
   if (!c->fragment_shader.ycbcr_2_rgb)
      return false;

   return true;
}

static bool
create_frag_shader_rgb_2_rgb(struct vl_compositor *c)
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

   c->fragment_shader.rgb_2_rgb = ureg_create_shader_and_destroy(shader, c->pipe);
   if (!c->fragment_shader.rgb_2_rgb)
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

   if (!create_vert_shader(c)) {
      debug_printf("Unable to create vertex shader.\n");
      return false;
   }
   if (!create_frag_shader_ycbcr_2_rgb(c)) {
      debug_printf("Unable to create YCbCr-to-RGB fragment shader.\n");
      return false;
   }
   if (!create_frag_shader_rgb_2_rgb(c)) {
      debug_printf("Unable to create RGB-to-RGB fragment shader.\n");
      return false;
   }

   return true;
}

static void cleanup_shaders(struct vl_compositor *c)
{
   assert(c);

   c->pipe->delete_vs_state(c->pipe, c->vertex_shader);
   c->pipe->delete_fs_state(c->pipe, c->fragment_shader.ycbcr_2_rgb);
   c->pipe->delete_fs_state(c->pipe, c->fragment_shader.rgb_2_rgb);
}

static bool
init_buffers(struct vl_compositor *c)
{
   struct fragment_shader_consts fsc;
   struct pipe_vertex_element vertex_elems[2];

   assert(c);

   /*
    * Create our vertex buffer and vertex buffer elements
    */
   c->vertex_buf.stride = sizeof(struct vertex4f);
   c->vertex_buf.max_index = (VL_COMPOSITOR_MAX_LAYERS + 2) * 6 - 1;
   c->vertex_buf.buffer_offset = 0;
   /* XXX: Create with DYNAMIC or STREAM */
   c->vertex_buf.buffer = pipe_buffer_create
   (
      c->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex4f) * (VL_COMPOSITOR_MAX_LAYERS + 2) * 6
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
   c->fs_const_buf = pipe_buffer_create
   (
      c->pipe->screen,
      PIPE_BIND_CONSTANT_BUFFER,
      sizeof(struct fragment_shader_consts)
   );

   vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_IDENTITY, NULL, true, fsc.matrix);

   vl_compositor_set_csc_matrix(c, fsc.matrix);

   return true;
}

static void
cleanup_buffers(struct vl_compositor *c)
{
   assert(c);

   c->pipe->delete_vertex_elements_state(c->pipe, c->vertex_elems_state);
   pipe_resource_reference(&c->vertex_buf.buffer, NULL);
   pipe_resource_reference(&c->fs_const_buf, NULL);
}

static void
texview_map_delete(const struct keymap *map,
                   const void *key, void *data,
                   void *user)
{
   struct pipe_sampler_view *sv = (struct pipe_sampler_view*)data;

   assert(map);
   assert(key);
   assert(data);
   assert(user);

   pipe_sampler_view_reference(&sv, NULL);
}

bool vl_compositor_init(struct vl_compositor *compositor, struct pipe_context *pipe)
{
   unsigned i;

   assert(compositor);

   memset(compositor, 0, sizeof(struct vl_compositor));

   compositor->pipe = pipe;

   compositor->texview_map = util_new_keymap(sizeof(struct pipe_surface*), -1,
                                             texview_map_delete);
   if (!compositor->texview_map)
      return false;

   if (!init_pipe_state(compositor)) {
      util_delete_keymap(compositor->texview_map, compositor->pipe);
      return false;
   }
   if (!init_shaders(compositor)) {
      util_delete_keymap(compositor->texview_map, compositor->pipe);
      cleanup_pipe_state(compositor);
      return false;
   }
   if (!init_buffers(compositor)) {
      util_delete_keymap(compositor->texview_map, compositor->pipe);
      cleanup_shaders(compositor);
      cleanup_pipe_state(compositor);
      return false;
   }

   compositor->fb_state.width = 0;
   compositor->fb_state.height = 0;
   compositor->bg = NULL;
   compositor->dirty_bg = false;
   for (i = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i)
      compositor->layers[i] = NULL;
   compositor->dirty_layers = 0;

   return true;
}

void vl_compositor_cleanup(struct vl_compositor *compositor)
{
   assert(compositor);

   util_delete_keymap(compositor->texview_map, compositor->pipe);
   cleanup_buffers(compositor);
   cleanup_shaders(compositor);
   cleanup_pipe_state(compositor);
}

void vl_compositor_set_background(struct vl_compositor *compositor,
                                 struct pipe_surface *bg, struct pipe_video_rect *bg_src_rect)
{
   assert(compositor);
   assert((bg && bg_src_rect) || (!bg && !bg_src_rect));

   if (compositor->bg != bg ||
       !u_video_rects_equal(&compositor->bg_src_rect, bg_src_rect)) {
      pipe_surface_reference(&compositor->bg, bg);
      /*if (!u_video_rects_equal(&compositor->bg_src_rect, bg_src_rect))*/
         compositor->bg_src_rect = *bg_src_rect;
      compositor->dirty_bg = true;
   }
}

void vl_compositor_set_layers(struct vl_compositor *compositor,
                              struct pipe_surface *layers[],
                              struct pipe_video_rect *src_rects[],
                              struct pipe_video_rect *dst_rects[],
                              unsigned num_layers)
{
   unsigned i;

   assert(compositor);
   assert(num_layers <= VL_COMPOSITOR_MAX_LAYERS);

   for (i = 0; i < num_layers; ++i)
   {
      assert((layers[i] && src_rects[i] && dst_rects[i]) ||
             (!layers[i] && !src_rects[i] && !dst_rects[i]));

      if (compositor->layers[i] != layers[i] ||
          !u_video_rects_equal(&compositor->layer_src_rects[i], src_rects[i]) ||
          !u_video_rects_equal(&compositor->layer_dst_rects[i], dst_rects[i]))
      {
         pipe_surface_reference(&compositor->layers[i], layers[i]);
         /*if (!u_video_rects_equal(&compositor->layer_src_rects[i], src_rects[i]))*/
            compositor->layer_src_rects[i] = *src_rects[i];
         /*if (!u_video_rects_equal(&compositor->layer_dst_rects[i], dst_rects[i]))*/
            compositor->layer_dst_rects[i] = *dst_rects[i];
         compositor->dirty_layers |= 1 << i;
      }

      if (layers[i])
         compositor->dirty_layers |= 1 << i;
   }

   for (; i < VL_COMPOSITOR_MAX_LAYERS; ++i)
      pipe_surface_reference(&compositor->layers[i], NULL);
}

static void gen_rect_verts(unsigned pos,
                           struct pipe_video_rect *src_rect,
                           struct vertex2f *src_inv_size,
                           struct pipe_video_rect *dst_rect,
                           struct vertex2f *dst_inv_size,
                           struct vertex4f *vb)
{
   assert(pos < VL_COMPOSITOR_MAX_LAYERS + 2);
   assert(src_rect);
   assert(src_inv_size);
   assert((dst_rect && dst_inv_size) /*|| (!dst_rect && !dst_inv_size)*/);
   assert(vb);

   vb[pos * 6 + 0].x = dst_rect->x * dst_inv_size->x;
   vb[pos * 6 + 0].y = dst_rect->y * dst_inv_size->y;
   vb[pos * 6 + 0].z = src_rect->x * src_inv_size->x;
   vb[pos * 6 + 0].w = src_rect->y * src_inv_size->y;

   vb[pos * 6 + 1].x = dst_rect->x * dst_inv_size->x;
   vb[pos * 6 + 1].y = (dst_rect->y + dst_rect->h) * dst_inv_size->y;
   vb[pos * 6 + 1].z = src_rect->x * src_inv_size->x;
   vb[pos * 6 + 1].w = (src_rect->y + src_rect->h) * src_inv_size->y;

   vb[pos * 6 + 2].x = (dst_rect->x + dst_rect->w) * dst_inv_size->x;
   vb[pos * 6 + 2].y = dst_rect->y * dst_inv_size->y;
   vb[pos * 6 + 2].z = (src_rect->x + src_rect->w) * src_inv_size->x;
   vb[pos * 6 + 2].w = src_rect->y * src_inv_size->y;

   vb[pos * 6 + 3].x = (dst_rect->x + dst_rect->w) * dst_inv_size->x;
   vb[pos * 6 + 3].y = dst_rect->y * dst_inv_size->y;
   vb[pos * 6 + 3].z = (src_rect->x + src_rect->w) * src_inv_size->x;
   vb[pos * 6 + 3].w = src_rect->y * src_inv_size->y;

   vb[pos * 6 + 4].x = dst_rect->x * dst_inv_size->x;
   vb[pos * 6 + 4].y = (dst_rect->y + dst_rect->h) * dst_inv_size->y;
   vb[pos * 6 + 4].z = src_rect->x * src_inv_size->x;
   vb[pos * 6 + 4].w = (src_rect->y + src_rect->h) * src_inv_size->y;

   vb[pos * 6 + 5].x = (dst_rect->x + dst_rect->w) * dst_inv_size->x;
   vb[pos * 6 + 5].y = (dst_rect->y + dst_rect->h) * dst_inv_size->y;
   vb[pos * 6 + 5].z = (src_rect->x + src_rect->w) * src_inv_size->x;
   vb[pos * 6 + 5].w = (src_rect->y + src_rect->h) * src_inv_size->y;
}

static unsigned gen_data(struct vl_compositor *c,
                         struct pipe_surface *src_surface,
                         struct pipe_video_rect *src_rect,
                         struct pipe_video_rect *dst_rect,
                         struct pipe_surface **textures,
                         void **frag_shaders)
{
   void *vb;
   struct pipe_transfer *buf_transfer;
   unsigned num_rects = 0;
   unsigned i;

   assert(c);
   assert(src_surface);
   assert(src_rect);
   assert(dst_rect);
   assert(textures);

   vb = pipe_buffer_map(c->pipe, c->vertex_buf.buffer,
                        PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
                        &buf_transfer);

   if (!vb)
      return 0;

   if (c->dirty_bg) {
      struct vertex2f bg_inv_size = {1.0f / c->bg->width, 1.0f / c->bg->height};
      gen_rect_verts(num_rects, &c->bg_src_rect, &bg_inv_size, NULL, NULL, vb);
      textures[num_rects] = c->bg;
      /* XXX: Hack */
      frag_shaders[num_rects] = c->fragment_shader.rgb_2_rgb;
      ++num_rects;
      c->dirty_bg = false;
   }

   {
      struct vertex2f src_inv_size = { 1.0f / src_surface->width, 1.0f / src_surface->height};
      gen_rect_verts(num_rects, src_rect, &src_inv_size, dst_rect, &c->fb_inv_size, vb);
      textures[num_rects] = src_surface;
      /* XXX: Hack, sort of */
      frag_shaders[num_rects] = c->fragment_shader.ycbcr_2_rgb;
      ++num_rects;
   }

   for (i = 0; c->dirty_layers > 0; i++) {
      assert(i < VL_COMPOSITOR_MAX_LAYERS);

      if (c->dirty_layers & (1 << i)) {
         struct vertex2f layer_inv_size = {1.0f / c->layers[i]->width, 1.0f / c->layers[i]->height};
         gen_rect_verts(num_rects, &c->layer_src_rects[i], &layer_inv_size,
                        &c->layer_dst_rects[i], &c->fb_inv_size, vb);
         textures[num_rects] = c->layers[i];
         /* XXX: Hack */
         frag_shaders[num_rects] = c->fragment_shader.rgb_2_rgb;
         ++num_rects;
         c->dirty_layers &= ~(1 << i);
      }
   }

   pipe_buffer_unmap(c->pipe, buf_transfer);

   return num_rects;
}

static void draw_layers(struct vl_compositor *c,
                        struct pipe_surface *src_surface,
                        struct pipe_video_rect *src_rect,
                        struct pipe_video_rect *dst_rect)
{
   unsigned num_rects;
   struct pipe_surface *src_surfaces[VL_COMPOSITOR_MAX_LAYERS + 2];
   void *frag_shaders[VL_COMPOSITOR_MAX_LAYERS + 2];
   unsigned i;

   assert(c);
   assert(src_surface);
   assert(src_rect);
   assert(dst_rect);

   num_rects = gen_data(c, src_surface, src_rect, dst_rect, src_surfaces, frag_shaders);

   for (i = 0; i < num_rects; ++i) {
      boolean delete_view = FALSE;
      struct pipe_sampler_view *surface_view = (struct pipe_sampler_view*)util_keymap_lookup(c->texview_map,
                                                                                             &src_surfaces[i]);
      if (!surface_view) {
         struct pipe_sampler_view templat;
         u_sampler_view_default_template(&templat, src_surfaces[i]->texture,
                                         src_surfaces[i]->texture->format);
         surface_view = c->pipe->create_sampler_view(c->pipe, src_surfaces[i]->texture,
                                                     &templat);
         if (!surface_view)
            return;

         delete_view = !util_keymap_insert(c->texview_map, &src_surfaces[i],
                                           surface_view, c->pipe);
      }

      c->pipe->bind_fs_state(c->pipe, frag_shaders[i]);
      c->pipe->set_fragment_sampler_views(c->pipe, 1, &surface_view);

      util_draw_arrays(c->pipe, PIPE_PRIM_TRIANGLES, i * 6, 6);

      if (delete_view) {
         pipe_sampler_view_reference(&surface_view, NULL);
      }
   }
}

void vl_compositor_render(struct vl_compositor          *compositor,
                          struct pipe_surface           *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_surface           *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_surface           *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_surface           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          struct pipe_fence_handle      **fence)
{
   assert(compositor);
   assert(src_surface);
   assert(src_area);
   assert(dst_surface);
   assert(dst_area);
   assert(picture_type == PIPE_MPEG12_PICTURE_TYPE_FRAME);

   if (compositor->fb_state.width != dst_surface->width) {
      compositor->fb_inv_size.x = 1.0f / dst_surface->width;
      compositor->fb_state.width = dst_surface->width;
   }
   if (compositor->fb_state.height != dst_surface->height) {
      compositor->fb_inv_size.y = 1.0f / dst_surface->height;
      compositor->fb_state.height = dst_surface->height;
   }

   compositor->fb_state.cbufs[0] = dst_surface;

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
   compositor->pipe->bind_fragment_sampler_states(compositor->pipe, 1, &compositor->sampler);
   compositor->pipe->bind_vs_state(compositor->pipe, compositor->vertex_shader);
   compositor->pipe->set_vertex_buffers(compositor->pipe, 1, &compositor->vertex_buf);
   compositor->pipe->bind_vertex_elements_state(compositor->pipe, compositor->vertex_elems_state);
   compositor->pipe->set_constant_buffer(compositor->pipe, PIPE_SHADER_FRAGMENT, 0, compositor->fs_const_buf);

   draw_layers(compositor, src_surface, src_area, dst_area);

   assert(!compositor->dirty_bg && !compositor->dirty_layers);
   compositor->pipe->flush(compositor->pipe, PIPE_FLUSH_RENDER_CACHE, fence);
}

void vl_compositor_set_csc_matrix(struct vl_compositor *compositor, const float *mat)
{
   struct pipe_transfer *buf_transfer;

   assert(compositor);

   memcpy
   (
      pipe_buffer_map(compositor->pipe, compositor->fs_const_buf,
                      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
                      &buf_transfer),
		mat,
		sizeof(struct fragment_shader_consts)
   );

   pipe_buffer_unmap(compositor->pipe, buf_transfer);
}
