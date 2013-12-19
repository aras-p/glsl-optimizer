/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#ifndef DRAW_LLVM_H
#define DRAW_LLVM_H

#include "draw/draw_private.h"

#include "draw/draw_vs.h"
#include "draw/draw_gs.h"

#include "gallivm/lp_bld_sample.h"
#include "gallivm/lp_bld_limits.h"

#include "pipe/p_context.h"
#include "util/u_simple_list.h"


struct draw_llvm;
struct llvm_vertex_shader;
struct llvm_geometry_shader;

struct draw_jit_texture
{
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t first_level;
   uint32_t last_level;
   const void *base;
   uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t mip_offsets[PIPE_MAX_TEXTURE_LEVELS];
};


struct draw_sampler_static_state
{
   /*
    * These attributes are effectively interleaved for more sane key handling.
    * However, there might be lots of null space if the amount of samplers and
    * textures isn't the same.
    */
   struct lp_static_sampler_state sampler_state;
   struct lp_static_texture_state texture_state;
};


struct draw_jit_sampler
{
   float min_lod;
   float max_lod;
   float lod_bias;
   float border_color[4];
};


enum {
   DRAW_JIT_TEXTURE_WIDTH = 0,
   DRAW_JIT_TEXTURE_HEIGHT,
   DRAW_JIT_TEXTURE_DEPTH,
   DRAW_JIT_TEXTURE_FIRST_LEVEL,
   DRAW_JIT_TEXTURE_LAST_LEVEL,
   DRAW_JIT_TEXTURE_BASE,
   DRAW_JIT_TEXTURE_ROW_STRIDE,
   DRAW_JIT_TEXTURE_IMG_STRIDE,
   DRAW_JIT_TEXTURE_MIP_OFFSETS,
   DRAW_JIT_TEXTURE_NUM_FIELDS  /* number of fields above */
};


enum {
   DRAW_JIT_SAMPLER_MIN_LOD,
   DRAW_JIT_SAMPLER_MAX_LOD,
   DRAW_JIT_SAMPLER_LOD_BIAS,
   DRAW_JIT_SAMPLER_BORDER_COLOR,
   DRAW_JIT_SAMPLER_NUM_FIELDS  /* number of fields above */
};


enum {
   DRAW_JIT_VERTEX_VERTEX_ID = 0,
   DRAW_JIT_VERTEX_CLIP,
   DRAW_JIT_VERTEX_PRE_CLIP_POS,
   DRAW_JIT_VERTEX_DATA
};

/**
 * This structure is passed directly to the generated vertex shader.
 *
 * It contains the derived state.
 *
 * Changes here must be reflected in the draw_jit_context_* macros.
 * Changes to the ordering should be avoided.
 *
 * Only use types with a clear size and padding here, in particular prefer the
 * stdint.h types to the basic integer types.
 */
struct draw_jit_context
{
   const float *vs_constants[LP_MAX_TGSI_CONST_BUFFERS];
   int num_vs_constants[LP_MAX_TGSI_CONST_BUFFERS];
   float (*planes) [DRAW_TOTAL_CLIP_PLANES][4];
   float *viewport;

