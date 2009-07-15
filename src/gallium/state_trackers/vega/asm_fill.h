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

static const char solid_fill_asm[] =
   "MOV %s, CONST[0]\n";


static const char linear_grad_asm[] =
   "MOV TEMP[0].xy, IN[0]\n"
   "MOV TEMP[0].z, CONST[1].yyyy\n"
   "DP3 TEMP[1], CONST[2], TEMP[0]\n"
   "DP3 TEMP[2], CONST[3], TEMP[0]\n"
   "DP3 TEMP[3], CONST[4], TEMP[0]\n"
   "RCP TEMP[3], TEMP[3]\n"
   "MUL TEMP[1], TEMP[1], TEMP[3]\n"
   "MUL TEMP[2], TEMP[2], TEMP[3]\n"
   "MOV TEMP[4].x, TEMP[1]\n"
   "MOV TEMP[4].y, TEMP[2]\n"
   "MUL TEMP[0], CONST[0].yyyy, TEMP[4].yyyy\n"
   "MAD TEMP[1], CONST[0].xxxx, TEMP[4].xxxx, TEMP[0]\n"
   "MUL TEMP[2], TEMP[1], CONST[0].zzzz\n"
   "TEX %s, TEMP[2], SAMP[0], 1D\n";

static const char radial_grad_asm[] =
   "MOV TEMP[0].xy, IN[0]\n"
   "MOV TEMP[0].z, CONST[1].yyyy\n"
   "DP3 TEMP[1], CONST[2], TEMP[0]\n"
   "DP3 TEMP[2], CONST[3], TEMP[0]\n"
   "DP3 TEMP[3], CONST[4], TEMP[0]\n"
   "RCP TEMP[3], TEMP[3]\n"
   "MUL TEMP[1], TEMP[1], TEMP[3]\n"
   "MUL TEMP[2], TEMP[2], TEMP[3]\n"
   "MOV TEMP[5].x, TEMP[1]\n"
   "MOV TEMP[5].y, TEMP[2]\n"
   "MUL TEMP[0], CONST[0].yyyy, TEMP[5].yyyy\n"
   "MAD TEMP[1], CONST[0].xxxx, TEMP[5].xxxx, TEMP[0]\n"
   "ADD TEMP[1], TEMP[1], TEMP[1]\n"
   "MUL TEMP[3], TEMP[5].yyyy, TEMP[5].yyyy\n"
   "MAD TEMP[4], TEMP[5].xxxx, TEMP[5].xxxx, TEMP[3]\n"
   "MOV TEMP[4], -TEMP[4]\n"
   "MUL TEMP[2], CONST[0].zzzz, TEMP[4]\n"
   "MUL TEMP[0], CONST[1].wwww, TEMP[2]\n"
   "MUL TEMP[3], TEMP[1], TEMP[1]\n"
   "SUB TEMP[2], TEMP[3], TEMP[0]\n"
   "RSQ TEMP[2], |TEMP[2]|\n"
   "RCP TEMP[2], TEMP[2]\n"
   "SUB TEMP[1], TEMP[2], TEMP[1]\n"
   "ADD TEMP[0], CONST[0].zzzz, CONST[0].zzzz\n"
   "RCP TEMP[0], TEMP[0]\n"
   "MUL TEMP[2], TEMP[1], TEMP[0]\n"
   "TEX %s, TEMP[2], SAMP[0], 1D\n";

static const char pattern_asm[] =
   "MOV TEMP[0].xy, IN[0]\n"
   "MOV TEMP[0].z, CONST[1].yyyy\n"
   "DP3 TEMP[1], CONST[2], TEMP[0]\n"
   "DP3 TEMP[2], CONST[3], TEMP[0]\n"
   "DP3 TEMP[3], CONST[4], TEMP[0]\n"
   "RCP TEMP[3], TEMP[3]\n"
   "MUL TEMP[1], TEMP[1], TEMP[3]\n"
   "MUL TEMP[2], TEMP[2], TEMP[3]\n"
   "MOV TEMP[4].x, TEMP[1]\n"
   "MOV TEMP[4].y, TEMP[2]\n"
   "RCP TEMP[0], CONST[1].zwzw\n"
   "MOV TEMP[1], TEMP[4]\n"
   "MUL TEMP[1].x, TEMP[1], TEMP[0]\n"
   "MUL TEMP[1].y, TEMP[1], TEMP[0]\n"
   "TEX %s, TEMP[1], SAMP[0], 2D\n";


