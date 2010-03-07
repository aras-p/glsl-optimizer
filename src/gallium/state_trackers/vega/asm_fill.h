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

#ifndef ASM_FILL_H
#define ASM_FILL_H

#include "tgsi/tgsi_ureg.h"

typedef void (* ureg_func)( struct ureg_program *ureg,
                            struct ureg_dst *out,
                            struct ureg_src *in,
                            struct ureg_src *sampler,
                            struct ureg_dst *temp,
                            struct ureg_src *constant);

static INLINE void
solid_fill( struct ureg_program *ureg,
            struct ureg_dst *out,
            struct ureg_src *in,
            struct ureg_src *sampler,
            struct ureg_dst *temp,
            struct ureg_src *constant)
{
   ureg_MOV(ureg, *out, constant[0]);
}

static INLINE void
linear_grad( struct ureg_program *ureg,
             struct ureg_dst *out,
             struct ureg_src *in,
             struct ureg_src *sampler,
             struct ureg_dst *temp,
             struct ureg_src *constant)
{

   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XY),
            in[0]);
   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_Z),
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y));
   ureg_DP3(ureg, temp[1], constant[2], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[2], constant[3], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[3], constant[4], ureg_src(temp[0]));
   ureg_RCP(ureg, temp[3], ureg_src(temp[3]));
   ureg_MUL(ureg, temp[1], ureg_src(temp[1]), ureg_src(temp[3]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[2]), ureg_src(temp[3]));
   ureg_MOV(ureg, ureg_writemask(temp[4], TGSI_WRITEMASK_X), ureg_src(temp[1]));
   ureg_MOV(ureg, ureg_writemask(temp[4], TGSI_WRITEMASK_Y), ureg_src(temp[2]));
   ureg_MUL(ureg, temp[0],
            ureg_scalar(constant[0], TGSI_SWIZZLE_Y),
            ureg_scalar(ureg_src(temp[4]), TGSI_SWIZZLE_Y));
   ureg_MAD(ureg, temp[1],
            ureg_scalar(constant[0], TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[4]), TGSI_SWIZZLE_X),
            ureg_src(temp[0]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[1]),
            ureg_scalar(constant[0], TGSI_SWIZZLE_Z));
   ureg_TEX(ureg, *out, TGSI_TEXTURE_1D, ureg_src(temp[2]), sampler[0]);
}

static INLINE void
radial_grad( struct ureg_program *ureg,
             struct ureg_dst *out,
             struct ureg_src *in,
             struct ureg_src *sampler,
             struct ureg_dst *temp,
             struct ureg_src *constant)
{

