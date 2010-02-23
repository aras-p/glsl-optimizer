#ifndef DRAW_LLVM_H
#define DRAW_LLVM_H

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

/* we are construction a function of the form:

struct vertex_header {
   uint32 vertex_id;

   float clip[4];
   float data[][4];
};

struct draw_jit_context
{
   const float *vs_constants;
   const float *gs_constants;

   struct draw_jit_texture textures[PIPE_MAX_SAMPLERS];
   const void *vbuffers;
};

void
draw_shader(struct draw_jit_context *context,
            struct vertex_header *io,
            unsigned start,
            unsigned count,
            unsigned stride)
{
  // do a fetch and a run vertex shader
  for (int i = 0; i < count; ++i) {
    struct vertex_header *header = &io[i];
    header->vertex_id = 0xffff;
    // follows code-genarted fetch/translate section
    // for each vertex_element ...
    codegened_translate(header->data[num_element],
                       context->vertex_elements[num_element],
                       context->vertex_buffers,
                       context->vbuffers);

    codegened_vertex_shader(header->data, context->vs_constants);
  }

  for (int i = 0; i < count; i += context->primitive_size) {
     struct vertex_header *prim[MAX_PRIMITIVE_SIZE];
     for (int j = 0; j < context->primitive_size; ++j) {
       header[j] = &io[i + j];
     }
     codegened_geometry_shader(prim, gs_constants);
  }
}
*/

typedef void
(*draw_jit_vert_func)(struct draw_jit_context *context,
                      struct vertex_header *io,
                      unsigned start,
                      unsigned count,
                      unsigned stride);

struct draw_llvm {
   struct draw_context *draw;

   struct draw_jit_context jit_context;

   draw_jit_vert_func jit_func;

   LLVMModuleRef module;
   LLVMExecutionEngineRef engine;
   LLVMModuleProviderRef provider;
   LLVMTargetDataRef target;
   LLVMPassManagerRef pass;

   LLVMTypeRef context_ptr_type;
   LLVMTypeRef vertex_header_ptr_type;
};


struct draw_llvm *
draw_llvm_create(struct draw_context *draw);

void
draw_llvm_destroy(struct draw_llvm *llvm);

void
draw_llvm_prepare(struct draw_llvm *llvm);

/* generates the draw jit function */
void
draw_llvm_generate(struct draw_llvm *llvm);


#endif