   struct draw_jit_texture textures[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   struct draw_jit_sampler samplers[PIPE_MAX_SAMPLERS];
};

enum {
   DRAW_JIT_CTX_CONSTANTS            = 0,
   DRAW_JIT_CTX_NUM_CONSTANTS        = 1,
   DRAW_JIT_CTX_PLANES               = 2,
   DRAW_JIT_CTX_VIEWPORT             = 3,
   DRAW_JIT_CTX_TEXTURES             = 4,
   DRAW_JIT_CTX_SAMPLERS             = 5,
   DRAW_JIT_CTX_NUM_FIELDS
};

#define draw_jit_context_vs_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_CTX_CONSTANTS, "vs_constants")

#define draw_jit_context_num_vs_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_CTX_NUM_CONSTANTS, "num_vs_constants")

#define draw_jit_context_planes(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_JIT_CTX_PLANES, "planes")

#define draw_jit_context_viewport(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_JIT_CTX_VIEWPORT, "viewport")

#define draw_jit_context_textures(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_CTX_TEXTURES, "textures")

#define draw_jit_context_samplers(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_CTX_SAMPLERS, "samplers")

#define draw_jit_header_id(_gallivm, _ptr)              \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_VERTEX_ID, "id")

#define draw_jit_header_clip(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_CLIP, "clip")

#define draw_jit_header_pre_clip_pos(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_PRE_CLIP_POS, "pre_clip_pos")

#define draw_jit_header_data(_gallivm, _ptr)            \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_DATA, "data")


#define draw_jit_vbuffer_stride(_gallivm, _ptr)         \
   lp_build_struct_get(_gallivm, _ptr, 0, "stride")

#define draw_jit_vbuffer_offset(_gallivm, _ptr)         \
   lp_build_struct_get(_gallivm, _ptr, 1, "buffer_offset")

enum {
   DRAW_JIT_DVBUFFER_MAP = 0,
   DRAW_JIT_DVBUFFER_SIZE,
   DRAW_JIT_DVBUFFER_NUM_FIELDS  /* number of fields above */
};

#define draw_jit_dvbuffer_map(_gallivm, _ptr)         \
   lp_build_struct_get(_gallivm, _ptr, DRAW_JIT_DVBUFFER_MAP, "map")

#define draw_jit_dvbuffer_size(_gallivm, _ptr)        \
   lp_build_struct_get(_gallivm, _ptr, DRAW_JIT_DVBUFFER_SIZE, "size")


/**
 * This structure is passed directly to the generated geometry shader.
 *
 * It contains the derived state.
 *
 * Changes here must be reflected in the draw_gs_jit_context_* macros.
 * Changes to the ordering should be avoided.
 *
 * Only use types with a clear size and padding here, in particular prefer the
 * stdint.h types to the basic integer types.
 */
struct draw_gs_jit_context
{
   const float *constants[LP_MAX_TGSI_CONST_BUFFERS];
   int num_constants[LP_MAX_TGSI_CONST_BUFFERS];
   float (*planes) [DRAW_TOTAL_CLIP_PLANES][4];
   float *viewport;

   /* There two need to be exactly at DRAW_JIT_CTX_TEXTURES and
    * DRAW_JIT_CTX_SAMPLERS positions in the struct */
   struct draw_jit_texture textures[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   struct draw_jit_sampler samplers[PIPE_MAX_SAMPLERS];
   
   int **prim_lengths;
   int *emitted_vertices;
   int *emitted_prims;
};

enum {
   DRAW_GS_JIT_CTX_CONSTANTS = 0,
   DRAW_GS_JIT_CTX_NUM_CONSTANTS = 1,
   DRAW_GS_JIT_CTX_PLANES = 2,
   DRAW_GS_JIT_CTX_VIEWPORT = 3,
   /* Textures and samples are reserved for DRAW_JIT_CTX_TEXTURES
    * and DRAW_JIT_CTX_SAMPLERS, because they both need
    * to be at exactly the same locations as they are in the
    * VS ctx structure for sampling to work. */
   DRAW_GS_JIT_CTX_TEXTURES = DRAW_JIT_CTX_TEXTURES,
   DRAW_GS_JIT_CTX_SAMPLERS = DRAW_JIT_CTX_SAMPLERS,
   DRAW_GS_JIT_CTX_PRIM_LENGTHS = 6,
   DRAW_GS_JIT_CTX_EMITTED_VERTICES = 7,
   DRAW_GS_JIT_CTX_EMITTED_PRIMS = 8,
   DRAW_GS_JIT_CTX_NUM_FIELDS = 9
};

#define draw_gs_jit_context_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_GS_JIT_CTX_CONSTANTS, "constants")

#define draw_gs_jit_context_num_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_GS_JIT_CTX_NUM_CONSTANTS, "num_constants")

#define draw_gs_jit_context_planes(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_GS_JIT_CTX_PLANES, "planes")

#define draw_gs_jit_context_viewport(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_GS_JIT_CTX_VIEWPORT, "viewport")

#define draw_gs_jit_context_textures(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_GS_JIT_CTX_TEXTURES, "textures")

#define draw_gs_jit_context_samplers(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_GS_JIT_CTX_SAMPLERS, "samplers")

#define draw_gs_jit_prim_lengths(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_GS_JIT_CTX_PRIM_LENGTHS, "prim_lengths")

#define draw_gs_jit_emitted_vertices(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_GS_JIT_CTX_EMITTED_VERTICES, "emitted_vertices")

#define draw_gs_jit_emitted_prims(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, DRAW_GS_JIT_CTX_EMITTED_PRIMS, "emitted_prims")



typedef int
(*draw_jit_vert_func)(struct draw_jit_context *context,
                      struct vertex_header *io,
                      const struct draw_vertex_buffer vbuffers[PIPE_MAX_ATTRIBS],
                      unsigned start,
                      unsigned count,
                      unsigned stride,
                      struct pipe_vertex_buffer *vertex_buffers,
                      unsigned instance_id,
                      unsigned vertex_id_offset);


typedef int
(*draw_jit_vert_func_elts)(struct draw_jit_context *context,
                           struct vertex_header *io,
                           const struct draw_vertex_buffer vbuffers[PIPE_MAX_ATTRIBS],
                           const unsigned *fetch_elts,
                           unsigned fetch_max_elt,
                           unsigned fetch_count,
                           unsigned stride,
                           struct pipe_vertex_buffer *vertex_buffers,
                           unsigned instance_id,
                           unsigned vertex_id_offset);


typedef int
(*draw_gs_jit_func)(struct draw_gs_jit_context *context,
                    float inputs[6][PIPE_MAX_SHADER_INPUTS][TGSI_NUM_CHANNELS][TGSI_NUM_CHANNELS],
                    struct vertex_header *output,
                    unsigned num_prims,
                    unsigned instance_id,
                    int *prim_ids);

struct draw_llvm_variant_key
{
   unsigned nr_vertex_elements:8;
   unsigned nr_samplers:8;
   unsigned nr_sampler_views:8;
   unsigned clamp_vertex_color:1;
   unsigned clip_xy:1;
   unsigned clip_z:1;
   unsigned clip_user:1;
   unsigned clip_halfz:1;
   unsigned bypass_viewport:1;
   unsigned need_edgeflags:1;
   unsigned has_gs:1;
   unsigned num_outputs:8;
   /*
    * it is important there are no holes in this struct
    * (and all padding gets zeroed).
    */
   unsigned ucp_enable:PIPE_MAX_CLIP_PLANES;
   unsigned pad1:24-PIPE_MAX_CLIP_PLANES;