   ureg_MOV(ureg, ureg_writemask(temp[0], TGSI_WRITEMASK_XY), in[0]);
   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_Z),
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y));
   ureg_DP3(ureg, temp[1], constant[2], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[2], constant[3], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[3], constant[4], ureg_src(temp[0]));
   ureg_RCP(ureg, temp[3], ureg_src(temp[3]));
   ureg_MUL(ureg, temp[1], ureg_src(temp[1]), ureg_src(temp[3]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[2]), ureg_src(temp[3]));
   ureg_MOV(ureg, ureg_writemask(temp[5], TGSI_WRITEMASK_X), ureg_src(temp[1]));
   ureg_MOV(ureg, ureg_writemask(temp[5], TGSI_WRITEMASK_Y), ureg_src(temp[2]));
   ureg_MUL(ureg, temp[0], ureg_scalar(constant[0], TGSI_SWIZZLE_Y),
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_Y));
   ureg_MAD(ureg, temp[1],
            ureg_scalar(constant[0], TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_X), ureg_src(temp[0]));
   ureg_ADD(ureg, temp[1], ureg_src(temp[1]), ureg_src(temp[1]));
   ureg_MUL(ureg, temp[3],
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_Y),
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_Y));
   ureg_MAD(ureg, temp[4],
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[5]), TGSI_SWIZZLE_X),
            ureg_src(temp[3]));
   ureg_MOV(ureg, temp[4], ureg_negate(ureg_src(temp[4])));
   ureg_MUL(ureg, temp[2],
            ureg_scalar(constant[0], TGSI_SWIZZLE_Z),
            ureg_src(temp[4]));
   ureg_MUL(ureg, temp[0],
            ureg_scalar(constant[1], TGSI_SWIZZLE_W),
            ureg_src(temp[2]));
   ureg_MUL(ureg, temp[3], ureg_src(temp[1]), ureg_src(temp[1]));

   ureg_SUB(ureg, temp[2], ureg_src(temp[3]), ureg_src(temp[0]));
   ureg_RSQ(ureg, temp[2], ureg_abs(ureg_src(temp[2])));
   ureg_RCP(ureg, temp[2], ureg_src(temp[2]));
   ureg_SUB(ureg, temp[1], ureg_src(temp[2]), ureg_src(temp[1]));
   ureg_ADD(ureg, temp[0],
            ureg_scalar(constant[0], TGSI_SWIZZLE_Z),
            ureg_scalar(constant[0], TGSI_SWIZZLE_Z));
   ureg_RCP(ureg, temp[0], ureg_src(temp[0]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[1]), ureg_src(temp[0]));
   ureg_TEX(ureg, *out, TGSI_TEXTURE_1D, ureg_src(temp[2]), sampler[0]);

}


static INLINE void
pattern( struct ureg_program *ureg,
         struct ureg_dst     *out,
         struct ureg_src     *in,
         struct ureg_src     *sampler,
         struct ureg_dst     *temp,
         struct ureg_src     *constant)
{
   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XY),
            in[0]);
   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_Z),
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y));
   ureg_DP3(ureg, temp[1], constant[2], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[2], constant[3], ureg_src(temp[0]));
   ureg_DP3(ureg, temp[3], constant[4], ureg_src(temp[0]));
   ureg_RCP(ureg, temp[3], ureg_src(temp[3]));
   ureg_MUL(ureg, temp[1], ureg_src(temp[1]), ureg_src(temp[3]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[2]), ureg_src(temp[3]));
   ureg_MOV(ureg, ureg_writemask(temp[4], TGSI_WRITEMASK_X), ureg_src(temp[1]));
   ureg_MOV(ureg, ureg_writemask(temp[4], TGSI_WRITEMASK_Y), ureg_src(temp[2]));
   ureg_RCP(ureg, temp[0],
            ureg_swizzle(constant[1],
                         TGSI_SWIZZLE_Z,
                         TGSI_SWIZZLE_W,
                         TGSI_SWIZZLE_Z,
                         TGSI_SWIZZLE_W));
   ureg_MOV(ureg, temp[1], ureg_src(temp[4]));
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_X),
            ureg_src(temp[1]),
            ureg_src(temp[0]));
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_Y),
            ureg_src(temp[1]),
            ureg_src(temp[0]));
   ureg_TEX(ureg, *out, TGSI_TEXTURE_2D, ureg_src(temp[1]), sampler[0]);
}

static INLINE void
mask( struct ureg_program *ureg,
      struct ureg_dst *out,
      struct ureg_src *in,
      struct ureg_src *sampler,
      struct ureg_dst *temp,
      struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[1]);
   ureg_MUL(ureg, ureg_writemask(temp[0], TGSI_WRITEMASK_W),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_MOV(ureg, *out, ureg_src(temp[0]));
}

static INLINE void
image_normal( struct ureg_program *ureg,
              struct ureg_dst *out,
              struct ureg_src *in,
              struct ureg_src *sampler,
              struct ureg_dst *temp,
              struct ureg_src *constant)
{
   ureg_TEX(ureg, *out, TGSI_TEXTURE_2D, in[1], sampler[3]);
}


static INLINE void
image_multiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[1], sampler[3]);
   ureg_MUL(ureg, *out, ureg_src(temp[0]), ureg_src(temp[1]));
}


