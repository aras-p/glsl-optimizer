/**************************************************************************
 *
 * Copyright 2013 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/* This head-up display module can draw transparent graphs on top of what
 * the app is rendering, visualizing various data like framerate, cpu load,
 * performance counters, etc. It can be hook up into any state tracker.
 *
 * The HUD is controlled with the GALLIUM_HUD environment variable.
 * Set GALLIUM_HUD=help for more info.
 */

#include <stdio.h>

#include "hud/hud_context.h"
#include "hud/hud_private.h"
#include "hud/font.h"

#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_sampler.h"
#include "util/u_simple_shaders.h"
#include "util/u_string.h"
#include "util/u_upload_mgr.h"
#include "tgsi/tgsi_text.h"
#include "tgsi/tgsi_dump.h"


struct hud_context {
   struct pipe_context *pipe;
   struct cso_context *cso;
   struct u_upload_mgr *uploader;

   struct list_head pane_list;

   /* states */
   struct pipe_blend_state alpha_blend;
   struct pipe_depth_stencil_alpha_state dsa;
   void *fs_color, *fs_text;
   struct pipe_rasterizer_state rasterizer;
   void *vs;
   struct pipe_vertex_element velems[2];

   /* font */
   struct util_font font;
   struct pipe_sampler_view *font_sampler_view;
   struct pipe_sampler_state font_sampler_state;

   /* VS constant buffer */
   struct {
      float color[4];
      float two_div_fb_width;
      float two_div_fb_height;
      float translate[2];
      float scale[2];
      float padding[2];
   } constants;
   struct pipe_constant_buffer constbuf;

   unsigned fb_width, fb_height;

   /* vertices for text and background drawing are accumulated here and then
    * drawn all at once */
   struct vertex_queue {
      float *vertices;
      struct pipe_vertex_buffer vbuf;
      unsigned max_num_vertices;
      unsigned num_vertices;
   } text, bg, whitelines;
};


static void
hud_draw_colored_prims(struct hud_context *hud, unsigned prim,
                       float *buffer, unsigned num_vertices,
                       float r, float g, float b, float a,
                       int xoffset, int yoffset, float yscale)
{
   struct cso_context *cso = hud->cso;
   struct pipe_vertex_buffer vbuffer = {0};

   hud->constants.color[0] = r;
   hud->constants.color[1] = g;
   hud->constants.color[2] = b;
   hud->constants.color[3] = a;
   hud->constants.translate[0] = (float) xoffset;
   hud->constants.translate[1] = (float) yoffset;
   hud->constants.scale[0] = 1;
   hud->constants.scale[1] = yscale;
   cso_set_constant_buffer(cso, PIPE_SHADER_VERTEX, 0, &hud->constbuf);

   vbuffer.user_buffer = buffer;
   vbuffer.stride = 2 * sizeof(float);

   cso_set_vertex_buffers(cso, cso_get_aux_vertex_buffer_slot(cso),
                          1, &vbuffer);
   cso_set_fragment_shader_handle(hud->cso, hud->fs_color);
   cso_draw_arrays(cso, prim, 0, num_vertices);
}

static void
hud_draw_colored_quad(struct hud_context *hud, unsigned prim,
                      unsigned x1, unsigned y1, unsigned x2, unsigned y2,
                      float r, float g, float b, float a)
{
   float buffer[] = {
      (float) x1, (float) y1,
      (float) x1, (float) y2,
      (float) x2, (float) y2,
      (float) x2, (float) y1,
   };

   hud_draw_colored_prims(hud, prim, buffer, 4, r, g, b, a, 0, 0, 1);
}

static void
hud_draw_background_quad(struct hud_context *hud,
                         unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
   float *vertices = hud->bg.vertices + hud->bg.num_vertices*2;
   unsigned num = 0;

   assert(hud->bg.num_vertices + 4 <= hud->bg.max_num_vertices);

   vertices[num++] = (float) x1;
   vertices[num++] = (float) y1;

   vertices[num++] = (float) x1;
   vertices[num++] = (float) y2;

   vertices[num++] = (float) x2;
   vertices[num++] = (float) y2;

   vertices[num++] = (float) x2;
   vertices[num++] = (float) y1;

   hud->bg.num_vertices += num/2;
}