   /* Variable number of vertex elements:
    */
   struct pipe_vertex_element vertex_element[1];

   /* Followed by variable number of samplers:
    */
/*   struct draw_sampler_static_state sampler; */
};

struct draw_gs_llvm_variant_key
{
   unsigned nr_samplers:8;
   unsigned nr_sampler_views:8;
   unsigned num_outputs:8;

   struct draw_sampler_static_state samplers[1];
};

#define DRAW_LLVM_MAX_VARIANT_KEY_SIZE \
   (sizeof(struct draw_llvm_variant_key) +	\
    PIPE_MAX_SHADER_SAMPLER_VIEWS * sizeof(struct draw_sampler_static_state) +	\
    (PIPE_MAX_ATTRIBS-1) * sizeof(struct pipe_vertex_element))

#define DRAW_GS_LLVM_MAX_VARIANT_KEY_SIZE \
   (sizeof(struct draw_gs_llvm_variant_key) +	\
    PIPE_MAX_SHADER_SAMPLER_VIEWS * sizeof(struct draw_sampler_static_state))


static INLINE size_t
draw_llvm_variant_key_size(unsigned nr_vertex_elements,
                           unsigned nr_samplers)
{
   return (sizeof(struct draw_llvm_variant_key) +
           nr_samplers * sizeof(struct draw_sampler_static_state) +
           (nr_vertex_elements - 1) * sizeof(struct pipe_vertex_element));
}


static INLINE size_t
draw_gs_llvm_variant_key_size(unsigned nr_samplers)
{
   return (sizeof(struct draw_gs_llvm_variant_key) +
           (nr_samplers - 1) * sizeof(struct draw_sampler_static_state));
}


static INLINE struct draw_sampler_static_state *
draw_llvm_variant_key_samplers(struct draw_llvm_variant_key *key)
{
   return (struct draw_sampler_static_state *)
      &key->vertex_element[key->nr_vertex_elements];
}


struct draw_llvm_variant_list_item
{
   struct draw_llvm_variant *base;
   struct draw_llvm_variant_list_item *next, *prev;
};

struct draw_gs_llvm_variant_list_item
{
   struct draw_gs_llvm_variant *base;
   struct draw_gs_llvm_variant_list_item *next, *prev;
};


struct draw_llvm_variant
{
   struct gallivm_state *gallivm;