static INLINE void
image_stencil( struct ureg_program *ureg,
               struct ureg_dst *out,
               struct ureg_src *in,
               struct ureg_src *sampler,
               struct ureg_dst *temp,
               struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[1], sampler[3]);
   ureg_MUL(ureg, *out, ureg_src(temp[0]), ureg_src(temp[1]));
}

#define EXTENDED_BLENDER_OVER_FUNC                                      \
   ureg_SUB(ureg, temp[3],                                              \
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y),                   \
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));            \
   ureg_SUB(ureg, temp[3],                                              \
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y),                   \
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));            \
   ureg_MUL(ureg, temp[3], ureg_src(temp[0]), ureg_src(temp[3]));       \
   ureg_MUL(ureg, temp[4], ureg_src(temp[1]), ureg_src(temp[4]));       \
   ureg_ADD(ureg, temp[3], ureg_src(temp[3]), ureg_src(temp[4]));


static INLINE void
blend_multiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[2]);
   EXTENDED_BLENDER_OVER_FUNC
   ureg_MUL(ureg, temp[4], ureg_src(temp[0]), ureg_src(temp[1]));
   ureg_ADD(ureg, temp[1], ureg_src(temp[4]), ureg_src(temp[3]));

   ureg_MUL(ureg, temp[2], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_ADD(ureg, temp[3], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_SUB(ureg, ureg_writemask(temp[1], TGSI_WRITEMASK_W),
            ureg_src(temp[3]), ureg_src(temp[2]));

   ureg_MOV(ureg, *out, ureg_src(temp[1]));
}

static INLINE void
blend_screen( struct ureg_program *ureg,
              struct ureg_dst     *out,
              struct ureg_src     *in,
              struct ureg_src     *sampler,
              struct ureg_dst     *temp,
              struct ureg_src     *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[2]);
   ureg_ADD(ureg, temp[3], ureg_src(temp[0]), ureg_src(temp[1]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[0]), ureg_src(temp[1]));
   ureg_SUB(ureg, *out, ureg_src(temp[3]), ureg_src(temp[2]));
}

static INLINE void
blend_darken( struct ureg_program *ureg,
              struct ureg_dst     *out,
              struct ureg_src     *in,
              struct ureg_src     *sampler,
              struct ureg_dst     *temp,
              struct ureg_src     *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[2]);
   EXTENDED_BLENDER_OVER_FUNC
   ureg_MUL(ureg, temp[4], ureg_src(temp[0]),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_MUL(ureg, temp[5], ureg_src(temp[1]),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));
   ureg_MIN(ureg, temp[4], ureg_src(temp[4]), ureg_src(temp[5]));
   ureg_ADD(ureg, temp[1], ureg_src(temp[3]), ureg_src(temp[4]));

   ureg_MUL(ureg, temp[2], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_ADD(ureg, temp[3], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_SUB(ureg, ureg_writemask(temp[1], TGSI_WRITEMASK_W),
            ureg_src(temp[3]), ureg_src(temp[2]));

   ureg_MOV(ureg, *out, ureg_src(temp[1]));
}

static INLINE void
blend_lighten( struct ureg_program *ureg,
               struct ureg_dst     *out,
               struct ureg_src     *in,
               struct ureg_src     *sampler,
               struct ureg_dst *temp,
               struct ureg_src     *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[2]);
   EXTENDED_BLENDER_OVER_FUNC
   ureg_MUL(ureg, temp[4], ureg_src(temp[0]),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_MUL(ureg, temp[5], ureg_src(temp[1]),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));
   ureg_MAX(ureg, temp[4], ureg_src(temp[4]), ureg_src(temp[5]));
   ureg_ADD(ureg, temp[1], ureg_src(temp[3]), ureg_src(temp[4]));

   ureg_MUL(ureg, temp[2], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_ADD(ureg, temp[3], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_SUB(ureg, ureg_writemask(temp[1], TGSI_WRITEMASK_W),
            ureg_src(temp[3]), ureg_src(temp[2]));

   ureg_MOV(ureg, *out, ureg_src(temp[1]));
}

static INLINE void
premultiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_MUL(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XYZ),
            ureg_src(temp[0]),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));
}

