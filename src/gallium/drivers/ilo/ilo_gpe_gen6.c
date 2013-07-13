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

#include "util/u_dual_blend.h"
#include "util/u_half.h"
#include "brw_defines.h"
#include "intel_reg.h"

#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_gpe_gen6.h"

/**
 * Translate winsys tiling to hardware tiling.
 */
int
ilo_gpe_gen6_translate_winsys_tiling(enum intel_tiling_mode tiling)
{
   switch (tiling) {
   case INTEL_TILING_NONE:
      return 0;
   case INTEL_TILING_X:
      return BRW_SURFACE_TILED;
   case INTEL_TILING_Y:
      return BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y;
   default:
      assert(!"unknown tiling");
      return 0;
   }
}

/**
 * Translate a pipe primitive type to the matching hardware primitive type.
 */
int
ilo_gpe_gen6_translate_pipe_prim(unsigned prim)
{
   static const int prim_mapping[PIPE_PRIM_MAX] = {
      [PIPE_PRIM_POINTS]                     = _3DPRIM_POINTLIST,
      [PIPE_PRIM_LINES]                      = _3DPRIM_LINELIST,
      [PIPE_PRIM_LINE_LOOP]                  = _3DPRIM_LINELOOP,
      [PIPE_PRIM_LINE_STRIP]                 = _3DPRIM_LINESTRIP,
      [PIPE_PRIM_TRIANGLES]                  = _3DPRIM_TRILIST,
      [PIPE_PRIM_TRIANGLE_STRIP]             = _3DPRIM_TRISTRIP,
      [PIPE_PRIM_TRIANGLE_FAN]               = _3DPRIM_TRIFAN,
      [PIPE_PRIM_QUADS]                      = _3DPRIM_QUADLIST,
      [PIPE_PRIM_QUAD_STRIP]                 = _3DPRIM_QUADSTRIP,
      [PIPE_PRIM_POLYGON]                    = _3DPRIM_POLYGON,
      [PIPE_PRIM_LINES_ADJACENCY]            = _3DPRIM_LINELIST_ADJ,
      [PIPE_PRIM_LINE_STRIP_ADJACENCY]       = _3DPRIM_LINESTRIP_ADJ,
      [PIPE_PRIM_TRIANGLES_ADJACENCY]        = _3DPRIM_TRILIST_ADJ,
      [PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]   = _3DPRIM_TRISTRIP_ADJ,
   };

   assert(prim_mapping[prim]);

   return prim_mapping[prim];
}

/**
 * Translate a pipe texture target to the matching hardware surface type.
 */
int
ilo_gpe_gen6_translate_texture(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_BUFFER:
      return BRW_SURFACE_BUFFER;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return BRW_SURFACE_1D;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
      return BRW_SURFACE_2D;
   case PIPE_TEXTURE_3D:
      return BRW_SURFACE_3D;
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return BRW_SURFACE_CUBE;
   default:
      assert(!"unknown texture target");
      return BRW_SURFACE_BUFFER;
   }
}

/**
 * Translate a depth/stencil pipe format to the matching hardware
 * format.  Return -1 on errors.
 */
static int
gen6_translate_depth_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      return BRW_DEPTHFORMAT_D16_UNORM;
   case PIPE_FORMAT_Z32_FLOAT:
      return BRW_DEPTHFORMAT_D32_FLOAT;
   case PIPE_FORMAT_Z24X8_UNORM:
      return BRW_DEPTHFORMAT_D24_UNORM_X8_UINT;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      return BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT;
   default:
      return -1;
   }
}

/**
 * Translate a pipe logicop to the matching hardware logicop.
 */
static int
gen6_translate_pipe_logicop(unsigned logicop)
{
   switch (logicop) {
   case PIPE_LOGICOP_CLEAR:         return BRW_LOGICOPFUNCTION_CLEAR;
   case PIPE_LOGICOP_NOR:           return BRW_LOGICOPFUNCTION_NOR;
   case PIPE_LOGICOP_AND_INVERTED:  return BRW_LOGICOPFUNCTION_AND_INVERTED;
   case PIPE_LOGICOP_COPY_INVERTED: return BRW_LOGICOPFUNCTION_COPY_INVERTED;
   case PIPE_LOGICOP_AND_REVERSE:   return BRW_LOGICOPFUNCTION_AND_REVERSE;
   case PIPE_LOGICOP_INVERT:        return BRW_LOGICOPFUNCTION_INVERT;
   case PIPE_LOGICOP_XOR:           return BRW_LOGICOPFUNCTION_XOR;
   case PIPE_LOGICOP_NAND:          return BRW_LOGICOPFUNCTION_NAND;
   case PIPE_LOGICOP_AND:           return BRW_LOGICOPFUNCTION_AND;
   case PIPE_LOGICOP_EQUIV:         return BRW_LOGICOPFUNCTION_EQUIV;
   case PIPE_LOGICOP_NOOP:          return BRW_LOGICOPFUNCTION_NOOP;
   case PIPE_LOGICOP_OR_INVERTED:   return BRW_LOGICOPFUNCTION_OR_INVERTED;
   case PIPE_LOGICOP_COPY:          return BRW_LOGICOPFUNCTION_COPY;
   case PIPE_LOGICOP_OR_REVERSE:    return BRW_LOGICOPFUNCTION_OR_REVERSE;
   case PIPE_LOGICOP_OR:            return BRW_LOGICOPFUNCTION_OR;
   case PIPE_LOGICOP_SET:           return BRW_LOGICOPFUNCTION_SET;
   default:
      assert(!"unknown logicop function");
      return BRW_LOGICOPFUNCTION_CLEAR;
   }
}

/**
 * Translate a pipe blend function to the matching hardware blend function.
 */
static int
gen6_translate_pipe_blend(unsigned blend)
{
   switch (blend) {
   case PIPE_BLEND_ADD:                return BRW_BLENDFUNCTION_ADD;
   case PIPE_BLEND_SUBTRACT:           return BRW_BLENDFUNCTION_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT:   return BRW_BLENDFUNCTION_REVERSE_SUBTRACT;
   case PIPE_BLEND_MIN:                return BRW_BLENDFUNCTION_MIN;
   case PIPE_BLEND_MAX:                return BRW_BLENDFUNCTION_MAX;
   default:
      assert(!"unknown blend function");
      return BRW_BLENDFUNCTION_ADD;
   };
}

/**
 * Translate a pipe blend factor to the matching hardware blend factor.
 */
static int
gen6_translate_pipe_blendfactor(unsigned blendfactor)
{
   switch (blendfactor) {
   case PIPE_BLENDFACTOR_ONE:                return BRW_BLENDFACTOR_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:          return BRW_BLENDFACTOR_SRC_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA:          return BRW_BLENDFACTOR_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA:          return BRW_BLENDFACTOR_DST_ALPHA;
   case PIPE_BLENDFACTOR_DST_COLOR:          return BRW_BLENDFACTOR_DST_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return BRW_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case PIPE_BLENDFACTOR_CONST_COLOR:        return BRW_BLENDFACTOR_CONST_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:        return BRW_BLENDFACTOR_CONST_ALPHA;
   case PIPE_BLENDFACTOR_SRC1_COLOR:         return BRW_BLENDFACTOR_SRC1_COLOR;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:         return BRW_BLENDFACTOR_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_ZERO:               return BRW_BLENDFACTOR_ZERO;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:      return BRW_BLENDFACTOR_INV_SRC_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:      return BRW_BLENDFACTOR_INV_SRC_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:      return BRW_BLENDFACTOR_INV_DST_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:      return BRW_BLENDFACTOR_INV_DST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:    return BRW_BLENDFACTOR_INV_CONST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:    return BRW_BLENDFACTOR_INV_CONST_ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:     return BRW_BLENDFACTOR_INV_SRC1_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:     return BRW_BLENDFACTOR_INV_SRC1_ALPHA;
   default:
      assert(!"unknown blend factor");
      return BRW_BLENDFACTOR_ONE;
   };
}

/**
 * Translate a pipe stencil op to the matching hardware stencil op.
 */
static int
gen6_translate_pipe_stencil_op(unsigned stencil_op)
{
   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:       return BRW_STENCILOP_KEEP;
   case PIPE_STENCIL_OP_ZERO:       return BRW_STENCILOP_ZERO;
   case PIPE_STENCIL_OP_REPLACE:    return BRW_STENCILOP_REPLACE;
   case PIPE_STENCIL_OP_INCR:       return BRW_STENCILOP_INCRSAT;
   case PIPE_STENCIL_OP_DECR:       return BRW_STENCILOP_DECRSAT;
   case PIPE_STENCIL_OP_INCR_WRAP:  return BRW_STENCILOP_INCR;
   case PIPE_STENCIL_OP_DECR_WRAP:  return BRW_STENCILOP_DECR;
   case PIPE_STENCIL_OP_INVERT:     return BRW_STENCILOP_INVERT;
   default:
      assert(!"unknown stencil op");
      return BRW_STENCILOP_KEEP;
   }
}

/**
 * Translate a pipe texture mipfilter to the matching hardware mipfilter.
 */
static int
gen6_translate_tex_mipfilter(unsigned filter)
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NEAREST: return BRW_MIPFILTER_NEAREST;
   case PIPE_TEX_MIPFILTER_LINEAR:  return BRW_MIPFILTER_LINEAR;
   case PIPE_TEX_MIPFILTER_NONE:    return BRW_MIPFILTER_NONE;
   default:
      assert(!"unknown mipfilter");
      return BRW_MIPFILTER_NONE;
   }
}

/**
 * Translate a pipe texture filter to the matching hardware mapfilter.
 */
static int
gen6_translate_tex_filter(unsigned filter)
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST: return BRW_MAPFILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:  return BRW_MAPFILTER_LINEAR;
   default:
      assert(!"unknown sampler filter");
      return BRW_MAPFILTER_NEAREST;
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
   case PIPE_TEX_WRAP_REPEAT:             return BRW_TEXCOORDMODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:      return BRW_TEXCOORDMODE_CLAMP;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:    return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:      return BRW_TEXCOORDMODE_MIRROR;
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(!"unknown sampler wrap mode");
      return BRW_TEXCOORDMODE_WRAP;
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
   case PIPE_FUNC_NEVER:      return BRW_COMPAREFUNCTION_NEVER;
   case PIPE_FUNC_LESS:       return BRW_COMPAREFUNCTION_LESS;
   case PIPE_FUNC_EQUAL:      return BRW_COMPAREFUNCTION_EQUAL;
   case PIPE_FUNC_LEQUAL:     return BRW_COMPAREFUNCTION_LEQUAL;
   case PIPE_FUNC_GREATER:    return BRW_COMPAREFUNCTION_GREATER;
   case PIPE_FUNC_NOTEQUAL:   return BRW_COMPAREFUNCTION_NOTEQUAL;
   case PIPE_FUNC_GEQUAL:     return BRW_COMPAREFUNCTION_GEQUAL;
   case PIPE_FUNC_ALWAYS:     return BRW_COMPAREFUNCTION_ALWAYS;
   default:
      assert(!"unknown depth/stencil/alpha test function");
      return BRW_COMPAREFUNCTION_NEVER;
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
    * For BRW_PREFILTER_x, the reference value is on the right-hand side of
    * the comparison, and 0.0 is returned when the comparison is true.
    */
   switch (func) {
   case PIPE_FUNC_NEVER:      return BRW_PREFILTER_ALWAYS;
   case PIPE_FUNC_LESS:       return BRW_PREFILTER_LEQUAL;
   case PIPE_FUNC_EQUAL:      return BRW_PREFILTER_NOTEQUAL;
   case PIPE_FUNC_LEQUAL:     return BRW_PREFILTER_LESS;
   case PIPE_FUNC_GREATER:    return BRW_PREFILTER_GEQUAL;
   case PIPE_FUNC_NOTEQUAL:   return BRW_PREFILTER_EQUAL;
   case PIPE_FUNC_GEQUAL:     return BRW_PREFILTER_GREATER;
   case PIPE_FUNC_ALWAYS:     return BRW_PREFILTER_NEVER;
   default:
      assert(!"unknown shadow compare function");
      return BRW_PREFILTER_NEVER;
   }
}

/**
 * Translate an index size to the matching hardware index format.
 */
static int
gen6_translate_index_size(int size)
{
   switch (size) {
   case 4: return BRW_INDEX_DWORD;
   case 2: return BRW_INDEX_WORD;
   case 1: return BRW_INDEX_BYTE;
   default:
      assert(!"unknown index size");
      return BRW_INDEX_BYTE;
   }
}

static void
gen6_emit_STATE_BASE_ADDRESS(const struct ilo_dev_info *dev,
                             struct intel_bo *general_state_bo,
                             struct intel_bo *surface_state_bo,
                             struct intel_bo *dynamic_state_bo,
                             struct intel_bo *indirect_object_bo,
                             struct intel_bo *instruction_bo,
                             uint32_t general_state_size,
                             uint32_t dynamic_state_size,
                             uint32_t indirect_object_size,
                             uint32_t instruction_size,
                             struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x0, 0x1, 0x01);
   const uint8_t cmd_len = 10;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /* 4K-page aligned */
   assert(((general_state_size | dynamic_state_size |
            indirect_object_size | instruction_size) & 0xfff) == 0);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));

   ilo_cp_write_bo(cp, 1, general_state_bo,
                       INTEL_DOMAIN_RENDER,
                       0);
   ilo_cp_write_bo(cp, 1, surface_state_bo,
                       INTEL_DOMAIN_SAMPLER,
                       0);
   ilo_cp_write_bo(cp, 1, dynamic_state_bo,
                       INTEL_DOMAIN_RENDER | INTEL_DOMAIN_INSTRUCTION,
                       0);
   ilo_cp_write_bo(cp, 1, indirect_object_bo,
                       0,
                       0);
   ilo_cp_write_bo(cp, 1, instruction_bo,
                       INTEL_DOMAIN_INSTRUCTION,
                       0);

   if (general_state_size) {
      ilo_cp_write_bo(cp, general_state_size | 1, general_state_bo,
                          INTEL_DOMAIN_RENDER,
                          0);
   }
   else {
      /* skip range check */
      ilo_cp_write(cp, 1);
   }

   if (dynamic_state_size) {
      ilo_cp_write_bo(cp, dynamic_state_size | 1, dynamic_state_bo,
                          INTEL_DOMAIN_RENDER | INTEL_DOMAIN_INSTRUCTION,
                          0);
   }
   else {
      /* skip range check */
      ilo_cp_write(cp, 0xfffff000 + 1);
   }

   if (indirect_object_size) {
      ilo_cp_write_bo(cp, indirect_object_size | 1, indirect_object_bo,
                          0,
                          0);
   }
   else {
      /* skip range check */
      ilo_cp_write(cp, 0xfffff000 + 1);
   }

   if (instruction_size) {
      ilo_cp_write_bo(cp, instruction_size | 1, instruction_bo,
                          INTEL_DOMAIN_INSTRUCTION,
                          0);
   }
   else {
      /* skip range check */
      ilo_cp_write(cp, 1);
   }

   ilo_cp_end(cp);
}

