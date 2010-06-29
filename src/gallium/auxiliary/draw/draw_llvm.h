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

#ifndef HAVE_LLVM_H
#define HAVE_LLVM_H

#include "draw/draw_private.h"

#include "draw/draw_vs.h"

#include "pipe/p_context.h"
#include "util/u_simple_list.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>

struct draw_llvm;
struct llvm_vertex_shader;

struct draw_jit_texture
{
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   const void *data;
};

enum {
   DRAW_JIT_TEXTURE_WIDTH = 0,
   DRAW_JIT_TEXTURE_HEIGHT,
   DRAW_JIT_TEXTURE_STRIDE,
   DRAW_JIT_TEXTURE_DATA
};

enum {
   DRAW_JIT_VERTEX_VERTEX_ID = 0,
   DRAW_JIT_VERTEX_CLIP,
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
   const float *vs_constants;
   const float *gs_constants;


   struct draw_jit_texture textures[PIPE_MAX_SAMPLERS];
};


#define draw_jit_context_vs_constants(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 0, "vs_constants")

#define draw_jit_context_gs_constants(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 1, "gs_constants")

#define DRAW_JIT_CONTEXT_TEXTURES_INDEX 2

#define draw_jit_context_textures(_builder, _ptr) \
   lp_build_struct_get_ptr(_builder, _ptr, DRAW_JIT_CONTEXT_TEXTURES_INDEX, "textures")



#define draw_jit_header_id(_builder, _ptr)              \
   lp_build_struct_get_ptr(_builder, _ptr, 0, "id")

#define draw_jit_header_clip(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 1, "clip")

#define draw_jit_header_data(_builder, _ptr)            \
   lp_build_struct_get_ptr(_builder, _ptr, 2, "data")


#define draw_jit_vbuffer_stride(_builder, _ptr)         \
   lp_build_struct_get(_builder, _ptr, 0, "stride")

#define draw_jit_vbuffer_max_index(_builder, _ptr)      \
   lp_build_struct_get(_builder, _ptr, 1, "max_index")

#define draw_jit_vbuffer_offset(_builder, _ptr)         \
   lp_build_struct_get(_builder, _ptr, 2, "buffer_offset")


typedef void
(*draw_jit_vert_func)(struct draw_jit_context *context,
                      struct vertex_header *io,
                      const char *vbuffers[PIPE_MAX_ATTRIBS],
                      unsigned start,
                      unsigned count,
                      unsigned stride,
                      struct pipe_vertex_buffer *vertex_buffers);


typedef void
(*draw_jit_vert_func_elts)(struct draw_jit_context *context,
                           struct vertex_header *io,
                           const char *vbuffers[PIPE_MAX_ATTRIBS],
                           const unsigned *fetch_elts,
                           unsigned fetch_count,
                           unsigned stride,
                           struct pipe_vertex_buffer *vertex_buffers);

struct draw_llvm_variant_key
{
   struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
   unsigned                   nr_vertex_elements;
   struct pipe_shader_state   vs;
};

struct draw_llvm_variant_list_item
{
   struct draw_llvm_variant *base;
   struct draw_llvm_variant_list_item *next, *prev;
};

struct draw_llvm_variant
{
   struct draw_llvm_variant_key key;
   LLVMValueRef function;
   LLVMValueRef function_elts;
   draw_jit_vert_func jit_func;
   draw_jit_vert_func_elts jit_func_elts;

   struct llvm_vertex_shader *shader;

   struct draw_llvm *llvm;
   struct draw_llvm_variant_list_item list_item_global;
   struct draw_llvm_variant_list_item list_item_local;
};

struct llvm_vertex_shader {
   struct draw_vertex_shader base;

   struct draw_llvm_variant_list_item variants;
   unsigned variants_created;
   unsigned variants_cached;
};

struct draw_llvm {
   struct draw_context *draw;

   struct draw_jit_context jit_context;

   struct draw_llvm_variant_list_item vs_variants_list;
   int nr_variants;

   LLVMModuleRef module;
   LLVMExecutionEngineRef engine;
   LLVMModuleProviderRef provider;
   LLVMTargetDataRef target;
   LLVMPassManagerRef pass;

   LLVMTypeRef context_ptr_type;
   LLVMTypeRef vertex_header_ptr_type;
   LLVMTypeRef buffer_ptr_type;
   LLVMTypeRef vb_ptr_type;
};

static INLINE struct llvm_vertex_shader *
llvm_vertex_shader(struct draw_vertex_shader *vs)
{
   return (struct llvm_vertex_shader *)vs;
}


struct draw_llvm *
draw_llvm_create(struct draw_context *draw);

void
draw_llvm_destroy(struct draw_llvm *llvm);

struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm, int num_inputs);

void
draw_llvm_destroy_variant(struct draw_llvm_variant *variant);

void
draw_llvm_make_variant_key(struct draw_llvm *llvm,
                           struct draw_llvm_variant_key *key);

LLVMValueRef
draw_llvm_translate_from(LLVMBuilderRef builder,
                         LLVMValueRef vbuffer,
                         enum pipe_format from_format);

#endif
