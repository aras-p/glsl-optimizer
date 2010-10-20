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

#include "draw_llvm.h"

#include "draw_context.h"
#include "draw_vs.h"

#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_swizzle.h"
#include "gallivm/lp_bld_struct.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_printf.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_init.h"

#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"

#include "util/u_cpu_detect.h"
#include "util/u_math.h"
#include "util/u_pointer.h"
#include "util/u_string.h"

#include <llvm-c/Transforms/Scalar.h>

#define DEBUG_STORE 0

/* generates the draw jit function */
static void
draw_llvm_generate(struct draw_llvm *llvm, struct draw_llvm_variant *var);
static void
draw_llvm_generate_elts(struct draw_llvm *llvm, struct draw_llvm_variant *var);

static void
init_globals(struct draw_llvm *llvm)
{
   LLVMTypeRef texture_type;

   /* struct draw_jit_texture */
   {
      LLVMTypeRef elem_types[DRAW_JIT_TEXTURE_NUM_FIELDS];

      elem_types[DRAW_JIT_TEXTURE_WIDTH]  = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_HEIGHT] = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_DEPTH] = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_LAST_LEVEL] = LLVMInt32Type();
      elem_types[DRAW_JIT_TEXTURE_ROW_STRIDE] =
         LLVMArrayType(LLVMInt32Type(), PIPE_MAX_TEXTURE_LEVELS);
      elem_types[DRAW_JIT_TEXTURE_IMG_STRIDE] =
         LLVMArrayType(LLVMInt32Type(), PIPE_MAX_TEXTURE_LEVELS);
      elem_types[DRAW_JIT_TEXTURE_DATA] =
         LLVMArrayType(LLVMPointerType(LLVMInt8Type(), 0),
                       PIPE_MAX_TEXTURE_LEVELS);
      elem_types[DRAW_JIT_TEXTURE_MIN_LOD] = LLVMFloatType();
      elem_types[DRAW_JIT_TEXTURE_MAX_LOD] = LLVMFloatType();
      elem_types[DRAW_JIT_TEXTURE_LOD_BIAS] = LLVMFloatType();
      elem_types[DRAW_JIT_TEXTURE_BORDER_COLOR] = 
         LLVMArrayType(LLVMFloatType(), 4);

      texture_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, width,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_WIDTH);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, height,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_HEIGHT);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, depth,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_DEPTH);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, last_level,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_LAST_LEVEL);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, row_stride,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_ROW_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, img_stride,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_IMG_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, data,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_DATA);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, min_lod,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_MIN_LOD);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, max_lod,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_MAX_LOD);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, lod_bias,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_LOD_BIAS);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, border_color,
                             llvm->target, texture_type,
                             DRAW_JIT_TEXTURE_BORDER_COLOR);
      LP_CHECK_STRUCT_SIZE(struct draw_jit_texture,
                           llvm->target, texture_type);

      LLVMAddTypeName(llvm->module, "texture", texture_type);
   }


   /* struct draw_jit_context */
   {
      LLVMTypeRef elem_types[5];
      LLVMTypeRef context_type;

      elem_types[0] = LLVMPointerType(LLVMFloatType(), 0); /* vs_constants */
      elem_types[1] = LLVMPointerType(LLVMFloatType(), 0); /* gs_constants */
      elem_types[2] = LLVMPointerType(LLVMArrayType(LLVMArrayType(LLVMFloatType(), 4), 12), 0); /* planes */
      elem_types[3] = LLVMPointerType(LLVMFloatType(), 0); /* viewport */
      elem_types[4] = LLVMArrayType(texture_type,
                                    PIPE_MAX_VERTEX_SAMPLERS); /* textures */

      context_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, vs_constants,
                             llvm->target, context_type, 0);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, gs_constants,
                             llvm->target, context_type, 1);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, planes,
                             llvm->target, context_type, 2);
      LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, textures,
                             llvm->target, context_type,
                             DRAW_JIT_CTX_TEXTURES);
      LP_CHECK_STRUCT_SIZE(struct draw_jit_context,
                           llvm->target, context_type);

      LLVMAddTypeName(llvm->module, "draw_jit_context", context_type);

      llvm->context_ptr_type = LLVMPointerType(context_type, 0);
   }
   {
      LLVMTypeRef buffer_ptr = LLVMPointerType(LLVMIntType(8), 0);
      llvm->buffer_ptr_type = LLVMPointerType(buffer_ptr, 0);
   }
   /* struct pipe_vertex_buffer */
   {
      LLVMTypeRef elem_types[4];
      LLVMTypeRef vb_type;

      elem_types[0] = LLVMInt32Type();
      elem_types[1] = LLVMInt32Type();
      elem_types[2] = LLVMInt32Type();
      elem_types[3] = LLVMPointerType(LLVMOpaqueType(), 0); /* vs_constants */

      vb_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct pipe_vertex_buffer, stride,
                             llvm->target, vb_type, 0);
      LP_CHECK_MEMBER_OFFSET(struct pipe_vertex_buffer, buffer_offset,
                             llvm->target, vb_type, 2);
      LP_CHECK_STRUCT_SIZE(struct pipe_vertex_buffer,
                           llvm->target, vb_type);

      LLVMAddTypeName(llvm->module, "pipe_vertex_buffer", vb_type);

      llvm->vb_ptr_type = LLVMPointerType(vb_type, 0);
   }
}

static LLVMTypeRef
create_vertex_header(struct draw_llvm *llvm, int data_elems)
{
   /* struct vertex_header */
   LLVMTypeRef elem_types[3];
   LLVMTypeRef vertex_header;
   char struct_name[24];

   util_snprintf(struct_name, 23, "vertex_header%d", data_elems);

   elem_types[0]  = LLVMIntType(32);
   elem_types[1]  = LLVMArrayType(LLVMFloatType(), 4);
   elem_types[2]  = LLVMArrayType(elem_types[1], data_elems);

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

   LLVMAddTypeName(llvm->module, struct_name, vertex_header);

   return LLVMPointerType(vertex_header, 0);
}