static void
gen6_emit_STATE_SIP(const struct ilo_dev_info *dev,
                    uint32_t sip,
                    struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x0, 0x1, 0x02);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len | (cmd_len - 2));
   ilo_cp_write(cp, cmd);
   ilo_cp_write(cp, sip);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_VF_STATISTICS(const struct ilo_dev_info *dev,
                                bool enable,
                                struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x1, 0x0, 0x0b);
   const uint8_t cmd_len = 1;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | enable);
   ilo_cp_end(cp);
}

static void
gen6_emit_PIPELINE_SELECT(const struct ilo_dev_info *dev,
                          int pipeline,
                          struct ilo_cp *cp)
{
   const int cmd = ILO_GPE_CMD(0x1, 0x1, 0x04);
   const uint8_t cmd_len = 1;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /* 3D or media */
   assert(pipeline == 0x0 || pipeline == 0x1);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | pipeline);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_VFE_STATE(const struct ilo_dev_info *dev,
                          int max_threads, int num_urb_entries,
                          int urb_entry_size,
                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x2, 0x0, 0x00);
   const uint8_t cmd_len = 8;
   uint32_t dw2, dw4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   dw2 = (max_threads - 1) << 16 |
         num_urb_entries << 8 |
         1 << 7 | /* Reset Gateway Timer */
         1 << 6;  /* Bypass Gateway Control */

   dw4 = urb_entry_size << 16 |  /* URB Entry Allocation Size */
         480;                    /* CURBE Allocation Size */

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0); /* scratch */
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0); /* MBZ */
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, 0); /* scoreboard */
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_CURBE_LOAD(const struct ilo_dev_info *dev,
                          uint32_t buf, int size,
                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x2, 0x0, 0x01);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   assert(buf % 32 == 0);
   /* gen6_emit_push_constant_buffer() allocates buffers in 256-bit units */
   size = align(size, 32);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0); /* MBZ */
   ilo_cp_write(cp, size);
   ilo_cp_write(cp, buf);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_INTERFACE_DESCRIPTOR_LOAD(const struct ilo_dev_info *dev,
                                          uint32_t offset, int num_ids,
                                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x2, 0x0, 0x02);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   assert(offset % 32 == 0);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0); /* MBZ */
   /* every ID has 8 DWords */
   ilo_cp_write(cp, num_ids * 8 * 4);
   ilo_cp_write(cp, offset);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_GATEWAY_STATE(const struct ilo_dev_info *dev,
                              int id, int byte, int thread_count,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x2, 0x0, 0x03);
   const uint8_t cmd_len = 2;
   uint32_t dw1;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   dw1 = id << 16 |
         byte << 8 |
         thread_count;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_STATE_FLUSH(const struct ilo_dev_info *dev,
                            int thread_count_water_mark,
                            int barrier_mask,
                            struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x2, 0x0, 0x04);
   const uint8_t cmd_len = 2;
   uint32_t dw1;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   dw1 = thread_count_water_mark << 16 |
         barrier_mask;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_end(cp);
}

static void
gen6_emit_MEDIA_OBJECT_WALKER(const struct ilo_dev_info *dev,
                              struct ilo_cp *cp)
{
   assert(!"MEDIA_OBJECT_WALKER unsupported");
}

static void
gen6_emit_3DSTATE_BINDING_TABLE_POINTERS(const struct ilo_dev_info *dev,
                                         uint32_t vs_binding_table,
                                         uint32_t gs_binding_table,
                                         uint32_t ps_binding_table,
                                         struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x01);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    GEN6_BINDING_TABLE_MODIFY_VS |
                    GEN6_BINDING_TABLE_MODIFY_GS |
                    GEN6_BINDING_TABLE_MODIFY_PS);
   ilo_cp_write(cp, vs_binding_table);
   ilo_cp_write(cp, gs_binding_table);
   ilo_cp_write(cp, ps_binding_table);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_SAMPLER_STATE_POINTERS(const struct ilo_dev_info *dev,
                                         uint32_t vs_sampler_state,
                                         uint32_t gs_sampler_state,
                                         uint32_t ps_sampler_state,
                                         struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x02);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    VS_SAMPLER_STATE_CHANGE |
                    GS_SAMPLER_STATE_CHANGE |
                    PS_SAMPLER_STATE_CHANGE);
   ilo_cp_write(cp, vs_sampler_state);
   ilo_cp_write(cp, gs_sampler_state);
   ilo_cp_write(cp, ps_sampler_state);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_URB(const struct ilo_dev_info *dev,
                      int vs_total_size, int gs_total_size,
                      int vs_entry_size, int gs_entry_size,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x05);
   const uint8_t cmd_len = 3;
   const int row_size = 128; /* 1024 bits */
   int vs_alloc_size, gs_alloc_size;
   int vs_num_entries, gs_num_entries;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /* in 1024-bit URB rows */
   vs_alloc_size = (vs_entry_size + row_size - 1) / row_size;
   gs_alloc_size = (gs_entry_size + row_size - 1) / row_size;

   /* the valid range is [1, 5] */
   if (!vs_alloc_size)
      vs_alloc_size = 1;
   if (!gs_alloc_size)
      gs_alloc_size = 1;
   assert(vs_alloc_size <= 5 && gs_alloc_size <= 5);

   /* the valid range is [24, 256] in multiples of 4 */
   vs_num_entries = (vs_total_size / row_size / vs_alloc_size) & ~3;
   if (vs_num_entries > 256)
      vs_num_entries = 256;
   assert(vs_num_entries >= 24);

   /* the valid range is [0, 256] in multiples of 4 */
   gs_num_entries = (gs_total_size / row_size / gs_alloc_size) & ~3;
   if (gs_num_entries > 256)
      gs_num_entries = 256;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, (vs_alloc_size - 1) << GEN6_URB_VS_SIZE_SHIFT |
                    vs_num_entries << GEN6_URB_VS_ENTRIES_SHIFT);
   ilo_cp_write(cp, gs_num_entries << GEN6_URB_GS_ENTRIES_SHIFT |
                    (gs_alloc_size - 1) << GEN6_URB_GS_SIZE_SHIFT);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_VERTEX_BUFFERS(const struct ilo_dev_info *dev,
                                 const struct pipe_vertex_buffer *vbuffers,
                                 uint64_t vbuffer_mask,
                                 const struct ilo_ve_state *ve,
                                 struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x08);
   uint8_t cmd_len;
   unsigned hw_idx;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 82:
    *
    *     "From 1 to 33 VBs can be specified..."
    */
   assert(vbuffer_mask <= (1UL << 33));

   if (!vbuffer_mask)
      return;

   cmd_len = 1;

   for (hw_idx = 0; hw_idx < ve->vb_count; hw_idx++) {
      const unsigned pipe_idx = ve->vb_mapping[hw_idx];

      if (vbuffer_mask & (1 << pipe_idx))
         cmd_len += 4;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));

   for (hw_idx = 0; hw_idx < ve->vb_count; hw_idx++) {
      const unsigned instance_divisor = ve->instance_divisors[hw_idx];
      const unsigned pipe_idx = ve->vb_mapping[hw_idx];
      const struct pipe_vertex_buffer *vb = &vbuffers[pipe_idx];
      uint32_t dw;

      if (!(vbuffer_mask & (1 << pipe_idx)))
         continue;

      dw = hw_idx << GEN6_VB0_INDEX_SHIFT;

      if (instance_divisor)
         dw |= GEN6_VB0_ACCESS_INSTANCEDATA;
      else
         dw |= GEN6_VB0_ACCESS_VERTEXDATA;

      if (dev->gen >= ILO_GEN(7))
         dw |= GEN7_VB0_ADDRESS_MODIFYENABLE;

      /* use null vb if there is no buffer or the stride is out of range */
      if (vb->buffer && vb->stride <= 2048) {
         const struct ilo_buffer *buf = ilo_buffer(vb->buffer);
         const uint32_t start_offset = vb->buffer_offset;
         /*
          * As noted in ilo_translate_format(), we treat some 3-component
          * formats as 4-component formats to work around hardware
          * limitations.  Imagine the case where the vertex buffer holds a
          * single PIPE_FORMAT_R16G16B16_FLOAT vertex, and buf->bo_size is 6.
          * The hardware would not be able to fetch it because the vertex
          * buffer is expected to hold a PIPE_FORMAT_R16G16B16A16_FLOAT vertex
          * and that takes at least 8 bytes.
          *
          * For the workaround to work, we query the physical size, which is
          * page aligned, to calculate end_offset so that the last vertex has
          * a better chance to be fetched.
          */
         const uint32_t end_offset = intel_bo_get_size(buf->bo) - 1;

         dw |= vb->stride << BRW_VB0_PITCH_SHIFT;

         ilo_cp_write(cp, dw);
         ilo_cp_write_bo(cp, start_offset, buf->bo, INTEL_DOMAIN_VERTEX, 0);
         ilo_cp_write_bo(cp, end_offset, buf->bo, INTEL_DOMAIN_VERTEX, 0);
         ilo_cp_write(cp, instance_divisor);
      }
      else {
         dw |= 1 << 13;

         ilo_cp_write(cp, dw);
         ilo_cp_write(cp, 0);
         ilo_cp_write(cp, 0);
         ilo_cp_write(cp, instance_divisor);
      }
   }

   ilo_cp_end(cp);
}

static void
ve_set_cso_edgeflag(const struct ilo_dev_info *dev,
                    struct ilo_ve_cso *cso)
{
   int format;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 94:
    *
    *     "- This bit (Edge Flag Enable) must only be ENABLED on the last
    *        valid VERTEX_ELEMENT structure.
    *
    *      - When set, Component 0 Control must be set to VFCOMP_STORE_SRC,
    *        and Component 1-3 Control must be set to VFCOMP_NOSTORE.
    *
    *      - The Source Element Format must be set to the UINT format.
    *
    *      - [DevSNB]: Edge Flags are not supported for QUADLIST
    *        primitives.  Software may elect to convert QUADLIST primitives
    *        to some set of corresponding edge-flag-supported primitive
    *        types (e.g., POLYGONs) prior to submission to the 3D pipeline."
    */

   cso->payload[0] |= GEN6_VE0_EDGE_FLAG_ENABLE;
   cso->payload[1] =
         BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT |
         BRW_VE1_COMPONENT_NOSTORE << BRW_VE1_COMPONENT_1_SHIFT |
         BRW_VE1_COMPONENT_NOSTORE << BRW_VE1_COMPONENT_2_SHIFT |
         BRW_VE1_COMPONENT_NOSTORE << BRW_VE1_COMPONENT_3_SHIFT;

   /*
    * Edge flags have format BRW_SURFACEFORMAT_R8_UINT when defined via
    * glEdgeFlagPointer(), and format BRW_SURFACEFORMAT_R32_FLOAT when defined
    * via glEdgeFlag(), as can be seen in vbo_attrib_tmp.h.
    *
    * Since all the hardware cares about is whether the flags are zero or not,
    * we can treat them as BRW_SURFACEFORMAT_R32_UINT in the latter case.
    */
   format = (cso->payload[0] >> BRW_VE0_FORMAT_SHIFT) & 0x1ff;
   if (format == BRW_SURFACEFORMAT_R32_FLOAT) {
      STATIC_ASSERT(BRW_SURFACEFORMAT_R32_UINT ==
            BRW_SURFACEFORMAT_R32_FLOAT - 1);

      cso->payload[0] -= (1 << BRW_VE0_FORMAT_SHIFT);
   }
   else {
      assert(format == BRW_SURFACEFORMAT_R8_UINT);
   }
}

static void
ve_init_cso_with_components(const struct ilo_dev_info *dev,
                            int comp0, int comp1, int comp2, int comp3,
                            struct ilo_ve_cso *cso)
{
   ILO_GPE_VALID_GEN(dev, 6, 7);

   STATIC_ASSERT(Elements(cso->payload) >= 2);
   cso->payload[0] = GEN6_VE0_VALID;
   cso->payload[1] =
         comp0 << BRW_VE1_COMPONENT_0_SHIFT |
         comp1 << BRW_VE1_COMPONENT_1_SHIFT |
         comp2 << BRW_VE1_COMPONENT_2_SHIFT |
         comp3 << BRW_VE1_COMPONENT_3_SHIFT;
}

static void
ve_init_cso(const struct ilo_dev_info *dev,
            const struct pipe_vertex_element *state,
            unsigned vb_index,
            struct ilo_ve_cso *cso)
{
   int comp[4] = {
      BRW_VE1_COMPONENT_STORE_SRC,
      BRW_VE1_COMPONENT_STORE_SRC,
      BRW_VE1_COMPONENT_STORE_SRC,
      BRW_VE1_COMPONENT_STORE_SRC,
   };
   int format;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   switch (util_format_get_nr_components(state->src_format)) {
   case 1: comp[1] = BRW_VE1_COMPONENT_STORE_0;
   case 2: comp[2] = BRW_VE1_COMPONENT_STORE_0;
   case 3: comp[3] = (util_format_is_pure_integer(state->src_format)) ?
                     BRW_VE1_COMPONENT_STORE_1_INT :
                     BRW_VE1_COMPONENT_STORE_1_FLT;
   }

