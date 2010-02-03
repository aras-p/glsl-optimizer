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

#include "shader.h"

#include "vg_context.h"
#include "shaders_cache.h"
#include "paint.h"
#include "mask.h"
#include "image.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

#define MAX_CONSTANTS 20

struct shader {
   struct vg_context *context;

   VGboolean masking;
   struct vg_paint *paint;
   struct vg_image *image;

   VGboolean drawing_image;
   VGImageMode image_mode;

   float constants[MAX_CONSTANTS];
   struct pipe_buffer *cbuf;
   struct pipe_shader_state fs_state;
   void *fs;
};

struct shader * shader_create(struct vg_context *ctx)
{
   struct shader *shader = 0;

   shader = CALLOC_STRUCT(shader);
   shader->context = ctx;

   return shader;
}

void shader_destroy(struct shader *shader)
{
   free(shader);
}

void shader_set_masking(struct shader *shader, VGboolean set)
{
   shader->masking = set;
}

VGboolean shader_is_masking(struct shader *shader)
{
   return shader->masking;
}

void shader_set_paint(struct shader *shader, struct vg_paint *paint)
{
   shader->paint = paint;
}

struct vg_paint * shader_paint(struct shader *shader)
{
   return shader->paint;
}


static void setup_constant_buffer(struct shader *shader)
{
   struct vg_context *ctx = shader->context;
   struct pipe_context *pipe = shader->context->pipe;
   struct pipe_buffer **cbuf = &shader->cbuf;
   VGint param_bytes = paint_constant_buffer_size(shader->paint);
   float temp_buf[MAX_CONSTANTS];

   assert(param_bytes <= sizeof(temp_buf));
   paint_fill_constant_buffer(shader->paint, temp_buf);

   if (*cbuf == NULL ||
       memcmp(temp_buf, shader->constants, param_bytes) != 0)
   {
      pipe_buffer_reference(cbuf, NULL);

      memcpy(shader->constants, temp_buf, param_bytes);
      *cbuf = pipe_user_buffer_create(pipe->screen,
                                      &shader->constants,
                                      sizeof(shader->constants));
   }

   ctx->pipe->set_constant_buffer(ctx->pipe, PIPE_SHADER_FRAGMENT, 0, *cbuf);
}

static VGint blend_bind_samplers(struct vg_context *ctx,
                                 struct pipe_sampler_state **samplers,
                                 struct pipe_texture **textures)
{
   VGBlendMode bmode = ctx->state.vg.blend_mode;

   if (bmode == VG_BLEND_MULTIPLY ||
       bmode == VG_BLEND_SCREEN ||
       bmode == VG_BLEND_DARKEN ||
       bmode == VG_BLEND_LIGHTEN) {
      struct st_framebuffer *stfb = ctx->draw_buffer;

      vg_prepare_blend_surface(ctx);

      samplers[2] = &ctx->blend_sampler;
      textures[2] = stfb->blend_texture;

      if (!samplers[0] || !textures[0]) {
         samplers[0] = samplers[2];
         textures[0] = textures[2];
      }
      if (!samplers[1] || !textures[1]) {
         samplers[1] = samplers[0];
         textures[1] = textures[0];
      }

      return 1;
   }
   return 0;
}

static void setup_samplers(struct shader *shader)
{
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
   struct vg_context *ctx = shader->context;
   /* a little wonky: we use the num as a boolean that just says
    * whether any sampler/textures have been set. the actual numbering
    * for samplers is always the same:
    * 0 - paint sampler/texture for gradient/pattern
    * 1 - mask sampler/texture
    * 2 - blend sampler/texture
    * 3 - image sampler/texture
    * */
   VGint num = 0;

   samplers[0] = NULL;
   samplers[1] = NULL;
   samplers[2] = NULL;
   samplers[3] = NULL;
   textures[0] = NULL;
   textures[1] = NULL;
   textures[2] = NULL;
   textures[3] = NULL;

   num += paint_bind_samplers(shader->paint, samplers, textures);
   num += mask_bind_samplers(samplers, textures);
   num += blend_bind_samplers(ctx, samplers, textures);
   if (shader->drawing_image && shader->image)
      num += image_bind_samplers(shader->image, samplers, textures);

   if (num) {
      cso_set_samplers(ctx->cso_context, 4, (const struct pipe_sampler_state **)samplers);
      cso_set_sampler_textures(ctx->cso_context, 4, textures);
   }
}

