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

#include "genhw/genhw.h"
#include "util/u_dual_blend.h"
#include "util/u_framebuffer.h"
#include "util/u_half.h"

#include "ilo_context.h"
#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_gpe_gen6.h"

/**
 * Translate a pipe logicop to the matching hardware logicop.
 */
static int
gen6_translate_pipe_logicop(unsigned logicop)
{
   switch (logicop) {
   case PIPE_LOGICOP_CLEAR:         return GEN6_LOGICOP_CLEAR;
   case PIPE_LOGICOP_NOR:           return GEN6_LOGICOP_NOR;
   case PIPE_LOGICOP_AND_INVERTED:  return GEN6_LOGICOP_AND_INVERTED;
   case PIPE_LOGICOP_COPY_INVERTED: return GEN6_LOGICOP_COPY_INVERTED;
   case PIPE_LOGICOP_AND_REVERSE:   return GEN6_LOGICOP_AND_REVERSE;
   case PIPE_LOGICOP_INVERT:        return GEN6_LOGICOP_INVERT;
   case PIPE_LOGICOP_XOR:           return GEN6_LOGICOP_XOR;
   case PIPE_LOGICOP_NAND:          return GEN6_LOGICOP_NAND;
   case PIPE_LOGICOP_AND:           return GEN6_LOGICOP_AND;
   case PIPE_LOGICOP_EQUIV:         return GEN6_LOGICOP_EQUIV;
   case PIPE_LOGICOP_NOOP:          return GEN6_LOGICOP_NOOP;
   case PIPE_LOGICOP_OR_INVERTED:   return GEN6_LOGICOP_OR_INVERTED;
   case PIPE_LOGICOP_COPY:          return GEN6_LOGICOP_COPY;
   case PIPE_LOGICOP_OR_REVERSE:    return GEN6_LOGICOP_OR_REVERSE;
   case PIPE_LOGICOP_OR:            return GEN6_LOGICOP_OR;
   case PIPE_LOGICOP_SET:           return GEN6_LOGICOP_SET;
   default:
      assert(!"unknown logicop function");
      return GEN6_LOGICOP_CLEAR;
   }
}

/**
 * Translate a pipe blend function to the matching hardware blend function.
 */
static int
gen6_translate_pipe_blend(unsigned blend)
{
   switch (blend) {
   case PIPE_BLEND_ADD:                return GEN6_BLENDFUNCTION_ADD;
   case PIPE_BLEND_SUBTRACT:           return GEN6_BLENDFUNCTION_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT:   return GEN6_BLENDFUNCTION_REVERSE_SUBTRACT;
   case PIPE_BLEND_MIN:                return GEN6_BLENDFUNCTION_MIN;
   case PIPE_BLEND_MAX:                return GEN6_BLENDFUNCTION_MAX;
   default:
      assert(!"unknown blend function");
      return GEN6_BLENDFUNCTION_ADD;
   };
}

/**
 * Translate a pipe blend factor to the matching hardware blend factor.
 */
static int
gen6_translate_pipe_blendfactor(unsigned blendfactor)
{
   switch (blendfactor) {
   case PIPE_BLENDFACTOR_ONE:                return GEN6_BLENDFACTOR_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:          return GEN6_BLENDFACTOR_SRC_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA:          return GEN6_BLENDFACTOR_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA:          return GEN6_BLENDFACTOR_DST_ALPHA;
   case PIPE_BLENDFACTOR_DST_COLOR:          return GEN6_BLENDFACTOR_DST_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return GEN6_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case PIPE_BLENDFACTOR_CONST_COLOR:        return GEN6_BLENDFACTOR_CONST_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:        return GEN6_BLENDFACTOR_CONST_ALPHA;
   case PIPE_BLENDFACTOR_SRC1_COLOR:         return GEN6_BLENDFACTOR_SRC1_COLOR;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:         return GEN6_BLENDFACTOR_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_ZERO:               return GEN6_BLENDFACTOR_ZERO;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:      return GEN6_BLENDFACTOR_INV_SRC_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:      return GEN6_BLENDFACTOR_INV_SRC_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:      return GEN6_BLENDFACTOR_INV_DST_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:      return GEN6_BLENDFACTOR_INV_DST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:    return GEN6_BLENDFACTOR_INV_CONST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:    return GEN6_BLENDFACTOR_INV_CONST_ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:     return GEN6_BLENDFACTOR_INV_SRC1_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:     return GEN6_BLENDFACTOR_INV_SRC1_ALPHA;
   default:
      assert(!"unknown blend factor");
      return GEN6_BLENDFACTOR_ONE;
   };
}

/**
 * Translate a pipe stencil op to the matching hardware stencil op.
 */
static int
gen6_translate_pipe_stencil_op(unsigned stencil_op)
{
   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:       return GEN6_STENCILOP_KEEP;
   case PIPE_STENCIL_OP_ZERO:       return GEN6_STENCILOP_ZERO;
   case PIPE_STENCIL_OP_REPLACE:    return GEN6_STENCILOP_REPLACE;
   case PIPE_STENCIL_OP_INCR:       return GEN6_STENCILOP_INCRSAT;
   case PIPE_STENCIL_OP_DECR:       return GEN6_STENCILOP_DECRSAT;
   case PIPE_STENCIL_OP_INCR_WRAP:  return GEN6_STENCILOP_INCR;
   case PIPE_STENCIL_OP_DECR_WRAP:  return GEN6_STENCILOP_DECR;
   case PIPE_STENCIL_OP_INVERT:     return GEN6_STENCILOP_INVERT;
   default:
      assert(!"unknown stencil op");
      return GEN6_STENCILOP_KEEP;
   }
}

/**
 * Translate a pipe texture mipfilter to the matching hardware mipfilter.
 */
static int
gen6_translate_tex_mipfilter(unsigned filter)
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NEAREST: return GEN6_MIPFILTER_NEAREST;
   case PIPE_TEX_MIPFILTER_LINEAR:  return GEN6_MIPFILTER_LINEAR;
   case PIPE_TEX_MIPFILTER_NONE:    return GEN6_MIPFILTER_NONE;
   default:
      assert(!"unknown mipfilter");
      return GEN6_MIPFILTER_NONE;
   }
}

/**
 * Translate a pipe texture filter to the matching hardware mapfilter.
 */
static int
gen6_translate_tex_filter(unsigned filter)
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST: return GEN6_MAPFILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:  return GEN6_MAPFILTER_LINEAR;
   default:
      assert(!"unknown sampler filter");
      return GEN6_MAPFILTER_NEAREST;
   }
}

/**
 * Translate a pipe texture coordinate wrapping mode to the matching hardware
 * wrapping mode.
 */
static int
gen6_translate_tex_wrap(unsigned wrap, bool clamp_to_edge)
{
   /* clamp to edge or border? */
   if (wrap == PIPE_TEX_WRAP_CLAMP) {
      wrap = (clamp_to_edge) ?
         PIPE_TEX_WRAP_CLAMP_TO_EDGE : PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   }

   switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:             return GEN6_TEXCOORDMODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:      return GEN6_TEXCOORDMODE_CLAMP;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:    return GEN6_TEXCOORDMODE_CLAMP_BORDER;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:      return GEN6_TEXCOORDMODE_MIRROR;
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(!"unknown sampler wrap mode");
      return GEN6_TEXCOORDMODE_WRAP;
   }
}

/**
 * Translate a pipe shadow compare function to the matching hardware shadow
 * function.
 */
static int
gen6_translate_shadow_func(unsigned func)
{
   /*
    * For PIPE_FUNC_x, the reference value is on the left-hand side of the
    * comparison, and 1.0 is returned when the comparison is true.
    *
    * For GEN6_COMPAREFUNCTION_x, the reference value is on the right-hand side of
    * the comparison, and 0.0 is returned when the comparison is true.
    */
   switch (func) {
   case PIPE_FUNC_NEVER:      return GEN6_COMPAREFUNCTION_ALWAYS;
   case PIPE_FUNC_LESS:       return GEN6_COMPAREFUNCTION_LEQUAL;
   case PIPE_FUNC_EQUAL:      return GEN6_COMPAREFUNCTION_NOTEQUAL;
   case PIPE_FUNC_LEQUAL:     return GEN6_COMPAREFUNCTION_LESS;
   case PIPE_FUNC_GREATER:    return GEN6_COMPAREFUNCTION_GEQUAL;
   case PIPE_FUNC_NOTEQUAL:   return GEN6_COMPAREFUNCTION_EQUAL;
   case PIPE_FUNC_GEQUAL:     return GEN6_COMPAREFUNCTION_GREATER;
   case PIPE_FUNC_ALWAYS:     return GEN6_COMPAREFUNCTION_NEVER;
   default:
      assert(!"unknown shadow compare function");
      return GEN6_COMPAREFUNCTION_NEVER;
   }
}

/**
 * Translate a pipe DSA test function to the matching hardware compare
 * function.
 */
static int
gen6_translate_dsa_func(unsigned func)
{
   switch (func) {
   case PIPE_FUNC_NEVER:      return GEN6_COMPAREFUNCTION_NEVER;
   case PIPE_FUNC_LESS:       return GEN6_COMPAREFUNCTION_LESS;
   case PIPE_FUNC_EQUAL:      return GEN6_COMPAREFUNCTION_EQUAL;
   case PIPE_FUNC_LEQUAL:     return GEN6_COMPAREFUNCTION_LEQUAL;
   case PIPE_FUNC_GREATER:    return GEN6_COMPAREFUNCTION_GREATER;
   case PIPE_FUNC_NOTEQUAL:   return GEN6_COMPAREFUNCTION_NOTEQUAL;
   case PIPE_FUNC_GEQUAL:     return GEN6_COMPAREFUNCTION_GEQUAL;
   case PIPE_FUNC_ALWAYS:     return GEN6_COMPAREFUNCTION_ALWAYS;
   default:
      assert(!"unknown depth/stencil/alpha test function");
      return GEN6_COMPAREFUNCTION_NEVER;
   }
}

static void
ve_init_cso(const struct ilo_dev_info *dev,
            const struct pipe_vertex_element *state,
            unsigned vb_index,
            struct ilo_ve_cso *cso)
{
   int comp[4] = {
      GEN6_VFCOMP_STORE_SRC,
      GEN6_VFCOMP_STORE_SRC,
      GEN6_VFCOMP_STORE_SRC,
      GEN6_VFCOMP_STORE_SRC,
   };
   int format;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   switch (util_format_get_nr_components(state->src_format)) {
   case 1: comp[1] = GEN6_VFCOMP_STORE_0;
   case 2: comp[2] = GEN6_VFCOMP_STORE_0;
   case 3: comp[3] = (util_format_is_pure_integer(state->src_format)) ?
                     GEN6_VFCOMP_STORE_1_INT :
                     GEN6_VFCOMP_STORE_1_FP;
   }

   format = ilo_translate_vertex_format(state->src_format);

   STATIC_ASSERT(Elements(cso->payload) >= 2);
   cso->payload[0] =
      vb_index << GEN6_VE_STATE_DW0_VB_INDEX__SHIFT |
      GEN6_VE_STATE_DW0_VALID |
      format << GEN6_VE_STATE_DW0_FORMAT__SHIFT |
      state->src_offset << GEN6_VE_STATE_DW0_VB_OFFSET__SHIFT;