   /* LLVM JIT builder types */
   LLVMTypeRef context_ptr_type;
   LLVMTypeRef buffer_ptr_type;
   LLVMTypeRef vb_ptr_type;
   LLVMTypeRef vertex_header_ptr_type;

   LLVMValueRef function;
   LLVMValueRef function_elts;
   draw_jit_vert_func jit_func;
   draw_jit_vert_func_elts jit_func_elts;

   struct llvm_vertex_shader *shader;

   struct draw_llvm *llvm;
   struct draw_llvm_variant_list_item list_item_global;
   struct draw_llvm_variant_list_item list_item_local;

   /* key is variable-sized, must be last */
   struct draw_llvm_variant_key key;
};


struct draw_gs_llvm_variant
{
   struct gallivm_state *gallivm;

   /* LLVM JIT builder types */
   LLVMTypeRef context_ptr_type;
   LLVMTypeRef vertex_header_ptr_type;
   LLVMTypeRef input_array_type;

   LLVMValueRef context_ptr;
   LLVMValueRef io_ptr;
   LLVMValueRef num_prims;
   LLVMValueRef function;
   draw_gs_jit_func jit_func;

   struct llvm_geometry_shader *shader;

   struct draw_llvm *llvm;
   struct draw_gs_llvm_variant_list_item list_item_global;
   struct draw_gs_llvm_variant_list_item list_item_local;

   /* key is variable-sized, must be last */
   struct draw_gs_llvm_variant_key key;
};

struct llvm_vertex_shader {
   struct draw_vertex_shader base;

   unsigned variant_key_size;
   struct draw_llvm_variant_list_item variants;
   unsigned variants_created;
   unsigned variants_cached;
};

struct llvm_geometry_shader {
   struct draw_geometry_shader base;

   unsigned variant_key_size;
   struct draw_gs_llvm_variant_list_item variants;
   unsigned variants_created;
   unsigned variants_cached;
};


struct draw_llvm {
   struct draw_context *draw;

   struct draw_jit_context jit_context;
   struct draw_gs_jit_context gs_jit_context;

   struct draw_llvm_variant_list_item vs_variants_list;
   int nr_variants;

   struct draw_gs_llvm_variant_list_item gs_variants_list;
   int nr_gs_variants;
};


static INLINE struct llvm_vertex_shader *
llvm_vertex_shader(struct draw_vertex_shader *vs)
{
   return (struct llvm_vertex_shader *)vs;
}

static INLINE struct llvm_geometry_shader *
llvm_geometry_shader(struct draw_geometry_shader *gs)
{
   return (struct llvm_geometry_shader *)gs;
}




struct draw_llvm *
draw_llvm_create(struct draw_context *draw);

void
draw_llvm_destroy(struct draw_llvm *llvm);

struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm,
                         unsigned num_vertex_header_attribs,
                         const struct draw_llvm_variant_key *key);

void
draw_llvm_destroy_variant(struct draw_llvm_variant *variant);

struct draw_llvm_variant_key *
draw_llvm_make_variant_key(struct draw_llvm *llvm, char *store);

void
draw_llvm_dump_variant_key(struct draw_llvm_variant_key *key);


struct draw_gs_llvm_variant *
draw_gs_llvm_create_variant(struct draw_llvm *llvm,
                            unsigned num_vertex_header_attribs,
                            const struct draw_gs_llvm_variant_key *key);

void
draw_gs_llvm_destroy_variant(struct draw_gs_llvm_variant *variant);

struct draw_gs_llvm_variant_key *
draw_gs_llvm_make_variant_key(struct draw_llvm *llvm, char *store);

void
draw_gs_llvm_dump_variant_key(struct draw_gs_llvm_variant_key *key);

struct lp_build_sampler_soa *
draw_llvm_sampler_soa_create(const struct draw_sampler_static_state *static_state,
                             LLVMValueRef context_ptr);

void
draw_llvm_set_sampler_state(struct draw_context *draw, unsigned shader_stage);

void
draw_llvm_set_mapped_texture(struct draw_context *draw,
                             unsigned shader_stage,
                             unsigned sview_idx,
                             uint32_t width, uint32_t height, uint32_t depth,
                             uint32_t first_level, uint32_t last_level,
                             const void *base_ptr,
                             uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                             uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                             uint32_t mip_offsets[PIPE_MAX_TEXTURE_LEVELS]);

#endif