struct draw_llvm *
draw_llvm_create(struct draw_context *draw)
{
   struct draw_llvm *llvm;

   llvm = CALLOC_STRUCT( draw_llvm );
   if (!llvm)
      return NULL;

   llvm->draw = draw;
   llvm->engine = draw->engine;

   debug_assert(llvm->engine);

   llvm->module = LLVMModuleCreateWithName("draw_llvm");
   llvm->provider = LLVMCreateModuleProviderForExistingModule(llvm->module);

   LLVMAddModuleProvider(llvm->engine, llvm->provider);

   llvm->target = LLVMGetExecutionEngineTargetData(llvm->engine);

   llvm->pass = LLVMCreateFunctionPassManager(llvm->provider);
   LLVMAddTargetData(llvm->target, llvm->pass);

   if ((gallivm_debug & GALLIVM_DEBUG_NO_OPT) == 0) {
      /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
       * but there are more on SVN. */
      /* TODO: Add more passes */

      LLVMAddCFGSimplificationPass(llvm->pass);

      if (HAVE_LLVM >= 0x207 && sizeof(void*) == 4) {
         /* For LLVM >= 2.7 and 32-bit build, use this order of passes to
          * avoid generating bad code.
          * Test with piglit glsl-vs-sqrt-zero test.
          */
         LLVMAddConstantPropagationPass(llvm->pass);
         LLVMAddPromoteMemoryToRegisterPass(llvm->pass);
      }
      else {
         LLVMAddPromoteMemoryToRegisterPass(llvm->pass);
         LLVMAddConstantPropagationPass(llvm->pass);
      }

      if(util_cpu_caps.has_sse4_1) {
         /* FIXME: There is a bug in this pass, whereby the combination of fptosi
          * and sitofp (necessary for trunc/floor/ceil/round implementation)
          * somehow becomes invalid code.
          */
         LLVMAddInstructionCombiningPass(llvm->pass);
      }
      LLVMAddGVNPass(llvm->pass);
   } else {
      /* We need at least this pass to prevent the backends to fail in
       * unexpected ways.
       */
      LLVMAddPromoteMemoryToRegisterPass(llvm->pass);
   }

   init_globals(llvm);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      LLVMDumpModule(llvm->module);
   }

   llvm->nr_variants = 0;
   make_empty_list(&llvm->vs_variants_list);

   return llvm;
}

void
draw_llvm_destroy(struct draw_llvm *llvm)
{
   LLVMDisposePassManager(llvm->pass);

   FREE(llvm);
}

struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm,
			 unsigned num_inputs,
			 const struct draw_llvm_variant_key *key)
{
   struct draw_llvm_variant *variant;
   struct llvm_vertex_shader *shader =
      llvm_vertex_shader(llvm->draw->vs.vertex_shader);

   variant = MALLOC(sizeof *variant +
		    shader->variant_key_size -
		    sizeof variant->key);
   if (variant == NULL)
      return NULL;

   variant->llvm = llvm;

   memcpy(&variant->key, key, shader->variant_key_size);

   llvm->vertex_header_ptr_type = create_vertex_header(llvm, num_inputs);

   draw_llvm_generate(llvm, variant);
   draw_llvm_generate_elts(llvm, variant);

   variant->shader = shader;
   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}

static void
generate_vs(struct draw_llvm *llvm,
            LLVMBuilderRef builder,
            LLVMValueRef (*outputs)[NUM_CHANNELS],
            const LLVMValueRef (*inputs)[NUM_CHANNELS],
            LLVMValueRef context_ptr,
            struct lp_build_sampler_soa *draw_sampler)
{
   const struct tgsi_token *tokens = llvm->draw->vs.vertex_shader->state.tokens;
   struct lp_type vs_type;
   LLVMValueRef consts_ptr = draw_jit_context_vs_constants(builder, context_ptr);
   struct lp_build_sampler_soa *sampler = 0;

   memset(&vs_type, 0, sizeof vs_type);
   vs_type.floating = TRUE; /* floating point values */
   vs_type.sign = TRUE;     /* values are signed */
   vs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   vs_type.width = 32;      /* 32-bit float */
   vs_type.length = 4;      /* 4 elements per vector */
#if 0
   num_vs = 4;              /* number of vertices per block */
#endif

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      tgsi_dump(tokens, 0);
   }

   if (llvm->draw->num_sampler_views &&
       llvm->draw->num_samplers)
      sampler = draw_sampler;

   lp_build_tgsi_soa(builder,
                     tokens,
                     vs_type,
                     NULL /*struct lp_build_mask_context *mask*/,
                     consts_ptr,
                     NULL /*pos*/,
                     inputs,
                     outputs,
                     sampler,
                     &llvm->draw->vs.vertex_shader->info);
}

#if DEBUG_STORE
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
#endif

