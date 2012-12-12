/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef TOY_TGSI_H
#define TOY_TGSI_H

#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "toy_compiler.h"

struct tgsi_token;
struct tgsi_full_instruction;
struct util_hash_table;

typedef void (*toy_tgsi_translate)(struct toy_compiler *tc,
      const struct tgsi_full_instruction *tgsi_inst,
      struct toy_dst *dst,
      struct toy_src *src);

struct toy_tgsi {
   struct toy_compiler *tc;
   bool aos;
   const toy_tgsi_translate *translate_table;

   struct util_hash_table *reg_mapping;

   struct {
      bool vs_prohibit_ucps;
      int fs_coord_origin;
      int fs_coord_pixel_center;
      bool fs_color0_writes_all_cbufs;
      int fs_depth_layout;
      int gs_input_prim;
      int gs_output_prim;
      int gs_max_output_vertices;
   } props;

   struct {
      enum toy_type *types;
      uint32_t (*buf)[4];
      int cur, size;
   } imm_data;

   struct {
      int index:16;
      unsigned usage_mask:4;        /* TGSI_WRITEMASK_x */
      unsigned semantic_name:8;     /* TGSI_SEMANTIC_x */
      unsigned semantic_index:8;
      unsigned interp:4;            /* TGSI_INTERPOLATE_x */
      unsigned centroid:1;
   } inputs[PIPE_MAX_SHADER_INPUTS];
   int num_inputs;

   struct {
      int index:16;
      unsigned undefined_mask:4;
      unsigned usage_mask:4;        /* TGSI_WRITEMASK_x */
      unsigned semantic_name:8;     /* TGSI_SEMANTIC_x */
      unsigned semantic_index:8;
   } outputs[PIPE_MAX_SHADER_OUTPUTS];
   int num_outputs;

   struct {
      int index:16;
      unsigned semantic_name:8;     /* TGSI_SEMANTIC_x */
      unsigned semantic_index:8;
   } system_values[8];
   int num_system_values;

   bool uses_kill;
};

/**
 * Find the slot of the TGSI input.
 */
static inline int
toy_tgsi_find_input(const struct toy_tgsi *tgsi, int index)
{
   int slot;

   for (slot = 0; slot < tgsi->num_inputs; slot++) {
      if (tgsi->inputs[slot].index == index)
         return slot;
   }

   return -1;
}

/**
 * Find the slot of the TGSI system value.
 */
static inline int
toy_tgsi_find_system_value(const struct toy_tgsi *tgsi, int index)
{
   int slot;

   for (slot = 0; slot < tgsi->num_system_values; slot++) {
      if (tgsi->system_values[slot].index == index)
         return slot;
   }

   return -1;
}

/**
 * Return the immediate data of the TGSI immediate.
 */
static inline const uint32_t *
toy_tgsi_get_imm(const struct toy_tgsi *tgsi, unsigned index,
                 enum toy_type *type)
{
   const uint32_t *imm;

   if (index >= tgsi->imm_data.cur)
      return NULL;

   imm = tgsi->imm_data.buf[index];
   if (type)
      *type = tgsi->imm_data.types[index];

   return imm;
}

/**
 * Return the dimension of the texture coordinates, as well as the location of
 * the shadow reference value or the sample index.
 */
static inline int
toy_tgsi_get_texture_coord_dim(int tgsi_tex, int *shadow_or_sample)
{
   int dim;

   /*
    * Depending on the texture target, (src0, src1.x) is interpreted
    * differently:
    *
    *   (s, *, *, *, *),          for 1D
    *   (s, t, *, *, *),          for 2D, RECT
    *   (s, t, r, *, *),          for 3D, CUBE
    *
    *   (s, layer, *, *, *),      for 1D_ARRAY
    *   (s, t, layer, *, *),      for 2D_ARRAY
    *   (s, t, r, layer, *),      for CUBE_ARRAY
    *
    *   (s, *, shadow, *, *),     for SHADOW1D
    *   (s, t, shadow, *, *),     for SHADOW2D, SHADOWRECT
    *   (s, t, r, shadow, *),     for SHADOWCUBE
    *
    *   (s, layer, shadow, *, *), for SHADOW1D_ARRAY
    *   (s, t, layer, shadow, *), for SHADOW2D_ARRAY
    *   (s, t, r, layer, shadow), for SHADOWCUBE_ARRAY
    *
    *   (s, t, sample, *, *),     for 2D_MSAA
    *   (s, t, layer, sample, *), for 2D_ARRAY_MSAA
    */
   switch (tgsi_tex) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
      dim = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
      dim = 2;
      break;
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      dim = 3;
      break;
   case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      dim = 4;
      break;
   default:
      assert(!"unknown texture target");
      dim = 0;
      break;
   }

   if (shadow_or_sample) {
      switch (tgsi_tex) {
      case TGSI_TEXTURE_SHADOW1D:
         /* there is a gap */
         *shadow_or_sample = 2;
         break;
      case TGSI_TEXTURE_SHADOW2D:
      case TGSI_TEXTURE_SHADOWRECT:
      case TGSI_TEXTURE_SHADOWCUBE:
      case TGSI_TEXTURE_SHADOW1D_ARRAY:
      case TGSI_TEXTURE_SHADOW2D_ARRAY:
      case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      case TGSI_TEXTURE_2D_MSAA:
      case TGSI_TEXTURE_2D_ARRAY_MSAA:
         *shadow_or_sample = dim;
         break;
      default:
         /* no shadow nor sample */
         *shadow_or_sample = -1;
         break;
      }
   }

   return dim;
}

void
toy_compiler_translate_tgsi(struct toy_compiler *tc,
                            const struct tgsi_token *tokens, bool aos,
                            struct toy_tgsi *tgsi);

void
toy_tgsi_cleanup(struct toy_tgsi *tgsi);

int
toy_tgsi_get_vrf(const struct toy_tgsi *tgsi,
                 enum tgsi_file_type file, int dimension, int index);

void
toy_tgsi_dump(const struct toy_tgsi *tgsi);

#endif /* TOY_TGSI_H */
