/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "polygon.h"

#include "matrix.h" /*for floatsEqual*/
#include "vg_context.h"
#include "vg_state.h"
#include "paint.h"
#include "renderer.h"
#include "util_array.h"
#include "VG/openvg.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"

#include "util/u_draw_quad.h"
#include "util/u_math.h"

#include <string.h>
#include <stdlib.h>

#define DEBUG_POLYGON 0

#define COMPONENTS 2

struct polygon
{
   VGfloat *data;
   VGint    size;

   VGint    num_verts;

   VGboolean dirty;
   struct pipe_buffer *vbuf;
   struct pipe_screen *screen;
};

static float *ptr_to_vertex(float *data, int idx)
{
   return data + (idx * COMPONENTS);
}

#if 0
static void polygon_print(struct polygon *poly)
{
   int i;
   float *vert;
   debug_printf("Polygon %p, size = %d\n", poly, poly->num_verts);
   for (i = 0; i < poly->num_verts; ++i) {
      vert = ptr_to_vertex(poly->data, i);
      debug_printf("%f, %f,  ", vert[0], vert[1]);
   }
   debug_printf("\nend\n");
}
#endif


struct polygon * polygon_create(int size)
{
   struct polygon *poly = (struct polygon*)malloc(sizeof(struct polygon));

   poly->data = malloc(sizeof(float) * COMPONENTS * size);
   poly->size = size;
   poly->num_verts = 0;
   poly->dirty = VG_TRUE;
   poly->vbuf = NULL;

   return poly;
}

struct polygon * polygon_create_from_data(float *data, int size)
{
   struct polygon *poly = polygon_create(size);

   memcpy(poly->data, data, sizeof(float) * COMPONENTS * size);
   poly->num_verts = size;
   poly->dirty = VG_TRUE;
   poly->vbuf = NULL;

   return poly;
}

void polygon_destroy(struct polygon *poly)
{
   if (poly->vbuf)
      pipe_buffer_reference(&poly->vbuf, NULL);

   free(poly->data);
   free(poly);
}

void polygon_resize(struct polygon *poly, int new_size)
{
   float *data = (float*)malloc(sizeof(float) * COMPONENTS * new_size);
   int size = MIN2(sizeof(float) * COMPONENTS * new_size,
                   sizeof(float) * COMPONENTS * poly->size);
   memcpy(data, poly->data, size);
   free(poly->data);
   poly->data = data;
   poly->size = new_size;
   poly->dirty = VG_TRUE;
}

int polygon_size(struct polygon *poly)
{
   return poly->size;
}

int polygon_vertex_count(struct polygon *poly)
{
   return poly->num_verts;
}

float * polygon_data(struct polygon *poly)
{
   return poly->data;
}

void polygon_vertex_append(struct polygon *p,
                           float x, float y)
{
   float *vert;
#if DEBUG_POLYGON
   debug_printf("Append vertex [%f, %f]\n", x, y);
#endif
   if (p->num_verts >= p->size) {
      polygon_resize(p, p->size * 2);
   }

   vert = ptr_to_vertex(p->data, p->num_verts);
   vert[0] = x;
   vert[1] = y;
   ++p->num_verts;
   p->dirty = VG_TRUE;
}

void polygon_set_vertex(struct polygon *p, int idx,
                        float x, float y)
{
   float *vert;
   if (idx >= p->num_verts) {
      /*fixme: error reporting*/
      abort();
      return;
   }

   vert = ptr_to_vertex(p->data, idx);
   vert[0] = x;
   vert[1] = y;
   p->dirty = VG_TRUE;
}

void polygon_vertex(struct polygon *p, int idx,
                    float *vertex)
{
   float *vert;
   if (idx >= p->num_verts) {
      /*fixme: error reporting*/
      abort();
      return;
   }

   vert = ptr_to_vertex(p->data, idx);
   vertex[0] = vert[0];
   vertex[1] = vert[1];
}

void polygon_bounding_rect(struct polygon *p,
                           float *rect)
{
   int i;
   float minx, miny, maxx, maxy;
   float *vert = ptr_to_vertex(p->data, 0);
   minx = vert[0];
   maxx = vert[0];
   miny = vert[1];
   maxy = vert[1];

   for (i = 1; i < p->num_verts; ++i) {
      vert = ptr_to_vertex(p->data, i);
      minx = MIN2(vert[0], minx);
      miny = MIN2(vert[1], miny);

      maxx = MAX2(vert[0], maxx);
      maxy = MAX2(vert[1], maxy);
   }

   rect[0] = minx;
   rect[1] = miny;
   rect[2] = maxx - minx;
   rect[3] = maxy - miny;
}

int polygon_contains_point(struct polygon *p,
                           float x, float y)
{
   return 0;
}

void polygon_append_polygon(struct polygon *dst,
                            struct polygon *src)
{
   if (dst->num_verts + src->num_verts >= dst->size) {
      polygon_resize(dst, dst->num_verts + src->num_verts * 1.5);
   }
   memcpy(ptr_to_vertex(dst->data, dst->num_verts),
          src->data, src->num_verts * COMPONENTS * sizeof(VGfloat));
   dst->num_verts += src->num_verts;
}