   cso->payload[1] =
         comp[0] << GEN6_VE_STATE_DW1_COMP0__SHIFT |
         comp[1] << GEN6_VE_STATE_DW1_COMP1__SHIFT |
         comp[2] << GEN6_VE_STATE_DW1_COMP2__SHIFT |
         comp[3] << GEN6_VE_STATE_DW1_COMP3__SHIFT;
}

void
ilo_gpe_init_ve(const struct ilo_dev_info *dev,
                unsigned num_states,
                const struct pipe_vertex_element *states,
                struct ilo_ve_state *ve)
{
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   ve->count = num_states;
   ve->vb_count = 0;

   for (i = 0; i < num_states; i++) {
      const unsigned pipe_idx = states[i].vertex_buffer_index;
      const unsigned instance_divisor = states[i].instance_divisor;
      unsigned hw_idx;

      /*
       * map the pipe vb to the hardware vb, which has a fixed instance
       * divisor
       */
      for (hw_idx = 0; hw_idx < ve->vb_count; hw_idx++) {
         if (ve->vb_mapping[hw_idx] == pipe_idx &&
             ve->instance_divisors[hw_idx] == instance_divisor)
            break;
      }

      /* create one if there is no matching hardware vb */
      if (hw_idx >= ve->vb_count) {
         hw_idx = ve->vb_count++;

         ve->vb_mapping[hw_idx] = pipe_idx;
         ve->instance_divisors[hw_idx] = instance_divisor;
      }

      ve_init_cso(dev, &states[i], hw_idx, &ve->cso[i]);
   }
}

void
ilo_gpe_init_vs_cso(const struct ilo_dev_info *dev,
                    const struct ilo_shader_state *vs,
                    struct ilo_shader_cso *cso)
{
   int start_grf, vue_read_len, max_threads;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   start_grf = ilo_shader_get_kernel_param(vs, ILO_KERNEL_URB_DATA_START_REG);
   vue_read_len = ilo_shader_get_kernel_param(vs, ILO_KERNEL_INPUT_COUNT);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 135:
    *
    *     "(Vertex URB Entry Read Length) Specifies the number of pairs of
    *      128-bit vertex elements to be passed into the payload for each
    *      vertex."
    *
    *     "It is UNDEFINED to set this field to 0 indicating no Vertex URB
    *      data to be read and passed to the thread."
    */
   vue_read_len = (vue_read_len + 1) / 2;
   if (!vue_read_len)
      vue_read_len = 1;

   switch (dev->gen) {
   case ILO_GEN(6):
      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 22:
       *
       *     "Device             # of EUs        #Threads/EU
       *      SNB GT2            12              5
       *      SNB GT1            6               4"
       */
      max_threads = (dev->gt == 2) ? 60 : 24;
      break;
   case ILO_GEN(7):
      /*
       * From the Ivy Bridge PRM, volume 1 part 1, page 18:
       *
       *     "Device             # of EUs        #Threads/EU
       *      Ivy Bridge (GT2)   16              8
       *      Ivy Bridge (GT1)   6               6"
       */
      max_threads = (dev->gt == 2) ? 128 : 36;
      break;
   case ILO_GEN(7.5):
      /* see brwCreateContext() */
      max_threads = (dev->gt >= 2) ? 280 : 70;
      break;
   default:
      max_threads = 1;
      break;
   }

   dw2 = (true) ? 0 : GEN6_THREADDISP_FP_MODE_ALT;

   dw4 = start_grf << GEN6_VS_DW4_URB_GRF_START__SHIFT |
         vue_read_len << GEN6_VS_DW4_URB_READ_LEN__SHIFT |
         0 << GEN6_VS_DW4_URB_READ_OFFSET__SHIFT;

   dw5 = GEN6_VS_DW5_STATISTICS |
         GEN6_VS_DW5_VS_ENABLE;

   if (dev->gen >= ILO_GEN(7.5))
      dw5 |= (max_threads - 1) << GEN75_VS_DW5_MAX_THREADS__SHIFT;
   else
      dw5 |= (max_threads - 1) << GEN6_VS_DW5_MAX_THREADS__SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 3);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
}

void
ilo_gpe_init_gs_cso_gen6(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *gs,
                         struct ilo_shader_cso *cso)
{
   int start_grf, vue_read_len, max_threads;
   uint32_t dw2, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   if (ilo_shader_get_type(gs) == PIPE_SHADER_GEOMETRY) {
      start_grf = ilo_shader_get_kernel_param(gs,
            ILO_KERNEL_URB_DATA_START_REG);

      vue_read_len = ilo_shader_get_kernel_param(gs, ILO_KERNEL_INPUT_COUNT);
   }
   else {
      start_grf = ilo_shader_get_kernel_param(gs,
            ILO_KERNEL_VS_GEN6_SO_START_REG);

      vue_read_len = ilo_shader_get_kernel_param(gs, ILO_KERNEL_OUTPUT_COUNT);
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 153:
    *
    *     "Specifies the amount of URB data read and passed in the thread
    *      payload for each Vertex URB entry, in 256-bit register increments.
    *
    *      It is UNDEFINED to set this field (Vertex URB Entry Read Length) to
    *      0 indicating no Vertex URB data to be read and passed to the
    *      thread."
    */
   vue_read_len = (vue_read_len + 1) / 2;
   if (!vue_read_len)
      vue_read_len = 1;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 154:
    *
    *     "Maximum Number of Threads valid range is [0,27] when Rendering
    *      Enabled bit is set."
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 173:
    *
    *     "Programming Note: If the GS stage is enabled, software must always
    *      allocate at least one GS URB Entry. This is true even if the GS
    *      thread never needs to output vertices to the pipeline, e.g., when
    *      only performing stream output. This is an artifact of the need to
    *      pass the GS thread an initial destination URB handle."
    *
    * As such, we always enable rendering, and limit the number of threads.
    */
   if (dev->gt == 2) {
      /* maximum is 60, but limited to 28 */
      max_threads = 28;
   }
   else {
      /* maximum is 24, but limited to 21 (see brwCreateContext()) */
      max_threads = 21;
   }

   dw2 = GEN6_THREADDISP_SPF;

   dw4 = vue_read_len << GEN6_GS_DW4_URB_READ_LEN__SHIFT |
         0 << GEN6_GS_DW4_URB_READ_OFFSET__SHIFT |
         start_grf << GEN6_GS_DW4_URB_GRF_START__SHIFT;

   dw5 = (max_threads - 1) << GEN6_GS_DW5_MAX_THREADS__SHIFT |
         GEN6_GS_DW5_STATISTICS |
         GEN6_GS_DW5_SO_STATISTICS |
         GEN6_GS_DW5_RENDER_ENABLE;

   /*
    * we cannot make use of GEN6_GS_REORDER because it will reorder
    * triangle strips according to D3D rules (triangle 2N+1 uses vertices
    * (2N+1, 2N+3, 2N+2)), instead of GL rules (triangle 2N+1 uses vertices
    * (2N+2, 2N+1, 2N+3)).
    */
   dw6 = GEN6_GS_DW6_GS_ENABLE;

   if (ilo_shader_get_kernel_param(gs, ILO_KERNEL_GS_DISCARD_ADJACENCY))
      dw6 |= GEN6_GS_DW6_DISCARD_ADJACENCY;

   if (ilo_shader_get_kernel_param(gs, ILO_KERNEL_VS_GEN6_SO)) {
      const uint32_t svbi_post_inc =
         ilo_shader_get_kernel_param(gs, ILO_KERNEL_GS_GEN6_SVBI_POST_INC);

      dw6 |= GEN6_GS_DW6_SVBI_PAYLOAD_ENABLE;
      if (svbi_post_inc) {
         dw6 |= GEN6_GS_DW6_SVBI_POST_INC_ENABLE |
                svbi_post_inc << GEN6_GS_DW6_SVBI_POST_INC_VAL__SHIFT;
      }
   }

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = dw6;
}

void
ilo_gpe_init_rasterizer_clip(const struct ilo_dev_info *dev,
                             const struct pipe_rasterizer_state *state,
                             struct ilo_rasterizer_clip *clip)
{
   uint32_t dw1, dw2, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   dw1 = GEN6_CLIP_DW1_STATISTICS;

   if (dev->gen >= ILO_GEN(7)) {
      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 219:
       *
       *     "Workaround : Due to Hardware issue "EarlyCull" needs to be
       *      enabled only for the cases where the incoming primitive topology
       *      into the clipper guaranteed to be Trilist."
       *
       * What does this mean?
       */
      dw1 |= 0 << 19 |
             GEN7_CLIP_DW1_EARLY_CULL_ENABLE;

      if (state->front_ccw)
         dw1 |= GEN7_CLIP_DW1_FRONTWINDING_CCW;

      switch (state->cull_face) {
      case PIPE_FACE_NONE:
         dw1 |= GEN7_CLIP_DW1_CULLMODE_NONE;
         break;
      case PIPE_FACE_FRONT:
         dw1 |= GEN7_CLIP_DW1_CULLMODE_FRONT;
         break;
      case PIPE_FACE_BACK:
         dw1 |= GEN7_CLIP_DW1_CULLMODE_BACK;
         break;
      case PIPE_FACE_FRONT_AND_BACK:
         dw1 |= GEN7_CLIP_DW1_CULLMODE_BOTH;
         break;
      }
   }

   dw2 = GEN6_CLIP_DW2_CLIP_ENABLE |
         GEN6_CLIP_DW2_XY_TEST_ENABLE |
         state->clip_plane_enable << GEN6_CLIP_DW2_UCP_CLIP_ENABLES__SHIFT |
         GEN6_CLIP_DW2_CLIPMODE_NORMAL;

   if (state->clip_halfz)
      dw2 |= GEN6_CLIP_DW2_APIMODE_D3D;
   else
      dw2 |= GEN6_CLIP_DW2_APIMODE_OGL;

   if (state->depth_clip)
      dw2 |= GEN6_CLIP_DW2_Z_TEST_ENABLE;

   if (state->flatshade_first) {
      dw2 |= 0 << GEN6_CLIP_DW2_TRI_PROVOKE__SHIFT |
             0 << GEN6_CLIP_DW2_LINE_PROVOKE__SHIFT |
             1 << GEN6_CLIP_DW2_TRIFAN_PROVOKE__SHIFT;
   }
   else {
      dw2 |= 2 << GEN6_CLIP_DW2_TRI_PROVOKE__SHIFT |
             1 << GEN6_CLIP_DW2_LINE_PROVOKE__SHIFT |
             2 << GEN6_CLIP_DW2_TRIFAN_PROVOKE__SHIFT;
   }

   dw3 = 0x1 << GEN6_CLIP_DW3_MIN_POINT_WIDTH__SHIFT |
         0x7ff << GEN6_CLIP_DW3_MAX_POINT_WIDTH__SHIFT;

   clip->payload[0] = dw1;
   clip->payload[1] = dw2;
   clip->payload[2] = dw3;

   clip->can_enable_guardband = true;

