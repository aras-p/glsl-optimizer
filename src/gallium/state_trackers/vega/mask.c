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

#include "mask.h"

#include "path.h"
#include "image.h"
#include "shaders_cache.h"
#include "renderer.h"
#include "asm_util.h"
#include "st_inlines.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"

struct vg_mask_layer {
   struct vg_object base;

   VGint width;
   VGint height;

   struct pipe_texture *texture;
};

static INLINE struct pipe_surface *
alpha_mask_surface(struct vg_context *ctx, int usage)
{
   struct pipe_screen *screen = ctx->pipe->screen;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   return screen->get_tex_surface(screen,
                                  stfb->alpha_mask,
                                  0, 0, 0,
                                  usage);
}

static INLINE VGboolean
intersect_rectangles(VGint dwidth, VGint dheight,
                     VGint swidth, VGint sheight,
                     VGint tx, VGint ty,
                     VGint twidth, VGint theight,
                     VGint *offsets,
                     VGint *location)
{
   if (tx + twidth <= 0 || tx >= dwidth)
      return VG_FALSE;
   if (ty + theight <= 0 || ty >= dheight)
      return VG_FALSE;

   offsets[0] = 0;
   offsets[1] = 0;
   location[0] = tx;
   location[1] = ty;

   if (tx < 0) {
      offsets[0] -= tx;
      location[0] = 0;

      location[2] = MIN2(tx + swidth, MIN2(dwidth, tx + twidth));
      offsets[2] = location[2];
   } else {
      offsets[2] = MIN2(twidth, MIN2(dwidth - tx, swidth ));
      location[2] = offsets[2];
   }

   if (ty < 0) {
      offsets[1] -= ty;
      location[1] = 0;

      location[3] = MIN2(ty + sheight, MIN2(dheight, ty + theight));
      offsets[3] = location[3];
   } else {
      offsets[3] = MIN2(theight, MIN2(dheight - ty, sheight));
      location[3] = offsets[3];
   }

   return VG_TRUE;
}

#if DEBUG_MASKS
static void read_alpha_mask(void * data, VGint dataStride,
                            VGImageFormat dataFormat,
                            VGint sx, VGint sy,
                            VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;

   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->alpha_mask;
   struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;

   VGfloat temp[VEGA_MAX_IMAGE_WIDTH][4];
   VGfloat *df = (VGfloat*)temp;
   VGint y = (fb->height - sy) - 1, yStep = -1;
   VGint i;
   VGubyte *dst = (VGubyte *)data;
   VGint xoffset = 0, yoffset = 0;

   /* make sure rendering has completed */
   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
   if (sx < 0) {
      xoffset = -sx;
      xoffset *= _vega_size_for_format(dataFormat);
      width += sx;
      sx = 0;
   }
   if (sy < 0) {
      yoffset = -sy;
      height += sy;
      sy = 0;
      y = (fb->height - sy) - 1;
      yoffset *= dataStride;
   }

   {
      struct pipe_surface *surf;

      surf = screen->get_tex_surface(screen, strb->texture,  0, 0, 0,
                                     PIPE_BUFFER_USAGE_CPU_READ);

      /* Do a row at a time to flip image data vertically */
      for (i = 0; i < height; i++) {
#if 0
         debug_printf("%d-%d  == %d\n", sy, height, y);
#endif
         pipe_get_tile_rgba(surf, sx, y, width, 1, df);
         y += yStep;
         _vega_pack_rgba_span_float(ctx, width, temp, dataFormat,
                                    dst + yoffset + xoffset);
         dst += dataStride;
      }

      pipe_surface_reference(&surf, NULL);
   }
}

void save_alpha_to_file(const char *filename)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;
   VGint *data;
   int i, j;

   data = malloc(sizeof(int) * fb->width * fb->height);
   read_alpha_mask(data, fb->width * sizeof(int),
                   VG_sRGBA_8888,
                   0, 0, fb->width, fb->height);
   fprintf(stderr, "/*---------- start */\n");
   fprintf(stderr, "const int image_width = %d;\n",
           fb->width);
   fprintf(stderr, "const int image_height = %d;\n",
           fb->height);
   fprintf(stderr, "const int image_data = {\n");
   for (i = 0; i < fb->height; ++i) {
      for (j = 0; j < fb->width; ++j) {
         int rgba = data[i * fb->height + j];
         int argb = 0;
         argb = (rgba >> 8);
         argb |= ((rgba & 0xff) << 24);
         fprintf(stderr, "0x%x, ", argb);
      }
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "};\n");
   fprintf(stderr, "/*---------- end */\n");
}
#endif