VGboolean polygon_is_closed(struct polygon *p)
{
   VGfloat start[2], end[2];

   polygon_vertex(p, 0, start);
   polygon_vertex(p, p->num_verts - 1, end);

   return floatsEqual(start[0], end[0]) && floatsEqual(start[1], end[1]);
}

static void set_blend_for_fill(struct pipe_blend_state *blend)
{
   memset(blend, 0, sizeof(struct pipe_blend_state));
   blend->rt[0].colormask = 0; /*disable colorwrites*/

   blend->rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend->rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend->rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   blend->rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
}

static void draw_polygon(struct vg_context *ctx,
                         struct polygon *poly)
{
   int vert_size;
   struct pipe_context *pipe;
   struct pipe_vertex_buffer vbuffer;
   struct pipe_vertex_element velement;

   vert_size = poly->num_verts * COMPONENTS * sizeof(float);

   /*polygon_print(poly);*/

   pipe = ctx->pipe;

   if (poly->vbuf == NULL || poly->dirty) {
      if (poly->vbuf) {
         pipe_buffer_reference(&poly->vbuf,
                               NULL);
      }
      poly->screen = pipe->screen;
      poly->vbuf= pipe_user_buffer_create(poly->screen,
                                          poly->data,
                                          vert_size);
      poly->dirty = VG_FALSE;
   }


   /* tell pipe about the vertex buffer */
   memset(&vbuffer, 0, sizeof(vbuffer));
   vbuffer.buffer = poly->vbuf;
   vbuffer.stride = COMPONENTS * sizeof(float);  /* vertex size */
   vbuffer.buffer_offset = 0;
   vbuffer.max_index = poly->num_verts - 1;
   pipe->set_vertex_buffers(pipe, 1, &vbuffer);

   /* tell pipe about the vertex attributes */
   velement.src_offset = 0;
   velement.instance_divisor = 0;
   velement.vertex_buffer_index = 0;
   velement.src_format = PIPE_FORMAT_R32G32_FLOAT;
   velement.nr_components = COMPONENTS;
   pipe->set_vertex_elements(pipe, 1, &velement);

   /* draw */
   pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLE_FAN, 
                     0, poly->num_verts);
}

void polygon_fill(struct polygon *poly, struct vg_context *ctx)
{
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_stencil_ref sr;
   struct pipe_blend_state blend;
   VGfloat bounds[4];
   VGfloat min_x, min_y, max_x, max_y;
   assert(poly);
   polygon_bounding_rect(poly, bounds);
   min_x = bounds[0];
   min_y = bounds[1];
   max_x = bounds[0] + bounds[2];
   max_y = bounds[1] + bounds[3];

#if DEBUG_POLYGON
   debug_printf("Poly bounds are [%f, %f], [%f, %f]\n",
                min_x, min_y, max_x, max_y);
#endif

   set_blend_for_fill(&blend);

   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
   memset(&sr, 0, sizeof(struct pipe_stencil_ref));
   /* only need a fixed 0. Rely on default or move it out at least? */
   cso_set_stencil_ref(ctx->cso_context, &sr);

   cso_save_blend(ctx->cso_context);
   cso_save_depth_stencil_alpha(ctx->cso_context);

   dsa.stencil[0].enabled = 1;
   if (ctx->state.vg.fill_rule == VG_EVEN_ODD) {
      dsa.stencil[0].writemask = 1;
      dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INVERT;
      dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
      dsa.stencil[0].valuemask = ~0;

      cso_set_blend(ctx->cso_context, &blend);
      cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
      draw_polygon(ctx, poly);
   } else if (ctx->state.vg.fill_rule == VG_NON_ZERO) {
      struct pipe_screen *screen = ctx->pipe->screen;

      if (screen->get_param(screen, PIPE_CAP_TWO_SIDED_STENCIL)) {
         /* front */
         dsa.stencil[0].writemask = ~0;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;
         dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[0].valuemask = ~0;

         /* back */
         dsa.stencil[1].enabled = 1;
         dsa.stencil[1].writemask = ~0;
         dsa.stencil[1].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[1].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[1].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;
         dsa.stencil[1].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[1].valuemask = ~0;

         cso_set_blend(ctx->cso_context, &blend);
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         draw_polygon(ctx, poly);
      } else {
         struct pipe_rasterizer_state raster;

         memcpy(&raster, &ctx->state.g3d.rasterizer, sizeof(struct pipe_rasterizer_state));

         cso_save_rasterizer(ctx->cso_context);
         dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[0].valuemask = ~0;

         raster.cull_mode = raster.front_winding ^ PIPE_WINDING_BOTH;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;

         cso_set_blend(ctx->cso_context, &blend);
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         cso_set_rasterizer(ctx->cso_context, &raster);
         draw_polygon(ctx, poly);

         raster.cull_mode = raster.front_winding;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         cso_set_rasterizer(ctx->cso_context, &raster);
         draw_polygon(ctx, poly);

         cso_restore_rasterizer(ctx->cso_context);
      }
   }

   /* restore color writes */
   cso_restore_blend(ctx->cso_context);
   /* setup stencil ops */
   dsa.stencil[0].func = PIPE_FUNC_NOTEQUAL;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = dsa.stencil[0].writemask;
   dsa.stencil[1].enabled = 0;
   memcpy(&dsa.depth, &ctx->state.g3d.dsa.depth,
          sizeof(struct pipe_depth_state));
   cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);

   /* render the quad to propagate the rendering from stencil */
   renderer_draw_quad(ctx->renderer, min_x, min_y,
                      max_x, max_y, 0.0f/*depth should be disabled*/);

   cso_restore_depth_stencil_alpha(ctx->cso_context);
}