   /*
    * There are several reasons that guard band test should be disabled
    *
    *  - GL wide points (to avoid partially visibie object)
    *  - GL wide or AA lines (to avoid partially visibie object)
    */
   if (state->point_size_per_vertex || state->point_size > 1.0f)
      clip->can_enable_guardband = false;
   if (state->line_smooth || state->line_width > 1.0f)
      clip->can_enable_guardband = false;
}

void
ilo_gpe_init_rasterizer_sf(const struct ilo_dev_info *dev,
                           const struct pipe_rasterizer_state *state,
                           struct ilo_rasterizer_sf *sf)
{
   float offset_const, offset_scale, offset_clamp;
   int line_width, point_width;
   uint32_t dw1, dw2, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   /*
    * Scale the constant term.  The minimum representable value used by the HW
    * is not large enouch to be the minimum resolvable difference.
    */
   offset_const = state->offset_units * 2.0f;

   offset_scale = state->offset_scale;
   offset_clamp = state->offset_clamp;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 248:
    *
    *     "This bit (Statistics Enable) should be set whenever clipping is
    *      enabled and the Statistics Enable bit is set in CLIP_STATE. It
    *      should be cleared if clipping is disabled or Statistics Enable in
    *      CLIP_STATE is clear."
    */
   dw1 = GEN7_SF_DW1_STATISTICS |
         GEN7_SF_DW1_VIEWPORT_ENABLE;

   /* XXX GEN6 path seems to work fine for GEN7 */
   if (false && dev->gen >= ILO_GEN(7)) {
      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 258:
       *
       *     "This bit (Legacy Global Depth Bias Enable, Global Depth Offset
       *      Enable Solid , Global Depth Offset Enable Wireframe, and Global
       *      Depth Offset Enable Point) should be set whenever non zero depth
       *      bias (Slope, Bias) values are used. Setting this bit may have
       *      some degradation of performance for some workloads."
       */
      if (state->offset_tri || state->offset_line || state->offset_point) {
         /* XXX need to scale offset_const according to the depth format */
         dw1 |= GEN7_SF_DW1_LEGACY_DEPTH_OFFSET;

         dw1 |= GEN7_SF_DW1_DEPTH_OFFSET_SOLID |
                GEN7_SF_DW1_DEPTH_OFFSET_WIREFRAME |
                GEN7_SF_DW1_DEPTH_OFFSET_POINT;
      }
      else {
         offset_const = 0.0f;
         offset_scale = 0.0f;
         offset_clamp = 0.0f;
      }
   }
   else {
      if (state->offset_tri)
         dw1 |= GEN7_SF_DW1_DEPTH_OFFSET_SOLID;
      if (state->offset_line)
         dw1 |= GEN7_SF_DW1_DEPTH_OFFSET_WIREFRAME;
      if (state->offset_point)
         dw1 |= GEN7_SF_DW1_DEPTH_OFFSET_POINT;
   }

   switch (state->fill_front) {
   case PIPE_POLYGON_MODE_FILL:
      dw1 |= GEN7_SF_DW1_FRONTFACE_SOLID;
      break;
   case PIPE_POLYGON_MODE_LINE:
      dw1 |= GEN7_SF_DW1_FRONTFACE_WIREFRAME;
      break;
   case PIPE_POLYGON_MODE_POINT:
      dw1 |= GEN7_SF_DW1_FRONTFACE_POINT;
      break;
   }

   switch (state->fill_back) {
   case PIPE_POLYGON_MODE_FILL:
      dw1 |= GEN7_SF_DW1_BACKFACE_SOLID;
      break;
   case PIPE_POLYGON_MODE_LINE:
      dw1 |= GEN7_SF_DW1_BACKFACE_WIREFRAME;
      break;
   case PIPE_POLYGON_MODE_POINT:
      dw1 |= GEN7_SF_DW1_BACKFACE_POINT;
      break;
   }

   if (state->front_ccw)
      dw1 |= GEN7_SF_DW1_FRONTWINDING_CCW;

   dw2 = 0;

   if (state->line_smooth) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 251:
       *
       *     "This field (Anti-aliasing Enable) must be disabled if any of the
       *      render targets have integer (UINT or SINT) surface format."
       *
       * From the Sandy Bridge PRM, volume 2 part 1, page 317:
       *
       *     "This field (Hierarchical Depth Buffer Enable) must be disabled
       *      if Anti-aliasing Enable in 3DSTATE_SF is enabled.
       *
       * TODO We do not check those yet.
       */
      dw2 |= GEN7_SF_DW2_AA_LINE_ENABLE |
             GEN7_SF_DW2_AA_LINE_CAP_1_0;
   }

   switch (state->cull_face) {
   case PIPE_FACE_NONE:
      dw2 |= GEN7_SF_DW2_CULLMODE_NONE;
      break;
   case PIPE_FACE_FRONT:
      dw2 |= GEN7_SF_DW2_CULLMODE_FRONT;
      break;
   case PIPE_FACE_BACK:
      dw2 |= GEN7_SF_DW2_CULLMODE_BACK;
      break;
   case PIPE_FACE_FRONT_AND_BACK:
      dw2 |= GEN7_SF_DW2_CULLMODE_BOTH;
      break;
   }

   /*
    * Smooth lines should intersect ceil(line_width) or (ceil(line_width) + 1)
    * pixels in the minor direction.  We have to make the lines slightly
    * thicker, 0.5 pixel on both sides, so that they intersect that many
    * pixels are considered into the lines.
    *
    * Line width is in U3.7.
    */
   line_width = (int) ((state->line_width +
            (float) state->line_smooth) * 128.0f + 0.5f);
   line_width = CLAMP(line_width, 0, 1023);

   if (line_width == 128 && !state->line_smooth) {
      /* use GIQ rules */
      line_width = 0;
   }

   dw2 |= line_width << GEN7_SF_DW2_LINE_WIDTH__SHIFT;

   if (dev->gen >= ILO_GEN(7.5) && state->line_stipple_enable)
      dw2 |= GEN75_SF_DW2_LINE_STIPPLE_ENABLE;

   if (state->scissor)
      dw2 |= GEN7_SF_DW2_SCISSOR_ENABLE;

   dw3 = GEN7_SF_DW3_TRUE_AA_LINE_DISTANCE |
         GEN7_SF_DW3_SUBPIXEL_8BITS;

   if (state->line_last_pixel)
      dw3 |= 1 << 31;

   if (state->flatshade_first) {
      dw3 |= 0 << GEN7_SF_DW3_TRI_PROVOKE__SHIFT |
             0 << GEN7_SF_DW3_LINE_PROVOKE__SHIFT |
             1 << GEN7_SF_DW3_TRIFAN_PROVOKE__SHIFT;
   }
   else {
      dw3 |= 2 << GEN7_SF_DW3_TRI_PROVOKE__SHIFT |
             1 << GEN7_SF_DW3_LINE_PROVOKE__SHIFT |
             2 << GEN7_SF_DW3_TRIFAN_PROVOKE__SHIFT;
   }

   if (!state->point_size_per_vertex)
      dw3 |= GEN7_SF_DW3_USE_POINT_WIDTH;

   /* in U8.3 */
   point_width = (int) (state->point_size * 8.0f + 0.5f);
   point_width = CLAMP(point_width, 1, 2047);

   dw3 |= point_width;

   STATIC_ASSERT(Elements(sf->payload) >= 6);
   sf->payload[0] = dw1;
   sf->payload[1] = dw2;
   sf->payload[2] = dw3;
   sf->payload[3] = fui(offset_const);
   sf->payload[4] = fui(offset_scale);
   sf->payload[5] = fui(offset_clamp);

   if (state->multisample) {
      sf->dw_msaa = GEN7_SF_DW2_MSRASTMODE_ON_PATTERN;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 251:
       *
       *     "Software must not program a value of 0.0 when running in
       *      MSRASTMODE_ON_xxx modes - zero-width lines are not available
       *      when multisampling rasterization is enabled."
       */
      if (!line_width) {
         line_width = 128; /* 1.0f */

         sf->dw_msaa |= line_width << GEN7_SF_DW2_LINE_WIDTH__SHIFT;
      }
   }
   else {
      sf->dw_msaa = 0;
   }
}

void
ilo_gpe_init_rasterizer_wm_gen6(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm)
{
   uint32_t dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /* only the FF unit states are set, as in GEN7 */

   dw5 = GEN6_WM_DW5_AA_LINE_WIDTH_2_0;

   /* same value as in 3DSTATE_SF */
   if (state->line_smooth)
      dw5 |= GEN6_WM_DW5_AA_LINE_CAP_1_0;

   if (state->poly_stipple_enable)
      dw5 |= GEN6_WM_DW5_POLY_STIPPLE_ENABLE;
   if (state->line_stipple_enable)
      dw5 |= GEN6_WM_DW5_LINE_STIPPLE_ENABLE;

   dw6 = GEN6_WM_DW6_ZW_INTERP_PIXEL |
         GEN6_WM_DW6_MSRASTMODE_OFF_PIXEL |
         GEN6_WM_DW6_MSDISPMODE_PERSAMPLE;

   if (state->bottom_edge_rule)
      dw6 |= GEN6_WM_DW6_POINT_RASTRULE_UPPER_RIGHT;

   /*
    * assertion that makes sure
    *
    *   dw6 |= wm->dw_msaa_rast | wm->dw_msaa_disp;
    *
    * is valid
    */
   STATIC_ASSERT(GEN6_WM_DW6_MSRASTMODE_OFF_PIXEL == 0 &&
                 GEN6_WM_DW6_MSDISPMODE_PERSAMPLE == 0);

   wm->dw_msaa_rast =
      (state->multisample) ? GEN6_WM_DW6_MSRASTMODE_ON_PATTERN : 0;
   wm->dw_msaa_disp = GEN6_WM_DW6_MSDISPMODE_PERPIXEL;

   STATIC_ASSERT(Elements(wm->payload) >= 2);
   wm->payload[0] = dw5;
   wm->payload[1] = dw6;
}

void
ilo_gpe_init_fs_cso_gen6(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *fs,
                         struct ilo_shader_cso *cso)
{
   int start_grf, input_count, interps, max_threads;
   uint32_t dw2, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   start_grf = ilo_shader_get_kernel_param(fs, ILO_KERNEL_URB_DATA_START_REG);
   input_count = ilo_shader_get_kernel_param(fs, ILO_KERNEL_INPUT_COUNT);
   interps = ilo_shader_get_kernel_param(fs,
         ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS);

   /* see brwCreateContext() */
   max_threads = (dev->gt == 2) ? 80 : 40;

   dw2 = (true) ? 0 : GEN6_THREADDISP_FP_MODE_ALT;

   dw4 = start_grf << GEN6_WM_DW4_URB_GRF_START0__SHIFT |
         0 << GEN6_WM_DW4_URB_GRF_START1__SHIFT |
         0 << GEN6_WM_DW4_URB_GRF_START2__SHIFT;