static void
generate_fetch(LLVMBuilderRef builder,
               LLVMValueRef vbuffers_ptr,
               LLVMValueRef *res,
               struct pipe_vertex_element *velem,
               LLVMValueRef vbuf,
               LLVMValueRef index,
               LLVMValueRef instance_id)
{
   LLVMValueRef indices = LLVMConstInt(LLVMInt64Type(), velem->vertex_buffer_index, 0);
   LLVMValueRef vbuffer_ptr = LLVMBuildGEP(builder, vbuffers_ptr,
                                           &indices, 1, "");
   LLVMValueRef vb_stride = draw_jit_vbuffer_stride(builder, vbuf);
   LLVMValueRef vb_max_index = draw_jit_vbuffer_max_index(builder, vbuf);
   LLVMValueRef vb_buffer_offset = draw_jit_vbuffer_offset(builder, vbuf);
   LLVMValueRef cond;
   LLVMValueRef stride;

   if (velem->instance_divisor) {
      /* array index = instance_id / instance_divisor */
      index = LLVMBuildUDiv(builder, instance_id,
                            LLVMConstInt(LLVMInt32Type(), velem->instance_divisor, 0),
                            "instance_divisor");
   }

   /* limit index to min(inex, vb_max_index) */
   cond = LLVMBuildICmp(builder, LLVMIntULE, index, vb_max_index, "");
   index = LLVMBuildSelect(builder, cond, index, vb_max_index, "");

   stride = LLVMBuildMul(builder, vb_stride, index, "");

   vbuffer_ptr = LLVMBuildLoad(builder, vbuffer_ptr, "vbuffer");

   stride = LLVMBuildAdd(builder, stride,
                         vb_buffer_offset,
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
soa_to_aos(LLVMBuilderRef builder,
           LLVMValueRef soa[NUM_CHANNELS],
           LLVMValueRef aos[NUM_CHANNELS])
{
   LLVMValueRef comp;
   int i = 0;

   debug_assert(NUM_CHANNELS == 4);

   aos[0] = LLVMConstNull(LLVMTypeOf(soa[0]));
   aos[1] = aos[2] = aos[3] = aos[0];

   for (i = 0; i < NUM_CHANNELS; ++i) {
      LLVMValueRef channel = LLVMConstInt(LLVMInt32Type(), i, 0);

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     LLVMConstInt(LLVMInt32Type(), 0, 0), "");
      aos[0] = LLVMBuildInsertElement(builder, aos[0], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     LLVMConstInt(LLVMInt32Type(), 1, 0), "");
      aos[1] = LLVMBuildInsertElement(builder, aos[1], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     LLVMConstInt(LLVMInt32Type(), 2, 0), "");
      aos[2] = LLVMBuildInsertElement(builder, aos[2], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     LLVMConstInt(LLVMInt32Type(), 3, 0), "");
      aos[3] = LLVMBuildInsertElement(builder, aos[3], comp, channel, "");

   }
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

static void
store_aos(LLVMBuilderRef builder,
          LLVMValueRef io_ptr,
          LLVMValueRef index,
          LLVMValueRef value,
          LLVMValueRef clipmask)
{
   LLVMValueRef id_ptr = draw_jit_header_id(builder, io_ptr);
   LLVMValueRef data_ptr = draw_jit_header_data(builder, io_ptr);
   LLVMValueRef indices[3];
   LLVMValueRef val, shift;

   indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indices[1] = index;
   indices[2] = LLVMConstInt(LLVMInt32Type(), 0, 0);

   /* initialize vertex id:16 = 0xffff, pad:3 = 0, edgeflag:1 = 1 */
   val = LLVMConstInt(LLVMInt32Type(), 0xffff1, 0); 
   shift  = LLVMConstInt(LLVMInt32Type(), 12, 0);          
   val = LLVMBuildShl(builder, val, shift, "");
   /* add clipmask:12 */   
   val = LLVMBuildOr(builder, val, clipmask, "");               

   /* store vertex header */
   LLVMBuildStore(builder, val, id_ptr);


#if DEBUG_STORE
   lp_build_printf(builder, "    ---- %p storing attribute %d (io = %p)\n", data_ptr, index, io_ptr);
#endif
#if 0
   /*lp_build_printf(builder, " ---- %p storing at %d (%p)  ", io_ptr, index, data_ptr);
     print_vectorf(builder, value);*/
   data_ptr = LLVMBuildBitCast(builder, data_ptr,
                               LLVMPointerType(LLVMArrayType(LLVMVectorType(LLVMFloatType(), 4), 0), 0),
                               "datavec");
   data_ptr = LLVMBuildGEP(builder, data_ptr, indices, 2, "");

   LLVMBuildStore(builder, value, data_ptr);
#else
   {
      LLVMValueRef x, y, z, w;
      LLVMValueRef idx0, idx1, idx2, idx3;
      LLVMValueRef gep0, gep1, gep2, gep3;
      data_ptr = LLVMBuildGEP(builder, data_ptr, indices, 3, "");

      idx0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
      idx1 = LLVMConstInt(LLVMInt32Type(), 1, 0);
      idx2 = LLVMConstInt(LLVMInt32Type(), 2, 0);
      idx3 = LLVMConstInt(LLVMInt32Type(), 3, 0);

      x = LLVMBuildExtractElement(builder, value,
                                  idx0, "");
      y = LLVMBuildExtractElement(builder, value,
                                  idx1, "");
      z = LLVMBuildExtractElement(builder, value,
                                  idx2, "");
      w = LLVMBuildExtractElement(builder, value,
                                  idx3, "");

      gep0 = LLVMBuildGEP(builder, data_ptr, &idx0, 1, "");
      gep1 = LLVMBuildGEP(builder, data_ptr, &idx1, 1, "");
      gep2 = LLVMBuildGEP(builder, data_ptr, &idx2, 1, "");
      gep3 = LLVMBuildGEP(builder, data_ptr, &idx3, 1, "");

      /*lp_build_printf(builder, "##### x = %f (%p), y = %f (%p), z = %f (%p), w = %f (%p)\n",
        x, gep0, y, gep1, z, gep2, w, gep3);*/
      LLVMBuildStore(builder, x, gep0);
      LLVMBuildStore(builder, y, gep1);
      LLVMBuildStore(builder, z, gep2);
      LLVMBuildStore(builder, w, gep3);
   }
#endif
}

static void
store_aos_array(LLVMBuilderRef builder,
                LLVMValueRef io_ptr,
                LLVMValueRef aos[NUM_CHANNELS],
                int attrib,
                int num_outputs,
                LLVMValueRef clipmask)
{
   LLVMValueRef attr_index = LLVMConstInt(LLVMInt32Type(), attrib, 0);
   LLVMValueRef ind0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
   LLVMValueRef ind1 = LLVMConstInt(LLVMInt32Type(), 1, 0);
   LLVMValueRef ind2 = LLVMConstInt(LLVMInt32Type(), 2, 0);
   LLVMValueRef ind3 = LLVMConstInt(LLVMInt32Type(), 3, 0);
   LLVMValueRef io0_ptr, io1_ptr, io2_ptr, io3_ptr;
   LLVMValueRef clipmask0, clipmask1, clipmask2, clipmask3;
   
   debug_assert(NUM_CHANNELS == 4);

   io0_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind0, 1, "");
   io1_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind1, 1, "");
   io2_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind2, 1, "");
   io3_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind3, 1, "");

   clipmask0 = LLVMBuildExtractElement(builder, clipmask,
                                       ind0, "");
   clipmask1 = LLVMBuildExtractElement(builder, clipmask,
                                       ind1, "");
   clipmask2 = LLVMBuildExtractElement(builder, clipmask,
                                       ind2, "");
   clipmask3 = LLVMBuildExtractElement(builder, clipmask,
                                       ind3, "");

#if DEBUG_STORE
   lp_build_printf(builder, "io = %p, indexes[%d, %d, %d, %d]\n, clipmask0 = %x, clipmask1 = %x, clipmask2 = %x, clipmask3 = %x\n",
                   io_ptr, ind0, ind1, ind2, ind3, clipmask0, clipmask1, clipmask2, clipmask3);