static void
hud_draw_string(struct hud_context *hud, unsigned x, unsigned y,
                const char *str, ...)
{
   char buf[256];
   char *s = buf;
   float *vertices = hud->text.vertices + hud->text.num_vertices*4;
   unsigned num = 0;

   va_list ap;
   va_start(ap, str);
   util_vsnprintf(buf, sizeof(buf), str, ap);
   va_end(ap);

   if (!*s)
      return;

   hud_draw_background_quad(hud,
                            x, y,
                            x + strlen(buf)*hud->font.glyph_width,
                            y + hud->font.glyph_height);

   while (*s) {
      unsigned x1 = x;
      unsigned y1 = y;
      unsigned x2 = x + hud->font.glyph_width;
      unsigned y2 = y + hud->font.glyph_height;
      unsigned tx1 = (*s % 16) * hud->font.glyph_width;
      unsigned ty1 = (*s / 16) * hud->font.glyph_height;
      unsigned tx2 = tx1 + hud->font.glyph_width;
      unsigned ty2 = ty1 + hud->font.glyph_height;

      if (*s == ' ') {
         x += hud->font.glyph_width;
         s++;
         continue;
      }

      assert(hud->text.num_vertices + num/4 + 4 <= hud->text.max_num_vertices);

      vertices[num++] = (float) x1;
      vertices[num++] = (float) y1;
      vertices[num++] = (float) tx1;
      vertices[num++] = (float) ty1;

      vertices[num++] = (float) x1;
      vertices[num++] = (float) y2;
      vertices[num++] = (float) tx1;
      vertices[num++] = (float) ty2;

      vertices[num++] = (float) x2;
      vertices[num++] = (float) y2;
      vertices[num++] = (float) tx2;
      vertices[num++] = (float) ty2;

      vertices[num++] = (float) x2;
      vertices[num++] = (float) y1;
      vertices[num++] = (float) tx2;
      vertices[num++] = (float) ty1;

      x += hud->font.glyph_width;
      s++;
   }

   hud->text.num_vertices += num/4;
}

static void
number_to_human_readable(uint64_t num, boolean is_in_bytes, char *out)
{
   static const char *byte_units[] =
      {"", " KB", " MB", " GB", " TB", " PB", " EB"};
   static const char *metric_units[] =
      {"", " k", " M", " G", " T", " P", " E"};
   const char **units = is_in_bytes ? byte_units : metric_units;
   double divisor = is_in_bytes ? 1024 : 1000;
   int unit = 0;
   double d = num;

   while (d > divisor) {
      d /= divisor;
      unit++;
   }

   if (d >= 100 || d == (int)d)
      sprintf(out, "%.0f%s", d, units[unit]);
   else if (d >= 10 || d*10 == (int)(d*10))
      sprintf(out, "%.1f%s", d, units[unit]);
   else
      sprintf(out, "%.2f%s", d, units[unit]);
}

static void
hud_draw_graph_line_strip(struct hud_context *hud, const struct hud_graph *gr,
                          unsigned xoffset, unsigned yoffset, float yscale)
{
   if (gr->num_vertices <= 1)
      return;

   assert(gr->index <= gr->num_vertices);

   hud_draw_colored_prims(hud, PIPE_PRIM_LINE_STRIP,
                          gr->vertices, gr->index,
                          gr->color[0], gr->color[1], gr->color[2], 1,
                          xoffset + (gr->pane->max_num_vertices - gr->index - 1) * 2 - 1,
                          yoffset, yscale);

   if (gr->num_vertices <= gr->index)
      return;

   hud_draw_colored_prims(hud, PIPE_PRIM_LINE_STRIP,
                          gr->vertices + gr->index*2,
                          gr->num_vertices - gr->index,
                          gr->color[0], gr->color[1], gr->color[2], 1,
                          xoffset - gr->index*2 - 1, yoffset, yscale);
}