static const char mask_asm[] =
   "TEX TEMP[1], IN[0], SAMP[1], 2D\n"
   "MUL TEMP[0].w, TEMP[0].wwww, TEMP[1].wwww\n"
   "MOV %s, TEMP[0]\n";


static const char image_normal_asm[] =
   "TEX %s, IN[1], SAMP[3], 2D\n";

static const char image_multiply_asm[] =
   "TEX TEMP[1], IN[1], SAMP[3], 2D\n"
   "MUL %s, TEMP[0], TEMP[1]\n";

static const char image_stencil_asm[] =
   "TEX TEMP[1], IN[1], SAMP[3], 2D\n"
   "MUL %s, TEMP[0], TEMP[1]\n";


#define EXTENDED_BLEND_OVER                     \
   "SUB TEMP[3], CONST[1].yyyy, TEMP[1].wwww\n" \
   "SUB TEMP[4], CONST[1].yyyy, TEMP[0].wwww\n" \
   "MUL TEMP[3], TEMP[0], TEMP[3]\n"            \
   "MUL TEMP[4], TEMP[1], TEMP[4]\n"            \
   "ADD TEMP[3], TEMP[3], TEMP[4]\n"

static const char blend_multiply_asm[] =
   "TEX TEMP[1], IN[0], SAMP[2], 2D\n"
   EXTENDED_BLEND_OVER
   "MUL TEMP[4], TEMP[0], TEMP[1]\n"
   "ADD TEMP[1], TEMP[4], TEMP[3]\n"/*result.rgb*/
   "MUL TEMP[2], TEMP[0].wwww, TEMP[1].wwww\n"
   "ADD TEMP[3], TEMP[0].wwww, TEMP[1].wwww\n"
   "SUB TEMP[1].w, TEMP[3], TEMP[2]\n"
   "MOV %s, TEMP[1]\n";
#if 1
static const char blend_screen_asm[] =
   "TEX TEMP[1], IN[0], SAMP[2], 2D\n"
   "ADD TEMP[3], TEMP[0], TEMP[1]\n"
   "MUL TEMP[2], TEMP[0], TEMP[1]\n"
   "SUB %s, TEMP[3], TEMP[2]\n";
#else
static const char blend_screen_asm[] =
   "TEX TEMP[1], IN[0], SAMP[2], 2D\n"
   "MOV %s, TEMP[1]\n";
#endif

static const char blend_darken_asm[] =
   "TEX TEMP[1], IN[0], SAMP[2], 2D\n"
   EXTENDED_BLEND_OVER
   "MUL TEMP[4], TEMP[0], TEMP[1].wwww\n"
   "MUL TEMP[5], TEMP[1], TEMP[0].wwww\n"
   "MIN TEMP[4], TEMP[4], TEMP[5]\n"
   "ADD TEMP[1], TEMP[3], TEMP[4]\n"
   "MUL TEMP[2], TEMP[0].wwww, TEMP[1].wwww\n"
   "ADD TEMP[3], TEMP[0].wwww, TEMP[1].wwww\n"
   "SUB TEMP[1].w, TEMP[3], TEMP[2]\n"
   "MOV %s, TEMP[1]\n";