   format = ilo_translate_vertex_format(state->src_format);

   STATIC_ASSERT(Elements(cso->payload) >= 2);
   cso->payload[0] =
      vb_index << GEN6_VE0_INDEX_SHIFT |
      GEN6_VE0_VALID |
      format << BRW_VE0_FORMAT_SHIFT |
      state->src_offset << BRW_VE0_SRC_OFFSET_SHIFT;

   cso->payload[1] =
         comp[0] << BRW_VE1_COMPONENT_0_SHIFT |
         comp[1] << BRW_VE1_COMPONENT_1_SHIFT |
         comp[2] << BRW_VE1_COMPONENT_2_SHIFT |
         comp[3] << BRW_VE1_COMPONENT_3_SHIFT;
}

void
ilo_gpe_init_ve(const struct ilo_dev_info *dev,
                unsigned num_states,
                const struct pipe_vertex_element *states,
                struct ilo_ve_state *ve)
{
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

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

static void
gen6_emit_3DSTATE_VERTEX_ELEMENTS(const struct ilo_dev_info *dev,
                                  const struct ilo_ve_state *ve,
                                  bool last_velement_edgeflag,
                                  bool prepend_generated_ids,
                                  struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x09);
   uint8_t cmd_len;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 93:
    *
    *     "Up to 34 (DevSNB+) vertex elements are supported."
    */
   assert(ve->count + prepend_generated_ids <= 34);

   if (!ve->count && !prepend_generated_ids) {
      struct ilo_ve_cso dummy;

      ve_init_cso_with_components(dev,
            BRW_VE1_COMPONENT_STORE_0,
            BRW_VE1_COMPONENT_STORE_0,
            BRW_VE1_COMPONENT_STORE_0,
            BRW_VE1_COMPONENT_STORE_1_FLT,
            &dummy);

      cmd_len = 3;
      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write_multi(cp, dummy.payload, 2);
      ilo_cp_end(cp);

      return;
   }

   cmd_len = 2 * (ve->count + prepend_generated_ids) + 1;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));

   if (prepend_generated_ids) {
      struct ilo_ve_cso gen_ids;

      ve_init_cso_with_components(dev,
            BRW_VE1_COMPONENT_STORE_VID,
            BRW_VE1_COMPONENT_STORE_IID,
            BRW_VE1_COMPONENT_NOSTORE,
            BRW_VE1_COMPONENT_NOSTORE,
            &gen_ids);

      ilo_cp_write_multi(cp, gen_ids.payload, 2);
   }

   if (last_velement_edgeflag) {
      struct ilo_ve_cso edgeflag;

      for (i = 0; i < ve->count - 1; i++)
         ilo_cp_write_multi(cp, ve->cso[i].payload, 2);

      edgeflag = ve->cso[i];
      ve_set_cso_edgeflag(dev, &edgeflag);
      ilo_cp_write_multi(cp, edgeflag.payload, 2);
   }
   else {
      for (i = 0; i < ve->count; i++)
         ilo_cp_write_multi(cp, ve->cso[i].payload, 2);
   }

   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_INDEX_BUFFER(const struct ilo_dev_info *dev,
                               const struct ilo_ib_state *ib,
                               bool enable_cut_index,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x0a);
   const uint8_t cmd_len = 3;
   struct ilo_buffer *buf = ilo_buffer(ib->hw_resource);
   uint32_t start_offset, end_offset;
   int format;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (!buf)
      return;

   format = gen6_translate_index_size(ib->hw_index_size);

   /*
    * set start_offset to 0 here and adjust pipe_draw_info::start with
    * ib->draw_start_offset in 3DPRIMITIVE
    */
   start_offset = 0;
   end_offset = buf->bo_size;

   /* end_offset must also be aligned and is inclusive */
   end_offset -= (end_offset % ib->hw_index_size);
   end_offset--;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    ((enable_cut_index) ? BRW_CUT_INDEX_ENABLE : 0) |
                    format << 8);
   ilo_cp_write_bo(cp, start_offset, buf->bo, INTEL_DOMAIN_VERTEX, 0);
   ilo_cp_write_bo(cp, end_offset, buf->bo, INTEL_DOMAIN_VERTEX, 0);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_VIEWPORT_STATE_POINTERS(const struct ilo_dev_info *dev,
                                          uint32_t clip_viewport,
                                          uint32_t sf_viewport,
                                          uint32_t cc_viewport,
                                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x0d);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    GEN6_CLIP_VIEWPORT_MODIFY |
                    GEN6_SF_VIEWPORT_MODIFY |
                    GEN6_CC_VIEWPORT_MODIFY);
   ilo_cp_write(cp, clip_viewport);
   ilo_cp_write(cp, sf_viewport);
   ilo_cp_write(cp, cc_viewport);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_CC_STATE_POINTERS(const struct ilo_dev_info *dev,
                                    uint32_t blend_state,
                                    uint32_t depth_stencil_state,
                                    uint32_t color_calc_state,
                                    struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x0e);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, blend_state | 1);
   ilo_cp_write(cp, depth_stencil_state | 1);
   ilo_cp_write(cp, color_calc_state | 1);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_SCISSOR_STATE_POINTERS(const struct ilo_dev_info *dev,
                                         uint32_t scissor_rect,
                                         struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x0f);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, scissor_rect);
   ilo_cp_end(cp);
}

void
ilo_gpe_init_vs_cso(const struct ilo_dev_info *dev,
                    const struct ilo_shader_state *vs,
                    struct ilo_shader_cso *cso)
{
   int start_grf, vue_read_len, max_threads;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 6, 7);

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
      max_threads = (dev->gt == 2) ? 280 : 70;
      break;
   default:
      max_threads = 1;
      break;
   }

   dw2 = (true) ? 0 : GEN6_VS_FLOATING_POINT_MODE_ALT;

   dw4 = start_grf << GEN6_VS_DISPATCH_START_GRF_SHIFT |
         vue_read_len << GEN6_VS_URB_READ_LENGTH_SHIFT |
         0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT;

   dw5 = GEN6_VS_STATISTICS_ENABLE |
         GEN6_VS_ENABLE;

   if (dev->gen >= ILO_GEN(7.5))
      dw5 |= (max_threads - 1) << HSW_VS_MAX_THREADS_SHIFT;
   else
      dw5 |= (max_threads - 1) << GEN6_VS_MAX_THREADS_SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 3);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
}

static void
gen6_emit_3DSTATE_VS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *vs,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x10);
   const uint8_t cmd_len = 6;
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (!vs) {
      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_end(cp);
      return;
   }

   cso = ilo_shader_get_kernel_cso(vs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= ((num_samplers + 3) / 4) << GEN6_VS_SAMPLER_COUNT_SHIFT;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, ilo_shader_get_kernel_offset(vs));
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0); /* scratch */
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, dw5);
   ilo_cp_end(cp);
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

   dw2 = GEN6_GS_SPF_MODE;

   dw4 = vue_read_len << GEN6_GS_URB_READ_LENGTH_SHIFT |
         0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT |
         start_grf << GEN6_GS_DISPATCH_START_GRF_SHIFT;

   dw5 = (max_threads - 1) << GEN6_GS_MAX_THREADS_SHIFT |
         GEN6_GS_STATISTICS_ENABLE |
         GEN6_GS_SO_STATISTICS_ENABLE |
         GEN6_GS_RENDERING_ENABLE;

   /*
    * we cannot make use of GEN6_GS_REORDER because it will reorder
    * triangle strips according to D3D rules (triangle 2N+1 uses vertices
    * (2N+1, 2N+3, 2N+2)), instead of GL rules (triangle 2N+1 uses vertices
    * (2N+2, 2N+1, 2N+3)).
    */
   dw6 = GEN6_GS_ENABLE;

   if (ilo_shader_get_kernel_param(gs, ILO_KERNEL_GS_DISCARD_ADJACENCY))
      dw6 |= GEN6_GS_DISCARD_ADJACENCY;

   if (ilo_shader_get_kernel_param(gs, ILO_KERNEL_VS_GEN6_SO)) {
      const uint32_t svbi_post_inc =
         ilo_shader_get_kernel_param(gs, ILO_KERNEL_GS_GEN6_SVBI_POST_INC);

      dw6 |= GEN6_GS_SVBI_PAYLOAD_ENABLE;
      if (svbi_post_inc) {
         dw6 |= GEN6_GS_SVBI_POSTINCREMENT_ENABLE |
                svbi_post_inc << GEN6_GS_SVBI_POSTINCREMENT_VALUE_SHIFT;
      }
   }

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = dw6;
}

static void
gen6_emit_3DSTATE_GS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *gs,
                     const struct ilo_shader_state *vs,
                     int verts_per_prim,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x11);
   const uint8_t cmd_len = 7;
   uint32_t dw1, dw2, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   if (gs) {
      const struct ilo_shader_cso *cso;

      dw1 = ilo_shader_get_kernel_offset(gs);

      cso = ilo_shader_get_kernel_cso(gs);
      dw2 = cso->payload[0];
      dw4 = cso->payload[1];
      dw5 = cso->payload[2];
      dw6 = cso->payload[3];
   }
   else if (vs && ilo_shader_get_kernel_param(vs, ILO_KERNEL_VS_GEN6_SO)) {
      struct ilo_shader_cso cso;
      enum ilo_kernel_param param;

      switch (verts_per_prim) {
      case 1:
         param = ILO_KERNEL_VS_GEN6_SO_POINT_OFFSET;
         break;
      case 2:
         param = ILO_KERNEL_VS_GEN6_SO_LINE_OFFSET;
         break;
      default:
         param = ILO_KERNEL_VS_GEN6_SO_TRI_OFFSET;
         break;
      }

      dw1 = ilo_shader_get_kernel_offset(vs) +
         ilo_shader_get_kernel_param(vs, param);

      /* cannot use VS's CSO */
      ilo_gpe_init_gs_cso_gen6(dev, vs, &cso);
      dw2 = cso.payload[0];
      dw4 = cso.payload[1];
      dw5 = cso.payload[2];
      dw6 = cso.payload[3];
   }
   else {
      dw1 = 0;
      dw2 = 0;
      dw4 = 1 << GEN6_GS_URB_READ_LENGTH_SHIFT;
      dw5 = GEN6_GS_STATISTICS_ENABLE;
      dw6 = 0;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, dw5);
   ilo_cp_write(cp, dw6);
   ilo_cp_end(cp);
}

void
ilo_gpe_init_rasterizer_clip(const struct ilo_dev_info *dev,
                             const struct pipe_rasterizer_state *state,
                             struct ilo_rasterizer_clip *clip)
{
   uint32_t dw1, dw2, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   dw1 = GEN6_CLIP_STATISTICS_ENABLE;

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
             GEN7_CLIP_EARLY_CULL;

      if (state->front_ccw)
         dw1 |= GEN7_CLIP_WINDING_CCW;

      switch (state->cull_face) {
      case PIPE_FACE_NONE:
         dw1 |= GEN7_CLIP_CULLMODE_NONE;
         break;
      case PIPE_FACE_FRONT:
         dw1 |= GEN7_CLIP_CULLMODE_FRONT;
         break;
      case PIPE_FACE_BACK:
         dw1 |= GEN7_CLIP_CULLMODE_BACK;
         break;
      case PIPE_FACE_FRONT_AND_BACK:
         dw1 |= GEN7_CLIP_CULLMODE_BOTH;
         break;
      }
   }

   dw2 = GEN6_CLIP_ENABLE |
         GEN6_CLIP_XY_TEST |
         state->clip_plane_enable << GEN6_USER_CLIP_CLIP_DISTANCES_SHIFT |
         GEN6_CLIP_MODE_NORMAL;

   if (state->clip_halfz)
      dw2 |= GEN6_CLIP_API_D3D;
   else
      dw2 |= GEN6_CLIP_API_OGL;

   if (state->depth_clip)
      dw2 |= GEN6_CLIP_Z_TEST;

   if (state->flatshade_first) {
      dw2 |= 0 << GEN6_CLIP_TRI_PROVOKE_SHIFT |
             0 << GEN6_CLIP_LINE_PROVOKE_SHIFT |
             1 << GEN6_CLIP_TRIFAN_PROVOKE_SHIFT;
   }
   else {
      dw2 |= 2 << GEN6_CLIP_TRI_PROVOKE_SHIFT |
             1 << GEN6_CLIP_LINE_PROVOKE_SHIFT |
             2 << GEN6_CLIP_TRIFAN_PROVOKE_SHIFT;
   }

   dw3 = 0x1 << GEN6_CLIP_MIN_POINT_WIDTH_SHIFT |
         0x7ff << GEN6_CLIP_MAX_POINT_WIDTH_SHIFT;

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

static void
gen6_emit_3DSTATE_CLIP(const struct ilo_dev_info *dev,
                       const struct ilo_rasterizer_state *rasterizer,
                       const struct ilo_shader_state *fs,
                       bool enable_guardband,
                       int num_viewports,
                       struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x12);
   const uint8_t cmd_len = 4;
   uint32_t dw1, dw2, dw3;

   if (rasterizer) {
      int interps;

      dw1 = rasterizer->clip.payload[0];
      dw2 = rasterizer->clip.payload[1];
      dw3 = rasterizer->clip.payload[2];

      if (enable_guardband && rasterizer->clip.can_enable_guardband)
         dw2 |= GEN6_CLIP_GB_TEST;

      interps = (fs) ?  ilo_shader_get_kernel_param(fs,
            ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS) : 0;

      if (interps & (1 << BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC |
                     1 << BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC |
                     1 << BRW_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC))
         dw2 |= GEN6_CLIP_NON_PERSPECTIVE_BARYCENTRIC_ENABLE;

      dw3 |= GEN6_CLIP_FORCE_ZERO_RTAINDEX |
             (num_viewports - 1);
   }
   else {
      dw1 = 0;
      dw2 = 0;
      dw3 = 0;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, dw3);
   ilo_cp_end(cp);
}