static void
hud_pane_accumulate_vertices(struct hud_context *hud,
                             const struct hud_pane *pane)
{
   struct hud_graph *gr;
   float *line_verts = hud->whitelines.vertices + hud->whitelines.num_vertices*2;
   unsigned i, num = 0;
   char str[32];

   /* draw background */
   hud_draw_background_quad(hud,
                            pane->x1, pane->y1,
                            pane->x2, pane->y2);

   /* draw numbers on the right-hand side */
   for (i = 0; i < 6; i++) {
      unsigned x = pane->x2 + 2;
      unsigned y = pane->inner_y1 + pane->inner_height * (5 - i) / 5 -
                   hud->font.glyph_height / 2;

      number_to_human_readable(pane->max_value * i / 5,
                               pane->uses_byte_units, str);
      hud_draw_string(hud, x, y, str);
   }

   /* draw info below the pane */
   i = 0;
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      unsigned x = pane->x1 + 2;
      unsigned y = pane->y2 + 2 + i*hud->font.glyph_height;

      number_to_human_readable(gr->current_value,
                               pane->uses_byte_units, str);
      hud_draw_string(hud, x, y, "  %s: %s", gr->name, str);
      i++;
   }

   /* draw border */
   assert(hud->whitelines.num_vertices + num/2 + 8 <= hud->whitelines.max_num_vertices);
   line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x2;
   line_verts[num++] = (float) pane->y1;

   line_verts[num++] = (float) pane->x2;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x2;
   line_verts[num++] = (float) pane->y2;

   line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y2;
   line_verts[num++] = (float) pane->x2;
   line_verts[num++] = (float) pane->y2;

   line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y2;

   /* draw horizontal lines inside the graph */
   for (i = 0; i <= 5; i++) {
      float y = round((pane->max_value * i / 5.0) * pane->yscale + pane->inner_y2);

      assert(hud->whitelines.num_vertices + num/2 + 2 <= hud->whitelines.max_num_vertices);
      line_verts[num++] = pane->x1;
      line_verts[num++] = y;
      line_verts[num++] = pane->x2;
      line_verts[num++] = y;
   }

   hud->whitelines.num_vertices += num/2;
}

static void
hud_pane_draw_colored_objects(struct hud_context *hud,
                              const struct hud_pane *pane)
{
   struct hud_graph *gr;
   unsigned i;

   /* draw colored quads below the pane */
   i = 0;
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      unsigned x = pane->x1 + 2;
      unsigned y = pane->y2 + 2 + i*hud->font.glyph_height;

      hud_draw_colored_quad(hud, PIPE_PRIM_QUADS, x + 1, y + 1, x + 12, y + 13,
                            gr->color[0], gr->color[1], gr->color[2], 1);
      i++;
   }

   /* draw the line strips */
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      hud_draw_graph_line_strip(hud, gr, pane->inner_x1, pane->inner_y2, pane->yscale);
   }
}

static void
hud_alloc_vertices(struct hud_context *hud, struct vertex_queue *v,
                   unsigned num_vertices, unsigned stride)
{
   v->num_vertices = 0;
   v->max_num_vertices = num_vertices;
   v->vbuf.stride = stride;
   u_upload_alloc(hud->uploader, 0, v->vbuf.stride * v->max_num_vertices,
                  &v->vbuf.buffer_offset, &v->vbuf.buffer,
                  (void**)&v->vertices);
}

/**
 * Draw the HUD to the texture \p tex.
 * The texture is usually the back buffer being displayed.
 */