   dw5 = (max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 275:
    *
    *     "This bit (Pixel Shader Kill Pixel), if ENABLED, indicates that the
    *      PS kernel or color calculator has the ability to kill (discard)
    *      pixels or samples, other than due to depth or stencil testing.
    *      This bit is required to be ENABLED in the following situations:
    *
    *      The API pixel shader program contains "killpix" or "discard"
    *      instructions, or other code in the pixel shader kernel that can
    *      cause the final pixel mask to differ from the pixel mask received
    *      on dispatch.
    *
    *      A sampler with chroma key enabled with kill pixel mode is used by
    *      the pixel shader.
    *
    *      Any render target has Alpha Test Enable or AlphaToCoverage Enable
    *      enabled.
    *
    *      The pixel shader kernel generates and outputs oMask.
    *
    *      Note: As ClipDistance clipping is fully supported in hardware and
    *      therefore not via PS instructions, there should be no need to
    *      ENABLE this bit due to ClipDistance clipping."
    */
   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_USE_KILL))
      dw5 |= GEN6_WM_DW5_PS_KILL;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 275:
    *
    *     "If a NULL Depth Buffer is selected, the Pixel Shader Computed Depth
    *      field must be set to disabled."
    *
    * TODO This is not checked yet.
    */
   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_OUTPUT_Z))
      dw5 |= GEN6_WM_DW5_PS_COMPUTE_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_Z))
      dw5 |= GEN6_WM_DW5_PS_USE_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_W))
      dw5 |= GEN6_WM_DW5_PS_USE_W;

   /*
    * TODO set this bit only when
    *
    *  a) fs writes colors and color is not masked, or
    *  b) fs writes depth, or
    *  c) fs or cc kills
    */
   if (true)
      dw5 |= GEN6_WM_DW5_PS_ENABLE;

   assert(!ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_DISPATCH_16_OFFSET));
   dw5 |= GEN6_WM_DW5_8_PIXEL_DISPATCH;

   dw6 = input_count << GEN6_WM_DW6_SF_ATTR_COUNT__SHIFT |
         GEN6_WM_DW6_POSOFFSET_NONE |
         interps << GEN6_WM_DW6_BARYCENTRIC_INTERP__SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = dw6;
}

struct ilo_zs_surface_info {
   int surface_type;
   int format;

   struct {
      struct intel_bo *bo;
      unsigned stride;
      enum intel_tiling_mode tiling;
      uint32_t offset;
   } zs, stencil, hiz;

   unsigned width, height, depth;
   unsigned lod, first_layer, num_layers;
   uint32_t x_offset, y_offset;
};

static void
zs_init_info_null(const struct ilo_dev_info *dev,
                  struct ilo_zs_surface_info *info)
{
   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   memset(info, 0, sizeof(*info));

   info->surface_type = GEN6_SURFTYPE_NULL;
   info->format = GEN6_ZFORMAT_D32_FLOAT;
   info->width = 1;
   info->height = 1;
   info->depth = 1;
   info->num_layers = 1;
}

static void
zs_init_info(const struct ilo_dev_info *dev,
             const struct ilo_texture *tex,
             enum pipe_format format, unsigned level,
             unsigned first_layer, unsigned num_layers,
             bool offset_to_layer, struct ilo_zs_surface_info *info)
{
   uint32_t x_offset[3], y_offset[3];
   bool separate_stencil;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   memset(info, 0, sizeof(*info));

   info->surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);

   if (info->surface_type == GEN6_SURFTYPE_CUBE) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 325-326:
       *
       *     "For Other Surfaces (Cube Surfaces):
       *      This field (Minimum Array Element) is ignored."
       *
       *     "For Other Surfaces (Cube Surfaces):
       *      This field (Render Target View Extent) is ignored."
       *
       * As such, we cannot set first_layer and num_layers on cube surfaces.
       * To work around that, treat it as a 2D surface.
       */
      info->surface_type = GEN6_SURFTYPE_2D;
   }

   if (dev->gen >= ILO_GEN(7)) {
      separate_stencil = true;
   }
   else {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 317:
       *
       *     "This field (Separate Stencil Buffer Enable) must be set to the
       *      same value (enabled or disabled) as Hierarchical Depth Buffer
       *      Enable."
       */
      separate_stencil =
         ilo_texture_can_enable_hiz(tex, level, first_layer, num_layers);
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 317:
    *
    *     "If this field (Hierarchical Depth Buffer Enable) is enabled, the
    *      Surface Format of the depth buffer cannot be
    *      D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT. Use of stencil
    *      requires the separate stencil buffer."
    *
    * From the Ironlake PRM, volume 2 part 1, page 330:
    *
    *     "If this field (Separate Stencil Buffer Enable) is disabled, the
    *      Surface Format of the depth buffer cannot be D24_UNORM_X8_UINT."
    *
    * There is no similar restriction for GEN6.  But when D24_UNORM_X8_UINT
    * is indeed used, the depth values output by the fragment shaders will
    * be different when read back.
    *
    * As for GEN7+, separate_stencil is always true.
    */
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      info->format = GEN6_ZFORMAT_D16_UNORM;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      info->format = GEN6_ZFORMAT_D32_FLOAT;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      info->format = (separate_stencil) ?
         GEN6_ZFORMAT_D24_UNORM_X8_UINT :
         GEN6_ZFORMAT_D24_UNORM_S8_UINT;
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      info->format = (separate_stencil) ?
         GEN6_ZFORMAT_D32_FLOAT :
         GEN6_ZFORMAT_D32_FLOAT_S8X24_UINT;
      break;
   case PIPE_FORMAT_S8_UINT:
      if (separate_stencil) {
         info->format = GEN6_ZFORMAT_D32_FLOAT;
         break;
      }
      /* fall through */
   default:
      assert(!"unsupported depth/stencil format");
      zs_init_info_null(dev, info);
      return;
      break;
   }

   if (format != PIPE_FORMAT_S8_UINT) {
      info->zs.bo = tex->bo;
      info->zs.stride = tex->bo_stride;
      info->zs.tiling = tex->tiling;

      if (offset_to_layer) {
         info->zs.offset = ilo_texture_get_slice_offset(tex,
               level, first_layer, &x_offset[0], &y_offset[0]);
      }
   }

   if (tex->separate_s8 || format == PIPE_FORMAT_S8_UINT) {
      const struct ilo_texture *s8_tex =
         (tex->separate_s8) ? tex->separate_s8 : tex;

      info->stencil.bo = s8_tex->bo;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 329:
       *
       *     "The pitch must be set to 2x the value computed based on width,
       *       as the stencil buffer is stored with two rows interleaved."
       *
       * According to the classic driver, we need to do the same for GEN7+
       * even though the Ivy Bridge PRM does not say anything about it.
       */
      info->stencil.stride = s8_tex->bo_stride * 2;

      info->stencil.tiling = s8_tex->tiling;

      if (offset_to_layer) {
         info->stencil.offset = ilo_texture_get_slice_offset(s8_tex,
               level, first_layer, &x_offset[1], &y_offset[1]);
      }
   }

   if (ilo_texture_can_enable_hiz(tex, level, first_layer, num_layers)) {
      info->hiz.bo = tex->hiz.bo;
      info->hiz.stride = tex->hiz.bo_stride;
      info->hiz.tiling = INTEL_TILING_Y;

      /*
       * Layer offsetting is used on GEN6 only.  And on GEN6, HiZ is enabled
       * only when the depth buffer is non-mipmapped and non-array, making
       * layer offsetting no-op.
       */
      if (offset_to_layer) {
         assert(level == 0 && first_layer == 0 && num_layers == 1);

         info->hiz.offset = 0;
         x_offset[2] = 0;
         y_offset[2] = 0;
      }
   }

   info->width = tex->base.width0;
   info->height = tex->base.height0;
   info->depth = (tex->base.target == PIPE_TEXTURE_3D) ?
      tex->base.depth0 : num_layers;

   info->lod = level;
   info->first_layer = first_layer;
   info->num_layers = num_layers;

   if (offset_to_layer) {
      /* the size of the layer */
      info->width = u_minify(info->width, level);
      info->height = u_minify(info->height, level);
      if (info->surface_type == GEN6_SURFTYPE_3D)
         info->depth = u_minify(info->depth, level);
      else
         info->depth = 1;

      /* no layered rendering */
      assert(num_layers == 1);

      info->lod = 0;
      info->first_layer = 0;
      info->num_layers = 1;

      /* all three share the same X/Y offsets */
      if (info->zs.bo) {
         if (info->stencil.bo) {
            assert(x_offset[0] == x_offset[1]);
            assert(y_offset[0] == y_offset[1]);
         }

         info->x_offset = x_offset[0];
         info->y_offset = y_offset[0];
      }
      else {
         assert(info->stencil.bo);

         info->x_offset = x_offset[1];
         info->y_offset = y_offset[1];
      }

      if (info->hiz.bo) {
         assert(info->x_offset == x_offset[2]);
         assert(info->y_offset == y_offset[2]);
      }

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 326:
       *
       *     "The 3 LSBs of both offsets (Depth Coordinate Offset Y and Depth
       *      Coordinate Offset X) must be zero to ensure correct alignment"
       *
       * XXX Skip the check for gen6, which seems to be fine.  We need to make
       * sure that does not happen eventually.
       */
      if (dev->gen >= ILO_GEN(7)) {
         assert((info->x_offset & 7) == 0 && (info->y_offset & 7) == 0);
         info->x_offset &= ~7;
         info->y_offset &= ~7;
      }

      info->width += info->x_offset;
      info->height += info->y_offset;

      /* we have to treat them as 2D surfaces */
      if (info->surface_type == GEN6_SURFTYPE_CUBE) {
         assert(tex->base.width0 == tex->base.height0);
         /* we will set slice_offset to point to the single face */
         info->surface_type = GEN6_SURFTYPE_2D;
      }
      else if (info->surface_type == GEN6_SURFTYPE_1D && info->height > 1) {
         assert(tex->base.height0 == 1);
         info->surface_type = GEN6_SURFTYPE_2D;
      }
   }
}

