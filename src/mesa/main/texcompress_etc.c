/*
 * Copyright (C) 2011 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file texcompress_etc.c
 * GL_OES_compressed_ETC1_RGB8_texture support.
 * Supported ETC2 texture formats are:
 * GL_COMPRESSED_RGB8_ETC2
 */

#include <stdbool.h>
#include "mfeatures.h"
#include "texcompress.h"
#include "texcompress_etc.h"
#include "texstore.h"
#include "macros.h"
#include "swrast/s_context.h"

struct etc2_block {
   int distance;
   uint32_t pixel_indices;
   const int *modifier_tables[2];
   bool flipped;
   bool is_ind_mode;
   bool is_diff_mode;
   bool is_t_mode;
   bool is_h_mode;
   bool is_planar_mode;
   uint8_t base_colors[3][3];
   uint8_t paint_colors[4][3];
};

static const int etc2_distance_table[8] = {
   3, 6, 11, 16, 23, 32, 41, 64 };

/* define etc1_parse_block and etc. */
#define UINT8_TYPE GLubyte
#define TAG(x) x
#include "texcompress_etc_tmp.h"
#undef TAG
#undef UINT8_TYPE

GLboolean
_mesa_texstore_etc1_rgb8(TEXSTORE_PARAMS)
{
   /* GL_ETC1_RGB8_OES is only valid in glCompressedTexImage2D */
   ASSERT(0);

   return GL_FALSE;
}

void
_mesa_fetch_texel_2d_f_etc1_rgb8(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   struct etc1_block block;
   GLubyte dst[3];
   const GLubyte *src;

   src = (const GLubyte *) texImage->Map +
      (((texImage->RowStride + 3) / 4) * (j / 4) + (i / 4)) * 8;

   etc1_parse_block(&block, src);
   etc1_fetch_texel(&block, i % 4, j % 4, dst);

   texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
   texel[ACOMP] = 1.0f;
}

/**
 * Decode texture data in format `MESA_FORMAT_ETC1_RGB8` to
 * `MESA_FORMAT_ABGR8888`.
 *
 * The size of the source data must be a multiple of the ETC1 block size,
 * which is 8, even if the texture image's dimensions are not aligned to 4.
 * From the GL_OES_compressed_ETC1_RGB8_texture spec:
 *   The texture is described as a number of 4x4 pixel blocks. If the
 *   texture (or a particular mip-level) is smaller than 4 pixels in
 *   any dimension (such as a 2x2 or a 8x1 texture), the texture is
 *   found in the upper left part of the block(s), and the rest of the
 *   pixels are not used. For instance, a texture of size 4x2 will be
 *   placed in the upper half of a 4x4 block, and the lower half of the
 *   pixels in the block will not be accessed.
 *
 * \param src_width in pixels
 * \param src_height in pixels
 * \param dst_stride in bytes
 */
void
_mesa_etc1_unpack_rgba8888(uint8_t *dst_row,
                           unsigned dst_stride,
                           const uint8_t *src_row,
                           unsigned src_stride,
                           unsigned src_width,
                           unsigned src_height)
{
   etc1_unpack_rgba8888(dst_row, dst_stride,
                        src_row, src_stride,
                        src_width, src_height);
}

static uint8_t
etc2_base_color1_t_mode(const uint8_t *in, GLuint index)
{
   uint8_t R1a = 0, x = 0;
   /* base col 1 = extend_4to8bits( (R1a << 2) | R1b, G1, B1) */
   switch(index) {
   case 0:
      R1a = (in[0] >> 3) & 0x3;
      x = ((R1a << 2) | (in[0] & 0x3));
      break;
   case 1:
      x = ((in[1] >> 4) & 0xf);
      break;
   case 2:
      x = (in[1] & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color2_t_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /*extend 4to8bits(R2, G2, B2)*/
   switch(index) {
   case 0:
      x = ((in[2] >> 4) & 0xf );
      break;
   case 1:
      x = (in[2] & 0xf);
      break;
   case 2:
      x = ((in[3] >> 4) & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color1_h_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /* base col 1 = extend 4to8bits(R1, (G1a << 1) | G1b, (B1a << 3) | B1b) */
   switch(index) {
   case 0:
      x = ((in[0] >> 3) & 0xf);
      break;
   case 1:
      x = (((in[0] & 0x7) << 1) | ((in[1] >> 4) & 0x1));
      break;
   case 2:
      x = ((in[1] & 0x8) |
           (((in[1] & 0x3) << 1) | ((in[2] >> 7) & 0x1)));
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
 }

static uint8_t
etc2_base_color2_h_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /* base col 2 = extend 4to8bits(R2, G2, B2) */
   switch(index) {
   case 0:
      x = ((in[2] >> 3) & 0xf );
      break;
   case 1:
      x = (((in[2] & 0x7) << 1) | ((in[3] >> 7) & 0x1));
      break;
   case 2:
      x = ((in[3] >> 3) & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
 }

static uint8_t
etc2_base_color_o_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = ((in[0] >> 1) & 0x3f); /* RO */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (((in[0] & 0x1) << 6) | /* GO1 */
             ((in[1] >> 1) & 0x3f)); /* GO2 */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = (((in[1] & 0x1) << 5) | /* BO1 */
             (in[2] & 0x18) | /* BO2 */
             (((in[2] & 0x3) << 1) | ((in[3] >> 7) & 0x1))); /* BO3 */
      return ((tmp << 2) | (tmp >> 4));
    default:
      /* invalid index */
      return 0;
   }
}

static uint8_t
etc2_base_color_h_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[3] & 0x7c) >> 1) | /* RH1 */
             (in[3] & 0x1));         /* RH2 */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (in[4] >> 1) & 0x7f; /* GH */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = (((in[4] & 0x1) << 5) |
             ((in[5] >> 3) & 0x1f)); /* BH */
      return ((tmp << 2) | (tmp >> 4));
   default:
      /* invalid index */
      return 0;
   }
}