void
hud_draw(struct hud_context *hud, struct pipe_resource *tex)
{
   struct cso_context *cso = hud->cso;
   struct pipe_context *pipe = hud->pipe;
   struct pipe_framebuffer_state fb;
   struct pipe_surface surf_templ, *surf;
   struct pipe_viewport_state viewport;
   const struct pipe_sampler_state *sampler_states[] =
         { &hud->font_sampler_state };
   struct hud_pane *pane;
   struct hud_graph *gr;

   hud->fb_width = tex->width0;
   hud->fb_height = tex->height0;
   hud->constants.two_div_fb_width = 2.0f / hud->fb_width;
   hud->constants.two_div_fb_height = 2.0f / hud->fb_height;

   cso_save_framebuffer(cso);
   cso_save_sample_mask(cso);
   cso_save_min_samples(cso);
   cso_save_blend(cso);
   cso_save_depth_stencil_alpha(cso);
   cso_save_fragment_shader(cso);
   cso_save_sampler_views(cso, PIPE_SHADER_FRAGMENT);
   cso_save_samplers(cso, PIPE_SHADER_FRAGMENT);
   cso_save_rasterizer(cso);
   cso_save_viewport(cso);
   cso_save_stream_outputs(cso);
   cso_save_geometry_shader(cso);
   cso_save_vertex_shader(cso);
   cso_save_vertex_elements(cso);
   cso_save_aux_vertex_buffer_slot(cso);
   cso_save_constant_buffer_slot0(cso, PIPE_SHADER_VERTEX);
   cso_save_render_condition(cso);

   /* set states */
   memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf = pipe->create_surface(pipe, tex, &surf_templ);

   memset(&fb, 0, sizeof(fb));
   fb.nr_cbufs = 1;
   fb.cbufs[0] = surf;
   fb.zsbuf = NULL;
   fb.width = hud->fb_width;
   fb.height = hud->fb_height;

   viewport.scale[0] = 0.5f * hud->fb_width;
   viewport.scale[1] = 0.5f * hud->fb_height;
   viewport.scale[2] = 1.0f;
   viewport.scale[3] = 1.0f;
   viewport.translate[0] = 0.5f * hud->fb_width;
   viewport.translate[1] = 0.5f * hud->fb_height;
   viewport.translate[2] = 0.0f;
   viewport.translate[3] = 0.0f;

   cso_set_framebuffer(cso, &fb);
   cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
   cso_set_blend(cso, &hud->alpha_blend);
   cso_set_depth_stencil_alpha(cso, &hud->dsa);
   cso_set_rasterizer(cso, &hud->rasterizer);
   cso_set_viewport(cso, &viewport);
   cso_set_stream_outputs(cso, 0, NULL, NULL);
   cso_set_geometry_shader_handle(cso, NULL);
   cso_set_vertex_shader_handle(cso, hud->vs);
   cso_set_vertex_elements(cso, 2, hud->velems);
   cso_set_render_condition(cso, NULL, FALSE, 0);
   cso_set_sampler_views(cso, PIPE_SHADER_FRAGMENT, 1,
                         &hud->font_sampler_view);
   cso_set_samplers(cso, PIPE_SHADER_FRAGMENT, 1, sampler_states);
   cso_set_constant_buffer(cso, PIPE_SHADER_VERTEX, 0, &hud->constbuf);

   /* prepare vertex buffers */
   hud_alloc_vertices(hud, &hud->bg, 4 * 128, 2 * sizeof(float));
   hud_alloc_vertices(hud, &hud->whitelines, 4 * 256, 2 * sizeof(float));
   hud_alloc_vertices(hud, &hud->text, 4 * 512, 4 * sizeof(float));

   /* prepare all graphs */
   LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
      LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
         gr->query_new_value(gr);
      }

      hud_pane_accumulate_vertices(hud, pane);
   }

   /* unmap the uploader's vertex buffer before drawing */
   u_upload_unmap(hud->uploader);

   /* draw accumulated vertices for background quads */
   cso_set_fragment_shader_handle(hud->cso, hud->fs_color);

   if (hud->bg.num_vertices) {
      hud->constants.color[0] = 0;
      hud->constants.color[1] = 0;
      hud->constants.color[2] = 0;
      hud->constants.color[3] = 0.666f;
      hud->constants.translate[0] = 0;
      hud->constants.translate[1] = 0;
      hud->constants.scale[0] = 1;
      hud->constants.scale[1] = 1;

      cso_set_constant_buffer(cso, PIPE_SHADER_VERTEX, 0, &hud->constbuf);
      cso_set_vertex_buffers(cso, cso_get_aux_vertex_buffer_slot(cso), 1,
                             &hud->bg.vbuf);
      cso_draw_arrays(cso, PIPE_PRIM_QUADS, 0, hud->bg.num_vertices);
   }
   pipe_resource_reference(&hud->bg.vbuf.buffer, NULL);

   /* draw accumulated vertices for white lines */
   hud->constants.color[0] = 1;
   hud->constants.color[1] = 1;
   hud->constants.color[2] = 1;
   hud->constants.color[3] = 1;
   hud->constants.translate[0] = 0;
   hud->constants.translate[1] = 0;
   hud->constants.scale[0] = 1;
   hud->constants.scale[1] = 1;
   cso_set_constant_buffer(cso, PIPE_SHADER_VERTEX, 0, &hud->constbuf);

   if (hud->whitelines.num_vertices) {
      cso_set_vertex_buffers(cso, cso_get_aux_vertex_buffer_slot(cso), 1,
                             &hud->whitelines.vbuf);
      cso_set_fragment_shader_handle(hud->cso, hud->fs_color);
      cso_draw_arrays(cso, PIPE_PRIM_LINES, 0, hud->whitelines.num_vertices);
   }
   pipe_resource_reference(&hud->whitelines.vbuf.buffer, NULL);

   /* draw accumulated vertices for text */
   if (hud->text.num_vertices) {
      cso_set_vertex_buffers(cso, cso_get_aux_vertex_buffer_slot(cso), 1,
                             &hud->text.vbuf);
      cso_set_fragment_shader_handle(hud->cso, hud->fs_text);
      cso_draw_arrays(cso, PIPE_PRIM_QUADS, 0, hud->text.num_vertices);
   }
   pipe_resource_reference(&hud->text.vbuf.buffer, NULL);

   /* draw the rest */
   LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
      if (pane)
         hud_pane_draw_colored_objects(hud, pane);
   }

   /* restore states */
   cso_restore_framebuffer(cso);
   cso_restore_sample_mask(cso);
   cso_restore_min_samples(cso);
   cso_restore_blend(cso);
   cso_restore_depth_stencil_alpha(cso);
   cso_restore_fragment_shader(cso);
   cso_restore_sampler_views(cso, PIPE_SHADER_FRAGMENT);
   cso_restore_samplers(cso, PIPE_SHADER_FRAGMENT);
   cso_restore_rasterizer(cso);
   cso_restore_viewport(cso);
   cso_restore_stream_outputs(cso);
   cso_restore_geometry_shader(cso);
   cso_restore_vertex_shader(cso);
   cso_restore_vertex_elements(cso);
   cso_restore_aux_vertex_buffer_slot(cso);
   cso_restore_constant_buffer_slot0(cso, PIPE_SHADER_VERTEX);
   cso_restore_render_condition(cso);

   pipe_surface_reference(&surf, NULL);
}

