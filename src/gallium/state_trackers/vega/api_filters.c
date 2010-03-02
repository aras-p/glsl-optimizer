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

#include "vg_context.h"
#include "image.h"
#include "renderer.h"
#include "shaders_cache.h"
#include "st_inlines.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_format.h"
#include "util/u_memory.h"


#include "asm_filters.h"


struct filter_info {
   struct vg_image *dst;
   struct vg_image *src;
   struct vg_shader * (*setup_shader)(struct vg_context *, void *);
   void *user_data;
   const void *const_buffer;
   VGint const_buffer_len;
   VGTilingMode tiling_mode;
   struct pipe_texture *extra_texture;
};

static INLINE struct pipe_texture *create_texture_1d(struct vg_context *ctx,
                                                     const VGuint *color_data,
                                                     const VGint color_data_len)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *tex = 0;
   struct pipe_texture templ;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_1D;
   templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   templ.last_level = 0;
   templ.width0 = color_data_len;
   templ.height0 = 1;
   templ.depth0 = 1;
   templ.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;

   tex = screen->texture_create(screen, &templ);

   { /* upload color_data */
      struct pipe_transfer *transfer =
         screen->get_tex_transfer(screen, tex,
                                  0, 0, 0,
                                  PIPE_TRANSFER_READ_WRITE ,
                                  0, 0, tex->width0, tex->height0);
      void *map = screen->transfer_map(screen, transfer);
      memcpy(map, color_data, sizeof(VGint)*color_data_len);
      screen->transfer_unmap(screen, transfer);
      screen->tex_transfer_destroy(transfer);
   }

   return tex;
}

static INLINE struct pipe_surface * setup_framebuffer(struct vg_image *dst)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_framebuffer_state fb;
   struct pipe_surface *dst_surf = pipe->screen->get_tex_surface(
      pipe->screen, dst->texture, 0, 0, 0,
      PIPE_BUFFER_USAGE_GPU_WRITE);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width  = dst->x + dst_surf->width;
   fb.height = dst->y + dst_surf->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst_surf;
   {
      VGint i;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         fb.cbufs[i] = 0;
   }
   cso_set_framebuffer(ctx->cso_context, &fb);

   return dst_surf;
}

static void setup_viewport(struct vg_image *dst)
{
   struct vg_context *ctx = vg_current_context();
   vg_set_viewport(ctx, VEGA_Y0_TOP);
}

static void setup_blend()
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_blend_state blend;
   memset(&blend, 0, sizeof(blend));
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   if (ctx->state.vg.filter_channel_mask & VG_RED)
      blend.rt[0].colormask |= PIPE_MASK_R;
   if (ctx->state.vg.filter_channel_mask & VG_GREEN)
      blend.rt[0].colormask |= PIPE_MASK_G;
   if (ctx->state.vg.filter_channel_mask & VG_BLUE)
      blend.rt[0].colormask |= PIPE_MASK_B;
   if (ctx->state.vg.filter_channel_mask & VG_ALPHA)
      blend.rt[0].colormask |= PIPE_MASK_A;
   blend.rt[0].blend_enable = 0;
   cso_set_blend(ctx->cso_context, &blend);
}

static void setup_constant_buffer(struct vg_context *ctx, const void *buffer,
                                  VGint param_bytes)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_buffer **cbuf = &ctx->filter.buffer;

   /* We always need to get a new buffer, to keep the drivers simple and
    * avoid gratuitous rendering synchronization. */
   pipe_buffer_reference(cbuf, NULL);

   *cbuf = pipe_buffer_create(pipe->screen, 16,
                              PIPE_BUFFER_USAGE_CONSTANT,
                              param_bytes);

   if (*cbuf) {
      st_no_flush_pipe_buffer_write(ctx, *cbuf,
                                    0, param_bytes, buffer);
   }

   ctx->pipe->set_constant_buffer(ctx->pipe, PIPE_SHADER_FRAGMENT, 0, *cbuf);
}