void
ilo_gpe_init_zs_surface(const struct ilo_dev_info *dev,
                        const struct ilo_texture *tex,
                        enum pipe_format format, unsigned level,
                        unsigned first_layer, unsigned num_layers,
                        bool offset_to_layer, struct ilo_zs_surface *zs)
{
   const int max_2d_size = (dev->gen >= ILO_GEN(7)) ? 16384 : 8192;
   const int max_array_size = (dev->gen >= ILO_GEN(7)) ? 2048 : 512;
   struct ilo_zs_surface_info info;
   uint32_t dw1, dw2, dw3, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   if (tex) {
      zs_init_info(dev, tex, format, level, first_layer, num_layers,
            offset_to_layer, &info);
   }
   else {
      zs_init_info_null(dev, &info);
   }

   switch (info.surface_type) {
   case GEN6_SURFTYPE_NULL:
      break;
   case GEN6_SURFTYPE_1D:
      assert(info.width <= max_2d_size && info.height == 1 &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case GEN6_SURFTYPE_2D:
      assert(info.width <= max_2d_size && info.height <= max_2d_size &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case GEN6_SURFTYPE_3D:
      assert(info.width <= 2048 && info.height <= 2048 && info.depth <= 2048);
      assert(info.first_layer < 2048 && info.num_layers <= max_array_size);
      assert(info.x_offset == 0 && info.y_offset == 0);
      break;
   case GEN6_SURFTYPE_CUBE:
      assert(info.width <= max_2d_size && info.height <= max_2d_size &&
             info.depth == 1);
      assert(info.first_layer == 0 && info.num_layers == 1);
      assert(info.width == info.height);
      assert(info.x_offset == 0 && info.y_offset == 0);
      break;
   default:
      assert(!"unexpected depth surface type");
      break;
   }

   dw1 = info.surface_type << 29 |
         info.format << 18;

   if (info.zs.bo) {
      /* required for GEN6+ */
      assert(info.zs.tiling == INTEL_TILING_Y);
      assert(info.zs.stride > 0 && info.zs.stride < 128 * 1024 &&
            info.zs.stride % 128 == 0);
      assert(info.width <= info.zs.stride);

      dw1 |= (info.zs.stride - 1);
      dw2 = info.zs.offset;
   }
   else {
      dw2 = 0;
   }

   if (dev->gen >= ILO_GEN(7)) {
      if (info.zs.bo)
         dw1 |= 1 << 28;

      if (info.stencil.bo)
         dw1 |= 1 << 27;

      if (info.hiz.bo)
         dw1 |= 1 << 22;

      dw3 = (info.height - 1) << 18 |
            (info.width - 1) << 4 |
            info.lod;

      dw4 = (info.depth - 1) << 21 |
            info.first_layer << 10;

      dw5 = info.y_offset << 16 | info.x_offset;

      dw6 = (info.num_layers - 1) << 21;
   }
   else {
      /* always Y-tiled */
      dw1 |= 1 << 27 |
             1 << 26;

      if (info.hiz.bo) {
         dw1 |= 1 << 22 |
                1 << 21;
      }

      dw3 = (info.height - 1) << 19 |
            (info.width - 1) << 6 |
            info.lod << 2 |
            GEN6_DEPTH_DW3_MIPLAYOUT_BELOW;

      dw4 = (info.depth - 1) << 21 |
            info.first_layer << 10 |
            (info.num_layers - 1) << 1;

      dw5 = info.y_offset << 16 | info.x_offset;

      dw6 = 0;
   }

   STATIC_ASSERT(Elements(zs->payload) >= 10);

   zs->payload[0] = dw1;
   zs->payload[1] = dw2;
   zs->payload[2] = dw3;
   zs->payload[3] = dw4;
   zs->payload[4] = dw5;
   zs->payload[5] = dw6;

   /* do not increment reference count */
   zs->bo = info.zs.bo;

   /* separate stencil */
   if (info.stencil.bo) {
      assert(info.stencil.stride > 0 && info.stencil.stride < 128 * 1024 &&
             info.stencil.stride % 128 == 0);

      zs->payload[6] = info.stencil.stride - 1;
      zs->payload[7] = info.stencil.offset;

      if (dev->gen >= ILO_GEN(7.5))
         zs->payload[6] |= GEN75_STENCIL_DW1_STENCIL_BUFFER_ENABLE;

      /* do not increment reference count */
      zs->separate_s8_bo = info.stencil.bo;
   }
   else {
      zs->payload[6] = 0;
      zs->payload[7] = 0;
      zs->separate_s8_bo = NULL;
   }

   /* hiz */
   if (info.hiz.bo) {
      zs->payload[8] = info.hiz.stride - 1;
      zs->payload[9] = info.hiz.offset;

      /* do not increment reference count */
      zs->hiz_bo = info.hiz.bo;
   }
   else {
      zs->payload[8] = 0;
      zs->payload[9] = 0;
      zs->hiz_bo = NULL;
   }
}

static void
viewport_get_guardband(const struct ilo_dev_info *dev,
                       int center_x, int center_y,
                       int *min_gbx, int *max_gbx,
                       int *min_gby, int *max_gby)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 234:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-16K,16K-1]
    *       - Maximum Post-Clamp Delta (X or Y): 16K"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 248:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-32K,32K-1]
    *       - Maximum Post-Clamp Delta (X or Y): N/A"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * Combined, the bounding box of any object can not exceed 8K in both
    * width and height.
    *
    * Below we set the guardband as a squre of length 8K, centered at where
    * the viewport is.  This makes sure all objects passing the GB test are
    * valid to the renderer, and those failing the XY clipping have a
    * better chance of passing the GB test.
    */
   const int max_extent = (dev->gen >= ILO_GEN(7)) ? 32768 : 16384;
   const int half_len = 8192 / 2;

   /* make sure the guardband is within the valid range */
   if (center_x - half_len < -max_extent)
      center_x = -max_extent + half_len;
   else if (center_x + half_len > max_extent - 1)
      center_x = max_extent - half_len;

   if (center_y - half_len < -max_extent)
      center_y = -max_extent + half_len;
   else if (center_y + half_len > max_extent - 1)
      center_y = max_extent - half_len;

   *min_gbx = (float) (center_x - half_len);
   *max_gbx = (float) (center_x + half_len);
   *min_gby = (float) (center_y - half_len);
   *max_gby = (float) (center_y + half_len);
}

void
ilo_gpe_set_viewport_cso(const struct ilo_dev_info *dev,
                         const struct pipe_viewport_state *state,
                         struct ilo_viewport_cso *vp)
{
   const float scale_x = fabs(state->scale[0]);
   const float scale_y = fabs(state->scale[1]);
   const float scale_z = fabs(state->scale[2]);
   int min_gbx, max_gbx, min_gby, max_gby;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   viewport_get_guardband(dev,
         (int) state->translate[0],
         (int) state->translate[1],
         &min_gbx, &max_gbx, &min_gby, &max_gby);

   /* matrix form */
   vp->m00 = state->scale[0];
   vp->m11 = state->scale[1];
   vp->m22 = state->scale[2];
   vp->m30 = state->translate[0];
   vp->m31 = state->translate[1];
   vp->m32 = state->translate[2];

   /* guardband in NDC space */
   vp->min_gbx = ((float) min_gbx - state->translate[0]) / scale_x;
   vp->max_gbx = ((float) max_gbx - state->translate[0]) / scale_x;
   vp->min_gby = ((float) min_gby - state->translate[1]) / scale_y;
   vp->max_gby = ((float) max_gby - state->translate[1]) / scale_y;

   /* viewport in screen space */
   vp->min_x = scale_x * -1.0f + state->translate[0];
   vp->max_x = scale_x *  1.0f + state->translate[0];
   vp->min_y = scale_y * -1.0f + state->translate[1];
   vp->max_y = scale_y *  1.0f + state->translate[1];
   vp->min_z = scale_z * -1.0f + state->translate[2];
   vp->max_z = scale_z *  1.0f + state->translate[2];
}

static int
gen6_blend_factor_dst_alpha_forced_one(int factor)
{
   switch (factor) {
   case GEN6_BLENDFACTOR_DST_ALPHA:
      return GEN6_BLENDFACTOR_ONE;
   case GEN6_BLENDFACTOR_INV_DST_ALPHA:
   case GEN6_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return GEN6_BLENDFACTOR_ZERO;
   default:
      return factor;
   }
}

static uint32_t
blend_get_rt_blend_enable(const struct ilo_dev_info *dev,
                          const struct pipe_rt_blend_state *rt,
                          bool dst_alpha_forced_one)
{
   int rgb_src, rgb_dst, a_src, a_dst;
   uint32_t dw;

   if (!rt->blend_enable)
      return 0;

   rgb_src = gen6_translate_pipe_blendfactor(rt->rgb_src_factor);
   rgb_dst = gen6_translate_pipe_blendfactor(rt->rgb_dst_factor);
   a_src = gen6_translate_pipe_blendfactor(rt->alpha_src_factor);
   a_dst = gen6_translate_pipe_blendfactor(rt->alpha_dst_factor);

   if (dst_alpha_forced_one) {
      rgb_src = gen6_blend_factor_dst_alpha_forced_one(rgb_src);
      rgb_dst = gen6_blend_factor_dst_alpha_forced_one(rgb_dst);
      a_src = gen6_blend_factor_dst_alpha_forced_one(a_src);
      a_dst = gen6_blend_factor_dst_alpha_forced_one(a_dst);
   }

   dw = 1 << 31 |
        gen6_translate_pipe_blend(rt->alpha_func) << 26 |
        a_src << 20 |
        a_dst << 15 |
        gen6_translate_pipe_blend(rt->rgb_func) << 11 |
        rgb_src << 5 |
        rgb_dst;

   if (rt->rgb_func != rt->alpha_func ||
       rgb_src != a_src || rgb_dst != a_dst)
      dw |= 1 << 30;

   return dw;
}

void
ilo_gpe_init_blend(const struct ilo_dev_info *dev,
                   const struct pipe_blend_state *state,
                   struct ilo_blend_state *blend)
{
   unsigned num_cso, i;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   if (state->independent_blend_enable) {
      num_cso = Elements(blend->cso);
   }
   else {
      memset(blend->cso, 0, sizeof(blend->cso));
      num_cso = 1;
   }

   blend->independent_blend_enable = state->independent_blend_enable;
   blend->alpha_to_coverage = state->alpha_to_coverage;
   blend->dual_blend = false;

   for (i = 0; i < num_cso; i++) {
      const struct pipe_rt_blend_state *rt = &state->rt[i];
      struct ilo_blend_cso *cso = &blend->cso[i];
      bool dual_blend;

      cso->payload[0] = 0;
      cso->payload[1] = GEN6_BLEND_DW1_COLORCLAMP_RTFORMAT |
                            0x3;

      if (!(rt->colormask & PIPE_MASK_A))
         cso->payload[1] |= 1 << 27;
      if (!(rt->colormask & PIPE_MASK_R))
         cso->payload[1] |= 1 << 26;
      if (!(rt->colormask & PIPE_MASK_G))
         cso->payload[1] |= 1 << 25;
      if (!(rt->colormask & PIPE_MASK_B))
         cso->payload[1] |= 1 << 24;

      if (state->dither)
         cso->payload[1] |= 1 << 12;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 365:
       *
       *     "Color Buffer Blending and Logic Ops must not be enabled
       *      simultaneously, or behavior is UNDEFINED."
       *
       * Since state->logicop_enable takes precedence over rt->blend_enable,
       * no special care is needed.
       */
      if (state->logicop_enable) {
         cso->dw_logicop = 1 << 22 |
            gen6_translate_pipe_logicop(state->logicop_func) << 18;

         cso->dw_blend = 0;
         cso->dw_blend_dst_alpha_forced_one = 0;

         dual_blend = false;
      }
      else {
         cso->dw_logicop = 0;

         cso->dw_blend = blend_get_rt_blend_enable(dev, rt, false);
         cso->dw_blend_dst_alpha_forced_one =
            blend_get_rt_blend_enable(dev, rt, true);

         dual_blend = (rt->blend_enable &&
               util_blend_state_is_dual(state, i));
      }

      cso->dw_alpha_mod = 0;

      if (state->alpha_to_coverage) {
         cso->dw_alpha_mod |= 1 << 31;

         if (dev->gen >= ILO_GEN(7))
            cso->dw_alpha_mod |= 1 << 29;
      }

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 378:
       *
       *     "If Dual Source Blending is enabled, this bit (AlphaToOne Enable)
       *      must be disabled."
       */
      if (state->alpha_to_one && !dual_blend)
         cso->dw_alpha_mod |= 1 << 30;

      if (dual_blend)
         blend->dual_blend = true;
   }
}