/**
 * Set the maximum value for the Y axis of the graph.
 * This scales the graph accordingly.
 */
void
hud_pane_set_max_value(struct hud_pane *pane, uint64_t value)
{
   pane->max_value = value;
   pane->yscale = -(int)pane->inner_height / (float)pane->max_value;
}

static struct hud_pane *
hud_pane_create(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
                unsigned period, uint64_t max_value)
{
   struct hud_pane *pane = CALLOC_STRUCT(hud_pane);

   if (!pane)
      return NULL;

   pane->x1 = x1;
   pane->y1 = y1;
   pane->x2 = x2;
   pane->y2 = y2;
   pane->inner_x1 = x1 + 1;
   pane->inner_x2 = x2 - 1;
   pane->inner_y1 = y1 + 1;
   pane->inner_y2 = y2 - 1;
   pane->inner_width = pane->inner_x2 - pane->inner_x1;
   pane->inner_height = pane->inner_y2 - pane->inner_y1;
   pane->period = period;
   pane->max_num_vertices = (x2 - x1 + 2) / 2;
   hud_pane_set_max_value(pane, max_value);
   LIST_INITHEAD(&pane->graph_list);
   return pane;
}

/**
 * Add a graph to an existing pane.
 * One pane can contain multiple graphs over each other.
 */
void
hud_pane_add_graph(struct hud_pane *pane, struct hud_graph *gr)
{
   static const float colors[][3] = {
      {0, 1, 0},
      {1, 0, 0},
      {0, 1, 1},
      {1, 0, 1},
      {1, 1, 0},
      {0.5, 0.5, 1},
      {0.5, 0.5, 0.5},
   };
   char *name = gr->name;

   /* replace '-' with a space */
   while (*name) {
      if (*name == '-')
         *name = ' ';
      name++;
   }

   assert(pane->num_graphs < Elements(colors));
   gr->vertices = MALLOC(pane->max_num_vertices * sizeof(float) * 2);
   gr->color[0] = colors[pane->num_graphs][0];
   gr->color[1] = colors[pane->num_graphs][1];
   gr->color[2] = colors[pane->num_graphs][2];
   gr->pane = pane;
   LIST_ADDTAIL(&gr->head, &pane->graph_list);
   pane->num_graphs++;
}

void
hud_graph_add_value(struct hud_graph *gr, uint64_t value)
{
   if (gr->index == gr->pane->max_num_vertices) {
      gr->vertices[0] = 0;
      gr->vertices[1] = gr->vertices[(gr->index-1)*2+1];
      gr->index = 1;
   }
   gr->vertices[(gr->index)*2+0] = (float) (gr->index * 2);
   gr->vertices[(gr->index)*2+1] = (float) value;
   gr->index++;

   if (gr->num_vertices < gr->pane->max_num_vertices) {
      gr->num_vertices++;
   }

   gr->current_value = value;
   if (value > gr->pane->max_value) {
      hud_pane_set_max_value(gr->pane, value);
   }
}

static void
hud_graph_destroy(struct hud_graph *graph)
{
   FREE(graph->vertices);
   if (graph->free_query_data)
      graph->free_query_data(graph->query_data);
   FREE(graph);
}

/**
 * Read a string from the environment variable.
 * The separators "+", ",", ":", and ";" terminate the string.
 * Return the number of read characters.
 */
static int
parse_string(const char *s, char *out)
{
   int i;

   for (i = 0; *s && *s != '+' && *s != ',' && *s != ':' && *s != ';';
        s++, out++, i++)
      *out = *s;

   *out = 0;

   if (*s && !i)
      fprintf(stderr, "gallium_hud: syntax error: unexpected '%c' (%i) while "
              "parsing a string\n", *s, *s);
   return i;
}