void
ilo_gpe_init_rasterizer_sf(const struct ilo_dev_info *dev,
                           const struct pipe_rasterizer_state *state,
                           struct ilo_rasterizer_sf *sf)
{
   float offset_const, offset_scale, offset_clamp;
   int line_width, point_width;
   uint32_t dw1, dw2, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

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
   dw1 = GEN6_SF_STATISTICS_ENABLE |
         GEN6_SF_VIEWPORT_TRANSFORM_ENABLE;

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
         dw1 |= GEN6_SF_LEGACY_GLOBAL_DEPTH_BIAS;

         dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_SOLID |
                GEN6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME |
                GEN6_SF_GLOBAL_DEPTH_OFFSET_POINT;
      }
      else {
         offset_const = 0.0f;
         offset_scale = 0.0f;
         offset_clamp = 0.0f;
      }
   }
   else {
      if (state->offset_tri)
         dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_SOLID;
      if (state->offset_line)
         dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME;
      if (state->offset_point)
         dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_POINT;
   }

   switch (state->fill_front) {
   case PIPE_POLYGON_MODE_FILL:
      dw1 |= GEN6_SF_FRONT_SOLID;
      break;
   case PIPE_POLYGON_MODE_LINE:
      dw1 |= GEN6_SF_FRONT_WIREFRAME;
      break;
   case PIPE_POLYGON_MODE_POINT:
      dw1 |= GEN6_SF_FRONT_POINT;
      break;
   }

   switch (state->fill_back) {
   case PIPE_POLYGON_MODE_FILL:
      dw1 |= GEN6_SF_BACK_SOLID;
      break;
   case PIPE_POLYGON_MODE_LINE:
      dw1 |= GEN6_SF_BACK_WIREFRAME;
      break;
   case PIPE_POLYGON_MODE_POINT:
      dw1 |= GEN6_SF_BACK_POINT;
      break;
   }

   if (state->front_ccw)
      dw1 |= GEN6_SF_WINDING_CCW;

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
      dw2 |= GEN6_SF_LINE_AA_ENABLE |
             GEN6_SF_LINE_END_CAP_WIDTH_1_0;
   }

   switch (state->cull_face) {
   case PIPE_FACE_NONE:
      dw2 |= GEN6_SF_CULL_NONE;
      break;
   case PIPE_FACE_FRONT:
      dw2 |= GEN6_SF_CULL_FRONT;
      break;
   case PIPE_FACE_BACK:
      dw2 |= GEN6_SF_CULL_BACK;
      break;
   case PIPE_FACE_FRONT_AND_BACK:
      dw2 |= GEN6_SF_CULL_BOTH;
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

   dw2 |= line_width << GEN6_SF_LINE_WIDTH_SHIFT;

   if (state->scissor)
      dw2 |= GEN6_SF_SCISSOR_ENABLE;

   dw3 = GEN6_SF_LINE_AA_MODE_TRUE |
         GEN6_SF_VERTEX_SUBPIXEL_8BITS;

   if (state->line_last_pixel)
      dw3 |= 1 << 31;

   if (state->flatshade_first) {
      dw3 |= 0 << GEN6_SF_TRI_PROVOKE_SHIFT |
             0 << GEN6_SF_LINE_PROVOKE_SHIFT |
             1 << GEN6_SF_TRIFAN_PROVOKE_SHIFT;
   }
   else {
      dw3 |= 2 << GEN6_SF_TRI_PROVOKE_SHIFT |
             1 << GEN6_SF_LINE_PROVOKE_SHIFT |
             2 << GEN6_SF_TRIFAN_PROVOKE_SHIFT;
   }

   if (!state->point_size_per_vertex)
      dw3 |= GEN6_SF_USE_STATE_POINT_WIDTH;

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
      sf->dw_msaa = GEN6_SF_MSRAST_ON_PATTERN;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 251:
       *
       *     "Software must not program a value of 0.0 when running in
       *      MSRASTMODE_ON_xxx modes - zero-width lines are not available
       *      when multisampling rasterization is enabled."
       */
      if (!line_width) {
         line_width = 128; /* 1.0f */

         sf->dw_msaa |= line_width << GEN6_SF_LINE_WIDTH_SHIFT;
      }
   }
   else {
      sf->dw_msaa = 0;
   }
}

/**
 * Fill in DW2 to DW7 of 3DSTATE_SF.
 */
void
ilo_gpe_gen6_fill_3dstate_sf_raster(const struct ilo_dev_info *dev,
                                    const struct ilo_rasterizer_state *rasterizer,
                                    int num_samples,
                                    enum pipe_format depth_format,
                                    uint32_t *payload, unsigned payload_len)
{
   const struct ilo_rasterizer_sf *sf = &rasterizer->sf;

   assert(payload_len == Elements(sf->payload));

   if (sf) {
      memcpy(payload, sf->payload, sizeof(sf->payload));

      if (num_samples > 1)
         payload[1] |= sf->dw_msaa;

      if (dev->gen >= ILO_GEN(7)) {
         int format;

         /* separate stencil */
         switch (depth_format) {
         case PIPE_FORMAT_Z24_UNORM_S8_UINT:
            depth_format = PIPE_FORMAT_Z24X8_UNORM;
            break;
         case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
            depth_format = PIPE_FORMAT_Z32_FLOAT;;
            break;
         case PIPE_FORMAT_S8_UINT:
            depth_format = PIPE_FORMAT_NONE;
            break;
         default:
            break;
         }

         format = gen6_translate_depth_format(depth_format);
         /* FLOAT surface is assumed when there is no depth buffer */
         if (format < 0)
            format = BRW_DEPTHFORMAT_D32_FLOAT;

         payload[0] |= format << GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT;
      }
   }
   else {
      payload[0] = 0;
      payload[1] = (num_samples > 1) ? GEN6_SF_MSRAST_ON_PATTERN : 0;
      payload[2] = 0;
      payload[3] = 0;
      payload[4] = 0;
      payload[5] = 0;
   }
}

/**
 * Fill in DW1 and DW8 to DW19 of 3DSTATE_SF.
 */
void
ilo_gpe_gen6_fill_3dstate_sf_sbe(const struct ilo_dev_info *dev,
                                 const struct ilo_rasterizer_state *rasterizer,
                                 const struct ilo_shader_state *fs,
                                 const struct ilo_shader_state *last_sh,
                                 uint32_t *dw, int num_dwords)
{
   int output_count, vue_offset, vue_len;
   const struct ilo_kernel_routing *routing;

   ILO_GPE_VALID_GEN(dev, 6, 7);
   assert(num_dwords == 13);

   if (!fs) {
      memset(dw, 0, sizeof(dw[0]) * num_dwords);

      if (dev->gen >= ILO_GEN(7))
         dw[0] = 1 << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT;
      else
         dw[0] = 1 << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT;

      return;
   }

   output_count = ilo_shader_get_kernel_param(fs, ILO_KERNEL_INPUT_COUNT);
   assert(output_count <= 32);

   routing = ilo_shader_get_kernel_routing(fs);

   vue_offset = routing->source_skip;
   assert(vue_offset % 2 == 0);
   vue_offset /= 2;

   vue_len = (routing->source_len + 1) / 2;
   if (!vue_len)
      vue_len = 1;

   if (dev->gen >= ILO_GEN(7)) {
      dw[0] = output_count << GEN7_SBE_NUM_OUTPUTS_SHIFT |
              vue_len << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT |
              vue_offset << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT;
      if (routing->swizzle_enable)
         dw[0] |= GEN7_SBE_SWIZZLE_ENABLE;
   }
   else {
      dw[0] = output_count << GEN6_SF_NUM_OUTPUTS_SHIFT |
              vue_len << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT |
              vue_offset << GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT;
      if (routing->swizzle_enable)
         dw[0] |= GEN6_SF_SWIZZLE_ENABLE;
   }

   switch (rasterizer->state.sprite_coord_mode) {
   case PIPE_SPRITE_COORD_UPPER_LEFT:
      dw[0] |= GEN6_SF_POINT_SPRITE_UPPERLEFT;
      break;
   case PIPE_SPRITE_COORD_LOWER_LEFT:
      dw[0] |= GEN6_SF_POINT_SPRITE_LOWERLEFT;
      break;
   }

   STATIC_ASSERT(Elements(routing->swizzles) >= 16);
   memcpy(&dw[1], routing->swizzles, 2 * 16);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 268:
    *
    *     "This field (Point Sprite Texture Coordinate Enable) must be
    *      programmed to 0 when non-point primitives are rendered."
    *
    * TODO We do not check that yet.
    */
   dw[9] = routing->point_sprite_enable;

   dw[10] = routing->const_interp_enable;

   /* WrapShortest enables */
   dw[11] = 0;
   dw[12] = 0;
}

static void
gen6_emit_3DSTATE_SF(const struct ilo_dev_info *dev,
                     const struct ilo_rasterizer_state *rasterizer,
                     const struct ilo_shader_state *fs,
                     const struct ilo_shader_state *last_sh,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x13);
   const uint8_t cmd_len = 20;
   uint32_t payload_raster[6], payload_sbe[13];

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_gpe_gen6_fill_3dstate_sf_raster(dev, rasterizer,
         1, PIPE_FORMAT_NONE, payload_raster, Elements(payload_raster));
   ilo_gpe_gen6_fill_3dstate_sf_sbe(dev, rasterizer,
         fs, last_sh, payload_sbe, Elements(payload_sbe));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, payload_sbe[0]);
   ilo_cp_write_multi(cp, payload_raster, 6);
   ilo_cp_write_multi(cp, &payload_sbe[1], 12);
   ilo_cp_end(cp);
}

void
ilo_gpe_init_rasterizer_wm_gen6(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm)
{
   uint32_t dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /* only the FF unit states are set, as in GEN7 */

   dw5 = GEN6_WM_LINE_AA_WIDTH_2_0;

   /* same value as in 3DSTATE_SF */
   if (state->line_smooth)
      dw5 |= GEN6_WM_LINE_END_CAP_AA_WIDTH_1_0;

   if (state->poly_stipple_enable)
      dw5 |= GEN6_WM_POLYGON_STIPPLE_ENABLE;
   if (state->line_stipple_enable)
      dw5 |= GEN6_WM_LINE_STIPPLE_ENABLE;

   dw6 = GEN6_WM_POSITION_ZW_PIXEL |
         GEN6_WM_MSRAST_OFF_PIXEL |
         GEN6_WM_MSDISPMODE_PERSAMPLE;

   if (state->bottom_edge_rule)
      dw6 |= GEN6_WM_POINT_RASTRULE_UPPER_RIGHT;

   /*
    * assertion that makes sure
    *
    *   dw6 |= wm->dw_msaa_rast | wm->dw_msaa_disp;
    *
    * is valid
    */
   STATIC_ASSERT(GEN6_WM_MSRAST_OFF_PIXEL == 0 &&
                 GEN6_WM_MSDISPMODE_PERSAMPLE == 0);

   wm->dw_msaa_rast =
      (state->multisample) ? GEN6_WM_MSRAST_ON_PATTERN : 0;
   wm->dw_msaa_disp = GEN6_WM_MSDISPMODE_PERPIXEL;

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

   dw2 = (true) ? 0 : GEN6_WM_FLOATING_POINT_MODE_ALT;

   dw4 = start_grf << GEN6_WM_DISPATCH_START_GRF_SHIFT_0 |
         0 << GEN6_WM_DISPATCH_START_GRF_SHIFT_1 |
         0 << GEN6_WM_DISPATCH_START_GRF_SHIFT_2;

   dw5 = (max_threads - 1) << GEN6_WM_MAX_THREADS_SHIFT;

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
      dw5 |= GEN6_WM_KILL_ENABLE;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 275:
    *
    *     "If a NULL Depth Buffer is selected, the Pixel Shader Computed Depth
    *      field must be set to disabled."
    *
    * TODO This is not checked yet.
    */
   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_OUTPUT_Z))
      dw5 |= GEN6_WM_COMPUTED_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_Z))
      dw5 |= GEN6_WM_USES_SOURCE_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_W))
      dw5 |= GEN6_WM_USES_SOURCE_W;

   /*
    * TODO set this bit only when
    *
    *  a) fs writes colors and color is not masked, or
    *  b) fs writes depth, or
    *  c) fs or cc kills
    */
   if (true)
      dw5 |= GEN6_WM_DISPATCH_ENABLE;

   assert(!ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_DISPATCH_16_OFFSET));
   dw5 |= GEN6_WM_8_DISPATCH_ENABLE;

   dw6 = input_count << GEN6_WM_NUM_SF_OUTPUTS_SHIFT |
         GEN6_WM_POSOFFSET_NONE |
         interps << GEN6_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = dw6;
}