static INLINE VGboolean is_format_bw(struct shader *shader)
{
#if 0
   struct vg_context *ctx = shader->context;
   struct st_framebuffer *stfb = ctx->draw_buffer;
#endif

   if (shader->drawing_image && shader->image) {
      if (shader->image->format == VG_BW_1)
         return VG_TRUE;
   }

   return VG_FALSE;
}

static void setup_shader_program(struct shader *shader)
{
   struct vg_context *ctx = shader->context;
   VGint shader_id = 0;
   VGBlendMode blend_mode = ctx->state.vg.blend_mode;
   VGboolean black_white = is_format_bw(shader);

   /* 1st stage: fill */
   if (!shader->drawing_image ||
       (shader->image_mode == VG_DRAW_IMAGE_MULTIPLY || shader->image_mode == VG_DRAW_IMAGE_STENCIL)) {
      switch(paint_type(shader->paint)) {
      case VG_PAINT_TYPE_COLOR:
         shader_id |= VEGA_SOLID_FILL_SHADER;
         break;
      case VG_PAINT_TYPE_LINEAR_GRADIENT:
         shader_id |= VEGA_LINEAR_GRADIENT_SHADER;
         break;
      case VG_PAINT_TYPE_RADIAL_GRADIENT:
         shader_id |= VEGA_RADIAL_GRADIENT_SHADER;
         break;
      case VG_PAINT_TYPE_PATTERN:
         shader_id |= VEGA_PATTERN_SHADER;
         break;

      default:
         abort();
      }
   }

   /* second stage image */
   if (shader->drawing_image) {
      switch(shader->image_mode) {
      case VG_DRAW_IMAGE_NORMAL:
         shader_id |= VEGA_IMAGE_NORMAL_SHADER;
         break;
      case VG_DRAW_IMAGE_MULTIPLY:
         shader_id |= VEGA_IMAGE_MULTIPLY_SHADER;
         break;
      case VG_DRAW_IMAGE_STENCIL:
         shader_id |= VEGA_IMAGE_STENCIL_SHADER;
         break;
      default:
         debug_printf("Unknown image mode!");
      }
   }

   if (shader->masking)
      shader_id |= VEGA_MASK_SHADER;

   switch(blend_mode) {
   case VG_BLEND_MULTIPLY:
      shader_id |= VEGA_BLEND_MULTIPLY_SHADER;
      break;
   case VG_BLEND_SCREEN:
      shader_id |= VEGA_BLEND_SCREEN_SHADER;
      break;
   case VG_BLEND_DARKEN:
      shader_id |= VEGA_BLEND_DARKEN_SHADER;
      break;
   case VG_BLEND_LIGHTEN:
      shader_id |= VEGA_BLEND_LIGHTEN_SHADER;
      break;
   default:
      /* handled by pipe_blend_state */
      break;
   }

   if (black_white)
      shader_id |= VEGA_BW_SHADER;

   shader->fs = shaders_cache_fill(ctx->sc, shader_id);
   cso_set_fragment_shader_handle(ctx->cso_context, shader->fs);
}


void shader_bind(struct shader *shader)
{
   /* first resolve the real paint type */
   paint_resolve_type(shader->paint);

   setup_constant_buffer(shader);
   setup_samplers(shader);
   setup_shader_program(shader);
}

void shader_set_image_mode(struct shader *shader, VGImageMode image_mode)
{
   shader->image_mode = image_mode;
}

VGImageMode shader_image_mode(struct shader *shader)
{
   return shader->image_mode;
}

void shader_set_drawing_image(struct shader *shader, VGboolean drawing_image)
{
   shader->drawing_image = drawing_image;
}

VGboolean shader_drawing_image(struct shader *shader)
{
   return shader->drawing_image;
}

void shader_set_image(struct shader *shader, struct vg_image *img)
{
   shader->image = img;
}
