#include "draw_llvm.h"

#include "draw_context.h"
#include "draw_vs.h"

#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_interp.h"
#include "gallivm/lp_bld_struct.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_printf.h"

#include "util/u_cpu_detect.h"

#include <llvm-c/Transforms/Scalar.h>

static void
init_globals(struct draw_llvm *llvm)
{
    LLVMTypeRef vertex_header;
    LLVMTypeRef texture_type;

   /* struct vertex_header */
   {
      LLVMTypeRef elem_types[3];

      elem_types[0]  = LLVMIntType(32);
      elem_types[1]  = LLVMArrayType(LLVMFloatType(), 4);
      elem_types[2]  = LLVMArrayType(elem_types[1], 0);

      vertex_header = LLVMStructType(elem_types, Elements(elem_types), 0);

      /* these are bit-fields and we can't take address of them
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, clipmask,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_CLIPMASK);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, edgeflag,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_EDGEFLAG);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, pad,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_PAD);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, vertex_id,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_VERTEX_ID);
      */
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, clip,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_CLIP);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, data,
                             llvm->target, vertex_header,
                             DRAW_JIT_VERTEX_DATA);

      LP_CHECK_STRUCT_SIZE(struct vertex_header,
                           llvm->target, vertex_header);

      LLVMAddTypeName(llvm->module, "vertex_header", vertex_header);

      llvm->vertex_header_ptr_type = LLVMPointerType(vertex_header, 0);
   }
      /* struct draw_jit_texture */
   {
      LLVMTypeRef elem_types[4];

      elem_types[DRAW_JIT_TEXTURE_WIDTH]  = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_HEIGHT] = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_STRIDE] = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_DATA]   = LLVMPointerType(LLVMInt8Type(), 0);

      texture_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, width,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_WIDTH);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, height,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_HEIGHT);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, stride,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, data,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_DATA);
      LP_CHECK_STRUCT_SIZE(struct draw_jit_texture,
                           llvm->target, texture_type);

      LLVMAddTypeName(llvm->module, "texture", texture_type);
   }


   /* struct draw_jit_context */
   {
      LLVMTypeRef elem_types[3];
      LLVMTypeRef context_type;

      elem_types[0] = LLVMPointerType(LLVMFloatType(), 0); /* vs_constants */
      elem_types[1] = LLVMPointerType(LLVMFloatType(), 0); /* vs_constants */
      elem_types[2] = LLVMArrayType(texture_type, PIPE_MAX_SAMPLERS); /* textures */

      context_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, vs_constants,
                             llvm->target, context_type, 0);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, gs_constants,
                             llvm->target, context_type, 1);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, textures,
                             llvm->target, context_type,
                             DRAW_JIT_CONTEXT_TEXTURES_INDEX);
      LP_CHECK_STRUCT_SIZE(struct draw_jit_context,
                           llvm->target, context_type);

      LLVMAddTypeName(llvm->module, "draw_jit_context", context_type);

      llvm->context_ptr_type = LLVMPointerType(context_type, 0);
   }
   {
      LLVMTypeRef buffer_ptr = LLVMPointerType(LLVMIntType(8), 0);
      llvm->buffer_ptr_type = LLVMPointerType(buffer_ptr, 0);
   }
}

struct draw_llvm *
draw_llvm_create(struct draw_context *draw)
{
   struct draw_llvm *llvm = CALLOC_STRUCT( draw_llvm );

   util_cpu_detect();

   llvm->draw = draw;
   llvm->engine = draw->engine;

   debug_assert(llvm->engine);

   llvm->module = LLVMModuleCreateWithName("draw_llvm");
   llvm->provider = LLVMCreateModuleProviderForExistingModule(llvm->module);

   LLVMAddModuleProvider(llvm->engine, llvm->provider);

   llvm->target = LLVMGetExecutionEngineTargetData(llvm->engine);

   llvm->pass = LLVMCreateFunctionPassManager(llvm->provider);
   LLVMAddTargetData(llvm->target, llvm->pass);
   /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
    * but there are more on SVN. */
   /* TODO: Add more passes */
   LLVMAddConstantPropagationPass(llvm->pass);
   if(util_cpu_caps.has_sse4_1) {
      /* FIXME: There is a bug in this pass, whereby the combination of fptosi
       * and sitofp (necessary for trunc/floor/ceil/round implementation)
       * somehow becomes invalid code.
       */
      LLVMAddInstructionCombiningPass(llvm->pass);
   }
   LLVMAddPromoteMemoryToRegisterPass(llvm->pass);
   LLVMAddGVNPass(llvm->pass);
   LLVMAddCFGSimplificationPass(llvm->pass);

   init_globals(llvm);


#if 1
   LLVMDumpModule(llvm->module);
#endif

   return llvm;
}

void
draw_llvm_destroy(struct draw_llvm *llvm)
{
   free(llvm);
}

void
draw_llvm_prepare(struct draw_llvm *llvm)
{
   draw_llvm_generate(llvm);
}