static void
gen6_emit_3DSTATE_WM(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *fs,
                     int num_samplers,
                     const struct ilo_rasterizer_state *rasterizer,
                     bool dual_blend, bool cc_may_kill,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x14);
   const uint8_t cmd_len = 9;
   const int num_samples = 1;
   const struct ilo_shader_cso *fs_cso;
   uint32_t dw2, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   if (!fs) {
      /* see brwCreateContext() */
      const int max_threads = (dev->gt == 2) ? 80 : 40;

      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      /* honor the valid range even if dispatching is disabled */
      ilo_cp_write(cp, (max_threads - 1) << GEN6_WM_MAX_THREADS_SHIFT);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_end(cp);

      return;
   }

   fs_cso = ilo_shader_get_kernel_cso(fs);
   dw2 = fs_cso->payload[0];
   dw4 = fs_cso->payload[1];
   dw5 = fs_cso->payload[2];
   dw6 = fs_cso->payload[3];

   dw2 |= (num_samplers + 3) / 4 << GEN6_WM_SAMPLER_COUNT_SHIFT;

   if (true) {
      dw4 |= GEN6_WM_STATISTICS_ENABLE;
   }
   else {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 248:
       *
       *     "This bit (Statistics Enable) must be disabled if either of these
       *      bits is set: Depth Buffer Clear , Hierarchical Depth Buffer
       *      Resolve Enable or Depth Buffer Resolve Enable."
       */
      dw4 |= GEN6_WM_DEPTH_CLEAR;
      dw4 |= GEN6_WM_DEPTH_RESOLVE;
      dw4 |= GEN6_WM_HIERARCHICAL_DEPTH_RESOLVE;
   }

   if (cc_may_kill) {
      dw5 |= GEN6_WM_KILL_ENABLE |
             GEN6_WM_DISPATCH_ENABLE;
   }

   if (dual_blend)
      dw5 |= GEN6_WM_DUAL_SOURCE_BLEND_ENABLE;

   dw5 |= rasterizer->wm.payload[0];

   dw6 |= rasterizer->wm.payload[1];

   if (num_samples > 1) {
      dw6 |= rasterizer->wm.dw_msaa_rast |
             rasterizer->wm.dw_msaa_disp;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, ilo_shader_get_kernel_offset(fs));
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0); /* scratch */
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, dw5);
   ilo_cp_write(cp, dw6);
   ilo_cp_write(cp, 0); /* kernel 1 */
   ilo_cp_write(cp, 0); /* kernel 2 */
   ilo_cp_end(cp);
}

static unsigned
gen6_fill_3dstate_constant(const struct ilo_dev_info *dev,
                           const uint32_t *bufs, const int *sizes,
                           int num_bufs, int max_read_length,
                           uint32_t *dw, int num_dwords)
{
   unsigned enabled = 0x0;
   int total_read_length, i;

   assert(num_dwords == 4);

   total_read_length = 0;
   for (i = 0; i < 4; i++) {
      if (i < num_bufs && sizes[i]) {
         /* in 256-bit units minus one */
         const int read_len = (sizes[i] + 31) / 32 - 1;

         assert(bufs[i] % 32 == 0);
         assert(read_len < 32);

         enabled |= 1 << i;
         dw[i] = bufs[i] | read_len;

         total_read_length += read_len + 1;
      }
      else {
         dw[i] = 0;
      }
   }

   assert(total_read_length <= max_read_length);

   return enabled;
}

static void
gen6_emit_3DSTATE_CONSTANT_VS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x15);
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 138:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 32"
    */
   buf_enabled = gen6_fill_3dstate_constant(dev,
         bufs, sizes, num_bufs, 32, buf_dw, Elements(buf_dw));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) | buf_enabled << 12);
   ilo_cp_write(cp, buf_dw[0]);
   ilo_cp_write(cp, buf_dw[1]);
   ilo_cp_write(cp, buf_dw[2]);
   ilo_cp_write(cp, buf_dw[3]);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_CONSTANT_GS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x16);
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 161:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 64"
    */
   buf_enabled = gen6_fill_3dstate_constant(dev,
         bufs, sizes, num_bufs, 64, buf_dw, Elements(buf_dw));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) | buf_enabled << 12);
   ilo_cp_write(cp, buf_dw[0]);
   ilo_cp_write(cp, buf_dw[1]);
   ilo_cp_write(cp, buf_dw[2]);
   ilo_cp_write(cp, buf_dw[3]);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_CONSTANT_PS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x17);
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 287:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 64"
    */
   buf_enabled = gen6_fill_3dstate_constant(dev,
         bufs, sizes, num_bufs, 64, buf_dw, Elements(buf_dw));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) | buf_enabled << 12);
   ilo_cp_write(cp, buf_dw[0]);
   ilo_cp_write(cp, buf_dw[1]);
   ilo_cp_write(cp, buf_dw[2]);
   ilo_cp_write(cp, buf_dw[3]);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_SAMPLE_MASK(const struct ilo_dev_info *dev,
                              unsigned sample_mask,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x18);
   const uint8_t cmd_len = 2;
   const unsigned valid_mask = 0xf;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   sample_mask &= valid_mask;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, sample_mask);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_DRAWING_RECTANGLE(const struct ilo_dev_info *dev,
                                    unsigned x, unsigned y,
                                    unsigned width, unsigned height,
                                    struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x00);
   const uint8_t cmd_len = 4;
   unsigned xmax = x + width - 1;
   unsigned ymax = y + height - 1;
   int rect_limit;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (dev->gen >= ILO_GEN(7)) {
      rect_limit = 16383;
   }
   else {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 230:
       *
       *     "[DevSNB] Errata: This field (Clipped Drawing Rectangle Y Min)
       *      must be an even number"
       */
      assert(y % 2 == 0);

      rect_limit = 8191;
   }

   if (x > rect_limit) x = rect_limit;
   if (y > rect_limit) y = rect_limit;
   if (xmax > rect_limit) xmax = rect_limit;
   if (ymax > rect_limit) ymax = rect_limit;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, y << 16 | x);
   ilo_cp_write(cp, ymax << 16 | xmax);

   /*
    * There is no need to set the origin.  It is intended to support front
    * buffer rendering.
    */
   ilo_cp_write(cp, 0);

   ilo_cp_end(cp);
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
   ILO_GPE_VALID_GEN(dev, 6, 7);

   memset(info, 0, sizeof(*info));

   info->surface_type = BRW_SURFACE_NULL;
   info->format = BRW_DEPTHFORMAT_D32_FLOAT;
   info->width = 1;
   info->height = 1;
   info->depth = 1;
   info->num_layers = 1;
}

static void
zs_init_info(const struct ilo_dev_info *dev,
             const struct ilo_texture *tex,
             enum pipe_format format,
             unsigned level,
             unsigned first_layer, unsigned num_layers,
             struct ilo_zs_surface_info *info)
{
   const bool rebase_layer = true;
   struct intel_bo * const hiz_bo = NULL;
   bool separate_stencil;
   uint32_t x_offset[3], y_offset[3];

   ILO_GPE_VALID_GEN(dev, 6, 7);

   memset(info, 0, sizeof(*info));

   info->surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);

   if (info->surface_type == BRW_SURFACE_CUBE) {
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
      info->surface_type = BRW_SURFACE_2D;
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
      separate_stencil = (hiz_bo != NULL);
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
      info->format = BRW_DEPTHFORMAT_D16_UNORM;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      info->format = BRW_DEPTHFORMAT_D32_FLOAT;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      info->format = (separate_stencil) ?
         BRW_DEPTHFORMAT_D24_UNORM_X8_UINT :
         BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      info->format = (separate_stencil) ?
         BRW_DEPTHFORMAT_D32_FLOAT :
         BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT;
      break;
   case PIPE_FORMAT_S8_UINT:
      if (separate_stencil) {
         info->format = BRW_DEPTHFORMAT_D32_FLOAT;
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

      if (rebase_layer) {
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

      if (rebase_layer) {
         info->stencil.offset = ilo_texture_get_slice_offset(s8_tex,
               level, first_layer, &x_offset[1], &y_offset[1]);
      }
   }

   if (hiz_bo) {
      info->hiz.bo = hiz_bo;
      info->hiz.stride = 0;
      info->hiz.tiling = 0;
      info->hiz.offset = 0;
      x_offset[2] = 0;
      y_offset[2] = 0;
   }

   info->width = tex->base.width0;
   info->height = tex->base.height0;
   info->depth = (tex->base.target == PIPE_TEXTURE_3D) ?
      tex->base.depth0 : num_layers;

   info->lod = level;
   info->first_layer = first_layer;
   info->num_layers = num_layers;

   if (rebase_layer) {
      /* the size of the layer */
      info->width = u_minify(info->width, level);
      info->height = u_minify(info->height, level);
      if (info->surface_type == BRW_SURFACE_3D)
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
      if (info->surface_type == BRW_SURFACE_CUBE) {
         assert(tex->base.width0 == tex->base.height0);
         /* we will set slice_offset to point to the single face */
         info->surface_type = BRW_SURFACE_2D;
      }
      else if (info->surface_type == BRW_SURFACE_1D && info->height > 1) {
         assert(tex->base.height0 == 1);
         info->surface_type = BRW_SURFACE_2D;
      }
   }
}

void
ilo_gpe_init_zs_surface(const struct ilo_dev_info *dev,
                        const struct ilo_texture *tex,
                        enum pipe_format format,
                        unsigned level,
                        unsigned first_layer, unsigned num_layers,
                        struct ilo_zs_surface *zs)
{
   const int max_2d_size = (dev->gen >= ILO_GEN(7)) ? 16384 : 8192;
   const int max_array_size = (dev->gen >= ILO_GEN(7)) ? 2048 : 512;
   struct ilo_zs_surface_info info;
   uint32_t dw1, dw2, dw3, dw4, dw5, dw6;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (tex)
      zs_init_info(dev, tex, format, level, first_layer, num_layers, &info);
   else
      zs_init_info_null(dev, &info);

   switch (info.surface_type) {
   case BRW_SURFACE_NULL:
      break;
   case BRW_SURFACE_1D:
      assert(info.width <= max_2d_size && info.height == 1 &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case BRW_SURFACE_2D:
      assert(info.width <= max_2d_size && info.height <= max_2d_size &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case BRW_SURFACE_3D:
      assert(info.width <= 2048 && info.height <= 2048 && info.depth <= 2048);
      assert(info.first_layer < 2048 && info.num_layers <= max_array_size);
      assert(info.x_offset == 0 && info.y_offset == 0);
      break;
   case BRW_SURFACE_CUBE:
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
            BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1;

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
gen6_emit_3DSTATE_DEPTH_BUFFER(const struct ilo_dev_info *dev,
                               const struct ilo_zs_surface *zs,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = (dev->gen >= ILO_GEN(7)) ?
      ILO_GPE_CMD(0x3, 0x0, 0x05) : ILO_GPE_CMD(0x3, 0x1, 0x05);
   const uint8_t cmd_len = 7;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, zs->payload[0]);
   ilo_cp_write_bo(cp, zs->payload[1], zs->bo,
         INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, zs->payload[2]);
   ilo_cp_write(cp, zs->payload[3]);
   ilo_cp_write(cp, zs->payload[4]);
   ilo_cp_write(cp, zs->payload[5]);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_POLY_STIPPLE_OFFSET(const struct ilo_dev_info *dev,
                                      int x_offset, int y_offset,
                                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x06);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 6, 7);
   assert(x_offset >= 0 && x_offset <= 31);
   assert(y_offset >= 0 && y_offset <= 31);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, x_offset << 8 | y_offset);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_POLY_STIPPLE_PATTERN(const struct ilo_dev_info *dev,
                                       const struct pipe_poly_stipple *pattern,
                                       struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x07);
   const uint8_t cmd_len = 33;
   int i;

   ILO_GPE_VALID_GEN(dev, 6, 7);
   assert(Elements(pattern->stipple) == 32);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   for (i = 0; i < 32; i++)
      ilo_cp_write(cp, pattern->stipple[i]);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_LINE_STIPPLE(const struct ilo_dev_info *dev,
                               unsigned pattern, unsigned factor,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x08);
   const uint8_t cmd_len = 3;
   unsigned inverse;

   ILO_GPE_VALID_GEN(dev, 6, 7);
   assert((pattern & 0xffff) == pattern);
   assert(factor >= 1 && factor <= 256);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, pattern);

   if (dev->gen >= ILO_GEN(7)) {
      /* in U1.16 */
      inverse = (unsigned) (65536.0f / factor);
      ilo_cp_write(cp, inverse << 15 | factor);
   }
   else {
      /* in U1.13 */
      inverse = (unsigned) (8192.0f / factor);
      ilo_cp_write(cp, inverse << 16 | factor);
   }

   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_AA_LINE_PARAMETERS(const struct ilo_dev_info *dev,
                                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x0a);
   const uint8_t cmd_len = 3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0 << 16 | 0);
   ilo_cp_write(cp, 0 << 16 | 0);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_GS_SVB_INDEX(const struct ilo_dev_info *dev,
                               int index, unsigned svbi,
                               unsigned max_svbi,
                               bool load_vertex_count,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x0b);
   const uint8_t cmd_len = 4;
   uint32_t dw1;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(index >= 0 && index < 4);

   dw1 = index << SVB_INDEX_SHIFT;
   if (load_vertex_count)
      dw1 |= SVB_LOAD_INTERNAL_VERTEX_COUNT;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, svbi);
   ilo_cp_write(cp, max_svbi);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_MULTISAMPLE(const struct ilo_dev_info *dev,
                              int num_samples,
                              const uint32_t *packed_sample_pos,
                              bool pixel_location_center,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x0d);
   const uint8_t cmd_len = (dev->gen >= ILO_GEN(7)) ? 4 : 3;
   uint32_t dw1, dw2, dw3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   dw1 = (pixel_location_center) ?
      MS_PIXEL_LOCATION_CENTER : MS_PIXEL_LOCATION_UPPER_LEFT;

   switch (num_samples) {
   case 0:
   case 1:
      dw1 |= MS_NUMSAMPLES_1;
      dw2 = 0;
      dw3 = 0;
      break;
   case 4:
      dw1 |= MS_NUMSAMPLES_4;
      dw2 = packed_sample_pos[0];
      dw3 = 0;
      break;
   case 8:
      assert(dev->gen >= ILO_GEN(7));
      dw1 |= MS_NUMSAMPLES_8;
      dw2 = packed_sample_pos[0];
      dw3 = packed_sample_pos[1];
      break;
   default:
      assert(!"unsupported sample count");
      dw1 |= MS_NUMSAMPLES_1;
      dw2 = 0;
      dw3 = 0;
      break;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, dw2);
   if (dev->gen >= ILO_GEN(7))
      ilo_cp_write(cp, dw3);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_STENCIL_BUFFER(const struct ilo_dev_info *dev,
                                 const struct ilo_zs_surface *zs,
                                 struct ilo_cp *cp)
{
   const uint32_t cmd = (dev->gen >= ILO_GEN(7)) ?
      ILO_GPE_CMD(0x3, 0x0, 0x06) :
      ILO_GPE_CMD(0x3, 0x1, 0x0e);
   const uint8_t cmd_len = 3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   /* see ilo_gpe_init_zs_surface() */
   ilo_cp_write(cp, zs->payload[6]);
   ilo_cp_write_bo(cp, zs->payload[7], zs->separate_s8_bo,
         INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_HIER_DEPTH_BUFFER(const struct ilo_dev_info *dev,
                                    const struct ilo_zs_surface *zs,
                                    struct ilo_cp *cp)
{
   const uint32_t cmd = (dev->gen >= ILO_GEN(7)) ?
      ILO_GPE_CMD(0x3, 0x0, 0x07) :
      ILO_GPE_CMD(0x3, 0x1, 0x0f);
   const uint8_t cmd_len = 3;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   /* see ilo_gpe_init_zs_surface() */
   ilo_cp_write(cp, zs->payload[8]);
   ilo_cp_write_bo(cp, zs->payload[9], zs->hiz_bo,
         INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DSTATE_CLEAR_PARAMS(const struct ilo_dev_info *dev,
                               uint32_t clear_val,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x10);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    GEN5_DEPTH_CLEAR_VALID);
   ilo_cp_write(cp, clear_val);
   ilo_cp_end(cp);
}

static void
gen6_emit_PIPE_CONTROL(const struct ilo_dev_info *dev,
                       uint32_t dw1,
                       struct intel_bo *bo, uint32_t bo_offset,
                       bool write_qword,
                       struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x2, 0x00);
   const uint8_t cmd_len = (write_qword) ? 5 : 4;
   const uint32_t read_domains = INTEL_DOMAIN_INSTRUCTION;
   const uint32_t write_domain = INTEL_DOMAIN_INSTRUCTION;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (dw1 & PIPE_CONTROL_CS_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "1 of the following must also be set (when CS stall is set):
       *
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Notify Enable ([8] of DW1)"
       *
       * From the Ivy Bridge PRM, volume 2 part 1, page 61:
       *
       *     "One of the following must also be set (when CS stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)"
       */
      uint32_t bit_test = PIPE_CONTROL_WRITE_FLUSH |
                          PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                          PIPE_CONTROL_STALL_AT_SCOREBOARD |
                          PIPE_CONTROL_DEPTH_STALL;

      /* post-sync op */
      bit_test |= PIPE_CONTROL_WRITE_IMMEDIATE |
                  PIPE_CONTROL_WRITE_DEPTH_COUNT |
                  PIPE_CONTROL_WRITE_TIMESTAMP;

      if (dev->gen == ILO_GEN(6))
         bit_test |= PIPE_CONTROL_INTERRUPT_ENABLE;

      assert(dw1 & bit_test);
   }

   if (dw1 & PIPE_CONTROL_DEPTH_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "Following bits must be clear (when Depth Stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)"
       */
      assert(!(dw1 & (PIPE_CONTROL_WRITE_FLUSH |
                      PIPE_CONTROL_DEPTH_CACHE_FLUSH)));
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write_bo(cp, bo_offset, bo, read_domains, write_domain);
   ilo_cp_write(cp, 0);
   if (write_qword)
      ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen6_emit_3DPRIMITIVE(const struct ilo_dev_info *dev,
                      const struct pipe_draw_info *info,
                      const struct ilo_ib_state *ib,
                      bool rectlist,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x3, 0x00);
   const uint8_t cmd_len = 6;
   const int prim = (rectlist) ?
      _3DPRIM_RECTLIST : ilo_gpe_gen6_translate_pipe_prim(info->mode);
   const int vb_access = (info->indexed) ?
      GEN4_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM :
      GEN4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL;
   const uint32_t vb_start = info->start +
      ((info->indexed) ? ib->draw_start_offset : 0);

   ILO_GPE_VALID_GEN(dev, 6, 6);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    prim << GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT |
                    vb_access);
   ilo_cp_write(cp, info->count);
   ilo_cp_write(cp, vb_start);
   ilo_cp_write(cp, info->instance_count);
   ilo_cp_write(cp, info->start_instance);
   ilo_cp_write(cp, info->index_bias);
   ilo_cp_end(cp);
}

static uint32_t
gen6_emit_INTERFACE_DESCRIPTOR_DATA(const struct ilo_dev_info *dev,
                                    const struct ilo_shader_state **cs,
                                    uint32_t *sampler_state,
                                    int *num_samplers,
                                    uint32_t *binding_table_state,
                                    int *num_surfaces,
                                    int num_ids,
                                    struct ilo_cp *cp)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 2, page 34:
    *
    *     "(Interface Descriptor Total Length) This field must have the same
    *      alignment as the Interface Descriptor Data Start Address.
    *
    *      It must be DQWord (32-byte) aligned..."
    *
    * From the Sandy Bridge PRM, volume 2 part 2, page 35:
    *
    *     "(Interface Descriptor Data Start Address) Specifies the 32-byte
    *      aligned address of the Interface Descriptor data."
    */
   const int state_align = 32 / 4;
   const int state_len = (32 / 4) * num_ids;
   uint32_t state_offset, *dw;
   int i;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   dw = ilo_cp_steal_ptr(cp, "INTERFACE_DESCRIPTOR_DATA",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_ids; i++) {
      dw[0] = ilo_shader_get_kernel_offset(cs[i]);
      dw[1] = 1 << 18; /* SPF */
      dw[2] = sampler_state[i] |
              (num_samplers[i] + 3) / 4 << 2;
      dw[3] = binding_table_state[i] |
              num_surfaces[i];
      dw[4] = 0 << 16 |  /* CURBE Read Length */
              0;         /* CURBE Read Offset */
      dw[5] = 0; /* Barrier ID */
      dw[6] = 0;
      dw[7] = 0;

      dw += 8;
   }