void polygon_array_fill(struct polygon_array *polyarray, struct vg_context *ctx)
{
   struct array *polys = polyarray->array;
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_stencil_ref sr;
   struct pipe_blend_state blend;
   VGfloat min_x = polyarray->min_x;
   VGfloat min_y = polyarray->min_y;
   VGfloat max_x = polyarray->max_x;
   VGfloat max_y = polyarray->max_y;
   VGint i;


#if DEBUG_POLYGON
   debug_printf("%s: Poly bounds are [%f, %f], [%f, %f]\n",
                __FUNCTION__,
                min_x, min_y, max_x, max_y);
#endif

   set_blend_for_fill(&blend);

   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
   memset(&sr, 0, sizeof(struct pipe_stencil_ref));
   /* only need a fixed 0. Rely on default or move it out at least? */
   cso_set_stencil_ref(ctx->cso_context, &sr);

   cso_save_blend(ctx->cso_context);
   cso_save_depth_stencil_alpha(ctx->cso_context);

   dsa.stencil[0].enabled = 1;
   if (ctx->state.vg.fill_rule == VG_EVEN_ODD) {
      dsa.stencil[0].writemask = 1;
      dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INVERT;
      dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
      dsa.stencil[0].valuemask = ~0;

      cso_set_blend(ctx->cso_context, &blend);
      cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
      for (i = 0; i < polys->num_elements; ++i) {
         struct polygon *poly = (((struct polygon**)polys->data)[i]);
         draw_polygon(ctx, poly);
      }
   } else if (ctx->state.vg.fill_rule == VG_NON_ZERO) {
      struct pipe_screen *screen = ctx->pipe->screen;

      if (screen->get_param(screen, PIPE_CAP_TWO_SIDED_STENCIL)) {
         /* front */
         dsa.stencil[0].writemask = ~0;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;
         dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[0].valuemask = ~0;

         /* back */
         dsa.stencil[1].enabled = 1;
         dsa.stencil[1].writemask = ~0;
         dsa.stencil[1].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[1].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[1].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;
         dsa.stencil[1].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[1].valuemask = ~0;

         cso_set_blend(ctx->cso_context, &blend);
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         for (i = 0; i < polys->num_elements; ++i) {
            struct polygon *poly = (((struct polygon**)polys->data)[i]);
            draw_polygon(ctx, poly);
         }
      } else {
         struct pipe_rasterizer_state raster;

         memcpy(&raster, &ctx->state.g3d.rasterizer, sizeof(struct pipe_rasterizer_state));

         cso_save_rasterizer(ctx->cso_context);
         dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
         dsa.stencil[0].valuemask = ~0;

         raster.cull_mode = raster.front_winding ^ PIPE_WINDING_BOTH;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;

         cso_set_blend(ctx->cso_context, &blend);
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         cso_set_rasterizer(ctx->cso_context, &raster);
         for (i = 0; i < polys->num_elements; ++i) {
            struct polygon *poly = (((struct polygon**)polys->data)[i]);
            draw_polygon(ctx, poly);
         }

         raster.cull_mode = raster.front_winding;
         dsa.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
         dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;
         cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);
         cso_set_rasterizer(ctx->cso_context, &raster);
         for (i = 0; i < polys->num_elements; ++i) {
            struct polygon *poly = (((struct polygon**)polys->data)[i]);
            draw_polygon(ctx, poly);
         }

         cso_restore_rasterizer(ctx->cso_context);
      }
   }

   /* restore color writes */
   cso_restore_blend(ctx->cso_context);
   /* setup stencil ops */
   dsa.stencil[0].func = PIPE_FUNC_NOTEQUAL;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = dsa.stencil[0].writemask;
   dsa.stencil[1].enabled = 0;
   memcpy(&dsa.depth, &ctx->state.g3d.dsa.depth,
          sizeof(struct pipe_depth_state));
   cso_set_depth_stencil_alpha(ctx->cso_context, &dsa);

   /* render the quad to propagate the rendering from stencil */
   renderer_draw_quad(ctx->renderer, min_x, min_y,
                      max_x, max_y, 0.0f/*depth should be disabled*/);

   cso_restore_depth_stencil_alpha(ctx->cso_context);
}
