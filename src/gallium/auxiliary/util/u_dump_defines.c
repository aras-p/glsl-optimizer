/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
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


#include "util/u_memory.h"
#include "util/u_debug.h" 
#include "util/u_dump.h"


#if 0
static const char *
util_dump_strip_prefix(const char *name,
                        const char *prefix) 
{
   const char *stripped;
   assert(name);
   assert(prefix);
   stripped = name;
   while(*prefix) {
      if(*stripped != *prefix)
	 return name;

      ++stripped;
      ++prefix;
   }
   return stripped;
}
#endif

static const char *
util_dump_enum_continuous(unsigned value,
                           unsigned num_names,
                           const char **names)
{
   if (value >= num_names)
      return UTIL_DUMP_INVALID_NAME;
   return names[value];
}


#define DEFINE_UTIL_DUMP_CONTINUOUS(_name) \
   const char * \
   util_dump_##_name(unsigned value, boolean shortened) \
   { \
      if(shortened) \
         return util_dump_enum_continuous(value, Elements(util_dump_##_name##_short_names), util_dump_##_name##_short_names); \
      else \
         return util_dump_enum_continuous(value, Elements(util_dump_##_name##_names), util_dump_##_name##_names); \
   }


/**
 * Same as DEFINE_UTIL_DUMP_CONTINUOUS but with static assertions to detect
 * failures to update lists.
 */
#define DEFINE_UTIL_DUMP_CONTINUOUS_COUNT(_name, _count) \
   const char * \
   util_dump_##_name(unsigned value, boolean shortened) \
   { \
      STATIC_ASSERT(Elements(util_dump_##_name##_names) == _count); \
      STATIC_ASSERT(Elements(util_dump_##_name##_short_names) == _count); \
      if(shortened) \
         return util_dump_enum_continuous(value, Elements(util_dump_##_name##_short_names), util_dump_##_name##_short_names); \
      else \
         return util_dump_enum_continuous(value, Elements(util_dump_##_name##_names), util_dump_##_name##_names); \
   }


static const char *
util_dump_blend_factor_names[] = {
   UTIL_DUMP_INVALID_NAME, /* 0x0 */
   "PIPE_BLENDFACTOR_ONE",
   "PIPE_BLENDFACTOR_SRC_COLOR",
   "PIPE_BLENDFACTOR_SRC_ALPHA",
   "PIPE_BLENDFACTOR_DST_ALPHA",
   "PIPE_BLENDFACTOR_DST_COLOR",
   "PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE",
   "PIPE_BLENDFACTOR_CONST_COLOR",
   "PIPE_BLENDFACTOR_CONST_ALPHA",
   "PIPE_BLENDFACTOR_SRC1_COLOR",
   "PIPE_BLENDFACTOR_SRC1_ALPHA",
   UTIL_DUMP_INVALID_NAME, /* 0x0b */
   UTIL_DUMP_INVALID_NAME, /* 0x0c */
   UTIL_DUMP_INVALID_NAME, /* 0x0d */
   UTIL_DUMP_INVALID_NAME, /* 0x0e */
   UTIL_DUMP_INVALID_NAME, /* 0x0f */
   UTIL_DUMP_INVALID_NAME, /* 0x10 */
   "PIPE_BLENDFACTOR_ZERO",
   "PIPE_BLENDFACTOR_INV_SRC_COLOR",
   "PIPE_BLENDFACTOR_INV_SRC_ALPHA",
   "PIPE_BLENDFACTOR_INV_DST_ALPHA",
   "PIPE_BLENDFACTOR_INV_DST_COLOR",
   UTIL_DUMP_INVALID_NAME, /* 0x16 */
   "PIPE_BLENDFACTOR_INV_CONST_COLOR",
   "PIPE_BLENDFACTOR_INV_CONST_ALPHA",
   "PIPE_BLENDFACTOR_INV_SRC1_COLOR",
   "PIPE_BLENDFACTOR_INV_SRC1_ALPHA"
};

static const char *
util_dump_blend_factor_short_names[] = {
   UTIL_DUMP_INVALID_NAME, /* 0x0 */
   "one",
   "src_color",
   "src_alpha",
   "dst_alpha",
   "dst_color",
   "src_alpha_saturate",
   "const_color",
   "const_alpha",
   "src1_color",
   "src1_alpha",
   UTIL_DUMP_INVALID_NAME, /* 0x0b */
   UTIL_DUMP_INVALID_NAME, /* 0x0c */
   UTIL_DUMP_INVALID_NAME, /* 0x0d */
   UTIL_DUMP_INVALID_NAME, /* 0x0e */
   UTIL_DUMP_INVALID_NAME, /* 0x0f */
   UTIL_DUMP_INVALID_NAME, /* 0x10 */
   "zero",
   "inv_src_color",
   "inv_src_alpha",
   "inv_dst_alpha",
   "inv_dst_color",
   UTIL_DUMP_INVALID_NAME, /* 0x16 */
   "inv_const_color",
   "inv_const_alpha",
   "inv_src1_color",
   "inv_src1_alpha"
};

DEFINE_UTIL_DUMP_CONTINUOUS(blend_factor)


static const char *
util_dump_blend_func_names[] = {
   "PIPE_BLEND_ADD",
   "PIPE_BLEND_SUBTRACT",
   "PIPE_BLEND_REVERSE_SUBTRACT",
   "PIPE_BLEND_MIN",
   "PIPE_BLEND_MAX"
};

static const char *
util_dump_blend_func_short_names[] = {
   "add",
   "sub",
   "rev_sub",
   "min",
   "max"
};

DEFINE_UTIL_DUMP_CONTINUOUS(blend_func)


static const char *
util_dump_logicop_names[] = {
   "PIPE_LOGICOP_CLEAR",
   "PIPE_LOGICOP_NOR",
   "PIPE_LOGICOP_AND_INVERTED",
   "PIPE_LOGICOP_COPY_INVERTED",
   "PIPE_LOGICOP_AND_REVERSE",
   "PIPE_LOGICOP_INVERT",
   "PIPE_LOGICOP_XOR",
   "PIPE_LOGICOP_NAND",
   "PIPE_LOGICOP_AND",
   "PIPE_LOGICOP_EQUIV",
   "PIPE_LOGICOP_NOOP",
   "PIPE_LOGICOP_OR_INVERTED",
   "PIPE_LOGICOP_COPY",
   "PIPE_LOGICOP_OR_REVERSE",
   "PIPE_LOGICOP_OR",
   "PIPE_LOGICOP_SET"
};

static const char *
util_dump_logicop_short_names[] = {
   "clear",
   "nor",
   "and_inverted",
   "copy_inverted",
   "and_reverse",
   "invert",
   "xor",
   "nand",
   "and",
   "equiv",
   "noop",
   "or_inverted",
   "copy",
   "or_reverse",
   "or",
   "set"
};

DEFINE_UTIL_DUMP_CONTINUOUS(logicop)


static const char *
util_dump_func_names[] = {
   "PIPE_FUNC_NEVER",
   "PIPE_FUNC_LESS",
   "PIPE_FUNC_EQUAL",
   "PIPE_FUNC_LEQUAL",
   "PIPE_FUNC_GREATER",
   "PIPE_FUNC_NOTEQUAL",
   "PIPE_FUNC_GEQUAL",
   "PIPE_FUNC_ALWAYS"
};

static const char *
util_dump_func_short_names[] = {
   "never",
   "less",
   "equal",
   "less_equal",
   "greater",
   "not_equal",
   "greater_equal",
   "always"
};

DEFINE_UTIL_DUMP_CONTINUOUS(func)


static const char *
util_dump_stencil_op_names[] = {
   "PIPE_STENCIL_OP_KEEP",
   "PIPE_STENCIL_OP_ZERO",
   "PIPE_STENCIL_OP_REPLACE",
   "PIPE_STENCIL_OP_INCR",
   "PIPE_STENCIL_OP_DECR",
   "PIPE_STENCIL_OP_INCR_WRAP",
   "PIPE_STENCIL_OP_DECR_WRAP",
   "PIPE_STENCIL_OP_INVERT"
};

static const char *
util_dump_stencil_op_short_names[] = {
   "keep",
   "zero",
   "replace",
   "incr",
   "decr",
   "incr_wrap",
   "decr_wrap",
   "invert"
};

DEFINE_UTIL_DUMP_CONTINUOUS(stencil_op)


static const char *
util_dump_tex_target_names[] = {
   "PIPE_BUFFER",
   "PIPE_TEXTURE_1D",
   "PIPE_TEXTURE_2D",
   "PIPE_TEXTURE_3D",
   "PIPE_TEXTURE_CUBE",
   "PIPE_TEXTURE_RECT",
   "PIPE_TEXTURE_1D_ARRAY",
   "PIPE_TEXTURE_2D_ARRAY",
   "PIPE_TEXTURE_CUBE_ARRAY",
};

static const char *
util_dump_tex_target_short_names[] = {
   "buffer",
   "1d",
   "2d",
   "3d",
   "cube",
   "rect",
   "1d_array",
   "2d_array",
   "cube_array",
};

DEFINE_UTIL_DUMP_CONTINUOUS_COUNT(tex_target, PIPE_MAX_TEXTURE_TYPES)


static const char *
util_dump_tex_wrap_names[] = {
   "PIPE_TEX_WRAP_REPEAT",
   "PIPE_TEX_WRAP_CLAMP",
   "PIPE_TEX_WRAP_CLAMP_TO_EDGE",
   "PIPE_TEX_WRAP_CLAMP_TO_BORDER",
   "PIPE_TEX_WRAP_MIRROR_REPEAT",
   "PIPE_TEX_WRAP_MIRROR_CLAMP",
   "PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE",
   "PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER"
};

static const char *
util_dump_tex_wrap_short_names[] = {
   "repeat",
   "clamp",
   "clamp_to_edge",
   "clamp_to_border",
   "mirror_repeat",
   "mirror_clamp",
   "mirror_clamp_to_edge",
   "mirror_clamp_to_border"
};

DEFINE_UTIL_DUMP_CONTINUOUS(tex_wrap)


static const char *
util_dump_tex_mipfilter_names[] = {
   "PIPE_TEX_MIPFILTER_NEAREST",
   "PIPE_TEX_MIPFILTER_LINEAR",
   "PIPE_TEX_MIPFILTER_NONE"
};

static const char *
util_dump_tex_mipfilter_short_names[] = {
   "nearest",
   "linear",
   "none"
};

DEFINE_UTIL_DUMP_CONTINUOUS(tex_mipfilter)


static const char *
util_dump_tex_filter_names[] = {
   "PIPE_TEX_FILTER_NEAREST",
   "PIPE_TEX_FILTER_LINEAR"
};

static const char *
util_dump_tex_filter_short_names[] = {
   "nearest",
   "linear"
};

DEFINE_UTIL_DUMP_CONTINUOUS(tex_filter)


static const char *
util_dump_query_type_names[] = {
   "PIPE_QUERY_OCCLUSION_COUNTER",
   "PIPE_QUERY_OCCLUSION_PREDICATE",
   "PIPE_QUERY_TIMESTAMP",
   "PIPE_QUERY_TIMESTAMP_DISJOINT",
   "PIPE_QUERY_TIME_ELAPSED",
   "PIPE_QUERY_PRIMITIVES_GENERATED",
   "PIPE_QUERY_PRIMITIVES_EMITTED",
   "PIPE_QUERY_SO_STATISTICS",
   "PIPE_QUERY_SO_OVERFLOW_PREDICATE",
   "PIPE_QUERY_GPU_FINISHED",
   "PIPE_QUERY_PIPELINE_STATISTICS",
};

static const char *
util_dump_query_type_short_names[] = {
   "occlusion_counter",
   "occlusion_predicate",
   "timestamp",
   "timestamp_disjoint",
   "time_elapsed",
   "primitives_generated",
   "primitives_emitted",
   "so_statistics",
   "so_overflow_predicate",
   "gpu_finished",
   "pipeline_statistics",
};

DEFINE_UTIL_DUMP_CONTINUOUS(query_type)