#endif
   /* store for each of the 4 vertices */
   store_aos(builder, io0_ptr, attr_index, aos[0], clipmask0);
   store_aos(builder, io1_ptr, attr_index, aos[1], clipmask1);
   store_aos(builder, io2_ptr, attr_index, aos[2], clipmask2);
   store_aos(builder, io3_ptr, attr_index, aos[3], clipmask3);
}

static void
convert_to_aos(LLVMBuilderRef builder,
               LLVMValueRef io,
               LLVMValueRef (*outputs)[NUM_CHANNELS],
               LLVMValueRef clipmask,
               int num_outputs,
               int max_vertices)
{
   unsigned chan, attrib;

#if DEBUG_STORE
   lp_build_printf(builder, "   # storing begin\n");
#endif
   for (attrib = 0; attrib < num_outputs; ++attrib) {
      LLVMValueRef soa[4];
      LLVMValueRef aos[4];
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(outputs[attrib][chan]) {
            LLVMValueRef out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
            lp_build_name(out, "output%u.%c", attrib, "xyzw"[chan]);
            /*lp_build_printf(builder, "output %d : %d ",
                            LLVMConstInt(LLVMInt32Type(), attrib, 0),
                            LLVMConstInt(LLVMInt32Type(), chan, 0));
              print_vectorf(builder, out);*/
            soa[chan] = out;
         } else
            soa[chan] = 0;
      }
      soa_to_aos(builder, soa, aos);
      store_aos_array(builder,
                      io,
                      aos,
                      attrib,
                      num_outputs,
                      clipmask);
   }
#if DEBUG_STORE
   lp_build_printf(builder, "   # storing end\n");
#endif
}

/*
 * Stores original vertex positions in clip coordinates
 * There is probably a more efficient way to do this, 4 floats at once
 * rather than extracting each element one by one.
 */
static void
store_clip(LLVMBuilderRef builder,
           LLVMValueRef io_ptr,           
           LLVMValueRef (*outputs)[NUM_CHANNELS])
{
   LLVMValueRef out[4];
   LLVMValueRef indices[2]; 
   LLVMValueRef io0_ptr, io1_ptr, io2_ptr, io3_ptr;
   LLVMValueRef clip_ptr0, clip_ptr1, clip_ptr2, clip_ptr3;
   LLVMValueRef clip0_ptr, clip1_ptr, clip2_ptr, clip3_ptr;    
   LLVMValueRef out0elem, out1elem, out2elem, out3elem;
   int i;

   LLVMValueRef ind0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
   LLVMValueRef ind1 = LLVMConstInt(LLVMInt32Type(), 1, 0);
   LLVMValueRef ind2 = LLVMConstInt(LLVMInt32Type(), 2, 0);
   LLVMValueRef ind3 = LLVMConstInt(LLVMInt32Type(), 3, 0);
   
   indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indices[1] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   
   out[0] = LLVMBuildLoad(builder, outputs[0][0], ""); /*x0 x1 x2 x3*/
   out[1] = LLVMBuildLoad(builder, outputs[0][1], ""); /*y0 y1 y2 y3*/
   out[2] = LLVMBuildLoad(builder, outputs[0][2], ""); /*z0 z1 z2 z3*/
   out[3] = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/  

   io0_ptr = LLVMBuildGEP(builder, io_ptr, &ind0, 1, "");
   io1_ptr = LLVMBuildGEP(builder, io_ptr, &ind1, 1, "");
   io2_ptr = LLVMBuildGEP(builder, io_ptr, &ind2, 1, "");
   io3_ptr = LLVMBuildGEP(builder, io_ptr, &ind3, 1, "");

   clip_ptr0 = draw_jit_header_clip(builder, io0_ptr);
   clip_ptr1 = draw_jit_header_clip(builder, io1_ptr);
   clip_ptr2 = draw_jit_header_clip(builder, io2_ptr);
   clip_ptr3 = draw_jit_header_clip(builder, io3_ptr);

   for (i = 0; i<4; i++){
      clip0_ptr = LLVMBuildGEP(builder, clip_ptr0,
                               indices, 2, ""); //x0
      clip1_ptr = LLVMBuildGEP(builder, clip_ptr1,
                               indices, 2, ""); //x1
      clip2_ptr = LLVMBuildGEP(builder, clip_ptr2,
                               indices, 2, ""); //x2
      clip3_ptr = LLVMBuildGEP(builder, clip_ptr3,
                               indices, 2, ""); //x3

      out0elem = LLVMBuildExtractElement(builder, out[i],
                                         ind0, ""); //x0
      out1elem = LLVMBuildExtractElement(builder, out[i],
                                         ind1, ""); //x1
      out2elem = LLVMBuildExtractElement(builder, out[i],
                                         ind2, ""); //x2
      out3elem = LLVMBuildExtractElement(builder, out[i],
                                         ind3, ""); //x3
  
      LLVMBuildStore(builder, out0elem, clip0_ptr);
      LLVMBuildStore(builder, out1elem, clip1_ptr);
      LLVMBuildStore(builder, out2elem, clip2_ptr);
      LLVMBuildStore(builder, out3elem, clip3_ptr);

      indices[1]= LLVMBuildAdd(builder, indices[1], ind1, "");
   }

}

/* Equivalent of _mm_set1_ps(a)
 */
static LLVMValueRef vec4f_from_scalar(LLVMBuilderRef bld,
				      LLVMValueRef a,
				      const char *name)
{
   LLVMValueRef res = LLVMGetUndef(LLVMVectorType(LLVMFloatType(), 4));
   int i;

   for(i = 0; i < 4; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(bld, res, a, index, i == 3 ? name : "");
   }

   return res;
}

/*
 * Transforms the outputs for viewport mapping
 */