struct draw_context *draw_create_with_llvm(LLVMExecutionEngineRef engine)
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   if (draw == NULL)
      goto fail;
   draw->engine = engine;

   if (!draw_init(draw))
      goto fail;

   return draw;

fail:
   draw_destroy( draw );
   return NULL;
}

static void
generate_vs(struct draw_llvm *llvm,
            LLVMBuilderRef builder,
            LLVMValueRef (*outputs)[NUM_CHANNELS],
            const LLVMValueRef (*inputs)[NUM_CHANNELS],
            LLVMValueRef context_ptr,
            LLVMValueRef io)
{
   const struct tgsi_token *tokens = llvm->draw->vs.vertex_shader->state.tokens;
   struct lp_type vs_type;
   LLVMValueRef consts_ptr = draw_jit_context_vs_constants(builder, context_ptr);

   memset(&vs_type, 0, sizeof vs_type);
   vs_type.floating = TRUE; /* floating point values */
   vs_type.sign = TRUE;     /* values are signed */
   vs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   vs_type.width = 32;      /* 32-bit float */
   vs_type.length = 4;      /* 4 elements per vector */
#if 0
   num_vs = 4;              /* number of vertices per block */
#endif

   lp_build_tgsi_soa(builder,
                     tokens,
                     vs_type,
                     NULL /*struct lp_build_mask_context *mask*/,
                     consts_ptr,
                     NULL /*pos*/,
                     inputs,
                     outputs,
                     NULL/*sampler*/);
}


static void print_vectorf(LLVMBuilderRef builder,
                         LLVMValueRef vec)
{
   LLVMValueRef val[4];
   val[0] = LLVMBuildExtractElement(builder, vec,
                                    LLVMConstInt(LLVMInt32Type(), 0, 0), "");
   val[1] = LLVMBuildExtractElement(builder, vec,
                                    LLVMConstInt(LLVMInt32Type(), 1, 0), "");
   val[2] = LLVMBuildExtractElement(builder, vec,
                                    LLVMConstInt(LLVMInt32Type(), 2, 0), "");
   val[3] = LLVMBuildExtractElement(builder, vec,
                                    LLVMConstInt(LLVMInt32Type(), 3, 0), "");
   lp_build_printf(builder, "vector = [%f, %f, %f, %f]\n",
                   val[0], val[1], val[2], val[3]);
}

static void
generate_fetch(LLVMBuilderRef builder,
               LLVMValueRef vbuffers_ptr,
               LLVMValueRef *res,
               struct pipe_vertex_element *velem,
               struct pipe_vertex_buffer *vbuf,
               LLVMValueRef index)
{
   LLVMValueRef indices = LLVMConstInt(LLVMInt64Type(), velem->vertex_buffer_index, 0);
   LLVMValueRef vbuffer_ptr = LLVMBuildGEP(builder, vbuffers_ptr,
                                           &indices, 1, "");
   LLVMValueRef stride = LLVMBuildMul(builder,
                                      LLVMConstInt(LLVMInt32Type(), vbuf->stride, 0),
                                      index, "");

   vbuffer_ptr = LLVMBuildLoad(builder, vbuffer_ptr, "vbuffer");

   stride = LLVMBuildAdd(builder, stride,
                         LLVMConstInt(LLVMInt32Type(), vbuf->buffer_offset, 0),
                         "");
   stride = LLVMBuildAdd(builder, stride,
                         LLVMConstInt(LLVMInt32Type(), velem->src_offset, 0),
                         "");

   /*lp_build_printf(builder, "vbuf index = %d, stride is %d\n", indices, stride);*/
   vbuffer_ptr = LLVMBuildGEP(builder, vbuffer_ptr, &stride, 1, "");

   *res = draw_llvm_translate_from(builder, vbuffer_ptr, velem->src_format);
}

static LLVMValueRef
aos_to_soa(LLVMBuilderRef builder,
           LLVMValueRef val0,
           LLVMValueRef val1,
           LLVMValueRef val2,
           LLVMValueRef val3,
           LLVMValueRef channel)
{
   LLVMValueRef ex, res;

   ex = LLVMBuildExtractElement(builder, val0,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                LLVMConstNull(LLVMTypeOf(val0)),
                                ex,
                                LLVMConstInt(LLVMInt32Type(), 0, 0),
                                "");

   ex = LLVMBuildExtractElement(builder, val1,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                LLVMConstInt(LLVMInt32Type(), 1, 0),
                                "");

   ex = LLVMBuildExtractElement(builder, val2,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                LLVMConstInt(LLVMInt32Type(), 2, 0),
                                "");

   ex = LLVMBuildExtractElement(builder, val3,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                LLVMConstInt(LLVMInt32Type(), 3, 0),
                                "");

   return res;
}