static uint8_t
etc2_base_color_v_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[5] & 0x7) << 0x3) |
             ((in[6] >> 5) & 0x7)); /* RV */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (((in[6] & 0x1f) << 2) |
             ((in[7] >> 6) & 0x3)); /* GV */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = in[7] & 0x3f; /* BV */
      return ((tmp << 2) | (tmp >> 4));
   default:
      /* invalid index */
      return 0;
   }
}

static uint8_t
etc2_clamp(int color)
{
   /* CLAMP(color, 0, 255) */
   return (uint8_t) CLAMP(color, 0, 255);
}

static void
etc2_rgb8_parse_block(struct etc2_block *block, const uint8_t *src)
{
   unsigned i;
   GLboolean diffbit = src[3] & 0x2;
   static const int lookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

   const int R_plus_dR = (src[0] >> 3) + lookup[src[0] & 0x7];
   const int G_plus_dG = (src[1] >> 3) + lookup[src[1] & 0x7];
   const int B_plus_dB = (src[2] >> 3) + lookup[src[2] & 0x7];

   /* Reset the mode flags */
   block->is_ind_mode = false;
   block->is_diff_mode = false;
   block->is_t_mode = false;
   block->is_h_mode = false;
   block->is_planar_mode = false;

   if (!diffbit) {
      /* individual mode */
      block->is_ind_mode = true;

      for (i = 0; i < 3; i++) {
         /* Texture decode algorithm is same for individual mode in etc1
          * & etc2.
          */
         block->base_colors[0][i] = etc1_base_color_ind_hi(src[i]);
         block->base_colors[1][i] = etc1_base_color_ind_lo(src[i]);
      }
   }
   else if (R_plus_dR < 0 || R_plus_dR > 31){
      /* T mode */
      block->is_t_mode = true;

      for(i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color1_t_mode(src, i);
         block->base_colors[1][i] = etc2_base_color2_t_mode(src, i);
      }
      /* pick distance */
      block->distance =
         etc2_distance_table[(((src[3] >> 2) & 0x3) << 1) |
                             (src[3] & 0x1)];

      for (i = 0; i < 3; i++) {
         block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i]);
         block->paint_colors[1][i] = etc2_clamp(block->base_colors[1][i] +
                                                block->distance);
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i]);
         block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
                                                block->distance);
      }
   }
   else if (G_plus_dG < 0 || G_plus_dG > 31){
      /* H mode */
      block->is_h_mode = true;
      int base_color_1_value, base_color_2_value;

      for(i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color1_h_mode(src, i);
         block->base_colors[1][i] = etc2_base_color2_h_mode(src, i);
      }

      base_color_1_value = (block->base_colors[0][0] << 16) +
                           (block->base_colors[0][1] << 8) +
                           block->base_colors[0][2];
      base_color_2_value = (block->base_colors[1][0] << 16) +
                           (block->base_colors[1][1] << 8) +
                           block->base_colors[1][2];
      /* pick distance */
      block->distance =
         etc2_distance_table[(src[3] & 0x4) |
                             ((src[3] & 0x1) << 1) |
                             (base_color_1_value >= base_color_2_value)];

      for (i = 0; i < 3; i++) {
         block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i] +
                                                block->distance);
         block->paint_colors[1][i] = etc2_clamp(block->base_colors[0][i] -
                                                block->distance);
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i] +
                                                block->distance);
         block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
                                                block->distance);
      }
   }
   else if (B_plus_dB < 0 || B_plus_dB > 31){
      /* Planar mode */
      block->is_planar_mode = true;

      for (i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color_o_planar(src, i);
         block->base_colors[1][i] = etc2_base_color_h_planar(src, i);
         block->base_colors[2][i] = etc2_base_color_v_planar(src, i);
      }
   }
   else if (diffbit) {
      /* differential mode */
      block->is_diff_mode = true;

      for (i = 0; i < 3; i++) {
         /* Texture decode algorithm is same for differential mode in etc1
          * & etc2.
          */
         block->base_colors[0][i] = etc1_base_color_diff_hi(src[i]);
         block->base_colors[1][i] = etc1_base_color_diff_lo(src[i]);
     }
   }

   if (block->is_ind_mode || block->is_diff_mode) {
      /* pick modifier tables. same for etc1 & etc2 textures */
      block->modifier_tables[0] = etc1_modifier_tables[(src[3] >> 5) & 0x7];
      block->modifier_tables[1] = etc1_modifier_tables[(src[3] >> 2) & 0x7];
      block->flipped = (src[3] & 0x1);
   }

   block->pixel_indices =
      (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

static void
etc2_rgb8_fetch_texel(const struct etc2_block *block,
      int x, int y, uint8_t *dst)
{
   const uint8_t *base_color;
   int modifier, bit, idx, blk;

   /* get pixel index */
   bit = y + x * 4;
   idx = ((block->pixel_indices >> (15 + bit)) & 0x2) |
         ((block->pixel_indices >>      (bit)) & 0x1);

   if (block->is_ind_mode || block->is_diff_mode) {
      /* Use pixel index and subblock to get the modifier */
      blk = (block->flipped) ? (y >= 2) : (x >= 2);
      base_color = block->base_colors[blk];
      modifier = block->modifier_tables[blk][idx];

      dst[0] = etc2_clamp(base_color[0] + modifier);
      dst[1] = etc2_clamp(base_color[1] + modifier);
      dst[2] = etc2_clamp(base_color[2] + modifier);
   }
   else if (block->is_t_mode || block->is_h_mode) {
      /* Use pixel index to pick one of the paint colors */
      dst[0] = block->paint_colors[idx][0];
      dst[1] = block->paint_colors[idx][1];
      dst[2] = block->paint_colors[idx][2];
   }
   else if (block->is_planar_mode) {
      /* {R(x, y) = clamp255((x × (RH − RO) + y × (RV − RO) + 4 × RO + 2) >> 2)
       * {G(x, y) = clamp255((x × (GH − GO) + y × (GV − GO) + 4 × GO + 2) >> 2)
       * {B(x, y) = clamp255((x × (BH − BO) + y × (BV − BO) + 4 × BO + 2) >> 2)
       */
      int red, green, blue;
      red = (x * (block->base_colors[1][0] - block->base_colors[0][0]) +
             y * (block->base_colors[2][0] - block->base_colors[0][0]) +
             4 * block->base_colors[0][0] + 2) >> 2;

      green = (x * (block->base_colors[1][1] - block->base_colors[0][1]) +
               y * (block->base_colors[2][1] - block->base_colors[0][1]) +
               4 * block->base_colors[0][1] + 2) >> 2;

      blue = (x * (block->base_colors[1][2] - block->base_colors[0][2]) +
              y * (block->base_colors[2][2] - block->base_colors[0][2]) +
              4 * block->base_colors[0][2] + 2) >> 2;

      dst[0] = etc2_clamp(red);
      dst[1] = etc2_clamp(green);
      dst[2] = etc2_clamp(blue);
  }
}

static void
etc2_unpack_rgb8(uint8_t *dst_row,
                 unsigned dst_stride,
                 const uint8_t *src_row,
                 unsigned src_stride,
                 unsigned width,
                 unsigned height)
{
   const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc2_block block;
   unsigned x, y, i, j;

   for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;

      for (x = 0; x < width; x+= bw) {
         etc2_rgb8_parse_block(&block, src);

         for (j = 0; j < bh; j++) {
            uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
            for (i = 0; i < bw; i++) {
               etc2_rgb8_fetch_texel(&block, i, j, dst);
               dst[3] = 255;
               dst += comps;
            }
         }

         src += bs;
      }

      src_row += src_stride;
   }
}