static void
generate_viewport(struct draw_llvm *llvm,
                  LLVMBuilderRef builder,
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  LLVMValueRef context_ptr)
{
   int i;
   struct lp_type f32_type = lp_type_float_vec(32);
   LLVMValueRef out3 = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/   
   LLVMValueRef const1 = lp_build_const_vec(f32_type, 1.0);       /*1.0 1.0 1.0 1.0*/ 
   LLVMValueRef vp_ptr = draw_jit_context_viewport(builder, context_ptr);

   /* for 1/w convention*/
   out3 = LLVMBuildFDiv(builder, const1, out3, "");
   LLVMBuildStore(builder, out3, outputs[0][3]);
  
   /* Viewport Mapping */
   for (i=0; i<3; i++){
      LLVMValueRef out = LLVMBuildLoad(builder, outputs[0][i], ""); /*x0 x1 x2 x3*/
      LLVMValueRef scale;
      LLVMValueRef trans;
      LLVMValueRef scale_i;
      LLVMValueRef trans_i;
      LLVMValueRef index;
      
      index = LLVMConstInt(LLVMInt32Type(), i, 0);
      scale_i = LLVMBuildGEP(builder, vp_ptr, &index, 1, "");

      index = LLVMConstInt(LLVMInt32Type(), i+4, 0);
      trans_i = LLVMBuildGEP(builder, vp_ptr, &index, 1, "");

      scale = vec4f_from_scalar(builder, LLVMBuildLoad(builder, scale_i, ""), "scale");
      trans = vec4f_from_scalar(builder, LLVMBuildLoad(builder, trans_i, ""), "trans");

      /* divide by w */
      out = LLVMBuildMul(builder, out, out3, "");
      /* mult by scale */
      out = LLVMBuildMul(builder, out, scale, "");
      /* add translation */
      out = LLVMBuildAdd(builder, out, trans, "");

      /* store transformed outputs */
      LLVMBuildStore(builder, out, outputs[0][i]);
   }
   
}


/*
 * Returns clipmask as 4xi32 bitmask for the 4 vertices
 */