static void setup_mask_framebuffer(struct pipe_surface *surf,
                                   VGint surf_width, VGint surf_height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_framebuffer_state fb;

   memset(&fb, 0, sizeof(fb));
   fb.width = surf_width;
   fb.height = surf_height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = surf;
   {
      VGint i;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         fb.cbufs[i] = 0;
   }
   cso_set_framebuffer(ctx->cso_context, &fb);
}


/* setup shader constants */
static void setup_mask_operation(VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_buffer **cbuf = &ctx->mask.cbuf;
   const VGint param_bytes = 4 * sizeof(VGfloat);
   const VGfloat ones[4] = {1.f, 1.f, 1.f, 1.f};
   void *shader = 0;

   /* We always need to get a new buffer, to keep the drivers simple and
    * avoid gratuitous rendering synchronization.
    */
   pipe_buffer_reference(cbuf, NULL);

   *cbuf = pipe_buffer_create(ctx->pipe->screen, 1,
                              PIPE_BUFFER_USAGE_CONSTANT,
                              param_bytes);
   if (*cbuf) {
      st_no_flush_pipe_buffer_write(ctx, *cbuf,
                                    0, param_bytes, ones);
   }

   ctx->pipe->set_constant_buffer(ctx->pipe, PIPE_SHADER_FRAGMENT, 0, *cbuf);
   switch (operation) {
   case VG_UNION_MASK: {
      if (!ctx->mask.union_fs) {
         ctx->mask.union_fs = shader_create_from_text(ctx->pipe,
                                                      union_mask_asm,
                                                      200,
                                                      PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.union_fs->driver;
   }
      break;
   case VG_INTERSECT_MASK: {
      if (!ctx->mask.intersect_fs) {
         ctx->mask.intersect_fs = shader_create_from_text(ctx->pipe,
                                                          intersect_mask_asm,
                                                          200,
                                                          PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.intersect_fs->driver;
   }
      break;
   case VG_SUBTRACT_MASK: {
      if (!ctx->mask.subtract_fs) {
         ctx->mask.subtract_fs = shader_create_from_text(ctx->pipe,
                                                         subtract_mask_asm,
                                                         200,
                                                         PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.subtract_fs->driver;
   }
      break;
   case VG_SET_MASK: {
      if (!ctx->mask.set_fs) {
         ctx->mask.set_fs = shader_create_from_text(ctx->pipe,
                                                    set_mask_asm,
                                                    200,
                                                    PIPE_SHADER_FRAGMENT);
      }
      shader = ctx->mask.set_fs->driver;
   }
      break;
   default:
         assert(0);
      break;
   }
   cso_set_fragment_shader_handle(ctx->cso_context, shader);
}

static void setup_mask_samplers(struct pipe_texture *umask)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
   struct st_framebuffer *fb_buffers = ctx->draw_buffer;
   struct pipe_texture *uprev = NULL;
   struct pipe_sampler_state sampler;

   uprev = fb_buffers->blend_texture;
   sampler = ctx->mask.sampler;
   sampler.normalized_coords = 1;

   samplers[0] = NULL;
   samplers[1] = NULL;
   samplers[2] = NULL;
   textures[0] = NULL;
   textures[1] = NULL;
   textures[2] = NULL;

   samplers[0] = &sampler;
   samplers[1] = &ctx->mask.sampler;

   textures[0] = umask;
   textures[1] = uprev;

   cso_set_samplers(ctx->cso_context, 2,
                    (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_textures(ctx->cso_context, 2, textures);
}


/* setup shader constants */
static void setup_mask_fill(const VGfloat color[4])
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_buffer **cbuf = &ctx->mask.cbuf;
   const VGint param_bytes = 4 * sizeof(VGfloat);

   /* We always need to get a new buffer, to keep the drivers simple and
    * avoid gratuitous rendering synchronization.
    */
   pipe_buffer_reference(cbuf, NULL);

   *cbuf = pipe_buffer_create(ctx->pipe->screen, 1,
                              PIPE_BUFFER_USAGE_CONSTANT,
                              param_bytes);
   if (*cbuf) {
      st_no_flush_pipe_buffer_write(ctx, *cbuf, 0, param_bytes, color);
   }

   ctx->pipe->set_constant_buffer(ctx->pipe, PIPE_SHADER_FRAGMENT, 0, *cbuf);
   cso_set_fragment_shader_handle(ctx->cso_context,
                                  shaders_cache_fill(ctx->sc,
                                                     VEGA_SOLID_FILL_SHADER));
}

static void setup_mask_viewport()
{
   struct vg_context *ctx = vg_current_context();
   vg_set_viewport(ctx, VEGA_Y0_TOP);
}

static void setup_mask_blend()
{
   struct vg_context *ctx = vg_current_context();

   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 0;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;

   cso_set_blend(ctx->cso_context, &blend);
}


static void surface_fill(struct pipe_surface *surf,
                         int surf_width, int surf_height,
                         int x, int y, int width, int height,
                         const VGfloat color[4])
{
   struct vg_context *ctx = vg_current_context();

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }

   cso_save_framebuffer(ctx->cso_context);
   cso_save_blend(ctx->cso_context);
   cso_save_fragment_shader(ctx->cso_context);
   cso_save_viewport(ctx->cso_context);

   setup_mask_blend();
   setup_mask_fill(color);
   setup_mask_framebuffer(surf, surf_width, surf_height);
   setup_mask_viewport();

   renderer_draw_quad(ctx->renderer, x, y,
                      x + width, y + height, 0.0f/*depth should be disabled*/);


   /* make sure rendering has completed */
   ctx->pipe->flush(ctx->pipe,
                    PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME,
                    NULL);

#if DEBUG_MASKS
   save_alpha_to_file(0);
#endif

   cso_restore_blend(ctx->cso_context);
   cso_restore_framebuffer(ctx->cso_context);
   cso_restore_fragment_shader(ctx->cso_context);
   cso_restore_viewport(ctx->cso_context);
}


static void mask_using_texture(struct pipe_texture *texture,
                               VGMaskOperation operation,
                               VGint x, VGint y,
                               VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_surface *surface =
      alpha_mask_surface(ctx, PIPE_BUFFER_USAGE_GPU_WRITE);
   VGint offsets[4], loc[4];

   if (!surface)
      return;
   if (!intersect_rectangles(surface->width, surface->height,
                             texture->width0, texture->height0,
                             x, y, width, height,
                             offsets, loc))
      return;
#if 0
   debug_printf("Offset = [%d, %d, %d, %d]\n", offsets[0],
                offsets[1], offsets[2], offsets[3]);
   debug_printf("Locati = [%d, %d, %d, %d]\n", loc[0],
                loc[1], loc[2], loc[3]);
#endif

   /* prepare our blend surface */
   vg_prepare_blend_surface_from_mask(ctx);

   cso_save_samplers(ctx->cso_context);
   cso_save_sampler_textures(ctx->cso_context);
   cso_save_framebuffer(ctx->cso_context);
   cso_save_blend(ctx->cso_context);
   cso_save_fragment_shader(ctx->cso_context);
   cso_save_viewport(ctx->cso_context);

   setup_mask_samplers(texture);
   setup_mask_blend();
   setup_mask_operation(operation);
   setup_mask_framebuffer(surface, surface->width, surface->height);
   setup_mask_viewport();

   /* render the quad to propagate the rendering from stencil */
   renderer_draw_texture(ctx->renderer, texture,
                         offsets[0], offsets[1],
                         offsets[0] + offsets[2], offsets[1] + offsets[3],
                         loc[0], loc[1], loc[0] + loc[2], loc[1] + loc[3]);

   /* make sure rendering has completed */
   ctx->pipe->flush(ctx->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
   cso_restore_blend(ctx->cso_context);
   cso_restore_framebuffer(ctx->cso_context);
   cso_restore_fragment_shader(ctx->cso_context);
   cso_restore_samplers(ctx->cso_context);
   cso_restore_sampler_textures(ctx->cso_context);
   cso_restore_viewport(ctx->cso_context);

   pipe_surface_reference(&surface, NULL);
}


#ifdef OPENVG_VERSION_1_1

struct vg_mask_layer * mask_layer_create(VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_mask_layer *mask = 0;

   mask = CALLOC_STRUCT(vg_mask_layer);
   vg_init_object(&mask->base, ctx, VG_OBJECT_MASK);
   mask->width = width;
   mask->height = height;

   {
      struct pipe_texture pt;
      struct pipe_screen *screen = ctx->pipe->screen;

      memset(&pt, 0, sizeof(pt));
      pt.target = PIPE_TEXTURE_2D;
      pt.format = PIPE_FORMAT_B8G8R8A8_UNORM;
      pt.last_level = 0;
      pt.width0 = width;
      pt.height0 = height;
      pt.depth0 = 1;
      pt.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;
      pt.compressed = 0;

      mask->texture = screen->texture_create(screen, &pt);
   }

   vg_context_add_object(ctx, VG_OBJECT_MASK, mask);

   return mask;
}

void mask_layer_destroy(struct vg_mask_layer *layer)
{
   struct vg_context *ctx = vg_current_context();

   vg_context_remove_object(ctx, VG_OBJECT_MASK, layer);
   pipe_texture_release(&layer->texture);
   free(layer);
}

void mask_layer_fill(struct vg_mask_layer *layer,
                     VGint x, VGint y,
                     VGint width, VGint height,
                     VGfloat value)
{
   struct vg_context *ctx = vg_current_context();
   VGfloat alpha_color[4] = {0, 0, 0, 0};
   struct pipe_surface *surface;

   alpha_color[3] = value;

   surface = ctx->pipe->screen->get_tex_surface(
      ctx->pipe->screen, layer->texture,
      0, 0, 0,
      PIPE_BUFFER_USAGE_GPU_WRITE);

   surface_fill(surface,
                layer->width, layer->height,
                x, y, width, height, alpha_color);

   ctx->pipe->screen->tex_surface_release(ctx->pipe->screen, &surface);
}

void mask_copy(struct vg_mask_layer *layer,
               VGint sx, VGint sy,
               VGint dx, VGint dy,
               VGint width, VGint height)
{
    struct vg_context *ctx = vg_current_context();
    struct st_framebuffer *fb_buffers = ctx->draw_buffer;

    renderer_copy_texture(ctx->renderer,
                          layer->texture,
                          sx, sy,
                          sx + width, sy + height,
                          fb_buffers->alpha_mask,
                          dx, dy,
                          dx + width, dy + height);
}

static void mask_layer_render_to(struct vg_mask_layer *layer,
                                 struct path *path,
                                 VGbitfield paint_modes)
{
   struct vg_context *ctx = vg_current_context();
   const VGfloat fill_color[4] = {1.f, 1.f, 1.f, 1.f};
   struct pipe_screen *screen = ctx->pipe->screen;
   struct pipe_surface *surface;

   surface = screen->get_tex_surface(screen, layer->texture,  0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   cso_save_framebuffer(ctx->cso_context);
   cso_save_fragment_shader(ctx->cso_context);
   cso_save_viewport(ctx->cso_context);

   setup_mask_blend();
   setup_mask_fill(fill_color);
   setup_mask_framebuffer(surface, layer->width, layer->height);
   setup_mask_viewport();

   if (paint_modes & VG_FILL_PATH) {
      struct matrix *mat = &ctx->state.vg.path_user_to_surface_matrix;
      path_fill(path, mat);
   }

   if (paint_modes & VG_STROKE_PATH){
      path_stroke(path);
   }


   /* make sure rendering has completed */
   ctx->pipe->flush(ctx->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   cso_restore_framebuffer(ctx->cso_context);
   cso_restore_fragment_shader(ctx->cso_context);
   cso_restore_viewport(ctx->cso_context);
   ctx->state.dirty |= BLEND_DIRTY;

   screen->tex_surface_release(ctx->pipe->screen, &surface);
}

void mask_render_to(struct path *path,
                    VGbitfield paint_modes,
                    VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();
   struct st_framebuffer *fb_buffers = ctx->draw_buffer;
   struct vg_mask_layer *temp_layer;
   VGint width, height;

   width = fb_buffers->alpha_mask->width0;
   height = fb_buffers->alpha_mask->width0;

   temp_layer = mask_layer_create(width, height);

   mask_layer_render_to(temp_layer, path, paint_modes);

   mask_using_layer(temp_layer, 0, 0, width, height,
                    operation);

   mask_layer_destroy(temp_layer);
}

void mask_using_layer(struct vg_mask_layer *layer,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height)
{
   mask_using_texture(layer->texture, operation,
                      x, y, width, height);
}

VGint mask_layer_width(struct vg_mask_layer *layer)
{
   return layer->width;
}

VGint mask_layer_height(struct vg_mask_layer *layer)
{
   return layer->height;
}


#endif

void mask_using_image(struct vg_image *image,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height)
{
   mask_using_texture(image->texture, operation,
                      x, y, width, height);
}

void mask_fill(VGint x, VGint y, VGint width, VGint height,
               VGfloat value)
{
   struct vg_context *ctx = vg_current_context();
   VGfloat alpha_color[4] = {.0f, .0f, .0f, value};
   struct pipe_surface *surf = alpha_mask_surface(
      ctx, PIPE_BUFFER_USAGE_GPU_WRITE);

#if DEBUG_MASKS
   debug_printf("mask_fill(%d, %d, %d, %d) with  rgba(%f, %f, %f, %f)\n",
                x, y, width, height,
                alpha_color[0], alpha_color[1],
                alpha_color[2], alpha_color[3]);
   debug_printf("XXX %f  === %f \n",
                alpha_color[3], value);
#endif

   surface_fill(surf, surf->width, surf->height,
                x, y, width, height, alpha_color);

   pipe_surface_reference(&surf, NULL);
}

VGint mask_bind_samplers(struct pipe_sampler_state **samplers,
                         struct pipe_texture **textures)
{
   struct vg_context *ctx = vg_current_context();

   if (ctx->state.vg.masking) {
      struct st_framebuffer *fb_buffers = ctx->draw_buffer;

      samplers[1] = &ctx->mask.sampler;
      textures[1] = fb_buffers->alpha_mask;
      return 1;
   } else
      return 0;
}
