#ifndef HAVE_LLVM_H
#define HAVE_LLVM_H

#include "draw/draw_private.h"

#include "pipe/p_context.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>

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

#define draw_jit_vbuffer_offset(_builder, _ptr)                 \
   lp_build_struct_get(_builder, _ptr, 2, "buffer_offset")


typedef void
(*draw_jit_vert_func)(struct draw_jit_context *context,
                      struct vertex_header *io,
                      const char *vbuffers[PIPE_MAX_ATTRIBS],
                      unsigned start,
                      unsigned count,
                      unsigned stride,
                      struct pipe_vertex_buffer *vertex_buffers);

struct draw_llvm {
   struct draw_context *draw;

   struct draw_jit_context jit_context;

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

struct draw_llvm_variant_key
{
   struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
   unsigned                   nr_vertex_elements;
   struct pipe_shader_state   vs;
};

struct draw_llvm_variant
{
   struct draw_llvm_variant_key key;
   LLVMValueRef function;
   draw_jit_vert_func jit_func;

   struct draw_llvm_variant *next;
};

struct draw_llvm *
draw_llvm_create(struct draw_context *draw);

void
draw_llvm_destroy(struct draw_llvm *llvm);

struct draw_llvm_variant *
draw_llvm_prepare(struct draw_llvm *llvm, int num_inputs);

void
draw_llvm_make_variant_key(struct draw_llvm *llvm,
                           struct draw_llvm_variant_key *key);

LLVMValueRef
draw_llvm_translate_from(LLVMBuilderRef builder,
                         LLVMValueRef vbuffer,
                         enum pipe_format from_format);
#endif