static const char blend_lighten_asm[] =
   "TEX TEMP[1], IN[0], SAMP[2], 2D\n"
   EXTENDED_BLEND_OVER
   "MUL TEMP[4], TEMP[0], TEMP[1].wwww\n"
   "MUL TEMP[5], TEMP[1], TEMP[0].wwww\n"
   "MAX TEMP[4], TEMP[4], TEMP[5]\n"
   "ADD TEMP[1], TEMP[3], TEMP[4]\n"
   "MUL TEMP[2], TEMP[0].wwww, TEMP[1].wwww\n"
   "ADD TEMP[3], TEMP[0].wwww, TEMP[1].wwww\n"
   "SUB TEMP[1].w, TEMP[3], TEMP[2]\n"
   "MOV %s, TEMP[1]\n";


static const char premultiply_asm[] =
   "MUL TEMP[0].xyz, TEMP[0], TEMP[0].wwww\n";

static const char unpremultiply_asm[] =
   "TEX TEMP[0], IN[0], SAMP[1], 2D\n";


static const char color_bw_asm[] =
   "ADD TEMP[1], CONST[1].yyyy, CONST[1].yyyy\n"
   "RCP TEMP[2], TEMP[1]\n"
   "ADD TEMP[1], CONST[1].yyyy, TEMP[2]\n"
   "ADD TEMP[2].x, TEMP[0].xxxx, TEMP[0].yyyy\n"
   "ADD TEMP[2].x, TEMP[0].zzzz, TEMP[0].xxxx\n"
   "SGE TEMP[0].xyz, TEMP[2].xxxx, TEMP[1]\n"
   "SGE TEMP[0].w, TEMP[0].wwww, TEMP[2].yyyy\n"
   "MOV %s, TEMP[0]\n";


struct shader_asm_info {
   VGint id;
   VGint num_tokens;
   const char * txt;

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
   {VEGA_SOLID_FILL_SHADER,       40,  solid_fill_asm,
    VG_FALSE, 0, 1, 0, 0, 0, 0},
   {VEGA_LINEAR_GRADIENT_SHADER, 200,  linear_grad_asm,
    VG_TRUE,  0, 5, 0, 1, 0, 5},
   {VEGA_RADIAL_GRADIENT_SHADER, 200,  radial_grad_asm,
    VG_TRUE,  0, 5, 0, 1, 0, 6},
   {VEGA_PATTERN_SHADER,         100,      pattern_asm,
    VG_TRUE,  1, 4, 0, 1, 0, 5},

   /* image draw modes */
   {VEGA_IMAGE_NORMAL_SHADER,    200, image_normal_asm,
    VG_TRUE,  0, 0, 3, 1, 0, 0},
   {VEGA_IMAGE_MULTIPLY_SHADER,  200, image_multiply_asm,
    VG_TRUE,  0, 0, 3, 1, 0, 2},
   {VEGA_IMAGE_STENCIL_SHADER,   200, image_stencil_asm,
    VG_TRUE,  0, 0, 3, 1, 0, 2},

   {VEGA_MASK_SHADER,            100,         mask_asm,
    VG_TRUE,  0, 0, 1, 1, 0, 2},

   /* extra blend modes */
   {VEGA_BLEND_MULTIPLY_SHADER,  200, blend_multiply_asm,
    VG_TRUE,  1, 1, 2, 1, 0, 5},
   {VEGA_BLEND_SCREEN_SHADER,    200, blend_screen_asm,
    VG_TRUE,  0, 0, 2, 1, 0, 4},
   {VEGA_BLEND_DARKEN_SHADER,    200, blend_darken_asm,
    VG_TRUE,  1, 1, 2, 1, 0, 6},
   {VEGA_BLEND_LIGHTEN_SHADER,   200, blend_lighten_asm,
    VG_TRUE,  1, 1, 2, 1, 0, 6},

   /* premultiply */
   {VEGA_PREMULTIPLY_SHADER,   100, premultiply_asm,
    VG_FALSE,  0, 0, 0, 0, 0, 1},
   {VEGA_UNPREMULTIPLY_SHADER,   100, unpremultiply_asm,
    VG_FALSE,  0, 0, 0, 0, 0, 1},

   /* color transform to black and white */
   {VEGA_BW_SHADER,   150, color_bw_asm,
    VG_FALSE,  1, 1, 0, 0, 0, 3},
};
#endif