/* ETC2 texture formats are valid in glCompressedTexImage2D and
 * glCompressedTexSubImage2D functions */
GLboolean
_mesa_texstore_etc2_rgb8(TEXSTORE_PARAMS)
{
   ASSERT(0);

   return GL_FALSE;
}

void
_mesa_fetch_texel_2d_f_etc2_rgb8(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   struct etc2_block block;
   uint8_t dst[3];
   const uint8_t *src;

   src = texImage->Map +
      (((texImage->RowStride + 3) / 4) * (j / 4) + (i / 4)) * 8;

   etc2_rgb8_parse_block(&block, src);
   etc2_rgb8_fetch_texel(&block, i % 4, j % 4, dst);

   texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
   texel[ACOMP] = 1.0f;
}


/**
 * Decode texture data in format `MESA_FORMAT_ETC2_RGB8`
 *
 * The size of the source data must be a multiple of the ETC2 block size
 * even if the texture image's dimensions are not aligned to 4.
 *
 * \param src_width in pixels
 * \param src_height in pixels
 * \param dst_stride in bytes
 */

void
_mesa_unpack_etc2_format(uint8_t *dst_row,
                         unsigned dst_stride,
                         const uint8_t *src_row,
                         unsigned src_stride,
                         unsigned src_width,
                         unsigned src_height,
                         gl_format format)
{
   etc2_unpack_rgb8(dst_row, dst_stride,
                    src_row, src_stride,
                    src_width, src_height);
}