static void setup_samplers(struct vg_context *ctx, struct filter_info *info)
{
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state sampler[3];
   int num_samplers = 0;
   int num_textures = 0;

   samplers[0] = NULL;
   samplers[1] = NULL;
   samplers[2] = NULL;
   samplers[3] = NULL;
   textures[0] = NULL;
   textures[1] = NULL;
   textures[2] = NULL;
   textures[3] = NULL;

   memset(&sampler[0], 0, sizeof(struct pipe_sampler_state));
   sampler[0].wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler[0].wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler[0].wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler[0].min_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampler[0].mag_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampler[0].normalized_coords = 1;

   switch(info->tiling_mode) {
   case VG_TILE_FILL:
      sampler[0].wrap_s = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      sampler[0].wrap_t = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      memcpy(sampler[0].border_color,
             ctx->state.vg.tile_fill_color,
             sizeof(VGfloat) * 4);
      break;
   case VG_TILE_PAD:
      sampler[0].wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler[0].wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      break;
   case VG_TILE_REPEAT:
      sampler[0].wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler[0].wrap_t = PIPE_TEX_WRAP_REPEAT;
      break;
   case VG_TILE_REFLECT:
      sampler[0].wrap_s = PIPE_TEX_WRAP_MIRROR_REPEAT;
      sampler[0].wrap_t = PIPE_TEX_WRAP_MIRROR_REPEAT;
      break;
   default:
      debug_assert(!"Unknown tiling mode");
   }

   samplers[0] = &sampler[0];
   textures[0] = info->src->texture;
   ++num_samplers;
   ++num_textures;

   if (info->extra_texture) {
      memcpy(&sampler[1], &sampler[0], sizeof(struct pipe_sampler_state));
      samplers[1] = &sampler[1];
      textures[1] = info->extra_texture;
      ++num_samplers;
      ++num_textures;
   }


   cso_set_samplers(ctx->cso_context, num_samplers, (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_textures(ctx->cso_context, num_textures, textures);
}

static struct vg_shader * setup_color_matrix(struct vg_context *ctx, void *user_data)
{
   struct vg_shader *shader =
      shader_create_from_text(ctx->pipe, color_matrix_asm, 200,
         PIPE_SHADER_FRAGMENT);
   cso_set_fragment_shader_handle(ctx->cso_context, shader->driver);
   return shader;
}

static struct vg_shader * setup_convolution(struct vg_context *ctx, void *user_data)
{
   char buffer[1024];
   VGint num_consts = (VGint)(long)(user_data);
   struct vg_shader *shader;

   snprintf(buffer, 1023, convolution_asm, num_consts, num_consts / 2 + 1);

   shader = shader_create_from_text(ctx->pipe, buffer, 200,
                                    PIPE_SHADER_FRAGMENT);

   cso_set_fragment_shader_handle(ctx->cso_context, shader->driver);
   return shader;
}

static struct vg_shader * setup_lookup(struct vg_context *ctx, void *user_data)
{
   struct vg_shader *shader =
      shader_create_from_text(ctx->pipe, lookup_asm,
                              200, PIPE_SHADER_FRAGMENT);

   cso_set_fragment_shader_handle(ctx->cso_context, shader->driver);
   return shader;
}


static struct vg_shader * setup_lookup_single(struct vg_context *ctx, void *user_data)
{
   char buffer[1024];
   VGImageChannel channel = (VGImageChannel)(user_data);
   struct vg_shader *shader;

   switch(channel) {
   case VG_RED:
      snprintf(buffer, 1023, lookup_single_asm, "xxxx");
      break;
   case VG_GREEN:
      snprintf(buffer, 1023, lookup_single_asm, "yyyy");
      break;
   case VG_BLUE:
      snprintf(buffer, 1023, lookup_single_asm, "zzzz");
      break;
   case VG_ALPHA:
      snprintf(buffer, 1023, lookup_single_asm, "wwww");
      break;
   default:
      debug_assert(!"Unknown color channel");
   }

   shader = shader_create_from_text(ctx->pipe, buffer, 200,
                                    PIPE_SHADER_FRAGMENT);

   cso_set_fragment_shader_handle(ctx->cso_context, shader->driver);
   return shader;
}

static void execute_filter(struct vg_context *ctx,
                           struct filter_info *info)
{
   struct pipe_surface *dst_surf;
   struct vg_shader *shader;

   cso_save_framebuffer(ctx->cso_context);
   cso_save_fragment_shader(ctx->cso_context);
   cso_save_viewport(ctx->cso_context);
   cso_save_blend(ctx->cso_context);
   cso_save_samplers(ctx->cso_context);
   cso_save_sampler_textures(ctx->cso_context);

   dst_surf = setup_framebuffer(info->dst);
   setup_viewport(info->dst);
   setup_blend();
   setup_constant_buffer(ctx, info->const_buffer, info->const_buffer_len);
   shader = info->setup_shader(ctx, info->user_data);
   setup_samplers(ctx, info);

   renderer_draw_texture(ctx->renderer,
                         info->src->texture,
                         info->dst->x, info->dst->y,
                         info->dst->x + info->dst->width,
                         info->dst->y + info->dst->height,
                         info->dst->x, info->dst->y,
                         info->dst->x + info->dst->width,
                         info->dst->y + info->dst->height);

   cso_restore_framebuffer(ctx->cso_context);
   cso_restore_fragment_shader(ctx->cso_context);
   cso_restore_viewport(ctx->cso_context);
   cso_restore_blend(ctx->cso_context);
   cso_restore_samplers(ctx->cso_context);
   cso_restore_sampler_textures(ctx->cso_context);

   vg_shader_destroy(ctx, shader);

   pipe_surface_reference(&dst_surf, NULL);
}

void vgColorMatrix(VGImage dst, VGImage src,
                   const VGfloat * matrix)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!matrix || !is_aligned(matrix)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = (struct vg_image*)dst;
   s = (struct vg_image*)src;

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_color_matrix;
   info.user_data = NULL;
   info.const_buffer = matrix;
   info.const_buffer_len = 20 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture = 0;
   execute_filter(ctx, &info);
}

static VGfloat texture_offset(VGfloat width, VGint kernelSize, VGint current, VGint shift)
{
   VGfloat diff = current - shift;

   return diff / width;
}

void vgConvolve(VGImage dst, VGImage src,
                VGint kernelWidth, VGint kernelHeight,
                VGint shiftX, VGint shiftY,
                const VGshort * kernel,
                VGfloat scale,
                VGfloat bias,
                VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   VGfloat *buffer;
   VGint buffer_len;
   VGint i, j;
   VGint idx = 0;
   struct vg_image *d, *s;
   VGint kernel_size = kernelWidth * kernelHeight;
   struct filter_info info;
   const VGint max_kernel_size = vgGeti(VG_MAX_KERNEL_SIZE);

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (kernelWidth <= 0 || kernelHeight <= 0 ||
      kernelWidth > max_kernel_size || kernelHeight > max_kernel_size) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!kernel || !is_aligned_to(kernel, 2)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = (struct vg_image*)dst;
   s = (struct vg_image*)src;

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);

   buffer_len = 8 + 2 * 4 * kernel_size;
   buffer = (VGfloat*)malloc(buffer_len * sizeof(VGfloat));

   buffer[0] = 0.f;
   buffer[1] = 1.f;
   buffer[2] = 2.f; /*unused*/
   buffer[3] = 4.f; /*unused*/

   buffer[4] = kernelWidth * kernelHeight;
   buffer[5] = scale;
   buffer[6] = bias;
   buffer[7] = 0.f;

   idx = 8;
   for (j = 0; j < kernelHeight; ++j) {
      for (i = 0; i < kernelWidth; ++i) {
         VGint index = j * kernelWidth + i;
         VGfloat x, y;

         x = texture_offset(s->width, kernelWidth, i, shiftX);
         y = texture_offset(s->height, kernelHeight, j, shiftY);

         buffer[idx + index*4 + 0] = x;
         buffer[idx + index*4 + 1] = y;
         buffer[idx + index*4 + 2] = 0.f;
         buffer[idx + index*4 + 3] = 0.f;
      }
   }
   idx += kernel_size * 4;

   for (j = 0; j < kernelHeight; ++j) {
      for (i = 0; i < kernelWidth; ++i) {
         /* transpose the kernel */
         VGint index = j * kernelWidth + i;
         VGint kindex = (kernelWidth - i - 1) * kernelHeight + (kernelHeight - j - 1);
         buffer[idx + index*4 + 0] = kernel[kindex];
         buffer[idx + index*4 + 1] = kernel[kindex];
         buffer[idx + index*4 + 2] = kernel[kindex];
         buffer[idx + index*4 + 3] = kernel[kindex];
      }
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_convolution;
   info.user_data = (void*)(long)(buffer_len/4);
   info.const_buffer = buffer;
   info.const_buffer_len = buffer_len * sizeof(VGfloat);
   info.tiling_mode = tilingMode;
   info.extra_texture = 0;
   execute_filter(ctx, &info);

   free(buffer);
}

void vgSeparableConvolve(VGImage dst, VGImage src,
                         VGint kernelWidth,
                         VGint kernelHeight,
                         VGint shiftX, VGint shiftY,
                         const VGshort * kernelX,
                         const VGshort * kernelY,
                         VGfloat scale,
                         VGfloat bias,
                         VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   VGshort *kernel;
   VGint i, j, idx = 0;
   const VGint max_kernel_size = vgGeti(VG_MAX_SEPARABLE_KERNEL_SIZE);

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (kernelWidth <= 0 || kernelHeight <= 0 ||
       kernelWidth > max_kernel_size || kernelHeight > max_kernel_size) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!kernelX || !kernelY ||
       !is_aligned_to(kernelX, 2) || !is_aligned_to(kernelY, 2)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   kernel = malloc(sizeof(VGshort)*kernelWidth*kernelHeight);
   for (i = 0; i < kernelWidth; ++i) {
      for (j = 0; j < kernelHeight; ++j) {
         kernel[idx] = kernelX[i] * kernelY[j];
         ++idx;
      }
   }
   vgConvolve(dst, src, kernelWidth, kernelHeight, shiftX, shiftY,
              kernel, scale, bias, tilingMode);
   free(kernel);
}

static INLINE VGfloat compute_gaussian_componenet(VGfloat x, VGfloat y,
                                                  VGfloat stdDeviationX,
                                                  VGfloat stdDeviationY)
{
   VGfloat mult = 1 / ( 2 * M_PI * stdDeviationX * stdDeviationY);
   VGfloat e = exp( - ( pow(x, 2)/(2*pow(stdDeviationX, 2)) +
                        pow(y, 2)/(2*pow(stdDeviationY, 2)) ) );
   return mult * e;
}

static INLINE VGint compute_kernel_size(VGfloat deviation)
{
   VGint size = ceil(2.146 * deviation);
   if (size > 11)
      return 11;
   return size;
}

static void compute_gaussian_kernel(VGfloat *kernel,
                                    VGint width, VGint height,
                                    VGfloat stdDeviationX,
                                    VGfloat stdDeviationY)
{
   VGint i, j;
   VGfloat scale = 0.0f;

   for (j = 0; j < height; ++j) {
      for (i = 0; i < width; ++i) {
         VGint idx =  (height - j -1) * width + (width - i -1);
         kernel[idx] = compute_gaussian_componenet(i-(ceil(width/2))-1,
                                                   j-ceil(height/2)-1,
                                                   stdDeviationX, stdDeviationY);
         scale += kernel[idx];
      }
   }

   for (j = 0; j < height; ++j) {
      for (i = 0; i < width; ++i) {
         VGint idx = j * width + i;
         kernel[idx] /= scale;
      }
   }
}

void vgGaussianBlur(VGImage dst, VGImage src,
                    VGfloat stdDeviationX,
                    VGfloat stdDeviationY,
                    VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   VGfloat *buffer, *kernel;
   VGint kernel_width, kernel_height, kernel_size;
   VGint buffer_len;
   VGint idx, i, j;
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (stdDeviationX <= 0 || stdDeviationY <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = (struct vg_image*)dst;
   s = (struct vg_image*)src;

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   kernel_width = compute_kernel_size(stdDeviationX);
   kernel_height = compute_kernel_size(stdDeviationY);
   kernel_size = kernel_width * kernel_height;
   kernel = malloc(sizeof(VGfloat)*kernel_size);
   compute_gaussian_kernel(kernel, kernel_width, kernel_height,
                           stdDeviationX, stdDeviationY);

   buffer_len = 8 + 2 * 4 * kernel_size;
   buffer = (VGfloat*)malloc(buffer_len * sizeof(VGfloat));

   buffer[0] = 0.f;
   buffer[1] = 1.f;
   buffer[2] = 2.f; /*unused*/
   buffer[3] = 4.f; /*unused*/

   buffer[4] = kernel_width * kernel_height;
   buffer[5] = 1.f;/*scale*/
   buffer[6] = 0.f;/*bias*/
   buffer[7] = 0.f;

   idx = 8;
   for (j = 0; j < kernel_height; ++j) {
      for (i = 0; i < kernel_width; ++i) {
         VGint index = j * kernel_width + i;
         VGfloat x, y;

         x = texture_offset(s->width, kernel_width, i, kernel_width/2);
         y = texture_offset(s->height, kernel_height, j, kernel_height/2);

         buffer[idx + index*4 + 0] = x;
         buffer[idx + index*4 + 1] = y;
         buffer[idx + index*4 + 2] = 0.f;
         buffer[idx + index*4 + 3] = 0.f;
      }
   }
   idx += kernel_size * 4;

   for (j = 0; j < kernel_height; ++j) {
      for (i = 0; i < kernel_width; ++i) {
         /* transpose the kernel */
         VGint index = j * kernel_width + i;
         VGint kindex = (kernel_width - i - 1) * kernel_height + (kernel_height - j - 1);
         buffer[idx + index*4 + 0] = kernel[kindex];
         buffer[idx + index*4 + 1] = kernel[kindex];
         buffer[idx + index*4 + 2] = kernel[kindex];
         buffer[idx + index*4 + 3] = kernel[kindex];
      }
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_convolution;
   info.user_data = (void*)(long)(buffer_len/4);
   info.const_buffer = buffer;
   info.const_buffer_len = buffer_len * sizeof(VGfloat);
   info.tiling_mode = tilingMode;
   info.extra_texture = 0;
   execute_filter(ctx, &info);

   free(buffer);
   free(kernel);
}

void vgLookup(VGImage dst, VGImage src,
              const VGubyte * redLUT,
              const VGubyte * greenLUT,
              const VGubyte * blueLUT,
              const VGubyte * alphaLUT,
              VGboolean outputLinear,
              VGboolean outputPremultiplied)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   VGuint color_data[256];
   VGint i;
   struct pipe_texture *lut_texture;
   VGfloat buffer[4];
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!redLUT || !greenLUT || !blueLUT || !alphaLUT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = (struct vg_image*)dst;
   s = (struct vg_image*)src;

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   for (i = 0; i < 256; ++i) {
      color_data[i] = blueLUT[i] << 24 | greenLUT[i] << 16 |
                      redLUT[i]  <<  8 | alphaLUT[i];
   }
   lut_texture = create_texture_1d(ctx, color_data, 255);

   buffer[0] = 0.f;
   buffer[1] = 0.f;
   buffer[2] = 1.f;
   buffer[3] = 1.f;

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_lookup;
   info.user_data = NULL;
   info.const_buffer = buffer;
   info.const_buffer_len = 4 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture = lut_texture;

   execute_filter(ctx, &info);

   pipe_texture_reference(&lut_texture, NULL);
}

void vgLookupSingle(VGImage dst, VGImage src,
                    const VGuint * lookupTable,
                    VGImageChannel sourceChannel,
                    VGboolean outputLinear,
                    VGboolean outputPremultiplied)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   struct pipe_texture *lut_texture;
   VGfloat buffer[4];
   struct filter_info info;
   VGuint color_data[256];
   VGint i;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!lookupTable || !is_aligned(lookupTable)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (sourceChannel != VG_RED && sourceChannel != VG_GREEN &&
       sourceChannel != VG_BLUE && sourceChannel != VG_ALPHA) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = (struct vg_image*)dst;
   s = (struct vg_image*)src;

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   for (i = 0; i < 256; ++i) {
      VGuint rgba = lookupTable[i];
      VGubyte blue, green, red, alpha;
      red   = (rgba & 0xff000000)>>24;
      green = (rgba & 0x00ff0000)>>16;
      blue  = (rgba & 0x0000ff00)>> 8;
      alpha = (rgba & 0x000000ff)>> 0;
      color_data[i] = blue << 24 | green << 16 |
                      red  <<  8 | alpha;
   }
   lut_texture = create_texture_1d(ctx, color_data, 256);

   buffer[0] = 0.f;
   buffer[1] = 0.f;
   buffer[2] = 1.f;
   buffer[3] = 1.f;

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_lookup_single;
   info.user_data = (void*)sourceChannel;
   info.const_buffer = buffer;
   info.const_buffer_len = 4 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture = lut_texture;

   execute_filter(ctx, &info);

   pipe_texture_reference(&lut_texture, NULL);
}
