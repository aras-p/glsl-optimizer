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

#include "VG/openvg.h"

#include "mask.h"
#include "renderer.h"

#include "vg_context.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"

#include "util/u_pack_color.h"
#include "util/u_draw_quad.h"
#include "util/u_memory.h"


#define DISABLE_1_1_MASKING 1

/**
 * Draw a screen-aligned quadrilateral.
 * Coords are window coords with y=0=bottom.  These coords will be transformed
 * by the vertex shader and viewport transform.
 */
static void
draw_clear_quad(struct vg_context *st,
                float x0, float y0, float x1, float y1, float z,
                const VGfloat color[4])
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_buffer *buf;
   VGuint i;

   /* positions */
   st->clear.vertices[0][0][0] = x0;
   st->clear.vertices[0][0][1] = y0;

   st->clear.vertices[1][0][0] = x1;
   st->clear.vertices[1][0][1] = y0;

   st->clear.vertices[2][0][0] = x1;
   st->clear.vertices[2][0][1] = y1;

   st->clear.vertices[3][0][0] = x0;
   st->clear.vertices[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      st->clear.vertices[i][0][2] = z;
      st->clear.vertices[i][0][3] = 1.0;
      st->clear.vertices[i][1][0] = color[0];
      st->clear.vertices[i][1][1] = color[1];
      st->clear.vertices[i][1][2] = color[2];
      st->clear.vertices[i][1][3] = color[3];
   }


   /* put vertex data into vbuf */
   buf =  pipe_user_buffer_create(pipe->screen,
                                  st->clear.vertices,
                                  sizeof(st->clear.vertices));


   /* draw */
   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}

/**
 * Do vgClear by drawing a quadrilateral.
 */
static void
clear_with_quad(struct vg_context *st, float x0, float y0,
                float width, float height, const VGfloat clear_color[4])
{
   VGfloat x1, y1;

   vg_validate_state(st);

   x1 = x0 + width;
   y1 = y0 + height;

   /*
     printf("%s %f,%f %f,%f\n", __FUNCTION__,
     x0, y0,
     x1, y1);
   */

   if (st->pipe->screen && st->pipe->screen->update_buffer)
      st->pipe->screen->update_buffer( st->pipe->screen,
                                       st->pipe->priv );

   cso_save_blend(st->cso_context);
   cso_save_rasterizer(st->cso_context);
   cso_save_fragment_shader(st->cso_context);
   cso_save_vertex_shader(st->cso_context);

   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].colormask = PIPE_MASK_RGBA;
      cso_set_blend(st->cso_context, &blend);
   }

   cso_set_rasterizer(st->cso_context, &st->clear.raster);

   cso_set_fragment_shader_handle(st->cso_context, st->clear.fs);
   cso_set_vertex_shader_handle(st->cso_context, vg_clear_vs(st));

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_clear_quad(st, x0, y0, x1, y1, 0, clear_color);

   /* Restore pipe state */
   cso_restore_blend(st->cso_context);
   cso_restore_rasterizer(st->cso_context);
   cso_restore_fragment_shader(st->cso_context);
   cso_restore_vertex_shader(st->cso_context);
}


void vgMask(VGHandle mask, VGMaskOperation operation,
            VGint x, VGint y,
            VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();

   if (width <=0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (operation < VG_CLEAR_MASK || operation > VG_SUBTRACT_MASK) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }


   vg_validate_state(ctx);

   if (operation == VG_CLEAR_MASK) {
      mask_fill(x, y, width, height, 0.f);
   } else if (operation == VG_FILL_MASK) {
      mask_fill(x, y, width, height, 1.f);
   } else if (vg_object_is_valid((void*)mask, VG_OBJECT_IMAGE)) {
      struct vg_image *image = (struct vg_image *)mask;
      mask_using_image(image, operation, x, y, width, height);
   } else if (vg_object_is_valid((void*)mask, VG_OBJECT_MASK)) {
#if DISABLE_1_1_MASKING
      return;
#else
      struct vg_mask_layer *layer = (struct vg_mask_layer *)mask;
      mask_using_layer(layer, operation, x, y, width, height);
#endif
   } else {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
   }
}

void vgClear(VGint x, VGint y,
             VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_framebuffer_state *fb;

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);
#if 0
   debug_printf("Clear [%d, %d, %d, %d] with [%f, %f, %f, %f]\n",
                x, y, width, height,
                ctx->state.vg.clear_color[0],
                ctx->state.vg.clear_color[1],
                ctx->state.vg.clear_color[2],
                ctx->state.vg.clear_color[3]);
#endif

   fb = &ctx->state.g3d.fb;
   /* check for a whole surface clear */
   if (!ctx->state.vg.scissoring &&
       (x == 0 && y == 0 && width == fb->width && height == fb->height)) {
      ctx->pipe->clear(ctx->pipe, PIPE_CLEAR_COLOR | PIPE_CLEAR_DEPTHSTENCIL,
                       ctx->state.vg.clear_color, 1., 0);
   } else {
      clear_with_quad(ctx, x, y, width, height, ctx->state.vg.clear_color);
   }
}


#ifdef OPENVG_VERSION_1_1


void vgRenderToMask(VGPath path,
                    VGbitfield paintModes,
                    VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!paintModes || (paintModes&(~(VG_STROKE_PATH|VG_FILL_PATH)))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (operation < VG_CLEAR_MASK ||
       operation > VG_SUBTRACT_MASK) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)path, VG_OBJECT_PATH)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

#if DISABLE_1_1_MASKING
   return;
#endif

   vg_validate_state(ctx);

   mask_render_to((struct path *)path, paintModes, operation);
}

VGMaskLayer vgCreateMaskLayer(VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();

   if (width <= 0 || height <= 0 ||
       width > vgGeti(VG_MAX_IMAGE_WIDTH) ||
       height > vgGeti(VG_MAX_IMAGE_HEIGHT)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return (VGMaskLayer)mask_layer_create(width, height);
}

void vgDestroyMaskLayer(VGMaskLayer maskLayer)
{
   struct vg_mask_layer *mask = 0;
   struct vg_context *ctx = vg_current_context();

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   mask = (struct vg_mask_layer *)maskLayer;
   mask_layer_destroy(mask);
}

void vgFillMaskLayer(VGMaskLayer maskLayer,
                     VGint x, VGint y,
                     VGint width, VGint height,
                     VGfloat value)
{
   struct vg_mask_layer *mask = 0;
   struct vg_context *ctx = vg_current_context();

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (value < 0 || value > 1) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (x < 0 || y < 0 || (x + width) < 0 || (y + height) < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   mask = (struct vg_mask_layer*)maskLayer;

   if (x + width > mask_layer_width(mask) ||
       y + height > mask_layer_height(mask)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

#if DISABLE_1_1_MASKING
   return;
#endif
   mask_layer_fill(mask, x, y, width, height, value);
}

void vgCopyMask(VGMaskLayer maskLayer,
                VGint sx, VGint sy,
                VGint dx, VGint dy,
                VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_mask_layer *mask = 0;

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

#if DISABLE_1_1_MASKING
   return;
#endif

   mask = (struct vg_mask_layer*)maskLayer;
   mask_copy(mask, sx, sy, dx, dy, width, height);
}

#endif