   return state_offset;
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

   ILO_GPE_VALID_GEN(dev, 6, 7);

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

static uint32_t
gen6_emit_SF_VIEWPORT(const struct ilo_dev_info *dev,
                      const struct ilo_viewport_cso *viewports,
                      unsigned num_viewports,
                      struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = 8 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 262:
    *
    *     "The viewport-specific state used by the SF unit (SF_VIEWPORT) is
    *      stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   dw = ilo_cp_steal_ptr(cp, "SF_VIEWPORT",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->m00);
      dw[1] = fui(vp->m11);
      dw[2] = fui(vp->m22);
      dw[3] = fui(vp->m30);
      dw[4] = fui(vp->m31);
      dw[5] = fui(vp->m32);
      dw[6] = 0;
      dw[7] = 0;

      dw += 8;
   }

   return state_offset;
}

static uint32_t
gen6_emit_CLIP_VIEWPORT(const struct ilo_dev_info *dev,
                        const struct ilo_viewport_cso *viewports,
                        unsigned num_viewports,
                        struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = 4 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 193:
    *
    *     "The viewport-related state is stored as an array of up to 16
    *      elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   dw = ilo_cp_steal_ptr(cp, "CLIP_VIEWPORT",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->min_gbx);
      dw[1] = fui(vp->max_gbx);
      dw[2] = fui(vp->min_gby);
      dw[3] = fui(vp->max_gby);

      dw += 4;
   }

   return state_offset;
}

static uint32_t
gen6_emit_CC_VIEWPORT(const struct ilo_dev_info *dev,
                      const struct ilo_viewport_cso *viewports,
                      unsigned num_viewports,
                      struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = 2 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 385:
    *
    *     "The viewport state is stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   dw = ilo_cp_steal_ptr(cp, "CC_VIEWPORT",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->min_z);
      dw[1] = fui(vp->max_z);

      dw += 2;
   }

   return state_offset;
}

static uint32_t
gen6_emit_COLOR_CALC_STATE(const struct ilo_dev_info *dev,
                           const struct pipe_stencil_ref *stencil_ref,
                           float alpha_ref,
                           const struct pipe_blend_color *blend_color,
                           struct ilo_cp *cp)
{
   const int state_align = 64 / 4;
   const int state_len = 6;
   uint32_t state_offset, *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   dw = ilo_cp_steal_ptr(cp, "COLOR_CALC_STATE",
         state_len, state_align, &state_offset);

   dw[0] = stencil_ref->ref_value[0] << 24 |
           stencil_ref->ref_value[1] << 16 |
           BRW_ALPHATEST_FORMAT_UNORM8;
   dw[1] = float_to_ubyte(alpha_ref);
   dw[2] = fui(blend_color->color[0]);
   dw[3] = fui(blend_color->color[1]);
   dw[4] = fui(blend_color->color[2]);
   dw[5] = fui(blend_color->color[3]);

   return state_offset;
}