void
ilo_gpe_init_dsa(const struct ilo_dev_info *dev,
                 const struct pipe_depth_stencil_alpha_state *state,
                 struct ilo_dsa_state *dsa)
{
   const struct pipe_depth_state *depth = &state->depth;
   const struct pipe_stencil_state *stencil0 = &state->stencil[0];
   const struct pipe_stencil_state *stencil1 = &state->stencil[1];
   const struct pipe_alpha_state *alpha = &state->alpha;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   STATIC_ASSERT(Elements(dsa->payload) >= 3);
   dw = dsa->payload;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 359:
    *
    *     "If the Depth Buffer is either undefined or does not have a surface
    *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
    *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 370:
    *
    *     "This field (Stencil Test Enable) cannot be enabled if
    *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
    *
    * TODO We do not check these yet.
    */
   if (stencil0->enabled) {
      dw[0] = 1 << 31 |
              gen6_translate_dsa_func(stencil0->func) << 28 |
              gen6_translate_pipe_stencil_op(stencil0->fail_op) << 25 |
              gen6_translate_pipe_stencil_op(stencil0->zfail_op) << 22 |
              gen6_translate_pipe_stencil_op(stencil0->zpass_op) << 19;
      if (stencil0->writemask)
         dw[0] |= 1 << 18;

      dw[1] = stencil0->valuemask << 24 |
              stencil0->writemask << 16;

      if (stencil1->enabled) {
         dw[0] |= 1 << 15 |
                  gen6_translate_dsa_func(stencil1->func) << 12 |
                  gen6_translate_pipe_stencil_op(stencil1->fail_op) << 9 |
                  gen6_translate_pipe_stencil_op(stencil1->zfail_op) << 6 |
                  gen6_translate_pipe_stencil_op(stencil1->zpass_op) << 3;
         if (stencil1->writemask)
            dw[0] |= 1 << 18;

         dw[1] |= stencil1->valuemask << 8 |
                  stencil1->writemask;
      }
   }
   else {
      dw[0] = 0;
      dw[1] = 0;
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 360:
    *
    *     "Enabling the Depth Test function without defining a Depth Buffer is
    *      UNDEFINED."
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 375:
    *
    *     "A Depth Buffer must be defined before enabling writes to it, or
    *      operation is UNDEFINED."
    *
    * TODO We do not check these yet.
    */
   dw[2] = depth->enabled << 31 |
           depth->writemask << 26;
   if (depth->enabled)
      dw[2] |= gen6_translate_dsa_func(depth->func) << 27;
   else
      dw[2] |= GEN6_COMPAREFUNCTION_ALWAYS << 27;

   /* dw_alpha will be ORed to BLEND_STATE */
   if (alpha->enabled) {
      dsa->dw_alpha = 1 << 16 |
                      gen6_translate_dsa_func(alpha->func) << 13;
   }
   else {
      dsa->dw_alpha = 0;
   }

   dsa->alpha_ref = float_to_ubyte(alpha->ref_value);
}

void
ilo_gpe_set_scissor(const struct ilo_dev_info *dev,
                    unsigned start_slot,
                    unsigned num_states,
                    const struct pipe_scissor_state *states,
                    struct ilo_scissor_state *scissor)
{
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   for (i = 0; i < num_states; i++) {
      uint16_t min_x, min_y, max_x, max_y;

      /* both max and min are inclusive in SCISSOR_RECT */
      if (states[i].minx < states[i].maxx &&
          states[i].miny < states[i].maxy) {
         min_x = states[i].minx;
         min_y = states[i].miny;
         max_x = states[i].maxx - 1;
         max_y = states[i].maxy - 1;
      }
      else {
         /* we have to make min greater than max */
         min_x = 1;
         min_y = 1;
         max_x = 0;
         max_y = 0;
      }

      scissor->payload[(start_slot + i) * 2 + 0] = min_y << 16 | min_x;
      scissor->payload[(start_slot + i) * 2 + 1] = max_y << 16 | max_x;
   }

   if (!start_slot && num_states)
      scissor->scissor0 = states[0];
}

void
ilo_gpe_set_scissor_null(const struct ilo_dev_info *dev,
                         struct ilo_scissor_state *scissor)
{
   unsigned i;

   for (i = 0; i < Elements(scissor->payload); i += 2) {
      scissor->payload[i + 0] = 1 << 16 | 1;
      scissor->payload[i + 1] = 0;
   }
}

void
ilo_gpe_init_view_surface_null_gen6(const struct ilo_dev_info *dev,
                                    unsigned width, unsigned height,
                                    unsigned depth, unsigned level,
                                    struct ilo_view_surface *surf)
{
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 71:
    *
    *     "A null surface will be used in instances where an actual surface is
    *      not bound. When a write message is generated to a null surface, no
    *      actual surface is written to. When a read message (including any
    *      sampling engine message) is generated to a null surface, the result
    *      is all zeros. Note that a null surface type is allowed to be used
    *      with all messages, even if it is not specificially indicated as
    *      supported. All of the remaining fields in surface state are ignored
    *      for null surfaces, with the following exceptions:
    *
    *        * [DevSNB+]: Width, Height, Depth, and LOD fields must match the
    *          depth buffer's corresponding state for all render target
    *          surfaces, including null.
    *        * Surface Format must be R8G8B8A8_UNORM."
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 82:
    *
    *     "If Surface Type is SURFTYPE_NULL, this field (Tiled Surface) must be
    *      true"
    */

   STATIC_ASSERT(Elements(surf->payload) >= 6);
   dw = surf->payload;

   dw[0] = GEN6_SURFTYPE_NULL << GEN6_SURFACE_DW0_TYPE__SHIFT |
           GEN6_FORMAT_B8G8R8A8_UNORM << GEN6_SURFACE_DW0_FORMAT__SHIFT;

   dw[1] = 0;

   dw[2] = (height - 1) << GEN6_SURFACE_DW2_HEIGHT__SHIFT |
           (width  - 1) << GEN6_SURFACE_DW2_WIDTH__SHIFT |
           level << GEN6_SURFACE_DW2_MIP_COUNT_LOD__SHIFT;

   dw[3] = (depth - 1) << GEN6_SURFACE_DW3_DEPTH__SHIFT |
           GEN6_TILING_X;

   dw[4] = 0;
   dw[5] = 0;

   surf->bo = NULL;
}

void
ilo_gpe_init_view_surface_for_buffer_gen6(const struct ilo_dev_info *dev,
                                          const struct ilo_buffer *buf,
                                          unsigned offset, unsigned size,
                                          unsigned struct_size,
                                          enum pipe_format elem_format,
                                          bool is_rt, bool render_cache_rw,
                                          struct ilo_view_surface *surf)
{
   const int elem_size = util_format_get_blocksize(elem_format);
   int width, height, depth, pitch;
   int surface_format, num_entries;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /*
    * For SURFTYPE_BUFFER, a SURFACE_STATE specifies an element of a
    * structure in a buffer.
    */

   surface_format = ilo_translate_color_format(elem_format);

   num_entries = size / struct_size;
   /* see if there is enough space to fit another element */
   if (size % struct_size >= elem_size)
      num_entries++;

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "For SURFTYPE_BUFFER render targets, this field (Surface Base
    *      Address) specifies the base address of first element of the
    *      surface. The surface is interpreted as a simple array of that
    *      single element type. The address must be naturally-aligned to the
    *      element size (e.g., a buffer containing R32G32B32A32_FLOAT elements
    *      must be 16-byte aligned).
    *
    *      For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    */
   if (is_rt)
      assert(offset % elem_size == 0);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 77:
    *
    *     "For buffer surfaces, the number of entries in the buffer ranges
    *      from 1 to 2^27."
    */
   assert(num_entries >= 1 && num_entries <= 1 << 27);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 81:
    *
    *     "For surfaces of type SURFTYPE_BUFFER, this field (Surface Pitch)
    *      indicates the size of the structure."
    */
   pitch = struct_size;

   pitch--;
   num_entries--;
   /* bits [6:0] */
   width  = (num_entries & 0x0000007f);
   /* bits [19:7] */
   height = (num_entries & 0x000fff80) >> 7;
   /* bits [26:20] */
   depth  = (num_entries & 0x07f00000) >> 20;

   STATIC_ASSERT(Elements(surf->payload) >= 6);
   dw = surf->payload;

   dw[0] = GEN6_SURFTYPE_BUFFER << GEN6_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN6_SURFACE_DW0_FORMAT__SHIFT;
   if (render_cache_rw)
      dw[0] |= GEN6_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = offset;

   dw[2] = height << GEN6_SURFACE_DW2_HEIGHT__SHIFT |
           width << GEN6_SURFACE_DW2_WIDTH__SHIFT;

   dw[3] = depth << GEN6_SURFACE_DW3_DEPTH__SHIFT |
           pitch << GEN6_SURFACE_DW3_PITCH__SHIFT;

   dw[4] = 0;
   dw[5] = 0;

   /* do not increment reference count */
   surf->bo = buf->bo;
}