static LLVMValueRef 
generate_clipmask(LLVMBuilderRef builder,
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  boolean clip_xy,
                  boolean clip_z,
                  boolean clip_user,
                  boolean clip_halfz,
                  unsigned nr,
                  LLVMValueRef context_ptr)
{
   LLVMValueRef mask; /* stores the <4xi32> clipmasks */     
   LLVMValueRef test, temp; 
   LLVMValueRef zero, shift;
   LLVMValueRef pos_x, pos_y, pos_z, pos_w;
   LLVMValueRef plane1, planes, plane_ptr, sum;

   unsigned i;

   struct lp_type f32_type = lp_type_float_vec(32); 

   mask = lp_build_const_int_vec(lp_type_int_vec(32), 0);
   temp = lp_build_const_int_vec(lp_type_int_vec(32), 0);
   zero = lp_build_const_vec(f32_type, 0);                    /* 0.0f 0.0f 0.0f 0.0f */
   shift = lp_build_const_int_vec(lp_type_int_vec(32), 1);    /* 1 1 1 1 */

   /* Assuming position stored at output[0] */
   pos_x = LLVMBuildLoad(builder, outputs[0][0], ""); /*x0 x1 x2 x3*/
   pos_y = LLVMBuildLoad(builder, outputs[0][1], ""); /*y0 y1 y2 y3*/
   pos_z = LLVMBuildLoad(builder, outputs[0][2], ""); /*z0 z1 z2 z3*/
   pos_w = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/   

   /* Cliptest, for hardwired planes */
   if (clip_xy){
      /* plane 1 */
      test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, pos_x , pos_w);
      temp = shift;
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = test;
   
      /* plane 2 */
      test = LLVMBuildFAdd(builder, pos_x, pos_w, "");
      test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, zero, test);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   
      /* plane 3 */
      test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, pos_y, pos_w);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");

      /* plane 4 */
      test = LLVMBuildFAdd(builder, pos_y, pos_w, "");
      test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, zero, test);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   }

   if (clip_z){
      temp = lp_build_const_int_vec(lp_type_int_vec(32), 16);
      if (clip_halfz){
         /* plane 5 */
         test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, zero, pos_z);
         test = LLVMBuildAnd(builder, test, temp, ""); 
         mask = LLVMBuildOr(builder, mask, test, "");
      }  
      else{
         /* plane 5 */
         test = LLVMBuildFAdd(builder, pos_z, pos_w, "");
         test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, zero, test);
         test = LLVMBuildAnd(builder, test, temp, ""); 
         mask = LLVMBuildOr(builder, mask, test, "");
      }
      /* plane 6 */
      test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, pos_z, pos_w);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   }   

   if (clip_user){
      LLVMValueRef planes_ptr = draw_jit_context_planes(builder, context_ptr);
      LLVMValueRef indices[3];
      temp = lp_build_const_int_vec(lp_type_int_vec(32), 32);

      /* userclip planes */
      for (i = 6; i < nr; i++) {
         indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
         indices[1] = LLVMConstInt(LLVMInt32Type(), i, 0);

         indices[2] = LLVMConstInt(LLVMInt32Type(), 0, 0);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_x");
         planes = vec4f_from_scalar(builder, plane1, "plane4_x");
         sum = LLVMBuildMul(builder, planes, pos_x, "");

         indices[2] = LLVMConstInt(LLVMInt32Type(), 1, 0);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_y"); 
         planes = vec4f_from_scalar(builder, plane1, "plane4_y");
         test = LLVMBuildMul(builder, planes, pos_y, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");
         
         indices[2] = LLVMConstInt(LLVMInt32Type(), 2, 0);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_z"); 
         planes = vec4f_from_scalar(builder, plane1, "plane4_z");
         test = LLVMBuildMul(builder, planes, pos_z, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");

         indices[2] = LLVMConstInt(LLVMInt32Type(), 3, 0);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_w"); 
         planes = vec4f_from_scalar(builder, plane1, "plane4_w");
         test = LLVMBuildMul(builder, planes, pos_w, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");

         test = lp_build_compare(builder, f32_type, PIPE_FUNC_GREATER, zero, sum);
         temp = LLVMBuildShl(builder, temp, shift, "");
         test = LLVMBuildAnd(builder, test, temp, ""); 
         mask = LLVMBuildOr(builder, mask, test, "");
      }
   }
   return mask;
}

/*
 * Returns boolean if any clipping has occurred
 * Used zero/non-zero i32 value to represent boolean 
 */
static void
clipmask_bool(LLVMBuilderRef builder, 
              LLVMValueRef clipmask,
              LLVMValueRef ret_ptr)
{
   LLVMValueRef ret = LLVMBuildLoad(builder, ret_ptr, "");   
   LLVMValueRef temp;
   int i;

   for (i=0; i<4; i++){   
      temp = LLVMBuildExtractElement(builder, clipmask,
                                     LLVMConstInt(LLVMInt32Type(), i, 0) , "");
      ret = LLVMBuildOr(builder, ret, temp, "");
   }
   
   LLVMBuildStore(builder, ret, ret_ptr);
}

static void
draw_llvm_generate(struct draw_llvm *llvm, struct draw_llvm_variant *variant)
{
   LLVMTypeRef arg_types[8];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef start, end, count, stride, step, io_itr;
   LLVMValueRef io_ptr, vbuffers_ptr, vb_ptr;
   LLVMValueRef instance_id;
   struct draw_context *draw = llvm->draw;
   unsigned i, j;
   struct lp_build_context bld;
   struct lp_build_loop_state lp_loop;
   const int max_vertices = 4;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   void *code;
   struct lp_build_sampler_soa *sampler = 0;
   LLVMValueRef ret, ret_ptr;
   boolean bypass_viewport = variant->key.bypass_viewport;
   boolean enable_cliptest = variant->key.clip_xy || 
                             variant->key.clip_z  ||
                             variant->key.clip_user;
   
   arg_types[0] = llvm->context_ptr_type;           /* context */
   arg_types[1] = llvm->vertex_header_ptr_type;     /* vertex_header */
   arg_types[2] = llvm->buffer_ptr_type;            /* vbuffers */
   arg_types[3] = LLVMInt32Type();                  /* start */
   arg_types[4] = LLVMInt32Type();                  /* count */
   arg_types[5] = LLVMInt32Type();                  /* stride */
   arg_types[6] = llvm->vb_ptr_type;                /* pipe_vertex_buffer's */
   arg_types[7] = LLVMInt32Type();                  /* instance_id */

   func_type = LLVMFunctionType(LLVMInt32Type(), arg_types, Elements(arg_types), 0);

   variant->function = LLVMAddFunction(llvm->module, "draw_llvm_shader", func_type);
   LLVMSetFunctionCallConv(variant->function, LLVMCCallConv);
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(variant->function, i), LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(variant->function, 0);
   io_ptr       = LLVMGetParam(variant->function, 1);
   vbuffers_ptr = LLVMGetParam(variant->function, 2);
   start        = LLVMGetParam(variant->function, 3);
   count        = LLVMGetParam(variant->function, 4);
   stride       = LLVMGetParam(variant->function, 5);
   vb_ptr       = LLVMGetParam(variant->function, 6);
   instance_id  = LLVMGetParam(variant->function, 7);

   lp_build_name(context_ptr, "context");
   lp_build_name(io_ptr, "io");
   lp_build_name(vbuffers_ptr, "vbuffers");
   lp_build_name(start, "start");
   lp_build_name(count, "count");
   lp_build_name(stride, "stride");
   lp_build_name(vb_ptr, "vb");
   lp_build_name(instance_id, "instance_id");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(variant->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_context_init(&bld, builder, lp_type_int(32));

   end = lp_build_add(&bld, start, count);

   step = LLVMConstInt(LLVMInt32Type(), max_vertices, 0);

   /* function will return non-zero i32 value if any clipped vertices */     
   ret_ptr = lp_build_alloca(builder, LLVMInt32Type(), "");   
   LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), ret_ptr);

   /* code generated texture sampling */
   sampler = draw_llvm_sampler_soa_create(
      draw_llvm_variant_key_samplers(&variant->key),
      context_ptr);

#if DEBUG_STORE
   lp_build_printf(builder, "start = %d, end = %d, step = %d\n",
                   start, end, step);
#endif
   lp_build_loop_begin(builder, start, &lp_loop);
   {
      LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
      LLVMValueRef aos_attribs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS] = { { 0 } };
      LLVMValueRef io;
      LLVMValueRef clipmask;   /* holds the clipmask value */
      const LLVMValueRef (*ptr_aos)[NUM_CHANNELS];

      io_itr = LLVMBuildSub(builder, lp_loop.counter, start, "");
      io = LLVMBuildGEP(builder, io_ptr, &io_itr, 1, "");
#if DEBUG_STORE
      lp_build_printf(builder, " --- io %d = %p, loop counter %d\n",
                      io_itr, io, lp_loop.counter);
#endif
      for (i = 0; i < NUM_CHANNELS; ++i) {
         LLVMValueRef true_index = LLVMBuildAdd(
            builder,
            lp_loop.counter,
            LLVMConstInt(LLVMInt32Type(), i, 0), "");
         for (j = 0; j < draw->pt.nr_vertex_elements; ++j) {
            struct pipe_vertex_element *velem = &draw->pt.vertex_element[j];
            LLVMValueRef vb_index = LLVMConstInt(LLVMInt32Type(),
                                                 velem->vertex_buffer_index,
                                                 0);
            LLVMValueRef vb = LLVMBuildGEP(builder, vb_ptr,
                                           &vb_index, 1, "");
            generate_fetch(builder, vbuffers_ptr,
                           &aos_attribs[j][i], velem, vb, true_index,
                           instance_id);
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
                  sampler);

      /* store original positions in clip before further manipulation */
      store_clip(builder, io, outputs);

      /* do cliptest */
      if (enable_cliptest){
         /* allocate clipmask, assign it integer type */
         clipmask = generate_clipmask(builder, outputs,
                                      variant->key.clip_xy,
                                      variant->key.clip_z, 
                                      variant->key.clip_user,
                                      variant->key.clip_halfz,
                                      variant->key.nr_planes,
                                      context_ptr);
         /* return clipping boolean value for function */
         clipmask_bool(builder, clipmask, ret_ptr);
      }
      else{
         clipmask = lp_build_const_int_vec(lp_type_int_vec(32), 0);    
      }
      
      /* do viewport mapping */
      if (!bypass_viewport){
         generate_viewport(llvm, builder, outputs, context_ptr);
      }

      /* store clipmask in vertex header and positions in data */
      convert_to_aos(builder, io, outputs, clipmask,
                     draw->vs.vertex_shader->info.num_outputs,
                     max_vertices);
   }

   lp_build_loop_end_cond(builder, end, step, LLVMIntUGE, &lp_loop);

   sampler->destroy(sampler);

#ifdef PIPE_ARCH_X86
   /* Avoid corrupting the FPU stack on 32bit OSes. */
   lp_build_intrinsic(builder, "llvm.x86.mmx.emms", LLVMVoidType(), NULL, 0);
#endif

   ret = LLVMBuildLoad(builder, ret_ptr,"");
   LLVMBuildRet(builder, ret);
      
   LLVMDisposeBuilder(builder);

   /*
    * Translate the LLVM IR into machine code.
    */
#ifdef DEBUG
   if(LLVMVerifyFunction(variant->function, LLVMPrintMessageAction)) {
      lp_debug_dump_value(variant->function);
      assert(0);
   }
#endif

   LLVMRunFunctionPassManager(llvm->pass, variant->function);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      lp_debug_dump_value(variant->function);
      debug_printf("\n");
   }

   code = LLVMGetPointerToGlobal(llvm->draw->engine, variant->function);
   variant->jit_func = (draw_jit_vert_func)pointer_to_func(code);

   if (gallivm_debug & GALLIVM_DEBUG_ASM) {
      lp_disassemble(code);
   }
   lp_func_delete_body(variant->function);
}