static int
gen6_blend_factor_dst_alpha_forced_one(int factor)
{
   switch (factor) {
   case BRW_BLENDFACTOR_DST_ALPHA:
      return BRW_BLENDFACTOR_ONE;
   case BRW_BLENDFACTOR_INV_DST_ALPHA:
   case BRW_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return BRW_BLENDFACTOR_ZERO;
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

   ILO_GPE_VALID_GEN(dev, 6, 7);

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
      cso->payload[1] = BRW_RENDERTARGET_CLAMPRANGE_FORMAT << 2 |
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

static uint32_t
gen6_emit_BLEND_STATE(const struct ilo_dev_info *dev,
                      const struct ilo_blend_state *blend,
                      const struct ilo_fb_state *fb,
                      const struct pipe_alpha_state *alpha,
                      struct ilo_cp *cp)
{
   const int state_align = 64 / 4;
   int state_len;
   uint32_t state_offset, *dw;
   unsigned num_targets, i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 376:
    *
    *     "The blend state is stored as an array of up to 8 elements..."
    */
   num_targets = fb->state.nr_cbufs;
   assert(num_targets <= 8);

   if (!num_targets) {
      if (!alpha->enabled)
         return 0;
      /* to be able to reference alpha func */
      num_targets = 1;
   }

   state_len = 2 * num_targets;

   dw = ilo_cp_steal_ptr(cp, "BLEND_STATE",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_targets; i++) {
      const unsigned idx = (blend->independent_blend_enable) ? i : 0;
      const struct ilo_blend_cso *cso = &blend->cso[idx];
      const int num_samples = fb->num_samples;
      const struct util_format_description *format_desc =
         (idx < fb->state.nr_cbufs) ?
         util_format_description(fb->state.cbufs[idx]->format) : NULL;
      bool rt_is_unorm, rt_is_pure_integer, rt_dst_alpha_forced_one;

      rt_is_unorm = true;
      rt_is_pure_integer = false;
      rt_dst_alpha_forced_one = false;

      if (format_desc) {
         int ch;

         switch (format_desc->format) {
         case PIPE_FORMAT_B8G8R8X8_UNORM:
            /* force alpha to one when the HW format has alpha */
            assert(ilo_translate_render_format(PIPE_FORMAT_B8G8R8X8_UNORM)
                  == BRW_SURFACEFORMAT_B8G8R8A8_UNORM);
            rt_dst_alpha_forced_one = true;
            break;
         default:
            break;
         }

         for (ch = 0; ch < 4; ch++) {
            if (format_desc->channel[ch].type == UTIL_FORMAT_TYPE_VOID)
               continue;

            if (format_desc->channel[ch].pure_integer) {
               rt_is_unorm = false;
               rt_is_pure_integer = true;
               break;
            }

            if (!format_desc->channel[ch].normalized ||
                format_desc->channel[ch].type != UTIL_FORMAT_TYPE_UNSIGNED)
               rt_is_unorm = false;
         }
      }

      dw[0] = cso->payload[0];
      dw[1] = cso->payload[1];

      if (!rt_is_pure_integer) {
         if (rt_dst_alpha_forced_one)
            dw[0] |= cso->dw_blend_dst_alpha_forced_one;
         else
            dw[0] |= cso->dw_blend;
      }

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 365:
       *
       *     "Logic Ops are only supported on *_UNORM surfaces (excluding
       *      _SRGB variants), otherwise Logic Ops must be DISABLED."
       *
       * Since logicop is ignored for non-UNORM color buffers, no special care
       * is needed.
       */
      if (rt_is_unorm)
         dw[1] |= cso->dw_logicop;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 356:
       *
       *     "When NumSamples = 1, AlphaToCoverage and AlphaToCoverage
       *      Dither both must be disabled."
       *
       * There is no such limitation on GEN7, or for AlphaToOne.  But GL
       * requires that anyway.
       */
      if (num_samples > 1)
         dw[1] |= cso->dw_alpha_mod;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 382:
       *
       *     "Alpha Test can only be enabled if Pixel Shader outputs a float
       *      alpha value."
       */
      if (alpha->enabled && !rt_is_pure_integer) {
         dw[1] |= 1 << 16 |
                  gen6_translate_dsa_func(alpha->func) << 13;
      }

      dw += 2;
   }

   return state_offset;
}

void
ilo_gpe_init_dsa(const struct ilo_dev_info *dev,
                 const struct pipe_depth_stencil_alpha_state *state,
                 struct ilo_dsa_state *dsa)
{
   const struct pipe_depth_state *depth = &state->depth;
   const struct pipe_stencil_state *stencil0 = &state->stencil[0];
   const struct pipe_stencil_state *stencil1 = &state->stencil[1];
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /* copy alpha state for later use */
   dsa->alpha = state->alpha;

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
      dw[2] |= BRW_COMPAREFUNCTION_ALWAYS << 27;
}

static uint32_t
gen6_emit_DEPTH_STENCIL_STATE(const struct ilo_dev_info *dev,
                              const struct ilo_dsa_state *dsa,
                              struct ilo_cp *cp)
{
   const int state_align = 64 / 4;
   const int state_len = 3;
   uint32_t state_offset, *dw;


   ILO_GPE_VALID_GEN(dev, 6, 7);

   dw = ilo_cp_steal_ptr(cp, "DEPTH_STENCIL_STATE",
         state_len, state_align, &state_offset);

   dw[0] = dsa->payload[0];
   dw[1] = dsa->payload[1];
   dw[2] = dsa->payload[2];

   return state_offset;
}

void
ilo_gpe_set_scissor(const struct ilo_dev_info *dev,
                    unsigned start_slot,
                    unsigned num_states,
                    const struct pipe_scissor_state *states,
                    struct ilo_scissor_state *scissor)
{
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

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

static uint32_t
gen6_emit_SCISSOR_RECT(const struct ilo_dev_info *dev,
                       const struct ilo_scissor_state *scissor,
                       unsigned num_viewports,
                       struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = 2 * num_viewports;
   uint32_t state_offset, *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 263:
    *
    *     "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
    *      stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   dw = ilo_cp_steal_ptr(cp, "SCISSOR_RECT",
         state_len, state_align, &state_offset);

   memcpy(dw, scissor->payload, state_len * 4);

   return state_offset;
}

static uint32_t
gen6_emit_BINDING_TABLE_STATE(const struct ilo_dev_info *dev,
                              uint32_t *surface_states,
                              int num_surface_states,
                              struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = num_surface_states;
   uint32_t state_offset, *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 69:
    *
    *     "It is stored as an array of up to 256 elements..."
    */
   assert(num_surface_states <= 256);

   if (!num_surface_states)
      return 0;

   dw = ilo_cp_steal_ptr(cp, "BINDING_TABLE_STATE",
         state_len, state_align, &state_offset);
   memcpy(dw, surface_states,
         num_surface_states * sizeof(surface_states[0]));

   return state_offset;
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

   dw[0] = BRW_SURFACE_NULL << BRW_SURFACE_TYPE_SHIFT |
           BRW_SURFACEFORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT;

   dw[1] = 0;

   dw[2] = (height - 1) << BRW_SURFACE_HEIGHT_SHIFT |
           (width  - 1) << BRW_SURFACE_WIDTH_SHIFT |
           level << BRW_SURFACE_LOD_SHIFT;

   dw[3] = (depth - 1) << BRW_SURFACE_DEPTH_SHIFT |
           BRW_SURFACE_TILED;

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

   dw[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
           surface_format << BRW_SURFACE_FORMAT_SHIFT;
   if (render_cache_rw)
      dw[0] |= BRW_SURFACE_RC_READ_WRITE;

   dw[1] = offset;

   dw[2] = height << BRW_SURFACE_HEIGHT_SHIFT |
           width << BRW_SURFACE_WIDTH_SHIFT;

   dw[3] = depth << BRW_SURFACE_DEPTH_SHIFT |
           pitch << BRW_SURFACE_PITCH_SHIFT;

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
                                           bool is_rt, bool render_cache_rw,
                                           struct ilo_view_surface *surf)
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;
   unsigned layer_offset, x_offset, y_offset;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);
   assert(surface_type != BRW_SURFACE_BUFFER);

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

   if (surface_type == BRW_SURFACE_CUBE) {
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
         surface_type = BRW_SURFACE_2D;
      }
      else {
         assert(num_layers % 6 == 0);
         depth = num_layers / 6;
      }
   }

   /* sanity check the size */
   assert(width >= 1 && height >= 1 && depth >= 1 && pitch >= 1);
   switch (surface_type) {
   case BRW_SURFACE_1D:
      assert(width <= 8192 && height == 1 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case BRW_SURFACE_2D:
      assert(width <= 8192 && height <= 8192 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case BRW_SURFACE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      assert(first_layer < 2048 && num_layers <= 512);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case BRW_SURFACE_CUBE:
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
      /*
       * Compute the offset to the layer manually.
       *
       * For rendering, the hardware requires LOD to be the same for all
       * render targets and the depth buffer.  We need to compute the offset
       * to the layer manually and always set LOD to 0.
       */
      if (true) {
         /* we lose the capability for layered rendering */
         assert(num_layers == 1);

         layer_offset = ilo_texture_get_slice_offset(tex,
               first_level, first_layer, &x_offset, &y_offset);

         assert(x_offset % 4 == 0);
         assert(y_offset % 2 == 0);
         x_offset /= 4;
         y_offset /= 2;

         /* derive the size for the LOD */
         width = u_minify(width, first_level);
         height = u_minify(height, first_level);
         if (surface_type == BRW_SURFACE_3D)
            depth = u_minify(depth, first_level);
         else
            depth = 1;

         first_level = 0;
         first_layer = 0;
         lod = 0;
      }
      else {
         layer_offset = 0;
         x_offset = 0;
         y_offset = 0;
      }

      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      layer_offset = 0;
      x_offset = 0;
      y_offset = 0;

      lod = num_levels - 1;
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

   dw[0] = surface_type << BRW_SURFACE_TYPE_SHIFT |
           surface_format << BRW_SURFACE_FORMAT_SHIFT |
           BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT;

   if (surface_type == BRW_SURFACE_CUBE && !is_rt) {
      dw[0] |= 1 << 9 |
               BRW_SURFACE_CUBEFACE_ENABLES;
   }

   if (render_cache_rw)
      dw[0] |= BRW_SURFACE_RC_READ_WRITE;

   dw[1] = layer_offset;

   dw[2] = (height - 1) << BRW_SURFACE_HEIGHT_SHIFT |
           (width - 1) << BRW_SURFACE_WIDTH_SHIFT |
           lod << BRW_SURFACE_LOD_SHIFT;

   dw[3] = (depth - 1) << BRW_SURFACE_DEPTH_SHIFT |
           (pitch - 1) << BRW_SURFACE_PITCH_SHIFT |
           ilo_gpe_gen6_translate_winsys_tiling(tex->tiling);

   dw[4] = first_level << BRW_SURFACE_MIN_LOD_SHIFT |
           first_layer << 17 |
           (num_layers - 1) << 8 |
           ((tex->base.nr_samples > 1) ? BRW_SURFACE_MULTISAMPLECOUNT_4 :
                                         BRW_SURFACE_MULTISAMPLECOUNT_1);

   dw[5] = x_offset << BRW_SURFACE_X_OFFSET_SHIFT |
           y_offset << BRW_SURFACE_Y_OFFSET_SHIFT;
   if (tex->valign_4)
      dw[5] |= BRW_SURFACE_VERTICAL_ALIGN_ENABLE;

   /* do not increment reference count */
   surf->bo = tex->bo;
}

static uint32_t
gen6_emit_SURFACE_STATE(const struct ilo_dev_info *dev,
                        const struct ilo_view_surface *surf,
                        bool for_render,
                        struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = (dev->gen >= ILO_GEN(7)) ? 8 : 6;
   uint32_t state_offset;
   uint32_t read_domains, write_domain;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   if (for_render) {
      read_domains = INTEL_DOMAIN_RENDER;
      write_domain = INTEL_DOMAIN_RENDER;
   }
   else {
      read_domains = INTEL_DOMAIN_SAMPLER;
      write_domain = 0;
   }

   ilo_cp_steal(cp, "SURFACE_STATE", state_len, state_align, &state_offset);

   STATIC_ASSERT(Elements(surf->payload) >= 8);

   ilo_cp_write(cp, surf->payload[0]);
   ilo_cp_write_bo(cp, surf->payload[1],
         surf->bo, read_domains, write_domain);
   ilo_cp_write(cp, surf->payload[2]);
   ilo_cp_write(cp, surf->payload[3]);
   ilo_cp_write(cp, surf->payload[4]);
   ilo_cp_write(cp, surf->payload[5]);

   if (dev->gen >= ILO_GEN(7)) {
      ilo_cp_write(cp, surf->payload[6]);
      ilo_cp_write(cp, surf->payload[7]);
   }

   ilo_cp_end(cp);

   return state_offset;
}

static uint32_t
gen6_emit_so_SURFACE_STATE(const struct ilo_dev_info *dev,
                           const struct pipe_stream_output_target *so,
                           const struct pipe_stream_output_info *so_info,
                           int so_index,
                           struct ilo_cp *cp)
{
   struct ilo_buffer *buf = ilo_buffer(so->buffer);
   unsigned bo_offset, struct_size;
   enum pipe_format elem_format;
   struct ilo_view_surface surf;

   ILO_GPE_VALID_GEN(dev, 6, 6);

   bo_offset = so->buffer_offset + so_info->output[so_index].dst_offset * 4;
   struct_size = so_info->stride[so_info->output[so_index].output_buffer] * 4;

   switch (so_info->output[so_index].num_components) {
   case 1:
      elem_format = PIPE_FORMAT_R32_FLOAT;
      break;
   case 2:
      elem_format = PIPE_FORMAT_R32G32_FLOAT;
      break;
   case 3:
      elem_format = PIPE_FORMAT_R32G32B32_FLOAT;
      break;
   case 4:
      elem_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      break;
   default:
      assert(!"unexpected SO components length");
      elem_format = PIPE_FORMAT_R32_FLOAT;
      break;
   }

   ilo_gpe_init_view_surface_for_buffer_gen6(dev, buf, bo_offset, so->buffer_size,
         struct_size, elem_format, false, true, &surf);

   return gen6_emit_SURFACE_STATE(dev, &surf, false, cp);
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

   ILO_GPE_VALID_GEN(dev, 6, 7);

   memset(sampler, 0, sizeof(*sampler));

   mip_filter = gen6_translate_tex_mipfilter(state->min_mip_filter);
   min_filter = gen6_translate_tex_filter(state->min_img_filter);
   mag_filter = gen6_translate_tex_filter(state->mag_img_filter);

   sampler->anisotropic = state->max_anisotropy;

   if (state->max_anisotropy >= 2 && state->max_anisotropy <= 16)
      max_aniso = state->max_anisotropy / 2 - 1;
   else if (state->max_anisotropy > 16)
      max_aniso = BRW_ANISORATIO_16;
   else
      max_aniso = BRW_ANISORATIO_2;

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
      wrap_cube = BRW_TEXCOORDMODE_CUBE;
   }
   else {
      wrap_cube = BRW_TEXCOORDMODE_CLAMP;
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
      assert(wrap_s == BRW_TEXCOORDMODE_CLAMP ||
             wrap_s == BRW_TEXCOORDMODE_CLAMP_BORDER);
      assert(wrap_t == BRW_TEXCOORDMODE_CLAMP ||
             wrap_t == BRW_TEXCOORDMODE_CLAMP_BORDER);
      assert(wrap_r == BRW_TEXCOORDMODE_CLAMP ||
             wrap_r == BRW_TEXCOORDMODE_CLAMP_BORDER);

      assert(mag_filter == BRW_MAPFILTER_NEAREST ||
             mag_filter == BRW_MAPFILTER_LINEAR);
      assert(min_filter == BRW_MAPFILTER_NEAREST ||
             min_filter == BRW_MAPFILTER_LINEAR);

      /* work around a bug in util_blitter */
      mip_filter = BRW_MIPFILTER_NONE;

      assert(mip_filter == BRW_MIPFILTER_NONE);
   }

   if (dev->gen >= ILO_GEN(7)) {
      dw0 = 1 << 28 |
            mip_filter << 20 |
            lod_bias << 1;

      sampler->dw_filter = mag_filter << 17 |
                           min_filter << 14;

      sampler->dw_filter_aniso = BRW_MAPFILTER_ANISOTROPIC << 17 |
                                 BRW_MAPFILTER_ANISOTROPIC << 14 |
                                 1;

      dw1 = min_lod << 20 |
            max_lod << 8;

      if (state->compare_mode != PIPE_TEX_COMPARE_NONE)
         dw1 |= gen6_translate_shadow_func(state->compare_func) << 1;

      dw3 = max_aniso << 19;

      /* round the coordinates for linear filtering */
      if (min_filter != BRW_MAPFILTER_NEAREST) {
         dw3 |= (BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
                 BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
                 BRW_ADDRESS_ROUNDING_ENABLE_R_MIN) << 13;
      }
      if (mag_filter != BRW_MAPFILTER_NEAREST) {
         dw3 |= (BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
                 BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
                 BRW_ADDRESS_ROUNDING_ENABLE_R_MAG) << 13;
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
                            BRW_TEXCOORDMODE_WRAP << 3 |
                            BRW_TEXCOORDMODE_WRAP;

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

      sampler->dw_filter_aniso = BRW_MAPFILTER_ANISOTROPIC << 17 |
                                 BRW_MAPFILTER_ANISOTROPIC << 14;

      dw1 = min_lod << 22 |
            max_lod << 12;

      sampler->dw_wrap = wrap_s << 6 |
                         wrap_t << 3 |
                         wrap_r;

      sampler->dw_wrap_1d = wrap_s << 6 |
                            BRW_TEXCOORDMODE_WRAP << 3 |
                            BRW_TEXCOORDMODE_WRAP;

      sampler->dw_wrap_cube = wrap_cube << 6 |
                              wrap_cube << 3 |
                              wrap_cube;

      dw3 = max_aniso << 19;

      /* round the coordinates for linear filtering */
      if (min_filter != BRW_MAPFILTER_NEAREST) {
         dw3 |= (BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
                 BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
                 BRW_ADDRESS_ROUNDING_ENABLE_R_MIN) << 13;
      }
      if (mag_filter != BRW_MAPFILTER_NEAREST) {
         dw3 |= (BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
                 BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
                 BRW_ADDRESS_ROUNDING_ENABLE_R_MAG) << 13;
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

static uint32_t
gen6_emit_SAMPLER_STATE(const struct ilo_dev_info *dev,
                        const struct ilo_sampler_cso * const *samplers,
                        const struct pipe_sampler_view * const *views,
                        const uint32_t *sampler_border_colors,
                        int num_samplers,
                        struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = 4 * num_samplers;
   uint32_t state_offset, *dw;
   int i;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 101:
    *
    *     "The sampler state is stored as an array of up to 16 elements..."
    */
   assert(num_samplers <= 16);

   if (!num_samplers)
      return 0;

   dw = ilo_cp_steal_ptr(cp, "SAMPLER_STATE",
         state_len, state_align, &state_offset);

   for (i = 0; i < num_samplers; i++) {
      const struct ilo_sampler_cso *sampler = samplers[i];
      const struct pipe_sampler_view *view = views[i];
      const uint32_t border_color = sampler_border_colors[i];
      uint32_t dw_filter, dw_wrap;

      /* there may be holes */
      if (!sampler || !view) {
         /* disabled sampler */
         dw[0] = 1 << 31;
         dw[1] = 0;
         dw[2] = 0;
         dw[3] = 0;
         dw += 4;

         continue;
      }

      /* determine filter and wrap modes */
      switch (view->texture->target) {
      case PIPE_TEXTURE_1D:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap_1d;
         break;
      case PIPE_TEXTURE_3D:
         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 103:
          *
          *     "Only MAPFILTER_NEAREST and MAPFILTER_LINEAR are supported for
          *      surfaces of type SURFTYPE_3D."
          */
         dw_filter = sampler->dw_filter;
         dw_wrap = sampler->dw_wrap;
         break;
      case PIPE_TEXTURE_CUBE:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap_cube;
         break;
      default:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap;
         break;
      }

      dw[0] = sampler->payload[0];
      dw[1] = sampler->payload[1];
      assert(!(border_color & 0x1f));
      dw[2] = border_color;
      dw[3] = sampler->payload[2];

      dw[0] |= dw_filter;

      if (dev->gen >= ILO_GEN(7)) {
         dw[3] |= dw_wrap;
      }
      else {
         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 21:
          *
          *     "[DevSNB] Errata: Incorrect behavior is observed in cases
          *      where the min and mag mode filters are different and
          *      SurfMinLOD is nonzero. The determination of MagMode uses the
          *      following equation instead of the one in the above
          *      pseudocode: MagMode = (LOD + SurfMinLOD - Base <= 0)"
          *
          * As a way to work around that, we set Base to
          * view->u.tex.first_level.
          */
         dw[0] |= view->u.tex.first_level << 22;

         dw[1] |= dw_wrap;
      }

      dw += 4;
   }

   return state_offset;
}

static uint32_t
gen6_emit_SAMPLER_BORDER_COLOR_STATE(const struct ilo_dev_info *dev,
                                     const struct ilo_sampler_cso *sampler,
                                     struct ilo_cp *cp)
{
   const int state_align = 32 / 4;
   const int state_len = (dev->gen >= ILO_GEN(7)) ? 4 : 12;
   uint32_t state_offset, *dw;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   dw = ilo_cp_steal_ptr(cp, "SAMPLER_BORDER_COLOR_STATE",
         state_len, state_align, &state_offset);

   /* see ilo_gpe_init_sampler_cso() */
   memcpy(dw, &sampler->payload[3], state_len * 4);

   return state_offset;
}

static uint32_t
gen6_emit_push_constant_buffer(const struct ilo_dev_info *dev,
                               int size, void **pcb,
                               struct ilo_cp *cp)
{
   /*
    * For all VS, GS, FS, and CS push constant buffers, they must be aligned
    * to 32 bytes, and their sizes are specified in 256-bit units.
    */
   const int state_align = 32 / 4;
   const int state_len = align(size, 32) / 4;
   uint32_t state_offset;
   char *buf;

   ILO_GPE_VALID_GEN(dev, 6, 7);

   buf = ilo_cp_steal_ptr(cp, "PUSH_CONSTANT_BUFFER",
         state_len, state_align, &state_offset);

   /* zero out the unused range */
   if (size < state_len * 4)
      memset(&buf[size], 0, state_len * 4 - size);

   if (pcb)
      *pcb = buf;

   return state_offset;
}

static int
gen6_estimate_command_size(const struct ilo_dev_info *dev,
                           enum ilo_gpe_gen6_command cmd,
                           int arg)
{
   static const struct {
      int header;
      int body;
   } gen6_command_size_table[ILO_GPE_GEN6_COMMAND_COUNT] = {
      [ILO_GPE_GEN6_STATE_BASE_ADDRESS]                       = { 0,  10 },
      [ILO_GPE_GEN6_STATE_SIP]                                = { 0,  2  },
      [ILO_GPE_GEN6_3DSTATE_VF_STATISTICS]                    = { 0,  1  },
      [ILO_GPE_GEN6_PIPELINE_SELECT]                          = { 0,  1  },
      [ILO_GPE_GEN6_MEDIA_VFE_STATE]                          = { 0,  8  },
      [ILO_GPE_GEN6_MEDIA_CURBE_LOAD]                         = { 0,  4  },
      [ILO_GPE_GEN6_MEDIA_INTERFACE_DESCRIPTOR_LOAD]          = { 0,  4  },
      [ILO_GPE_GEN6_MEDIA_GATEWAY_STATE]                      = { 0,  2  },
      [ILO_GPE_GEN6_MEDIA_STATE_FLUSH]                        = { 0,  2  },
      [ILO_GPE_GEN6_MEDIA_OBJECT_WALKER]                      = { 17, 1  },
      [ILO_GPE_GEN6_3DSTATE_BINDING_TABLE_POINTERS]           = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_SAMPLER_STATE_POINTERS]           = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_URB]                              = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_VERTEX_BUFFERS]                   = { 1,  4  },
      [ILO_GPE_GEN6_3DSTATE_VERTEX_ELEMENTS]                  = { 1,  2  },
      [ILO_GPE_GEN6_3DSTATE_INDEX_BUFFER]                     = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_VIEWPORT_STATE_POINTERS]          = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_CC_STATE_POINTERS]                = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_SCISSOR_STATE_POINTERS]           = { 0,  2  },
      [ILO_GPE_GEN6_3DSTATE_VS]                               = { 0,  6  },
      [ILO_GPE_GEN6_3DSTATE_GS]                               = { 0,  7  },
      [ILO_GPE_GEN6_3DSTATE_CLIP]                             = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_SF]                               = { 0,  20 },
      [ILO_GPE_GEN6_3DSTATE_WM]                               = { 0,  9  },
      [ILO_GPE_GEN6_3DSTATE_CONSTANT_VS]                      = { 0,  5  },
      [ILO_GPE_GEN6_3DSTATE_CONSTANT_GS]                      = { 0,  5  },
      [ILO_GPE_GEN6_3DSTATE_CONSTANT_PS]                      = { 0,  5  },
      [ILO_GPE_GEN6_3DSTATE_SAMPLE_MASK]                      = { 0,  2  },
      [ILO_GPE_GEN6_3DSTATE_DRAWING_RECTANGLE]                = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_DEPTH_BUFFER]                     = { 0,  7  },
      [ILO_GPE_GEN6_3DSTATE_POLY_STIPPLE_OFFSET]              = { 0,  2  },
      [ILO_GPE_GEN6_3DSTATE_POLY_STIPPLE_PATTERN]             = { 0,  33 },
      [ILO_GPE_GEN6_3DSTATE_LINE_STIPPLE]                     = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_AA_LINE_PARAMETERS]               = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_GS_SVB_INDEX]                     = { 0,  4  },
      [ILO_GPE_GEN6_3DSTATE_MULTISAMPLE]                      = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_STENCIL_BUFFER]                   = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_HIER_DEPTH_BUFFER]                = { 0,  3  },
      [ILO_GPE_GEN6_3DSTATE_CLEAR_PARAMS]                     = { 0,  2  },
      [ILO_GPE_GEN6_PIPE_CONTROL]                             = { 0,  5  },
      [ILO_GPE_GEN6_3DPRIMITIVE]                              = { 0,  6  },
   };
   const int header = gen6_command_size_table[cmd].header;
   const int body = gen6_command_size_table[arg].body;
   const int count = arg;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(cmd < ILO_GPE_GEN6_COMMAND_COUNT);

   return (likely(count)) ? header + body * count : 0;
}

