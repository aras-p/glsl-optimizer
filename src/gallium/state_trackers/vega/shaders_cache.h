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

#ifndef SHADERS_CACHE_H
#define SHADERS_CACHE_H


struct vg_context;
struct pipe_context;
struct tgsi_token;
struct shaders_cache;

enum VegaShaderType {
   VEGA_SOLID_FILL_SHADER         = 1 <<  0,
   VEGA_LINEAR_GRADIENT_SHADER    = 1 <<  1,
   VEGA_RADIAL_GRADIENT_SHADER    = 1 <<  2,
   VEGA_PATTERN_SHADER            = 1 <<  3,
   VEGA_IMAGE_NORMAL_SHADER       = 1 <<  4,
   VEGA_IMAGE_MULTIPLY_SHADER     = 1 <<  5,
   VEGA_IMAGE_STENCIL_SHADER      = 1 <<  6,

   VEGA_MASK_SHADER               = 1 <<  7,

   VEGA_BLEND_MULTIPLY_SHADER     = 1 <<  8,
   VEGA_BLEND_SCREEN_SHADER       = 1 <<  9,
   VEGA_BLEND_DARKEN_SHADER       = 1 << 10,
   VEGA_BLEND_LIGHTEN_SHADER      = 1 << 11,

   VEGA_PREMULTIPLY_SHADER        = 1 << 12,
   VEGA_UNPREMULTIPLY_SHADER      = 1 << 13,

   VEGA_BW_SHADER                 = 1 << 14
};

struct vg_shader {
   void *driver;
   struct tgsi_token *tokens;
   int type;/* PIPE_SHADER_VERTEX, PIPE_SHADER_FRAGMENT */
};

struct shaders_cache *shaders_cache_create(struct vg_context *pipe);
void shaders_cache_destroy(struct shaders_cache *sc);
void *shaders_cache_fill(struct shaders_cache *sc,
                         int shader_key);

struct vg_shader *shader_create_from_text(struct pipe_context *pipe,
                                          const char *txt, int num_tokens,
                                          int type);

void vg_shader_destroy(struct vg_context *ctx, struct vg_shader *shader);



#endif