static void
draw_llvm_generate_elts(struct draw_llvm *llvm, struct draw_llvm_variant *variant)
{
   LLVMTypeRef arg_types[8];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef fetch_elts, fetch_count, stride, step, io_itr;
   LLVMValueRef io_ptr, vbuffers_ptr, vb_ptr;
   LLVMValueRef instance_id;
   struct draw_context *draw = llvm->draw;
   unsigned i, j;
   struct lp_build_context bld;
   struct lp_build_loop_state lp_loop;
   const int max_vertices = 4;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   LLVMValueRef fetch_max;
   void *code;
   struct lp_build_sampler_soa *sampler = 0;
   LLVMValueRef ret, ret_ptr;
   boolean bypass_viewport = variant->key.bypass_viewport;
   boolean enable_cliptest = variant->key.clip_xy || 
                             variant->key.clip_z  ||
                             variant->key.clip_user;
   
   arg_types[0] = llvm->context_ptr_type;               /* context */
   arg_types[1] = llvm->vertex_header_ptr_type;         /* vertex_header */
   arg_types[2] = llvm->buffer_ptr_type;                /* vbuffers */
   arg_types[3] = LLVMPointerType(LLVMInt32Type(), 0);  /* fetch_elts * */
   arg_types[4] = LLVMInt32Type();                      /* fetch_count */
   arg_types[5] = LLVMInt32Type();                      /* stride */
   arg_types[6] = llvm->vb_ptr_type;                    /* pipe_vertex_buffer's */
   arg_types[7] = LLVMInt32Type();                      /* instance_id */

   func_type = LLVMFunctionType(LLVMInt32Type(), arg_types, Elements(arg_types), 0);

   variant->function_elts = LLVMAddFunction(llvm->module, "draw_llvm_shader_elts", func_type);
   LLVMSetFunctionCallConv(variant->function_elts, LLVMCCallConv);
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(variant->function_elts, i),
                          LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(variant->function_elts, 0);
   io_ptr       = LLVMGetParam(variant->function_elts, 1);
   vbuffers_ptr = LLVMGetParam(variant->function_elts, 2);
   fetch_elts   = LLVMGetParam(variant->function_elts, 3);
   fetch_count  = LLVMGetParam(variant->function_elts, 4);
   stride       = LLVMGetParam(variant->function_elts, 5);
   vb_ptr       = LLVMGetParam(variant->function_elts, 6);
   instance_id  = LLVMGetParam(variant->function_elts, 7);

   lp_build_name(context_ptr, "context");
   lp_build_name(io_ptr, "io");
   lp_build_name(vbuffers_ptr, "vbuffers");
   lp_build_name(fetch_elts, "fetch_elts");
   lp_build_name(fetch_count, "fetch_count");
   lp_build_name(stride, "stride");
   lp_build_name(vb_ptr, "vb");
   lp_build_name(instance_id, "instance_id");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(variant->function_elts, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_context_init(&bld, builder, lp_type_int(32));

   step = LLVMConstInt(LLVMInt32Type(), max_vertices, 0);

   /* code generated texture sampling */
   sampler = draw_llvm_sampler_soa_create(
      draw_llvm_variant_key_samplers(&variant->key),
      context_ptr);

   fetch_max = LLVMBuildSub(builder, fetch_count,
                            LLVMConstInt(LLVMInt32Type(), 1, 0),
                            "fetch_max");

   /* function returns non-zero i32 value if any clipped vertices */
   ret_ptr = lp_build_alloca(builder, LLVMInt32Type(), ""); 
   LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), ret_ptr);

   lp_build_loop_begin(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), &lp_loop);
   {
      LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
      LLVMValueRef aos_attribs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS] = { { 0 } };
      LLVMValueRef io;
      LLVMValueRef clipmask;   /* holds the clipmask value */
      const LLVMValueRef (*ptr_aos)[NUM_CHANNELS];

      io_itr = lp_loop.counter;
      io = LLVMBuildGEP(builder, io_ptr, &io_itr, 1, "");
#if DEBUG_STORE
      lp_build_printf(builder, " --- io %d = %p, loop counter %d\n",
                      io_itr, io, lp_loop.counter);
#endif
      for (i = 0; i < NUM_CHANNELS; ++i) {
         LLVMValueRef true_index = LLVMBuildAdd(
            builder,
            lp_loop.counter,
            LLVMConstInt(LLVMInt32Type(), i, 0), "");
         LLVMValueRef fetch_ptr;

         /* make sure we're not out of bounds which can happen
          * if fetch_count % 4 != 0, because on the last iteration
          * a few of the 4 vertex fetches will be out of bounds */
         true_index = lp_build_min(&bld, true_index, fetch_max);

         fetch_ptr = LLVMBuildGEP(builder, fetch_elts,
                                  &true_index, 1, "");
         true_index = LLVMBuildLoad(builder, fetch_ptr, "fetch_elt");
         for (j = 0; j < draw->pt.nr_vertex_elements; ++j) {
            struct pipe_vertex_element *velem = &draw->pt.vertex_element[j];
            LLVMValueRef vb_index = LLVMConstInt(LLVMInt32Type(),
                                                 velem->vertex_buffer_index,
                                                 0);
            LLVMValueRef vb = LLVMBuildGEP(builder, vb_ptr,
                                           &vb_index, 1, "");
            generate_fetch(builder, vbuffers_ptr,
                           &aos_attribs[j][i], velem, vb, true_index,
                           instance_id);
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
                  sampler);

      /* store original positions in clip before further manipulation */
      store_clip(builder, io, outputs);

      /* do cliptest */
      if (enable_cliptest){
         /* allocate clipmask, assign it integer type */
         clipmask = generate_clipmask(builder, outputs,
                                      variant->key.clip_xy,
                                      variant->key.clip_z, 
                                      variant->key.clip_user,
                                      variant->key.clip_halfz,
                                      variant->key.nr_planes,
                                      context_ptr);
         /* return clipping boolean value for function */
         clipmask_bool(builder, clipmask, ret_ptr);
      }
      else{
         clipmask = lp_build_const_int_vec(lp_type_int_vec(32), 0);
      }
      
      /* do viewport mapping */
      if (!bypass_viewport){
         generate_viewport(llvm, builder, outputs, context_ptr);
      }

      /* store clipmask in vertex header, 
       * original positions in clip 
       * and transformed positions in data 
       */   
      convert_to_aos(builder, io, outputs, clipmask,
                     draw->vs.vertex_shader->info.num_outputs,
                     max_vertices);
   }

   lp_build_loop_end_cond(builder, fetch_count, step, LLVMIntUGE, &lp_loop);

   sampler->destroy(sampler);