static boolean
has_occlusion_query(struct pipe_screen *screen)
{
   return screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY) != 0;
}

static boolean
has_streamout(struct pipe_screen *screen)
{
   return screen->get_param(screen, PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS) != 0;
}

static boolean
has_pipeline_stats_query(struct pipe_screen *screen)
{
   return screen->get_param(screen, PIPE_CAP_QUERY_PIPELINE_STATISTICS) != 0;
}

static void
hud_parse_env_var(struct hud_context *hud, const char *env)
{
   unsigned num, i;
   char name[256], s[256];
   struct hud_pane *pane = NULL;
   unsigned x = 10, y = 10;
   unsigned width = 251, height = 100;
   unsigned period = 500 * 1000;  /* default period (1/2 second) */
   const char *period_env;

   /*
    * The GALLIUM_HUD_PERIOD env var sets the graph update rate.
    * The env var is in seconds (a float).
    * Zero means update after every frame.
    */
   period_env = getenv("GALLIUM_HUD_PERIOD");
   if (period_env) {
      float p = (float) atof(period_env);
      if (p >= 0.0f) {
         period = (unsigned) (p * 1000 * 1000);
      }
   }

   while ((num = parse_string(env, name)) != 0) {
      env += num;

      if (!pane) {
         pane = hud_pane_create(x, y, x + width, y + height, period, 10);
         if (!pane)
            return;
      }

      /* Add a graph. */
      /* IF YOU CHANGE THIS, UPDATE print_help! */
      if (strcmp(name, "fps") == 0) {
         hud_fps_graph_install(pane);
      }
      else if (strcmp(name, "cpu") == 0) {
         hud_cpu_graph_install(pane, ALL_CPUS);
      }
      else if (sscanf(name, "cpu%u%s", &i, s) == 1) {
         hud_cpu_graph_install(pane, i);
      }
      else if (strcmp(name, "samples-passed") == 0 &&
               has_occlusion_query(hud->pipe->screen)) {
         hud_pipe_query_install(pane, hud->pipe, "samples-passed",
                                PIPE_QUERY_OCCLUSION_COUNTER, 0, 0, FALSE);
      }
      else if (strcmp(name, "primitives-generated") == 0 &&
               has_streamout(hud->pipe->screen)) {
         hud_pipe_query_install(pane, hud->pipe, "primitives-generated",
                                PIPE_QUERY_PRIMITIVES_GENERATED, 0, 0, FALSE);
      }
      else {
         boolean processed = FALSE;

         /* pipeline statistics queries */
         if (has_pipeline_stats_query(hud->pipe->screen)) {
            static const char *pipeline_statistics_names[] =
            {
               "ia-vertices",
               "ia-primitives",
               "vs-invocations",
               "gs-invocations",
               "gs-primitives",
               "clipper-invocations",
               "clipper-primitives-generated",
               "ps-invocations",
               "hs-invocations",
               "ds-invocations",
               "cs-invocations"
            };
            for (i = 0; i < Elements(pipeline_statistics_names); ++i)
               if (strcmp(name, pipeline_statistics_names[i]) == 0)
                  break;
            if (i < Elements(pipeline_statistics_names)) {
               hud_pipe_query_install(pane, hud->pipe, name,
                                      PIPE_QUERY_PIPELINE_STATISTICS, i,
                                      0, FALSE);
               processed = TRUE;
            }
         }

         /* driver queries */
         if (!processed) {
            if (!hud_driver_query_install(pane, hud->pipe, name)){
               fprintf(stderr, "gallium_hud: unknown driver query '%s'\n", name);
            }
         }
      }

      if (*env == ':') {
         env++;

         if (!pane) {
            fprintf(stderr, "gallium_hud: syntax error: unexpected ':', "
                    "expected a name\n");
            break;
         }

         num = parse_string(env, s);
         env += num;

         if (num && sscanf(s, "%u", &i) == 1) {
            hud_pane_set_max_value(pane, i);
         }
         else {
            fprintf(stderr, "gallium_hud: syntax error: unexpected '%c' (%i) "
                    "after ':'\n", *env, *env);
         }
      }

      if (*env == 0)
         break;

      /* parse a separator */
      switch (*env) {
      case '+':
         env++;
         break;

      case ',':
         env++;
         y += height + hud->font.glyph_height * (pane->num_graphs + 2);

         if (pane && pane->num_graphs) {
            LIST_ADDTAIL(&pane->head, &hud->pane_list);
            pane = NULL;
         }
         break;

      case ';':
         env++;
         y = 10;
         x += width + hud->font.glyph_width * 7;

         if (pane && pane->num_graphs) {
            LIST_ADDTAIL(&pane->head, &hud->pane_list);
            pane = NULL;
         }
         break;

      default:
         fprintf(stderr, "gallium_hud: syntax error: unexpected '%c'\n", *env);
      }
   }

   if (pane) {
      if (pane->num_graphs) {
         LIST_ADDTAIL(&pane->head, &hud->pane_list);
      }
      else {
         FREE(pane);
      }
   }
}