void
ilo_gpe_init_view_surface_for_texture_gen6(const struct ilo_dev_info *dev,
                                           const struct ilo_texture *tex,
                                           enum pipe_format format,
                                           unsigned first_level,
                                           unsigned num_levels,
                                           unsigned first_layer,
                                           unsigned num_layers,
                                           bool is_rt, bool offset_to_layer,
                                           struct ilo_view_surface *surf)
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;
   unsigned layer_offset, x_offset, y_offset;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);
   assert(surface_type != GEN6_SURFTYPE_BUFFER);

   if (format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT && tex->separate_s8)
      format = PIPE_FORMAT_Z32_FLOAT;

   if (is_rt)
      surface_format = ilo_translate_render_format(format);
   else
      surface_format = ilo_translate_texture_format(format);
   assert(surface_format >= 0);

   width = tex->base.width0;
   height = tex->base.height0;
   depth = (tex->base.target == PIPE_TEXTURE_3D) ?
      tex->base.depth0 : num_layers;
   pitch = tex->bo_stride;

   if (surface_type == GEN6_SURFTYPE_CUBE) {
      /*
       * From the Sandy Bridge PRM, volume 4 part 1, page 81:
       *
       *     "For SURFTYPE_CUBE: [DevSNB+]: for Sampling Engine Surfaces, the
       *      range of this field (Depth) is [0,84], indicating the number of
       *      cube array elements (equal to the number of underlying 2D array
       *      elements divided by 6). For other surfaces, this field must be
       *      zero."
       *
       * When is_rt is true, we treat the texture as a 2D one to avoid the
       * restriction.
       */
      if (is_rt) {
         surface_type = GEN6_SURFTYPE_2D;
      }
      else {
         assert(num_layers % 6 == 0);
         depth = num_layers / 6;
      }
   }

   /* sanity check the size */
   assert(width >= 1 && height >= 1 && depth >= 1 && pitch >= 1);
   switch (surface_type) {
   case GEN6_SURFTYPE_1D:
      assert(width <= 8192 && height == 1 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case GEN6_SURFTYPE_2D:
      assert(width <= 8192 && height <= 8192 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case GEN6_SURFTYPE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      assert(first_layer < 2048 && num_layers <= 512);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case GEN6_SURFTYPE_CUBE:
      assert(width <= 8192 && height <= 8192 && depth <= 85);
      assert(width == height);
      assert(first_layer < 512 && num_layers <= 512);
      if (is_rt)
         assert(first_layer == 0);
      break;
   default:
      assert(!"unexpected surface type");
      break;
   }

   /* non-full array spacing is supported only on GEN7+ */
   assert(tex->array_spacing_full);
   /* non-interleaved samples are supported only on GEN7+ */
   if (tex->base.nr_samples > 1)
      assert(tex->interleaved);

   if (is_rt) {
      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      lod = num_levels - 1;
   }

   /*
    * Offset to the layer.  When rendering, the hardware requires LOD and
    * Depth to be the same for all render targets and the depth buffer.  We
    * need to offset to the layer manually and always set LOD and Depth to 0.
    */
   if (offset_to_layer) {
      /* we lose the capability for layered rendering */
      assert(is_rt && num_layers == 1);

      layer_offset = ilo_texture_get_slice_offset(tex,
            first_level, first_layer, &x_offset, &y_offset);

      assert(x_offset % 4 == 0);
      assert(y_offset % 2 == 0);
      x_offset /= 4;
      y_offset /= 2;

      /* derive the size for the LOD */
      width = u_minify(width, first_level);
      height = u_minify(height, first_level);

      first_level = 0;
      first_layer = 0;

      lod = 0;
      depth = 1;
   }
   else {
      layer_offset = 0;
      x_offset = 0;
      y_offset = 0;
   }

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "Linear render target surface base addresses must be element-size
    *      aligned, for non-YUV surface formats, or a multiple of 2
    *      element-sizes for YUV surface formats. Other linear surfaces have
    *      no alignment requirements (byte alignment is sufficient.)"
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 81:
    *
    *     "For linear render target surfaces, the pitch must be a multiple
    *      of the element size for non-YUV surface formats. Pitch must be a
    *      multiple of 2 * element size for YUV surface formats."
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 86:
    *
    *     "For linear surfaces, this field (X Offset) must be zero"
    */
   if (tex->tiling == INTEL_TILING_NONE) {
      if (is_rt) {
         const int elem_size = util_format_get_blocksize(format);
         assert(layer_offset % elem_size == 0);
         assert(pitch % elem_size == 0);
      }

      assert(!x_offset);
   }

   STATIC_ASSERT(Elements(surf->payload) >= 6);
   dw = surf->payload;

   dw[0] = surface_type << GEN6_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN6_SURFACE_DW0_FORMAT__SHIFT |
           GEN6_SURFACE_DW0_MIPLAYOUT_BELOW;

   if (surface_type == GEN6_SURFTYPE_CUBE && !is_rt) {
      dw[0] |= 1 << 9 |
               GEN6_SURFACE_DW0_CUBE_FACE_ENABLES__MASK;
   }

   if (is_rt)
      dw[0] |= GEN6_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = layer_offset;

   dw[2] = (height - 1) << GEN6_SURFACE_DW2_HEIGHT__SHIFT |
           (width - 1) << GEN6_SURFACE_DW2_WIDTH__SHIFT |
           lod << GEN6_SURFACE_DW2_MIP_COUNT_LOD__SHIFT;

   dw[3] = (depth - 1) << GEN6_SURFACE_DW3_DEPTH__SHIFT |
           (pitch - 1) << GEN6_SURFACE_DW3_PITCH__SHIFT |
           ilo_gpe_gen6_translate_winsys_tiling(tex->tiling);

   dw[4] = first_level << GEN6_SURFACE_DW4_MIN_LOD__SHIFT |
           first_layer << 17 |
           (num_layers - 1) << 8 |
           ((tex->base.nr_samples > 1) ? GEN6_SURFACE_DW4_MULTISAMPLECOUNT_4 :
                                         GEN6_SURFACE_DW4_MULTISAMPLECOUNT_1);

   dw[5] = x_offset << GEN6_SURFACE_DW5_X_OFFSET__SHIFT |
           y_offset << GEN6_SURFACE_DW5_Y_OFFSET__SHIFT;
   if (tex->valign_4)
      dw[5] |= GEN6_SURFACE_DW5_VALIGN_4;

   /* do not increment reference count */
   surf->bo = tex->bo;
}

static void
sampler_init_border_color_gen6(const struct ilo_dev_info *dev,
                               const union pipe_color_union *color,
                               uint32_t *dw, int num_dwords)
{
   float rgba[4] = {
      color->f[0], color->f[1], color->f[2], color->f[3],
   };

   ILO_GPE_VALID_GEN(dev, 6, 6);

   assert(num_dwords >= 12);

   /*
    * This state is not documented in the Sandy Bridge PRM, but in the
    * Ironlake PRM.  SNORM8 seems to be in DW11 instead of DW1.
    */

   /* IEEE_FP */
   dw[1] = fui(rgba[0]);
   dw[2] = fui(rgba[1]);
   dw[3] = fui(rgba[2]);
   dw[4] = fui(rgba[3]);

   /* FLOAT_16 */
   dw[5] = util_float_to_half(rgba[0]) |
           util_float_to_half(rgba[1]) << 16;
   dw[6] = util_float_to_half(rgba[2]) |
           util_float_to_half(rgba[3]) << 16;

   /* clamp to [-1.0f, 1.0f] */
   rgba[0] = CLAMP(rgba[0], -1.0f, 1.0f);
   rgba[1] = CLAMP(rgba[1], -1.0f, 1.0f);
   rgba[2] = CLAMP(rgba[2], -1.0f, 1.0f);
   rgba[3] = CLAMP(rgba[3], -1.0f, 1.0f);

   /* SNORM16 */
   dw[9] =  (int16_t) util_iround(rgba[0] * 32767.0f) |
            (int16_t) util_iround(rgba[1] * 32767.0f) << 16;
   dw[10] = (int16_t) util_iround(rgba[2] * 32767.0f) |
            (int16_t) util_iround(rgba[3] * 32767.0f) << 16;

   /* SNORM8 */
   dw[11] = (int8_t) util_iround(rgba[0] * 127.0f) |
            (int8_t) util_iround(rgba[1] * 127.0f) << 8 |
            (int8_t) util_iround(rgba[2] * 127.0f) << 16 |
            (int8_t) util_iround(rgba[3] * 127.0f) << 24;

   /* clamp to [0.0f, 1.0f] */
   rgba[0] = CLAMP(rgba[0], 0.0f, 1.0f);
   rgba[1] = CLAMP(rgba[1], 0.0f, 1.0f);
   rgba[2] = CLAMP(rgba[2], 0.0f, 1.0f);
   rgba[3] = CLAMP(rgba[3], 0.0f, 1.0f);

   /* UNORM8 */
   dw[0] = (uint8_t) util_iround(rgba[0] * 255.0f) |
           (uint8_t) util_iround(rgba[1] * 255.0f) << 8 |
           (uint8_t) util_iround(rgba[2] * 255.0f) << 16 |
           (uint8_t) util_iround(rgba[3] * 255.0f) << 24;

   /* UNORM16 */
   dw[7] = (uint16_t) util_iround(rgba[0] * 65535.0f) |
           (uint16_t) util_iround(rgba[1] * 65535.0f) << 16;
   dw[8] = (uint16_t) util_iround(rgba[2] * 65535.0f) |
           (uint16_t) util_iround(rgba[3] * 65535.0f) << 16;
}

void
ilo_gpe_init_sampler_cso(const struct ilo_dev_info *dev,
                         const struct pipe_sampler_state *state,
                         struct ilo_sampler_cso *sampler)
{
   int mip_filter, min_filter, mag_filter, max_aniso;
   int lod_bias, max_lod, min_lod;
   int wrap_s, wrap_t, wrap_r, wrap_cube;
   bool clamp_is_to_edge;
   uint32_t dw0, dw1, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   memset(sampler, 0, sizeof(*sampler));

   mip_filter = gen6_translate_tex_mipfilter(state->min_mip_filter);
   min_filter = gen6_translate_tex_filter(state->min_img_filter);
   mag_filter = gen6_translate_tex_filter(state->mag_img_filter);

   sampler->anisotropic = state->max_anisotropy;

   if (state->max_anisotropy >= 2 && state->max_anisotropy <= 16)
      max_aniso = state->max_anisotropy / 2 - 1;
   else if (state->max_anisotropy > 16)
      max_aniso = GEN6_ANISORATIO_16;
   else
      max_aniso = GEN6_ANISORATIO_2;

   /*
    *
    * Here is how the hardware calculate per-pixel LOD, from my reading of the
    * PRMs:
    *
    *  1) LOD is set to log2(ratio of texels to pixels) if not specified in
    *     other ways.  The number of texels is measured using level
    *     SurfMinLod.
    *  2) Bias is added to LOD.
    *  3) LOD is clamped to [MinLod, MaxLod], and the clamped value is
    *     compared with Base to determine whether magnification or
    *     minification is needed.  (if preclamp is disabled, LOD is compared
    *     with Base before clamping)
    *  4) If magnification is needed, or no mipmapping is requested, LOD is
    *     set to floor(MinLod).
    *  5) LOD is clamped to [0, MIPCnt], and SurfMinLod is added to LOD.
    *
    * With Gallium interface, Base is always zero and
    * pipe_sampler_view::u.tex.first_level specifies SurfMinLod.
    */
   if (dev->gen >= ILO_GEN(7)) {
      const float scale = 256.0f;

      /* [-16.0, 16.0) in S4.8 */
      lod_bias = (int)
         (CLAMP(state->lod_bias, -16.0f, 15.9f) * scale);
      lod_bias &= 0x1fff;

      /* [0.0, 14.0] in U4.8 */
      max_lod = (int) (CLAMP(state->max_lod, 0.0f, 14.0f) * scale);
      min_lod = (int) (CLAMP(state->min_lod, 0.0f, 14.0f) * scale);
   }
   else {
      const float scale = 64.0f;

      /* [-16.0, 16.0) in S4.6 */
      lod_bias = (int)
         (CLAMP(state->lod_bias, -16.0f, 15.9f) * scale);
      lod_bias &= 0x7ff;

      /* [0.0, 13.0] in U4.6 */
      max_lod = (int) (CLAMP(state->max_lod, 0.0f, 13.0f) * scale);
      min_lod = (int) (CLAMP(state->min_lod, 0.0f, 13.0f) * scale);
   }

   /*
    * We want LOD to be clamped to determine magnification/minification, and
    * get set to zero when it is magnification or when mipmapping is disabled.
    * The hardware would set LOD to floor(MinLod) and that is a problem when
    * MinLod is greater than or equal to 1.0f.
    *
    * With Base being zero, it is always minification when MinLod is non-zero.
    * To achieve our goal, we just need to set MinLod to zero and set
    * MagFilter to MinFilter when mipmapping is disabled.
    */
   if (state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE && min_lod) {
      min_lod = 0;
      mag_filter = min_filter;
   }

   /*
    * For nearest filtering, PIPE_TEX_WRAP_CLAMP means
    * PIPE_TEX_WRAP_CLAMP_TO_EDGE;  for linear filtering, PIPE_TEX_WRAP_CLAMP
    * means PIPE_TEX_WRAP_CLAMP_TO_BORDER while additionally clamping the
    * texture coordinates to [0.0, 1.0].
    *
    * The clamping will be taken care of in the shaders.  There are two
    * filters here, but let the minification one has a say.
    */
   clamp_is_to_edge = (state->min_img_filter == PIPE_TEX_FILTER_NEAREST);
   if (!clamp_is_to_edge) {
      sampler->saturate_s = (state->wrap_s == PIPE_TEX_WRAP_CLAMP);
      sampler->saturate_t = (state->wrap_t == PIPE_TEX_WRAP_CLAMP);
      sampler->saturate_r = (state->wrap_r == PIPE_TEX_WRAP_CLAMP);
   }

   /* determine wrap s/t/r */
   wrap_s = gen6_translate_tex_wrap(state->wrap_s, clamp_is_to_edge);
   wrap_t = gen6_translate_tex_wrap(state->wrap_t, clamp_is_to_edge);
   wrap_r = gen6_translate_tex_wrap(state->wrap_r, clamp_is_to_edge);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 107:
    *
    *     "When using cube map texture coordinates, only TEXCOORDMODE_CLAMP
    *      and TEXCOORDMODE_CUBE settings are valid, and each TC component
    *      must have the same Address Control mode."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 96:
    *
    *     "This field (Cube Surface Control Mode) must be set to
    *      CUBECTRLMODE_PROGRAMMED"
    *
    * Therefore, we cannot use "Cube Surface Control Mode" for semless cube
    * map filtering.
    */
   if (state->seamless_cube_map &&
       (state->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
        state->mag_img_filter != PIPE_TEX_FILTER_NEAREST)) {
      wrap_cube = GEN6_TEXCOORDMODE_CUBE;
   }
   else {
      wrap_cube = GEN6_TEXCOORDMODE_CLAMP;
   }

   if (!state->normalized_coords) {
      /*
       * From the Ivy Bridge PRM, volume 4 part 1, page 98:
       *
       *     "The following state must be set as indicated if this field
       *      (Non-normalized Coordinate Enable) is enabled:
       *
       *      - TCX/Y/Z Address Control Mode must be TEXCOORDMODE_CLAMP,
       *        TEXCOORDMODE_HALF_BORDER, or TEXCOORDMODE_CLAMP_BORDER.
       *      - Surface Type must be SURFTYPE_2D or SURFTYPE_3D.
       *      - Mag Mode Filter must be MAPFILTER_NEAREST or
       *        MAPFILTER_LINEAR.
       *      - Min Mode Filter must be MAPFILTER_NEAREST or
       *        MAPFILTER_LINEAR.
       *      - Mip Mode Filter must be MIPFILTER_NONE.
       *      - Min LOD must be 0.
       *      - Max LOD must be 0.
       *      - MIP Count must be 0.
       *      - Surface Min LOD must be 0.
       *      - Texture LOD Bias must be 0."
       */
      assert(wrap_s == GEN6_TEXCOORDMODE_CLAMP ||
             wrap_s == GEN6_TEXCOORDMODE_CLAMP_BORDER);
      assert(wrap_t == GEN6_TEXCOORDMODE_CLAMP ||
             wrap_t == GEN6_TEXCOORDMODE_CLAMP_BORDER);
      assert(wrap_r == GEN6_TEXCOORDMODE_CLAMP ||
             wrap_r == GEN6_TEXCOORDMODE_CLAMP_BORDER);

      assert(mag_filter == GEN6_MAPFILTER_NEAREST ||
             mag_filter == GEN6_MAPFILTER_LINEAR);
      assert(min_filter == GEN6_MAPFILTER_NEAREST ||
             min_filter == GEN6_MAPFILTER_LINEAR);

      /* work around a bug in util_blitter */
      mip_filter = GEN6_MIPFILTER_NONE;

      assert(mip_filter == GEN6_MIPFILTER_NONE);
   }

   if (dev->gen >= ILO_GEN(7)) {
      dw0 = 1 << 28 |
            mip_filter << 20 |
            lod_bias << 1;

      sampler->dw_filter = mag_filter << 17 |
                           min_filter << 14;

      sampler->dw_filter_aniso = GEN6_MAPFILTER_ANISOTROPIC << 17 |
                                 GEN6_MAPFILTER_ANISOTROPIC << 14 |
                                 1;

      dw1 = min_lod << 20 |
            max_lod << 8;

      if (state->compare_mode != PIPE_TEX_COMPARE_NONE)
         dw1 |= gen6_translate_shadow_func(state->compare_func) << 1;

      dw3 = max_aniso << 19;

      /* round the coordinates for linear filtering */
      if (min_filter != GEN6_MAPFILTER_NEAREST) {
         dw3 |= (GEN6_SAMPLER_DW3_U_MIN_ROUND |
                 GEN6_SAMPLER_DW3_V_MIN_ROUND |
                 GEN6_SAMPLER_DW3_R_MIN_ROUND);
      }
      if (mag_filter != GEN6_MAPFILTER_NEAREST) {
         dw3 |= (GEN6_SAMPLER_DW3_U_MAG_ROUND |
                 GEN6_SAMPLER_DW3_V_MAG_ROUND |
                 GEN6_SAMPLER_DW3_R_MAG_ROUND);
      }

      if (!state->normalized_coords)
         dw3 |= 1 << 10;

      sampler->dw_wrap = wrap_s << 6 |
                         wrap_t << 3 |
                         wrap_r;

      /*
       * As noted in the classic i965 driver, the HW may still reference
       * wrap_t and wrap_r for 1D textures.  We need to set them to a safe
       * mode
       */
      sampler->dw_wrap_1d = wrap_s << 6 |
                            GEN6_TEXCOORDMODE_WRAP << 3 |
                            GEN6_TEXCOORDMODE_WRAP;

      sampler->dw_wrap_cube = wrap_cube << 6 |
                              wrap_cube << 3 |
                              wrap_cube;

      STATIC_ASSERT(Elements(sampler->payload) >= 7);

      sampler->payload[0] = dw0;
      sampler->payload[1] = dw1;
      sampler->payload[2] = dw3;

      memcpy(&sampler->payload[3],
            state->border_color.ui, sizeof(state->border_color.ui));
   }
   else {
      dw0 = 1 << 28 |
            mip_filter << 20 |
            lod_bias << 3;

      if (state->compare_mode != PIPE_TEX_COMPARE_NONE)
         dw0 |= gen6_translate_shadow_func(state->compare_func);

      sampler->dw_filter = (min_filter != mag_filter) << 27 |
                           mag_filter << 17 |
                           min_filter << 14;

      sampler->dw_filter_aniso = GEN6_MAPFILTER_ANISOTROPIC << 17 |
                                 GEN6_MAPFILTER_ANISOTROPIC << 14;

      dw1 = min_lod << 22 |
            max_lod << 12;

      sampler->dw_wrap = wrap_s << 6 |
                         wrap_t << 3 |
                         wrap_r;

      sampler->dw_wrap_1d = wrap_s << 6 |
                            GEN6_TEXCOORDMODE_WRAP << 3 |
                            GEN6_TEXCOORDMODE_WRAP;

      sampler->dw_wrap_cube = wrap_cube << 6 |
                              wrap_cube << 3 |
                              wrap_cube;

      dw3 = max_aniso << 19;

      /* round the coordinates for linear filtering */
      if (min_filter != GEN6_MAPFILTER_NEAREST) {
         dw3 |= (GEN6_SAMPLER_DW3_U_MIN_ROUND |
                 GEN6_SAMPLER_DW3_V_MIN_ROUND |
                 GEN6_SAMPLER_DW3_R_MIN_ROUND);
      }
      if (mag_filter != GEN6_MAPFILTER_NEAREST) {
         dw3 |= (GEN6_SAMPLER_DW3_U_MAG_ROUND |
                 GEN6_SAMPLER_DW3_V_MAG_ROUND |
                 GEN6_SAMPLER_DW3_R_MAG_ROUND);
      }

      if (!state->normalized_coords)
         dw3 |= 1;

      STATIC_ASSERT(Elements(sampler->payload) >= 15);

      sampler->payload[0] = dw0;
      sampler->payload[1] = dw1;
      sampler->payload[2] = dw3;

      sampler_init_border_color_gen6(dev,
            &state->border_color, &sampler->payload[3], 12);
   }
}

void
ilo_gpe_set_fb(const struct ilo_dev_info *dev,
               const struct pipe_framebuffer_state *state,
               struct ilo_fb_state *fb)
{
   const struct pipe_surface *first;
   unsigned num_surfaces, first_idx;

   ILO_GPE_VALID_GEN(dev, 6, 7.5);

   util_copy_framebuffer_state(&fb->state, state);

   ilo_gpe_init_view_surface_null(dev,
         state->width, state->height,
         1, 0, &fb->null_rt);

   first = NULL;
   for (first_idx = 0; first_idx < state->nr_cbufs; first_idx++) {
      if (state->cbufs[first_idx]) {
         first = state->cbufs[first_idx];
         break;
      }
   }
   if (!first)
      first = state->zsbuf;

   fb->num_samples = (first) ? first->texture->nr_samples : 1;
   if (!fb->num_samples)
      fb->num_samples = 1;

   fb->offset_to_layers = false;

   /*
    * The PRMs list several restrictions when the framebuffer has more than
    * one surface, but it seems they are lifted on GEN7+.
    */
   num_surfaces = state->nr_cbufs + !!state->zsbuf;

   if (dev->gen < ILO_GEN(7) && num_surfaces > 1) {
      const unsigned first_depth =
         (first->texture->target == PIPE_TEXTURE_3D) ?
         first->texture->depth0 :
         first->u.tex.last_layer - first->u.tex.first_layer + 1;
      bool has_3d_target = (first->texture->target == PIPE_TEXTURE_3D);
      unsigned i;

      for (i = first_idx + 1; i < num_surfaces; i++) {
         const struct pipe_surface *surf =
            (i < state->nr_cbufs) ? state->cbufs[i] : state->zsbuf;
         unsigned depth;

         if (!surf)
            continue;

         depth = (surf->texture->target == PIPE_TEXTURE_3D) ?
            surf->texture->depth0 :
            surf->u.tex.last_layer - surf->u.tex.first_layer + 1;

         has_3d_target |= (surf->texture->target == PIPE_TEXTURE_3D);

         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 79:
          *
          *     "The LOD of a render target must be the same as the LOD of the
          *      other render target(s) and of the depth buffer (defined in
          *      3DSTATE_DEPTH_BUFFER)."
          *
          * From the Sandy Bridge PRM, volume 4 part 1, page 81:
          *
          *     "The Depth of a render target must be the same as the Depth of
          *      the other render target(s) and of the depth buffer (defined
          *      in 3DSTATE_DEPTH_BUFFER)."
          */
         if (surf->u.tex.level != first->u.tex.level ||
             depth != first_depth) {
            fb->offset_to_layers = true;
            break;
         }

         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 77:
          *
          *     "The Height of a render target must be the same as the Height
          *      of the other render targets and the depth buffer (defined in
          *      3DSTATE_DEPTH_BUFFER), unless Surface Type is SURFTYPE_1D or
          *      SURFTYPE_2D with Depth = 0 (non-array) and LOD = 0 (non-mip
          *      mapped)."
          *
          * From the Sandy Bridge PRM, volume 4 part 1, page 78:
          *
          *     "The Width of a render target must be the same as the Width of
          *      the other render target(s) and the depth buffer (defined in
          *      3DSTATE_DEPTH_BUFFER), unless Surface Type is SURFTYPE_1D or
          *      SURFTYPE_2D with Depth = 0 (non-array) and LOD = 0 (non-mip
          *      mapped)."
          */
         if (surf->texture->width0 != first->texture->width0 ||
             surf->texture->height0 != first->texture->height0) {
            if (has_3d_target || first->u.tex.level || first_depth > 1) {
               fb->offset_to_layers = true;
               break;
            }
         }
      }
   }
}