#ifdef PIPE_ARCH_X86
   /* Avoid corrupting the FPU stack on 32bit OSes. */
   lp_build_intrinsic(builder, "llvm.x86.mmx.emms", LLVMVoidType(), NULL, 0);
#endif

   ret = LLVMBuildLoad(builder, ret_ptr,"");   
   LLVMBuildRet(builder, ret);
   
   LLVMDisposeBuilder(builder);

   /*
    * Translate the LLVM IR into machine code.
    */
#ifdef DEBUG
   if(LLVMVerifyFunction(variant->function_elts, LLVMPrintMessageAction)) {
      lp_debug_dump_value(variant->function_elts);
      assert(0);
   }
#endif

   LLVMRunFunctionPassManager(llvm->pass, variant->function_elts);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      lp_debug_dump_value(variant->function_elts);
      debug_printf("\n");
   }

   code = LLVMGetPointerToGlobal(llvm->draw->engine, variant->function_elts);
   variant->jit_func_elts = (draw_jit_vert_func_elts)pointer_to_func(code);

   if (gallivm_debug & GALLIVM_DEBUG_ASM) {
      lp_disassemble(code);
   }
   lp_func_delete_body(variant->function_elts);
}


struct draw_llvm_variant_key *
draw_llvm_make_variant_key(struct draw_llvm *llvm, char *store)
{
   unsigned i;
   struct draw_llvm_variant_key *key;
   struct lp_sampler_static_state *sampler;

   key = (struct draw_llvm_variant_key *)store;

   /* Presumably all variants of the shader should have the same
    * number of vertex elements - ie the number of shader inputs.
    */
   key->nr_vertex_elements = llvm->draw->pt.nr_vertex_elements;

   /* will have to rig this up properly later */
   key->clip_xy = llvm->draw->clip_xy;
   key->clip_z = llvm->draw->clip_z;
   key->clip_user = llvm->draw->clip_user;
   key->bypass_viewport = llvm->draw->identity_viewport;
   key->clip_halfz = !llvm->draw->rasterizer->gl_rasterization_rules;
   key->need_edgeflags = (llvm->draw->vs.edgeflag_output ? TRUE : FALSE);
   key->nr_planes = llvm->draw->nr_planes;
   key->pad = 0;

   /* All variants of this shader will have the same value for
    * nr_samplers.  Not yet trying to compact away holes in the
    * sampler array.
    */
   key->nr_samplers = llvm->draw->vs.vertex_shader->info.file_max[TGSI_FILE_SAMPLER] + 1;

   sampler = draw_llvm_variant_key_samplers(key);

   memcpy(key->vertex_element,
          llvm->draw->pt.vertex_element,
          sizeof(struct pipe_vertex_element) * key->nr_vertex_elements);
   
   memset(sampler, 0, key->nr_samplers * sizeof *sampler);

   for (i = 0 ; i < key->nr_samplers; i++) {
      lp_sampler_static_state(&sampler[i],
			      llvm->draw->sampler_views[i],
			      llvm->draw->samplers[i]);
   }

   return key;
}

void
draw_llvm_set_mapped_texture(struct draw_context *draw,
                             unsigned sampler_idx,
                             uint32_t width, uint32_t height, uint32_t depth,
                             uint32_t last_level,
                             uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                             uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                             const void *data[PIPE_MAX_TEXTURE_LEVELS])
{
   unsigned j;
   struct draw_jit_texture *jit_tex;

   assert(sampler_idx < PIPE_MAX_VERTEX_SAMPLERS);


   jit_tex = &draw->llvm->jit_context.textures[sampler_idx];

   jit_tex->width = width;
   jit_tex->height = height;
   jit_tex->depth = depth;
   jit_tex->last_level = last_level;

   for (j = 0; j <= last_level; j++) {
      jit_tex->data[j] = data[j];
      jit_tex->row_stride[j] = row_stride[j];
      jit_tex->img_stride[j] = img_stride[j];
   }
}


void
draw_llvm_set_sampler_state(struct draw_context *draw)
{
   unsigned i;

   for (i = 0; i < draw->num_samplers; i++) {
      struct draw_jit_texture *jit_tex = &draw->llvm->jit_context.textures[i];

      if (draw->samplers[i]) {
         jit_tex->min_lod = draw->samplers[i]->min_lod;
         jit_tex->max_lod = draw->samplers[i]->max_lod;
         jit_tex->lod_bias = draw->samplers[i]->lod_bias;
         COPY_4V(jit_tex->border_color, draw->samplers[i]->border_color);
      }
   }
}


void
draw_llvm_destroy_variant(struct draw_llvm_variant *variant)
{
   struct draw_llvm *llvm = variant->llvm;
   struct draw_context *draw = llvm->draw;

   if (variant->function_elts) {
      if (variant->function_elts)
         LLVMFreeMachineCodeForFunction(draw->engine,
                                        variant->function_elts);
      LLVMDeleteFunction(variant->function_elts);
   }

   if (variant->function) {
      if (variant->function)
         LLVMFreeMachineCodeForFunction(draw->engine,
                                        variant->function);
      LLVMDeleteFunction(variant->function);
   }

   remove_from_list(&variant->list_item_local);
   variant->shader->variants_cached--;
   remove_from_list(&variant->list_item_global);
   llvm->nr_variants--;
   FREE(variant);
}