static void
convert_to_soa(LLVMBuilderRef builder,
               LLVMValueRef (*aos)[NUM_CHANNELS],
               LLVMValueRef (*soa)[NUM_CHANNELS],
               int num_attribs)
{
   int i;

   debug_assert(NUM_CHANNELS == 4);

   for (i = 0; i < num_attribs; ++i) {
      LLVMValueRef val0 = aos[i][0];
      LLVMValueRef val1 = aos[i][1];
      LLVMValueRef val2 = aos[i][2];
      LLVMValueRef val3 = aos[i][3];

      soa[i][0] = aos_to_soa(builder, val0, val1, val2, val3,
                             LLVMConstInt(LLVMInt32Type(), 0, 0));
      soa[i][1] = aos_to_soa(builder, val0, val1, val2, val3,
                             LLVMConstInt(LLVMInt32Type(), 1, 0));
      soa[i][2] = aos_to_soa(builder, val0, val1, val2, val3,
                             LLVMConstInt(LLVMInt32Type(), 2, 0));
      soa[i][3] = aos_to_soa(builder, val0, val1, val2, val3,
                             LLVMConstInt(LLVMInt32Type(), 3, 0));
   }
}

void
draw_llvm_generate(struct draw_llvm *llvm)
{
   LLVMTypeRef arg_types[6];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef function;
   LLVMValueRef start, end, count, stride, step;
   LLVMValueRef io_ptr, vbuffers_ptr;
   struct draw_context *draw = llvm->draw;
   unsigned i, j;
   struct lp_build_context bld;
   struct lp_build_loop_state lp_loop;
   struct lp_type vs_type = lp_type_float_vec(32);
   const int max_vertices = 4;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];

   arg_types[0] = llvm->context_ptr_type;           /* context */
   arg_types[1] = llvm->vertex_header_ptr_type;     /* vertex_header */
   arg_types[2] = llvm->buffer_ptr_type;            /* vbuffers */
   arg_types[3] = LLVMInt32Type();                  /* start */
   arg_types[4] = LLVMInt32Type();                  /* count */
   arg_types[5] = LLVMInt32Type();                  /* stride */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   function = LLVMAddFunction(llvm->module, "draw_llvm_shader", func_type);
   LLVMSetFunctionCallConv(function, LLVMCCallConv);
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(function, i), LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(function, 0);
   io_ptr       = LLVMGetParam(function, 1);
   vbuffers_ptr = LLVMGetParam(function, 2);
   start        = LLVMGetParam(function, 3);
   count        = LLVMGetParam(function, 4);
   stride       = LLVMGetParam(function, 5);

   lp_build_name(context_ptr, "context");
   lp_build_name(io_ptr, "io");
   lp_build_name(vbuffers_ptr, "vbuffers");
   lp_build_name(start, "start");
   lp_build_name(count, "count");
   lp_build_name(stride, "stride");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_context_init(&bld, builder, vs_type);

   end = lp_build_add(&bld, start, count);

   step = LLVMConstInt(LLVMInt32Type(), max_vertices, 0);

   lp_build_loop_begin(builder, start, &lp_loop);
   {
      LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
      LLVMValueRef aos_attribs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
      LLVMValueRef io = LLVMBuildGEP(builder, io_ptr, &lp_loop.counter, 1, "");
      const LLVMValueRef (*ptr_aos)[NUM_CHANNELS];

      for (i = 0; i < NUM_CHANNELS; ++i) {
         LLVMValueRef true_index = LLVMBuildAdd(
            builder,
            lp_loop.counter,
            LLVMConstInt(LLVMInt32Type(), i, 0), "");
         for (j = 0; j < draw->pt.nr_vertex_elements; ++j) {
            struct pipe_vertex_element *velem = &draw->pt.vertex_element[j];
            struct pipe_vertex_buffer *vbuf = &draw->pt.vertex_buffer[
               velem->vertex_buffer_index];
            generate_fetch(builder, vbuffers_ptr,
                           &aos_attribs[j][i], velem, vbuf, true_index);
            /*print_vectorf(builder, aos_attribs[j][i]);*/
         }
      }
      convert_to_soa(builder, aos_attribs, inputs,
                     draw->pt.nr_vertex_elements);

      ptr_aos = (const LLVMValueRef (*)[NUM_CHANNELS]) inputs;
      generate_vs(llvm,
                  builder,
                  outputs,
                  ptr_aos,
                  context_ptr,
                  io);
   }
   lp_build_loop_end(builder, end, step, &lp_loop);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);

   /*
    * Translate the LLVM IR into machine code.
    */

#ifdef DEBUG
   if(LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
      LLVMDumpValue(function);
      assert(0);
   }
#endif

   LLVMRunFunctionPassManager(llvm->pass, function);

   if (1) {
      LLVMDumpValue(function);
      debug_printf("\n");
   }
   llvm->jit_func = (draw_jit_vert_func)LLVMGetPointerToGlobal(llvm->draw->engine, function);

   if (1)
      lp_disassemble(llvm->jit_func);
}