static int
gen6_estimate_state_size(const struct ilo_dev_info *dev,
                         enum ilo_gpe_gen6_state state,
                         int arg)
{
   static const struct {
      int alignment;
      int body;
      bool is_array;
   } gen6_state_size_table[ILO_GPE_GEN6_STATE_COUNT] = {
      [ILO_GPE_GEN6_INTERFACE_DESCRIPTOR_DATA]          = { 8,  8,  true },
      [ILO_GPE_GEN6_SF_VIEWPORT]                        = { 8,  8,  true },
      [ILO_GPE_GEN6_CLIP_VIEWPORT]                      = { 8,  4,  true },
      [ILO_GPE_GEN6_CC_VIEWPORT]                        = { 8,  2,  true },
      [ILO_GPE_GEN6_COLOR_CALC_STATE]                   = { 16, 6,  false },
      [ILO_GPE_GEN6_BLEND_STATE]                        = { 16, 2,  true },
      [ILO_GPE_GEN6_DEPTH_STENCIL_STATE]                = { 16, 3,  false },
      [ILO_GPE_GEN6_SCISSOR_RECT]                       = { 8,  2,  true },
      [ILO_GPE_GEN6_BINDING_TABLE_STATE]                = { 8,  1,  true },
      [ILO_GPE_GEN6_SURFACE_STATE]                      = { 8,  6,  false },
      [ILO_GPE_GEN6_SAMPLER_STATE]                      = { 8,  4,  true },
      [ILO_GPE_GEN6_SAMPLER_BORDER_COLOR_STATE]         = { 8,  12, false },
      [ILO_GPE_GEN6_PUSH_CONSTANT_BUFFER]               = { 8,  1,  true },
   };
   const int alignment = gen6_state_size_table[state].alignment;
   const int body = gen6_state_size_table[state].body;
   const bool is_array = gen6_state_size_table[state].is_array;
   const int count = arg;
   int estimate;

   ILO_GPE_VALID_GEN(dev, 6, 6);
   assert(state < ILO_GPE_GEN6_STATE_COUNT);

   if (likely(count)) {
      if (is_array) {
         estimate = (alignment - 1) + body * count;
      }
      else {
         estimate = (alignment - 1) + body;
         /* all states are aligned */
         if (count > 1)
            estimate += util_align_npot(body, alignment) * (count - 1);
      }
   }
   else {
      estimate = 0;
   }

   return estimate;
}

static const struct ilo_gpe_gen6 gen6_gpe = {
   .estimate_command_size = gen6_estimate_command_size,
   .estimate_state_size = gen6_estimate_state_size,

#define GEN6_SET(name) .emit_ ## name = gen6_emit_ ## name
   GEN6_SET(STATE_BASE_ADDRESS),
   GEN6_SET(STATE_SIP),
   GEN6_SET(3DSTATE_VF_STATISTICS),
   GEN6_SET(PIPELINE_SELECT),
   GEN6_SET(MEDIA_VFE_STATE),
   GEN6_SET(MEDIA_CURBE_LOAD),
   GEN6_SET(MEDIA_INTERFACE_DESCRIPTOR_LOAD),
   GEN6_SET(MEDIA_GATEWAY_STATE),
   GEN6_SET(MEDIA_STATE_FLUSH),
   GEN6_SET(MEDIA_OBJECT_WALKER),
   GEN6_SET(3DSTATE_BINDING_TABLE_POINTERS),
   GEN6_SET(3DSTATE_SAMPLER_STATE_POINTERS),
   GEN6_SET(3DSTATE_URB),
   GEN6_SET(3DSTATE_VERTEX_BUFFERS),
   GEN6_SET(3DSTATE_VERTEX_ELEMENTS),
   GEN6_SET(3DSTATE_INDEX_BUFFER),
   GEN6_SET(3DSTATE_VIEWPORT_STATE_POINTERS),
   GEN6_SET(3DSTATE_CC_STATE_POINTERS),
   GEN6_SET(3DSTATE_SCISSOR_STATE_POINTERS),
   GEN6_SET(3DSTATE_VS),
   GEN6_SET(3DSTATE_GS),
   GEN6_SET(3DSTATE_CLIP),
   GEN6_SET(3DSTATE_SF),
   GEN6_SET(3DSTATE_WM),
   GEN6_SET(3DSTATE_CONSTANT_VS),
   GEN6_SET(3DSTATE_CONSTANT_GS),
   GEN6_SET(3DSTATE_CONSTANT_PS),
   GEN6_SET(3DSTATE_SAMPLE_MASK),
   GEN6_SET(3DSTATE_DRAWING_RECTANGLE),
   GEN6_SET(3DSTATE_DEPTH_BUFFER),
   GEN6_SET(3DSTATE_POLY_STIPPLE_OFFSET),
   GEN6_SET(3DSTATE_POLY_STIPPLE_PATTERN),
   GEN6_SET(3DSTATE_LINE_STIPPLE),
   GEN6_SET(3DSTATE_AA_LINE_PARAMETERS),
   GEN6_SET(3DSTATE_GS_SVB_INDEX),
   GEN6_SET(3DSTATE_MULTISAMPLE),
   GEN6_SET(3DSTATE_STENCIL_BUFFER),
   GEN6_SET(3DSTATE_HIER_DEPTH_BUFFER),
   GEN6_SET(3DSTATE_CLEAR_PARAMS),
   GEN6_SET(PIPE_CONTROL),
   GEN6_SET(3DPRIMITIVE),
   GEN6_SET(INTERFACE_DESCRIPTOR_DATA),
   GEN6_SET(SF_VIEWPORT),
   GEN6_SET(CLIP_VIEWPORT),
   GEN6_SET(CC_VIEWPORT),
   GEN6_SET(COLOR_CALC_STATE),
   GEN6_SET(BLEND_STATE),
   GEN6_SET(DEPTH_STENCIL_STATE),
   GEN6_SET(SCISSOR_RECT),
   GEN6_SET(BINDING_TABLE_STATE),
   GEN6_SET(SURFACE_STATE),
   GEN6_SET(so_SURFACE_STATE),
   GEN6_SET(SAMPLER_STATE),
   GEN6_SET(SAMPLER_BORDER_COLOR_STATE),
   GEN6_SET(push_constant_buffer),
#undef GEN6_SET
};

const struct ilo_gpe_gen6 *
ilo_gpe_gen6_get(void)
{
   return &gen6_gpe;
}