static INLINE void
unpremultiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[0], TGSI_TEXTURE_2D, in[0], sampler[1]);
}


static INLINE void
color_bw( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_ADD(ureg, temp[1],
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y),
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y));
   ureg_RCP(ureg, temp[2], ureg_src(temp[1]));
   ureg_ADD(ureg, temp[1],
            ureg_scalar(constant[1], TGSI_SWIZZLE_Y),
            ureg_src(temp[2]));
   ureg_ADD(ureg, ureg_writemask(temp[2], TGSI_WRITEMASK_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_Y));
   ureg_ADD(ureg, ureg_writemask(temp[2], TGSI_WRITEMASK_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_Z),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_X));
   ureg_SGE(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XYZ),
            ureg_scalar(ureg_src(temp[2]), TGSI_SWIZZLE_X),
            ureg_src(temp[1]));
  ureg_SGE(ureg,
           ureg_writemask(temp[0], TGSI_WRITEMASK_W),
           ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
           ureg_scalar(ureg_src(temp[2]), TGSI_SWIZZLE_Y));
  ureg_MOV(ureg, *out, ureg_src(temp[0]));
}


struct shader_asm_info {
   VGint id;
   ureg_func func;

   VGboolean needs_position;

   VGint start_const;
   VGint num_consts;

   VGint start_sampler;
   VGint num_samplers;

   VGint start_temp;
   VGint num_temps;
};


static const struct shader_asm_info shaders_asm[] = {
   /* fills */
   {VEGA_SOLID_FILL_SHADER, solid_fill,
    VG_FALSE, 0, 1, 0, 0, 0, 0},
   {VEGA_LINEAR_GRADIENT_SHADER, linear_grad,
    VG_TRUE,  0, 5, 0, 1, 0, 5},
   {VEGA_RADIAL_GRADIENT_SHADER, radial_grad,
    VG_TRUE,  0, 5, 0, 1, 0, 6},
   {VEGA_PATTERN_SHADER, pattern,
    VG_TRUE,  1, 4, 0, 1, 0, 5},

   /* image draw modes */
   {VEGA_IMAGE_NORMAL_SHADER, image_normal,
    VG_TRUE,  0, 0, 3, 1, 0, 0},
   {VEGA_IMAGE_MULTIPLY_SHADER, image_multiply,
    VG_TRUE,  0, 0, 3, 1, 0, 2},
   {VEGA_IMAGE_STENCIL_SHADER, image_stencil,
    VG_TRUE,  0, 0, 3, 1, 0, 2},

   {VEGA_MASK_SHADER, mask,
    VG_TRUE,  0, 0, 1, 1, 0, 2},

   /* extra blend modes */
   {VEGA_BLEND_MULTIPLY_SHADER, blend_multiply,
    VG_TRUE,  1, 1, 2, 1, 0, 5},
   {VEGA_BLEND_SCREEN_SHADER, blend_screen,
    VG_TRUE,  0, 0, 2, 1, 0, 4},
   {VEGA_BLEND_DARKEN_SHADER, blend_darken,
    VG_TRUE,  1, 1, 2, 1, 0, 6},
   {VEGA_BLEND_LIGHTEN_SHADER, blend_lighten,
    VG_TRUE,  1, 1, 2, 1, 0, 6},

   /* premultiply */
   {VEGA_PREMULTIPLY_SHADER, premultiply,
    VG_FALSE,  0, 0, 0, 0, 0, 1},
   {VEGA_UNPREMULTIPLY_SHADER, unpremultiply,
    VG_FALSE,  0, 0, 0, 0, 0, 1},

   /* color transform to black and white */
   {VEGA_BW_SHADER, color_bw,
    VG_FALSE,  1, 1, 0, 0, 0, 3},
};
#endif