static void
print_help(struct pipe_screen *screen)
{
   int i, num_queries, num_cpus = hud_get_num_cpus();

   puts("Syntax: GALLIUM_HUD=name1[+name2][...][:value1][,nameI...][;nameJ...]");
   puts("");
   puts("  Names are identifiers of data sources which will be drawn as graphs");
   puts("  in panes. Multiple graphs can be drawn in the same pane.");
   puts("  There can be multiple panes placed in rows and columns.");
   puts("");
   puts("  '+' separates names which will share a pane.");
   puts("  ':[value]' specifies the initial maximum value of the Y axis");
   puts("             for the given pane.");
   puts("  ',' creates a new pane below the last one.");
   puts("  ';' creates a new pane at the top of the next column.");
   puts("");
   puts("  Example: GALLIUM_HUD=\"cpu,fps;primitives-generated\"");
   puts("");
   puts("  Available names:");
   puts("    fps");
   puts("    cpu");

   for (i = 0; i < num_cpus; i++)
      printf("    cpu%i\n", i);

   if (has_occlusion_query(screen))
      puts("    samples-passed");
   if (has_streamout(screen))
      puts("    primitives-generated");

   if (has_pipeline_stats_query(screen)) {
      puts("    ia-vertices");
      puts("    ia-primitives");
      puts("    vs-invocations");
      puts("    gs-invocations");
      puts("    gs-primitives");
      puts("    clipper-invocations");
      puts("    clipper-primitives-generated");
      puts("    ps-invocations");
      puts("    hs-invocations");
      puts("    ds-invocations");
      puts("    cs-invocations");
   }

   if (screen->get_driver_query_info){
      struct pipe_driver_query_info info;
      num_queries = screen->get_driver_query_info(screen, 0, NULL);

      for (i = 0; i < num_queries; i++){
         screen->get_driver_query_info(screen, i, &info);
         printf("    %s\n", info.name);
      }
   }

   puts("");
}

struct hud_context *
hud_create(struct pipe_context *pipe, struct cso_context *cso)
{
   struct hud_context *hud;
   struct pipe_sampler_view view_templ;
   unsigned i;
   const char *env = debug_get_option("GALLIUM_HUD", NULL);

   if (!env || !*env)
      return NULL;

   if (strcmp(env, "help") == 0) {
      print_help(pipe->screen);
      return NULL;
   }

   hud = CALLOC_STRUCT(hud_context);
   if (!hud)
      return NULL;

   hud->pipe = pipe;
   hud->cso = cso;
   hud->uploader = u_upload_create(pipe, 256 * 1024, 16,
                                   PIPE_BIND_VERTEX_BUFFER);

   /* font */
   if (!util_font_create(pipe, UTIL_FONT_FIXED_8X13, &hud->font)) {
      u_upload_destroy(hud->uploader);
      FREE(hud);
      return NULL;
   }

   /* blend state */
   hud->alpha_blend.rt[0].colormask = PIPE_MASK_RGBA;
   hud->alpha_blend.rt[0].blend_enable = 1;
   hud->alpha_blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   hud->alpha_blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   hud->alpha_blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   hud->alpha_blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   hud->alpha_blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
   hud->alpha_blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;

   /* fragment shader */
   hud->fs_color =
         util_make_fragment_passthrough_shader(pipe,
                                               TGSI_SEMANTIC_COLOR,
                                               TGSI_INTERPOLATE_CONSTANT,
                                               TRUE);

   {
      /* Read a texture and do .xxxx swizzling. */
      static const char *fragment_shader_text = {
         "FRAG\n"
         "DCL IN[0], GENERIC[0], LINEAR\n"
         "DCL SAMP[0]\n"
         "DCL OUT[0], COLOR[0]\n"
         "DCL TEMP[0]\n"

         "TEX TEMP[0], IN[0], SAMP[0], RECT\n"
         "MOV OUT[0], TEMP[0].xxxx\n"
         "END\n"
      };

      struct tgsi_token tokens[1000];
      struct pipe_shader_state state = {tokens};

      if (!tgsi_text_translate(fragment_shader_text, tokens, Elements(tokens))) {
         assert(0);
         pipe_resource_reference(&hud->font.texture, NULL);
         u_upload_destroy(hud->uploader);
         FREE(hud);
         return NULL;
      }

      hud->fs_text = pipe->create_fs_state(pipe, &state);
   }

   /* rasterizer */
   hud->rasterizer.half_pixel_center = 1;
   hud->rasterizer.bottom_edge_rule = 1;
   hud->rasterizer.depth_clip = 1;
   hud->rasterizer.line_width = 1;
   hud->rasterizer.line_last_pixel = 1;

   /* vertex shader */
   {
      static const char *vertex_shader_text = {
         "VERT\n"
         "DCL IN[0..1]\n"
         "DCL OUT[0], POSITION\n"
         "DCL OUT[1], COLOR[0]\n" /* color */
         "DCL OUT[2], GENERIC[0]\n" /* texcoord */
         /* [0] = color,
          * [1] = (2/fb_width, 2/fb_height, xoffset, yoffset)
          * [2] = (xscale, yscale, 0, 0) */
         "DCL CONST[0..2]\n"
         "DCL TEMP[0]\n"
         "IMM[0] FLT32 { -1, 0, 0, 1 }\n"

         /* v = in * (xscale, yscale) + (xoffset, yoffset) */
         "MAD TEMP[0].xy, IN[0], CONST[2].xyyy, CONST[1].zwww\n"
         /* pos = v * (2 / fb_width, 2 / fb_height) - (1, 1) */
         "MAD OUT[0].xy, TEMP[0], CONST[1].xyyy, IMM[0].xxxx\n"
         "MOV OUT[0].zw, IMM[0]\n"

         "MOV OUT[1], CONST[0]\n"
         "MOV OUT[2], IN[1]\n"
         "END\n"
      };

      struct tgsi_token tokens[1000];
      struct pipe_shader_state state = {tokens};

      if (!tgsi_text_translate(vertex_shader_text, tokens, Elements(tokens))) {
         assert(0);
         pipe_resource_reference(&hud->font.texture, NULL);
         u_upload_destroy(hud->uploader);
         FREE(hud);
         return NULL;
      }

      hud->vs = pipe->create_vs_state(pipe, &state);
   }

   /* vertex elements */
   for (i = 0; i < 2; i++) {
      hud->velems[i].src_offset = i * 2 * sizeof(float);
      hud->velems[i].src_format = PIPE_FORMAT_R32G32_FLOAT;
      hud->velems[i].vertex_buffer_index = cso_get_aux_vertex_buffer_slot(cso);
   }

   /* sampler view */
   u_sampler_view_default_template(
         &view_templ, hud->font.texture, hud->font.texture->format);
   hud->font_sampler_view = pipe->create_sampler_view(pipe, hud->font.texture,
                                                      &view_templ);

   /* sampler state (for font drawing) */
   hud->font_sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   hud->font_sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   hud->font_sampler_state.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   hud->font_sampler_state.normalized_coords = 0;

   /* constants */
   hud->constbuf.buffer_size = sizeof(hud->constants);
   hud->constbuf.user_buffer = &hud->constants;

   LIST_INITHEAD(&hud->pane_list);

   hud_parse_env_var(hud, env);
   return hud;
}

void
hud_destroy(struct hud_context *hud)
{
   struct pipe_context *pipe = hud->pipe;
   struct hud_pane *pane, *pane_tmp;
   struct hud_graph *graph, *graph_tmp;

   LIST_FOR_EACH_ENTRY_SAFE(pane, pane_tmp, &hud->pane_list, head) {
      LIST_FOR_EACH_ENTRY_SAFE(graph, graph_tmp, &pane->graph_list, head) {
         LIST_DEL(&graph->head);
         hud_graph_destroy(graph);
      }
      LIST_DEL(&pane->head);
      FREE(pane);
   }

   pipe->delete_fs_state(pipe, hud->fs_color);
   pipe->delete_fs_state(pipe, hud->fs_text);
   pipe->delete_vs_state(pipe, hud->vs);
   pipe_sampler_view_reference(&hud->font_sampler_view, NULL);
   pipe_resource_reference(&hud->font.texture, NULL);
   u_upload_destroy(hud->uploader);
   FREE(hud);
}
